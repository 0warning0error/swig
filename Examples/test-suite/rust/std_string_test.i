/* -----------------------------------------------------------------------------
 * std_string_test.i
 *
 * Test case for SwigString (std::string wrapper) implementation.
 * Verifies:
 *   - Full std::string interface methods
 *   - From/Into conversions with Rust String
 *   - Display/Debug/PartialEq/PartialOrd traits
 *   - String operations (find, replace, substr, etc.)
 * ----------------------------------------------------------------------------- */

%module std_string_test

%include <rust/std_string.i>

%{
#include <string>
#include <cstring>
%}

/* -----------------------------------------------------------------------------
 * Test class with std::string members and methods
 * ----------------------------------------------------------------------------- */

%inline %{

// A class that uses std::string in various ways
class StringHolder {
public:
    std::string name;
    std::string value;
    
    StringHolder() : name(""), value("") {}
    StringHolder(const std::string& n, const std::string& v) : name(n), value(v) {}
    
    // Return by value
    std::string get_name() const { return name; }
    std::string get_value() const { return value; }
    
    // Return by reference
    const std::string& get_name_ref() const { return name; }
    
    // Take by value
    void set_name(const std::string& n) { name = n; }
    void set_value(const std::string& v) { value = v; }
    
    // Append operation
    void append_to_name(const std::string& suffix) {
        name += suffix;
    }
    
    // Concatenation
    std::string concat() const {
        return name + " = " + value;
    }
    
    // String manipulation
    std::string upper() const {
        std::string result = name;
        for (char& c : result) {
            c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
        }
        return result;
    }
    
    std::string lower() const {
        std::string result = name;
        for (char& c : result) {
            c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        }
        return result;
    }
    
    // Search
    bool contains(const std::string& substr) const {
        return name.find(substr) != std::string::npos;
    }
    
    int find_char(char c) const {
        size_t pos = name.find(c);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }
    
    // Substring
    std::string substring(size_t start, size_t len) const {
        return name.substr(start, len);
    }
    
    // Comparison
    int compare_name(const std::string& other) const {
        return name.compare(other);
    }
    
    bool equals(const std::string& other) const {
        return name == other;
    }
    
    // Length
    size_t name_length() const { return name.length(); }
    size_t value_length() const { return value.length(); }
    bool is_empty() const { return name.empty() && value.empty(); }
    
    // Clear
    void clear() {
        name.clear();
        value.clear();
    }
};

// Global functions with std::string
std::string make_greeting(const std::string& name) {
    return "Hello, " + name + "!";
}

std::string repeat_string(const std::string& s, int times) {
    std::string result;
    result.reserve(s.length() * times);
    for (int i = 0; i < times; i++) {
        result += s;
    }
    return result;
}

bool strings_equal(const std::string& a, const std::string& b) {
    return a == b;
}

int string_compare(const std::string& a, const std::string& b) {
    return a.compare(b);
}

std::string join_strings(const std::string& a, const std::string& b, const std::string& sep) {
    return a + sep + b;
}

// Test std::string pointer
std::string* create_string(const char* init) {
    return new std::string(init);
}

void delete_string(std::string* s) {
    delete s;
}

std::string* get_string_ptr(std::string* s) {
    return s;
}

// Test std::string reference output
std::string& get_global_string() {
    static std::string global_str = "global value";
    return global_str;
}

// Test C++20-style string operations
bool test_starts_with(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && 
           s.compare(0, prefix.size(), prefix) == 0;
}

bool test_ends_with(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size() && 
           s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

%}

/* -----------------------------------------------------------------------------
 * Test cases for Rust side
 * ----------------------------------------------------------------------------- */

// The following are expected to work in Rust:
//
// 1. Create SwigString from Rust String
//    let s: SwigString = "hello".to_string().into();
//    let s = SwigString::from("hello");
//    let s = SwigString::from_c_str("hello");
//
// 2. Convert SwigString to Rust String
//    let rs: String = s.into();
//    let rs = s.to_string_lossy();
//
// 3. Use std::string methods
//    s.size()
//    s.length()
//    s.empty()
//    s.c_str()
//    s.append(...)
//    s.find(...)
//    s.substr(...)
//
// 4. Use Rust traits
//    println!("{}", s);  // Display
//    println!("{:?}", s);  // Debug
//    s1 == s2  // PartialEq
//    s1 < s2   // PartialOrd
//
// 5. Use operators
//    s1 + s2   // Add
//    s1 += s2  // AddAssign
//    s[0]      // Index