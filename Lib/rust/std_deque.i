/* -----------------------------------------------------------------------------
 * std_deque.i
 *
 * SWIG typemaps for std::deque<T>
 * Rust implementation
 *
 * Design notes:
 *   - std::deque<T> is wrapped as a struct with methods
 *   - Maps to Rust VecDeque<T> semantics (double-ended queue)
 *   - Provides efficient push/pop at both ends
 *
 * Usage:
 *   %template(IntDeque) std::deque<int>;
 *   RUST_DEQUE_TRAITS(int, IntDeque)
 * ----------------------------------------------------------------------------- */

%{
#include <deque>
#include <algorithm>
#include <stdexcept>
%}

/* -----------------------------------------------------------------------------
 * std::deque template definition
 * ----------------------------------------------------------------------------- */

namespace std {

template<class T> class deque {
public:
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;

  // Constructors
  deque();
  deque(size_type count);
  deque(size_type count, const T& value);
  deque(const deque& other);

  // Capacity
  size_type size() const;
  bool empty() const;
  void resize(size_type count);
  void resize(size_type count, const T& value);

  // Element access
  reference at(size_type pos);
  const_reference at(size_type pos) const;
  reference operator[](size_type pos);
  const_reference operator[](size_type pos) const;
  reference front();
  const_reference front() const;
  reference back();
  const_reference back() const;

  // Modifiers
  void assign(size_type count, const T& value);
  
  // Push/pop at front (deque-specific)
  void push_front(const T& value);
  void push_front(T&& value);
  void pop_front();
  
  // Push/pop at back
  void push_back(const T& value);
  void push_back(T&& value);
  void pop_back();
  
  void clear();
  void swap(deque& other);
};

} // namespace std

/* -----------------------------------------------------------------------------
 * Extended methods for Rust convenience
 * ----------------------------------------------------------------------------- */

%extend std::deque {
  // Get item copy (for Index trait)
  %newobject getitem(size_t index) throw(std::out_of_range);
  T getitem(size_t index) throw(std::out_of_range) {
    if (index >= $self->size())
      throw std::out_of_range("deque index out of range");
    return (*$self)[index];
  }
  
  // Set item (for IndexMut trait)
  void setitem(size_t index, const T& value) throw(std::out_of_range) {
    if (index >= $self->size())
      throw std::out_of_range("deque index out of range");
    (*$self)[index] = value;
  }
  
  // Rust-style method names
  %rename(len) size;
  %rename(is_empty) empty;
  
  // Contains (linear search)
  bool contains(const T& value) const {
    return std::find($self->begin(), $self->end(), value) != $self->end();
  }
  
  // Make room at front (like VecDeque::make_contiguous)
  void make_contiguous() {
    // std::deque is always contiguous in chunks, no-op for compatibility
  }
}

/* -----------------------------------------------------------------------------
 * Helper macro for Rust trait implementations
 *
 * Usage after %template:
 *   %template(IntDeque) std::deque<int>;
 *   RUST_DEQUE_TRAITS(int, IntDeque)
 * ----------------------------------------------------------------------------- */

%define RUST_DEQUE_TRAITS(CTYPE, NAME...)
// Generate Rust trait implementations
%insert("rustcode") %{
impl NAME {
    /// Convert to Rust VecDeque<T>
    pub fn to_vecdeque(&self) -> std::collections::VecDeque<$typemap(rusttype, CTYPE)> {
        let len = self.len();
        let mut deque = std::collections::VecDeque::with_capacity(len);
        for i in 0..len {
            deque.push_back(self.getitem(i));
        }
        deque
    }
    
    /// Create from Rust VecDeque<T>
    pub fn from_vecdeque(deque: std::collections::VecDeque<$typemap(rusttype, CTYPE)>) -> Self {
        let mut result = Self::new();
        for item in deque {
            result.push_back(item);
        }
        result
    }
    
    /// Convert to Rust Vec<T>
    pub fn to_vec(&self) -> Vec<$typemap(rusttype, CTYPE)> {
        let len = self.len();
        let mut vec = Vec::with_capacity(len);
        for i in 0..len {
            vec.push(self.getitem(i));
        }
        vec
    }
    
    /// Create from Rust Vec<T>
    pub fn from_vec(vec: Vec<$typemap(rusttype, CTYPE)>) -> Self {
        let mut result = Self::new();
        for item in vec {
            result.push_back(item);
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
            deque: self,
            index: 0,
        }
    }
}

/// Iterator for NAME
pub struct NAMEIterator {
    deque: NAME,
    index: usize,
}

impl Iterator for NAMEIterator {
    type Item = $typemap(rusttype, CTYPE);
    
    fn next(&mut self) -> Option<Self::Item> {
        if self.index < self.deque.len() {
            let item = self.deque.getitem(self.index);
            self.index += 1;
            Some(item)
        } else {
            None
        }
    }
}

// Implement From<VecDeque<T>>
impl From<std::collections::VecDeque<$typemap(rusttype, CTYPE)>> for NAME {
    fn from(deque: std::collections::VecDeque<$typemap(rusttype, CTYPE)>) -> Self {
        Self::from_vecdeque(deque)
    }
}

// Implement Into<VecDeque<T>>
impl Into<std::collections::VecDeque<$typemap(rusttype, CTYPE)>> for NAME {
    fn into(self) -> std::collections::VecDeque<$typemap(rusttype, CTYPE)> {
        self.to_vecdeque()
    }
}
%}
%enddef

/* -----------------------------------------------------------------------------
 * Common deque specializations
 * ----------------------------------------------------------------------------- */

// Specialization for int
namespace std {
  template<> class deque<int> {
    typedef size_t size_type;
    typedef int value_type;
    typedef int& reference;
    typedef const int& const_reference;
    
    deque();
    deque(size_type count);
    deque(const deque& other);
    ~deque();
    
    size_type size() const;
    bool empty() const;
    void clear();
    void push_front(const int& x);
    void push_back(const int& x);
    void pop_front();
    void pop_back();
    int& operator[](size_type i);
    const int& operator[](size_type i) const;
    int& front();
    int& back();
  };
}

// Specialization for double
namespace std {
  template<> class deque<double> {
    typedef size_t size_type;
    typedef double value_type;
    typedef double& reference;
    typedef const double& const_reference;
    
    deque();
    deque(size_type count);
    deque(const deque& other);
    ~deque();
    
    size_type size() const;
    bool empty() const;
    void clear();
    void push_front(const double& x);
    void push_back(const double& x);
    void pop_front();
    void pop_back();
    double& operator[](size_type i);
    const double& operator[](size_type i) const;
    double& front();
    double& back();
  };
}
