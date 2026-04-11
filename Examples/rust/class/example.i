/* File: example.i */
%module example

%{
#include "example.h"
%}

/* Include the class definitions */
%include "example.h"

/* Mark Shape as an abstract base class */
%feature("notabstract") Shape;

/* Directors needed for inheritance */
%feature("director") Shape;
