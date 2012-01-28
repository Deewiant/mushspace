// File created: 2012-01-28 00:50:27

#ifndef MUSHSPACE_BOUNDS_RAY_INTERSECTS_H
#define MUSHSPACE_BOUNDS_RAY_INTERSECTS_H

#include "bounds.all.h"

#define mushbounds_ray_intersects MUSHSPACE_CAT(mushcoords,_ray_intersects)

bool mushbounds_ray_intersects(mushcoords, mushcoords,
                               const mushbounds*, mushucell*, mushcoords*);

#endif
