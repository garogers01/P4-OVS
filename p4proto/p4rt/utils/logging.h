// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0

#ifndef P4SERVER_UTILS_LOGGING_H_
#define P4SERVER_UTILS_LOGGING_H_

#include <glog/logging.h> // IWYU pragma: export

// These are exported in open source glog but not base/logging.h
using ::google::ERROR;
using ::google::FATAL;
using ::google::INFO;
using ::google::WARNING;

#endif  // P4SERVER_UTILS_LOGGING_H_
