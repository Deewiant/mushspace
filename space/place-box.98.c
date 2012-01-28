// File created: 2012-01-27 22:57:55

#include "space/place-box.98.h"

#include <assert.h>

#include "aabb/consume.98.h"
#include "aabb/subsume.98.h"
#include "aabb/space-area.98.h"
#include "space/heuristic-constants.98.h"

typedef struct consumee { size_t idx; size_t size; } consumee;

MUSH_DECL_DYN_ARRAY(size_t)

static mush_aabb* really_place_box(mushspace*, mush_aabb*);

static void subsume_contains(
	mushspace*, size_t*, mush_arr_size_t*, const mush_bounds*,
	consumee*, size_t*);

static bool subsume_fusables(
	mushspace*, size_t*, mush_arr_size_t*, mush_bounds*, consumee*, size_t*);

static bool subsume_disjoint(
	mushspace*, size_t*, mush_arr_size_t*, mush_bounds*, consumee*, size_t*);

static bool disjoint_mms_validator(
	const mush_bounds*, const mush_aabb*, size_t, void*);

static bool subsume_overlaps(
	mushspace*, size_t*, mush_arr_size_t*, mush_bounds*, consumee*, size_t*);

static bool overlaps_mms_validator(
	const mush_bounds*, const mush_aabb*, size_t, void*);

static void min_max_size(mush_bounds*, consumee*, size_t*, mush_caabb_idx);

static bool valid_min_max_size(
	bool (*)(const mush_bounds*, const mush_aabb*, size_t, void*), void*,
	mush_bounds*, consumee*, size_t*, mush_caabb_idx);

static bool cheaper_to_alloc(size_t, size_t);

static bool consume_and_subsume(
	mushspace*, mush_arr_size_t, size_t, mush_aabb*);

static void irrelevize_subsumption_order(mushspace*, mush_arr_size_t);

bool mushspace_place_box(
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
		box = really_place_box(space, box);
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

// Returns the placed box, which may be bigger than the given box. Returns NULL
// if memory allocation failed.
static mush_aabb* really_place_box(mushspace* space, mush_aabb* aabb)
{
	for (size_t i = 0; i < space->box_count; ++i)
		assert (!mush_bounds_contains_bounds(
			&space->boxen[i].bounds, &aabb->bounds));

	mush_arr_size_t subsumees;

	subsumees.ptr      = malloc(space->box_count * sizeof *subsumees.ptr);
	size_t *candidates = malloc(space->box_count * sizeof *candidates);

	// If space->box_count is zero, malloc can validly return NULL.
	if ((!subsumees.ptr || !candidates) && space->box_count != 0) {
		free(subsumees.ptr);
		free(candidates);
		return NULL;
	}

	consumee consumee = {.size = 0};
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

		    subsume_contains(PARAMS);
		if (subsume_fusables(PARAMS)) continue;
		if (subsume_disjoint(PARAMS)) continue;
		if (subsume_overlaps(PARAMS)) continue;
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
		ok = consume_and_subsume(space, subsumees, consumee.idx, &consumer);
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
static void subsume_contains(
	mushspace* space,
	size_t* candidates, mush_arr_size_t* subsumees,	const mush_bounds* consumer,
	consumee* consumee, size_t* used_cells)
{
	for (size_t i = 0; i < space->box_count; ++i) {
		size_t c = candidates[i];
		if (c == space->box_count)
			continue;

		mush_caabb_idx box = mushspace_get_caabb_idx(space, c);

		if (!mush_bounds_contains_bounds(consumer, &box.aabb->bounds))
			continue;

		subsumees->ptr[subsumees->len++] = c;
		min_max_size(NULL, consumee, used_cells, box);

		candidates[i] = space->box_count;

		mushstats_add(space->stats, MushStat_subsumed_contains, 1);
	}
}
static bool subsume_fusables(
	mushspace* space,
	size_t* candidates, mush_arr_size_t* subsumees, mush_bounds* consumer,
	consumee* consumee, size_t* used_cells)
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

		min_max_size(consumer, consumee, used_cells,
		             mushspace_get_caabb_idx(space, subsumees->ptr[i]));

		candidates[corrected] = space->box_count;

		mushstats_add(space->stats, MushStat_subsumed_fusables, 1);
	}
	return true;
}
static bool subsume_disjoint(
	mushspace* space,
	size_t* candidates, mush_arr_size_t* subsumees, mush_bounds* consumer,
	consumee* consumee, size_t* used_cells)
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

		if (valid_min_max_size(disjoint_mms_validator, NULL, consumer, consumee,
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
static bool disjoint_mms_validator(
	const mush_bounds* b, const mush_aabb* fodder, size_t used_cells, void* nil)
{
	(void)nil;
	return cheaper_to_alloc(
		mush_bounds_clamped_size(b), used_cells + fodder->size);
}
static bool subsume_overlaps(
	mushspace* space,
	size_t* candidates, mush_arr_size_t* subsumees, mush_bounds* consumer,
	consumee* consumee, size_t* used_cells)
{
	const size_t s0 = subsumees->len;
	for (size_t i = 0; i < space->box_count; ++i) {
		size_t c = candidates[i];
		if (c == space->box_count)
			continue;

		mush_caabb_idx box = mushspace_get_caabb_idx(space, c);

		if (!mush_bounds_overlaps(consumer, &box.aabb->bounds))
			continue;

		if (valid_min_max_size(overlaps_mms_validator, consumer, consumer,
		                       consumee, used_cells, box))
		{
			subsumees->ptr[subsumees->len++] = c;
			candidates[i] = space->box_count;

			mushstats_add(space->stats, MushStat_subsumed_overlaps, 1);
		}
	}
	assert (subsumees->len >= s0);
	return subsumees->len > s0;
}
static bool overlaps_mms_validator(
	const mush_bounds* b, const mush_aabb* fodder, size_t used_cells, void* cp)
{
	const mush_bounds *consumer = cp;

	mush_aabb overlap;
	mush_bounds_get_overlap(consumer, &fodder->bounds, &overlap.bounds);
	mush_aabb_finalize(&overlap);

	return cheaper_to_alloc(
		mush_bounds_clamped_size(b), used_cells + fodder->size - overlap.size);
}
static void min_max_size(
	mush_bounds* bounds,
	consumee* max, size_t* total_size, mush_caabb_idx box)
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
// Fills in the input values with the min_max_size data, returning what the
// given validator function returns.
//
// The validator takes:
// - bounds of the box that subsumes
// - box to be subsumed (allocated)
// - number of cells that are currently contained in any box that the subsumer
//   contains
// - arbitrary user-provided data
static bool valid_min_max_size(
	bool (*valid)(const mush_bounds*, const mush_aabb*, size_t, void*),
	void* userdata,
	mush_bounds* bounds,
	consumee* max, size_t* total_size,
	mush_caabb_idx box)
{
	mush_bounds try_bounds = *bounds;
	consumee try_max = *max;
	size_t try_total_size = *total_size;

	min_max_size(&try_bounds, &try_max, &try_total_size, box);

	if (!valid(&try_bounds, box.aabb, *total_size, userdata))
		return false;

	*bounds     = try_bounds;
	*max        = try_max;
	*total_size = try_total_size;
	return true;
}
static bool cheaper_to_alloc(size_t together, size_t separate) {
	return together <= ACCEPTABLE_WASTE
	    ||   sizeof(mushcell) * (together - ACCEPTABLE_WASTE)
	       < sizeof(mushcell) * separate + sizeof(mush_aabb);
}
static bool consume_and_subsume(
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

	irrelevize_subsumption_order(space, subsumees);

	mush_aabb_finalize(consumer);
	if (!mush_aabb_consume(consumer, &space->boxen[consumee])) {
		free(removed_offsets);
		return false;
	}

	// NOTE: strictly speaking we should sort subsumees and go from
	// subsumees.len down to 0, since we don't want below-boxes to overwrite
	// top-boxes' data. However, irrelevize_subsumption_order copies the data so
	// that the order is, in fact, irrelevant.
	//
	// I think that consumee would also have to be simply
	// subsumees.ptr[subsumees.len-1] after sorting, but I haven't thought this
	// completely through so I'm not sure.
	//
	// In debug mode, do exactly the "wrong" thing (subsume top-down), in the
	// hopes of catching a bug in irrelevize_subsumption_order.

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
// Since mushspace_get_tight_bounds checks all boxes without considering
// overlappingness, we also space the overlapped area in C to prevent mishaps
// there.
//
// Caveat: this assumes that the final box will always be placed bottom-most.
// This does not really matter, it's just extra work if it's not; but in any
// case, if not, the relevant overlapping boxes would be those which would end
// up above the final box.
static void irrelevize_subsumption_order(
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
