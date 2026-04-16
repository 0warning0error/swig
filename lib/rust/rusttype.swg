/* -----------------------------------------------------------------------------
 * rusttype.swg
 *
 * Rust type mappings for SWIG.
 * 
 * Typemaps:
 *   rusttype   - Rust type visible to user (safe wrapper)
 *   rsffitype  - Rust FFI type (extern "C" declarations)
 *   cout       - C output type for wrapper functions
 *   in         - C input conversion (standard SWIG typemap)
 *   out        - C output conversion (standard SWIG typemap)
 * ----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
 * Default "in" typemap for all types
 * 
 * This is required for emit_num_arguments() to work correctly.
 * ----------------------------------------------------------------------------- */
%typemap(in) SWIGTYPE {
  $1 = ($1_ltype)$input;
}

%typemap(in) SWIGTYPE * {
  $1 = ($1_ltype)$input;
}

%typemap(in) SWIGTYPE & {
  $1 = ($1_ltype)$input;
}

/* -----------------------------------------------------------------------------
 * Default "out" typemap for all types
 * 
 * $result is the wrapper function's return variable (jresult)
 * $1 is the result of calling the wrapped function
 * ----------------------------------------------------------------------------- */
%typemap(out) SWIGTYPE {
  $result = $1;
}

%typemap(out) SWIGTYPE * {
  $result = $1;
}

%typemap(out) SWIGTYPE & {
  $result = $1;
}

/* -----------------------------------------------------------------------------
 * Basic integer types
 * ----------------------------------------------------------------------------- */

/* bool */
%typemap(rusttype) bool, const bool &               "bool"
%typemap(rsffitype) bool, const bool &              "c_uchar"
%typemap(cout) bool                                 "int"
%typemap(in) bool                                   "$1 = ($1_ltype)$input;"
%typemap(out) bool                                  "$result = $1;"

/* char */
%typemap(rusttype) char, const char &               "c_char"
%typemap(rsffitype) char, const char &              "c_char"
%typemap(cout) char                                 "char"
%typemap(in) char                                   "$1 = ($1_ltype)$input;"
%typemap(out) char                                  "$result = $1;"

/* signed char */
%typemap(rusttype) signed char, const signed char & "i8"
%typemap(rsffitype) signed char, const signed char & "c_schar"
%typemap(cout) signed char                          "signed char"
%typemap(in) signed char                          "$1 = ($1_ltype)$input;"
%typemap(out) signed char                         "$result = $1;"

/* unsigned char */
%typemap(rusttype) unsigned char, const unsigned char & "u8"
%typemap(rsffitype) unsigned char, const unsigned char & "c_uchar"
%typemap(cout) unsigned char                        "unsigned char"
%typemap(in) unsigned char                        "$1 = ($1_ltype)$input;"
%typemap(out) unsigned char                       "$result = $1;"

/* short */
%typemap(rusttype) short, const short &             "i16"
%typemap(rsffitype) short, const short &            "c_short"
%typemap(cout) short                                "short"
%typemap(in) short                                "$1 = ($1_ltype)$input;"
%typemap(out) short                               "$result = $1;"

/* unsigned short */
%typemap(rusttype) unsigned short, const unsigned short & "u16"
%typemap(rsffitype) unsigned short, const unsigned short & "c_ushort"
%typemap(cout) unsigned short                       "unsigned short"
%typemap(in) unsigned short                       "$1 = ($1_ltype)$input;"
%typemap(out) unsigned short                      "$result = $1;"

/* int */
%typemap(rusttype) int, const int &                 "i32"
%typemap(rsffitype) int, const int &                "c_int"
%typemap(cout) int                                  "int"
%typemap(in) int                                  "$1 = ($1_ltype)$input;"
%typemap(out) int                                 "$result = $1;"

/* unsigned int */
%typemap(rusttype) unsigned int, const unsigned int & "u32"
%typemap(rsffitype) unsigned int, const unsigned int & "c_uint"
%typemap(cout) unsigned int                         "unsigned int"
%typemap(in) unsigned int                         "$1 = ($1_ltype)$input;"
%typemap(out) unsigned int                        "$result = $1;"

/* long - platform dependent, use c_long for correct size */
%typemap(rusttype) long, const long &               "c_long"
%typemap(rsffitype) long, const long &              "c_long"
%typemap(cout) long                                 "long"
%typemap(in) long                                 "$1 = ($1_ltype)$input;"
%typemap(out) long                                "$result = $1;"

/* unsigned long - platform dependent, use c_ulong for correct size */
%typemap(rusttype) unsigned long, const unsigned long & "c_ulong"
%typemap(rsffitype) unsigned long, const unsigned long & "c_ulong"
%typemap(cout) unsigned long                        "unsigned long"
%typemap(in) unsigned long                        "$1 = ($1_ltype)$input;"
%typemap(out) unsigned long                       "$result = $1;"

/* long long */
%typemap(rusttype) long long, const long long &     "i64"
%typemap(rsffitype) long long, const long long &    "c_longlong"
%typemap(cout) long long                            "long long"
%typemap(in) long long                            "$1 = ($1_ltype)$input;"
%typemap(out) long long                           "$result = $1;"

/* unsigned long long */
%typemap(rusttype) unsigned long long, const unsigned long long & "u64"
%typemap(rsffitype) unsigned long long, const unsigned long long & "c_ulonglong"
%typemap(cout) unsigned long long                   "unsigned long long"
%typemap(in) unsigned long long                   "$1 = ($1_ltype)$input;"
%typemap(out) unsigned long long                  "$result = $1;"

/* -----------------------------------------------------------------------------
 * Floating point types
 * ----------------------------------------------------------------------------- */

/* float */
%typemap(rusttype) float, const float &             "f32"
%typemap(rsffitype) float, const float &            "c_float"
%typemap(cout) float                                "float"
%typemap(in) float                                "$1 = ($1_ltype)$input;"
%typemap(out) float                               "$result = $1;"

/* double */
%typemap(rusttype) double, const double &           "f64"
%typemap(rsffitype) double, const double &          "c_double"
%typemap(cout) double                               "double"
%typemap(in) double                               "$1 = ($1_ltype)$input;"
%typemap(out) double                              "$result = $1;"

/* -----------------------------------------------------------------------------
 * Special types
 * ----------------------------------------------------------------------------- */

/* void */
%typemap(rusttype) void                             ""
%typemap(rsffitype) void                            ""
%typemap(cout) void                                 "void"

/* size_t - platform dependent pointer-sized unsigned */
%typemap(rusttype) size_t, const size_t &           "usize"
%typemap(rsffitype) size_t, const size_t &          "usize"
%typemap(cout) size_t                               "size_t"
%typemap(in) size_t                               "$1 = ($1_ltype)$input;"
%typemap(out) size_t                              "$result = $1;"

/* ptrdiff_t - platform dependent pointer-sized signed */
%typemap(rusttype) ptrdiff_t, const ptrdiff_t &     "isize"
%typemap(rsffitype) ptrdiff_t, const ptrdiff_t &    "isize"
%typemap(cout) ptrdiff_t                            "ptrdiff_t"
%typemap(in) ptrdiff_t                            "$1 = ($1_ltype)$input;"
%typemap(out) ptrdiff_t                           "$result = $1;"

/* wchar_t - wide character
 * Note: wchar_t is 2 bytes on Windows (UTF-16), 4 bytes on Unix (UTF-32)
 * Rust doesn't have c_wchar in std::os::raw, so we use:
 * - u16 on Windows (matches wchar_t size)
 * - u32 on Unix (matches wchar_t size)
 * For cross-platform code, use std::wstring instead.
 */
%typemap(rusttype) wchar_t, const wchar_t &         "u16"
%typemap(rsffitype) wchar_t, const wchar_t &        "u16"
%typemap(cout) wchar_t                              "wchar_t"
%typemap(in) wchar_t                              "$1 = ($1_ltype)$input;"
%typemap(out) wchar_t                             "$result = $1;"

/* wchar_t* - wide string pointer (C-style) */
%typemap(rusttype) wchar_t *                        "*mut u16"
%typemap(rsffitype) wchar_t *                       "*mut u16"
%typemap(cout) wchar_t *                            "wchar_t *"
%typemap(in) wchar_t *                            "$1 = ($1_ltype)$input;"
%typemap(out) wchar_t *                           "$result = $1;"

%typemap(rusttype) const wchar_t *                  "*const u16"
%typemap(rsffitype) const wchar_t *                 "*const u16"
%typemap(cout) const wchar_t *                      "const wchar_t *"
%typemap(in) const wchar_t *                      "$1 = ($1_ltype)$input;"
%typemap(out) const wchar_t *                     "$result = $1;"

/* -----------------------------------------------------------------------------
 * Pointer types
 * ----------------------------------------------------------------------------- */

/* void* - opaque pointer */
%typemap(rusttype) void *                           "*mut c_void"
%typemap(rsffitype) void *                          "*mut c_void"
%typemap(cout) void *                               "void *"
%typemap(in) void *                               "$1 = ($1_ltype)$input;"
%typemap(out) void *                              "$result = $1;"

/* const void* */
%typemap(rusttype) const void *                     "*const c_void"
%typemap(rsffitype) const void *                    "*const c_void"
%typemap(cout) const void *                         "const void *"
%typemap(in) const void *                         "$1 = ($1_ltype)$input;"
%typemap(out) const void *                        "$result = $1;"

/* Generic pointer types - these are handled specially in rust.cxx */
%typemap(rusttype) SWIGTYPE *                       "*mut c_void"
%typemap(rsffitype) SWIGTYPE *                      "*mut c_void"
%typemap(cout) SWIGTYPE *                           "void *"
%typemap(in) SWIGTYPE *                           "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE *                          "$result = $1;"

%typemap(rusttype) SWIGTYPE *const                  "*const c_void"
%typemap(rsffitype) SWIGTYPE *const                 "*const c_void"
%typemap(cout) SWIGTYPE *const                      "void *"
%typemap(in) SWIGTYPE *const                      "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE *const                     "$result = $1;"

/* const pointer */
%typemap(rusttype) const SWIGTYPE *                 "*const c_void"
%typemap(rsffitype) const SWIGTYPE *                "*const c_void"
%typemap(cout) const SWIGTYPE *                     "const void *"
%typemap(in) const SWIGTYPE *                     "$1 = ($1_ltype)$input;"
%typemap(out) const SWIGTYPE *                    "$result = $1;"

/* -----------------------------------------------------------------------------
 * Reference types
 * 
 * Use $rustclassname special variable which gets substituted by rust.cxx
 * Similar to C#'s $csclassname and Java's $javaclassname
 * ----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
 * Reference types
 * 
 * References in C++ are similar to pointers for FFI purposes.
 * We map them to raw pointers in Rust.
 * 
 * Note: For member variable getters returning references, the rsout typemap
 * converts the FFI pointer to the wrapped type.
 * ----------------------------------------------------------------------------- */

// Non-const reference - use pointer type
%typemap(rusttype) SWIGTYPE &                       "*mut $*rustclassname"
%typemap(rsffitype) SWIGTYPE &                      "*mut c_void"
%typemap(cout) SWIGTYPE &                           "void *"
%typemap(in) SWIGTYPE &                           "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE &                          "$result = $1;"

// Const reference - use const pointer type
%typemap(rusttype) const SWIGTYPE &                 "*const $*rustclassname"
%typemap(rsffitype) const SWIGTYPE &                "*const c_void"
%typemap(cout) const SWIGTYPE &                     "const void *"
%typemap(in) const SWIGTYPE &                     "$1 = ($1_ltype)$input;"
%typemap(out) const SWIGTYPE &                    "$result = $1;"

/* Rvalue reference (C++11) */
%typemap(rusttype) SWIGTYPE &&                      "$*rustclassname"
%typemap(rsffitype) SWIGTYPE &&                     "*mut c_void"
%typemap(cout) SWIGTYPE &&                          "void *"
%typemap(in) SWIGTYPE &&                          "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE &&                         "$result = $1;"

/* -----------------------------------------------------------------------------
 * Array types
 * In C/C++, array parameters decay to pointers: int arr[] is equivalent to int* arr
 * So we map them to pointer types in Rust
 * 
 * Note: We don't define rusttype for arrays - let the code use getRustUserType()
 * which properly handles array type conversion (const int[] -> *const i32)
 * ----------------------------------------------------------------------------- */

// C-side typemaps (still needed for C wrapper generation)
%typemap(rsffitype) SWIGTYPE []                     "*mut c_void"
%typemap(cout) SWIGTYPE []                          "void *"
%typemap(in) SWIGTYPE []                          "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE []                         "$result = $1;"

// Fixed-size arrays
%typemap(rsffitype) SWIGTYPE [ANY]                  "*mut c_void"
%typemap(cout) SWIGTYPE [ANY]                       "void *"
%typemap(in) SWIGTYPE [ANY]                       "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE [ANY]                      "$result = $1;"

/* -----------------------------------------------------------------------------
 * Value types (passed by value)
 * For return by value, we need to allocate on heap to return a pointer.
 * 
 * Note: We don't define rusttype for SWIGTYPE here because we can't use
 * typemap variables like $&1_type in rusttype values. Instead, let the code
 * in rust.cxx use getRustUserType() to generate proper type names.
 * ----------------------------------------------------------------------------- */

%typemap(rusttype) SWIGTYPE                        "$&rustclassname"
%typemap(rsffitype) SWIGTYPE                        "*mut c_void"
%typemap(cout) SWIGTYPE                             "void *"
%typemap(in) SWIGTYPE                             "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE                            "$result = (void *)new $1_type($1);"

/* -----------------------------------------------------------------------------
 * String types
 * 
 * Important: char* and const char* need special handling.
 * 
 * char* - mutable string pointer, typically used as output buffer
 *   Use *mut c_char because Rust String cannot provide mutable pointer
 *   User needs to create buffer manually: let mut buf = [0i8; 256];
 * 
 * const char* - immutable string pointer
 *   Use *const c_char to avoid lifetime issues
 *   User can convert: CStr::from_ptr(ptr).to_str().unwrap() or .to_string_lossy()
 * ----------------------------------------------------------------------------- */

/* char* - mutable string pointer */
%typemap(rusttype) char *                           "*mut c_char"
%typemap(rsffitype) char *                          "*mut c_char"
%typemap(cout) char *                               "char *"
%typemap(in) char *                               "$1 = ($1_ltype)$input;"
%typemap(out) char *, char *&                     "$result = $1;"

/* const char* - immutable string pointer */
%typemap(rusttype) const char *                     "*const c_char"
%typemap(rsffitype) const char *                    "*const c_char"
%typemap(cout) const char *                         "const char *"
%typemap(in) const char *                         "$1 = ($1_ltype)$input;"
%typemap(out) const char *                        "$result = $1;"

/* unsigned char* - byte buffer pointer */
%typemap(rusttype) unsigned char *                  "*mut c_uchar"
%typemap(rsffitype) unsigned char *                 "*mut c_uchar"
%typemap(cout) unsigned char *                      "unsigned char *"
%typemap(in) unsigned char *                      "$1 = ($1_ltype)$input;"
%typemap(out) unsigned char *                     "$result = $1;"

/* const unsigned char* - immutable byte buffer pointer */
%typemap(rusttype) const unsigned char *            "*const c_uchar"
%typemap(rsffitype) const unsigned char *           "*const c_uchar"
%typemap(cout) const unsigned char *                "const unsigned char *"
%typemap(in) const unsigned char *                "$1 = ($1_ltype)$input;"
%typemap(out) const unsigned char *               "$result = $1;"

/* signed char* */
%typemap(rusttype) signed char *                    "*mut c_schar"
%typemap(rsffitype) signed char *                   "*mut c_schar"
%typemap(cout) signed char *                        "signed char *"
%typemap(in) signed char *                        "$1 = ($1_ltype)$input;"
%typemap(out) signed char *                       "$result = $1;"

/* const signed char* */
%typemap(rusttype) const signed char *              "*const c_schar"
%typemap(rsffitype) const signed char *             "*const c_schar"
%typemap(cout) const signed char *                  "const signed char *"
%typemap(in) const signed char *                  "$1 = ($1_ltype)$input;"
%typemap(out) const signed char *                 "$result = $1;"

/* Basic type pointers - map to c_void for generic handling */
/* int* and const int* */
%typemap(rusttype) int *                            "*mut i32"
%typemap(rsffitype) int *                           "*mut c_void"
%typemap(cout) int *                                "int *"
%typemap(in) int *                                "$1 = ($1_ltype)$input;"
%typemap(out) int *                               "$result = $1;"

%typemap(rusttype) const int *                      "*const i32"
%typemap(rsffitype) const int *                     "*const c_void"
%typemap(cout) const int *                          "const int *"
%typemap(in) const int *                          "$1 = ($1_ltype)$input;"
%typemap(out) const int *                         "$result = $1;"

/* long* and const long* */
%typemap(rusttype) long *                           "*mut c_long"
%typemap(rsffitype) long *                          "*mut c_void"
%typemap(cout) long *                               "long *"
%typemap(in) long *                               "$1 = ($1_ltype)$input;"
%typemap(out) long *                              "$result = $1;"

%typemap(rusttype) const long *                     "*const c_long"
%typemap(rsffitype) const long *                    "*const c_void"
%typemap(cout) const long *                         "const long *"
%typemap(in) const long *                         "$1 = ($1_ltype)$input;"
%typemap(out) const long *                        "$result = $1;"

/* double* and const double* */
%typemap(rusttype) double *                         "*mut f64"
%typemap(rsffitype) double *                        "*mut c_void"
%typemap(cout) double *                             "double *"
%typemap(in) double *                             "$1 = ($1_ltype)$input;"
%typemap(out) double *                            "$result = $1;"

%typemap(rusttype) const double *                   "*const f64"
%typemap(rsffitype) const double *                  "*const c_void"
%typemap(cout) const double *                       "const double *"
%typemap(in) const double *                       "$1 = ($1_ltype)$input;"
%typemap(out) const double *                      "$result = $1;"

/* float* and const float* */
%typemap(rusttype) float *                          "*mut f32"
%typemap(rsffitype) float *                         "*mut c_void"
%typemap(cout) float *                              "float *"
%typemap(in) float *                              "$1 = ($1_ltype)$input;"
%typemap(out) float *                             "$result = $1;"

%typemap(rusttype) const float *                    "*const f32"
%typemap(rsffitype) const float *                   "*const c_void"
%typemap(cout) const float *                        "const float *"
%typemap(in) const float *                        "$1 = ($1_ltype)$input;"
%typemap(out) const float *                       "$result = $1;"

/* char arrays */
%typemap(rusttype) char [ANY]                       "[c_char; $1_dim0]"
%typemap(rsffitype) char [ANY]                      "*mut c_char"
%typemap(cout) char [ANY]                           "char *"
%typemap(in) char [ANY]                           "$1 = ($1_ltype)$input;"
%typemap(out) char [ANY]                          "$result = $1;"
%typemap(rsin) char [ANY]                          "$input.as_mut_ptr()"
%typemap(rsout) char [ANY]                         "$result"

/* -----------------------------------------------------------------------------
 * Enum types
 * 
 * Enums are handled specially in rust.cxx to generate proper type names.
 * The rusttype is generated in functionWrapper() using SwigType_base().
 * ----------------------------------------------------------------------------- */

%typemap(rusttype) enum SWIGTYPE                    "SWIGENUM"
%typemap(rsffitype) enum SWIGTYPE                   "c_int"
%typemap(cout) enum SWIGTYPE                        "int"
%typemap(in) enum SWIGTYPE                        "$1 = ($1_ltype)$input;"
%typemap(out) enum SWIGTYPE                       "$result = static_cast<int>($1);"

%typemap(rusttype) const enum SWIGTYPE &            "SWIGENUM"
%typemap(rsffitype) const enum SWIGTYPE &           "c_int"
%typemap(cout) const enum SWIGTYPE &                "int"
%typemap(in) const enum SWIGTYPE &                "$1 = ($1_ltype)$input;"
%typemap(out) const enum SWIGTYPE &               "$result = $1;"

/* -----------------------------------------------------------------------------
 * Member function pointer types (rarely used)
 * ----------------------------------------------------------------------------- */

%typemap(rusttype) SWIGTYPE (CLASS::*)              "usize"
%typemap(rsffitype) SWIGTYPE (CLASS::*)             "usize"
%typemap(cout) SWIGTYPE (CLASS::*)                  "void *"
%typemap(in) SWIGTYPE (CLASS::*)                  "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE (CLASS::*)                 "$result = $1;"

/* -----------------------------------------------------------------------------
 * Typecheck typemaps for overloaded functions
 * 
 * These are used to detect when multiple C++ types map to the same Rust type,
 * which would cause ambiguity in overloaded functions.
 * ----------------------------------------------------------------------------- */

%typecheck(SWIG_TYPECHECK_BOOL) bool, const bool & ""
%typecheck(SWIG_TYPECHECK_CHAR) char, const char &, unsigned char, const unsigned char & ""
%typecheck(SWIG_TYPECHECK_INT8) signed char, const signed char & ""
%typecheck(SWIG_TYPECHECK_INT16) short, const short & ""
%typecheck(SWIG_TYPECHECK_UINT16) unsigned short, const unsigned short & ""
%typecheck(SWIG_TYPECHECK_INT32) int, const int & ""
%typecheck(SWIG_TYPECHECK_UINT32) unsigned int, const unsigned int & ""
%typecheck(SWIG_TYPECHECK_INT64) long, const long &, long long, const long long & ""
%typecheck(SWIG_TYPECHECK_UINT64) unsigned long, const unsigned long &, unsigned long long, const unsigned long long & ""
%typecheck(SWIG_TYPECHECK_FLOAT) float, const float & ""
%typecheck(SWIG_TYPECHECK_DOUBLE) double, const double & ""
%typecheck(SWIG_TYPECHECK_STRING) char *, const char *, char *&, char[ANY], char[] ""
%typecheck(SWIG_TYPECHECK_POINTER) SWIGTYPE, SWIGTYPE *, SWIGTYPE &, SWIGTYPE &&, SWIGTYPE [], SWIGTYPE (CLASS::*) ""

/* -----------------------------------------------------------------------------
 * Apply typemap patterns
 * ----------------------------------------------------------------------------- */

%apply SWIGTYPE * { SWIGTYPE *const }
%apply SWIGTYPE & { SWIGTYPE *const& }
%apply SWIGTYPE (CLASS::*) { SWIGTYPE (CLASS::*const) }
%apply SWIGTYPE (CLASS::*) { SWIGTYPE (CLASS::*const&) }