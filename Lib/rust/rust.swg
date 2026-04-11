/* -----------------------------------------------------------------------------
 * rust.swg
 *
 * Rust configuration module for SWIG.
 * ----------------------------------------------------------------------------- */

%include <rusttype.swg>
%include <rustrun.swg>

/* Code insertion directives for Rust */
#define %rust_import(...) %insert(rust_imports) %{__VA_ARGS__%}
#define %rust_code(...)   %insert(rust_code) %{__VA_ARGS__%}

/* -----------------------------------------------------------------------------
 * Default features for Rust
 * ----------------------------------------------------------------------------- */

// Use naturalvar by default for member variables
%feature("naturalvar");

// Enable const correctness
%feature("constcorrect");
