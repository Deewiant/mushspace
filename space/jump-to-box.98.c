// File created: 2012-01-28 00:28:27

#include "space/jump-to-box.98.h"

#include <assert.h>

#include "bounds/ray-intersects.98.h"

bool mushspace_jump_to_box(
   mushspace* space, mushcoords* pos, mushcoords delta,
   MushCursorMode* hit_type, mushaabb** aabb, mushboxen_iter* aabb_iter)
{
   assert (!mushboxen_get(&space->boxen, *pos));

   mushucell  moves = 0;
   mushcoords pos2;
   mushucell  m;
   mushcoords c;

   bool hit_static;
   mushboxen_iter aabb_iter2;

   // Pick the closest box that we hit, starting from the topmost.

   if (mushbounds_ray_intersects(*pos, delta, &MUSHSTATICAABB_BOUNDS, &m, &c))
   {
      pos2       = c;
      moves      = m;
      hit_static = true;
   }

   for (mushboxen_iter it = mushboxen_iter_init(&space->boxen);
        !mushboxen_iter_done( it, &space->boxen);
         mushboxen_iter_next(&it, &space->boxen))
   {
      // The !moves option is necessary: we can't initialize moves and rely on
      // "m < moves" because m might legitimately be the greatest possible
      // mushucell value.
      if (mushbounds_ray_intersects(
            *pos, delta, &mushboxen_iter_box(it)->bounds, &m, &c)
       && ((m < moves) | !moves))
      {
         pos2       = c;
         moves      = m;
         aabb_iter2 = it;
         hit_static = false;
      }
   }

   if (space->bak.data
    && mushbounds_ray_intersects(*pos, delta, &space->bak.bounds, &m, &c)
    && ((m < moves) | !moves))
   {
      *pos      = c;
      *hit_type = MushCursorMode_bak;
      return true;
   }

   if (!moves)
      return false;

   if (hit_static) {
      *pos      = pos2;
      *hit_type = MushCursorMode_static;
      return true;
   }

   *pos       = pos2;
   *hit_type  = MushCursorMode_dynamic;
   *aabb_iter = aabb_iter2;
   *aabb      = mushboxen_iter_box(*aabb_iter);
   return true;
}
