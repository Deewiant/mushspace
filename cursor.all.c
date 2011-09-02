// File created: 2011-09-02 23:36:15

#include "cursor.all.h"

struct mushcursor {
	mushspace *space;
};

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
