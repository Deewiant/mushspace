// File created: 2012-11-08 11:26:46

#include <alloca.h>
#include <assert.h>
#include <string.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

#ifdef R_T_ORDER_AT_SEARCH_TIME
// Helpers for working with r_time_ranges.

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

// Capitalized because they can be macros: see #else.
static void R_TIME_RANGE_EXPAND(r_time_range* a, r_time_range b) {
   a->min = MIN(a->min, b.min);
   a->max = MAX(a->max, b.max);
}
static void R_TIME_RANGE_EXPAND1(r_time_range* a, uint64_t t) {
   a->min = MIN(a->min, t);
   a->max = MAX(a->max, t);
}

typedef union r_ins_time {
   r_time_range range;
   uint64_t     time;
} r_ins_time;
#else
// Define out the functions to avoid having to #ifdef every call site.

#define R_TIME_RANGE_EXPAND(...)  while (0)
#define R_TIME_RANGE_EXPAND1(...) while (0)
#endif

typedef union r_elem {
   rtree    *branch_ptr;
   mushaabb *leaf_aabb_ptr;
} r_elem;

typedef struct r_insertee {
   R_DEPTH insert_depth;
   const mushbounds *bounds;
#ifdef R_T_ORDER_AT_SEARCH_TIME
   r_ins_time time;
#endif
   r_elem elem;
} r_insertee;

////////////////////////////////////////// Iterator aux

// We use the given void* as a pointer to a buffer holding the path to the
// current iterator position. That is, a node and an index for each level of
// the tree but the last.
//
// The index is the index we descended into, so the following holds for all
// valid x and x+1:
//
//    path_nodes[x+1] == path_nodes[x]->branch_nodes[path_idxs[x]]

#define AUX_SIZEOF (sizeof(rtree*) + sizeof(R_IDX))

static inline void iter_aux_split(
   const mushboxen* boxen,
   void *a, rtree ***path_nodes, R_IDX **path_idxs)
{
   *path_nodes = (rtree**)a;
   *path_idxs  = (R_IDX*)((char*)a + boxen->max_depth * sizeof(rtree*));
}
size_t mushboxen_iter_aux_size(const mushboxen* boxen) {
   return boxen->max_depth * AUX_SIZEOF;
}

const size_t mushboxen_iter_aux_size_init = 16 * AUX_SIZEOF;

////////////////////////////////////////// Boxen main

bool mushboxen_init(mushboxen* boxen) {
   *boxen = (mushboxen){
      .max_depth  = 0,
      .count      = 0,
      .root.count = 0,
   };
   return true;
}

static void r_free(rtree* node) {
   if (node->count >= 0)
      for (R_IDX i = 0; i < node->count; ++i)
         free(node->leaf_aabbs[i].data);
   else {
      for (R_IDX i = 0, count = -node->count; i < count; ++i) {
         r_free(node->branch_nodes[i]);
         free(node->branch_nodes[i]);
      }
   }
}
void mushboxen_free(mushboxen* boxen) {
   r_free(&boxen->root);
}

static bool r_copy(rtree* copy, const rtree* orig) {
   memcpy(copy, orig, sizeof *orig);
   if (copy->count >= 0) {
      for (R_IDX i = 0; i < copy->count; ++i) {
         mushaabb *box = &copy->leaf_aabbs[i];
         const mushcell *orig_data = box->data;
         box->data = malloc(box->size * sizeof *box->data);
         if (!box->data) {
            while (i--)
               free(copy->leaf_aabbs[i].data);
            return false;
         }
         memcpy(box->data, orig_data, box->size * sizeof *box->data);
      }
   } else {
      for (R_IDX i = 0, count = -copy->count; i < count; ++i) {
         copy->branch_nodes[i] = malloc(sizeof *copy->branch_nodes[i]);
         if (!copy->branch_nodes[i]
          || !r_copy(copy->branch_nodes[i], orig->branch_nodes[i]))
         {
            for (R_IDX j = 0; j <= i; ++j)
               free(copy->branch_nodes[j]);
            return false;
         }
      }
   }
   return true;
}
bool mushboxen_copy(mushboxen* copy, const mushboxen* boxen) {
   memcpy(copy, boxen, sizeof *boxen);
   return r_copy(&copy->root, &boxen->root);
}

size_t mushboxen_count(const mushboxen* boxen) { return boxen->count; }

#ifdef R_T_ORDER_AT_SEARCH_TIME
static mushaabb* r_get(const rtree* node, mushcoords pos, uint64_t* oldest) {
   const mushaabb *best = NULL;
   for (R_IDX i = 0, count = ABS(node->count); i < count; ++i) {
      if (!mushbounds_contains(&node->bounds[i], pos))
         continue;
      if (node->count >= 0) {
         if (node->leaf_times[i] < *oldest) {
            *oldest =  node->leaf_times[i];
            best    = &node->leaf_aabbs[i];
         }
         continue;
      }
      if (node->branch_times[i].min < *oldest) {
         mushaabb *aabb = r_get(node->branch_nodes[i], pos, oldest);
         if (aabb)
            best = aabb;
      }
   }
   return (mushaabb*)best;
}
#else
static mushaabb* r_get(const rtree* node, mushcoords pos) {
   for (R_IDX i = 0, count = ABS(node->count); i < count; ++i) {
      if (!mushbounds_contains(&node->bounds[i], pos))
         continue;
      if (node->count >= 0)
         return (mushaabb*)&node->leaf_aabbs[i];

      mushaabb *aabb = r_get(node->branch_nodes[i], pos);
      if (aabb)
         return aabb;
   }
   return NULL;
}
#endif
mushaabb* mushboxen_get(const mushboxen* boxen, mushcoords pos) {
#ifdef R_T_ORDER_AT_SEARCH_TIME
   uint64_t oldest = UINT64_MAX;
   return r_get(&boxen->root, pos, &oldest);
#else
   return r_get(&boxen->root, pos);
#endif
}

#ifdef R_T_ORDER_AT_SEARCH_TIME
static mushboxen_iter r_get_iter(
   const rtree* node, R_DEPTH depth, mushcoords pos, uint64_t* oldest,
   const mushboxen* boxen, void* aux)
{
   rtree **path_nodes;
   R_IDX  *path_idxs;
   iter_aux_split(boxen, aux, &path_nodes, &path_idxs);

   mushboxen_iter best = mushboxen_iter_null;

   for (R_IDX i = 0, count = ABS(node->count); i < count; ++i) {
      if (!mushbounds_contains(&node->bounds[i], pos))
         continue;
      if (node->count >= 0) {
         if (node->leaf_times[i] < *oldest) {
            *oldest = node->leaf_times[i];
            best    = (mushboxen_iter){ (rtree*)node, i, depth, aux };
         }
         continue;
      }
      if (node->branch_times[i].min < *oldest) {
         mushboxen_iter iter = r_get_iter(node->branch_nodes[i], depth + 1,
                                          pos, oldest, boxen, aux);
         if (!mushboxen_iter_is_null(iter)) {
            path_nodes[depth] = (rtree*)node;
            path_idxs [depth] = i;
            best = iter;
         }
      }
   }
   return best;
}
#else
static mushboxen_iter r_get_iter(
   const rtree* node, R_DEPTH depth, mushcoords pos,
   const mushboxen* boxen, void* aux)
{
   rtree **path_nodes;
   R_IDX  *path_idxs;
   iter_aux_split(boxen, aux, &path_nodes, &path_idxs);

   for (R_IDX i = 0, count = ABS(node->count); i < count; ++i) {
      if (!mushbounds_contains(&node->bounds[i], pos))
         continue;
      if (node->count >= 0)
         return (mushboxen_iter){ (rtree*)node, i, depth, aux };

      mushboxen_iter iter = r_get_iter(node->branch_nodes[i], depth + 1,
                                       pos, boxen, aux);
      if (!mushboxen_iter_is_null(iter)) {
         path_nodes[depth] = (rtree*)node;
         path_idxs [depth] = i;
         return iter;
      }
   }
   return mushboxen_iter_null;
}
#endif
mushboxen_iter mushboxen_get_iter(
   const mushboxen* boxen, mushcoords pos, void* aux)
{
#ifdef R_T_ORDER_AT_SEARCH_TIME
   uint64_t oldest = UINT64_MAX;
   return r_get_iter(&boxen->root, 0, pos, &oldest, boxen, aux);
#else
   return r_get_iter(&boxen->root, 0, pos, boxen, aux);
#endif
}

static bool r_contains_bounds(const rtree* node, const mushbounds* bs) {
   for (R_IDX i = 0, count = ABS(node->count); i < count; ++i) {
      if (!mushbounds_contains_bounds(&node->bounds[i], bs))
         continue;
      if (node->count >= 0 || r_contains_bounds(node->branch_nodes[i], bs))
         return true;
   }
   return false;
}
bool mushboxen_contains_bounds(const mushboxen* boxen, const mushbounds* bs) {
   return r_contains_bounds(&boxen->root, bs);
}

void mushboxen_loosen_bounds(const mushboxen* boxen, mushbounds* bounds) {
   for (R_IDX i = 0; i < ABS(boxen->root.count); ++i)
      mushbounds_expand_to_cover(bounds, &boxen->root.bounds[i]);
}

static void r_flatten_node(bool is_leaf, rtree* node, R_IDX count) {
   for (R_IDX i = 0, j = 0, remaining = count; remaining; ++i) {
      if (is_leaf ? !node->leaf_aabbs[i].data : !node->branch_nodes[i])
         continue;

      while (is_leaf ? !!node->leaf_aabbs[j].data : !!node->branch_nodes[j])
         if (++j == count)
            return;

      if (i >= j) {
         // i points to a nonnull box while j points to a null slot earlier on.
         // Move i to j.
         node->bounds[j] = node->bounds[i];
         if (is_leaf) {
            node->leaf_aabbs[j] = node->leaf_aabbs[i];
            node->leaf_aabbs[i].data = NULL;
#ifdef R_T_ORDER_AT_SEARCH_TIME
            node->leaf_times[j] = node->leaf_times[i];
#endif
         } else {
            node->branch_nodes[j] = node->branch_nodes[i];
            node->branch_nodes[i] = NULL;
#ifdef R_T_ORDER_AT_SEARCH_TIME
            node->branch_times[j] = node->branch_times[i];
#endif
         }
      }
      --remaining;
   }
}

// The new node will be T-below the old node.
static mushboxen_iter r_split_node(
   rtree* old, rtree* new, mushbounds* pold_cover, mushbounds* pnew_cover,
#ifdef R_T_ORDER_AT_SEARCH_TIME
   r_time_range* old_time_range, r_time_range* new_time_range,
#endif
   const mushbounds* elem_bounds, r_elem elem
#ifdef R_T_ORDER_AT_SEARCH_TIME
 , r_ins_time elem_time
#endif
) {
   assert (ABS(old->count) == R_BRANCHING_FACTOR);

   const bool is_leaf = old->count >= 0;

#ifdef R_SPLIT_QUADRATIC
   // The classic quadratic algorithm from Guttman 1984: For each pair of
   // bounds, see how much "wasted" space remains in a box that encompasses
   // them both. Split the pair that wastes the most space into separate nodes.
   //
#ifndef R_T_ORDER_AT_SEARCH_TIME
   // And our addition: make sure to preserve T-ordering. The way we do it
   // makes this actually cubic.

   // The transitive closure of T-aboveness. We need this because of the
   // following kind of situation:
   //
   // +----+
   // | A  |   +---+
   // |   +----|  +---+
   // +---| B  |C | D |
   //     +----|  +---+
   //          +---+
   //
   // Here A is T-below B which is T-below C which is T-below D.
   //
   // For example, if we put C in the new node, that directly also forces B
   // into the new node. But forcing B into the new node indirectly forces A
   // into the new node as well. We won't know that unless we look at
   // transitive T-aboveness from C.
   //
   // Putting C into the new node forces D to be either left of C in the new
   // node or anywhere in the old node. For this reason we preserve element
   // order in assignments: we do new[i] = old[i] instead of something like
   // new[n++] = old[i], and then flatten things at the end.
   //
   // Symmetrically, putting B into the old node, forces C and transitively D
   // into the old node. A is forced to be either right of B in the old node or
   // anywhere in the new node.
   //
   // tabove[i][j / TABOVE_BITS] & 1 << j % TABOVE_BITS) will be true if old[i]
   // is T-above old[j], or if old[i] is T-above some old[k] (...) which is
   // T-above old[j].
   //
   // We require one extra bit in TABOVE_BITS_LEN because we want to know which
   // boxes are T-above elem as well.
   const ptrdiff_t
      TABOVE_BITS      = CHAR_BIT * sizeof(uint_fast32_t),
      TABOVE_BITS_LEN = (R_BRANCHING_FACTOR+1 + TABOVE_BITS - 1) / TABOVE_BITS;

   uint_fast32_t tabove[R_BRANCHING_FACTOR][TABOVE_BITS_LEN];

   static const uint_fast32_t TABOVE_ONE = 1;

   #define IS_TABOVE(I, J) \
      (tabove[I][J / TABOVE_BITS] & TABOVE_ONE << J % TABOVE_BITS)

   #define SET_TABOVE(I, J) \
      (tabove[I][J / TABOVE_BITS] |= TABOVE_ONE << J % TABOVE_BITS)

   // Zero init.
   for (R_IDX i = 0; i < R_BRANCHING_FACTOR + 1; ++i)
      for (R_IDX j = 0; j < TABOVE_BITS_LEN; ++j)
         tabove[i][j] = 0;

   // Direct T-aboveness.
   for (R_IDX i = 0; i < R_BRANCHING_FACTOR; ++i) {
      // For convenience, we say that each box is T-above itself. This allows
      // NEXT_UNASS to avoid a comparison.
      SET_TABOVE(i, i);

      for (R_IDX j = i + 1; j < R_BRANCHING_FACTOR; ++j)
         if (mushbounds_overlaps(&old->bounds[i], &old->bounds[j]))
            SET_TABOVE(i, j);

      if (mushbounds_overlaps(&old->bounds[i], elem_bounds))
         SET_TABOVE(i, R_BRANCHING_FACTOR);
   }

   // The transitive closure.
   for (R_IDX i = 0; i < R_BRANCHING_FACTOR; ++i)
      for (R_IDX j = i + 1; j < R_BRANCHING_FACTOR; ++j)
         if (IS_TABOVE(i, j))
            for (R_IDX k = j + 1; k < R_BRANCHING_FACTOR + 1; ++k)
               tabove[i][k / TABOVE_BITS] |=
                  tabove[j][k / TABOVE_BITS] & TABOVE_ONE << k % TABOVE_BITS;
#endif

   // A value of R_BRANCHING_FACTOR refers to elem.
   //
   // This solution definitely works, since the number of boxes transitively
   // T-below max_waste_j is bounded from above by the number of valid indices
   // after max_waste_j plus one. (See the inequality in the loop below.) As
   // can be shown:
   //
   //    R_BRANCHING_FACTOR + 1 - #(T-below max_waste_j)
   // >= R_BRANCHING_FACTOR + 1 - (#(valid indices after max_waste_j]) + 1)
   // == R_BRANCHING_FACTOR + 1 - ((R_BRANCHING_FACTOR - 1 - max_waste_j) + 1)
   // == R_BRANCHING_FACTOR + 1 - (R_BRANCHING_FACTOR - max_waste_j)
   // == 1 + max_waste_j
   //
   // We want this to be >= R_MIN_XS_PER_NODE and different from max_waste_i,
   // so set max_waste_j = R_MIN_XS_PER_NODE.
   size_t max_waste = 0;
   R_IDX max_waste_i = 0,
         max_waste_j = R_MIN_XS_PER_NODE;

   for (R_IDX i = 0; i < R_BRANCHING_FACTOR; ++i) {
      const mushbounds *bounds_i = &old->bounds[i];
      const size_t size_i = is_leaf ? old->leaf_aabbs[i].size
                                    : mushbounds_size(bounds_i);

#ifndef R_T_ORDER_AT_SEARCH_TIME
      bool i_tabove_checked = false;
#endif

      for (R_IDX j = i + 1; j < R_BRANCHING_FACTOR; ++j) {
         const mushbounds *bounds_j = &old->bounds[j];
         const size_t size_j = is_leaf ? old->leaf_aabbs[j].size
                                       : mushbounds_size(bounds_j);

         mushbounds bounds = *bounds_i;
         mushbounds_expand_to_cover(&bounds, bounds_j);

         const size_t together = mushbounds_clamped_size(&bounds),
                      waste    = together - size_i - size_j;
         if (waste <= max_waste)
            continue;

#ifndef R_T_ORDER_AT_SEARCH_TIME
         // While this (putting i in old and j in new) is a better solution, it
         // may violate the R_MIN_XS_PER_NODE invariant when we take T-ordering
         // into consideration.
         //
         // This forces all boxes T-above i into the old node and all boxes
         // T-below j into the new node. So make sure that the following holds:
         //
         //    R_BRANCHING_FACTOR + 1 - #(T-below j) - 1 >= R_MIN_XS_PER_NODE
         //
         // I.e. that at least R_MIN_XS_PER_NODE are guaranteed to be available
         // for the old node. The +1 is due to the elem we're placing and the
         // -1 is due to j itself. They cancel each other out, leaving us with:
         //
         //    R_BRANCHING_FACTOR - #(T-below j) >= R_MIN_XS_PER_NODE
         //
         // Similarly, at least R_MIN_XS_PER_NODE need to be available for the
         // new node:
         //
         //    R_BRANCHING_FACTOR - #(T-above i) >= R_MIN_XS_PER_NODE
         //
         // To simplify the check, we transform it a bit. We want to make sure
         // the following never becomes true:
         //
         //    R_BRANCHING_FACTOR - R_MIN_XS_PER_NODE - #(T-related) < 0
         if (!i_tabove_checked) {
            R_IDX check = R_BRANCHING_FACTOR - R_MIN_XS_PER_NODE;
            for (R_IDX k = 0; k < i; ++k)
               if (IS_TABOVE(k, i) && !check--)
                   goto next_i;
            i_tabove_checked = true;
         }

         // There are R_BRANCHING_FACTOR - j - 1 possible boxes T-below j, plus
         // one for elem.
         //
         // In other words:
         //
         //    #(T-below j) <= R_BRANCHING_FACTOR - j
         //
         // Meanwhile, we want to ensure:
         //
         //    #(T-below j) <= R_BRANCHING_FACTOR - R_MIN_XS_PER_NODE
         //
         // So if the following holds, this is guaranteed:
         //
         //    R_BRANCHING_FACTOR - j <= R_BRANCHING_FACTOR - R_MIN_XS_PER_NODE
         //
         // I.e.:
         //
         //    - j <= - R_MIN_XS_PER_NODE
         //      j >=   R_MIN_XS_PER_NODE
         if (j < R_MIN_XS_PER_NODE) {
            R_IDX check = R_BRANCHING_FACTOR - R_MIN_XS_PER_NODE;
            for (R_IDX k = j + 1; k <= R_BRANCHING_FACTOR; ++k)
               if (IS_TABOVE(j, k) && !check--)
                  goto next_j;
         }
#endif
         max_waste   = waste;
         max_waste_i = i;
         max_waste_j = j;
#ifndef R_T_ORDER_AT_SEARCH_TIME
next_j:;
#endif
      }

      // elem also has a bound to be included. No boxes are T-below it, so we
      // don't need to worry about that.
      mushbounds bounds = *bounds_i;
      mushbounds_expand_to_cover(&bounds, elem_bounds);
      const size_t waste = mushbounds_clamped_size(&bounds)
                         - size_i
                         - (is_leaf ? elem.leaf_aabb_ptr->size
                                    : mushbounds_size(elem_bounds));
      if (waste <= max_waste)
         continue;
#ifndef R_T_ORDER_AT_SEARCH_TIME
      if (!i_tabove_checked) {
         R_IDX check = R_BRANCHING_FACTOR - R_MIN_XS_PER_NODE;
         for (R_IDX k = 0; k < i; ++k)
            if (IS_TABOVE(k, i) && !check--)
                goto next_i;
      }
#endif
      max_waste   = waste;
      max_waste_i = i;
      max_waste_j = R_BRANCHING_FACTOR;
#ifndef R_T_ORDER_AT_SEARCH_TIME
next_i:;
#endif
   }

#ifdef R_T_ORDER_AT_SEARCH_TIME
   // Keep track of assigned boxes with a bit field.
   const ptrdiff_t
      ASSIGNED_BITS = CHAR_BIT * sizeof(uint_fast32_t),
      ASSIGNED_LEN  =
         (R_BRANCHING_FACTOR+1 + ASSIGNED_BITS - 1) / ASSIGNED_BITS;
   uint_fast32_t assigned[ASSIGNED_LEN];
   static const uint_fast32_t ASSIGNED_ONE = 1;
   #define IS_ASSIGNED(I) \
      (assigned[I / ASSIGNED_BITS] & ASSIGNED_ONE << I % ASSIGNED_BITS)
   #define SET_ASSIGNED(I) \
      (assigned[I / ASSIGNED_BITS] |= ASSIGNED_ONE << I % ASSIGNED_BITS)

   for (R_IDX i = 0; i < ASSIGNED_LEN; ++i)
      assigned[i] = 0;

   #define SET_ELEM_ASSIGNED SET_ASSIGNED(R_BRANCHING_FACTOR)
   #define IS_ELEM_ASSIGNED  IS_ASSIGNED (R_BRANCHING_FACTOR)

   // A convenience since we can't put #ifdefs in #defines.
   #define IF_R_T_ORDER_AT_SEARCH_TIME(X) (X)
#else
   // We will subvert T-aboveness: if something is marked as T-above
   // max_waste_i, it's something that has been assigned to the old node.
   // Assignments to the new node are marked by nullifying the corresponding
   // element in old (which has to be done anyway).
   #define SET_ASSIGNED(I) SET_TABOVE(I, max_waste_i)

   bool assigned_elem = false;

   #define SET_ELEM_ASSIGNED (assigned_elem = true)
   #define IS_ELEM_ASSIGNED  (assigned_elem)

   #define IF_R_T_ORDER_AT_SEARCH_TIME(X)
#endif

   bool elem_in_old;

   // We'll be doing a lot of assignments to both nodes: reduce repetition with
   // these macros.
   //
   // We won't use them in every case, though, since we can sometimes get by
   // with doing less.

   #define ASSIGN_TO_NEW(I) do { \
      SET_ASSIGNED(I); \
      new->bounds[I] = old->bounds[I]; \
      mushbounds_expand_to_cover(&new_cover, &new->bounds[I]); \
      if (is_leaf) { \
         new->leaf_aabbs[I] = old->leaf_aabbs[I]; \
         old->leaf_aabbs[I].data = NULL; \
         IF_R_T_ORDER_AT_SEARCH_TIME( \
            new->leaf_times[I] = old->leaf_times[I]); \
         R_TIME_RANGE_EXPAND1(new_time_range, new->leaf_times[I]); \
      } else { \
         new->branch_nodes[I] = old->branch_nodes[I]; \
         old->branch_nodes[I] = NULL; \
         IF_R_T_ORDER_AT_SEARCH_TIME( \
            new->branch_times[I] = old->branch_times[I]); \
         R_TIME_RANGE_EXPAND(new_time_range, new->branch_times[I]); \
      } \
      ++new->count; \
   } while (0)

   #define ASSIGN_TO_OLD(I) do { \
      SET_ASSIGNED(I); \
      mushbounds_expand_to_cover(&old_cover, &old->bounds[I]); \
      if (is_leaf) { \
         new->leaf_aabbs[I].data = NULL; \
         R_TIME_RANGE_EXPAND1(old_time_range, old->leaf_times[I]); \
      } else { \
         new->branch_nodes[I] = NULL; \
         R_TIME_RANGE_EXPAND(old_time_range, old->branch_times[I]); \
      } \
      ++old->count; \
   } while (0)

   // Note that elem is not written anywhere in either of these macros: since
   // it must come last, we only actually write it at the very end.
   #define ASSIGN_ELEM_TO_NEW do { \
      SET_ELEM_ASSIGNED; \
      elem_in_old = false; \
      mushbounds_expand_to_cover(&new_cover, elem_bounds); \
      if (is_leaf) R_TIME_RANGE_EXPAND1(new_time_range, elem_time.time); \
      else         R_TIME_RANGE_EXPAND (new_time_range, elem_time.range); \
      ++new->count; \
   } while (0)

   #define ASSIGN_ELEM_TO_OLD do { \
      SET_ELEM_ASSIGNED; \
      elem_in_old = true; \
      mushbounds_expand_to_cover(&old_cover, elem_bounds); \
      if (is_leaf) R_TIME_RANGE_EXPAND1(old_time_range, elem_time.time); \
      else         R_TIME_RANGE_EXPAND (old_time_range, elem_time.range); \
      ++old->count; \
   } while (0)

   // Assign max_waste_j to the new node.
#ifdef R_T_ORDER_AT_SEARCH_TIME
   SET_ASSIGNED(max_waste_j);
#endif
   new->count = 1;
   mushbounds new_cover;
   if (max_waste_j == R_BRANCHING_FACTOR) {
      new_cover = *elem_bounds;
      elem_in_old = false;

#ifdef R_T_ORDER_AT_SEARCH_TIME
      *new_time_range =
         is_leaf ? (r_time_range){ elem_time.time, elem_time.time }
                 : elem_time.range;
#else
      assigned_elem = true;
#endif
   } else {
      new_cover = new->bounds[max_waste_j] = old->bounds[max_waste_j];

      if (is_leaf) {
         new->leaf_aabbs[max_waste_j] = old->leaf_aabbs[max_waste_j];
         old->leaf_aabbs[max_waste_j].data = NULL;
#ifdef R_T_ORDER_AT_SEARCH_TIME
         new->leaf_times[max_waste_j] = old->leaf_times[max_waste_j];
#endif
      } else {
         new->branch_nodes[max_waste_j] = old->branch_nodes[max_waste_j];
         old->branch_nodes[max_waste_j] = NULL;
#ifdef R_T_ORDER_AT_SEARCH_TIME
         new->branch_times[max_waste_j] = old->branch_times[max_waste_j];
#endif
      }

#ifdef R_T_ORDER_AT_SEARCH_TIME
      *new_time_range =
         is_leaf ? (r_time_range){ new->leaf_times[max_waste_j],
                                   new->leaf_times[max_waste_j] }
                 : new->branch_times[max_waste_j];
#else
      // Assign all boxes transitively T-below max_waste_j to the new node as
      // well.
      for (R_IDX i = max_waste_j + 1; i < R_BRANCHING_FACTOR; ++i)
         if (IS_TABOVE(max_waste_j, i))
            ASSIGN_TO_NEW(i);
      if (IS_TABOVE(max_waste_j, R_BRANCHING_FACTOR))
         ASSIGN_ELEM_TO_NEW;
#endif
   }

   // Assign max_waste_i to the old node.
#ifdef R_T_ORDER_AT_SEARCH_TIME
   SET_ASSIGNED(max_waste_i);
#endif
   old->count = 1;
   mushbounds old_cover;
   if (max_waste_i == R_BRANCHING_FACTOR) {
      old_cover = *elem_bounds;
      elem_in_old = true;
#ifdef R_T_ORDER_AT_SEARCH_TIME
      *old_time_range =
         is_leaf ? (r_time_range){ elem_time.time, elem_time.time }
                 : elem_time.range;
#else
      assigned_elem = true;
#endif
   } else {
      old_cover = old->bounds[max_waste_i];
      if (is_leaf)
         new->leaf_aabbs[max_waste_i].data = NULL;
      else
         new->branch_nodes[max_waste_i] = NULL;

#ifdef R_T_ORDER_AT_SEARCH_TIME
      *old_time_range =
         is_leaf ? (r_time_range){ old->leaf_times[max_waste_i],
                                   old->leaf_times[max_waste_i] }
                 : old->branch_times[max_waste_i];
#endif
   }

#ifndef R_T_ORDER_AT_SEARCH_TIME
   // Assign all boxes transitively T-above max_waste_i as well.
   for (R_IDX i = 0; i < max_waste_i; ++i)
      if (IS_TABOVE(i, max_waste_i))
         ASSIGN_TO_OLD(i);
#endif

   size_t old_cover_size = mushbounds_clamped_size(&old_cover),
          new_cover_size = mushbounds_clamped_size(&new_cover);

   for (R_IDX first_unass = 0, remaining;
        (remaining = R_BRANCHING_FACTOR - new->count - old->count + 1);)
   {
#ifdef R_T_ORDER_AT_SEARCH_TIME
      #define NEXT_UNASS(i) while (i<R_BRANCHING_FACTOR && IS_ASSIGNED(i)) ++i;
#else
      // Moves the given index to an unassigned box.
      //
      // Assigned boxes are null in old (assigned to new) or T-above
      // max_waste_i (assigned to old).
      #define NEXT_UNASS(i) \
         while (i < R_BRANCHING_FACTOR \
             && ((is_leaf ? !old->leaf_aabbs[i].data : !old->branch_nodes[i]) \
              || IS_TABOVE(i, max_waste_i))) \
            ++i;
#endif

      NEXT_UNASS(first_unass);

      assert ((first_unass < R_BRANCHING_FACTOR || !IS_ELEM_ASSIGNED)
           && "need to assign more, but nothing left");

      if (old->count + remaining == R_MIN_XS_PER_NODE) {
         // Assign the rest to old.
         if (!IS_ELEM_ASSIGNED)
            ASSIGN_ELEM_TO_OLD;
         for (R_IDX i = first_unass; i < R_BRANCHING_FACTOR;) {
            ASSIGN_TO_OLD(i);
            ++i;
            NEXT_UNASS(i);
         }
         break;
      }

      if (new->count + remaining == R_MIN_XS_PER_NODE) {
         // Assign the rest to new.
         if (!IS_ELEM_ASSIGNED)
            ASSIGN_ELEM_TO_NEW;
         for (R_IDX i = first_unass; i < R_BRANCHING_FACTOR;) {
            ASSIGN_TO_NEW(i);
            ++i;
            NEXT_UNASS(i);
         }
         break;
      }

      // Select the next box to place: take the one with the highest preference
      // for either node, i.e. with the highest difference between how much it
      // increases the size of each node's covering bounds. Place it in the
      // node whose covering bounds will be enlarged the least.
      //
#ifndef R_T_ORDER_AT_SEARCH_TIME
      // That is, if T-ordering is not taken into account. Since T-ordering
      // forces some placements, if placing a box in one node would force the
      // other one's count below R_MIN_XS_PER_NODE, we have no choice but to
      // put that box in the other node.
#endif

      size_t     picked_ddiff = 0;
      R_IDX      picked_idx;
      bool       picked_old;
      mushbounds picked_cover;
      size_t     picked_cover_size;

      #define TRY_PICK_NEXT(BOUNDS, IDX) do { \
         mushbounds old_cover2 = old_cover, \
                    new_cover2 = new_cover; \
         mushbounds_expand_to_cover(&old_cover2, BOUNDS); \
         mushbounds_expand_to_cover(&new_cover2, BOUNDS); \
         const size_t old_cover2_size = mushbounds_clamped_size(&old_cover2), \
                      new_cover2_size = mushbounds_clamped_size(&new_cover2), \
                      old_diff        = old_cover2_size - old_cover_size, \
                      new_diff        = new_cover2_size - new_cover_size, \
                      ddiff = ABS((ptrdiff_t)new_diff - (ptrdiff_t)old_diff); \
         \
         if (ddiff > picked_ddiff || !picked_ddiff) { \
            picked_ddiff      = ddiff; \
            picked_idx        = IDX; \
            picked_old        = old_diff <= new_diff; \
            picked_cover      = picked_old ? old_cover2 : new_cover2; \
            picked_cover_size = \
               picked_old ? old_cover2_size : new_cover2_size; \
         } \
      } while (0)

      for (R_IDX i = first_unass; i < R_BRANCHING_FACTOR;) {
#ifndef R_T_ORDER_AT_SEARCH_TIME
         R_IDX tabove_forced = 0,
               tbelow_forced = 0;
         for (R_IDX j = first_unass; j < R_BRANCHING_FACTOR;) {
            if (IS_TABOVE(j, i))
               ++tabove_forced;
            if (IS_TABOVE(i, j))
               ++tbelow_forced;
            ++j;
            NEXT_UNASS(j);
         }
         if (!assigned_elem && IS_TABOVE(i, R_BRANCHING_FACTOR))
            ++tbelow_forced;

         // If the number of unassigned boxes transitively T-above this one
         // would force new->count too low, this must be placed in new.
         if (new->count + remaining - tabove_forced - 1 < R_MIN_XS_PER_NODE) {
            picked_idx        = i;
            picked_cover      = new_cover;
            picked_cover_size = 0;
            mushbounds_expand_to_cover(&picked_cover, &old->bounds[i]);
            goto forced_new;
         }

         // And similarly, if the number of unassigned boxes transitively
         // T-below this one would force old->count too low, this must be
         // placed in old.
         if (old->count + remaining - tbelow_forced - 1 < R_MIN_XS_PER_NODE) {
            // Be sneaky and only write those variables which are used in the
            // picked_old case.
            picked_idx        = i;
            picked_cover      = old_cover;
            picked_cover_size = 0;
            mushbounds_expand_to_cover(&picked_cover, &old->bounds[i]);
            goto forced_old;
         }
#endif
         TRY_PICK_NEXT(&old->bounds[i], i);
         ++i;
         NEXT_UNASS(i);
      }
      if (!IS_ELEM_ASSIGNED) {
#ifndef R_T_ORDER_AT_SEARCH_TIME
         // elem can only force things that are T-above it.
         R_IDX tabove_forced = 0;
         for (R_IDX j = first_unass; j < R_BRANCHING_FACTOR;) {
            if (IS_TABOVE(j, R_BRANCHING_FACTOR))
               ++tabove_forced;
            ++j;
            NEXT_UNASS(j);
         }

         if (new->count + remaining - tabove_forced - 1 < R_MIN_XS_PER_NODE) {
            picked_idx        = R_BRANCHING_FACTOR;
            picked_cover      = new_cover;
            picked_cover_size = 0;
            mushbounds_expand_to_cover(&picked_cover, elem_bounds);
            goto forced_elem_new;
         }
#endif
         TRY_PICK_NEXT(elem_bounds, R_BRANCHING_FACTOR);
      }

      if (picked_old) {
         if (picked_idx == R_BRANCHING_FACTOR)
            ASSIGN_ELEM_TO_OLD;
         else {
#ifndef R_T_ORDER_AT_SEARCH_TIME
forced_old:
#endif
            ASSIGN_TO_OLD(picked_idx);
         }
         old_cover = picked_cover;

#ifdef R_T_ORDER_AT_SEARCH_TIME
         old_cover_size = picked_cover_size;
#else
         // Putting this one in old also means that all boxes transitively
         // T-above it must stay in old.
         for (R_IDX i = first_unass; i < picked_idx;) {
            if (IS_TABOVE(i, picked_idx)) {
               ASSIGN_TO_OLD(i);

               // Unset this so that we know we have to recompute it below.
               picked_cover_size = 0;
            }
            ++i;
            NEXT_UNASS(i);
         }
         old_cover_size =
            picked_cover_size ? picked_cover_size
                              : mushbounds_clamped_size(&old_cover);
#endif
      } else {
         if (picked_idx == R_BRANCHING_FACTOR) {
#ifndef R_T_ORDER_AT_SEARCH_TIME
forced_elem_new:
#endif
            ASSIGN_ELEM_TO_NEW;
         } else {
#ifndef R_T_ORDER_AT_SEARCH_TIME
forced_new:
#endif
            ASSIGN_TO_NEW(picked_idx);
         }
         new_cover = picked_cover;

#ifdef R_T_ORDER_AT_SEARCH_TIME
         new_cover_size = picked_cover_size;
#else
         // Putting this one in new also means that all boxes transitively
         // T-below it must be put in new.
         R_IDX i = picked_idx + 1;
         NEXT_UNASS(i);
         while (i < R_BRANCHING_FACTOR) {
            if (IS_TABOVE(picked_idx, i)) {
               ASSIGN_TO_NEW(i);
               picked_cover_size = 0;
            }
            ++i;
            NEXT_UNASS(i);
         }
         if (!IS_ELEM_ASSIGNED && IS_TABOVE(picked_idx, R_BRANCHING_FACTOR)) {
            ASSIGN_ELEM_TO_NEW;
            picked_cover_size = 0;
         }
         new_cover_size =
            picked_cover_size ? picked_cover_size
                              : mushbounds_clamped_size(&new_cover);
#endif
      }
      #undef NEXT_UNASS
      #undef TRY_PICK_NEXT
   }

#if defined(R_T_ORDER_AT_SEARCH_TIME) \
 && defined(MUSH_ENABLE_EXPENSIVE_DEBUGGING)
   for (R_IDX i = 0; i <= R_BRANCHING_FACTOR; ++i)
      assert (IS_ASSIGNED(i));
#endif
   assert (IS_ELEM_ASSIGNED);
   assert (old->count - 1 + new->count == R_BRANCHING_FACTOR);
   assert (old->count >= R_MIN_XS_PER_NODE);
   assert (new->count >= R_MIN_XS_PER_NODE);

   *pold_cover = old_cover;
   *pnew_cover = new_cover;

   // Both nodes still have nullified entries interspersed among their real
   // entries, so move everything in them to the beginning.
   r_flatten_node(is_leaf, old, old->count -  elem_in_old);
   r_flatten_node(is_leaf, new, new->count - !elem_in_old);

   // Place elem wherever it belongs. Don't increment the node's count: that's
   // already been done.

   mushboxen_iter iter;

   iter.node = elem_in_old ? old : new;
   iter.idx  = iter.node->count - 1;
   iter.node->bounds[iter.idx] = *elem_bounds;
   if (is_leaf) {
      iter.node->leaf_aabbs[iter.idx] = *elem.leaf_aabb_ptr;
#ifdef R_T_ORDER_AT_SEARCH_TIME
      iter.node->leaf_times[iter.idx] = elem_time.time;
#endif
   } else {
      iter.node->branch_nodes[iter.idx] = elem.branch_ptr;
#ifdef R_T_ORDER_AT_SEARCH_TIME
      iter.node->branch_times[iter.idx] = elem_time.range;
#endif
   }

#ifdef MUSH_ENABLE_EXPENSIVE_DEBUGGING
   for (R_IDX i = 0; i < old->count; ++i) {
      assert (mushbounds_contains_bounds(pold_cover, &old->bounds[i]));
      if (is_leaf) assert (old->leaf_aabbs  [i].data != NULL);
      else         assert (old->branch_nodes[i]      != NULL);
   }
   for (R_IDX i = 0; i < new->count; ++i) {
      assert (mushbounds_contains_bounds(pnew_cover, &new->bounds[i]));
      if (is_leaf) assert (new->leaf_aabbs  [i].data != NULL);
      else         assert (new->branch_nodes[i]      != NULL);
   }
#endif

   if (!is_leaf) {
      old->count *= -1;
      new->count *= -1;
   }
   return iter;

#ifdef R_T_ORDER_AT_SEARCH_TIME
   #undef IS_ASSIGNED
#else
   #undef IS_TABOVE
   #undef SET_TABOVE
#endif
   #undef IF_R_T_ORDER_AT_SEARCH_TIME
   #undef SET_ASSIGNED
   #undef SET_ELEM_ASSIGNED
   #undef IS_ELEM_ASSIGNED
   #undef ASSIGN_TO_NEW
   #undef ASSIGN_TO_OLD
   #undef ASSIGN_ELEM_TO_NEW
   #undef ASSIGN_ELEM_TO_OLD

#else
#error Only the quadratic split algorithm is implemented!
#endif
}

// FIXME: handle malloc failure, gracefully
static mushboxen_iter r_insert(
   r_insertee insertee, rtree *node, rtree **path_nodes, R_IDX *path_idxs,
   mushbounds *node_cover,
#ifdef R_T_ORDER_AT_SEARCH_TIME
   r_time_range *node_time_range,
#endif
   rtree** split, mushbounds *split_cover
#ifdef R_T_ORDER_AT_SEARCH_TIME
 , r_time_range *split_time_range
#endif
) {
   r_elem ins_here;
   const mushbounds *ins_here_bounds;
   mushbounds ins_here_bounds_st;
#ifdef R_T_ORDER_AT_SEARCH_TIME
   r_ins_time ins_here_time;
#endif

   R_IDX child_idx;

   mushboxen_iter iter;

   R_IDX abs_count = ABS(node->count);

   if (insertee.insert_depth == 0) {
      ins_here        = insertee.elem;
      ins_here_bounds = insertee.bounds;
#ifdef R_T_ORDER_AT_SEARCH_TIME
      ins_here_time   = insertee.time;
#endif
   } else {
      // Select the child to recurse into: the one whose bounds are already
      // closest to insertee.bounds, resolving ties by size.
      //
#ifndef R_T_ORDER_AT_SEARCH_TIME
      // But if any child overlaps with insertee.bounds, to preserve
      // T-ordering, the "rightmost" overlapping child is selected.
#endif

      size_t best_size = SIZE_MAX,
             best_diff = SIZE_MAX;

      for (R_IDX i = abs_count; i--;) {
         mushbounds bounds = node->bounds[i];
#ifndef R_T_ORDER_AT_SEARCH_TIME
         if (mushbounds_overlaps(insertee.bounds, &bounds)) {
            child_idx = i;
            break;
         }
#endif
         const size_t bounds_size = mushbounds_size(&bounds);

         mushbounds_expand_to_cover(&bounds, insertee.bounds);
         const size_t exp_size = mushbounds_clamped_size(&bounds);

         const size_t diff = exp_size - bounds_size;
         if (diff < best_diff || (diff == best_diff && exp_size < best_size)) {
            best_diff = diff;
            best_size = exp_size;
            child_idx = i;
         }
      }

      --insertee.insert_depth;

      iter = r_insert(insertee, node->branch_nodes[child_idx],
                      path_nodes + 1, path_idxs + 1,
                      &node->bounds[child_idx],
#ifdef R_T_ORDER_AT_SEARCH_TIME
                      &node->branch_times[child_idx],
#endif
                      &ins_here.branch_ptr, &ins_here_bounds_st
#ifdef R_T_ORDER_AT_SEARCH_TIME
                    , &ins_here_time.range
#endif
      );
      if (mushboxen_iter_is_null(iter))
         return iter;

      ++iter.path_depth;

      // Write the relevant auxiliary info. Since we move the pointers on every
      // recursive call, no indexing is needed.
      *path_nodes = node;
      *path_idxs  = child_idx;

      // Reset to the insert depth of this call, since we still use it below.
      ++insertee.insert_depth;

      if (!ins_here.branch_ptr) {
         // No split happened below: we're done.
         goto no_split_done;
      }
      ins_here_bounds = &ins_here_bounds_st;
   }

   if (abs_count == R_BRANCHING_FACTOR) {
      // Node is full: need to split.

      *split = malloc(sizeof **split);
      if (!*split)
         return mushboxen_iter_null;

#ifndef R_T_ORDER_AT_SEARCH_TIME
      R_IDX i = child_idx + 1;
      if (ins_here_bounds == &ins_here_bounds_st && i != R_BRANCHING_FACTOR) {
         // We're placing the result of a split of a non-last node further
         // down. In order to preserve T-ordering, we have to place it
         // immediately after the node from which it was split.
         //
         // Turning ins_here into the current last element is a bit of a hack;
         // telling r_split_node where the given new element belongs seems more
         // sensible. But this might also be the simplest solution.

         mushbounds last_bounds = node->bounds      [R_BRANCHING_FACTOR-1];
         rtree     *last        = node->branch_nodes[R_BRANCHING_FACTOR-1];

         mushbounds  *arr0 = node->bounds;
         rtree      **arr1 = node->branch_nodes;
         const R_IDX move = R_BRANCHING_FACTOR - i - 1;
         memmove(arr0 + i + 1, arr0 + i, move * sizeof *arr0);
         memmove(arr1 + i + 1, arr1 + i, move * sizeof *arr1);

         node->bounds      [i] = ins_here_bounds_st;
         node->branch_nodes[i] = ins_here.branch_ptr;

         ins_here_bounds_st  = last_bounds;
         ins_here.branch_ptr = last;
      }
#endif

      const mushboxen_iter spliter =
         r_split_node(node, *split, node_cover, split_cover,
#ifdef R_T_ORDER_AT_SEARCH_TIME
                      node_time_range, split_time_range,
#endif
                      ins_here_bounds, ins_here
#ifdef R_T_ORDER_AT_SEARCH_TIME
                    , ins_here_time
#endif
         );

      // If the insert depth is nonzero, we already got iter from the recursive
      // call above.
      if (insertee.insert_depth == 0) {
         iter = spliter;
         iter.path_depth = 0;
      }
      return iter;
   }

   if (insertee.insert_depth == 0) {
      iter.node       = node;
      iter.idx        = abs_count;
      iter.path_depth = 0;
   }

   if (node->count >= 0) {
      node->bounds    [abs_count] = *ins_here_bounds;
      node->leaf_aabbs[abs_count] = *ins_here.leaf_aabb_ptr;
#ifdef R_T_ORDER_AT_SEARCH_TIME
      node->leaf_times[abs_count] = ins_here_time.time;
#endif
      ++node->count;
   } else {
      R_IDX i;

#ifndef R_T_ORDER_AT_SEARCH_TIME
      if (ins_here_bounds == &ins_here_bounds_st) {
         // Like in the splitting case, we have to preserve T-ordering when
         // placing the result of a split.
         i = child_idx + 1;
         const R_IDX move = abs_count - i;
         if (move) {
            mushbounds  *arr0 = node->bounds;
            rtree      **arr1 = node->branch_nodes;
            memmove(arr0 + i + 1, arr0 + i, move * sizeof *arr0);
            memmove(arr1 + i + 1, arr1 + i, move * sizeof *arr1);
         }
      } else
#endif
         i = abs_count;

      node->bounds      [i] = *ins_here_bounds;
      node->branch_nodes[i] =  ins_here.branch_ptr;
#ifdef R_T_ORDER_AT_SEARCH_TIME
      node->branch_times[i] = ins_here_time.range;
#endif
      --node->count;
   }
   ++abs_count;

no_split_done:
   *split = NULL;
   *node_cover = node->bounds[0];
   for (R_IDX i = 1; i < abs_count; ++i)
      mushbounds_expand_to_cover(node_cover, &node->bounds[i]);
#ifdef R_T_ORDER_AT_SEARCH_TIME
   if (node->count >= 0) {
      node_time_range->min = node->leaf_times[0];
      node_time_range->max = node->leaf_times[0];
      for (R_IDX i = 1; i < abs_count; ++i) {
         node_time_range->min = MIN(node_time_range->min, node->leaf_times[i]);
         node_time_range->max = MAX(node_time_range->max, node->leaf_times[i]);
      }
   } else {
      *node_time_range = node->branch_times[0];
      for (R_IDX i = 1; i < abs_count; ++i)
         R_TIME_RANGE_EXPAND(node_time_range, node->branch_times[i]);
   }
#endif
   return iter;
}

// Note that the returned iterator may well be nonsensical if the insertee is
// not to end up in a leaf! In that case, it only makes sense to check whether
// it is null.
static mushboxen_iter r_root_insert(
   mushboxen* boxen, r_insertee insertee, void* aux)
{
   rtree **path_nodes;
   R_IDX  *path_idxs;
   iter_aux_split(boxen, aux, &path_nodes, &path_idxs);

   rtree      *split;
   mushbounds  root_cover, split_cover;
#ifdef R_T_ORDER_AT_SEARCH_TIME
   r_time_range root_time_range, split_time_range;
#endif
   mushboxen_iter iter =
      r_insert(insertee, &boxen->root, path_nodes, path_idxs,
               &root_cover,
#ifdef R_T_ORDER_AT_SEARCH_TIME
               &root_time_range,
#endif
               &split, &split_cover
#ifdef R_T_ORDER_AT_SEARCH_TIME
             , &split_time_range
#endif
      );
   if (mushboxen_iter_is_null(iter))
      return iter;

   if (split) {
      // The root was split: allocate a new root.
      //
      // (Or rather, allocate a new nonroot since we always store the root
      // directly in the mushboxen.)

      rtree *new = malloc(sizeof *new);
      if (!new)
         return mushboxen_iter_null;

      memcpy(new, &boxen->root, sizeof boxen->root);

      boxen->root.count = -2;

#ifndef R_T_ORDER_AT_SEARCH_TIME
      // T-ordering: either split has to come after new or it doesn't matter,
      // so just always put it after new.
#endif
      boxen->root.branch_nodes[0] = new;
      boxen->root.branch_nodes[1] = split;

      boxen->root.bounds[0] = root_cover;
      boxen->root.bounds[1] = split_cover;

#ifdef R_T_ORDER_AT_SEARCH_TIME
      boxen->root.branch_times[0] = root_time_range;
      boxen->root.branch_times[1] = split_time_range;
#endif

      if (iter.node == &boxen->root)
         iter.node = new;

      ++boxen->max_depth;
   }

   ++boxen->count;

   return iter;
}

mushboxen_iter mushboxen_insert(mushboxen* boxen, mushaabb* box, void* aux) {
   return r_root_insert(
      boxen, (r_insertee){
                .insert_depth = boxen->max_depth,
                .bounds       = &box->bounds,
#ifdef R_T_ORDER_AT_SEARCH_TIME
                .time         = { .time = boxen->time++ },
#endif
                .elem         = { .leaf_aabb_ptr = box }}, aux);
}

bool mushboxen_reserve_preserve(
   mushboxen* boxen, mushboxen_reservation* reserve, mushboxen_iter* preserve)
{
   // FIXME: do this properly
   (void)boxen; (void)reserve; (void)preserve;
   return true;
}
void mushboxen_unreserve(mushboxen* boxen, mushboxen_reservation* reserve) {
   // FIXME: do this properly
   (void)boxen; (void)reserve;
}
mushboxen_iter mushboxen_insert_reservation(
   mushboxen* boxen, mushboxen_reservation* reserve, mushaabb* box, void* aux)
{
   // FIXME: do this properly
   (void)reserve;
   return mushboxen_insert(boxen, box, aux);
}

////////////////////////////////////////// Iterator helpers

// Iterator tests: these functions return true if the given node's given index
// is acceptable to the iterator, with acceptable having a different meaning
// for each iterator. The node may be either a leaf or non-leaf node; hence
// some iterators have separate tests for the two cases.

#define CONST_TRUE(...) true

#ifdef R_T_ORDER_AT_SEARCH_TIME
static bool above_nonleaf_test(
   mushboxen_iter_above it, const rtree* node, R_IDX i)
{
   return mushbounds_overlaps(it.bounds, &node->bounds[i])
       && node->branch_times[i].min < it.sentinel_time;
}
static bool above_leaf_test(
   mushboxen_iter_above it, const rtree* node, R_IDX i)
{
   return mushbounds_overlaps(it.bounds, &node->bounds[i])
       && node->leaf_times[i] < it.sentinel_time;
}

static bool below_nonleaf_test(
   mushboxen_iter_below it, const rtree* node, R_IDX i)
{
   return mushbounds_overlaps(it.bounds, &node->bounds[i])
       && node->branch_times[i].max > it.sentinel_time;
}
static bool below_leaf_test(
   mushboxen_iter_below it, const rtree* node, R_IDX i)
{
   return mushbounds_overlaps(it.bounds, &node->bounds[i])
       && node->leaf_times[i] > it.sentinel_time;
}
#else
static bool abovebelow_test(
   mushboxen_iter_above it, const rtree* node, R_IDX i)
{
   return mushbounds_overlaps(it.bounds, &node->bounds[i]);
}
#endif

static bool in_leaf_test(mushboxen_iter_in it, const rtree* node, R_IDX i) {
   return mushbounds_contains_bounds(it.bounds, &node->bounds[i]);
}
static bool in_nonleaf_test(mushboxen_iter_in it, const rtree* node, R_IDX i) {
   return mushbounds_overlaps(it.bounds, &node->bounds[i]);
}

static bool overout_test(mushboxen_iter_overout it, const rtree* node, R_IDX i)
{
   return  mushbounds_overlaps       (it.over, &node->bounds[i])
       && !mushbounds_contains_bounds(it.out,  &node->bounds[i]);
}

// Miscellaneous R-tree helpers

// Assumes that the iterator is called "iter" and that "boxen" is in scope.
#define ITER_GOTO_FIRST_ACCEPTABLE_LEAF(FIRST_BACK_UP, NL_TEST, L_TEST) do { \
   mushboxen_iter *it = (mushboxen_iter*)iter; \
   \
   rtree **path_nodes; \
   R_IDX  *path_idxs; \
   iter_aux_split(boxen, it->aux, &path_nodes, &path_idxs); \
   \
   if (FIRST_BACK_UP) { \
back_up:; \
      R_DEPTH i = it->path_depth; \
      for (;;) { \
         if (i-- == 0) { \
            /* Can't back up any further. Just make sure the iterator is
             * invalid. */ \
            it->idx = it->node->count; \
            goto done; \
         } \
         \
         R_IDX        j = path_idxs [i] + 1; \
         const rtree *n = path_nodes[i]; \
         assert (n->count < 0 && "descended past a leaf?"); \
         \
         for (const R_IDX count = -n->count; j < count; ++j) { \
            if (NL_TEST(*iter, n, j)) { \
               path_idxs[i] = j; \
               goto found_nonleaf_up; \
            } \
         } \
      } \
found_nonleaf_up: \
      it->node = path_nodes[i]->branch_nodes[path_idxs[i]]; \
      it->path_depth = i + 1; \
   } \
   \
   while (it->node->count < 0) { \
      R_IDX i = 0; \
      for (const R_IDX count = -it->node->count; i < count; ++i) \
         if (NL_TEST(*iter, it->node, i)) \
            goto found_nonleaf; \
      \
      /* No acceptable nonleaves: have to back up. */ \
      goto back_up; \
      \
found_nonleaf: \
      path_nodes[it->path_depth] = it->node; \
      path_idxs [it->path_depth] = i; \
      it->node = it->node->branch_nodes[i]; \
      ++it->path_depth; \
   } \
   \
   for (R_IDX i = 0; i < it->node->count; ++i) { \
      if (L_TEST(*iter, it->node, i)) { \
         it->idx = i; \
         goto done; \
      } \
   } \
   \
   /* No acceptable leaves: have to back up. */ \
   goto back_up; \
   \
done:; \
   assert (it->idx == it->node->count || it->node->count >= 0); \
} while (0)

#ifndef R_T_ORDER_AT_SEARCH_TIME
// Ditto assumptions.
#define ITER_GOTO_LAST_ACCEPTABLE_LEAF(FIRST_BACK_UP, NL_TEST, L_TEST) do { \
   mushboxen_iter *it = (mushboxen_iter*)iter; \
   \
   rtree **path_nodes; \
   R_IDX  *path_idxs; \
   iter_aux_split(boxen, it->aux, &path_nodes, &path_idxs); \
   \
   if (FIRST_BACK_UP) { \
back_up:; \
      R_DEPTH i = it->path_depth; \
      for (;;) { \
         if (i-- == 0) { \
            /* Can't back up any further. Just make sure the iterator is
             * invalid. */ \
            it->idx = -1; \
            goto done; \
         } \
         \
         R_IDX        j = path_idxs [i]; \
         const rtree *n = path_nodes[i]; \
         assert (n->count < 0 && "descended past a leaf?"); \
         \
         while (j--) { \
            if (NL_TEST(*iter, n, j)) { \
               path_idxs[i] = j; \
               goto found_nonleaf_up; \
            } \
         } \
      } \
found_nonleaf_up: \
      it->node = path_nodes[i]->branch_nodes[path_idxs[i]]; \
      it->path_depth = i + 1; \
   } \
   \
   while (it->node->count < 0) { \
      R_IDX i = -it->node->count; \
      while (i--) \
         if (NL_TEST(*iter, it->node, i)) \
            goto found_nonleaf; \
      \
      /* No acceptable nonleaves: have to back up. */ \
      goto back_up; \
      \
found_nonleaf: \
      path_nodes[it->path_depth] = it->node; \
      path_idxs [it->path_depth] = i; \
      it->node = it->node->branch_nodes[i]; \
      ++it->path_depth; \
   } \
   \
   for (R_IDX i = it->node->count; i--;) { \
      if (L_TEST(*iter, it->node, i)) { \
         it->idx = i; \
         goto done; \
      } \
   } \
   \
   /* No acceptable leaves: have to back up. */ \
   goto back_up; \
   \
done:; \
   assert (it->idx == -1 || it->node->count >= 0); \
} while (0)
#endif

////////////////////////////////////////// Iterator init

mushboxen_iter mushboxen_iter_init(const mushboxen* boxen, void* aux) {
   mushboxen_iter i0 = {
      .node       = (rtree*)&boxen->root,
      .idx        = 0,
      .path_depth = 0,
      .aux        = aux,
   }, *iter = &i0;
   if (boxen->count)
      ITER_GOTO_FIRST_ACCEPTABLE_LEAF(false, CONST_TRUE, CONST_TRUE);
   return i0;
}
mushboxen_iter mushboxen_iter_copy(mushboxen_iter it, void* aux) {
   assert (!it.path_depth || aux != it.aux);
   memcpy(aux, it.aux, it.path_depth * AUX_SIZEOF);
   it.aux = aux;
   return it;
}
mushboxen_iter_above mushboxen_iter_above_init(
   const mushboxen* boxen, mushboxen_iter sentinel, void* aux)
{
   mushboxen_iter_above i0 = {
#ifdef R_T_ORDER_AT_SEARCH_TIME
      .iter          = { (rtree*)&boxen->root, 0, 0, aux },
      .sentinel_time = sentinel.node->leaf_times[sentinel.idx],
#else
      .iter   = mushboxen_iter_copy(sentinel, aux),
#endif
      .bounds = &mushboxen_iter_box(sentinel)->bounds,
   }, *iter = &i0;
#ifdef R_T_ORDER_AT_SEARCH_TIME
   if (boxen->count)
      ITER_GOTO_FIRST_ACCEPTABLE_LEAF(
         false, above_nonleaf_test, above_leaf_test);
#else
   mushboxen_iter_above_next(iter, boxen);
#endif
   return i0;
}
mushboxen_iter_below mushboxen_iter_below_init(
   const mushboxen* boxen, mushboxen_iter sentinel, void* aux)
{
   mushboxen_iter_below i0 = {
#ifdef R_T_ORDER_AT_SEARCH_TIME
      .iter          = { (rtree*)&boxen->root, 0, 0, aux },
      .sentinel_time = sentinel.node->leaf_times[sentinel.idx],
#else
      .iter   = mushboxen_iter_copy(sentinel, aux),
#endif
      .bounds = &mushboxen_iter_box(sentinel)->bounds,
   }, *iter = &i0;
#ifdef R_T_ORDER_AT_SEARCH_TIME
   if (boxen->count)
      ITER_GOTO_FIRST_ACCEPTABLE_LEAF(
         false, below_nonleaf_test, below_leaf_test);
#else
   mushboxen_iter_below_next(iter, boxen);
#endif
   return i0;
}
mushboxen_iter_in mushboxen_iter_in_init(
   const mushboxen* boxen, const mushbounds* bounds, void* aux)
{
   mushboxen_iter_in i0 = {
      .iter   = { (rtree*)&boxen->root, 0, 0, aux },
      .bounds = bounds,
   }, *iter = &i0;
   if (boxen->count)
      ITER_GOTO_FIRST_ACCEPTABLE_LEAF(false, in_nonleaf_test, in_leaf_test);
   return i0;
}
mushboxen_iter_in_bottomup mushboxen_iter_in_bottomup_init(
   const mushboxen* boxen, const mushbounds* bounds, void* aux)
{
   mushboxen_iter_in_bottomup i0 = {
      .iter   = { (rtree*)&boxen->root, 0, 0, aux },
      .bounds = bounds,
   }, *iter = &i0;

#ifdef R_T_ORDER_AT_SEARCH_TIME

   // Tricky.
   //
   // At the root, find out the range of times corresponding to boxes that
   // we (may) have to traverse. Since we're going bottom-up, we need to
   // traverse these times from largest to smallest. So keep track of the
   // current next time we are looking for, starting from the largest seen
   // at the root, and always seek out that one from the root.
   //
   // To get rid of some pointless retraversals, start all our searches from
   // the topmost node which may contain anything valid: it's not necessarily
   // the root.
   //
   // While we don't need aux and path_depth ourselves, removal uses it, so
   // fill those in.

   rtree **path_nodes;
   R_IDX  *path_idxs;
   iter_aux_split(boxen, iter->iter.aux, &path_nodes, &path_idxs);

   if (boxen->root.count > 0) {
      // If the root is a leaf, just find out the interesting range and, while
      // we're at it, move to the first box we want, if any.

      iter->interesting_root = &boxen->root;

interesting_leaf:;
      const rtree *node = iter->interesting_root;
      iter->iter.idx = -1;
      bool only_one = false;

      for (R_IDX i = 0; i < node->count; ++i) {
         if (!mushbounds_contains_bounds(iter->bounds, &node->bounds[i]))
            continue;

         const uint64_t time = node->leaf_times[i];

         if (iter->iter.idx == -1) {
            iter->time_range = (r_time_range){ time, time };
            iter->iter.idx = i;
            only_one = true;
            continue;
         }

         only_one = false;
         R_TIME_RANGE_EXPAND1(&iter->time_range, time);
         if (time == iter->time_range.max)
            iter->iter.idx = i;
      }

      // We want the time range's maximum to point to the next one, not the one
      // we're currently at. Whether such a leaf exists or not is irrelevant
      // (since we can't easily guarantee that in general), so just decrement
      // the maximum here for the root-is-a-leaf case.
      if (only_one)
         iter->time_range = (r_time_range){1,0};
      else
         --iter->time_range.max;

   } else if (boxen->root.count < 0) {
      // If the root is a nonleaf, find out the time range, go as deep as
      // possible to set interesting_root, and let _next handle the rest.
      //
      // Unless the deepest interesting node is a leaf, in which case we handle
      // it like the root above.

      R_IDX interestings;
      const rtree *node = &boxen->root;
      for (;;) {
         interestings = 0;
         R_IDX interest;

         for (R_IDX i = 0, count = -node->count; i < count; ++i) {
            if (!mushbounds_overlaps(iter->bounds, &node->bounds[i]))
               continue;

            if (interestings++ == 0) {
               iter->time_range = node->branch_times[interest = i];
               continue;
            }
            R_TIME_RANGE_EXPAND(&iter->time_range, node->branch_times[i]);
         }
         if (interestings != 1) {
            iter->interesting_root = node;
            break;
         }

         path_nodes[iter->iter.path_depth] = (rtree*)node;
         path_idxs [iter->iter.path_depth] = interest;
         iter->interesting_depth = ++iter->iter.path_depth;

         node = node->branch_nodes[interest];
         if (node->count >= 0) {
            iter->interesting_root = node;
            iter->iter.node = (rtree*)node;
            goto interesting_leaf;
         }
      }

      mushboxen_iter_in_bottomup_next(iter, boxen);
   }

   assert (iter->iter.idx == -1
        || mushbounds_contains_bounds(
              iter->bounds, &mushboxen_iter_in_bottomup_box(*iter)->bounds));
#else
   if (boxen->count)
      ITER_GOTO_LAST_ACCEPTABLE_LEAF(false, in_nonleaf_test, in_leaf_test);
#endif
   return i0;
}
mushboxen_iter_overout mushboxen_iter_overout_init(
   const mushboxen* boxen, const mushbounds* over, const mushbounds* out,
   void* aux)
{
   mushboxen_iter_overout i0 = {
      .iter = { (rtree*)&boxen->root, 0, 0, aux },
      .over = over,
      .out  = out,
   }, *iter = &i0;
   if (boxen->count)
      ITER_GOTO_FIRST_ACCEPTABLE_LEAF(false, overout_test, overout_test);
   return i0;
}

////////////////////////////////////////// Iterator done

bool mushboxen_iter_done(mushboxen_iter it, const mushboxen* boxen) {
   (void)boxen;
   return it.idx == it.node->count;
}
bool mushboxen_iter_above_done(mushboxen_iter_above it, const mushboxen* boxen)
{
#ifdef R_T_ORDER_AT_SEARCH_TIME
   return mushboxen_iter_done(it.iter, boxen);
#else
   (void)boxen;
   return it.iter.idx == -1;
#endif
}
bool mushboxen_iter_below_done(mushboxen_iter_below it, const mushboxen* boxen)
{
   return mushboxen_iter_done(it.iter, boxen);
}
bool mushboxen_iter_in_done(mushboxen_iter_in it, const mushboxen* boxen) {
   return mushboxen_iter_done(it.iter, boxen);
}
bool mushboxen_iter_in_bottomup_done(
   mushboxen_iter_in_bottomup it, const mushboxen* boxen)
{
   (void)boxen;
   return it.iter.idx == -1;
}
bool mushboxen_iter_overout_done(
   mushboxen_iter_overout it, const mushboxen* boxen)
{
   return mushboxen_iter_done(it.iter, boxen);
}

////////////////////////////////////////// Iterator next

#define ITER_NEXT(NL_TEST, L_TEST) \
   mushboxen_iter *jt = (mushboxen_iter*)iter; \
   \
   assert (jt->node->count >= 0 && "not a leaf"); \
   while (++jt->idx < jt->node->count) \
      if (L_TEST(*iter, jt->node, jt->idx)) \
         return; \
   \
   if (jt->path_depth == 0) \
      return; \
   \
   ITER_GOTO_FIRST_ACCEPTABLE_LEAF(true, NL_TEST, L_TEST);

#ifndef R_T_ORDER_AT_SEARCH_TIME
#define ITER_PREV(NL_TEST, L_TEST) \
   mushboxen_iter *jt = (mushboxen_iter*)iter; \
   \
   assert (jt->node->count >= 0 && "not a leaf"); \
   while (jt->idx--) \
      if (L_TEST(*iter, jt->node, jt->idx)) \
         return; \
   \
   if (jt->path_depth == 0) \
      return; \
   \
   ITER_GOTO_LAST_ACCEPTABLE_LEAF(true, NL_TEST, L_TEST);
#endif

void mushboxen_iter_next(mushboxen_iter* iter, const mushboxen* boxen) {
   ITER_NEXT(CONST_TRUE, CONST_TRUE);
}
void mushboxen_iter_above_next(
   mushboxen_iter_above* iter, const mushboxen* boxen)
{
#ifdef R_T_ORDER_AT_SEARCH_TIME
   ITER_NEXT(above_nonleaf_test, above_leaf_test);
#else
   ITER_PREV(abovebelow_test, abovebelow_test);
#endif
}
void mushboxen_iter_below_next(
   mushboxen_iter_below* iter, const mushboxen* boxen)
{
#ifdef R_T_ORDER_AT_SEARCH_TIME
   ITER_NEXT(below_nonleaf_test, below_leaf_test);
#else
   ITER_NEXT(abovebelow_test, abovebelow_test);
#endif
}
void mushboxen_iter_in_next(mushboxen_iter_in* iter, const mushboxen* boxen) {
   ITER_NEXT(in_nonleaf_test, in_leaf_test);
}

#ifdef R_T_ORDER_AT_SEARCH_TIME
static bool r_in_bottomup_next(
   mushboxen_iter_in_bottomup* iter, const rtree* node, uint64_t* max_valid,
   rtree** path_nodes, R_IDX* path_idxs)
{
   if (node->count >= 0) {
      // We're at a leaf with a range which contains the maximum of our range.
      // If there's actually a box that has it, that's great: go there and
      // stay. Otherwise, return the largest in-range time we saw that
      // corresponds to a valid box.
      for (R_IDX i = 0; i < node->count; ++i) {
         if (!mushbounds_contains_bounds(iter->bounds, &node->bounds[i]))
            continue;

         const uint64_t time = node->leaf_times[i];
         if (time == iter->time_range.max) {
            iter->iter.node = (rtree*)node;
            iter->iter.idx  = i;
            return true;
         }
         if (time < iter->time_range.max && time >= iter->time_range.min)
            *max_valid = MAX(*max_valid, time);
      }
      return false;
   }

   // We're at a nonleaf which contains the maximum of our range. Recurse into
   // each overlapping child that also contains it, and keep track of max_valid
   // among all overlapping children.

   for (R_IDX i = 0, count = -node->count; i < count; ++i) {
      if (!mushbounds_overlaps(iter->bounds, &node->bounds[i]))
         continue;

      const r_time_range range = node->branch_times[i];
      if (range.max < iter->time_range.max) {
         // The maximum isn't here. But something else valid might be.
         if (range.min >= iter->time_range.min)
            *max_valid = MAX(*max_valid, range.max);
         continue;
      }
      if (range.min > iter->time_range.max)
         continue;

      if (r_in_bottomup_next(iter, node->branch_nodes[i], max_valid,
                             path_nodes + 1, path_idxs + 1))
      {
         *path_nodes = (rtree*)node;
         *path_idxs  = i;
         ++iter->iter.path_depth;
         return true;
      }
   }
   return false;
}
#endif
void mushboxen_iter_in_bottomup_next(
   mushboxen_iter_in_bottomup* iter, const mushboxen* boxen)
{
#ifdef R_T_ORDER_AT_SEARCH_TIME
   // Tricky.
   //
   // From _init or a previous _next, we have the range of times that we're
   // interested in. We want to find, from the root, the box corresponding to
   // the current maximum of that range. Or, if such a box does not exist, we
   // want to find the box corresponding to the greatest time in that range
   // which has a corresponding box in the tree. If no time in the range has a
   // corresponding box, we're done.

   (void)boxen;

   rtree **path_nodes;
   R_IDX  *path_idxs;
   iter_aux_split(boxen, iter->iter.aux, &path_nodes, &path_idxs);

   while (iter->time_range.max >= iter->time_range.min) {
      iter->iter.path_depth = iter->interesting_depth;

      uint64_t max_valid_time = 0;
      if (r_in_bottomup_next(iter, iter->interesting_root, &max_valid_time,
                             path_nodes + iter->iter.path_depth,
                             path_idxs  + iter->iter.path_depth))
      {
         // We found the box corresponding to the time range's maximum. Hence
         // we haven't necessarily gone through the whole tree and thus we
         // can't trust max_valid_time, so just decrement the count.
         if (iter->time_range.max-- == 0)
            iter->time_range = (r_time_range){1,0};

         assert (mushbounds_contains_bounds(
                    iter->bounds,
                    &mushboxen_iter_in_bottomup_box(*iter)->bounds));
         return;
      }
      if (iter->time_range.max == iter->time_range.min) {
         // We were at the minimum and didn't find it, or didn't find anything
         // valid remaining: we're done.
         break;
      }
      iter->time_range.max = max_valid_time;
   }
   iter->iter.idx = -1;
#else
   ITER_PREV(in_nonleaf_test, in_leaf_test);
#endif
}
void mushboxen_iter_overout_next(
   mushboxen_iter_overout* iter, const mushboxen* boxen)
{
   ITER_NEXT(overout_test, overout_test);
}

////////////////////////////////////////// Iterator misc

void mushboxen_iter_overout_updated_next(
   mushboxen_iter_overout* it, const mushboxen* boxen)
{
   *it = mushboxen_iter_overout_init(boxen, it->over, it->out, it->iter.aux);
}

mushaabb* mushboxen_iter_box(mushboxen_iter it) {
   return &it.node->leaf_aabbs[it.idx];
}

const mushboxen_iter mushboxen_iter_null = { .node = NULL };
bool mushboxen_iter_is_null(mushboxen_iter it) { return it.node ==  NULL; }

////////////////////////////////////////// Removal

void mushboxen_iter_remove(mushboxen_iter* it, mushboxen* boxen) {
   mushboxen_remsched rs =
      { it->node, it->idx, it->idx, it->idx, it->aux, it->path_depth };
   mushboxen_remsched_apply(boxen, &rs);
   if (!rs.node)
      *it = mushboxen_iter_init(boxen, it->aux);
}

mushboxen_remsched mushboxen_remsched_init(
   mushboxen* boxen, mushboxen_iter it, void* aux)
{
   (void)boxen;
   assert (!it.path_depth || aux != it.aux);
   memcpy(aux, it.aux, it.path_depth * AUX_SIZEOF);
   return (mushboxen_remsched){
      .node       = it.node,
      .beg        = it.idx,
      .end        = it.idx,
      .also       = it.idx,
      .aux        = aux,
      .path_depth = it.path_depth,
   };
}

void mushboxen_iter_in_bottomup_sched_remove(
   mushboxen_iter_in_bottomup* it, mushboxen* boxen, mushboxen_remsched* rs)
{
   const R_IDX i = it->iter.idx;
   if (!rs->node) {
      rs->node                     = it->iter.node;
      rs->path_depth               = it->iter.path_depth;
      rs->beg = rs->end = rs->also = i;
      memcpy(rs->aux, it->iter.aux, rs->path_depth * AUX_SIZEOF);
      goto end;
   }
   if (it->iter.node == rs->node) {
      if (i == rs->beg - 1) { rs->beg = i; goto end; }
      if (i == rs->end + 1) { rs->end = i; goto end; }
      rs->also = i;
   }

   mushboxen_remsched_apply(boxen, rs);

   if (!rs->node) {
      // The tree might be completely different: we have to "start over".
      //
#ifdef R_T_ORDER_AT_SEARCH_TIME
      // While we don't care about our current position in the search as such,
      // this is still important because interesting_root might change.
#endif
      *it = mushboxen_iter_in_bottomup_init(boxen, it->bounds, it->iter.aux);
      return;
   }

   if (it->iter.node == rs->node) {
#ifndef R_T_ORDER_AT_SEARCH_TIME
      if (rs->end < i)
         it->iter.idx -= rs->end - rs->beg + 1;
      if (rs->also < i && rs->also != rs->beg && rs->also != rs->end)
         --it->iter.idx;
#endif
      // Nullify the node, because we have nothing to remove now.
      rs->node = NULL;
   } else {
      rs->node                     = it->iter.node;
      rs->path_depth               = it->iter.path_depth;
      rs->beg = rs->end = rs->also = it->iter.idx;
      memcpy(rs->aux, it->iter.aux, rs->path_depth * AUX_SIZEOF);
   }
end:
   mushboxen_iter_in_bottomup_next(it, boxen);
}

// Sets rs->node to NULL iff the node was relocated in the tree.
void mushboxen_remsched_apply(mushboxen* boxen, mushboxen_remsched* rs) {
   if (!rs->node)
      return;

   const R_IDX b = rs->beg, e = rs->end;

   assert (b <= e);
   assert (rs->node->count > 0);
   assert (e < rs->node->count);

   for (R_IDX i = b; i <= e; ++i)
      free(rs->node->leaf_aabbs[i].data);

   R_IDX new_len = rs->node->count - (e - b + 1);
   if (b != new_len) {
      mushbounds *arr0 = rs->node->bounds;
      mushaabb   *arr1 = rs->node->leaf_aabbs;
      const R_IDX move = new_len - b;
      memmove(arr0 + b, arr0 + e + 1, move * sizeof *arr0);
      memmove(arr1 + b, arr1 + e + 1, move * sizeof *arr1);

#ifdef R_T_ORDER_AT_SEARCH_TIME
      uint64_t *arr2 = rs->node->leaf_times;
      memmove(arr2 + b, arr2 + e + 1, move * sizeof *arr2);
#endif
   }

   if (rs->also < b || rs->also > e) {
      const R_IDX i = rs->also - (rs->also > e ? e - b + 1 : 0);

      free(rs->node->leaf_aabbs[i].data);
      --new_len;
      if (i != new_len) {
         mushbounds *arr0 = rs->node->bounds;
         mushaabb   *arr1 = rs->node->leaf_aabbs;
         const R_IDX move = new_len - i;
         memmove(arr0 + i, arr0 + i + 1, move * sizeof *arr0);
         memmove(arr1 + i, arr1 + i + 1, move * sizeof *arr1);

#ifdef R_T_ORDER_AT_SEARCH_TIME
         uint64_t *arr2 = rs->node->leaf_times;
         memmove(arr2 + i, arr2 + i + 1, move * sizeof *arr2);
#endif
      }
   }

   boxen->count -= rs->node->count - new_len;
   rs->node->count = new_len;

   // Walk up the tree and do necessary fixups: update covering bounds, and
   // make sure that nonroot nodes hold to the R_MIN_XS_PER_NODE lower limit.

   rtree   *reinsertions[boxen->max_depth];
   R_DEPTH  depths      [boxen->max_depth];
   R_DEPTH r = 0;

   rtree **path_nodes;
   R_IDX  *path_idxs;
   iter_aux_split(boxen, rs->aux, &path_nodes, &path_idxs);

   R_DEPTH d = rs->path_depth;
   for (rtree *node = rs->node, *parent; d--; node = parent) {
              parent = path_nodes[d];
      R_IDX node_idx = path_idxs [d];

      assert (parent->count < 0);
      assert (parent->branch_nodes[node_idx] == node);

      if (ABS(node->count) >= R_MIN_XS_PER_NODE) {
#ifndef R_T_ORDER_AT_SEARCH_TIME
update_bounds_only:
#endif
         // Simply update the entry's covering bounds in parent.
         parent->bounds[node_idx] = node->bounds[0];
         for (R_IDX i = 1, count = ABS(node->count); i < count; ++i)
            mushbounds_expand_to_cover(&parent->bounds[node_idx],
                                       &node->bounds[i]);
         continue;
      }

#ifndef R_T_ORDER_AT_SEARCH_TIME
      // If T-ordering constraints may prevent the node's entries from being
      // reinserted, do nothing.
      //
      // FIXME: we could try to distribute the entries among the siblings of
      // node in parent instead.

      const mushbounds *bounds = &parent->bounds[node_idx];

      // To avoid possibly checking the whole tree, we are conservative and
      // only check whether any ancestor's sibling (including parent's
      // siblings) overlaps with these bounds. This is conservative because it
      // doesn't mean that an actual AABB in the tree overlaps.
      R_IDX d2 = d;
      do {
         rtree *ancestor = path_nodes[d2];
         R_IDX node2_idx = path_idxs [d2];

         R_IDX i = 0;
         for (; i < node2_idx; ++i)
            if (mushbounds_overlaps(&ancestor->bounds[i], bounds))
               goto update_bounds_only;
         ++i;
         for (R_IDX count = -ancestor->count; i < count; ++i)
            if (mushbounds_overlaps(bounds, &ancestor->bounds[i]))
               goto update_bounds_only;
      } while (d2--);

      // No T-ordering constraints: delete node from parent and schedule node's
      // entries for reinsertion.
#endif

      const R_IDX move = -++parent->count - node_idx;
      if (move) {
         mushbounds *arr0 = parent->bounds;
         rtree     **arr1 = parent->branch_nodes;
         memmove(arr0 + node_idx, arr0 + node_idx + 1, move * sizeof *arr0);
         memmove(arr1 + node_idx, arr1 + node_idx + 1, move * sizeof *arr1);

#ifdef R_T_ORDER_AT_SEARCH_TIME
         r_time_range *arr2 = parent->branch_times;
         memmove(arr2 + node_idx, arr2 + node_idx + 1, move * sizeof *arr2);
#endif
      }

      reinsertions[r] = node;
      depths      [r] = d + 1;
      ++r;
   }

   if (r) {
      // Perform the scheduled reinsertions.

      void *aux = alloca(mushboxen_iter_aux_size(boxen));

      for (R_DEPTH i = 0; i < r; ++i) {
         rtree *node = reinsertions[i];
         R_DEPTH ins_depth = depths[i];

         const bool leaf = node->count >= 0;
         if (i)
            assert (!leaf);

         for (R_IDX j = 0, count = ABS(node->count); j < count; ++j) {
            if (i && node->branch_nodes[j] == reinsertions[i-1])
               continue;

            const r_insertee insertee = {
               .insert_depth = ins_depth,
               .bounds       = &node->bounds[j],
#ifdef R_T_ORDER_AT_SEARCH_TIME
               .time =
                  leaf ? (r_ins_time){ .time  = node->leaf_times  [j] }
                       : (r_ins_time){ .range = node->branch_times[j] },
#endif
               .elem =
                  leaf ? (r_elem){ .leaf_aabb_ptr = &node->leaf_aabbs  [j] }
                       : (r_elem){ .branch_ptr    =  node->branch_nodes[j] }};

            mushboxen_iter iter = r_root_insert(boxen, insertee, aux);
            assert (!mushboxen_iter_is_null(iter)
                 && "FIXME: can't handle malloc failure on reinsertion");
            (void)iter;
         }
         free(node);
      }
      rs->node = NULL;
   }

   // If the root only has one child, make that child the new root.
   if (boxen->root.count == -1) {
      --boxen->max_depth;
      rtree *child = boxen->root.branch_nodes[0];
      memcpy(&boxen->root, child, sizeof *child);
      free(child);
      rs->node = NULL;
   }
}
