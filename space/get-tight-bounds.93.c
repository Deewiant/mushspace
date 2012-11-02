// File created: 2012-01-29 11:01:05

#include "space/get-tight-bounds.all.h"

#include <assert.h>

bool mushspace_get_tight_bounds(const mushspace* space, mushbounds* bounds) {
   bounds->beg.x = 79;
   bounds->end.x =  0;

   mushcoords c;

   bool found_on_row,
        found_anywhere = false;
   for (c.y = 0; c.y < 25; ++c.y) {

      if (bounds->beg.x == 0) {
         assert (found_anywhere);
         assert (bounds->beg.y < c.y);

         if (bounds->end.x == 79)
            break;
      } else {
         found_on_row = false;
         for (c.x = 0; c.x < 80; ++c.x) {
            if (mushstaticaabb_get(&space->box, c) != ' ') {
               mushcell_min_into(&bounds->beg.x, c.x);
               found_on_row = true;
               break;
            }
         }
         if (!found_on_row)
            continue;

         if (!found_anywhere) {
            found_anywhere = true;
            bounds->beg.y = c.y;
         } else if (bounds->end.x == 79)
            continue;
      }

      for (c.x = 80; c.x-- > 0;) {
         if (mushstaticaabb_get(&space->box, c) != ' ') {
            mushcell_max_into(&bounds->end.x, c.x);
            break;
         }
      }
   }
   if (!found_anywhere)
      return false;

   // Still need bounds->end.y.

   if (bounds->beg.y == 24 || (c.y == 24 && found_on_row)) {
      bounds->end.y = 24;
      return true;
   }

   for (c.y = 25; c.y-- > bounds->beg.y;) {
      for (c.x = 0; c.x < 80; ++c.x) {
         if (mushstaticaabb_get(&space->box, c) != ' ') {
            bounds->end.y = c.y;
            return true;
         }
      }
   }
   MUSH_UNREACHABLE("found nonspace in x- but not y-direction");
}
