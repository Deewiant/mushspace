// File created: 2011-08-06 17:57:40

#include "aabb.98.h"

#include <assert.h>
#include <string.h>

#include "stdlib.any.h"

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

mushcell mush_aabb_get_no_offset(const mush_aabb* aabb, mushcoords c) {
	// Can't assert contains(c + beg) since no_offset usage typically means that
	// our beg/end don't match data.
	return aabb->data[mush_aabb_get_idx_no_offset(aabb, c)];
}

void mush_aabb_put_no_offset(mush_aabb* aabb, mushcoords p, mushcell c) {
	aabb->data[mush_aabb_get_idx_no_offset(aabb, p)] = c;
}

mushcell mush_aabb_getter_no_offset(const void* aabb, mushcoords c) {
	return mush_aabb_get_no_offset(aabb, c);
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

bool mush_aabb_can_direct_copy(
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
bool mush_aabb_can_direct_copy_area(
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
