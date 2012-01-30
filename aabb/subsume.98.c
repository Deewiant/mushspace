// File created: 2012-01-28 01:06:38

#include "aabb/subsume.98.h"

#include <assert.h>
#include <string.h>

MUSH_DECL_CONST_DYN_ARRAY(mushcell)

// Copies from data to aabb, given that it's an area contained in owner.
static void subsume_owners_area(
   mushaabb* aabb, const mushaabb* owner, const mushaabb* area,
   mushcarr_mushcell data);

void mushaabb_subsume(mushaabb* a, const mushaabb* b) {
   assert (mushbounds_contains_bounds(&a->bounds, &b->bounds));

   subsume_owners_area(a, b, b, (mushcarr_mushcell){b->data, b->size});
}

void mushaabb_subsume_area(
   mushaabb* a, const mushaabb* b, const mushaabb* area)
{
   assert (mushbounds_contains_bounds(&a->bounds, &area->bounds));
   assert (mushbounds_contains_bounds(&b->bounds, &area->bounds));

   const mushbounds* ab = &area->bounds;

   size_t beg_idx = mushaabb_get_idx(b, ab->beg),
          end_idx = mushaabb_get_idx(b, ab->end);

   subsume_owners_area(
      a, b, area,
      (mushcarr_mushcell){b->data + beg_idx, end_idx - beg_idx + 1});

   assert (mushaabb_get(a, ab->beg) == mushaabb_get(b, ab->beg));
   assert (mushaabb_get(a, ab->end) == mushaabb_get(b, ab->end));
}

static void subsume_owners_area(
   mushaabb* aabb, const mushaabb* owner, const mushaabb* area,
   mushcarr_mushcell arr)
{
   assert (mushbounds_contains_bounds(&aabb->bounds,  &area->bounds));
   assert (mushbounds_contains_bounds(&owner->bounds, &area->bounds));

   // We can't just use only area since the data is usually not continuous:
   //
   //   ownerowner
   //   owner****D
   //   ATADA****D
   //   ATADA****D
   //   ATADA****r
   //   ownerowner
   //
   // (In the above: area is shown with asterisks, owner with "owner", and the
   //  area from arr.ptr to arr.ptr+arr.len with "DATA".)
   //
   // In such a case, if we were to advance in the array by area->width instead
   // of owner->width we'd be screwed.

   static const size_t SIZE = sizeof *arr.ptr;

   const mushbounds* ab = &area->bounds;

   const size_t beg_idx = mushaabb_get_idx(aabb, ab->beg);

   if (mushaabb_can_direct_copy_area(aabb, area, owner)) {
      memcpy(aabb->data + beg_idx, arr.ptr, arr.len * SIZE);
      goto end;
   }

   assert (MUSHSPACE_DIM > 1);

#if MUSHSPACE_DIM == 2
   for (size_t o = 0, a = beg_idx; o < arr.len;) {
      memcpy(aabb->data + a, arr.ptr + o, area->width * SIZE);
      o += owner->width;
      a +=  aabb->width;
   }

#elif MUSHSPACE_DIM == 3
   const size_t owner_area_size = area->area / area->width * owner->width;

   for (size_t o = 0, a = beg_idx; o < arr.len;) {
      for (size_t oy = o, ay = a; oy < o + owner_area_size;) {
         memcpy(aabb->data + ay, arr.ptr + oy, area->width * SIZE);
         oy += owner->width;
         ay +=  aabb->width;
      }
      o += owner->area;
      a +=  aabb->area;
   }
#endif
end:
   assert (mushaabb_get(aabb, ab->beg) == arr.ptr[0]);
   assert (mushaabb_get(aabb, ab->end) == arr.ptr[arr.len-1]);
   assert (mushaabb_get(aabb, ab->beg) == mushaabb_get(owner, ab->beg));
   assert (mushaabb_get(aabb, ab->end) == mushaabb_get(owner, ab->end));
}
