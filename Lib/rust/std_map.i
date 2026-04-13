/* -----------------------------------------------------------------------------
 * std_map.i
 *
 * SWIG typemaps for std::map<K, V, C>
 * Rust implementation
 *
 * Design notes:
 *   - std::map<K, V> is ordered, maps to Rust BTreeMap<K, V>
 *   - For unordered maps, use std::unordered_map (maps to HashMap)
 *   - Provides Index/IndexMut trait implementations
 *   - Provides IntoIterator for iteration
 *   - Provides From/Into conversions with BTreeMap<K, V>
 *
 * Usage:
 *   %include <std_map.i>
 *   %template(IntStringMap) std::map<int, std::string>
 *   RUST_MAP_TRAITS(int, std::string, IntStringMap)
 * ----------------------------------------------------------------------------- */

%{
#include <map>
#include <algorithm>
#include <stdexcept>
%}

/* -----------------------------------------------------------------------------
 * std::map template definition
 * ----------------------------------------------------------------------------- */

namespace std {

template<class K, class V, class C = std::less<K>> class map {
public:
  typedef K key_type;
  typedef V mapped_type;
  typedef std::pair<const K, V> value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;

  // Constructors
  map();
  map(const map& other);
  
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
  
  void swap(map& other);

  // Lookup
  size_type count(const K& key) const;
  
  %rename(find_iter) find;
  // iterator find(const K& key);
  
  // Iterators (simplified for FFI)
  value_type* begin();
  value_type* end();
  const value_type* begin() const;
  const value_type* end() const;
};

} // namespace std

/* -----------------------------------------------------------------------------
 * Extended methods for Rust convenience
 * ----------------------------------------------------------------------------- */

%extend std::map {
  // Get item (throws if not found)
  %newobject getitem(const K& key) throw(std::out_of_range);
  V getitem(const K& key) throw(std::out_of_range) {
    auto iter = $self->find(key);
    if (iter == $self->end())
      throw std::out_of_range("key not found");
    return iter->second;
  }
  
  // Set item (insert or update)
  void setitem(const K& key, const V& value) {
    (*$self)[key] = value;
  }
  
  // Check if key exists
  bool contains_key(const K& key) const {
    return $self->find(key) != $self->end();
  }
  
  // Insert (returns true if inserted, false if key existed)
  bool insert_new(const K& key, const V& value) {
    auto result = $self->insert(std::make_pair(key, value));
    return result.second;
  }
  
  // Remove and return value (like BTreeMap::remove)
  %newobject remove(const K& key);
  V remove(const K& key) {
    auto iter = $self->find(key);
    if (iter == $self->end()) {
      // Return default value or throw - depends on V
      return V();
    }
    V value = iter->second;
    $self->erase(iter);
    return value;
  }
  
  // Remove if exists (returns true if removed)
  bool remove_if_exists(const K& key) {
    auto iter = $self->find(key);
    if (iter != $self->end()) {
      $self->erase(iter);
      return true;
    }
    return false;
  }
  
  // Get or insert default (like BTreeMap::entry.or_default)
  V& get_or_insert(const K& key) {
    return (*$self)[key];
  }
  
  // Get or insert with (like BTreeMap::entry.or_insert_with)
  V& get_or_insert_with(const K& key, V default_value) {
    auto iter = $self->find(key);
    if (iter == $self->end()) {
      $self->insert(std::make_pair(key, default_value));
      return (*$self)[key];
    }
    return iter->second;
  }
  
  // Get all keys as vector
  std::vector<K> keys() const {
    std::vector<K> result;
    for (const auto& pair : *$self) {
      result.push_back(pair.first);
    }
    return result;
  }
  
  // Get all values as vector
  std::vector<V> values() const {
    std::vector<V> result;
    for (const auto& pair : *$self) {
      result.push_back(pair.second);
    }
    return result;
  }
  
  // Get all entries as vector of pairs
  std::vector<std::pair<K, V>> entries() const {
    std::vector<std::pair<K, V>> result;
    for (const auto& pair : *$self) {
      result.push_back(std::make_pair(pair.first, pair.second));
    }
    return result;
  }
  
  // Rust-style names
  %rename(len) size;
  %rename(is_empty) empty;
}

/* -----------------------------------------------------------------------------
 * Helper macro for Rust trait implementations
 *
 * Usage after %template:
 *   %template(IntStringMap) std::map<int, std::string>;
 *   RUST_MAP_TRAITS(int, std::string, IntStringMap)
 * ----------------------------------------------------------------------------- */

%define RUST_MAP_TRAITS(KTYPE, VTYPE, NAME...)
// Generate Rust trait implementations
%insert("rustcode") %{
impl NAME {
    /// Convert to Rust BTreeMap<K, V>
    pub fn to_btree_map(&self) -> std::collections::BTreeMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)> {
        let mut map = std::collections::BTreeMap::new();
        let keys = self.keys();
        for key in &keys {
            map.insert(key.clone(), self.getitem(key));
        }
        map
    }
    
    /// Create from Rust BTreeMap<K, V>
    pub fn from_btree_map(map: std::collections::BTreeMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)>) -> Self {
        let mut result = Self::new();
        for (key, value) in map {
            result.setitem(key, value);
        }
        result
    }
}

// Implement Index trait
impl std::ops::Index<$typemap(rusttype, KTYPE)> for NAME {
    type Output = $typemap(rusttype, VTYPE);
    
    fn index(&self, key: $typemap(rusttype, KTYPE)) -> &Self::Output {
        // Note: FFI can't safely return reference to C++ memory
        // Use getitem() for safe access
        unimplemented!("Use getitem() for safe access")
    }
}

// Implement IntoIterator for iteration
impl IntoIterator for NAME {
    type Item = ($typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE));
    type IntoIter = NAMEIterator;
    
    fn into_iter(self) -> Self::IntoIter {
        let entries = self.entries();
        NAMEIterator {
            entries: entries,
            index: 0,
        }
    }
}

/// Iterator for NAME
pub struct NAMEIterator {
    entries: std::vector<std::pair<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)>>,
    index: usize,
}

impl Iterator for NAMEIterator {
    type Item = ($typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE));
    
    fn next(&mut self) -> Option<Self::Item> {
        if self.index < self.entries.len() {
            let entry = self.entries.getitem(self.index);
            self.index += 1;
            Some((entry.first, entry.second))
        } else {
            None
        }
    }
}

// Implement From<BTreeMap>
impl From<std::collections::BTreeMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)>> for NAME {
    fn from(map: std::collections::BTreeMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)>) -> Self {
        Self::from_btree_map(map)
    }
}

// Implement Into<BTreeMap>
impl Into<std::collections::BTreeMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)>> for NAME {
    fn into(self) -> std::collections::BTreeMap<$typemap(rusttype, KTYPE), $typemap(rusttype, VTYPE)> {
        self.to_btree_map()
    }
}
%}
%enddef

/* -----------------------------------------------------------------------------
 * std::unordered_map (maps to HashMap)
 * ----------------------------------------------------------------------------- */

%{
#include <unordered_map>
%}

namespace std {

template<class K, class V, class H = std::hash<K>, class P = std::equal_to<K>> 
class unordered_map {
public:
  typedef K key_type;
  typedef V mapped_type;
  typedef std::pair<const K, V> value_type;
  typedef size_t size_type;

  unordered_map();
  unordered_map(const unordered_map& other);
  
  size_type size() const;
  bool empty() const;
  void clear();
  
  V& at(const K& key);
  V& operator[](const K& key);
  
  size_type count(const K& key) const;
  size_type erase(const K& key);
};

} // namespace std

%extend std::unordered_map {
  V getitem(const K& key) throw(std::out_of_range) {
    auto iter = $self->find(key);
    if (iter == $self->end())
      throw std::out_of_range("key not found");
    return iter->second;
  }
  
  void setitem(const K& key, const V& value) {
    (*$self)[key] = value;
  }
  
  bool contains_key(const K& key) const {
    return $self->find(key) != $self->end();
  }
  
  %rename(len) size;
  %rename(is_empty) empty;
}

/* -----------------------------------------------------------------------------
 * Common map specializations
 * ----------------------------------------------------------------------------- */

// int -> int
namespace std {
  template<> class map<int, int> {
    typedef int key_type;
    typedef int mapped_type;
    typedef size_t size_type;
    
    map();
    map(const map& other);
    ~map();
    
    size_type size() const;
    bool empty() const;
    void clear();
    int& operator[](int key);
  };
}

// int -> double
namespace std {
  template<> class map<int, double> {
    typedef int key_type;
    typedef double mapped_type;
    typedef size_t size_type;
    
    map();
    map(const map& other);
    ~map();
    
    size_type size() const;
    bool empty() const;
    void clear();
    double& operator[](int key);
  };
}

// string -> string (requires std_string.i)
%include <std_string.i>

namespace std {
  template<> class map<std::string, std::string> {
    typedef std::string key_type;
    typedef std::string mapped_type;
    typedef size_t size_type;
    
    map();
    map(const map& other);
    ~map();
    
    size_type size() const;
    bool empty() const;
    void clear();
    std::string& operator[](const std::string& key);
  };
}
