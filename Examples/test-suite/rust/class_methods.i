%module class_methods

// Test various class method types
%inline %{
class MethodTest {
public:
    int value;
    
    MethodTest() : value(0) {}
    MethodTest(int v) : value(v) {}
    
    // Void method
    void void_method() {}
    
    // Method with parameters
    void set_value(int v) { value = v; }
    
    // Method with return value
    int get_value() const { return value; }
    
    // Method with multiple parameters
    int add(int a, int b) const { return a + b + value; }
    
    // Static method
    static int static_add(int a, int b) { return a + b; }
    
    // Const method
    int const_method() const { return value * 2; }
    
    // Default parameter (should work)
    int with_default(int a, int b = 10) { return a + b; }
};

// Free functions
void free_void() {}
int free_add(int a, int b) { return a + b; }
%}
