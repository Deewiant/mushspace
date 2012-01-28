// File created: 2011-08-21 16:17:06

#ifndef MUSHSPACE_BOUNDS_H
#define MUSHSPACE_BOUNDS_H

#include "coords.all.h"
#include "stdlib.any.h"
#include "typenames.any.h"

#define mushbounds MUSHSPACE_NAME(mushbounds)

typedef struct mushbounds {
	mushcoords beg, end;
} mushbounds;

MUSH_DECL_CONST_DYN_ARRAY(mushbounds)

#define mushbounds_clamped_size    MUSHSPACE_CAT(mushbounds,_clamped_size)
#define mushbounds_contains        MUSHSPACE_CAT(mushbounds,_contains)
#define mushbounds_safe_contains   MUSHSPACE_CAT(mushbounds,_safe_contains)
#define mushbounds_contains_bounds MUSHSPACE_CAT(mushbounds,_contains_bounds)
#define mushbounds_overlaps        MUSHSPACE_CAT(mushbounds,_overlaps)
#define mushbounds_get_overlap     MUSHSPACE_CAT(mushbounds,_get_overlap)
#define mushbounds_on_same_axis    MUSHSPACE_CAT(mushbounds,_on_same_axis)
#define mushbounds_can_fuse        MUSHSPACE_CAT(mushbounds,_can_fuse)
#define mushbounds_tessellate      MUSHSPACE_CAT(mushbounds,_tessellate)
#define mushbounds_tessellate1     MUSHSPACE_CAT(mushbounds,_tessellate1)
#define mushbounds_on_same_primary_axis \
	MUSHSPACE_CAT(mushbounds,_on_same_primary_axis)

size_t mushbounds_clamped_size(const mushbounds*);

// The safe version works even if the bounds have beg > end.
bool mushbounds_contains     (const mushbounds*, mushcoords);
bool mushbounds_safe_contains(const mushbounds*, mushcoords);

bool mushbounds_contains_bounds(const mushbounds*, const mushbounds*);
bool mushbounds_overlaps       (const mushbounds*, const mushbounds*);

#if !MUSHSPACE_93
bool mushbounds_get_overlap(const mushbounds*, const mushbounds*, mushbounds*);
#endif

#if MUSHSPACE_DIM > 1
bool mushbounds_on_same_axis        (const mushbounds*, const mushbounds*);
bool mushbounds_on_same_primary_axis(const mushbounds*, const mushbounds*);
#endif

#if !MUSHSPACE_93
// True if we can create a new AABB which covers exactly the two bounds given.
bool mushbounds_can_fuse(const mushbounds*, const mushbounds*);
#endif

// Modifies the given bounds to be such that they contain the given coordinates
// but don't overlap with any of the given boxes. The coordinates should, of
// course, be already contained between the beg and end.
void mushbounds_tessellate(mushbounds*, mushcoords, mushcarr_mushbounds);

// Since the algorithm is (currently) just a fold over the boxes, this simpler
// version exists to simplify usage in some cases.
//
// It requires that the given bounds overlap with the area to be avoided. This
// is because there's no sense in avoiding part of an axis just because there's
// a non-overlapping box there.
void mushbounds_tessellate1(mushbounds*, mushcoords, const mushbounds*);

#endif
