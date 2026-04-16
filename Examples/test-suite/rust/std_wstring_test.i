/* -----------------------------------------------------------------------------
 * std_wstring_test.i
 *
 * Test SwigWString FFI implementation
 * ----------------------------------------------------------------------------- */

%module std_wstring_test

%include <rust/std_wstring.i>

%inline %{
#include <string>

// Test functions that use std::wstring

// Return a wstring
std::wstring get_wgreeting() {
    return L"Hello, World!";
}

// Take a wstring parameter
std::wstring echo_wstring(const std::wstring& s) {
    return s;
}

// Concatenate two wstrings
std::wstring concat_wstrings(const std::wstring& a, const std::wstring& b) {
    return a + b;
}

// Get wstring length
size_t wstring_length(const std::wstring& s) {
    return s.length();
}

// A class with wstring member
class WStringHolder {
public:
    WStringHolder() : value_(L"default") {}
    WStringHolder(const std::wstring& value) : value_(value) {}
    
    void set_value(const std::wstring& value) { value_ = value; }
    std::wstring get_value() const { return value_; }
    
    void append(const std::wstring& s) { value_ += s; }
    void clear() { value_.clear(); }
    
private:
    std::wstring value_;
};

// Test wchar_t
wchar_t get_wchar() { return L'A'; }
wchar_t echo_wchar(wchar_t c) { return c; }

%}
