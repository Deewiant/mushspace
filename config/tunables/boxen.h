// File created: 2012-11-20 18:23:44

#ifndef MUSH_CONFIG_TUNABLES_BOXEN_H
#define MUSH_CONFIG_TUNABLES_BOXEN_H

#include "config/tunables/overall.h"

#ifdef BOXEN_RTREE

// Possibilities here:
//
// 1. The quadratic split, which becomes cubic due to T-ordering preserval.
// 2. The linear split, with unknown T-ordering costs; probably also cubic.
// 3 and 4. The same choices but without caring about T-ordering: just store a
//          T-order uint64 in every node. Then when searching, order based on
//          that. The tree structure still allows for speedup in all cases.
// 5+. R+ trees
// 6. R* trees
//
// TODO: figure out the best one for our use-case.
//
// Currently only 1, R_SPLIT_QUADRATIC, is implemented.
#define R_SPLIT_QUADRATIC

// Must be at most R_BRANCHING_FACTOR / 2, or splitting won't work.
//
// Doesn't apply to the root, of course.
//
// Read XS as "things" or "elements" or whatever.
#define R_MIN_XS_PER_NODE (R_BRANCHING_FACTOR / 2)

#endif

#endif
