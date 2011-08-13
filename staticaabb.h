// File created: 2011-08-06 16:12:18

#ifndef MUSHSPACE_STATICAABB_H
#define MUSHSPACE_STATICAABB_H

#pragma push_macro("INCLUDEE")
#undef INCLUDEE
#define INCLUDEE "staticaabb_impl.h"
#include "includer.h"

// 93 uses this as well.
#pragma push_macro("MUSHSPACE_93")
#pragma push_macro("MUSHSPACE_DIM")
#pragma push_macro("MUSHSPACE_STRUCT_SUFFIX")

#undef MUSHSPACE_DIM
#undef MUSHSPACE_STRUCT_SUFFIX
#define MUSHSPACE_93
#define MUSHSPACE_DIM 2
#define MUSHSPACE_STRUCT_SUFFIX _93
#include INCLUDEE

#pragma pop_macro("MUSHSPACE_STRUCT_SUFFIX")
#pragma pop_macro("MUSHSPACE_DIM")
#pragma pop_macro("MUSHSPACE_93")

#pragma pop_macro("INCLUDEE")

#endif
