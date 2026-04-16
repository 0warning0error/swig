/* -----------------------------------------------------------------------------
 * std_pair.i
 *
 * SWIG typemaps for std::pair<T, U>
 * Rust implementation
 *
 * Design notes:
 *   - std::pair<T, U> maps to Rust tuple (T, U)
 *   - .first -> .0
 *   - .second -> .1
 *   - Provides From/Into conversions between pair and tuple
 * ----------------------------------------------------------------------------- */

%{
#include <utility>
%}

namespace std {

/* -----------------------------------------------------------------------------
 * std::pair template definition
 *
 * This is a simple wrapper that exposes first and second members.
 * Rust users typically access via .0 and .1 tuple syntax.
 * ----------------------------------------------------------------------------- */

template<class T, class U> struct pair {
  typedef T first_type;
  typedef U second_type;

  // Constructors
  pair();
  pair(T first, U second);
  pair(const pair& other);

  // Members
  T first;
  U second;
};

/* -----------------------------------------------------------------------------
 * Typemaps for std::pair<T, U>
 *
 * The Rust tuple type ($typemap(rusttype, T), $typemap(rusttype, U))
 * ----------------------------------------------------------------------------- */

// Note: Full typemap specialization requires %template instantiation
// The basic struct above allows SWIG to generate wrappers for pair<T, U>

} // namespace std

/* -----------------------------------------------------------------------------
 * Helper macro for pair instantiation
 *
 * Usage:
 *   %template(IntStringPair) std::pair<int, std::string>;
 *   RUST_PAIR_HELPERS(int, std::string, IntStringPair)
 * ----------------------------------------------------------------------------- */

%define RUST_PAIR_HELPERS(T, U, NAME...)
// Generate From/Into implementations for tuple conversion
%insert("rustcode") %{
impl From<NAME> for ($typemap(rusttype, T), $typemap(rusttype, U)) {
    fn from(pair: NAME) -> Self {
        (pair.first, pair.second)
    }
}

impl From<($typemap(rusttype, T), $typemap(rusttype, U))> for NAME {
    fn from(tuple: ($typemap(rusttype, T), $typemap(rusttype, U))) -> Self {
        NAME::new(tuple.0, tuple.1)
    }
}
%}
%enddef
