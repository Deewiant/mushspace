// File created: 2012-01-28 00:01:32

#include "space/place-box-for.98.h"

#include <assert.h>

#include "space/heuristic-constants.98.h"
#include "space/place-box.98.h"

static void mushspace_get_box_for(mushspace*, mushcoords, mush_aabb*);

static bool mushspace_get_box_along_recent_line_for(
	mushspace*, mushcoords, mush_aabb*);

static bool mushspace_get_box_along_recent_volume_for(
	const mushspace*, mushcoords, mush_aabb*);

static bool mushspace_extend_big_sequence_start_for(
	const mushspace*, mushcoords, const mush_bounds*, mush_aabb*);

static bool mushspace_extend_first_placed_big_for(
	const mushspace*, mushcoords, const mush_bounds*, mush_aabb*);

bool mushspace_place_box_for(
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
