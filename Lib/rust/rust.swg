/* -----------------------------------------------------------------------------
 * rust.swg
 *
 * Rust configuration module for SWIG.
 * ----------------------------------------------------------------------------- */

%include <rusttype.swg>
%include <rustrun.swg>
%include <rustexception.swg>

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

/* -----------------------------------------------------------------------------
 * Operator Renaming Macros
 * 
 * These macros rename C++ operators to valid Rust method names.
 * Usage: %rename_operators at the top of your interface file
 * ----------------------------------------------------------------------------- */

%define %rename_operators
%rename(op_add) operator+;
%rename(op_sub) operator-;
%rename(op_mul) operator*;
%rename(op_div) operator/;
%rename(op_mod) operator%;
%rename(op_eq) operator==;
%rename(op_ne) operator!=;
%rename(op_lt) operator<;
%rename(op_le) operator<=;
%rename(op_gt) operator>;
%rename(op_ge) operator>=;
%rename(op_neg) operator-();
%rename(op_not) operator!;
%rename(op_index) operator[];
%rename(op_add_assign) operator+=;
%rename(op_sub_assign) operator-=;
%rename(op_mul_assign) operator*=;
%rename(op_div_assign) operator/=;
%rename(op_bitand) operator&;
%rename(op_bitor) operator|;
%rename(op_bitxor) operator^;
%rename(op_shl) operator<<;
%rename(op_shr) operator>>;
%rename(op_bitnot) operator~;
%rename(op_deref) operator*;
%rename(op_call) operator();
%enddef
