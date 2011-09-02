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

MUSH_DECL_DYN_ARRAY(mushcell)
MUSH_DECL_DYN_ARRAY(mush_aabb)
MUSH_DECL_DYN_ARRAY(mush_bounds)
MUSH_DECL_DYN_ARRAY(size_t)

typedef struct { const mush_aabb *aabb; size_t idx; } mush_caabb_idx;
typedef struct {       mushcell   cell; size_t idx; } mush_cell_idx;

typedef struct mush_bounded_pos {
	const mush_bounds *bounds;
	mushcoords *pos;
} mush_bounded_pos;

const size_t mushspace_sizeof = sizeof(mushspace);

#define mushspace_find_beg_in       MUSHSPACE_CAT(mushspace,_find_beg_in)
#define mushspace_find_end_in       MUSHSPACE_CAT(mushspace,_find_end_in)
#define mushspace_invalidate_all    MUSHSPACE_CAT(mushspace,_invalidate_all)
#define mushspace_get_caabb_idx     MUSHSPACE_CAT(mushspace,_get_caabb_idx)
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

static bool mushspace_find_beg_in(
	mushcoords*, mushdim, const mush_bounds*,
	mushcell(*)(const void*, mushcoords), const void*);

static void mushspace_find_end_in(
	mushcoords*, mushdim, const mush_bounds*,
	mushcell(*)(const void*, mushcoords), const void*);

static void mushspace_invalidate_all(mushspace*);

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
	const mushspace*, mushcoords, const mush_bounds*, mush_aabb*);

static bool mushspace_extend_first_placed_big_for(
	const mushspace*, mushcoords, const mush_bounds*, mush_aabb*);

static mush_aabb* mushspace_really_place_box(mushspace*, mush_aabb*);

typedef struct mushspace_consumee {
	size_t idx;
	size_t size;
} mushspace_consumee;

static void mushspace_subsume_contains(
	mushspace*, size_t*, mush_arr_size_t*, const mush_bounds*,
	mushspace_consumee*, size_t*);

static bool mushspace_subsume_fusables(
	mushspace*, size_t*, mush_arr_size_t*, mush_bounds*,
	mushspace_consumee*, size_t*);

static bool mushspace_subsume_disjoint(
	mushspace*, size_t*, mush_arr_size_t*, mush_bounds*,
	mushspace_consumee*, size_t*);

static bool mushspace_disjoint_mms_validator(
	const mush_bounds*, const mush_aabb*, size_t, void*);

static bool mushspace_subsume_overlaps(
	mushspace*, size_t*, mush_arr_size_t*, mush_bounds*,
	mushspace_consumee*, size_t*);

static bool mushspace_overlaps_mms_validator(
	const mush_bounds*, const mush_aabb*, size_t, void*);

static void mushspace_min_max_size(
	mush_bounds*, mushspace_consumee*, size_t*, mush_caabb_idx);

static bool mushspace_valid_min_max_size(
	bool (*)(const mush_bounds*, const mush_aabb*, size_t, void*), void*,
	mush_bounds*, mushspace_consumee*, size_t*, mush_caabb_idx);

static bool mushspace_cheaper_to_alloc(size_t, size_t);

static bool mushspace_consume_and_subsume(
	mushspace*, mush_arr_size_t, size_t, mush_aabb*);

static void mushspace_irrelevize_subsumption_order(
	mushspace*, mush_arr_size_t);

static void mushspace_map_no_place(
	mushspace*, const mush_aabb*, void*,
	void(*)(mush_arr_mushcell, void*, mushstats*),
	void(*)(size_t, void*));

static bool mushspace_map_in_box(
	mushspace*, mush_bounded_pos, mush_caabb_idx,
	void*, void(*f)(mush_arr_mushcell, void*, mushstats*));

static bool mushspace_map_in_static(
	mushspace*, mush_bounded_pos,
	void*, void(*f)(mush_arr_mushcell, void*, mushstats*));

static void mushspace_mapex_no_place(
	mushspace*, const mush_aabb*, void*,
	void(*)(mush_arr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*),
	void(*)(size_t, void*));

static bool mushspace_mapex_in_box(
	mushspace*, mush_bounded_pos, mush_caabb_idx, void*,
	void(*)(mush_arr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*));

static bool mushspace_mapex_in_static(
	mushspace*, mush_bounded_pos, void*,
	void(*)(mush_arr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*));

static bool mushspace_get_next_in(
	const mushspace*, const mush_aabb*, mushcoords*, size_t*);

static void mushspace_get_next_in1(
	mushucell, const mush_bounds*, mushcell, size_t, mushcoords, size_t,
	mush_cell_idx*, mush_cell_idx*);

static void mushspace_get_aabbs(
	const char*, size_t, mushcoords target, bool binary, mush_arr_mush_aabb*);

#if MUSHSPACE_DIM >= 2
static bool mushspace_newline(
	bool*, mushcoords*, mushcoords, mush_arr_mush_bounds, size_t*, size_t*,
	mushcoords, size_t*, uint8_t*);
#endif

static size_t mushspace_get_aabbs_binary(
	const char*, size_t len, mushcoords target, mush_bounds*);

static void mushspace_binary_load_arr(mush_arr_mushcell, void*, mushstats*);
static void mushspace_binary_load_blank(size_t, void*);
static void mushspace_load_arr(mush_arr_mushcell, void*,
                               size_t, size_t, size_t, size_t, uint8_t*,
                               mushstats*);
static void mushspace_load_blank(size_t, void*);

mushspace* mushspace_allocate(void* vp, mushstats* stats) {
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

void mushspace_free(mushspace* space) {
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
	bool last_beg_was_space = mushspace_get(space, space->last_beg) == ' ',
	     last_end_was_space = mushspace_get(space, space->last_end) == ' ',
	     found_nonspace = true;

	if (last_beg_was_space == last_end_was_space) {
		if (last_beg_was_space) {
			*beg = MUSHCOORDS(MUSHCELL_MAX, MUSHCELL_MAX, MUSHCELL_MAX);
			*end = MUSHCOORDS(MUSHCELL_MIN, MUSHCELL_MIN, MUSHCELL_MIN);
			found_nonspace = false;
		} else {
			*beg = space->last_beg;
			*end = space->last_end;
		}
	} else {
		if (last_beg_was_space)
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

static void mushspace_invalidate_all(mushspace* space) {
	void (**i)(void*) = space->invalidatees;
	void  **d         = space->invalidatees_data;
	if (i)
		while (*i)
			(*i++)(*d++);
}

static mush_caabb_idx mushspace_get_caabb_idx(const mushspace* sp, size_t i) {
	return (mush_caabb_idx){&sp->boxen[i], i};
}

static mush_aabb* mushspace_find_box(const mushspace* space, mushcoords c) {
	for (size_t i = 0; i < space->box_count; ++i)
		if (mush_bounds_contains(&space->boxen[i].bounds, c))
			return &space->boxen[i];
	return NULL;
}

static void mushspace_remove_boxes(mushspace* space, size_t i, size_t j) {
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

static bool mushspace_place_box(
	mushspace* space, mush_aabb* aabb,
	mushcoords* reason, mush_aabb** reason_box)
{
	assert ((reason == NULL) == (reason_box == NULL));
	if (reason)
		assert (mush_bounds_safe_contains(&aabb->bounds, *reason));

	// Split the box up along any axes it wraps around on.
	mush_aabb aabbs[1 << MUSHSPACE_DIM];
	aabbs[0] = *aabb;
	size_t a = 1;

	for (size_t b = 0; b < a; ++b) {
		mush_bounds *bounds = &aabbs[b].bounds;
		for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
			if (bounds->beg.v[i] <= bounds->end.v[i])
				continue;

			mush_bounds clipped = *bounds;
			clipped.end.v[i] = MUSHCELL_MAX;
			mush_aabb_make_unsafe(&aabbs[a++], &clipped);
			bounds->beg.v[i] = MUSHCELL_MIN;
		}
	}

	// Then do the actual placement.
	for (size_t b = 0; b < a; ++b) {
		mush_aabb   *box    = &aabbs[b];
		mush_bounds *bounds = &box->bounds;

		if (   mush_staticaabb_contains(bounds->beg)
		    && mush_staticaabb_contains(bounds->end))
		{
incorporated:
			mushstats_add(space->stats, MushStat_boxes_incorporated, 1);
			continue;
		}

		for (size_t i = 0; i < space->box_count; ++i)
			if (mush_bounds_contains_bounds(&space->boxen[i].bounds, bounds))
				goto incorporated;

		mush_aabb_finalize(box);
		box = mushspace_really_place_box(space, box);
		if (box == NULL)
			return false;

		bounds = &box->bounds;

		if (reason && mush_bounds_contains(bounds, *reason)) {
			*reason_box = box;

			// This can only happen once.
			reason = NULL;
		}

		// If it crossed bak, we need to fix things up and move any occupied
		// cells from bak (which is below all boxen) to the appropriate box
		// (which may not be *box, if it has any overlaps).
		if (!space->bak.data || !mush_bakaabb_size(&space->bak))
			continue;

		if (!mush_bounds_overlaps(bounds, &space->bak.bounds))
			continue;

		assert (box == &space->boxen[space->box_count-1]);

		bool overlaps = false;
		for (size_t b = 0; b < space->box_count-1; ++b) {
			if (mush_bounds_overlaps(bounds, &space->boxen[b].bounds)) {
				overlaps = true;
				break;
			}
		}

		unsigned char buf[mush_bakaabb_iter_sizeof];
		mush_bakaabb_iter *it = mush_bakaabb_it_start(&space->bak, buf);

		for (; !mush_bakaabb_it_done(it, &space->bak);
		        mush_bakaabb_it_next(it, &space->bak))
		{
			mushcoords c = mush_bakaabb_it_pos(it, &space->bak);
			if (!mush_bounds_contains(bounds, c))
				continue;

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

// Returns the placed box, which may be bigger than the given box. Returns NULL
// if memory allocation failed.
static mush_aabb* mushspace_really_place_box(mushspace* space, mush_aabb* aabb)
{
	for (size_t i = 0; i < space->box_count; ++i)
		assert (!mush_bounds_contains_bounds(
			&space->boxen[i].bounds, &aabb->bounds));

	mush_arr_size_t subsumees;

	subsumees.ptr      = malloc(space->box_count * sizeof *subsumees.ptr);
	size_t *candidates = malloc(space->box_count * sizeof *candidates);

	if (!subsumees.ptr || !candidates) {
		free(subsumees.ptr);
		free(candidates);
		return NULL;
	}

	mushspace_consumee consumee = {.size = 0};
	size_t used_cells = aabb->size;

	mush_aabb consumer;
	mush_aabb_make_unsafe(&consumer, &aabb->bounds);

	// boxen that we haven't yet subsumed. Removed entries are set to
	// space->box_count.
	for (size_t i = 0; i < space->box_count; ++i)
		candidates[i] = i;

	subsumees.len = 0;

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
		               &subsumees,       \
		               &consumer.bounds, \
		               &consumee, &used_cells

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
	if (subsumees.len) {
		ok = mushspace_consume_and_subsume(space, subsumees,
		                                   consumee.idx, &consumer);
		free(subsumees.ptr);
	} else {
		free(subsumees.ptr);
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

	mushspace_invalidate_all(space);

	return &space->boxen[space->box_count-1];
}
// Doesn't return bool like the others because it doesn't change consumer.
static void mushspace_subsume_contains(
	mushspace* space,
	size_t* candidates, mush_arr_size_t* subsumees,	const mush_bounds* consumer,
	mushspace_consumee* consumee, size_t* used_cells)
{
	for (size_t i = 0; i < space->box_count; ++i) {
		size_t c = candidates[i];
		if (c == space->box_count)
			continue;

		mush_caabb_idx box = mushspace_get_caabb_idx(space, c);

		if (!mush_bounds_contains_bounds(consumer, &box.aabb->bounds))
			continue;

		subsumees->ptr[subsumees->len++] = c;
		mushspace_min_max_size(NULL, consumee, used_cells, box);

		candidates[i] = space->box_count;

		mushstats_add(space->stats, MushStat_subsumed_contains, 1);
	}
}
static bool mushspace_subsume_fusables(
	mushspace* space,
	size_t* candidates, mush_arr_size_t* subsumees, mush_bounds* consumer,
	mushspace_consumee* consumee, size_t* used_cells)
{
	const size_t s0 = subsumees->len;

	// First, get all the fusables.
	//
	// This is somewhat HACKY to avoid memory allocation: subsumees->ptr[s0] to
	// subsumees->ptr[subsumees->len-1] are now indices to candidates, not
	// space->boxen.
	for (size_t i = 0; i < space->box_count; ++i) {
		size_t c = candidates[i];
		if (c == space->box_count)
			continue;
		if (mush_bounds_can_fuse(consumer, &space->boxen[c].bounds))
			subsumees->ptr[subsumees->len++] = c;
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
	if (subsumees->len - s0 > 1) {
		size_t j = s0;
		for (size_t i = s0; i < subsumees->len; ++i) {
			size_t s = subsumees->ptr[i];
			const mush_bounds *bounds = &space->boxen[candidates[s]].bounds;
			if (mush_bounds_on_same_primary_axis(consumer, bounds))
				subsumees->ptr[j++] = s;
		}

		if (j == s0) {
			// Just grab the first one instead of being smart about it.
			const mush_bounds *bounds =
				&space->boxen[candidates[subsumees->ptr[s0]]].bounds;

			j = s0 + 1;

			for (size_t i = j; i < subsumees->len; ++i) {
				size_t s = subsumees->ptr[i];
				const mush_bounds *sbounds = &space->boxen[candidates[s]].bounds;
				if (mush_bounds_on_same_axis(bounds, sbounds))
					subsumees->ptr[j++] = s;
			}
		}
		subsumees->len = j;
	}
#endif

	if (subsumees->len == s0)
		return false;

	assert (subsumees->len > s0);

	// Sort them so that we can find the correct offset to apply to the array
	// index (since we're removing these from candidates as we go): if the
	// lowest index is always next, the following ones' indices are reduced by
	// one.
	qsort(subsumees->ptr + s0, subsumees->len - s0,
	      sizeof *subsumees->ptr, mush_size_t_qsort_cmp);

	size_t offset = 0;
	for (size_t i = s0; i < subsumees->len; ++i) {
		size_t corrected = subsumees->ptr[i] - offset++;
		subsumees->ptr[i] = candidates[corrected];

		mushspace_min_max_size(
			consumer, consumee, used_cells,
			mushspace_get_caabb_idx(space, subsumees->ptr[i]));

		candidates[corrected] = space->box_count;

		mushstats_add(space->stats, MushStat_subsumed_fusables, 1);
	}
	return true;
}
static bool mushspace_subsume_disjoint(
	mushspace* space,
	size_t* candidates, mush_arr_size_t* subsumees, mush_bounds* consumer,
	mushspace_consumee* consumee, size_t* used_cells)
{
	const size_t s0 = subsumees->len;
	for (size_t i = 0; i < space->box_count; ++i) {
		size_t c = candidates[i];
		if (c == space->box_count)
			continue;

		mush_caabb_idx box = mushspace_get_caabb_idx(space, c);

		// All fusables have been removed, so a sufficient condition for
		// disjointness is non-overlappingness.
		if (mush_bounds_overlaps(consumer, &box.aabb->bounds))
			continue;

		if (mushspace_valid_min_max_size(mushspace_disjoint_mms_validator,
		                                 NULL, consumer, consumee,
		                                 used_cells, box))
		{
			subsumees->ptr[subsumees->len++] = c;
			candidates[i] = space->box_count;

			mushstats_add(space->stats, MushStat_subsumed_disjoint, 1);
		}
	}
	assert (subsumees->len >= s0);
	return subsumees->len > s0;
}
static bool mushspace_disjoint_mms_validator(
	const mush_bounds* b, const mush_aabb* fodder, size_t used_cells, void* nil)
{
	(void)nil;
	return mushspace_cheaper_to_alloc(
		mush_bounds_clamped_size(b), used_cells + fodder->size);
}
static bool mushspace_subsume_overlaps(
	mushspace* space,
	size_t* candidates, mush_arr_size_t* subsumees, mush_bounds* consumer,
	mushspace_consumee* consumee, size_t* used_cells)
{
	const size_t s0 = subsumees->len;
	for (size_t i = 0; i < space->box_count; ++i) {
		size_t c = candidates[i];
		if (c == space->box_count)
			continue;

		mush_caabb_idx box = mushspace_get_caabb_idx(space, c);

		if (!mush_bounds_overlaps(consumer, &box.aabb->bounds))
			continue;

		if (mushspace_valid_min_max_size(mushspace_overlaps_mms_validator,
		                                 consumer, consumer, consumee,
		                                 used_cells, box))
		{
			subsumees->ptr[subsumees->len++] = c;
			candidates[i] = space->box_count;

			mushstats_add(space->stats, MushStat_subsumed_overlaps, 1);
		}
	}
	assert (subsumees->len >= s0);
	return subsumees->len > s0;
}
static bool mushspace_overlaps_mms_validator(
	const mush_bounds* b, const mush_aabb* fodder, size_t used_cells, void* cp)
{
	const mush_bounds *consumer = cp;

	mush_aabb overlap;
	mush_bounds_get_overlap(consumer, &fodder->bounds, &overlap.bounds);
	mush_aabb_finalize(&overlap);

	return mushspace_cheaper_to_alloc(
		mush_bounds_clamped_size(b), used_cells + fodder->size - overlap.size);
}
static void mushspace_min_max_size(
	mush_bounds* bounds,
	mushspace_consumee* max, size_t* total_size,
	mush_caabb_idx box)
{
	*total_size += box.aabb->size;
	if (box.aabb->size > max->size) {
		max->size = box.aabb->size;
		max->idx  = box.idx;
	}
	if (bounds) {
		mushcoords_min_into(&bounds->beg, box.aabb->bounds.beg);
		mushcoords_max_into(&bounds->end, box.aabb->bounds.end);
	}
}
// Fills in the input values with the mushspace_min_max_size data, returning
// what the given validator function returns.
//
// The validator takes:
// - bounds of the box that subsumes
// - box to be subsumed (allocated)
// - number of cells that are currently contained in any box that the subsumer
//   contains
// - arbitrary user-provided data
static bool mushspace_valid_min_max_size(
	bool (*valid)(const mush_bounds*, const mush_aabb*, size_t, void*),
	void* userdata,
	mush_bounds* bounds,
	mushspace_consumee* max, size_t* total_size,
	mush_caabb_idx box)
{
	mush_bounds try_bounds = *bounds;
	mushspace_consumee try_max = *max;
	size_t try_total_size = *total_size;

	mushspace_min_max_size(&try_bounds, &try_max, &try_total_size, box);

	if (!valid(&try_bounds, box.aabb, *total_size, userdata))
		return false;

	*bounds     = try_bounds;
	*max        = try_max;
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
	mush_arr_size_t subsumees, size_t consumee, mush_aabb* consumer)
{
	assert (subsumees.len > 0);

	// This allocation needs to be up here: anything below the consumption has
	// to succeed unconditionally lest we leave the mushspace in a corrupted
	// state.
	size_t *removed_offsets =
		malloc(space->box_count * sizeof *removed_offsets);
	if (!removed_offsets)
		return false;

	mushspace_irrelevize_subsumption_order(space, subsumees);

	mush_aabb_finalize(consumer);
	if (!mush_aabb_consume(consumer, &space->boxen[consumee])) {
		free(removed_offsets);
		return false;
	}

	// NOTE: strictly speaking we should sort subsumees and go from
	// subsumees.len down to 0, since we don't want below-boxes to overwrite
	// top-boxes' data. However, mushspace_irrelevize_subsumption_order copies
	// the data so that the order is, in fact, irrelevant.
	//
	// I think that consumee would also have to be simply
	// subsumees.ptr[subsumees.len-1] after sorting, but I haven't thought this
	// completely through so I'm not sure.
	//
	// In debug mode, do exactly the "wrong" thing (subsume top-down), in the
	// hopes of catching a bug in mushspace_irrelevize_subsumption_order.

#ifndef NDEBUG
	qsort(subsumees.ptr, subsumees.len,
	      sizeof *subsumees.ptr, mush_size_t_qsort_cmp);
#endif

	for (size_t i = 0; i < subsumees.len; ++i) {
		size_t s = subsumees.ptr[i];
		if (s == consumee)
			continue;

		mush_aabb_subsume(consumer, &space->boxen[s]);
		free(space->boxen[s].data);
	}

	const size_t orig_box_count = space->box_count;

	size_t range_beg = subsumees.ptr[0], range_end = subsumees.ptr[0];

	for (size_t i = 1; i < subsumees.len; ++i) {
		const size_t s = subsumees.ptr[i];

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
	mushspace* space, mush_arr_size_t subsumees)
{
	for (size_t i = 0; i < subsumees.len; ++i) {
		size_t s = subsumees.ptr[i];
		mush_aabb* higher = &space->boxen[s];

		for (size_t t = s+1; t < space->box_count; ++t) {
			mush_aabb* lower = &space->boxen[t];

			if (   mush_bounds_contains_bounds(&higher->bounds, &lower->bounds)
			    || mush_bounds_contains_bounds(&lower->bounds, &higher->bounds))
				continue;

			mush_aabb overlap;

			// If they overlap, copy the overlap area to the lower box and space
			// that area in the higher one.
			if (mush_bounds_get_overlap(&higher->bounds, &lower->bounds,
			                            &overlap.bounds))
			{
				mush_aabb_finalize(&overlap);
				mush_aabb_subsume_area(lower, higher, &overlap);
				mush_aabb_space_area(higher, &overlap);
			}
		}
	}
}

static void mushspace_map_no_place(
	mushspace* space, const mush_aabb* aabb, void* fg,
	void(*f)(mush_arr_mushcell, void*, mushstats*), void(*g)(size_t, void*))
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
static bool mushspace_map_in_box(
	mushspace* space, mush_bounded_pos bpos, mush_caabb_idx cai,
	void* caller_data, void(*f)(mush_arr_mushcell, void*, mushstats*))
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

	f((mush_arr_mushcell){box->data, end_idx - beg_idx + 1},
	  caller_data, space->stats);
	return hit_end;
}
static bool mushspace_map_in_static(
	mushspace* space, mush_bounded_pos bpos,
	void* caller_data, void(*f)(mush_arr_mushcell, void*, mushstats*))
{
	bool hit_end;
	const size_t
		beg_idx = mush_staticaabb_get_idx(*bpos.pos),
		end_idx = mush_staticaabb_get_idx(mushcoords_get_end_of_contiguous_range(
			MUSH_STATICAABB_END, bpos.pos, bpos.bounds->end, bpos.bounds->beg,
			&hit_end, MUSH_STATICAABB_BEG, MUSH_STATICAABB_BEG));

	assert (beg_idx <= end_idx);

	f((mush_arr_mushcell){space->static_box.array, end_idx - beg_idx + 1},
	  caller_data, space->stats);
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
	mushspace* space, const mush_aabb* aabb, void* fg,
	void(*f)(mush_arr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*),
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
static bool mushspace_mapex_in_box(
	mushspace* space, mush_bounded_pos bpos,
	mush_caabb_idx cai,
	void* caller_data,
	void(*f)(mush_arr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*))
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
	  caller_data, width, area, line_start, page_start, &hit, space->stats);

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
static bool mushspace_mapex_in_static(
	mushspace* space, mush_bounded_pos bpos,
	void* caller_data,
	void(*f)(mush_arr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*,
	        mushstats*))
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
	  caller_data, width, area, line_start, page_start, &hit, space->stats);

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

int mushspace_load_string(mushspace* space, const char* str, size_t len,
                          mushcoords* end, mushcoords target, bool binary)
{
	mush_arr_mush_aabb aabbs;
	mushspace_get_aabbs(str, len, target, binary, &aabbs);

	if (!aabbs.ptr) {
		// The error code was placed in aabbs.len.
		return (int)aabbs.len;
	}

	if (end)
		*end = target;

	for (size_t i = 0; i < aabbs.len; ++i) {
		if (end)
			mushcoords_max_into(end, aabbs.ptr[i].bounds.end);

		if (!mushspace_place_box(space, &aabbs.ptr[i], NULL, NULL))
			return MUSH_ERR_OOM;
	}

	// Build one to rule them all.
	//
	// Note that it may have beg > end along any number of axes!
	mush_aabb aabb = aabbs.ptr[0];
	if (aabbs.len > 1) {
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

		mush_bounds *bounds = &aabb.bounds;

		uint8_t found_past = 0, found_before = 0;

		for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
			if (bounds->beg.v[i] < target.v[i]) found_past   |= 1 << i;
			else                                found_before |= 1 << i;
		}

		for (size_t b = 1; b < aabbs.len; ++b) {
			const mush_bounds *box = &aabbs.ptr[b].bounds;

			for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
				const uint8_t axis = 1 << i;

				if (box->beg.v[i] < target.v[i]) {
					// This box is past this axis, in the positive direction: it
					// both begins and ends in the negative direction from the
					// target location.

					if (found_past & axis)
						mushcell_max_into(&bounds->end.v[i], box->end.v[i]);
					else {
						bounds->end.v[i] = box->end.v[i];
						found_past |= axis;
					}

					if (!(found_before & axis))
						mushcell_min_into(&bounds->beg.v[i], box->beg.v[i]);
				} else {
					// This box is before this axis, in the negative direction: it
					// both begins and ends in the positive direction from the
					// target location.

					if (found_before & axis)
						mushcell_min_into(&bounds->beg.v[i], box->beg.v[i]);
					else {
						bounds->beg.v[i] = box->beg.v[i];
						found_before |= axis;
					}

					if (!(found_past & axis))
						mushcell_max_into(&bounds->end.v[i], box->end.v[i]);
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
			, target.x, target.x, aabb.bounds.beg.x
		#if MUSHSPACE_DIM >= 3
			, target.y, target.y, aabb.bounds.beg.y
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

// Sets aabbs_out to an array of AABBs (a slice out of a static buffer) which
// describe where the input should be loaded. There are at most 2^dim of them;
// in binary mode, at most 2.
//
// If nothing would be loaded, aabbs_out->ptr is set to NULL and an error code
// (an int) is written into aabbs_out->len.
static void mushspace_get_aabbs(
	const char* str, size_t len, mushcoords target, bool binary,
	mush_arr_mush_aabb* aabbs_out)
{
	static mush_aabb aabbs[1 << MUSHSPACE_DIM];

	mush_bounds *bounds = (mush_bounds*)aabbs;

	aabbs_out->ptr = aabbs;

	if (binary) {
		size_t n = mushspace_get_aabbs_binary(str, len, target, bounds);
		if (n == SIZE_MAX) {
			*aabbs_out = (mush_arr_mush_aabb){NULL, MUSH_ERR_NO_ROOM};
			return;
		}
		assert (n <= 2);
		aabbs_out->len = n;
		return;
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
		bounds[i].beg = MUSHCOORDS(MUSHCELL_MAX, MUSHCELL_MAX, MUSHCELL_MAX);
		bounds[i].end = MUSHCOORDS(MUSHCELL_MIN, MUSHCELL_MIN, MUSHCELL_MIN);
	}

	#if MUSHSPACE_DIM >= 2
		bool got_cr = false;

		const mush_arr_mush_bounds bounds_arr = {bounds, MUSH_ARRAY_LEN(aabbs)};

		#define mush_hit_newline do { \
			if (!mushspace_newline(&got_cr, &pos, target, \
			                       bounds_arr, &a, &max_a, \
			                       last_nonspace, &found_nonspace_for, &get_beg))\
			{ \
				*aabbs_out = (mush_arr_mush_aabb){NULL, MUSH_ERR_NO_ROOM}; \
				return; \
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
					bounds[a].beg.v[i] = mushcell_min(bounds[a].beg.v[i], pos.v[i]);
					get_beg &= ~(1 << i);
				}
			}
		}
		if ((pos.x = mushcell_inc(pos.x)) == MUSHCELL_MIN) {
			if (found_nonspace_for == a)
				mushcoords_max_into(&bounds[a].end, last_nonspace);

			found_nonspace_for = MUSH_ARRAY_LEN(aabbs);
			get_beg = DimensionBits;

			max_a = mush_size_t_max(max_a, a |= 0x01);

		} else if (pos.x == target.x) {
			// Oops, came back to where we started. That's not good.
			*aabbs_out = (mush_arr_mush_aabb){NULL, MUSH_ERR_NO_ROOM};
			return;
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
			mushcell_max_into(&bounds[a].end.x, last_nonspace.x);
			mushcell_max_into(&bounds[a].end.y, last_nonspace.y);

			pos.x = target.x;
			pos.y = target.y;

			if ((pos.z = mushcell_inc(pos.z)) == MUSHCELL_MIN) {
				if (found_nonspace_for == a)
					mushcoords_max_into(&bounds[a].end, last_nonspace);

				found_nonspace_for = MUSH_ARRAY_LEN(aabbs);

				max_a = mush_size_t_max(max_a, a |= 0x04);

			} else if (pos.z == target.z) {
				*aabbs_out = (mush_arr_mush_aabb){NULL, MUSH_ERR_NO_ROOM};
				return;
			}
			a &= ~0x03;
			get_beg = found_nonspace_for == a ? 0x03 : 0x07;
		#endif
		break;
	}
	#if MUSHSPACE_DIM >= 2
	#undef mush_hit_newline
	#endif

	if (found_nonspace_for_anyone == MUSH_ARRAY_LEN(aabbs)) {
		// Nothing to load. Not an error, but don't need to do anything so bail.
		*aabbs_out = (mush_arr_mush_aabb){NULL, MUSH_ERR_NONE};
		return;
	}

	if (found_nonspace_for < MUSH_ARRAY_LEN(aabbs))
		mushcoords_max_into(&bounds[found_nonspace_for].end, last_nonspace);

	// Since a is a bitmask, the AABBs that we used aren't necessarily in order.
	// Fix that.
	size_t n = 0;
	for (size_t i = 0; i <= max_a; ++i) {
		const mush_bounds *box = &bounds[i];

		if (!(box->beg.x == MUSHCELL_MAX && box->end.x == MUSHCELL_MIN)) {
			// The box has been initialized, so make sure it's valid and put it in
			// place.

			for (mushdim j = 0; j < MUSHSPACE_DIM; ++j)
				assert (box->beg.v[j] <= box->end.v[j]);

			if (i != n)
				bounds[n] = bounds[i];
			++n;
		}
	}
	aabbs_out->len = n;
}
#if MUSHSPACE_DIM >= 2
static bool mushspace_newline(
	bool* got_cr, mushcoords* pos, mushcoords target,
	mush_arr_mush_bounds bounds, size_t* a, size_t* max_a,
	mushcoords last_nonspace, size_t* found_nonspace_for, uint8_t* get_beg)
{
	*got_cr = false;

	mushcell_max_into(&bounds.ptr[*a].end.x, last_nonspace.x);

	pos->x = target.x;

	if ((pos->y = mushcell_inc(pos->y)) == MUSHCELL_MIN) {
		if (*found_nonspace_for == *a)
			mushcoords_max_into(&bounds.ptr[*a].end, last_nonspace);

		*found_nonspace_for = bounds.len;

		*max_a = mush_size_t_max(*max_a, *a |= 0x02);
	} else if (pos->y == target.y)
		return false;

	*a &= ~0x01;
	*get_beg = *found_nonspace_for == *a ? 0x01 : 0x03;
	return true;
}
#endif

static size_t mushspace_get_aabbs_binary(
	const char* str, size_t len, mushcoords target, mush_bounds* bounds)
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
		bounds[a++] = (mush_bounds){beg, end};
		beg.x = MUSHCELL_MIN;
	}
	end.x += i;

	bounds[a++] = (mush_bounds){beg, end};
	return a;
}

static void mushspace_binary_load_arr(
	mush_arr_mushcell arr, void* p, mushstats* stats)
{
	const char **strp = p, *str = *strp;
	for (mushcell *end = arr.ptr + arr.len; arr.ptr < end; ++arr.ptr) {
		char c = *str++;
		if (c != ' ') {
			*arr.ptr = c;
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
	mush_arr_mushcell arr, void* p,
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

	for (size_t i = 0; i < arr.len;) {
		assert (str < str_end);

		const char c = *str++;
		switch (c) {
		default:
			arr.ptr[i++] = c;
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

				if (i >= arr.len) {
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

				if (i >= arr.len) {
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
