%module inherit_basic

// Test inheritance
%inline %{
class Base {
public:
    int base_value;
    
    Base() : base_value(1) {}
    Base(int v) : base_value(v) {}
    
    virtual ~Base() {}
    
    int base_method() { return base_value; }
    virtual int virtual_method() { return base_value * 10; }
};

class Derived : public Base {
public:
    int derived_value;
    
    Derived() : Base(10), derived_value(2) {}
    Derived(int base_v, int derived_v) : Base(base_v), derived_value(derived_v) {}
    
    int derived_method() { return derived_value; }
    
    // Override virtual method
    int virtual_method() override { return base_value * 100 + derived_value; }
};

class GrandDerived : public Derived {
public:
    GrandDerived() : Derived() {}
    
    int grand_method() { return base_value + derived_value; }
};
%}
