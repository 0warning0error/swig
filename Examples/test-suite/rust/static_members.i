%module static_members

// Test static members and global variables
%inline %{
// Global variables
int global_int = 42;
double global_double = 3.14;
const char* global_str = "hello";

// Global functions
int get_global_int() { return global_int; }
void set_global_int(int v) { global_int = v; }

// Class with static members
class StaticClass {
public:
    static int static_int;
    static double static_double;
    
    int instance_int;
    
    StaticClass() : instance_int(0) {}
    StaticClass(int v) : instance_int(v) {}
    
    // Static methods
    static int get_static_int() { return static_int; }
    static void set_static_int(int v) { static_int = v; }
    static int add_static(int v) { return static_int + v; }
    
    // Instance methods
    int get_instance() const { return instance_int; }
    void set_instance(int v) { instance_int = v; }
};

// Initialize static members
int StaticClass::static_int = 100;
double StaticClass::static_double = 2.718;
%}
