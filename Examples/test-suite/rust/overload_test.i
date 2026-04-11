%module overload_test

// Test function overloading
%inline %{
class OverloadClass {
public:
    OverloadClass() : value_(0) {}
    OverloadClass(int v) : value_(v) {}
    OverloadClass(double v) : value_(static_cast<int>(v)) {}
    
    // Overloaded methods
    int process(int x) { return x + value_; }
    int process(double x) { return static_cast<int>(x) + value_; }
    int process(int x, int y) { return x + y + value_; }
    
    // Const vs non-const overload
    int get() { return value_; }
    int get() const { return value_ * 10; }
    
private:
    int value_;
};

// Global overloaded functions
int add(int a, int b) { return a + b; }
int add(int a, int b, int c) { return a + b + c; }
double add(double a, double b) { return a + b; }
%}
