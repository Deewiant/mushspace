// File created: 2011-08-06 17:40:27

#include <stdlib.h>

#include "typenames.h"

#include "coords.h"

typedef struct mush_aabb {
	mushcell *data;
	size_t size;
	mushcoords beg, end;

#if MUSHSPACE_DIM >= 2
	size_t width;
#endif
#if MUSHSPACE_DIM >= 3
	size_t area;
#endif
} mush_aabb;

mush_aabb MUSHSPACE_CAT(mush_aabb,_make)       (mushcoords, mushcoords);
mush_aabb MUSHSPACE_CAT(mush_aabb,_make_unsafe)(mushcoords, mushcoords);
void      MUSHSPACE_CAT(mush_aabb,_finalize)   (mush_aabb*);
