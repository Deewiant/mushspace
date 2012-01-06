// File created: 2011-09-02 23:36:15

#include "cursor.all.h"

#include <assert.h>

#if MUSHSPACE_93
#define STATIC_BOX(sp) (&(sp)->box)
#else
#define STATIC_BOX(sp) (&(sp)->static_box)
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

mushcell mushcursor_get(mushcursor* cursor) {
	mushspace *sp = cursor->space;

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
