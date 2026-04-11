# Makefile for Rust simple example

SWIG = swig
SWIGOPT = -rust

all: example.rs

example.rs: example.i
	$(SWIG) $(SWIGOPT) example.i

clean:
	rm -f *.rs *_wrap.cxx

check: all
