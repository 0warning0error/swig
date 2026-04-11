/* File: example.h */

class Shape {
public:
    Shape();
    virtual ~Shape();
    
    void move(double dx, double dy);
    double get_x() const;
    double get_y() const;
    
    virtual double area() const = 0;
    virtual double perimeter() const = 0;
    
protected:
    double x, y;
};

class Circle : public Shape {
public:
    Circle(double r);
    virtual ~Circle();
    
    virtual double area() const;
    virtual double perimeter() const;
    
private:
    double radius;
};

class Square : public Shape {
public:
    Square(double s);
    virtual ~Square();
    
    virtual double area() const;
    virtual double perimeter() const;
    
private:
    double side;
};
