%module const_var

// Test constants and variables
%inline %{
// Global constants
const int INT_CONST = 42;
const double DOUBLE_CONST = 3.14159;
const char* STR_CONST = "Hello";

// Global variables
int global_int = 100;
double global_double = 2.71828;

// Class with constants and static members
class ConstClass {
public:
    static const int CLASS_CONST = 99;
    static int static_var;
    
    int instance_var;
    
    ConstClass(int v) : instance_var(v) {}
    int get_value() const { return instance_var; }
    void set_value(int v) { instance_var = v; }
};

int ConstClass::static_var = 0;
%}
