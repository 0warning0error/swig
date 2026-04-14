/* -----------------------------------------------------------------------------
 * std_unordered_set.i
 *
 * SWIG typemaps for std::unordered_set<T>
 * Rust implementation
 *
 * Design notes:
 *   - std::unordered_set<T> maps to Rust HashSet<T>
 *   - Provides hash-based set storage with O(1) average lookup
 *
 * Usage:
 *   %template(IntHashSet) std::unordered_set<int>;
 *   RUST_UNORDERED_SET_TRAITS(int, IntHashSet)
 * ----------------------------------------------------------------------------- */

%{
#include <unordered_set>
#include <stdexcept>
%}

/* -----------------------------------------------------------------------------
 * std::unordered_set template definition
 * ----------------------------------------------------------------------------- */

namespace std {

template<class T> class unordered_set {
public:
  typedef T key_type;
  typedef T value_type;
  typedef size_t size_type;

  // Constructors
  unordered_set();
  unordered_set(const unordered_set& other);

  // Capacity
  size_type size() const;
  bool empty() const;

  // Modifiers
  void clear();
  
  %rename(insert_val) insert;
  void insert(const T& value);
  
  %rename(erase_val) erase;
  size_type erase(const T& value);

  // Lookup
  size_type count(const T& value) const;
  bool contains(const T& value) const;
};

} // namespace std

/* -----------------------------------------------------------------------------
 * Extended methods for Rust convenience
 * ----------------------------------------------------------------------------- */

%extend std::unordered_set {
  // Rust-style method names
  %rename(len) size;
  %rename(is_empty) empty;
  
  // Insert with return indicating if new
  bool insert_new(const T& value) {
    auto result = $self->insert(value);
    return result.second;  // true if inserted, false if already existed
  }
  
  // Remove and return if existed
  bool remove(const T& value) {
    return $self->erase(value) > 0;
  }
  
  // Get all values (as vector)
  %newobject to_vec();
  std::vector<T>* to_vec() const {
    auto* vec = new std::vector<T>();
    for (const auto& val : *$self) {
      vec->push_back(val);
    }
    return vec;
  }
}

/* -----------------------------------------------------------------------------
 * Helper macro for Rust trait implementations
 *
 * Usage after %template:
 *   %template(IntHashSet) std::unordered_set<int>;
 *   RUST_UNORDERED_SET_TRAITS(int, IntHashSet)
 * ----------------------------------------------------------------------------- */

%define RUST_UNORDERED_SET_TRAITS(CTYPE, NAME...)
// Generate Rust trait implementations
%insert("rustcode") %{
impl NAME {
    /// Convert to Rust HashSet
    pub fn to_hashset(&self) -> std::collections::HashSet<$typemap(rusttype, CTYPE)> {
        let mut set = std::collections::HashSet::new();
        let vec = self.to_vec();
        for i in 0..vec.len() {
            set.insert(vec.getitem(i));
        }
        set
    }
    
    /// Create from Rust HashSet
    pub fn from_hashset(set: std::collections::HashSet<$typemap(rusttype, CTYPE)>) -> Self {
        let mut result = Self::new();
        for val in set {
            result.insert_val(val);
        }
        result
    }
    
    /// Convert to Rust Vec<T>
    pub fn to_vector(&self) -> Vec<$typemap(rusttype, CTYPE)> {
        let vec = self.to_vec();
        let mut result = Vec::with_capacity(vec.len());
        for i in 0..vec.len() {
            result.push(vec.getitem(i));
        }
        result
    }
    
    /// Create from Rust Vec<T>
    pub fn from_vec(vec: Vec<$typemap(rusttype, CTYPE)>) -> Self {
        let mut result = Self::new();
        for val in vec {
            result.insert_val(val);
        }
        result
    }
}

// Implement From<HashSet>
impl From<std::collections::HashSet<$typemap(rusttype, CTYPE)>> for NAME {
    fn from(set: std::collections::HashSet<$typemap(rusttype, CTYPE)>) -> Self {
        Self::from_hashset(set)
    }
}

// Implement Into<HashSet>
impl Into<std::collections::HashSet<$typemap(rusttype, CTYPE)>> for NAME {
    fn into(self) -> std::collections::HashSet<$typemap(rusttype, CTYPE)> {
        self.to_hashset()
    }
}
%}
%enddef

/* -----------------------------------------------------------------------------
 * Common unordered_set specializations
 * ----------------------------------------------------------------------------- */

%include <std_vector.i>

// Specialization for int
namespace std {
  template<> class unordered_set<int> {
    typedef int key_type;
    typedef int value_type;
    typedef size_t size_type;
    
    unordered_set();
    unordered_set(const unordered_set& other);
    ~unordered_set();
    
    size_type size() const;
    bool empty() const;
    void clear();
    void insert(const int& value);
    size_type erase(const int& value);
    size_type count(const int& value) const;
    bool contains(const int& value) const;
  };
}

// Specialization for double
namespace std {
  template<> class unordered_set<double> {
    typedef double key_type;
    typedef double value_type;
    typedef size_t size_type;
    
    unordered_set();
    unordered_set(const unordered_set& other);
    ~unordered_set();
    
    size_type size() const;
    bool empty() const;
    void clear();
    void insert(const double& value);
    size_type erase(const double& value);
    size_type count(const double& value) const;
    bool contains(const double& value) const;
  };
}

// Specialization for std::string
%include <std_string.i>

namespace std {
  template<> class unordered_set<std::string> {
    typedef std::string key_type;
    typedef std::string value_type;
    typedef size_t size_type;
    
    unordered_set();
    unordered_set(const unordered_set& other);
    ~unordered_set();
    
    size_type size() const;
    bool empty() const;
    void clear();
    void insert(const std::string& value);
    size_type erase(const std::string& value);
    size_type count(const std::string& value) const;
    bool contains(const std::string& value) const;
  };
}
