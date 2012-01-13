// File created: 2011-09-02 23:36:23

#ifndef MUSHSPACE_CURSOR_H
#define MUSHSPACE_CURSOR_H

#include "space.all.h"

#define mushcursor MUSHSPACE_NAME(mushcursor)

#if MUSHSPACE_93
#define MUSHCURSOR_MODE(x) MushCursorMode_static
#else
#define MUSHCURSOR_MODE(x) ((x)->mode)
#endif

typedef struct mushcursor {
#if !MUSHSPACE_93
	MushCursorMode mode;
#endif
	mushspace *space;
	union {
		// For dynamic mode.
		struct {
			// For static mode (only rel_pos).
			mushcoords rel_pos;

#if !MUSHSPACE_93
			mush_bounds rel_bounds;
			mushcoords  obeg;
			mush_aabb  *box;
			size_t      box_idx;
#endif
		};
#if !MUSHSPACE_93
		// For bak mode.
		struct {
			mushcoords  actual_pos;
			mush_bounds actual_bounds;
		};
#endif
	};
} mushcursor;

#define mushcursor_sizeof      MUSHSPACE_CAT(mushcursor,_sizeof)
#define mushcursor_init        MUSHSPACE_CAT(mushcursor,_init)
#define mushcursor_get_pos     MUSHSPACE_CAT(mushcursor,_get_pos)
#define mushcursor_set_pos     MUSHSPACE_CAT(mushcursor,_set_pos)
#define mushcursor_get         MUSHSPACE_CAT(mushcursor,_get)
#define mushcursor_get_unsafe  MUSHSPACE_CAT(mushcursor,_get_unsafe)
#define mushcursor_put         MUSHSPACE_CAT(mushcursor,_put)
#define mushcursor_put_unsafe  MUSHSPACE_CAT(mushcursor,_put_unsafe)
#define mushcursor_advance     MUSHSPACE_CAT(mushcursor,_advance)
#define mushcursor_retreat     MUSHSPACE_CAT(mushcursor,_retreat)
#define mushcursor_recalibrate MUSHSPACE_CAT(mushcursor,_recalibrate)

extern const size_t mushcursor_sizeof;

int mushcursor_init(mushspace*, mushcoords, mushcoords, void**);

mushcoords mushcursor_get_pos(const mushcursor*);
void       mushcursor_set_pos(      mushcursor*, mushcoords);

mushcell mushcursor_get       (mushcursor*);
mushcell mushcursor_get_unsafe(mushcursor*);
void     mushcursor_put       (mushcursor*, mushcell);
void     mushcursor_put_unsafe(mushcursor*, mushcell);

void mushcursor_advance(mushcursor*, mushcoords);
void mushcursor_retreat(mushcursor*, mushcoords);

void mushcursor_recalibrate(mushcursor*);

#endif
