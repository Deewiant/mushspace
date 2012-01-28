// File created: 2012-01-28 00:18:01

#include "space/get-tight-bounds.all.h"

#include <assert.h>

static bool mushspace_find_beg_in(
	mushcoords*, mushdim, const mush_bounds*,
	mushcell(*)(const void*, mushcoords), const void*);

static void mushspace_find_end_in(
	mushcoords*, mushdim, const mush_bounds*,
	mushcell(*)(const void*, mushcoords), const void*);

bool mushspace_get_tight_bounds(
	mushspace* space, mushcoords* beg, mushcoords* end)
{
	bool last_beg_space = mushspace_get(space, space->last_beg) == ' ',
	     last_end_space = mushspace_get(space, space->last_end) == ' ',
	     found_nonspace = true;

	if (last_beg_space == last_end_space) {
		if (last_beg_space) {
			*beg = MUSHCOORDS(MUSHCELL_MAX, MUSHCELL_MAX, MUSHCELL_MAX);
			*end = MUSHCOORDS(MUSHCELL_MIN, MUSHCELL_MIN, MUSHCELL_MIN);
			found_nonspace = false;
		} else {
			*beg = space->last_beg;
			*end = space->last_end;
		}
	} else {
		if (last_beg_space)
			*beg = *end = space->last_end;
		else
			*beg = *end = space->last_beg;
	}

	bool changed = false;

	for (mushdim axis = 0; axis < MUSHSPACE_DIM; ++axis) {
		found_nonspace |=
			!mushspace_find_beg_in(
				beg, axis, &MUSH_STATICAABB_BOUNDS,
				mush_staticaabb_getter_no_offset, &space->static_box);

		mushspace_find_end_in(
			end, axis, &MUSH_STATICAABB_BOUNDS,
			mush_staticaabb_getter_no_offset, &space->static_box);

		for (size_t i = 0; i < space->box_count; ++i) {
			if (
				mushspace_find_beg_in(
					beg, axis, &space->boxen[i].bounds,
					mush_aabb_getter_no_offset, &space->boxen[i]))
			{
				mushspace_remove_boxes(space, i, i);
				mushstats_add(space->stats, MushStat_empty_boxes_dropped, 1);
				changed = true;
				continue;
			}
			found_nonspace = true;
			mushspace_find_end_in(end, axis, &space->boxen[i].bounds,
										 mush_aabb_getter_no_offset, &space->boxen[i]);
		}
	}

	if (changed)
		mushspace_invalidate_all(space);

	if (space->bak.data && mush_bakaabb_size(&space->bak) > 0) {
		found_nonspace = true;

		// Might as well tighten the approximate space->bak.bounds while we're at
		// it.
		mush_bounds bak_bounds = {space->bak.bounds.end, space->bak.bounds.beg};

		unsigned char buf[mush_bakaabb_iter_sizeof];
		mush_bakaabb_iter *it = mush_bakaabb_it_start(&space->bak, buf);

		for (; !mush_bakaabb_it_done(it, &space->bak);
		        mush_bakaabb_it_next(it, &space->bak))
		{
			assert (mush_bakaabb_it_val(it, &space->bak) != ' ');

			mushcoords c = mush_bakaabb_it_pos(it, &space->bak);
			mushcoords_min_into(&bak_bounds.beg, c);
			mushcoords_max_into(&bak_bounds.end, c);
		}
		mush_bakaabb_it_stop(it);

		space->bak.bounds = bak_bounds;

		mushcoords_min_into(beg, bak_bounds.beg);
		mushcoords_max_into(end, bak_bounds.end);
	}
	space->last_beg = *beg;
	space->last_end = *end;
	return found_nonspace;
}
static bool mushspace_find_beg_in(
	mushcoords* beg, mushdim axis, const mush_bounds* bounds,
	mushcell(*getter_no_offset)(const void*, mushcoords), const void* box)
{
	if (beg->v[axis] <= bounds->beg.v[axis])
		return false;

	// Quickly check the corner, in case we get lucky and can avoid a lot of
	// complication.
	if (getter_no_offset(box, MUSHCOORDS(0,0,0)) != ' ') {
		mushcoords_min_into(beg, bounds->beg);
		return false;
	}

	bool empty_box = true;

	mushcoords last = bounds->end;

	// If our beg is already better than part of the box, only check up to it
	// instead of the whole box.
	if (beg->v[axis] <= last.v[axis]) {
		// This decrement cannot underflow because we already checked against the
		// box's beginning coordinate above: we know the box's beg is not
		// MUSHCELL_MIN and thus its end isn't either.
		last.v[axis] = beg->v[axis] - 1;

		// We can conclude that the box is empty only if we're going to traverse
		// it completely.
		empty_box = false;
	}

	mushcoords_sub_into(&last, bounds->beg);

	switch (axis) {
		mushcoords c;

	#define CHECK { \
		if (getter_no_offset(box, c) == ' ') \
			continue; \
	\
		mushcoords_min_into(beg, mushcoords_add(c, bounds->beg)); \
		if (c.v[axis] == 0) \
			return false; \
	\
		mushcell_min_into(&last.v[axis], c.v[axis]); \
		empty_box = false; \
		break; \
	}

	case 0:
#if MUSHSPACE_DIM >= 3
		for (c.z = 0; c.z <= last.z; ++c.z)
#endif
#if MUSHSPACE_DIM >= 2
		for (c.y = 0; c.y <= last.y; ++c.y)
#endif
		for (c.x = 0; c.x <= last.x; ++c.x)
			CHECK
		break;

#if MUSHSPACE_DIM >= 2
	case 1:
#if MUSHSPACE_DIM >= 3
		for (c.z = 0; c.z <= last.z; ++c.z)
#endif
		for (c.x = 0; c.x <= last.x; ++c.x)
		for (c.y = 0; c.y <= last.y; ++c.y)
			CHECK
		break;
#endif

#if MUSHSPACE_DIM >= 3
	case 2:
		for (c.y = 0; c.y <= last.y; ++c.y)
		for (c.x = 0; c.x <= last.x; ++c.x)
		for (c.z = 0; c.z <= last.z; ++c.z)
			CHECK
		break;
#endif

	default: assert (false);
	#undef CHECK
	}
	return empty_box;
}
static void mushspace_find_end_in(
	mushcoords* end, mushdim axis, const mush_bounds* bounds,
	mushcell(*getter_no_offset)(const void*, mushcoords), const void* box)
{
	if (end->v[axis] >= bounds->end.v[axis])
		return;

	if (getter_no_offset(box, mushcoords_sub(bounds->end, bounds->beg)) != ' ')
	{
		mushcoords_max_into(end, bounds->end);
		return;
	}

	mushcoords last = MUSHCOORDS(0,0,0);

	if (end->v[axis] >= bounds->beg.v[axis])
		last.v[axis] = end->v[axis] + 1 - bounds->beg.v[axis];

	const mushcoords start = mushcoords_sub(bounds->end, bounds->beg);

	switch (axis) {
		mushcoords c;

	#define CHECK { \
		if (getter_no_offset(box, c) == ' ') \
			continue; \
	\
		mushcoords_max_into(end, mushcoords_add(c, bounds->beg)); \
		if (end->v[axis] == bounds->end.v[axis]) \
			return; \
	\
		mushcell_max_into(&last.v[axis], c.v[axis]); \
		break; \
	}

	case 0:
#if MUSHSPACE_DIM >= 3
		for (c.z = start.z; c.z >= last.z; --c.z)
#endif
#if MUSHSPACE_DIM >= 2
		for (c.y = start.y; c.y >= last.y; --c.y)
#endif
		for (c.x = start.x; c.x >= last.x; --c.x)
			CHECK
		break;

#if MUSHSPACE_DIM >= 2
	case 1:
#if MUSHSPACE_DIM >= 3
		for (c.z = start.z; c.z >= last.z; --c.z)
#endif
		for (c.x = start.x; c.x >= last.x; --c.x)
		for (c.y = start.y; c.y >= last.y; --c.y)
			CHECK
		break;
#endif

#if MUSHSPACE_DIM >= 3
	case 2:
		for (c.y = start.y; c.y >= last.y; --c.y)
		for (c.x = start.x; c.x >= last.x; --c.x)
		for (c.z = start.z; c.z >= last.z; --c.z)
			CHECK
		break;
#endif

	default: assert (false);
	#undef CHECK
	}
}
