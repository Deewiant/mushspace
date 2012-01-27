// File created: 2012-01-28 00:58:15

#ifndef MUSHSPACE_AABB_CONSUME_H
#define MUSHSPACE_AABB_CONSUME_H

#include "aabb.98.h"

#define mush_aabb_consume MUSHSPACE_CAT(mush_aabb,_consume)

// box should be unallocated and old allocated.
//
// box takes ownership of old's array. old must be contained within box.
bool mush_aabb_consume(mush_aabb* box, mush_aabb* old);

#endif
