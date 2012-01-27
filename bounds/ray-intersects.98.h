// File created: 2012-01-28 00:50:27

#ifndef MUSHSPACE_BOUNDS_RAY_INTERSECTS_H
#define MUSHSPACE_BOUNDS_RAY_INTERSECTS_H

#include "bounds.all.h"

#define mush_bounds_ray_intersects MUSHSPACE_CAT(mushcoords,_ray_intersects)

bool mush_bounds_ray_intersects(mushcoords, mushcoords,
                                const mush_bounds*, mushucell*, mushcoords*);

#endif
