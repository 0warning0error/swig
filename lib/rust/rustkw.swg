#ifndef RUST_RUSTKW_SWG_
#define RUST_RUSTKW_SWG_

/* Warnings for Rust keywords */
/* Rust 2018+ allows raw identifiers with r# prefix, but for simplicity
   we rename by adding underscore suffix (like C# does) */

#define RUSTKW(x) %keywordwarn("'" `x` "' is a Rust keyword",rename="%s_")  `x`

/*
   Rust keywords from:
   https://doc.rust-lang.org/reference/keywords.html
   
   Strict keywords - cannot be used as identifiers
*/

RUSTKW(as);
RUSTKW(break);
RUSTKW(const);
RUSTKW(continue);
RUSTKW(crate);
RUSTKW(else);
RUSTKW(enum);
RUSTKW(extern);
RUSTKW(false);
RUSTKW(fn);
RUSTKW(for);
RUSTKW(if);
RUSTKW(impl);
RUSTKW(in);
RUSTKW(let);
RUSTKW(loop);
RUSTKW(match);
RUSTKW(mod);
RUSTKW(move);
RUSTKW(mut);
RUSTKW(pub);
RUSTKW(ref);
RUSTKW(return);
/* self and Self are handled specially in rust.cxx cleanRustName() */
/* RUSTKW(self); */
/* RUSTKW(Self); */
RUSTKW(static);
RUSTKW(struct);
RUSTKW(super);
RUSTKW(trait);
RUSTKW(true);
RUSTKW(type);
RUSTKW(unsafe);
RUSTKW(use);
RUSTKW(where);
RUSTKW(while);

/*
   Reserved keywords - reserved for future use
*/

RUSTKW(abstract);
RUSTKW(become);
RUSTKW(box);
RUSTKW(do);
RUSTKW(final);
RUSTKW(macro);
RUSTKW(override);
RUSTKW(priv);
RUSTKW(typeof);
RUSTKW(unsized);
RUSTKW(virtual);
RUSTKW(yield);

/* 
   Async/await keywords (Rust 2018+)
*/
RUSTKW(async);
RUSTKW(await);
RUSTKW(dyn);

#undef RUSTKW

#endif //RUST_RUSTKW_SWG_
