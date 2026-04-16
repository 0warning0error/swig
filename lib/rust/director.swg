/* -----------------------------------------------------------------------------
 * director.swg
 *
 * This file contains support for director classes so that Rust trait
 * implementations can be called from C++.
 * ----------------------------------------------------------------------------- */

#ifndef SWIG_DIRECTOR_RUST_HEADER_
#define SWIG_DIRECTOR_RUST_HEADER_

#include <string>
#include <exception>

/*
  Use -DSWIG_DIRECTOR_NORTTI if you prefer to avoid the use of the
  native C++ RTTI and dynamic_cast<>. But be aware that directors
  could stop working when using this option.
*/
#ifdef SWIG_DIRECTOR_NORTTI
/*
   When we don't use the native C++ RTTI, we implement a minimal one
   only for Directors.
*/
# ifndef SWIG_DIRECTOR_RTDIR
# define SWIG_DIRECTOR_RTDIR

#include <map>

namespace Swig {
  class Director;
  SWIGINTERN std::map<void *, Director *>& get_rtdir_map() {
    static std::map<void *, Director *> rtdir_map;
    return rtdir_map;
  }

  SWIGINTERNINLINE void set_rtdir(void *vptr, Director *rtdir) {
    get_rtdir_map()[vptr] = rtdir;
  }

  SWIGINTERNINLINE Director *get_rtdir(void *vptr) {
    std::map<void *, Director *>::const_iterator pos = get_rtdir_map().find(vptr);
    Director *rtdir = (pos != get_rtdir_map().end()) ? pos->second : 0;
    return rtdir;
  }
}
# endif /* SWIG_DIRECTOR_RTDIR */

# define SWIG_DIRECTOR_CAST(ARG) Swig::get_rtdir(static_cast<void *>(ARG))
# define SWIG_DIRECTOR_RGTR(ARG1, ARG2) Swig::set_rtdir(static_cast<void *>(ARG1), ARG2)

#else

# define SWIG_DIRECTOR_CAST(ARG) dynamic_cast<Swig::Director *>(ARG)
# define SWIG_DIRECTOR_RGTR(ARG1, ARG2)

#endif /* SWIG_DIRECTOR_NORTTI */

namespace Swig {
  /* Director base class */
  class Director {
  protected:
    /* Flag indicating if the director is owned by Rust or C++ */
    bool swig_disown_flag;
    
    /* Pointer to the Rust trait object (opaque) */
    void *swig_rust_obj;
    
  public:
    Director() : swig_disown_flag(false), swig_rust_obj(nullptr) {
    }
    
    virtual ~Director() {
    }
    
    /* Set the Rust trait object pointer */
    void swig_set_rust_obj(void *obj) {
      swig_rust_obj = obj;
    }
    
    /* Get the Rust trait object pointer */
    void *swig_get_rust_obj() const {
      return swig_rust_obj;
    }
    
    /* Acquire ownership from Rust (disown from Rust side) */
    void swig_disown() {
      swig_disown_flag = true;
    }
    
    /* Check if owned by C++ */
    bool swig_is_disowned() const {
      return swig_disown_flag;
    }
  };

  /* Base class for director exceptions */
  class DirectorException : public std::exception {
  protected:
    std::string swig_msg;

  public:
    DirectorException(const char *msg) : swig_msg(msg) {
    }

    DirectorException(const std::string &msg) : swig_msg(msg) {
    }

    virtual ~DirectorException() SWIG_NOEXCEPT {
    }

    const char *what() const SWIG_NOEXCEPT {
      return swig_msg.c_str();
    }
  };

  /* Pure virtual method exception */
  class DirectorPureVirtualException : public DirectorException {
  public:
    DirectorPureVirtualException(const char *msg) : DirectorException(std::string("Attempt to invoke pure virtual method ") + msg) {
    }

    static void raise(const char *msg) {
      throw DirectorPureVirtualException(msg);
    }
  };

  /* Type mismatch in the return value */
  class DirectorTypeMismatchException : public DirectorException {
  public:
    DirectorTypeMismatchException(const char *msg = "") 
      : DirectorException(std::string("SWIG director type mismatch: ") + msg) {
    }

    static void raise(const char *msg) {
      throw DirectorTypeMismatchException(msg);
    }
  };

  /* Exception during method call */
  class DirectorMethodException : public DirectorException {
  public:
    DirectorMethodException(const char *msg = "") 
      : DirectorException(std::string("SWIG director method error: ") + msg) {
    }

    static void raise(const char *msg) {
      throw DirectorMethodException(msg);
    }
  };
}

#endif /* SWIG_DIRECTOR_RUST_HEADER_ */
