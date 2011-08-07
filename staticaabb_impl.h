// File created: 2011-08-06 17:54:41

#include <stdbool.h>

#include "typenames.h"
#include "coords.h"

#if MUSHSPACE_93
	#undef STATICAABB_SIZE_X
	#undef STATICAABB_SIZE_Y
	#undef STATICAABB_BEG

	// Need these separately for defining the static array.
	#define STATICAABB_SIZE_X 80
	#define STATICAABB_SIZE_Y 25

	#define STATICAABB_BEG MUSHCOORDS(0,0,0)
#else
	#undef STATICAABB_SIZE_X
	#undef STATICAABB_SIZE_Y
	#undef STATICAABB_SIZE_Z
	#undef STATICAABB_BEG

	// We want the static area to cover the size of a "typical" program
	// (whatever that is) and have power-of-two width and height for fast
	// indexing calculations.
	//
	// We don't want it to be too big, not only for the sake of memory usage,
	// but also due at least to get_*_bounds.
	#define STATICAABB_SIZE_X 128
	#define STATICAABB_SIZE_Y 512
	#define STATICAABB_SIZE_Z   3

	#define STATICAABB_BEG MUSHCOORDS(-16, -16, -1)
#endif

#define STATICAABB_SIZE MUSHCOORDS(STATICAABB_SIZE_X, STATICAABB_SIZE_Y,\
                                   STATICAABB_SIZE_Z)

#define STATICAABB_END MUSHCOORDS(STATICAABB_BEG.x + STATICAABB_SIZE.x - 1,\
                                  STATICAABB_BEG.y + STATICAABB_SIZE.y - 1,\
                                  STATICAABB_BEG.z + STATICAABB_SIZE.z - 1)

typedef struct mush_staticaabb {

#if MUSHSPACE_DIM == 1
		mushcell array[STATICAABB_SIZE_X];
#elif MUSHSPACE_DIM == 2
		mushcell array[STATICAABB_SIZE_X*STATICAABB_SIZE_Y];
#elif MUSHSPACE_DIM == 3
		mushcell array[STATICAABB_SIZE_X*STATICAABB_SIZE_Y*STATICAABB_SIZE_Z];
#endif

} mush_staticaabb;

#define mush_staticaabb_contains MUSHSPACE_CAT(mush_staticaabb,_contains)
#define mush_staticaabb_get      MUSHSPACE_CAT(mush_staticaabb,_get)

bool mush_staticaabb_contains(mushcoords);

mushcell mush_staticaabb_get(mush_staticaabb*, mushcoords);
