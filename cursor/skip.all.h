// File created: 2012-01-28 01:17:50

#ifndef MUSHSPACE_CURSOR_SKIP_H
#define MUSHSPACE_CURSOR_SKIP_H

#include "cursor.all.h"

#define mushcursor_skip_spaces     MUSHSPACE_CAT(mushcursor,_skip_spaces)
#define mushcursor_skip_semicolons MUSHSPACE_CAT(mushcursor,_skip_semicolons)
#define mushcursor_skip_to_last_space \
	MUSHSPACE_CAT(mushcursor,_skip_to_last_space)

#if MUSHSPACE_93
#define mushcursor_skip_markers mushcursor2_93_skip_markers_98
#else
#define mushcursor_skip_markers MUSHSPACE_CAT(mushcursor,_skip_markers)
#endif

int mushcursor_skip_markers      (mushcursor*, mushcoords);
int mushcursor_skip_semicolons   (mushcursor*, mushcoords);
int mushcursor_skip_spaces       (mushcursor*, mushcoords);
int mushcursor_skip_to_last_space(mushcursor*, mushcoords);

#endif
