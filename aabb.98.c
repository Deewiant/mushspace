// File created: 2011-08-06 17:57:40

#include "aabb.98.h"

#include <assert.h>
#include <string.h>

#include "stdlib.any.h"

void mushaabb_make(mushaabb* aabb, const mushbounds* bounds) {
	mushaabb_make_unsafe(aabb, bounds);
	mushaabb_finalize(aabb);
}

void mushaabb_make_unsafe(mushaabb* aabb, const mushbounds* bounds) {
	*aabb = (mushaabb){.bounds = *bounds};
}

void mushaabb_finalize(mushaabb* aabb) {
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

bool mushaabb_alloc(mushaabb* aabb) {
	aabb->data = malloc(aabb->size);
	if (!aabb->data)
		return false;
	mushcell_space(aabb->data, aabb->size);
	return true;
}

size_t mushaabb_volume_on(const mushaabb* aabb, mushdim axis) {
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

mushcell mushaabb_get(const mushaabb* aabb, mushcoords c) {
	assert (mushbounds_contains(&aabb->bounds, c));
	return aabb->data[mushaabb_get_idx(aabb, c)];
}

void mushaabb_put(mushaabb* aabb, mushcoords p, mushcell c) {
	assert (mushbounds_contains(&aabb->bounds, p));
	aabb->data[mushaabb_get_idx(aabb, p)] = c;
}

mushcell mushaabb_get_no_offset(const mushaabb* aabb, mushcoords c) {
	// Can't assert contains(c + beg) since no_offset usage typically means that
	// our beg/end don't match data.
	return aabb->data[mushaabb_get_idx_no_offset(aabb, c)];
}

void mushaabb_put_no_offset(mushaabb* aabb, mushcoords p, mushcell c) {
	aabb->data[mushaabb_get_idx_no_offset(aabb, p)] = c;
}

mushcell mushaabb_getter_no_offset(const void* aabb, mushcoords c) {
	return mushaabb_get_no_offset(aabb, c);
}

size_t mushaabb_get_idx(const mushaabb* a, mushcoords c) {
	return mushaabb_get_idx_no_offset(a, mushcoords_sub(c, a->bounds.beg));
}
size_t mushaabb_get_idx_no_offset(const mushaabb* aabb, mushcoords c) {
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

bool mushaabb_can_direct_copy(const mushaabb* copier, const mushaabb* copiee) {
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
bool mushaabb_can_direct_copy_area(
	const mushaabb* copier, const mushaabb* copiee, const mushaabb* owner)
{
#if MUSHSPACE_DIM >= 2
	if (copiee->width != owner->width) return false;
#if MUSHSPACE_DIM >= 3
	if (copiee->area  != owner->area)  return false;
#endif
#endif
	(void)owner;
	return mushaabb_can_direct_copy(copier, copiee);
}
