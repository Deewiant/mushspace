// File created: 2011-08-06 15:44:53

#include "space.all.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "stdlib.any.h"
#include "types.h"
#include "space/heuristic-constants.98.h"
#include "space/place-box.98.h"

#define mushspace_find_beg_in   MUSHSPACE_CAT(mushspace,_find_beg_in)
#define mushspace_find_end_in   MUSHSPACE_CAT(mushspace,_find_end_in)
#define mushspace_place_box_for MUSHSPACE_CAT(mushspace,_place_box_for)
#define mushspace_get_box_for   MUSHSPACE_CAT(mushspace,_get_box_for)
#define mushspace_get_box_along_recent_volume_for \
	MUSHSPACE_CAT(mushspace,_get_box_along_recent_volume_for)
#define mushspace_get_box_along_recent_line_for \
	MUSHSPACE_CAT(mushspace,_get_box_along_recent_line_for)
#define mushspace_extend_big_sequence_start_for \
	MUSHSPACE_CAT(mushspace,_extend_big_sequence_start_for)
#define mushspace_extend_first_placed_big_for \
	MUSHSPACE_CAT(mushspace,_extend_first_placed_big_for)

static bool mushspace_find_beg_in(
	mushcoords*, mushdim, const mush_bounds*,
	mushcell(*)(const void*, mushcoords), const void*);

static void mushspace_find_end_in(
	mushcoords*, mushdim, const mush_bounds*,
	mushcell(*)(const void*, mushcoords), const void*);

static bool mushspace_place_box_for(mushspace*, mushcoords, mush_aabb**);

static void mushspace_get_box_for(mushspace*, mushcoords, mush_aabb*);

static bool mushspace_get_box_along_recent_line_for(
	mushspace*, mushcoords, mush_aabb*);

static bool mushspace_get_box_along_recent_volume_for(
	const mushspace*, mushcoords, mush_aabb*);

static bool mushspace_extend_big_sequence_start_for(
	const mushspace*, mushcoords, const mush_bounds*, mush_aabb*);

static bool mushspace_extend_first_placed_big_for(
	const mushspace*, mushcoords, const mush_bounds*, mush_aabb*);

mushspace* mushspace_init(void* vp, mushstats* stats) {
	mushspace *space = vp ? vp : malloc(sizeof *space);
	if (!space)
		return NULL;

	if (!(space->stats = stats ? stats : malloc(sizeof *space->stats))) {
		free(space);
		return NULL;
	}

	space->invalidatees = NULL;

	space->box_count = 0;
	space->boxen     = NULL;
	space->bak.data  = NULL;

	// Placate valgrind and such: it's not necessary to define these before the
	// first use.
	space->last_beg = space->last_end = MUSHCOORDS(0,0,0);

	mush_anamnesic_ring_init(&space->recent_buf);

	mushcell_space(
		space->static_box.array, MUSH_ARRAY_LEN(space->static_box.array));
	return space;
}

void mushspace_free(mushspace* space) {
	for (size_t i = space->box_count; i--;)
		free(space->boxen[i].data);
	free(space->boxen);
	mush_bakaabb_free(&space->bak);
}

mushspace* mushspace_copy(void* vp, const mushspace* space, mushstats* stats) {
	mushspace *copy = vp ? vp : malloc(sizeof *copy);
	if (!copy)
		return NULL;

	// Easiest to do here, so we don't have to worry about whether we need to
	// free copy->stats or not.
	if (space->bak.data && !mush_bakaabb_copy(&copy->bak, &space->bak)) {
		free(copy);
		return NULL;
	}

	copy->stats = stats ? stats : malloc(sizeof *copy->stats);
	if (!copy->stats) {
		free(copy);
		return NULL;
	}

	memcpy(copy, space, sizeof *copy);

	copy->boxen = malloc(copy->box_count * sizeof *copy->boxen);
	memcpy(copy->boxen, space->boxen, copy->box_count * sizeof *copy->boxen);

	for (size_t i = 0; i < copy->box_count; ++i) {
		mush_aabb *box = &copy->boxen[i];
		const mushcell *orig = box->data;
		box->data = malloc(box->size * sizeof *box->data);
		memcpy(box->data, orig, box->size * sizeof *box->data);
	}

	// Invalidatees refer to the original space, not the copy.
	copy->invalidatees = NULL;

	return copy;
}

mushcell mushspace_get(const mushspace* space, mushcoords c) {
	if (mush_staticaabb_contains(c))
		return mush_staticaabb_get(&space->static_box, c);

	const mush_aabb* box;
	if ((box = mushspace_find_box(space, c)))
		return mush_aabb_get(box, c);

	if (space->bak.data)
		return mush_bakaabb_get(&space->bak, c);

	return ' ';
}

int mushspace_put(mushspace* space, mushcoords p, mushcell c) {
	if (mush_staticaabb_contains(p)) {
		mush_staticaabb_put(&space->static_box, p, c);
		return MUSH_ERR_NONE;
	}

	mush_aabb* box;
	if (    (box = mushspace_find_box(space, p))
	     || mushspace_place_box_for  (space, p, &box))
	{
		mush_aabb_put(box, p, c);
		return MUSH_ERR_NONE;
	}

	if (!space->bak.data && !mush_bakaabb_init(&space->bak, p))
		return MUSH_ERR_OOM;

	if (!mush_bakaabb_put(&space->bak, p, c))
		return MUSH_ERR_OOM;

	return MUSH_ERR_NONE;
}

void mushspace_get_loose_bounds(
	const mushspace* space, mushcoords* beg, mushcoords* end)
{
	*beg = MUSH_STATICAABB_BEG;
	*end = MUSH_STATICAABB_END;

	for (size_t i = 0; i < space->box_count; ++i) {
		mushcoords_min_into(beg, space->boxen[i].bounds.beg);
		mushcoords_max_into(end, space->boxen[i].bounds.end);
	}
	if (space->bak.data) {
		mushcoords_min_into(beg, space->bak.bounds.beg);
		mushcoords_max_into(end, space->bak.bounds.end);
	}
}

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

void mushspace_invalidate_all(mushspace* space) {
	void (**i)(void*) = space->invalidatees;
	void  **d         = space->invalidatees_data;
	if (i)
		while (*i)
			(*i++)(*d++);
}

mush_caabb_idx mushspace_get_caabb_idx(const mushspace* sp, size_t i) {
	return (mush_caabb_idx){&sp->boxen[i], i};
}

mush_aabb* mushspace_find_box(const mushspace* space, mushcoords c) {
	size_t i;
	return mushspace_find_box_and_idx(space, c, &i);
}

mush_aabb* mushspace_find_box_and_idx(
	const mushspace* space, mushcoords c, size_t* pi)
{
	for (size_t i = 0; i < space->box_count; ++i)
		if (mush_bounds_contains(&space->boxen[i].bounds, c))
			return &space->boxen[*pi = i];
	return NULL;
}

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

void mushspace_remove_boxes(mushspace* space, size_t i, size_t j) {
	assert (i <= j);
	assert (j < space->box_count);

	for (size_t k = i; k <= j; ++k)
		free(space->boxen[k].data);

	size_t new_len = space->box_count -= j - i + 1;
	if (i < new_len) {
		mush_aabb *arr = space->boxen;
		memmove(arr + i, arr + j + 1, (new_len - i) * sizeof *arr);
	}
}

static bool mushspace_place_box_for(
	mushspace* space, mushcoords c, mush_aabb** placed)
{
	if (space->box_count >= MAX_PLACED_BOXEN)
		return false;

	mush_aabb aabb;
	mushspace_get_box_for(space, c, &aabb);

	if (!mushspace_place_box(space, &aabb, &c, placed))
		return false;

	mush_anamnesic_ring_push(
		&space->recent_buf, (mush_memory){.placed = (*placed)->bounds, c});

	return true;
}
static void mushspace_get_box_for(
	mushspace* space, mushcoords c, mush_aabb* aabb)
{
	for (size_t b = 0; b < space->box_count; ++b)
		assert (!mush_bounds_contains(&space->boxen[b].bounds, c));

	if (space->recent_buf.full) {
		if (space->just_placed_big) {
			if (mushspace_get_box_along_recent_volume_for(space, c, aabb))
				goto end;
		} else
			if (mushspace_get_box_along_recent_line_for(space, c, aabb))
				goto end;
	}

	space->just_placed_big = false;

	mush_bounds bounds = {mushcoords_subs_clamped(c, NEWBOX_PAD),
	                      mushcoords_adds_clamped(c, NEWBOX_PAD)};
	mush_aabb_make(aabb, &bounds);

end:
	assert (mush_bounds_safe_contains(&aabb->bounds, c));
}
static bool mushspace_get_box_along_recent_line_for(
	mushspace* space, mushcoords c, mush_aabb* aabb)
{
	// Detect mushspace_put patterns where we seem to be moving in a straight
	// line and allocate one long, narrow box. This is the first big box
	// allocation so it has to look for a sequence of small boxes in
	// space->recent_buf:
	//
	// AAABBBCCC
	// AAABBBCCCc
	// AAABBBCCC
	//
	// Where A, B, and C are distinct boxes and c is c.

	assert (space->recent_buf.full);

	mush_memory recents[MUSH_ANAMNESIC_RING_SIZE];
	mush_anamnesic_ring_read(&space->recent_buf, recents);

	// Find the axis along which the first two recent placements are aligned, if
	// any, and note whether it was along the positive or negative direction.
	bool positive = *&positive;
	mushdim axis = MUSHSPACE_DIM;

	for (mushdim d = 0; d < MUSHSPACE_DIM; ++d) {
		mushcell diff = mushcell_sub(recents[1].c.v[d], recents[0].c.v[d]);

		if (axis == MUSHSPACE_DIM) {
			if (diff > NEWBOX_PAD && diff <= NEWBOX_PAD + BIG_SEQ_MAX_SPACING) {
				positive = true;
				axis = d;
				continue;
			}
			if (diff < -NEWBOX_PAD && diff >= -NEWBOX_PAD - BIG_SEQ_MAX_SPACING) {
				positive = false;
				axis = d;
				continue;
			}
		}
		if (diff)
			return false;
	}
	if (axis == MUSHSPACE_DIM)
		return false;

	// Check that the other recents are aligned on the same line.
	for (size_t i = 1; i < MUSH_ARRAY_LEN(recents) - 1; ++i) {

		const mushcell *a = recents[i].c.v, *b = recents[i+1].c.v;

		for (mushdim d = 0; d < axis; ++d)
			if (mushcell_sub(b[d], a[d]))
				return false;
		for (mushdim d = axis+1; d < MUSHSPACE_DIM; ++d)
			if (mushcell_sub(b[d], a[d]))
				return false;

		mushcell diff = mushcell_sub(b[axis], a[axis]);
		if (!positive)
			diff *= -1;

		if (diff <= NEWBOX_PAD || diff > NEWBOX_PAD + BIG_SEQ_MAX_SPACING)
			return false;
	}

	space->just_placed_big    = true;
	space->first_placed_big   = c;
	space->big_sequence_start = recents[0].c;

	mush_bounds bounds = {c,c};
	if (positive) mushcell_add_into(&bounds.end.v[axis], BIGBOX_PAD);
	else          mushcell_sub_into(&bounds.beg.v[axis], BIGBOX_PAD);

	mush_aabb_make_unsafe(aabb, &bounds);
	return true;
}
static bool mushspace_get_box_along_recent_volume_for(
	const mushspace* space, mushcoords c, mush_aabb* aabb)
{
	assert (space->recent_buf.full);
	assert (space->just_placed_big);

	const mush_bounds *last =
		&mush_anamnesic_ring_last(&space->recent_buf)->placed;

	if (mushspace_extend_big_sequence_start_for(space, c, last, aabb))
		return true;

	if (mushspace_extend_first_placed_big_for(space, c, last, aabb))
		return true;

	return false;
}
static bool mushspace_extend_big_sequence_start_for(
	const mushspace* space, mushcoords c, const mush_bounds* last,
	mush_aabb* aabb)
{
	// See if c is at big_sequence_start except for one axis, along which it's
	// just past last->end or last->beg. The typical case for this is the
	// following:
	//
	// sBBBBBBc
	//
	// Where B are boxes, s is space->big_sequence_start, and c is c.

	bool positive = *&positive;
	mushdim axis = MUSHSPACE_DIM;

	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
		if (axis == MUSHSPACE_DIM) {
			if (   c.v[i] >  last->end.v[i]
			    && c.v[i] <= last->end.v[i] + BIG_SEQ_MAX_SPACING)
			{
				positive = true;
				axis = i;
				continue;
			}
			if (   c.v[i] <  last->beg.v[i]
			    && c.v[i] >= last->beg.v[i] - BIG_SEQ_MAX_SPACING)
			{
				positive = false;
				axis = i;
				continue;
			}
		}
		if (c.v[i] != space->big_sequence_start.v[i])
			return false;
	}
	if (axis == MUSHSPACE_DIM)
		return false;

	// Extend last along the axis where c was outside it.
	mush_bounds bounds = *last;
	if (positive) mushcell_add_into(&bounds.end.v[axis], BIGBOX_PAD);
	else          mushcell_sub_into(&bounds.beg.v[axis], BIGBOX_PAD);

	mush_aabb_make_unsafe(aabb, &bounds);
	return true;
}

static bool mushspace_extend_first_placed_big_for(
	const mushspace* space, mushcoords c, const mush_bounds* last,
	mush_aabb* aabb)
{
	// Match against space->first_placed_big. This is for the case when we've
	// made a few non-big boxes and then hit a new dimension for the first time
	// in a location which doesn't match with the actual box. E.g.:
	//
	// BsBfBBB
	// BBBc  b
	//  n
	//
	// B being boxes, s being space->big_sequence_start, f being
	// space->first_placed_big, and c being c. b and n are explained below.
	//
	// So what we want is that c is "near" (up to BIG_SEQ_MAX_SPACING away from)
	// space->first_placed_big on one axis. In the diagram, this corresponds to
	// c being one line below f.
	//
	// We also want it to match space->first_placed_big on the other axes. This
	// prevents us from matching a point like b in the diagram and ensures that
	// c is contained within the resulting box.

#if MUSHSPACE_DIM == 1
	(void)space; (void)c; (void)last; (void)aabb;
	return false;
#else
	bool positive = *&positive;
	mushdim axis = MUSHSPACE_DIM;

	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
		if (axis == MUSHSPACE_DIM) {
			if (   c.v[i] >  space->first_placed_big.v[i]
			    && c.v[i] <= space->first_placed_big.v[i] + BIG_SEQ_MAX_SPACING)
			{
				positive = true;
				axis = i;
				continue;
			}
			if (   c.v[i] <  space->first_placed_big.v[i]
			    && c.v[i] >= space->first_placed_big.v[i] - BIG_SEQ_MAX_SPACING)
			{
				positive = false;
				axis = i;
				continue;
			}
		}
		if (c.v[i] != space->first_placed_big.v[i])
			return false;
	}
	if (axis == MUSHSPACE_DIM)
		return false;

	mush_bounds bounds;
	if (positive) {
		bounds.beg = space->big_sequence_start;
		bounds.end = last->end;

		mushcell_add_into(&bounds.end.v[axis], BIGBOX_PAD);
	} else {
		bounds.beg = last->beg;
		bounds.end = space->big_sequence_start;

		mushcell_sub_into(&bounds.beg.v[axis], BIGBOX_PAD);
	}
	mush_aabb_make_unsafe(aabb, &bounds);
	return true;
#endif
}

bool mushspace_add_invalidatee(mushspace* space, void(*i)(void*), void* d) {
	size_t n = 0;
	void (**is)(void*) = space->invalidatees;
	if (is)
		while (*is++)
			++n;

	is = realloc(is, n * sizeof *is);
	if (!is)
		return false;

	void **id = realloc(space->invalidatees_data, n * sizeof *id);
	if (!id) {
		free(is);
		return false;
	}

	space->invalidatees      = is;
	space->invalidatees_data = id;

	is[n] = i;
	id[n] = d;
	return true;
}
