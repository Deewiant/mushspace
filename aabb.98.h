// File created: 2011-08-06 16:12:02

#ifndef MUSHSPACE_AABB_H
#define MUSHSPACE_AABB_H

#include <stdlib.h>

#include "bounds.all.h"
#include "coords.all.h"
#include "typenames.any.h"

#define mush_aabb MUSHSPACE_NAME(mush_aabb)

typedef struct mush_aabb {
	mush_bounds bounds;
	mushcell *data;
	size_t size;

#if MUSHSPACE_DIM >= 2
	size_t width;
#endif
#if MUSHSPACE_DIM >= 3
	size_t area;
#endif
} mush_aabb;

#define mush_aabb_make              MUSHSPACE_CAT(mush_aabb,_make)
#define mush_aabb_make_unsafe       MUSHSPACE_CAT(mush_aabb,_make_unsafe)
#define mush_aabb_finalize          MUSHSPACE_CAT(mush_aabb,_finalize)
#define mush_aabb_alloc             MUSHSPACE_CAT(mush_aabb,_alloc)
#define mush_aabb_volume_on         MUSHSPACE_CAT(mush_aabb,_volume_on)
#define mush_aabb_get               MUSHSPACE_CAT(mush_aabb,_get)
#define mush_aabb_put               MUSHSPACE_CAT(mush_aabb,_put)
#define mush_aabb_get_no_offset     MUSHSPACE_CAT(mush_aabb,_get_no_offset)
#define mush_aabb_put_no_offset     MUSHSPACE_CAT(mush_aabb,_put_no_offset)
#define mush_aabb_getter_no_offset  MUSHSPACE_CAT(mush_aabb,_getter_no_offset)
#define mush_aabb_get_idx           MUSHSPACE_CAT(mush_aabb,_get_idx)
#define mush_aabb_get_idx_no_offset MUSHSPACE_CAT(mush_aabb,_get_idx_no_offset)
#define mush_aabb_can_direct_copy   MUSHSPACE_CAT(mush_aabb,_can_direct_copy)
#define mush_aabb_space_area        MUSHSPACE_CAT(mush_aabb,_space_area)
#define mush_aabb_can_direct_copy_area \
	MUSHSPACE_CAT(mush_aabb,_can_direct_copy_area)

void mush_aabb_make       (mush_aabb*, const mush_bounds*);
void mush_aabb_make_unsafe(mush_aabb*, const mush_bounds*);
void mush_aabb_finalize   (mush_aabb*);
bool mush_aabb_alloc      (mush_aabb*);

size_t mush_aabb_volume_on(const mush_aabb*, mushdim);

mushcell mush_aabb_get(const mush_aabb*, mushcoords);
void     mush_aabb_put(      mush_aabb*, mushcoords, mushcell);

mushcell mush_aabb_get_no_offset(const mush_aabb*, mushcoords);
void     mush_aabb_put_no_offset(      mush_aabb*, mushcoords, mushcell);

mushcell mush_aabb_getter_no_offset(const void*, mushcoords);

size_t mush_aabb_get_idx          (const mush_aabb*, mushcoords);
size_t mush_aabb_get_idx_no_offset(const mush_aabb*, mushcoords);

bool mush_aabb_can_direct_copy(const mush_aabb*, const mush_aabb*);
bool mush_aabb_can_direct_copy_area(
	const mush_aabb*, const mush_aabb*, const mush_aabb*);

void mush_aabb_space_area(mush_aabb*, const mush_aabb*);

#endif
