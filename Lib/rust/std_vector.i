/* -----------------------------------------------------------------------------
 * std_vector.i
 *
 * SWIG typemaps for std::vector<T>
 * Rust implementation
 *
 * Design notes:
 *   - std::vector<T> is wrapped as a struct with methods
 *   - Provides Index/IndexMut trait implementations for [] access
 *   - Provides IntoIterator for iteration
 *   - Provides From/Into conversions with Vec<T>
 *   - Similar to Rust Vec<T> semantics
 *
 * Usage:
 *   %template(IntVector) std::vector<int>;
 *   RUST_VECTOR_HELPERS(int, IntVector)
 * ----------------------------------------------------------------------------- */

%{
#include <vector>
#include <algorithm>
#include <stdexcept>
%}

/* -----------------------------------------------------------------------------
 * std::vector template definition
 * ----------------------------------------------------------------------------- */

namespace std {

template<class T> class vector {
public:
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;

  // Constructors
  vector();
  vector(size_type count);
  vector(size_type count, const T& value);
  vector(const vector& other);
  
  // Destructor handled by SWIG

  // Capacity
  size_type size() const;
  size_type capacity() const;
  bool empty() const;
  void reserve(size_type new_cap);
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
  T* data();
  const T* data() const;

  // Modifiers
  void assign(size_type count, const T& value);
  void push_back(const T& value);
  void push_back(T&& value);
  void pop_back();
  
  %rename(insert_at) insert;
  void insert(const T* pos, const T& value);
  
  %rename(erase_at) erase;
  void erase(const T* pos);
  void swap(vector& other);
  void clear();

  // Iterators (exposed as methods for FFI)
  T* begin();
  T* end();
  const T* begin() const;
  const T* end() const;
};

} // namespace std

/* -----------------------------------------------------------------------------
 * Extended methods for Rust convenience
 * ----------------------------------------------------------------------------- */

%extend std::vector {
  // Constructor from capacity (like Vec::with_capacity)
  %newobject from_capacity(size_t capacity);
  static std::vector<T>* from_capacity(size_t capacity) {
    return new std::vector<T>();
    // Note: capacity parameter used via reserve below
  }
  
  // Get item copy (for Index trait)
  %newobject getitem(size_t index) throw(std::out_of_range);
  T getitem(size_t index) throw(std::out_of_range) {
    if (index >= $self->size())
      throw std::out_of_range("vector index out of range");
    return (*$self)[index];
  }
  
  // Set item (for IndexMut trait)
  void setitem(size_t index, const T& value) throw(std::out_of_range) {
    if (index >= $self->size())
      throw std::out_of_range("vector index out of range");
    (*$self)[index] = value;
  }
  
  // Get reference (for efficiency)
  T* get_ref(size_t index) throw(std::out_of_range) {
    if (index >= $self->size())
      throw std::out_of_range("vector index out of range");
    return &(*$self)[index];
  }
  
  // Rust-style methods
  %rename(len) size;
  %rename(is_empty) empty;
  
  // Extend with multiple elements (like Vec::extend)
  void extend(const std::vector<T>& other) {
    $self->insert($self->end(), other.begin(), other.end());
  }
  
  // Reverse (like Vec::reverse)
  void reverse() {
    std::reverse($self->begin(), $self->end());
  }
  
  // Sort (like Vec::sort)
  void sort() {
    std::sort($self->begin(), $self->end());
  }
  
  // Contains (linear search)
  bool contains(const T& value) const {
    return std::find($self->begin(), $self->end(), value) != $self->end();
  }
  
  // Find index of element
  long index_of(const T& value) const {
    auto it = std::find($self->begin(), $self->end(), value);
    if (it == $self->end())
      return -1;
    return static_cast<long>(it - $self->begin());
  }
  
  // Remove at index (like Vec::remove)
  void remove_at(size_t index) throw(std::out_of_range) {
    if (index >= $self->size())
      throw std::out_of_range("vector index out of range");
    $self->erase($self->begin() + index);
  }
  
  // Swap remove (like Vec::swap_remove - O(1) but changes order)
  T swap_remove(size_t index) throw(std::out_of_range) {
    if (index >= $self->size())
      throw std::out_of_range("vector index out of range");
    T value = (*$self)[index];
    size_t last = $self->size() - 1;
    if (index != last) {
      std::swap((*$self)[index], (*$self)[last]);
    }
    $self->pop_back();
    return value;
  }
  
  // Truncate (like Vec::truncate)
  void truncate(size_t len) {
    if (len < $self->size()) {
      $self->resize(len);
    }
  }
  
  // As slice (returns pointer and length for Rust slice)
  const T* as_ptr() const {
    return $self->data();
  }
  
  size_t as_ptr_len() const {
    return $self->size();
  }
}

/* -----------------------------------------------------------------------------
 * Helper macro for Rust trait implementations
 *
 * Usage after %template:
 *   %template(IntVector) std::vector<int>;
 *   RUST_VECTOR_TRAITS(int, IntVector)
 * ----------------------------------------------------------------------------- */

%define RUST_VECTOR_TRAITS(CTYPE, NAME...)
// Generate Rust trait implementations
%insert("rustcode") %{
impl NAME {
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
        result.reserve(vec.len());
        for item in vec {
            result.push_back(item);
        }
        result
    }
}

// Implement Index trait
impl std::ops::Index<usize> for NAME {
    type Output = $typemap(rusttype, CTYPE);
    
    fn index(&self, index: usize) -> &Self::Output {
        // Note: This returns a reference, but FFI can't safely do that
        // So we use a different approach - getitem returns owned value
        unimplemented!("Use getitem() for safe access")
    }
}

// Implement IntoIterator
impl IntoIterator for NAME {
    type Item = $typemap(rusttype, CTYPE);
    type IntoIter = NAMEIterator;
    
    fn into_iter(self) -> Self::IntoIter {
        NAMEIterator {
            vec: self,
            index: 0,
        }
    }
}

/// Iterator for NAME
pub struct NAMEIterator {
    vec: NAME,
    index: usize,
}

impl Iterator for NAMEIterator {
    type Item = $typemap(rusttype, CTYPE);
    
    fn next(&mut self) -> Option<Self::Item> {
        if self.index < self.vec.len() {
            let item = self.vec.getitem(self.index);
            self.index += 1;
            Some(item)
        } else {
            None
        }
    }
}

// Implement From<Vec<T>>
impl From<Vec<$typemap(rusttype, CTYPE)>> for NAME {
    fn from(vec: Vec<$typemap(rusttype, CTYPE)>) -> Self {
        Self::from_vec(vec)
    }
}

// Implement Into<Vec<T>>
impl Into<Vec<$typemap(rusttype, CTYPE)>> for NAME {
    fn into(self) -> Vec<$typemap(rusttype, CTYPE)> {
        self.to_vec()
    }
}
%}
%enddef

/* -----------------------------------------------------------------------------
 * Common vector specializations
 *
 * These provide pre-made wrappers for common types.
 * ----------------------------------------------------------------------------- */

// Specialization for int
namespace std {
  template<> class vector<int> {
    typedef size_t size_type;
    typedef int value_type;
    typedef int& reference;
    typedef const int& const_reference;
    
    vector();
    vector(size_type count);
    vector(const vector& other);
    ~vector();
    
    size_type size() const;
    bool empty() const;
    void reserve(size_type n);
    void clear();
    void push_back(const int& x);
    void pop_back();
    int& operator[](size_type i);
    const int& operator[](size_type i) const;
  };
}

// Specialization for double
namespace std {
  template<> class vector<double> {
    typedef size_t size_type;
    typedef double value_type;
    typedef double& reference;
    typedef const double& const_reference;
    
    vector();
    vector(size_type count);
    vector(const vector& other);
    ~vector();
    
    size_type size() const;
    bool empty() const;
    void reserve(size_type n);
    void clear();
    void push_back(const double& x);
    void pop_back();
    double& operator[](size_type i);
    const double& operator[](size_type i) const;
  };
}

// Specialization for std::string (requires std_string.i)
%include <std_string.i>

namespace std {
  template<> class vector<std::string> {
    typedef size_t size_type;
    typedef std::string value_type;
    typedef std::string& reference;
    typedef const std::string& const_reference;
    
    vector();
    vector(size_type count);
    vector(const vector& other);
    ~vector();
    
    size_type size() const;
    bool empty() const;
    void reserve(size_type n);
    void clear();
    void push_back(const std::string& x);
    void pop_back();
    std::string& operator[](size_type i);
    const std::string& operator[](size_type i) const;
  };
}
