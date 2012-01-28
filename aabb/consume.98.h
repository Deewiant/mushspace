// File created: 2012-01-28 00:58:15

#ifndef MUSHSPACE_AABB_CONSUME_H
#define MUSHSPACE_AABB_CONSUME_H

#include "aabb.98.h"

#define mushaabb_consume MUSHSPACE_CAT(mushaabb,_consume)

// box should be unallocated and old allocated.
//
// box takes ownership of old's array. old must be contained within box.
bool mushaabb_consume(mushaabb* box, mushaabb* old);

#endif
