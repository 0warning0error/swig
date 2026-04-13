%module std_string_test

%include <std_string.i>

%inline %{
#include <string>

// Test functions with std::string
std::string get_greeting() {
    return "Hello from C++!";
}

std::string concat_strings(const std::string& a, const std::string& b) {
    return a + b;
}

int string_length(const std::string& s) {
    return static_cast<int>(s.length());
}

std::string to_upper(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return s;
}

// Test class with std::string members
class StringHolder {
public:
    std::string value;
    
    StringHolder() : value("") {}
    StringHolder(const std::string& v) : value(v) {}
    
    std::string get_value() const { return value; }
    void set_value(const std::string& v) { value = v; }
    
    std::string append(const std::string& s) {
        value += s;
        return value;
    }
    
    int size() const { return static_cast<int>(value.size()); }
};
%}
