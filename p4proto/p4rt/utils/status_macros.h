// Copyright 2018 Google LLC
// Copyright 2018-present Open Networking Foundation
// SPDX-License-Identifier: Apache-2.0


// This library contains helper macros and methods to make returning errors
// and propagating statuses easier.
//
// We use ::util::Status for error codes.  Methods that return status should
// have signatures like
//   ::util::Status Method(arg, ...);
// or
//   ::util::StatusOr<ValueType> Method(arg, ...);
//
// Inside the method, to return errors, use the macros
//   RETURN_ERROR() << "Message with ::util::error::UNKNOWN code";
//   RETURN_ERROR(code_enum)
//       << "Message with an error code, in that error_code's ErrorSpace "
//       << "(See ErrorCodeOptions below)";
//   RETURN_ERROR(error_space, code_int)
//       << "Message with integer error code in specified ErrorSpace "
//       << "(Not recommended - use previous form with an enum code instead)";
//
// When calling another method, use this to propagate status easily.
//   RETURN_IF_ERROR(method(args));
//
// Use this to also append to the end of the error message when propagating
// an error:
//   RETURN_IF_ERROR_WITH_APPEND(method(args)) << " for method with " << args;
//
// Use this to propagate the status to a Stubby1 or Stubby2 RPC easily. This
// assumes an AutoClosureRunner has been set up on the RPC's associated
// closure, or it gets run some other way to signal the RPC's termination.
//   RETURN_RPC_IF_ERROR(rpc, method(args));
//
// Use this to propagate the status to a ::util::Task* instance
// calling task->Return() with the status.
//   RETURN_TASK_IF_ERROR(task, method(args));
//
// For StatusOr results, you can extract the value or return on error.
//   ASSIGN_OR_RETURN(ValueType value, MaybeGetValue(arg));
// Or:
//   ValueType value;
//   ASSIGN_OR_RETURN(value, MaybeGetValue(arg));
//
// WARNING: ASSIGN_OR_RETURN expands into multiple statements; it cannot be used
//  in a single statement (e.g. as the body of an if statement without {})!
//
// This can optionally be used to return ::util::Status::OK.
//   RETURN_OK();
//
// To construct an error without immediately returning it, use MAKE_ERROR,
// which supports the same argument types as RETURN_ERROR.
//   ::util::Status status = MAKE_ERROR(...) << "Message";
//
// To add additional text onto an existing error, use
//   ::util::Status new_status = APPEND_ERROR(status) << ", additional details";
//
// These macros can also be assigned to a ::util::StatusOr variable:
//   ::util::StatusOr<T> status_or = MAKE_ERROR(...) << "Message";
//
// They can also be used to return from a function that returns
// ::util::StatusOr:
//   ::util::StatusOr<T> MyFunction() {
//     RETURN_ERROR(...) << "Message";
//   }
//
//
// Error codes:
//
// Using error codes is optional.  ::util::error::UNKNOWN will be used if no
// code is provided.
//
// By default, these macros work with canonical ::util::error::Code codes,
// using the canonical ErrorSpace. These macros will also work with
// project-specific ErrorSpaces and error code enums if a specialization
// of ErrorCodeOptions is defined.
//
//
// Logging:
//
// RETURN_ERROR and MAKE_ERROR log the error to LOG(ERROR) by default.
//
// Logging can be turned on or off for a specific error by using
//   RETURN_ERROR().with_logging() << "Message logged to LOG(ERROR)";
//   RETURN_ERROR().without_logging() << "Message not logged";
//   RETURN_ERROR().set_logging(false) << "Message not logged";
//   RETURN_ERROR().severity(INFO) << "Message logged to LOG(INFO)";
//
// If logging is enabled, this will make an error also log a stack trace.
//   RETURN_ERROR().with_log_stack_trace() << "Message";
//
// Logging can also be controlled within a scope using
// ScopedErrorLogSuppression.
//
//
// Assertion handling:
//
// When you would use a CHECK, CHECK_EQ, etc, you can instead use RET_CHECK
// to return a ::util::Status if the condition is not met:
//   RET_CHECK(ptr != null);
//   RET_CHECK_GT(value, 0) << "Optional additional message";
//   RET_CHECK_FAIL() << "Always fail, like a LOG(FATAL)";
//
// These are a better replacement for CHECK because they don't crash, and for
// DCHECK and LOG(DFATAL) because they don't ignore errors in opt builds.
//
// The RET_CHECK* macros can only be used in functions that return
// ::util::Status.
//
// The returned error will have the ::util::error::INTERNAL error code and the
// message will include the file and line number.  The current stack trace
// will also be logged.

#ifndef P4SERVER_UTILS_STATUS_MACROS_H_
#define P4SERVER_UTILS_STATUS_MACROS_H_

#include <memory>
#include <ostream>  // NOLINT
#include <sstream>  // NOLINT  // IWYU pragma: keep
#include <string>
#include <vector>

#include <absl/base/optimization.h>

#include "logging.h"
#include "status.h"
#include "statusor.h"

namespace util {

namespace status_macros {

using google::LogSeverity;

// Base class for options attached to a project-specific error code enum.
// Projects that use non-canonical error codes should specialize the
// ErrorCodeOptions template below with a subclass of this class, overriding
// GetErrorSpace, and optionally other methods.
class BaseErrorCodeOptions {
 public:
  // Return the ErrorSpace to use for this error code enum.
  // Not implemented in base class - must be overridden.
  const ::util::ErrorSpace* GetErrorSpace();

  // Returns true if errors with this code should be logged upon creation, by
  // default.  (Default can be overridden with modifiers on MakeErrorStream.)
  // Can be overridden to customize default logging per error code.
  bool IsLoggedByDefault(int code) { return true; }
};

// Template that should be specialized for any project-specific error code enum.
template <typename ERROR_CODE_ENUM_TYPE>
class ErrorCodeOptions;

// Specialization for the canonical error codes and canonical ErrorSpace.
template <>
class ErrorCodeOptions< ::util::error::Code> : public BaseErrorCodeOptions {
 public:
  const ::util::ErrorSpace* GetErrorSpace() {
    return ::util::Status::canonical_space();
  }
};

// Stream object used to collect error messages in MAKE_ERROR macros or
// append error messages with APPEND_ERROR.
// It accepts any arguments with operator<< to build an error string, and
// then has an implicit cast operator to ::util::Status, which converts the
// logged string to a Status object and returns it, after logging the error.
// At least one call to operator<< is required; a compile time error will be
// generated if none are given. Errors will only be logged by default for
// certain status codes, as defined in IsLoggedByDefault. This class will
// give DFATAL errors if you don't retrieve a ::util::Status exactly once before
// destruction.
//
// The class converts into an intermediate wrapper object
// MakeErrorStreamWithOutput to check that the error stream gets at least one
// item of input.
class MakeErrorStream {
 public:
  // Wrapper around MakeErrorStream that only allows for output. This is created
  // as output of the first operator<< call on MakeErrorStream. The bare
  // MakeErrorStream does not have a ::util::Status operator. The net effect of
  // that is that you have to call operator<< at least once or else you'll get
  // a compile time error.
  class MakeErrorStreamWithOutput {
   public:
    explicit MakeErrorStreamWithOutput(MakeErrorStream* error_stream)
        : wrapped_error_stream_(error_stream) {}

    template <typename T>
    MakeErrorStreamWithOutput& operator<<(const T& value) {
      *wrapped_error_stream_ << value;
      return *this;
    }

    // Implicit cast operators to ::util::Status and ::util::StatusOr.
    // Exactly one of these must be called exactly once before destruction.
    operator ::util::Status() {
      return wrapped_error_stream_->GetStatus();
    }
    template <typename T>
    operator ::util::StatusOr<T>() {
      return wrapped_error_stream_->GetStatus();
    }

    // MakeErrorStreamWithOutput is neither copyable nor movable.
    MakeErrorStreamWithOutput(const MakeErrorStreamWithOutput&) = delete;
    MakeErrorStreamWithOutput& operator=(const MakeErrorStreamWithOutput&) =
        delete;

   private:
    MakeErrorStream* wrapped_error_stream_;
  };

  // Make an error with ::util::error::UNKNOWN.
  MakeErrorStream(const char* file, int line)
      : impl_(new Impl(file, line,
                       ::util::Status::canonical_space(),
                       ::util::error::UNKNOWN, this)) {}

  // Make an error with the given error code and error_space.
  MakeErrorStream(const char* file, int line,
                  const ::util::ErrorSpace* error_space, int code)
      : impl_(new Impl(file, line, error_space, code, this)) {}

  // Make an error that appends additional messages onto a copy of status.
  MakeErrorStream(::util::Status status, const char* file, int line)
      : impl_(new Impl(status, file, line, this)) {}

  // Make an error with the given code, inferring its ErrorSpace from
  // code's type using the specialized ErrorCodeOptions.
  template <typename ERROR_CODE_TYPE>
  MakeErrorStream(const char* file, int line, ERROR_CODE_TYPE code)
    : impl_(new Impl(
          file, line,
          ErrorCodeOptions<ERROR_CODE_TYPE>().GetErrorSpace(),
          code, this,
          ErrorCodeOptions<ERROR_CODE_TYPE>().IsLoggedByDefault(code))) {}

  template <typename T>
  MakeErrorStreamWithOutput& operator<<(const T& value) {
    CheckNotDone();
    impl_->stream_ << value;
    return impl_->make_error_stream_with_output_wrapper_;
  }

  // Disable sending this message to LOG(ERROR), even if this code is usually
  // logged. Some error codes are logged by default, and others are not.
  // Usage:
  //   return MAKE_ERROR().without_logging() << "Message";
  MakeErrorStream& without_logging() {
    impl_->should_log_ = false;
    return *this;
  }

  // Determine whether to log this message based on the value of <should_log>.
  MakeErrorStream& set_logging(bool should_log) {
    impl_->should_log_ = should_log;
    return *this;
  }

  // Log the status at this LogSeverity: INFO, WARNING, or ERROR.
  // Setting severity to NUM_SEVERITIES will disable logging.
  MakeErrorStream& severity(LogSeverity log_severity) {
    impl_->log_severity_ = log_severity;
    return *this;
  }

  // MakeErrorStream is neither copyable nor movable.
  MakeErrorStream(const MakeErrorStream&) = delete;
  MakeErrorStream& operator=(const MakeErrorStream&) = delete;

 private:
  class Impl {
   public:
    Impl(const char* file, int line,
         const ::util::ErrorSpace* error_space, int  code,
         MakeErrorStream* error_stream,
         bool is_logged_by_default = true);
    Impl(const ::util::Status& status, const char* file, int line,
         MakeErrorStream* error_stream);

    ~Impl();

    // This must be called exactly once before destruction.
    ::util::Status GetStatus();

    void CheckNotDone() const;

    // Impl is neither copyable nor movable.
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

   private:
    const char* file_;
    int line_;
    const ::util::ErrorSpace* error_space_;
    int code_;

    std::string prior_message_;
    bool is_done_;  // true after Status object has been returned
    std::ostringstream stream_;
    bool should_log_;
    LogSeverity log_severity_;
    bool should_log_stack_trace_;

    // Wrapper around the MakeErrorStream object that has a ::util::Status
    // conversion. The first << operator called on MakeErrorStream will return
    // this object, and only this object can implicitly convert to
    // ::util::Status. The net effect of this is that you'll get a compile time
    // error if you call MAKE_ERROR etc. without adding any output.
    MakeErrorStreamWithOutput make_error_stream_with_output_wrapper_;

    friend class MakeErrorStream;
  };

  void CheckNotDone() const;

  // Returns the status. Used by MakeErrorStreamWithOutput.
  ::util::Status GetStatus() const { return impl_->GetStatus(); }

  // Store the actual data on the heap to reduce stack frame sizes.
  std::unique_ptr<Impl> impl_;
};

// Make an error ::util::Status, building message with LOG-style shift
// operators.  The error also gets sent to LOG(ERROR).
//
// Takes an optional error code parameter. Uses ::util::error::UNKNOWN by
// default.  Returns a ::util::Status object that must be returned or stored.
//
// Examples:
//   return MAKE_ERROR() << "Message";
//   return MAKE_ERROR(INTERNAL_ERROR) << "Message";
//   ::util::Status status = MAKE_ERROR() << "Message";
#define MAKE_ERROR(...) \
  ::util::status_macros::MakeErrorStream(__FILE__, __LINE__, ##__VA_ARGS__)

// Run a command that returns a ::util::Status.  If the called code returns an
// error status, return that status up out of this method too.
//
// Example:
//   RETURN_IF_ERROR(DoThings(4));
#define RETURN_IF_ERROR(expr)                                                \
  do {                                                                       \
    /* Using _status below to avoid capture problems if expr is "status". */ \
    const ::util::Status _status = (expr);                                   \
    if (ABSL_PREDICT_FALSE(!_status.ok())) {                                 \
      LOG(ERROR) << "Return Error: " << #expr << " failed with " << _status; \
      return _status;                                                        \
    }                                                                        \
  } while (0)

}  // namespace status_macros
}  // namespace util

#endif  // P4SERVER_UTILS_STATUS_MACROS_H_
