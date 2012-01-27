// File created: 2012-01-28 00:28:27

#include "space/jump-to-box.98.h"

#include <assert.h>

#include "bounds/ray-intersects.98.h"

bool mushspace_jump_to_box(
	mushspace* space, mushcoords* pos, mushcoords delta,
	MushCursorMode* hit_type, mush_aabb** aabb, size_t* aabb_idx)
{
	assert (!mushspace_find_box(space, *pos));

	mushucell  moves = 0;
	mushcoords pos2;
	size_t     idx;
	mushucell  m;
	mushcoords c;
	bool       hit_static;

	// Pick the closest box that we hit, starting from the topmost.

	if (mush_bounds_ray_intersects(*pos, delta, &MUSH_STATICAABB_BOUNDS,
	                               &m, &c))
	{
		pos2       = c;
		moves      = m;
		hit_static = true;
	}

	for (size_t i = 0; i < space->box_count; ++i) {

		// The !moves option is necessary: we can't initialize moves and rely on
		// "m < moves" because m might legitimately be the greatest possible
		// mushucell value.
		if (mush_bounds_ray_intersects(*pos, delta, &space->boxen[i].bounds,
		                               &m, &c)
		 && ((m < moves) | !moves))
		{
			pos2       = c;
			idx        = i;
			moves      = m;
			hit_static = false;
		}
	}

	if (space->bak.data
	 && mush_bounds_ray_intersects(*pos, delta, &space->bak.bounds, &m, &c)
	 && ((m < moves) | !moves))
	{
		*pos      = c;
		*hit_type = MushCursorMode_bak;
		return true;
	}

	if (!moves)
		return false;

	if (hit_static) {
		*pos      = pos2;
		*hit_type = MushCursorMode_static;
		return true;
	}

	*pos      = pos2;
	*aabb_idx = idx;
	*aabb     = &space->boxen[idx];
	*hit_type = MushCursorMode_dynamic;
	return true;
}
