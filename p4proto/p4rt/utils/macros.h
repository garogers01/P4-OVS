// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0


#ifndef P4SERVER_UTILS_MACROS_H_
#define P4SERVER_UTILS_MACROS_H_

#include <string>

#include "status_macros.h"
#include "logging.h"
#include "error.h"

namespace p4server {

#define ABSL_DIE_IF_NULL CHECK_NOTNULL

// Stringify the result of expansion of a macro to a string
// e.g:
// #define A text
// STRINGIFY(A) => "text"
// Ref: https://gcc.gnu.org/onlinedocs/gcc-4.8.5/cpp/Stringification.html
#define STRINGIFY_INNER(s) #s
#define STRINGIFY(s) STRINGIFY_INNER(s)

}  // namespace p4server

#endif  // P4SERVER_UTILS_MACROS_H_
