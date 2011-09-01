// File created: 2011-08-06 16:12:18

#ifndef MUSHSPACE_STATICAABB_H
#define MUSHSPACE_STATICAABB_H

#include <stdbool.h>
#include <stdlib.h>

#include "bounds.all.h"
#include "coords.all.h"
#include "typenames.any.h"

#define mush_staticaabb MUSHSPACE_NAME(mush_staticaabb)

#ifdef MUSHSPACE_93
	// These need to be separate instead of MUSHCOORDS so that
	// MUSH_STATICAABB_BOUNDS can be defined as a global constant (BEG) and so
	// that we can define the static array (SIZE).
	//
	// (Evidently (mushcoords){{.x = 1}}.x isn't a constant expression.)

	#define MUSH_STATICAABB_SIZE_X 80
	#define MUSH_STATICAABB_SIZE_Y 25

	#define MUSH_STATICAABB_BEG_X 0
	#define MUSH_STATICAABB_BEG_Y 0
	#define MUSH_STATICAABB_BEG_Z 0
#else
	// We want the static area to cover the size of a "typical" program
	// (whatever that is) and have power-of-two width and height for fast
	// indexing calculations.
	//
	// We don't want it to be too big, not only for the sake of memory usage,
	// but also due at least to get_*_bounds.
	#define MUSH_STATICAABB_SIZE_X 128
	#define MUSH_STATICAABB_SIZE_Y 512
	#define MUSH_STATICAABB_SIZE_Z   3

	#define MUSH_STATICAABB_BEG_X -16
	#define MUSH_STATICAABB_BEG_Y -16
	#define MUSH_STATICAABB_BEG_Z -1
#endif

#define MUSH_STATICAABB_BEG_INIT MUSHCOORDS_INIT(MUSH_STATICAABB_BEG_X,\
                                                 MUSH_STATICAABB_BEG_Y,\
                                                 MUSH_STATICAABB_BEG_Z)

#define MUSH_STATICAABB_END_INIT \
	MUSHCOORDS_INIT(MUSH_STATICAABB_BEG_X + MUSH_STATICAABB_SIZE_X - 1,\
	                MUSH_STATICAABB_BEG_Y + MUSH_STATICAABB_SIZE_Y - 1,\
	                MUSH_STATICAABB_BEG_Z + MUSH_STATICAABB_SIZE_Z - 1)

#define MUSH_STATICAABB_SIZE MUSHCOORDS(MUSH_STATICAABB_SIZE_X,\
                                        MUSH_STATICAABB_SIZE_Y,\
                                        MUSH_STATICAABB_SIZE_Z)

#define MUSH_STATICAABB_BEG (mushcoords)MUSH_STATICAABB_BEG_INIT
#define MUSH_STATICAABB_END (mushcoords)MUSH_STATICAABB_END_INIT

typedef struct mush_staticaabb {

		mushcell array[
			  MUSH_STATICAABB_SIZE_X
#if MUSHSPACE_DIM == 2
			* MUSH_STATICAABB_SIZE_Y
#if MUSHSPACE_DIM == 3
			* MUSH_STATICAABB_SIZE_Z
#endif
#endif
		];

} mush_staticaabb;

#define MUSH_STATICAABB_BOUNDS \
	MUSHSPACE_CAT(MUSHSPACE_NAME(MUSH_STATICAABB),_BOUNDS)

#define mush_staticaabb_contains MUSHSPACE_CAT(mush_staticaabb,_contains)
#define mush_staticaabb_get      MUSHSPACE_CAT(mush_staticaabb,_get)
#define mush_staticaabb_put      MUSHSPACE_CAT(mush_staticaabb,_put)
#define mush_staticaabb_get_idx  MUSHSPACE_CAT(mush_staticaabb,_get_idx)
#define mush_staticaabb_get_idx_no_offset \
	MUSHSPACE_CAT(mush_staticaabb,_get_idx_no_offset)

extern const mush_bounds MUSH_STATICAABB_BOUNDS;

bool mush_staticaabb_contains(mushcoords);

mushcell mush_staticaabb_get(const mush_staticaabb*, mushcoords);
void     mush_staticaabb_put(      mush_staticaabb*, mushcoords, mushcell);

size_t mush_staticaabb_get_idx          (mushcoords);
size_t mush_staticaabb_get_idx_no_offset(mushcoords);

#endif
