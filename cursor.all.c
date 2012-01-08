// File created: 2011-09-02 23:36:15

#include "cursor.all.h"

#include <assert.h>

#if MUSHSPACE_93
#define STATIC_BOX(sp) (&(sp)->box)
#else
#define STATIC_BOX(sp) (&(sp)->static_box)
#endif

#define mushcursor_in_box  MUSHSPACE_CAT(mushcursor,_in_box)
#define mushcursor_get_box MUSHSPACE_CAT(mushcursor,_get_box)

static bool mushcursor_in_box(const mushcursor*);

#if !MUSHSPACE_93
static bool mushcursor_get_box(mushcursor*, mushcoords);
#endif

const size_t mushcursor_sizeof = sizeof(mushcursor);

mushcursor* mushcursor_init(
	mushspace* space, mushcoords pos, mushcoords delta, void* vp)
{
	mushcursor *cursor = vp ? vp : malloc(sizeof *cursor);
	if (!cursor)
		return NULL;

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
	if (!mushcursor_in_box(cursor))
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
