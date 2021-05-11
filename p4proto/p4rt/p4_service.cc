// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#include <functional>
#include <sstream>  // IWYU pragma: keep
#include <utility>
#include <iostream>

#include <absl/base/macros.h>
#include <absl/memory/memory.h>
#include <absl/numeric/int128.h>
#include <absl/strings/str_cat.h>
#include <absl/synchronization/mutex.h>
#include <absl/time/time.h>
#include <absl/strings/str_join.h>
#include <absl/strings/str_split.h>
#include <absl/status/status.h>
#include <gflags/gflags.h>
#include <google/protobuf/any.pb.h>

#include "p4_service.h"
#include "p4_service_interface.h"
#include "google/rpc/code.pb.h"
#include "google/rpc/status.pb.h"
#include "common/server_writer_wrapper.h"
#include "common/channel.h"
#include "utils/cleanup.h"
#include "utils/logging.h"
#include "utils/status_macros.h"
#include "utils/utils.h"
#include "utils/error.pb.h"

#if 0
using namespace ::stratum::barefoot;
extern BfInterface* bfIntf;
#endif //bfIntf

DEFINE_string(forwarding_pipeline_configs_file,
              "/var/run/p4server/pipeline_cfg.pb.txt",
              "The latest set of verified ForwardingPipelineConfig protos "
              "pushed to the switch. This file is updated whenever "
              "ForwardingPipelineConfig proto for switching node is added or "
              "modified.");
DEFINE_int32(max_num_controllers_per_node, 5,
             "Max number of controllers that can manage a node.");
DEFINE_int32(max_num_controller_connections, 20,
             "Max number of active/inactive streaming connections from outside "
             "controllers (for all of the nodes combined).");

using namespace std;

P4Service::P4Service() {return;}

P4Service::~P4Service() {}

::util::Status P4Service::Setup() {
    return ::util::OkStatus();
}

::util::Status P4Service::Teardown() {
  {
    absl::WriterMutexLock l(&controller_lock_);
    node_id_to_controllers_.clear();
    connection_ids_.clear();
  }
  {
    absl::WriterMutexLock l(&stream_response_thread_lock_);
    // Unregister writers and close PacketIn Channels.
    for (const auto& pair : stream_response_channels_) {
      ::util::Status status = ::util::OkStatus();
      LOG(INFO) << "UnregisterStreamMessageResponseWriter";
      if (!status.ok()) {
        LOG(ERROR) << status;
      }
      pair.second->Close();
    }
    stream_response_channels_.clear();
    // Join threads.
    for (const auto& tid : stream_response_reader_tids_) {
      int ret = pthread_join(tid, nullptr);
      if (ret) {
        LOG(ERROR) << "Failed to join thread " << tid << " with error " << ret
                   << ".";
      }
    }
  }
  {
    absl::WriterMutexLock l(&config_lock_);
    forwarding_pipeline_configs_ = nullptr;
  }

  return ::util::OkStatus();
}

::grpc::Status ToGrpcStatus(const ::absl::Status& status) {
  // We need to create a ::google::rpc::Status and populate it with all the
  // details, then convert it to ::grpc::Status.
  ::google::rpc::Status from;
  if (!status.ok()) {
    from.set_code(p4server::ToGoogleRpcCode(status.raw_code()));
    //from.set_message(status.message());
    from.set_message(::absl::StatusCodeToString(status.code()));
  } else {
    from.set_code(::google::rpc::OK);
  }

  return ::grpc::Status(p4server::ToGrpcCode(from.code()), from.message(),
                        from.SerializeAsString());
}

// Helper function to generate a StreamMessageResponse from a failed Status.
::p4::v1::StreamMessageResponse ToStreamMessageResponse(
    const ::util::Status& status) {
  CHECK(!status.ok());
  ::p4::v1::StreamMessageResponse resp;
  auto stream_error = resp.mutable_error();
  stream_error->set_canonical_code(
                p4server::ToGoogleRpcCode(status.CanonicalCode()));
  stream_error->set_message(status.error_message());
  stream_error->set_code(status.error_code());

  return resp;
}

::grpc::Status P4Service::Write(::grpc::ServerContext* context,
                                const ::p4::v1::WriteRequest* req,
                                ::p4::v1::WriteResponse* resp) {

  if (!req->updates_size()) return ::grpc::Status::OK;  // Nothing to do.

  // device_id is nothing but the node_id specified in the config for the node.
  uint64 node_id = req->device_id();
  if (node_id == 0) {
    return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                          "Invalid device ID.");
  }

  // Require valid election_id for Write.
  absl::uint128 election_id =
      absl::MakeUint128(req->election_id().high(), req->election_id().low());
  if (election_id == 0) {
    return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                          "Invalid election ID.");
  }

  // Make sure this node already has a master controller and the given
  // election_id and the uri of the client matches those of the master.
  if (!IsWritePermitted(node_id, election_id, context->peer())) {
    return ::grpc::Status(::grpc::StatusCode::PERMISSION_DENIED,
                          "Write from non-master is not permitted.");
  }

  std::vector<::util::Status> results = {};

  //::absl::Status status = bfIntf->Write(*req, resp);
  ::absl::Status status = absl::OkStatus();
  LOG(INFO) << "P4 server: WriteForwardingEntries executed successfully";

  if (!status.ok()) {
    LOG(ERROR) << "Failed to write forwarding entries to node " << node_id;
    for (const auto& result : results) {
      LOG(ERROR) << result;
    }
  }

  return ToGrpcStatus(status);
}

::grpc::Status P4Service::Read(
    ::grpc::ServerContext* context, const ::p4::v1::ReadRequest* req,
    ::grpc::ServerWriter<::p4::v1::ReadResponse>* writer) {

  if (!req->entities_size()) return ::grpc::Status::OK;
  if (req->device_id() == 0) {
    return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                          "Invalid device ID.");
  }

  ServerWriterWrapper<::p4::v1::ReadResponse> wrapper(writer);
  std::vector<::util::Status> details = {};
  //TODO
  //::absl::Status status = bfIntf->Read(*req, writer);
  ::absl::Status status = absl::OkStatus();
  LOG(INFO) << "P4 server: ReadForwardingEntries executed successfully";
  if (!status.ok()) {
    LOG(ERROR) << "Failed to read forwarding entries from node "
               << req->device_id() << ": "
               << ::absl::StatusCodeToString(status.code());
  }

  return ToGrpcStatus(status);
}

::grpc::Status P4Service::SetForwardingPipelineConfig(
    ::grpc::ServerContext* context,
    const ::p4::v1::SetForwardingPipelineConfigRequest* req,
    ::p4::v1::SetForwardingPipelineConfigResponse* resp) {

  // device_id is nothing but the node_id specified in the config for the node.
  uint64 node_id = req->device_id();
  if (node_id == 0) {
    return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                          "Invalid device ID.");
  }

  // We need valid election ID for SetForwardingPipelineConfig RPC
  absl::uint128 election_id =
      absl::MakeUint128(req->election_id().high(), req->election_id().low());
  if (election_id == 0) {
    return ::grpc::Status(
        ::grpc::StatusCode::INVALID_ARGUMENT,
        absl::StrCat("Invalid election ID for node ", node_id, "."));
  }
  // Make sure this node already has a master controller and the given
  // election_id and the uri of the client matches those of the
  // master. According to the P4Runtime specification, only master can perform
  // SetForwardingPipelineConfig RPC.
  if (!IsWritePermitted(node_id, election_id, context->peer())) {
    return ::grpc::Status(
        ::grpc::StatusCode::PERMISSION_DENIED,
        absl::StrCat("SetForwardingPipelineConfig from non-master is not "
                     "permitted for node ",
                     node_id, "."));
  }

  ::util::Status status = ::util::OkStatus();
  switch (req->action()) {
    case ::p4::v1::SetForwardingPipelineConfigRequest::VERIFY:
      LOG(INFO) << "P4 server: bfIntf->VerifyForwardingPipelineConfig";
      break;
    case ::p4::v1::SetForwardingPipelineConfigRequest::VERIFY_AND_COMMIT:
    case ::p4::v1::SetForwardingPipelineConfigRequest::VERIFY_AND_SAVE: {
      absl::WriterMutexLock l(&config_lock_);
      // configs_to_save_in_file will have a copy of the configs that will be
      // saved in file at the end. Note that this copy may NOT be the same as
      // forwarding_pipeline_configs_.
      ::p4server::hal::ForwardingPipelineConfigs configs_to_save_in_file;
      if (forwarding_pipeline_configs_ != nullptr) {
        configs_to_save_in_file = *forwarding_pipeline_configs_;
      } else {
        forwarding_pipeline_configs_ =
            absl::make_unique<::p4server::hal::ForwardingPipelineConfigs>();
      }
      //::util::Status error = ::util::OkStatus();
      ::util::Status error ;
      if (req->action() ==
          ::p4::v1::SetForwardingPipelineConfigRequest::VERIFY_AND_COMMIT) {
          LOG(INFO) << "P4 server: bfIntf->PushForwardingPipelineConfig";
          ::absl::Status status = absl::OkStatus();
          //status = bfIntf->SetForwardingPipelineConfig(*req, resp);
      } else {  // VERIFY_AND_SAVE
          LOG(INFO) << "P4 server: bfIntf->SaveForwardingPipelineConfig";
      }

      if (error.ok() || error.error_code() == ::p4server::ERR_REBOOT_REQUIRED) {
        (*configs_to_save_in_file.mutable_node_id_to_config())[node_id] =
            req->config();

        error = ::p4server::WriteProtoToTextFile(configs_to_save_in_file,
                                 FLAGS_forwarding_pipeline_configs_file);
      }
      if (error.ok()) {
        (*forwarding_pipeline_configs_->mutable_node_id_to_config())[node_id] =
            req->config();
      }
      break;
    }
    case ::p4::v1::SetForwardingPipelineConfigRequest::COMMIT: {
      ::util::Status error = ::util::OkStatus();
          LOG(INFO) << "P4 server: bfIntf->CommitForwardingPipelineConfig";
      break;
    }
    case ::p4::v1::SetForwardingPipelineConfigRequest::RECONCILE_AND_COMMIT:
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED,
                            "RECONCILE_AND_COMMIT action not supported yet");
    default:
      return ::grpc::Status(
          ::grpc::StatusCode::INVALID_ARGUMENT,
          absl::StrCat("Invalid action passed for node ", node_id, "."));
  }

  if (!status.ok()) {
    LOG(ERROR) << "Failed to set forwarding pipeline config for node "
              << node_id << ".";
    return ::grpc::Status(p4server::ToGrpcCode(status.CanonicalCode()),
                          status.error_message());
  }

  return ::grpc::Status::OK;
}

::grpc::Status P4Service::GetForwardingPipelineConfig(
    ::grpc::ServerContext* context,
    const ::p4::v1::GetForwardingPipelineConfigRequest* req,
    ::p4::v1::GetForwardingPipelineConfigResponse* resp) {

  // device_id is nothing but the node_id specified in the config for the node.
  uint64 node_id = req->device_id();
  if (node_id == 0) {
    return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                          "Invalid device ID.");
  }

  absl::ReaderMutexLock l(&config_lock_);
  if (forwarding_pipeline_configs_ == nullptr ||
      forwarding_pipeline_configs_->node_id_to_config_size() == 0) {
    LOG(ERROR) << "P4 server: No valid forwarding pipeline config has been pushed.";
    return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          "No valid forwarding pipeline config has been pushed "
                          "for any node so far.");
  }
  auto it = forwarding_pipeline_configs_->node_id_to_config().find(node_id);
  if (it == forwarding_pipeline_configs_->node_id_to_config().end()) {
    return ::grpc::Status(::grpc::StatusCode::FAILED_PRECONDITION,
                          absl::StrCat("Invalid node id or no valid forwarding "
                                       "pipeline config has been pushed for "
                                       "node ",
                                       node_id, " yet."));
  }

  LOG(INFO) << "GetForwardingPipelineConfig";
  switch (req->response_type()) {
    case p4::v1::GetForwardingPipelineConfigRequest::ALL: {
      *resp->mutable_config() = it->second;
      break;
    }
    case p4::v1::GetForwardingPipelineConfigRequest::COOKIE_ONLY: {
      *resp->mutable_config()->mutable_cookie() = it->second.cookie();
      break;
    }
    case p4::v1::GetForwardingPipelineConfigRequest::P4INFO_AND_COOKIE: {
      *resp->mutable_config()->mutable_p4info() = it->second.p4info();
      *resp->mutable_config()->mutable_cookie() = it->second.cookie();
      break;
    }
    case p4::v1::GetForwardingPipelineConfigRequest::DEVICE_CONFIG_AND_COOKIE: {
      *resp->mutable_config()->mutable_p4_device_config() =
          it->second.p4_device_config();
      *resp->mutable_config()->mutable_cookie() = it->second.cookie();
      break;
    }
    default:
      return ::grpc::Status(
          ::grpc::StatusCode::INVALID_ARGUMENT,
          absl::StrCat("Invalid action passed for node ", node_id, "."));
  }

  return ::grpc::Status::OK;
}

::grpc::Status P4Service::StreamChannel(
    ::grpc::ServerContext* context, ServerStreamChannelReaderWriter* stream) {

  // Here are the rules:
  // 1- When a client (aka controller) connects for the first time, we do not do
  //    anything until a MasterArbitrationUpdate proto is received.
  // 2- After MasterArbitrationUpdate is received at any time (we can receive
  //    this many time), the controller becomes/stays master or slave.
  // 3- At any point of time, only the master stream is capable of sending
  //    and receiving packets.

  // First thing to do is to find a new ID for this connection.
  uint64 connection_id = 0;
  auto status = FindNewConnectionId(&connection_id);
  if (!status.ok()) {
    return ::grpc::Status(p4server::ToGrpcCode(status.CanonicalCode()),
                           status.error_message());
  }

  // The ID of the node this stream channel corresponds to. This is MUST NOT
  // change after it is set for the first time.
  uint64 node_id = 0;

  // The cleanup object. Will call RemoveController() upon exit.
  auto cleaner = ::p4server::gtl::MakeCleanup([this, &node_id, &connection_id]() {
    this->RemoveController(node_id, connection_id);
  });

  ::p4::v1::StreamMessageRequest req;
  while (stream->Read(&req)) {
    switch (req.update_case()) {
      case ::p4::v1::StreamMessageRequest::kArbitration: {
        if (req.arbitration().device_id() == 0) {
          return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                                "Invalid node (aka device) ID.");
        } else if (node_id == 0) {
          node_id = req.arbitration().device_id();
        } else if (node_id != req.arbitration().device_id()) {
          std::stringstream ss;
          ss << "Node (aka device) ID for this stream has changed. Was "
             << node_id << ", now is " << req.arbitration().device_id() << ".";
          return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, ss.str());
        }
        absl::uint128 election_id =
            absl::MakeUint128(req.arbitration().election_id().high(),
                              req.arbitration().election_id().low());
        if (election_id == 0) {
          return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                                "Invalid election ID.");
        }
        // Try to add the controller to controllers_.
        auto status = AddOrModifyController(node_id, connection_id, election_id,
                                            context->peer(), stream);
        if (!status.ok()) {
          return ::grpc::Status(p4server::ToGrpcCode(status.CanonicalCode()),
                                status.error_message());
        }
        break;
      }
      case ::p4::v1::StreamMessageRequest::kPacket: {
        // If this stream is not the master stream generate a stream error.
        ::util::Status status = ::util::OkStatus();
        if (!IsMasterController(node_id, connection_id)) {
          status = MAKE_ERROR(::p4server::ERR_PERMISSION_DENIED)
                   << "Controller with connection ID " << connection_id
                   << "is not a master";
        } else {
          // If master, try to transmit the packet.
          LOG(INFO) << "P4 server: bfIntf->HandleStreamMessageRequest";
        }
        if (!status.ok()) {
          LOG_EVERY_N(INFO, 500) << "Failed to transmit packet: " << status;
          auto resp = ToStreamMessageResponse(status);
          *resp.mutable_error()->mutable_packet_out()->mutable_packet_out() =
              req.packet();
          stream->Write(resp);  // Best effort.
        }
        break;
      }
      case ::p4::v1::StreamMessageRequest::kDigestAck: {
        // If this stream is not the master stream generate a stream error.
        ::util::Status status = ::util::OkStatus();
        if (!IsMasterController(node_id, connection_id)) {
          status = MAKE_ERROR(::p4server::ERR_PERMISSION_DENIED)
                   << "Controller with connection ID " << connection_id
                   << "is not a master";
        } else {
          // If master, try to ack the digest.
          LOG(INFO) << "P4 server: bfIntf->HandleStreamMessageRequest";
        }
        if (!status.ok()) {
          LOG(INFO) << "Failed to ack digest: " << status;
          // TODO(max): investigate if creating responses for every failure is
          // too resource intensive.
          auto resp = ToStreamMessageResponse(status);
          *resp.mutable_error()
               ->mutable_digest_list_ack()
               ->mutable_digest_list_ack() = req.digest_ack();
          stream->Write(resp);  // Best effort.
        }
        break;
      }
      case ::p4::v1::StreamMessageRequest::UPDATE_NOT_SET:
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT,
                              "Need to specify either arbitration or packet.");
    }
  }

  return ::grpc::Status::OK;
}

::grpc::Status P4Service::Capabilities(
    ::grpc::ServerContext* context,
    const ::p4::v1::CapabilitiesRequest* request,
    ::p4::v1::CapabilitiesResponse* response) {
  response->set_p4runtime_api_version(STRINGIFY(P4RUNTIME_VER));
  return ::grpc::Status::OK;
}

::util::Status P4Service::FindNewConnectionId(uint64 *conn_id) {
  absl::WriterMutexLock l(&controller_lock_);
  if (static_cast<int>(connection_ids_.size()) >=
      FLAGS_max_num_controller_connections) {
    return MAKE_ERROR(::p4server::ERR_NO_RESOURCE)
           << "Can have max " << FLAGS_max_num_controller_connections
           << " active/inactive streams for all the node.";
  }
  uint64 max_connection_id =
      connection_ids_.empty() ? 0 : *connection_ids_.end();
  for (uint64 i = 1; i <= max_connection_id + 1; ++i) {
    if (!connection_ids_.count(i)) {
      connection_ids_.insert(i);
      *conn_id = i;
      return ::util::OkStatus();
    }
  }
  return MAKE_ERROR(::p4server::ERR_TABLE_FULL)
           << "Connection list id full.";
}

::util::Status P4Service::AddOrModifyController(
    uint64 node_id, uint64 connection_id, absl::uint128 election_id,
    const std::string& uri, ServerStreamChannelReaderWriter* stream) {
  // To be called by all the threads handling controller connections.
  absl::WriterMutexLock l(&controller_lock_);
  auto it = node_id_to_controllers_.find(node_id);
  if (it == node_id_to_controllers_.end()) {
    absl::WriterMutexLock l(&stream_response_thread_lock_);
    // This is the first time we are hearing about this node. Lets try to add
    // an RX response writer for it. If the node_id is invalid, registration
    // will fail.
    std::shared_ptr<::p4server::Channel<::p4::v1::StreamMessageResponse>> channel =
        ::p4server::Channel<::p4::v1::StreamMessageResponse>::Create(128);
    // Create the writer and register with the SwitchInterface.
    auto writer =
        std::make_shared<::p4server::hal::ChannelWriterWrapper<::p4::v1::StreamMessageResponse>>(
            ::p4server::ChannelWriter<::p4::v1::StreamMessageResponse>::Create(channel));

    LOG(INFO) << "P4 server: bfIntf->RegisterStreamMessageResponseWriter";

    // Create the reader and pass it to a new thread.
    auto reader =
        ::p4server::ChannelReader<::p4::v1::StreamMessageResponse>::Create(channel);
    pthread_t tid = 0;

    int ret = pthread_create(&tid, nullptr, StreamResponseReceiveThreadFunc,
                             new ReaderArgs<::p4::v1::StreamMessageResponse>{
                             this, std::move(reader), node_id});

    if (ret) {
      // Clean up state and return error.
      LOG(INFO) << "P4 server: bfIntf->UnregisterStreamMessageResponseWriter";
      return MAKE_ERROR(::p4server::ERR_INTERNAL)
             << "Failed to create packet-in receiver thread for node "
             << node_id << " with error " << ret << ".";
    }
    // Store Channel and tid for Teardown().
    stream_response_reader_tids_.push_back(tid);
    stream_response_channels_[node_id] = channel;
    node_id_to_controllers_[node_id] = {};
    it = node_id_to_controllers_.find(node_id);
  }

  // Need to see if this controller was master before we process this new
  // request.
  bool was_master = (!it->second.empty() &&
                     connection_id == it->second.begin()->connection_id());

  // Need to check we do not go beyond the max number of connections per node.
  if (static_cast<int>(it->second.size()) >=
      FLAGS_max_num_controllers_per_node) {
    return MAKE_ERROR(::p4server::ERR_NO_RESOURCE)
           << "Cannot have more than " << FLAGS_max_num_controllers_per_node
           << " controllers for node (aka device) with ID " << node_id << ".";
  }

  // Next see if this is a new controller for this node, or this is an existing
  // one. If there exist a controller with this connection_id remove it first.
  auto cont = std::find_if(
      it->second.begin(), it->second.end(),
      [=](const Controller& c) { return c.connection_id() == connection_id; });
  if (cont != it->second.end()) {
    it->second.erase(cont);
  }

  // Now add the controller to the set of controllers for this node. The add
  // will possibly lead to a new master.
  Controller controller(connection_id, election_id, uri, stream);
  it->second.insert(controller);

  // Find the most updated master. Also find out if this controller is master
  // after this new Controller instance was inserted.
  auto master = it->second.begin();  // points to master
  bool is_master = (election_id == master->election_id());

  // Now we need to do the following:
  // - If this new controller is master (no matter if it was a master before
  //   or not), we need to send its election_id to all connected controllers
  //   for this node. The arbitration token sent back to all the connected
  //   controllers will have OK status for the master and non-OK for slaves.
  // - The controller was master but it is not master now, this means a master
  //   change. We need to notify all connected controllers in this case as well.
  // - If this new controller is not master now and it was not master before,
  //   we just need to send the arbitration token with non-OK status to this
  //   controller.
  ::p4::v1::StreamMessageResponse resp;
  resp.mutable_arbitration()->set_device_id(node_id);
  resp.mutable_arbitration()->mutable_election_id()->set_high(
      master->election_id_high());
  resp.mutable_arbitration()->mutable_election_id()->set_low(
      master->election_id_low());
  if (is_master || was_master) {
    resp.mutable_arbitration()->mutable_status()->set_code(::google::rpc::OK);
    for (const auto& c : it->second) {
      if (!c.stream()->Write(resp)) {
        return MAKE_ERROR(::p4server::ERR_INTERNAL)
               << "Failed to write to a stream for node " << node_id << ".";
      }
      // For non masters.
      resp.mutable_arbitration()->mutable_status()->set_code(
          ::google::rpc::ALREADY_EXISTS);
      resp.mutable_arbitration()->mutable_status()->set_message(
          "You are not my master!");
    }
  } else {
    resp.mutable_arbitration()->mutable_status()->set_code(
        ::google::rpc::ALREADY_EXISTS);
    resp.mutable_arbitration()->mutable_status()->set_message(
        "You are not my master!");
    if (!stream->Write(resp)) {
      return MAKE_ERROR(::p4server::ERR_INTERNAL)
             << "Failed to write to a stream for node " << node_id << ".";
    }
  }

  LOG(INFO) << "Controller " << controller.Name() << " is connected as "
            << (is_master ? "MASTER" : "SLAVE")
            << " for node (aka device) with ID " << node_id << ".";

  return ::util::OkStatus();
}

void P4Service::RemoveController(uint64 node_id, uint64 connection_id) {
  absl::WriterMutexLock l(&controller_lock_);
  connection_ids_.erase(connection_id);
  auto it = node_id_to_controllers_.find(node_id);
  if (it == node_id_to_controllers_.end()) return;
  auto controller = std::find_if(
      it->second.begin(), it->second.end(),
      [=](const Controller& c) { return c.connection_id() == connection_id; });
  if (controller != it->second.end()) {
    // Need to see if we are removing a master. Removing a master means
    // mastership change.
    bool is_master =
        controller->connection_id() == it->second.begin()->connection_id();
    // Get the name of the controller before removing it for logging purposes.
    std::string name = controller->Name();
    it->second.erase(controller);
    // Log the transition. Very useful for debugging. Also if there was a change
    // in mastership, let all other controller know.
    if (is_master) {
      if (it->second.empty()) {
        LOG(INFO) << "Controller " << name << " which was MASTER for node "
                  << "(aka device) with ID " << node_id
                  << " is disconnected. The node is "
                  << "now orphan :(";
      } else {
        LOG(INFO) << "Controller " << name << " which was MASTER for node "
                  << "(aka device) with ID " << node_id
                  << " is disconnected. New master is "
                  << it->second.begin()->Name();
        // We need to let all the connected controller know about this
        // mastership change.
        ::p4::v1::StreamMessageResponse resp;
        resp.mutable_arbitration()->set_device_id(node_id);
        resp.mutable_arbitration()->mutable_election_id()->set_high(
            it->second.begin()->election_id_high());
        resp.mutable_arbitration()->mutable_election_id()->set_low(
            it->second.begin()->election_id_low());
        resp.mutable_arbitration()->mutable_status()->set_code(
            ::google::rpc::OK);
        for (const auto& c : it->second) {
          c.stream()->Write(resp);  // Best effort.
          // For non masters.
          resp.mutable_arbitration()->mutable_status()->set_code(
              ::google::rpc::ALREADY_EXISTS);
          resp.mutable_arbitration()->mutable_status()->set_message(
              "You are not my master!");
        }
      }
    } else {
      if (it->second.empty()) {
        LOG(INFO) << "Controller " << name << " which was SLAVE for node "
                  << "(aka device) with ID " << node_id
                  << " is disconnected. The node is now orphan :(";
      } else {
        LOG(INFO) << "Controller " << name << " which was SLAVE for node "
                  << "(aka device) with ID " << node_id << " is disconnected.";
      }
    }
  }
}

bool P4Service::IsWritePermitted(uint64 node_id, absl::uint128 election_id,
                                 const std::string& uri) const {
  absl::ReaderMutexLock l(&controller_lock_);
  auto it = node_id_to_controllers_.find(node_id);
  if (it == node_id_to_controllers_.end() || it->second.empty()) return false;
  // TODO(unknown): Find a way to check for uri as well.
  return it->second.begin()->election_id() == election_id;
}

bool P4Service::IsMasterController(uint64 node_id, uint64 connection_id) const {
  absl::ReaderMutexLock l(&controller_lock_);
  auto it = node_id_to_controllers_.find(node_id);
  if (it == node_id_to_controllers_.end() || it->second.empty()) return false;
  return it->second.begin()->connection_id() == connection_id;
}

void* P4Service::StreamResponseReceiveThreadFunc(void* arg) {
  auto* args =
      reinterpret_cast<ReaderArgs<::p4::v1::StreamMessageResponse>*>(arg);
  auto* p4_service = args->p4_service;
  auto node_id = args->node_id;
  auto reader = std::move(args->reader);
  delete args;
  return p4_service->ReceiveStreamRespones(node_id, std::move(reader));
}

void* P4Service::ReceiveStreamRespones(
    uint64 node_id,
    std::unique_ptr<::p4server::ChannelReader<::p4::v1::StreamMessageResponse>> reader) {
  do {
    ::p4::v1::StreamMessageResponse resp;
    // Block on next stream response RX from Channel.
    int code = reader->Read(&resp, absl::InfiniteDuration()).error_code();
    // Exit if the Channel is closed.
    if (code == ::p4server::ERR_CANCELLED) break;
    // Read should never timeout.
    if (code == ::p4server::ERR_ENTRY_NOT_FOUND) {
      LOG(ERROR) << "Read with infinite timeout failed with ENTRY_NOT_FOUND.";
      continue;
    }
    // Handle StreamMessageResponse.
    StreamResponseReceiveHandler(node_id, resp);
  } while (true);
  return nullptr;
}

void P4Service::StreamResponseReceiveHandler(
    uint64 node_id, const ::p4::v1::StreamMessageResponse& resp) {
  // We don't expect arbitration updates from the switch.
  if (resp.has_arbitration()) {
    LOG(FATAL) << "Received MasterArbitrationUpdate from switch. This should "
                  "never happen!";
  }
  // We send the responses only to the master controller stream for this node.
  absl::ReaderMutexLock l(&controller_lock_);
  auto it = node_id_to_controllers_.find(node_id);
  if (it == node_id_to_controllers_.end() || it->second.empty()) return;
  it->second.begin()->stream()->Write(resp);
}
