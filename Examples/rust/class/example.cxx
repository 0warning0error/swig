/* File: example.cxx */

#include "example.h"

Shape::Shape() {
    x = 0;
    y = 0;
}

Shape::~Shape() {
}

void Shape::move(double dx, double dy) {
    x += dx;
    y += dy;
}

double Shape::get_x() const {
    return x;
}

double Shape::get_y() const {
    return y;
}

Circle::Circle(double r) : radius(r) {
}

Circle::~Circle() {
}

double Circle::area() const {
    return 3.14159 * radius * radius;
}

double Circle::perimeter() const {
    return 2 * 3.14159 * radius;
}

Square::Square(double s) : side(s) {
}

Square::~Square() {
}

double Square::area() const {
    return side * side;
}

double Square::perimeter() const {
    return 4 * side;
}
