// File created: 2011-08-21 16:17:09

#include "bounds.all.h"

#include <assert.h>

size_t mushbounds_clamped_size(const mushbounds* bounds) {
   size_t sz = 1;
   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
      sz = mush_size_t_mul_clamped(
         sz, mush_size_t_add_clamped(bounds->end.v[i] - bounds->beg.v[i], 1));
   return sz;
}

#if !MUSHSPACE_93
mush_double_size_t mushbounds_volume_on(const mushbounds* bounds, mushdim axis)
{
   assert (axis < MUSHSPACE_DIM);
   mush_double_size_t sz = {0,1};
   for (mushdim i = 0; i < axis; ++i)
      mush_double_size_t_mul1_into(
         &sz, mushcell_inc(mushcell_sub(bounds->end.v[i], bounds->beg.v[i])));
   return sz;
}
#endif

bool mushbounds_contains(const mushbounds* bounds, mushcoords pos) {
   if (pos.x < bounds->beg.x || pos.x > bounds->end.x) return false;
#if MUSHSPACE_DIM >= 2
   if (pos.y < bounds->beg.y || pos.y > bounds->end.y) return false;
#if MUSHSPACE_DIM >= 3
   if (pos.z < bounds->beg.z || pos.z > bounds->end.z) return false;
#endif
#endif
   return true;
}
bool mushbounds_safe_contains(const mushbounds* bounds, mushcoords pos) {
   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
      if (bounds->beg.v[i] > bounds->end.v[i]) {
         if (!(pos.v[i] >= bounds->beg.v[i] || pos.v[i] <= bounds->end.v[i]))
            return false;
      } else {
         if (!(pos.v[i] >= bounds->beg.v[i] && pos.v[i] <= bounds->end.v[i]))
            return false;
      }
   }
   return true;
}

bool mushbounds_contains_bounds(const mushbounds* a, const mushbounds* b) {
   return mushbounds_contains(a, b->beg) && mushbounds_contains(a, b->end);
}
bool mushbounds_overlaps(const mushbounds* a, const mushbounds* b) {
   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
      if (a->beg.v[i] > b->end.v[i] || b->beg.v[i] > a->end.v[i])
         return false;
   return true;
}

#if !MUSHSPACE_93
bool mushbounds_get_overlap(
   const mushbounds* a, const mushbounds* b, mushbounds* overlap)
{
   if (!mushbounds_overlaps(a, b))
      return false;

   overlap->beg = a->beg; mushcoords_max_into(&overlap->beg, b->beg);
   overlap->end = a->end; mushcoords_min_into(&overlap->end, b->end);

   assert (mushbounds_contains_bounds(a, overlap));
   assert (mushbounds_contains_bounds(b, overlap));
   return true;
}

#if MUSHSPACE_DIM > 1
bool mushbounds_on_same_axis(const mushbounds* a, const mushbounds* b) {
   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
      if (a->beg.v[i] == b->beg.v[i] && a->end.v[i] == b->end.v[i])
         return true;
   return false;
}
bool mushbounds_on_same_primary_axis(const mushbounds* a, const mushbounds* b)
{
   const mushdim I = MUSHSPACE_DIM-1;
   return a->beg.v[I] == b->beg.v[I] && a->end.v[I] == b->end.v[I];
}
#endif

bool mushbounds_can_fuse(const mushbounds* a, const mushbounds* b) {
   bool overlap = mushbounds_overlaps(a, b);

   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
      if (a->beg.v[i] == b->beg.v[i] && a->end.v[i] == b->end.v[i])
         continue;

      if (!(   overlap
            || mushcell_add_clamped(a->end.v[i], 1) == b->beg.v[i]
            || mushcell_add_clamped(b->end.v[i], 1) == a->beg.v[i]))
         return false;

      for (mushdim j = i+1; j < MUSHSPACE_DIM; ++j)
         if (a->beg.v[j] != b->beg.v[j] || a->end.v[j] != b->end.v[j])
            return false;

      #if MUSHSPACE_DIM > 1
         assert (mushbounds_on_same_axis(a, b));
      #endif
      return true;
   }
   return false;
}

void mushbounds_tessellate(
   mushbounds* bounds, mushcoords pos, mushcarr_mushbounds bs)
{
   assert (mushbounds_contains(bounds, pos));

   for (size_t i = 0; i < bs.len; ++i)
      mushbounds_tessellate1(bounds, pos, &bs.ptr[i]);
}
void mushbounds_tessellate1(
   mushbounds* bounds, mushcoords pos, const mushbounds* avoid)
{
   assert (mushbounds_contains(bounds, pos));

   if (!mushbounds_overlaps(bounds, avoid))
      return;

   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
      // This could be improved, consider for instance the bottommost box in
      // the following graphic and its current tessellation:
      //
      // +-------+    +--*--*-+
      // |       |    |X .  . |
      // |       |    |  .  . |
      // |     +---   *..*..+---
      // |     |      |  .  |
      // |  +--|      *..+--|
      // |  |  |      |  |  |
      // |  |  |      |  |  |
      // +--|  |      +--|  |
      //
      // (Note that this isn't actually a tessellation: all points will get
      // a rectangle containing the rectangle at X.)
      //
      // Any of the following three would be an improvement (and they would
      // actually be tessellations):
      //
      // +--*--*-+    +-------+    +-----*-+
      // |  .  . |    |       |    |     . |
      // |  .  . |    |       |    |     . |
      // |  .  +---   *.....+---   |     +---
      // |  .  |      |     |      |     |
      // |  +--|      *..+--|      *..+--|
      // |  |  |      |  |  |      |  |  |
      // |  |  |      |  |  |      |  |  |
      // +--|  |      +--|  |      +--|  |

      mushcell ab = avoid->beg.v[i],
               ae = avoid->end.v[i],
               p  = pos.v[i];
      if (ae < p) bounds->beg.v[i] = mushcell_max(bounds->beg.v[i], ae+1);
      if (ab > p) bounds->end.v[i] = mushcell_min(bounds->end.v[i], ab-1);
   }
   assert (!mushbounds_overlaps(bounds, avoid));
}
#endif // !MUSHSPACE_93
