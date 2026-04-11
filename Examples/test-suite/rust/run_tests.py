#!/usr/bin/env python3
"""
Rust test-suite runner for SWIG.

Usage:
    python run_tests.py [--swig=path] [--verbose] [test1 test2 ...]

Examples:
    python run_tests.py                    # Run all tests
    python run_tests.py enums primitive_types  # Run specific tests
    python run_tests.py --verbose          # Run with verbose output
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path

# Test configuration
SWIG_LIB = os.environ.get('SWIG_LIB', None)
TEST_SUITE_DIR = Path(__file__).parent.parent
RUST_TEST_DIR = Path(__file__).parent

# Rust-specific test cases (located in rust/ directory)
RUST_TESTS = [
    'const_var',
    'class_methods',
    'inherit_basic',
    'enums_test',
    'namespace_test',
    'pointer_ref',
    'overload_test',
    'template_test',
    'director_test',
    'static_members',
    'primitive_types_simple',
]

# Basic test cases from common test-suite
CPP_TESTS = [
    'enums',
    'struct_value',
    'template_basic',
    'inherit',
    'overload_simple',
]

C_TESTS = [
    'enums',
]


def find_swig():
    """Find SWIG executable."""
    # Check environment
    swig = os.environ.get('SWIG', None)
    if swig and os.path.exists(swig):
        return swig
    
    # Check common locations
    for path in ['swig', './build/Release/swig.exe', '../build/Release/swig.exe']:
        try:
            result = subprocess.run([path, '-version'], capture_output=True, text=True)
            if result.returncode == 0:
                return path
        except FileNotFoundError:
            continue
    
    return None


def run_test(swig_exe, test_name, is_cpp=True, is_rust_specific=False, verbose=False):
    """Run a single test."""
    # Determine test file location
    if is_rust_specific:
        test_file = RUST_TEST_DIR / f"{test_name}.i"
    else:
        test_file = TEST_SUITE_DIR / f"{test_name}.i"
    
    if not test_file.exists():
        print(f"SKIP: {test_name} - test file not found")
        return None
    
    # Generate Rust wrapper
    output_rs = RUST_TEST_DIR / f"{test_name}.rs"
    
    cmd = [swig_exe, '-rust']
    if is_cpp:
        cmd.append('-c++')
    cmd.extend(['-module', test_name])
    cmd.extend(['-outdir', str(RUST_TEST_DIR)])
    cmd.append(str(test_file))
    
    if SWIG_LIB:
        cmd.insert(1, f'-I{SWIG_LIB}')
    
    if verbose:
        print(f"  Running: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            print(f"FAIL: {test_name} - SWIG failed")
            if verbose:
                print(result.stderr)
            return False
        
        # Check if output was generated
        if not output_rs.exists():
            print(f"FAIL: {test_name} - no output generated")
            return False
        
        # Basic validation: check for key Rust constructs
        content = output_rs.read_text()
        if 'mod ffi' not in content:
            print(f"FAIL: {test_name} - missing FFI module")
            return False
        
        print(f"PASS: {test_name}")
        return True
        
    except Exception as e:
        print(f"FAIL: {test_name} - {e}")
        return False


def main():
    parser = argparse.ArgumentParser(description='Run SWIG Rust test-suite')
    parser.add_argument('--swig', help='Path to SWIG executable')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('tests', nargs='*', help='Tests to run (default: all)')
    args = parser.parse_args()
    
    # Find SWIG
    swig_exe = args.swig or find_swig()
    if not swig_exe:
        print("ERROR: SWIG executable not found")
        print("Set SWIG environment variable or use --swig option")
        sys.exit(1)
    
    print(f"Using SWIG: {swig_exe}")
    
    # Determine tests to run
    if args.tests:
        tests_to_run = args.tests
    else:
        tests_to_run = RUST_TESTS + CPP_TESTS
    
    # Run tests
    results = {'pass': 0, 'fail': 0, 'skip': 0}
    
    for test in tests_to_run:
        is_rust_specific = test in RUST_TESTS
        is_cpp = test not in C_TESTS
        result = run_test(swig_exe, test, is_cpp, is_rust_specific, args.verbose)
        if result is True:
            results['pass'] += 1
        elif result is False:
            results['fail'] += 1
        else:
            results['skip'] += 1
    
    # Summary
    print(f"\nResults: {results['pass']} passed, {results['fail']} failed, {results['skip']} skipped")
    
    if results['fail'] > 0:
        sys.exit(1)


if __name__ == '__main__':
    main()