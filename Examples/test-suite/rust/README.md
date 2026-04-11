# SWIG Rust Test Suite

This directory contains the test suite for the SWIG Rust language module.

## Running Tests

### Using Python Runner

```bash
# Set SWIG_LIB if needed
export SWIG_LIB=/path/to/swig/Lib

# Run all tests
python run_tests.py --swig /path/to/swig

# Run specific tests
python run_tests.py --swig /path/to/swig enums primitive_types

# Verbose output
python run_tests.py --swig /path/to/swig --verbose
```

### Using Make (Unix)

```bash
./configure
make check
```

## Test Categories

| Category | Tests | Description |
|----------|-------|-------------|
| Basic | `enums`, `primitive_types` | Basic type mappings |
| Classes | `struct_value`, `inherit` | Class handling |
| Templates | `template_basic` | Template instantiation |
| Overloading | `overload_simple` | Function overloading |

## Adding New Tests

1. Create a `.i` file in `../test-suite/` (if not already existing)
2. Add test name to `CPP_TESTS` or `C_TESTS` in `run_tests.py`
3. Create a `_runme.rs` file if runtime testing is needed

## Test Validation

The test runner performs basic validation:
- SWIG executes without errors
- Rust output file is generated
- Output contains `mod ffi` (FFI declarations)

For comprehensive testing, create `_runme.rs` files that:
- Compile the generated Rust code
- Link with the C wrapper library
- Execute test assertions
