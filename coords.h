// File created: 2011-08-06 16:12:12

#ifndef MUSHSPACE_COORDS_H
#define MUSHSPACE_COORDS_H

#define MUSHCOORDS1(a,b,c) (mushcoords){{.x = a}}
#define MUSHCOORDS2(a,b,c) (mushcoords){{.x = a, .y = b}}
#define MUSHCOORDS3(a,b,c) (mushcoords){{.x = a, .y = b, .z = c}}

#pragma push_macro("INCLUDEE")
#undef INCLUDEE
#define INCLUDEE "coords_impl.h"
#include "includer.h"

#endif
