// File created: 2011-08-06 15:59:05

// Because we can have foo.c -> includer.h -> bar.h -> includer.h -> baz.h
// chains, we have to push_macro everything here (and INCLUDEE itself in foo.c
// and bar.h).

#pragma push_macro("MUSHSPACE_STRUCT_SUFFIX")
#pragma push_macro("MUSHSPACE_93")
#pragma push_macro("MUSHSPACE_DIM")

#undef MUSHSPACE_STRUCT_SUFFIX
#undef MUSHSPACE_93
#undef MUSHSPACE_DIM
#define MUSHSPACE_STRUCT_SUFFIX
#define MUSHSPACE_93  false
#define MUSHSPACE_DIM 1
#include INCLUDEE

#undef MUSHSPACE_DIM
#define MUSHSPACE_DIM 2
#include INCLUDEE

#undef MUSHSPACE_DIM
#define MUSHSPACE_DIM 3
#include INCLUDEE

#undef MUSHSPACE_STRUCT_SUFFIX
#undef MUSHSPACE_93
#undef MUSHSPACE_DIM
#define MUSHSPACE_STRUCT_SUFFIX _93
#define MUSHSPACE_93  true
#define MUSHSPACE_DIM 2
#include INCLUDEE

#pragma pop_macro("INCLUDEE")
#pragma pop_macro("MUSHSPACE_STRUCT_SUFFIX")
#pragma pop_macro("MUSHSPACE_93")
#pragma pop_macro("MUSHSPACE_DIM")
