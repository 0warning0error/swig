%module enums_test

// Test various enum types
%inline %{
// Basic enum
enum Color {
    RED,
    GREEN,
    BLUE
};

// Enum with specific values
enum Size {
    SMALL = 1,
    MEDIUM = 5,
    LARGE = 10
};

// Enum in class
class EnumClass {
public:
    enum Status {
        OK,
        ERROR,
        PENDING
    };
    
    EnumClass() : status_(OK) {}
    
    Status get_status() const { return status_; }
    void set_status(Status s) { status_ = s; }
    
private:
    Status status_;
};

// Function using enum
Color get_favorite_color() { return GREEN; }
Size get_default_size() { return MEDIUM; }
%}
