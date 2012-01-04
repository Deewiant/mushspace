// File created: 2011-09-02 23:36:23

#ifndef MUSHSPACE_CURSOR_H
#define MUSHSPACE_CURSOR_H

#include "space.all.h"

#define mushcursor MUSHSPACE_NAME(mushcursor)

// What kind of an area is the cursor in?
typedef enum MushCursorMode {
	MushCursorMode_static,
	MushCursorMode_dynamic,
	MushCursorMode_bak,
} MushCursorMode;

typedef struct mushcursor {
#if !MUSHSPACE_93
	MushCursorMode mode;
#endif
	mushspace *space;
} mushcursor;

#define mushcursor_sizeof MUSHSPACE_CAT(mushcursor,_sizeof)
#define mushcursor_init   MUSHSPACE_CAT(mushcursor,_init)

extern const size_t mushcursor_sizeof;

mushcursor* mushcursor_init(mushspace*, mushcoords, mushcoords, void*);

#endif
