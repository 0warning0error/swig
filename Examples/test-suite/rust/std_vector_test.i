%module std_vector_test

%include <std_string.i>
%include <std_vector.i>

%template(IntVector) std::vector<int>;
%template(DoubleVector) std::vector<double>;
%template(StringVector) std::vector<std::string>;

%inline %{
#include <vector>
#include <string>
#include <algorithm>

// Test functions with std::vector
std::vector<int> create_int_vector() {
    return std::vector<int>{1, 2, 3, 4, 5};
}

int vector_sum(const std::vector<int>& v) {
    int sum = 0;
    for (int i : v) {
        sum += i;
    }
    return sum;
}

std::vector<int> double_elements(const std::vector<int>& v) {
    std::vector<int> result;
    result.reserve(v.size());
    for (int i : v) {
        result.push_back(i * 2);
    }
    return result;
}

std::vector<int> merge_vectors(const std::vector<int>& a, const std::vector<int>& b) {
    std::vector<int> result = a;
    result.insert(result.end(), b.begin(), b.end());
    return result;
}

// Test class with std::vector members
class IntVectorHolder {
public:
    std::vector<int> data;
    
    IntVectorHolder() {}
    IntVectorHolder(const std::vector<int>& v) : data(v) {}
    
    void push(int value) {
        data.push_back(value);
    }
    
    int pop() {
        if (data.empty()) {
            return 0;  // Default value for empty
        }
        int value = data.back();
        data.pop_back();
        return value;
    }
    
    int get(size_t index) const {
        if (index >= data.size()) {
            return 0;
        }
        return data[index];
    }
    
    void set(size_t index, int value) {
        if (index < data.size()) {
            data[index] = value;
        }
    }
    
    size_t size() const {
        return data.size();
    }
    
    bool empty() const {
        return data.empty();
    }
    
    void clear() {
        data.clear();
    }
    
    int sum() const {
        int s = 0;
        for (int i : data) {
            s += i;
        }
        return s;
    }
    
    void sort_ascending() {
        std::sort(data.begin(), data.end());
    }
    
    void sort_descending() {
        std::sort(data.begin(), data.end(), std::greater<int>());
    }
};

// Double vector functions
std::vector<double> create_double_vector() {
    return std::vector<double>{1.1, 2.2, 3.3, 4.4, 5.5};
}

double double_vector_sum(const std::vector<double>& v) {
    double sum = 0.0;
    for (double d : v) {
        sum += d;
    }
    return sum;
}

// String vector functions
std::vector<std::string> create_string_vector() {
    return std::vector<std::string>{"hello", "world", "rust", "swig"};
}

std::string join_strings(const std::vector<std::string>& v, const std::string& delimiter) {
    std::string result;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i > 0) {
            result += delimiter;
        }
        result += v[i];
    }
    return result;
}
%}
