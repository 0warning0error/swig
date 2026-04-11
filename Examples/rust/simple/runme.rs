// File: runme.rs
// Simple example demonstrating SWIG Rust binding usage

fn main() {
    // Call the wrapped C functions
    println!("fact(5) = {}", example::fact(5));
    println!("my_mod(17, 5) = {}", example::my_mod(17, 5));
    println!("Current time: {}", example::get_time());
}
