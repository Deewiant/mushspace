// File created: 2012-11-08 11:26:49

#include <stddef.h>
#include <stdint.h>

#include "config/tunables/boxen.h"

typedef struct rtree rtree;

union size_helper { rtree *branch_node; mushaabb leaf_aabb; };

#ifdef R_T_ORDER_AT_SEARCH_TIME
typedef struct r_time_range { uint64_t min, max; } r_time_range;
#endif

#define R_DEPTH uint8_t
#define R_IDX   int_fast8_t

// The R_IDX cast is to silence spurious "comparison between signed and
// unsigned" warnings, but then we need a separate #define for the struct
// definition.
#define R_BRANCHING_FACTOR ((R_IDX)R_BRANCHING_FACTOR_N)

#ifdef R_BRANCHING_FACTOR_FROM_ROOT_SIZE
#ifdef R_BRANCHING_FACTOR_DIRECT
#error Define only one of R_BRANCHING_FACTOR_FROM_ROOT_SIZE \
and R_BRANCHING_FACTOR_DIRECT, please!
#endif

#ifdef R_T_ORDER_AT_SEARCH_TIME
#define R_ARRAYS_ELEMENT_SIZEOF \
   (sizeof(mushbounds) + sizeof(union size_helper) + sizeof(r_time_range))

#define R_ROOT_NONARRAY_SIZEOF \
   (sizeof(R_DEPTH) + sizeof(size_t) + sizeof(uint64_t) + sizeof(R_IDX))
#else
#define R_ARRAYS_ELEMENT_SIZEOF \
   (sizeof(mushbounds) + sizeof(union size_helper))

#define R_ROOT_NONARRAY_SIZEOF \
   (sizeof(R_DEPTH) + sizeof(size_t) + sizeof(R_IDX))
#endif

#define R_BRANCHING_FACTOR_CANDIDATE \
   ((R_BRANCHING_FACTOR_FROM_ROOT_SIZE - R_ROOT_NONARRAY_SIZEOF) \
    / R_ARRAYS_ELEMENT_SIZEOF)

#elif defined(R_BRANCHING_FACTOR_DIRECT)
#define R_BRANCHING_FACTOR_CANDIDATE R_BRANCHING_FACTOR_DIRECT
#else
#error One of R_BRANCHING_FACTOR_FROM_ROOT_SIZE and R_BRANCHING_FACTOR_DIRECT \
must be defined!
#define R_BRANCHING_FACTOR_CANDIDATE 3 // To silence further errors.
#endif

// The branching factor must be at least 3: the algorithms can't handle less.
#define R_BRANCHING_FACTOR_N \
   (R_BRANCHING_FACTOR_CANDIDATE < 3 ? 3 : R_BRANCHING_FACTOR_CANDIDATE)

_Static_assert(R_BRANCHING_FACTOR_N <= INT_FAST8_MAX,
               "rare platform: please bump R_IDX from int_fast8_t");

// An R-tree.
//
#ifdef R_T_ORDER_AT_SEARCH_TIME
// T-ordering is implemented by storing, for each box, a value corresponding to
// that box's global "insertion time", starting from zero for the first
// insertion. Queries can then act on these values as appropriate. Nonleaf
// nodes store the range of times included in each child node, to speed up
// querying.
#else
// T-ordering is implemented by keeping the invariant that for any box A that
// is T-above another box B, A is either in a lesser index in the same node as
// B, or, for an ancestor P of B, in a subtree rooted at P or rooted at a
// lesser index in the same node as P. Colloquially: A has to be "to the left"
// of B in the tree.
#endif
struct rtree {
   // The sign bit determines whether this is a leaf: nonleafs have negative
   // counts.
   R_IDX count;

   mushbounds bounds[R_BRANCHING_FACTOR_N];
   union {
      rtree   *branch_nodes[R_BRANCHING_FACTOR_N];
      mushaabb leaf_aabbs  [R_BRANCHING_FACTOR_N];
   };
#ifdef R_T_ORDER_AT_SEARCH_TIME
   union {
      r_time_range branch_times[R_BRANCHING_FACTOR_N];
      uint64_t     leaf_times  [R_BRANCHING_FACTOR_N];
   };
#endif
};

typedef struct mushboxen {
   // This is also the depth of all leaf nodes. The depth of the root is zero.
   R_DEPTH max_depth;
   size_t count;

#ifdef R_T_ORDER_AT_SEARCH_TIME
   uint64_t time;
#endif

   rtree root;
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
} mushboxen_iter_in;

#ifdef R_T_ORDER_AT_SEARCH_TIME
typedef struct {
   mushboxen_iter iter;
   const mushbounds *bounds;

   const rtree *interesting_root;
   R_DEPTH      interesting_depth;
   r_time_range time_range;
} mushboxen_iter_in_bottomup;
#else
typedef mushboxen_iter_in mushboxen_iter_in_bottomup;
#endif

typedef struct {
   mushboxen_iter iter;
   const mushbounds *bounds;
#ifdef R_T_ORDER_AT_SEARCH_TIME
   uint64_t sentinel_time;
#endif
} mushboxen_iter_above, mushboxen_iter_below;

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
