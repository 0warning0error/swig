/* ------------------------------------------------------------
 * gostring.swg
 *
 * Support for returning strings from C to Rust.
 * ------------------------------------------------------------ */

// C/C++ code to convert a memory buffer into a Rust string allocated in
// C/C++ memory.
%fragment("AllocateString", "runtime") %{
static _gostring_ Swig_AllocateString(const char *p, size_t l) {
  _gostring_ ret;
  ret.p = (char*)malloc(l);
  memcpy(ret.p, p, l);
  ret.n = l;
  return ret;
}
%}

// Rust code to convert a string allocated in C++ memory to one
// allocated in Rust memory.
%fragment("CopyString", "rust_runtime") %{
#[repr(C)]
struct swig_gostring { p : *mut c_void, n : i32 }

fn swigCopyString(s : *const c_void) -> String {
  let new_s = s as *const swig_gostring;
  
  let slice =  unsafe {std::slice::from_raw_parts((*new_s).p as *const u8, (*new_s).n as usize) };
  let r = unsafe {String::from_utf8_lossy(slice).to_string() }; 
  
  Swig_free((*new_s).p);
  return r;
}
%}
