// File created: 2011-09-02 23:36:15

#include "cursor.all.h"

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
	return cursor;
}
