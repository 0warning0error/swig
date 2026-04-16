/* -----------------------------------------------------------------------------
 * std_wstring.i
 *
 * SWIG typemaps for std::wstring
 * Rust implementation with SwigWString wrapper
 *
 * Design:
 *   - std::wstring is wrapped as SwigWString (struct holding C++ pointer)
 *   - FFI layer uses opaque pointer (*mut c_void)
 *   - wchar_t is 2 bytes on Windows, 4 bytes on Unix - handled accordingly
 *   - No automatic conversion to Rust String (use explicit methods if needed)
 *
 * Platform Notes:
 *   - Windows: wchar_t = 2 bytes (UTF-16)
 *   - Unix: wchar_t = 4 bytes (UTF-32)
 *
 * Usage:
 *   %include <rust/std_wstring.i>
 * ----------------------------------------------------------------------------- */

%{
#include <string>
#include <cwchar>
#include <cstdlib>
%}

/* -----------------------------------------------------------------------------
 * C++ helper functions for SwigWString FFI
 * These are exported as extern "C" functions for Rust to call
 * ----------------------------------------------------------------------------- */

%{
extern "C" {

// Create an empty std::wstring
void* SwigWString_new() {
    return new std::wstring();
}

// Create std::wstring from wchar_t array and length
// Note: wchar_t size varies by platform (2 bytes on Windows, 4 bytes on Unix)
void* SwigWString_from_wchars(const wchar_t* data, size_t len) {
    if (data && len > 0) {
        return new std::wstring(data, len);
    }
    return new std::wstring();
}

// Create std::wstring from UTF-16 data (for cross-platform use)
// This handles the conversion from 16-bit values to wchar_t
void* SwigWString_from_utf16(const unsigned short* data, size_t len) {
    if (data && len > 0) {
        std::wstring result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            result.push_back(static_cast<wchar_t>(data[i]));
        }
        return new std::wstring(result);
    }
    return new std::wstring();
}

// Create std::wstring from UTF-32 data (for Unix platforms)
void* SwigWString_from_utf32(const unsigned int* data, size_t len) {
    if (data && len > 0) {
        std::wstring result;
        result.reserve(len);
        for (size_t i = 0; i < len; ++i) {
            result.push_back(static_cast<wchar_t>(data[i]));
        }
        return new std::wstring(result);
    }
    return new std::wstring();
}

// Delete a std::wstring
void SwigWString_delete(void* ptr) {
    if (ptr) {
        delete reinterpret_cast<std::wstring*>(ptr);
    }
}

// Get wchar_t data pointer
const wchar_t* SwigWString_data(const void* ptr) {
    if (ptr) {
        return reinterpret_cast<const std::wstring*>(ptr)->data();
    }
    return nullptr;
}

// Get string length
size_t SwigWString_len(const void* ptr) {
    if (ptr) {
        return reinterpret_cast<const std::wstring*>(ptr)->size();
    }
    return 0;
}

// Check if empty
int SwigWString_is_empty(const void* ptr) {
    if (ptr) {
        return reinterpret_cast<const std::wstring*>(ptr)->empty() ? 1 : 0;
    }
    return 1;
}

// Clear the string
void SwigWString_clear(void* ptr) {
    if (ptr) {
        reinterpret_cast<std::wstring*>(ptr)->clear();
    }
}

// Append wchar_t array
void SwigWString_append(void* ptr, const wchar_t* data, size_t len) {
    if (ptr && data && len > 0) {
        reinterpret_cast<std::wstring*>(ptr)->append(data, len);
    }
}

// Append UTF-16 data
void SwigWString_append_utf16(void* ptr, const unsigned short* data, size_t len) {
    if (ptr && data && len > 0) {
        std::wstring* s = reinterpret_cast<std::wstring*>(ptr);
        for (size_t i = 0; i < len; ++i) {
            s->push_back(static_cast<wchar_t>(data[i]));
        }
    }
}

// Get wchar_t at index
wchar_t SwigWString_at(const void* ptr, size_t index) {
    if (ptr) {
        const std::wstring* s = reinterpret_cast<const std::wstring*>(ptr);
        if (index < s->size()) {
            return (*s)[index];
        }
    }
    return L'\0';
}

// Set wchar_t at index
void SwigWString_set_at(void* ptr, size_t index, wchar_t c) {
    if (ptr) {
        std::wstring* s = reinterpret_cast<std::wstring*>(ptr);
        if (index < s->size()) {
            (*s)[index] = c;
        }
    }
}

// Get capacity
size_t SwigWString_capacity(const void* ptr) {
    if (ptr) {
        return reinterpret_cast<const std::wstring*>(ptr)->capacity();
    }
    return 0;
}

// Reserve capacity
void SwigWString_reserve(void* ptr, size_t capacity) {
    if (ptr) {
        reinterpret_cast<std::wstring*>(ptr)->reserve(capacity);
    }
}

// Compare two wstrings
int SwigWString_compare(const void* ptr1, const void* ptr2) {
    if (!ptr1 && !ptr2) return 0;
    if (!ptr1) return -1;
    if (!ptr2) return 1;
    const std::wstring* s1 = reinterpret_cast<const std::wstring*>(ptr1);
    const std::wstring* s2 = reinterpret_cast<const std::wstring*>(ptr2);
    return s1->compare(*s2);
}

// Clone a wstring
void* SwigWString_clone(const void* ptr) {
    if (ptr) {
        return new std::wstring(*reinterpret_cast<const std::wstring*>(ptr));
    }
    return new std::wstring();
}

// Resize wstring
void SwigWString_resize(void* ptr, size_t new_len, wchar_t fill_char) {
    if (ptr) {
        reinterpret_cast<std::wstring*>(ptr)->resize(new_len, fill_char);
    }
}

// Get substring
void* SwigWString_substr(const void* ptr, size_t pos, size_t len) {
    if (ptr) {
        const std::wstring* s = reinterpret_cast<const std::wstring*>(ptr);
        if (pos <= s->size()) {
            return new std::wstring(s->substr(pos, len));
        }
    }
    return new std::wstring();
}

// Find substring
long SwigWString_find(const void* ptr, const wchar_t* needle, size_t pos) {
    if (ptr && needle) {
        const std::wstring* s = reinterpret_cast<const std::wstring*>(ptr);
        size_t result = s->find(needle, pos);
        return (result == std::wstring::npos) ? -1 : static_cast<long>(result);
    }
    return -1;
}

// Reverse find
long SwigWString_rfind(const void* ptr, const wchar_t* needle, size_t pos) {
    if (ptr && needle) {
        const std::wstring* s = reinterpret_cast<const std::wstring*>(ptr);
        size_t result = s->rfind(needle, pos);
        return (result == std::wstring::npos) ? -1 : static_cast<long>(result);
    }
    return -1;
}

// Push back a single wchar_t
void SwigWString_push_back(void* ptr, wchar_t c) {
    if (ptr) {
        reinterpret_cast<std::wstring*>(ptr)->push_back(c);
    }
}

// Pop back a wchar_t
int SwigWString_pop_back(void* ptr) {
    if (ptr && !reinterpret_cast<std::wstring*>(ptr)->empty()) {
        reinterpret_cast<std::wstring*>(ptr)->pop_back();
        return 1;
    }
    return 0;
}

// Insert wchar_t array at position
void SwigWString_insert(void* ptr, size_t pos, const wchar_t* data, size_t len) {
    if (ptr && data && len > 0) {
        std::wstring* s = reinterpret_cast<std::wstring*>(ptr);
        if (pos <= s->size()) {
            s->insert(pos, data, len);
        }
    }
}

// Erase characters from position
void SwigWString_erase(void* ptr, size_t pos, size_t len) {
    if (ptr) {
        std::wstring* s = reinterpret_cast<std::wstring*>(ptr);
        if (pos < s->size()) {
            s->erase(pos, len);
        }
    }
}

// Replace characters at position
void SwigWString_replace(void* ptr, size_t pos, size_t len, const wchar_t* data, size_t data_len) {
    if (ptr && data && data_len > 0) {
        std::wstring* s = reinterpret_cast<std::wstring*>(ptr);
        if (pos <= s->size()) {
            s->replace(pos, len, data, data_len);
        }
    }
}

// Find first occurrence of any wchar_t from the set
long SwigWString_find_first_of(const void* ptr, const wchar_t* chars, size_t pos) {
    if (ptr && chars) {
        const std::wstring* s = reinterpret_cast<const std::wstring*>(ptr);
        size_t result = s->find_first_of(chars, pos);
        return (result == std::wstring::npos) ? -1 : static_cast<long>(result);
    }
    return -1;
}

// Find last occurrence of any wchar_t from the set
long SwigWString_find_last_of(const void* ptr, const wchar_t* chars, size_t pos) {
    if (ptr && chars) {
        const std::wstring* s = reinterpret_cast<const std::wstring*>(ptr);
        size_t result = s->find_last_of(chars, pos);
        return (result == std::wstring::npos) ? -1 : static_cast<long>(result);
    }
    return -1;
}

// Shrink capacity to fit size
void SwigWString_shrink_to_fit(void* ptr) {
    if (ptr) {
        reinterpret_cast<std::wstring*>(ptr)->shrink_to_fit();
    }
}

// Get front wchar_t
wchar_t SwigWString_front(const void* ptr) {
    if (ptr) {
        const std::wstring* s = reinterpret_cast<const std::wstring*>(ptr);
        if (!s->empty()) {
            return s->front();
        }
    }
    return L'\0';
}

// Get back wchar_t
wchar_t SwigWString_back(const void* ptr) {
    if (ptr) {
        const std::wstring* s = reinterpret_cast<const std::wstring*>(ptr);
        if (!s->empty()) {
            return s->back();
        }
    }
    return L'\0';
}

// Swap contents with another wstring
void SwigWString_swap(void* ptr1, void* ptr2) {
    if (ptr1 && ptr2) {
        std::wstring* s1 = reinterpret_cast<std::wstring*>(ptr1);
        std::wstring* s2 = reinterpret_cast<std::wstring*>(ptr2);
        s1->swap(*s2);
    }
}

// Get sizeof(wchar_t) - for platform detection in Rust
size_t SwigWString_wchar_size() {
    return sizeof(wchar_t);
}

} // extern "C"
%}

namespace std {

%naturalvar wstring;

class wstring;

/* -----------------------------------------------------------------------------
 * Type mappings for std::wstring (by value)
 * ----------------------------------------------------------------------------- */

%typemap(ctype) std::wstring "void *"
%typemap(rusttype) std::wstring "SwigWString"
%typemap(rsffitype) std::wstring "*mut std::ffi::c_void"

%typemap(rsin) std::wstring "$input.ptr"
%typemap(rsout) std::wstring "SwigWString { ptr: $result }"

%typemap(in, canthrow=1) std::wstring
%{ $1 = *reinterpret_cast<std::wstring*>($input); %}

%typemap(out) std::wstring
%{ $result = reinterpret_cast<void*>(new std::wstring($1)); %}

%typemap(directorin) std::wstring
%{ $input = reinterpret_cast<void*>(new std::wstring($1)); %}

%typemap(directorout, canthrow=1) std::wstring
%{ $result = *reinterpret_cast<std::wstring*>($input); %}

%typemap(typecheck) std::wstring = SWIGTYPE;

/* -----------------------------------------------------------------------------
 * Type mappings for const std::wstring& (by const reference)
 * ----------------------------------------------------------------------------- */

%typemap(ctype) const std::wstring & "const void *"
%typemap(rusttype) const std::wstring & "&SwigWString"
%typemap(rsffitype) const std::wstring & "*const std::ffi::c_void"

%typemap(rsin) const std::wstring & "$input.ptr"
%typemap(rsout) const std::wstring & "SwigWString { ptr: $result as *mut std::ffi::c_void }"

%typemap(in, canthrow=1) const std::wstring &
%{ $1 = reinterpret_cast<const std::wstring*>($input); %}

%typemap(out) const std::wstring &
%{ $result = reinterpret_cast<const void*>($1); %}

%typemap(directorin) const std::wstring &
%{ $input = reinterpret_cast<const void*>($1); %}

%typemap(directorout, warning=SWIGWARN_TYPEMAP_THREAD_UNSAFE_MSG) const std::wstring &
%{ static std::wstring $1_str;
   $1_str = *reinterpret_cast<const std::wstring*>($input);
   $result = &$1_str; %}

%typemap(typecheck) const std::wstring & = SWIGTYPE;

/* -----------------------------------------------------------------------------
 * Type mappings for std::wstring& (by mutable reference)
 * ----------------------------------------------------------------------------- */

%typemap(ctype) std::wstring & "void *"
%typemap(rusttype) std::wstring & "&mut SwigWString"
%typemap(rsffitype) std::wstring & "*mut std::ffi::c_void"

%typemap(rsin) std::wstring & "$input.ptr"
%typemap(rsout) std::wstring & "SwigWString { ptr: $result }"

%typemap(in) std::wstring &
%{ $1 = reinterpret_cast<std::wstring*>($input); %}

%typemap(out) std::wstring &
%{ $result = reinterpret_cast<void*>($1); %}

%typemap(typecheck) std::wstring & = SWIGTYPE;

/* -----------------------------------------------------------------------------
 * Type mappings for std::wstring* (pointer)
 * ----------------------------------------------------------------------------- */

%typemap(ctype) std::wstring * "void *"
%typemap(rusttype) std::wstring * "Option<SwigWString>"
%typemap(rsffitype) std::wstring * "*mut std::ffi::c_void"

%typemap(rsin) std::wstring * "$input.map(|s| s.ptr).unwrap_or(std::ptr::null_mut())"
%typemap(rsout) std::wstring * "$result"

%typemap(in) std::wstring *
%{ $1 = reinterpret_cast<std::wstring*>($input); %}

%typemap(out) std::wstring *
%{ $result = reinterpret_cast<void*>($input); %}

%typemap(typecheck) std::wstring * = SWIGTYPE;

/* -----------------------------------------------------------------------------
 * Exception handling
 * ----------------------------------------------------------------------------- */

%typemap(throws, canthrow=1) std::wstring
%{ throw std::runtime_error("wstring exception"); %}

%typemap(throws, canthrow=1) const std::wstring &
%{ throw std::runtime_error("wstring exception"); %}

} // namespace std

/* -----------------------------------------------------------------------------
 * SwigWString type definition and trait implementations
 * ----------------------------------------------------------------------------- */

%insert("rustcode") %{
/// Wrapper for C++ std::wstring
/// 
/// This type provides a safe wrapper around C++ std::wstring.
/// 
/// # Platform Notes
/// 
/// - Windows: wchar_t is 2 bytes (UTF-16)
/// - Unix: wchar_t is 4 bytes (UTF-32)
/// 
/// Use `wchar_size()` to determine the current platform's wchar_t size.
pub struct SwigWString {
    ptr: *mut std::ffi::c_void,
}

// FFI declarations for SwigWString helper functions
mod swig_wstring_ffi {
    extern "C" {
        pub fn SwigWString_new() -> *mut std::ffi::c_void;
        pub fn SwigWString_from_wchars(data: *const std::ffi::c_void, len: usize) -> *mut std::ffi::c_void;
        pub fn SwigWString_from_utf16(data: *const u16, len: usize) -> *mut std::ffi::c_void;
        pub fn SwigWString_from_utf32(data: *const u32, len: usize) -> *mut std::ffi::c_void;
        pub fn SwigWString_delete(ptr: *mut std::ffi::c_void);
        pub fn SwigWString_data(ptr: *const std::ffi::c_void) -> *const std::ffi::c_void;
        pub fn SwigWString_len(ptr: *const std::ffi::c_void) -> usize;
        pub fn SwigWString_is_empty(ptr: *const std::ffi::c_void) -> std::os::raw::c_int;
        pub fn SwigWString_clear(ptr: *mut std::ffi::c_void);
        pub fn SwigWString_append(ptr: *mut std::ffi::c_void, data: *const std::ffi::c_void, len: usize);
        pub fn SwigWString_append_utf16(ptr: *mut std::ffi::c_void, data: *const u16, len: usize);
        pub fn SwigWString_at(ptr: *const std::ffi::c_void, index: usize) -> u16;
        pub fn SwigWString_set_at(ptr: *mut std::ffi::c_void, index: usize, c: u16);
        pub fn SwigWString_capacity(ptr: *const std::ffi::c_void) -> usize;
        pub fn SwigWString_reserve(ptr: *mut std::ffi::c_void, capacity: usize);
        pub fn SwigWString_compare(ptr1: *const std::ffi::c_void, ptr2: *const std::ffi::c_void) -> std::os::raw::c_int;
        pub fn SwigWString_clone(ptr: *const std::ffi::c_void) -> *mut std::ffi::c_void;
        pub fn SwigWString_resize(ptr: *mut std::ffi::c_void, new_len: usize, fill_char: u16);
        pub fn SwigWString_substr(ptr: *const std::ffi::c_void, pos: usize, len: usize) -> *mut std::ffi::c_void;
        pub fn SwigWString_find(ptr: *const std::ffi::c_void, needle: *const std::ffi::c_void, pos: usize) -> std::os::raw::c_long;
        pub fn SwigWString_rfind(ptr: *const std::ffi::c_void, needle: *const std::ffi::c_void, pos: usize) -> std::os::raw::c_long;
        pub fn SwigWString_push_back(ptr: *mut std::ffi::c_void, c: u16);
        pub fn SwigWString_pop_back(ptr: *mut std::ffi::c_void) -> std::os::raw::c_int;
        pub fn SwigWString_insert(ptr: *mut std::ffi::c_void, pos: usize, data: *const std::ffi::c_void, len: usize);
        pub fn SwigWString_erase(ptr: *mut std::ffi::c_void, pos: usize, len: usize);
        pub fn SwigWString_replace(ptr: *mut std::ffi::c_void, pos: usize, len: usize, data: *const std::ffi::c_void, data_len: usize);
        pub fn SwigWString_find_first_of(ptr: *const std::ffi::c_void, chars: *const std::ffi::c_void, pos: usize) -> std::os::raw::c_long;
        pub fn SwigWString_find_last_of(ptr: *const std::ffi::c_void, chars: *const std::ffi::c_void, pos: usize) -> std::os::raw::c_long;
        pub fn SwigWString_shrink_to_fit(ptr: *mut std::ffi::c_void);
        pub fn SwigWString_front(ptr: *const std::ffi::c_void) -> u16;
        pub fn SwigWString_back(ptr: *const std::ffi::c_void) -> u16;
        pub fn SwigWString_swap(ptr1: *mut std::ffi::c_void, ptr2: *mut std::ffi::c_void);
        pub fn SwigWString_wchar_size() -> usize;
    }
}

impl SwigWString {
    /// Size of wchar_t on the current platform
    /// - 2 on Windows (UTF-16)
    /// - 4 on Unix (UTF-32)
    pub fn wchar_size() -> usize {
        unsafe { swig_wstring_ffi::SwigWString_wchar_size() }
    }
    
    /// Create an empty SwigWString
    pub fn new() -> Self {
        unsafe {
            Self { ptr: swig_wstring_ffi::SwigWString_new() }
        }
    }
    
    /// Create from UTF-16 data
    /// This is the preferred way on Windows where wchar_t = u16
    pub fn from_utf16(data: &[u16]) -> Self {
        unsafe {
            Self { 
                ptr: swig_wstring_ffi::SwigWString_from_utf16(data.as_ptr(), data.len())
            }
        }
    }
    
    /// Create from UTF-32 data
    /// This is the preferred way on Unix where wchar_t = u32
    pub fn from_utf32(data: &[u32]) -> Self {
        unsafe {
            Self { 
                ptr: swig_wstring_ffi::SwigWString_from_utf32(data.as_ptr(), data.len())
            }
        }
    }
    
    /// Get string length (number of wchar_t characters)
    pub fn len(&self) -> usize {
        unsafe { swig_wstring_ffi::SwigWString_len(self.ptr) }
    }
    
    /// Check if empty
    pub fn is_empty(&self) -> bool {
        unsafe { swig_wstring_ffi::SwigWString_is_empty(self.ptr) != 0 }
    }
    
    /// Get the data as a slice of u16 (for Windows/UTF-16)
    /// # Safety
    /// This is only valid on platforms where wchar_t is 2 bytes
    pub unsafe fn as_u16_slice(&self) -> &[u16] {
        let data = swig_wstring_ffi::SwigWString_data(self.ptr) as *const u16;
        let len = swig_wstring_ffi::SwigWString_len(self.ptr);
        if data.is_null() || len == 0 {
            &[]
        } else {
            std::slice::from_raw_parts(data, len)
        }
    }
    
    /// Get the data as a slice of u32 (for Unix/UTF-32)
    /// # Safety
    /// This is only valid on platforms where wchar_t is 4 bytes
    pub unsafe fn as_u32_slice(&self) -> &[u32] {
        let data = swig_wstring_ffi::SwigWString_data(self.ptr) as *const u32;
        let len = swig_wstring_ffi::SwigWString_len(self.ptr);
        if data.is_null() || len == 0 {
            &[]
        } else {
            std::slice::from_raw_parts(data, len)
        }
    }
    
    /// Clear the string
    pub fn clear(&mut self) {
        unsafe { swig_wstring_ffi::SwigWString_clear(self.ptr) }
    }
    
    /// Append UTF-16 data
    pub fn append_utf16(&mut self, data: &[u16]) {
        unsafe {
            swig_wstring_ffi::SwigWString_append_utf16(self.ptr, data.as_ptr(), data.len())
        }
    }
    
    /// Get capacity
    pub fn capacity(&self) -> usize {
        unsafe { swig_wstring_ffi::SwigWString_capacity(self.ptr) }
    }
    
    /// Reserve capacity
    pub fn reserve(&mut self, capacity: usize) {
        unsafe { swig_wstring_ffi::SwigWString_reserve(self.ptr, capacity) }
    }
    
    /// Clone the string
    pub fn clone_wstring(&self) -> Self {
        unsafe { Self { ptr: swig_wstring_ffi::SwigWString_clone(self.ptr) } }
    }
    
    /// Resize the string
    pub fn resize(&mut self, new_len: usize, fill_char: u16) {
        unsafe { swig_wstring_ffi::SwigWString_resize(self.ptr, new_len, fill_char) }
    }
    
    /// Get substring
    pub fn substr(&self, pos: usize, len: usize) -> Self {
        unsafe { Self { ptr: swig_wstring_ffi::SwigWString_substr(self.ptr, pos, len) } }
    }
    
    /// Compare with another SwigWString
    pub fn compare(&self, other: &SwigWString) -> std::cmp::Ordering {
        unsafe {
            let result = swig_wstring_ffi::SwigWString_compare(self.ptr, other.ptr);
            result.cmp(&0)
        }
    }
    
    /// Erase characters starting from position
    pub fn erase(&mut self, pos: usize, len: usize) {
        unsafe { swig_wstring_ffi::SwigWString_erase(self.ptr, pos, len) }
    }
    
    /// Shrink capacity to fit size
    pub fn shrink_to_fit(&mut self) {
        unsafe { swig_wstring_ffi::SwigWString_shrink_to_fit(self.ptr) }
    }
    
    /// Swap contents with another SwigWString
    pub fn swap(&mut self, other: &mut SwigWString) {
        unsafe { swig_wstring_ffi::SwigWString_swap(self.ptr, other.ptr) }
    }
}

impl Default for SwigWString {
    fn default() -> Self {
        Self::new()
    }
}

impl Drop for SwigWString {
    fn drop(&mut self) {
        unsafe {
            if !self.ptr.is_null() {
                swig_wstring_ffi::SwigWString_delete(self.ptr);
            }
        }
    }
}

impl Clone for SwigWString {
    fn clone(&self) -> Self {
        self.clone_wstring()
    }
}

impl std::fmt::Debug for SwigWString {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "SwigWString(len={}, wchar_size={})", self.len(), Self::wchar_size())
    }
}

impl PartialEq for SwigWString {
    fn eq(&self, other: &Self) -> bool {
        self.compare(other) == std::cmp::Ordering::Equal
    }
}

impl Eq for SwigWString {}

impl PartialOrd for SwigWString {
    fn partial_cmp(&self, other: &Self) -> Option<std::cmp::Ordering> {
        Some(self.cmp(other))
    }
}

impl Ord for SwigWString {
    fn cmp(&self, other: &Self) -> std::cmp::Ordering {
        self.compare(other)
    }
}
%}
