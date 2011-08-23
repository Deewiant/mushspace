// File created: 2011-08-21 16:17:06

#ifndef MUSHSPACE_BOUNDS_H
#define MUSHSPACE_BOUNDS_H

#include "coords.all.h"
#include "typenames.any.h"

#define mush_bounds MUSHSPACE_NAME(mush_bounds)

typedef struct mush_bounds {
	mushcoords beg, end;
} mush_bounds;

MUSH_DECL_CONST_DYN_ARRAY(mush_bounds)

#define mush_bounds_clamped_size    MUSHSPACE_CAT(mush_bounds,_clamped_size)
#define mush_bounds_contains        MUSHSPACE_CAT(mush_bounds,_contains)
#define mush_bounds_safe_contains   MUSHSPACE_CAT(mush_bounds,_safe_contains)
#define mush_bounds_contains_bounds MUSHSPACE_CAT(mush_bounds,_contains_bounds)
#define mush_bounds_overlaps        MUSHSPACE_CAT(mush_bounds,_overlaps)
#define mush_bounds_get_overlap     MUSHSPACE_CAT(mush_bounds,_get_overlap)
#define mush_bounds_on_same_axis    MUSHSPACE_CAT(mush_bounds,_on_same_axis)
#define mush_bounds_can_fuse        MUSHSPACE_CAT(mush_bounds,_can_fuse)
#define mush_bounds_tessellate      MUSHSPACE_CAT(mush_bounds,_tessellate)
#define mush_bounds_tessellate1     MUSHSPACE_CAT(mush_bounds,_tessellate1)

#define mush_bounds_on_same_primary_axis \
	MUSHSPACE_CAT(mush_bounds,_on_same_primary_axis)

size_t mush_bounds_clamped_size(const mush_bounds*);

// The safe version works even if the bounds have beg > end.
bool mush_bounds_contains     (const mush_bounds*, mushcoords);
bool mush_bounds_safe_contains(const mush_bounds*, mushcoords);

bool mush_bounds_contains_bounds(const mush_bounds*, const mush_bounds*);
bool mush_bounds_overlaps       (const mush_bounds*, const mush_bounds*);

#if !MUSHSPACE_93
bool mush_bounds_get_overlap(
	const mush_bounds*, const mush_bounds*, mush_bounds*);
#endif

#if MUSHSPACE_DIM > 1
bool mush_bounds_on_same_axis        (const mush_bounds*, const mush_bounds*);
bool mush_bounds_on_same_primary_axis(const mush_bounds*, const mush_bounds*);
#endif

#if !MUSHSPACE_93
// True if we can create a new AABB which covers exactly the two bounds given.
bool mush_bounds_can_fuse(const mush_bounds*, const mush_bounds*);
#endif

// Modifies the given bounds to be such that they contain the given coordinates
// but don't overlap with any of the given boxes. The coordinates should, of
// course, be already contained between the beg and end.
void mush_bounds_tessellate(
	mushcoords, const mush_bounds*, size_t, mush_bounds*);

// Since the algorithm is currently just a fold over the boxes, this simpler
// version exists to avoid heap allocation in some cases.
void mush_bounds_tessellate1(mushcoords, const mush_bounds*, mush_bounds*);

#endif
