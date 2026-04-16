/* -----------------------------------------------------------------------------
 * std_unique_ptr.i
 *
 * Rust language specific implementation of std::unique_ptr.
 *
 * Design:
 * - FFI layer: unique_ptr<T> is passed as *mut c_void (opaque handle)
 * - Safe layer: Wrapper struct with ownership semantics
 * - Ownership is transferred when passed to Rust
 *
 * Usage:
 *   %include <rust/std_unique_ptr.i>
 *   %unique_ptr(MyClass)
 * ----------------------------------------------------------------------------- */

// Fragment for NoDelete wrapper (used for non-owning references)
%fragment("SwigNoDeleteUniquePtr", "header", fragment="<memory>") {
namespace swig {
  template<typename T>
  struct NoDeleteUniquePtr {
    std::unique_ptr<T> uptr;
    NoDeleteUniquePtr(T *p = 0) : uptr(p) {}
    ~NoDeleteUniquePtr() { uptr.release(); }
  };
}
}

// Declare the unique_ptr template
namespace std {
  template <class T> class unique_ptr {};
}

// Main macro for defining unique_ptr typemaps for a type
%define %unique_ptr(TYPE...)
SWIG_UNIQUE_PTR_TYPEMAPS(, TYPE)
SWIG_UNIQUE_PTR_TYPEMAPS(const, TYPE)
%enddef

// Rust-specific typemap implementation
%define SWIG_UNIQUE_PTR_TYPEMAPS(CONST, TYPE...)

// Mark TYPE as having natural variable handling
%naturalvar TYPE;
%naturalvar std::unique_ptr< CONST TYPE >;

// Destructor modification
%feature("unref") TYPE "(void)arg1; delete smartarg1;"

// --- FFI layer types (cout, rsffitype) ---

// unique_ptr<T> by value
%typemap(cout) std::unique_ptr< CONST TYPE > "void *"
%typemap(rsffitype) std::unique_ptr< CONST TYPE > "*mut c_void"

// unique_ptr<T> by reference (rvalue reference for move semantics)
%typemap(cout) std::unique_ptr< CONST TYPE > && "void *"
%typemap(rsffitype) std::unique_ptr< CONST TYPE > && "*mut c_void"

// Plain TYPE pointer (when ownership is transferred)
%typemap(cout) CONST TYPE * "void *"
%typemap(rsffitype) CONST TYPE * "*mut c_void"

// --- Rust user-visible types (rusttype) ---

// unique_ptr<T> maps to the TYPE wrapper (ownership transferred)
%typemap(rusttype) std::unique_ptr< CONST TYPE > "$typemap(rusttype, TYPE)"
%typemap(rusttype) std::unique_ptr< CONST TYPE > && "$typemap(rusttype, TYPE)"

// --- Input typemaps (in) ---

// unique_ptr<T> by value (ownership transferred from caller)
%typemap(in) std::unique_ptr< CONST TYPE >
%{
  if ($input) {
    $1 = std::move(*(std::unique_ptr< CONST TYPE > *)$input);
    delete (std::unique_ptr< CONST TYPE > *)$input;
  }
%}

// unique_ptr<T> by rvalue reference
%typemap(in) std::unique_ptr< CONST TYPE > &&
%{
  if ($input) {
    $1 = std::move(*(std::unique_ptr< CONST TYPE > *)$input);
    delete (std::unique_ptr< CONST TYPE > *)$input;
  }
%}

// Plain TYPE pointer (no ownership transfer by default)
%typemap(in) CONST TYPE * (std::unique_ptr< CONST TYPE > *smartarg = 0)
%{
  smartarg = (std::unique_ptr< CONST TYPE > *)$input;
  $1 = (TYPE *)(smartarg ? smartarg->get() : 0);
%}

// --- Output typemaps (out) ---

// unique_ptr<T> by value (ownership transferred to caller)
%typemap(out) std::unique_ptr< CONST TYPE >
%{ $result = $1 ? new std::unique_ptr< CONST TYPE >(std::move($1)) : 0; %}

// unique_ptr<T> by rvalue reference
%typemap(out) std::unique_ptr< CONST TYPE > &&
%{ $result = $1 ? new std::unique_ptr< CONST TYPE >(std::move($1)) : 0; %}

// Plain TYPE pointer (ownership transferred)
%typemap(out) CONST TYPE *
%{ $result = $1 ? new std::unique_ptr< CONST TYPE >($1) : 0; %}

// Plain TYPE by value (ownership implied)
%typemap(out) CONST TYPE
%{ $result = new std::unique_ptr< CONST TYPE >(new $1_ltype($1)); %}

%enddef
