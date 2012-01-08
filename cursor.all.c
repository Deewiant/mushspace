// File created: 2011-09-02 23:36:15

#include "cursor.all.h"

#include <assert.h>

#if MUSHSPACE_93
#define STATIC_BOX(sp) (&(sp)->box)
#else
#define STATIC_BOX(sp) (&(sp)->static_box)
#endif

#define mushcursor_in_box           MUSHSPACE_CAT(mushcursor,_in_box)
#define mushcursor_get_box          MUSHSPACE_CAT(mushcursor,_get_box)
#define mushcursor_recalibrate_void MUSHSPACE_CAT(mushcursor,_recalibrate_void)
#define mushcursor_tessellate       MUSHSPACE_CAT(mushcursor,_tessellate)

static bool mushcursor_in_box(const mushcursor*);

#if !MUSHSPACE_93
static bool mushcursor_get_box(mushcursor*, mushcoords);
static void mushcursor_recalibrate_void(void*);
static void mushcursor_tessellate(mushcursor*, mushcoords);
#endif

const size_t mushcursor_sizeof = sizeof(mushcursor);

mushcursor* mushcursor_init(
	mushspace* space, mushcoords pos, mushcoords delta, void* vp)
{
	mushcursor *cursor = vp ? vp : malloc(sizeof *cursor);
	if (!cursor)
		return NULL;

#if !MUSHSPACE_93
	mushspace_add_invalidatee(space, mushcursor_recalibrate_void, cursor);
#endif

	cursor->space = space;

#if MUSHSPACE_93
	cursor->rel_pos = mushcoords_sub(pos, MUSH_STATICAABB_BEG);
#else
	if (!mushcursor_get_box(cursor, pos))
		assert (false && "TODO");
#endif
	return cursor;
}

mushcoords mushcursor_get_pos(const mushcursor* cursor) {
	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		return mushcoords_add(cursor->rel_pos, MUSH_STATICAABB_BEG);
#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		return mushcoords_add(cursor->rel_pos, cursor->obeg);
	case MushCursorMode_bak:
		return cursor->actual_pos;
#endif
	}
	assert (false);
}

static bool mushcursor_in_box(const mushcursor* cursor) {
	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		return mush_bounds_contains(&MUSH_STATICAABB_REL_BOUNDS, cursor->rel_pos);

#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		return mush_bounds_contains(&cursor->rel_bounds, cursor->rel_pos);

	case MushCursorMode_bak:
		return mush_bounds_contains(&cursor->actual_bounds, cursor->actual_pos);
#endif
	}
	assert (false);
}

#if !MUSHSPACE_93
static bool mushcursor_get_box(mushcursor* cursor, mushcoords pos) {
	if (mush_staticaabb_contains(pos)) {
		cursor->mode = MushCursorMode_static;
		mushcursor_tessellate(cursor, pos);
		return true;
	}

	mushspace *sp = cursor->space;

	if ((cursor->box = mushspace_find_box_and_idx(sp, pos, &cursor->box_idx))) {
		cursor->mode = MushCursorMode_dynamic;
		mushcursor_tessellate(cursor, pos);
		return true;
	}
	if (sp->bak.data && mush_bounds_contains(&sp->bak.bounds, pos)) {
		cursor->mode = MushCursorMode_bak;
		mushcursor_tessellate(cursor, pos);
		return true;
	}
	return false;
}
#endif

mushcell mushcursor_get(mushcursor* cursor) {
#if MUSHSPACE_93
	assert (mushcursor_in_box(cursor));
#else
	if (!mushcursor_in_box(cursor)
	 && !mushcursor_get_box(cursor, mushcursor_get_pos(cursor)))
	{
		mushstats_add(cursor->space->stats, MushStat_lookups, 1);
		return ' ';
	}
#endif
	return mushcursor_get_unsafe(cursor);
}
mushcell mushcursor_get_unsafe(mushcursor* cursor) {
	mushspace *sp = cursor->space;

	mushstats_add(sp->stats, MushStat_lookups, 1);

	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		return mush_staticaabb_get_no_offset(STATIC_BOX(sp), cursor->rel_pos);

#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		return mush_aabb_get_no_offset(cursor->box, cursor->rel_pos);

	case MushCursorMode_bak:
		return mush_bakaabb_get(&sp->bak, cursor->actual_pos);
#endif
	}
	assert (false);
}

void mushcursor_recalibrate(mushcursor* cursor) {
#if MUSHSPACE_93
	(void)cursor;
#else
	if (!mushcursor_get_box(cursor, mushcursor_get_pos(cursor))) {
		// Just grab a box which we aren't contained in: get/set can handle it
		// and skip_markers can sort it out. Prefer static because it's the
		// fastest to work with.
		cursor->mode = MushCursorMode_static;
	}
#endif
}
#if !MUSHSPACE_93
static void mushcursor_recalibrate_void(void* cursor) {
	mushcursor_recalibrate(cursor);
}
#endif

#if !MUSHSPACE_93
static void mushcursor_tessellate(mushcursor* cursor, mushcoords pos) {
	mushspace *sp = cursor->space;

	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		cursor->rel_pos = mushcoords_sub(pos, MUSH_STATICAABB_BEG);
		break;

	case MushCursorMode_bak:
		cursor->actual_pos    = pos;
		cursor->actual_bounds = sp->bak.bounds;

		// bak is the lowest, so we tessellate with all boxes.
		mush_bounds_tessellate1(
			&cursor->actual_bounds, pos, &MUSH_STATICAABB_BOUNDS);
		mush_bounds_tessellate(
			&cursor->actual_bounds, pos,
			(mush_carr_mush_bounds)
				{(const mush_bounds*)sp->boxen, sp->box_count});
		break;

	case MushCursorMode_dynamic: {
		// cursor->box now becomes only a view. it shares its data array with the
		// original box, but has different bounds. In addition, it is weird: its
		// width and height are not its own, so that index calculation in the
		// _no_offset functions works correctly.
		//
		// BE CAREFUL! Only the *_no_offset functions work properly on it, since
		// the others (notably, _get_idx and thereby _get and _put) tend to
		// depend on the bounds matching the data and the width/height being
		// sensible.

		mush_bounds *bounds = &cursor->box->bounds;
		cursor->obeg = bounds->beg;

		// Here we need to tessellate only with the boxes above cursor->box.
		mush_bounds_tessellate1(bounds, pos, &MUSH_STATICAABB_BOUNDS);
		for (size_t i = 0; i < cursor->box_idx; ++i)
			if (mush_bounds_overlaps(bounds, &sp->boxen[i].bounds))
				mush_bounds_tessellate1(bounds, pos, &sp->boxen[i].bounds);

		cursor->rel_pos    = mushcoords_sub(pos, cursor->obeg);
		cursor->rel_bounds =
			(mush_bounds){mushcoords_sub(bounds->beg, cursor->obeg),
			              mushcoords_sub(bounds->end, cursor->obeg)};
		break;
	}

	default: assert (false);
	}
}
#endif
