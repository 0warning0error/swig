%module pointer_ref

// Test pointers and references
%inline %{
class ValueClass {
public:
    int value;
    ValueClass(int v = 0) : value(v) {}
    int get() const { return value; }
    void set(int v) { value = v; }
};

// Functions taking pointers
void set_via_pointer(ValueClass* obj, int v) {
    if (obj) obj->set(v);
}

int get_via_pointer(const ValueClass* obj) {
    return obj ? obj->get() : -1;
}

// Functions taking references
void set_via_ref(ValueClass& obj, int v) {
    obj.set(v);
}

int get_via_ref(const ValueClass& obj) {
    return obj.get();
}

// Return pointer
ValueClass* create_value(int v) {
    return new ValueClass(v);
}

void destroy_value(ValueClass* obj) {
    delete obj;
}

// Class with pointer member
class PointerHolder {
public:
    ValueClass* ptr;
    
    PointerHolder() : ptr(nullptr) {}
    PointerHolder(ValueClass* p) : ptr(p) {}
    
    void set_ptr(ValueClass* p) { ptr = p; }
    ValueClass* get_ptr() const { return ptr; }
    int get_value() const { return ptr ? ptr->get() : -1; }
};
%}
