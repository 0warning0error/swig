/* -----------------------------------------------------------------------------
 * rustrun.swg
 *
 * Rust runtime support for SWIG.
 * ----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
 * Runtime type information
 * 
 * For Rust, we don't need complex runtime type checking like scripting languages.
 * The type safety is enforced at compile time by Rust's type system.
 * ----------------------------------------------------------------------------- */

/* No runtime type checking needed for Rust - it's statically typed */

/* -----------------------------------------------------------------------------
 * Memory management helpers
 * 
 * These are generated as needed in the Rust output.
 * ----------------------------------------------------------------------------- */

/* Memory allocation/deallocation is handled via Rust's Drop trait */

/* -----------------------------------------------------------------------------
 * Exception handling
 * 
 * Map C++ exceptions to Rust panic or Result types.
 * ----------------------------------------------------------------------------- */

%typemap(throws) bool,
                 char,
                 signed char,
                 unsigned char,
                 short,
                 unsigned short,
                 int,
                 unsigned int,
                 long,
                 unsigned long,
                 long long,
                 unsigned long long,
                 float,
                 double
%{ panic!("C++ exception thrown"); %}

%typemap(throws) SWIGTYPE, SWIGTYPE &, SWIGTYPE &&, SWIGTYPE *, SWIGTYPE [], SWIGTYPE [ANY]
%{ panic!("C++ $1_type exception thrown"); %}

%typemap(throws) char *
%{ panic!("C++ exception: {}", $1); %}

/* -----------------------------------------------------------------------------
 * Director support
 * 
 * Directors allow Rust code to implement C++ virtual functions.
 * ----------------------------------------------------------------------------- */

/* Director base class helpers - these will be expanded for director classes */

/* -----------------------------------------------------------------------------
 * Smart pointer support
 * ----------------------------------------------------------------------------- */

/* std::shared_ptr support - will be expanded when used */
%typemap(rusttype) std::shared_ptr<SWIGTYPE> "std::shared_ptr<$1_type>"
%typemap(rsffitype) std::shared_ptr<SWIGTYPE> "*mut c_void"

/* std::unique_ptr support */
%typemap(rusttype) std::unique_ptr<SWIGTYPE> "$1_type"
%typemap(rsffitype) std::unique_ptr<SWIGTYPE> "*mut c_void"

/* -----------------------------------------------------------------------------
 * Standard library types
 * ----------------------------------------------------------------------------- */

/* Note: std::string typemaps are defined in std_string.i
 * Do not define them here to avoid conflicts.
 * Users should %include <rust/std_string.i> for proper std::string support.
 */

/* std::vector - simplified, will need more detailed implementation */
%typemap(rusttype) std::vector<SWIGTYPE> "Vec<$1_type>"
%typemap(rsffitype) std::vector<SWIGTYPE> "*mut c_void"

/* -----------------------------------------------------------------------------
 * Const correctness
 * ----------------------------------------------------------------------------- */

/* These help preserve const semantics in Rust */
%apply const SWIGTYPE & { const SWIGTYPE & }

/* -----------------------------------------------------------------------------
 * Rust-specific attributes
 * ----------------------------------------------------------------------------- */

/* Allow non_snake_case for FFI functions - these attributes are added
   automatically by the Rust module in rust.cxx */
