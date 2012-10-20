// File created: 2012-01-27 21:54:22

#include "space/load-string.all.h"

#include <assert.h>

#include "space/map-no-place.98.h"
#include "space/place-box.98.h"

#include "lib/icu/utf.h"

MUSH_DECL_DYN_ARRAY(mushaabb)
MUSH_DECL_DYN_ARRAY(mushbounds)

typedef struct {
   const void *str, *end;
} binary_load_arr_auxdata;

typedef struct {
   const void *str, *end;

   const mushbounds *bounds;

   mushcoords pos;

   #if MUSHSPACE_DIM >= 2
      mushcell target_x;
   #if MUSHSPACE_DIM >= 3
      mushcell target_y;
   #endif
   #endif
} load_arr_auxdata;

static int load_string_generic(
   mushspace* space, const void** str, const void* str_end,
   mushcoords* end, mushcoords target, bool binary,
   void (*get_aabbs)(const void*, const void*, mushcoords, bool,
                     musharr_mushaabb*),
   void (*load_arr)         (musharr_mushcell, void*,
                             size_t, size_t, size_t, size_t, uint8_t*),
   void (*load_blank)       (size_t, void*),
   void (*binary_load_arr)  (musharr_mushcell, void*),
   void (*binary_load_blank)(size_t, void*))
{
   musharr_mushaabb aabbs;
   get_aabbs(*str, str_end, target, binary, &aabbs);

   if (!aabbs.ptr) {
      // The error code was placed in aabbs.len.
      return (int)aabbs.len;
   }

   if (end)
      *end = target;

   if (!aabbs.len) {
      if (end)
         end->x = mushcell_dec(end->x);
      return MUSHERR_NONE;
   }

   for (size_t i = 0; i < aabbs.len; ++i) {
      if (end)
         mushcoords_max_into(end, aabbs.ptr[i].bounds.end);

      if (!mushspace_place_box(space, &aabbs.ptr[i], NULL, NULL))
         return MUSHERR_OOM;
   }

   // Build one to rule them all.
   //
   // Note that it may have beg > end along any number of axes!
   mushaabb aabb = aabbs.ptr[0];
   mushbounds *bounds = &aabb.bounds;
   if (aabbs.len > 1) {
      // If any box was placed past an axis, the end of that axis is the
      // maximum of the ends of such boxes. Otherwise, it's the maximum of all
      // the boxes' ends.
      //
      // Similarly, if any box was placed before an axis, the beg is the
      // minimum of such boxes' begs.
      //
      // Note that these boxes, from get_aabbs, don't cover any excess area!
      // They may be smaller than the range from target to the end point due to
      // dropping space-filled lines and columns, but they haven't been
      // subsumed into bigger boxes or anything.

      uint8_t found_past = 0, found_before = 0;

      for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
         if (bounds->beg.v[i] < target.v[i]) found_past   |= 1 << i;
         else                                found_before |= 1 << i;
      }

      for (size_t b = 1; b < aabbs.len; ++b) {
         const mushbounds *box = &aabbs.ptr[b].bounds;

         for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) {
            const uint8_t axis = 1 << i;

            if (box->beg.v[i] < target.v[i]) {
               // This box is past this axis, in the positive direction: it
               // both begins and ends in the negative direction from the
               // target location.

               if (found_past & axis)
                  mushcell_max_into(&bounds->end.v[i], box->end.v[i]);
               else {
                  bounds->end.v[i] = box->end.v[i];
                  found_past |= axis;
               }

               if (!(found_before & axis))
                  mushcell_min_into(&bounds->beg.v[i], box->beg.v[i]);
            } else {
               // This box is before this axis, in the negative direction: it
               // both begins and ends in the positive direction from the
               // target location.

               if (found_before & axis)
                  mushcell_min_into(&bounds->beg.v[i], box->beg.v[i]);
               else {
                  bounds->beg.v[i] = box->beg.v[i];
                  found_before |= axis;
               }

               if (!(found_past & axis))
                  mushcell_max_into(&bounds->end.v[i], box->end.v[i]);
            }
         }
      }
   }

   if (binary) {
      binary_load_arr_auxdata aux = {*str, str_end};
      mushspace_map_no_place(space, bounds, &aux,
                             binary_load_arr, binary_load_blank);
      *str = aux.str;
   } else {
      load_arr_auxdata aux =
         { *str, str_end
         , bounds
         , target
      #if MUSHSPACE_DIM >= 2
         , target.x
      #endif
      #if MUSHSPACE_DIM >= 3
         , target.y
      #endif
      };

      mushspace_mapex_no_place(space, bounds, &aux, load_arr, load_blank);
      *str = aux.str;
   }
   return MUSHERR_NONE;
}

#define UTF
#define C unsigned char
#define NEXT(s, s_end, c) do { (void)s_end; (c = (*(s)++)); } while (0)
#include "space/load-string.inc.c"

#define UTF _utf8
#define C uint8_t
#define NEXT(s, s_end, c) U8_NEXT_PTR(s, s_end, c)
#include "space/load-string.inc.c"

#define UTF _utf16
#define C uint16_t
#define NEXT(s, s_end, c) U16_NEXT_PTR(s, s_end, c)
#include "space/load-string.inc.c"

#define UTF _cell
#define C mushcell
#define NEXT(s, s_end, c) do { (void)s_end; (c = (*(s)++)); } while (0)
#include "space/load-string.inc.c"
