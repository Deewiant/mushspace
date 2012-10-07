// File created: 2012-10-07 20:21:27

#ifndef COORDS_H
#define COORDS_H

#include <mush/coords.h>

#if MUSHSPACE_93
#define MUSHCOORDS(a,b,c)      MUSHCOORDS93(a,b)
#define MUSHCOORDS_INIT(a,b,c) MUSHCOORDS93_INIT(a,b)
#elif MUSHSPACE_DIM == 1
#define MUSHCOORDS(a,b,c)      MUSHCOORDS1(a)
#define MUSHCOORDS_INIT(a,b,c) MUSHCOORDS1_INIT(a)
#elif MUSHSPACE_DIM == 2
#define MUSHCOORDS(a,b,c)      MUSHCOORDS2(a,b)
#define MUSHCOORDS_INIT(a,b,c) MUSHCOORDS2_INIT(a,b)
#elif MUSHSPACE_DIM == 3
#define MUSHCOORDS             MUSHCOORDS3
#define MUSHCOORDS_INIT        MUSHCOORDS3_INIT
#endif

#endif
