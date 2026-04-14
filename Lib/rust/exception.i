/* -----------------------------------------------------------------------------
 * exception.i
 *
 * Rust exception handling for SWIG.
 *
 * Design follows SWIG's standard exception handling pattern:
 * - C++ exceptions are caught and converted to error codes
 * - Error state stored in thread-local variables (like C# pending exception)
 * - Rust checks for pending errors via FFI functions
 * - Rust can use Result<T, SwigError> for idiomatic error handling
 *
 * Usage:
 *   %include <rust/exception.i>
 *   %exception { RUST_EXCEPTION_HANDLER }  // Apply to functions that may throw
 *
 * This is similar to:
 *   - C#: %include <csharp/csharphead.swg> + SWIG_CSharpSetPendingException
 *   - Go:   %include <go/exception.i> + _swig_gopanic
 * ----------------------------------------------------------------------------- */

/* Include the C/C++ runtime support code */
%include <rust/rustexception.swg>

/* -----------------------------------------------------------------------------
 * Standard C++ exception typemaps
 * 
 * Map C++ exception types to Rust error codes.
 * Similar to Java's SWIG_JavaThrowException mappings.
 * ----------------------------------------------------------------------------- */

%typemap(throws) std::exception %{
  SWIG_RustSetErrorFromException(SWIG_RUST_RuntimeError, $1);
  return $null;
%}

%typemap(throws) std::runtime_error %{
  SWIG_RustSetErrorFromException(SWIG_RUST_RuntimeError, $1);
  return $null;
%}

%typemap(throws) std::invalid_argument %{
  SWIG_RustSetErrorFromException(SWIG_RUST_ValueError, $1);
  return $null;
%}

%typemap(throws) std::out_of_range %{
  SWIG_RustSetErrorFromException(SWIG_RUST_IndexError, $1);
  return $null;
%}

%typemap(throws) std::bad_alloc %{
  SWIG_RustSetErrorFromException(SWIG_RUST_MemoryError, $1);
  return $null;
%}

%typemap(throws) std::logic_error %{
  SWIG_RustSetErrorFromException(SWIG_RUST_RuntimeError, $1);
  return $null;
%}

%typemap(throws) std::domain_error %{
  SWIG_RustSetErrorFromException(SWIG_RUST_ValueError, $1);
  return $null;
%}

%typemap(throws) std::length_error %{
  SWIG_RustSetErrorFromException(SWIG_RUST_ValueError, $1);
  return $null;
%}

%typemap(throws) std::range_error %{
  SWIG_RustSetErrorFromException(SWIG_RUST_ValueError, $1);
  return $null;
%}

%typemap(throws) std::overflow_error %{
  SWIG_RustSetErrorFromException(SWIG_RUST_OverflowError, $1);
  return $null;
%}

%typemap(throws) std::underflow_error %{
  SWIG_RustSetErrorFromException(SWIG_RUST_OverflowError, $1);
  return $null;
%}

/* -----------------------------------------------------------------------------
 * Exception Handler Macros
 * 
 * RUST_EXCEPTION_HANDLER - Catch all standard C++ exceptions
 * Similar to %exception blocks used in other language modules.
 * ----------------------------------------------------------------------------- */

%define RUST_EXCEPTION_HANDLER
  try {
    $action
  } catch (std::bad_alloc &_e) {
    SWIG_RustSetErrorFromException(SWIG_RUST_MemoryError, _e);
    return $null;
  } catch (std::out_of_range &_e) {
    SWIG_RustSetErrorFromException(SWIG_RUST_IndexError, _e);
    return $null;
  } catch (std::invalid_argument &_e) {
    SWIG_RustSetErrorFromException(SWIG_RUST_ValueError, _e);
    return $null;
  } catch (std::overflow_error &_e) {
    SWIG_RustSetErrorFromException(SWIG_RUST_OverflowError, _e);
    return $null;
  } catch (std::underflow_error &_e) {
    SWIG_RustSetErrorFromException(SWIG_RUST_OverflowError, _e);
    return $null;
  } catch (std::exception &_e) {
    SWIG_RustSetErrorFromException(SWIG_RUST_RuntimeError, _e);
    return $null;
  } catch (...) {
    SWIG_RustSetError(SWIG_RUST_UnknownError, "Unknown C++ exception");
    return $null;
  }
%enddef