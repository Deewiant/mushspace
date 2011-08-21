// File created: 2011-08-07 18:11:59

#include "staticaabb.all.h"

#include <assert.h>
#include <stdlib.h>

const mush_bounds MUSH_STATICAABB_BOUNDS =
	{.beg = MUSH_STATICAABB_BEG_INIT, .end = MUSH_STATICAABB_END_INIT};

bool mush_staticaabb_contains(mushcoords c) {
	return mushcoords_contains(c, MUSH_STATICAABB_BEG, MUSH_STATICAABB_END);
}

mushcell mush_staticaabb_get(const mush_staticaabb* aabb, mushcoords p) {
	assert (mush_staticaabb_contains(p));
	return aabb->array[mush_staticaabb_get_idx(p)];
}

void mush_staticaabb_put(mush_staticaabb* aabb, mushcoords p, mushcell c) {
	assert (mush_staticaabb_contains(p));
	aabb->array[mush_staticaabb_get_idx(p)] = c;
}

size_t mush_staticaabb_get_idx(mushcoords c) {
	return mush_staticaabb_get_idx_no_offset(
		mushcoords_sub(c, MUSH_STATICAABB_BEG));
}
size_t mush_staticaabb_get_idx_no_offset(mushcoords c) {
	size_t i = c.x;
#if MUSHSPACE_DIM >= 2
	i += MUSH_STATICAABB_SIZE.x * c.y;
#if MUSHSPACE_DIM >= 3
	i += MUSH_STATICAABB_SIZE.y * c.z;
#endif
#endif
	return i;
}
