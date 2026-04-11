/* -----------------------------------------------------------------------------
 * exception.i
 *
 * Rust language specific exception handling.
 *
 * Design:
 * - C++ exceptions are caught and converted to error codes
 * - Rust side can handle via panic (default) or Result type
 *
 * Usage:
 *   %include <rust/exception.i>
 *
 *   // For functions that throw:
 *   %exception {
 *     try {
 *       $action
 *     } catch (std::exception &e) {
 *       SWIG_RustSetError(e.what());
 *       return $null;
 *     }
 *   }
 * ----------------------------------------------------------------------------- */

// Define error codes matching SWIG standard errors
%{
#define SWIG_RUST_OK           0
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
#define SWIG_RUST_UnknownError    99
%}

// Thread-local error storage for passing exceptions across FFI
%insert("runtime") %{
#ifdef __cplusplus
#include <exception>
#include <string>

namespace {
  // Thread-local error state
  thread_local int swig_rust_last_error_code = 0;
  thread_local std::string swig_rust_last_error_msg;
}

extern "C" {
  // Get the last error code
  int SWIG_RustGetLastError() {
    return swig_rust_last_error_code;
  }

  // Get the last error message
  const char* SWIG_RustGetLastErrorMsg() {
    return swig_rust_last_error_msg.c_str();
  }

  // Clear the error state
  void SWIG_RustClearError() {
    swig_rust_last_error_code = 0;
    swig_rust_last_error_msg.clear();
  }

  // Set an error
  void SWIG_RustSetError(int code, const char* msg) {
    swig_rust_last_error_code = code;
    swig_rust_last_error_msg = msg ? msg : "";
  }
}

// SWIG exception macro for Rust
#define SWIG_exception(code, msg) \
  do { \
    SWIG_RustSetError(code, msg); \
    SWIG_fail; \
  } while (0)

#endif
%}

// Standard exception handlers
%typemap(throws) std::exception, std::exception& %{
  SWIG_RustSetError(SWIG_RuntimeError, $1.what());
  return $null;
%}

%typemap(throws) std::runtime_error, std::runtime_error& %{
  SWIG_RustSetError(SWIG_RuntimeError, $1.what());
  return $null;
%}

%typemap(throws) std::invalid_argument, std::invalid_argument& %{
  SWIG_RustSetError(SWIG_ValueError, $1.what());
  return $null;
%}

%typemap(throws) std::out_of_range, std::out_of_range& %{
  SWIG_RustSetError(SWIG_IndexError, $1.what());
  return $null;
%}

%typemap(throws) std::bad_alloc, std::bad_alloc& %{
  SWIG_RustSetError(SWIG_MemoryError, $1.what());
  return $null;
%}

%typemap(throws) std::logic_error, std::logic_error& %{
  SWIG_RustSetError(SWIG_RuntimeError, $1.what());
  return $null;
%}

// Catch-all exception handler
%typemap(throws) ... %{
  SWIG_RustSetError(SWIG_UnknownError, "Unknown exception");
  return $null;
%}

// Generate Rust error checking functions
%insert("wrapper") %{
// Error checking FFI functions
extern "C" {

// Check if last call resulted in an error
int Rust_get_last_error_code() {
  return SWIG_RustGetLastError();
}

// Get the error message
const char* Rust_get_last_error_msg() {
  return SWIG_RustGetLastErrorMsg();
}

// Clear error state
void Rust_clear_error() {
  SWIG_RustClearError();
}

} // extern "C"
%}

// FFI declarations for error checking (will be added to f_ffi_code)
