// File created: 2011-08-06 16:12:18

#ifndef MUSHSPACE_STATICAABB_H
#define MUSHSPACE_STATICAABB_H

#include <stdbool.h>
#include <stdlib.h>

#include "bounds.all.h"
#include "coords.all.h"
#include "typenames.any.h"

#define mushstaticaabb MUSHSPACE_NAME(mushstaticaabb)

#ifdef MUSHSPACE_93
	// These need to be separate instead of MUSHCOORDS so that
	// MUSHSTATICAABB_BOUNDS can be defined as a global constant (BEG) and so
	// that we can define the static array (SIZE).
	//
	// (Evidently (mushcoords){{.x = 1}}.x isn't a constant expression.)

	#define MUSHSTATICAABB_SIZE_X 80
	#define MUSHSTATICAABB_SIZE_Y 25

	#define MUSHSTATICAABB_BEG_X 0
	#define MUSHSTATICAABB_BEG_Y 0
	#define MUSHSTATICAABB_BEG_Z 0
#else
	// We want the static area to cover the size of a "typical" program
	// (whatever that is) and have power-of-two width and height for fast
	// indexing calculations.
	//
	// We don't want it to be too big, not only for the sake of memory usage,
	// but also due at least to get_*_bounds.
	#define MUSHSTATICAABB_SIZE_X 128
	#define MUSHSTATICAABB_SIZE_Y 512
	#define MUSHSTATICAABB_SIZE_Z   3

	#define MUSHSTATICAABB_BEG_X -16
	#define MUSHSTATICAABB_BEG_Y -16
	#define MUSHSTATICAABB_BEG_Z -1
#endif

#define MUSHSTATICAABB_BEG_INIT MUSHCOORDS_INIT(MUSHSTATICAABB_BEG_X,\
                                                MUSHSTATICAABB_BEG_Y,\
                                                MUSHSTATICAABB_BEG_Z)

#define MUSHSTATICAABB_END_INIT \
	MUSHCOORDS_INIT(MUSHSTATICAABB_BEG_X + MUSHSTATICAABB_SIZE_X - 1,\
	                MUSHSTATICAABB_BEG_Y + MUSHSTATICAABB_SIZE_Y - 1,\
	                MUSHSTATICAABB_BEG_Z + MUSHSTATICAABB_SIZE_Z - 1)

// size = end - beg + 1 = rel_end + 1
#define MUSHSTATICAABB_REL_END_INIT \
	MUSHCOORDS_INIT(MUSHSTATICAABB_SIZE_X - 1,\
	                MUSHSTATICAABB_SIZE_Y - 1,\
	                MUSHSTATICAABB_SIZE_Z - 1)

#define MUSHSTATICAABB_SIZE MUSHCOORDS(MUSHSTATICAABB_SIZE_X,\
                                        MUSHSTATICAABB_SIZE_Y,\
                                        MUSHSTATICAABB_SIZE_Z)

#define MUSHSTATICAABB_BEG (mushcoords)MUSHSTATICAABB_BEG_INIT
#define MUSHSTATICAABB_END (mushcoords)MUSHSTATICAABB_END_INIT

typedef struct mushstaticaabb {

		mushcell array[
			  MUSHSTATICAABB_SIZE_X
#if MUSHSPACE_DIM == 2
			* MUSHSTATICAABB_SIZE_Y
#if MUSHSPACE_DIM == 3
			* MUSHSTATICAABB_SIZE_Z
#endif
#endif
		];

} mushstaticaabb;

#define MUSHSTATICAABB_BOUNDS \
	MUSHSPACE_CAT(MUSHSPACE_NAME(MUSHSTATICAABB),_BOUNDS)
#define MUSHSTATICAABB_REL_BOUNDS \
	MUSHSPACE_CAT(MUSHSPACE_NAME(MUSHSTATICAABB),_REL_BOUNDS)

#define mushstaticaabb_contains MUSHSPACE_CAT(mushstaticaabb,_contains)
#define mushstaticaabb_get      MUSHSPACE_CAT(mushstaticaabb,_get)
#define mushstaticaabb_put      MUSHSPACE_CAT(mushstaticaabb,_put)
#define mushstaticaabb_get_idx  MUSHSPACE_CAT(mushstaticaabb,_get_idx)
#define mushstaticaabb_get_no_offset \
	MUSHSPACE_CAT(mushstaticaabb,_get_no_offset)
#define mushstaticaabb_put_no_offset \
	MUSHSPACE_CAT(mushstaticaabb,_put_no_offset)
#define mushstaticaabb_getter_no_offset \
	MUSHSPACE_CAT(mushstaticaabb,_getter_no_offset)
#define mushstaticaabb_get_idx_no_offset \
	MUSHSPACE_CAT(mushstaticaabb,_get_idx_no_offset)

extern const mushbounds MUSHSTATICAABB_BOUNDS, MUSHSTATICAABB_REL_BOUNDS;

bool mushstaticaabb_contains(mushcoords);

mushcell mushstaticaabb_get(const mushstaticaabb*, mushcoords);
void     mushstaticaabb_put(      mushstaticaabb*, mushcoords, mushcell);

mushcell mushstaticaabb_get_no_offset(const mushstaticaabb*, mushcoords);
void     mushstaticaabb_put_no_offset(mushstaticaabb*, mushcoords, mushcell);

mushcell mushstaticaabb_getter_no_offset(const void*, mushcoords);

size_t mushstaticaabb_get_idx          (mushcoords);
size_t mushstaticaabb_get_idx_no_offset(mushcoords);

#endif
