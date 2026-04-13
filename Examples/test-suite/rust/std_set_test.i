%module std_set_test

%include <std_string.i>
%include <std_set.i>

%template(IntSet) std::set<int>;
%template(DoubleSet) std::set<double>;
%template(StringSet) std::set<std::string>;

%inline %{
#include <set>
#include <string>
#include <vector>

// Test functions with std::set
std::set<int> create_int_set() {
    std::set<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);
    s.insert(4);
    s.insert(5);
    return s;
}

bool int_set_contains(const std::set<int>& s, int value) {
    return s.find(value) != s.end();
}

size_t int_set_size(const std::set<int>& s) {
    return s.size();
}

std::set<int> merge_int_sets(const std::set<int>& a, const std::set<int>& b) {
    std::set<int> result = a;
    for (int val : b) {
        result.insert(val);
    }
    return result;
}

std::set<int> intersect_int_sets(const std::set<int>& a, const std::set<int>& b) {
    std::set<int> result;
    for (int val : a) {
        if (b.find(val) != b.end()) {
            result.insert(val);
        }
    }
    return result;
}

std::set<int> diff_int_sets(const std::set<int>& a, const std::set<int>& b) {
    std::set<int> result = a;
    for (int val : b) {
        result.erase(val);
    }
    return result;
}

// Test class with std::set members
class IntSetHolder {
public:
    std::set<int> data;
    
    IntSetHolder() {}
    IntSetHolder(const std::set<int>& s) : data(s) {}
    
    void insert(int value) {
        data.insert(value);
    }
    
    bool remove(int value) {
        return data.erase(value) > 0;
    }
    
    bool contains(int value) const {
        return data.find(value) != data.end();
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
    
    int min() const {
        if (data.empty()) return 0;
        return *data.begin();
    }
    
    int max() const {
        if (data.empty()) return 0;
        return *data.rbegin();
    }
    
    std::vector<int> to_sorted_vector() const {
        std::vector<int> result;
        for (int val : data) {
            result.push_back(val);
        }
        return result;
    }
};

// Double set functions
std::set<double> create_double_set() {
    std::set<double> s;
    s.insert(1.1);
    s.insert(2.2);
    s.insert(3.3);
    return s;
}

double double_set_sum(const std::set<double>& s) {
    double sum = 0.0;
    for (double d : s) {
        sum += d;
    }
    return sum;
}

// String set functions
std::set<std::string> create_string_set() {
    std::set<std::string> s;
    s.insert("apple");
    s.insert("banana");
    s.insert("cherry");
    return s;
}

bool string_set_contains(const std::set<std::string>& s, const std::string& value) {
    return s.find(value) != s.end();
}

// Set operations
bool is_subset(const std::set<int>& a, const std::set<int>& b) {
    for (int val : a) {
        if (b.find(val) == b.end()) {
            return false;
        }
    }
    return true;
}

bool are_disjoint(const std::set<int>& a, const std::set<int>& b) {
    for (int val : a) {
        if (b.find(val) != b.end()) {
            return false;
        }
    }
    return true;
}
%}
