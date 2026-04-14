/* -----------------------------------------------------------------------------
 * std_string.i
 *
 * Typemaps for std::string and const std::string&
 * These are mapped to Rust String and &str respectively.
 *
 * Design notes:
 *   - std::string -> Rust String (owned, heap allocated)
 *   - const std::string& -> Rust &str (borrowed slice)
 *   - std::string& -> treated as std::string (passed by value)
 *   - FFI layer uses *mut c_char / *const c_char
 *
 * To use non-const std::string references, use:
 *   %apply const std::string & {std::string &};
 * ----------------------------------------------------------------------------- */

%{
#include <string>
%}

namespace std {

%naturalvar string;

class string;

/* -----------------------------------------------------------------------------
 * std::string (by value)
 *
 * Rust type: String
 * FFI type: *mut c_char (null-terminated)
 * ----------------------------------------------------------------------------- */

%typemap(ctype) string "const char *"
%typemap(rusttype) string "String"
%typemap(rsffitype) string "*mut c_char"

// Input: Rust String -> C++ std::string
// The Rust side converts String to *mut c_char (via CString::into_raw)
%typemap(in, canthrow=1) string
%{ if (!$input) {
    throw std::invalid_argument("null string");
   }
   $1.assign($input); %}

// Output: C++ std::string -> Rust String
// Return the C string pointer, Rust will convert to String
%typemap(out) string
%{ $result = $1.c_str(); %}

// Director input: C++ -> Rust callback
%typemap(directorin) string
%{ $input = $1.c_str(); %}

// Director output: Rust callback -> C++
%typemap(directorout, canthrow=1) string
%{ if (!$input) {
    SWIG_RustSetError(SWIG_RUST_NullReference, "null string");
    Swig::DirectorException::raise("null string");
   }
   $result.assign($input); %}

// Rust input conversion (generated in rust.cxx)
%typemap(rsin) string
%{ let $1_cstring = std::ffi::CString::new($input).expect("String contains null byte");
   let $1 = $1_cstring.as_ptr(); %}

// Rust output conversion (generated in rust.cxx)
%typemap(rsout) string
%{ let $result = std::ffi::CStr::from_ptr($1).to_string_lossy().into_owned(); %}

// Typecheck for overloaded functions
%typemap(typecheck) string = char *;

/* -----------------------------------------------------------------------------
 * const std::string& (by reference)
 *
 * Rust type: &str
 * FFI type: *const c_char (null-terminated)
 * ----------------------------------------------------------------------------- */

%typemap(ctype) const string & "const char *"
%typemap(rusttype) const string & "&str"
%typemap(rsffitype) const string & "*const c_char"

// Input: Rust &str -> C++ const std::string&
%typemap(in, canthrow=1) const string &
%{ if (!$input) {
    throw std::invalid_argument("null string");
   }
   $*1_ltype $1_str($input);
   $1 = &$1_str; %}

// Output: C++ const std::string& -> Rust String (converted to owned)
// Note: We can't safely return &str pointing to C++ memory, so we return String
%typemap(out) const string &
%{ $result = $1->c_str(); %}

// Director input
%typemap(directorin) const string &
%{ $input = $1.c_str(); %}

// Director output (warning: thread-unsafe for static local)
%typemap(directorout, canthrow=1, warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const string &
%{ if (!$input) {
    SWIG_RustSetError(SWIG_RUST_NullReference, "null string");
    Swig::DirectorException::raise("null string");
   }
   /* possible thread/reentrant code problem */
   static $*1_ltype $1_str;
   $1_str = $input;
   $result = &$1_str; %}

// Typecheck for overloaded functions
%typemap(typecheck) const string & = char *;

/* -----------------------------------------------------------------------------
 * std::string& (non-const reference)
 *
 * Treated as pass-by-value for simplicity.
 * Use %apply const std::string & {std::string &} for better semantics.
 * ----------------------------------------------------------------------------- */

%apply string { std::string & };

/* -----------------------------------------------------------------------------
 * std::string* (pointer)
 *
 * Mapped to Option<&mut String> or *mut String depending on use case.
 * ----------------------------------------------------------------------------- */

%typemap(ctype) string * "const char *"
%typemap(rusttype) string * "Option<String>"
%typemap(rsffitype) string * "*mut c_char"

%typemap(in) string *
%{ $1 = ($1_ltype)$input; %}

%typemap(out) string *
%{ $result = $input ? $input->c_str() : nullptr; %}

/* -----------------------------------------------------------------------------
 * const std::string* (const pointer)
 * ----------------------------------------------------------------------------- */

%typemap(ctype) const string * "const char *"
%typemap(rusttype) const string * "Option<&str>"
%typemap(rsffitype) const string * "*const c_char"

%typemap(in) const string *
%{ $1 = ($1_ltype)$input; %}

%typemap(out) const string *
%{ $result = $input ? $input->c_str() : nullptr; %}

/* -----------------------------------------------------------------------------
 * Exception handling
 * ----------------------------------------------------------------------------- */

%typemap(throws, canthrow=1) string
%{ throw std::runtime_error($1.c_str()); %}

%typemap(throws, canthrow=1) const string &
%{ throw std::runtime_error($1->c_str()); %}

} // namespace std
