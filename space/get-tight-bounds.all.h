// File created: 2012-01-28 00:18:04

#ifndef MUSHSPACE_SPACE_GET_TIGHT_BOUNDS_H
#define MUSHSPACE_SPACE_GET_TIGHT_BOUNDS_H

#include "space.all.h"

#define mushspace_get_tight_bounds MUSHSPACE_CAT(mushspace,_get_tight_bounds)

// Returns false if the space is completely empty. In that case, the bounds are
// undefined.
bool mushspace_get_tight_bounds(
#if MUSHSPACE_93
	const
#endif
	mushspace*, mushbounds*);

#endif
