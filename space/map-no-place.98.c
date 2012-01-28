// File created: 2012-01-27 20:58:47

#include "space/map-no-place.98.h"

#include <assert.h>
#include <string.h>

typedef struct { mushcell cell; size_t idx; } mush_cell_idx;

static bool mushspace_map_in_box(
	mushspace*, mush_bounded_pos, mush_caabb_idx,
	void*, void(*f)(mush_arr_mushcell, void*));

static bool mushspace_map_in_static(
	mushspace*, mush_bounded_pos,
	void*, void(*f)(mush_arr_mushcell, void*));

static bool mushspace_mapex_in_box(
	mushspace*, mush_bounded_pos, mush_caabb_idx, void*,
	void(*)(mush_arr_mushcell, void*, size_t,size_t, size_t,size_t, uint8_t*));

static bool mushspace_mapex_in_static(
	mushspace*, mush_bounded_pos, void*,
	void(*)(mush_arr_mushcell, void*, size_t,size_t, size_t,size_t, uint8_t*));

static bool mushspace_get_next_in(
	const mushspace*, const mush_aabb*, mushcoords*, size_t*);

static void mushspace_get_next_in1(
	mushucell, const mush_bounds*, mushcell, size_t, mushcoords, size_t,
	mush_cell_idx*, mush_cell_idx*);

void mushspace_map_no_place(
	mushspace* space, const mush_aabb* aabb, void* fg,
	void(*f)(mush_arr_mushcell, void*), void(*g)(size_t, void*))
{
	mushcoords        pos = aabb->bounds.beg;
	mush_bounded_pos bpos = {&aabb->bounds, &pos};

	for (;;) next_pos: {
		if (mush_staticaabb_contains(pos)) {
			if (mushspace_map_in_static(space, bpos, fg, f))
				return;
			else
				goto next_pos;
		}

		for (size_t b = 0; b < space->box_count; ++b) {
			mush_caabb_idx box = mushspace_get_caabb_idx(space, b);

			if (!mush_bounds_contains(&box.aabb->bounds, pos))
				continue;

			if (mushspace_map_in_box(space, bpos, box, fg, f))
				return;
			else
				goto next_pos;
		}

		// No hits for pos: find the next pos we can hit, or stop if there's
		// nothing left.
		size_t skipped = 0;
		bool found = mushspace_get_next_in(space, aabb, &pos, &skipped);
		if (g)
			g(skipped, fg);
		if (!found)
			return;
	}
}
bool mushspace_map_in_box(
	mushspace* space, mush_bounded_pos bpos, mush_caabb_idx cai,
	void* caller_data, void(*f)(mush_arr_mushcell, void*))
{
	// Consider:
	//     +-----+
	// x---|░░░░░|---+
	// |░░░|░░░░░|░░░|
	// |░B░|░░A░░|░░░y
	// |   |     |   |
	// +---|     |---+
	//     +-----+
	// We want to map the range from x to y (shaded). Unless we tessellate,
	// we'll get the whole thing from box B straight away.

	const mush_aabb *box = cai.aabb;

	mush_bounds tes = box->bounds;
	mush_bounds_tessellate(
		&tes, *bpos.pos,
		(mush_carr_mush_bounds){(const mush_bounds*)space->boxen, cai.idx});

	// The static box is above all dynamic boxen, so check it as well.
	mush_bounds_tessellate1(&tes, *bpos.pos, &MUSH_STATICAABB_BOUNDS);

	bool hit_end;
	const size_t
		beg_idx = mush_aabb_get_idx(box, *bpos.pos),
		end_idx = mush_aabb_get_idx(box, mushcoords_get_end_of_contiguous_range(
			tes.end, bpos.pos, bpos.bounds->end,
			bpos.bounds->beg, &hit_end, tes.beg, box->bounds.beg));

	assert (beg_idx <= end_idx);

	f((mush_arr_mushcell){box->data, end_idx - beg_idx + 1}, caller_data);
	return hit_end;
}
bool mushspace_map_in_static(
	mushspace* space, mush_bounded_pos bpos,
	void* caller_data, void(*f)(mush_arr_mushcell, void*))
{
	bool hit_end;
	const size_t
		beg_idx = mush_staticaabb_get_idx(*bpos.pos),
		end_idx = mush_staticaabb_get_idx(mushcoords_get_end_of_contiguous_range(
			MUSH_STATICAABB_END, bpos.pos, bpos.bounds->end, bpos.bounds->beg,
			&hit_end, MUSH_STATICAABB_BEG, MUSH_STATICAABB_BEG));

	assert (beg_idx <= end_idx);

	f((mush_arr_mushcell){space->static_box.array, end_idx - beg_idx + 1},
	  caller_data);
	return hit_end;
}

// Passes some extra data to the function, for matching array index
// calculations with the location of the mushcell* (probably quite specific to
// file loading, where this is used):
//
// - The width and area of the enclosing box.
//
// - The indices in the mushcell* of the previous line and page (note: always
//   zero or negative (thus big numbers, since unsigned)).
//
// - Whether a new line or page was just reached, with one bit for each boolean
//   (LSB for line, next-most for page). This may be updated by the function to
//   reflect that it's done with the line/page.
void mushspace_mapex_no_place(
	mushspace* space, const mush_aabb* aabb, void* fg,
	void(*f)(mush_arr_mushcell, void*, size_t,size_t, size_t,size_t, uint8_t*),
	void(*g)(size_t, void*))
{
	mushcoords        pos = aabb->bounds.beg;
	mush_bounded_pos bpos = {&aabb->bounds, &pos};

	for (;;) next_pos: {
		if (mush_staticaabb_contains(pos)) {
			if (mushspace_mapex_in_static(space, bpos, fg, f))
				return;
			else
				goto next_pos;
		}

		for (size_t b = 0; b < space->box_count; ++b) {
			mush_caabb_idx box = mushspace_get_caabb_idx(space, b);

			if (!mush_bounds_contains(&box.aabb->bounds, pos))
				continue;

			if (mushspace_mapex_in_box(space, bpos, box, fg, f))
				return;
			else
				goto next_pos;
		}

		// No hits for pos: find the next pos we can hit, or stop if there's
		// nothing left.
		size_t skipped = 0;
		bool found = mushspace_get_next_in(space, aabb, &pos, &skipped);
		if (g)
			g(skipped, fg);
		if (!found)
			return;
	}
}
bool mushspace_mapex_in_box(
	mushspace* space, mush_bounded_pos bpos,
	mush_caabb_idx cai,
	void* caller_data,
	void(*f)(mush_arr_mushcell, void*, size_t,size_t, size_t,size_t, uint8_t*))
{
	// Self-initialize: they're used uninitialized in Unefunge (and area in
	// Befunge). This method appears to silence -Wuninitialized in both GCC and
	// Clang.
	size_t width = *&width, area = *&area, page_start = *&page_start;

	const mush_aabb   *box    = cai.aabb;
	const mush_bounds *bounds = bpos.bounds;
	mushcoords        *pos    = bpos.pos;

	// These depend on the original pos and thus have to be initialized before
	// the call to mushcoords_get_end_of_contiguous_range.

	// {box->bounds.beg.x, pos->y, pos->z}
	mushcoords ls = *pos;
	ls.x = box->bounds.beg.x;

#if MUSHSPACE_DIM >= 2
	// {box->bounds.beg.x, box->bounds.beg.y, pos->z}
	mushcoords ps = box->bounds.beg;
	memcpy(ps.v + 2, pos->v + 2, (MUSHSPACE_DIM - 2) * sizeof(mushcell));
#endif

#if MUSHSPACE_DIM >= 2
	const mushcell prev_y = pos->y;
#if MUSHSPACE_DIM >= 3
	const mushcell prev_z = pos->z;
#endif
#endif

	mush_bounds tes = box->bounds;
	mush_bounds_tessellate(
		&tes, *pos,
		(mush_carr_mush_bounds){(const mush_bounds*)space->boxen, cai.idx});
	mush_bounds_tessellate1(&tes, *pos, &MUSH_STATICAABB_BOUNDS);

	bool hit_end;
	const size_t
		beg_idx = mush_aabb_get_idx(box, *pos),
		end_idx = mush_aabb_get_idx(box, mushcoords_get_end_of_contiguous_range(
			tes.end, pos, bounds->end,
			bounds->beg, &hit_end, tes.beg, box->bounds.beg));

	assert (beg_idx <= end_idx);

	uint8_t hit = 0;

	// Unefunge needs this to skip leading spaces.
	const size_t line_start = mush_aabb_get_idx(box, ls) - beg_idx;

#if MUSHSPACE_DIM >= 2
	width = box->width;
	hit |= (pos->x == bounds->beg.x && pos->y != ls.y) << 0;

	// Befunge needs this to skip leading newlines.
	page_start = mush_aabb_get_idx(box, ps) - beg_idx;
#endif
#if MUSHSPACE_DIM >= 3
	area = box->area;
	hit |= (pos->y == bounds->beg.y && pos->z != ls.z) << 1;
#endif

	f((mush_arr_mushcell){box->data, end_idx - beg_idx + 1},
	  caller_data, width, area, line_start, page_start, &hit);

#if MUSHSPACE_DIM >= 2
	if (hit == 0x01 && pos->y == prev_y) {
		// f hit an EOL and pos->y hasn't been bumped, so bump it.
		pos->x = bounds->beg.x;
		if ((pos->y = mushcell_inc(pos->y)) > bounds->end.y) {
			#if MUSHSPACE_DIM >= 3
				goto bump_z;
			#else
				hit_end = true;
			#endif
		}
	}
#endif
#if MUSHSPACE_DIM >= 3
	if (hit == 0x02 && pos->z == prev_z) {
		// Ditto for EOP.
		pos->x = bounds->beg.x;
bump_z:
		pos->y = bounds->beg.y;
		if ((pos->z = mushcell_inc(pos->z)) > bounds->end.z)
			hit_end = true;
	}
#endif
	return hit_end;
}
bool mushspace_mapex_in_static(
	mushspace* space, mush_bounded_pos bpos,
	void* caller_data,
	void(*f)(mush_arr_mushcell, void*, size_t,size_t, size_t,size_t, uint8_t*))
{
	size_t width = *&width, area = *&area, page_start = *&page_start;

	const mush_bounds *bounds = bpos.bounds;
	mushcoords        *pos    = bpos.pos;

	mushcoords ls = *pos;
	ls.x = MUSH_STATICAABB_BEG.x;

#if MUSHSPACE_DIM >= 2
	mushcoords ps = MUSH_STATICAABB_BEG;
	memcpy(ps.v + 2, pos->v + 2, (MUSHSPACE_DIM - 2) * sizeof(mushcell));
#endif

#if MUSHSPACE_DIM >= 2
	const mushcell prev_y = pos->y;
#if MUSHSPACE_DIM >= 3
	const mushcell prev_z = pos->z;
#endif
#endif

	bool hit_end;
	size_t
		beg_idx = mush_staticaabb_get_idx(*pos),
		end_idx = mush_staticaabb_get_idx(mushcoords_get_end_of_contiguous_range(
			MUSH_STATICAABB_END, pos, bounds->end,
			bounds->beg, &hit_end, MUSH_STATICAABB_BEG, MUSH_STATICAABB_BEG));

	assert (beg_idx <= end_idx);

	uint8_t hit = 0;

	const size_t line_start = mush_staticaabb_get_idx(ls) - beg_idx;

#if MUSHSPACE_DIM >= 2
	width = MUSH_STATICAABB_SIZE.x;
	hit |= (pos->x == bounds->beg.x && pos->y != ls.y) << 0;

	page_start = mush_staticaabb_get_idx(ps) - beg_idx;
#endif
#if MUSHSPACE_DIM >= 3
	area = MUSH_STATICAABB_SIZE.x * MUSH_STATICAABB_SIZE.y;
	hit |= (pos->y == bounds->beg.y && pos->z != ls.z) << 1;
#endif

	f((mush_arr_mushcell){space->static_box.array, end_idx - beg_idx + 1},
	  caller_data, width, area, line_start, page_start, &hit);

#if MUSHSPACE_DIM >= 2
	if (hit == 0x01 && pos->y == prev_y) {
		pos->x = bounds->beg.x;
		if ((pos->y = mushcell_inc(pos->y)) > bounds->end.y) {
			#if MUSHSPACE_DIM >= 3
				goto bump_z;
			#else
				hit_end = true;
			#endif
		}
	}
#endif
#if MUSHSPACE_DIM >= 3
	if (hit == 0x02 && pos->z == prev_z) {
		pos->x = bounds->beg.x;
bump_z:
		pos->y = bounds->beg.y;
		if ((pos->z = mushcell_inc(pos->z)) > bounds->end.z)
			hit_end = true;
	}
#endif
	return hit_end;
}

// The next (linewise) allocated point after *pos which is also within the
// given AABB. Updates *skipped to reflect the number of unallocated cells
// skipped.
//
// Assumes that the point, if it exists, was allocated within some box: doesn't
// look at bakaabb at all.
static bool mushspace_get_next_in(
	const mushspace* space, const mush_aabb* aabb,
	mushcoords* pos, size_t* skipped)
{
restart:
	{
		assert (!mush_staticaabb_contains(*pos));
		assert (!mushspace_find_box(space, *pos));
	}

	const size_t box_count = space->box_count;

	// A value of box_count here refers to the static box.
	//
	// Separate solutions for the best non-wrapping and the best wrapping
	// coordinate, with the wrapping coordinate used only if a non-wrapping
	// solution is not found.
	mush_cell_idx
		best_coord   = {.idx = box_count + 1},
		best_wrapped = {.idx = box_count + 1};

	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {

		// Go through every box, starting from the static one.

		mushspace_get_next_in1(i, &aabb->bounds, pos->v[i], box_count,
		                       MUSH_STATICAABB_BEG, box_count,
		                       &best_coord, &best_wrapped);

		for (mushucell b = 0; b < box_count; ++b)
			mushspace_get_next_in1(i, &aabb->bounds, pos->v[i], box_count,
			                       space->boxen[b].bounds.beg, b,
			                       &best_coord, &best_wrapped);

		if (best_coord.idx > box_count) {
			if (best_wrapped.idx > box_count) {
				// No solution: look along the next axis.
				continue;
			}

			// Take the wrapping solution as it's the only one available.
			best_coord = best_wrapped;
		}

		const mushcoords old = *pos;

		memcpy(pos->v, aabb->bounds.beg.v, i * sizeof(mushcell));
		pos->v[i] = best_coord.cell;

		// Old was already a space, or we wouldn't've called this function in the
		// first place. (See assertions.) Hence skipped is always at least one.
		++*skipped;

		mushdim j;
#if MUSHSPACE_DIM >= 2
		for (j = 0; j < MUSHSPACE_DIM-1; ++j)
			*skipped +=   mushcell_sub(aabb->bounds.end.v[j], old.v[j])
			            * mush_aabb_volume_on(aabb, j);
#else
		// Avoid "condition is always true" warnings by doing this instead of the
		// above loop.
		j = MUSHSPACE_DIM - 1;
#endif

		skipped +=   mushcell_sub(mushcell_sub(pos->v[j], old.v[j]), 1)
		           * mush_aabb_volume_on(aabb, j);

		// When memcpying pos->v above, we may not end up in any box.

		// If we didn't memcpy it's a guaranteed hit.
		if (!i)
			return true;

		// If we ended up in the box, that's fine too.
		if (   best_coord.idx < box_count
		    && mush_bounds_contains(&space->boxen[best_coord.idx].bounds, *pos))
			return true;

		// If we ended up in some other box, that's also fine.
		if (mush_staticaabb_contains(*pos))
			return true;
		if (mushspace_find_box(space, *pos))
			return true;

		// Otherwise, go again with the new *pos.
		goto restart;
	}
	return false;
}
static void mushspace_get_next_in1(
	mushucell x, const mush_bounds* bounds, mushcell posx, size_t box_count,
	mushcoords box_beg, size_t box_idx,
	mush_cell_idx* best_coord, mush_cell_idx* best_wrapped)
{
	assert (best_wrapped->cell <= best_coord->cell);

	// If the box begins later than the best solution we've found, there's no
	// point in looking further into it.
	if (box_beg.v[x] >= best_coord->cell && best_coord->idx == box_count+1)
		return;

	// If this box doesn't overlap with the AABB we're interested in, skip it.
	if (!mush_bounds_safe_contains(bounds, box_beg))
		return;

	// If pos has crossed an axis within the AABB, prevent us from grabbing a
	// new pos on the other side of the axis we're wrapped around, or we'll just
	// keep looping around that axis.
	if (posx < bounds->beg.v[x] && box_beg.v[x] > bounds->end.v[x])
		return;

	// If the path from pos to bounds->end requires a wraparound, take the
	// global minimum box.beg as a last-resort option if nothing else is found,
	// so that we wrap around if there's no non-wrapping solution.
	//
	// Note that best_wrapped->cell <= best_coord->cell so we can safely test
	// this after the first best_coord->cell check.
	if (   posx > bounds->end.v[x]
	    && (   box_beg.v[x] < best_wrapped->cell
	        || best_wrapped->idx == box_count+1))
	{
		best_wrapped->cell = box_beg.v[x];
		best_wrapped->idx  = box_idx;

	// The ordinary best solution is the minimal box.beg greater than pos.
	} else if (box_beg.v[x] > posx) {
		best_coord->cell = box_beg.v[x];
		best_coord->idx  = box_idx;
	}
}
