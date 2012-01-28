// File created: 2012-01-28 01:06:38

#include "aabb/subsume.98.h"

#include <assert.h>
#include <string.h>

MUSH_DECL_CONST_DYN_ARRAY(mushcell)

// Copies from data to aabb, given that it's an area contained in owner.
static void subsume_owners_area(
	mush_aabb* aabb, const mush_aabb* owner, const mush_aabb* area,
	mush_carr_mushcell data);

void mush_aabb_subsume(mush_aabb* a, const mush_aabb* b) {
	assert (mush_bounds_contains_bounds(&a->bounds, &b->bounds));

	subsume_owners_area(a, b, b, (mush_carr_mushcell){b->data, b->size});
}

void mush_aabb_subsume_area(
	mush_aabb* a, const mush_aabb* b, const mush_aabb* area)
{
	assert (mush_bounds_contains_bounds(&a->bounds, &area->bounds));
	assert (mush_bounds_contains_bounds(&b->bounds, &area->bounds));

	const mush_bounds* ab = &area->bounds;

	size_t beg_idx = mush_aabb_get_idx(b, ab->beg),
	       end_idx = mush_aabb_get_idx(b, ab->end);

	subsume_owners_area(
		a, b, area,
		(mush_carr_mushcell){b->data + beg_idx, end_idx - beg_idx + 1});

	assert (mush_aabb_get(a, ab->beg) == mush_aabb_get(b, ab->beg));
	assert (mush_aabb_get(a, ab->end) == mush_aabb_get(b, ab->end));
}
static void subsume_owners_area(
	mush_aabb* aabb, const mush_aabb* owner, const mush_aabb* area,
	mush_carr_mushcell arr)
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
	//  area from arr.ptr to arr.ptr+arr.len with "DATA".)
	//
	// In such a case, if we were to advance in the array by area->width instead
	// of owner->width we'd be screwed.

	static const size_t SIZE = sizeof *arr.ptr;

	const mush_bounds* ab = &area->bounds;

	const size_t beg_idx = mush_aabb_get_idx(aabb, ab->beg);

	if (mush_aabb_can_direct_copy_area(aabb, area, owner)) {
		memcpy(aabb->data + beg_idx, arr.ptr, arr.len * SIZE);
		goto end;
	}

	assert (MUSHSPACE_DIM > 1);

#if MUSHSPACE_DIM == 2
	for (size_t o = 0, a = beg_idx; o < arr.len;) {
		memcpy(aabb->data + a, arr.ptr + o, area->width * SIZE);
		o += owner->width;
		a +=  aabb->width;
	}

#elif MUSHSPACE_DIM == 3
	const size_t owner_area_size = area->area / area->width * owner->width;

	for (size_t o = 0, a = beg_idx; o < arr.len;) {
		for (size_t oy = o, ay = a; oy < o + owner_area_size;) {
			memcpy(aabb->data + ay, arr.ptr + oy, area->width * SIZE);
			oy += owner->width;
			ay +=  aabb->width;
		}
		o += owner->area;
		a +=  aabb->area;
	}
#endif
end:
	assert (mush_aabb_get(aabb, ab->beg) == arr.ptr[0]);
	assert (mush_aabb_get(aabb, ab->end) == arr.ptr[arr.len-1]);
	assert (mush_aabb_get(aabb, ab->beg) == mush_aabb_get(owner, ab->beg));
	assert (mush_aabb_get(aabb, ab->end) == mush_aabb_get(owner, ab->end));
}
