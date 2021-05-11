// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#include <cxxabi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>  // IWYU pragma: keep
#include <string>

#include <absl/strings/str_split.h>
#include <absl/strings/substitute.h>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/util/message_differencer.h>

#include "utils.h"
#include "macros.h"
#include "error.h"

namespace p4server {

using ::google::protobuf::util::MessageDifferencer;

::util::Status WriteProtoToBinFile(const ::google::protobuf::Message& message,
                                   const std::string& filename) {
  std::string buffer;
  if (!message.SerializeToString(&buffer)) {
    return MAKE_ERROR(ERR_INVALID_PARAM)
           << "Failed to convert proto to bin string buffer: "
           << message.ShortDebugString();
  }
  RETURN_IF_ERROR(WriteStringToFile(buffer, filename));

  return ::util::OkStatus();
}

::util::Status ReadProtoFromBinFile(const std::string& filename,
                                    ::google::protobuf::Message* message) {
  std::string buffer;
  RETURN_IF_ERROR(ReadFileToString(filename, &buffer));
  if (!message->ParseFromString(buffer)) {
    return MAKE_ERROR(ERR_INTERNAL) << "Failed to parse the binary content of "
                                    << filename << " to proto.";
  }

  return ::util::OkStatus();
}

::util::Status WriteProtoToTextFile(const ::google::protobuf::Message& message,
                                    const std::string& filename) {
  std::string text;
  RETURN_IF_ERROR(PrintProtoToString(message, &text));
  RETURN_IF_ERROR(WriteStringToFile(text, filename));

  return ::util::OkStatus();
}

::util::Status ReadProtoFromTextFile(const std::string& filename,
                                     ::google::protobuf::Message* message) {
  std::string text;
  RETURN_IF_ERROR(ReadFileToString(filename, &text));
  RETURN_IF_ERROR(ParseProtoFromString(text, message));

  return ::util::OkStatus();
}

::util::Status PrintProtoToString(const ::google::protobuf::Message& message,
                                  std::string* text) {
  if (!::google::protobuf::TextFormat::PrintToString(message, text)) {
    return MAKE_ERROR(ERR_INTERNAL)
           << "Failed to print proto to string: " << message.ShortDebugString();
  }

  return ::util::OkStatus();
}

::util::Status ParseProtoFromString(const std::string& text,
                                    ::google::protobuf::Message* message) {
  if (!::google::protobuf::TextFormat::ParseFromString(text, message)) {
    return MAKE_ERROR(ERR_INTERNAL)
           << "Failed to parse proto from the following string: " << text;
  }

  return ::util::OkStatus();
}

::util::Status WriteStringToFile(const std::string& buffer,
                                 const std::string& filename, bool append) {
  std::ofstream outfile;
  outfile.open(filename.c_str(), append
                                     ? std::ofstream::out | std::ofstream::app
                                     : std::ofstream::out);
  if (!outfile.is_open()) {
    return MAKE_ERROR(ERR_INTERNAL) << "Error when opening " << filename << ".";
  }
  outfile << buffer;
  outfile.close();

  return ::util::OkStatus();
}

::util::Status ReadFileToString(const std::string& filename,
                                std::string* buffer) {
  if (!PathExists(filename)) {
    return MAKE_ERROR(ERR_FILE_NOT_FOUND) << filename << " not found.";
  }
  if (IsDir(filename)) {
    return MAKE_ERROR(ERR_FILE_NOT_FOUND) << filename << " is a dir.";
  }

  std::ifstream infile;
  infile.open(filename.c_str());
  if (!infile.is_open()) {
    return MAKE_ERROR(ERR_INTERNAL) << "Error when opening " << filename << ".";
  }

  std::string contents((std::istreambuf_iterator<char>(infile)),
                       (std::istreambuf_iterator<char>()));
  buffer->append(contents);
  infile.close();

  return ::util::OkStatus();
}

}  // namespace p4server
