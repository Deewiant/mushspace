// File created: 2011-08-21 16:17:09

#include "bounds.all.h"

#include <assert.h>

size_t mush_bounds_clamped_size(const mush_bounds* bounds) {
	size_t sz = 1;
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
		sz = mush_size_t_mul_clamped(
			sz, mush_size_t_add_clamped(bounds->end.v[i] - bounds->beg.v[i], 1));
	return sz;
}

bool mush_bounds_contains(const mush_bounds* bounds, mushcoords pos) {
	if (pos.x < bounds->beg.x || pos.x > bounds->end.x) return false;
#if MUSHSPACE_DIM >= 2
	if (pos.y < bounds->beg.y || pos.y > bounds->end.y) return false;
#if MUSHSPACE_DIM >= 3
	if (pos.z < bounds->beg.z || pos.z > bounds->end.z) return false;
#endif
#endif
	return true;
}
bool mush_bounds_safe_contains(const mush_bounds* bounds, mushcoords pos) {
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
		if (bounds->beg.v[i] > bounds->end.v[i]) {
			if (!(pos.v[i] >= bounds->beg.v[i] || pos.v[i] <= bounds->end.v[i]))
				return false;
		} else {
			if (!(pos.v[i] >= bounds->beg.v[i] && pos.v[i] <= bounds->end.v[i]))
				return false;
		}
	}
	return true;
}

bool mush_bounds_contains_bounds(const mush_bounds* a, const mush_bounds* b) {
	return mush_bounds_contains(a, b->beg) && mush_bounds_contains(a, b->end);
}
bool mush_bounds_overlaps(const mush_bounds* a, const mush_bounds* b) {
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
		if (a->beg.v[i] > b->end.v[i] || b->beg.v[i] > a->end.v[i])
			return false;
	return true;
}

#if !MUSHSPACE_93
bool mush_bounds_get_overlap(
	const mush_bounds* a, const mush_bounds* b, mush_bounds* overlap)
{
	if (!mush_bounds_overlaps(a, b))
		return false;

	overlap->beg = a->beg; mushcoords_max_into(&overlap->beg, b->beg);
	overlap->end = a->end; mushcoords_min_into(&overlap->end, b->end);

	assert (mush_bounds_contains_bounds(a, overlap));
	assert (mush_bounds_contains_bounds(b, overlap));
	return true;
}
#endif

#if MUSHSPACE_DIM > 1
bool mush_bounds_on_same_axis(const mush_bounds* a, const mush_bounds* b) {
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
		if (a->beg.v[i] == b->beg.v[i] && a->end.v[i] == b->end.v[i])
			return true;
	return false;
}
bool mush_bounds_on_same_primary_axis(
	const mush_bounds* a, const mush_bounds* b)
{
	const mushdim I = MUSHSPACE_DIM-1;
	return a->beg.v[I] == b->beg.v[I] && a->end.v[I] == b->end.v[I];
}
#endif

#if !MUSHSPACE_93
bool mush_bounds_can_fuse(const mush_bounds* a, const mush_bounds* b) {
	bool overlap = mush_bounds_overlaps(a, b);

	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
		if (a->beg.v[i] == b->beg.v[i] && a->end.v[i] == b->end.v[i])
			continue;

		if (!(   overlap
		      || mushcell_add_clamped(a->end.v[i], 1) == b->beg.v[i]
		      || mushcell_add_clamped(b->end.v[i], 1) == a->beg.v[i]))
			return false;

		for (mushdim j = i+1; j < MUSHSPACE_DIM; ++j)
			if (a->beg.v[j] != b->beg.v[j] || a->end.v[j] != b->end.v[j])
				return false;

		#if MUSHSPACE_DIM > 1
			assert (mush_bounds_on_same_axis(a, b));
		#endif
		return true;
	}
	return false;
}
#endif

void mush_bounds_tessellate(
	mushcoords pos, mush_carr_mush_bounds bs, mush_bounds* bounds)
{
	assert (mush_bounds_contains(bounds, pos));

	for (size_t i = 0; i < bs.len; ++i)
		mush_bounds_tessellate1(pos, &bs.ptr[i], bounds);
}
void mush_bounds_tessellate1(
	mushcoords pos, const mush_bounds* avoid, mush_bounds* bounds)
{
	assert (mush_bounds_contains(bounds, pos));

	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
		// This could be improved, consider for instance the bottommost box in
		// the following graphic and its current tessellation:
		//
		// +-------+    +--*--*-+
		// |       |    |X .  . |
		// |       |    |  .  . |
		// |     +---   *..*..+---
		// |     |      |  .  |
		// |  +--|      *..+--|
		// |  |  |      |  |  |
		// |  |  |      |  |  |
		// +--|  |      +--|  |
		//
		// (Note that this isn't actually a tessellation: all points will get
		// a rectangle containing the rectangle at X.)
		//
		// Any of the following three would be an improvement (and they would
		// actually be tessellations):
		//
		// +--*--*-+    +-------+    +-----*-+
		// |  .  . |    |       |    |     . |
		// |  .  . |    |       |    |     . |
		// |  .  +---   *.....+---   |     +---
		// |  .  |      |     |      |     |
		// |  +--|      *..+--|      *..+--|
		// |  |  |      |  |  |      |  |  |
		// |  |  |      |  |  |      |  |  |
		// +--|  |      +--|  |      +--|  |

		mushcell ab = avoid->beg.v[i],
		         ae = avoid->end.v[i],
		         p  = pos.v[i];
		if (ae < p) bounds->beg.v[i] = mushcell_max(bounds->beg.v[i], ae+1);
		if (ab > p) bounds->end.v[i] = mushcell_min(bounds->end.v[i], ab-1);
	}
	assert (!mush_bounds_overlaps(bounds, avoid));
}
