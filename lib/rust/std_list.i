/* -----------------------------------------------------------------------------
 * std_list.i
 *
 * SWIG typemaps for std::list<T>
 * Rust implementation
 *
 * Design notes:
 *   - std::list<T> maps to Rust std::collections::LinkedList<T>
 *   - Provides doubly-linked list with O(1) insert/erase anywhere
 *   - Note: LinkedList is rarely used in Rust; Vec/VecDeque preferred
 *
 * Usage:
 *   %template(IntList) std::list<int>;
 *   RUST_LIST_TRAITS(int, IntList)
 * ----------------------------------------------------------------------------- */

%{
#include <list>
#include <algorithm>
#include <stdexcept>
%}

/* -----------------------------------------------------------------------------
 * std::list template definition
 * ----------------------------------------------------------------------------- */

namespace std {

template<class T> class list {
public:
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T value_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;

  // Constructors
  list();
  list(size_type count);
  list(size_type count, const T& value);
  list(const list& other);

  // Capacity
  size_type size() const;
  bool empty() const;
  void resize(size_type count);
  void resize(size_type count, const T& value);

  // Element access
  reference front();
  const_reference front() const;
  reference back();
  const_reference back() const;

  // Modifiers
  void assign(size_type count, const T& value);
  void push_front(const T& value);
  void push_back(const T& value);
  void pop_front();
  void pop_back();
  
  %rename(insert_val) insert;
  void insert(const T& value);
  
  %rename(erase_val) erase;
  void erase(const T& value);
  
  void clear();
  void swap(list& other);
  
  // Operations
  void remove(const T& value);
  void unique();
  void reverse();
  void sort();
};

} // namespace std

/* -----------------------------------------------------------------------------
 * Extended methods for Rust convenience
 * ----------------------------------------------------------------------------- */

%extend std::list {
  // Rust-style method names
  %rename(len) size;
  %rename(is_empty) empty;
  
  // Get element at index (O(n) for list!)
  %newobject getitem(size_t index) throw(std::out_of_range);
  T getitem(size_t index) throw(std::out_of_range) {
    if (index >= $self->size())
      throw std::out_of_range("list index out of range");
    auto it = $self->begin();
    std::advance(it, index);
    return *it;
  }
  
  // Set element at index (O(n) for list!)
  void setitem(size_t index, const T& value) throw(std::out_of_range) {
    if (index >= $self->size())
      throw std::out_of_range("list index out of range");
    auto it = $self->begin();
    std::advance(it, index);
    *it = value;
  }
  
  // Contains (linear search, O(n))
  bool contains(const T& value) const {
    return std::find($self->begin(), $self->end(), value) != $self->end();
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
 *   %template(IntList) std::list<int>;
 *   RUST_LIST_TRAITS(int, IntList)
 * ----------------------------------------------------------------------------- */

%define RUST_LIST_TRAITS(CTYPE, NAME...)
// Generate Rust trait implementations
%insert("rustcode") %{
impl NAME {
    /// Convert to Rust LinkedList
    pub fn to_linkedlist(&self) -> std::collections::LinkedList<$typemap(rusttype, CTYPE)> {
        let mut list = std::collections::LinkedList::new();
        let vec = self.to_vec();
        for i in 0..vec.len() {
            list.push_back(vec.getitem(i));
        }
        list
    }
    
    /// Create from Rust LinkedList
    pub fn from_linkedlist(list: std::collections::LinkedList<$typemap(rusttype, CTYPE)>) -> Self {
        let mut result = Self::new();
        for val in list {
            result.push_back(val);
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
            result.push_back(val);
        }
        result
    }
}

// Implement IntoIterator
impl IntoIterator for NAME {
    type Item = $typemap(rusttype, CTYPE);
    type IntoIter = NAMEIterator;
    
    fn into_iter(self) -> Self::IntoIter {
        NAMEIterator {
            list: self,
            index: 0,
        }
    }
}

/// Iterator for NAME
pub struct NAMEIterator {
    list: NAME,
    index: usize,
}

impl Iterator for NAMEIterator {
    type Item = $typemap(rusttype, CTYPE);
    
    fn next(&mut self) -> Option<Self::Item> {
        if self.index < self.list.len() {
            let item = self.list.getitem(self.index);
            self.index += 1;
            Some(item)
        } else {
            None
        }
    }
}

// Implement From<LinkedList>
impl From<std::collections::LinkedList<$typemap(rusttype, CTYPE)>> for NAME {
    fn from(list: std::collections::LinkedList<$typemap(rusttype, CTYPE)>) -> Self {
        Self::from_linkedlist(list)
    }
}

// Implement Into<LinkedList>
impl Into<std::collections::LinkedList<$typemap(rusttype, CTYPE)>> for NAME {
    fn into(self) -> std::collections::LinkedList<$typemap(rusttype, CTYPE)> {
        self.to_linkedlist()
    }
}
%}
%enddef

/* -----------------------------------------------------------------------------
 * Common list specializations
 * ----------------------------------------------------------------------------- */

%include <std_vector.i>

// Specialization for int
namespace std {
  template<> class list<int> {
    typedef size_t size_type;
    typedef int value_type;
    typedef int& reference;
    typedef const int& const_reference;
    
    list();
    list(size_type count);
    list(const list& other);
    ~list();
    
    size_type size() const;
    bool empty() const;
    void clear();
    void push_front(const int& x);
    void push_back(const int& x);
    void pop_front();
    void pop_back();
    int& front();
    int& back();
    void remove(const int& value);
    void reverse();
    void sort();
  };
}

// Specialization for double
namespace std {
  template<> class list<double> {
    typedef size_t size_type;
    typedef double value_type;
    typedef double& reference;
    typedef const double& const_reference;
    
    list();
    list(size_type count);
    list(const list& other);
    ~list();
    
    size_type size() const;
    bool empty() const;
    void clear();
    void push_front(const double& x);
    void push_back(const double& x);
    void pop_front();
    void pop_back();
    double& front();
    double& back();
    void remove(const double& value);
    void reverse();
    void sort();
  };
}
