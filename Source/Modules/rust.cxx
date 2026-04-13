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
     -trait-overload     - Use trait-based overload resolution (Rust-idiomatic)\n\
     -director-vtable    - Use associated const vtable for directors (default, zero overhead)\n\
     -director-thin      - Use thin vtable for directors (Box<dyn Trait> based)\n\
     -director-boxed     - Use Box<Box<dyn Trait>> for directors (simpler)\n\
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
    trait_overload_flag(false),
    director_thin_flag(false),  // Thin vtable mode
    director_vtable_flag(true), // Default: Associated const vtable mode (zero overhead, requires Rust 1.20+)
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
    director_callbacks(NULL),
    director_rust_callbacks(NULL),
    director_vtable_fields(NULL),
    swig_types_hash(NULL),
    filenames_list(NULL),
    class_method_names(NULL) {
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
        } else if (strcmp(argv[i], "-trait-overload") == 0) {
          Swig_mark_arg(i);
          trait_overload_flag = true;
        } else if (strcmp(argv[i], "-director-thin") == 0) {
          Swig_mark_arg(i);
          director_thin_flag = true;
          director_vtable_flag = false;
        } else if (strcmp(argv[i], "-director-boxed") == 0) {
          Swig_mark_arg(i);
          director_thin_flag = false;
          director_vtable_flag = false;
        } else if (strcmp(argv[i], "-director-vtable") == 0) {
          Swig_mark_arg(i);
          director_vtable_flag = true;
          director_thin_flag = false;  // vtable mode is mutually exclusive with thin/boxed
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

    // Initialize output files first
    f_runtime = NewString("");
    f_init = NewString("");
    f_header = NewString("");
    f_wrappers = NewString("");
    f_directors_h = NewString("");
    f_directors = NewString("");
    f_runtime_h = NULL;  // Will be set if directors enabled

    // Rust-specific output sections
    f_ffi_begin = NewString("");
    f_ffi_imports = NewString("");
    f_ffi_code = NewString("");
    f_wrapper_begin = NewString("");
    f_wrapper_code = NewString("");

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

    // Director support: add upcall variable for director methods
    // This is needed when CWRAP_DIRECTOR_TWO_CALLS is used, which generates
    // code like: if (upcall) { Base::method(); } else { method(); }
    bool director_method = is_member && GetFlag(n, "virtual") && Swig_directors_enabled();
    bool is_director_class = false;
    if (is_member && Swig_directors_enabled()) {
      // Check if the class has director enabled
      Node *parent = Getattr(n, "parentNode");
      if (parent && GetFlag(parent, "feature:director")) {
        is_director_class = true;
      }
    }
    
    if (is_director_class && !is_constructor && !is_destructor) {
      // Add director pointer and upcall variable
      Node *parent = Getattr(n, "parentNode");
      String *dirname = directorClassName(parent);
      Wrapper_add_local(f, "director", "Swig::Director *director = SWIG_DIRECTOR_CAST(arg1)");
      Wrapper_add_local(f, "upcall", "bool upcall = false");
      Printf(f->code, "upcall = (director != nullptr);\n");
    }

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
    
    // Initialize method name set for collision detection
    class_method_names = NewHash();
    collectClassMethodNames(n);
    
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

    // Generate trait-based overload resolution if enabled
    if (trait_overload_flag) {
      Hash *overload_info = buildOverloadInfo(n);
      emitOverloadTraits(n, overload_info);
      emitOverloadTraitImpls(n, overload_info);
      
      // Generate entry points in a separate impl block
      if (First(overload_info).key) {
        Printf(f_wrapper_code, "impl %s {\n", class_name);
        emitOverloadEntryPoints(n, overload_info);
        Printf(f_wrapper_code, "}\n\n");
      }
      
      Delete(overload_info);
    }

    // Generate upcast functions for additional base classes (multiple inheritance)
    if (baselist && Len(baselist) > 1) {
      emitUpcasts(n, baselist);
    }

    // Close namespace mod if needed
    addCloseMod(current_nspace, f_wrapper_code);

    class_name = NULL;
    class_node = NULL;
    
    // Clean up method name set
    if (class_method_names) {
      Delete(class_method_names);
      class_method_names = NULL;
    }
    
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
    Language::variableHandler(n);
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
   * For class-scoped enums, generates independent names like ClassName_EnumName.
   * ----------------------------------------------------------------------------- */
  virtual int enumDeclaration(Node *n) {
    String *name = Getattr(n, "sym:name");
    String *nspace = Getattr(n, "sym:nspace");
    
    // Check if this is a class-scoped enum
    Node *current_class = getCurrentClass();
    String *class_prefix = NULL;
    if (current_class) {
      String *class_name = Getattr(current_class, "sym:name");
      if (class_name) {
        class_prefix = Copy(class_name);
      }
    }
    
    // Skip anonymous enums (names starting with $ or empty)
    if (!name || Len(name) == 0 || Strstr(name, "$")) {
      // For anonymous enums, generate constants instead
      int anon_value = 0;
      for (Node *child = firstChild(n); child; child = nextSibling(child)) {
        // Note: SWIG uses "enumitem" as the node type for enum values
        if (Strcmp(nodeType(child), "enumitem") == 0) {
          String *vname = Getattr(child, "sym:name");
          String *value = Getattr(child, "enumvalue");
          if (vname) {
            // Open namespace mod if needed
            addOpenMod(nspace, f_wrapper_code);
            
            // Use explicit value if provided, otherwise use auto-increment
            if (value && Len(value) > 0) {
              Printf(f_wrapper_code, "pub const %s: i32 = %s;\n", vname, value);
            } else {
              Printf(f_wrapper_code, "pub const %s: i32 = %d;\n", vname, anon_value);
            }
            anon_value++;
            
            // Close namespace mod if needed
            addCloseMod(nspace, f_wrapper_code);
          }
        }
      }
      // Don't call Language::enumDeclaration for same reason as regular enums
      if (class_prefix) Delete(class_prefix);
      return SWIG_OK;
    }
    
    // Count enum values
    // Note: SWIG uses "enumitem" as the node type for enum values, not "enumvalue"
    int value_count = 0;
    for (Node *child = firstChild(n); child; child = nextSibling(child)) {
      if (Strcmp(nodeType(child), "enumitem") == 0) {
        value_count++;
      }
    }
    
    // Skip empty enums
    if (value_count == 0) {
      // Don't call Language::enumDeclaration for same reason
      if (class_prefix) Delete(class_prefix);
      return SWIG_OK;
    }
    
    // Generate the enum name
    // For class-scoped enums, prefix with class name (e.g., EnumClass_Status)
    String *enum_name;
    if (class_prefix) {
      enum_name = NewStringf("%s_%s", class_prefix, name);
    } else {
      enum_name = Copy(name);
    }
    
    // Store the generated Rust enum name for later use in type references
    Setattr(n, "rust:enumname", enum_name);
    
    // Open namespace mod if needed
    addOpenMod(nspace, f_wrapper_code);
    
    // Generate Rust enum
    Printf(f_wrapper_code, "#[repr(C)]\n");
    Printf(f_wrapper_code, "#[derive(Debug, Copy, Clone, PartialEq, Eq)]\n");
    Printf(f_wrapper_code, "pub enum %s {\n", enum_name);
    
    // Process enum values
    // Note: SWIG uses "enumitem" as the node type for enum values
    for (Node *child = firstChild(n); child; child = nextSibling(child)) {
      if (Strcmp(nodeType(child), "enumitem") == 0) {
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
    
    // Note: We don't call Language::enumDeclaration(n) here because:
    // 1. We already handled enum values in the Rust enum above
    // 2. Calling base class would trigger enumvalueDeclaration -> constantWrapper
    //    which generates incorrect Rust constants like "pub const RED: i32 = RED;"
    // 
    // If we need the base class processing for other reasons (symbol table, etc.),
    // we can selectively call specific base class methods.
    
    // Cleanup
    Delete(enum_name);
    if (class_prefix) Delete(class_prefix);
    
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
   * For custom types (classes/structs), returns the wrapper type name.
   * ----------------------------------------------------------------------------- */
  String *getRustUserType(SwigType *t) {
    if (!t) return NewString("*mut c_void");
    
    // Check for pointer/reference types first - these become opaque pointers
    if (SwigType_ispointer(t) || SwigType_isreference(t)) {
      return NewString("*mut c_void");
    }
    
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
      case T_USER:
        // For user-defined types (classes/structs), return the type name as wrapper
        {
          String *base = SwigType_base(t);
          String *clean = cleanTypeName(base);
          Delete(base);
          return clean;
        }
      default:
        // For other unknown types, try to get the base name
        {
          String *base = SwigType_base(t);
          if (base && Len(base) > 0) {
            String *clean = cleanTypeName(base);
            Delete(base);
            return clean;
          }
          if (base) Delete(base);
          return NewString("*mut c_void");
        }
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
   * For class-scoped types like "EnumClass::Status", replaces "::" with "_".
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
    
    // Replace C++ scope separator "::" with "_" for class-scoped types
    // e.g., "EnumClass::Status" -> "EnumClass_Status"
    if (Strstr(result, "::")) {
      Replaceall(result, "::", "_");
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
      Printf(f_rust, "use std::os::raw::*;\n\n");  // Import types needed by wrapper code
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
  /* ----------------------------------------------------------------------------- 
   * collectClassMethodNames()
   * 
   * Collect all method names in a class for collision detection.
   * This includes member functions and static member functions.
   * ----------------------------------------------------------------------------- */
  void collectClassMethodNames(Node *class_node) {
    if (!class_node) return;
    
    for (Node *child = firstChild(class_node); child; child = nextSibling(child)) {
      String *node_type = nodeType(child);
      
      // Check for member functions (cdecl with function decl)
      if (Strcmp(node_type, "cdecl") == 0) {
        String *decl = Getattr(child, "decl");
        if (decl && SwigType_isfunction(decl)) {
          String *mname = Getattr(child, "sym:name");
          if (mname) {
            Setattr(class_method_names, mname, "1");
          }
        }
      }
      // Also check for constructors and destructors
      else if (Strcmp(node_type, "constructor") == 0 || Strcmp(node_type, "destructor") == 0) {
        String *mname = Getattr(child, "sym:name");
        if (mname) {
          Setattr(class_method_names, mname, "1");
        }
      }
    }
  }

  /* ----------------------------------------------------------------------------- 
   * getUniqueMethodName()
   * 
   * Get a unique method name that doesn't conflict with existing methods.
   * Naming priority for getter: field -> get_field -> field_var
   * Naming priority for setter: set_field -> field_set -> set_field_var
   * ----------------------------------------------------------------------------- */
  String *getUniqueMethodName(String *field_name, bool is_getter) {
    if (!field_name || !class_method_names) {
      return is_getter ? Copy(field_name) : NewStringf("set_%s", field_name);
    }
    
    String *candidate = NULL;
    
    if (is_getter) {
      // Try: field
      candidate = Copy(field_name);
      if (!Getattr(class_method_names, candidate)) {
        return candidate;
      }
      Delete(candidate);
      
      // Try: get_field
      candidate = NewStringf("get_%s", field_name);
      if (!Getattr(class_method_names, candidate)) {
        return candidate;
      }
      Delete(candidate);
      
      // Try: field_var
      candidate = NewStringf("%s_var", field_name);
      if (!Getattr(class_method_names, candidate)) {
        return candidate;
      }
      Delete(candidate);
      
      // Fallback: field_getter
      candidate = NewStringf("%s_getter", field_name);
      return candidate;
    } else {
      // Setter
      // Try: set_field
      candidate = NewStringf("set_%s", field_name);
      if (!Getattr(class_method_names, candidate)) {
        return candidate;
      }
      Delete(candidate);
      
      // Try: field_set
      candidate = NewStringf("%s_set", field_name);
      if (!Getattr(class_method_names, candidate)) {
        return candidate;
      }
      Delete(candidate);
      
      // Try: set_field_var
      candidate = NewStringf("set_%s_var", field_name);
      if (!Getattr(class_method_names, candidate)) {
        return candidate;
      }
      Delete(candidate);
      
      // Fallback: field_setter
      candidate = NewStringf("%s_setter", field_name);
      return candidate;
    }
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
    bool is_member_set = GetFlag(n, "memberset");
    bool is_member_get = GetFlag(n, "memberget");
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
    // BUT: member variable accessors (getter/setter) should NOT be skipped
    if (is_member && !is_static && !is_member_set && !is_member_get) {
      return;
    }
    
    // IMPORTANT: Check if the first parameter is a self pointer (pointer to a class type)
    // This is the most reliable way to detect member functions in SWIG
    // For member functions, the first parameter is the "this" pointer
    // BUT: member variable accessors (getter/setter) should be processed, not skipped
    if (l && !is_static && !is_member_set && !is_member_get) {
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
    // Also for member variable accessors
    String *class_impl_name = NULL;
    if (is_member && is_static) {
      class_impl_name = Getattr(n, "parent:sym:name");
      if (!class_impl_name) {
        class_impl_name = Getattr(Getattr(n, "parentNode"), "sym:name");
      }
    }
    
    // For member variable accessors, get class name and field name
    // Function names are like "ClassName_field_get" or "ClassName_field_set"
    String *field_name = NULL;
    if (is_member_get || is_member_set) {
      // Get class name from parent node
      class_impl_name = Getattr(n, "parent:sym:name");
      if (!class_impl_name) {
        class_impl_name = Getattr(Getattr(n, "parentNode"), "sym:name");
      }
      
      // Extract field name from symname
      // Format: ClassName_field_get or ClassName_field_set
      if (symname && class_impl_name) {
        String *prefix = NewStringf("%s_", class_impl_name);
        if (Strstr(symname, prefix) == Char(symname)) {
          // Remove class prefix
          String *rest = NewString(Char(symname) + Len(prefix));
          // Remove _get or _set suffix
          int len = Len(rest);
          if (len > 4) {
            const char *end = Char(rest) + len - 4;
            if (strcmp(end, "_get") == 0 || strcmp(end, "_set") == 0) {
              field_name = NewStringf("%.*s", len - 4, Char(rest));
            }
          }
          Delete(rest);
        }
        Delete(prefix);
      }
    }

    // Determine function name - handle overloads and naming collisions
    String *func_name = NULL;
    
    // For member variable accessors, use collision-aware naming
    if ((is_member_get || is_member_set) && field_name) {
      func_name = getUniqueMethodName(field_name, is_member_get);
    } else {
      func_name = Copy(symname);
    }
    
    String *overname = Getattr(n, "sym:overname");
    
    // If this is an overloaded function, add type suffix
    // BUT: for member variable accessors, we should NOT add overload suffix
    if (overname && Len(overname) > 0 && !is_member_get && !is_member_set) {
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
      // Static member function or member variable accessor - generate inside impl block
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

    // For member variable accessors, first parameter is self
    if (is_member_get || is_member_set) {
      Printf(f_wrapper_code, "&self");
      arg_num = 1;
      // Skip the first parameter (self pointer)
      if (p && Getattr(p, "tmap:in:next")) {
        p = Getattr(p, "tmap:in:next");
      } else if (p) {
        p = nextSibling(p);
      }
    }

    // Process remaining parameters for signature
    for (int i = arg_num; i < num_arguments && p; i++) {
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
    
    // For member variable accessors, first parameter is self.ptr
    if (is_member_get || is_member_set) {
      Printf(f_wrapper_code, "self.ptr");
      arg_num = 1;
      // Skip the first parameter (self pointer)
      if (p && Getattr(p, "tmap:in:next")) {
        p = Getattr(p, "tmap:in:next");
      } else if (p) {
        p = nextSibling(p);
      }
    }

    for (int i = arg_num; i < num_arguments && p; i++) {
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
    if (field_name) Delete(field_name);
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
      } else if (Strstr(type_str, "double")) {
        // Check double BEFORE int (since "double" contains "int" substring)
        Printf(simple_type, "_f64");
      } else if (Strstr(type_str, "float")) {
        Printf(simple_type, "_f32");
      } else if (Strstr(type_str, "int")) {
        Printf(simple_type, "_int");
      } else if (Strstr(type_str, "char")) {
        Printf(simple_type, "_str");
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
          // Use the same const detection logic as emitRustImpl for consistency
          bool is_const_method = false;
          SwigType *child_type = Getattr(child, "type");
          if (child_type && SwigType_isconst(child_type)) {
            is_const_method = true;
          }
          if (decl && Strstr(decl, "r.q(const)")) {
            is_const_method = true;
          }
          
          String *self_type = is_const_method ? NewString("&self") : NewString("&mut self");

          // Check if this method is overloaded
          int total_count = GetInt(method_counts, mname);
          int current_index = GetInt(method_indices, mname);
          SetInt(method_indices, mname, current_index + 1);
          
          // Skip overloaded methods if using trait-based overload resolution
          // They will be handled by the unified entry point
          if (trait_overload_flag && total_count > 1) {
            continue;
          }
          
          // Generate method name with suffix if overloaded (legacy mode)
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

          // Add parameters
          // Note: For class member functions, the raw params don't include the 'this' pointer
          // So we don't need to skip the first parameter
          int num_params = 0;
          for (Parm *p = params; p; p = nextSibling(p)) {
            String *pname = Getattr(p, "name");
            SwigType *ptype = Getattr(p, "type");
            
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
            
            // Add comma before parameter
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
            if (!ret_type || Len(ret_type) == 0 || Strstr(ret_type, "$")) {
              // Typemap not found or contains unresolved variables
              // Use our own type resolution
              ret_type = getRustUserType(mtype);
            } else {
              // Check if the type string looks invalid (e.g., "PODType *" with space)
              if (Strstr(ret_type, " ")) {
                // Invalid Rust type, use fallback
                ret_type = getRustUserType(mtype);
              } else {
                // Process the type (handle SWIGENUM etc.)
                ret_type = processRustType(ret_type, mtype);
              }
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
          
          // Skip overloaded methods if using trait-based overload resolution
          if (trait_overload_flag && total_count > 1) {
            continue;
          }
          
          // Generate method name with suffix if overloaded (legacy mode)
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
            if (!ret_type || Len(ret_type) == 0 || Strstr(ret_type, "$")) {
              // Typemap not found or contains unresolved variables
              ret_type = getRustUserType(mtype);
            } else if (Strstr(ret_type, " ")) {
              // Invalid Rust type (e.g., "PODType *")
              ret_type = getRustUserType(mtype);
            } else {
              ret_type = processRustType(ret_type, mtype);
            }
            if (ret_type && Len(ret_type) > 0) {
              Printf(f_wrapper_code, " -> %s", ret_type);
              has_return = true;
            }
          }

          Printf(f_wrapper_code, " {\n");
          
          // Generate FFI call
          // For custom return types, we need to wrap the result
          Printf(f_wrapper_code, "        ");
          
          if (has_return) {
            // Check if return type is a custom type (not basic type)
            SwigType *return_type = Getattr(child, "type");
            bool is_custom_type = false;
            String *return_type_name = NULL;
            
            if (return_type && SwigType_type(return_type) == T_USER) {
              is_custom_type = true;
              return_type_name = getRustUserType(return_type);
            }
            
            if (is_custom_type && return_type_name) {
              // Wrap the pointer result in the struct
              Printf(f_wrapper_code, "%s { ptr: unsafe { ffi::%s(self.ptr", return_type_name, wname);
              
              // Add parameters
              for (Parm *p = params; p; p = nextSibling(p)) {
                String *pname = Getattr(p, "name");
                Printf(f_wrapper_code, ", %s", pname ? pname : "arg");
              }
              
              Printf(f_wrapper_code, ") } }\n");
              Delete(return_type_name);
            } else {
              // Basic type return - just call FFI
              Printf(f_wrapper_code, "unsafe { ffi::%s(self.ptr", wname);
              
              // Add parameters
              for (Parm *p = params; p; p = nextSibling(p)) {
                String *pname = Getattr(p, "name");
                Printf(f_wrapper_code, ", %s", pname ? pname : "arg");
              }
              
              Printf(f_wrapper_code, ") }\n");
            }
          } else {
            // No return value
            Printf(f_wrapper_code, "unsafe { ffi::%s(self.ptr", wname);
            
            // Add parameters
            for (Parm *p = params; p; p = nextSibling(p)) {
              String *pname = Getattr(p, "name");
              Printf(f_wrapper_code, ", %s", pname ? pname : "arg");
            }
            
            Printf(f_wrapper_code, ") }\n");
          }
          
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
   * emitOverloadTraits()
   * 
   * Generate trait definitions for overloaded methods (Trait-based overload resolution).
   * For a method like: void bar(int), void bar(const char*)
   * Generates: pub trait Foo_bar { type Output; fn call(self, foo: &mut Foo) -> Self::Output; }
   * ----------------------------------------------------------------------------- */
  void emitOverloadTraits(Node *n, Hash *overload_info) {
    String *class_name = Getattr(n, "sym:name");
    
    // Iterate over all overloaded method names
    for (Iterator it = First(overload_info); it.key; it = Next(it)) {
      String *method_name = it.key;
      List *overloads = (List *) it.item;
      
      // Skip if not overloaded (only one variant)
      if (Len(overloads) <= 1) continue;
      
      // Generate trait for this overloaded method
      // Trait name: ClassName_methodName
      Printf(f_wrapper_code, "/// Trait for overload resolution of %s::%s\n", class_name, method_name);
      Printf(f_wrapper_code, "pub trait %s_%s {\n", class_name, method_name);
      Printf(f_wrapper_code, "    type Output;\n");
      Printf(f_wrapper_code, "    fn call(self, obj: &mut %s) -> Self::Output;\n", class_name);
      Printf(f_wrapper_code, "}\n\n");
    }
  }

  /* ----------------------------------------------------------------------------- 
   * emitOverloadTraitImpls()
   * 
   * Generate trait implementations for each overload variant.
   * For: void bar(int x) -> impl Foo_bar for i32 { ... }
   * For: void bar(const char* s) -> impl Foo_bar for &str { ... }
   * For: void bar(int x, int y) -> impl Foo_bar for (i32, i32) { ... }
   * ----------------------------------------------------------------------------- */
  void emitOverloadTraitImpls(Node *n, Hash *overload_info) {
    String *class_name = Getattr(n, "sym:name");
    
    for (Iterator it = First(overload_info); it.key; it = Next(it)) {
      String *method_name = it.key;
      List *overloads = (List *) it.item;
      
      // Skip if not overloaded
      if (Len(overloads) <= 1) continue;
      
      // Generate impl for each overload
      for (Iterator oit = First(overloads); oit.item; oit = Next(oit)) {
        Node *method_node = (Node *) oit.item;
        
        ParmList *params = Getattr(method_node, "parms");
        SwigType *return_type = Getattr(method_node, "type");
        String *wname = Getattr(method_node, "wrap:name");
        
        // Determine the Rust type for this overload's parameters
        // Single parameter: use the parameter type directly
        // Multiple parameters: use a tuple type
        String *impl_type = NewString("");
        int param_count = 0;
        
        for (Parm *p = params; p; p = nextSibling(p)) {
          String *pname = Getattr(p, "name");
          SwigType *ptype = Getattr(p, "type");
          
          // Skip self parameter if present
          if (pname && (Cmp(pname, "self") == 0 || Cmp(pname, "this") == 0)) continue;
          if (SwigType_ispointer(ptype) && param_count == 0) {
            // First param might be self pointer, check type
            String *base = SwigType_base(ptype);
            if (Cmp(base, class_name) == 0) {
              Delete(base);
              continue;
            }
            Delete(base);
          }
          
          String *rust_type = getRustUserType(ptype);
          if (param_count > 0) {
            Append(impl_type, ", ");
          }
          Append(impl_type, rust_type);
          param_count++;
          Delete(rust_type);
        }
        
        // Generate impl block
        String *trait_name = NewStringf("%s_%s", class_name, method_name);
        
        if (param_count == 0) {
          // No parameters - use () unit type
          Printf(f_wrapper_code, "impl %s for () {\n", trait_name);
        } else if (param_count == 1) {
          // Single parameter - use the type directly
          Printf(f_wrapper_code, "impl %s for %s {\n", trait_name, impl_type);
        } else {
          // Multiple parameters - use tuple
          Printf(f_wrapper_code, "impl %s for (%s) {\n", trait_name, impl_type);
        }
        
        // Output type
        Printf(f_wrapper_code, "    type Output = ");
        if (return_type && SwigType_type(return_type) != T_VOID) {
          String *rust_ret = getRustUserType(return_type);
          Printf(f_wrapper_code, "%s", rust_ret);
          Delete(rust_ret);
        } else {
          Printf(f_wrapper_code, "()");
        }
        Printf(f_wrapper_code, ";\n");
        
        // call method
        Printf(f_wrapper_code, "    fn call(self, obj: &mut %s) -> Self::Output {\n", class_name);
        Printf(f_wrapper_code, "        unsafe { ffi::%s(obj.ptr", wname);
        
        // Add parameters
        if (param_count == 0) {
          // No parameters
        } else if (param_count == 1) {
          // Single parameter - use self directly
          Printf(f_wrapper_code, ", self");
        } else {
          // Multiple parameters - unpack tuple
          for (int i = 0; i < param_count; i++) {
            Printf(f_wrapper_code, ", self.%d", i);
          }
        }
        
        Printf(f_wrapper_code, ") }\n");
        Printf(f_wrapper_code, "    }\n");
        Printf(f_wrapper_code, "}\n\n");
        
        Delete(impl_type);
        Delete(trait_name);
      }
    }
  }

  /* ----------------------------------------------------------------------------- 
   * emitOverloadEntryPoints()
   * 
   * Generate unified entry point methods for overloaded functions.
   * For: void bar(int), void bar(const char*)
   * Generates: pub fn bar<P: Foo_bar>(&mut self, p: P) -> P::Output { p.call(self) }
   * ----------------------------------------------------------------------------- */
  void emitOverloadEntryPoints(Node *n, Hash *overload_info) {
    String *class_name = Getattr(n, "sym:name");
    
    for (Iterator it = First(overload_info); it.key; it = Next(it)) {
      String *method_name = it.key;
      List *overloads = (List *) it.item;
      
      // Skip if not overloaded
      if (Len(overloads) <= 1) continue;
      
      // Get info from first overload for const-ness
      Node *first_method = (Node *) Getitem(overloads, 0);
      String *decl = Getattr(first_method, "decl");
      bool is_const_method = (decl && Strstr(decl, "q(const)"));
      
      // Generate entry point
      String *trait_name = NewStringf("%s_%s", class_name, method_name);
      String *self_type = is_const_method ? NewString("&self") : NewString("&mut self");
      
      Printf(f_wrapper_code, "    /// Unified entry point for overloaded method %s\n", method_name);
      Printf(f_wrapper_code, "    pub fn %s<P: %s>(%s, p: P) -> P::Output {\n", method_name, trait_name, self_type);
      Printf(f_wrapper_code, "        p.call(self)\n");
      Printf(f_wrapper_code, "    }\n\n");
      
      Delete(trait_name);
      Delete(self_type);
    }
  }

  /* ----------------------------------------------------------------------------- 
   * buildOverloadInfo()
   * 
   * Build a hash table mapping method names to lists of overload nodes.
   * Returns a new Hash that must be deleted by caller.
   * ----------------------------------------------------------------------------- */
  Hash *buildOverloadInfo(Node *n) {
    Hash *overload_info = NewHash();
    
    for (Node *child = firstChild(n); child; child = nextSibling(child)) {
      if (Strcmp(nodeType(child), "cdecl") == 0) {
        if (GetFlag(child, "ismember") && !GetFlag(child, "static")) {
          String *decl = Getattr(child, "decl");
          if (decl && SwigType_isfunction(decl)) {
            String *mname = Getattr(child, "sym:name");
            if (mname) {
              List *overloads = Getattr(overload_info, mname);
              if (!overloads) {
                overloads = NewList();
                Setattr(overload_info, mname, overloads);
              }
              Append(overloads, child);
            }
          }
        }
      }
    }
    
    return overload_info;
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

    // Add parameters - use a more robust approach
    int arg_num = 0;
    if (l) {
      for (Parm *p = l; p; p = nextSibling(p)) {
        // Skip parameters with numinputs=0
        if (checkAttribute(p, "tmap:in:numinputs", "0")) {
          continue;
        }
        
        String *pname = Getattr(p, "name");
        String *ln = Getattr(p, "lname");
        SwigType *ptype = Getattr(p, "type");
        
        // Get Rust type - try typemap first, then use getRustUserType as fallback
        String *rust_type = Getattr(p, "tmap:rusttype");
        if (!rust_type || Len(rust_type) == 0) {
          // Fallback: generate based on SWIG type
          rust_type = getRustUserType(ptype);
        } else {
          // Process the type (handle SWIGENUM etc.)
          rust_type = processRustType(rust_type, ptype);
        }

        if (arg_num > 0) {
          Printf(f_wrapper_code, ", ");
        }
        
        // Use lname (which is set by SWIG) or pname, or generate a name
        String *arg_name = ln ? ln : (pname ? pname : NewStringf("arg%d", arg_num));
        Printf(f_wrapper_code, "%s: %s", arg_name, rust_type);
        arg_num++;
      }
    }

    Printf(f_wrapper_code, ") -> Self {\n");
    Printf(f_wrapper_code, "        Self {\n");
    
    // Generate FFI call with parameters
    Printf(f_wrapper_code, "            ptr: unsafe { ffi::%s(", wname);
    
    arg_num = 0;
    if (l) {
      for (Parm *p = l; p; p = nextSibling(p)) {
        // Skip parameters with numinputs=0
        if (checkAttribute(p, "tmap:in:numinputs", "0")) {
          continue;
        }
        
        String *pname = Getattr(p, "name");
        String *ln = Getattr(p, "lname");

        if (arg_num > 0) {
          Printf(f_wrapper_code, ", ");
        }
        
        // Use lname (which is set by SWIG) or pname
        String *arg_name = ln ? ln : (pname ? pname : NewStringf("arg%d", arg_num));
        Printf(f_wrapper_code, "%s", arg_name);
        arg_num++;
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
   * 
   * Supports per-class director mode configuration via features:
   *   %feature("director:vtable") ClassName;  // Use VTable mode for this class
   *   %feature("director:thin") ClassName;    // Use thin mode for this class
   *   %feature("director:boxed") ClassName;   // Use boxed mode for this class
   * ----------------------------------------------------------------------------- */
  int classDirectorInit(Node *n) {
    // Set up director constructor code
    Delete(director_ctor_code);
    director_ctor_code = NewString("$director_new");

    String *classname = Getattr(n, "sym:name");
    String *classtype = Getattr(n, "classtype");
    String *dirclassname = directorClassName(n);

    // Check for per-class director mode features (override global settings)
    bool use_vtable = director_vtable_flag;
    bool use_thin = director_thin_flag;
    
    if (GetFlag(n, "feature:director:vtable")) {
      use_vtable = true;
      use_thin = false;
    } else if (GetFlag(n, "feature:director:thin")) {
      use_vtable = false;
      use_thin = true;
    } else if (GetFlag(n, "feature:director:boxed")) {
      use_vtable = false;
      use_thin = false;
    }
    // Save the determined mode for use in other methods
    Setattr(n, "rust:director:vtable", use_vtable ? NewString("1") : NULL);
    Setattr(n, "rust:director:thin", use_thin ? NewString("1") : NULL);

    // Generate C++ director class declaration
    // VTable mode: no VTable struct in C++, just callback function pointers
    Printf(f_directors_h, "class %s : public %s, public Swig::Director {\n", dirclassname, classtype);
    Printf(f_directors_h, "public:\n");
    // Note: constructor and destructor declarations are handled by classDirectorConstructor and Language base class
    
    // Store director class name for later use
    Setattr(n, "director:name", dirclassname);
    
    // Initialize callback buffers for this class
    director_callbacks = NewString("");
    director_rust_callbacks = NewString("");
    director_vtable_fields = NewString("");
    director_vtable_inits = NewString("");
    director_vtable_thunks = NewString("");
    director_vtable_fields_cpp = NewString("");  // C++ version of VTable fields

    if (use_vtable) {
      // Associated const VTable mode: VTable struct will be generated in classDirectorEnd
      // after we know all the methods
    }

    // Generate Rust director support
    // Director trait for Rust implementations (generated first, before VTable)
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

    if (use_thin) {
      // Thin vtable mode: VTable will be generated in classDirectorEnd after trait is closed
    } else if (use_vtable) {
      // VTable mode: VTable struct and VTableProvider will be generated in classDirectorEnd
    } else {
      // Boxed mode: no explicit VTable, use Box<Box<dyn Trait>>
    }

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

    // Get per-class director mode settings (set in classDirectorInit)
    bool use_vtable = GetFlag(n, "rust:director:vtable");
    bool use_thin = GetFlag(n, "rust:director:thin");

    // Close the Director trait
    Printf(f_wrapper_code, "}\n\n");

    if (use_vtable) {
      // Associated const VTable mode
      // This mode uses Rust's associated constants to provide a static VTable
      // for each implementing type, achieving zero overhead abstraction
      
      // Generate VTable struct definition with all fields
      Printf(f_wrapper_code, "/// VTable for %s director callbacks\n", classname);
      Printf(f_wrapper_code, "/// Each field is a function pointer that calls the corresponding trait method\n");
      Printf(f_wrapper_code, "#[repr(C)]\n");
      Printf(f_wrapper_code, "pub struct %sVTable {\n", classname);
      // Output VTable fields
      if (director_vtable_fields && Len(director_vtable_fields) > 0) {
        Dump(director_vtable_fields, f_wrapper_code);
        Delete(director_vtable_fields);
        director_vtable_fields = NULL;
      }
      Printf(f_wrapper_code, "}\n\n");
      
      // Output VTable thunk functions
      if (director_vtable_thunks && Len(director_vtable_thunks) > 0) {
        Dump(director_vtable_thunks, f_wrapper_code);
        Delete(director_vtable_thunks);
        director_vtable_thunks = NULL;
      }
      
      // Generate VTableProvider trait with associated const VTABLE
      Printf(f_wrapper_code, "/// Trait that provides a static VTable for %sDirector implementations\n", classname);
      Printf(f_wrapper_code, "/// Each implementing type automatically gets a VTable through blanket impl\n");
      Printf(f_wrapper_code, "pub trait %sVTableProvider: %sDirector + Sized {\n", classname, classname);
      Printf(f_wrapper_code, "    const VTABLE: %sVTable = %sVTable {\n", classname, classname);
      // Generate VTable initializers - we need to iterate through method names again
      // The director_vtable_inits buffer was populated in classDirectorMethod
      if (director_vtable_inits && Len(director_vtable_inits) > 0) {
        Dump(director_vtable_inits, f_wrapper_code);
        Delete(director_vtable_inits);
        director_vtable_inits = NULL;
      }
      Printf(f_wrapper_code, "    };\n");
      Printf(f_wrapper_code, "}\n\n");
      
      // Blanket impl: all Sized types that implement Director get VTableProvider automatically
      Printf(f_wrapper_code, "/// Blanket implementation: all Sized types implementing %sDirector get VTableProvider\n", classname);
      Printf(f_wrapper_code, "impl<T: %sDirector + Sized> %sVTableProvider for T {}\n\n", classname, classname);
      
      // Generate new_with_vtable constructor
      Printf(f_wrapper_code, "impl %s {\n", classname);
      Printf(f_wrapper_code, "    /// Create a new %s with a Rust Director implementation using VTable\n", classname);
      Printf(f_wrapper_code, "    /// \n");
      Printf(f_wrapper_code, "    /// This uses associated constants for zero-overhead virtual dispatch.\n");
      Printf(f_wrapper_code, "    /// Requires Rust 1.20+\n");
      Printf(f_wrapper_code, "    pub fn new_with_vtable<D: %sDirector + Sized + 'static>(director: D) -> Self {\n", classname);
      Printf(f_wrapper_code, "        // Get the VTable through the VTableProvider trait\n");
      Printf(f_wrapper_code, "        let vtable: &'static %sVTable = &<D as %sVTableProvider>::VTABLE;\n", classname, classname);
      Printf(f_wrapper_code, "        // Box the director object\n");
      Printf(f_wrapper_code, "        let director_ptr = Box::into_raw(Box::new(director)) as *mut c_void;\n");
      Printf(f_wrapper_code, "        // Pass both vtable and director pointer to C++\n");
      Printf(f_wrapper_code, "        Self {\n");
      Printf(f_wrapper_code, "            ptr: unsafe { ffi::%s_new_director_vtable(vtable as *const %sVTable as *mut c_void, director_ptr) },\n", dirclassname, classname);
      Printf(f_wrapper_code, "        }\n");
      Printf(f_wrapper_code, "    }\n");
      Printf(f_wrapper_code, "}\n\n");
      
    } else if (use_thin) {
      // Thin mode: use Box<dyn Trait> pattern
      // Note: We don't use VTable because static VTable with generics is not allowed in Rust
      
      // Generate Director struct - holds the trait object
      Printf(f_wrapper_code, "/// Director struct for %s\n", classname);
      Printf(f_wrapper_code, "pub struct Director%s {\n", classname);
      Printf(f_wrapper_code, "    inner: Box<dyn %sDirector>,\n", classname);
      Printf(f_wrapper_code, "}\n\n");
      
      // Generate create_director helper function
      Printf(f_wrapper_code, "/// Create a Director struct from a %sDirector implementation\n", classname);
      Printf(f_wrapper_code, "pub fn create_director_%s<D: %sDirector + 'static>(d: D) -> Director%s {\n", classname, classname, classname);
      Printf(f_wrapper_code, "    Director%s {\n", classname);
      Printf(f_wrapper_code, "        inner: Box::new(d),\n");
      Printf(f_wrapper_code, "    }\n");
      Printf(f_wrapper_code, "}\n\n");
      
      // Generate new_with_trait using create_director
      Printf(f_wrapper_code, "impl %s {\n", classname);
      Printf(f_wrapper_code, "    /// Create a new %s with a Rust Director implementation\n", classname);
      Printf(f_wrapper_code, "    pub fn new_with_trait<D: %sDirector + 'static>(director: D) -> Self {\n", classname);
      Printf(f_wrapper_code, "        let d = create_director_%s(director);\n", classname);
      Printf(f_wrapper_code, "        let director_ptr = Box::into_raw(Box::new(d)) as *mut c_void;\n");
      Printf(f_wrapper_code, "        Self {\n");
      Printf(f_wrapper_code, "            ptr: unsafe { ffi::%s_new_director(director_ptr) },\n", dirclassname);
      Printf(f_wrapper_code, "        }\n");
      Printf(f_wrapper_code, "    }\n");
      Printf(f_wrapper_code, "}\n\n");
      
    } else {
      // Boxed mode: Box<Box<dyn Trait>> pattern
      // Generate new_with_trait constructor in Rust
      // Uses Box<Box<dyn Trait>> pattern: outer Box gives us a single thin pointer
      // that can be safely passed to C++, while inner Box holds the fat pointer (trait object)
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
      Printf(f_wrapper_code, "        // Box the trait object as Box<dyn %sDirector> (fat pointer)\n", classname);
      Printf(f_wrapper_code, "        let trait_obj: Box<dyn %sDirector> = Box::new(director);\n", classname);
      Printf(f_wrapper_code, "        // Box again to get a thin pointer we can pass to C++\n");
      Printf(f_wrapper_code, "        let director_ptr = Box::into_raw(Box::new(trait_obj)) as *mut c_void;\n");
      Printf(f_wrapper_code, "        Self {\n");
      Printf(f_wrapper_code, "            ptr: unsafe { ffi::%s_new_director(director_ptr) },\n", dirclassname);
      Printf(f_wrapper_code, "        }\n");
      Printf(f_wrapper_code, "    }\n");
      Printf(f_wrapper_code, "}\n\n");
    }

    // Close C++ director class
    // Note: swig_rust_director_ needs to be public for the new_director function to access it
    Printf(f_directors_h, "public:\n");
    
    // For VTable mode, save field names before outputting (needed for new_director_vtable)
    String *vtable_field_names = NULL;
    if (use_vtable && director_vtable_fields_cpp && Len(director_vtable_fields_cpp) > 0) {
      // Parse field names and save them
      vtable_field_names = NewString("");
      char *start = Char(director_vtable_fields_cpp);
      while (start && *start) {
        char *paren_star = strstr(start, "(*");
        if (!paren_star) break;
        paren_star += 2;
        char *close_paren = strchr(paren_star, ')');
        if (!close_paren) break;
        int name_len = close_paren - paren_star;
        if (Len(vtable_field_names) > 0) {
          Printf(vtable_field_names, ",");
        }
        Printf(vtable_field_names, "%.*s", name_len, paren_star);
        start = strchr(close_paren, '\n');
        if (start) start++;
      }
    }
    
    if (use_vtable) {
      // VTable mode: output callback function pointer members
      if (director_vtable_fields_cpp && Len(director_vtable_fields_cpp) > 0) {
        Dump(director_vtable_fields_cpp, f_directors_h);
        Delete(director_vtable_fields_cpp);
        director_vtable_fields_cpp = NULL;
      }
    }
    Printf(f_directors_h, "    void *swig_rust_director_;  // Pointer to Rust trait object\n");
    Printf(f_directors_h, "};\n\n");

    // Output callback function declarations after class definition
    // (only for non-VTable modes)
    if (!use_vtable) {
      if (director_callbacks && Len(director_callbacks) > 0) {
        Dump(director_callbacks, f_directors_h);
        Delete(director_callbacks);
        director_callbacks = NULL;
      }

      // Output Rust callback function implementations
      // These are the #[no_mangle] extern "C" functions that C++ will call
      if (director_rust_callbacks && Len(director_rust_callbacks) > 0) {
        Dump(director_rust_callbacks, f_wrapper_code);
        Delete(director_rust_callbacks);
        director_rust_callbacks = NULL;
      }
    } else {
      // VTable mode: clean up buffers (not used)
      if (director_callbacks) {
        Delete(director_callbacks);
        director_callbacks = NULL;
      }
      if (director_rust_callbacks) {
        Delete(director_rust_callbacks);
        director_rust_callbacks = NULL;
      }
    }

    // Generate the director new function(s)
    if (use_vtable) {
      // VTable mode: generate new_director_vtable function
      // The vtable parameter points to a Rust VTable struct with function pointers
      // We need to copy each function pointer to the corresponding C++ member
      Printf(f_directors, "extern \"C\" SWIGEXPORT void *%s_new_director_vtable(void *vtable, void *rust_director) {\n", dirclassname);
      Printf(f_directors, "    %s *director = new %s();\n", dirclassname, dirclassname);
      // Cast vtable to function pointer array and copy each entry
      // The Rust VTable struct is #[repr(C)] so the layout matches C function pointer array
      Printf(f_directors, "    // Copy function pointers from Rust VTable\n");
      Printf(f_directors, "    typedef void (*VTableFuncPtr)();\n");
      Printf(f_directors, "    VTableFuncPtr *entries = reinterpret_cast<VTableFuncPtr *>(vtable);\n");
      // Copy each function pointer from the VTable using saved field names
      if (vtable_field_names && Len(vtable_field_names) > 0) {
        int field_index = 0;
        String *names_copy = Copy(vtable_field_names);
        char *token = strtok(Char(names_copy), ",");
        while (token) {
          Printf(f_directors, "    director->%s = reinterpret_cast<decltype(director->%s)>(entries[%d]);\n", token, token, field_index);
          field_index++;
          token = strtok(NULL, ",");
        }
        Delete(names_copy);
      }
      Printf(f_directors, "    director->swig_rust_director_ = rust_director;\n");
      Printf(f_directors, "    return director;\n");
      Printf(f_directors, "}\n\n");
      if (vtable_field_names) {
        Delete(vtable_field_names);
        vtable_field_names = NULL;
      }
    } else {
      // Non-VTable modes: generate new_director function
      Printf(f_directors, "extern \"C\" SWIGEXPORT void *%s_new_director(void *rust_director) {\n", dirclassname);
      Printf(f_directors, "    %s *director = new %s();\n", dirclassname, dirclassname);
      Printf(f_directors, "    director->swig_rust_director_ = rust_director;\n");
      Printf(f_directors, "    return director;\n");
      Printf(f_directors, "}\n\n");
    }

    // Note: Destructor is generated by classDirectorDestructor, not here

    // Generate FFI declaration for director constructor and drop
    Printf(f_ffi_code, "    extern \"C\" {\n");
    if (use_vtable) {
      Printf(f_ffi_code, "        pub fn %s_new_director_vtable(vtable: *mut c_void, rust_director: *mut c_void) -> *mut c_void;\n", dirclassname);
    } else {
      Printf(f_ffi_code, "        pub fn %s_new_director(rust_director: *mut c_void) -> *mut c_void;\n", dirclassname);
    }
    Printf(f_ffi_code, "        pub fn %s_drop_director(rust_director: *mut c_void);\n", dirclassname);
    Printf(f_ffi_code, "    }\n\n");

    // Generate Rust drop function implementation
    if (use_vtable) {
      // VTable mode: drop the director object (Box<D>)
      Printf(f_wrapper_code, "// Drop function for %s director (called from C++ destructor)\n", classname);
      Printf(f_wrapper_code, "#[no_mangle]\n");
      Printf(f_wrapper_code, "pub unsafe extern \"C\" fn %s_drop_director(director: *mut c_void) {\n", dirclassname);
      Printf(f_wrapper_code, "    // Reconstruct the Box<D> and let it drop\n");
      Printf(f_wrapper_code, "    // Note: We don't know the concrete type D here, but we only need to free the memory\n");
      Printf(f_wrapper_code, "    // The actual Drop impl will be called when the Box is dropped\n");
      Printf(f_wrapper_code, "    // We use a helper to drop as Box<dyn Director> to invoke proper cleanup\n");
      Printf(f_wrapper_code, "    let _ = Box::from_raw(director as *mut ());\n");
      Printf(f_wrapper_code, "}\n\n");
    } else if (use_thin) {
      // Thin vtable mode: drop Director struct
      Printf(f_wrapper_code, "// Drop function for %s director (called from C++ destructor)\n", classname);
      Printf(f_wrapper_code, "#[no_mangle]\n");
      Printf(f_wrapper_code, "pub unsafe extern \"C\" fn %s_drop_director(director: *mut c_void) {\n", dirclassname);
      Printf(f_wrapper_code, "    // Reconstruct the Box<Director%s> and let it drop\n", classname);
      Printf(f_wrapper_code, "    let _ = Box::from_raw(director as *mut Director%s);\n", classname);
      Printf(f_wrapper_code, "}\n\n");
    } else {
      // Boxed mode: drop Box<Box<dyn Trait>>
      Printf(f_wrapper_code, "// Drop function for %s director (called from C++ destructor)\n", classname);
      Printf(f_wrapper_code, "#[no_mangle]\n");
      Printf(f_wrapper_code, "pub unsafe extern \"C\" fn %s_drop_director(director: *mut c_void) {\n", dirclassname);
      Printf(f_wrapper_code, "    // Reconstruct the Box<Box<dyn %sDirector>> and let it drop\n", classname);
      Printf(f_wrapper_code, "    // This frees both the outer Box and the inner Box (trait object)\n");
      Printf(f_wrapper_code, "    let _ = Box::from_raw(director as *mut Box<dyn %sDirector>);\n", classname);
      Printf(f_wrapper_code, "}\n\n");
    }

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
    // Get per-class director mode settings from parent node
    bool use_vtable = GetFlag(parent, "rust:director:vtable");
    
    if (l) {
      int first = 1;
      for (Parm *p = l; p; p = nextSibling(p)) {
        if (!first) Printf(f_directors, ", ");
        String *pn = Getattr(p, "name");
        Printf(f_directors, "%s", pn);
        first = 0;
      }
    }
    // Note: no swig_vtable_ member needed - function pointers are stored as individual members
    Printf(f_directors, "), swig_rust_director_(nullptr) {\n");
    Printf(f_directors, "}\n\n");
    
    Delete(dirclassname);
    return Language::classDirectorConstructor(n);
  }

  /* ----------------------------------------------------------------------------- 
   * classDirectorDestructor()
   * 
   * Generate destructor for director class.
   * This overrides the base class to add cleanup code for swig_rust_director_.
   * ----------------------------------------------------------------------------- */
  int classDirectorDestructor(Node *n) {
    Node *current_class = getCurrentClass();
    String *dirclassname = directorClassName(current_class);
    
    // Generate destructor declaration in header
    Printf(f_directors_h, "    virtual ~%s();\n", dirclassname);
    
    // Generate destructor implementation
    Printf(f_directors, "%s::~%s() {\n", dirclassname, dirclassname);
    Printf(f_directors, "    if (swig_rust_director_) {\n");
    Printf(f_directors, "        %s_drop_director(swig_rust_director_);\n", dirclassname);
    Printf(f_directors, "        swig_rust_director_ = nullptr;\n");
    Printf(f_directors, "    }\n");
    Printf(f_directors, "}\n\n");
    
    Delete(dirclassname);
    return SWIG_OK;
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
  int classDirectorMethods(Node *n) {
    return Language::classDirectorMethods(n);
  }

  int classDirectorMethod(Node *n, Node *parent, String *super) {
    String *classname = Getattr(parent, "sym:name");
    String *dirclassname = directorClassName(parent);
    String *name = Getattr(n, "name");
    String *symname = Getattr(n, "sym:name");
    SwigType *returntype = Getattr(n, "type");
    ParmList *l = Getattr(n, "parms");
    
    // Skip defaultargs shortened versions - only keep the longest version
    Node *defaultargs = Getattr(n, "defaultargs");
    if (defaultargs) {
      return SWIG_OK;
    }
    
    // Get per-class director mode settings (set in classDirectorInit)
    bool use_vtable = GetFlag(parent, "rust:director:vtable");
    bool use_thin = GetFlag(parent, "rust:director:thin");
    
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
    
    // In VTable mode, even pure virtual functions need implementation
    // because we need to call through VTable
    if (pure_virtual && !use_vtable) {
      Printf(f_directors_h, " = 0;\n");
    } else {
      Printf(f_directors_h, " override;\n");
    }
    
    // Generate C++ method implementation
    // For non-pure virtual OR for pure virtual in VTable mode
    if (!pure_virtual || use_vtable) {
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
      // For overloaded virtual functions, add a type suffix to the callback name
      String *callback_suffix = emitOverloadSuffix(l);
      String *callback_name = NewStringf("%s_%s%s_callback", dirclassname, name, callback_suffix);
      // Note: Don't Delete callback_suffix here - it's used later for VTable fields and thunks
      
      if (use_vtable) {
        // VTable mode: call the stored callback function pointer directly
        // The member variable name must match the field name in director_vtable_fields_cpp
        String *vtable_field_suffix = emitOverloadSuffix(l);
        String *callback_ptr_name = NewStringf("%s%s", name, vtable_field_suffix);
        
        if (!is_void) {
          Printf(f_directors, "        if (%s) {\n", callback_ptr_name);
          Printf(f_directors, "            %s result;\n", ret_str);
          Printf(f_directors, "            if (%s(swig_rust_director_, &result", callback_ptr_name);
          if (l) {
            for (Parm *p = l; p; p = nextSibling(p)) {
              String *pn = Getattr(p, "name");
              Printf(f_directors, ", %s", pn);
            }
          }
          Printf(f_directors, ")) {\n");
          Printf(f_directors, "                return result;\n");
          Printf(f_directors, "            }\n");
          Printf(f_directors, "        }\n");
        } else {
          // void return type
          Printf(f_directors, "        if (%s) {\n", callback_ptr_name);
          Printf(f_directors, "            if (%s(swig_rust_director_", callback_ptr_name);
          if (l) {
            for (Parm *p = l; p; p = nextSibling(p)) {
              String *pn = Getattr(p, "name");
              Printf(f_directors, ", %s", pn);
            }
          }
          Printf(f_directors, ")) {\n");
          Printf(f_directors, "                return;\n");
          Printf(f_directors, "            }\n");
          Printf(f_directors, "        }\n");
        }
        Delete(callback_ptr_name);
        Delete(vtable_field_suffix);
      } else {
        // Non-VTable modes: call the callback function directly
        if (!is_void) {
          Printf(f_directors, "        %s result;\n", ret_str);
          Printf(f_directors, "        if (%s(swig_rust_director_, &result", callback_name);
        } else {
          Printf(f_directors, "        if (%s(swig_rust_director_", callback_name);
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
          Printf(f_directors, ")) {\n");
          Printf(f_directors, "            return;\n");
          Printf(f_directors, "        }\n");
        }
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
      
      // Store callback declaration in buffer (will be output after class definition)
      // Skip in VTable mode - callbacks are generated as thunk functions instead
      if (!use_vtable) {
        Printf(director_callbacks, "// Rust callback for %s::%s\n", dirclassname, name);
        Printf(director_callbacks, "extern \"C\" bool %s(void *director", callback_name);
        if (!is_void) {
          Printf(director_callbacks, ", %s *result", ret_str);
        }
        if (l) {
          for (Parm *p = l; p; p = nextSibling(p)) {
            String *pt = SwigType_str(Getattr(p, "type"), 0);
            String *pn = Getattr(p, "name");
            Printf(director_callbacks, ", %s %s", pt, pn);
            Delete(pt);
          }
        }
        Printf(director_callbacks, ");\n\n");
        
        // Generate the #[no_mangle] callback function that C++ will call
        if (use_thin) {
          // Thin vtable mode: generate callback function for Director struct
          Printf(director_rust_callbacks, "// C callback for %s::%s%s (called from C++)\n", classname, name, callback_suffix);
          Printf(director_rust_callbacks, "#[no_mangle]\n");
          Printf(director_rust_callbacks, "pub unsafe extern \"C\" fn %s(director: *mut c_void", callback_name);
          if (!is_void) {
            String *rust_ret = getRustUserType(returntype);
            Printf(director_rust_callbacks, ", result: *mut %s", rust_ret);
            Delete(rust_ret);
          }
          if (l) {
            for (Parm *p = l; p; p = nextSibling(p)) {
              String *pn = Getattr(p, "name");
              SwigType *pt = Getattr(p, "type");
              String *rust_type = getRustUserType(pt);
              Printf(director_rust_callbacks, ", %s: %s", pn, rust_type);
              Delete(rust_type);
            }
          }
          Printf(director_rust_callbacks, ") -> bool {\n");
          Printf(director_rust_callbacks, "    // Get the Director struct from the director pointer\n");
          Printf(director_rust_callbacks, "    let d = &mut *(director as *mut Director%s);\n", classname);
          Printf(director_rust_callbacks, "    // Call the trait method\n");
          if (!is_void) {
            Printf(director_rust_callbacks, "    match d.inner.%s%s(", name, callback_suffix);
          } else {
            Printf(director_rust_callbacks, "    d.inner.%s%s(", name, callback_suffix);
          }
          if (l) {
            int first = 1;
            for (Parm *p = l; p; p = nextSibling(p)) {
              if (!first) Printf(director_rust_callbacks, ", ");
              String *pn = Getattr(p, "name");
              Printf(director_rust_callbacks, "%s", pn);
              first = 0;
            }
          }
          if (!is_void) {
            Printf(director_rust_callbacks, ") {\n");
            Printf(director_rust_callbacks, "        Some(v) => { *result = v; true }\n");
            Printf(director_rust_callbacks, "        None => false,\n");
            Printf(director_rust_callbacks, "    }\n");
          } else {
            Printf(director_rust_callbacks, ");\n");
            Printf(director_rust_callbacks, "    true\n");
          }
          Printf(director_rust_callbacks, "}\n\n");
        } else {
          // Boxed mode: generate standalone callback function
          // This is the function that C++ will call to invoke the Rust trait method
          Printf(director_rust_callbacks, "// Callback for %s::%s (called from C++)\n", classname, name);
          Printf(director_rust_callbacks, "#[no_mangle]\n");
          Printf(director_rust_callbacks, "pub unsafe extern \"C\" fn %s(director: *mut c_void", callback_name);
          if (!is_void) {
            String *rust_ret = getRustUserType(returntype);
            Printf(director_rust_callbacks, ", result: *mut %s", rust_ret);
            Delete(rust_ret);
          }
          if (l) {
            for (Parm *p = l; p; p = nextSibling(p)) {
              String *pn = Getattr(p, "name");
              SwigType *pt = Getattr(p, "type");
              String *rust_type = getRustUserType(pt);
              Printf(director_rust_callbacks, ", %s: %s", pn, rust_type);
              Delete(rust_type);
            }
          }
          Printf(director_rust_callbacks, ") -> bool {\n");
          Printf(director_rust_callbacks, "    // Safety: director pointer was created by new_with_trait\n");
          Printf(director_rust_callbacks, "    // and points to Box<Box<dyn %sDirector>> (outer Box gives thin pointer)\n", classname);
          Printf(director_rust_callbacks, "    let outer_box = director as *mut Box<dyn %sDirector>;\n", classname);
          Printf(director_rust_callbacks, "    let director_ref: &dyn %sDirector = &**outer_box;\n", classname);
          
          // Call the trait method
          if (!is_void) {
            Printf(director_rust_callbacks, "    match (*director_ref).%s%s(", name, callback_suffix);
          } else {
            Printf(director_rust_callbacks, "    (*director_ref).%s%s(", name, callback_suffix);
          }
          
          // Add arguments to the call
          if (l) {
            int first = 1;
            for (Parm *p = l; p; p = nextSibling(p)) {
              if (!first) Printf(director_rust_callbacks, ", ");
              String *pn = Getattr(p, "name");
              Printf(director_rust_callbacks, "%s", pn);
              first = 0;
            }
          }
          
          if (!is_void) {
            Printf(director_rust_callbacks, ") {\n");
            Printf(director_rust_callbacks, "        Some(value) => {\n");
            Printf(director_rust_callbacks, "            *result = value;\n");
            Printf(director_rust_callbacks, "            true\n");
            Printf(director_rust_callbacks, "        }\n");
            Printf(director_rust_callbacks, "        None => false,\n");
            Printf(director_rust_callbacks, "    }\n");
          } else {
            Printf(director_rust_callbacks, ");\n");
            Printf(director_rust_callbacks, "    true  // void methods always succeed\n");
          }
          
          Printf(director_rust_callbacks, "}\n\n");
        }  // end else (boxed mode)
      }  // end if (!use_vtable)
      
      Delete(callback_name);
      Delete(callback_suffix);
    }
    
    // Generate Rust Director trait method
    // Determine self type based on const-ness
    String *self_type = SwigType_isconst(Getattr(n, "decl")) ? NewString("&self") : NewString("&self");
    
    // For overloaded virtual methods, add type suffix to the trait method name
    String *trait_method_suffix = emitOverloadSuffix(l);
    Printf(f_wrapper_code, "    fn %s%s(%s", name, trait_method_suffix, self_type);
    Delete(trait_method_suffix);
    
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
    
    // Generate VTable support for associated const vtable mode
    if (use_vtable) {
      // Generate VTable field (function pointer type)
      // The signature: fn(*const c_void, args...) -> ret_type
      // For void: fn(*const c_void, args...) -> bool
      // For non-void: fn(*const c_void, *mut ret_type, args...) -> bool
      String *vtable_field_suffix = emitOverloadSuffix(l);
      String *vtable_field_name = NewStringf("%s%s", name, vtable_field_suffix);
      
      // Rust field definition
      Printf(director_vtable_fields, "    pub %s: unsafe extern \"C\" fn(*const c_void", vtable_field_name);
      if (!is_void) {
        String *rust_ret = getRustUserType(returntype);
        Printf(director_vtable_fields, ", *mut %s", rust_ret);
        Delete(rust_ret);
      }
      if (l) {
        for (Parm *p = l; p; p = nextSibling(p)) {
          SwigType *pt = Getattr(p, "type");
          String *rust_type = getRustUserType(pt);
          Printf(director_vtable_fields, ", %s", rust_type);
          Delete(rust_type);
        }
      }
      Printf(director_vtable_fields, ") -> bool,\n");
      
      // C++ field definition
      Printf(director_vtable_fields_cpp, "    bool (*%s)(void*", vtable_field_name);
      if (!is_void) {
        Printf(director_vtable_fields_cpp, ", %s*", ret_str);
      }
      if (l) {
        for (Parm *p = l; p; p = nextSibling(p)) {
          String *pt = SwigType_str(Getattr(p, "type"), 0);
          Printf(director_vtable_fields_cpp, ", %s", pt);
          Delete(pt);
        }
      }
      Printf(director_vtable_fields_cpp, ");\n");
      
      // VTable initializer (references the thunk function)
      Printf(director_vtable_inits, "        %s: %s_thunk_%s::<Self>,\n", vtable_field_name, classname, vtable_field_name);
      
      // Generate thunk function for this method
      // The thunk casts the void* to the concrete type and calls the trait method
      Printf(director_vtable_thunks, "/// Thunk function for %s::%s (VTable entry)\n", classname, vtable_field_name);
      Printf(director_vtable_thunks, "/// Casts the data pointer to type D and calls the trait method\n");
      Printf(director_vtable_thunks, "pub unsafe extern \"C\" fn %s_thunk_%s<D: %sDirector>(data: *const c_void", classname, vtable_field_name, classname);
      if (!is_void) {
        String *rust_ret = getRustUserType(returntype);
        Printf(director_vtable_thunks, ", result: *mut %s", rust_ret);
        Delete(rust_ret);
      }
      if (l) {
        for (Parm *p = l; p; p = nextSibling(p)) {
          String *pn = Getattr(p, "name");
          SwigType *pt = Getattr(p, "type");
          String *rust_type = getRustUserType(pt);
          Printf(director_vtable_thunks, ", %s: %s", pn, rust_type);
          Delete(rust_type);
        }
      }
      Printf(director_vtable_thunks, ") -> bool {\n");
      Printf(director_vtable_thunks, "    let obj = &*(data as *const D);\n");
      if (!is_void) {
        Printf(director_vtable_thunks, "    match obj.%s(", vtable_field_name);
      } else {
        Printf(director_vtable_thunks, "    obj.%s(", vtable_field_name);
      }
      if (l) {
        int first = 1;
        for (Parm *p = l; p; p = nextSibling(p)) {
          if (!first) Printf(director_vtable_thunks, ", ");
          String *pn = Getattr(p, "name");
          Printf(director_vtable_thunks, "%s", pn);
          first = 0;
        }
      }
      if (!is_void) {
        Printf(director_vtable_thunks, ") {\n");
        Printf(director_vtable_thunks, "        Some(v) => { *result = v; true }\n");
        Printf(director_vtable_thunks, "        None => false,\n");
        Printf(director_vtable_thunks, "    }\n");
      } else {
        Printf(director_vtable_thunks, ");\n");
        Printf(director_vtable_thunks, "    true\n");
      }
      Printf(director_vtable_thunks, "}\n\n");
      
      Delete(vtable_field_name);
      Delete(vtable_field_suffix);
    }
    
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
  bool trait_overload_flag;         // Flag for trait-based overload resolution
  bool director_thin_flag;          // Flag for thin vtable director (vs Box<Box<dyn Trait>>)
  bool director_vtable_flag;        // Flag for associated const vtable director (zero overhead)

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
  String *director_callbacks;       // Buffer for director callback declarations
  String *director_rust_callbacks;  // Buffer for Rust callback function implementations
  String *director_vtable_fields;   // Buffer for VTable fields (thin vtable mode)
  String *director_vtable_inits;    // Buffer for VTable initializers (thin vtable mode)
  String *director_vtable_thunks;   // Buffer for VTable thunk functions (associated const vtable mode)
  String *director_vtable_fields_cpp; // Buffer for C++ VTable fields (associated const vtable mode)

  Hash *swig_types_hash;
  List *filenames_list;
  
  // Member variable accessor naming support
  Hash *class_method_names;         // Set of method names in current class (for collision detection)
};

/* -----------------------------------------------------------------------------
 * swig_rust()
 * 
 * Factory function to create a RUST language module instance.
 * ----------------------------------------------------------------------------- */
extern "C" Language *swig_rust(void) {
  return new RUST();
}
