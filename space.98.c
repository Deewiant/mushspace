// File created: 2011-08-06 15:44:53

#include "space.all.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "anamnesic_ring.98.h"
#include "aabb.98.h"
#include "bakaabb.98.h"
#include "coords.all.h"
#include "staticaabb.all.h"
#include "stats.any.h"
#include "stdlib.any.h"

// Threshold for switching to mush_bakaabb. Only limits mushspace_put, not
// mushspace_load_string.
#define MAX_PLACED_BOXEN 64

// Padding of box created by default when mushspace_putting to an unallocated
// area. The size of the resulting box will be NEWBOX_PAD+1 along each axis.
// (Clamped to the edges of space to avoid allocating more than one box.)
#define NEWBOX_PAD 8

// One-directional padding of box created when heuristics determine that
// something like an array is being written by mushspace_put.
#define BIGBOX_PAD 512

// How much leeway we have in heuristic big box requirement testing. If you're
// mushspace_putting a one-dimensional area, this is the maximum stride you can
// use for it to be noticed, causing a big box to be placed.
//
// Implicitly defines an ACCEPTABLE_WASTE for BIGBOXes: it's
// (BIG_SEQ_MAX_SPACING - 1) * BIGBOX_PAD^(MUSHSPACE_DIM-1).
//
// This is a distance between two cells, not the number of spaces between them,
// and thus should always be at least 1.
#define BIG_SEQ_MAX_SPACING 4

// How much space we can live with allocating "uselessly" when joining AABBs
// together. For example, in Unefunge, this is the maximum distance allowed
// between two boxes.
//
// A box 5 units wide, otherwise of size NEWBOX_PAD.
#define ACCEPTABLE_WASTE_Y (MUSHSPACE_DIM >= 2 ? NEWBOX_PAD : 1)
#define ACCEPTABLE_WASTE_Z (MUSHSPACE_DIM >= 3 ? NEWBOX_PAD : 1)
#define ACCEPTABLE_WASTE   (5 * ACCEPTABLE_WASTE_Y * ACCEPTABLE_WASTE_Z)

struct mushspace {
	mush_anamnesic_ring recent_buf;
	bool just_placed_big;
	mushcoords big_sequence_start, first_placed_big;

	void (**invalidatees)(void*);
	void  **invalidatees_data;

	mushcoords last_beg, last_end;

	mushstats *stats;

	size_t box_count;

	mush_aabb    *boxen;
	mush_bakaabb  bak;

	mush_staticaabb static_box;
};

const size_t MUSHSPACE_CAT(mushspace,_size) = sizeof(mushspace);

#define mushspace_find_box          MUSHSPACE_CAT(mushspace,_find_box)
#define mushspace_remove_boxes      MUSHSPACE_CAT(mushspace,_remove_boxes)
#define mushspace_place_box         MUSHSPACE_CAT(mushspace,_place_box)
#define mushspace_place_box_for     MUSHSPACE_CAT(mushspace,_place_box_for)
#define mushspace_get_box_for       MUSHSPACE_CAT(mushspace,_get_box_for)
#define mushspace_really_place_box  MUSHSPACE_CAT(mushspace,_really_place_box)
#define mushspace_subsume_contains  MUSHSPACE_CAT(mushspace,_subsume_contains)
#define mushspace_subsume_fusables  MUSHSPACE_CAT(mushspace,_subsume_fusables)
#define mushspace_subsume_disjoint  MUSHSPACE_CAT(mushspace,_subsume_disjoint)
#define mushspace_subsume_overlaps  MUSHSPACE_CAT(mushspace,_subsume_overlaps)
#define mushspace_min_max_size      MUSHSPACE_CAT(mushspace,_min_max_size)
#define mushspace_cheaper_to_alloc  MUSHSPACE_CAT(mushspace,_cheaper_to_alloc)
#define mushspace_map_no_place      MUSHSPACE_CAT(mushspace,_map_no_place)
#define mushspace_map_in_static     MUSHSPACE_CAT(mushspace,_map_in_static)
#define mushspace_map_in_box        MUSHSPACE_CAT(mushspace,_map_in_box)
#define mushspace_mapex_no_place    MUSHSPACE_CAT(mushspace,_mapex_no_place)
#define mushspace_mapex_in_static   MUSHSPACE_CAT(mushspace,_mapex_in_static)
#define mushspace_mapex_in_box      MUSHSPACE_CAT(mushspace,_mapex_in_box)
#define mushspace_get_next_in       MUSHSPACE_CAT(mushspace,_get_next_in)
#define mushspace_get_next_in1      MUSHSPACE_CAT(mushspace,_get_next_in1)
#define mushspace_get_aabbs         MUSHSPACE_CAT(mushspace,_get_aabbs)
#define mushspace_newline           MUSHSPACE_CAT(mushspace,_newline)
#define mushspace_get_aabbs_binary  MUSHSPACE_CAT(mushspace,_get_aabbs_binary)
#define mushspace_binary_load_arr   MUSHSPACE_CAT(mushspace,_binary_load_arr)
#define mushspace_binary_load_blank MUSHSPACE_CAT(mushspace,_binary_load_blank)
#define mushspace_load_arr          MUSHSPACE_CAT(mushspace,_load_arr)
#define mushspace_load_blank        MUSHSPACE_CAT(mushspace,_load_blank)
#define mushspace_get_box_along_recent_volume_for \
	MUSHSPACE_CAT(mushspace,_get_box_along_recent_volume_for)
#define mushspace_get_box_along_recent_line_for \
	MUSHSPACE_CAT(mushspace,_get_box_along_recent_line_for)
#define mushspace_extend_big_sequence_start_for \
	MUSHSPACE_CAT(mushspace,_extend_big_sequence_start_for)
#define mushspace_extend_first_placed_big_for \
	MUSHSPACE_CAT(mushspace,_extend_first_placed_big_for)
#define mushspace_disjoint_mms_validator \
	MUSHSPACE_CAT(mushspace,_disjoint_mms_validator)
#define mushspace_overlaps_mms_validator \
	MUSHSPACE_CAT(mushspace,_overlaps_mms_validator)
#define mushspace_valid_min_max_size \
	MUSHSPACE_CAT(mushspace,_valid_min_max_size)
#define mushspace_consume_and_subsume \
	MUSHSPACE_CAT(mushspace,_consume_and_subsume)
#define mushspace_irrelevize_subsumption_order \
	MUSHSPACE_CAT(mushspace,_irrelevize_subsumption_order)

static mush_aabb* mushspace_find_box(const mushspace*, mushcoords);

// Removes the given range of boxes, inclusive.
static void mushspace_remove_boxes(mushspace*, size_t, size_t);

static bool mushspace_place_box(
	mushspace*, mush_aabb*, mushcoords*, mush_aabb**);

static bool mushspace_place_box_for(mushspace*, mushcoords, mush_aabb**);

static void mushspace_get_box_for(mushspace*, mushcoords, mush_aabb*);

static bool mushspace_get_box_along_recent_line_for(
	mushspace*, mushcoords, mush_aabb*);

static bool mushspace_get_box_along_recent_volume_for(
	const mushspace*, mushcoords, mush_aabb*);

static bool mushspace_extend_big_sequence_start_for(
	const mushspace*, mushcoords, const mush_aabb*, mush_aabb*);

static bool mushspace_extend_first_placed_big_for(
	const mushspace*, mushcoords, const mush_aabb*, mush_aabb*);

static mush_aabb* mushspace_really_place_box(mushspace*, mush_aabb*);

static void mushspace_subsume_contains(
	mushspace*, size_t*, size_t*, size_t*, const mush_aabb*,
	size_t*, size_t*, size_t*);

static bool mushspace_subsume_fusables(
	mushspace*, size_t*, size_t*, size_t*, mush_aabb*,
	size_t*, size_t*, size_t*);

static bool mushspace_subsume_disjoint(
	mushspace*, size_t*, size_t*, size_t*, mush_aabb*,
	size_t*, size_t*, size_t*);

static bool mushspace_disjoint_mms_validator(
	const mush_aabb*, const mush_aabb*, size_t, void*);

static bool mushspace_subsume_overlaps(
	mushspace*, size_t*, size_t*, size_t*, mush_aabb*,
	size_t*, size_t*, size_t*);

static bool mushspace_overlaps_mms_validator(
	const mush_aabb*, const mush_aabb*, size_t, void*);

static void mushspace_min_max_size(
	mushcoords*, mushcoords*, size_t*, size_t*, size_t*,
	size_t, const mush_aabb*);

static bool mushspace_valid_min_max_size(
	bool (*)(const mush_aabb*, const mush_aabb*, size_t, void*), void*,
	mushcoords*, mushcoords*, size_t*, size_t*, size_t*,
	size_t, const mush_aabb*);

static bool mushspace_cheaper_to_alloc(size_t, size_t);

static bool mushspace_consume_and_subsume(
	mushspace*, size_t*, size_t, size_t, mush_aabb*);

static void mushspace_irrelevize_subsumption_order(
	mushspace*, size_t*, size_t);

static void mushspace_map_no_place(
	mushspace*, const mush_aabb*, void*,
	void(*)(mushcell*, mushcell*, void*, mushstats*),
	void(*)(size_t, void*));

static bool mushspace_map_in_box(
	mushspace*, const mush_aabb*, mushcoords*,
	const mush_aabb*, size_t,
	void*, void(*f)(mushcell*, mushcell*, void*, mushstats*));

static bool mushspace_map_in_static(
	mushspace*, const mush_aabb*, mushcoords*,
	void*, void(*f)(mushcell*, mushcell*, void*, mushstats*));

static void mushspace_mapex_no_place(
	mushspace*, const mush_aabb*, void*,
	void(*)(mushcell*, size_t, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*),
	void(*)(size_t, void*));

static bool mushspace_mapex_in_box(
	mushspace*, const mush_aabb*, mushcoords*,
	const mush_aabb*, size_t, void*,
	void(*)(mushcell*, size_t, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*));

static bool mushspace_mapex_in_static(
	mushspace*, const mush_aabb*, mushcoords*, void*,
	void(*)(mushcell*, size_t, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*));

static bool mushspace_get_next_in(
	const mushspace*, const mush_aabb*, mushcoords*, size_t*);

static void mushspace_get_next_in1(
	mushucell, const mush_aabb*, mushcell, size_t, mushcoords, size_t,
	mushcell*, mushcell*, size_t*, size_t*);

static mush_aabb* mushspace_get_aabbs(
	const char*, size_t len, mushcoords target, bool binary, size_t* len_out);

#if MUSHSPACE_DIM >= 2
static bool mushspace_newline(
	bool*, mushcoords*, mushcoords, mush_aabb*, size_t, size_t*, size_t*,
	mushcoords, size_t*, uint8_t*);
#endif

static size_t mushspace_get_aabbs_binary(
	const char*, size_t len, mushcoords target, mush_aabb* aabbs_out);

static void mushspace_binary_load_arr(mushcell*, mushcell*, void*, mushstats*);
static void mushspace_binary_load_blank(size_t, void*);
static void mushspace_load_arr(mushcell*, size_t, void*,
                               size_t, size_t, size_t, size_t, uint8_t*,
                               mushstats*);
static void mushspace_load_blank(size_t, void*);

mushspace* MUSHSPACE_CAT(mushspace,_allocate)(void* vp, mushstats* stats) {
	mushspace *space = vp ? vp : malloc(sizeof *space);
	if (space) {
		space->invalidatees = NULL;
		space->stats        = stats ? stats : malloc(sizeof *space->stats);

		space->box_count = 0;
		space->boxen     = NULL;
		space->bak.data  = NULL;

		mush_anamnesic_ring_init(&space->recent_buf);

		mushcell_space(
			space->static_box.array, MUSH_ARRAY_LEN(space->static_box.array));
	}
	return space;
}

void MUSHSPACE_CAT(mushspace,_free)(mushspace* space) {
	for (size_t i = space->box_count; i--;)
		free(space->boxen[i].data);
	free(space->boxen);
	mush_bakaabb_free(&space->bak);
	free(space->stats);
}

mushcell mushspace_get(
#ifndef MUSH_ENABLE_STATS
	const
#endif
	mushspace* space, mushcoords c)
{
	mushstats_add(space->stats, MushStat_lookups, 1);

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
	mushstats_add(space->stats, MushStat_assignments, 1);

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

static mush_aabb* mushspace_find_box(const mushspace* space, mushcoords c) {
	for (size_t i = 0; i < space->box_count; ++i)
		if (mush_aabb_contains(&space->boxen[i], c))
			return &space->boxen[i];
	return NULL;
}

static void mushspace_remove_boxes(mushspace* space, size_t i, size_t j) {
	assert (i <= j);
	assert (j < space->box_count);

	size_t new_len = space->box_count -= j - i + 1;
	if (i < new_len) {
		mush_aabb *arr = space->boxen;
		memmove(arr + i, arr + j + 1, (new_len - i) * sizeof *arr);
	}
}

static bool mushspace_place_box(
	mushspace* space, mush_aabb* aabb,
	mushcoords* reason, mush_aabb** reason_box)
{
	assert ((reason == NULL) == (reason_box == NULL));
	if (reason)
		assert (mush_aabb_safe_contains(aabb, *reason));

	// Split the box up along any axes it wraps around on.
	mush_aabb aabbs[1 << MUSHSPACE_DIM];
	aabbs[0] = *aabb;
	size_t a = 1;

	for (size_t b = 0; b < a; ++b) {
		mush_aabb *box = &aabbs[b];
		for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
			if (box->beg.v[i] <= box->end.v[i])
				continue;

			mushcoords end = box->end;
			end.v[i] = MUSHCELL_MAX;
			mush_aabb_make_unsafe(&aabbs[a++], box->beg, end);
			box->beg.v[i] = MUSHCELL_MIN;
		}
	}

	// Then do the actual placement.
	for (size_t b = 0; b < a; ++b) {
		mush_aabb *box = &aabbs[b];

		if (   mush_staticaabb_contains(box->beg)
		    && mush_staticaabb_contains(box->end))
		{
incorporated:
			mushstats_add(space->stats, MushStat_boxes_incorporated, 1);
			continue;
		}

		for (size_t i = 0; i < space->box_count; ++i)
			if (mush_aabb_contains_box(&space->boxen[i], box))
				goto incorporated;

		mush_aabb_finalize(box);
		box = mushspace_really_place_box(space, box);
		if (box == NULL)
			return false;

		if (reason && mush_aabb_contains(box, *reason)) {
			*reason_box = box;

			// This can only happen once.
			reason = NULL;
		}

		// If it crossed bak, we need to fix things up and move any occupied
		// cells from bak (which is below all boxen) to the appropriate box
		// (which may not be *box, if it has any overlaps).
		if (!space->bak.data || !mush_bakaabb_size(&space->bak))
			continue;

		if (!mush_aabb_overlapsc(box, space->bak.beg, space->bak.end))
			continue;

		assert (box == &space->boxen[space->box_count-1]);

		bool overlaps = false;
		for (size_t b = 0; b < space->box_count-1; ++b) {
			if (mush_aabb_overlaps(box, &space->boxen[b])) {
				overlaps = true;
				break;
			}
		}

		mush_bakaabb_iter *it = mush_bakaabb_it_start(&space->bak);
		if (!it)
			return false;

		while (!mush_bakaabb_it_done(it, &space->bak)) {
			mushcoords c = mush_bakaabb_it_pos(it, &space->bak);
			if (!mush_aabb_contains(box, c)) {
				mush_bakaabb_it_next(it, &space->bak);
				continue;
			}

			mushcell v = mush_bakaabb_it_val(it, &space->bak);

			mush_bakaabb_it_remove(it, &space->bak);

			if (overlaps)
				mushspace_put(space, c, v);
			else
				mush_aabb_put(box, c, v);
		}
		mush_bakaabb_it_stop(it);
	}
	return true;
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
		&space->recent_buf, (mush_memory){.placed = **placed, c});

	return true;
}
static void mushspace_get_box_for(
	mushspace* space, mushcoords c, mush_aabb* aabb)
{
	for (size_t b = 0; b < space->box_count; ++b)
		assert (!mush_aabb_contains(&space->boxen[b], c));

	if (space->recent_buf.full) {
		if (space->just_placed_big) {
			if (mushspace_get_box_along_recent_volume_for(space, c, aabb))
				goto end;
		} else
			if (mushspace_get_box_along_recent_line_for(space, c, aabb))
				goto end;
	}

	space->just_placed_big = false;
	mush_aabb_make(aabb, mushcoords_subs_clamped(c, NEWBOX_PAD),
								mushcoords_adds_clamped(c, NEWBOX_PAD));

end:
	assert (mush_aabb_safe_contains(aabb, c));
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
	bool positive;
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

	mushcoords beg = c, end = c;
	if (positive) end.v[axis] = mushcell_add(end.v[axis], BIGBOX_PAD);
	else          beg.v[axis] = mushcell_sub(beg.v[axis], BIGBOX_PAD);

	mush_aabb_make_unsafe(aabb, beg, end);
	return true;
}
static bool mushspace_get_box_along_recent_volume_for(
	const mushspace* space, mushcoords c, mush_aabb* aabb)
{
	assert (space->recent_buf.full);
	assert (space->just_placed_big);

	const mush_aabb *last =
		&mush_anamnesic_ring_last(&space->recent_buf)->placed;

	if (mushspace_extend_big_sequence_start_for(space, c, last, aabb))
		return true;

	if (mushspace_extend_first_placed_big_for(space, c, last, aabb))
		return true;

	return false;
}
static bool mushspace_extend_big_sequence_start_for(
	const mushspace* space, mushcoords c, const mush_aabb* last,
	mush_aabb* aabb)
{
	// See if c is at big_sequence_start except for one axis, along which it's
	// just past last->end or last->beg. The typical case for this is the
	// following:
	//
	// sBBBBBBc
	//
	// Where B are boxes, s is space->big_sequence_start, and c is c.

	bool positive;
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
	mushcoords beg = last->beg, end = last->end;
	if (positive) end.v[axis] = mushcell_add(end.v[axis], BIGBOX_PAD);
	else          beg.v[axis] = mushcell_sub(beg.v[axis], BIGBOX_PAD);

	mush_aabb_make_unsafe(aabb, beg, end);
	return true;
}

static bool mushspace_extend_first_placed_big_for(
	const mushspace* space, mushcoords c, const mush_aabb* last,
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
	bool positive;
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

	mushcoords beg, end;
	if (positive) {
		beg = space->big_sequence_start;
		end = last->end;

		end.v[axis] = mushcell_add(end.v[axis], BIGBOX_PAD);
	} else {
		beg = last->beg;
		end = space->big_sequence_start;

		beg.v[axis] = mushcell_sub(beg.v[axis], BIGBOX_PAD);
	}
	mush_aabb_make_unsafe(aabb, beg, end);
	return true;
#endif
}

// Returns the placed box, which may be bigger than the given box. Returns NULL
// if memory allocation failed.
static mush_aabb* mushspace_really_place_box(mushspace* space, mush_aabb* aabb)
{
	for (size_t i = 0; i < space->box_count; ++i)
		assert (!mush_aabb_contains_box(&space->boxen[i], aabb));

	size_t *subsumees  = malloc(space->box_count * sizeof *subsumees),
	       *candidates = malloc(space->box_count * sizeof *candidates);

	if (!subsumees || !candidates) {
		free(subsumees);
		free(candidates);
		return NULL;
	}

	size_t consumee,
	       consumee_size = 0,
	       used_cells = aabb->size;

	mush_aabb consumer;
	mush_aabb_make_unsafe(&consumer, aabb->beg, aabb->end);

	// boxen that we haven't yet subsumed. Removed entries are set to
	// space->box_count.
	for (size_t i = 0; i < space->box_count; ++i)
		candidates[i] = i;

	size_t slen = 0;

	for (;;) {
		// Disjoint assumes that it comes after fusables. Some reasoning for why
		// that's probably a good idea anyway:
		//
		// F
		// FD
		// A
		//
		// F is a fusable, D is a disjoint, and A is *aabb. If we looked for
		// disjoints before fusables, we might subsume D, leaving us worse off
		// than if we'd subsumed F.

		#define PARAMS space,            \
		               candidates,       \
		               subsumees, &slen, \
		               &consumer,        \
		               &consumee, &consumee_size, &used_cells

		    mushspace_subsume_contains(PARAMS);
		if (mushspace_subsume_fusables(PARAMS)) continue;
		if (mushspace_subsume_disjoint(PARAMS)) continue;
		if (mushspace_subsume_overlaps(PARAMS)) continue;
		break;

		#undef PARAMS
	}

	free(candidates);

	// Even though consume_and_subsume might reduce the box count and generally
	// free some memory (by defragmentation if nothing else), that doesn't
	// necessarily happen. (Due to other programs if nothing else.)
	//
	// So in the worst case, we do need one more AABB's worth of memory. And in
	// that case, if we do the allocation after the consume_and_subsume and it
	// fails, we're screwed: we've eaten the consumee but can't place the
	// consumer. Thus we must preallocate here even if it ends up being
	// unnecessary.
	size_t max_needed_boxen = space->box_count + 1;
	mush_aabb *boxen = realloc(space->boxen, max_needed_boxen * sizeof *boxen);
	if (!boxen)
		return NULL;
	space->boxen = boxen;

	bool ok;
	if (slen) {
		ok = mushspace_consume_and_subsume(space, subsumees, slen,
		                                          consumee, &consumer);
		free(subsumees);
	} else {
		free(subsumees);
		ok = mush_aabb_alloc(&consumer);
	}
	if (!ok)
		return NULL;

	// Try to reduce size of space->boxen if possible.
	if (space->box_count+1 < max_needed_boxen) {
		boxen = realloc(space->boxen, (space->box_count+1) * sizeof *boxen);
		if (boxen)
			space->boxen = boxen;
	}

	space->boxen[space->box_count++] = consumer;

	mushstats_add    (space->stats, MushStat_boxes_placed, 1);
	mushstats_new_max(space->stats, MushStat_max_boxes_live,
	                  (uint64_t)space->box_count);

	void (**i)(void*) = space->invalidatees;
	void  **d         = space->invalidatees_data;
	if (i)
		while (*i)
			(*i++)(*d++);

	return &space->boxen[space->box_count-1];
}
// Doesn't return bool like the others because it doesn't change consumer.
static void mushspace_subsume_contains(
	mushspace* space,
	size_t* candidates, size_t* subsumees, size_t* slen,
	const mush_aabb* consumer,
	size_t* consumee, size_t* consumee_size, size_t* used_cells)
{
	for (size_t i = 0; i < space->box_count; ++i) {
		size_t c = candidates[i];
		if (c == space->box_count)
			continue;

		if (!mush_aabb_contains_box(consumer, &space->boxen[c]))
			continue;

		subsumees[*slen++] = c;
		mushspace_min_max_size(
			NULL, NULL, consumee, consumee_size, used_cells, c, &space->boxen[c]);

		candidates[i] = space->box_count;

		mushstats_add(space->stats, MushStat_subsumed_contains, 1);
	}
}
static bool mushspace_subsume_fusables(
	mushspace* space,
	size_t* candidates, size_t* subsumees, size_t* slen,
	mush_aabb* consumer,
	size_t* consumee, size_t* consumee_size, size_t* used_cells)
{
	size_t s0 = *slen;

	// First, get all the fusables.
	//
	// This is somewhat HACKY to avoid memory allocation: subsumees[s0] to
	// subsumees[*slen-1] are now indices to candidates, not space->boxen.
	for (size_t i = 0; i < space->box_count; ++i) {
		size_t c = candidates[i];
		if (c == space->box_count)
			continue;
		if (mush_aabb_can_fuse_with(consumer, &space->boxen[c]))
			subsumees[*slen++] = i;
	}

	// Now, grab those that we can actually fuse with.
	//
	// We prefer those along the primary axis (y for 2D, z for 3D) because then
	// we have less memcpying/memmoving to do.
	//
	// This ensures that all the ones we fuse with are along the same axis. For
	// instance, A can't fuse with both X and Y in the following:
	//
	// X
	// AY
	//
	// This is not needed in one dimension because then they're all trivially
	// along the same axis.
#if MUSHSPACE_DIM > 1
	if (*slen - s0 > 1) {
		size_t j = s0;
		for (size_t i = s0; i < *slen; ++i) {
			size_t s = subsumees [i],
			       c = candidates[s];
			if (mush_aabb_on_same_primary_axis(consumer, &space->boxen[c]))
				subsumees[j++] = s;
		}

		if (j == s0) {
			// Just grab the first one instead of being smart about it.
			const mush_aabb *box = &space->boxen[candidates[subsumees[s0]]];

			j = s0 + 1;

			for (size_t i = j; i < *slen; ++i) {
				size_t s = subsumees [i],
				       c = candidates[s];
				if (mush_aabb_on_same_axis(box, &space->boxen[c]))
					subsumees[j++] = s;
			}
		}
		*slen = j;
	}
#endif

	if (*slen == s0)
		return false;

	assert (*slen > s0);

	// Sort them so that we can find the correct offset to apply to the array
	// index (since we're removing these from candidates as we go): if the
	// lowest index is always next, the following ones' indices are reduced by
	// one.
	qsort(subsumees + s0, *slen - s0, sizeof *subsumees, mush_size_t_qsort_cmp);

	size_t offset = 0;
	for (size_t i = s0; i < *slen; ++i) {
		size_t corrected = subsumees[i] - offset++;
		subsumees[i] = candidates[corrected];

		mushspace_min_max_size(&consumer->beg, &consumer->end,
		                       consumee, consumee_size, used_cells,
		                       subsumees[i], &space->boxen[subsumees[i]]);

		candidates[corrected] = space->box_count;

		mushstats_add(space->stats, MushStat_subsumed_fusables, 1);
	}
	return true;
}
static bool mushspace_subsume_disjoint(
	mushspace* space,
	size_t* candidates, size_t* subsumees, size_t* slen,
	mush_aabb* consumer,
	size_t* consumee, size_t* consumee_size, size_t* used_cells)
{
	const size_t s0 = *slen;
	for (size_t i = 0; i < space->box_count; ++i) {
		size_t c = candidates[i];
		if (c == space->box_count)
			continue;

		// All fusables have been removed, so a sufficient condition for
		// disjointness is non-overlappingness.
		if (mush_aabb_overlaps(consumer, &space->boxen[c]))
			continue;

		if (mushspace_valid_min_max_size(mushspace_disjoint_mms_validator, NULL,
		                                 &consumer->beg, &consumer->end,
		                                 consumee, consumee_size, used_cells,
		                                 c, &space->boxen[c]))
		{
			subsumees[*slen++] = c;
			candidates[i] = space->box_count;

			mushstats_add(space->stats, MushStat_subsumed_disjoint, 1);
		}
	}
	assert (*slen >= s0);
	return *slen > s0;
}
static bool mushspace_disjoint_mms_validator(
	const mush_aabb* b, const mush_aabb* fodder, size_t used_cells, void* nil)
{
	(void)nil;
	return mushspace_cheaper_to_alloc(
		mush_aabb_clamped_size(b), used_cells + fodder->size);
}
static bool mushspace_subsume_overlaps(
	mushspace* space,
	size_t* candidates, size_t* subsumees, size_t* slen,
	mush_aabb* consumer,
	size_t* consumee, size_t* consumee_size, size_t* used_cells)
{
	const size_t s0 = *slen;
	for (size_t i = 0; i < space->box_count; ++i) {
		size_t c = candidates[i];
		if (c == space->box_count)
			continue;

		if (!mush_aabb_overlaps(consumer, &space->boxen[c]))
			continue;

		if (
			mushspace_valid_min_max_size(
				mushspace_overlaps_mms_validator, consumer,
				&consumer->beg, &consumer->end,
				consumee, consumee_size, used_cells,
				c, &space->boxen[c]))
		{
			subsumees[*slen++] = c;
			candidates[i] = space->box_count;

			mushstats_add(space->stats, MushStat_subsumed_overlaps, 1);
		}
	}
	assert (*slen >= s0);
	return *slen > s0;
}
static bool mushspace_overlaps_mms_validator(
	const mush_aabb* b, const mush_aabb* fodder, size_t used_cells, void* ep)
{
	const mush_aabb *eater = ep;

	mush_aabb overlap;
	overlap.size = 0;

	mush_aabb_get_overlap_with(eater, fodder, &overlap);

	return mushspace_cheaper_to_alloc(
		mush_aabb_clamped_size(b), used_cells + fodder->size - overlap.size);
}
static void mushspace_min_max_size(
	mushcoords* beg, mushcoords* end,
	size_t* max_idx, size_t* max_size, size_t* total_size,
	size_t box_idx, const mush_aabb* box)
{
	*total_size += box->size;
	if (box->size > *max_size) {
		*max_size = box->size;
		*max_idx  = box_idx;
	}
	if (beg) mushcoords_min_into(beg, box->beg);
	if (end) mushcoords_max_into(end, box->end);
}
// Fills in the input values with the mushspace_min_max_size data, returning
// what the given validator function returns.
//
// The validator takes:
// - box that subsumes (unallocated)
// - box to be subsumed (allocated)
// - number of cells that are currently contained in any box that the subsumer
//   contains
// - arbitrary user-provided data
static bool mushspace_valid_min_max_size(
	bool (*valid)(const mush_aabb*, const mush_aabb*, size_t, void*),
	void* userdata,
	mushcoords* beg, mushcoords* end,
	size_t* max_idx, size_t* max_size, size_t* total_size,
	size_t box_idx, const mush_aabb* box)
{
	mushcoords try_beg = *beg, try_end = *end;
	size_t try_max_idx, try_max_size = *max_size, try_total_size = *total_size;

	mushspace_min_max_size(
		&try_beg, &try_end, &try_max_idx, &try_max_size, &try_total_size,
		box_idx, box);

	mush_aabb be;
	mush_aabb_make(&be, try_beg, try_end);

	if (!valid(&be, box, *total_size, userdata))
		return false;

	*beg        = try_beg;
	*end        = try_end;
	*max_idx    = try_max_idx;
	*max_size   = try_max_size;
	*total_size = try_total_size;
	return true;
}
static bool mushspace_cheaper_to_alloc(size_t together, size_t separate) {
	return together <= ACCEPTABLE_WASTE
	    ||   sizeof(mushcell) * (together - ACCEPTABLE_WASTE)
	       < sizeof(mushcell) * separate + sizeof(mush_aabb);
}
static bool mushspace_consume_and_subsume(
	mushspace* space,
	size_t* subsumees, size_t slen, size_t consumee,
	mush_aabb* consumer)
{
	assert (slen > 0);

	// This allocation needs to be up here: anything below the consumption has
	// to succeed unconditionally lest we leave the mushspace in a corrupted
	// state.
	size_t *removed_offsets = malloc(space->box_count * sizeof *removed_offsets);
	if (!removed_offsets)
		return false;

	mushspace_irrelevize_subsumption_order(space, subsumees, slen);

	mush_aabb_finalize(consumer);
	if (!mush_aabb_consume(consumer, &space->boxen[consumee])) {
		free(removed_offsets);
		return false;
	}

	// NOTE: strictly speaking we should sort subsumees and go from slen down to
	// 0, since we don't want below-boxes to overwrite top-boxes' data. However,
	// mushspace_irrelevize_subsumption_order copies the data so that the order
	// is, in fact, irrelevant.
	//
	// I think that consumee would also have to be simply subsumees[slen-1]
	// after sorting, but I haven't thought this completely through so I'm not
	// sure.
	//
	// In debug mode, do exactly the "wrong" thing (subsume top-down), in the
	// hopes of catching a bug in mushspace_irrelevize_subsumption_order.

#ifndef NDEBUG
	qsort(subsumees, slen, sizeof *subsumees, mush_size_t_qsort_cmp);
#endif

	for (size_t i = 0; i < slen; ++i) {
		size_t s = subsumees[i];
		if (s == consumee)
			continue;

		mush_aabb_subsume(consumer, &space->boxen[s]);
		free(space->boxen[s].data);
	}

	const size_t orig_box_count = space->box_count;

	size_t range_beg = subsumees[0], range_end = subsumees[0];

	for (size_t i = 1; i < slen; ++i) {
		const size_t s = subsumees[i];

		// s is an index into the original space->boxen, so after we've removed
		// some boxes it's no longer valid and needs to be offset.
		const size_t b = s - removed_offsets[s];

		if (b == range_beg - 1) { range_beg = b; continue; }
		if (b == range_end + 1) { range_end = b; continue; }

		mushspace_remove_boxes(space, range_beg, range_end);

		// Any future subsumees after the end of the removed range need to be
		// offset backward by the number of boxes we just removed.
		size_t later = range_end + 1;
		for (size_t r = later + removed_offsets[later]; r < orig_box_count; ++r)
			removed_offsets[r] += range_end - range_beg + 1;

		range_beg = range_end = b;
	}
	free(removed_offsets);
	mushspace_remove_boxes(space, range_beg, range_end);
	return true;
}
// Consider the following:
//
// +-----++---+
// | A +--| C |
// +---|B +*--+
//     +----+
//
// Here, A is the one being placed and C is a fusable. * is a point whose data
// is in C but which is contained in both B and C. Since the final subsumer-box
// is going to be below all existing boxes, we'll end up with:
//
// +----------+
// | X +----+ |
// +---|B  *|-+
//     +----+
//
// Where X is the final box placed. Note that * is now found in B, not in X,
// but its data was in C (now X)! Oops!
//
// So, we do the following, which in the above case would copy the data from C
// to B.
//
// Since mushspace_find_beg and mushspace_find_end check all boxes without
// considering overlappingness, we also space the overlapped area in C to
// prevent mishaps there.
//
// Caveat: this assumes that the final box will always be placed bottom-most.
// This does not really matter, it's just extra work if it's not; but in any
// case, if not, the relevant overlapping boxes would be those which would end
// up above the final box.
static void mushspace_irrelevize_subsumption_order(
	mushspace* space, size_t* subsumees, size_t slen)
{
	for (size_t i = 0; i < slen; ++i) {
		size_t s = subsumees[i];
		mush_aabb* higher = &space->boxen[s];

		for (size_t t = s+1; t < space->box_count; ++t) {
			mush_aabb* lower = &space->boxen[t];

			if (   mush_aabb_contains_box(higher, lower)
			    || mush_aabb_contains_box(lower, higher))
				continue;

			mush_aabb overlap;

			// If they overlap, copy the overlap area to the lower box and space
			// that area in the higher one.
			if (mush_aabb_get_overlap_with(higher, lower, &overlap)) {
				mush_aabb_subsume_area(lower, higher, &overlap);
				mush_aabb_space_area(higher, &overlap);
			}
		}
	}
}

static void mushspace_map_no_place(
	mushspace* space, const mush_aabb* aabb, void* caller_data,
	void(*f)(mushcell*, mushcell*, void*, mushstats*),
	void(*g)(size_t, void*))
{
	mushcoords pos = aabb->beg;

	for (;;) next_pos: {
		if (mush_staticaabb_contains(pos)) {
			if (mushspace_map_in_static(space, aabb, &pos, caller_data, f))
				return;
			else
				goto next_pos;
		}

		for (size_t b = 0; b < space->box_count; ++b) {
			const mush_aabb *box = &space->boxen[b];

			if (!mush_aabb_contains(box, pos))
				continue;

			if (mushspace_map_in_box(space, aabb, &pos, box, b, caller_data, f))
				return;
			else
				goto next_pos;
		}

		// No hits for pos: find the next pos we can hit, or stop if there's
		// nothing left.
		size_t skipped = 0;
		bool found = mushspace_get_next_in(space, aabb, &pos, &skipped);
		if (g)
			g(skipped, caller_data);
		if (!found)
			return;
	}
}
static bool mushspace_map_in_box(
	mushspace* space, const mush_aabb* aabb, mushcoords* pos,
	const mush_aabb* box, size_t box_idx,
	void* caller_data, void(*f)(mushcell*, mushcell*, void*, mushstats*))
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

	mushcoords tes_beg = box->beg;
	mushcoords tes_end = box->end;
	mush_aabb_tessellate(*pos, space->boxen, box_idx, &tes_beg, &tes_end);

	// The static box is above all dynamic boxen, so check it as well.
	mush_aabb_tessellate1(*pos, MUSH_STATICAABB_BEG, MUSH_STATICAABB_END,
	                      &tes_beg, &tes_end);

	bool hit_end;
	const size_t
		beg_idx = mush_aabb_get_idx(box, *pos),
		end_idx = mush_aabb_get_idx(box, mushcoords_get_end_of_contiguous_range(
			tes_end, pos, aabb->end, aabb->beg, &hit_end, tes_beg, box->beg));

	assert (beg_idx <= end_idx);

	mushcell *p = box->data;
	size_t len = end_idx - beg_idx + 1;
	f(p, p + len, caller_data, space->stats);
	return hit_end;
}
static bool mushspace_map_in_static(
	mushspace* space, const mush_aabb* aabb, mushcoords* pos,
	void* caller_data, void(*f)(mushcell*, mushcell*, void*, mushstats*))
{
	bool hit_end;
	const size_t
		beg_idx = mush_staticaabb_get_idx(*pos),
		end_idx = mush_staticaabb_get_idx(mushcoords_get_end_of_contiguous_range(
			MUSH_STATICAABB_END, pos, aabb->end,
			aabb->beg, &hit_end, MUSH_STATICAABB_BEG, MUSH_STATICAABB_BEG));

	assert (beg_idx <= end_idx);

	mushcell *p = space->static_box.array;
	size_t len = end_idx - beg_idx + 1;
	f(p, p + len, caller_data, space->stats);
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
static void mushspace_mapex_no_place(
	mushspace* space, const mush_aabb* aabb, void* caller_data,
	void(*f)(mushcell*, size_t, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*),
	void(*g)(size_t, void*))
{
	mushcoords pos = aabb->beg;

	for (;;) next_pos: {
		if (mush_staticaabb_contains(pos)) {
			if (mushspace_mapex_in_static(space, aabb, &pos, caller_data, f))
				return;
			else
				goto next_pos;
		}

		for (size_t b = 0; b < space->box_count; ++b) {
			const mush_aabb *box = &space->boxen[b];

			if (!mush_aabb_contains(box, pos))
				continue;

			if (mushspace_mapex_in_box(space, aabb, &pos, box, b, caller_data, f))
				return;
			else
				goto next_pos;
		}

		// No hits for pos: find the next pos we can hit, or stop if there's
		// nothing left.
		size_t skipped = 0;
		bool found = mushspace_get_next_in(space, aabb, &pos, &skipped);
		if (g)
			g(skipped, caller_data);
		if (!found)
			return;
	}
}
static bool mushspace_mapex_in_box(
	mushspace* space, const mush_aabb* aabb, mushcoords* pos,
	const mush_aabb* box, size_t box_idx,
	void* caller_data,
	void(*f)(mushcell*, size_t, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*))
{
	// Self-initialize: they're used uninitialized in Unefunge (and area in
	// Befunge). This method appears to silence -Wuninitialized in both GCC and
	// Clang.
	size_t width = *&width, area = *&area, page_start = *&page_start;

	// These depend on the original pos and thus have to be initialized before
	// the call to mushcoords_get_end_of_contiguous_range.

	// {box->beg.x, pos->y, pos->z}
	mushcoords ls = *pos;
	ls.x = box->beg.x;

#if MUSHSPACE_DIM >= 2
	// {box->beg.x, box->beg.y, pos->z}
	mushcoords ps = box->beg;
	memcpy(ps.v + 2, pos->v + 2, (MUSHSPACE_DIM - 2) * sizeof(mushcell));
#endif

#if MUSHSPACE_DIM >= 2
	const mushcell prev_y = pos->y;
#if MUSHSPACE_DIM >= 3
	const mushcell prev_z = pos->z;
#endif
#endif

	mushcoords tes_beg = box->beg;
	mushcoords tes_end = box->end;
	mush_aabb_tessellate(*pos, space->boxen, box_idx, &tes_beg, &tes_end);

	mush_aabb_tessellate1(*pos, MUSH_STATICAABB_BEG, MUSH_STATICAABB_END,
	                      &tes_beg, &tes_end);

	bool hit_end;
	const size_t
		beg_idx = mush_aabb_get_idx(box, *pos),
		end_idx = mush_aabb_get_idx(box, mushcoords_get_end_of_contiguous_range(
			tes_end, pos, aabb->end, aabb->beg, &hit_end, tes_beg, box->beg));

	assert (beg_idx <= end_idx);

	mushcell *p = box->data;
	size_t len = end_idx - beg_idx + 1;

	uint8_t hit = 0;

	// Unefunge needs this to skip leading spaces.
	const size_t line_start = mush_aabb_get_idx(box, ls) - beg_idx;

#if MUSHSPACE_DIM >= 2
	width = box->width;
	hit |= (pos->x == aabb->beg.x && pos->y != ls.y) << 0;

	// Befunge needs this to skip leading newlines.
	page_start = mush_aabb_get_idx(box, ps) - beg_idx;
#endif
#if MUSHSPACE_DIM >= 3
	area = box->area;
	hit |= (pos->y == aabb->beg.y && pos->z != ls.z) << 1;
#endif

	f(p, len, caller_data, width, area, line_start, page_start, &hit,
	  space->stats);

#if MUSHSPACE_DIM >= 2
	if (hit == 0x01 && pos->y == prev_y) {
		// f hit an EOL and pos->y hasn't been bumped, so bump it.
		pos->x = aabb->beg.x;
		if ((pos->y = mushcell_inc(pos->y)) > aabb->end.y) {
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
		pos->x = aabb->beg.x;
bump_z:
		pos->y = aabb->beg.y;
		if ((pos->z = mushcell_inc(pos->z)) > aabb->end.z)
			hit_end = true;
	}
#endif
	return hit_end;
}
static bool mushspace_mapex_in_static(
	mushspace* space, const mush_aabb* aabb, mushcoords* pos,
	void* caller_data,
	void(*f)(mushcell*, size_t, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*))
{
	size_t width = *&width, area = *&area, page_start = *&page_start;

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
			MUSH_STATICAABB_END, pos, aabb->end,
			aabb->beg, &hit_end, MUSH_STATICAABB_BEG, MUSH_STATICAABB_BEG));

	assert (beg_idx <= end_idx);

	mushcell *p = space->static_box.array;
	const size_t len = end_idx - beg_idx + 1;

	uint8_t hit = 0;

	const size_t line_start = mush_staticaabb_get_idx(ls) - beg_idx;

#if MUSHSPACE_DIM >= 2
	width = MUSH_STATICAABB_SIZE.x;
	hit |= (pos->x == aabb->beg.x && pos->y != ls.y) << 0;

	page_start = mush_staticaabb_get_idx(ps) - beg_idx;
#endif
#if MUSHSPACE_DIM >= 3
	area = MUSH_STATICAABB_SIZE.x * MUSH_STATICAABB_SIZE.y;
	hit |= (pos->y == aabb->beg.y && pos->z != ls.z) << 1;
#endif

	f(p, len, caller_data, width, area, line_start, page_start, &hit,
	  space->stats);

#if MUSHSPACE_DIM >= 2
	if (hit == 0x01 && pos->y == prev_y) {
		pos->x = aabb->beg.x;
		if ((pos->y = mushcell_inc(pos->y)) > aabb->end.y) {
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
		pos->x = aabb->beg.x;
bump_z:
		pos->y = aabb->beg.y;
		if ((pos->z = mushcell_inc(pos->z)) > aabb->end.z)
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
	size_t best_in    = box_count + 1,
	       wrapped_in = box_count + 1;

	mushcell best_coord, best_wrapped;

	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {

		// Go through every box, starting from the static one.

		mushspace_get_next_in1(
			i, aabb, pos->v[i], box_count, MUSH_STATICAABB_BEG, box_count,
			&best_coord, &best_wrapped, &best_in, &wrapped_in);

		for (mushucell b = 0; b < box_count; ++b)
			mushspace_get_next_in1(
				i, aabb, pos->v[i], box_count, space->boxen[b].beg, b,
				&best_coord, &best_wrapped, &best_in, &wrapped_in);

		if (best_in > box_count) {
			if (wrapped_in > box_count) {
				// No solution: look along the next axis.
				continue;
			}

			// Take the wrapping solution as it's the only one available.
			best_coord = best_wrapped;
			best_in    = wrapped_in;
		}

		const mushcoords old = *pos;

		memcpy(pos->v, aabb->beg.v, i * sizeof(mushcell));
		pos->v[i] = best_coord;

		// Old was already a space, or we wouldn't've called this function in the
		// first place. (See assertions.) Hence skipped is always at least one.
		++*skipped;

		mushdim j;
#if MUSHSPACE_DIM >= 2
		for (j = 0; j < MUSHSPACE_DIM-1; ++j)
			*skipped +=   mushcell_sub(aabb->end.v[j], old.v[j])
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
		if (   best_in < box_count
		    && mush_aabb_contains(&space->boxen[best_in], *pos))
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
	mushucell x, const mush_aabb* aabb, mushcell posx, size_t box_count,
	mushcoords box_beg, size_t box_idx,
	mushcell* best_coord, mushcell* best_wrapped,
	size_t* best_in, size_t* wrapped_in)
{
	assert (*best_wrapped <= *best_coord);

	// If the box begins later than the best solution we've found, there's no
	// point in looking further into it.
	if (box_beg.v[x] >= *best_coord && *best_in == box_count+1)
		return;

	// If this box doesn't overlap with the AABB we're interested in, skip it.
	if (!mush_aabb_safe_contains(aabb, box_beg))
		return;

	// If pos has crossed an axis within the AABB, prevent us from grabbing a
	// new pos on the other side of the axis we're wrapped around, or we'll just
	// keep looping around that axis.
	if (posx < aabb->beg.v[x] && box_beg.v[x] > aabb->end.v[x])
		return;

	// If the path from pos to aabb->end requires a wraparound, take the global
	// minimum box.beg as a last-resort option if nothing else is found, so that
	// we wrap around if there's no non-wrapping solution.
	//
	// Note that *best_wrapped <= *best_coord so we can safely test this after
	// the first *best_coord check.
	if (   posx > aabb->end.v[x]
	    && (box_beg.v[x] < *best_wrapped || *wrapped_in == box_count+1))
	{
		*best_wrapped = box_beg.v[x];
		*wrapped_in   = box_idx;

	// The ordinary best solution is the minimal box.beg greater than pos.
	} else if (box_beg.v[x] > posx) {
		*best_coord = box_beg.v[x];
		*best_in    = box_idx;
	}
}

#define mushspace_load_arr_auxdata MUSHSPACE_CAT(mushspace,_load_arr_auxdata)
typedef struct {
	const char *str, *str_end;

	#if MUSHSPACE_DIM >= 2
		mushcell x, target_x, aabb_beg_x;
	#if MUSHSPACE_DIM >= 3
		mushcell y, target_y, aabb_beg_y;
	#endif
	#endif
} mushspace_load_arr_auxdata;

int MUSHSPACE_CAT(mushspace,_load_string)(
	mushspace* space, const char* str, size_t len,
	mushcoords* end, mushcoords target, bool binary)
{
	size_t aabbs_len;
	mush_aabb *aabbs =
		mushspace_get_aabbs(str, len, target, binary, &aabbs_len);

	if (end)
		*end = target;

	if (!aabbs) {
		// The error code was placed in aabbs_len.
		return (int)aabbs_len;
	}

	for (size_t i = 0; i < aabbs_len; ++i) {
		if (end)
			mushcoords_max_into(end, aabbs[i].end);

		if (!mushspace_place_box(space, &aabbs[i], NULL, NULL))
			return MUSH_ERR_OOM;
	}

	// Build one to rule them all.
	//
	// Note that it may have beg > end along any number of axes!
	mush_aabb aabb = aabbs[0];
	if (aabbs_len > 1) {
		// If any box was placed past an axis, the end of that axis is the
		// maximum of the ends of such boxes. Otherwise, it's the maximum of all
		// the boxes' ends.
		//
		// Similarly, if any box was placed before an axis, the beg is the
		// minimum of such boxes' begs.
		//
		// Note that these boxes, from mushspace_get_aabbs, don't cover any
		// excess area! They may be smaller than the range from target to the end
		// point due to dropping space-filled lines and columns, but they haven't
		// been subsumed into bigger boxes or anything.

		uint8_t found_past = 0, found_before = 0;

		for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
			if (aabb.beg.v[i] < target.v[i]) found_past   |= 1 << i;
			else                             found_before |= 1 << i;
		}

		for (size_t b = 1; b < aabbs_len; ++b) {
			const mush_aabb *box = &aabbs[b];

			for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
				const uint8_t axis = 1 << i;

				if (box->beg.v[i] < target.v[i]) {
					// This box is past this axis, in the positive direction: it
					// both begins and ends in the negative direction from the
					// target location.

					if (found_past & axis)
						aabb.end.v[i] = mushcell_max(aabb.end.v[i], box->end.v[i]);
					else {
						aabb.end.v[i] = box->end.v[i];
						found_past |= axis;
					}

					if (!(found_before & axis))
						aabb.beg.v[i] = mushcell_min(aabb.beg.v[i], box->beg.v[i]);
				} else {
					// This box is before this axis, in the negative direction: it
					// both begins and ends in the positive direction from the
					// target location.

					if (found_before & axis)
						aabb.beg.v[i] = mushcell_min(aabb.beg.v[i], box->beg.v[i]);
					else {
						aabb.beg.v[i] = box->beg.v[i];
						found_before |= axis;
					}

					if (!(found_past & axis))
						aabb.end.v[i] = mushcell_max(aabb.end.v[i], box->end.v[i]);
				}
			}
		}
		mush_aabb_finalize(&aabb);
	}

	const char *str_end = str + len;

	if (binary)
		mushspace_map_no_place(
			space, &aabb, &str,
			mushspace_binary_load_arr, mushspace_binary_load_blank);
	else {
		mushspace_load_arr_auxdata aux =
			{ str, str_end
		#if MUSHSPACE_DIM >= 2
			, target.x, target.x, aabb.beg.x
		#if MUSHSPACE_DIM >= 3
			, target.y, target.y, aabb.beg.y
		#endif
		#endif
		};

		mushspace_mapex_no_place(
			space, &aabb, &aux, mushspace_load_arr, mushspace_load_blank);

		str = aux.str;
	}

	for (; str < str_end; ++str)
		assert (*str == ' ' || *str == '\r' || *str == '\n' || *str == '\f');
	assert (!(str > str_end));
	return MUSH_ERR_NONE;
}

// Returns an array of AABBs (a slice out of a static buffer) which describe
// where the input should be loaded. There are at most 2^dim of them; in binary
// mode, at most 2. The length is written to *len_out.
//
// If nothing would be loaded, returns NULL and writes an error code (an int)
// into *aabbs_out.
static mush_aabb* mushspace_get_aabbs(
	const char* str, size_t len, mushcoords target, bool binary,
	size_t* len_out)
{
	static mush_aabb aabbs[1 << MUSHSPACE_DIM];

	if (binary) {
		size_t n = mushspace_get_aabbs_binary(str, len, target, aabbs);
		if (n == SIZE_MAX)
			*len_out = MUSH_ERR_NO_ROOM;
		else {
			assert (n <= 2);
			*len_out = n;
		}
		return aabbs;
	}

	// The index a as used below is a bitmask of along which axes pos
	// overflowed. Thus it changes over time as we read something like:
	//
	//          |
	//   foobarb|az
	//      qwer|ty
	// ---------+--------
	//      arst|mei
	//     qwfp |
	//          |
	//
	// After the ending 'p', a will not have its maximum value, which was in the
	// "mei" quadrant. So we have to keep track of it separately.
	size_t a = 0, max_a = 0;

	mushcoords pos = target;

	// All bits set up to the MUSHSPACE_DIM'th.
	static const uint8_t DimensionBits = (1 << MUSHSPACE_DIM) - 1;

	// A bitmask of which axes we want to search for the beginning point for.
	// Reset completely at overflows and partially at line and page breaks.
	uint8_t get_beg = DimensionBits;

	// We want minimal boxes, and thus exclude spaces at edges. These are
	// helpers toward that. lastNonSpace points to the last found nonspace
	// and foundNonSpaceFor is the index of the box it belonged to.
	mushcoords last_nonspace = target;
	size_t found_nonspace_for = MUSH_ARRAY_LEN(aabbs);

	// Not per-box: if this remains unchanged, we don't need to load a thing.
	size_t found_nonspace_for_anyone = MUSH_ARRAY_LEN(aabbs);

	for (size_t i = 0; i < MUSH_ARRAY_LEN(aabbs); ++i) {
		aabbs[i].beg = MUSHCOORDS(MUSHCELL_MAX, MUSHCELL_MAX, MUSHCELL_MAX);
		aabbs[i].end = MUSHCOORDS(MUSHCELL_MIN, MUSHCELL_MIN, MUSHCELL_MIN);
	}

	#if MUSHSPACE_DIM >= 2
		bool got_cr = false;

		#define mush_hit_newline do { \
			if (!mushspace_newline(&got_cr, &pos, target, \
			                       aabbs, MUSH_ARRAY_LEN(aabbs), &a, &max_a, \
			                       last_nonspace, &found_nonspace_for, &get_beg))\
			{ \
				*len_out = MUSH_ERR_NO_ROOM; \
				return NULL; \
			} \
		} while (0)
	#endif

	for (const char* str_end = str + len; str < str_end; ++str) switch (*str) {
	default:
		#if MUSHSPACE_DIM >= 2
			if (got_cr)
				mush_hit_newline;
		#endif

		if (*str != ' ') {
			found_nonspace_for = found_nonspace_for_anyone = a;
			last_nonspace = pos;

			if (get_beg) for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
				if (get_beg & 1 << i) {
					aabbs[a].beg.v[i] = mushcell_min(aabbs[a].beg.v[i], pos.v[i]);
					get_beg &= ~(1 << i);
				}
			}
		}
		if ((pos.x = mushcell_inc(pos.x)) == MUSHCELL_MIN) {
			if (found_nonspace_for == a)
				mushcoords_max_into(&aabbs[a].end, last_nonspace);

			found_nonspace_for = MUSH_ARRAY_LEN(aabbs);
			get_beg = DimensionBits;

			max_a = mush_size_t_max(max_a, a |= 0x01);

		} else if (pos.x == target.x) {
			// Oops, came back to where we started. That's not good.
			*len_out = MUSH_ERR_NO_ROOM;
			return NULL;
		}
		break;

	case '\r':
		#if MUSHSPACE_DIM >= 2
			got_cr = true;
		#endif
		break;

	case '\n':
		#if MUSHSPACE_DIM >= 2
			mush_hit_newline;
		#endif
		break;

	case '\f':
		#if MUSHSPACE_DIM >= 2
			if (got_cr)
				mush_hit_newline;
		#endif
		#if MUSHSPACE_DIM >= 3
			aabbs[a].end.x = mushcell_max(aabbs[a].end.x, last_nonspace.x);
			aabbs[a].end.y = mushcell_max(aabbs[a].end.y, last_nonspace.y);

			pos.x = target.x;
			pos.y = target.y;

			if ((pos.z = mushcell_inc(pos.z)) == MUSHCELL_MIN) {
				if (found_nonspace_for == a)
					mushcoords_max_into(&aabbs[a].end, last_nonspace);

				found_nonspace_for = MUSH_ARRAY_LEN(aabbs);

				max_a = mush_size_t_max(max_a, a |= 0x04);

			} else if (pos.z == target.z) {
				*len_out = MUSH_ERR_NO_ROOM;
				return NULL;
			}
			a &= ~0x03;
			get_beg = found_nonspace_for == a ? 0x03 : 0x07;
		#endif
		break;
	}
	#if MUSHSPACE_DIM >= 2
	#undef mush_hit_newline
	#endif

	if (found_nonspace_for_anyone == MUSH_ARRAY_LEN(aabbs))
		return NULL;

	if (found_nonspace_for < MUSH_ARRAY_LEN(aabbs))
		mushcoords_max_into(&aabbs[found_nonspace_for].end, last_nonspace);

	// Since a is a bitmask, the AABBs that we used aren't necessarily in order.
	// Fix that.
	size_t n = 0;
	for (size_t i = 0; i <= max_a; ++i) {
		const mush_aabb* box = &aabbs[i];

		if (!(box->beg.x == MUSHCELL_MAX && box->end.x == MUSHCELL_MIN)) {
			// The box has been initialized, so make sure it's valid and put it in
			// place.

			for (mushdim j = 0; j < MUSHSPACE_DIM; ++j)
				assert (box->beg.v[j] <= box->end.v[j]);

			if (i != n)
				aabbs[n] = aabbs[i];
			++n;
		}
	}
	*len_out = n;
	return aabbs;
}
#if MUSHSPACE_DIM >= 2
static bool mushspace_newline(
	bool* got_cr, mushcoords* pos, mushcoords target,
	mush_aabb* aabbs, size_t aabbs_len, size_t* a, size_t* max_a,
	mushcoords last_nonspace, size_t* found_nonspace_for, uint8_t* get_beg)
{
	*got_cr = false;

	aabbs[*a].end.x = mushcell_max(aabbs[*a].end.x, last_nonspace.x);

	pos->x = target.x;

	if ((pos->y = mushcell_inc(pos->y)) == MUSHCELL_MIN) {
		if (*found_nonspace_for == *a)
			mushcoords_max_into(&aabbs[*a].end, last_nonspace);

		*found_nonspace_for = aabbs_len;

		*max_a = mush_size_t_max(*max_a, *a |= 0x02);
	} else if (pos->y == target.y)
		return false;

	*a &= ~0x01;
	*get_beg = *found_nonspace_for == *a ? 0x01 : 0x03;
	return true;
}
#endif

static size_t mushspace_get_aabbs_binary(
	const char* str, size_t len, mushcoords target, mush_aabb* aabbs)
{
	size_t a = 0;
	mushcoords beg = target, end = target;

	size_t i = 0;
	while (i < len && str[i++] == ' ');

	if (i == len) {
		// All spaces: nothing to load.
		return 0;
	}

	beg.x += i-1;

	// No need to check bounds here since we've already established that it's
	// not all spaces.
	i = len;
	while (str[--i] == ' ');

	if (i > (size_t)MUSHCELL_MAX) {
		// Oops, that's not going to fit! Bail.
		return SIZE_MAX;
	}

	if (end.x > MUSHCELL_MAX - (mushcell)i) {
		end.x = MUSHCELL_MAX;
		mush_aabb_make(&aabbs[a++], beg, end);
		beg.x = MUSHCELL_MIN;
	}
	end.x += i;

	mush_aabb_make(&aabbs[a++], beg, end);
	return a;
}

static void mushspace_binary_load_arr(
	mushcell* arr, mushcell* end, void* p, mushstats* stats)
{
	const char **strp = p, *str = *strp;
	for (; arr < end; ++arr) {
		char c = *str++;
		if (c != ' ') {
			*arr = c;
			mushstats_add(stats, MushStat_assignments, 1);
		}
	}
	*strp = str;
}
static void mushspace_binary_load_blank(size_t blanks, void* p) {
	const char **strp = p, *str = *strp;
	while (blanks) {
		if (*str != ' ')
			break;
		--blanks;
		++str;
	}
	*strp = str;
}
static void mushspace_load_arr(
	mushcell* arr, size_t len, void* p,
	size_t width, size_t area, size_t line_start, size_t page_start,
	uint8_t* hit, mushstats* stats)
{
	#if MUSHSPACE_DIM == 1
	(void)width; (void)area; (void)page_start; (void)hit;
	#elif MUSHSPACE_DIM == 2
	(void)area;
	#endif

	mushspace_load_arr_auxdata *aux = p;
	const char *str = aux->str, *str_end = aux->str_end;

	// x and y are used only for skipping leading spaces/newlines and thus
	// aren't really representative of the cursor position at any point.
	#if MUSHSPACE_DIM >= 2
		mushcell x = aux->x;
		const mushcell target_x = aux->target_x, aabb_beg_x = aux->aabb_beg_x;
	#if MUSHSPACE_DIM >= 3
		mushcell y = aux->y;
		const mushcell target_y = aux->target_y, aabb_beg_y = aux->aabb_beg_y;
	#endif
	#endif

	for (size_t i = 0; i < len;) {
		assert (str < str_end);

		const char c = *str++;
		switch (c) {
		default:
			arr[i++] = c;
			mushstats_add(stats, MushStat_assignments, 1);
			break;

		case ' ': {
			// Ignore leading spaces (west of aabb.beg.x)
			bool leading_space = i == line_start;
			#if MUSHSPACE_DIM >= 2
				leading_space &= (x = mushcell_inc(x)) < aabb_beg_x;
			#endif
			if (!leading_space)
				++i;

	#if MUSHSPACE_DIM < 2
		case '\r': case '\n':
	#if MUSHSPACE_DIM < 3
		case '\f':
	#endif
	#endif
			break;
		}

	#if MUSHSPACE_DIM >= 2
		case '\r':
			if (str < str_end && *str == '\n')
				++str;
		case '\n': {
			// Ignore leading newlines (north of aabb.beg.y)
			bool leading_newline = i == page_start;
			#if MUSHSPACE_DIM >= 3
				leading_newline &= (y = mushcell_inc(y)) < aabb_beg_y;
			#endif
			if (!leading_newline) {
				i = line_start += width;
				x = target_x;

				if (i >= len) {
					// The next cell we would want to load falls on the next line:
					// report that.
					*hit = 1 << 0;

					// We set x above, no need to write it back.
					aux->str = str;
					#if MUSHSPACE_DIM >= 3
						aux->y = y;
					#endif
					return;
				}
			}
			break;
		}
	#endif
	#if MUSHSPACE_DIM >= 3
		case '\f':
			// Ignore leading form feeds (above aabb.beg.z)
			if (i) {
				i = line_start = page_start += area;
				y = target_y;

				if (i >= len) {
					*hit = 1 << 1;

					// We set y above, no need to write it back.
					aux->str = str;
					aux->x = x;
					return;
				}
			}
			break;
	#endif
		}
	}
	if (str >= str_end) {
		// x and y are needed no longer: no need to write them back.
		aux->str = str;
		return;
	}

	// When we've hit an EOL/EOP in Funge-Space, we need to skip any
	// trailing whitespace until we hit it in the file as well.
	#if MUSHSPACE_DIM == 3
		if (*hit & 1 << 1) {
			while (*str == '\r' || *str == '\n' || *str == ' ')
				++str;

			if (str < str_end) {
				assert (*str == '\f');
				++str;
			}
			goto end;
		}
	#endif
	#if MUSHSPACE_DIM >= 2
		if (*hit & 1 << 0) {
			while (*str == ' ' || (MUSHSPACE_DIM < 3 && *str == '\f'))
				++str;

			if (str < str_end) {
				assert (*str == '\r' || *str == '\n');
				if (*str++ == '\r' && str < str_end && *str == '\n')
					++str;
			}
			goto end;
		}
	#endif

	// See if we've just barely hit an EOL/EOP and report it if so. This is just
	// an optimization, we'd catch it and handle it appropriately come next call
	// anyway.
	#if MUSHSPACE_DIM >= 3
		if (*str == '\f') {
			++str;
			*hit = 1 << 1;
			goto end;
		}
	#endif
	#if MUSHSPACE_DIM >= 2
		if (*str == '\r') {
			++str;
			*hit = 1 << 0;
		}
		if (str < str_end && *str == '\n') {
			++str;
			*hit = 1 << 0;
		}
	#endif
	#if MUSHSPACE_DIM >= 2
end:
		aux->x = x;
	#if MUSHSPACE_DIM >= 3
		aux->y = y;
	#endif
	#endif
	aux->str = str;
	return;
}
static void mushspace_load_blank(size_t blanks, void* p) {
	mushspace_load_arr_auxdata *aux = p;
	const char *str = aux->str;
	while (blanks) {
		if (!(*str == ' ' || *str == '\r' || *str == '\n' || *str == '\f'))
			break;
		--blanks;
		++str;
	}
	aux->str = str;
}