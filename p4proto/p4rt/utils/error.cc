// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0


#include <string>

#include "error.h"

namespace p4server {

namespace error {

static const char kErrorSpaceName[] = "P4serverErrorSpace";

class P4serverErrorSpace : public ::util::ErrorSpace {
 public:
  P4serverErrorSpace() : ::util::ErrorSpace(kErrorSpaceName) {}
  ~P4serverErrorSpace() override {}

  ::std::string String(int code) const final {
    if (!ErrorCode_IsValid(code)) {
      code = ERR_UNKNOWN;
    }
    return ErrorCode_Name(static_cast<ErrorCode>(code));
  }

  ::util::error::Code CanonicalCode(const ::util::Status& status) const final {
    switch (status.error_code()) {
      case ERR_SUCCESS:
        return ::util::error::OK;
      case ERR_CANCELLED:
        return ::util::error::CANCELLED;
      case ERR_UNKNOWN:
        return ::util::error::UNKNOWN;
      case ERR_PERMISSION_DENIED:
        return ::util::error::PERMISSION_DENIED;
      case ERR_ABORTED:
        return ::util::error::ABORTED;
      case ERR_DATA_LOSS:
        return ::util::error::DATA_LOSS;
      case ERR_UNAUTHENTICATED:
        return ::util::error::UNAUTHENTICATED;
      case ERR_INTERNAL:
      case ERR_HARDWARE_ERROR:
        return ::util::error::INTERNAL;
      case ERR_INVALID_PARAM:
      case ERR_INVALID_P4_INFO:
        return ::util::error::INVALID_ARGUMENT;
      case ERR_OPER_TIMEOUT:
        return ::util::error::DEADLINE_EXCEEDED;
      case ERR_ENTRY_NOT_FOUND:
        return ::util::error::NOT_FOUND;
      case ERR_ENTRY_EXISTS:
        return ::util::error::ALREADY_EXISTS;
      case ERR_UNIMPLEMENTED:
      case ERR_OPER_NOT_SUPPORTED:
      case ERR_OPER_DISABLED:
        return ::util::error::UNIMPLEMENTED;
      case ERR_FEATURE_UNAVAILABLE:
        return ::util::error::UNAVAILABLE;
      case ERR_NO_RESOURCE:
        return ::util::error::RESOURCE_EXHAUSTED;
      case ERR_FAILED_PRECONDITION:
      case ERR_NOT_INITIALIZED:
        return ::util::error::FAILED_PRECONDITION;
      case ERR_OUT_OF_RANGE:
      case ERR_TABLE_FULL:
      case ERR_TABLE_EMPTY:
        return ::util::error::OUT_OF_RANGE;
      default:
        return ::util::error::UNKNOWN;  // Default error.
    }
  }

  // P4serverErrorSpace is neither copyable nor movable.
  P4serverErrorSpace(const P4serverErrorSpace&) = delete;
  P4serverErrorSpace& operator=(const P4serverErrorSpace&) = delete;
};

}  // namespace error

// Singleton P4serverErrorSpace.
const ::util::ErrorSpace* P4serverErrorSpace() {
  static const ::util::ErrorSpace* space = new error::P4serverErrorSpace();
  return space;
}

// Force registering of the errorspace at run-time.
static const ::util::ErrorSpace* dummy __attribute__((unused)) =
    P4serverErrorSpace();

}  // namespace p4server
