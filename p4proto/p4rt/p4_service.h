// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#ifndef P4SERVER_P4_SERVICE_H_
#define P4SERVER_P4_SERVICE_H_

#include <pthread.h>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <absl/base/thread_annotations.h>
#include <absl/numeric/int128.h>
#include <absl/synchronization/mutex.h>
#include <grpcpp/grpcpp.h>

#include "proto/p4/v1/p4runtime.grpc.pb.h"
#include "common/channel_writer_wrapper.h"
#include "common/forwarding_pipeline_configs.pb.h"
#include "utils/utils.h"

constexpr char kExternalP4serverUrls[] =
    "0.0.0.0:28000,0.0.0.0:9339,0.0.0.0:9559";
constexpr char kLocalP4serverUrl[] = "localhost:28000";

// Typedefs for more readable reference.
typedef ::grpc::ServerReaderWriter<::p4::v1::StreamMessageResponse,
                                   ::p4::v1::StreamMessageRequest>
    ServerStreamChannelReaderWriter;

// The "P4Service" class implements P4Runtime::Service. It handles all
// the RPCs that are part of the P4-based PI API.
class P4Service final : public ::p4::v1::P4Runtime::Service {
 public:
  // This class encapsulates the connection information for a connected
  // controller.
  class Controller {
   public:
    Controller()
        : connection_id_(0), election_id_(0), uri_(""), stream_(nullptr) {}
    Controller(uint64 connection_id, absl::uint128 election_id,
               const std::string& uri, ServerStreamChannelReaderWriter* stream)
        : connection_id_(connection_id),
          election_id_(election_id),
          uri_(uri),
          stream_(stream) {}
    // TODO(unknown): Done for unit testing. Find a better way.
    // stream_(ABSL_DIE_IF_NULL(stream)) {}
    uint64 connection_id() const { return connection_id_; }
    uint64 election_id_high() const {
      return absl::Uint128High64(election_id_);
    }
    uint64 election_id_low() const { return absl::Uint128Low64(election_id_); }
    absl::uint128 election_id() const { return election_id_; }
    std::string uri() const { return uri_; }
    ServerStreamChannelReaderWriter* stream() const { return stream_; }
    // A unique name string for the controller.
    std::string Name() const {
      std::stringstream ss;
      ss << "(connection_id: " << connection_id_
         << ", election_id: " << election_id_ << ", uri: " << uri_ << ")";
      return ss.str();
    }

   private:
    uint64 connection_id_;
    absl::uint128 election_id_;
    std::string uri_;
    ServerStreamChannelReaderWriter* stream_;  // not owned
  };

  // Custom comparator for Controller class.
  struct ControllerComp {
    bool operator()(const Controller& x, const Controller& y) const {
      // To make sure controller with the highest election_id is the 1st element
      return x.election_id() > y.election_id();
    }
  };

  P4Service();
  ~P4Service();

  // Sets up the service in coldboot and warmboot mode. In the coldboot mode,
  // the function initializes the class and pushes the saved forwarding pipeline
  // config to the switch. In the warmboot mode, it only restores the internal
  // state of the class.
  ::util::Status Setup() LOCKS_EXCLUDED(config_lock_);

  // Tears down the class. Called in both warmboot or coldboot mode. It will
  // not alter any state on the hardware when called.
  ::util::Status Teardown() LOCKS_EXCLUDED(config_lock_, controller_lock_,
                                           stream_response_thread_lock_);

  // Writes one or more forwarding entries on the target as part of P4 Runtime
  // API. Entries include tables entries, action profile members/groups, meter
  // entries, and counter entries.
  ::grpc::Status Write(::grpc::ServerContext* context,
                       const ::p4::v1::WriteRequest* req,
                       ::p4::v1::WriteResponse* resp) override;

  // Streams the forwarding entries, previously written on the target, out as
  // part of P4 Runtime API.
  ::grpc::Status Read(
      ::grpc::ServerContext* context, const ::p4::v1::ReadRequest* req,
      ::grpc::ServerWriter<::p4::v1::ReadResponse>* writer) override;

  // Pushes the P4-based forwarding pipeline configuration of one or more
  // switching nodes.
  ::grpc::Status SetForwardingPipelineConfig(
      ::grpc::ServerContext* context,
      const ::p4::v1::SetForwardingPipelineConfigRequest* req,
      ::p4::v1::SetForwardingPipelineConfigResponse* resp) override
      LOCKS_EXCLUDED(config_lock_);

  // Gets the P4-based forwarding pipeline configuration of one or more
  // switching nodes previously pushed to the switch.
  ::grpc::Status GetForwardingPipelineConfig(
      ::grpc::ServerContext* context,
      const ::p4::v1::GetForwardingPipelineConfigRequest* req,
      ::p4::v1::GetForwardingPipelineConfigResponse* resp) override
      LOCKS_EXCLUDED(config_lock_);

  // Bidirectional channel between controller and the switch for packet I/O,
  // master arbitration and stream errors.
  ::grpc::Status StreamChannel(
      ::grpc::ServerContext* context,
      ServerStreamChannelReaderWriter* stream) override;


  // Offers a mechanism through which a P4Runtime client can discover the
  // capabilities of the P4Runtime server implementation.
  ::grpc::Status Capabilities(
      ::grpc::ServerContext* context,
      const ::p4::v1::CapabilitiesRequest* request,
      ::p4::v1::CapabilitiesResponse* response) override;

  // P4Service is neither copyable nor movable.
  P4Service(const P4Service&) = delete;
  P4Service& operator=(const P4Service&) = delete;

 private:
  // ReaderArgs encapsulates the arguments for a Channel reader thread.
  template <typename T>
  struct ReaderArgs {
    P4Service* p4_service;
    std::unique_ptr<::p4server::ChannelReader<T>> reader;
    uint64 node_id;
  };

  // Specifies the max number of controllers that can connect for a node.
  static constexpr size_t kMaxNumControllerPerNode = 5;

  // Finds a new connection ID for a newly connected controller and adds it to
  // connection_ids_. Checks the number of active connections as well to make
  // sure we do not end with so many dangling threads.
  ::util::Status FindNewConnectionId(uint64 *conn_id)
      LOCKS_EXCLUDED(controller_lock_);

  // Adds a new controller to the controllers_ set. If the election_id in the
  // 'arbitration' token is highest among the existing controllers (or if this
  // is the first controller that is connected), this controller will become
  // master. This functions also returns the appropriate resp back to the
  // remote controller client(s), while it has the controller_lock_ lock. This
  // will make sure the response is sent back to the client (in case a packet
  // is received right at the same time) before StreamResponseReceiveHandler()
  // takes the lock. After successful completion of this function, the first
  // element in controllers_ set will have the master controller stream for
  // packet I/O.
  ::util::Status AddOrModifyController(uint64 node_id, uint64 connection_id,
                                       absl::uint128 election_id,
                                       const std::string& uri,
                                       ServerStreamChannelReaderWriter* stream)
      LOCKS_EXCLUDED(controller_lock_);

  // Removes an existing controller from the controllers_ set given its stream.
  // To be called after stream from an existing controller is broken (e.g.
  // controller is disconnected).
  void RemoveController(uint64 node_id, uint64 connection_id)
      LOCKS_EXCLUDED(controller_lock_);

  // Returns true if given (election_id, uri) for a Write request belongs to the
  // master controller stream for a node given by its node ID.
  bool IsWritePermitted(uint64 node_id, absl::uint128 election_id,
                        const std::string& uri) const
      LOCKS_EXCLUDED(controller_lock_);

  // Returns true if the given connection_id belongs to the master controller
  // stream for a node given by its node ID.
  bool IsMasterController(uint64 node_id, uint64 connection_id) const
      LOCKS_EXCLUDED(controller_lock_);

  // Thread function for handling stream response RX.
  static void* StreamResponseReceiveThreadFunc(void* arg)
      LOCKS_EXCLUDED(controller_lock_);

  // Blocks on the Channel registered with SwitchInterface to read received
  // responses.
  void* ReceiveStreamRespones(
      uint64 node_id,
      std::unique_ptr<::p4server::ChannelReader<::p4::v1::StreamMessageResponse>> reader)
      LOCKS_EXCLUDED(controller_lock_);

  // Callback to be called whenever we receive a stream response on the
  // specified node which is destined to controller.
  void StreamResponseReceiveHandler(uint64 node_id,
                                    const ::p4::v1::StreamMessageResponse& resp)
      LOCKS_EXCLUDED(controller_lock_);

  // Mutex lock used to protect node_id_to_controllers_ which is updated
  // every time mastership for any of the controllers connected to each node is
  // modified, or when a controller is diconnected.
  mutable absl::Mutex controller_lock_;

  // Mutex lock for protecting the internal forwarding pipeline configs pushed
  // to the switch.
  mutable absl::Mutex config_lock_;

  // Mutex which protects the creation and destruction of the stream response RX
  // Channels and threads.
  mutable absl::Mutex stream_response_thread_lock_;

  // Map from node ID to the set of Controller instances corresponding to the
  // external controller clients connected to that node. The Controller
  // instances for each node are sorted such that the master (Controller
  // with highest election_id) is the first element.
  std::map<uint64, std::set<Controller, ControllerComp>> node_id_to_controllers_
      GUARDED_BY(controller_lock_);

  // List of threads which send received responses up to the controller.
  std::vector<pthread_t> stream_response_reader_tids_
      GUARDED_BY(stream_response_thread_lock_);

  // Map of per-node Channels which are used to forward received responses to
  // P4Service.
  std::map<uint64, std::shared_ptr<::p4server::Channel<::p4::v1::StreamMessageResponse>>>
      stream_response_channels_ GUARDED_BY(stream_response_thread_lock_);

  // Holds the IDs of all streaming connections. Every time there is a new
  // streaming connection, we select min{1,...,max(connection_ids_) + 1} as
  // the ID of the new connection. Also, whenever the connection is dropped
  // we remove the connection ID from connection_ids_.
  std::set<uint64> connection_ids_ GUARDED_BY(controller_lock_);

  // Forwarding pipeline configs of all the switching nodes. Updated as we push
  // forwarding pipeline configs for new or existing nodes.
  std::unique_ptr<::p4server::hal::ForwardingPipelineConfigs> forwarding_pipeline_configs_
      GUARDED_BY(config_lock_);

  friend class P4ServiceTest;
};

#endif  // P4SERVER_P4_SERVICE_H_
