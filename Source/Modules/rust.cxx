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
 * ----------------------------------------------------------------------------- */

#include "swigmod.h"
#include "cparse.h"
#include <ctype.h>
#include <cstdio>

static const char *usage = "\
Rust Options (available with -rust)\n\
     -crate-name <name> - Set the crate name (default: module name)\n\
     -module <name>      - Set the module name\n\
     -namespace <name>   - Generate wrappers into Rust mod <name>\n\
     -safe-wrapper       - Generate safe wrapper layer (default: on)\n\
     -ffi-only           - Only generate FFI declarations\n\
     -no-directors       - Disable director support\n\
";

class RUST : public Language {
public:
  /* ----------------------------------------------------------------------------- 
   * RUST()
   * ----------------------------------------------------------------------------- */
  RUST() :
    f_begin(NULL),
    f_runtime(NULL),
    f_runtime_h(NULL),
    f_header(NULL),
    f_wrappers(NULL),
    f_init(NULL),
    f_directors(NULL),
    f_directors_h(NULL),
    f_ffi_begin(NULL),
    f_ffi_imports(NULL),
    f_ffi_code(NULL),
    f_wrapper_begin(NULL),
    f_wrapper_code(NULL),
    crate_name(NULL),
    module_name(NULL),
    namespce(NULL),
    current_nspace(NULL),
    safe_wrapper_flag(true),
    ffi_only_flag(false),
    directors_flag(true),
    proxy_flag(true),
    static_flag(false),
    variable_wrapper_flag(false),
    class_name(NULL),
    class_node(NULL),
    proxy_class_def(NULL),
    proxy_class_code(NULL),
    proxy_class_constants_code(NULL),
    module_class_code(NULL),
    module_class_constants_code(NULL),
    baseclass(NULL),
    derived_flag(false),
    upcasts_code(NULL),
    dmethods_seq(NULL),
    dmethods_table(NULL),
    n_dmethods(0),
    n_directors(0),
    first_class_dmethod(0),
    curr_class_dmethod(0),
    swig_types_hash(NULL),
    filenames_list(NULL) {
    /* For now, multiple inheritance in directors is disabled.
       This should be easy to implement though. */
    director_multiple_inheritance = 0;
    directorLanguage();
  }

  /* ----------------------------------------------------------------------------- 
   * ~RUST()
   * ----------------------------------------------------------------------------- */
  ~RUST() {
  }

  /* ----------------------------------------------------------------------------- 
   * main()
   * 
   * Process command line options.
   * ----------------------------------------------------------------------------- */
  virtual void main(int argc, char *argv[]) {
    SWIG_library_directory("rust");

    for (int i = 1; i < argc; i++) {
      if (argv[i]) {
        if (strcmp(argv[i], "-crate-name") == 0) {
          if (argv[i + 1]) {
            crate_name = NewString(argv[i + 1]);
            Swig_mark_arg(i);
            Swig_mark_arg(i + 1);
            i++;
          } else {
            Swig_arg_error();
          }
        } else if (strcmp(argv[i], "-module") == 0) {
          if (argv[i + 1]) {
            module_name = NewString(argv[i + 1]);
            Swig_mark_arg(i);
            Swig_mark_arg(i + 1);
            i++;
          } else {
            Swig_arg_error();
          }
        } else if (strcmp(argv[i], "-safe-wrapper") == 0) {
          Swig_mark_arg(i);
          safe_wrapper_flag = true;
        } else if (strcmp(argv[i], "-ffi-only") == 0) {
          Swig_mark_arg(i);
          ffi_only_flag = true;
          safe_wrapper_flag = false;
        } else if (strcmp(argv[i], "-no-directors") == 0) {
          Swig_mark_arg(i);
          directors_flag = false;
        } else if (strcmp(argv[i], "-noproxy") == 0) {
          Swig_mark_arg(i);
          proxy_flag = false;
        } else if (strcmp(argv[i], "-namespace") == 0) {
          if (argv[i + 1]) {
            namespce = NewString(argv[i + 1]);
            if (Len(namespce) == 0) {
              Delete(namespce);
              namespce = NULL;
            }
            Swig_mark_arg(i);
            Swig_mark_arg(i + 1);
            i++;
          } else {
            Swig_arg_error();
          }
        } else if (strcmp(argv[i], "-help") == 0) {
          Printf(stdout, "%s", usage);
        }
      }
    }

    // Add preprocessor symbol to parser
    Preprocessor_define("SWIGRUST 1", 0);

    SWIG_config_file("rust.swg");

    allow_overloading();
  }

  /* ----------------------------------------------------------------------------- 
   * top()
   * 
   * Initialize output files and process the parse tree.
   * ----------------------------------------------------------------------------- */
  virtual int top(Node *n) {
    // Get module options
    Node *module = Getattr(n, "module");
    Node *optionsnode = Getattr(module, "options");

    if (optionsnode) {
      if (Getattr(optionsnode, "directors")) {
        allow_directors();
      }
      if (Getattr(optionsnode, "dirprot")) {
        allow_dirprot();
      }
      allow_allprotected(GetFlag(optionsnode, "allprotected"));
    }

    // Initialize output files
    String *outfile = Getattr(n, "outfile");
    String *outfile_h = Getattr(n, "outfile_h");

    if (!outfile) {
      Printf(stderr, "Unable to determine outfile\n");
      Exit(EXIT_FAILURE);
    }

    f_begin = NewFile(outfile, "w", SWIG_output_files());
    if (!f_begin) {
      FileErrorDisplay(outfile);
      Exit(EXIT_FAILURE);
    }

    if (Swig_directors_enabled()) {
      if (!outfile_h) {
        Printf(stderr, "Unable to determine outfile_h\n");
        Exit(EXIT_FAILURE);
      }
      f_runtime_h = NewFile(outfile_h, "w", SWIG_output_files());
      if (!f_runtime_h) {
        FileErrorDisplay(outfile_h);
        Exit(EXIT_FAILURE);
      }
    }

    // Initialize output sections
    f_runtime = NewString("");
    f_init = NewString("");
    f_header = NewString("");
    f_wrappers = NewString("");
    f_directors_h = NewString("");
    f_directors = NewString("");

    // Rust-specific output sections
    f_ffi_begin = NewString("");
    f_ffi_imports = NewString("");
    f_ffi_code = NewString("");
    f_wrapper_begin = NewString("");
    f_wrapper_code = NewString("");

    // Register file targets
    Swig_register_filebyname("header", f_header);
    Swig_register_filebyname("wrapper", f_wrappers);
    Swig_register_filebyname("begin", f_begin);
    Swig_register_filebyname("runtime", f_runtime);
    Swig_register_filebyname("init", f_init);
    Swig_register_filebyname("director", f_directors);
    Swig_register_filebyname("director_h", f_directors_h);

    swig_types_hash = NewHash();
    filenames_list = NewList();

    // Set module and crate names
    if (!module_name) {
      module_name = Copy(Getattr(n, "name"));
    }
    if (!crate_name) {
      crate_name = Copy(module_name);
    }

    // Initialize proxy class buffers
    proxy_class_def = NewString("");
    proxy_class_code = NewString("");
    proxy_class_constants_code = NewString("");
    module_class_code = NewString("");
    module_class_constants_code = NewString("");

    // Initialize director method tracking
    dmethods_seq = NewList();
    dmethods_table = NewHash();
    n_dmethods = 0;
    n_directors = 0;
    first_class_dmethod = 0;
    curr_class_dmethod = 0;

    // Write banner
    Swig_banner(f_begin);

    Swig_obligatory_macros(f_runtime, "RUST");

    // Enable directors if requested
    if (Swig_directors_enabled()) {
      Printf(f_runtime, "#define SWIG_DIRECTORS\n");

      Swig_banner(f_directors_h);
      Printf(f_directors_h, "\n");
      Printf(f_directors_h, "#ifndef SWIG_%s_WRAP_H_\n", module_name);
      Printf(f_directors_h, "#define SWIG_%s_WRAP_H_\n\n", module_name);

      Printf(f_directors, "\n\n");
      Printf(f_directors, "/* ---------------------------------------------------\n");
      Printf(f_directors, " * C++ director class methods\n");
      Printf(f_directors, " * --------------------------------------------------- */\n\n");
      if (outfile_h) {
        String *filename = Swig_file_filename(outfile_h);
        Printf(f_directors, "#include \"%s\"\n\n", filename);
        Delete(filename);
      }
    }

    Printf(f_runtime, "\n");

    // Register wrapper name format
    Swig_name_register("wrapper", "Rust_%f");

    // Start extern "C" block for C wrappers
    Printf(f_wrappers, "\n#ifdef __cplusplus\n");
    Printf(f_wrappers, "extern \"C\" {\n");
    Printf(f_wrappers, "#endif\n\n");

    // Process the parse tree
    Language::top(n);

    // Insert director runtime if enabled
    if (Swig_directors_enabled()) {
      Swig_insert_file("director_common.swg", f_runtime);
      Swig_insert_file("director.swg", f_runtime);
    }

    // Output upcast functions for multiple inheritance
    if (upcasts_code) {
      Printv(f_wrappers, upcasts_code, NIL);
    }

    // End extern "C" block
    Printf(f_wrappers, "#ifdef __cplusplus\n");
    Printf(f_wrappers, "}\n");
    Printf(f_wrappers, "#endif\n");

    // Emit type wrapper classes
    for (Iterator swig_type = First(swig_types_hash); swig_type.key; swig_type = Next(swig_type)) {
      emitTypeWrapperClass(swig_type.key, swig_type.item);
    }

    // Generate the Rust output file
    emitRustFile(n);

    // Cleanup
    Delete(swig_types_hash);
    swig_types_hash = NULL;
    Delete(filenames_list);
    filenames_list = NULL;

    // Close files
    Dump(f_runtime, f_begin);
    Dump(f_header, f_begin);

    if (Swig_directors_enabled()) {
      Dump(f_directors, f_begin);
      Dump(f_directors_h, f_runtime_h);

      Printf(f_runtime_h, "\n");
      Printf(f_runtime_h, "#endif\n");

      Delete(f_runtime_h);
      f_runtime_h = NULL;
      Delete(f_directors);
      f_directors = NULL;
      Delete(f_directors_h);
      f_directors_h = NULL;
    }

    Dump(f_wrappers, f_begin);
    Wrapper_pretty_print(f_init, f_begin);

    Delete(f_header);
    Delete(f_wrappers);
    Delete(f_init);
    Delete(f_runtime);
    Delete(f_begin);

    Delete(f_ffi_begin);
    Delete(f_ffi_imports);
    Delete(f_ffi_code);
    Delete(f_wrapper_begin);
    Delete(f_wrapper_code);

    return SWIG_OK;
  }

      /* ----------------------------------------------------------------------------- 

       * functionWrapper()

       * 

       * Generate wrapper code for a function.

       * ----------------------------------------------------------------------------- */

      virtual int functionWrapper(Node *n) {

        String *symname = Getattr(n, "sym:name");

        SwigType *returntype = Getattr(n, "type");

        ParmList *l = Getattr(n, "parms");

        String *tm;

        Parm *p;

        int i;

        String *c_return_type = NewString("");

        String *rust_return_type = NewString("");

        String *rust_ffi_return_type = NewString("");

        String *cleanup = NewString("");

        String *outarg = NewString("");

        int num_arguments = 0;

        bool is_void_return;

        String *overloaded_name = getOverloadedName(n);

    // Create a new wrapper function object
    Wrapper *f = NewWrapper();

    // Make a wrapper name for this function
    String *wname = Swig_name_wrapper(overloaded_name);
    
    /* Attach typemaps to parameters - custom typemaps */
    if (l) {
      Swig_typemap_attach_parms("cout", l, f);
      Swig_typemap_attach_parms("rusttype", l, f);
      Swig_typemap_attach_parms("rsffitype", l, f);
    }

    // Check if this is a constructor or destructor
    // Check both view attribute and nodeType for robustness
    String *view = Getattr(n, "view");
    String *nodeType = Getattr(n, "nodeType");
    bool is_constructor = (view && Cmp(view, "constructorhandler") == 0) ||
                          (nodeType && Cmp(nodeType, "constructor") == 0) ||
                          GetFlag(n, "handled_as_constructor");
    bool is_destructor = (view && Cmp(view, "destructorhandler") == 0) ||
                         (nodeType && Cmp(nodeType, "destructor") == 0);
    
    // Additional check: constructors/destructors based on sym:name pattern
    // Swig_name_construct generates "new_ClassName"
    // Swig_name_destroy generates "delete_ClassName"
    if (symname) {
      const char *symname_str = Char(symname);
      if (strncmp(symname_str, "new_", 4) == 0) {
        is_constructor = true;
      }
      if (strncmp(symname_str, "delete_", 7) == 0) {
        is_destructor = true;
      }
    }
    
    /* Get return types */
    if (is_constructor) {
      // Constructors return void* (pointer to new object)
      Printf(c_return_type, "void *");
      Printf(rust_return_type, "*mut c_void");
      Printf(rust_ffi_return_type, "*mut c_void");
    } else if ((tm = Swig_typemap_lookup("cout", n, "", 0))) {
      Printf(c_return_type, "%s", tm);
      // Get the Rust return type (user visible)
      if ((tm = Swig_typemap_lookup("rusttype", n, "", 0))) {
        // Check for special SWIGENUM marker - need to replace with actual enum name
        if (Cmp(tm, "SWIGENUM") == 0) {
          // Get the base type name without "enum" keyword
          String *enum_name = SwigType_base(returntype);
          // Check for anonymous enum (names starting with $)
          if (Strstr(enum_name, "$")) {
            Printf(rust_return_type, "i32");
          } else {
            Printf(rust_return_type, "%s", enum_name);
          }
          Delete(enum_name);
        } else {
          Printf(rust_return_type, "%s", tm);
        }
      } else {
        // No typemap found - try to generate a reasonable type
        if (SwigType_type(returntype) != T_VOID) {
          // Check if it's an enum type
          if (SwigType_isenum(returntype)) {
            String *enum_name = SwigType_base(returntype);
            if (Strstr(enum_name, "$")) {
              Printf(rust_return_type, "i32");
            } else {
              Printf(rust_return_type, "%s", enum_name);
            }
            Delete(enum_name);
          } else {
            Printf(rust_return_type, "*mut c_void");
          }
        }
      }
      // Get the Rust FFI return type
      if ((tm = Swig_typemap_lookup("rsffitype", n, "", 0))) {
        Printf(rust_ffi_return_type, "%s", tm);
      } else {
        // Default to c_int for enums, otherwise use rusttype
        if (SwigType_isenum(returntype)) {
          Printf(rust_ffi_return_type, "c_int");
        } else {
          Printf(rust_ffi_return_type, "%s", rust_return_type);
        }
      }
    } else {
      // Default: use the SWIG type
      String *tm1 = SwigType_str(returntype, 0);
      Printf(c_return_type, "%s", tm1);
      Delete(tm1);
      // Default: use opaque pointer for unknown types
      if (SwigType_type(returntype) != T_VOID) {
        Printf(rust_return_type, "*mut c_void");
        Printf(rust_ffi_return_type, "*mut c_void");
      }
    }

    // Check if return type is void
    // Constructors return pointer (not void), but destructors return void
    is_void_return = is_destructor || (!is_constructor && (SwigType_type(returntype) == T_VOID));

    // Start writing the wrapper function
    Printf(f->def, "SWIGEXPORT %s %s(", c_return_type, wname);

    // Add local variable for result
    if (!is_void_return) {
      Wrapper_add_localv(f, "jresult", c_return_type, "jresult = 0", NIL);
    }

    // Emit parameter variables (creates arg1, arg2, etc.)
    // Only process if we have parameters
    if (l) {
      emit_parameter_variables(l, f);

      // Attach standard parameter maps (handles "in", "typecheck", "argout", "check", "freearg")
      emit_attach_parmmaps(l, f);
    }

    // Parameter overloading info
    Setattr(n, "wrap:parms", l);
    Setattr(n, "wrap:name", wname);

    // Get number of arguments (after typemap processing)
    num_arguments = l ? emit_num_arguments(l) : 0;

    // Check if this is a member function
    bool is_member = GetFlag(n, "ismember");
    bool is_static = GetFlag(n, "static");

    // Build parameter list for C wrapper
    // Note: emit_parameter_variables already handled the self parameter for member functions
    int gencomma = 0;
    
    for (i = 0, p = l; i < num_arguments && p; i++) {
      // Skip parameters with numinputs=0
      while (p && checkAttribute(p, "tmap:in:numinputs", "0")) {
        p = Getattr(p, "tmap:in:next");
      }
      
      if (!p) break;  // Safety check

      SwigType *pt = Getattr(p, "type");
      String *ln = Getattr(p, "lname");
      String *arg = NewStringf("j%s", ln);  // Use j prefix for external arg name

      // Get the C type for this parameter
      String *c_param_type = NewString("");
      if ((tm = Getattr(p, "tmap:cout"))) {
        Printf(c_param_type, "%s", tm);
      } else {
        String *pt_str = SwigType_str(pt, 0);
        Printf(c_param_type, "%s", pt_str);
        Delete(pt_str);
      }

      // Add parameter to C function
      if (gencomma) {
        Printf(f->def, ", ");
      }
      Printf(f->def, "%s %s", c_param_type, arg);

      // Apply typemap if available
      if ((tm = Getattr(p, "tmap:in"))) {
        Replaceall(tm, "$input", arg);
        Setattr(p, "emit:input", arg);
        Printf(f->code, "%s\n", tm);
        p = Getattr(p, "tmap:in:next");
      } else {
        p = nextSibling(p);
      }

      Delete(arg);
      Delete(c_param_type);
      gencomma = 1;
    }

    // Close function definition
    Printf(f->def, ") {");

    // Emit the function call with return value handling
    if (!is_void_return) {
      // For constructors, manually declare result variable with correct type
      // Swig_ConstructorToFunction sets returntype as "ClassName *"
      if (is_constructor) {
        // Set this flag to prevent emit_return_variable from being called
        // (we handle the result variable manually)
        SetFlag(n, "tmap:out:optimal");
        
        // Use SwigType_str to get the correct type string for result variable
        String *result_type_str = SwigType_str(returntype, 0);
        Printf(f->code, "  %s %s = 0;\n", result_type_str, Swig_cresult_name());
        Delete(result_type_str);
      } else {
        // Normal case: use emit_return_variable
        emit_return_variable(n, returntype, f);
      }
      
      // Get action code
      String *actioncode = emit_action(n);
      
      // Use "out" typemap for return value
      if ((tm = Swig_typemap_lookup_out("out", n, Swig_cresult_name(), f, actioncode))) {
        Replaceall(tm, "$result", "jresult");
        if (GetFlag(n, "feature:new"))
          Replaceall(tm, "$owner", "1");
        else
          Replaceall(tm, "$owner", "0");
        Printf(f->code, "%s", tm);
        if (Len(tm))
          Printf(f->code, "\n");
      } else {
        Swig_warning(WARN_TYPEMAP_OUT_UNDEF, input_file, line_number, 
                     "Unable to use return type %s in function %s.\n", 
                     SwigType_str(returntype, 0), Getattr(n, "name"));
      }
    } else {
      // Void return - just emit action
      String *action = emit_action(n);
      Printv(f->code, action, NIL);
    }

    // Add return statement
    if (!is_void_return) {
      Printf(f->code, "  return jresult;\n");
    }

    Printf(f->code, "}\n");

    // Write the wrapper to output
    Wrapper_print(f, f_wrappers);

    // Generate Rust FFI declaration
    if (!ffi_only_flag || proxy_flag) {
      emitRustFFIDeclaration(n, wname, rust_ffi_return_type, is_void_return);
    }

    // Generate safe Rust wrapper
    if (safe_wrapper_flag && !ffi_only_flag) {
      emitRustSafeWrapper(n, wname, rust_return_type, is_void_return);
    }

    // Cleanup
    DelWrapper(f);
    Delete(c_return_type);
    Delete(rust_return_type);
    Delete(rust_ffi_return_type);
    Delete(cleanup);
    Delete(outarg);
    Delete(overloaded_name);

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------------- 
   * classHandler()
   * 
   * Process a class definition.
   * Handles inheritance by detecting base classes.
   * Supports namespaces (sym:nspace) mapped to Rust mod.
   * ----------------------------------------------------------------------------- */
  virtual int classHandler(Node *n) {
    class_name = Getattr(n, "sym:name");
    class_node = n;
    
    // Save old namespace and set current namespace
    String *old_nspace = current_nspace;
    current_nspace = Getattr(n, "sym:nspace");

    // Open namespace mod if needed
    addOpenMod(current_nspace, f_wrapper_code);

    // Handle inheritance - find base class
    baseclass = NULL;
    derived_flag = false;
    List *baselist = Getattr(n, "bases");
    if (baselist) {
      for (Iterator base = First(baselist); base.item; base = Next(base)) {
        if (!GetFlag(base.item, "feature:ignore")) {
          String *base_name = Getattr(base.item, "sym:name");
          if (base_name) {
            if (!baseclass) {
              // First base class - use for trait inheritance
              baseclass = Copy(base_name);
              derived_flag = true;
            } else {
              // Multiple inheritance - not directly supported in Rust
              // Generate upcast functions instead
              Printf(stderr, "Warning: Multiple inheritance detected for %s, base %s will use upcast functions.\n",
                     class_name, base_name);
            }
          }
        }
      }
    }

    // Generate Rust struct for this class
    emitRustStruct(n);

    // Generate Rust trait for this class
    emitRustTrait(n);

    // Process class members - call base class handler
    int result = Language::classHandler(n);
    
    // Generate impl block
    emitRustImpl(n);

    // Generate upcast functions for additional base classes (multiple inheritance)
    if (baselist && Len(baselist) > 1) {
      emitUpcasts(n, baselist);
    }

    // Close namespace mod if needed
    addCloseMod(current_nspace, f_wrapper_code);

    class_name = NULL;
    class_node = NULL;
    
    // Restore old namespace
    current_nspace = old_nspace;
    
    if (baseclass) {
      Delete(baseclass);
      baseclass = NULL;
    }
    derived_flag = false;

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------------- 
   * constructorHandler()
   * 
   * Process a constructor.
   * ----------------------------------------------------------------------------- */
  virtual int constructorHandler(Node *n) {
    // Call base class first - this sets up wrap:action correctly and calls functionWrapper
    Language::constructorHandler(n);

    // Generate Rust new() function
    emitRustConstructor(n);

    return SWIG_OK;
  }
  
  /* ----------------------------------------------------------------------------- 
   * memberfunctionHandler()
   * 
   * Process a member function.
   * ----------------------------------------------------------------------------- */
  virtual int memberfunctionHandler(Node *n) {
    // Call base class - this will eventually call functionWrapper()
    return Language::memberfunctionHandler(n);
  }
  
  /* ----------------------------------------------------------------------------- 
   * functionHandler()
   * 
   * Override to handle member functions.
   * ----------------------------------------------------------------------------- */
  virtual int functionHandler(Node *n) {
    return Language::functionHandler(n);
  }

  /* ----------------------------------------------------------------------------- 
   * destructorHandler()
   * 
   * Process a destructor.
   * ----------------------------------------------------------------------------- */
  virtual int destructorHandler(Node *n) {
    // Call base class first - this sets up wrap:action correctly and calls functionWrapper
    Language::destructorHandler(n);

    // Generate Rust Drop impl
    emitRustDrop(n);

    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------------- 
   * staticmemberfunctionHandler()
   * 
   * Process a static member function.
   * Static functions become associated functions in Rust (no self parameter).
   * ----------------------------------------------------------------------------- */
  virtual int staticmemberfunctionHandler(Node *n) {
    static_flag = true;
    Language::staticmemberfunctionHandler(n);
    
    // Generate wrapper
    functionWrapper(n);
    
    static_flag = false;
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------------- 
   * variableHandler()
   * 
   * Process a variable (getter/setter generation).
   * ----------------------------------------------------------------------------- */
  virtual int variableHandler(Node *n) {
    variable_wrapper_flag = true;
    
    // Generate getter
    functionWrapper(n);

    // Generate setter if not const
    if (!GetFlag(n, "constant")) {
      // Create setter node and generate
      // (simplified for now)
    }

    variable_wrapper_flag = false;
    return SWIG_OK;
  }
  
  /* ----------------------------------------------------------------------------- 
   * membervariableHandler()
   * 
   * Process a member variable.
   * Generates getter and setter methods.
   * ----------------------------------------------------------------------------- */
  virtual int membervariableHandler(Node *n) {
    variable_wrapper_flag = true;
    Language::membervariableHandler(n);
    variable_wrapper_flag = false;
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------------- 
   * staticmembervariableHandler()
   * 
   * Process a static member variable.
   * ----------------------------------------------------------------------------- */
  virtual int staticmembervariableHandler(Node *n) {
    static_flag = true;
    variable_wrapper_flag = true;
    Language::staticmembervariableHandler(n);
    static_flag = false;
    variable_wrapper_flag = false;
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------------- 
   * enumDeclaration()
   * 
   * Process an enum declaration.
   * Supports namespaces mapped to Rust mod.
   * Handles anonymous enums (skips them or generates constants).
   * ----------------------------------------------------------------------------- */
  virtual int enumDeclaration(Node *n) {
    String *name = Getattr(n, "sym:name");
    String *nspace = Getattr(n, "sym:nspace");
    
    // Skip anonymous enums (names starting with $ or empty)
    if (!name || Len(name) == 0 || Strstr(name, "$")) {
      // For anonymous enums, generate constants instead
      for (Node *child = firstChild(n); child; child = nextSibling(child)) {
        if (Strcmp(nodeType(child), "enumvalue") == 0) {
          String *vname = Getattr(child, "sym:name");
          String *value = Getattr(child, "enumvalue");
          if (vname) {
            // Open namespace mod if needed
            addOpenMod(nspace, f_wrapper_code);
            
            if (value) {
              Printf(f_wrapper_code, "pub const %s: i32 = %s;\n", vname, value);
            } else {
              Printf(f_wrapper_code, "pub const %s: i32 = 0;\n", vname);
            }
            
            // Close namespace mod if needed
            addCloseMod(nspace, f_wrapper_code);
          }
        }
      }
      return SWIG_OK;
    }
    
    // Count enum values
    int value_count = 0;
    for (Node *child = firstChild(n); child; child = nextSibling(child)) {
      if (Strcmp(nodeType(child), "enumvalue") == 0) {
        value_count++;
      }
    }
    
    // Skip empty enums
    if (value_count == 0) {
      return SWIG_OK;
    }
    
    // Open namespace mod if needed
    addOpenMod(nspace, f_wrapper_code);
    
    // Generate Rust enum
    Printf(f_wrapper_code, "#[repr(C)]\n");
    Printf(f_wrapper_code, "#[derive(Debug, Copy, Clone, PartialEq, Eq)]\n");
    Printf(f_wrapper_code, "pub enum %s {\n", name);
    
    // Process enum values
    for (Node *child = firstChild(n); child; child = nextSibling(child)) {
      if (Strcmp(nodeType(child), "enumvalue") == 0) {
        String *vname = Getattr(child, "sym:name");
        String *value = Getattr(child, "enumvalue");
        if (vname) {
          if (value) {
            Printf(f_wrapper_code, "    %s = %s,\n", vname, value);
          } else {
            Printf(f_wrapper_code, "    %s,\n", vname);
          }
        }
      }
    }
    
    Printf(f_wrapper_code, "}\n\n");
    
    // Close namespace mod if needed
    addCloseMod(nspace, f_wrapper_code);
    
    return SWIG_OK;
  }

  /* ----------------------------------------------------------------------------- 
   * constantWrapper()
   * 
   * Process a constant.
   * ----------------------------------------------------------------------------- */
  virtual int constantWrapper(Node *n) {
    String *name = Getattr(n, "sym:name");
    SwigType *type = Getattr(n, "type");
    String *value = Getattr(n, "value");

    // Generate Rust constant
    String *rust_type = Swig_typemap_lookup("rusttype", n, "", 0);
    if (!rust_type) {
      rust_type = SwigType_str(type, 0);
    }

    Printf(f_wrapper_code, "pub const %s: %s = %s;\n", name, rust_type, value);

    return SWIG_OK;
  }

private:
  /* ----------------------------------------------------------------------------- 
   * getOverloadedName()
   * 
   * Get a unique name for overloaded functions.
   * ----------------------------------------------------------------------------- */
  String *getOverloadedName(Node *n) {
    String *symname = Getattr(n, "sym:name");
    String *overname = Getattr(n, "sym:overname");
    
    if (overname) {
      return NewStringf("%s%s", symname, overname);
    } else {
      return Copy(symname);
    }
  }

  /* ----------------------------------------------------------------------------- 
   * getRustType()
   * 
   * Get Rust FFI type from a SWIG type.
   * This returns FFI-compatible types like c_int, c_long, etc.
   * ----------------------------------------------------------------------------- */
  String *getRustType(SwigType *t) {
    if (!t) return NewString("*mut c_void");
    
    // Handle basic types
    switch (SwigType_type(t)) {
      case T_BOOL:
        return NewString("bool");
      case T_CHAR:
      case T_SCHAR:
        return NewString("c_char");
      case T_UCHAR:
        return NewString("c_uchar");
      case T_SHORT:
        return NewString("c_short");
      case T_USHORT:
        return NewString("c_ushort");
      case T_INT:
        return NewString("c_int");
      case T_UINT:
        return NewString("c_uint");
      case T_LONG:
        return NewString("c_long");
      case T_ULONG:
        return NewString("c_ulong");
      case T_LONGLONG:
        return NewString("c_longlong");
      case T_ULONGLONG:
        return NewString("c_ulonglong");
      case T_FLOAT:
        return NewString("f32");
      case T_DOUBLE:
        return NewString("f64");
      case T_VOID:
        return NewString("()");
      default:
        // For unknown types, return opaque pointer
        return NewString("*mut c_void");
    }
  }

  /* ----------------------------------------------------------------------------- 
   * getRustUserType()
   * 
   * Get Rust user-visible type from a SWIG type.
   * This returns user-friendly types like i32, i64, etc. for use in trait/impl signatures.
   * ----------------------------------------------------------------------------- */
  String *getRustUserType(SwigType *t) {
    if (!t) return NewString("*mut c_void");
    
    // Handle basic types
    switch (SwigType_type(t)) {
      case T_BOOL:
        return NewString("bool");
      case T_CHAR:
      case T_SCHAR:
        return NewString("i8");
      case T_UCHAR:
        return NewString("u8");
      case T_SHORT:
        return NewString("i16");
      case T_USHORT:
        return NewString("u16");
      case T_INT:
        return NewString("i32");
      case T_UINT:
        return NewString("u32");
      case T_LONG:
        return NewString("i64");
      case T_ULONG:
        return NewString("u64");
      case T_LONGLONG:
        return NewString("i64");
      case T_ULONGLONG:
        return NewString("u64");
      case T_FLOAT:
        return NewString("f32");
      case T_DOUBLE:
        return NewString("f64");
      case T_VOID:
        return NewString("()");
      default:
        // For unknown types, return opaque pointer
        return NewString("*mut c_void");
    }
  }

  /* ----------------------------------------------------------------------------- 
   * processRustType()
   * 
   * Process a Rust type string, handling special markers like SWIGENUM.
   * If the type is SWIGENUM, extract the actual enum name from the SWIG type.
   * Returns a new string that should be deleted by the caller.
   * ----------------------------------------------------------------------------- */
  String *processRustType(String *rust_type, SwigType *swig_type) {
    if (!rust_type || Len(rust_type) == 0) {
      return NewString("*mut c_void");
    }
    
    // Check for SWIGENUM marker
    if (Cmp(rust_type, "SWIGENUM") == 0) {
      if (swig_type) {
        // Get the base type name without "enum" keyword
        String *enum_name = SwigType_base(swig_type);
        // Check for anonymous enum (names starting with $)
        if (Strstr(enum_name, "$")) {
          Delete(enum_name);
          return NewString("i32");
        }
        // Clean the type name (remove enum/struct/class prefix)
        String *clean_name = cleanTypeName(enum_name);
        Delete(enum_name);
        return clean_name;
      }
      return NewString("i32");
    }
    
    // Return a copy of the original type
    return Copy(rust_type);
  }

  /* ----------------------------------------------------------------------------- 
   * cleanTypeName()
   * 
   * Clean a type name by removing C/C++ keywords like enum, struct, class.
   * Also handles types that have these keywords embedded in the name string.
   * Returns a new string that should be deleted by the caller.
   * ----------------------------------------------------------------------------- */
  String *cleanTypeName(String *type_name) {
    if (!type_name || Len(type_name) == 0) {
      return NewString("");
    }
    
    String *result = Copy(type_name);
    
    // Remove "enum " prefix (with space)
    if (Strstr(result, "enum ")) {
      Replaceall(result, "enum ", "");
    }
    // Remove "struct " prefix
    if (Strstr(result, "struct ")) {
      Replaceall(result, "struct ", "");
    }
    // Remove "class " prefix
    if (Strstr(result, "class ")) {
      Replaceall(result, "class ", "");
    }
    
    // Also remove just "enum" if it's at the start (without space, followed by name)
    // This handles cases like "enumfoo2" -> "foo2"
    char *str = Char(result);
    if (strncmp(str, "enum", 4) == 0 && Len(result) > 4) {
      String *temp = NewString(Char(result) + 4);
      Delete(result);
      result = temp;
    }
    
    return result;
  }

  /* ----------------------------------------------------------------------------- 
   * addOpenMod()
   * 
   * Generate opening of Rust mod for namespace support.
   * ----------------------------------------------------------------------------- */
  void addOpenMod(const String *nspace, File *file) {
    if (namespce || nspace) {
      if (namespce) {
        Printf(file, "pub mod %s {\n", namespce);
        if (nspace) {
          Printf(file, "pub mod %s {\n", nspace);
        }
      } else if (nspace) {
        Printf(file, "pub mod %s {\n", nspace);
      }
    }
  }

  /* ----------------------------------------------------------------------------- 
   * addCloseMod()
   * 
   * Generate closing of Rust mod for namespace support.
   * ----------------------------------------------------------------------------- */
  void addCloseMod(const String *nspace, File *file) {
    if (namespce || nspace) {
      if (namespce) {
        if (nspace) {
          Printf(file, "}\n");
        }
        Printf(file, "}\n");
      } else if (nspace) {
        Printf(file, "}\n");
      }
    }
  }

  /* ----------------------------------------------------------------------------- 
   * getNSpace()
   * 
   * Get current namespace (sym:nspace attribute of current node).
   * ----------------------------------------------------------------------------- */
  String *getNSpace() {
    return current_nspace;
  }

  /* ----------------------------------------------------------------------------- 
   * outputDirectory()
   * 
   * Return the directory to use for generating Rust files.
   * For Rust, we create subdirectories based on namespace/module structure.
   * ----------------------------------------------------------------------------- */
  String *outputDirectory(String *nspace) {
    String *output_directory = Copy(SWIG_output_directory());
    if (nspace) {
      // Replace dots with path separators for nested namespaces
      String *nspace_subdirectory = Copy(nspace);
      Replaceall(nspace_subdirectory, ".", SWIG_FILE_DELIMITER);
      String *newdir_error = Swig_new_subdirectory(output_directory, nspace_subdirectory);
      if (newdir_error) {
        Printf(stderr, "%s\n", newdir_error);
        Delete(newdir_error);
        Exit(EXIT_FAILURE);
      }
      Printv(output_directory, nspace_subdirectory, SWIG_FILE_DELIMITER, 0);
      Delete(nspace_subdirectory);
    }
    return output_directory;
  }

  /* ----------------------------------------------------------------------------- 
   * emitRustFile()
   * 
   * Generate the final Rust output file.
   * ----------------------------------------------------------------------------- */
  void emitRustFile(Node *n) {
    // Debug: Check if we have content to output
    if (!f_ffi_code || !f_wrapper_code) {
      Printf(stderr, "Error: Rust output buffers not initialized\n");
      return;
    }
    
    // Get output filename - module_name should have been set in top()
    String *rust_filename;
    if (module_name && Len(module_name) > 0) {
      rust_filename = NewStringf("%s%s.rs", SWIG_output_directory(), module_name);
    } else {
      // Fallback: use the input filename base
      String *input_file = Getattr(n, "infile");
      if (input_file) {
        String *basename = Swig_file_filename(input_file);
        String *name_only = Swig_file_basename(basename);
        rust_filename = NewStringf("%s%s.rs", SWIG_output_directory(), name_only);
        Delete(basename);
        Delete(name_only);
      } else {
        rust_filename = NewStringf("%srust_output.rs", SWIG_output_directory());
      }
    }
    
    File *f_rust = NewFile(rust_filename, "w", SWIG_output_files());
    
    if (!f_rust) {
      FileErrorDisplay(rust_filename);
      Exit(EXIT_FAILURE);
    }

    // Write banner
    Swig_banner_target_lang(f_rust, "//");
    Printf(f_rust, "\n");

    // Write FFI module
    if (!ffi_only_flag) {
      Printf(f_rust, "mod ffi {\n");
      Printf(f_rust, "    #![allow(non_snake_case)]\n");
      Printf(f_rust, "    #![allow(non_camel_case_types)]\n");
      Printf(f_rust, "    #![allow(dead_code)]\n\n");
      
      Printf(f_rust, "    use std::os::raw::*;\n\n");
      
      // Write FFI declarations
      Dump(f_ffi_code, f_rust);
      
      Printf(f_rust, "}\n\n");
    }

    // Write safe wrapper code
    if (safe_wrapper_flag) {
      Printf(f_rust, "// Safe wrapper functions\n\n");
      Dump(f_wrapper_code, f_rust);
    }

    Delete(f_rust);
    Delete(rust_filename);
  }

  /* ----------------------------------------------------------------------------- 
   * emitRustFFIDeclaration()
   * 
   * Generate Rust FFI function declaration.
   * ----------------------------------------------------------------------------- */
    void emitRustFFIDeclaration(Node *n, String *wname, String *return_type, bool is_void) {
      ParmList *l = Getattr(n, "parms");
      
      Printf(f_ffi_code, "    extern \"C\" {\n");
      Printf(f_ffi_code, "        pub fn %s(", wname);
  
      // Get number of arguments (handle NULL parameter list)
      int num_arguments = l ? emit_num_arguments(l) : 0;
      Parm *p = l;
      int arg_num = 0;
  
      // Process parameters
      for (int i = 0; i < num_arguments && p; i++) {
        // Skip parameters with numinputs=0
        while (p && checkAttribute(p, "tmap:in:numinputs", "0")) {
          p = Getattr(p, "tmap:in:next");
        }
        
        if (!p) break;
  
        String *ln = Getattr(p, "lname");
        String *arg_name = NewStringf("j%s", ln);
        
        // Get FFI type from typemap
        String *rust_type = Getattr(p, "tmap:rsffitype");
        if (!rust_type) {
          rust_type = Getattr(p, "tmap:rusttype");
        }
        if (!rust_type) {
          rust_type = NewString("*mut c_void");
        }
  
        if (arg_num > 0) {
          Printf(f_ffi_code, ", ");
        }
        Printf(f_ffi_code, "%s: %s", arg_name, rust_type);
        arg_num++;
  
        // Move to next parameter
        if (Getattr(p, "tmap:in:next")) {
          p = Getattr(p, "tmap:in:next");
        } else {
          p = nextSibling(p);
        }
  
        Delete(arg_name);
      }
  
      
      Printf(f_ffi_code, ")");
  
      // Return type - only output if not void AND return_type is not empty
      if (!is_void && return_type && Len(return_type) > 0) {
        Printf(f_ffi_code, " -> %s", return_type);
      }
  
      Printf(f_ffi_code, ";\n");
      Printf(f_ffi_code, "    }\n\n");
    }
  /* ----------------------------------------------------------------------------- 
   * emitRustSafeWrapper()
   * 
   * Generate safe Rust wrapper function for global (non-member) functions.
   * Member functions are handled separately in emitRustImpl.
   * Handles:
   *   - Global functions
   *   - Static member functions (associated functions)
   *   - Overloaded functions (adds type-based suffix)
   * ----------------------------------------------------------------------------- */
  void emitRustSafeWrapper(Node *n, String *wname, String *return_type, bool is_void) {
    String *symname = Getattr(n, "sym:name");
    ParmList *l = Getattr(n, "parms");
    bool is_member = GetFlag(n, "ismember");
    bool is_static = GetFlag(n, "static") || static_flag;
    String *view = Getattr(n, "view");
    String *nodeType = Getattr(n, "nodeType");
    bool is_constructor = (view && Cmp(view, "constructorhandler") == 0) ||
                          (nodeType && Cmp(nodeType, "constructor") == 0) ||
                          GetFlag(n, "handled_as_constructor");
    bool is_destructor = (view && Cmp(view, "destructorhandler") == 0) ||
                         (nodeType && Cmp(nodeType, "destructor") == 0);
    
    // Also check sym:name pattern - constructors are named "new_ClassName", destructors "delete_ClassName"
    if (symname) {
      const char *symname_str = Char(symname);
      if (strncmp(symname_str, "new_", 4) == 0) {
        is_constructor = true;
      }
      if (strncmp(symname_str, "delete_", 7) == 0) {
        is_destructor = true;
      }
    }
    
    // Skip constructors and destructors - they are handled separately in emitRustConstructor/emitRustDrop
    if (is_constructor || is_destructor) {
      return;
    }
    
    // Skip non-static member functions - they are handled in emitRustImpl
    if (is_member && !is_static) {
      return;
    }
    
    // IMPORTANT: Check if the first parameter is a self pointer (pointer to a class type)
    // This is the most reliable way to detect member functions in SWIG
    // For member functions, the first parameter is the "this" pointer
    if (l && !is_static) {
      Parm *first_parm = l;
      SwigType *ptype = Getattr(first_parm, "type");
      String *lname = Getattr(first_parm, "lname");
      
      // In SWIG, the self parameter typically has lname "arg1" for the first argument
      // and is a pointer type. We check if the first parameter's lname starts with "arg"
      // and the type is a pointer.
      if (ptype && lname) {
        // Check if lname is "arg1" which is typical for self parameter
        if (Strstr(lname, "arg1") && (SwigType_ispointer(ptype) || SwigType_isreference(ptype))) {
          // This is likely a member function - skip it
          return;
        }
      }
    }
    
    // For static member functions, we need the class name for the impl block
    String *class_impl_name = NULL;
    if (is_member && is_static) {
      class_impl_name = Getattr(n, "parent:sym:name");
      if (!class_impl_name) {
        class_impl_name = Getattr(Getattr(n, "parentNode"), "sym:name");
      }
    }

    // Determine function name - handle overloads
    String *func_name = Copy(symname);
    String *overname = Getattr(n, "sym:overname");
    
    // If this is an overloaded function, add type suffix
    if (overname && Len(overname) > 0) {
      // Generate suffix based on parameter types
      String *suffix = emitOverloadSuffix(l);
      if (suffix && Len(suffix) > 0) {
        Delete(func_name);
        func_name = NewStringf("%s%s", symname, suffix);
      }
      Delete(suffix);
    }

    // Generate function signature
    if (class_impl_name) {
      // Static member function - generate inside impl block
      Printf(f_wrapper_code, "impl %s {\n", class_impl_name);
      Printf(f_wrapper_code, "    pub fn %s(", func_name);
    } else {
      // Global function
      Printf(f_wrapper_code, "pub fn %s(", func_name);
    }

    // Get number of arguments (handle NULL parameter list)
    int num_arguments = l ? emit_num_arguments(l) : 0;
    Parm *p = l;
    int arg_num = 0;

    // Process parameters for signature
    for (int i = 0; i < num_arguments && p; i++) {
      // Skip parameters with numinputs=0
      while (p && checkAttribute(p, "tmap:in:numinputs", "0")) {
        p = Getattr(p, "tmap:in:next");
      }
      
      if (!p) break;

      String *pname = Getattr(p, "name");
      String *ln = Getattr(p, "lname");
      SwigType *ptype = Getattr(p, "type");
      
      // Get Rust type from typemap and process SWIGENUM
      String *rust_type_raw = Getattr(p, "tmap:rusttype");
      String *rust_type = processRustType(rust_type_raw, ptype);

      if (arg_num > 0) {
        Printf(f_wrapper_code, ", ");
      }
      // Use original parameter name if available, otherwise use lname
      Printf(f_wrapper_code, "%s: %s", pname ? pname : ln, rust_type);
      arg_num++;
      Delete(rust_type);

      // Move to next parameter
      if (Getattr(p, "tmap:in:next")) {
        p = Getattr(p, "tmap:in:next");
      } else {
        p = nextSibling(p);
      }
    }

    Printf(f_wrapper_code, ")");

    // Return type
    if (!is_void) {
      Printf(f_wrapper_code, " -> %s", return_type);
    }

    Printf(f_wrapper_code, " {\n");

    // Function body - call FFI
    if (class_impl_name) {
      Printf(f_wrapper_code, "        unsafe { ffi::%s(", wname);
    } else {
      Printf(f_wrapper_code, "    unsafe { ffi::%s(", wname);
    }

    // Pass parameters
    arg_num = 0;
    p = l;
    for (int i = 0; i < num_arguments && p; i++) {
      // Skip parameters with numinputs=0
      while (p && checkAttribute(p, "tmap:in:numinputs", "0")) {
        p = Getattr(p, "tmap:in:next");
      }
      
      if (!p) break;

      String *pname = Getattr(p, "name");
      String *ln = Getattr(p, "lname");

      if (arg_num > 0) {
        Printf(f_wrapper_code, ", ");
      }
      // Use original parameter name for safe wrapper
      Printf(f_wrapper_code, "%s", pname ? pname : ln);
      arg_num++;

      // Move to next parameter
      if (Getattr(p, "tmap:in:next")) {
        p = Getattr(p, "tmap:in:next");
      } else {
        p = nextSibling(p);
      }
    }

    if (class_impl_name) {
      Printf(f_wrapper_code, ") }\n");
      Printf(f_wrapper_code, "    }\n");
      Printf(f_wrapper_code, "}\n\n");
    } else {
      Printf(f_wrapper_code, ") }\n");
      Printf(f_wrapper_code, "}\n\n");
    }
    
    Delete(func_name);
  }

  /* ----------------------------------------------------------------------------- 
   * emitOverloadSuffix()
   * 
   * Generate a suffix for overloaded methods based on parameter types.
   * Example: bar(int) -> _int, bar(int, int) -> _int_int
   * ----------------------------------------------------------------------------- */
  String *emitOverloadSuffix(ParmList *params) {
    String *suffix = NewString("");
    
    for (Parm *p = params; p; p = nextSibling(p)) {
      SwigType *ptype = Getattr(p, "type");
      String *type_str = SwigType_str(ptype, 0);
      String *base_type = SwigType_base(ptype);
      
      // Simplify type name for suffix
      String *simple_type = NewString("");
      
      // Check if it's an enum type
      if (SwigType_isenum(ptype)) {
        // For enum types, use the base name (cleaned, without "enum" keyword)
        String *clean_base = cleanTypeName(base_type);
        Printf(simple_type, "_%s", clean_base);
        Delete(clean_base);
      } else if (Strstr(type_str, "int")) {
        Printf(simple_type, "_int");
      } else if (Strstr(type_str, "char")) {
        Printf(simple_type, "_str");
      } else if (Strstr(type_str, "double")) {
        Printf(simple_type, "_f64");
      } else if (Strstr(type_str, "float")) {
        Printf(simple_type, "_f32");
      } else if (Strstr(type_str, "bool")) {
        Printf(simple_type, "_bool");
      } else if (Strstr(type_str, "long")) {
        Printf(simple_type, "_long");
      } else if (Strstr(type_str, "void")) {
        Printf(simple_type, "_void");
      } else {
        // For other complex types, use the base type name
        Printf(simple_type, "_%s", base_type);
      }
      
      Append(suffix, simple_type);
      Delete(simple_type);
      Delete(type_str);
      Delete(base_type);
    }
    
    return suffix;
  }

  /* ----------------------------------------------------------------------------- 
   * countOverloads()
   * 
   * Count how many methods with the same name exist in a class.
   * ----------------------------------------------------------------------------- */
  int countOverloads(Node *class_node, const String *method_name) {
    int count = 0;
    for (Node *child = firstChild(class_node); child; child = nextSibling(child)) {
      if (Strcmp(nodeType(child), "cdecl") == 0) {
        if (GetFlag(child, "ismember") && !GetFlag(child, "static")) {
          String *decl = Getattr(child, "decl");
          if (decl && SwigType_isfunction(decl)) {
            String *mname = Getattr(child, "sym:name");
            if (mname && Cmp(mname, method_name) == 0) {
              count++;
            }
          }
        }
      }
    }
    return count;
  }

  /* ----------------------------------------------------------------------------- 
   * emitRustStruct()
   * 
   * Generate Rust struct for a C++ class.
   * The struct holds a pointer to the C++ object.
   * ----------------------------------------------------------------------------- */
  void emitRustStruct(Node *n) {
    String *name = Getattr(n, "sym:name");
    

    // Generate struct documentation
    Printf(f_wrapper_code, "/// Rust wrapper for C++ class %s\n", name);
    Printf(f_wrapper_code, "/// Holds a pointer to the underlying C++ object.\n");
    
    // Generate the struct
    Printf(f_wrapper_code, "pub struct %s {\n", name);
    Printf(f_wrapper_code, "    /// Pointer to the C++ object\n");
    Printf(f_wrapper_code, "    ptr: *mut c_void,\n");
    Printf(f_wrapper_code, "}\n\n");
  }

  /* ----------------------------------------------------------------------------- 
   * emitRustTrait()
   * 
   * Generate Rust trait for a C++ class.
   * If the class has a base class, the trait extends the base trait.
   * Handles overloaded methods by adding type-based suffixes.
   * ----------------------------------------------------------------------------- */
  void emitRustTrait(Node *n) {
    String *name = Getattr(n, "sym:name");
    
    // Generate trait with optional inheritance
    Printf(f_wrapper_code, "/// Trait defining the interface for C++ class %s\n", name);
    if (baseclass && derived_flag) {
      Printf(f_wrapper_code, "pub trait %sTrait: %sTrait {\n", name, baseclass);
    } else {
      Printf(f_wrapper_code, "pub trait %sTrait {\n", name);
    }

    // Track method names to detect overloads
    Hash *method_counts = NewHash();

    // First pass: count occurrences of each method name
    for (Node *child = firstChild(n); child; child = nextSibling(child)) {
      if (Strcmp(nodeType(child), "cdecl") == 0) {
        if (GetFlag(child, "ismember") && !GetFlag(child, "static")) {
          String *decl = Getattr(child, "decl");
          if (decl && SwigType_isfunction(decl)) {
            String *mname = Getattr(child, "sym:name");
            if (mname) {
              int count = GetInt(method_counts, mname);
              SetInt(method_counts, mname, count + 1);
            }
          }
        }
      }
    }

    // Second pass: generate trait methods with suffixes for overloads
    Hash *method_indices = NewHash();  // Track current index for each method name

    for (Node *child = firstChild(n); child; child = nextSibling(child)) {
      if (Strcmp(nodeType(child), "cdecl") == 0) {
        if (GetFlag(child, "ismember") && !GetFlag(child, "static")) {
          // Check if this is a function (method), not a variable
          String *decl = Getattr(child, "decl");
          if (!decl || !SwigType_isfunction(decl)) continue;
          
          String *mname = Getattr(child, "sym:name");
          if (!mname) continue;  // Skip unnamed declarations
          
          SwigType *mtype = Getattr(child, "type");
          ParmList *params = Getattr(child, "parms");

          // Determine self type based on const-ness
          bool is_const_method = false;
          if (decl && Strstr(decl, "q(const)")) {
            is_const_method = true;
          }
          
          String *self_type = is_const_method ? NewString("&self") : NewString("&mut self");

          // Check if this method is overloaded
          int total_count = GetInt(method_counts, mname);
          int current_index = GetInt(method_indices, mname);
          SetInt(method_indices, mname, current_index + 1);
          
          // Generate method name with suffix if overloaded
          String *final_mname;
          if (total_count > 1) {
            // Add type-based suffix for overloaded methods
            String *suffix = emitOverloadSuffix(params);
            final_mname = NewStringf("%s%s", mname, suffix);
            Delete(suffix);
          } else {
            final_mname = Copy(mname);
          }

          Printf(f_wrapper_code, "    fn %s(%s", final_mname, self_type);

          // Add parameters (skip first parameter which is the self pointer)
          // The self parameter in SWIG's representation is the first param with name "self" or a pointer type
          int num_params = 0;
          bool first_param = true;
          for (Parm *p = params; p; p = nextSibling(p)) {
            String *pname = Getattr(p, "name");
            SwigType *ptype = Getattr(p, "type");
            
            // Skip the self parameter (first parameter that is a pointer to the class)
            if (first_param) {
              first_param = false;
              // Check if this looks like a self parameter
              if (pname && (Cmp(pname, "self") == 0 || Cmp(pname, "this") == 0)) {
                continue;
              }
              // Also skip if the type is a pointer (likely self pointer)
              if (SwigType_ispointer(ptype) || SwigType_isreference(ptype)) {
                continue;
              }
            }
            
            // Get Rust type using typemap lookup
            // Note: lname may be NULL if parameters haven't been processed by emit_parameter_variables
            // In that case, use getRustUserType for user-visible types
            String *lname_attr = Getattr(p, "lname");
            String *rust_type = NULL;
            if (lname_attr) {
              rust_type = Swig_typemap_lookup("rusttype", p, lname_attr, 0);
            }
            if (!rust_type || Len(rust_type) == 0) {
              // Fallback: generate based on SWIG type (user-visible type)
              rust_type = getRustUserType(ptype);
            }
            
            // Add comma before parameter (first param after self needs comma, subsequent params too)
            Printf(f_wrapper_code, ", ");
            
            // Use pname if available, otherwise use a generated name
            String *param_name = pname ? pname : NewStringf("arg%d", num_params);
            Printf(f_wrapper_code, "%s: %s", param_name, rust_type);
            num_params++;
          }

          Printf(f_wrapper_code, ")");

          // Return type
          if (mtype && SwigType_type(mtype) != T_VOID) {
            String *ret_type = Swig_typemap_lookup("rusttype", child, "", 0);
            if (!ret_type || Len(ret_type) == 0) {
              ret_type = getRustType(mtype);
            }
            if (ret_type && Len(ret_type) > 0) {
              Printf(f_wrapper_code, " -> %s", ret_type);
            }
          }

          Printf(f_wrapper_code, ";\n");

          Delete(self_type);
          Delete(final_mname);
        }
      }
    }

    Delete(method_counts);
    Delete(method_indices);

    Printf(f_wrapper_code, "}\n\n");
  }

  /* ----------------------------------------------------------------------------- 
   * emitRustImpl()
   * 
   * Generate Rust impl block for a class.
   * Generates:
   *   - impl block with ptr() method for internal use
   *   - impl Trait for struct with all member function implementations
   * ----------------------------------------------------------------------------- */
  void emitRustImpl(Node *n) {
    String *name = Getattr(n, "sym:name");

    // Generate main impl block
    Printf(f_wrapper_code, "impl %s {\n", name);

    // Add ptr() method for internal use (returns const pointer)
    Printf(f_wrapper_code, "    /// Get the underlying C++ pointer (read-only)\n");
    Printf(f_wrapper_code, "    pub fn ptr(&self) -> *const c_void {\n");
    Printf(f_wrapper_code, "        self.ptr as *const c_void\n");
    Printf(f_wrapper_code, "    }\n\n");

    // Add ptr_mut() method for internal use (returns mutable pointer)
    Printf(f_wrapper_code, "    /// Get the underlying C++ pointer (mutable)\n");
    Printf(f_wrapper_code, "    pub fn ptr_mut(&mut self) -> *mut c_void {\n");
    Printf(f_wrapper_code, "        self.ptr\n");
    Printf(f_wrapper_code, "    }\n");

    // If derived, add upcast to base class method
    if (derived_flag && baseclass) {
      Printf(f_wrapper_code, "\n    /// Upcast to base class %s\n", baseclass);
      Printf(f_wrapper_code, "    pub fn as_%s(&self) -> &%s {\n", baseclass, baseclass);
      Printf(f_wrapper_code, "        // SAFETY: The pointer is valid and points to a %s\n", baseclass);
      Printf(f_wrapper_code, "        unsafe { &*(self.ptr as *const %s) }\n", baseclass);
      Printf(f_wrapper_code, "    }\n");
      
      Printf(f_wrapper_code, "\n    /// Upcast to base class %s (mutable)\n", baseclass);
      Printf(f_wrapper_code, "    pub fn as_%s_mut(&mut self) -> &mut %s {\n", baseclass, baseclass);
      Printf(f_wrapper_code, "        // SAFETY: The pointer is valid and points to a %s\n", baseclass);
      Printf(f_wrapper_code, "        unsafe { &mut *(self.ptr as *mut %s) }\n", baseclass);
      Printf(f_wrapper_code, "    }\n");
    }

    Printf(f_wrapper_code, "}\n\n");

    // Generate impl of trait
    Printf(f_wrapper_code, "impl %sTrait for %s {\n", name, name);

    // Track method names to detect overloads (same as emitRustTrait)
    Hash *method_counts = NewHash();
    for (Node *child = firstChild(n); child; child = nextSibling(child)) {
      if (Strcmp(nodeType(child), "cdecl") == 0) {
        if (GetFlag(child, "ismember") && !GetFlag(child, "static")) {
          String *decl = Getattr(child, "decl");
          if (decl && SwigType_isfunction(decl)) {
            String *mname = Getattr(child, "sym:name");
            if (mname) {
              int count = GetInt(method_counts, mname);
              SetInt(method_counts, mname, count + 1);
            }
          }
        }
      }
    }
    Hash *method_indices = NewHash();

    // Process member functions
    for (Node *child = firstChild(n); child; child = nextSibling(child)) {
      if (Strcmp(nodeType(child), "cdecl") == 0) {
        if (GetFlag(child, "ismember") && !GetFlag(child, "static")) {
          // Check if this is a function (method), not a variable
          String *decl_attr = Getattr(child, "decl");
          if (!decl_attr || !SwigType_isfunction(decl_attr)) continue;
          
          String *mname = Getattr(child, "sym:name");
          String *wname = Getattr(child, "wrap:name");  // Use the actual wrapper name
          if (!wname) {
            wname = Swig_name_wrapper(mname);
          }
          SwigType *mtype = Getattr(child, "type");
          ParmList *params = Getattr(child, "parms");

          // Determine self type based on const-ness
          bool is_const_method = false;
          SwigType *type = Getattr(child, "type");
          if (type && SwigType_isconst(type)) {
            is_const_method = true;
          }
          if (decl_attr && Strstr(decl_attr, "r.q(const)")) {
            is_const_method = true;
          }
          
          String *self_type = is_const_method ? NewString("&self") : NewString("&mut self");

          // Check if this method is overloaded - same logic as emitRustTrait
          int total_count = GetInt(method_counts, mname);
          int current_index = GetInt(method_indices, mname);
          SetInt(method_indices, mname, current_index + 1);
          
          // Generate method name with suffix if overloaded
          String *final_mname;
          if (total_count > 1) {
            String *suffix = emitOverloadSuffix(params);
            final_mname = NewStringf("%s%s", mname, suffix);
            Delete(suffix);
          } else {
            final_mname = Copy(mname);
          }

          Printf(f_wrapper_code, "    fn %s(%s", final_mname, self_type);

          // Add parameters
          // Note: For class member functions, the raw params don't include the 'this' pointer
          // So we don't need to skip the first parameter
          int num_params = 0;
          for (Parm *p = params; p; p = nextSibling(p)) {
            String *pname = Getattr(p, "name");
            SwigType *ptype = Getattr(p, "type");
            
            // Get Rust type - handle NULL lname
            String *lname_attr = Getattr(p, "lname");
            String *rust_type = NULL;
            if (lname_attr) {
              rust_type = Swig_typemap_lookup("rusttype", p, lname_attr, 0);
            }
            if (!rust_type || Len(rust_type) == 0) {
              rust_type = getRustUserType(ptype);
            }
            
            // Add comma before parameter
            Printf(f_wrapper_code, ", ");
            Printf(f_wrapper_code, "%s: %s", pname ? pname : "arg", rust_type);
            num_params++;
          }

          Printf(f_wrapper_code, ")");

          // Return type
          bool has_return = false;
          if (mtype && SwigType_type(mtype) != T_VOID) {
            String *ret_type = Swig_typemap_lookup("rusttype", child, "", 0);
            if (!ret_type || Len(ret_type) == 0) {
              ret_type = getRustType(mtype);
            }
            if (ret_type && Len(ret_type) > 0) {
              Printf(f_wrapper_code, " -> %s", ret_type);
              has_return = true;
            }
          }

          Printf(f_wrapper_code, " {\n");
          
          // Generate FFI call - always pass self.ptr as first argument
          Printf(f_wrapper_code, "        unsafe { ffi::%s(self.ptr", wname);

          // Add parameters (pass all method parameters after self.ptr)
          for (Parm *p = params; p; p = nextSibling(p)) {
            String *pname = Getattr(p, "name");
            Printf(f_wrapper_code, ", %s", pname ? pname : "arg");
          }

          Printf(f_wrapper_code, ") }\n");
          Printf(f_wrapper_code, "    }\n");

          Delete(self_type);
          Delete(final_mname);
        }
      }
    }

    Delete(method_counts);
    Delete(method_indices);

    Printf(f_wrapper_code, "}\n\n");
  }

  /* ----------------------------------------------------------------------------- 
   * emitRustConstructor()
   * 
   * Generate Rust new() function for constructor.
   * Supports constructors with parameters.
   * Handles overloaded constructors by adding type-based suffix.
   * ----------------------------------------------------------------------------- */
  void emitRustConstructor(Node *n) {
    String *class_name = Getattr(n, "parent:sym:name");
    if (!class_name) {
      class_name = Getattr(Getattr(n, "parentNode"), "sym:name");
    }
    
    // Get the constructor's symbol name - this should be like "new_ClassName" or "new_ClassName__SWIG_0"
    // The wrap:name attribute is set by functionWrapper, which uses Swig_name_wrapper(symname)
    String *symname = Getattr(n, "sym:name");
    String *wname = Getattr(n, "wrap:name");
    
    // If wrap:name is not set, we need to generate it ourselves
    // This can happen if emitRustConstructor is called before functionWrapper sets wrap:name
    if (!wname && symname) {
      // Use the same wrapper name generation as functionWrapper
      String *overname = Getattr(n, "sym:overname");
      String *full_symname;
      if (overname) {
        full_symname = NewStringf("%s%s", symname, overname);
      } else {
        full_symname = Copy(symname);
      }
      wname = Swig_name_wrapper(full_symname);
      Delete(full_symname);
    }
    ParmList *l = Getattr(n, "parms");

    // Get number of arguments
    int num_arguments = 0;
    if (l) {
      num_arguments = emit_num_arguments(l);
    }

    // Determine constructor name - handle overloads
    String *ctor_name = NewString("new");
    String *overname = Getattr(n, "sym:overname");
    
    // If this is an overloaded constructor, add type suffix
    if (overname && Len(overname) > 0) {
      // Generate suffix based on parameter types
      String *suffix = emitOverloadSuffix(l);
      if (suffix && Len(suffix) > 0) {
        Delete(ctor_name);
        ctor_name = NewStringf("new%s", suffix);
      }
      Delete(suffix);
    }

    Printf(f_wrapper_code, "impl %s {\n", class_name);
    
    // Generate constructor signature
    Printf(f_wrapper_code, "    pub fn %s(", ctor_name);

    // Add parameters
    if (l && num_arguments > 0) {
      Parm *p = l;
      int arg_num = 0;
      for (int i = 0; i < num_arguments && p; i++) {
        while (p && checkAttribute(p, "tmap:in:numinputs", "0")) {
          p = Getattr(p, "tmap:in:next");
        }
        
        if (!p) break;

        String *pname = Getattr(p, "name");
        String *ln = Getattr(p, "lname");
        String *rust_type = Getattr(p, "tmap:rusttype");
        if (!rust_type) {
          rust_type = NewString("*mut c_void");
        }

        if (arg_num > 0) {
          Printf(f_wrapper_code, ", ");
        }
        Printf(f_wrapper_code, "%s: %s", pname ? pname : ln, rust_type);
        arg_num++;

        if (Getattr(p, "tmap:in:next")) {
          p = Getattr(p, "tmap:in:next");
        } else {
          p = nextSibling(p);
        }
      }
    }

    Printf(f_wrapper_code, ") -> Self {\n");
    Printf(f_wrapper_code, "        Self {\n");
    
    // Generate FFI call with parameters
    Printf(f_wrapper_code, "            ptr: unsafe { ffi::%s(", wname);
    
    if (l && num_arguments > 0) {
      Parm *p = l;
      int arg_num = 0;
      for (int i = 0; i < num_arguments && p; i++) {
        while (p && checkAttribute(p, "tmap:in:numinputs", "0")) {
          p = Getattr(p, "tmap:in:next");
        }
        
        if (!p) break;

        String *pname = Getattr(p, "name");
        String *ln = Getattr(p, "lname");

        if (arg_num > 0) {
          Printf(f_wrapper_code, ", ");
        }
        Printf(f_wrapper_code, "%s", pname ? pname : ln);
        arg_num++;

        if (Getattr(p, "tmap:in:next")) {
          p = Getattr(p, "tmap:in:next");
        } else {
          p = nextSibling(p);
        }
      }
    }
    
    Printf(f_wrapper_code, ") },\n");
    Printf(f_wrapper_code, "        }\n");
    Printf(f_wrapper_code, "    }\n");
    Printf(f_wrapper_code, "}\n\n");
    
    Delete(ctor_name);
  }

  /* ----------------------------------------------------------------------------- 
   * emitRustDrop()
   * 
   * Generate Rust Drop impl for destructor.
   * ----------------------------------------------------------------------------- */
  void emitRustDrop(Node *n) {
    String *class_name = Getattr(n, "parent:sym:name");
    if (!class_name) {
      class_name = Getattr(Getattr(n, "parentNode"), "sym:name");
    }
    
    // Get the destructor's symbol name - this should be like "delete_ClassName"
    String *symname = Getattr(n, "sym:name");
    String *wname = Getattr(n, "wrap:name");
    
    // If wrap:name is not set, we need to generate it ourselves
    if (!wname && symname) {
      // Use the same wrapper name generation as functionWrapper
      String *overname = Getattr(n, "sym:overname");
      String *full_symname;
      if (overname) {
        full_symname = NewStringf("%s%s", symname, overname);
      } else {
        full_symname = Copy(symname);
      }
      wname = Swig_name_wrapper(full_symname);
      Delete(full_symname);
    }

    Printf(f_wrapper_code, "impl Drop for %s {\n", class_name);
    Printf(f_wrapper_code, "    fn drop(&mut self) {\n");
    Printf(f_wrapper_code, "        unsafe { ffi::%s(self.ptr) }\n", wname);
    Printf(f_wrapper_code, "    }\n");
    Printf(f_wrapper_code, "}\n\n");
  }

  /* ----------------------------------------------------------------------------- 
   * emitUpcasts()
   * 
   * Generate upcast functions for multiple inheritance.
   * In C++, a derived class can have multiple base classes.
   * In Rust, we can only inherit from one trait.
   * For additional base classes, we generate upcast methods.
   * ----------------------------------------------------------------------------- */
  void emitUpcasts(Node *n, List *baselist) {
    String *derived_name = Getattr(n, "sym:name");
    bool first = true;
    
    for (Iterator base = First(baselist); base.item; base = Next(base)) {
      if (!GetFlag(base.item, "feature:ignore")) {
        String *base_name = Getattr(base.item, "sym:name");
        if (base_name) {
          if (first) {
            // First base class is handled by the main trait inheritance
            first = false;
            continue;
          }
          
          // Generate upcast function for additional base class
          // This generates a C wrapper function to cast the pointer
          String *wname = NewStringf("Rust_%s_to_%s", derived_name, base_name);
          
          // Generate C upcast function
          if (!upcasts_code) {
            upcasts_code = NewString("");
          }
          Printf(upcasts_code, "SWIGEXPORT void *%s(void *obj) {\n", wname);
          Printf(upcasts_code, "    return (void *)(%s *)obj;\n", base_name);
          Printf(upcasts_code, "}\n\n");
          
          // Generate Rust FFI declaration
          Printf(f_ffi_code, "    extern \"C\" {\n");
          Printf(f_ffi_code, "        pub fn %s(obj: *mut c_void) -> *mut c_void;\n", wname);
          Printf(f_ffi_code, "    }\n\n");
          
          // Generate Rust upcast method
          Printf(f_wrapper_code, "impl %s {\n", derived_name);
          Printf(f_wrapper_code, "    /// Upcast to additional base class %s\n", base_name);
          Printf(f_wrapper_code, "    pub fn upcast_to_%s(&self) -> %s {\n", base_name, base_name);
          Printf(f_wrapper_code, "        %s { ptr: unsafe { ffi::%s(self.ptr) } }\n", base_name, wname);
          Printf(f_wrapper_code, "    }\n");
          Printf(f_wrapper_code, "}\n\n");
          
          Delete(wname);
        }
      }
    }
  }

  /* ----------------------------------------------------------------------------- 
   * emitTypeWrapperClass()
   * 
   * Generate a type wrapper class for SWIG types.
   * ----------------------------------------------------------------------------- */
  void emitTypeWrapperClass(String *type, String *decl) {
    // For Rust, we generate a simple struct wrapper for unknown types
    Printf(f_wrapper_code, "pub struct SWIGTYPE_%s {\n", type);
    Printf(f_wrapper_code, "    ptr: *mut c_void,\n");
    Printf(f_wrapper_code, "}\n\n");
  }

  /* ----------------------------------------------------------------------------- 
   * directorClassName()
   * 
   * Get the director class name for a class.
   * ----------------------------------------------------------------------------- */
  String *directorClassName(Node *n) {
    String *name = Getattr(n, "sym:name");
    return NewStringf("SwigDirector_%s", name);
  }

  /* ----------------------------------------------------------------------------- 
   * classDirectorInit()
   * 
   * Initialize director support for a class.
   * Generates:
   *   - C++ director class declaration in header
   *   - Rust director struct and trait
   * ----------------------------------------------------------------------------- */
  int classDirectorInit(Node *n) {
    // Set up director constructor code
    Delete(director_ctor_code);
    director_ctor_code = NewString("$director_new");

    String *classname = Getattr(n, "sym:name");
    String *classtype = Getattr(n, "classtype");
    String *dirclassname = directorClassName(n);

    // Generate C++ director class declaration
    Printf(f_directors_h, "class %s : public %s, public Swig::Director {\n", dirclassname, classtype);
    Printf(f_directors_h, "public:\n");

    // Store director class name for later use
    Setattr(n, "director:name", dirclassname);

    // Generate Rust director support
    // Director trait for Rust implementations
    Printf(f_wrapper_code, "/// Trait for Rust implementations of %s that can be used as Director callbacks\n", classname);
    Printf(f_wrapper_code, "/// \n");
    Printf(f_wrapper_code, "/// # Safety\n");
    Printf(f_wrapper_code, "/// \n");
    Printf(f_wrapper_code, "/// This trait is used for C++ callbacks (Director pattern).\n");
    Printf(f_wrapper_code, "/// C++ may call back into Rust during method execution.\n");
    Printf(f_wrapper_code, "/// Methods use `&self` by default to allow safe re-entrancy.\n");
    Printf(f_wrapper_code, "/// - Use `Cell<T>` for simple mutable state (Copy types)\n");
    Printf(f_wrapper_code, "/// - Use `RefCell<T>` for complex state (panics on re-entrancy)\n");
    Printf(f_wrapper_code, "pub trait %sDirector {\n", classname);

    // We'll add methods in classDirectorMethod

    return Language::classDirectorInit(n);
  }

  /* ----------------------------------------------------------------------------- 
   * classDirectorEnd()
   * 
   * Finalize director class.
   * Generates:
   *   - swig_connect_director method
   *   - Closes class definition
   *   - Rust new_with_trait constructor
   * ----------------------------------------------------------------------------- */
  int classDirectorEnd(Node *n) {
    String *classname = Getattr(n, "sym:name");
    String *dirclassname = directorClassName(n);

    // Close the Director trait
    Printf(f_wrapper_code, "}\n\n");

    // Generate new_with_trait constructor in Rust
    Printf(f_wrapper_code, "impl %s {\n", classname);
    Printf(f_wrapper_code, "    /// Create a new %s with a Rust Director implementation\n", classname);
    Printf(f_wrapper_code, "    /// \n");
    Printf(f_wrapper_code, "    /// # Arguments\n");
    Printf(f_wrapper_code, "    /// * `director` - A Rust implementation of %sDirector\n", classname);
    Printf(f_wrapper_code, "    /// \n");
    Printf(f_wrapper_code, "    /// # Safety\n");
    Printf(f_wrapper_code, "    /// The returned %s holds a C++ object that calls back into Rust.\n", classname);
    Printf(f_wrapper_code, "    /// The director must remain valid for the lifetime of the %s.\n", classname);
    Printf(f_wrapper_code, "    pub fn new_with_trait<D: %sDirector + 'static>(director: D) -> Self {\n", classname);
    Printf(f_wrapper_code, "        let director_box = Box::new(director);\n");
    Printf(f_wrapper_code, "        let director_ptr = Box::into_raw(director_box) as *mut c_void;\n");
    Printf(f_wrapper_code, "        Self {\n");
    Printf(f_wrapper_code, "            ptr: unsafe { ffi::%s_new_director(director_ptr) },\n", dirclassname);
    Printf(f_wrapper_code, "        }\n");
    Printf(f_wrapper_code, "    }\n");
    Printf(f_wrapper_code, "}\n\n");

    // Close C++ director class
    Printf(f_directors_h, "private:\n");
    Printf(f_directors_h, "    void *swig_rust_director_;  // Pointer to Rust trait object\n");
    Printf(f_directors_h, "};\n\n");

    // Generate the director new function
    Printf(f_directors, "extern \"C\" SWIGEXPORT void *%s_new_director(void *rust_director) {\n", dirclassname);
    Printf(f_directors, "    %s *director = new %s();\n", dirclassname, dirclassname);
    Printf(f_directors, "    director->swig_rust_director_ = rust_director;\n");
    Printf(f_directors, "    return director;\n");
    Printf(f_directors, "}\n\n");

    // Generate FFI declaration for director constructor
    Printf(f_ffi_code, "    extern \"C\" {\n");
    Printf(f_ffi_code, "        pub fn %s_new_director(rust_director: *mut c_void) -> *mut c_void;\n", dirclassname);
    Printf(f_ffi_code, "    }\n\n");

    Delete(dirclassname);
    return Language::classDirectorEnd(n);
  }

  /* ----------------------------------------------------------------------------- 
   * classDirectorConstructor()
   * 
   * Generate constructor for director class.
   * ----------------------------------------------------------------------------- */
  int classDirectorConstructor(Node *n) {
    Node *parent = Getattr(n, "parentNode");
    String *dirclassname = directorClassName(parent);
    String *classname = Getattr(parent, "sym:name");
    
    ParmList *l = Getattr(n, "parms");
    String *symname = Getattr(n, "sym:name");
    
    // Generate C++ director constructor declaration
    Printf(f_directors_h, "    %s(", dirclassname);
    
    // Add parameters
    if (l) {
      int first = 1;
      for (Parm *p = l; p; p = nextSibling(p)) {
        if (!first) Printf(f_directors_h, ", ");
        String *pt = SwigType_str(Getattr(p, "type"), 0);
        String *pn = Getattr(p, "name");
        Printf(f_directors_h, "%s %s", pt, pn);
        Delete(pt);
        first = 0;
      }
    }
    Printf(f_directors_h, ");\n");
    
    // Generate C++ director constructor implementation
    Printf(f_directors, "%s::%s(", dirclassname, dirclassname);
    if (l) {
      int first = 1;
      for (Parm *p = l; p; p = nextSibling(p)) {
        if (!first) Printf(f_directors, ", ");
        String *pt = SwigType_str(Getattr(p, "type"), 0);
        String *pn = Getattr(p, "name");
        Printf(f_directors, "%s %s", pt, pn);
        Delete(pt);
        first = 0;
      }
    }
    Printf(f_directors, ") : Swig::Director(), %s(", classname);
    
    // Call base class constructor with parameters
    if (l) {
      int first = 1;
      for (Parm *p = l; p; p = nextSibling(p)) {
        if (!first) Printf(f_directors, ", ");
        String *pn = Getattr(p, "name");
        Printf(f_directors, "%s", pn);
        first = 0;
      }
    }
    Printf(f_directors, "), swig_rust_director_(nullptr) {\n");
    Printf(f_directors, "}\n\n");
    
    Delete(dirclassname);
    return Language::classDirectorConstructor(n);
  }

  /* ----------------------------------------------------------------------------- 
   * classDirectorMethod()
   * 
   * Generate director method for virtual function.
   * This generates:
   *   - C++ override that calls back to Rust
   *   - Rust callback FFI declaration
   *   - Rust Director trait method
   * ----------------------------------------------------------------------------- */
  int classDirectorMethod(Node *n, Node *parent, String *super) {
    String *classname = Getattr(parent, "sym:name");
    String *dirclassname = directorClassName(parent);
    String *name = Getattr(n, "name");
    String *symname = Getattr(n, "sym:name");
    SwigType *returntype = Getattr(n, "type");
    ParmList *l = Getattr(n, "parms");
    
    bool is_void = (Cmp(returntype, "void") == 0);
    bool pure_virtual = checkAttribute(n, "storage", "virtual") && checkAttribute(n, "value", "0");
    
    // Generate C++ method declaration
    Printf(f_directors_h, "    virtual ");
    String *ret_str = SwigType_str(returntype, 0);
    Printf(f_directors_h, "%s %s(", ret_str, name);
    
    // Add parameters to declaration
    if (l) {
      int first = 1;
      for (Parm *p = l; p; p = nextSibling(p)) {
        if (!first) Printf(f_directors_h, ", ");
        String *pt = SwigType_str(Getattr(p, "type"), 0);
        String *pn = Getattr(p, "name");
        Printf(f_directors_h, "%s %s", pt, pn);
        Delete(pt);
        first = 0;
      }
    }
    Printf(f_directors_h, ")");
    
    // Check for const method
    if (SwigType_isconst(Getattr(n, "decl"))) {
      Printf(f_directors_h, " const");
    }
    
    if (pure_virtual) {
      Printf(f_directors_h, " = 0;\n");
    } else {
      Printf(f_directors_h, " override;\n");
    }
    
    // Generate C++ method implementation (only for non-pure virtual)
    if (!pure_virtual) {
      Printf(f_directors, "%s %s::%s(", ret_str, dirclassname, name);
      
      // Add parameters to implementation
      if (l) {
        int first = 1;
        for (Parm *p = l; p; p = nextSibling(p)) {
          if (!first) Printf(f_directors, ", ");
          String *pt = SwigType_str(Getattr(p, "type"), 0);
          String *pn = Getattr(p, "name");
          Printf(f_directors, "%s %s", pt, pn);
          Delete(pt);
          first = 0;
        }
      }
      Printf(f_directors, ")");
      
      if (SwigType_isconst(Getattr(n, "decl"))) {
        Printf(f_directors, " const");
      }
      Printf(f_directors, " {\n");
      
      // Check if Rust has overridden this method
      Printf(f_directors, "    if (swig_rust_director_) {\n");
      
      // Call Rust callback
      String *callback_name = NewStringf("%s_%s_callback", dirclassname, name);
      
      if (!is_void) {
        Printf(f_directors, "        %s result;\n", ret_str);
        Printf(f_directors, "        if (%s(swig_rust_director_, &result", callback_name);
      } else {
        Printf(f_directors, "        %s(swig_rust_director_", callback_name);
      }
      
      // Add parameters to callback
      if (l) {
        for (Parm *p = l; p; p = nextSibling(p)) {
          String *pn = Getattr(p, "name");
          Printf(f_directors, ", %s", pn);
        }
      }
      
      if (!is_void) {
        Printf(f_directors, ")) {\n");
        Printf(f_directors, "            return result;\n");
        Printf(f_directors, "        }\n");
      } else {
        Printf(f_directors, ");\n");
      }
      Printf(f_directors, "    }\n");
      
      // Call base class implementation as fallback
      if (!pure_virtual) {
        Printf(f_directors, "    return %s::%s(", classname, name);
        if (l) {
          int first = 1;
          for (Parm *p = l; p; p = nextSibling(p)) {
            if (!first) Printf(f_directors, ", ");
            String *pn = Getattr(p, "name");
            Printf(f_directors, "%s", pn);
            first = 0;
          }
        }
        Printf(f_directors, ");\n");
      }
      Printf(f_directors, "}\n\n");
      
      // Generate C callback function declaration
      Printf(f_directors_h, "\n// Rust callback for %s::%s\n", dirclassname, name);
      Printf(f_directors_h, "extern \"C\" bool %s(void *director", callback_name);
      if (!is_void) {
        Printf(f_directors_h, ", %s *result", ret_str);
      }
      if (l) {
        for (Parm *p = l; p; p = nextSibling(p)) {
          String *pt = SwigType_str(Getattr(p, "type"), 0);
          String *pn = Getattr(p, "name");
          Printf(f_directors_h, ", %s %s", pt, pn);
          Delete(pt);
        }
      }
      Printf(f_directors_h, ");\n\n");
      
      Delete(callback_name);
    }
    
    // Generate Rust Director trait method
    // Determine self type based on const-ness
    String *self_type = SwigType_isconst(Getattr(n, "decl")) ? NewString("&self") : NewString("&self");
    
    Printf(f_wrapper_code, "    fn %s(%s", name, self_type);
    
    // Add parameters
    if (l) {
      for (Parm *p = l; p; p = nextSibling(p)) {
        String *pn = Getattr(p, "name");
        SwigType *pt = Getattr(p, "type");
        String *rust_type = getRustUserType(pt);
        Printf(f_wrapper_code, ", %s: %s", pn, rust_type);
      }
    }
    
    Printf(f_wrapper_code, ")");
    
    // Return type
    if (!is_void) {
      String *rust_ret = getRustUserType(returntype);
      Printf(f_wrapper_code, " -> Option<%s>", rust_ret);
    }
    
    Printf(f_wrapper_code, ";\n");
    
    Delete(ret_str);
    Delete(self_type);
    Delete(dirclassname);
    
    return SWIG_OK;
  }

  /* Data members */
  File *f_begin;
  File *f_runtime;
  File *f_runtime_h;
  File *f_header;
  File *f_wrappers;
  File *f_init;
  File *f_directors;
  File *f_directors_h;

  // Rust-specific output
  File *f_ffi_begin;
  File *f_ffi_imports;
  File *f_ffi_code;
  File *f_wrapper_begin;
  File *f_wrapper_code;

  String *crate_name;
  String *module_name;
  String *namespce;                 // Optional namespace name (from -namespace option)
  String *current_nspace;           // Current namespace being processed

  bool safe_wrapper_flag;
  bool ffi_only_flag;
  bool directors_flag;
  bool proxy_flag;
  bool static_flag;                 // Flag for static member functions
  bool variable_wrapper_flag;       // Flag for variable wrapper

  String *class_name;
  Node *class_node;

  String *proxy_class_def;
  String *proxy_class_code;
  String *proxy_class_constants_code;
  String *module_class_code;
  String *module_class_constants_code;

  // Inheritance support
  String *baseclass;                // Base class name for inheritance
  bool derived_flag;                // Whether current class is derived
  String *upcasts_code;             // C++ casts for inheritance hierarchies

  // Director support
  List *dmethods_seq;               // Sequence of director methods
  Hash *dmethods_table;             // Table of director methods
  int n_dmethods;                   // Number of director methods
  int n_directors;                  // Number of director classes
  int first_class_dmethod;          // First method index for current class
  int curr_class_dmethod;           // Current method index

  Hash *swig_types_hash;
  List *filenames_list;
};

/* -----------------------------------------------------------------------------
 * swig_rust()
 * 
 * Factory function to create a RUST language module instance.
 * ----------------------------------------------------------------------------- */
extern "C" Language *swig_rust(void) {
  return new RUST();
}
