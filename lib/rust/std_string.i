/* -----------------------------------------------------------------------------
 * std_string.i
 *
 * SWIG typemaps for std::string
 * Rust implementation with SwigString wrapper
 *
 * Design:
 *   - std::string is wrapped as SwigString (struct holding C++ pointer)
 *   - FFI layer uses opaque pointer (*mut c_void)
 *   - Provides From/Into conversions with Rust String
 *   - Memory management: SwigString owns the std::string and frees it on Drop
 *
 * Typemaps used:
 *   rusttype   - Rust user-visible type (SwigString)
 *   rsffitype  - Rust FFI type (*mut c_void)
 *   rsin       - How to pass Rust param to FFI (uses .ptr field)
 *   rsout      - How to wrap FFI return to Rust (constructs SwigString)
 *   in/out     - C layer conversions
 *
 * Usage:
 *   %include <rust/std_string.i>
 *   
 *   // In Rust:
 *   let s = SwigString::from_str("hello");   // From &str
 *   let s: SwigString = "hello".into();      // From String
 *   let rust_str: String = s.into();         // To String
 * ----------------------------------------------------------------------------- */

%{
#include <string>
#include <cstring>
#include <cstdlib>
%}

/* -----------------------------------------------------------------------------
 * C++ helper functions for SwigString FFI
 * These are exported as extern "C" functions for Rust to call
 * ----------------------------------------------------------------------------- */

%{
extern "C" {

// Create an empty std::string
SWIGEXPORT void* SwigString_new() {
    return new std::string();
}

// Create std::string from C string and length
SWIGEXPORT void* SwigString_from_bytes(const char* data, size_t len) {
    if (data && len > 0) {
        return new std::string(data, len);
    }
    return new std::string();
}

// Create std::string from null-terminated C string
SWIGEXPORT void* SwigString_from_c_str(const char* s) {
    if (s) {
        return new std::string(s);
    }
    return new std::string();
}

// Delete a std::string
SWIGEXPORT void SwigString_delete(void* ptr) {
    if (ptr) {
        delete reinterpret_cast<std::string*>(ptr);
    }
}

// Get C string pointer (null-terminated)
SWIGEXPORT const char* SwigString_c_str(const void* ptr) {
    if (ptr) {
        return reinterpret_cast<const std::string*>(ptr)->c_str();
    }
    return "";
}

// Get string data pointer (may not be null-terminated)
SWIGEXPORT const char* SwigString_data(const void* ptr) {
    if (ptr) {
        return reinterpret_cast<const std::string*>(ptr)->data();
    }
    return nullptr;
}

// Get string length
SWIGEXPORT size_t SwigString_len(const void* ptr) {
    if (ptr) {
        return reinterpret_cast<const std::string*>(ptr)->size();
    }
    return 0;
}

// Check if empty
SWIGEXPORT int SwigString_is_empty(const void* ptr) {
    if (ptr) {
        return reinterpret_cast<const std::string*>(ptr)->empty() ? 1 : 0;
    }
    return 1;
}

// Clear the string
SWIGEXPORT void SwigString_clear(void* ptr) {
    if (ptr) {
        reinterpret_cast<std::string*>(ptr)->clear();
    }
}

// Append bytes
SWIGEXPORT void SwigString_append(void* ptr, const char* data, size_t len) {
    if (ptr && data && len > 0) {
        reinterpret_cast<std::string*>(ptr)->append(data, len);
    }
}

// Append C string
SWIGEXPORT void SwigString_append_c_str(void* ptr, const char* s) {
    if (ptr && s) {
        reinterpret_cast<std::string*>(ptr)->append(s);
    }
}

// Assign from bytes
SWIGEXPORT void SwigString_assign(void* ptr, const char* data, size_t len) {
    if (ptr) {
        if (data && len > 0) {
            reinterpret_cast<std::string*>(ptr)->assign(data, len);
        } else {
            reinterpret_cast<std::string*>(ptr)->clear();
        }
    }
}

// Get capacity
SWIGEXPORT size_t SwigString_capacity(const void* ptr) {
    if (ptr) {
        return reinterpret_cast<const std::string*>(ptr)->capacity();
    }
    return 0;
}

// Reserve capacity
SWIGEXPORT void SwigString_reserve(void* ptr, size_t capacity) {
    if (ptr) {
        reinterpret_cast<std::string*>(ptr)->reserve(capacity);
    }
}

// Compare two strings
SWIGEXPORT int SwigString_compare(const void* ptr1, const void* ptr2) {
    if (!ptr1 && !ptr2) return 0;
    if (!ptr1) return -1;
    if (!ptr2) return 1;
    const std::string* s1 = reinterpret_cast<const std::string*>(ptr1);
    const std::string* s2 = reinterpret_cast<const std::string*>(ptr2);
    return s1->compare(*s2);
}

// Clone a string
SWIGEXPORT void* SwigString_clone(const void* ptr) {
    if (ptr) {
        return new std::string(*reinterpret_cast<const std::string*>(ptr));
    }
    return new std::string();
}

// Resize string
SWIGEXPORT void SwigString_resize(void* ptr, size_t new_len, char fill_char) {
    if (ptr) {
        reinterpret_cast<std::string*>(ptr)->resize(new_len, fill_char);
    }
}

// Get substring
SWIGEXPORT void* SwigString_substr(const void* ptr, size_t pos, size_t len) {
    if (ptr) {
        const std::string* s = reinterpret_cast<const std::string*>(ptr);
        if (pos <= s->size()) {
            return new std::string(s->substr(pos, len));
        }
    }
    return new std::string();
}

// Find substring
SWIGEXPORT long SwigString_find(const void* ptr, const char* needle, size_t pos) {
    if (ptr && needle) {
        const std::string* s = reinterpret_cast<const std::string*>(ptr);
        size_t result = s->find(needle, pos);
        return (result == std::string::npos) ? -1 : static_cast<long>(result);
    }
    return -1;
}

// Reverse find
SWIGEXPORT long SwigString_rfind(const void* ptr, const char* needle, size_t pos) {
    if (ptr && needle) {
        const std::string* s = reinterpret_cast<const std::string*>(ptr);
        size_t result = s->rfind(needle, pos);
        return (result == std::string::npos) ? -1 : static_cast<long>(result);
    }
    return -1;
}

// Push back a single character
SWIGEXPORT void SwigString_push_back(void* ptr, char c) {
    if (ptr) {
        reinterpret_cast<std::string*>(ptr)->push_back(c);
    }
}

// Pop back a character
SWIGEXPORT int SwigString_pop_back(void* ptr) {
    if (ptr && !reinterpret_cast<std::string*>(ptr)->empty()) {
        reinterpret_cast<std::string*>(ptr)->pop_back();
        return 1;
    }
    return 0;
}

// Get character at index
SWIGEXPORT char SwigString_at(const void* ptr, size_t index) {
    if (ptr) {
        const std::string* s = reinterpret_cast<const std::string*>(ptr);
        if (index < s->size()) {
            return (*s)[index];
        }
    }
    return '\0';
}

// Set character at index
SWIGEXPORT void SwigString_set_at(void* ptr, size_t index, char c) {
    if (ptr) {
        std::string* s = reinterpret_cast<std::string*>(ptr);
        if (index < s->size()) {
            (*s)[index] = c;
        }
    }
}

// ============================================================================
// Advanced string methods
// ============================================================================

// Insert a C string at position
SWIGEXPORT void SwigString_insert(void* ptr, size_t pos, const char* data, size_t len) {
    if (ptr && data && len > 0) {
        std::string* s = reinterpret_cast<std::string*>(ptr);
        if (pos <= s->size()) {
            s->insert(pos, data, len);
        }
    }
}

// Insert a single character at position (count times)
SWIGEXPORT void SwigString_insert_char(void* ptr, size_t pos, size_t count, char c) {
    if (ptr) {
        std::string* s = reinterpret_cast<std::string*>(ptr);
        if (pos <= s->size()) {
            s->insert(pos, count, c);
        }
    }
}

// Erase characters from position
SWIGEXPORT void SwigString_erase(void* ptr, size_t pos, size_t len) {
    if (ptr) {
        std::string* s = reinterpret_cast<std::string*>(ptr);
        if (pos < s->size()) {
            s->erase(pos, len);
        }
    }
}

// Replace characters at position with C string
SWIGEXPORT void SwigString_replace(void* ptr, size_t pos, size_t len, const char* data, size_t data_len) {
    if (ptr && data && data_len > 0) {
        std::string* s = reinterpret_cast<std::string*>(ptr);
        if (pos <= s->size()) {
            s->replace(pos, len, data, data_len);
        }
    }
}

// Find first occurrence of any character from the set
SWIGEXPORT long SwigString_find_first_of(const void* ptr, const char* chars, size_t pos) {
    if (ptr && chars) {
        const std::string* s = reinterpret_cast<const std::string*>(ptr);
        size_t result = s->find_first_of(chars, pos);
        return (result == std::string::npos) ? -1 : static_cast<long>(result);
    }
    return -1;
}

// Find last occurrence of any character from the set
SWIGEXPORT long SwigString_find_last_of(const void* ptr, const char* chars, size_t pos) {
    if (ptr && chars) {
        const std::string* s = reinterpret_cast<const std::string*>(ptr);
        size_t result = s->find_last_of(chars, pos);
        return (result == std::string::npos) ? -1 : static_cast<long>(result);
    }
    return -1;
}

// Find first character NOT in the set
SWIGEXPORT long SwigString_find_first_not_of(const void* ptr, const char* chars, size_t pos) {
    if (ptr && chars) {
        const std::string* s = reinterpret_cast<const std::string*>(ptr);
        size_t result = s->find_first_not_of(chars, pos);
        return (result == std::string::npos) ? -1 : static_cast<long>(result);
    }
    return -1;
}

// Find last character NOT in the set
SWIGEXPORT long SwigString_find_last_not_of(const void* ptr, const char* chars, size_t pos) {
    if (ptr && chars) {
        const std::string* s = reinterpret_cast<const std::string*>(ptr);
        size_t result = s->find_last_not_of(chars, pos);
        return (result == std::string::npos) ? -1 : static_cast<long>(result);
    }
    return -1;
}

// Shrink capacity to fit size
SWIGEXPORT void SwigString_shrink_to_fit(void* ptr) {
    if (ptr) {
        reinterpret_cast<std::string*>(ptr)->shrink_to_fit();
    }
}

// Get front character
SWIGEXPORT char SwigString_front(const void* ptr) {
    if (ptr) {
        const std::string* s = reinterpret_cast<const std::string*>(ptr);
        if (!s->empty()) {
            return s->front();
        }
    }
    return '\0';
}

// Get back character
SWIGEXPORT char SwigString_back(const void* ptr) {
    if (ptr) {
        const std::string* s = reinterpret_cast<const std::string*>(ptr);
        if (!s->empty()) {
            return s->back();
        }
    }
    return '\0';
}

// Swap contents with another string
SWIGEXPORT void SwigString_swap(void* ptr1, void* ptr2) {
    if (ptr1 && ptr2) {
        std::string* s1 = reinterpret_cast<std::string*>(ptr1);
        std::string* s2 = reinterpret_cast<std::string*>(ptr2);
        s1->swap(*s2);
    }
}

} // extern "C"
%}

namespace std {

%naturalvar string;

class string;

/* -----------------------------------------------------------------------------
 * Type mappings for std::string (by value)
 * ----------------------------------------------------------------------------- */

%typemap(ctype) std::string "void *"
%typemap(rusttype) std::string "SwigString"
%typemap(rsffitype) std::string "*mut std::ffi::c_void"

// Rust input: use .ptr field to get the FFI pointer
%typemap(rsin) std::string "$input.ptr"

// Rust output: wrap the FFI pointer in SwigString
%typemap(rsout) std::string "SwigString { ptr: $result }"

// C input: copy from the pointer
%typemap(in, canthrow=1) std::string
%{ $1 = *reinterpret_cast<std::string*>($input); %}

// C output: allocate new string and return pointer
%typemap(out) std::string
%{ $result = reinterpret_cast<void*>(new std::string($1)); %}

%typemap(directorin) std::string
%{ $input = reinterpret_cast<void*>(new std::string($1)); %}

%typemap(directorout, canthrow=1) std::string
%{ $result = *reinterpret_cast<std::string*>($input); %}

%typemap(typecheck) std::string = SWIGTYPE;

/* -----------------------------------------------------------------------------
 * Type mappings for const std::string& (by const reference)
 * ----------------------------------------------------------------------------- */

%typemap(ctype) const std::string & "const void *"
%typemap(rusttype) const std::string & "&SwigString"
%typemap(rsffitype) const std::string & "*const std::ffi::c_void"

// Rust input: use .ptr field
%typemap(rsin) const std::string & "$input.ptr"

// Rust output: wrap the pointer
%typemap(rsout) const std::string & "SwigString { ptr: unsafe { swig_string_ffi::SwigString_clone($result) } }"

%typemap(in, canthrow=1) const std::string &
%{ $*1_ltype $1_str = *reinterpret_cast<const std::string*>($input);
   $1 = &$1_str; %}

%typemap(out) const std::string &
%{ $result = reinterpret_cast<const void*>($1); %}

%typemap(directorin) const std::string &
%{ $input = reinterpret_cast<const void*>($1); %}

%typemap(directorout, warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const std::string &
%{ static std::string $1_str;
   $1_str = *reinterpret_cast<const std::string*>($input);
   $result = &$1_str; %}

%typemap(typecheck) const std::string & = SWIGTYPE;

/* -----------------------------------------------------------------------------
 * Type mappings for std::string& (by mutable reference)
 *
 * Note: For return types, we return owned SwigString instead of &mut SwigString
 * because Rust lifetimes cannot track C++ object lifetime. The caller should
 * clone the string if they need to modify it independently.
 * ----------------------------------------------------------------------------- */

%typemap(ctype) std::string & "void *"
// For input parameters: use &SwigString (immutable reference is safer)
// For return types: use SwigString (owned, cloned from the reference)
// Note: SWIG uses the same typemap for both, so we use SwigString for both
// to avoid lifetime issues with return types. Input params can still use &SwigString
// via the rsin typemap.
%typemap(rusttype) std::string & "SwigString"
%typemap(rsffitype) std::string & "*mut std::ffi::c_void"

%typemap(rsin) std::string & "$input.ptr"
// For return: clone the string to avoid lifetime issues with C++ references
%typemap(rsout) std::string & "SwigString { ptr: unsafe { swig_string_ffi::SwigString_clone($result as *const std::ffi::c_void) } }"

%typemap(in) std::string &
%{ $1 = reinterpret_cast<std::string*>($input); %}

%typemap(out) std::string &
%{ $result = reinterpret_cast<void*>($1); %}

%typemap(typecheck) std::string & = SWIGTYPE;

/* -----------------------------------------------------------------------------
 * Type mappings for std::string* (pointer)
 *
 * std::string* maps to Option<SwigString> where:
 * - null pointer -> None
 * - valid pointer -> Some(SwigString { ptr: ... })
 *
 * Note: The rsout typemap uses a special format that rust.cxx understands
 * to properly handle Option types without double-calling FFI functions.
 * ----------------------------------------------------------------------------- */

%typemap(ctype) std::string * "void *"
%typemap(rusttype) std::string * "Option<SwigString>"
%typemap(rsffitype) std::string * "*mut std::ffi::c_void"

%typemap(rsin) std::string * "$input.map(|s| s.ptr).unwrap_or(std::ptr::null_mut())"
// For return: use SWIG_OPTIONAL_PTR marker for rust.cxx to handle properly
// The code generator will: 1) call FFI once, 2) check for null, 3) wrap in Option
%typemap(rsout) std::string * "SWIG_OPTIONAL_PTR(SwigString, $result)"

%typemap(in) std::string *
%{ $1 = reinterpret_cast<std::string*>($input); %}

%typemap(out) std::string *
%{ $result = reinterpret_cast<void*>($input); %}

%typemap(typecheck) std::string * = SWIGTYPE;

/* -----------------------------------------------------------------------------
 * Type mappings for const std::string* (const pointer)
 *
 * const std::string* maps to Option<SwigString> for return types
 * to avoid lifetime issues with C++ references.
 * ----------------------------------------------------------------------------- */

%typemap(ctype) const std::string * "const void *"
%typemap(rusttype) const std::string * "Option<SwigString>"
%typemap(rsffitype) const std::string * "*const std::ffi::c_void"

%typemap(rsin) const std::string * "$input.map(|s| s.ptr as *const std::ffi::c_void).unwrap_or(std::ptr::null())"
// For return: use SWIG_OPTIONAL_PTR marker for rust.cxx to handle properly
%typemap(rsout) const std::string * "SWIG_OPTIONAL_PTR(SwigString, $result)"

%typemap(in) const std::string *
%{ $1 = reinterpret_cast<const std::string*>($input); %}

%typemap(out) const std::string *
%{ $result = reinterpret_cast<const void*>($input); %}

%typemap(typecheck) const std::string * = SWIGTYPE;

/* -----------------------------------------------------------------------------
 * Exception handling
 * ----------------------------------------------------------------------------- */

%typemap(throws, canthrow=1) std::string
%{ throw std::runtime_error($1.c_str()); %}

%typemap(throws, canthrow=1) const std::string &
%{ throw std::runtime_error($1->c_str()); %}

} // namespace std

/* -----------------------------------------------------------------------------
 * SwigString type definition and trait implementations
 * 
 * This is inserted into the generated Rust code via the "rustcode" section.
 * ----------------------------------------------------------------------------- */

%insert("rustcode") %{
/// Wrapper for C++ std::string
/// 
/// This type provides a safe wrapper around C++ std::string and supports
/// bidirectional conversion between C++ std::string and Rust's String type.
/// 
/// # Memory Management
/// 
/// SwigString owns the underlying C++ std::string and will free it when dropped.
/// 
/// # Example
/// 
/// ```
/// let s = SwigString::from_str("hello");
/// assert_eq!(s.len(), 5);
/// let rust_str: String = s.into();
/// assert_eq!(rust_str, "hello");
/// ```
#[repr(C)]
pub struct SwigString {
    ptr: *mut std::ffi::c_void,
}

// FFI declarations for SwigString helper functions
mod swig_string_ffi {
    extern "C" {
        pub fn SwigString_new() -> *mut std::ffi::c_void;
        pub fn SwigString_from_bytes(data: *const std::os::raw::c_char, len: usize) -> *mut std::ffi::c_void;
        pub fn SwigString_from_c_str(s: *const std::os::raw::c_char) -> *mut std::ffi::c_void;
        pub fn SwigString_delete(ptr: *mut std::ffi::c_void);
        pub fn SwigString_c_str(ptr: *const std::ffi::c_void) -> *const std::os::raw::c_char;
        pub fn SwigString_data(ptr: *const std::ffi::c_void) -> *const std::os::raw::c_char;
        pub fn SwigString_len(ptr: *const std::ffi::c_void) -> usize;
        pub fn SwigString_is_empty(ptr: *const std::ffi::c_void) -> std::os::raw::c_int;
        pub fn SwigString_clear(ptr: *mut std::ffi::c_void);
        pub fn SwigString_append(ptr: *mut std::ffi::c_void, data: *const std::os::raw::c_char, len: usize);
        pub fn SwigString_append_c_str(ptr: *mut std::ffi::c_void, s: *const std::os::raw::c_char);
        pub fn SwigString_assign(ptr: *mut std::ffi::c_void, data: *const std::os::raw::c_char, len: usize);
        pub fn SwigString_capacity(ptr: *const std::ffi::c_void) -> usize;
        pub fn SwigString_reserve(ptr: *mut std::ffi::c_void, capacity: usize);
        pub fn SwigString_compare(ptr1: *const std::ffi::c_void, ptr2: *const std::ffi::c_void) -> std::os::raw::c_int;
        pub fn SwigString_clone(ptr: *const std::ffi::c_void) -> *mut std::ffi::c_void;
        pub fn SwigString_resize(ptr: *mut std::ffi::c_void, new_len: usize, fill_char: std::os::raw::c_char);
        pub fn SwigString_substr(ptr: *const std::ffi::c_void, pos: usize, len: usize) -> *mut std::ffi::c_void;
        pub fn SwigString_find(ptr: *const std::ffi::c_void, needle: *const std::os::raw::c_char, pos: usize) -> std::os::raw::c_long;
        pub fn SwigString_rfind(ptr: *const std::ffi::c_void, needle: *const std::os::raw::c_char, pos: usize) -> std::os::raw::c_long;
        pub fn SwigString_push_back(ptr: *mut std::ffi::c_void, c: std::os::raw::c_char);
        pub fn SwigString_pop_back(ptr: *mut std::ffi::c_void) -> std::os::raw::c_int;
        pub fn SwigString_at(ptr: *const std::ffi::c_void, index: usize) -> std::os::raw::c_char;
        pub fn SwigString_set_at(ptr: *mut std::ffi::c_void, index: usize, c: std::os::raw::c_char);
        // Advanced methods
        pub fn SwigString_insert(ptr: *mut std::ffi::c_void, pos: usize, data: *const std::os::raw::c_char, len: usize);
        pub fn SwigString_insert_char(ptr: *mut std::ffi::c_void, pos: usize, count: usize, c: std::os::raw::c_char);
        pub fn SwigString_erase(ptr: *mut std::ffi::c_void, pos: usize, len: usize);
        pub fn SwigString_replace(ptr: *mut std::ffi::c_void, pos: usize, len: usize, data: *const std::os::raw::c_char, data_len: usize);
        pub fn SwigString_find_first_of(ptr: *const std::ffi::c_void, chars: *const std::os::raw::c_char, pos: usize) -> std::os::raw::c_long;
        pub fn SwigString_find_last_of(ptr: *const std::ffi::c_void, chars: *const std::os::raw::c_char, pos: usize) -> std::os::raw::c_long;
        pub fn SwigString_find_first_not_of(ptr: *const std::ffi::c_void, chars: *const std::os::raw::c_char, pos: usize) -> std::os::raw::c_long;
        pub fn SwigString_find_last_not_of(ptr: *const std::ffi::c_void, chars: *const std::os::raw::c_char, pos: usize) -> std::os::raw::c_long;
        pub fn SwigString_shrink_to_fit(ptr: *mut std::ffi::c_void);
        pub fn SwigString_front(ptr: *const std::ffi::c_void) -> std::os::raw::c_char;
        pub fn SwigString_back(ptr: *const std::ffi::c_void) -> std::os::raw::c_char;
        pub fn SwigString_swap(ptr1: *mut std::ffi::c_void, ptr2: *mut std::ffi::c_void);
    }
}

impl SwigString {
    /// Special value indicating "until end of string" or "not found"
    pub const NPOS: usize = usize::MAX;
    
    /// Create an empty SwigString
    pub fn new() -> Self {
        unsafe {
            Self { ptr: swig_string_ffi::SwigString_new() }
        }
    }
    
    /// Create from a Rust &str
    pub fn from_str(s: &str) -> Self {
        unsafe {
            Self { 
                ptr: swig_string_ffi::SwigString_from_bytes(
                    s.as_ptr() as *const std::os::raw::c_char, 
                    s.len()
                ) 
            }
        }
    }
    
    /// Create from bytes (may not be valid UTF-8)
    pub fn from_bytes(bytes: &[u8]) -> Self {
        unsafe {
            Self { 
                ptr: swig_string_ffi::SwigString_from_bytes(
                    bytes.as_ptr() as *const std::os::raw::c_char, 
                    bytes.len()
                ) 
            }
        }
    }
    
    /// Get as C string pointer (null-terminated)
    pub fn as_c_str(&self) -> *const std::os::raw::c_char {
        unsafe { swig_string_ffi::SwigString_c_str(self.ptr) }
    }
    
    /// Get string length
    pub fn len(&self) -> usize {
        unsafe { swig_string_ffi::SwigString_len(self.ptr) }
    }
    
    /// Check if empty
    pub fn is_empty(&self) -> bool {
        unsafe { swig_string_ffi::SwigString_is_empty(self.ptr) != 0 }
    }
    
    /// Convert to Rust String (owned)
    pub fn into_string(self) -> String {
        let bytes = self.as_bytes();
        let bytes = bytes.to_vec();
        std::mem::forget(self);
        String::from_utf8_lossy(&bytes).into_owned()
    }
    
    /// Convert to Rust String with lossy UTF-8 conversion
    pub fn to_string_lossy(&self) -> String {
        String::from_utf8_lossy(self.as_bytes()).into_owned()
    }
    
    /// Get the bytes as a slice
    pub fn as_bytes(&self) -> &[u8] {
        unsafe {
            let data = swig_string_ffi::SwigString_data(self.ptr);
            let len = swig_string_ffi::SwigString_len(self.ptr);
            if data.is_null() || len == 0 {
                &[]
            } else {
                std::slice::from_raw_parts(data as *const u8, len)
            }
        }
    }
    
    /// Clear the string
    pub fn clear(&mut self) {
        unsafe { swig_string_ffi::SwigString_clear(self.ptr) }
    }
    
    /// Append bytes
    pub fn append(&mut self, bytes: &[u8]) {
        unsafe {
            swig_string_ffi::SwigString_append(
                self.ptr, 
                bytes.as_ptr() as *const std::os::raw::c_char, 
                bytes.len()
            )
        }
    }
    
    /// Append a string
    pub fn append_str(&mut self, s: &str) {
        self.append(s.as_bytes())
    }
    
    /// Get capacity
    pub fn capacity(&self) -> usize {
        unsafe { swig_string_ffi::SwigString_capacity(self.ptr) }
    }
    
    /// Reserve capacity
    pub fn reserve(&mut self, capacity: usize) {
        unsafe { swig_string_ffi::SwigString_reserve(self.ptr, capacity) }
    }
    
    /// Clone the string
    pub fn clone_swig(&self) -> Self {
        unsafe { Self { ptr: swig_string_ffi::SwigString_clone(self.ptr) } }
    }
    
    /// Resize the string
    pub fn resize(&mut self, new_len: usize, fill_char: u8) {
        unsafe { swig_string_ffi::SwigString_resize(self.ptr, new_len, fill_char as std::os::raw::c_char) }
    }
    
    /// Get substring
    pub fn substr(&self, pos: usize, len: usize) -> Self {
        unsafe { Self { ptr: swig_string_ffi::SwigString_substr(self.ptr, pos, len) } }
    }
    
    /// Find substring
    pub fn find(&self, needle: &str, pos: usize) -> Option<usize> {
        unsafe {
            let result = swig_string_ffi::SwigString_find(
                self.ptr, 
                needle.as_ptr() as *const std::os::raw::c_char, 
                pos
            );
            if result < 0 { None } else { Some(result as usize) }
        }
    }
    
    /// Reverse find
    pub fn rfind(&self, needle: &str, pos: usize) -> Option<usize> {
        unsafe {
            let result = swig_string_ffi::SwigString_rfind(
                self.ptr, 
                needle.as_ptr() as *const std::os::raw::c_char, 
                pos
            );
            if result < 0 { None } else { Some(result as usize) }
        }
    }
    
    /// Compare with another SwigString
    pub fn compare(&self, other: &SwigString) -> std::cmp::Ordering {
        unsafe {
            let result = swig_string_ffi::SwigString_compare(self.ptr, other.ptr);
            result.cmp(&0)
        }
    }
    
    // ========================================================================
    // Advanced string methods
    // ========================================================================
    
    /// Insert bytes at position
    pub fn insert(&mut self, pos: usize, bytes: &[u8]) {
        unsafe {
            swig_string_ffi::SwigString_insert(
                self.ptr,
                pos,
                bytes.as_ptr() as *const std::os::raw::c_char,
                bytes.len()
            )
        }
    }
    
    /// Insert a string at position
    pub fn insert_str(&mut self, pos: usize, s: &str) {
        self.insert(pos, s.as_bytes())
    }
    
    /// Insert a character count times at position
    pub fn insert_char(&mut self, pos: usize, count: usize, c: u8) {
        unsafe {
            swig_string_ffi::SwigString_insert_char(self.ptr, pos, count, c as std::os::raw::c_char)
        }
    }
    
    /// Erase characters starting from position
    pub fn erase(&mut self, pos: usize, len: usize) {
        unsafe { swig_string_ffi::SwigString_erase(self.ptr, pos, len) }
    }
    
    /// Replace characters at position with bytes
    pub fn replace(&mut self, pos: usize, len: usize, bytes: &[u8]) {
        unsafe {
            swig_string_ffi::SwigString_replace(
                self.ptr,
                pos,
                len,
                bytes.as_ptr() as *const std::os::raw::c_char,
                bytes.len()
            )
        }
    }
    
    /// Replace characters at position with a string
    pub fn replace_str(&mut self, pos: usize, len: usize, s: &str) {
        self.replace(pos, len, s.as_bytes())
    }
    
    /// Find first occurrence of any character from the set
    pub fn find_first_of(&self, chars: &str, pos: usize) -> Option<usize> {
        unsafe {
            let result = swig_string_ffi::SwigString_find_first_of(
                self.ptr,
                chars.as_ptr() as *const std::os::raw::c_char,
                pos
            );
            if result < 0 { None } else { Some(result as usize) }
        }
    }
    
    /// Find last occurrence of any character from the set
    pub fn find_last_of(&self, chars: &str, pos: usize) -> Option<usize> {
        unsafe {
            let result = swig_string_ffi::SwigString_find_last_of(
                self.ptr,
                chars.as_ptr() as *const std::os::raw::c_char,
                pos
            );
            if result < 0 { None } else { Some(result as usize) }
        }
    }
    
    /// Find first character NOT in the set
    pub fn find_first_not_of(&self, chars: &str, pos: usize) -> Option<usize> {
        unsafe {
            let result = swig_string_ffi::SwigString_find_first_not_of(
                self.ptr,
                chars.as_ptr() as *const std::os::raw::c_char,
                pos
            );
            if result < 0 { None } else { Some(result as usize) }
        }
    }
    
    /// Find last character NOT in the set
    pub fn find_last_not_of(&self, chars: &str, pos: usize) -> Option<usize> {
        unsafe {
            let result = swig_string_ffi::SwigString_find_last_not_of(
                self.ptr,
                chars.as_ptr() as *const std::os::raw::c_char,
                pos
            );
            if result < 0 { None } else { Some(result as usize) }
        }
    }
    
    /// Shrink capacity to fit size
    pub fn shrink_to_fit(&mut self) {
        unsafe { swig_string_ffi::SwigString_shrink_to_fit(self.ptr) }
    }
    
    /// Get front character (first character)
    pub fn front(&self) -> Option<u8> {
        unsafe {
            let c = swig_string_ffi::SwigString_front(self.ptr);
            if c == '\0' as std::os::raw::c_char { None } else { Some(c as u8) }
        }
    }
    
    /// Get back character (last character)
    pub fn back(&self) -> Option<u8> {
        unsafe {
            let c = swig_string_ffi::SwigString_back(self.ptr);
            if c == '\0' as std::os::raw::c_char { None } else { Some(c as u8) }
        }
    }
    
    /// Swap contents with another SwigString
    pub fn swap(&mut self, other: &mut SwigString) {
        unsafe { swig_string_ffi::SwigString_swap(self.ptr, other.ptr) }
    }
}

impl Default for SwigString {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for SwigString {
    fn drop(&mut self) {
        unsafe {
            if !self.ptr.is_null() {
                swig_string_ffi::SwigString_delete(self.ptr);
            }
        }
    }
}

impl Clone for SwigString {
    fn clone(&self) -> Self {
        self.clone_swig()
    }
}

impl std::fmt::Display for SwigString {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let s = self.to_string_lossy();
        write!(f, "{}", s)
    }
}

impl std::fmt::Debug for SwigString {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let s = self.to_string_lossy();
        write!(f, "SwigString({:?})", s)
    }
}

impl PartialEq for SwigString {
    fn eq(&self, other: &Self) -> bool {
        self.compare(other) == std::cmp::Ordering::Equal
    }
}

impl Eq for SwigString {}

impl PartialOrd for SwigString {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for SwigString {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.compare(other)
    }
}

impl std::hash::Hash for SwigString {
    fn hash<H: std::hash::Hasher>(&self, state: &mut H) {
        self.as_bytes().hash(state);
    }
}

impl From<String> for SwigString {
    fn from(s: String) -> Self {
        Self::from_str(&s)
    }
}

impl From<&str> for SwigString {
    fn from(s: &str) -> Self {
        Self::from_str(s)
    }
}

impl From<SwigString> for String {
    fn from(s: SwigString) -> Self {
        s.into_string()
    }
}

impl From<&SwigString> for String {
    fn from(s: &SwigString) -> Self {
        s.to_string_lossy()
    }
}

// Note: Into<String> for SwigString is auto-generated by Rust from From<SwigString> for String

impl std::ops::Deref for SwigString {
    type Target = [u8];
    
    fn deref(&self) -> &Self::Target {
        self.as_bytes()
    }
}

impl std::borrow::Borrow<[u8]> for SwigString {
    fn borrow(&self) -> &[u8] {
        self.as_bytes()
    }
}
%}
