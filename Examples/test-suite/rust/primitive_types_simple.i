%module primitive_types_simple

// Simplified primitive types test for Rust
%inline %{
// Boolean
bool test_bool(bool b) { return !b; }

// Integers
signed char test_schar(signed char c) { return c + 1; }
unsigned char test_uchar(unsigned char c) { return c + 1; }
short test_short(short s) { return s + 1; }
unsigned short test_ushort(unsigned short s) { return s + 1; }
int test_int(int i) { return i + 1; }
unsigned int test_uint(unsigned int i) { return i + 1; }
long test_long(long l) { return l + 1; }
unsigned long test_ulong(unsigned long l) { return l + 1; }
long long test_llong(long long l) { return l + 1; }
unsigned long long test_ullong(unsigned long long l) { return l + 1; }

// Floating point
float test_float(float f) { return f + 1.0f; }
double test_double(double d) { return d + 1.0; }

// Size types
size_t test_size_t(size_t s) { return s + 1; }
ptrdiff_t test_ptrdiff_t(ptrdiff_t p) { return p + 1; }

// Pointers
void* test_void_ptr(void* p) { return p; }
int* test_int_ptr(int* p) { return p; }

// Character
char test_char(char c) { return c + 1; }
%}
