/* -----------------------------------------------------------------------------
 * typemaps.i
 *
 * Rust pointer and reference handling typemap library.
 *
 * Provides INPUT, OUTPUT, INOUT typemaps for Rust.
 *
 * Design differences from other languages:
 * - Rust has two layers: FFI (rsffitype) and Safe Wrapper (rusttype)
 * - Ownership semantics: owned vs borrowed
 * - Option<T> for nullable pointers
 * ----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
 * INPUT_TYPEMAP macro
 *
 * Turns a pointer/reference into an input value.
 * In Rust, the value is passed directly, not by pointer.
 * ----------------------------------------------------------------------------- */
%define INPUT_TYPEMAP(TYPE, RUSTTYPE, RSFFITYPE)
// Rust user type
%typemap(rusttype) TYPE *INPUT, TYPE &INPUT "RUSTTYPE"

// FFI type (pass by value)
%typemap(rsffitype) TYPE *INPUT, TYPE &INPUT "RSFFITYPE"

// C wrapper takes pointer, but we pass by value
%typemap(cout) TYPE *INPUT, TYPE &INPUT "TYPE"

// Input conversion: take address of input
%typemap(in) TYPE *INPUT, TYPE &INPUT
%{ $1 = ($1_ltype)&$input; %}

// No output conversion needed
%typemap(out) TYPE *INPUT, TYPE &INPUT ""

// No freearg needed
%typemap(freearg) TYPE *INPUT, TYPE &INPUT ""

// No argout needed  
%typemap(argout) TYPE *INPUT, TYPE &INPUT ""

// Rust ownership: borrowed (we take a reference)
%typemap(rustownership) TYPE *INPUT, TYPE &INPUT "borrowed"

%enddef

/* Apply INPUT typemaps */
INPUT_TYPEMAP(bool, bool, c_uchar)
INPUT_TYPEMAP(signed char, i8, c_schar)
INPUT_TYPEMAP(unsigned char, u8, c_uchar)
INPUT_TYPEMAP(short, i16, c_short)
INPUT_TYPEMAP(unsigned short, u16, c_ushort)
INPUT_TYPEMAP(int, i32, c_int)
INPUT_TYPEMAP(unsigned int, u32, c_uint)
INPUT_TYPEMAP(long, i64, c_long)
INPUT_TYPEMAP(unsigned long, u64, c_ulong)
INPUT_TYPEMAP(long long, i64, c_longlong)
INPUT_TYPEMAP(unsigned long long, u64, c_ulonglong)
INPUT_TYPEMAP(float, f32, c_float)
INPUT_TYPEMAP(double, f64, c_double)

#undef INPUT_TYPEMAP

/* -----------------------------------------------------------------------------
 * OUTPUT_TYPEMAP macro
 *
 * Turns a pointer/reference into an output-only parameter.
 * In Rust, this returns the value (not a slice like Go).
 * ----------------------------------------------------------------------------- */
%define OUTPUT_TYPEMAP(TYPE, RUSTTYPE, RSFFITYPE)
// Rust user type - returns the value
%typemap(rusttype) TYPE *OUTPUT, TYPE &OUTPUT "RUSTTYPE"

// FFI type
%typemap(rsffitype) TYPE *OUTPUT, TYPE &OUTPUT "*mut RSFFITYPE"

// C wrapper output type
%typemap(cout) TYPE *OUTPUT, TYPE &OUTPUT "TYPE *"

// Input: allocate temp storage
%typemap(in, numinputs=0) TYPE *OUTPUT($*1_ltype temp), TYPE &OUTPUT($*1_ltype temp)
%{ $1 = &temp; %}

// Output: return the temp value
%typemap(out) TYPE *OUTPUT, TYPE &OUTPUT
%{ $result = *$1; %}

// No freearg needed
%typemap(freearg) TYPE *OUTPUT, TYPE &OUTPUT ""

// No argout needed (handled by out)
%typemap(argout) TYPE *OUTPUT, TYPE &OUTPUT ""

// Rust ownership: owned (we create a new value)
%typemap(rustownership) TYPE *OUTPUT, TYPE &OUTPUT "owned"

%enddef

/* Apply OUTPUT typemaps */
OUTPUT_TYPEMAP(bool, bool, c_uchar)
OUTPUT_TYPEMAP(signed char, i8, c_schar)
OUTPUT_TYPEMAP(unsigned char, u8, c_uchar)
OUTPUT_TYPEMAP(short, i16, c_short)
OUTPUT_TYPEMAP(unsigned short, u16, c_ushort)
OUTPUT_TYPEMAP(int, i32, c_int)
OUTPUT_TYPEMAP(unsigned int, u32, c_uint)
OUTPUT_TYPEMAP(long, i64, c_long)
OUTPUT_TYPEMAP(unsigned long, u64, c_ulong)
OUTPUT_TYPEMAP(long long, i64, c_longlong)
OUTPUT_TYPEMAP(unsigned long long, u64, c_ulonglong)
OUTPUT_TYPEMAP(float, f32, c_float)
OUTPUT_TYPEMAP(double, f64, c_double)

#undef OUTPUT_TYPEMAP

/* -----------------------------------------------------------------------------
 * INOUT_TYPEMAP macro
 *
 * Parameter is both input and output.
 * In Rust, we pass by mutable reference: &mut T
 * ----------------------------------------------------------------------------- */
%define INOUT_TYPEMAP(TYPE, RUSTTYPE, RSFFITYPE)
// Rust user type - mutable reference
%typemap(rusttype) TYPE *INOUT, TYPE &INOUT "&mut RUSTTYPE"

// FFI type - pointer
%typemap(rsffitype) TYPE *INOUT, TYPE &INOUT "*mut RSFFITYPE"

// C wrapper output type
%typemap(cout) TYPE *INOUT, TYPE &INOUT "TYPE *"

// Input: pass pointer directly
%typemap(in) TYPE *INOUT, TYPE &INOUT
%{ $1 = ($1_ltype)$input; %}

// Output: value is already in place
%typemap(out) TYPE *INOUT, TYPE &INOUT ""

// No freearg needed
%typemap(freearg) TYPE *INOUT, TYPE &INOUT ""

// No argout needed
%typemap(argout) TYPE *INOUT, TYPE &INOUT ""

// Rust ownership: borrowed mutably
%typemap(rustownership) TYPE *INOUT, TYPE &INOUT "borrowed_mut"

%enddef

/* Apply INOUT typemaps */
INOUT_TYPEMAP(bool, bool, c_uchar)
INOUT_TYPEMAP(signed char, i8, c_schar)
INOUT_TYPEMAP(unsigned char, u8, c_uchar)
INOUT_TYPEMAP(short, i16, c_short)
INOUT_TYPEMAP(unsigned short, u16, c_ushort)
INOUT_TYPEMAP(int, i32, c_int)
INOUT_TYPEMAP(unsigned int, u32, c_uint)
INOUT_TYPEMAP(long, i64, c_long)
INOUT_TYPEMAP(unsigned long, u64, c_ulong)
INOUT_TYPEMAP(long long, i64, c_longlong)
INOUT_TYPEMAP(unsigned long long, u64, c_ulonglong)
INOUT_TYPEMAP(float, f32, c_float)
INOUT_TYPEMAP(double, f64, c_double)

#undef INOUT_TYPEMAP

/* -----------------------------------------------------------------------------
 * Optional pointer typemaps
 *
 * Maps nullable C++ pointers to Rust Option<T>
 * ----------------------------------------------------------------------------- */
%define OPTIONAL_TYPEMAP(TYPE, RUSTTYPE, RSFFITYPE)
// Rust type - Option
%typemap(rusttype) TYPE *OPTIONAL "Option<RUSTTYPE>"

// FFI type - nullable pointer
%typemap(rsffitype) TYPE *OPTIONAL "*mut RSFFITYPE"

// C wrapper type
%typemap(cout) TYPE *OPTIONAL "TYPE *"

// Input: convert Option to pointer
%typemap(in) TYPE *OPTIONAL
%{ $1 = $input.is_some() ? ($1_ltype)$input.unwrap() : NULL; %}

// Output: convert pointer to Option
%typemap(out) TYPE *OPTIONAL
%{ $result = $1 ? Some(*$1) : None; %}

// Rust ownership: depends on context
%typemap(rustownership) TYPE *OPTIONAL "conditional"

%enddef

/* -----------------------------------------------------------------------------
 * String typemaps
 *
 * Special handling for C strings in Rust
 * ----------------------------------------------------------------------------- */

// const char* as &str (borrowed, no allocation)
%typemap(rusttype) const char *INPUT "&str"
%typemap(rsffitype) const char *INPUT "*const c_char"
%typemap(rustin) const char *INPUT "$input.as_ptr()"

// char* as String (owned, makes copy)
%typemap(rusttype) char *OUTPUT "String"
%typemap(rsffitype) char *OUTPUT "*mut c_char"
%typemap(rustout) char *OUTPUT "CStr::from_ptr($1).to_string_lossy().into_owned()"
%typemap(rustownership) char *OUTPUT "owned"

/* -----------------------------------------------------------------------------
 * Documentation
 *
 * Usage examples:
 *
 * // INPUT: pass by value instead of pointer
 * double fadd(double *INPUT, double *INPUT);
 * // Rust: fn fadd(a: f64, b: f64) -> f64
 *
 * // OUTPUT: return value instead of pointer param
 * void get_values(int *OUTPUT, int *OUTPUT);
 * // Rust: fn get_values() -> (i32, i32)
 *
 * // INOUT: mutable reference
 * void increment(int *INOUT);
 * // Rust: fn increment(val: &mut i32)
 *
 * // OPTIONAL: nullable pointer
 * void process(int *OPTIONAL);
 * // Rust: fn process(val: Option<i32>)
 * ----------------------------------------------------------------------------- */
