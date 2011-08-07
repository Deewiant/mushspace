// File created: 2011-08-06 16:12:18

#ifndef MUSHSPACE_STATICAABB_H
#define MUSHSPACE_STATICAABB_H

#include "coords.h"

// Need these separately for defining the static array.
#define STATICAABB_SIZE_X 128
#define STATICAABB_SIZE_Y 512
#define STATICAABB_SIZE_Z   3

#define STATICAABB_BEG  MUSHCOORDS( -16, -16, -1)
#define STATICAABB_SIZE MUSHCOORDS(STATICAABB_SIZE_X, STATICAABB_SIZE_Y,\
                                   STATICAABB_SIZE_Z)

#pragma push_macro("INCLUDEE")
#undef INCLUDEE
#define INCLUDEE "staticaabb_impl.h"
#include "includer.h"

#endif
