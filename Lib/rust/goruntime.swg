/* ------------------------------------------------------------
 * goruntime.swg
 *
 * Rust runtime code for the various generated files.
 * ------------------------------------------------------------ */

%inline %{
static void Swig_free(void* p) {
  free(p);
}

static void* Swig_malloc(int c) {
  return malloc(c);
}
%}

%insert(runtime) %{
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

%}

%insert(crust_comment_typedefs) %{
#include <stddef.h>
#include <stdint.h>
%}

#if SWIGRUST_INTRUST_SIZE == 32
%insert(runtime) %{
typedef int intrust;
typedef unsigned int uintrust;
%}
%insert(crust_comment_typedefs) %{
typedef int intrust;
typedef unsigned int uintrust;
%}
#elif SWIGRUST_INTRUST_SIZE == 64
%insert(runtime) %{
typedef long long intrust;
typedef unsigned long long uintrust;
%}
%insert(crust_comment_typedefs) %{
typedef long long intrust;
typedef unsigned long long uintrust;
%}
#else
%insert(runtime) %{
typedef ptrdiff_t intrust;
typedef size_t uintrust;
%}
%insert(crust_comment_typedefs) %{
typedef ptrdiff_t intrust;
typedef size_t uintrust;
%}
#endif

#ifndef SWIGGO_GCCGO
// Set the host compiler struct attribute that will be
// used to match gc's struct layout. For example, on 386 Windows,
// gcc wants to 8-align int64s, but gc does not.
// Use __gcc_struct__ to work around http://gcc.gnu.org/PR52991 on x86,
// and https://golang.org/issue/5603.
// See: https://github.com/golang/go/blob/fcbf04f9b93b4cd8addd05c2ed784118eb50a46c/src/cmd/cgo/out.go#L663
%insert(runtime) %{
# if !defined(__clang__) && (defined(__i386__) || defined(__x86_64__))
#   define SWIGSTRUCTPACKED __attribute__((__packed__, __gcc_struct__))
# else
#   define SWIGSTRUCTPACKED __attribute__((__packed__))
# endif
%}
#else
# define SWIGSTRUCTPACKED
#endif

%insert(runtime) %{

typedef struct { char *p; intrust n; } _gostring_;
typedef struct { void* array; intrust len; intrust cap; } _goslice_;

%}

%insert(crust_comment_typedefs) %{

typedef struct { char *p; intrust n; } _gostring_;
typedef struct { void* array; intrust len; intrust cap; } _goslice_;

%}

#ifdef SWIGGO_GCCGO

/* Boilerplate for C/C++ code when using gccgo.  */
%insert(runtime) %{
#define SWIGGO_GCCGO

#ifdef __cplusplus
extern "C" {
#endif
extern void *_cgo_allocate(size_t);
extern void _cgo_panic(const char *);
#ifdef __cplusplus
}
#endif

#define _swig_goallocate _cgo_allocate
#define _swig_rustpanic _cgo_panic
%}

#endif

#ifndef SWIGGO_GCCGO

// %rust_import("unsafe", _ "runtime/cgo")
%rust_import(std::ffi::CStr)
%rust_import(std::os::raw::c_void)

#else

// %rust_import("syscall", "unsafe")
%rust_import(std::ffi::CStr)

%insert(rust_header) %{

// type _ syscall.Sockaddr

%}

#endif
%insert(rust_header) %{
// it can be trait object since Rust 1.0
pub trait BaseCPtrTrait {
    fn get_c_ptr(&self) -> *mut c_void {
        self as *const Self as *const c_void as *mut c_void
    }
    fn change_base(&mut self, base: *mut c_void) {
        unimplemented!("change_base not implemented");
    }
}

fn get_swig_cptr(obj : &impl BaseCPtrTrait) -> *mut c_void {
    return obj.get_c_ptr();
}
%}


%insert(rust_header) %{

macro_rules! swig_conv {
    // 基础类型到基础类型 - 直接传值
    ($val:expr, i32) => { $val };
    ($val:expr, i64) => { $val };
    ($val:expr, f32) => { $val };
    ($val:expr, f64) => { $val };
    ($val:expr, bool) => { $val };
    ($val:expr, u8) => { $val };
    ($val:expr, u16) => { $val };
    ($val:expr, u32) => { $val };
    ($val:expr, u64) => { $val };
    ($val:expr, i8) => { $val };
    ($val:expr, i16) => { $val };
    ($val:expr, isize) => { $val };
    ($val:expr, usize) => { $val };
    
    // 指针相关转换
    ($val:expr, *const c_void) => { 
        swig_to_c_void_ptr($val)
    };
    ($val:expr, *mut c_void) => { 
        swig_to_c_void_ptr_mut($val)
    };
    
    // 从 c_void 指针转换到具体类型指针
    ($val:expr, *const $ty:ty) => { 
        $val as *const $ty
    };
    ($val:expr, *mut $ty:ty) => { 
        $val as *mut $ty
    };
    
    // 默认情况，尝试直接传值
    ($val:expr, $ty:ty) => { $val };
}

/// 辅助函数：将任意指针转换为 *const c_void
#[inline(always)]
fn swig_to_c_void_ptr<T>(ptr: *const T) -> *const c_void {
    ptr as *const c_void
}

/// 辅助函数：将任意可变指针转换为 *mut c_void
#[inline(always)]
fn swig_to_c_void_ptr_mut<T>(ptr: *mut T) -> *mut c_void {
    ptr as *mut c_void
}

/// 辅助函数：将引用转换为 *const c_void
#[inline(always)]
fn swig_ref_to_c_void_ptr<T>(r: &T) -> *const c_void {
    r as *const T as *const c_void
}

/// 辅助函数：将可变引用转换为 *mut c_void
#[inline(always)]
fn swig_ref_mut_to_c_void_ptr<T>(r: &mut T) -> *mut c_void {
    r as *mut T as *mut c_void
}

/// 专门处理 SWIG 结构体的转换宏
macro_rules! swig_struct_conv {
    // 不可变引用到 c_void 指针
    (&$val:expr) => {
        swig_ref_to_c_void_ptr(&$val)
    };
    // 可变引用到 c_void 指针
    (&mut $val:expr) => {
        swig_ref_mut_to_c_void_ptr($val)
    };
}


%}

/* Swig_always_false is used to conditionally assign parameters to
   Swig_escape_val so that the compiler thinks that they escape.  We
   only assign them if Swig_always_false is true, which it never is.
   We export the variable so that the compiler doesn't realize that it
   is never set. */
%insert(rust_header) %{
// var Swig_escape_always_false bool
// var Swig_escape_val interface{}
%}

/* Function pointers are translated by the code in go.cxx into
   _swig_fnptr.  Member pointers are translated to _swig_memberptr.  */

%insert(rust_header) %{
// type _swig_fnptr *byte
// type _swig_memberptr *byte
%}

/* Convert a Rust interface value into a C++ pointer.  */

//%insert(rust_header) %{
//fn getSwigcptr(v interface { Swigcptr() uintptr }) uintptr {
//	if v == nil {
//		return 0
//	}
//	return v.Swigcptr()
//}
//%}

/* For directors we need C++ to track a Rust pointer.  Since we can't
   pass a Rust pointer into C++, we use a map to track the pointers on
   the Rust side.  */

// %rust_import("sync")
%rust_import(std::convert::TryFrom)

%insert(rust_header) %{
use std::collections::HashMap;
use std::sync::{Arc, RwLock, Once};
use std::any::Any;
use std::ptr;
use std::error::Error;
use std::fmt;
use use std::ffi::CString;
type SwigRefType = i32;
type swig_intrust = i64;
%}



%insert(rust_director) %{



#[derive(Debug,Clone,Copy)]
pub enum DirectorError {
    NotFound(SwigRefType),
    InvalidPointer(SwigRefType),
    UseAfterFree(SwigRefType),
    AlreadyExists(SwigRefType)
}


impl fmt::Display for DirectorError {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        match self {
            DirectorError::NotFound(id) => {
                write!(f, "C++ director pointer {} not found", id)
            }
            DirectorError::InvalidPointer(id) => {
                write!(f, "C++ director pointer {} invalid (possible memory corruption)", id)
            }
            DirectorError::UseAfterFree(id) => {
                write!(f, "C++ director pointer {} not found (possible use-after-free)", id)
            }
            DirectorError::AlreadyExists(id) => {
                write!(f, "C++ director pointer {} already exists (possible too many objects existing)", id)
            }
        }
    }
}

impl Error for DirectorError {}



%}

