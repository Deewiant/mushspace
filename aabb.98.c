// File created: 2011-08-06 17:57:40

#include "aabb.98.h"

#include <assert.h>
#include <string.h>

#define mush_aabb_can_direct_copy MUSHSPACE_CAT(mush_aabb,_can_direct_copy)
#define mush_aabb_can_direct_copy_area \
	MUSHSPACE_CAT(mush_aabb,_can_direct_copy_area)

#define mush_aabb_subsume_owners_area \
	MUSHSPACE_CAT(mush_aabb,_subsume_owners_area)

static bool mush_aabb_can_direct_copy(const mush_aabb*, const mush_aabb*);
static bool mush_aabb_can_direct_copy_area(
	const mush_aabb*, const mush_aabb*, const mush_aabb*);

// Copies from data to aabb, given that it's an area contained in owner.
static void mush_aabb_subsume_owners_area(
	mush_aabb* aabb, const mush_aabb* owner, const mush_aabb* area,
	const mushcell* data, size_t);

#if MUSHSPACE_DIM > 1
#define mush_aabb_consume_2d MUSHSPACE_CAT(mush_aabb,_consume_2d)
static void mush_aabb_consume_2d(
	const mush_aabb*, const mush_aabb*, size_t, size_t, size_t);
#endif

void mush_aabb_make(mush_aabb* aabb, const mush_bounds* bounds) {
	mush_aabb_make_unsafe(aabb, bounds);
	mush_aabb_finalize(aabb);
}

void mush_aabb_make_unsafe(mush_aabb* aabb, const mush_bounds* bounds) {
	*aabb = (mush_aabb){.bounds = *bounds};
}

void mush_aabb_finalize(mush_aabb* aabb) {
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
		assert (aabb->bounds.beg.v[i] <= aabb->bounds.end.v[i]);

	aabb->size = aabb->bounds.end.x - aabb->bounds.beg.x + 1;

#if MUSHSPACE_DIM >= 2
	aabb->width = aabb->size;
	aabb->size *= aabb->bounds.end.y - aabb->bounds.beg.y + 1;
#endif
#if MUSHSPACE_DIM >= 3
	aabb->area  = aabb->size;
	aabb->size *= aabb->bounds.end.z - aabb->bounds.beg.z + 1;
#endif
}

bool mush_aabb_alloc(mush_aabb* aabb) {
	aabb->data = malloc(aabb->size);
	if (!aabb->data)
		return false;
	mushcell_space(aabb->data, aabb->size);
	return true;
}

size_t mush_aabb_volume_on(const mush_aabb* aabb, mushdim axis) {
	assert (axis < MUSHSPACE_DIM);
#if MUSHSPACE_DIM >= 2
	if (axis == 1) return aabb->width;
#if MUSHSPACE_DIM >= 3
	if (axis == 2) return aabb->area;
#endif
#endif
	(void)aabb; (void)axis;
	return 1;
}

mushcell mush_aabb_get(const mush_aabb* aabb, mushcoords c) {
	assert (mush_bounds_contains(&aabb->bounds, c));
	return aabb->data[mush_aabb_get_idx(aabb, c)];
}

void mush_aabb_put(mush_aabb* aabb, mushcoords p, mushcell c) {
	assert (mush_bounds_contains(&aabb->bounds, p));
	aabb->data[mush_aabb_get_idx(aabb, p)] = c;
}

size_t mush_aabb_get_idx(const mush_aabb* a, mushcoords c) {
	return mush_aabb_get_idx_no_offset(a, mushcoords_sub(c, a->bounds.beg));
}
size_t mush_aabb_get_idx_no_offset(const mush_aabb* aabb, mushcoords c)
{
	size_t i = c.x;
#if MUSHSPACE_DIM >= 2
	i += aabb->width * c.y;
#if MUSHSPACE_DIM >= 3
	i += aabb->area  * c.z;
#endif
#else
	(void)aabb;
#endif
	return i;
}

static bool mush_aabb_can_direct_copy(
	const mush_aabb* copier, const mush_aabb* copiee)
{
#if MUSHSPACE_DIM == 1
	(void)copier; (void)copiee;
	return true;
#else
	if (copiee->size == copiee->width && copiee->size <= copier->width)
		return true;

	return copier->width == copiee->width
#if MUSHSPACE_DIM >= 3
	    && copier->area  == copiee->area
#endif
	;
#endif
}
static bool mush_aabb_can_direct_copy_area(
	const mush_aabb* copier, const mush_aabb* copiee, const mush_aabb* owner)
{
#if MUSHSPACE_DIM >= 2
	if (copiee->width != owner->width) return false;
#if MUSHSPACE_DIM >= 3
	if (copiee->area  != owner->area)  return false;
#endif
#endif
	(void)owner;
	return mush_aabb_can_direct_copy(copier, copiee);
}

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
void mush_aabb_subsume(mush_aabb* a, const mush_aabb* b) {
	assert (mush_bounds_contains_bounds(&a->bounds, &b->bounds));

	mush_aabb_subsume_owners_area(a, b, b, b->data, b->size);
}
void mush_aabb_subsume_area(
	mush_aabb* a, const mush_aabb* b, const mush_aabb* area)
{
	assert (mush_bounds_contains_bounds(&a->bounds, &area->bounds));
	assert (mush_bounds_contains_bounds(&b->bounds, &area->bounds));

	const mush_bounds* ab = &area->bounds;

	size_t beg_idx = mush_aabb_get_idx(b, ab->beg),
	       end_idx = mush_aabb_get_idx(b, ab->end);

	mush_aabb_subsume_owners_area(
		a, b, area, b->data + beg_idx, end_idx - beg_idx + 1);

	assert (mush_aabb_get(a, ab->beg) == mush_aabb_get(b, ab->beg));
	assert (mush_aabb_get(a, ab->end) == mush_aabb_get(b, ab->end));
}
static void mush_aabb_subsume_owners_area(
	mush_aabb* aabb, const mush_aabb* owner, const mush_aabb* area,
	const mushcell* data, size_t len)
{
	assert (mush_bounds_contains_bounds(&aabb->bounds,  &area->bounds));
	assert (mush_bounds_contains_bounds(&owner->bounds, &area->bounds));

	// We can't just use only area since the data is usually not continuous:
	//
	//   ownerowner
	//   owner****D
	//   ATADA****D
	//   ATADA****D
	//   ATADA****r
	//   ownerowner
	//
	// (In the above: area is shown with asterisks, owner with "owner", and the
	//  area from data to data+len with "DATA".)
	//
	// In such a case, if we were to advance in the array by area->width instead
	// of owner->width we'd be screwed.

	static const size_t SIZE = sizeof *data;

	const mush_bounds* ab = &area->bounds;

	const size_t beg_idx = mush_aabb_get_idx(aabb, ab->beg);

	if (mush_aabb_can_direct_copy_area(aabb, area, owner)) {
		memcpy(aabb->data + beg_idx, data, len * SIZE);
		goto end;
	}

	assert (MUSHSPACE_DIM > 1);

#if MUSHSPACE_DIM == 2
	for (size_t o = 0, a = beg_idx; o < len;) {
		memcpy(aabb->data + a, data + o, area->width * SIZE);
		o += owner->width;
		a +=  aabb->width;
	}

#elif MUSHSPACE_DIM == 3
	const size_t owner_area_size = area->area / area->width * owner->width;

	for (size_t o = 0, a = beg_idx; o < len;) {
		for (size_t oy = o, ay = a; oy < o + owner_area_size;) {
			memcpy(aabb->data + ay, data + oy, area->width * SIZE);
			oy += owner->width;
			ay +=  aabb->width;
		}
		o += owner->area;
		a +=  aabb->area;
	}
#endif
end:
	assert (mush_aabb_get(aabb, ab->beg) == data[0]);
	assert (mush_aabb_get(aabb, ab->end) == data[len-1]);
	assert (mush_aabb_get(aabb, ab->beg) == mush_aabb_get(owner, ab->beg));
	assert (mush_aabb_get(aabb, ab->end) == mush_aabb_get(owner, ab->end));
}
void mush_aabb_space_area(mush_aabb* aabb, const mush_aabb* area) {
	assert (mush_bounds_contains_bounds(&aabb->bounds, &area->bounds));

	const mush_bounds* ab = &area->bounds;

	const size_t beg_idx = mush_aabb_get_idx(aabb, ab->beg);

	if (mush_aabb_can_direct_copy(aabb, area)) {
		mushcell_space(aabb->data + beg_idx, area->size);
		goto end;
	}

#if MUSHSPACE_DIM == 2
	const size_t end_idx = beg_idx + area->size / area->width * aabb->width;

	for (size_t i = beg_idx; i < end_idx; i += aabb->width)
		mushcell_space(aabb->data + i, area->width);

#elif MUSHSPACE_DIM == 3
	const size_t end_idx   = beg_idx + area->size / area->area  * aabb->area,
	             area_size =           area->area / area->width * aabb->width;

	for (size_t j = beg_idx; j < end_idx; j += aabb->area)
		for (size_t i = j, e = j + area_size; i < e; i += aabb->width)
			mushcell_space(aabb->data + i, area->width);
#endif
end:
	assert (mush_aabb_get(aabb, ab->beg) == ' ');
	assert (mush_aabb_get(aabb, ab->end) == ' ');
}
