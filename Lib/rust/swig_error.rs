// This file is part of SWIG Rust binding
// 
// Rust code template for exception handling.
// Auto-generated into the .rs output file when -exception is enabled.
//
// The design follows SWIG's standard exception handling pattern:
// - C++ exceptions are caught and stored in thread-local state
// - Rust checks for pending errors via FFI functions
// - SwigError implements std::error::Error for idiomatic Rust usage

/// Error codes matching C++ exception types.
/// These correspond to SWIG_RUST_* constants in the generated C++ code.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(C)]
pub enum SwigErrorCode {
    Ok = 0,
    MemoryError = 1,
    IOError = 2,
    RuntimeError = 3,
    IndexError = 4,
    DivisionByZero = 5,
    OverflowError = 6,
    SyntaxError = 7,
    ValueError = 8,
    TypeError = 9,
    NullReference = 10,
    SystemError = 11,
    UnknownError = 99,
}

impl SwigErrorCode {
    fn from_c_int(code: c_int) -> Self {
        match code {
            0 => SwigErrorCode::Ok,
            1 => SwigErrorCode::MemoryError,
            2 => SwigErrorCode::IOError,
            3 => SwigErrorCode::RuntimeError,
            4 => SwigErrorCode::IndexError,
            5 => SwigErrorCode::DivisionByZero,
            6 => SwigErrorCode::OverflowError,
            7 => SwigErrorCode::SyntaxError,
            8 => SwigErrorCode::ValueError,
            9 => SwigErrorCode::TypeError,
            10 => SwigErrorCode::NullReference,
            11 => SwigErrorCode::SystemError,
            _ => SwigErrorCode::UnknownError,
        }
    }
}

/// Error type for SWIG-wrapped C++ exceptions.
/// 
/// This type wraps C++ exceptions that cross the FFI boundary,
/// similar to how C# wraps pending exceptions.
/// 
/// # Example
/// 
/// ```ignore
/// let result = obj.method_that_may_throw();
/// if let Some(error) = SwigError::check() {
///     println!("Error: {}", error);
/// }
/// ```
#[derive(Debug, Clone)]
pub struct SwigError {
    pub code: SwigErrorCode,
    pub message: String,
}

impl SwigError {
    /// Create a new SwigError from error code and message.
    pub fn new(code: SwigErrorCode, message: impl Into<String>) -> Self {
        SwigError {
            code,
            message: message.into(),
        }
    }

    /// Check if an error is pending from the last FFI call.
    /// 
    /// Returns `Some(SwigError)` if a C++ exception was caught,
    /// or `None` if no error is pending.
    /// 
    /// This is similar to C#'s pending exception check mechanism.
    pub fn check() -> Option<Self> {
        unsafe {
            if ffi::Rust_is_error_pending() != 0 {
                let code = SwigErrorCode::from_c_int(ffi::Rust_get_last_error_code());
                let msg_ptr = ffi::Rust_get_last_error_msg();
                let message = if msg_ptr.is_null() {
                    String::from("Unknown error")
                } else {
                    CStr::from_ptr(msg_ptr).to_string_lossy().into_owned()
                };
                ffi::Rust_clear_error();
                Some(SwigError { code, message })
            } else {
                None
            }
        }
    }

    /// Clear any pending error without returning it.
    pub fn clear() {
        unsafe { ffi::Rust_clear_error() }
    }
}

impl fmt::Display for SwigError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:?}: {}", self.code, self.message)
    }
}

impl std::error::Error for SwigError {}

/// Convenience macro for converting FFI calls to Result.
/// 
/// Returns `Err(SwigError)` if an error is pending, `Ok(())` otherwise.
/// 
/// # Example
/// 
/// ```ignore
/// fn safe_call(&mut self) -> Result<(), SwigError> {
///     self.ffi_method();
///     swig_check_error!()
/// }
/// ```
#[macro_export]
macro_rules! swig_check_error {
    () => {{
        if let Some(e) = $crate::SwigError::check() {
            Err(e)
        } else {
            Ok(())
        }
    }};
}
