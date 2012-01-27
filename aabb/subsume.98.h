// File created: 2012-01-28 01:06:27

#ifndef MUSHSPACE_AABB_SUBSUME_H
#define MUSHSPACE_AABB_SUBSUME_H

#include "aabb.98.h"

#define mush_aabb_subsume      MUSHSPACE_CAT(mush_aabb,_subsume)
#define mush_aabb_subsume_area MUSHSPACE_CAT(mush_aabb,_subsume_area)

void mush_aabb_subsume     (mush_aabb*, const mush_aabb*);
void mush_aabb_subsume_area(mush_aabb*, const mush_aabb*, const mush_aabb*);

#endif
