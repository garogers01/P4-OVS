// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#ifndef P4SERVER_COMMON_WRITER_INTERFACE_H_
#define P4SERVER_COMMON_WRITER_INTERFACE_H_

#include <memory>
#include <utility>

// An interface for a wrapper around the Write operation for various data
// transport mechanisms, e.g. P4server Channel, gRPC ServerWriter. This class
// makes the SwitchInterface class more abstract and eliminates the dependency
// to gRPC as well as P4server-internal constructs.
template <typename T>
class WriterInterface {
 public:
  virtual ~WriterInterface() {}
  // Blocking Write() operation which passes a message of type T into the
  // underlying transfer mechanism.
  virtual bool Write(const T& msg) = 0;

 protected:
  // Default constructor. To be called by the Mock class instance only.
  WriterInterface() {}
};

#endif  // P4SERVER_COMMON_WRITER_INTERFACE_H_
