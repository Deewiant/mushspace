// File created: 2011-08-06 16:12:02

#ifndef MUSHSPACE_AABB_H
#define MUSHSPACE_AABB_H

#include <stdlib.h>

#include "coords.all.h"
#include "typenames.any.h"

#define mush_aabb MUSHSPACE_NAME(mush_aabb)

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

#define mush_aabb_make        MUSHSPACE_CAT(mush_aabb,_make)
#define mush_aabb_make_unsafe MUSHSPACE_CAT(mush_aabb,_make_unsafe)
#define mush_aabb_finalize    MUSHSPACE_CAT(mush_aabb,_finalize)
#define mush_aabb_alloc       MUSHSPACE_CAT(mush_aabb,_alloc)

#define mush_aabb_volume_on MUSHSPACE_CAT(mush_aabb,_volume_on)

#define mush_aabb_contains      MUSHSPACE_CAT(mush_aabb,_contains)
#define mush_aabb_contains_box  MUSHSPACE_CAT(mush_aabb,_contains_box)
#define mush_aabb_safe_contains MUSHSPACE_CAT(mush_aabb,_safe_contains)

#define mush_aabb_get MUSHSPACE_CAT(mush_aabb,_get)
#define mush_aabb_put MUSHSPACE_CAT(mush_aabb,_put)

#define mush_aabb_get_idx           MUSHSPACE_CAT(mush_aabb,_get_idx)
#define mush_aabb_get_idx_no_offset MUSHSPACE_CAT(mush_aabb,_get_idx_no_offset)

#define mush_aabb_overlaps MUSHSPACE_CAT(mush_aabb,_overlaps)

#define mush_aabb_on_same_axis MUSHSPACE_CAT(mush_aabb,_on_same_axis)
#define mush_aabb_on_same_primary_axis \
	MUSHSPACE_CAT(mush_aabb,_on_same_primary_axis)

#define mush_aabb_consume      MUSHSPACE_CAT(mush_aabb,_consume)
#define mush_aabb_subsume      MUSHSPACE_CAT(mush_aabb,_subsume)
#define mush_aabb_subsume_area MUSHSPACE_CAT(mush_aabb,_subsume_area)
#define mush_aabb_space_area   MUSHSPACE_CAT(mush_aabb,_space_area)

#define mush_aabb_tessellate  MUSHSPACE_CAT(mush_aabb,_tessellate)
#define mush_aabb_tessellate1 MUSHSPACE_CAT(mush_aabb,_tessellate1)

void mush_aabb_make       (mush_aabb*, mushcoords, mushcoords);
void mush_aabb_make_unsafe(mush_aabb*, mushcoords, mushcoords);
void mush_aabb_finalize   (mush_aabb*);
bool mush_aabb_alloc      (mush_aabb*);

size_t mush_aabb_volume_on(const mush_aabb*, mushdim);

bool mush_aabb_contains    (const mush_aabb*, mushcoords);
bool mush_aabb_contains_box(const mush_aabb*, const mush_aabb*);

// Works even if the AABB is an unsafe box with beg > end.
bool mush_aabb_safe_contains(const mush_aabb*, mushcoords);

mushcell mush_aabb_get(const mush_aabb*, mushcoords);
void     mush_aabb_put(      mush_aabb*, mushcoords, mushcell);

size_t mush_aabb_get_idx          (const mush_aabb*, mushcoords);
size_t mush_aabb_get_idx_no_offset(const mush_aabb*, mushcoords);

bool mush_aabb_overlaps(const mush_aabb*, const mush_aabb*);

#if MUSHSPACE_DIM > 1
bool mush_aabb_on_same_axis        (const mush_aabb*, const mush_aabb*);
bool mush_aabb_on_same_primary_axis(const mush_aabb*, const mush_aabb*);
#endif

// box should be unallocated and old allocated.
//
// box takes ownership of old's array. old must be contained within box.
bool mush_aabb_consume(mush_aabb* box, mush_aabb* old);

void mush_aabb_subsume     (mush_aabb*, const mush_aabb*);
void mush_aabb_subsume_area(mush_aabb*, const mush_aabb*, const mush_aabb*);
void mush_aabb_space_area  (mush_aabb*, const mush_aabb*);

// Modifies the given beg/end pair to give a box which contains the given
// coordinates but doesn't overlap with any of the given boxes. The coordinates
// should, of course, be already contained between the beg and end.
void mush_aabb_tessellate(
	mushcoords, const mush_aabb*, size_t, mushcoords*, mushcoords*);

// Since the algorithm is currently just a fold over the boxes, this simpler
// version exists to avoid heap allocation in some cases.
void mush_aabb_tessellate1(
	mushcoords, mushcoords, mushcoords, mushcoords*, mushcoords*);

#endif
