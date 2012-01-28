// File created: 2012-01-27 20:59:31

#ifndef MUSHSPACE_SPACE_MAP_NO_PLACE_H
#define MUSHSPACE_SPACE_MAP_NO_PLACE_H

#include "space.all.h"

#define mushspace_map_no_place   MUSHSPACE_CAT(mushspace,_map_no_place)
#define mushspace_mapex_no_place MUSHSPACE_CAT(mushspace,_mapex_no_place)

typedef struct mushbounded_pos {
	const mushbounds *bounds;
	mushcoords *pos;
} mushbounded_pos;

void mushspace_map_no_place(
	mushspace*, const mushaabb*, void*,
	void(*)(musharr_mushcell, void*), void(*)(size_t, void*));

void mushspace_mapex_no_place(
	mushspace*, const mushaabb*, void*,
	void(*)(musharr_mushcell, void*, size_t, size_t, size_t, size_t, uint8_t*),
	void(*)(size_t, void*));

#endif
