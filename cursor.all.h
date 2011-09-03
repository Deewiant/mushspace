// File created: 2011-09-02 23:36:23

#ifndef MUSHSPACE_CURSOR_H
#define MUSHSPACE_CURSOR_H

#include "space.all.h"

#define mushcursor MUSHSPACE_NAME(mushcursor)

typedef struct mushcursor {
	mushspace *space;
} mushcursor;

#define mushcursor_sizeof MUSHSPACE_CAT(mushcursor,_sizeof)
#define mushcursor_init   MUSHSPACE_CAT(mushcursor,_init)

extern const size_t mushcursor_sizeof;

mushcursor* mushcursor_init(mushspace*, mushcoords, mushcoords, void*);

#endif
