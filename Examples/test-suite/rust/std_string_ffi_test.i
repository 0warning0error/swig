/* -----------------------------------------------------------------------------
 * std_string_ffi_test.i
 *
 * Test SwigString FFI implementation
 * ----------------------------------------------------------------------------- */

%module std_string_ffi_test

%include <rust/std_string.i>

%inline %{
#include <string>

// Test functions that use std::string

// Return a string
std::string get_greeting() {
    return "Hello, World!";
}

// Take a string parameter
std::string echo_string(const std::string& s) {
    return s;
}

// Concatenate two strings
std::string concat_strings(const std::string& a, const std::string& b) {
    return a + b;
}

// Get string length
size_t string_length(const std::string& s) {
    return s.length();
}

// Check if string is empty
bool is_string_empty(const std::string& s) {
    return s.empty();
}

// A class with string member
class StringHolder {
public:
    StringHolder() : value_("default") {}
    StringHolder(const std::string& value) : value_(value) {}
    
    void set_value(const std::string& value) { value_ = value; }
    std::string get_value() const { return value_; }
    
    void append(const std::string& s) { value_ += s; }
    void clear() { value_.clear(); }
    
    size_t length() const { return value_.length(); }
    
private:
    std::string value_;
};

%}
