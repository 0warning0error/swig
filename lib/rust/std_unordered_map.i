/* -----------------------------------------------------------------------------
 * std_unordered_map.i
 *
 * SWIG typemaps for std::unordered_map<K, V>
 * Rust implementation
 *
 * Design notes:
 *   - std::unordered_map<K, V> maps to Rust HashMap<K, V>
 *   - Provides hash-based key-value storage with O(1) average lookup
 *
 * Usage:
 *   %template(IntIntHashMap) std::unordered_map<int, int>;
 *   RUST_UNORDERED_MAP_TRAITS(int, int, IntIntHashMap)
 * ----------------------------------------------------------------------------- */

%{
#include <unordered_map>
#include <stdexcept>
%}

/* -----------------------------------------------------------------------------
 * std::unordered_map template definition
 * ----------------------------------------------------------------------------- */

namespace std {

template<class K, class V> class unordered_map {
public:
  typedef K key_type;
  typedef V mapped_type;
  typedef std::pair<const K, V> value_type;
  typedef size_t size_type;

  // Constructors
  unordered_map();
  unordered_map(const unordered_map& other);

  // Capacity
  size_type size() const;
  bool empty() const;

  // Element access
  V& at(const K& key);
  const V& at(const K& key) const;
  V& operator[](const K& key);

  // Modifiers
  void clear();
  
  %rename(insert_pair) insert;
  void insert(const value_type& value);
  
  %rename(erase_key) erase;
  size_type erase(const K& key);

  // Lookup
  size_type count(const K& key) const;
  bool contains(const K& key) const;
};

} // namespace std

/* -----------------------------------------------------------------------------
 * Extended methods for Rust convenience
 * ----------------------------------------------------------------------------- */

%extend std::unordered_map {
  // Rust-style method names
  %rename(len) size;
  %rename(is_empty) empty;
  
  // Get value or default
  V get_or_default(const K& key, const V& default_value) {
    auto it = $self->find(key);
    if (it != $self->end())
      return it->second;
    return default_value;
  }
  
  // Insert with return indicating if new
  bool insert_new(const K& key, const V& value) {
    auto result = $self->insert({key, value});
    return result.second;  // true if inserted, false if key existed
  }
  
  // Get all keys (as vector)
  %newobject keys();
  std::vector<K>* keys() const {
    auto* vec = new std::vector<K>();
    for (const auto& pair : *$self) {
      vec->push_back(pair.first);
    }
    return vec;
  }
  
  // Get all values (as vector)
  %newobject values();
  std::vector<V>* values() const {
    auto* vec = new std::vector<V>();
    for (const auto& pair : *$self) {
      vec->push_back(pair.second);
    }
    return vec;
  }
}

/* -----------------------------------------------------------------------------
 * Helper macro for Rust trait implementations
 *
 * Usage after %template:
 *   %template(IntIntHashMap) std::unordered_map<int, int>;
 *   RUST_UNORDERED_MAP_TRAITS(int, int, IntIntHashMap)
 * ----------------------------------------------------------------------------- */

%define RUST_UNORDERED_MAP_TRAITS(KTYPE, VTYPE, NAME...)
// Generate Rust trait implementations
%insert("rustcode") %{
impl NAME {
    /// Convert to Rust HashMap
    pub fn to_hashmap(&self) -> std::collections::HashMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)> {
        let mut map = std::collections::HashMap::new();
        let keys = self.keys();
        for i in 0..keys.len() {
            let k = keys.getitem(i);
            let v = self.at(k);
            map.insert(k, v);
        }
        map
    }
    
    /// Create from Rust HashMap
    pub fn from_hashmap(map: std::collections::HashMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)>) -> Self {
        let mut result = Self::new();
        for (k, v) in map {
            result.insert_pair(k, v);
        }
        result
    }
    
    /// Get a value, returning None if key not found
    pub fn get_opt(&self, key: $typemap(rusttype, KTYPE)) -> Option<$typemap(rusttype, VTYPE)> {
        if self.contains(key) {
            Some(self.at(key))
        } else {
            None
        }
    }
}

// Implement From<HashMap>
impl From<std::collections::HashMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)>> for NAME {
    fn from(map: std::collections::HashMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)>) -> Self {
        Self::from_hashmap(map)
    }
}

// Implement Into<HashMap>
impl Into<std::collections::HashMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)>> for NAME {
    fn into(self) -> std::collections::HashMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)> {
        self.to_hashmap()
    }
}
%}
%enddef

/* -----------------------------------------------------------------------------
 * Common unordered_map specializations
 * ----------------------------------------------------------------------------- */

%include <std_vector.i>

// Specialization for int -> int
namespace std {
  template<> class unordered_map<int, int> {
    typedef int key_type;
    typedef int mapped_type;
    typedef size_t size_type;
    
    unordered_map();
    unordered_map(const unordered_map& other);
    ~unordered_map();
    
    size_type size() const;
    bool empty() const;
    int& at(const int& key);
    int& operator[](const int& key);
    void clear();
    void insert(const std::pair<const int, int>& value);
    size_type erase(const int& key);
    size_type count(const int& key) const;
    bool contains(const int& key) const;
  };
}

// Specialization for string -> int
%include <std_string.i>

namespace std {
  template<> class unordered_map<std::string, int> {
    typedef std::string key_type;
    typedef int mapped_type;
    typedef size_t size_type;
    
    unordered_map();
    unordered_map(const unordered_map& other);
    ~unordered_map();
    
    size_type size() const;
    bool empty() const;
    int& at(const std::string& key);
    int& operator[](const std::string& key);
    void clear();
    void insert(const std::pair<const std::string, int>& value);
    size_type erase(const std::string& key);
    size_type count(const std::string& key) const;
    bool contains(const std::string& key) const;
  };
}
