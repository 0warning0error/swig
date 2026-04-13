%module std_map_test

%include <std_string.i>
%include <std_map.i>

%template(IntIntMap) std::map<int, int>;
%template(IntDoubleMap) std::map<int, double>;
%template(StringStringMap) std::map<std::string, std::string>;

%inline %{
#include <map>
#include <string>
#include <vector>

// Test functions with std::map
std::map<int, int> create_int_map() {
    std::map<int, int> m;
    m[1] = 10;
    m[2] = 20;
    m[3] = 30;
    return m;
}

int map_get(const std::map<int, int>& m, int key, int default_value) {
    auto it = m.find(key);
    if (it != m.end()) {
        return it->second;
    }
    return default_value;
}

bool map_contains(const std::map<int, int>& m, int key) {
    return m.find(key) != m.end();
}

int map_sum_values(const std::map<int, int>& m) {
    int sum = 0;
    for (const auto& pair : m) {
        sum += pair.second;
    }
    return sum;
}

std::map<int, int> map_add(const std::map<int, int>& a, const std::map<int, int>& b) {
    std::map<int, int> result = a;
    for (const auto& pair : b) {
        result[pair.first] += pair.second;
    }
    return result;
}

// Test class with std::map members
class IntIntMapHolder {
public:
    std::map<int, int> data;
    
    IntIntMapHolder() {}
    IntIntMapHolder(const std::map<int, int>& m) : data(m) {}
    
    void insert(int key, int value) {
        data[key] = value;
    }
    
    bool remove(int key) {
        return data.erase(key) > 0;
    }
    
    int get(int key, int default_value) const {
        auto it = data.find(key);
        if (it != data.end()) {
            return it->second;
        }
        return default_value;
    }
    
    bool contains(int key) const {
        return data.find(key) != data.end();
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
    
    int sum_values() const {
        int sum = 0;
        for (const auto& pair : data) {
            sum += pair.second;
        }
        return sum;
    }
    
    std::vector<int> keys() const {
        std::vector<int> result;
        for (const auto& pair : data) {
            result.push_back(pair.first);
        }
        return result;
    }
    
    std::vector<int> values() const {
        std::vector<int> result;
        for (const auto& pair : data) {
            result.push_back(pair.second);
        }
        return result;
    }
};

// String map functions
std::map<std::string, std::string> create_string_map() {
    std::map<std::string, std::string> m;
    m["name"] = "SWIG";
    m["language"] = "Rust";
    m["version"] = "4.0";
    return m;
}

std::string map_get_string(const std::map<std::string, std::string>& m, 
                           const std::string& key, 
                           const std::string& default_value) {
    auto it = m.find(key);
    if (it != m.end()) {
        return it->second;
    }
    return default_value;
}
%}
