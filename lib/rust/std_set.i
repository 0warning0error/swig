/* -----------------------------------------------------------------------------
 * std_set.i
 *
 * SWIG typemaps for std::set<T, C>
 * Rust implementation
 *
 * Design notes:
 *   - std::set<T> is ordered, maps to Rust BTreeSet<T>
 *   - For unordered sets, use std::unordered_set (maps to HashSet)
 *
 * Usage:
 *   %include <std_set.i>
 *   %template(IntSet) std::set<int>
 * ----------------------------------------------------------------------------- */

%{
#include <set>
#include <algorithm>
#include <stdexcept>
#include <vector>
%}

/* -----------------------------------------------------------------------------
 * std::set template definition
 * ----------------------------------------------------------------------------- */

namespace std {

template<class T, class C = std::less<T>> class set {
public:
  typedef T key_type;
  typedef T value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;

  // Constructors
  set();
  set(const set& other);
  
  // Capacity
  size_type size() const;
  bool empty() const;

  // Modifiers
  void clear();
  void insert(const value_type& value);
  size_type erase(const key_type& key);
  void swap(set& other);

  // Lookup
  size_type count(const key_type& key) const;
};

} // namespace std

/* -----------------------------------------------------------------------------
 * Extended methods for Rust convenience
 * ----------------------------------------------------------------------------- */

%extend std::set {
  // Check if value exists
  bool contains(const T& value) const {
    return $self->find(value) != $self->end();
  }
  
  // Insert and return whether it was new
  bool insert_new(const T& value) {
    std::pair<typename std::set<T>::iterator, bool> result = $self->insert(value);
    return result.second;
  }
  
  // Remove and return whether it existed
  bool remove(const T& value) {
    return $self->erase(value) > 0;
  }
  
  // Get all values as vector (for iteration)
  std::vector<T> to_vec() const {
    std::vector<T> result;
    for (typename std::set<T>::const_iterator it = $self->begin(); it != $self->end(); ++it) {
      result.push_back(*it);
    }
    return result;
  }
  
  // Rust-style names
  %rename(len) size;
  %rename(is_empty) empty;
}

/* -----------------------------------------------------------------------------
 * std::unordered_set (maps to HashSet)
 * ----------------------------------------------------------------------------- */

%{
#include <unordered_set>
%}

namespace std {

template<class T, class H = std::hash<T>, class P = std::equal_to<T>> 
class unordered_set {
public:
  typedef T key_type;
  typedef T value_type;
  typedef size_t size_type;
  typedef const value_type& const_reference;

  unordered_set();
  unordered_set(const unordered_set& other);
  
  size_type size() const;
  bool empty() const;
  void clear();
  void insert(const value_type& value);
  size_type erase(const key_type& key);
  size_type count(const key_type& key) const;
};

} // namespace std

%extend std::unordered_set {
  bool contains(const T& value) const {
    return $self->find(value) != $self->end();
  }
  
  bool insert_new(const T& value) {
    std::pair<typename std::unordered_set<T>::iterator, bool> result = $self->insert(value);
    return result.second;
  }
  
  bool remove(const T& value) {
    return $self->erase(value) > 0;
  }
  
  std::vector<T> to_vec() const {
    std::vector<T> result;
    for (typename std::unordered_set<T>::const_iterator it = $self->begin(); it != $self->end(); ++it) {
      result.push_back(*it);
    }
    return result;
  }
  
  %rename(len) size;
  %rename(is_empty) empty;
}

/* -----------------------------------------------------------------------------
 * Common set specializations
 * ----------------------------------------------------------------------------- */

// Specialization for int
namespace std {
  template<> class set<int> {
    typedef int key_type;
    typedef int value_type;
    typedef size_t size_type;
    
    set();
    set(const set& other);
    ~set();
    
    size_type size() const;
    bool empty() const;
    void clear();
    void insert(const int& x);
    size_type erase(const int& x);
    size_type count(const int& x) const;
  };
}

// Specialization for double
namespace std {
  template<> class set<double> {
    typedef double key_type;
    typedef double value_type;
    typedef size_t size_type;
    
    set();
    set(const set& other);
    ~set();
    
    size_type size() const;
    bool empty() const;
    void clear();
    void insert(const double& x);
    size_type erase(const double& x);
    size_type count(const double& x) const;
  };
}

// Specialization for std::string (requires std_string.i)
%include <std_string.i>

namespace std {
  template<> class set<std::string> {
    typedef std::string key_type;
    typedef std::string value_type;
    typedef size_t size_type;
    
    set();
    set(const set& other);
    ~set();
    
    size_type size() const;
    bool empty() const;
    void clear();
    void insert(const std::string& x);
    size_type erase(const std::string& x);
    size_type count(const std::string& x) const;
  };
}

/* -----------------------------------------------------------------------------
 * Unordered set specializations
 * ----------------------------------------------------------------------------- */

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
    void insert(const int& x);
    size_type erase(const int& x);
    size_type count(const int& x) const;
  };
}

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
    void insert(const std::string& x);
    size_type erase(const std::string& x);
    size_type count(const std::string& x) const;
  };
}