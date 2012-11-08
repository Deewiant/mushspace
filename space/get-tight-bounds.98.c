// File created: 2012-01-28 00:18:01

#include "space/get-tight-bounds.all.h"

#include <alloca.h>
#include <assert.h>

static bool find_beg_in(mushcoords*, mushdim, const mushbounds*,
                        mushcell(*)(const void*, mushcoords), const void*);

static void find_end_in(mushcoords*, mushdim, const mushbounds*,
                        mushcell(*)(const void*, mushcoords), const void*);

bool mushspace_get_tight_bounds(mushspace* space, mushbounds* bounds) {
   bool last_beg_space = mushspace_get(space, space->last_beg) == ' ',
        last_end_space = mushspace_get(space, space->last_end) == ' ',
        found_nonspace = true;

   if (last_beg_space == last_end_space) {
      if (last_beg_space) {
         bounds->beg = MUSHCOORDS(MUSHCELL_MAX, MUSHCELL_MAX, MUSHCELL_MAX);
         bounds->end = MUSHCOORDS(MUSHCELL_MIN, MUSHCELL_MIN, MUSHCELL_MIN);
         found_nonspace = false;
      } else {
         bounds->beg = space->last_beg;
         bounds->end = space->last_end;
      }
   } else {
      if (last_beg_space)
         bounds->beg = bounds->end = space->last_end;
      else
         bounds->beg = bounds->end = space->last_beg;
   }

   bool changed = false;

   void *aux = alloca(mushboxen_iter_aux_size(&space->boxen));

   for (mushdim axis = 0; axis < MUSHSPACE_DIM; ++axis) {
      for (mushboxen_iter it = mushboxen_iter_init(&space->boxen, aux);
           !mushboxen_iter_done( it, &space->boxen);)
      {
         mushaabb *box = mushboxen_iter_box(it);
         if (find_beg_in(&bounds->beg, axis, &box->bounds,
                         mushaabb_getter_no_offset, box))
         {
            mushboxen_iter_remove(&it, &space->boxen);
            mushstats_add(space->stats, MushStat_empty_boxes_dropped, 1);
            changed = true;
            continue;
         }
         found_nonspace = true;
         find_end_in(&bounds->end, axis, &box->bounds,
                     mushaabb_getter_no_offset, box);
         mushboxen_iter_next(&it, &space->boxen);
      }

      // The non-static boxes are more likely to give the more extreme bounds,
      // so check the static box only afterwards, avoiding a lot of looping if
      // it's very empty.
      found_nonspace |=
         !find_beg_in(&bounds->beg, axis, &MUSHSTATICAABB_BOUNDS,
                      mushstaticaabb_getter_no_offset, &space->static_box);

      find_end_in(&bounds->end, axis, &MUSHSTATICAABB_BOUNDS,
                  mushstaticaabb_getter_no_offset, &space->static_box);
   }

   if (changed)
      mushspace_invalidate_all(space);

#if USE_BAKAABB
   if (space->bak.data && mushbakaabb_size(&space->bak) > 0) {
      found_nonspace = true;

      // We might as well tighten the approximate space->bak.bounds while we're
      // at it.
      mushbounds bak_bounds = {space->bak.bounds.end, space->bak.bounds.beg};

      unsigned char buf[mushbakaabb_iter_sizeof];
      mushbakaabb_iter *it = mushbakaabb_it_start(&space->bak, buf);

      for (; !mushbakaabb_it_done(it, &space->bak);
              mushbakaabb_it_next(it, &space->bak))
      {
         assert (mushbakaabb_it_val(it, &space->bak) != ' ');

         mushcoords c = mushbakaabb_it_pos(it, &space->bak);
         mushbounds_expand_to_cover(&bak_bounds, &(mushbounds){c,c});
      }
      mushbakaabb_it_stop(it);

      space->bak.bounds = bak_bounds;
      mushbounds_expand_to_cover(bounds, &bak_bounds);
   }
#endif
   space->last_beg = bounds->beg;
   space->last_end = bounds->end;
   return found_nonspace;
}

static bool find_beg_in(
   mushcoords* beg, mushdim axis, const mushbounds* bounds,
   mushcell(*getter_no_offset)(const void*, mushcoords), const void* box)
{
   if (beg->v[axis] <= bounds->beg.v[axis])
      return false;

   // Quickly check the corner, in case we get lucky and can avoid a lot of
   // complication.
   if (getter_no_offset(box, MUSHCOORDS(0,0,0)) != ' ') {
      mushcoords_min_into(beg, bounds->beg);
      return false;
   }

   bool empty_box = true;

   mushcoords last = bounds->end;

   // If our beg is already better than part of the box, only check up to it
   // instead of the whole box.
   if (beg->v[axis] <= last.v[axis]) {
      // This decrement cannot underflow because we already checked against the
      // box's beginning coordinate above: we know the box's beg is not
      // MUSHCELL_MIN and thus its end isn't either.
      last.v[axis] = beg->v[axis] - 1;

      // We can conclude that the box is empty only if we're going to traverse
      // it completely.
      empty_box = false;
   }

   mushcoords_sub_into(&last, bounds->beg);

   switch (axis) {
      mushcoords c;

   #define CHECK { \
      if (getter_no_offset(box, c) == ' ') \
         continue; \
   \
      mushcoords_min_into(beg, mushcoords_add(c, bounds->beg)); \
      if (c.v[axis] == 0) \
         return false; \
   \
      mushcell_min_into(&last.v[axis], c.v[axis]); \
      empty_box = false; \
      break; \
   }

   case 0:
#if MUSHSPACE_DIM >= 3
      for (c.z = 0; c.z <= last.z; ++c.z)
#endif
#if MUSHSPACE_DIM >= 2
      for (c.y = 0; c.y <= last.y; ++c.y)
#endif
      for (c.x = 0; c.x <= last.x; ++c.x)
         CHECK
      break;

#if MUSHSPACE_DIM >= 2
   case 1:
#if MUSHSPACE_DIM >= 3
      for (c.z = 0; c.z <= last.z; ++c.z)
#endif
      for (c.x = 0; c.x <= last.x; ++c.x)
      for (c.y = 0; c.y <= last.y; ++c.y)
         CHECK
      break;
#endif

#if MUSHSPACE_DIM >= 3
   case 2:
      for (c.y = 0; c.y <= last.y; ++c.y)
      for (c.x = 0; c.x <= last.x; ++c.x)
      for (c.z = 0; c.z <= last.z; ++c.z)
         CHECK
      break;
#endif

   default: MUSH_UNREACHABLE("axis not supported by dimensionality");
   #undef CHECK
   }
   return empty_box;
}

static void find_end_in(
   mushcoords* end, mushdim axis, const mushbounds* bounds,
   mushcell(*getter_no_offset)(const void*, mushcoords), const void* box)
{
   if (end->v[axis] >= bounds->end.v[axis])
      return;

   if (getter_no_offset(box, mushcoords_sub(bounds->end, bounds->beg)) != ' ')
   {
      mushcoords_max_into(end, bounds->end);
      return;
   }

   mushcoords last = MUSHCOORDS(0,0,0);

   if (end->v[axis] >= bounds->beg.v[axis])
      last.v[axis] = end->v[axis] + 1 - bounds->beg.v[axis];

   const mushcoords start = mushcoords_sub(bounds->end, bounds->beg);

   switch (axis) {
      mushcoords c;

   #define CHECK { \
      if (getter_no_offset(box, c) == ' ') \
         continue; \
   \
      mushcoords_max_into(end, mushcoords_add(c, bounds->beg)); \
      if (end->v[axis] == bounds->end.v[axis]) \
         return; \
   \
      mushcell_max_into(&last.v[axis], c.v[axis]); \
      break; \
   }

   case 0:
#if MUSHSPACE_DIM >= 3
      for (c.z = start.z; c.z >= last.z; --c.z)
#endif
#if MUSHSPACE_DIM >= 2
      for (c.y = start.y; c.y >= last.y; --c.y)
#endif
      for (c.x = start.x; c.x >= last.x; --c.x)
         CHECK
      break;

#if MUSHSPACE_DIM >= 2
   case 1:
#if MUSHSPACE_DIM >= 3
      for (c.z = start.z; c.z >= last.z; --c.z)
#endif
      for (c.x = start.x; c.x >= last.x; --c.x)
      for (c.y = start.y; c.y >= last.y; --c.y)
         CHECK
      break;
#endif

#if MUSHSPACE_DIM >= 3
   case 2:
      for (c.y = start.y; c.y >= last.y; --c.y)
      for (c.x = start.x; c.x >= last.x; --c.x)
      for (c.z = start.z; c.z >= last.z; --c.z)
         CHECK
      break;
#endif

   default: MUSH_UNREACHABLE("axis not supported by dimensionality");
   #undef CHECK
   }
}
