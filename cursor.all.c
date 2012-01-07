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

static bool mushcursor_in_box(const mushcursor*);

#if !MUSHSPACE_93
static bool mushcursor_get_box(mushcursor*, mushcoords);
static void mushcursor_recalibrate_void(void*);
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
	cursor->pos   = pos;
	return cursor;
}

static bool mushcursor_in_box(const mushcursor* cursor) {
	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		return mush_staticaabb_contains(cursor->pos);

#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		return mush_bounds_contains(&cursor->box->bounds, cursor->pos);

	case MushCursorMode_bak:
		return mush_bounds_contains(&cursor->space->bak.bounds, cursor->pos);
#endif
	}
	assert (false);
}

#if !MUSHSPACE_93
static bool mushcursor_get_box(mushcursor* cursor, mushcoords pos) {
	if (mush_staticaabb_contains(pos)) {
		cursor->mode = MushCursorMode_static;
		return true;
	}

	mushspace *sp = cursor->space;

	if ((cursor->box = mushspace_find_box_and_idx(sp, pos, &cursor->box_idx))) {
		cursor->mode = MushCursorMode_dynamic;
		return true;
	}
	if (sp->bak.data && mush_bounds_contains(&sp->bak.bounds, pos)) {
		cursor->mode = MushCursorMode_bak;
		return true;
	}
	return false;
}
#endif

mushcell mushcursor_get(mushcursor* cursor) {
	mushspace *sp = cursor->space;

#if MUSHSPACE_93
	assert (mushcursor_in_box(cursor));
#else
	if (!mushcursor_in_box(cursor) && !mushcursor_get_box(cursor, cursor->pos))
	{
		mushstats_add(sp->stats, MushStat_lookups, 1);
		return ' ';
	}
#endif

	mushstats_add(sp->stats, MushStat_lookups, 1);

	switch (MUSHCURSOR_MODE(cursor)) {
	case MushCursorMode_static:
		return mush_staticaabb_get(STATIC_BOX(sp), cursor->pos);

#if !MUSHSPACE_93
	case MushCursorMode_dynamic:
		return mush_aabb_get(cursor->box, cursor->pos);

	case MushCursorMode_bak:
		return mush_bakaabb_get(&sp->bak, cursor->pos);
#endif
	}
	assert (false);
}

void mushcursor_recalibrate(mushcursor* cursor) {
#if MUSHSPACE_93
	(void)cursor;
#else
	if (!mushcursor_get_box(cursor, cursor->pos)) {
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
