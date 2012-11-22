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
// Currently only 1 and 3, R_SPLIT_QUADRATIC without or with
// R_T_ORDER_AT_SEARCH_TIME respectively, are implemented.
#define R_SPLIT_QUADRATIC

// Instead of worrying about T-ordering when inserting and splitting, use the
// vanilla R-tree algorithms, but store a monotonically increasing T-order
// value in each leaf node (and a range of T-orders in each nonleaf). Then,
// when searching, order the results based on that.
//
// This essentially moves the cost of T-ordering from modifications to queries.
// But it's not that clear-cut, because giving more freedom to the modification
// functions may result in a better ordered and balanced tree which can make
// queries cheaper again.
//#define R_T_ORDER_AT_SEARCH_TIME

// Must be at most R_BRANCHING_FACTOR / 2, or splitting won't work.
//
// Doesn't apply to the root, of course.
//
// Read XS as "things" or "elements" or whatever.
#define R_MIN_XS_PER_NODE (R_BRANCHING_FACTOR / 2)

// The R-tree branching factor. Can be given either by #defining
// R_BRANCHING_FACTOR_FROM_ROOT_SIZE, which results in as high a possible
// branching factor so that the root's sizeof is at most that size, or directly
// by #defining R_BRANCHING_FACTOR_DIRECT.
//
// Either way, if the end result is too large, you may get errors about R_IDX
// not being able to hold the value.
#define R_BRANCHING_FACTOR_FROM_ROOT_SIZE (1 << 11)

#endif

#endif
