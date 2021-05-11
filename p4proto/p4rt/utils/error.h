// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0


#ifndef P4SERVER_UTILS_ERROR_H_
#define P4SERVER_UTILS_ERROR_H_

#include "error.pb.h"
#include "status_macros.h"

namespace p4server {

// P4serverErrorSpace returns the singleton instance to be used through
// out the code.
const ::util::ErrorSpace* P4serverErrorSpace();

}  // namespace p4server

// Allow using status_macros. For example:
// return MAKE_ERROR(ERR_UNKNOWN) << "test";
namespace util {
namespace status_macros {

template <>
class ErrorCodeOptions<::p4server::ErrorCode>
    : public BaseErrorCodeOptions {
 public:
  const ::util::ErrorSpace* GetErrorSpace() {
    return ::p4server::P4serverErrorSpace();
  }
};

}  // namespace status_macros
}  // namespace util

#endif  // P4SERVER_UTILS_ERROR_H_
