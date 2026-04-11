%module namespace_test

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
%}
