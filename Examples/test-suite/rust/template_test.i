%module template_test

// Test template instantiation
%inline %{
template<typename T>
class Container {
public:
    T value;
    
    Container() : value(T()) {}
    Container(T v) : value(v) {}
    
    T get() const { return value; }
    void set(T v) { value = v; }
    
    T add(T other) { return value + other; }
};

template<typename T>
T global_add(T a, T b) {
    return a + b;
}
%}

// Instantiate templates
%template(IntContainer) Container<int>;
%template(DoubleContainer) Container<double>;

%template(global_int_add) global_add<int>;
%template(global_double_add) global_add<double>;
