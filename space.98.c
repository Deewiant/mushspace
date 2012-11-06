// File created: 2011-08-06 15:44:53

#include "space.all.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "stdlib.any.h"
#include "space/map-no-place.98.h"
#include "space/place-box.98.h"
#include "space/place-box-for.98.h"

mushspace* mushspace_init(void* vp, mushstats* stats) {
   mushspace *space = vp ? vp : malloc(sizeof *space);
   if (!space)
      return NULL;

   if (stats) {
      space->private_stats = false;
      space->stats = stats;
   } else {
      space->private_stats = true;
      if (!(space->stats = malloc(sizeof *space->stats))) {
         free(space);
         return NULL;
      }

// See http://lists.cs.uiuc.edu/pipermail/cfe-dev/2012-February/019920.html for
// discussion on this. As of Clang 3.1, this is our only option apart from
// spelling out an initializer for each field.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
      *space->stats = (mushstats){0};
#pragma clang diagnostic pop
   }

   space->invalidatees = NULL;

   mushboxen_init(&space->boxen);

   space->bak.data = NULL;

   // Placate valgrind and such: it's not necessary to define these before the
   // first use.
   space->last_beg = space->last_end = MUSHCOORDS(0,0,0);

   mushmemorybuf_init(&space->recent_buf);

   mushcell_space(
      space->static_box.array, MUSH_ARRAY_LEN(space->static_box.array));
   return space;
}

void mushspace_free(mushspace* space) {
   mushboxen_free(&space->boxen);
   mushbakaabb_free(&space->bak);
   if (space->invalidatees) {
      free(space->invalidatees);
      free(space->invalidatees_data);
   }
   if (space->private_stats)
      free(space->stats);
}

mushspace* mushspace_copy(void* vp, const mushspace* space, mushstats* stats) {
   mushspace *copy = vp ? vp : malloc(sizeof *copy);
   if (!copy)
      return NULL;

   // Easiest to do here, so we don't have to worry about whether we need to
   // free copy->stats or not.
   if (space->bak.data && !mushbakaabb_copy(&copy->bak, &space->bak)) {
      free(copy);
      return NULL;
   }

   memcpy(copy, space, sizeof *copy);

   if (stats) {
      copy->private_stats = false;
      copy->stats = stats;
   } else {
      copy->private_stats = true;
      copy->stats = malloc(sizeof *copy->stats);
      if (!copy->stats) {
         free(copy);
         return NULL;
      }
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
      *space->stats = (mushstats){0};
#pragma clang diagnostic pop
   }

   mushboxen_copy(&copy->boxen, &space->boxen);

   // Invalidatees refer to the original space, not the copy.
   copy->invalidatees = NULL;

   return copy;
}

mushcell mushspace_get(const mushspace* space, mushcoords c) {
   if (mushstaticaabb_contains(c))
      return mushstaticaabb_get(&space->static_box, c);

   const mushaabb* box;
   if ((box = mushboxen_get(&space->boxen, c)))
      return mushaabb_get(box, c);

   if (space->bak.data)
      return mushbakaabb_get(&space->bak, c);

   return ' ';
}

int mushspace_put(mushspace* space, mushcoords p, mushcell c) {
   if (mushstaticaabb_contains(p)) {
      mushstaticaabb_put(&space->static_box, p, c);
      return MUSHERR_NONE;
   }

   mushaabb* box;
   if ((box = mushboxen_get(&space->boxen, p))
    || mushspace_place_box_for(space, p, &box))
   {
      mushaabb_put(box, p, c);
      return MUSHERR_NONE;
   }

   if (!space->bak.data && !mushbakaabb_init(&space->bak, p))
      return MUSHERR_OOM;

   if (!mushbakaabb_put(&space->bak, p, c))
      return MUSHERR_OOM;

   return MUSHERR_NONE;
}

void mushspace_get_loose_bounds(const mushspace* space, mushbounds* bounds) {
   bounds->beg = MUSHSTATICAABB_BEG;
   bounds->end = MUSHSTATICAABB_END;
   mushboxen_loosen_bounds(&space->boxen, bounds);
   if (space->bak.data) {
      mushcoords_min_into(&bounds->beg, space->bak.bounds.beg);
      mushcoords_max_into(&bounds->end, space->bak.bounds.end);
   }
}

int mushspace_map(mushspace* space, mushbounds bounds,
                  void(*f)(musharr_mushcell, mushcoords, mushcoords, void*),
                  void* data)
{
   mushaabb aabb;
   mushaabb_make(&aabb, &bounds);
   if (!mushspace_place_box(space, &aabb, NULL, NULL))
      return MUSHERR_OOM;

   mushspace_map_no_place(space, &bounds, data, f, NULL);
   return MUSHERR_NONE;
}
void mushspace_map_existing(
   mushspace* space, mushbounds bounds,
   void(*f)(musharr_mushcell, mushcoords, mushcoords, void*),
   void(*g)(                  mushcoords, mushcoords, void*),
   void* data)
{
   mushaabb aabb;
   mushaabb_make(&aabb, &bounds);
   mushspace_map_no_place(space, &bounds, data, f, g);
}

void mushspace_invalidate_all(mushspace* space) {
   void (**i)(void*) = space->invalidatees;
   void  **d         = space->invalidatees_data;
   if (i)
      while (*i)
         (*i++)(*d++);
}

bool mushspace_add_invalidatee(mushspace* space, void(*i)(void*), void* d) {
   size_t n = 1;
   void (**is)(void*) = space->invalidatees;
   void  **id;
   if (is) {
      while (*is++)
         ++n;
      id = space->invalidatees_data;
   } else
      id = NULL;

   is = realloc(space->invalidatees, (n+1) * sizeof *is);
   if (!is)
      return false;

   // The realloc succeeded, so space->invalidatees is invalid, so we have to
   // overwrite it immediately. If the realloc below fails, this is still fine;
   // we're just using one function pointer's worth of memory for nothing.
   space->invalidatees = is;

   // We need to place the terminator immediately, though, since if this was
   // the first invalidatee and the realloc below fails, we'd leave
   // invalidatees as a nonnull but unterminated array.
   is[n] = NULL;

   id = realloc(id, n * sizeof *id);
   if (!id)
      return false;

   space->invalidatees_data = id;

   is[n-1] = i;
   id[n-1] = d;
   return true;
}
void mushspace_del_invalidatee(mushspace* spac, void* d) {
   size_t i = 0;
   void  **id         = spac->invalidatees_data;
   void (**is)(void*) = spac->invalidatees;

   for (; id[i] != d; ++i)
      assert (is[i]);

   if (is[i+1]) {
      size_t rest_len = i+1;
      while (is[rest_len])
         ++rest_len;

      memmove(id + i, id + i + 1,  rest_len      * sizeof *id);
      memmove(is + i, is + i + 1, (rest_len + 1) * sizeof *is);
   } else
      is[i] = NULL;

   if (i) {
      // Potential realloc failures don't matter; we'll just be using some
      // extra memory. The element has already been deleted (and the list
      // properly null-terminated) above.

      if ((id = realloc(id,  i    * sizeof *id))) spac->invalidatees_data = id;
      if ((is = realloc(is, (i+1) * sizeof *is))) spac->invalidatees      = is;
   } else {
      free(id);
      free(is);
      spac->invalidatees = NULL;
   }
}
