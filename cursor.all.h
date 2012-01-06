// File created: 2011-09-02 23:36:23

#ifndef MUSHSPACE_CURSOR_H
#define MUSHSPACE_CURSOR_H

#include "space.all.h"

#define mushcursor MUSHSPACE_NAME(mushcursor)

// What kind of an area is the cursor in?
typedef enum MushCursorMode {
	MushCursorMode_static,
#if !MUSHSPACE_93
	MushCursorMode_dynamic,
	MushCursorMode_bak,
#endif
} MushCursorMode;

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
	mushcoords pos;
#if !MUSHSPACE_93
	size_t     box_idx;
	mush_aabb *box;
#endif
} mushcursor;

#define mushcursor_sizeof MUSHSPACE_CAT(mushcursor,_sizeof)
#define mushcursor_init   MUSHSPACE_CAT(mushcursor,_init)
#define mushcursor_get    MUSHSPACE_CAT(mushcursor,_get)

extern const size_t mushcursor_sizeof;

mushcursor* mushcursor_init(mushspace*, mushcoords, mushcoords, void*);

mushcell mushcursor_get(mushcursor*);

#endif
