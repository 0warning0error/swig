/* -----------------------------------------------------------------------------
 * rustexception.swg
 *
 * Rust exception handling support - C/C++ runtime code.
 * 
 * This file provides the C/C++ runtime code for exception capture and error storage.
 * It follows SWIG's standard pattern used by C#, Java, and other language modules.
 *
 * The code is inserted into the generated _wrap.cxx file via %insert(runtime).
 * 
 * Rust-specific code templates are in swig_error.rs (read by rust.cxx).
 * ----------------------------------------------------------------------------- */

%insert(runtime) %{
/* Rust exception handling - error codes matching Rust SwigErrorCode enum */
#define SWIG_RUST_OK               0
#define SWIG_RUST_MemoryError     1
#define SWIG_RUST_IOError         2
#define SWIG_RUST_RuntimeError    3
#define SWIG_RUST_IndexError      4
#define SWIG_RUST_DivisionByZero  5
#define SWIG_RUST_OverflowError   6
#define SWIG_RUST_SyntaxError     7
#define SWIG_RUST_ValueError      8
#define SWIG_RUST_TypeError       9
#define SWIG_RUST_NullReference   10
#define SWIG_RUST_SystemError     11
#define SWIG_RUST_UnknownError    99

/* SWIG fail macro - jump to cleanup/fail section */
#ifndef SWIG_fail
#define SWIG_fail goto fail
#endif
%}

%insert(runtime) %{
#ifdef __cplusplus
#include <exception>
#include <string>

namespace {
  /* Thread-local error state for passing exceptions across FFI boundary.
   * This follows the same pattern as C#'s pending exception mechanism:
   * - C++ catches exception and stores error info
   * - Rust checks via FFI and converts to Result<T, SwigError>
   */
  thread_local int swig_rust_last_error_code = 0;
  thread_local std::string swig_rust_last_error_msg;
  thread_local bool swig_rust_error_pending = false;
}

extern "C" {
  /* FFI functions for Rust to check error state */

  SWIGEXPORT int SWIG_RustGetLastError(void) {
    return swig_rust_last_error_code;
  }

  SWIGEXPORT const char* SWIG_RustGetLastErrorMsg(void) {
    return swig_rust_last_error_msg.c_str();
  }

  SWIGEXPORT int SWIG_RustIsErrorPending(void) {
    return swig_rust_error_pending ? 1 : 0;
  }

  SWIGEXPORT void SWIG_RustClearError(void) {
    swig_rust_last_error_code = 0;
    swig_rust_last_error_msg.clear();
    swig_rust_error_pending = false;
  }

  SWIGEXPORT void SWIG_RustSetError(int code, const char* msg) {
    swig_rust_last_error_code = code;
    swig_rust_last_error_msg = msg ? msg : "";
    swig_rust_error_pending = true;
  }

  SWIGEXPORT void SWIG_RustSetErrorFromException(int code, const std::exception& e) {
    swig_rust_last_error_code = code;
    swig_rust_last_error_msg = e.what();
    swig_rust_error_pending = true;
  }
}

/* SWIG exception macro - similar to SWIG_exception in other modules */
#define SWIG_RustException(code, msg) \
  do { \
    SWIG_RustSetError(code, msg); \
    SWIG_fail; \
  } while (0)

#endif
%}
