%module namespace_test

// Enable namespace support for Rust mod generation
// This maps C++ namespaces to Rust pub mod
%feature("nspace") Outer::OuterClass;
%feature("nspace") Outer::Inner::InnerClass;
%feature("nspace") MYTEST::MyClass;
%feature("nspace") MY::MyClass;
%feature("nspace") MY::MyStruct;
%feature("nspace") MY::MyEnum;
%feature("nspace") MY::MyUnion;
%feature("nspace") MY;  // Enable for all MY namespace members including global functions

// Test namespaces
%inline %{
namespace Outer {
    int outer_var = 100;
    
    int outer_func() { return outer_var; }
    
    class OuterClass {
    public:
        int value;
        OuterClass() : value(1) {}
        int method() { return value; }
    };
    
    namespace Inner {
        int inner_var = 200;
        
        int inner_func() { return inner_var; }
        
        class InnerClass {
        public:
            int value;
            InnerClass() : value(2) {}
            int method() { return value; }
        };
    }
}

// Test uppercase namespace
namespace MYTEST {
    class MyClass {
    public:
        int value;
        MyClass() : value(3) {}
        int method() { return value; }
    };
}

// Test namespace MY with global function, struct, enum, union
namespace MY {
    inline int my_global_check_status(int code) {
        return code;
    }
    
    class MyClass {
    public:
        int value;
        MyClass() : value(4) {}
        int method() { return value; }
    };
    
    // Struct in namespace
    struct MyStruct {
        int x;
        int y;
    };
    
    // Enum in namespace
    enum MyEnum {
        MyEnum_Value1,
        MyEnum_Value2,
        MyEnum_Value3
    };
    
    // Union in namespace
    union MyUnion {
        int int_val;
        float float_val;
    };
}
%}
