/* -------------------------------------------------------------
 * SWIG library containing argc and argv multi-argument typemaps
 * ------------------------------------------------------------- */

%typemap(rusttype) (int ARGC, char **ARGV) "[]string"

%insert(rust_wrapper) %{
//export cgo_swig_get_string_slice_idx
func cgo_swig_get_string_slice_idx(s []string, i C.swig_intrust) string {
    return s[i]
}
%}

%{
extern
#ifdef __cplusplus
  "C"
#endif
_gostring_ cgo_swig_get_string_slice_idx(_goslice_ s, intrust i);
%}

%typemap(in) (int ARGC, char **ARGV) {
  $1_ltype len = ($1_ltype)$input.len;
  size_t aralloc = (size_t)((len + 1) * sizeof(char *));
  if (len < 0) {
    _swig_rustpanic("negative array length");
  }
  $2 = ($2_ltype) Swig_malloc((int)aralloc);
  if ($2 == NULL) {
    _swig_rustpanic("fail allocating memory for array");
  }
  memset($2, 0, aralloc);
  $1 = len;
  {
    $1_ltype i;
    for (i = 0; i < len; i++) {
      char *p;
      _gostring_ st = cgo_swig_get_string_slice_idx($input, (intrust)i);
      if (st.n < 0) {
        _swig_rustpanic("string length negative");
      }
      p = (char *) Swig_malloc((int)(st.n + 1));
      if (p == NULL) {
        _swig_rustpanic("fail allocating memory for a string");
      }
      memcpy(p, st.p, st.n);
      p[st.n] = 0;
      $2[i] = p;
    }
    $2[i] = NULL;
  }
}

%typemap(freearg) (int ARGC, char **ARGV) {
  if ($2 != NULL) {
    $1_ltype i;
    for (i = 0; i < $1; i++) {
      Swig_free((void *)$2[i]);
    }
    Swig_free((void *)$2);
  }
}
