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

/* long */
%typemap(rusttype) long, const long &               "i64"
%typemap(rsffitype) long, const long &              "c_long"
%typemap(cout) long                                 "long"
%typemap(in) long                                 "$1 = ($1_ltype)$input;"
%typemap(out) long                                "$result = $1;"

/* unsigned long */
%typemap(rusttype) unsigned long, const unsigned long & "u64"
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

/* size_t */
%typemap(rusttype) size_t, const size_t &           "usize"
%typemap(rsffitype) size_t, const size_t &          "c_ulong"
%typemap(cout) size_t                               "size_t"
%typemap(in) size_t                               "$1 = ($1_ltype)$input;"
%typemap(out) size_t                              "$result = $1;"

/* ptrdiff_t */
%typemap(rusttype) ptrdiff_t, const ptrdiff_t &     "isize"
%typemap(rsffitype) ptrdiff_t, const ptrdiff_t &    "c_long"
%typemap(cout) ptrdiff_t                            "ptrdiff_t"
%typemap(in) ptrdiff_t                            "$1 = ($1_ltype)$input;"
%typemap(out) ptrdiff_t                           "$result = $1;"

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
 * ----------------------------------------------------------------------------- */

%typemap(rusttype) SWIGTYPE &                       "&mut $*1_type"
%typemap(rsffitype) SWIGTYPE &                      "*mut c_void"
%typemap(cout) SWIGTYPE &                           "void *"
%typemap(in) SWIGTYPE &                           "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE &                          "$result = $1;"

%typemap(rusttype) const SWIGTYPE &                 "&$*1_type"
%typemap(rsffitype) const SWIGTYPE &                "*const c_void"
%typemap(cout) const SWIGTYPE &                     "const void *"
%typemap(in) const SWIGTYPE &                     "$1 = ($1_ltype)$input;"
%typemap(out) const SWIGTYPE &                    "$result = $1;"

/* Rvalue reference (C++11) */
%typemap(rusttype) SWIGTYPE &&                      "$*1_type"
%typemap(rsffitype) SWIGTYPE &&                     "*mut c_void"
%typemap(cout) SWIGTYPE &&                          "void *"
%typemap(in) SWIGTYPE &&                          "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE &&                         "$result = $1;"

/* -----------------------------------------------------------------------------
 * Array types
 * ----------------------------------------------------------------------------- */

%typemap(rusttype) SWIGTYPE []                      "&[$1_type]"
%typemap(rsffitype) SWIGTYPE []                     "*mut c_void"
%typemap(cout) SWIGTYPE []                          "void *"
%typemap(in) SWIGTYPE []                          "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE []                         "$result = $1;"

%typemap(rusttype) SWIGTYPE [ANY]                   "&[$1_type; $1_dim0]"
%typemap(rsffitype) SWIGTYPE [ANY]                  "*mut c_void"
%typemap(cout) SWIGTYPE [ANY]                       "void *"
%typemap(in) SWIGTYPE [ANY]                       "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE [ANY]                      "$result = $1;"

/* -----------------------------------------------------------------------------
 * Value types (passed by value)
 * For return by value, we need to allocate on heap to return a pointer.
 * ----------------------------------------------------------------------------- */

%typemap(rusttype) SWIGTYPE                         "$&1_type"
%typemap(rsffitype) SWIGTYPE                        "*mut c_void"
%typemap(cout) SWIGTYPE                             "void *"
%typemap(in) SWIGTYPE                             "$1 = ($1_ltype)$input;"
%typemap(out) SWIGTYPE                            "$result = (void *)new $1_type($1);"

/* -----------------------------------------------------------------------------
 * String types
 * ----------------------------------------------------------------------------- */

/* char* as string */
%typemap(rusttype) char *                           "String"
%typemap(rsffitype) char *                          "*mut c_char"
%typemap(cout) char *                               "char *"
%typemap(in) char *                               "$1 = ($1_ltype)$input;"
%typemap(out) char *, char *&                     "$result = $1;"

/* const char* as &str */
%typemap(rusttype) const char *                     "&str"
%typemap(rsffitype) const char *                    "*const c_char"
%typemap(cout) const char *                         "const char *"
%typemap(in) const char *                         "$1 = ($1_ltype)$input;"
%typemap(out) const char *                        "$result = $1;"

/* char arrays */
%typemap(rusttype) char [ANY]                       "[c_char; $1_dim0]"
%typemap(rsffitype) char [ANY]                      "*mut c_char"
%typemap(cout) char [ANY]                           "char *"
%typemap(in) char [ANY]                           "$1 = ($1_ltype)$input;"
%typemap(out) char [ANY]                          "$result = $1;"

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
