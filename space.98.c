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

   if (!(space->stats = stats ? stats : malloc(sizeof *space->stats))) {
      free(space);
      return NULL;
   }

   space->invalidatees = NULL;

   space->box_count = 0;
   space->boxen     = NULL;
   space->bak.data  = NULL;

   // Placate valgrind and such: it's not necessary to define these before the
   // first use.
   space->last_beg = space->last_end = MUSHCOORDS(0,0,0);

   mushmemorybuf_init(&space->recent_buf);

   mushcell_space(
      space->static_box.array, MUSH_ARRAY_LEN(space->static_box.array));
   return space;
}

void mushspace_free(mushspace* space) {
   for (size_t i = space->box_count; i--;)
      free(space->boxen[i].data);
   free(space->boxen);
   mushbakaabb_free(&space->bak);
   if (space->invalidatees) {
      free(space->invalidatees);
      free(space->invalidatees_data);
   }
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

   copy->stats = stats ? stats : malloc(sizeof *copy->stats);
   if (!copy->stats) {
      free(copy);
      return NULL;
   }

   memcpy(copy, space, sizeof *copy);

   copy->boxen = malloc(copy->box_count * sizeof *copy->boxen);
   memcpy(copy->boxen, space->boxen, copy->box_count * sizeof *copy->boxen);

   for (size_t i = 0; i < copy->box_count; ++i) {
      mushaabb *box = &copy->boxen[i];
      const mushcell *orig = box->data;
      box->data = malloc(box->size * sizeof *box->data);
      memcpy(box->data, orig, box->size * sizeof *box->data);
   }

   // Invalidatees refer to the original space, not the copy.
   copy->invalidatees = NULL;

   return copy;
}

mushcell mushspace_get(const mushspace* space, mushcoords c) {
   if (mushstaticaabb_contains(c))
      return mushstaticaabb_get(&space->static_box, c);

   const mushaabb* box;
   if ((box = mushspace_find_box(space, c)))
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
   if ((box = mushspace_find_box(space, p))
    || mushspace_place_box_for  (space, p, &box))
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

   for (size_t i = 0; i < space->box_count; ++i) {
      mushcoords_min_into(&bounds->beg, space->boxen[i].bounds.beg);
      mushcoords_max_into(&bounds->end, space->boxen[i].bounds.end);
   }
   if (space->bak.data) {
      mushcoords_min_into(&bounds->beg, space->bak.bounds.beg);
      mushcoords_max_into(&bounds->end, space->bak.bounds.end);
   }
}

int mushspace_map(mushspace* space, mushbounds bounds,
                   void(*f)(musharr_mushcell, void*), void* data)
{
   mushaabb aabb;
   mushaabb_make(&aabb, &bounds);
   if (!mushspace_place_box(space, &aabb, NULL, NULL))
      return MUSHERR_OOM;

   mushspace_map_no_place(space, &aabb, data, f, NULL);
   return MUSHERR_NONE;
}
void mushspace_map_existing(
   mushspace* space, mushbounds bounds,
   void(*f)(musharr_mushcell, void*), void(*g)(size_t, void*), void* data)
{
   mushaabb aabb;
   mushaabb_make(&aabb, &bounds);
   mushspace_map_no_place(space, &aabb, data, f, g);
}

void mushspace_invalidate_all(mushspace* space) {
   void (**i)(void*) = space->invalidatees;
   void  **d         = space->invalidatees_data;
   if (i)
      while (*i)
         (*i++)(*d++);
}

mushcaabb_idx mushspace_get_caabb_idx(const mushspace* sp, size_t i) {
   return (mushcaabb_idx){&sp->boxen[i], i};
}

mushaabb* mushspace_find_box(const mushspace* space, mushcoords c) {
   size_t i;
   return mushspace_find_box_and_idx(space, c, &i);
}

mushaabb* mushspace_find_box_and_idx(
   const mushspace* space, mushcoords c, size_t* pi)
{
   for (size_t i = 0; i < space->box_count; ++i)
      if (mushbounds_contains(&space->boxen[i].bounds, c))
         return &space->boxen[*pi = i];
   return NULL;
}

void mushspace_remove_boxes(mushspace* space, size_t i, size_t j) {
   assert (i <= j);
   assert (j < space->box_count);

   for (size_t k = i; k <= j; ++k)
      free(space->boxen[k].data);

   size_t new_len = space->box_count -= j - i + 1;
   if (i < new_len) {
      mushaabb *arr = space->boxen;
      memmove(arr + i, arr + j + 1, (new_len - i) * sizeof *arr);
   }
}

bool mushspace_add_invalidatee(mushspace* space, void(*i)(void*), void* d) {
   size_t n = 0;
   void (**is)(void*) = space->invalidatees;
   if (is)
      while (*is++)
         ++n;

   is = realloc(is, n * sizeof *is);
   if (!is)
      return false;

   void **id = realloc(space->invalidatees_data, n * sizeof *id);
   if (!id) {
      free(is);
      return false;
   }

   space->invalidatees      = is;
   space->invalidatees_data = id;

   is[n] = i;
   id[n] = d;
   return true;
}
bool mushspace_del_invalidatee(mushspace* space, void* d) {
   size_t i = 0;
   void  **id         = space->invalidatees_data;
   void (**is)(void*) = space->invalidatees;

   while (id[i] != d) {
      assert (is[i]);
      ++i;
   }

   if (is[i+1]) {
      size_t rest_len = i+1;
      while (is[rest_len])
         ++rest_len;

      memmove(id + i, id + i + 1, rest_len * sizeof *id);
      memmove(is + i, is + i + 1, rest_len * sizeof *is);
   }

   id = realloc(id, i * sizeof *id);
   if (i && !id)
      return false;

   is = realloc(is, i * sizeof *is);
   if (i && !is)
      return false;

   space->invalidatees      = is;
   space->invalidatees_data = id;
   return true;
}
