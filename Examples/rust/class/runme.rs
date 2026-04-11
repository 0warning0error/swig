// File: runme.rs
// Class example demonstrating SWIG Rust binding usage with C++ classes

fn main() {
    // Create a Circle
    let circle = Circle::new(5.0);
    println!("Circle radius 5.0:");
    println!("  area = {}", circle.area());
    println!("  perimeter = {}", circle.perimeter());
    
    // Create a Square
    let square = Square::new(4.0);
    println!("Square side 4.0:");
    println!("  area = {}", square.area());
    println!("  perimeter = {}", square.perimeter());
    
    // Use Shape trait methods
    circle.move(1.0, 2.0);
    println!("Circle moved to ({}, {})", circle.get_x(), circle.get_y());
}
