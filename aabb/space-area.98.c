// File created: 2012-01-28 01:09:42

#include "aabb/space-area.98.h"

#include <assert.h>

void mushaabb_space_area(mushaabb* aabb, const mushaabb* area) {
	assert (mushbounds_contains_bounds(&aabb->bounds, &area->bounds));

	const mushbounds* ab = &area->bounds;

	const size_t beg_idx = mushaabb_get_idx(aabb, ab->beg);

	if (mushaabb_can_direct_copy(aabb, area)) {
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
	assert (mushaabb_get(aabb, ab->beg) == ' ');
	assert (mushaabb_get(aabb, ab->end) == ' ');
}
