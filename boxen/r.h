// File created: 2012-11-08 11:26:49

#include <stddef.h>
#include <stdint.h>

typedef struct rtree rtree;

union size_helper { rtree *branch_node; mushaabb leaf_aabb; };

#define R_DEPTH uint8_t
#define R_IDX   int_fast8_t

// As many branches as possible so that the root fits in 2k.
//
// The R_IDX cast is to silence spurious "comparison between signed and
// unsigned" warnings, but then we need a separate #define for the struct
// definition.
#define R_BRANCHING_FACTOR ((R_IDX)R_BRANCHING_FACTOR_N)

#define R_BRANCHING_FACTOR_CANDIDATE \
   (((1 << 11) - sizeof(R_DEPTH) - sizeof(size_t) - sizeof(R_IDX)) \
    / (sizeof(mushbounds) + sizeof(union size_helper)))

// The branching factor must be at least 3: the algorithms can't handle less.
#define R_BRANCHING_FACTOR_N \
   (R_BRANCHING_FACTOR_CANDIDATE < 3 ? 3 : R_BRANCHING_FACTOR_CANDIDATE)

_Static_assert(R_BRANCHING_FACTOR_N <= INT_FAST8_MAX,
               "rare platform: please bump R_IDX from int_fast8_t");

// An R-tree.
//
// T-ordering is implemented by keeping the invariant that for any box A that
// is T-above another box B, A is either in a lesser index in the same node as
// B, or, for an ancestor P of B, in a subtree rooted at P or rooted at a
// lesser index in the same node as P. Colloquially: A has to be "to the left"
// of B in the tree.
struct rtree {
   // The sign bit determines whether this is a leaf: nonleafs have negative
   // counts.
   R_IDX count;

   mushbounds bounds[R_BRANCHING_FACTOR_N];
   union {
      rtree   *branch_nodes[R_BRANCHING_FACTOR_N];
      mushaabb leaf_aabbs  [R_BRANCHING_FACTOR_N];
   };
};

typedef struct mushboxen {
   // This is also the depth of all leaf nodes. The depth of the root is zero.
   R_DEPTH max_depth;

   size_t count;
   rtree  root;
} mushboxen;

typedef struct mushboxen_iter {
   rtree *node;
   R_IDX  idx;
   R_DEPTH path_depth;

   // Only used when traversing: not (guaranteed to be) filled in when
   // returning from mushboxen_get_iter or mushboxen_insert, for example.
   void *aux;
} mushboxen_iter;

typedef struct {
   mushboxen_iter iter;
   const mushbounds *bounds;
} mushboxen_iter_above, mushboxen_iter_below,
  mushboxen_iter_in, mushboxen_iter_in_bottomup, mushboxen_iter_out;

typedef struct {
   mushboxen_iter iter;
   const mushbounds *over, *out;
} mushboxen_iter_overout;

typedef struct mushboxen_reservation { char unused; } mushboxen_reservation;

typedef struct mushboxen_remsched {
   rtree *node;
   R_IDX beg, end, also;

   void *aux;
   R_DEPTH path_depth;
} mushboxen_remsched;
