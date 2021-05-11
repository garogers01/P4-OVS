// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#ifndef P4SERVER_UTILS_UTILS_H_
#define P4SERVER_UTILS_UTILS_H_

#include <libgen.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <chrono>  // NOLINT
#include <functional>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/message_differencer.h>
#include <grpcpp/grpcpp.h>

#include "google/rpc/code.pb.h"
#include "google/rpc/status.pb.h"
#include "integral_types.h"
#include "status.h"

namespace p4server {

// Writes a proto message in binary format to the given file path.
::util::Status WriteProtoToBinFile(const ::google::protobuf::Message& message,
                                   const std::string& filename);

// Reads proto from a file containing the proto message in binary format.
::util::Status ReadProtoFromBinFile(const std::string& filename,
                                    ::google::protobuf::Message* message);

// Writes a proto message in text format to the given file path.
::util::Status WriteProtoToTextFile(const ::google::protobuf::Message& message,
                                    const std::string& filename);

// Reads proto from a text file containing the proto message in text format.
::util::Status ReadProtoFromTextFile(const std::string& filename,
                                     ::google::protobuf::Message* message);

// Serializes proto to a string. Wrapper around TextFormat::PrintToString().
::util::Status PrintProtoToString(const ::google::protobuf::Message& message,
                                  std::string* text);

// Parses a proto from a string. Wrapper around TextFormat::ParseFromString().
::util::Status ParseProtoFromString(const std::string& text,
                                    ::google::protobuf::Message* message);

// Writes a string buffer to a text file. 'append' (default false) specifies
// whether the string need to appended to the end of the file as opposed to
// truncating the file contents. The default is false.
::util::Status WriteStringToFile(const std::string& buffer,
                                 const std::string& filename,
                                 bool append = false);

// Reads the contents of a file to a string buffer.
::util::Status ReadFileToString(const std::string& filename,
                                std::string* buffer);

// Checks to see if a path exists.
inline bool PathExists(const std::string& path) {
  struct stat stbuf;
  return (stat(path.c_str(), &stbuf) >= 0);
}

// Checks to see if a path is a dir.
inline bool IsDir(const std::string& path) {
  struct stat stbuf;
  if (stat(path.c_str(), &stbuf) < 0) {
    return false;
  }
  return S_ISDIR(stbuf.st_mode);
}

// Helper for converting an int error code to a GRPC canonical error code.
constexpr ::grpc::StatusCode kGrpcCodeMin = ::grpc::StatusCode::OK;
constexpr ::grpc::StatusCode kGrpcCodeMax = ::grpc::StatusCode::UNAUTHENTICATED;
inline ::grpc::StatusCode ToGrpcCode(int from) {
  ::grpc::StatusCode code = ::grpc::StatusCode::UNKNOWN;
  if (from >= kGrpcCodeMin && from <= kGrpcCodeMax) {
    code = static_cast<::grpc::StatusCode>(from);
  }
  return code;
}

// Helper for converting an int error code to a Google RPC canonical error code.
constexpr ::google::rpc::Code kGoogleRpcCodeMin = ::google::rpc::OK;
constexpr ::google::rpc::Code kGoogleRpcCodeMax =
    ::google::rpc::UNAUTHENTICATED;
inline ::google::rpc::Code ToGoogleRpcCode(int from) {
  ::google::rpc::Code code = ::google::rpc::UNKNOWN;
  if (from >= kGoogleRpcCodeMin && from <= kGoogleRpcCodeMax) {
    code = static_cast<::google::rpc::Code>(from);
  }
  return code;
}

}  // namespace p4server

#endif  // P4SERVER_UTILS_UTILS_H_
