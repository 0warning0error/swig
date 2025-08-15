/* -----------------------------------------------------------------------------
 * This file is part of SWIG, which is licensed as a whole under version 3
 * (or any later version) of the GNU General Public License. Some additional
 * terms also apply to certain portions of SWIG. The full details of the SWIG
 * license and copyrights can be found in the LICENSE and COPYRIGHT files
 * included with the SWIG source code as distributed by the SWIG developers
 * and at https://www.swig.org/legal.html.
 *
 * rust.cxx
 *
 * Rust language module for SWIG.
 * -----------------------------------------------------------------------------
 */

#include "cparse.h"
#include "swigmod.h"
#include <ctype.h>

/* ----------------------------------------------------------------------
 * siphash()
 *
 * 64-bit SipHash-2-4 to generate unique id for each module
 * ---------------------------------------------------------------------- */

// An unsigned 64-bit integer that works on a 32-bit host.
typedef struct {
  // Assume unsigned long is at least 32 bits.
  unsigned long hi;
  unsigned long lo;
} swig_uint64;

// Rotate v left by bits, which must be <= 32.
static inline void _rotl(swig_uint64 *v, int bits) {
  assert(bits <= 32);
  unsigned long tmp = v->hi;
  if (bits == 32) {
    v->hi = v->lo;
    v->lo = tmp;
  } else {
    v->hi = (tmp << bits) | ((0xfffffffful & v->lo) >> (32 - bits));
    v->lo = (v->lo << bits) | ((0xfffffffful & tmp) >> (32 - bits));
  }
}

// dst ^= src
static inline void _xor(swig_uint64 *dst, swig_uint64 *src) {
  dst->lo ^= src->lo;
  dst->hi ^= src->hi;
}

// dst += src
static inline void _add(swig_uint64 *dst, swig_uint64 *src) {
  dst->lo += src->lo;
  dst->hi +=
      src->hi + ((dst->lo & 0xfffffffful) < (src->lo & 0xfffffffful) ? 1 : 0);
}
#define SIPROUND                                                               \
  do {                                                                         \
    _add(&v0, &v1);                                                            \
    _rotl(&v1, 13);                                                            \
    _xor(&v1, &v0);                                                            \
    _rotl(&v0, 32);                                                            \
    _add(&v2, &v3);                                                            \
    _rotl(&v3, 16);                                                            \
    _xor(&v3, &v2);                                                            \
    _add(&v0, &v3);                                                            \
    _rotl(&v3, 21);                                                            \
    _xor(&v3, &v0);                                                            \
    _add(&v2, &v1);                                                            \
    _rotl(&v1, 17);                                                            \
    _xor(&v1, &v2);                                                            \
    _rotl(&v2, 32);                                                            \
  } while (0)

// Set out to the hash of inc/inlen.
static void siphash(swig_uint64 *out, const char *inc, unsigned long inlen) {
  /* "somepseudorandomlygeneratedbytes" */
  swig_uint64 v0 = {0x736f6d65UL, 0x70736575UL};
  swig_uint64 v1 = {0x646f7261UL, 0x6e646f6dUL};
  swig_uint64 v2 = {0x6c796765UL, 0x6e657261UL};
  swig_uint64 v3 = {0x74656462UL, 0x79746573UL};
  swig_uint64 b;
  /* hard-coded k. */
  swig_uint64 k0 = {0x07060504UL, 0x03020100UL};
  swig_uint64 k1 = {0x0F0E0D0CUL, 0x0B0A0908UL};
  int i;
  const int cROUNDS = 2, dROUNDS = 4;
  const unsigned char *in = (const unsigned char *)inc;
  const unsigned char *end = in + inlen - (inlen % 8);
  int left = inlen & 7;
  _xor(&v3, &k1);
  _xor(&v2, &k0);
  _xor(&v1, &k1);
  _xor(&v0, &k0);
  for (; in != end; in += 8) {
    b.hi = 0;
    b.lo = 0;
    for (i = 0; i < 4; i++) {
      b.lo |= ((unsigned long)in[i]) << (8 * i);
    }
    for (i = 0; i < 4; i++) {
      b.hi |= ((unsigned long)in[i + 4]) << (8 * i);
    }
    _xor(&v3, &b);
    for (i = 0; i < cROUNDS; i++) {
      SIPROUND;
    }
    _xor(&v0, &b);
  }
  b.hi = (inlen & 0xff) << 24;
  b.lo = 0;
  for (; left; left--) {
    if (left > 4) {
      b.hi |= ((unsigned long)in[left - 1]) << (8 * left - 8 - 32);
    } else {
      b.lo |= ((unsigned long)in[left - 1]) << (8 * left - 8);
    }
  }
  _xor(&v3, &b);
  for (i = 0; i < cROUNDS; i++) {
    SIPROUND;
  }
  _xor(&v0, &b);
  v2.lo ^= 0xff;
  for (i = 0; i < dROUNDS; i++) {
    SIPROUND;
  }
  out->lo = 0;
  out->hi = 0;
  _xor(out, &v0);
  _xor(out, &v1);
  _xor(out, &v2);
  _xor(out, &v3);
}
#undef SIPROUND

class RUST : public Language {
  static const char *const usage;

  // Rust package name.
  String *package;
  // SWIG module name.
  String *module;
  // Flag for generating gccrust output.
  bool gccrust_flag;
  // Prefix to use with gccrust.
  String *rust_prefix;
  // -frust-prefix option.
  String *prefix_option;
  // -frust-pkgpath option.
  String *pkgpath_option;
  // Prefix for translating %import directive to import statements.
  String *import_prefix;
  // Whether to use a shared library.
  bool use_shlib;
  // Name of shared library to import.
  String *soname;
  // Size in bits of the Rust type "int".  0 if not specified.
  int intrust_type_size;

  /* Output files */
  File *f_c_begin;
  File *f_rust_begin;

  /* Output fragments */
  File *f_c_runtime;
  File *f_c_header;
  File *f_c_wrappers;
  File *f_c_init;
  File *f_c_directors;
  File *f_c_directors_h;
  File *f_rust_imports;
  File *f_rust_runtime;
  File *f_rust_header;
  File *f_rust_wrappers;
  File *f_rust_directors;
  File *f_crust_comment;
  File *f_crust_comment_typedefs;
  File *f_rust_pre_extern;
  File *f_rust_after_director_extern;
  File *f_rust_traits_wrappers;
  File *f_rust_trait_base_calls;

  // True if we imported a module.
  bool saw_import;
  // If not NULL, name of import package being processed.
  String *imported_package;
  // Build interface methods while handling a class.  This is only
  // non-NULL when we are handling methods.
  String *interfaces;
  // The class node while handling a class.  This is only non-NULL
  // when we are handling methods.
  Node *class_node;
  // The class name while handling a class.  This is only non-NULL
  // when we are handling methods.  This is the name of the class as
  // SWIG sees it.
  String *class_name;
  // The receiver name while handling a class.  This is only non-NULL
  // when we are handling methods.  This is the name of the class
  // as run through rustCPointerType.
  String *class_receiver;
  // A hash table of method names that we have seen when processing a
  // class.  This lets us detect base class methods that we don't want
  // to use.
  Hash *class_methods;
  // True when we are generating the wrapper functions for a variable.
  bool making_variable_wrappers;
  // True when working with a static member function.
  bool is_static_member_function;
  // A hash table of enum types that we have seen but which may not have
  // been defined.  The index is a SwigType.
  Hash *undefined_enum_types;
  // A hash table of types that we have seen but which may not have
  // been defined.  The index is a SwigType.
  Hash *undefined_types;
  // A hash table of classes which were defined.  The index is a Rust
  // type name.
  Hash *defined_types;
  // A hash table of all the rust_imports already imported. The index is a full
  // import name e.g. '"runtime"' or '_ "runtime/crust"' or 'sc "syscall"'.
  Hash *rust_imports;
  // A unique ID used to make public symbols unique.
  String *unique_id;

public:
  RUST()
      : package(NULL), module(NULL), gccrust_flag(false), rust_prefix(NULL),
        prefix_option(NULL), pkgpath_option(NULL), import_prefix(NULL),
        use_shlib(false), soname(NULL), intrust_type_size(0), f_c_begin(NULL),
        f_rust_begin(NULL), f_c_runtime(NULL), f_c_header(NULL),
        f_c_wrappers(NULL), f_c_init(NULL), f_c_directors(NULL),
        f_c_directors_h(NULL), f_rust_imports(NULL), f_rust_runtime(NULL),
        f_rust_header(NULL), f_rust_wrappers(NULL), f_rust_directors(NULL),f_crust_comment(NULL), 
        f_crust_comment_typedefs(NULL), f_rust_pre_extern(NULL),
        f_rust_after_director_extern(NULL), f_rust_traits_wrappers(NULL),
        f_rust_trait_base_calls(NULL),saw_import(false), imported_package(NULL), interfaces(NULL),
        class_node(NULL), class_name(NULL), class_receiver(NULL), class_methods(NULL),
        making_variable_wrappers(false), is_static_member_function(false),
        undefined_enum_types(NULL), undefined_types(NULL), defined_types(NULL),
        rust_imports(NULL), unique_id(NULL) {
    director_multiple_inheritance = 1;
    directorLanguage();
    director_prot_ctor_code = NewString("_swig_rustpanic(\"accessing abstract "
                                        "class or protected constructor\");");
  }

private:
  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */
  virtual void main(int argc, char *argv[]) {

    SWIG_library_directory("rust");
    // bool saw_nocrust_flag = false;

    // Process command line options.
    // for (int i = 1; i < argc; i++) {
    //   if (argv[i]) {
    // if (strcmp(argv[i], "-package") == 0) {
    //   if (argv[i + 1]) {
    //     package = NewString(argv[i + 1]);
    //     Swig_mark_arg(i);
    //     Swig_mark_arg(i + 1);
    //     i++;
    //   } else {
    //     Swig_arg_error();
    //   }
    // } else if (strcmp(argv[i], "-crust") == 0) {
    //   Swig_mark_arg(i);
    // } else if (strcmp(argv[i], "-no-crust") == 0) {
    //   Swig_mark_arg(i);
    //   saw_nocrust_flag = true;
    // } else if (strcmp(argv[i], "-gccrust") == 0) {
    //   Swig_mark_arg(i);
    //   gccrust_flag = true;
    // } else if (strcmp(argv[i], "-rust-prefix") == 0) {
    //   if (argv[i + 1]) {
    //     prefix_option = NewString(argv[i + 1]);
    //     Swig_mark_arg(i);
    //     Swig_mark_arg(i + 1);
    //     i++;
    //   } else {
    //     Swig_arg_error();
    //   }
    // } else if (strcmp(argv[i], "-rust-pkgpath") == 0) {
    //   if (argv[i + 1]) {
    //     pkgpath_option = NewString(argv[i + 1]);
    //     Swig_mark_arg(i);
    //     Swig_mark_arg(i + 1);
    //     i++;
    //   } else {
    //     Swig_arg_error();
    //   }
    // } else if (strcmp(argv[i], "-import-prefix") == 0) {
    //   if (argv[i + 1]) {
    //     import_prefix = NewString(argv[i + 1]);
    //     Swig_mark_arg(i);
    //     Swig_mark_arg(i + 1);
    //     i++;
    //   } else {
    //     Swig_arg_error();
    //   }
    // } else if (strcmp(argv[i], "-use-shlib") == 0) {
    //   Swig_mark_arg(i);
    //   use_shlib = true;
    // } else if (strcmp(argv[i], "-soname") == 0) {
    //   if (argv[i + 1]) {
    //     soname = NewString(argv[i + 1]);
    //     Swig_mark_arg(i);
    //     Swig_mark_arg(i + 1);
    //     i++;
    //   } else {
    //     Swig_arg_error();
    //   }
    // } else if (strcmp(argv[i], "-longsize") == 0) {
    //   // Ignore for backward compatibility.
    //   if (argv[i + 1]) {
    //     Swig_mark_arg(i);
    //     Swig_mark_arg(i + 1);
    //     ++i;
    //   } else {
    //     Swig_arg_error();
    //   }
    // } else if (strcmp(argv[i], "-intrustsize") == 0) {
    //   if (argv[i + 1]) {
    //     intrust_type_size = atoi(argv[i + 1]);
    //     if (intrust_type_size != 32 && intrust_type_size != 64) {
    //       Printf(stderr, "-intrustsize not 32 or 64\n");
    //       Swig_arg_error();
    //     }
    //     Swig_mark_arg(i);
    //     Swig_mark_arg(i + 1);
    //     ++i;
    //   } else {
    //     Swig_arg_error();
    //   }
    // } else if (strcmp(argv[i], "-help") == 0) {
    //   Printf(stdout, "%s\n", usage);
    // }
    //   }
    // }

    // if (saw_nocrust_flag) {
    //   Printf(stderr, "SWIG -rust: -no-crust option is no longer
    //   supported\n"); Exit(EXIT_FAILURE);
    // }

    if (gccrust_flag && !pkgpath_option && !prefix_option) {
      prefix_option = NewString("rust");
    }

    // Add preprocessor symbol to parser.
    Preprocessor_define("SWIGRUST 1", 0);

    // if (gccrust_flag) {
    //   Preprocessor_define("SWIGRUST_GCCRUST 1", 0);
    // }

    if (intrust_type_size == 32) {
      Preprocessor_define("SWIGRUST_INTRUST_SIZE 32", 0);
    } else if (intrust_type_size == 64) {
      Preprocessor_define("SWIGRUST_INTRUST_SIZE 64", 0);
    } else {
      Preprocessor_define("SWIGRUST_INTRUST_SIZE 64", 0);
    }

    SWIG_config_file("rust.swg");

    allow_overloading();
  }

  /* ---------------------------------------------------------------------
   * top()
   *
   * For rust, we are going to create the following files:
   *
   * 1) A .c or .cxx file compiled with gcc.  This file will contain
   *    function wrappers.  Each wrapper will take a pointer to a
   *    struct holding the arguments, unpack them, and call the real
   *    function.
   *
   * 2) A .rs file which defines the Rust form of all types, and which
   *    defines Rust function wrappers.  Each wrapper will call the C
   *    function wrapper in the second file.
   *
   * 3) A .c file compiled with 6c/8c.  This file will define
   *    Rust-callable C function wrappers.  Each wrapper will use
   *    crustcall to call the function wrappers in the first file.
   *
   * When generating code for gccrust, we don't need the third file, and
   * the function wrappers in the first file have a different form.
   *
   * --------------------------------------------------------------------- */

  virtual int top(Node *n) {
    Node *optionsnode = Getattr(Getattr(n, "module"), "options");
    if (optionsnode) {
      if (Getattr(optionsnode, "directors")) {
        allow_directors();
      }
      if (Getattr(optionsnode, "dirprot")) {
        allow_dirprot();
      }
      allow_allprotected(GetFlag(optionsnode, "allprotected"));
    }

    module = Getattr(n, "name");
    if (!package) {
      package = Copy(module);
    }
    if (!soname && use_shlib) {
      soname = Copy(package);
      Append(soname, ".so");
    }

    // if (gccrust_flag) {
    //   String *pref;
    //   if (pkgpath_option) {
    // pref = pkgpath_option;
    //   } else {
    // pref = prefix_option;
    //   }
    //   rust_prefix = NewString("");
    //   for (char *p = Char(pref); *p != '\0'; p++) {
    // if ((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') || (*p >= '0' &&
    // *p <= '9') || *p == '.' || *p == '$') {
    //   Putc(*p, rust_prefix);
    // } else {
    //   Putc('_', rust_prefix);
    // }
    //   }
    //   if (!pkgpath_option) {
    // Append(rust_prefix, ".");
    // Append(rust_prefix, getModuleName(package));
    //   }
    // }

    // Get filenames.

    String *swig_filename = Getattr(n, "infile");
    String *c_filename = Getattr(n, "outfile");
    String *c_filename_h = Getattr(n, "outfile_h");

    String *rust_filename = NewString("");
    Printf(rust_filename, "%s%s.rs", SWIG_output_directory(), module);

    // Generate a unique ID based on a hash of the SWIG input.
    swig_uint64 hash = {0, 0};
    FILE *swig_input = Swig_open(swig_filename);
    if (swig_input == NULL) {
      FileErrorDisplay(swig_filename);
      Exit(EXIT_FAILURE);
    }
    String *swig_input_content = Swig_read_file(swig_input);
    siphash(&hash, Char(swig_input_content), Len(swig_input_content));
    Delete(swig_input_content);
    fclose(swig_input);
    unique_id = NewString("");
    Printf(unique_id, "_%s_%08x%08x", getModuleName(package), hash.hi, hash.lo);

    // Open files.

    f_c_begin = NewFile(c_filename, "w", SWIG_output_files());
    if (!f_c_begin) {
      FileErrorDisplay(c_filename);
      Exit(EXIT_FAILURE);
    }

    if (Swig_directors_enabled()) {
      if (!c_filename_h) {
        Printf(stderr, "Unable to determine outfile_h\n");
        Exit(EXIT_FAILURE);
      }
      f_c_directors_h = NewFile(c_filename_h, "w", SWIG_output_files());
      if (!f_c_directors_h) {
        FileErrorDisplay(c_filename_h);
        Exit(EXIT_FAILURE);
      }
    }

    f_rust_begin = NewFile(rust_filename, "w", SWIG_output_files());
    if (!f_rust_begin) {
      FileErrorDisplay(rust_filename);
      Exit(EXIT_FAILURE);
    }

    f_c_runtime = NewString("");
    f_c_header = NewString("");
    f_c_wrappers = NewString("");
    f_c_init = NewString("");
    f_c_directors = NewString("");
    f_rust_imports = NewString("");
    f_rust_runtime = NewString("");
    f_rust_header = NewString("");
    f_rust_wrappers = NewString("");
    f_rust_directors = NewString("");
    f_crust_comment = NewString("");
    f_crust_comment_typedefs = NewString("");
    f_rust_pre_extern = NewString("");
    f_rust_after_director_extern = NewString("");
    f_rust_traits_wrappers = NewString("");
    f_rust_trait_base_calls = NewString("");

    Swig_register_filebyname("begin", f_c_begin);
    Swig_register_filebyname("runtime", f_c_runtime);
    Swig_register_filebyname("header", f_c_header);
    Swig_register_filebyname("wrapper", f_c_wrappers);
    Swig_register_filebyname("init", f_c_init);
    Swig_register_filebyname("director", f_c_directors);
    Swig_register_filebyname("director_h", f_c_directors_h);
    Swig_register_filebyname("rust_begin", f_rust_begin);
    Swig_register_filebyname("rust_imports", f_rust_imports);
    Swig_register_filebyname("rust_runtime", f_rust_runtime);
    Swig_register_filebyname("rust_header", f_rust_header);
    Swig_register_filebyname("rust_wrapper", f_rust_wrappers);
    Swig_register_filebyname("rust_director", f_rust_directors);
    Swig_register_filebyname("crust_comment", f_crust_comment);
    Swig_register_filebyname("crust_comment_typedefs",
                             f_crust_comment_typedefs);
    Swig_register_filebyname("rust_pre_extern", f_rust_pre_extern);

    Swig_banner(f_c_begin);

    Swig_obligatory_macros(f_c_runtime, "RUST");

    if (CPlusPlus) {
      Printf(f_c_begin, "\n// source: %s\n\n", swig_filename);
    } else {
      Printf(f_c_begin, "\n/* source: %s */\n\n", swig_filename);
    }

    Printf(f_c_runtime, "#define SWIGMODULE %s\n", module);

    if (gccrust_flag) {
      Printf(f_c_runtime, "#define SWIGRUST_PREFIX %s\n", rust_prefix);
    }

    if (Swig_directors_enabled()) {
      Printf(f_c_runtime, "#define SWIG_DIRECTORS\n");

      Swig_banner(f_c_directors_h);
      Printf(f_c_directors_h, "\n// source: %s\n\n", swig_filename);

      Printf(f_c_directors_h, "#ifndef SWIG_%s_WRAP_H_\n", module);
      Printf(f_c_directors_h, "#define SWIG_%s_WRAP_H_\n\n", module);
      Printf(f_c_directors_h, "class Swig_memory;\n\n");

      Printf(f_c_directors, "\n// C++ director class methods.\n");
      String *filename = Swig_file_filename(c_filename_h);
      Printf(f_c_directors, "#include \"%s\"\n\n", filename);
      Delete(filename);
    }

    Swig_banner(f_rust_begin);
    Printf(f_rust_begin, "\n// source: %s\n", swig_filename);

    Printv(f_crust_comment_typedefs, "/*\n", NULL);

    // The crust program defines the intrust type after our function
    // definitions, but we want those definitions to be able to use
    // intrust also.
    // Printv(f_crust_comment_typedefs, "#define intrust swig_intrust\n", NULL);
    // Printv(f_crust_comment_typedefs, "typedef void *swig_voidp;\n", NULL);

    // Output module initialization code.

    // Printf(f_rust_begin, "\npackage %s\n\n", getModuleName(package));

    // All the C++ wrappers should be extern "C".

    Printv(f_c_wrappers, "#ifdef __cplusplus\n", "extern \"C\" {\n",
           "#endif\n\n", NULL);

    // Set up the hash table for types not defined by SWIG.

    undefined_enum_types = NewHash();
    undefined_types = NewHash();
    defined_types = NewHash();
    rust_imports = NewHash();

    // Emit code.

    Language::top(n);

    if (Swig_directors_enabled()) {
      // Insert director runtime into the f_runtime file (make it occur before
      // %header section)
      Swig_insert_file("director_common.swg", f_c_runtime);
      Swig_insert_file("director.swg", f_c_runtime);
    }

    Delete(rust_imports);

    // Write out definitions for the types not defined by SWIG.

    if (Len(undefined_enum_types) > 0)
      Printv(f_rust_wrappers, "\n", NULL);
    for (Iterator p = First(undefined_enum_types); p.key; p = Next(p)) {
      String *name = p.item;
      Printv(f_rust_wrappers, "type ", name, "= i32;\n", NULL);
    }

    Printv(f_rust_wrappers, "\n", NULL);
    for (Iterator p = First(undefined_types); p.key; p = Next(p)) {
      String *ty = rustType(NULL, p.key);
      if (!Getattr(defined_types, ty)) {
        String *cp = rustCPointerType(p.key, false);
        if (!Getattr(defined_types, cp)) {
          // Printv(f_rust_wrappers, "type ", cp, " = usize;\n", NULL);
          // Printv(f_rust_wrappers, "trait ", ty, " {\n", NULL);
          // Printv(f_rust_wrappers, "\tfn Swigcptr() -> usize;\n", NULL);
          // Printv(f_rust_wrappers, "}\n", NULL);
          // Printv(f_rust_wrappers, "impl ", ty, " for ", cp, "{\n", NULL);
          // Printv(f_rust_wrappers,
          //        "\tfn Swigcptr(&self) -> usize { return self as usize; }\n",
          //        NULL);
          // Printv(f_rust_wrappers, "}\n\n", NULL);
        }
        Delete(cp);
      }
      Delete(ty);
    }
    Delete(undefined_enum_types);
    Delete(undefined_types);
    Delete(defined_types);

    /* Write and cleanup */

    Dump(f_c_header, f_c_runtime);

    if (Swig_directors_enabled()) {
      Printf(f_c_directors_h, "#endif\n");
      Delete(f_c_directors_h);
      f_c_directors_h = NULL;

      Dump(f_c_directors, f_c_runtime);
      Delete(f_c_directors);
      f_c_directors = NULL;
    }

    // End the extern "C".
    Printv(f_c_wrappers, "#ifdef __cplusplus\n", "}\n", "#endif\n\n", NULL);

    // End the crust comment.
    // Printv(f_crust_comment, "#undef intrust\n", NULL);
    // Printv(f_crust_comment, "*/\n", NULL);
    // Printv(f_crust_comment, "import \"C\"\n", NULL);
    // Printv(f_crust_comment, "\n", NULL);

    bool need_panic = false;
    if (Strstr(f_c_runtime, "SWIG_contract_assert(") != 0 ||
        Strstr(f_c_wrappers, "SWIG_contract_assert(") != 0) {
      Printv(f_c_begin,
             "\n#define SWIG_contract_assert(expr, msg) if (!(expr)) { "
             "_swig_rustpanic(msg); } else\n\n",
             NULL);
      need_panic = true;
    }

    if (!gccrust_flag &&
        (need_panic || Strstr(f_c_runtime, "_swig_rustpanic") != 0 ||
         Strstr(f_c_wrappers, "_swig_rustpanic") != 0)) {
      Printv(f_rust_header, "#[no_mangle]", "\n", NULL);
      Printv(f_rust_header, "pub extern \"C\" fn rust_panic_", unique_id,
             "(p : *const i8) {\n", NULL);
      Printv(f_rust_header,
             "\tlet c_str = unsafe { ::std::ffi::CStr::from_ptr(p) };\n", NULL);
      Printv(f_rust_header, "\tmatch c_str.to_str() {\n", NULL);
      Printv(f_rust_header, "\t\tOk(rust_str) => panic!(\"{}\", rust_str),\n",
             NULL);
      Printv(f_rust_header,
             "\t\tErr(_) => panic!(\"received str from C/C++ but cannot "
             "convert it to Rust string\"),\n",
             NULL);
      Printv(f_rust_header, "\t}\n", NULL);
      Printv(f_rust_header, "}\n\n", NULL);

      Printv(f_c_begin, "\nextern\n", NULL);
      Printv(f_c_begin, "#ifdef __cplusplus\n", NULL);
      Printv(f_c_begin, "  \"C\"\n", NULL);
      Printv(f_c_begin, "#endif\n", NULL);
      Printv(f_c_begin, "  void rust_panic_", unique_id, "(const char*);\n",
             NULL);
      Printv(f_c_begin, "static void _swig_rustpanic(const char *p) {\n", NULL);
      Printv(f_c_begin, "  rust_panic_", unique_id, "(p);\n", NULL);
      Printv(f_c_begin, "}\n\n", NULL);
    }

    Dump(f_c_runtime, f_c_begin);
    Dump(f_c_wrappers, f_c_begin);
    Dump(f_c_init, f_c_begin);
    // Dump(f_crust_comment_typedefs, f_rust_begin);
    // Dump(f_crust_comment, f_rust_begin);
    Dump(f_rust_imports, f_rust_begin);
    Dump(f_rust_pre_extern, f_rust_begin);
    Dump(f_rust_header, f_rust_begin);
    Dump(f_rust_runtime, f_rust_begin);
    Dump(f_rust_wrappers, f_rust_begin);
    Dump(f_rust_after_director_extern, f_rust_begin);
    if (Swig_directors_enabled()) {
      Dump(f_rust_directors, f_rust_begin);
      Dump(f_rust_traits_wrappers,f_rust_begin);
      Dump(f_rust_trait_base_calls,f_rust_begin);
    }
    Delete(f_c_runtime);
    Delete(f_c_header);
    Delete(f_c_wrappers);
    Delete(f_c_init);
    Delete(f_rust_imports);
    Delete(f_rust_runtime);
    Delete(f_rust_header);
    Delete(f_rust_wrappers);
    Delete(f_rust_directors);
    Delete(f_crust_comment);
    Delete(f_crust_comment_typedefs);
    Delete(f_rust_pre_extern);
    Delete(f_rust_after_director_extern);
    Delete(f_rust_traits_wrappers);
    Delete(f_rust_trait_base_calls);
    Delete(f_c_begin);
    Delete(f_rust_begin);

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * importDirective()
   *
   * Handle a SWIG import statement by generating a Rust import
   * statement.
   * ------------------------------------------------------------ */

  virtual int importDirective(Node *n) {
    String *hold_import = imported_package;
    String *modname = Getattr(n, "module");
    if (modname) {
      if (!Getattr(rust_imports, modname)) {
        Setattr(rust_imports, modname, modname);
        Printv(f_rust_imports, "use ", NULL);
        if (import_prefix) {
          Printv(f_rust_imports, import_prefix, "::", NULL);
        }
        Printv(f_rust_imports, modname, ";\n", NULL);
      }
      imported_package = modname;
      saw_import = true;
    }
    int r = Language::importDirective(n);
    imported_package = hold_import;
    return r;
  }

  /* ----------------------------------------------------------------------
   * Language::insertDirective()
   *
   * If the section is rust_imports, store them for later.
   * ---------------------------------------------------------------------- */
  virtual int insertDirective(Node *n) {
    char *section = Char(Getattr(n, "section"));
    if ((ImportMode && !Getattr(n, "generated")) || !section ||
        (strcmp(section, "rust_imports") != 0)) {
      return Language::insertDirective(n);
    }

    char *code = Char(Getattr(n, "code"));
    char *pch = strtok(code, ",");
    while (pch != NULL) {
      // Do not import same thing more than once.
      if (!Getattr(rust_imports, pch)) {
        Setattr(rust_imports, pch, pch);
        Printv(f_rust_imports, "use ", pch, ";\n", NULL);
      }
      pch = strtok(NULL, ",");
    }
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * functionWrapper()
   *
   * Implement a function.
   * ---------------------------------------------------------------------- */

  virtual int functionWrapper(Node *n) {
    if (GetFlag(n, "feature:ignore")) {
      return SWIG_OK;
    }

    // We don't need explicit calls.
    if (GetFlag(n, "explicitcall")) {
      return SWIG_OK;
    }

    String *name = Getattr(n, "sym:name");
    String *nodetype = Getattr(n, "nodeType");
    bool is_static = is_static_member_function || isStatic(n);
    bool is_friend = isFriend(n);
    bool is_ctor_dtor = false;

    SwigType *result = Getattr(n, "type");

    // For some reason SWIG changs the "type" value during the call to
    // functionWrapper.  We need to remember the type for possible
    // overload processing.
    Setattr(n, "rust:type", Copy(result));

    String *rust_name;

    String *r1 = NULL;
    if (making_variable_wrappers) {
      // Change the name of the variable setter and getter functions
      // to be more Rust like.
      // C++ get/set function use non const pointer to the instance
      // this will cause Rust requires mutable references. 
      // somehow troublesome
      bool is_set = Strcmp(Char(name) + Len(name) - 4, "_set") == 0;
      assert(is_set || Strcmp(Char(name) + Len(name) - 4, "_get") == 0);

      // Start with Set or Get.
      rust_name = NewString(is_set ? "Set" : "Get");

      // If this is a static variable, put in the class name,
      // capitalized.
      if (is_static && class_name) {
        String *ccn = exportedName(class_name);
        Append(rust_name, ccn);
        Delete(ccn);
      }

      // Add the rest of the name, capitalized, dropping the _set or
      // _get.
      String *c1 = removeClassname(name);
      String *c2 = exportedName(c1);
      char *p = Char(c2);
      int len = Len(p);
      for (int i = 0; i < len - 4; ++i) {
        Putc(p[i], rust_name);
      }
      Delete(c2);
      Delete(c1);

      if (!checkIgnoredParameters(n, rust_name)) {
        Delete(rust_name);
        return SWIG_NOWRAP;
      }
    } else if (Cmp(nodetype, "constructor") == 0) {
      is_ctor_dtor = true;

      // Change the name of a constructor to be more Rust like.  Change
      // new_ to New, and capitalize the class name.
      assert(Strncmp(name, "new_", 4) == 0);
      String *c1 = NewString(Char(name) + 4);
      String *c2 = exportedName(c1);
      rust_name = NewString("New");
      Append(rust_name, c2);
      Delete(c2);
      Delete(c1);

      if (Swig_methodclass(n) && Swig_directorclass(n)) {
        // The core SWIG code skips the first parameter when
        // generating the $nondirector_new string.  Recreate the
        // action in this case.  But don't it if we are using the
        // special code for an abstract class.
        String *call =
            Swig_cppconstructor_call(getClassType(), Getattr(n, "parms"));
        SwigType *type = Copy(getClassType());
        SwigType_add_pointer(type);
        String *cres = Swig_cresult(type, Swig_cresult_name(), call);
        if (!Equal(Getattr(n, "wrap:action"), director_prot_ctor_code)) {
          Setattr(n, "wrap:action", cres);
        }
      }
    } else if (Cmp(nodetype, "destructor") == 0) {
      // No need to emit protected destructors.
      if (!is_public(n)) {
        return SWIG_OK;
      }

      is_ctor_dtor = true;

      // Change the name of a destructor to be more Rust like.  Change
      // delete_ to Delete and capitalize the class name.
      assert(Strncmp(name, "delete_", 7) == 0);
      String *c1 = NewString(Char(name) + 7);
      String *c2 = exportedName(c1);
      rust_name = NewString("Delete");
      Append(rust_name, c2);
      Delete(c2);
      Delete(c1);

      result = NewString("void");
      r1 = result;
    } else {
      if (!checkFunctionVisibility(n, NULL)) {
        return SWIG_OK;
      }

      rust_name = buildRustName(name, is_static, is_friend);

      if (!checkIgnoredParameters(n, rust_name)) {
        Delete(rust_name);
        return SWIG_NOWRAP;
      }
    }

    String *overname = NULL;
    if (Getattr(n, "sym:overloaded")) {
      overname = Getattr(n, "sym:overname");
    } else {
      String *scope;
      if (!class_name || is_static || is_ctor_dtor) {
        scope = NULL;
      } else {
        scope = NewString("swigrustscope.");
        Append(scope, class_name);
      }
      if (!checkNameConflict(rust_name, n, scope)) {
        Delete(rust_name);
        return SWIG_NOWRAP;
      }
    }

    String *wname = Swig_name_wrapper(name);
    if (overname) {
      Append(wname, overname);
    }
    Append(wname, unique_id);
    Setattr(n, "wrap:name", wname);

    ParmList *parms = Getattr(n, "parms");
    Setattr(n, "wrap:parms", parms);

    int r = makeWrappers(n, rust_name, overname, wname, NULL, parms, result,
                         is_static);
    if (r != SWIG_OK) {
      return r;
    }

    if (Getattr(n, "sym:overloaded") && !Getattr(n, "sym:nextSibling")) {
      String *scope;
      if (!class_name || is_static || is_ctor_dtor) {
        scope = NULL;
      } else {
        scope = NewString("swigrustscope.");
        Append(scope, class_name);
      }
      if (!checkNameConflict(rust_name, n, scope)) {
        Delete(rust_name);
        return SWIG_NOWRAP;
      }

      String *receiver = class_receiver;
      if (is_static || is_ctor_dtor) {
        receiver = NULL;
      }
      // todo
      // r = makeDispatchFunction(n, rust_name, receiver, is_static, NULL, false);
      if (r != SWIG_OK) {
        return r;
      }
    }

    Delete(wname);
    Delete(rust_name);
    Delete(r1);

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * staticmemberfunctionHandler()
   *
   * For some reason the language code removes the "storage" attribute
   * for a static function before calling functionWrapper, which means
   * that we have no way of knowing whether a function is static or
   * not.  That makes no sense in the Rust context.  Here we note that a
   * function is static.
   * ---------------------------------------------------------------------- */

  int staticmemberfunctionHandler(Node *n) {
    assert(!is_static_member_function);
    is_static_member_function = true;
    int r = Language::staticmemberfunctionHandler(n);
    is_static_member_function = false;
    return r;
  }

  /* ----------------------------------------------------------------------
   * makeWrappers()
   *
   * Write out the various function wrappers.
   * n: The function we are emitting.
   * rust_name: The name of the function in Rust.
   * overname: The overload string for overloaded function.
   * wname: The SWIG wrapped name--the name of the C function.
   * base: A list of the names of base classes, in the case where this
   *       is a virtual method not defined in the current class.
   * parms: The parameters.
   * result: The result type.
   * is_static: Whether this is a static method or member.
   * ---------------------------------------------------------------------- */

  int makeWrappers(Node *n, String *rust_name, String *overname, String *wname,
                   List *base, ParmList *parms, SwigType *result,
                   bool is_static) {

    assert(result);

    int ret = SWIG_OK;

    int r = makeRustWrappers(n, rust_name, overname, wname, base, parms,
                              result, is_static);
    if (r != SWIG_OK) {
      ret = r;
    }

    if (class_methods) {
      Setattr(class_methods, Getattr(n, "name"), NewString(""));
    }

    return ret;
  }

  /* ----------------------------------------------------------------------
   * struct cppRustWrapperInfo
   *
   * Information needed by the CRUST wrapper functions.
   * ---------------------------------------------------------------------- */

  struct cppRustWrapperInfo {
    // The function we are generating code for.
    Node *n;
    // The name of the Rust function.
    String *rust_name;
    // The overload string for an overloaded function.
    String *overname;
    // The name of the C wrapper function.
    String *wname;
    // The base classes.
    List *base;
    // The parameters.
    ParmList *parms;
    // The result type.
    SwigType *result;
    // Whether this is a static function, not a class method.
    bool is_static;
    // The Rust receiver type.
    String *receiver;
    // Whether this is a class constructor.
    bool is_constructor;
    // Whether this is a class destructor.
    bool is_destructor;
  };

  /* ----------------------------------------------------------------------
   * makeRustWrappers()
   *
   * Write out the wrappers for a function when producing Rust input
   * files.
   * ---------------------------------------------------------------------- */

  int makeRustWrappers(Node *n, String *rust_name, String *overname,
                        String *wname, List *base, ParmList *parms,
                        SwigType *result, bool is_static) {
    Swig_save("makeRustWrappers", n, "emit:crusttype", "emit:crusttypestruct",
              NULL);

    cppRustWrapperInfo info;

    info.n = n;
    info.rust_name = rust_name;
    info.overname = overname;
    info.wname = wname;
    info.base = base;
    info.parms = parms;
    info.result = result;
    info.is_static = is_static;

    info.receiver = class_receiver;
    if (is_static) {
      info.receiver = NULL;
    }

    String *nodetype = Getattr(n, "nodeType");
    info.is_constructor = Cmp(nodetype, "constructor") == 0;
    info.is_destructor = Cmp(nodetype, "destructor") == 0;
    if (info.is_constructor || info.is_destructor) {
      assert(class_receiver);
      assert(!base);
      info.receiver = NULL;
    }

    int ret = SWIG_OK;

    int r = callCppRustWrapper(&info);
    if (r != SWIG_OK) {
      ret = r;
    }

    r = rustExternWrapper(&info);
    if (r != SWIG_OK) {
      ret = r;
    }

    r = cppWrapper(&info);
    if (r != SWIG_OK) {
      ret = r;
    }

    Swig_restore(n);

    return ret;
  }

  /* ----------------------------------------------------------------------
   * callCppRustWrapper()
   *
   * Write out Rust code to call a C/C++ function.  This code will rust into
   * the generated Rust output file.
   * ---------------------------------------------------------------------- */
  int callCppRustWrapper(const cppRustWrapperInfo *info) {

    Wrapper *dummy = initRustTypemaps(info->parms);

    bool add_to_interface = interfaces && !info->is_constructor && !info->is_static &&
                            !info->overname &&
                            checkFunctionVisibility(info->n, NULL);

    // Printv(f_rust_wrappers, "func ", NULL);
    Printv(f_rust_wrappers, "pub fn ", NULL);

    Parm *p = info->parms;
    int pi = 0;

    // Add the receiver first if this is a method.
    //   if (info->receiver) {
    //     Printv(f_rust_wrappers, "(", NULL);
    //     if (info->base && info->receiver) {
    // Printv(f_rust_wrappers, "_swig_base", NULL);
    //     } else {
    // Printv(f_rust_wrappers, Getattr(p, "lname"), NULL);
    // p = nextParm(p);
    // ++pi;
    //     }
    //     Printv(f_rust_wrappers, " ", info->receiver, ") ", NULL);
    //   }

    Printv(f_rust_wrappers, info->rust_name, NULL);
    if (info->overname) {
      Printv(f_rust_wrappers, info->overname, NULL);
    }
    Printv(f_rust_wrappers, "(", NULL);

    // If we are doing methods, add this method to the interface.
    // if (add_to_interface) {
    //   Printv(interfaces, "\tfn ", info->rust_name, "(", NULL);
    // }

    // Write out the parameters to both the function definition and
    // the interface.

    String *parm_print = NewString("");

    int parm_count = emit_num_arguments(info->parms);
    int required_count = emit_num_required(info->parms);
    int args = 0;

    bool is_mut_self_ref = false;
    bool is_self = false;
    bool is_const_function = false;
    String *is_member_get = Getattr(info->n,"memberget");

    if (info->base) {
      parm_count += 1;
      required_count += 1;
      if (add_to_interface) {
        String * type = Getattr(info->n, "qualifier");
        if (type && isStringConst(type)){
          Printv(parm_print, "&self", NULL);
          is_const_function = true;
        } else if(is_member_get != NULL && Strcmp(is_member_get,"1") == 0 ){
          Printv(parm_print, "&self", NULL);
          is_const_function = true;
        } else if(!info->is_destructor) {
          is_mut_self_ref = true;
          Printv(parm_print, "&mut self", NULL);
        } else {
          Printv(parm_print, "mut self", NULL);
          is_self = true;
        }
        ++args;
      }
      pi++;
    }

    for (; pi < parm_count; ++pi) {
      p = getParm(p);
      if (pi == 0 && add_to_interface) {
        SwigType * type = Getattr(p, "type");
        if (type && isStringConst(type)){
          Printv(parm_print, "&self", NULL);
          is_const_function = true;
        } else if(is_member_get != NULL && Strcmp(is_member_get,"1") == 0 ){
          Printv(parm_print, "&self", NULL);
          is_const_function = true;
        } else if(!info->is_destructor) {
          is_mut_self_ref = true;
          Printv(parm_print, "&mut self", NULL);
        } else {
          Printv(parm_print, "mut self", NULL);
          is_self = true;
        }
        ++args;
      } else {
        if (args > 0) {
          Printv(parm_print, ", ", NULL);
        }
        ++args;
        if (pi >= required_count) {
          Printf(stderr, "Warning: Currently swig for Rust doesn't support default parameters, skip\n");
          // Printv(parm_print, "_swig_args ...interface{}", NULL);
          break;
        }
        Printv(parm_print, Getattr(p, "lname"), " : ", NULL);
        String * lname_ty = Getattr(p, "type");
        String *tm = rustType(p, lname_ty);
        Printv(parm_print, tm, NULL);
        Delete(tm);
      }
      p = nextParm(p);
    }

    Printv(parm_print, ")", NULL);

    // Write out the result type.
    if (info->is_constructor) {
      String *cl = exportedName(class_name);
      Printv(parm_print, " -> *mut ", cl, " ", NULL);
      Delete(cl);
    } else {
      if (SwigType_type(info->result) != T_VOID) {
        String *tm = rustType(info->n, info->result);
        Printv(parm_print, " -> ", tm, NULL);
        
        Delete(tm);
      }
    }

    Printv(f_rust_wrappers, parm_print, NULL);
    if (add_to_interface) {
      Printv(interfaces, parm_print, ";\n", NULL);
    }

    // Write out the function body.

    Printv(f_rust_wrappers, " {\n", NULL);

    if (parm_count > required_count) {
      Parm *p = info->parms;
      int i;
      for (i = 0; i < required_count; ++i) {
        p = getParm(p);
        p = nextParm(p);
      }
      for (; i < parm_count; ++i) {
        p = getParm(p);
        String *tm = rustImType(p, Getattr(p, "type"));
        Printv(f_rust_wrappers, "\tlet ", Getattr(p, "lname"), " : ", tm, ";\n",
               NULL);
        Printf(f_rust_wrappers, "\tif len(_swig_args) > %d {\n",
               i - required_count);
        Printf(f_rust_wrappers, "\t\t%s = _swig_args[%d].(%s)\n",
               Getattr(p, "lname"), i - required_count, tm);
        Printv(f_rust_wrappers, "\t}\n", NULL);
        Delete(tm);
        p = nextParm(p);
      }
    }

    String *call = NewString("\t");

    String *ret_type = NULL;
    bool memcpy_ret = false;
    String *wt = NULL;
    bool has_swig_convert = false;
    if (SwigType_type(info->result) != T_VOID) {
      if (info->is_constructor) {
        ret_type = NewString("*mut ");
        Append(ret_type,exportedName(class_name));
        // ret_type = exportedName(class_name);
      } else {
        ret_type = rustImType(info->n, info->result);
      }
      // Printv(f_rust_wrappers, "\tvar swig_r ", ret_type, "\n", NULL);

      bool c_struct_type;
      Delete(crustTypeForRustValue(info->n, info->result, &c_struct_type));
      if (c_struct_type) {
        memcpy_ret = true;
      }

      if (memcpy_ret) {
        Printv(call, "let swig_r_p = ", NULL);
      } else {
        Printv(call, "let swig_r : ", ret_type, " = swig_conv!(", NULL);
        has_swig_convert = true;
      }

      if (info->is_constructor || rustTypeIsTrait(info->n, info->result)) {
        if (info->is_constructor) {
          wt = Copy(class_receiver);
        } else {
          wt = rustWrapperType(info->n, info->result, true);
        }
        if (info->is_constructor) {

        } else {
          Printv(call, wt, "(", NULL);
        }
      }
    }

    Printv(call, "unsafe {", info->wname, "(", NULL);

    args = 0;

    if (parm_count > required_count) {
      Printv(call, "C.swig_intrust(len(_swig_args))", NULL);
      ++args;
    }

    if (info->receiver) {

      if (args > 0) {
        Printv(call, ", ", NULL);
      }
      

      if(info->base) {
        ++args;
        if(is_self) {
          Printv(call, "swig_conv!(&mut self,*mut ",info->receiver,")",NULL);
        } else if(is_mut_self_ref) {
          Printv(call, "swig_conv!(self,*mut ",info->receiver,")",NULL);
        } else {
          Printv(call, "swig_conv!(self,*const ",info->receiver,")",NULL);
        }
      } else {
        // if(is_self) {
        //   Printf(call, "swig_struct_conv!(&mut &mut self)");
        // } else if(is_mut_self_ref) {
        //   Printf(call, "swig_struct_conv!(&mut self)");
        // } else {
        //   Printf(call, "swig_struct_conv!(&self)");
        // }
      }
      // Printv(call, "_swig_base as uszie", NULL);
    }

    p = info->parms;
    int i = 0;
    if (info->base) {
      i = 1;
    }

    for (; i < parm_count; ++i) {
      p = getParm(p);
      if (args > 0) {
        Printv(call, ", ", NULL);
      }
      ++args;

      SwigType *pt = Getattr(p, "type");
      String *ln = Getattr(p, "lname");
      if(i == 0 && add_to_interface) {
        ln = NewString("self");
      } else {
        ln = Getattr(p, "lname");
      }

      String *ivar = NewStringf("_swig_i_%d", i);

      String *rustin = rustGetattr(p, "tmap:rustin");
      if (rustin == NULL) {
        if(i == 0 && add_to_interface) {
          if(is_member_get && Strcmp(is_member_get,"1") == 0){
            pt = Copy(pt);
            // is_special_processed = true;
            SwigType_del_pointer(pt);
            SwigType_add_qualifier(pt, "const");
            SwigType_add_pointer(pt);
          }
          String *itm = rustImType(p, pt);
          Printv(f_rust_wrappers, "\tlet ", ivar, " : ", itm, " = ", NULL);
          bool need_close = false;
          
          if(is_mut_self_ref) {
            Printv(f_rust_wrappers,"swig_struct_conv!(&mut ", ln, NULL);
          } else if(is_self) {
            if(info->is_destructor) {
              Printv(f_rust_wrappers,"swig_struct_conv!(&mut &mut self", NULL);
            } else {
              Printv(f_rust_wrappers,"swig_struct_conv!(&self", NULL);
            }
          } else {
            Printv(f_rust_wrappers,"swig_struct_conv!(&", ln, NULL);
          }

          Printv(f_rust_wrappers, ");\n", NULL);
          Setattr(p, "emit:rustinput", ln);
        }  else {
          String *itm = rustImType(p, pt);
          Printv(f_rust_wrappers, "\tlet ", ivar, " : ", itm, " = swig_conv!(", NULL);
          bool need_close = false;
          if ((i == 0 && info->is_destructor) ||
              ((i > 0 || !info->receiver || info->base || info->is_constructor) &&
               rustTypeIsTrait(p, pt))) {
            Printv(f_rust_wrappers, "get_swig_cptr(", NULL);
            need_close = true;
          }
          Printv(f_rust_wrappers, ln, NULL);
          if (need_close) {
            Printv(f_rust_wrappers, ")", NULL);
          }
          Printv(f_rust_wrappers, ",",itm,");\n", NULL);
          Setattr(p, "emit:rustinput", ln);
        }
      } else {
        String *itm = rustImType(p, pt);
        Printv(f_rust_wrappers, "\tlet ", ivar, " : ", itm, "\n", NULL);
        rustin = Copy(rustin);
        Replaceall(rustin, "$input", ln);
        Replaceall(rustin, "$result", ivar);
        Printv(f_rust_wrappers, rustin, "\n", NULL);
        Delete(rustin);
        Setattr(p, "emit:rustinput", ivar);
      }

      bool c_struct_type;
      String *ct = rustTypeForCppValue(p, pt, &c_struct_type);
      if (c_struct_type) {
        Printv(call, "*(*C.", ct, ")(unsafe.Pointer(&", ivar, "))", NULL);
      } else {
        Printv(call,  ivar, NULL);
      }
      Delete(ct);

      p = nextParm(p);
    }
    if(has_swig_convert) {
      Printv(f_rust_wrappers, call,")"," } ," ,ret_type, NULL);
    } else {
      Printv(f_rust_wrappers, call, ")", " } " , NULL);
    }
    Delete(call);

    if (SwigType_type(info->result) != T_VOID && !memcpy_ret) {
      // Close the type conversion of the return value.
      Printv(f_rust_wrappers, ")", NULL);
    }
    if (info->is_constructor) {
      // If this is a constructor, we need to return the pointer to the
      // new object.
      // String *tm = rustType(info->n, info->result);
      // Printv(f_rust_wrappers, " as usize as *mut ", tm, ";\n", NULL);
      Printv(f_rust_wrappers, ";\n", NULL);
    } else {
      if (wt) {
        // Close the type conversion to the wrapper type.
        Printv(f_rust_wrappers, ")", NULL);
      }
      // if(SwigType_type(info->result) != T_VOID) {
      //   Printv(f_rust_wrappers, ".clone().into();\n", NULL);
      // } else {
      //   Printv(f_rust_wrappers, ";\n", NULL);
      // }
      Printv(f_rust_wrappers, ";\n", NULL);
    } 

    if (memcpy_ret) {
      Printv(f_rust_wrappers, "\tswig_r = ", "(unsafe.Pointer(&swig_r_p)).clone().into();\n", NULL);
    }
    if (ret_type) {
      Delete(ret_type);
    }

    rustargout(info->parms);

    if (SwigType_type(info->result) != T_VOID) {

      Swig_save("callCppRustWrapper", info->n, "type", "tmap:rustout", NULL);
      Setattr(info->n, "type", info->result);

      String *rustout = rustTypemapLookup("rustout", info->n, "swig_r");
      if (rustout == NULL) {
        Printv(f_rust_wrappers, "\treturn swig_r;\n", NULL);
      } else {
        String *tm = rustType(info->n, info->result);
        Printv(f_rust_wrappers, "\tlet swig_r_1 : ", tm, " ;\n", NULL);
        rustout = Copy(rustout);
        Replaceall(rustout, "$input", "swig_r");
        Replaceall(rustout, "$result", "swig_r_1");
        Printv(f_rust_wrappers, rustout, "\n", NULL);
        Printv(f_rust_wrappers, "\treturn swig_r_1;\n", NULL);
      }

      Swig_restore(info->n);
    }

    Printv(f_rust_wrappers, "}\n\n", NULL);

    DelWrapper(dummy);

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * rustExternWrapper()
   *
   * Write out a rust function to call a C/C++ function.  This code
   * will rust into the crust comment in the generated Rust output file.
   * ---------------------------------------------------------------------- */
  int rustExternWrapper(const cppRustWrapperInfo *info) {
    String *ret_type;
    if (SwigType_type(info->result) == T_VOID) {
      ret_type = NULL;
    } else {
      bool c_struct_type;
      ret_type = rustTypeForCppValue(info->n, info->result,&c_struct_type);
    }

    // Printv(f_crust_comment, "extern ", ret_type, " ", info->wname, "(",
    // NULL);
    Printv(f_rust_pre_extern, "extern \"C\" { fn ", info->wname, "(", NULL);

    int parm_count = emit_num_arguments(info->parms);
    int required_count = emit_num_required(info->parms);
    int args = 0;

    if (parm_count > required_count) {
      // Printv(f_crust_comment, "intrust _swig_args", NULL);
      ++args;
    }

    if (info->base && info->receiver) {
      if (args > 0) {
        Printv(f_crust_comment, ", ", NULL);
        Printv(f_rust_pre_extern, ", ", NULL);
      }
      ++args;
      String *ct = rustImType(info->n, info->receiver);
      String *qualifier = Getattr(info->n,"qualifier");
      if (qualifier && isStringConst(qualifier)){
        Printv(f_rust_pre_extern, "base_ptr : *const ", ct ,NULL);
      } else {
        Printv(f_rust_pre_extern, "base_ptr : *mut ", ct ,NULL);
      }

      Printv(f_crust_comment, "uintptr_t _swig_base", NULL);
      // Printv(f_rust_pre_extern, "base_ptr : ", ct ,NULL);
    }

    Parm *p = info->parms;
    for (int i = 0; i < parm_count; ++i) {
      p = getParm(p);
      SwigType *pt = Getattr(p, "type");
      String *ln = Getattr(p, "lname");
      String *is_member_get = Getattr(info->n,"memberget");
      bool is_special_processed = false;

      // special memberget process to reduce requirement of member_get
      // (let getter code from &mut self to &self)
      // Regardless of the actual C++ code parameters,
      // we only reduce the requirements in Rust.
      if(args == 0 && is_member_get != NULL && Strcmp(is_member_get,"1") == 0) {
        // bypass param check
        pt = Copy(pt);
        is_special_processed = true;
        SwigType_del_pointer(pt);
        SwigType_add_qualifier(pt, "const");
        SwigType_add_pointer(pt);
      }
      if (args > 0) {
        Printv(f_crust_comment, ", ", NULL);
        Printv(f_rust_pre_extern, ", ", NULL);
      }
      ++args;


      bool c_struct_type;
      String *ct = rustImType(p, pt);
      Printv(f_crust_comment, ct, " ", ln, NULL);
      Printv(f_rust_pre_extern, ln, " : ", ct, NULL);
      Delete(ct);
      if(is_special_processed) {
        Delete(pt);
      }

      p = nextParm(p);
    }

    if (args == 0) {
      Printv(f_crust_comment, "void", NULL);
    }

    Printv(f_crust_comment, ");\n", NULL);
    Printv(f_rust_pre_extern, ")", NULL);

    if (NULL != ret_type) {
      Printv(f_rust_pre_extern, " -> ", ret_type, " ; }\n", NULL);
    } else {
      Printv(f_rust_pre_extern, " ; }\n", NULL);
    }

    Delete(ret_type);
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * cppWrapper()
   *
   * Write out code to the C/C++ wrapper file.  This code will be
   * called by the code generated by rustExternWrapper.
   * ---------------------------------------------------------------------- */
  int cppWrapper(const cppRustWrapperInfo *info) {
    Wrapper *f = NewWrapper();

    Swig_save("cppWrapper", info->n, "parms", NULL);

    ParmList *parms = info->parms;

    Parm *base_parm = NULL;
    if (info->base && !isStatic(info->n)) {
      SwigType *base_type = Copy(getClassType());
      SwigType_add_pointer(base_type);
      base_parm = NewParm(base_type, NewString("arg1"), info->n);
      set_nextSibling(base_parm, parms);
      parms = base_parm;
    }

    emit_parameter_variables(parms, f);
    emit_attach_parmmaps(parms, f);
    int parm_count = emit_num_arguments(parms);
    int required_count = emit_num_required(parms);

    emit_return_variable(info->n, info->result, f);

    // Start the function definition.

    String *fnname = NewString("");
    Printv(fnname, info->wname, "(", NULL);

    int args = 0;

    if (parm_count > required_count) {
      Printv(fnname, "intrust _swig_optargc", NULL);
      ++args;
    }

    Parm *p = parms;
    for (int i = 0; i < parm_count; ++i) {
      if (args > 0) {
        Printv(fnname, ", ", NULL);
      }
      ++args;

      p = getParm(p);

      SwigType *pt = Copy(Getattr(p, "type"));
      if (SwigType_isarray(pt) && Getattr(p, "tmap:rusttype") == NULL) {
        SwigType_del_array(pt);
        SwigType_add_pointer(pt);
      }
      String *pn = NewStringf("_swig_rust_%d", i);
      String *ct = gcCTypeForRustValue(p, pt, pn);
      Printv(fnname, ct, NULL);
      Delete(ct);
      Delete(pn);
      Delete(pt);

      p = nextParm(p);
    }

    Printv(fnname, ")", NULL);

    if (SwigType_type(info->result) == T_VOID) {
      Printv(f->def, "void ", fnname, NULL);
    } else {
      String *ct = gcCTypeForRustValue(info->n, info->result, fnname);
      Printv(f->def, ct, NULL);
      Delete(ct);

      String *ln = NewString("_swig_rust_result");
      ct = gcCTypeForRustValue(info->n, info->result, ln);
      Wrapper_add_local(f, "_swig_rust_result", ct);
      Delete(ct);
      Delete(ln);
    }

    Delete(fnname);

    Printv(f->def, " {\n", NULL);

    // Apply the in typemaps.

    p = parms;
    for (int i = 0; i < parm_count; ++i) {
      p = getParm(p);
      String *tm = Getattr(p, "tmap:in");
      if (!tm) {
        Swig_warning(WARN_TYPEMAP_IN_UNDEF, input_file, line_number,
                     "unable to use type %s as a function argument\n",
                     SwigType_str(Getattr(p, "type"), 0));
      } else {
        tm = Copy(tm);
        String *pn = NewStringf("_swig_rust_%d", i);
        Replaceall(tm, "$input", pn);
        if (i < required_count) {
          Printv(f->code, "\t", tm, "\n", NULL);
        } else {
          Printf(f->code, "\tif (_swig_optargc > %d) {\n", i - required_count);
          Printv(f->code, "\t\t", tm, "\n", NULL);
          Printv(f->code, "\t}\n", NULL);
        }
        Delete(tm);
        Setattr(p, "emit:input", pn);
      }
      p = nextParm(p);
    }

    Printv(f->code, "\n", NULL);

    // Do the real work of the function.

    checkConstraints(parms, f);

    emitRustAction(info->n, info->base, parms, info->result, f);

    argout(parms, f);

    cleanupFunction(info->n, f, parms);

    if (SwigType_type(info->result) != T_VOID) {
      Printv(f->code, "\treturn _swig_rust_result;\n", NULL);
    }

    Printv(f->code, "}\n", NULL);

    Wrapper_print(f, f_c_wrappers);

    Swig_restore(info->n);

    DelWrapper(f);
    if (base_parm) {
      Delete(base_parm);
    }

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * initRustTypemaps()
   *
   * Initialize the typenames for a Rust wrapper, returning a dummy
   * Wrapper*.  Also set consistent names for the parameters.
   * ---------------------------------------------------------------------- */

  Wrapper *initRustTypemaps(ParmList *parms) {
    Wrapper *dummy = NewWrapper();
    emit_attach_parmmaps(parms, dummy);

    Parm *p = parms;
    int parm_count = emit_num_arguments(parms);
    for (int i = 0; i < parm_count; ++i) {
      p = getParm(p);
      Swig_cparm_name(p, i);
      p = nextParm(p);
    }

    Swig_typemap_attach_parms("default", parms, dummy);
    Swig_typemap_attach_parms("rusttype", parms, dummy);
    Swig_typemap_attach_parms("rustin", parms, dummy);
    Swig_typemap_attach_parms("rustargout", parms, dummy);
    Swig_typemap_attach_parms("imtype", parms, dummy);

    return dummy;
  }

  /* -----------------------------------------------------------------------
   * checkConstraints()
   *
   * Check parameter constraints if any.  This is used for the C/C++
   * function.  This assumes that each parameter has an "emit:input"
   * property with the name to use to refer to that parameter.
   * ----------------------------------------------------------------------- */

  void checkConstraints(ParmList *parms, Wrapper *f) {
    Parm *p = parms;
    while (p) {
      String *tm = Getattr(p, "tmap:check");
      if (!tm) {
        p = nextSibling(p);
      } else {
        tm = Copy(tm);
        Replaceall(tm, "$input", Getattr(p, "emit:input"));
        Printv(f->code, tm, "\n\n", NULL);
        Delete(tm);
        p = Getattr(p, "tmap:check:next");
      }
    }
  }

  /* -----------------------------------------------------------------------
   * emitRustAction()
   *
   * Emit the action of the function.  This is used for the C/C++ function.
   * ----------------------------------------------------------------------- */

  void emitRustAction(Node *n, List *base, ParmList *parms, SwigType *result,
                      Wrapper *f) {
    String *actioncode;
    if (!base || isStatic(n)) {
      Swig_director_emit_dynamic_cast(n, f);
      actioncode = emit_action(n);
    } else {
      // Call the base class method.
      actioncode = NewString("");

      String *current = NewString("");
      Printv(current, Getattr(parms, "lname"), NULL);

      int vc = 0;
      for (Iterator bi = First(base); bi.item; bi = Next(bi)) {
        Printf(actioncode, "  %s *swig_b%d = (%s *)%s;\n", bi.item, vc, bi.item,
               current);
        Delete(current);
        current = NewString("");
        Printf(current, "swig_b%d", vc);
        ++vc;
      }

      String *code = Copy(Getattr(n, "wrap:action"));
      Replace(code, Getattr(parms, "lname"), current,
              DOH_REPLACE_ANY | DOH_REPLACE_ID);
      Delete(current);
      Printv(actioncode, code, "\n", NULL);
    }

    Swig_save("emitRustAction", n, "type", "tmap:out", NULL);

    Setattr(n, "type", result);

    String *tm =
        Swig_typemap_lookup_out("out", n, Swig_cresult_name(), f, actioncode);
    if (!tm) {
      Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number,
                   "Unable to use return type %s\n", SwigType_str(result, 0));
    } else {
      Replaceall(tm, "$result", "_swig_rust_result");
      if (GetFlag(n, "feature:new")) {
        Replaceall(tm, "$owner", "1");
      } else {
        Replaceall(tm, "$owner", "0");
      }
      Printv(f->code, tm, "\n", NULL);
      Delete(tm);
    }

    Swig_restore(n);
  }

  /* -----------------------------------------------------------------------
   * argout()
   *
   * Handle argument output code if any.  This is used for the C/C++
   * function.  This assumes that each parameter has an "emit:input"
   * property with the name to use to refer to that parameter.
   * ----------------------------------------------------------------------- */

  void argout(ParmList *parms, Wrapper *f) {
    Parm *p = parms;
    while (p) {
      String *tm = Getattr(p, "tmap:argout");
      if (!tm) {
        p = nextSibling(p);
      } else {
        tm = Copy(tm);
        Replaceall(tm, "$result", Swig_cresult_name());
        Replaceall(tm, "$input", Getattr(p, "emit:input"));
        Printv(f->code, tm, "\n", NULL);
        Delete(tm);
        p = Getattr(p, "tmap:argout:next");
      }
    }
  }

  /* -----------------------------------------------------------------------
   * rustargout()
   *
   * Handle Rust argument output code if any.  This is used for the Rust
   * function.  This assumes that each parameter has an "emit:rustinput"
   * property with the name to use to refer to that parameter.
   * ----------------------------------------------------------------------- */

  void rustargout(ParmList *parms) {
    Parm *p = parms;
    while (p) {
      String *tm = Getattr(p, "tmap:rustargout");
      if (!tm) {
        p = nextSibling(p);
      } else {
        tm = Copy(tm);
        Replaceall(tm, "$result", "swig_r");
        Replaceall(tm, "$input", Getattr(p, "emit:rustinput"));
        Printv(f_rust_wrappers, tm, "\n", NULL);
        Delete(tm);
        p = Getattr(p, "tmap:rustargout:next");
      }
    }

    // If we need to memcpy a parameter to pass it to the C code, the
    // compiler may think that the parameter is not live during the
    // function call.  If the garbage collector runs while the C/C++
    // function is running, the parameter may be freed.  Force the
    // compiler to see the parameter as live across the C/C++ function.
    int parm_count = emit_num_arguments(parms);
    p = parms;
    for (int i = 0; i < parm_count; ++i) {
      p = getParm(p);
      bool c_struct_type;
      Delete(crustTypeForRustValue(p, Getattr(p, "type"), &c_struct_type));
      if (c_struct_type) {
        Printv(f_rust_wrappers, "\tif Swig_escape_always_false {\n", NULL);
        Printv(f_rust_wrappers,
               "\t\tSwig_escape_val = ", Getattr(p, "emit:rustinput"), "\n",
               NULL);
        Printv(f_rust_wrappers, "\t}\n", NULL);
      }
      p = nextParm(p);
    }
  }

  /* -----------------------------------------------------------------------
   * freearg()
   *
   * Handle argument cleanup code if any.  This is used for the C/C++
   * function.  This assumes that each parameter has an "emit:input"
   * property with the name to use to refer to that parameter.
   * ----------------------------------------------------------------------- */

  String *freearg(ParmList *parms) {
    String *ret = NewString("");
    Parm *p = parms;
    while (p) {
      String *tm = Getattr(p, "tmap:freearg");
      if (!tm) {
        p = nextSibling(p);
      } else {
        tm = Copy(tm);
        Replaceall(tm, "$input", Getattr(p, "emit:input"));
        Printv(ret, tm, "\n", NULL);
        Delete(tm);
        p = Getattr(p, "tmap:freearg:next");
      }
    }
    return ret;
  }

  /* -----------------------------------------------------------------------
   * cleanupFunction()
   *
   * Final function cleanup code.
   * ----------------------------------------------------------------------- */

  void cleanupFunction(Node *n, Wrapper *f, ParmList *parms) {
    SwigType *returntype = Getattr(n, "type");
    String *cleanup = freearg(parms);
    Printv(f->code, cleanup, NULL);

    if (GetFlag(n, "feature:new")) {
      String *tm = Swig_typemap_lookup("newfree", n, Swig_cresult_name(), 0);
      if (tm) {
        Printv(f->code, tm, "\n", NULL);
        Delete(tm);
      }
    }

    Replaceall(f->code, "$cleanup", cleanup);
    Delete(cleanup);

    /* See if there is any return cleanup code */
    String *tm;
    if ((tm = Swig_typemap_lookup("ret", n, Swig_cresult_name(), 0))) {
      Printf(f->code, "%s\n", tm);
      Delete(tm);
    }

    bool isvoid = !Cmp(returntype, "void");
    Replaceall(f->code, "$isvoid", isvoid ? "1" : "0");

    Replaceall(f->code, "$symname", Getattr(n, "sym:name"));
  }

  /* -----------------------------------------------------------------------
   * variableHandler()
   *
   * This exists just to set the making_variable_wrappers flag.
   * ----------------------------------------------------------------------- */

  virtual int variableHandler(Node *n) {
    assert(!making_variable_wrappers);
    making_variable_wrappers = true;
    int r = Language::variableHandler(n);
    making_variable_wrappers = false;
    return r;
  }

  /* -----------------------------------------------------------------------
   * constantWrapper()
   *
   * Product a const declaration.
   * ------------------------------------------------------------------------ */

  virtual int constantWrapper(Node *n) {
    SwigType *type = Getattr(n, "type");

    if (Swig_storage_isstatic(n)) {
      return rustComplexConstant(n, type);
    }

    String *value = Getattr(n, "value");
    String *copy = NULL;
    int typecode = SwigType_type(type);
    if (typecode == T_STRING) {
      String *stringval = Getattr(n, "stringval");
      if (!stringval) {
        return rustComplexConstant(n, type);
      }
      // Backslash sequences are somewhat different in Rust and C/C++.
      copy = NewStringf("\"%(rustescape)s\"", stringval);
      value = copy;
    } else if (typecode == T_CHAR) {
      String *stringval = Getattr(n, "stringval");
      if (!stringval || Len(stringval) != 1) {
        return rustComplexConstant(n, type);
      }
      // Backslash sequences are somewhat different in Rust and C/C++.
      copy = NewStringf("'%(rustescape)s'", stringval);
      value = copy;
    } else if (!SwigType_issimple(type)) {
      return rustComplexConstant(n, type);
    } else if (Swig_storage_isstatic(n)) {
      return rustComplexConstant(n, type);
    } else if (Getattr(n, "numval")) {
      value = Getattr(n, "numval");
      if (typecode == T_BOOL) {
        copy = NewString(*Char(value) == '0' ? "false" : "true");
        value = copy;
      }
    } else {
      // Currently numval only gets set for integer and boolean literals, so
      // check for a floating point literal we can just use in Rust here.
      //
      // Accept digits, decimal point, and exponentiation.  Treat anything else
      // as too complicated to handle as a Rust constant.
      char *p = Char(value);
      for (int i = 0; p[i]; ++i) {
        switch (p[i]) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '.':
        case 'e':
        case 'E':
        case '+':
        case '-':
          break;
        default:
          return rustComplexConstant(n, type);
        }
      }
    }

    String *rust_name = buildRustName(Getattr(n, "sym:name"), false, false);

    if (!checkNameConflict(rust_name, n, NULL)) {
      Delete(rust_name);
      Delete(copy);
      return SWIG_NOWRAP;
    }

    String *tm = rustType(n, type);

    Printv(f_rust_wrappers, "const ", rust_name, " : ", tm, " = ", value, ";\n",
           NIL);

    Delete(tm);
    Delete(rust_name);
    Delete(copy);

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * enumDeclaration()
   *
   * A C++ enum type turns into a Named rust int type.
   * ---------------------------------------------------------------------- */

  virtual int enumDeclaration(Node *n) {
    if (getCurrentClass() && (cplus_mode != PUBLIC))
      return SWIG_NOWRAP;

    String *name = rustEnumName(n);
    if (Strcmp(name, "int") != 0) {
      if (!ImportMode || !imported_package) {
        if (!checkNameConflict(name, n, NULL)) {
          Delete(name);
          return SWIG_NOWRAP;
        }
        int ret = generateRustEnum(n,f_rust_wrappers);
        if (ret == SWIG_OK) {
          return SWIG_OK;
        }
      } else {
        String *nw = NewString("");
        Printv(nw, getModuleName(imported_package), ".", name, NULL);
        Setattr(n, "rust:enumname", nw);
      }
    }
    Delete(name);

    return Language::enumDeclaration(n);
  }

  int generateRustEnum(Node *n, File *f) {
    if (!n || !f) return SWIG_NOWRAP;
    
    // Get enum name and base type
    String *enumname = Getattr(n, "enumname");
    String *enumbase = Getattr(n, "enumbase");
    String *enumtype = Getattr(n, "enumtype");
    
    if (!enumname) {
        enumname = Getattr(n, "name");
    }
    
    if (!enumname) {
        Printf(stderr, "Warning: Swig for Rust doesn't support unnamed enum, skip\n");
        return SWIG_NOWRAP;
    }

    if (!enumbase)
    {
      enumbase = NewString("int");
    }
    
    
    // Start generating the enum
    Printv(f, "#[repr(",NULL);
    String *rust_base_type = rustType(n, enumbase);

    
    Printv(f,rust_base_type, ")]\n",NULL);
    Printf(f, "#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]\n");
    Printf(f, "pub enum %s {\n", enumname);
    
    // Iterate through enum items
    Node *child = firstChild(n);
    bool first_item = true;
    
    while (child) {
        String *nodetype = nodeType(child);
        
        if (Strcmp(nodetype, "enumitem") == 0) {
            String *itemname = Getattr(child, "name");
            String *enumvalue = Getattr(child, "enumvalue");
            String *enumvalueex = Getattr(child, "enumvalueex");
            
            if (itemname) {
                Printf(f, "\t%s", itemname);
                // Handle enum values
                if (enumvalue) {
                    Printf(f, " = %s", enumvalue);
                } 
                // else if (enumvalueex) {
                //     // Handle expressions like "RED + 1"
                //     // Convert to Rust-compatible syntax if needed
                //     String *rust_expr = Copy(enumvalueex);
                    
                //     // Simple transformations for common patterns
                //     Replaceall(rust_expr, " + ", " + ");
                //     Replaceall(rust_expr, " - ", " - ");
                    
                //     Printf(f, " = Self::%s", rust_expr);
                //     Delete(rust_expr);
                // }
                
                Printf(f, ",\n");
            }
        }
        
        child = nextSibling(child);
    }
    
    Printf(f, "}\n\n");
    
    // Generate implementation block with useful methods
    Printf(f, "impl TryFrom<%s> for %s {\n",rust_base_type, enumname);
    Printf(f, "\ttype Error = &'static str;\n");
    Printf(f, "\t/// Convert from raw integer value\n");
    Printv(f, "\tfn try_from(value: ",rust_base_type, ") -> Result<Self,Self::Error> {\n",NULL);

    child = firstChild(n);
    while (child) {
        String *nodetype = nodeType(child);
        
        if (Strcmp(nodetype, "enumitem") == 0) {
            String *itemname = Getattr(child, "name");
            String *enumvalue = Getattr(child, "enumvalue");
            String *enumvalueex = Getattr(child, "enumvalueex");
            
            if (itemname) {
                Printf(f, "\t\tconst %s_%s_VAL : %s = %s::%s as %s;\n",enumname, itemname,rust_base_type,enumname,itemname,rust_base_type);
            }
        }
        
        child = nextSibling(child);
    }

    Printf(f, "\t\tmatch value {\n");
    
    // Generate match arms for each enum value
    child = firstChild(n);
    while (child) {
        String *nodetype = nodeType(child);
        
        if (Strcmp(nodetype, "enumitem") == 0) {
            String *itemname = Getattr(child, "name");
            String *enumvalue = Getattr(child, "enumvalue");
            String *enumvalueex = Getattr(child, "enumvalueex");
            
            if (itemname) {
                Printf(f, "\t\t\t%s_%s_VAL => Ok(Self::%s),\n",enumname, itemname,itemname);
            }
        }
        
        child = nextSibling(child);
    }
    
    Printf(f, "\t\t\t_ => Err(\"The input arg has no corresponding enum value.\"),\n");
    Printf(f, "\t\t}\n");
    Printf(f, "\t}\n");
    Printf(f, "}\n\n");
    return SWIG_OK;
}


  /* -----------------------------------------------------------------------
   * enumvalueDeclaration()
   *
   * Declare a single value of an enum type.  We fetch the value by
   * calling a C/C++ function.
   * ------------------------------------------------------------------------ */

  virtual int enumvalueDeclaration(Node *n) {
    if (!is_public(n)) {
      return SWIG_OK;
    }

    Swig_require("enumvalueDeclaration", n, "*sym:name", NIL);
    Node *parent = parentNode(n);

    if (Getattr(parent, "unnamed")) {
      Setattr(n, "type", NewString("int"));
    } else {
      Setattr(n, "type", Getattr(parent, "enumtype"));
    }

    if (GetFlag(parent, "scopedenum")) {
      String *symname = Getattr(n, "sym:name");
      symname = Swig_name_member(0, Getattr(parent, "sym:name"), symname);
      Setattr(n, "sym:name", symname);
      Delete(symname);
    }

    int ret = rustComplexConstant(n, Getattr(n, "type"));
    Swig_restore(n);
    return ret;
  }

  /* -----------------------------------------------------------------------
   * rustComplexConstant()
   *
   * Handle a const declaration for something which is not a Rust constant.
   * ------------------------------------------------------------------------ */

  int rustComplexConstant(Node *n, SwigType *type) {
    String *symname = Getattr(n, "sym:name");
    if (!symname) {
      symname = Getattr(n, "name");
    }

    String *varname = buildRustName(symname, true, false);

    if (!checkNameConflict(varname, n, NULL)) {
      Delete(varname);
      return SWIG_NOWRAP;
    }

    if (!Getattr(n, "stringval") &&
        !Getattr(n, "enumvalueDeclaration:sym:name")) {
      // Based on Swig_VargetToFunction
      String *nname = NewStringf("(%s)", Getattr(n, "value"));
      String *call;
      if (SwigType_isclass(type)) {
        call = NewStringf("%s", nname);
      } else {
        call = SwigType_lcaststr(type, nname);
      }
      String *cres = Swig_cresult(type, Swig_cresult_name(), call);
      Setattr(n, "wrap:action", cres);
      Delete(nname);
      Delete(call);
      Delete(cres);
    } else {
      String *get = NewString("");
      Printv(get, Swig_cresult_name(), " = ", NULL);

      if (SwigType_type(type) == T_STRING) {
        Printv(get, "(char *)", NULL);
      }

      Printv(get, Getattr(n, "value"), NULL);

      Printv(get, ";\n", NULL);

      Setattr(n, "wrap:action", get);
      Delete(get);
    }

    String *sname = Copy(symname);
    if (class_name) {
      Append(sname, "_");
      Append(sname, class_name);
    }

    String *rust_name = NewString("_swig_get");
    if (class_name) {
      Append(rust_name, class_name);
      Append(rust_name, "_");
    }
    Append(rust_name, sname);

    String *wname = Swig_name_wrapper(sname);
    Append(wname, unique_id);
    Setattr(n, "wrap:name", wname);

    int r = makeWrappers(n, rust_name, NULL, wname, NULL, NULL, type, true);

    if (r != SWIG_OK) {
      return r;
    }

    String *t = rustType(n, type);
    Printv(f_rust_wrappers, "static ", varname, " : ", t, " = ", rust_name, "()\n",
           NULL);

    Delete(varname);
    Delete(t);
    Delete(rust_name);
    Delete(sname);

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * classHandler()
   *
   * For a C++ class, in Rust we generate both a struct and an
   * interface.  The interface will declare all the class public
   * methods.  We will define all the methods on the struct, so that
   * the struct meets the interface.  We then expect users of the
   * class to use the interface.
   * ------------------------------------------------------------ */

  virtual int classHandler(Node *n) {
    class_node = n;

    List *baselist = Getattr(n, "bases");
    bool has_base_classes = baselist && Len(baselist) > 0;

    String *name = Getattr(n, "sym:name");

    String *rust_name = exportedName(name);

    if (!checkNameConflict(rust_name, n, NULL)) {
      Delete(rust_name);
      SetFlag(n, "rust:conflict");
      return SWIG_NOWRAP;
    }

    String *rust_type_name = rustCPointerType(Getattr(n, "classtypeobj"), true);

    class_name = name;
    class_receiver = rust_type_name;
    class_methods = NewHash();

    int isdir = GetFlag(n, "feature:director");
    int isnodir = GetFlag(n, "feature:nodirector");
    bool is_director = isdir && !isnodir;

    // Printv(f_rust_wrappers, "type ", rust_type_name, " = usize;\n\n", NULL);
    Printv(f_rust_pre_extern, "#[repr(C)]\n#[derive(Debug)]\npub struct ",rust_name, "([u8; 0]);\n", NULL);
    Printv(f_rust_pre_extern, "impl BaseCPtrTrait for ",rust_name, " {} \n", NULL);
    // A method to return the pointer to the C++ class.  This is used
    // by generated code to convert between the interface and the C++
    // value.
    Printv(f_rust_wrappers, "impl ", rust_name, " {\n",
           NULL);
    // Printv(f_rust_wrappers, "fn Swigcptr(&self) -> usize {\n", NULL);
    // Printv(f_rust_wrappers, "\treturn self as usize;\n", NULL);
    // Printv(f_rust_wrappers, "}\n\n", NULL);

    // // A method used as a marker for the class, to avoid invalid
    // // interface conversions when using multiple inheritance.
    // Printv(f_rust_wrappers, "fn SwigIs", rust_name, "(&self) {\n", NULL);
    // Printv(f_rust_wrappers, "}\n\n", NULL);

    // if (is_director) {
    //   // Return the interface passed to the NewDirector function.
    //   Printv(f_rust_wrappers, "fn DirectorTrait(&self) interface{} {\n", NULL);
    //   Printv(f_rust_wrappers, "\treturn nil;\n", NULL);
    //   Printv(f_rust_wrappers, "}\n\n", NULL);
    // }

    // We have seen a definition for this type.
    Setattr(defined_types, rust_name, rust_name);
    Setattr(defined_types, rust_type_name, rust_type_name);

    interfaces = NewString("");

    int r = Language::classHandler(n);
    if (r != SWIG_OK) {
      return r;
    }

    if (has_base_classes) {
      // For each method defined in a base class but not defined in
      // this class, we need to define the method in this class.  We
      // can't use anonymous field inheritance because it works
      // differently in Rust and in C++.

      Hash *local = NewHash();
      for (Node *ni = Getattr(n, "firstChild"); ni; ni = nextSibling(ni)) {

        if (!is_public(ni)) {
          continue;
        }

        String *type = Getattr(ni, "nodeType");
        if (Cmp(type, "constructor") == 0 || Cmp(type, "destructor") == 0) {
          continue;
        }

        String *cname = Getattr(ni, "sym:name");
        if (!cname) {
          cname = Getattr(ni, "name");
        }
        if (cname) {
          Setattr(local, cname, NewString(""));
        }
      }

      for (Iterator b = First(baselist); b.item; b = Next(b)) {
        List *bases = NewList();
        Append(bases, Getattr(b.item, "classtype"));
        int r = addBase(n, b.item, bases, local);
        if (r != SWIG_OK) {
          return r;
        }
        Delete(bases);
      }

      Delete(local);

      Hash *parents = NewHash();
      addFirstBaseInterface(n, parents, baselist);
      int r = addExtraBaseInterfaces(n, parents, baselist);
      Delete(parents);
      if (r != SWIG_OK) {
        return r;
      }
    }
    Printv(f_rust_wrappers, "}\n\n", NULL);

    // Printv(f_rust_wrappers, "trait ", rust_name, " {\n", NULL);
    // Printv(f_rust_wrappers, "\tfn Swigcptr() -> uintptr;\n", NULL);
    // Printv(f_rust_wrappers, "\tfn SwigIs", rust_name, "();\n", NULL);
    // if (is_director) {
    //   Printv(f_rust_wrappers, "\tfn DirectorTrait() -> uintptr;\n", NULL);
    // }
    // Append(f_rust_wrappers, interfaces);
    // Printv(f_rust_wrappers, "}\n\n", NULL);

    // Printv(f_rust_wrappers, "type ", rust_name, " interface {\n", NULL);
    // Printv(f_rust_wrappers, "\tSwigcptr() uintptr\n", NULL);
    // Printv(f_rust_wrappers, "\tSwigIs", rust_name, "()\n", NULL);

    // if (is_director) {
    //   Printv(f_rust_wrappers, "\tDirectorTrait() interface{}\n", NULL);
    // }

    // Append(f_rust_wrappers, interfaces);
    // Printv(f_rust_wrappers, "}\n\n", NULL);
    Delete(interfaces);

    interfaces = NULL;
    class_name = NULL;
    class_receiver = NULL;
    class_node = NULL;
    Delete(class_methods);
    class_methods = NULL;

    Delete(rust_type_name);

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * addBase()
   *
   * Implement methods and members defined in a parent class for a
   * child class.
   * ------------------------------------------------------------ */

  int addBase(Node *n, Node *base, List *bases, Hash *local) {
    if (GetFlag(base, "feature:ignore")) {
      return SWIG_OK;
    }

    for (Node *ni = Getattr(base, "firstChild"); ni; ni = nextSibling(ni)) {
      int r = rustBaseEntry(n, bases, local, ni);
      if (r != SWIG_OK) {
        return r;
      }
    }

    List *baselist = Getattr(base, "bases");
    if (baselist && Len(baselist) > 0) {
      for (Iterator b = First(baselist); b.item; b = Next(b)) {
        List *nb = Copy(bases);
        Append(nb, Getattr(b.item, "classtype"));
        int r = addBase(n, b.item, nb, local);
        Delete(nb);
        if (r != SWIG_OK) {
          return r;
        }
      }
    }

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * rustBaseEntry()
   *
   * Implement one entry defined in a parent class for a child class.
   * n is the child class.
   * ------------------------------------------------------------ */

  int rustBaseEntry(Node *n, List *bases, Hash *local, Node *entry) {
    if (GetFlag(entry, "feature:ignore")) {
      return SWIG_OK;
    }

    if (!is_public(entry)) {
      return SWIG_OK;
    }

    String *type = Getattr(entry, "nodeType");
    if (Strcmp(type, "constructor") == 0 || Strcmp(type, "destructor") == 0 ||
        Strcmp(type, "enum") == 0 || Strcmp(type, "using") == 0 ||
        Strcmp(type, "classforward") == 0 || Strcmp(type, "template") == 0) {
      return SWIG_OK;
    }

    if (Strcmp(type, "extend") == 0) {
      for (Node *extend = firstChild(entry); extend;
           extend = nextSibling(extend)) {
        if (isStatic(extend)) {
          // If we don't do this, the extend_default test case fails.
          continue;
        }

        int r = rustBaseEntry(n, bases, local, extend);
        if (r != SWIG_OK) {
          return r;
        }
      }
      return SWIG_OK;
    }

    String *storage = Getattr(entry, "storage");
    if (storage &&
        (Strcmp(storage, "typedef") == 0 || Strstr(storage, "friend"))) {
      return SWIG_OK;
    }

    String *mname = Getattr(entry, "sym:name");
    if (!mname) {
      return SWIG_OK;
    }

    String *lname = Getattr(entry, "name");
    if (Getattr(class_methods, lname)) {
      return SWIG_OK;
    }
    if (Getattr(local, lname)) {
      return SWIG_OK;
    }
    Setattr(local, lname, NewString(""));

    String *ty = NewString(Getattr(entry, "type"));
    SwigType_push(ty, Getattr(entry, "decl"));
    String *fullty = SwigType_typedef_resolve_all(ty);
    bool is_function = SwigType_isfunction(fullty) ? true : false;
    Delete(ty);
    Delete(fullty);

    if (is_function) {
      int r = rustBaseMethod(n, bases, entry);
      if (r != SWIG_OK) {
        return r;
      }

      if (Getattr(entry, "sym:overloaded")) {
        for (Node *on = Getattr(entry, "sym:nextSibling"); on;
             on = Getattr(on, "sym:nextSibling")) {
          r = rustBaseMethod(n, bases, on);
          if (r != SWIG_OK) {
            return r;
          }
        }

        String *receiver = class_receiver;
        bool is_static = isStatic(entry);
        if (is_static) {
          receiver = NULL;
        }
        String *rust_name =
            buildRustName(Getattr(entry, "sym:name"), is_static, false);
        // todo
        // r = makeDispatchFunction(entry, rust_name, receiver, is_static, NULL,
        //                          false);
        Delete(rust_name);
        if (r != SWIG_OK) {
          return r;
        }
      }
    } else {
      int r = rustBaseVariable(n, bases, entry);
      if (r != SWIG_OK) {
        return r;
      }
    }

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * rustBaseMethod()
   *
   * Implement a method defined in a parent class for a child class.
   * ------------------------------------------------------------ */

  int rustBaseMethod(Node *method_class, List *bases, Node *method) {
    String *symname = Getattr(method, "sym:name");
    if (!validIdentifier(symname)) {
      return SWIG_OK;
    }

    String *name = NewString("");
    Printv(name, Getattr(method_class, "sym:name"), "_", symname, NULL);

    bool is_static = isStatic(method);

    String *rust_name = buildRustName(name, is_static, false);

    String *overname = NULL;
    if (Getattr(method, "sym:overloaded")) {
      overname = Getattr(method, "sym:overname");
    }
    String *wname = Swig_name_wrapper(name);
    if (overname) {
      Append(wname, overname);
    }
    Append(wname, unique_id);

    String *result = NewString(Getattr(method, "type"));
    SwigType_push(result, Getattr(method, "decl"));
    // shouldn't remove const qualifier
    // if (SwigType_isqualifier(result)) {
    //   Delete(SwigType_pop(result));
    // }
    Delete(SwigType_pop_function(result));

    // If the base method is imported, wrap:action may not be set.
    Swig_save("rustBaseMethod", method, "wrap:name", "wrap:action", "parms",
              NULL);
    Setattr(method, "wrap:name", wname);
    if (!Getattr(method, "wrap:action")) {
      if (!is_static) {
        Swig_MethodToFunction(method, getNSpace(), getClassType(),
                              (Getattr(method, "template")
                                   ? SmartPointer
                                   : Extend | SmartPointer),
                              NULL, false);
        // Remove any self parameter that was just added.
        ParmList *parms = Getattr(method, "parms");
        if (parms && Getattr(parms, "self")) {
          parms = CopyParmList(nextSibling(parms));
          Setattr(method, "parms", parms);
        }
      } else {
        String *call = Swig_cfunction_call(Getattr(method, "name"),
                                           Getattr(method, "parms"));
        Setattr(
            method, "wrap:action",
            Swig_cresult(Getattr(method, "type"), Swig_cresult_name(), call));
      }
    }

    // A method added by %extend in a base class may have void parms.
    ParmList *parms = Getattr(method, "parms");
    if (parms != NULL && SwigType_type(Getattr(parms, "type")) == T_VOID) {
      parms = NULL;
    }

    int r = makeWrappers(method, rust_name, overname, wname, bases, parms,
                         result, is_static);

    Swig_restore(method);

    Delete(result);
    Delete(rust_name);
    Delete(name);

    return r;
  }

  /* ------------------------------------------------------------
   * rustBaseVariable()
   *
   * Add accessors for a member variable defined in a parent class for
   * a child class.
   * ------------------------------------------------------------ */

  int rustBaseVariable(Node *var_class, List *bases, Node *var) {
    if (isStatic(var)) {
      return SWIG_OK;
    }

    String *var_name = buildRustName(Getattr(var, "sym:name"), false, false);

    Swig_save("rustBaseVariable", var, "type", "wrap:action", NULL);

    // For a pointer type we apparently have to wrap in the decl.
    SwigType *var_type = NewString(Getattr(var, "type"));
    SwigType_push(var_type, Getattr(var, "decl"));
    Setattr(var, "type", var_type);

    SwigType *vt = Copy(var_type);

    int flags = Extend | SmartPointer | use_naturalvar_mode(var);
    if (isNonVirtualProtectedAccess(var)) {
      flags |= CWRAP_ALL_PROTECTED_ACCESS;
    }

    // Copied from Swig_wrapped_member_var_type.
    if (SwigType_isclass(vt)) {
      if (flags & CWRAP_NATURAL_VAR) {
        if (CPlusPlus) {
          if (!SwigType_isconst(vt)) {
            SwigType_add_qualifier(vt, "const");
          }
          SwigType_add_reference(vt);
        }
      } else {
        SwigType_add_pointer(vt);
      }
    }

    String *mname =
        Swig_name_member(getNSpace(), Getattr(var_class, "sym:name"), var_name);

    if (!is_immutable(var)) {
      for (Iterator ki = First(var); ki.key; ki = Next(ki)) {
        if (Strncmp(ki.key, "tmap:", 5) == 0) {
          Delattr(var, ki.key);
        }
      }
      Swig_save("rustBaseVariableSet", var, "name", "sym:name", "type", NULL);

      String *mname_set = NewString("Set");
      Append(mname_set, mname);

      String *rust_name = NewString("Set");
      Append(rust_name, var_name);

      Swig_MembersetToFunction(var, class_name, flags);

      String *wname = Swig_name_wrapper(mname_set);
      Append(wname, unique_id);
      ParmList *parms = NewParm(vt, var_name, var);
      String *result = NewString("void");
      int r = makeWrappers(var, rust_name, NULL, wname, bases, parms, result,
                           false);
      if (r != SWIG_OK) {
        return r;
      }
      Delete(wname);
      Delete(parms);
      Delete(result);
      Delete(rust_name);
      Delete(mname_set);

      Swig_restore(var);
      for (Iterator ki = First(var); ki.key; ki = Next(ki)) {
        if (Strncmp(ki.key, "tmap:", 5) == 0) {
          Delattr(var, ki.key);
        }
      }
    }

    Swig_MembergetToFunction(var, class_name, flags);

    String *mname_get = NewString("Get");
    Append(mname_get, mname);

    String *rust_name = NewString("Get");
    Append(rust_name, var_name);

    String *wname = Swig_name_wrapper(mname_get);
    Append(wname, unique_id);

    int r = makeWrappers(var, rust_name, NULL, wname, bases, NULL, vt, false);
    if (r != SWIG_OK) {
      return r;
    }

    Delete(wname);
    Delete(mname_get);
    Delete(rust_name);
    Delete(mname);
    Delete(var_name);
    Delete(var_type);
    Delete(vt);

    Swig_restore(var);

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * addFirstBaseInterface()
   *
   * When a C++ class uses multiple inheritance, we can use the C++
   * pointer for the first base class but not for any subsequent base
   * classes.  However, the Rust interface will match the interface for
   * all the base classes.  To avoid accidentally treating a class as
   * a pointer to a base class other than the first one, we use an
   * isClassname method.  This function adds those methods as
   * required.
   *
   * For convenience when using multiple inheritance, we also add
   * functions to retrieve the base class pointers.
   * ------------------------------------------------------------ */

  void addFirstBaseInterface(Node *n, Hash *parents, List *bases) {
    if (!bases || Len(bases) == 0) {
      return;
    }
    Iterator b = First(bases);
    if (!GetFlag(b.item, "feature:ignore")) {
      String *rust_name = buildRustName(Getattr(n, "sym:name"), false, false);
      String *rust_type_name =
          rustCPointerType(Getattr(n, "classtypeobj"), true);
      String *rust_base_name = exportedName(Getattr(b.item, "sym:name"));
      String *rust_base_type = rustType(n, Getattr(b.item, "classtypeobj"));
      String *rust_base_type_name =
          rustCPointerType(Getattr(b.item, "classtypeobj"), true);

      // Printv(f_rust_wrappers, "func (p ", rust_type_name, ") SwigIs",
      //        rust_base_name, "() {\n", NULL);
      // Printv(f_rust_wrappers, "}\n\n", NULL);

      // Printv(interfaces, "\tfn SwigIs", rust_base_name, "();\n", NULL);

      Printv(f_rust_wrappers, "pub fn SwigGet",
             rust_base_name, "(&self) -> *mut ", rust_base_type, " {\n", NULL);
      Printv(f_rust_wrappers, "\treturn (get_swig_cptr(self)) as *mut ",rust_base_type,";\n", NULL);
      Printv(f_rust_wrappers, "}\n\n", NULL);

      // Printv(interfaces, "\tfn SwigGet", rust_base_name, "() -> ",
      //        rust_base_type, ";\n", NULL);

      Setattr(parents, rust_base_name, NewString(""));

      Delete(rust_name);
      Delete(rust_type_name);
      Delete(rust_base_type);
      Delete(rust_base_type_name);
    }

    addFirstBaseInterface(n, parents, Getattr(b.item, "bases"));
  }

  /* ------------------------------------------------------------
   * addExtraBaseInterfaces()
   *
   * Add functions to retrieve the base class pointers for all base
   * classes other than the first.
   * ------------------------------------------------------------ */

  int addExtraBaseInterfaces(Node *n, Hash *parents, List *bases) {
    Iterator b = First(bases);

    Node *fb = b.item;

    for (b = Next(b); b.item; b = Next(b)) {
      if (GetFlag(b.item, "feature:ignore")) {
        continue;
      }

      String *rust_base_name = exportedName(Getattr(b.item, "sym:name"));

      Swig_save("addExtraBaseInterface", n, "wrap:action", "wrap:name",
                "wrap:parms", NULL);

      SwigType *type = Copy(Getattr(n, "classtypeobj"));
      SwigType_add_pointer(type);
      Parm *parm = NewParm(type, "self", n);
      Setattr(n, "wrap:parms", parm);

      String *pn = Swig_cparm_name(parm, 0);
      String *action = NewString("");
      Printv(action, Swig_cresult_name(), " = (", Getattr(b.item, "classtype"),
             "*)", pn, ";", NULL);
      Delete(pn);

      Setattr(n, "wrap:action", action);

      String *name = Copy(class_name);
      Append(name, "_SwigGet");
      Append(name, rust_base_name);

      String *rust_name = NewString("SwigGet");
      String *c1 = exportedName(rust_base_name);
      Append(rust_name, c1);
      Delete(c1);

      String *wname = Swig_name_wrapper(name);
      Append(wname, unique_id);
      Setattr(n, "wrap:name", wname);

      SwigType *result = Copy(Getattr(b.item, "classtypeobj"));
      SwigType_add_pointer(result);

      int r =
          makeWrappers(n, rust_name, NULL, wname, NULL, parm, result, false);
      if (r != SWIG_OK) {
        return r;
      }

      Swig_restore(n);

      Setattr(parents, rust_base_name, NewString(""));

      Delete(rust_name);
      Delete(type);
      Delete(parm);
      Delete(action);
      Delete(result);

      String *ns = NewString("");
      addParentExtraBaseInterfaces(n, parents, b.item, false, ns);
      Delete(ns);
    }

    if (!GetFlag(fb, "feature:ignore")) {
      String *ns = NewString("");
      addParentExtraBaseInterfaces(n, parents, fb, true, ns);
      Delete(ns);
    }

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * addParentExtraBaseInterfaces()
   *
   * Add functions to retrieve the base class pointers for all base
   * classes of parents other than the first base class at each level.
   * ------------------------------------------------------------ */

  void addParentExtraBaseInterfaces(Node *n, Hash *parents, Node *base,
                                    bool is_base_first, String *sofar) {
    List *baselist = Getattr(base, "bases");
    if (!baselist || Len(baselist) == 0) {
      return;
    }

    String *rust_this_base_name = exportedName(Getattr(base, "sym:name"));

    String *sf = NewString("");
    Printv(sf, sofar, ".SwigGet", rust_this_base_name, "()", NULL);

    Iterator b = First(baselist);

    if (is_base_first) {
      if (!b.item) {
        return;
      }
      if (!GetFlag(b.item, "feature:ignore")) {
        addParentExtraBaseInterfaces(n, parents, b.item, true, sf);
      }

      b = Next(b);
    }

    String *rust_name = buildRustName(Getattr(n, "sym:name"), false, false);
    String *rust_type_name = rustCPointerType(Getattr(n, "classtypeobj"), true);

    for (; b.item; b = Next(b)) {
      if (GetFlag(b.item, "feature:ignore")) {
        continue;
      }

      String *rust_base_name = exportedName(Getattr(b.item, "sym:name"));

      if (!Getattr(parents, rust_base_name)) {
        Printv(f_rust_wrappers, "func (p ", rust_type_name, ") SwigGet",
               rust_base_name, "() ", rust_base_name, " {\n", NULL);
        Printv(f_rust_wrappers, "\treturn p", sf, ".SwigGet", rust_base_name,
               "()\n", NULL);
        Printv(f_rust_wrappers, "}\n\n", NULL);

        Printv(interfaces, "\tfn SwigGet", rust_base_name, "() ->",
               rust_base_name, ";\n", NULL);

        addParentExtraBaseInterfaces(n, parents, b.item, false, sf);

        Setattr(parents, rust_base_name, NewString(""));
      }
    }

    Delete(rust_name);
    Delete(rust_type_name);
    Delete(rust_this_base_name);
    Delete(sf);
  }

  /* ------------------------------------------------------------
   * classDirectorInit
   *
   * Add support for a director class.
   *
   * Virtual inheritance is different in Rust and C++.  We implement
   * director classes by defining a new function in Rust,
   * NewDirectorClassname, which takes a empty interface value and
   * creates an instance of a new child class.  The new child class
   * refers all methods back to Rust.  The Rust code checks whether the
   * value passed to NewDirectorClassname implements that method; if
   * it does, it calls it, otherwise it calls back into C++.
   * ------------------------------------------------------------ */
  int classDirectorInit(Node *n) {
    // Because we use a different function to handle inheritance in
    // Rust, ordinary creations of the object should not create a
    // director object.
    Delete(director_ctor_code);
    director_ctor_code = NewString("$nondirector_new");

    class_node = n;

    String *name = Getattr(n, "sym:name");

    assert(!class_name);
    class_name = name;

    String *rust_name = exportedName(name);

    String *rust_type_name = rustCPointerType(Getattr(n, "classtypeobj"), true);

    assert(!class_receiver);
    class_receiver = rust_type_name;

    String *cxx_director_name = NewString("SwigDirector_");
    Append(cxx_director_name, name);

    /*
    
    fn get_base_const(&self) -> *const Event {
        self.get_c_ptr() as *const Event
    }
    fn get_base_mut(&mut self) -> *mut Event {
        self.get_c_ptr() as *mut Event
    }
    fn change_base(&mut self, base: *mut c_void) {
        unimplemented!("change_base not implemented");
    }
    */
    // Generate base trait for the class
    String *bases = NULL;
    if (List *baselist = Getattr(n, "bases")) {
      bases = NewString("");
      for (Iterator base = First(baselist); base.item; ) {
	if (GetFlag(base.item, "feature:ignore")){
    base = Next(base);
	  continue; 
  }
  String *cn = exportedName(Getattr(base.item, "sym:name"));
  Printv(f_rust_wrappers, "impl ", cn, "TraitBase for ",rust_name," { }\n", NULL);
  String *base_iname_base_trait = NewStringf("%sTrait ",cn);
	  Append(bases, base_iname_base_trait);
      base = Next(base);
      if (base.item) {
        Append(bases, "+ ");
      }
      }
    }
    Printv(f_rust_wrappers, "// Base trait for ", rust_name, " providing access to C++ base object\n", NULL);
    Printv(f_rust_wrappers, "pub trait ", rust_name, "TraitBase : BaseCPtrTrait {\n", NULL);
    Printv(f_rust_wrappers, "\tfn get_",rust_name,"_base_const(&self) -> *const ", rust_name, " {\n", NULL);
    Printv(f_rust_wrappers, "\t\tself.get_c_ptr() as *const ", rust_name, "\n\t}\n", NULL);
    Printv(f_rust_wrappers, "\tfn get_",rust_name,"_base_mut(&mut self) -> *mut ", rust_name, " {\n", NULL);
    Printv(f_rust_wrappers, "\t\tself.get_c_ptr() as *mut ", rust_name, "\n\t}\n", NULL);
    Printv(f_rust_wrappers, "}\n\n", NULL);

    // Generate virtual method trait - DON'T CLOSE IT YET!
    Printv(f_rust_wrappers, "// Virtual method trait for ", rust_name, " - implement this to override virtual methods\n", NULL);
    if(bases != NULL) {
      Printv(f_rust_wrappers, "pub trait ", rust_name, "Trait: ", rust_name, "TraitBase + ", bases, " {\n", NULL);
    } else {
      Printv(f_rust_wrappers, "pub trait ", rust_name, "Trait: ", rust_name, "TraitBase {\n", NULL);
    }

    Printv(f_rust_traits_wrappers,"#[derive(Clone)]\npub struct ", rust_name,"TraitWrapper {\n", NULL);
    Printv(f_rust_traits_wrappers,"\tpub obj: Arc<RwLock<dyn ",rust_name, "Trait + Send + Sync>> \n}\n\n", NULL);
    Printv(f_rust_traits_wrappers,"unsafe impl Send for ",rust_name, 
            "TraitWrapper {}\nunsafe impl Sync for ",rust_name,"TraitWrapper {}\n\n", NULL);

    // Start defining the C++ director class
    Printv(f_c_directors_h, "class ", cxx_director_name, " : public ",
           Getattr(n, "classtype"), "\n", NULL);
    Printv(f_c_directors_h, "{\n", NULL);
    Printv(f_c_directors_h, " public:\n", NULL);

    Delete(cxx_director_name);

    class_methods = NewHash();

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * classDirectorConstructor
   *
   * Emit a constructor for a director class.
   * ------------------------------------------------------------ */

  int classDirectorConstructor(Node *n) {
    bool is_ignored = GetFlag(n, "feature:ignore") ? true : false;

    String *name = Getattr(n, "sym:name");
    if (!name) {
      assert(is_ignored);
      name = Getattr(n, "name");
    }

    String *overname = NULL;
    if (Getattr(n, "sym:overloaded")) {
      overname = Getattr(n, "sym:overname");
    }

    String *rust_name = exportedName(name);

    ParmList *parms = Getattr(n, "parms");
    Setattr(n, "wrap:parms", parms);

    String *cn = exportedName(Getattr(parentNode(n), "sym:name"));

    String *rust_type_name =
        rustCPointerType(Getattr(parentNode(n), "classtypeobj"), true);

    String *fn_name = NewString("register_director_");
    Append(fn_name, cn);
    if (overname) {
      Append(fn_name, overname);
    }

    if (!overname && !is_ignored) {
      if (!checkNameConflict(fn_name, n, NULL)) {
        return SWIG_NOWRAP;
      }
    }

    String *wname = Swig_name_wrapper(fn_name);
    if (overname) {
      Append(wname, overname);
    }
    Append(wname, unique_id);
    Setattr(n, "wrap:name", wname);

    bool is_static = isStatic(n);

    Wrapper *dummy = NewWrapper();
    emit_attach_parmmaps(parms, dummy);
    DelWrapper(dummy);

    Swig_typemap_attach_parms("rusttype", parms, NULL);
    Swig_typemap_attach_parms("rustin", parms, NULL);
    Swig_typemap_attach_parms("rustargout", parms, NULL);
    Swig_typemap_attach_parms("imtype", parms, NULL);
    int parm_count = emit_num_arguments(parms);

    if (!is_ignored) {
      // Declare the C wrapper function
      Printv(f_rust_pre_extern, "extern \"C\" { fn ", wname, "(rust_id: *mut c_void", NULL);

      Parm *p = parms;
      for (int i = 0; i < parm_count; ++i) {
        p = getParm(p);
        String *ct = rustImType(p, Getattr(p, "type"));
        Printv(f_rust_pre_extern, ", ", Getattr(p, "lname"), ": ", ct, NULL);
        Delete(ct);
        p = nextParm(p);
      }
      Printv(f_rust_pre_extern, ") -> *mut c_void; }\n", NULL);

      // Write out the Rust constructor function
      Printv(f_rust_after_director_extern,"impl ",cn,"TraitWrapper {\n", NULL);
      Printv(f_rust_after_director_extern, "\tpub fn register_director(&mut self", NULL);

      p = parms;
      for (int i = 0; i < parm_count; ++i) {
        p = getParm(p);
        Printv(f_rust_after_director_extern, ", ", Getattr(p, "lname"), ": ", NULL);
        String *tm = rustType(p, Getattr(p, "type"));
        Printv(f_rust_after_director_extern, tm, NULL);
        Delete(tm);
        p = nextParm(p);
      }

      Printv(f_rust_after_director_extern, ") -> Result<(), DirectorError> {\n", NULL);

      // Add the Rust object to the global tracker
      Printv(f_rust_after_director_extern, "\t\tlet rust_id = self as *mut Self as *mut c_void;\n", NULL);
      Printv(f_rust_after_director_extern, "\t\tlet mut inner_obj = self.obj.write().unwrap();\n", NULL);

      String *call = NewString("");
      Printv(call, "\t\tlet cptr = unsafe { ", wname, "(rust_id", NULL);

      p = parms;
      for (int i = 0; i < parm_count; ++i) {
        Printv(call, ", ", NULL);
        p = getParm(p);
        String *pt = Getattr(p, "type");
        String *ln = Getattr(p, "lname");

        String *ivar = NewStringf("_swig_i_%d", i);

        String *rustin = rustGetattr(p, "tmap:rustin");
        if (rustin == NULL) {
          Printv(f_rust_after_director_extern, "\t\tlet ", ivar, " = ", NULL);
          bool need_close = false;
          if (rustTypeIsTrait(p, pt)) {
            Printv(f_rust_after_director_extern, "get_swig_cptr(", NULL);
            need_close = true;
          }
          Printv(f_rust_after_director_extern, ln, NULL);
          if (need_close) {
            Printv(f_rust_after_director_extern, ")", NULL);
          }
          Printv(f_rust_after_director_extern, ".clone().into();\n", NULL);
        } else {
          String *itm = rustImType(p, pt);
          Printv(f_rust_after_director_extern, "\t\tlet ", ivar, ": ", itm, ";\n", NULL);
          rustin = Copy(rustin);
          Replaceall(rustin, "$input", ln);
          Replaceall(rustin, "$result", ivar);
          Printv(f_rust_after_director_extern, "\t\t", rustin, ";\n", NULL);
          Delete(rustin);
        }

        Setattr(p, "emit:rustinput", ivar);

        bool c_struct_type;
        String *ct = rustTypeForCppValue(p, pt, &c_struct_type);
        if (c_struct_type) {
          Printv(call, "*(*", ct, ")(unsafe.Pointer(&", ivar, "))", NULL);
        } else {
          Printv(call, ivar, NULL);
        }
        Delete(ct);

        p = nextParm(p);
      }

      Printv(call, ") } ;\n", NULL);
      Printv(f_rust_after_director_extern, call, NULL);
      Delete(call);

      // Get the Rust object back from the tracker  


      Printv(f_rust_after_director_extern, "\t\tinner_obj.change_base(cptr);\n\t\tOk(())\n", NULL);
      rustargout(parms);
      Printv(f_rust_after_director_extern, "\t}\n\n", NULL);
      Printv(f_rust_after_director_extern, "}\n\n", NULL);

      // Generate the C++ constructor implementation
      SwigType *result = Copy(Getattr(parentNode(n), "classtypeobj"));
      SwigType_add_pointer(result);

      Swig_save("classDirectorConstructor", n, "wrap:name", "wrap:action", NULL);

      String *dwname = Swig_name_wrapper(name);
      Append(dwname, unique_id);
      Setattr(n, "wrap:name", dwname);

      String *action = NewString("");
      Printv(action, Swig_cresult_name(), " = new SwigDirector_", class_name, "(", NULL);
      
      // Pass the rust_id as the first parameter
      Printv(action, "arg1", NULL);
      
      p = parms;
      for (int i = 0; i < parm_count; ++i) {
        p = getParm(p);
        String *pname = Swig_cparm_name(NULL, i + 2); // +2 because rust_id is arg1
        Printv(action, ", ", NULL);
        if (SwigType_isreference(Getattr(p, "type"))) {
          Printv(action, "*", NULL);
        }
        Printv(action, pname, NULL);
        Delete(pname);
        p = nextParm(p);
      }
      Printv(action, ");", NULL);
      Setattr(n, "wrap:action", action);

      // Create parameter list including rust_id
      SwigType *first_type = NewString("void");
      Parm *first_parm = NewParm(first_type, "rust_id", n);
      set_nextSibling(first_parm, parms);

      cppRustWrapperInfo info;
      info.n = n;
      info.rust_name = fn_name;
      info.overname = overname;
      info.wname = wname;
      info.base = NULL;
      info.parms = first_parm;
      info.result = result;
      info.is_static = false;
      info.receiver = NULL;
      info.is_constructor = true;
      info.is_destructor = false;

      int r = cppWrapper(&info);
      if (r != SWIG_OK) {
        return r;
      }

      Swig_restore(n);

      Delete(result);
      Delete(first_type);
      Delete(first_parm);
    }

    // Generate C++ constructor declaration and implementation
    String *cxx_director_name = NewString("SwigDirector_");
    Append(cxx_director_name, class_name);

    String *decl = Swig_method_decl(NULL, Getattr(n, "decl"), cxx_director_name,
                                    NewParm(NewString("void *"), "rust_id", n), 0);
    Printv(f_c_directors_h, "  ", decl, ";\n", NULL);
    Delete(decl);

    decl = Swig_method_decl(NULL, Getattr(n, "decl"), cxx_director_name,
                            NewParm(NewString("void *"), "rust_id", n), 0);
    Printv(f_c_directors, cxx_director_name, "::", decl, "\n", NULL);
    Delete(decl);

    Printv(f_c_directors, "    : ", Getattr(parentNode(n), "classtype"), "(", NULL);

    Parm *p = parms;
    for (int i = 0; i < parm_count; ++i) {
      p = getParm(p);
      if (i > 0) {
        Printv(f_c_directors, ", ", NULL);
      }
      String *pn = Getattr(p, "name");
      assert(pn);
      Printv(f_c_directors, pn, NULL);
      p = nextParm(p);
    }
    Printv(f_c_directors, "),\n", NULL);
    Printv(f_c_directors, "      rust_val(rust_id), swig_mem(0)\n", NULL);
    Printv(f_c_directors, "{ }\n\n", NULL);

    if (Getattr(n, "sym:overloaded") && !Getattr(n, "sym:nextSibling")) {
      // todo
      // int r = makeDispatchFunction(n, fn_name, cn, is_static,
      //                              Getattr(parentNode(n), "classtypeobj"), false);
      // if (r != SWIG_OK) {
      //   return r;
      // }
    }

    Delete(cxx_director_name);
    Delete(rust_name);
    Delete(cn);
    Delete(rust_type_name);
    Delete(fn_name);
    Delete(wname);

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * classDirectorDestructor
   *
   * Emit a destructor for a director class.
   * ------------------------------------------------------------ */

  int classDirectorDestructor(Node *n) {
    if (!is_public(n)) {
      return SWIG_OK;
    }

    bool is_ignored = GetFlag(n, "feature:ignore") ? true : false;

    if (!is_ignored) {
      String *fnname = NewString("DeleteDirector");
      String *c1 = exportedName(class_name);
      Append(fnname, c1);
      Delete(c1);

      String *wname = Swig_name_wrapper(fnname);
      Append(wname, unique_id);

      Setattr(n, "wrap:name", fnname);

      Swig_DestructorToFunction(n, getNSpace(), getClassType(), CPlusPlus,
                                Extend);

      ParmList *parms = Getattr(n, "parms");
      Setattr(n, "wrap:parms", parms);

      String *result = NewString("void");
      int r = makeWrappers(n, fnname, NULL, wname, NULL, parms, result,
                           isStatic(n));
      if (r != SWIG_OK) {
        return r;
      }

      Delete(result);
      Delete(fnname);
      Delete(wname);
    }

    // Generate the destructor for the C++ director class.  Since the
    // Rust code is keeping a pointer to the C++ object, we need to call
    // back to the Rust code to let it know that the C++ object is rustne.

    String *rust_name = NewString("Swigrust_DeleteDirector_");
    Append(rust_name, class_name);

    String *cn = exportedName(class_name);

    String *director_struct_name = NewString("_swig_Director");
    Append(director_struct_name, cn);

    Printv(f_c_directors_h, "  virtual ~SwigDirector_", class_name, "()", NULL);

    String *throws = buildThrow(n);
    if (throws) {
      Printv(f_c_directors_h, " ", throws, NULL);
    }

    Printv(f_c_directors_h, ";\n", NULL);

    String *director_sig = NewString("");

    Printv(director_sig, "SwigDirector_", class_name, "::~SwigDirector_",
           class_name, "()", NULL);

    if (throws) {
      Printv(director_sig, " ", throws, NULL);
      Delete(throws);
    }

    Printv(director_sig, "\n", NULL);
    Printv(director_sig, "{\n", NULL);

    if (is_ignored) {
      Printv(f_c_directors, director_sig, NULL);
    } else {
      makeDirectorDestructorWrapper(rust_name, director_struct_name,
                                    director_sig);
    }

    Printv(f_c_directors, "  delete swig_mem;\n", NULL);

    Printv(f_c_directors, "}\n\n", NULL);

    Delete(director_sig);
    Delete(rust_name);
    Delete(cn);
    Delete(director_struct_name);

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * makeDirectorDestructorWrapper
   *
   * Emit the function wrapper for the destructor of a director class.
   * ------------------------------------------------------------ */

  void makeDirectorDestructorWrapper(String *rust_name,
                                     String *director_struct_name,
                                     String *director_sig) {
    String *wname = Copy(rust_name);
    Append(wname, unique_id);

    Printv(f_rust_wrappers, "#[no_mangle]\n", NULL);
    Printv(f_rust_wrappers, "pub extern \"C\" fn ", wname, "(c : SwigRefType) {\n", NULL);
    Printv(f_rust_wrappers, "\tswigDirectorLookup(c).(*", director_struct_name,
           ").", class_receiver, " = 0\n", NULL);
    Printv(f_rust_wrappers, "\tswigDirectorDelete(c)\n", NULL);
    Printv(f_rust_wrappers, "}\n\n", NULL);

    Printv(f_c_directors, "extern \"C\" void ", wname, "(intrust);\n", NULL);
    Printv(f_c_directors, director_sig, NULL);
    Printv(f_c_directors, "  ", wname, "(rust_val);\n", NULL);
  }

  /* ------------------------------------------------------------
   * classDirectorMethod
   *
   * Emit a method for a director class, plus its overloads.
   * ------------------------------------------------------------ */

  int classDirectorMethod(Node *n, Node *parent, String *super) {
    bool is_ignored = GetFlag(n, "feature:ignore") ? true : false;

    // We don't need explicit calls.
    if (GetFlag(n, "explicitcall")) {
      return SWIG_OK;
    }

    String *name = Getattr(n, "sym:name");
    if (!name) {
      assert(is_ignored);
      (void)is_ignored;
      name = Getattr(n, "name");
    }

    bool overloaded =
        Getattr(n, "sym:overloaded") && !Getattr(n, "explicitcallnode");
    if (!overloaded) {
      int r = oneClassDirectorMethod(n, parent, super);
      if (r != SWIG_OK) {
        return r;
      }
    } else {
      // Handle overloaded methods here, because otherwise we will
      // reject them in the class_methods hash table.  We need to use
      // class_methods so that we correctly handle cases where a
      // function in one class hides a function of the same name in a
      // parent class.
      if (!Getattr(class_methods, name)) {
        for (Node *on = Getattr(n, "sym:overloaded"); on;
             on = Getattr(on, "sym:nextSibling")) {
          // Swig_overload_rank expects wrap:name and wrap:parms to be
          // set.
          String *wn = Swig_name_wrapper(Getattr(on, "sym:name"));
          Append(wn, Getattr(on, "sym:overname"));
          Append(wn, unique_id);
          Setattr(on, "wrap:name", wn);
          Delete(wn);
          Setattr(on, "wrap:parms", Getattr(on, "parms"));
        }
      }

      int r = oneClassDirectorMethod(n, parent, super);
      if (r != SWIG_OK) {
        return r;
      }

      if (!Getattr(n, "sym:nextSibling")) {
        // Last overloaded function
        Node *on = Getattr(n, "sym:overloaded");
        bool is_static = isStatic(on);

        String *cn = exportedName(Getattr(parent, "sym:name"));
        String *rust_name = buildRustName(name, is_static, false);

        String *director_struct_name = NewString("_swig_Director");
        Append(director_struct_name, cn);
        // todo
        // int r = makeDispatchFunction(on, rust_name, director_struct_name,
        //                              is_static, director_struct_name, false);
        if (r != SWIG_OK) {
          return r;
        }

        if (!GetFlag(n, "abstract")) {
          String *rust_upcall = NewString("Director");
          Append(rust_upcall, cn);
          Append(rust_upcall, rust_name);
          // todo
          // r = makeDispatchFunction(on, rust_upcall, director_struct_name,
          //                          is_static, director_struct_name, true);
          if (r != SWIG_OK) {
            return r;
          }
          Delete(rust_upcall);
        }

        Delete(director_struct_name);
        Delete(rust_name);
        Delete(cn);
      }
    }
    Setattr(class_methods, name, NewString(""));

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * oneClassDirectorMethod
   *
   * Emit a method for a director class.
   * ------------------------------------------------------------ */

  int oneClassDirectorMethod(Node *n, Node *parent, String *super) {
    String *symname = Getattr(n, "sym:name");
    if (!checkFunctionVisibility(n, parent)) {
      return SWIG_OK;
    }

    bool is_ignored = GetFlag(n, "feature:ignore") ? true : false;
    bool is_pure_virtual = (Cmp(Getattr(n, "storage"), "virtual") == 0 &&
                            Cmp(Getattr(n, "value"), "0") == 0);

    String *name = Getattr(n, "sym:name");
    if (!name) {
      assert(is_ignored);
      name = Getattr(n, "name");
    }

    String *overname = NULL;
    if (Getattr(n, "sym:overloaded")) {
      overname = Getattr(n, "sym:overname");
    }

    String *cn = exportedName(Getattr(parent, "sym:name"));
    String *rust_type_name = NewString(Getattr(parent, "classtypeobj"));

    bool is_static = isStatic(n);
    bool is_const = isConst(n);
    String *rust_name = buildRustName(name, is_static, false);

    ParmList *parms = Getattr(n, "parms");
    Setattr(n, "wrap:parms", parms);

    Wrapper *dummy = NewWrapper();
    emit_attach_parmmaps(parms, dummy);

    Swig_typemap_attach_parms("rusttype", parms, NULL);
    Swig_typemap_attach_parms("imtype", parms, NULL);
    int parm_count = emit_num_arguments(parms);

    SwigType *returntype = Getattr(n, "type");
    bool is_void = (SwigType_type(returntype) == T_VOID);

    // Save the type for overload processing
    Setattr(n, "rust:type", returntype);

    String *director_struct_name = NewString("_swig_Director");
    Append(director_struct_name, cn);

    String *callback_name = Copy(director_struct_name);
    Append(callback_name, "_callback_");
    Append(callback_name, name);
    Replace(callback_name, "_swig", "Swig", DOH_REPLACE_FIRST);
    if (overname) {
      Append(callback_name, overname);
    }
    Append(callback_name, unique_id);

    String *upcall_name = Copy(director_struct_name);
    Append(upcall_name, "_upcall_");
    Append(upcall_name, rust_name);

    String *upcall_wname = Swig_name_wrapper(upcall_name);
    if (overname) {
      Append(upcall_wname, overname);
    }
    Append(upcall_wname, unique_id);

    String *rust_with_over_name = Copy(rust_name);
    if (overname) {
      Append(rust_with_over_name, overname);
    }

    Parm *p = 0;
    Wrapper *w = NewWrapper();

    Swig_director_parms_fixup(parms);

    Swig_typemap_attach_parms("directorin", parms, w);
    Swig_typemap_attach_parms("directorargout", parms, w);
    Swig_typemap_attach_parms("rustdirectorin", parms, w);
    Swig_typemap_attach_parms("rustin", parms, dummy);
    Swig_typemap_attach_parms("rustargout", parms, dummy);

    DelWrapper(dummy);

    if (!is_ignored) {
      // Add the method signature to the trait (inside the trait definition)
      Printv(f_rust_wrappers, "\t// Virtual method: ", rust_with_over_name, "\n", NULL);
      Printv(f_rust_wrappers, "\tfn ", rust_with_over_name, "(", NULL);
      if(!is_pure_virtual) {
        if (overname) {
          Printv(f_rust_trait_base_calls,"#[inline]\npub fn ",cn,"_upcall_",name,overname,"(",NULL);
        } else {
          Printv(f_rust_trait_base_calls,"#[inline]\npub fn ",cn,"_upcall_",name,"(",NULL);
        }
      }
      
      if (!is_static) {
        if (is_const) {
          Printv(f_rust_wrappers, "&self", NULL);
          if(!is_pure_virtual) {
            Printv(f_rust_trait_base_calls,"obj : &impl ",cn,"TraitBase",NULL);
          }
        } else {
          Printv(f_rust_wrappers, "&mut self", NULL);
          if(!is_pure_virtual) {
            Printv(f_rust_trait_base_calls,"obj : &mut impl ",cn,"TraitBase",NULL);
          }
        }
      }

      p = parms;
      for (int i = 0; i < parm_count; ++i) {
        p = getParm(p);
        if (i > 0 || !is_static) {
          Printv(f_rust_wrappers, ", ", NULL);
          if(!is_pure_virtual) {
            Printv(f_rust_trait_base_calls,", ",NULL);
          }
        }
        Printv(f_rust_wrappers, Getattr(p, "lname"), ": ", NULL);
        String *tm = rustType(p, Getattr(p, "type"));
        Printv(f_rust_wrappers, tm, NULL);
        if(!is_pure_virtual) {
          Printv(f_rust_trait_base_calls, Getattr(p, "lname"), ": ", NULL);
          Printv(f_rust_trait_base_calls, tm, NULL);
        }
        Delete(tm);
        p = nextParm(p);
      }

      Printv(f_rust_wrappers, ")", NULL);
      if(!is_pure_virtual) {
        Printv(f_rust_trait_base_calls, ")", NULL);
      }

      if (!is_void) {
        String *tm = rustType(n, returntype);
        Printv(f_rust_wrappers, " -> ", tm, NULL);
        if(!is_pure_virtual) {
          Printv(f_rust_trait_base_calls, " -> ", tm, NULL);
        }
        Delete(tm);
      }
      
      if(!is_pure_virtual) {
        Printv(f_rust_trait_base_calls, "{\n", NULL);
      }
      Printv(f_rust_wrappers, " {\n", NULL);
      
      if (is_pure_virtual) {
        Printv(f_rust_wrappers, "\t\tunimplemented!(\"Pure virtual method is not implemented\");\n", NULL);
      } else {
        // Call base class implementation
        Printv(f_rust_trait_base_calls, "\tunsafe {\n", NULL);
        Printv(f_rust_wrappers, "\t\tunsafe {\n", NULL);
        if (!is_void) {
          Printv(f_rust_wrappers, "\t\t\treturn ", NULL);
          Printv(f_rust_trait_base_calls, "\t\treturn ", NULL);
        } else {
          Printv(f_rust_wrappers, "\t\t\t", NULL);
          Printv(f_rust_trait_base_calls, "\t\t", NULL);
        }
        
        String *super_call = NewString("");
        Printv(super_call, upcall_wname, "(", NULL);
        Printv(f_rust_trait_base_calls, upcall_wname, "(", NULL);
        if (!is_static) {
          if(is_const){
            Printv(super_call, "self.get_",cn,"_base_const()", NULL);
            Printv(f_rust_trait_base_calls, "obj.get_",cn,"_base_const()", NULL);
          } else {
            Printv(super_call, "self.get_",cn,"_base_mut()", NULL);
            Printv(f_rust_trait_base_calls, "obj.get_",cn,"_base_mut()", NULL);
          }
        }
        
        p = parms;
        for (int i = 0; i < parm_count; ++i) {
          p = getParm(p);
          if (i > 0 || !is_static) {
            Printv(super_call, ", ", NULL);
            Printv(f_rust_trait_base_calls, ", ", NULL);
          }
          Printv(super_call, Getattr(p, "lname"), NULL);
          Printv(f_rust_trait_base_calls, Getattr(p, "lname"), NULL);
          p = nextParm(p);
        }
        Printv(super_call, ")", NULL);
        Printv(f_rust_trait_base_calls, ");\n", NULL);
        
        Printv(f_rust_wrappers, super_call, ";\n", NULL);
        Printv(f_rust_wrappers, "\t\t}\n", NULL);
        Printv(f_rust_trait_base_calls, "\t}\n", NULL);
        Delete(super_call);
      }
      
      Printv(f_rust_wrappers, "\t}\n\n", NULL);
      if(!is_pure_virtual) {
        Printv(f_rust_trait_base_calls, "}\n\n", NULL);
      }

      // Generate the C callback function that will be called from C++
      if (!GetFlag(n, "abstract")) {
        Printv(f_rust_pre_extern, "extern \"C\" { fn ", upcall_wname, "(", NULL);
        if (!is_static) {
          if(is_const) {
            Printv(f_rust_pre_extern, "base_ptr: *const ", rust_type_name, NULL);
          } else {
            Printv(f_rust_pre_extern, "base_ptr: *mut ", rust_type_name, NULL);
          }
        }
        
        p = parms;
        for (int i = 0; i < parm_count; ++i) {
          p = getParm(p);
          if (i > 0 || !is_static) {
            Printv(f_rust_pre_extern, ", ", NULL);
          }
          String *ct = rustImType(p, Getattr(p, "type"));
          Printv(f_rust_pre_extern, Getattr(p, "lname"), ": ", ct, NULL);
          Delete(ct);
          p = nextParm(p);
        }
        
        Printv(f_rust_pre_extern, ")", NULL);
        if (!is_void) {
          String *ret_type = rustImType(n, returntype);
          Printv(f_rust_pre_extern, " -> ", ret_type, NULL);
          Delete(ret_type);
        }
        Printv(f_rust_pre_extern, "; }\n", NULL);
        String *upcall_method_name = NewString("_swig_upcall_");
        Append(upcall_method_name, name);
        if (overname) {
          Append(upcall_method_name, overname);
        }
        SwigType *rtype = Getattr(n, "classDirectorMethods:type");
        String *upcall_decl = Swig_method_decl(rtype, Getattr(n, "decl"),
                                               upcall_method_name, parms, 0);
        Printv(f_c_directors_h, "  ", upcall_decl, " {\n", NULL);
        Delete(upcall_decl);

        Printv(f_c_directors_h, "    ", NULL);
        if (!is_void) {
          Printv(f_c_directors_h, "return ", NULL);
        }

        String *super_call = Swig_method_call(super, parms);
        Printv(f_c_directors_h, super_call, ";\n", NULL);
        Delete(super_call);

        Printv(f_c_directors_h, "  }\n", NULL);

        // Define the C++ function that the Rust function calls.

        SwigType *first_type = NULL;
        Parm *first_parm = parms;
        if (!is_static) {
          first_type = NewString("SwigDirector_");
          Append(first_type, class_name);
          SwigType_add_pointer(first_type);
          first_parm = NewParm(first_type, "p", n);
          set_nextSibling(first_parm, parms);
        }

        Swig_save("classDirectorMethod", n, "wrap:name", "wrap:action", NULL);

        Setattr(n, "wrap:name", upcall_wname);

        String *action = NewString("");
        if (!is_void) {
          Printv(action, Swig_cresult_name(), " = (",
                 SwigType_lstr(returntype, 0), ")", NULL);
          if (SwigType_isreference(returntype)) {
            Printv(action, "&", NULL);
          }
        }
        Printv(action, Swig_cparm_name(NULL, 0), "->", upcall_method_name, "(",
               NULL);

        p = parms;
        int i = 0;
        while (p != NULL) {
          if (SwigType_type(Getattr(p, "type")) != T_VOID) {
            String *pname = Swig_cparm_name(NULL, i + 1);
            if (i > 0) {
              Printv(action, ", ", NULL);
            }

            // A parameter whose type is a reference is converted into a
            // pointer type by gcCTypeForRustValue.  We are calling a
            // function which expects a reference so we need to convert
            // back.
            if (SwigType_isreference(Getattr(p, "type"))) {
              Printv(action, "*", NULL);
            }

            Printv(action, pname, NULL);
            Delete(pname);
            i++;
          }
          p = nextSibling(p);
        }
        Printv(action, ");", NULL);
        Setattr(n, "wrap:action", action);

        cppRustWrapperInfo info;

        info.n = n;
        info.rust_name = rust_name;
        info.overname = overname;
        info.wname = upcall_wname;
        info.base = NULL;
        info.parms = first_parm;
        info.result = returntype;
        info.is_static = is_static;
        info.receiver = NULL;
        info.is_constructor = false;
        info.is_destructor = false;

        int r = cppWrapper(&info);
        if (r != SWIG_OK) {
          return r;
        }

        Delete(first_type);
        if (first_parm != parms) {
          Delete(first_parm);
        }

        Swig_restore(n);
        Delete(upcall_method_name);
      }

      // Generate the Rust callback function
      Printv(f_rust_after_director_extern, "#[no_mangle]\n", NULL);
      if(is_const){
        Printv(f_rust_after_director_extern, "pub extern \"C\" fn ", callback_name, "(rust_id: *const ",cn, "TraitWrapper", NULL);
      } else {
        Printv(f_rust_after_director_extern, "pub extern \"C\" fn ", callback_name, "(rust_id: *mut ",cn, "TraitWrapper", NULL);
      }

      p = parms;
      for (int i = 0; i < parm_count; ++i) {
        p = getParm(p);
        String *tm = rustWrapperType(p, Getattr(p, "type"), false);
        Printv(f_rust_after_director_extern, ", ", Getattr(p, "lname"), ": ", tm, NULL);
        Delete(tm);
        p = nextParm(p);
      }

      Printv(f_rust_after_director_extern, ")", NULL);
      String *result_wrapper = NULL;
      if (!is_void) {
        result_wrapper = rustWrapperType(n, returntype, true);
        Printv(f_rust_after_director_extern, " -> ", result_wrapper, NULL);
      }
      Printv(f_rust_after_director_extern, " {\n", NULL);

      if (is_ignored) {
        if (!is_void) {
          Printv(f_rust_after_director_extern, "\tpanic!(\"Ignored method called\");\n", NULL);
        } else {
          Printv(f_rust_after_director_extern, "\t// Ignored method - do nothing\n", NULL);
        }
      } else {
        if(is_const) {
          Printv(f_rust_after_director_extern, "\tlet r = unsafe { &*rust_id };\n", NULL);
        } else {
          Printv(f_rust_after_director_extern, "\tlet r = unsafe { &mut *rust_id };\n", NULL);
        }
        // Get the Rust object from the tracker and call the method
        // Printv(f_rust_after_director_extern, "    match swig_director_lookup(rust_id) {\n", NULL);
        // Printv(f_rust_after_director_extern, "        Ok(rust_obj_arc) => {\n", NULL);
        
        // if (is_const) {
        //   Printv(f_rust_after_director_extern, "            let rust_obj = rust_obj_arc.read().unwrap();\n", NULL);
        // } else {
        //   Printv(f_rust_after_director_extern, "            let rust_obj = rust_obj_arc.write().unwrap();\n", NULL);
        // }
        
        // Printv(f_rust_after_director_extern, "            let trait_obj = rust_obj.downcast_ref::<", cn, "TraitWrapper>().unwrap();\n", NULL);
        
        String *call = NewString("");
        if (!is_void) {
          if (is_const) {
            Printv(call, "\treturn ", cn,"Trait::",rust_with_over_name,"(& *r.obj.read().unwrap()", NULL);
          } else {
            Printv(call, "\treturn ", cn,"Trait::",rust_with_over_name,"(&mut *r.obj.write().unwrap()", NULL);
          }
        } else {
          if (is_const) {
            Printv(call, "\t",cn,"Trait::",rust_with_over_name,"(& *r.obj.read().unwrap()", NULL);
          } else {
            Printv(call, "\t",cn,"Trait::",rust_with_over_name,"(&mut *r.obj.write().unwrap()", NULL);
          }
        }

        p = parms;
        for (int i = 1; i < parm_count; ++i) {
          p = getParm(p);
          if (i > 0) {
            Printv(call, ", ", NULL);
          }
          SwigType *pt = Getattr(p, "type");
          String *ln = Getattr(p, "lname");

          // Handle interface types that need conversion
          bool is_interface = rustTypeIsTrait(p, pt);
          if (is_interface) {
            String *wt = rustWrapperType(p, pt, true);
            Printv(call, wt, "(", ln, ")", NULL);
            Delete(wt);
          } else {
            Printv(call, ln, NULL);
          }
          p = nextParm(p);
        }

        Printv(call, ");\n", NULL);
        Printv(f_rust_after_director_extern, call, NULL);
        Delete(call);
        
        // Printv(f_rust_after_director_extern, "        },\n", NULL);
        // Printv(f_rust_after_director_extern, "        Err(e) => {\n", NULL);
        // Printv(f_rust_after_director_extern, "            panic!(\"Director lookup failed: {}\", e);\n", NULL);
        // Printv(f_rust_after_director_extern, "        }\n", NULL);
        // Printv(f_rust_after_director_extern, "    }\n", NULL);
      }

      Printv(f_rust_after_director_extern, "}\n\n", NULL);

      Delete(result_wrapper);
    }

    if (!is_ignored || is_pure_virtual) {
      // Declare the method for the C++ director class
      SwigType *rtype = Getattr(n, "conversion_operator") ? 0 : Getattr(n, "classDirectorMethods:type");
      String *decl = Swig_method_decl(rtype, Getattr(n, "decl"), Getattr(n, "name"), parms, 0);
      Printv(f_c_directors_h, "  virtual ", decl, NULL);
      Delete(decl);

      String *qname = NewString("");
      Printv(qname, "SwigDirector_", class_name, "::", Getattr(n, "name"), NULL);
      decl = Swig_method_decl(rtype, Getattr(n, "decl"), qname, parms, 0);
      Printv(w->def, decl, NULL);
      Delete(decl);
      Delete(qname);

      String *throws = buildThrow(n);
      if (throws) {
        Printv(f_c_directors_h, " ", throws, NULL);
        Printv(w->def, " ", throws, NULL);
        Delete(throws);
      }

      Printv(f_c_directors_h, ";\n", NULL);
      Printv(w->def, " {\n", NULL);

      if (!is_void) {
        if (!SwigType_isclass(returntype)) {
          if (!(SwigType_ispointer(returntype) || SwigType_isreference(returntype))) {
            String *construct_result = NewStringf("= SwigValueInit< %s >()", SwigType_lstr(returntype, 0));
            Wrapper_add_localv(w, "c_result", SwigType_lstr(returntype, "c_result"), construct_result, NIL);
            Delete(construct_result);
          } else {
            Wrapper_add_localv(w, "c_result", SwigType_lstr(returntype, "c_result"), "= 0", NIL);
          }
        } else {
          String *cres = SwigType_lstr(returntype, "c_result");
          Printf(w->code, "%s;\n", cres);
          Delete(cres);
        }
      }

      if (!is_ignored) {
        makeDirectorMethodWrapper(n, w, callback_name);
      } else {
        assert(is_pure_virtual);
        Printv(w->code, "  _swig_rustpanic(\"call to pure virtual function ", Getattr(parent, "sym:name"), name, "\");\n", NULL);
        if (!is_void) {
          String *retstr = SwigType_rcaststr(returntype, "c_result");
          Printv(w->code, "  return ", retstr, ";\n", NULL);
          Delete(retstr);
        }
      }

      Printv(w->code, "}", NULL);
      Replaceall(w->code, "$isvoid", is_void ? "1" : "0");
      Replaceall(w->code, "$symname", symname);
      Wrapper_print(w, f_c_directors);
    }

    Delete(cn);
    Delete(rust_type_name);
    Delete(callback_name);
    Delete(upcall_wname);
    Delete(rust_with_over_name);
    Delete(rust_name);
    DelWrapper(w);

    return SWIG_OK;
  }

  /* ------------------------------------------------------------
   * makeDirectorMethodWrapper
   *
   * Emit the function wrapper for a director method.
   * ------------------------------------------------------------ */
  void makeDirectorMethodWrapper(Node *n, Wrapper *w, String *callback_name) {
    ParmList *parms = Getattr(n, "wrap:parms");
    SwigType *returntype = Getattr(n, "type");
    bool is_void = (SwigType_type(returntype) == T_VOID);

    Printv(f_c_directors, "extern \"C\" ", NULL);

    String *fnname = Copy(callback_name);
    Append(fnname, "(void *");

    Parm *p = parms;
    while (p) {
      while (checkAttribute(p, "tmap:directorin:numinputs", "0")) {
        p = Getattr(p, "tmap:directorin:next");
      }
      String *cg = gcCTypeForRustValue(p, Getattr(p, "type"), Getattr(p, "lname"));
      Printv(fnname, ", ", cg, NULL);
      Delete(cg);
      p = Getattr(p, "tmap:directorin:next");
    }

    Printv(fnname, ")", NULL);

    if (is_void) {
      Printv(f_c_directors, "void ", fnname, NULL);
    } else {
      String *tm = gcCTypeForRustValue(n, returntype, fnname);
      Printv(f_c_directors, tm, NULL);
      Delete(tm);
    }

    Delete(fnname);
    Printv(f_c_directors, ";\n", NULL);

    if (!is_void) {
      String *r = NewString(Swig_cresult_name());
      String *tm = gcCTypeForRustValue(n, returntype, r);
      Wrapper_add_local(w, r, tm);
      Delete(tm);
      Delete(r);
    }

    String *args = NewString("");

    p = parms;
    while (p) {
      while (checkAttribute(p, "tmap:directorin:numinputs", "0")) {
        p = Getattr(p, "tmap:directorin:next");
      }

      String *pn = NewString("swig_");
      Append(pn, Getattr(p, "lname"));
      Setattr(p, "emit:directorinput", pn);

      String *tm = gcCTypeForRustValue(p, Getattr(p, "type"), pn);
      Wrapper_add_local(w, pn, tm);
      Delete(tm);

      tm = Getattr(p, "tmap:directorin");
      if (!tm) {
        Swig_warning(WARN_TYPEMAP_DIRECTORIN_UNDEF, input_file, line_number,
                     "unable to use type %s as director method argument\n",
                     SwigType_str(Getattr(p, "type"), 0));
      } else {
        tm = Copy(tm);
        Replaceall(tm, "$input", pn);
        Replaceall(tm, "$owner", 0);
        Printv(w->code, "  ", tm, "\n", NULL);
        Delete(tm);
        Printv(args, ", ", pn, NULL);
      }

      p = Getattr(p, "tmap:directorin:next");
    }

    Printv(w->code, "  ", NULL);
    if (!is_void) {
      Printv(w->code, Swig_cresult_name(), " = ", NULL);
    }
    Printv(w->code, callback_name, "(rust_val", args, ");\n", NULL);

    /* Marshal outputs */
    for (p = parms; p;) {
      String *tm;
      if ((tm = Getattr(p, "tmap:directorargout"))) {
        tm = Copy(tm);
        Replaceall(tm, "$result", "jresult");
        Replaceall(tm, "$input", Getattr(p, "emit:directorinput"));
        Printv(w->code, tm, "\n", NULL);
        Delete(tm);
        p = Getattr(p, "tmap:directorargout:next");
      } else {
        p = nextSibling(p);
      }
    }

    if (!is_void) {
      String *result_str = NewString("c_result");
      String *tm = Swig_typemap_lookup("directorout", n, result_str, NULL);
      if (!tm) {
        Swig_warning(WARN_TYPEMAP_DIRECTOROUT_UNDEF, input_file, line_number,
                     "Unable to use type %s as director method returntype\n",
                     SwigType_str(returntype, 0));
      } else {
        tm = Copy(tm);
        Replaceall(tm, "$input", Swig_cresult_name());
        Replaceall(tm, "$result", "c_result");
        Printv(w->code, "  ", tm, "\n", NULL);
        String *retstr = SwigType_rcaststr(returntype, "c_result");
        Printv(w->code, "  return ", retstr, ";\n", NULL);
        Delete(retstr);
        Delete(tm);
      }
      Delete(result_str);
    }

    Delete(args);
  }
  
  /* ------------------------------------------------------------
   * classDirectorEnd
   *
   * Complete support for a director class.
   * ------------------------------------------------------------ */

  int classDirectorEnd(Node *n) {

    // Close the trait definition that was opened in classDirectorInit
    Printv(f_rust_wrappers, "}\n\n", NULL);

    Printv(f_c_directors_h, " private:\n", NULL);
    Printv(f_c_directors_h, "  void * rust_val;\n", NULL);
    Printv(f_c_directors_h, "  Swig_memory *swig_mem;\n", NULL);
    Printv(f_c_directors_h, "};\n\n", NULL);

    class_name = NULL;
    class_node = NULL;

    Delete(class_receiver);
    class_receiver = NULL;

    Delete(class_methods);
    class_methods = NULL;

    return SWIG_OK;
  }
  /* ------------------------------------------------------------
   * classDirectorDisown
   *
   * I think Rust does not require a disown method.
   * ------------------------------------------------------------ */

  int classDirectorDisown(Node *n) {
    (void)n;
    return SWIG_OK;
  }

  /*----------------------------------------------------------------------
   * buildThrow()
   *
   * Build and return a throw clause if needed.
   *--------------------------------------------------------------------*/

  String *buildThrow(Node *n) {
    if (Getattr(n, "noexcept"))
      return NewString("noexcept");
    ParmList *throw_parm_list = Getattr(n, "throws");
    if (!throw_parm_list && !Getattr(n, "throw"))
      return NULL;
    String *ret = NewString("throw(");
    if (throw_parm_list) {
      Swig_typemap_attach_parms("throws", throw_parm_list, NULL);
    }
    bool first = true;
    for (Parm *p = throw_parm_list; p; p = nextSibling(p)) {
      if (Getattr(p, "tmap:throws")) {
        if (first) {
          first = false;
        } else {
          Printv(ret, ", ", NULL);
        }
        String *s = SwigType_str(Getattr(p, "type"), 0);
        Printv(ret, s, NULL);
        Delete(s);
      }
    }
    Printv(ret, ")", NULL);
    return ret;
  }

  /*----------------------------------------------------------------------
   * extraDirectorProtectedCPPMethodsRequired()
   *
   * We don't need to check upcall when calling methods.
   *--------------------------------------------------------------------*/

  bool extraDirectorProtectedCPPMethodsRequired() const { return false; }

  /*----------------------------------------------------------------------
   * makeDispatchFunction
   *
   * Make a dispatch function for an overloaded C++ function.  The
   * receiver parameter is the receiver for a method, unless is_upcall
   * is true.  If is_upcall is true, then the receiver parameter is
   * the type of the first argument to the function.
   *--------------------------------------------------------------------*/

  int makeDispatchFunction(Node *n, String *rust_name, String *receiver,
                           bool is_static, SwigType *director_struct,
                           bool is_upcall) {
    bool is_director = director_struct ? true : false;

    String *nodetype = Getattr(n, "nodeType");
    bool is_constructor = Cmp(nodetype, "constructor") == 0;
    bool is_destructor = Cmp(nodetype, "destructor") == 0;

    bool can_use_receiver = (!is_constructor && !is_destructor && !is_upcall);

    bool use_receiver = (!is_static && can_use_receiver);

    bool add_to_interface = (interfaces && !is_constructor && !is_destructor &&
                             !is_static && !is_upcall);

    List *dispatch = Swig_overload_rank(n, false);
    int nfunc = Len(dispatch);

    SwigType *all_result;
    bool mismatch;
    if (is_constructor) {
      assert(!is_upcall);
      if (!is_director) {
        all_result = Copy(Getattr(class_node, "classtypeobj"));
      } else {
        all_result = Copy(director_struct);
      }
      mismatch = false;
    } else {
      all_result = NULL;
      mismatch = false;
      bool any_void = false;
      for (int i = 0; i < nfunc; ++i) {
        Node *nn = Getitem(dispatch, i);
        Node *ni =
            Getattr(nn, "directorNode") ? Getattr(nn, "directorNode") : nn;
        SwigType *result = Getattr(ni, "rust:type");
        assert(result);

        if (SwigType_type(result) == T_VOID) {
          if (all_result) {
            mismatch = true;
          }
          any_void = true;
        } else {
          if (any_void) {
            mismatch = true;
          } else if (!all_result) {
            all_result = Copy(result);
          } else if (Cmp(result, all_result) != 0) {
            mismatch = true;
          }
        }
      }
      if (mismatch) {
        Delete(all_result);
        all_result = NULL;
      } else if (all_result) {
        ;
      } else {
        all_result = NewString("void");
      }
    }

    Printv(f_rust_wrappers, "func ", NULL);

    if (receiver && use_receiver) {
      Printv(f_rust_wrappers, "(p ", receiver, ") ", NULL);
    }

    Printv(f_rust_wrappers, rust_name, "(", NULL);
    if (is_director && is_constructor) {
      Printv(f_rust_wrappers, "abi interface{}, ", NULL);
      assert(!add_to_interface);
    }
    if (is_upcall) {
      Printv(f_rust_wrappers, "p *", receiver, ", ", NULL);
      assert(!add_to_interface);
    }
    Printv(f_rust_wrappers, "a ...interface{})", NULL);

    if (add_to_interface) {
      Printv(interfaces, "\tfn ", rust_name, "(a ...interface{})", NULL);
    }

    if (mismatch) {
      Printv(f_rust_wrappers, " interface{}", NULL);
      if (add_to_interface) {
        Printv(interfaces, " interface{}", NULL);
      }
    } else if (all_result && SwigType_type(all_result) != T_VOID) {
      if (is_director && is_constructor) {
        Printv(f_rust_wrappers, " ", receiver, NULL);
        if (add_to_interface) {
          Printv(interfaces, " ", receiver, NULL);
        }
      } else {
        String *tm = rustType(n, all_result);
        Printv(f_rust_wrappers, " ", tm, NULL);
        if (add_to_interface) {
          Printv(interfaces, " ", tm, NULL);
        }
        Delete(tm);
      }
    }
    Printv(f_rust_wrappers, " {\n", NULL);
    if (add_to_interface) {
      Printv(interfaces, "\n", NULL);
    }

    Printv(f_rust_wrappers, "\targc := len(a)\n", NULL);

    for (int i = 0; i < nfunc; ++i) {
      int fn = 0;
      Node *nn = Getitem(dispatch, i);
      Node *ni = Getattr(nn, "directorNode") ? Getattr(nn, "directorNode") : nn;
      Parm *pi = Getattr(ni, "wrap:parms");

      // If we are using a receiver, we want to ignore a leading self
      // parameter.  Because of the way this is called, there may or
      // may not be a self parameter at this point.
      if (use_receiver && pi && Getattr(pi, "self")) {
        pi = getParm(pi);
        if (pi) {
          pi = nextParm(pi);
        }
      }

      int num_required = emit_num_required(pi);
      int num_arguments = emit_num_arguments(pi);
      bool varargs = emit_isvarargs(pi) ? true : false;

      if (varargs) {
        Printf(f_rust_wrappers, "\tif argc >= %d {\n", num_required);
      } else {
        if (num_required == num_arguments) {
          Printf(f_rust_wrappers, "\tif argc == %d {\n", num_required);
        } else {
          Printf(f_rust_wrappers, "\tif argc >= %d && argc <= %d {\n",
                 num_required, num_arguments);
        }
      }

      // Build list of collisions with the same number of arguments.
      List *coll = NewList();
      for (int k = i + 1; k < nfunc; ++k) {
        Node *nnk = Getitem(dispatch, k);
        Node *nk =
            Getattr(nnk, "directorNode") ? Getattr(nnk, "directorNode") : nnk;
        Parm *pk = Getattr(nk, "wrap:parms");
        if (use_receiver && pk && Getattr(pk, "self")) {
          pk = getParm(pk);
          if (pk) {
            pk = nextParm(pk);
          }
        }
        int nrk = emit_num_required(pk);
        int nak = emit_num_arguments(pk);
        if ((nrk >= num_required && nrk <= num_arguments) ||
            (nak >= num_required && nak <= num_arguments) ||
            (nrk <= num_required && nak >= num_arguments) ||
            (varargs && nrk >= num_required)) {
          Append(coll, nk);
        }
      }

      int num_braces = 0;
      if (Len(coll) > 0 && num_arguments > 0) {
        int j = 0;
        Parm *pj = pi;
        while (pj) {
          pj = getParm(pj);
          if (!pj) {
            break;
          }

          // If all the overloads have the same type in this position,
          // we can omit the check.
          SwigType *tm = rustOverloadType(pj, Getattr(pj, "type"));
          bool emitcheck = false;
          for (int k = 0; k < Len(coll) && !emitcheck; ++k) {
            Node *nk = Getitem(coll, k);
            Parm *pk = Getattr(nk, "wrap:parms");
            if (use_receiver && pk && Getattr(pk, "self")) {
              pk = getParm(pk);
              if (pk) {
                pk = nextParm(pk);
              }
            }
            int nak = emit_num_arguments(pk);
            if (nak <= j)
              continue;
            int l = 0;
            Parm *pl = pk;
            while (pl && l <= j) {
              pl = getParm(pl);
              if (!pl) {
                break;
              }
              if (l == j) {
                SwigType *tml = rustOverloadType(pl, Getattr(pl, "type"));
                if (Cmp(tm, tml) != 0) {
                  emitcheck = true;
                }
                Delete(tml);
              }
              pl = nextParm(pl);
              ++l;
            }
          }

          if (emitcheck) {
            if (j >= num_required) {
              Printf(f_rust_wrappers, "\t\tif argc > %d {\n", j);
              ++num_braces;
            }

            fn = i + 1;
            Printf(f_rust_wrappers, "\t\tif _, ok := a[%d].(%s); !ok {\n", j,
                   tm);
            Printf(f_rust_wrappers, "\t\t\trustto check_%d\n", fn);
            Printv(f_rust_wrappers, "\t\t}\n", NULL);
          }

          Delete(tm);

          pj = nextParm(pj);

          ++j;
        }
      }

      for (; num_braces > 0; --num_braces) {
        Printv(f_rust_wrappers, "\t\t}\n", NULL);
      }

      // We may need to generate multiple calls if there are variable
      // argument lists involved.  Build the start of the call.

      String *start = NewString("");

      SwigType *result = Getattr(ni, "rust:type");

      if (is_constructor) {
        result = all_result;
      } else if (is_destructor) {
        result = NULL;
      }

      if (result && SwigType_type(result) != T_VOID &&
          (!all_result || SwigType_type(all_result) != T_VOID)) {
        Printv(start, "return ", NULL);
      }

      bool advance_parm = false;

      if (receiver && use_receiver) {
        Printv(start, "p.", rust_name, NULL);
      } else if (can_use_receiver && !isStatic(ni) && pi &&
                 Getattr(pi, "self")) {
        // This is an overload of a static function and a non-static
        // function.
        assert(num_required > 0);
        SwigType *tm = rustWrapperType(pi, Getattr(pi, "type"), true);
        String *nm =
            buildRustName(Getattr(ni, "sym:name"), false, isFriend(ni));
        Printv(start, "a[0].(", tm, ").", nm, NULL);
        Delete(nm);
        Delete(tm);
        advance_parm = true;
      } else {
        Printv(start, rust_name, NULL);
      }

      Printv(start, Getattr(ni, "sym:overname"), "(", NULL);

      bool need_comma = false;

      if (is_director && is_constructor) {
        Printv(start, "abi", NULL);
        need_comma = true;
      }
      if (is_upcall) {
        Printv(start, "p", NULL);
        need_comma = true;
      }
      Parm *p = pi;
      int pn = 0;
      if (advance_parm) {
        p = getParm(p);
        if (p) {
          p = nextParm(p);
        }
        ++pn;
      }
      while (pn < num_required) {
        p = getParm(p);

        if (need_comma) {
          Printv(start, ", ", NULL);
        }

        SwigType *tm = rustType(p, Getattr(p, "type"));
        Printf(start, "a[%d].(%s)", pn, tm);
        Delete(tm);

        need_comma = true;
        ++pn;
        p = nextParm(p);
      }

      String *end = NULL;
      if (!result || SwigType_type(result) == T_VOID ||
          (all_result && SwigType_type(all_result) == T_VOID)) {
        end = NewString("");
        Printv(end, "return", NULL);
        if (!all_result || SwigType_type(all_result) != T_VOID) {
          Printv(end, " 0", NULL);
        }
      }

      if (num_required == num_arguments) {
        Printv(f_rust_wrappers, "\t\t", start, ")\n", NULL);
        if (end) {
          Printv(f_rust_wrappers, "\t\t", end, "\n", NULL);
        }
      } else {
        Printv(f_rust_wrappers, "\t\tswitch argc {\n", NULL);
        for (int j = num_required; j <= num_arguments; ++j) {
          Printf(f_rust_wrappers, "\t\tcase %d:\n", j);
          Printv(f_rust_wrappers, "\t\t\t", start, NULL);
          bool nc = need_comma;
          for (int k = num_required; k < j; ++k) {
            if (nc) {
              Printv(f_rust_wrappers, ", ", NULL);
            }
            Printf(f_rust_wrappers, "a[%d]", k);
            nc = true;
          }
          Printv(f_rust_wrappers, ")\n", NULL);
          if (end) {
            Printv(f_rust_wrappers, "\t\t\t", end, "\n", NULL);
          }
        }
        Printv(f_rust_wrappers, "\t\t}\n", NULL);
      }

      Printv(f_rust_wrappers, "\t}\n", NULL);

      if (fn != 0) {
        Printf(f_rust_wrappers, "check_%d:\n", fn);
      }

      Delete(coll);
    }

    Printv(f_rust_wrappers,
           "\tpanic(\"No match for overloaded function call\")\n", NULL);
    Printv(f_rust_wrappers, "}\n\n", NULL);

    Delete(all_result);
    Delete(dispatch);

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------
   * checkFunctionVisibility()
   *
   * Return true if we should write out a function based on its
   * visibility, false otherwise.
   * ---------------------------------------------------------------------- */

  bool checkFunctionVisibility(Node *n, Node *parent) {
    // Write out a public function.
    if (is_public(n))
      return true;
    // Don't write out a private function.
    if (is_private(n))
      return false;
    // Write a protected function for a director class in
    // dirprot_mode.
    if (parent == NULL) {
      return false;
    }
    if (dirprot_mode() && Swig_directorclass(parent))
      return true;
    // Otherwise don't write out a protected function.
    return false;
  }

  /* ----------------------------------------------------------------------
   * exportedName()
   *
   * Given a C/C++ name, return a name in Rust which will be exported.
   * If the first character is an upper case letter, this returns a
   * copy of its argument.  If the first character is a lower case
   * letter, this forces it to upper case.  Otherwise, this prepends
   * 'X'.
   * ---------------------------------------------------------------------- */

  String *exportedName(SwigType *name) {
    SwigType *copy = Copy(name);
    char c = *Char(copy);
    if (islower(c)) {
      char l[2];
      char u[2];
      l[0] = c;
      l[1] = '\0';
      u[0] = toupper(c);
      u[1] = '\0';
      Replace(copy, l, u, DOH_REPLACE_FIRST);
    } else if (!isalpha(c)) {
      char l[2];
      char u[3];
      l[0] = c;
      l[1] = '\0';
      u[0] = 'X';
      u[1] = c;
      u[2] = '\0';
      Replace(copy, l, u, DOH_REPLACE_FIRST);
    }
    String *ret = Swig_name_mangle_type(copy);
    Delete(copy);
    return ret;
  }

  /* ----------------------------------------------------------------------
   * removeClassname()
   *
   * If the name starts with the current class name, followed by an
   * underscore, remove it.  If there is no current class name, this
   * simply returns a copy of the name.  This undoes Swig's way of
   * recording the class name in a member name.
   * ---------------------------------------------------------------------- */

  String *removeClassname(String *name) {
    String *copy = Copy(name);
    if (class_name) {
      char *p = Char(name);
      if (Strncmp(name, class_name, Len(class_name)) == 0 &&
          p[Len(class_name)] == '_') {
        Replace(copy, class_name, "", DOH_REPLACE_FIRST);
        Replace(copy, "_", "", DOH_REPLACE_FIRST);
      }
    }
    return copy;
  }

  /* ----------------------------------------------------------------------
   * buildRustName()
   *
   * Build the name to use for an ordinary function, variable, or
   * whatever in Rust.  The name argument is something like the sym:name
   * attribute of the node.  If is_static is false, this could be a
   * method, and the returned name will be the name of the
   * method--i.e., it will not include the class name.
   * ---------------------------------------------------------------------- */

  String *buildRustName(String *name, bool is_static, bool is_friend) {
    String *nw = NewString("");
    if (is_static && !is_friend && class_name) {
      String *c1 = exportedName(class_name);
      Append(nw, c1);
      Delete(c1);
    }
    String *c2 = removeClassname(name);
    String *c3 = exportedName(c2);
    Append(nw, c3);
    Delete(c2);
    Delete(c3);
    String *ret = Swig_name_mangle_string(nw);
    Delete(nw);
    return ret;
  }

  /* ----------------------------------------------------------------------
   * buildRustWrapperName()
   *
   * Build the name to use for a Rust wrapper function.  This is a
   * function called by the real Rust function in order to convert C++
   * classes from interfaces to pointers, and other such conversions
   * between the Rust type and the C++ type.
   * ---------------------------------------------------------------------- */

  String *buildRustWrapperName(String *name, String *overname) {
    String *s1 = NewString("_swig_wrap_");
    Append(s1, name);
    String *s2 = Swig_name_mangle_string(s1);
    Delete(s1);
    if (overname) {
      Append(s2, overname);
    }
    return s2;
  }

  /* ----------------------------------------------------------------------
   * checkNameConflict()
   *
   * Check for a name conflict on the name we are going to use in Rust.
   * These conflicts are likely because of the enforced
   * capitalization.  When we find one, issue a warning and return
   * false.  If the name is OK, return true.
   * ---------------------------------------------------------------------- */

  bool checkNameConflict(String *name, Node *n,
                         const_String_or_char_ptr scope) {
    Node *lk = symbolLookup(name, scope);
    if (lk) {
      String *n1 = Getattr(n, "sym:name");
      if (!n1) {
        n1 = Getattr(n, "name");
      }
      String *n2 = Getattr(lk, "sym:name");
      if (!n2) {
        n2 = Getattr(lk, "name");
      }
      Swig_warning(WARN_RUST_NAME_CONFLICT, input_file, line_number,
                   "Ignoring '%s' due to Rust name ('%s') conflict with '%s'\n",
                   n1, name, n2);
      return false;
    }
    bool r = addSymbol(name, n, scope) ? true : false;
    assert(r);
    (void)r;
    return true;
  }

  /* ----------------------------------------------------------------------
   * checkIgnoredParameters()
   *
   * If any of the parameters of this function, or the return type,
   * are ignored due to a name conflict, give a warning and return
   * false.
   * ---------------------------------------------------------------------- */

  bool checkIgnoredParameters(Node *n, String *rust_name) {
    ParmList *parms = Getattr(n, "parms");
    if (parms) {
      Wrapper *dummy = NewWrapper();
      emit_attach_parmmaps(parms, dummy);
      int parm_count = emit_num_arguments(parms);
      Parm *p = parms;

      for (int i = 0; i < parm_count; ++i) {
        p = getParm(p);
        if (!checkIgnoredType(n, rust_name, Getattr(p, "type"))) {
          DelWrapper(dummy);
          return false;
        }
        p = nextParm(p);
      }

      DelWrapper(dummy);
    }

    if (!checkIgnoredType(n, rust_name, Getattr(n, "type"))) {
      return false;
    }

    return true;
  }

  /* ----------------------------------------------------------------------
   * checkIgnoredType()
   *
   * If this type is being ignored due to a name conflict, give a
   * warning and return false.
   * ---------------------------------------------------------------------- */

  bool checkIgnoredType(Node *n, String *rust_name, SwigType *type) {
    if (hasRustTypemap(n, type)) {
      return true;
    }

    SwigType *t = SwigType_typedef_resolve_all(type);

    bool ret = true;
    bool is_conflict = false;
    Node *e = Language::enumLookup(t);
    if (e) {
      if (GetFlag(e, "rust:conflict")) {
        is_conflict = true;
      }
    } else if (SwigType_issimple(t)) {
      Node *cn = classLookup(t);
      if (cn) {
        if (GetFlag(cn, "rust:conflict")) {
          is_conflict = true;
        }
      }
    } else if (SwigType_ispointer(t) || SwigType_isarray(t) ||
               SwigType_isqualifier(t) || SwigType_isreference(t)) {
      SwigType *r = Copy(t);
      if (SwigType_ispointer(r)) {
        SwigType_del_pointer(r);
      } else if (SwigType_isarray(r)) {
        SwigType_del_array(r);
      } else if (SwigType_isqualifier(r)) {
        SwigType_del_qualifier(r);
      } else {
        SwigType_del_reference(r);
      }

      if (!checkIgnoredType(n, rust_name, r)) {
        ret = false;
      }

      Delete(r);
    }

    if (is_conflict) {
      String *s = SwigType_str(t, NULL);
      Swig_warning(WARN_RUST_NAME_CONFLICT, input_file, line_number,
                   "Ignoring '%s' (Rust name '%s') due to Rust name conflict "
                   "for parameter or result type '%s'\n",
                   Getattr(n, "name"), rust_name, s);
      Delete(s);
      ret = false;
    }

    Delete(t);

    return ret;
  }

  /* ----------------------------------------------------------------------
   * rustType()
   *
   * Given a SWIG type, return a string for the type in Rust.
   * ---------------------------------------------------------------------- */

  String *rustType(Node *n, SwigType *type) {
    return rustTypeWithInfo(n, type, false, NULL);
  }

  /* ----------------------------------------------------------------------
   * rustImType()
   *
   * Given a SWIG type, return a string for the intermediate Rust type
   * to pass to C/C++.  This is like rustType except that it looks for
   * an imtype typemap entry first.
   * ---------------------------------------------------------------------- */

  String *rustImType(Node *n, SwigType *type) {
    return rustTypeWithInfo(n, type, true, NULL);
  }

  /* ----------------------------------------------------------------------
   * rustTypeWithInfo()
   *
   * Like rustType, but return some more information.
   *
   * If use_imtype is true, this look for a imtype typemap entry.
   *
   * If the p_is_interface parameter is not NULL, this sets
   * *p_is_interface to indicate whether this type is going to be
   * represented by a Rust interface type.  These are cases where the Rust
   * code needs to make some adjustments when passing values back and
   * forth with C/C++.
   * ---------------------------------------------------------------------- */

  String *rustTypeWithInfo(Node *n, SwigType *type, bool use_imtype,
                           bool *p_is_interface) {
    if (p_is_interface) {
      *p_is_interface = false;
    }

    String *ret = NULL;
    if (use_imtype) {
      if (n && Cmp(type, Getattr(n, "type")) == 0) {
        if (Strcmp(Getattr(n, "nodeType"), "parm") == 0) {
          ret = Getattr(n, "tmap:imtype");
        }
        if (!ret) {
          ret = Swig_typemap_lookup("imtype", n, "", NULL);
        }
      } else {
        Parm *p = NewParm(type, "rustImType", n);
        ret = Swig_typemap_lookup("imtype", p, "", NULL);
        Delete(p);
      }
    }
    if (!ret) {
      if (n && Cmp(type, Getattr(n, "type")) == 0) {
        if (Strcmp(Getattr(n, "nodeType"), "parm") == 0) {
          ret = Getattr(n, "tmap:rusttype");
        }
        if (!ret) {
          ret = Swig_typemap_lookup("rusttype", n, "", NULL);
        }
      } else {
        Parm *p = NewParm(type, "rustType", n);
        ret = Swig_typemap_lookup("rusttype", p, "", NULL);
        Delete(p);
      }
    }

    if (ret && Strstr(ret, "$rusttypename") != 0) {
      ret = NULL;
    }

    if (ret) {
      return Copy(ret);
    }

    SwigType *t = SwigType_typedef_resolve_all(type);

    if (SwigType_isenum(t)) {
      Node *e = Language::enumLookup(t);
      if (e) {
        ret = rustEnumName(e);
      } else if (Strcmp(t, "enum ") == 0) {
        ret = NewString("int");
      } else {
        // An unknown enum - one that has not been parsed (neither a C enum
        // forward reference nor a definition) or an ignored enum
        String *tt = Copy(t);
        Replace(tt, "enum ", "", DOH_REPLACE_ANY);
        ret = exportedName(tt);
        Setattr(undefined_enum_types, t, ret);
        Delete(tt);
      }
    } else if (SwigType_isfunctionpointer(t) || SwigType_isfunction(t)) {
      ret = NewString("_swig_fnptr");
    } else if (SwigType_ismemberpointer(t)) {
      ret = NewString("_swig_memberptr");
    } else if (SwigType_issimple(t)) {
      Node *cn = classLookup(t);
      if (cn) {
        ret = Getattr(cn, "sym:name");
        if (!ret) {
          ret = Getattr(cn, "name");
        }
        ret = exportedName(ret);

        Node *cnmod = Getattr(cn, "module");
        if (!cnmod || Strcmp(Getattr(cnmod, "name"), module) == 0) {
          Setattr(undefined_types, t, t);
        } else {
          String *nw = NewString("");
          Printv(nw, getModuleName(Getattr(cnmod, "name")), ".", ret, NULL);
          Delete(ret);
          ret = nw;
        }
      } else {
        // SWIG does not know about this type.
        ret = exportedName(t);
        Setattr(undefined_types, t, t);
      }
      if (p_is_interface) {
        *p_is_interface = true;
      }
    } else if (SwigType_ispointer(t) || SwigType_isarray(t)) {
      SwigType *r = Copy(t);
      if (SwigType_ispointer(r)) {
        // SwigType_del_pointer(r);
        return swig_c_ptr_to_rust_ptr(r);
      } else {
        SwigType_del_array(r);
      }

      if (SwigType_type(r) == T_VOID) {
        ret = NewString("*mut c_void");
      } else {
        bool is_interface;
        String *base = rustTypeWithInfo(n, r, false, &is_interface);

        // At the Rust level, an unknown or class type is handled as an
        // interface wrapping a pointer.  This means that if a
        // function returns the C type X, we will be wrapping the C
        // type X*.  In Rust we will call that type X.  That means that
        // if a C function expects X*, we can pass the Rust type X.  And
        // that means that when we see the C type X*, we should use
        // the Rust type X.

        // The is_interface variable tells us this.  However, it will
        // be true both for the case of X and for the case of X*.  If
        // r is a pointer here, then we are looking at X**.  There is
        // really no rustod way for us to handle that.
        bool is_pointer_to_pointer = false;
        if (is_interface) {
          SwigType *c = Copy(r);
          if (SwigType_isqualifier(c)) {
            SwigType_del_qualifier(c);
            if (SwigType_ispointer(c) || SwigType_isarray(c)) {
              is_pointer_to_pointer = true;
            }
          }
          Delete(c);
        }

        if (is_interface) {
          if (!is_pointer_to_pointer) {
            ret = base;
            if (p_is_interface) {
              *p_is_interface = true;
            }
          } else {
            ret = NewString("uintptr");
          }
        } else {
          ret = NewString("*");
          Append(ret, base);
          Delete(base);
        }
      }

      Delete(r);
    } else if (SwigType_isreference(t)) {
      SwigType *r = Copy(t);
      SwigType_del_reference(r);

      // If this is a const reference, and we are looking at a pointer
      // to it, then we just use the pointer we already have.
      bool add_pointer = true;
      if (SwigType_isqualifier(r)) {
        String *q = SwigType_parm(r);
        if (Strcmp(q, "const") == 0) {
          SwigType *c = Copy(r);
          SwigType_del_qualifier(c);
          if (SwigType_ispointer(c)) {
            add_pointer = false;
          }
          Delete(c);
        }
      }
      if (add_pointer) {
        SwigType_add_pointer(r);
      }
      ret = rustTypeWithInfo(n, r, false, p_is_interface);
      Delete(r);
    } else if (SwigType_isqualifier(t)) {
      SwigType *r = Copy(t);
      SwigType_del_qualifier(r);
      ret = rustTypeWithInfo(n, r, false, p_is_interface);
      Delete(r);
    } else if (SwigType_isvarargs(t)) {
      ret = NewString("[]interface{}");
    }

    Delete(t);

    if (!ret) {
      Swig_warning(WARN_LANG_NATIVE_UNIMPL, input_file, line_number,
                   "No Rust typemap defined for %s\n", SwigType_str(type, 0));
      ret = NewString("uintptr");
    }

    return ret;
  }


  String *rustTypeForCppValue(Node *n, SwigType *type, bool *c_struct_type) {
    *c_struct_type = false;

    bool is_trait;
    String *rust_type = rustTypeWithInfo(n, type, true, &is_trait);
    if (is_trait) {
      Delete(rust_type);
      return NewString("*mut c_void");
    }
    if (Strcmp(rust_type, "usize") == 0) {
      Delete(rust_type);
      return NewString("usize");
    }
    if (((char *)Char(rust_type))[0] == '*') {
      // Treat all pointers as void*.  Using reference type in Rust
      // is too troublesome, and that lets us reduce
      // worrying about borrowing checking.
      return rust_type;
    } else if(rust_type != NULL) {
      return rust_type;
    }

    // Check for some Rust types that are really pointers under the covers.
    bool is_hidden_pointer = Strncmp(rust_type, "func(", 5) == 0 ||
                             Strncmp(rust_type, "map[", 4) == 0 ||
                             Strncmp(rust_type, "chan ", 5) == 0;

    Delete(rust_type);

    String *ct = Getattr(n, "emit:crusttype");
    if (ct) {
      *c_struct_type = Getattr(n, "emit:crusttypestruct") ? true : false;
      return Copy(ct);
    }

    String *t = Copy(type);
    if (SwigType_isarray(t) && Getattr(n, "tmap:rusttype") == NULL) {
      SwigType_del_array(t);
      SwigType_add_pointer(t);
    }

    bool add_typedef = true;

    static int count;
    ++count;
    ct = NewStringf("swig_type_%d", count);

    String *gct = gcCTypeForRustValue(n, t, ct);
    Delete(t);

    if (Strncmp(gct, "_ruststring_", 10) == 0 ||
        Strncmp(gct, "_rustslice_", 9) == 0) {
      *c_struct_type = true;
      Setattr(n, "emit:crusttypestruct", type);
    } else {
      char *p = Strstr(gct, ct);
      if (p != NULL && p > (char *)Char(gct) && p[-1] == '*' &&
          p[Len(ct)] == '\0') {
        // Treat all pointers as void*.  See above.
        Delete(ct);
        --count;
        ct = NewString("*mut c_void");
        add_typedef = false;
        if (is_hidden_pointer) {
          // A Rust type that is really a pointer, like func, map, chan,
          // is being represented in C by a pointer.  This is fine,
          // but we have to memcpy the type rather than simply
          // converting it.
          *c_struct_type = true;
          Setattr(n, "emit:crusttypestruct", type);
        }
      }

      if (Strncmp(gct, "bool ", 5) == 0) {
        // Change the C++ type bool to the C type _Bool.
        Replace(gct, "bool", "_Bool", DOH_REPLACE_FIRST);
      }
      if (Strncmp(gct, "intrust ", 6) == 0) {
        // We #define intrust to swig_intrust for the crust comment.
        Replace(gct, "intrust", "swig_intrust", DOH_REPLACE_FIRST);
      }
      p = Strstr(gct, ct);
      if (p != NULL && p > (char *)Char(gct) && p[-1] == ' ' &&
          p[Len(ct)] == '\0') {
        String *q = NewStringWithSize(gct, Len(gct) - Len(ct) - 1);
        if (validIdentifier(q)) {
          // This is a simple type name, and we can use it directly.
          Delete(ct);
          --count;
          ct = q;
          add_typedef = false;
        }
      }
    }
    if (add_typedef) {
      Printv(f_crust_comment_typedefs, "typedef ", gct, ";\n", NULL);
    }

    Setattr(n, "emit:crusttype", ct);

    Delete(gct);

    return Copy(ct);
  }

  /* ----------------------------------------------------------------------
   * crustTypeForRustValue()
   *
   * Given a SWIG type, return a string for the C type to use for the
   * crust wrapper code.  This always returns a simple identifier, since
   * it is used in Rust code as C.name.
   *
   * This sets *c_struct_type if the C type uses a struct where the Rust
   * type uses a simple type.  This is true for strings and slices.
   * When this is true the Rust code has to jump through unsafe hoops to
   * pass the type checker.
   * ---------------------------------------------------------------------- */

  String *crustTypeForRustValue(Node *n, SwigType *type, bool *c_struct_type) {
    *c_struct_type = false;

    bool is_interface;
    String *rust_type = rustTypeWithInfo(n, type, true, &is_interface);
    if (is_interface) {
      Delete(rust_type);
      return NewString("uintptr_t");
    }
    if (Strcmp(rust_type, "uintptr") == 0) {
      Delete(rust_type);
      return NewString("uintptr_t");
    }
    if (((char *)Char(rust_type))[0] == '*') {
      // Treat all pointers as void*.  There is no meaningful type
      // checking going on here anyhow, and that lets us avoid
      // worrying about defining the base type of the pointer.
      Delete(rust_type);
      return NewString("swig_voidp");
    }

    // Check for some Rust types that are really pointers under the covers.
    bool is_hidden_pointer = Strncmp(rust_type, "func(", 5) == 0 ||
                             Strncmp(rust_type, "map[", 4) == 0 ||
                             Strncmp(rust_type, "chan ", 5) == 0;

    Delete(rust_type);

    String *ct = Getattr(n, "emit:crusttype");
    if (ct) {
      *c_struct_type = Getattr(n, "emit:crusttypestruct") ? true : false;
      return Copy(ct);
    }

    String *t = Copy(type);
    if (SwigType_isarray(t) && Getattr(n, "tmap:rusttype") == NULL) {
      SwigType_del_array(t);
      SwigType_add_pointer(t);
    }

    bool add_typedef = true;

    static int count;
    ++count;
    ct = NewStringf("swig_type_%d", count);

    String *gct = gcCTypeForRustValue(n, t, ct);
    Delete(t);

    if (Strncmp(gct, "_ruststring_", 10) == 0 ||
        Strncmp(gct, "_rustslice_", 9) == 0) {
      *c_struct_type = true;
      Setattr(n, "emit:crusttypestruct", type);
    } else {
      char *p = Strstr(gct, ct);
      if (p != NULL && p > (char *)Char(gct) && p[-1] == '*' &&
          p[Len(ct)] == '\0') {
        // Treat all pointers as void*.  See above.
        Delete(ct);
        --count;
        ct = NewString("swig_voidp");
        add_typedef = false;
        if (is_hidden_pointer) {
          // A Rust type that is really a pointer, like func, map, chan,
          // is being represented in C by a pointer.  This is fine,
          // but we have to memcpy the type rather than simply
          // converting it.
          *c_struct_type = true;
          Setattr(n, "emit:crusttypestruct", type);
        }
      }

      if (Strncmp(gct, "bool ", 5) == 0) {
        // Change the C++ type bool to the C type _Bool.
        Replace(gct, "bool", "_Bool", DOH_REPLACE_FIRST);
      }
      if (Strncmp(gct, "intrust ", 6) == 0) {
        // We #define intrust to swig_intrust for the crust comment.
        Replace(gct, "intrust", "swig_intrust", DOH_REPLACE_FIRST);
      }
      p = Strstr(gct, ct);
      if (p != NULL && p > (char *)Char(gct) && p[-1] == ' ' &&
          p[Len(ct)] == '\0') {
        String *q = NewStringWithSize(gct, Len(gct) - Len(ct) - 1);
        if (validIdentifier(q)) {
          // This is a simple type name, and we can use it directly.
          Delete(ct);
          --count;
          ct = q;
          add_typedef = false;
        }
      }
    }
    if (add_typedef) {
      Printv(f_crust_comment_typedefs, "typedef ", gct, ";\n", NULL);
    }

    Setattr(n, "emit:crusttype", ct);

    Delete(gct);

    return Copy(ct);
  }

  /* ----------------------------------------------------------------------
   * rustWrapperType()
   *
   * Given a type, return a string for the type to use for the wrapped
   * Rust function.  This function exists because for a C++ class we
   * need to convert interface and reference types.
   * ---------------------------------------------------------------------- */

  String *rustWrapperType(Node *n, SwigType *type, bool is_result) {
    bool is_interface;
    String *ret = rustTypeWithInfo(n, type, true, &is_interface);

    // If this is an interface, we want to pass the real type.
    if (is_interface) {
      Delete(ret);
      if (!is_result) {
        ret = NewString("uintptr");
      } else {
        SwigType *ty = SwigType_typedef_resolve_all(type);
        while (true) {
          if (SwigType_ispointer(ty)) {
            SwigType_del_pointer(ty);
          } else if (SwigType_isarray(ty)) {
            SwigType_del_array(ty);
          } else if (SwigType_isreference(ty)) {
            SwigType_del_reference(ty);
          } else if (SwigType_isqualifier(ty)) {
            SwigType_del_qualifier(ty);
          } else {
            break;
          }
        }
        assert(SwigType_issimple(ty));
        String *p = rustCPointerType(ty, true);
        Delete(ty);
        ret = p;
      }
    }

    return ret;
  }

  /* ----------------------------------------------------------------------
   * rustOverloadType()
   *
   * Given a type, return the Rust type to use when dispatching of
   * overloaded functions.  This is normally just the usual Rust type.
   * However, for a C++ class, the usual Rust type is an interface type.
   * And if that interface type represents a C++ type that SWIG does
   * not know about, then the interface type generated for any C++
   * class will match that interface.  So for that case, we match on
   * the underlying integer type.
   *
   * It has to work this way so that we can handle a derived type of a
   * %ignore'd type.  It's unlikely that anybody will have a value of
   * an undefined type, but we support it because it worked in the
   * past.
   * ---------------------------------------------------------------------- */

  String *rustOverloadType(Node *n, SwigType *type) {
    SwigType *ty = SwigType_typedef_resolve_all(type);
    while (true) {
      if (SwigType_ispointer(ty)) {
        SwigType_del_pointer(ty);
      } else if (SwigType_isarray(ty)) {
        SwigType_del_array(ty);
      } else if (SwigType_isreference(ty)) {
        SwigType_del_reference(ty);
      } else if (SwigType_isqualifier(ty)) {
        SwigType_del_qualifier(ty);
      } else {
        break;
      }
    }

    String *rust_type = rustType(n, ty);

    if (Getattr(undefined_types, ty) && !Getattr(defined_types, rust_type)) {
      Delete(rust_type);
      return rustWrapperType(n, type, true);
    }

    Delete(rust_type);
    return rustType(n, type);
  }

  /* ----------------------------------------------------------------------
   * rustCPointerType()
   *
   * Return the name of the Rust type to use for the C pointer value.
   * The regular C type is the name of an interface type which wraps a
   * pointer whose name is returned by this function.
   * ---------------------------------------------------------------------- */

  String *rustCPointerType(SwigType *type, bool add_to_hash) {
    SwigType *ty = SwigType_typedef_resolve_all(type);
    Node *cn = classLookup(ty);
    String *ex;
    String *ret;
    if (!cn) {
      if (add_to_hash) {
        Setattr(undefined_types, ty, ty);
      }
      ret = NewString("Swigcptr");
      ex = exportedName(ty);
      Append(ret, ex);
    } else {
      String *cname = Getattr(cn, "sym:name");
      if (!cname) {
        cname = Getattr(cn, "name");
      }
      ex = exportedName(cname);
      Node *cnmod = Getattr(cn, "module");
      if (!cnmod || Strcmp(Getattr(cnmod, "name"), module) == 0) {
        if (add_to_hash) {
          Setattr(undefined_types, ty, ty);
        }
        ret = NewString(ex);
        // Append(ret, ex);
        // ret = NewString("Swigcptr");
      } else {
        ret = NewString("");
        Printv(ret, getModuleName(Getattr(cnmod, "name")), ".Swigcptr", ex,
               NULL);
      }
    }
    Delete(ty);
    Delete(ex);
    return ret;
  }

  /* ----------------------------------------------------------------------
   * gcCTypeForRustValue()
   *
   * Given a type, return the C/C++ type which will be used to catch
   * the value in Rust.  This is the gc version.
   * ---------------------------------------------------------------------- */

  String *gcCTypeForRustValue(Node *n, SwigType *type, String *name) {
    bool is_interface;
    String *gt = rustTypeWithInfo(n, type, true, &is_interface);

    String *tail = NewString("");
    SwigType *t = SwigType_typedef_resolve_all(type);
    bool is_const_ref = false;
    if (SwigType_isreference(t)) {
      SwigType *tt = Copy(t);
      SwigType_del_reference(tt);
      if (SwigType_isqualifier(tt)) {
        String *q = SwigType_parm(tt);
        if (Strcmp(q, "const") == 0) {
          is_const_ref = true;
        }
      }
      Delete(tt);
    }
    if (!is_const_ref) {
      while (Strncmp(gt, "*", 1) == 0) {
        Replace(gt, "*", "", DOH_REPLACE_FIRST);
        Printv(tail, "*", NULL);
      }
    }
    Delete(t);

    bool is_string = Strcmp(gt, "String") == 0;
    bool is_slice = Strncmp(gt, "[]", 2) == 0;
    bool is_function = Strcmp(gt, "_swig_fnptr") == 0;
    bool is_member = Strcmp(gt, "_swig_memberptr") == 0;
    bool is_complex64 = Strcmp(gt, "complex64") == 0;
    bool is_complex128 = Strcmp(gt, "complex128") == 0;
    bool is_bool = false;
    bool is_int8 = false;
    bool is_int16 = false;
    bool is_int = Strcmp(gt, "i32") == 0 || Strcmp(gt, "u32") == 0;
    bool is_int32 = false;
    bool is_int64 = false;
    bool is_float32 = false;
    bool is_float64 = false;

    bool has_typemap = (n != NULL && Getattr(n, "tmap:rusttype") != NULL) ||
                       hasRustTypemap(n, type);
    if (has_typemap) {
      is_bool = Strcmp(gt, "bool") == 0;
      is_int8 = Strcmp(gt, "i8") == 0 || Strcmp(gt, "u8") == 0 ||
                Strcmp(gt, "byte") == 0;
      is_int16 = Strcmp(gt, "i16") == 0 || Strcmp(gt, "u16") == 0;
      is_int32 = Strcmp(gt, "i32") == 0 || Strcmp(gt, "u32") == 0;
      is_int64 = Strcmp(gt, "i64") == 0 || Strcmp(gt, "u64") == 0;
      is_float32 = Strcmp(gt, "f32") == 0;
      is_float64 = Strcmp(gt, "f64") == 0;
    }
    Delete(gt);

    String *ret;
    if (is_string) {
      // Note that we don't turn a reference to a string into a
      // pointer to a string.  Strings are immutable anyhow.
      ret = NewString("");
      Printv(ret, "_ruststring_", tail, " ", name, NULL);
      Delete(tail);
      return ret;
    } else if (is_slice) {
      // Slices are always passed as a _rustslice_, whether or not references
      // are involved.
      ret = NewString("");
      Printv(ret, "_rustslice_", tail, " ", name, NULL);
      Delete(tail);
      return ret;
    } else if (is_function || is_member) {
      ret = NewString("");
      Printv(ret, "void*", tail, " ", name, NULL);
      Delete(tail);
      return ret;
    } else if (is_complex64) {
      ret = NewString("_Complex float ");
    } else if (is_complex128) {
      ret = NewString("_Complex double ");
    } else if (is_interface) {
      SwigType *t = SwigType_typedef_resolve_all(type);
      if (SwigType_ispointer(t)) {
        SwigType_del_pointer(t);
      }
      if (SwigType_isreference(t)) {
        SwigType_del_reference(t);
      }
      SwigType_add_pointer(t);
      ret = SwigType_lstr(t, name);
      Delete(t);
      Delete(tail);
      return ret;
    } else {
      SwigType *t = SwigType_typedef_resolve_all(type);
      if (!has_typemap && SwigType_isreference(t)) {
        // A const reference to a known type, or to a pointer, is not
        // mapped to a pointer.
        SwigType_del_reference(t);
        if (SwigType_isqualifier(t)) {
          String *q = SwigType_parm(t);
          if (Strcmp(q, "const") == 0) {
            SwigType_del_qualifier(t);
            if (hasRustTypemap(n, t) || SwigType_ispointer(t)) {
              if (is_int) {
                ret = NewString("intrust ");
                Append(ret, name);
              } else if (is_int64) {
                ret = NewString("long long ");
                Append(ret, name);
              } else {
                ret = SwigType_lstr(t, name);
              }
              Delete(q);
              Delete(t);
              Delete(tail);
              return ret;
            }
          }
          Delete(q);
        }
      }

      if (Language::enumLookup(t) != NULL) {
        is_int = true;
      } else {
        SwigType *tstripped = SwigType_strip_qualifiers(t);
        if (SwigType_isenum(tstripped))
          is_int = true;
        Delete(tstripped);
      }

      Delete(t);
      if (is_bool) {
        ret = NewString("bool ");
      } else if (is_int8) {
        ret = NewString("char ");
      } else if (is_int16) {
        ret = NewString("short ");
      } else if (is_int) {
        ret = NewString("intrust ");
      } else if (is_int32) {
        ret = NewString("int ");
      } else if (is_int64) {
        ret = NewString("long long ");
      } else if (is_float32) {
        ret = NewString("float ");
      } else if (is_float64) {
        ret = NewString("double ");
      } else {
        Delete(tail);
        return SwigType_lstr(type, name);
      }
    }

    Append(ret, tail);
    if (!has_typemap && SwigType_isreference(type)) {
      Append(ret, "* ");
    }
    Append(ret, name);
    Delete(tail);
    return ret;
  }

  /* ----------------------------------------------------------------------
   * rustTypeIsTrait
   *
   * Return whether this C++ type is represented as an interface type
   * in Rust.  These types require adjustments in the Rust code when
   * passing them back and forth between Rust and C++.
   * ---------------------------------------------------------------------- */

  bool rustTypeIsTrait(Node *n, SwigType *type) {
    bool is_interface;
    Delete(rustTypeWithInfo(n, type, false, &is_interface));
    return is_interface;
  }

  /* ----------------------------------------------------------------------
   * hasRustTypemap
   *
   * Return whether a type has a "rusttype" typemap entry.
   * ---------------------------------------------------------------------- */

  bool hasRustTypemap(Node *n, SwigType *type) {
    Parm *p = NewParm(type, "test", n);
    SwigType *tm = Swig_typemap_lookup("rusttype", p, "", NULL);
    Delete(p);
    if (tm && Strstr(tm, "$rusttypename") == 0) {
      Delete(tm);
      return true;
    }
    Delete(tm);
    return false;
  }

  /* ----------------------------------------------------------------------
   * rustEnumName()
   *
   * Given an enum node, return a string to use for the enum type in Rust.
   * ---------------------------------------------------------------------- */

  String *rustEnumName(Node *n) {
    String *ret = Getattr(n, "rust:enumname");
    if (ret) {
      return Copy(ret);
    }

    if (Equal(Getattr(n, "type"), "enum ")) {
      return NewString("int");
    }

    SwigType *type = Getattr(n, "enumtype");
    assert(type);
    char *p = Char(type);
    int len = Len(type);
    SwigType *s = NewString("");
    bool capitalize = true;
    for (int i = 0; i < len; ++i, ++p) {
      if (*p == ':') {
        ++i;
        ++p;
        assert(*p == ':');
        capitalize = true;
      } else if (capitalize) {
        Putc(toupper(*p), s);
        capitalize = false;
      } else {
        Putc(*p, s);
      }
    }

    ret = Swig_name_mangle_type(s);
    Delete(s);
    return ret;
  }

  /* ----------------------------------------------------------------------
   * getParm()
   *
   * Get the real parameter to use.
   * ---------------------------------------------------------------------- */

  Parm *getParm(Parm *p) {
    while (p && checkAttribute(p, "tmap:in:numinputs", "0")) {
      p = Getattr(p, "tmap:in:next");
    }
    return p;
  }

  /* ----------------------------------------------------------------------
   * nextParm()
   *
   * Return the next parameter.
   * ---------------------------------------------------------------------- */

  Parm *nextParm(Parm *p) {
    if (!p) {
      return NULL;
    } else if (Getattr(p, "tmap:in")) {
      return Getattr(p, "tmap:in:next");
    } else {
      return nextSibling(p);
    }
  }

  bool isConst(Node *n) {
    String * qualifier = Getattr(n, "qualifier");
    return qualifier && isStringConst(qualifier);
  }

  inline bool isStringConst(String * qualifier) {
    return Strstr(qualifier,"q(const)");
  }

  /* ----------------------------------------------------------------------
   * isStatic
   *
   * Return whether a node should be considered as static rather than
   * as a member.
   * ---------------------------------------------------------------------- */

  bool isStatic(Node *n) {
    String *storage = Getattr(n, "storage");
    return (storage &&
            (Swig_storage_isstatic(n) || Strstr(storage, "friend")) &&
            (!SmartPointer || !Getattr(n, "allocate:smartpointeraccess")));
  }

  /* ----------------------------------------------------------------------
   * isFriend
   *
   * Return whether a node is a friend.
   * ---------------------------------------------------------------------- */

  bool isFriend(Node *n) {
    String *storage = Getattr(n, "storage");
    return storage && Strstr(storage, "friend");
  }

  /* ----------------------------------------------------------------------
   * rustGetattr
   *
   * Fetch an attribute from a node but return NULL if it is the empty string.
   * ---------------------------------------------------------------------- */
  Node *rustGetattr(Node *n, const char *name) {
    Node *ret = Getattr(n, name);
    if (ret != NULL && Len(ret) == 0) {
      ret = NULL;
    }
    return ret;
  }

  /* ----------------------------------------------------------------------
   * rustTypemapLookup
   *
   * Look up a typemap but return NULL if it is the empty string.
   * ---------------------------------------------------------------------- */
  String *rustTypemapLookup(const char *name, Node *node, const char *lname) {
    String *ret = Swig_typemap_lookup(name, node, lname, NULL);
    if (ret != NULL && Len(ret) == 0) {
      ret = NULL;
    }
    return ret;
  }

  /* ----------------------------------------------------------------------
   * getModuleName
   *
   * Return the name of a module. This is different from module path:
   * "some/path/to/module" -> "module".
   * ---------------------------------------------------------------------- */

  String *getModuleName(String *module_path) {
    char *suffix = strrchr(Char(module_path), '/');
    if (suffix == NULL) {
      return module_path;
    }
    return Str(suffix + 1);
  }

  /* -----------------------------------------------------------------------------
  * swig_c_ptr_to_rust_ptr()
  *
  * Given a C/C++ type, return the Rust type to use for a pointer to
  * that type.
  * ----------------------------------------------------------------------------- */
  inline String * swig_c_ptr_to_rust_ptr(SwigType *t) {
    SwigType *r = SwigType_typedef_resolve_all(t);
    String * ret = NewString("");
    while(SwigType_ispointer(r)) {
      if(SwigType_isconst(r)){
        Append(ret, "*const ");
      } else {
        Append(ret, "*mut ");
      }
      SwigType_del_pointer(r);
    }
    if(SwigType_type(r) == T_VOID){
      Append(ret, "c_void");
    } else {
      Append(ret, rustType(NULL, r));
    }

    // if (SwigType_isreference(r)) {
    //   SwigType_del_reference(r);
    // }
    return ret;
  }

}; /* class RUST */



/* -----------------------------------------------------------------------------
 * swig_rust()    - Instantiate module
 * -----------------------------------------------------------------------------
 */

static Language *new_swig_rust() { return new RUST(); }
extern "C" Language *swig_rust(void) { return new_swig_rust(); }

/* -----------------------------------------------------------------------------
 * Static member variables
 * -----------------------------------------------------------------------------
 */

// Usage message.
const char *const RUST::usage = "\
Rust Options (available with -rust)\n\
     -crust                - Generate crust input files\n\
     -no-crust             - Do not generate crust input files\n\
     -gccrust              - Generate code for gccrust rather than gc\n\
     -rust-pkgpath <p>     - Like gccrust -frust-pkgpath option\n\
     -rust-prefix <p>      - Like gccrust -frust-prefix option\n\
     -import-prefix <p>  - Prefix to add to %import directives\n\
     -intrustsize <s>      - Set size of Rust int type--32 or 64 bits\n\
     -package <name>     - Set name of the Rust package to <name>\n\
     -use-shlib          - Force use of a shared library\n\
     -soname <name>      - Set shared library holding C/C++ code to <name>\n\
\n";
