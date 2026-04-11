/* -----------------------------------------------------------------------------
 * std_shared_ptr.i
 *
 * Rust language specific implementation of std::shared_ptr.
 *
 * Design:
 * - FFI layer: shared_ptr<T> is passed as *mut c_void (opaque handle)
 * - Safe layer: Wrapper struct with Arc-like semantics
 *
 * Usage:
 *   %include <rust/std_shared_ptr.i>
 *   %shared_ptr(MyClass)
 * ----------------------------------------------------------------------------- */

// Define namespace to std
#ifndef SWIG_SHARED_PTR_NAMESPACE
#define SWIG_SHARED_PTR_NAMESPACE std
#endif

#define SWIG_SHARED_PTR_QNAMESPACE SWIG_SHARED_PTR_NAMESPACE

// Declare the shared_ptr template
namespace SWIG_SHARED_PTR_NAMESPACE {
  template <class T> class shared_ptr {
  };
}

// Fragment for null deleter (used for non-owning references)
%fragment("SWIG_null_deleter", "header") {
struct SWIG_null_deleter {
  void operator() (void const *) const {}
};
%#define SWIG_NO_NULL_DELETER_0 , SWIG_null_deleter()
%#define SWIG_NO_NULL_DELETER_1
}

// Main macro for defining shared_ptr typemaps for a type
%define %shared_ptr(TYPE...)
%feature("smartptr", noblock=1) TYPE { SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< TYPE > }
SWIG_SHARED_PTR_TYPEMAPS(, TYPE)
SWIG_SHARED_PTR_TYPEMAPS(const, TYPE)
%enddef

// Rust-specific typemap implementation
%define SWIG_SHARED_PTR_TYPEMAPS(CONST, TYPE...)

// Mark TYPE as having natural variable handling
%naturalvar TYPE;
%naturalvar SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >;

// Destructor modification - delete the smart pointer
%feature("unref") TYPE "(void)arg1; delete smartarg1;"

// --- FFI layer types (cout, rsffitype) ---

// shared_ptr<T> by value
%typemap(cout) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > "void *"
%typemap(rsffitype) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > "*mut c_void"

// shared_ptr<T> by reference
%typemap(cout) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > & "void *"
%typemap(rsffitype) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > & "*mut c_void"

// shared_ptr<T> by pointer
%typemap(cout) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > * "void *"
%typemap(rsffitype) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > * "*mut c_void"

// Plain TYPE pointer (using shared_ptr internally)
%typemap(cout) CONST TYPE * "void *"
%typemap(rsffitype) CONST TYPE * "*mut c_void"

// Plain TYPE reference
%typemap(cout) CONST TYPE & "void *"
%typemap(rsffitype) CONST TYPE & "*mut c_void"

// --- Rust user-visible types (rusttype) ---

// shared_ptr<T> maps to the TYPE wrapper
%typemap(rusttype) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > "$typemap(rusttype, TYPE)"
%typemap(rusttype) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > & "$typemap(rusttype, TYPE)"
%typemap(rusttype) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > * "Option<$typemap(rusttype, TYPE)>"

// --- Input typemaps (in) ---

// shared_ptr<T> by value
%typemap(in) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >
%{ if ($input) $1 = *($&1_ltype)$input; %}

// shared_ptr<T> by reference
%typemap(in) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > & ($*1_ltype tempnull)
%{ $1 = $input ? ($1_ltype)$input : &tempnull; %}

// shared_ptr<T> by pointer
%typemap(in) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > * ($*1_ltype tempnull)
%{ $1 = $input ? ($1_ltype)$input : &tempnull; %}

// Plain TYPE pointer
%typemap(in) CONST TYPE * (SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *smartarg = 0)
%{
  smartarg = (SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *)$input;
  $1 = (TYPE *)(smartarg ? smartarg->get() : 0);
%}

// Plain TYPE reference
%typemap(in) CONST TYPE &
%{
  $1 = ($1_ltype)(((SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *)$input) ? 
       ((SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *)$input)->get() : 0);
  if (!$1) {
    SWIG_exception(SWIG_ValueError, "Reference is null");
  }
%}

// --- Output typemaps (out) ---

// shared_ptr<T> by value
%typemap(out) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >
%{ $result = $1 ? new $1_ltype($1) : 0; %}

// shared_ptr<T> by reference
%typemap(out) SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > &
%{ $result = *$1 ? new $*1_ltype(*$1) : 0; %}

// shared_ptr<T> by pointer
%typemap(out, fragment="SWIG_null_deleter") SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE > *
%{
  $result = ($1 && *$1) ? new $*1_ltype(*$1) : 0;
  if ($owner) delete $1;
%}

// Plain TYPE pointer
%typemap(out, fragment="SWIG_null_deleter") CONST TYPE *
%{ $result = $1 ? new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1 SWIG_NO_NULL_DELETER_$owner) : 0; %}

// Plain TYPE reference
%typemap(out, fragment="SWIG_null_deleter") CONST TYPE &
%{ $result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >($1 SWIG_NO_NULL_DELETER_$owner); %}

// Plain TYPE by value
%typemap(out) CONST TYPE
%{ $result = new SWIG_SHARED_PTR_QNAMESPACE::shared_ptr< CONST TYPE >(new $1_ltype($1)); %}

%enddef
