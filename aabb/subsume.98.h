// File created: 2012-01-28 01:06:27

#ifndef MUSHSPACE_AABB_SUBSUME_H
#define MUSHSPACE_AABB_SUBSUME_H

#include "aabb.98.h"

#define mushaabb_subsume      MUSHSPACE_CAT(mushaabb,_subsume)
#define mushaabb_subsume_area MUSHSPACE_CAT(mushaabb,_subsume_area)

void mushaabb_subsume     (mushaabb*, const mushaabb*);
void mushaabb_subsume_area(mushaabb*, const mushaabb*, const mushaabb*);

#endif
