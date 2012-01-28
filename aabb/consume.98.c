// File created: 2012-01-28 00:58:10

#include "aabb/consume.98.h"

#include <assert.h>
#include <string.h>

#if MUSHSPACE_DIM > 1
static void mush_aabb_consume_2d(
	const mush_aabb*, const mush_aabb*, size_t, size_t, size_t);
#endif

bool mush_aabb_consume(mush_aabb* box, mush_aabb* old) {
	assert (mush_bounds_contains_bounds(&box->bounds, &old->bounds));

	static const size_t SIZE = sizeof *box->data;

	if (!(box->data = realloc(old->data, box->size * SIZE)))
		return false;

	mushcell_space(box->data + old->size, box->size - old->size);

	const size_t old_idx = mush_aabb_get_idx(box, old->bounds.beg);

	if (mush_aabb_can_direct_copy(box, old)) {
		if (old_idx == 0)
			return true;

		if (old_idx < old->size) {
			memmove(box->data + old_idx, box->data, old->size * SIZE);
			mushcell_space(box->data, old_idx);
		} else {
			memcpy(box->data + old_idx, box->data, old->size * SIZE);
			mushcell_space(box->data, old->size);
		}
		return true;
	}

	assert (MUSHSPACE_DIM > 1);

#if MUSHSPACE_DIM > 1
	const bool same_beg = mushcoords_equal(box->bounds.beg, old->bounds.beg);

	// If the beginning coordinates are identical, the first row is already in
	// the right place so we don't need to touch it.
	const size_t last_row_idx_summand = same_beg ? old->width : 0;
#endif

#if MUSHSPACE_DIM == 2
	mush_aabb_consume_2d(box, old, old_idx, old->size, last_row_idx_summand);
#elif MUSHSPACE_DIM == 3
	const size_t
		last_area_idx =
			old_idx + (same_beg && box->width == old->width ? old->area : 0),

		new_old_end = old_idx + old->size / old->area * box->area;

	for (size_t b = new_old_end, o = old->size; b > last_area_idx;) {
		b -= box->area;
		mush_aabb_consume_2d(box, old, b, o, last_row_idx_summand);
		o -= old->area;
	}
#endif
	return true;
}
#if MUSHSPACE_DIM > 1
static void mush_aabb_consume_2d(
	const mush_aabb* box, const mush_aabb* old,
	size_t old_idx, size_t old_end, size_t last_row_idx_summand)
{
	static const size_t SIZE = sizeof *box->data;
	const size_t
#if MUSHSPACE_DIM == 2
		old_rect_size = old->size,
#else
		old_rect_size = old->area,
#endif
		old_height    = old_rect_size / old->width,
		new_old_end   = old_idx + old_height * box->width,
		last_row_idx  = old_idx + last_row_idx_summand;

	for (size_t b = new_old_end, o = old_end; b > last_row_idx;) {
		b -= box->width;
		o -= old->width;

		// The old data is always earlier in the array than the new location, so
		// overlapping can only occur from one direction: b + old->width <= o
		// can't happen.
		assert (b + old->width > o);

		if (o + old->width <= b) {
			memcpy        (box->data + b, box->data + o, old->width * SIZE);
			mushcell_space(               box->data + o, old->width);
		} else if (b != o) {
			memmove       (box->data + b, box->data + o, old->width * SIZE);
			mushcell_space(               box->data + o, b - o);
		}
	}
}
#endif
