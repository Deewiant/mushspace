// File created: 2011-08-07 18:11:59

#include "staticaabb.all.h"

#include <assert.h>
#include <stdlib.h>

const mushbounds
	MUSHSTATICAABB_BOUNDS =
		{.beg = MUSHSTATICAABB_BEG_INIT, .end = MUSHSTATICAABB_END_INIT},
	MUSHSTATICAABB_REL_BOUNDS =
		{.beg = MUSHCOORDS_INIT(0,0,0),  .end = MUSHSTATICAABB_REL_END_INIT};

bool mushstaticaabb_contains(mushcoords c) {
	return mushbounds_contains(&MUSHSTATICAABB_BOUNDS, c);
}

mushcell mushstaticaabb_get(const mushstaticaabb* aabb, mushcoords p) {
	assert (mushstaticaabb_contains(p));
	return aabb->array[mushstaticaabb_get_idx(p)];
}

void mushstaticaabb_put(mushstaticaabb* aabb, mushcoords p, mushcell c) {
	assert (mushstaticaabb_contains(p));
	aabb->array[mushstaticaabb_get_idx(p)] = c;
}

mushcell mushstaticaabb_get_no_offset(
	const mushstaticaabb* aabb, mushcoords p)
{
	assert (mushstaticaabb_contains(mushcoords_add(p, MUSHSTATICAABB_BEG)));
	return aabb->array[mushstaticaabb_get_idx_no_offset(p)];
}

void mushstaticaabb_put_no_offset(
	mushstaticaabb* aabb, mushcoords p, mushcell c)
{
	assert (mushstaticaabb_contains(mushcoords_add(p, MUSHSTATICAABB_BEG)));
	aabb->array[mushstaticaabb_get_idx_no_offset(p)] = c;
}

mushcell mushstaticaabb_getter_no_offset(const void* aabb, mushcoords c) {
	return mushstaticaabb_get_no_offset(aabb, c);
}

size_t mushstaticaabb_get_idx(mushcoords c) {
	return mushstaticaabb_get_idx_no_offset(
		mushcoords_sub(c, MUSHSTATICAABB_BEG));
}
size_t mushstaticaabb_get_idx_no_offset(mushcoords c) {
	size_t i = c.x;
#if MUSHSPACE_DIM >= 2
	i += MUSHSTATICAABB_SIZE.x * c.y;
#if MUSHSPACE_DIM >= 3
	i += MUSHSTATICAABB_SIZE.y * c.z;
#endif
#endif
	return i;
}
