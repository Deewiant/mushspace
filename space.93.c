// File created: 2011-08-14 00:10:39

#include "space.all.h"

#include <assert.h>
#include <string.h>

#include "stdlib.any.h"

mushspace* mushspace_init(void* vp) {
   mushspace *space = vp ? vp : malloc(sizeof *space);
   if (!space)
      return NULL;

   mushcell_space(space->box.array, MUSH_ARRAY_LEN(space->box.array));

#if MUSH_CAN_SIGNAL
   space->signal = musherr_default_handler;
#endif
   return space;
}

void mushspace_free(mushspace* space) { (void)space; }

mushspace* mushspace_copy(void* vp, const mushspace* space) {
   mushspace *copy = vp ? vp : malloc(sizeof *copy);
   if (!copy)
      return NULL;

   memcpy(copy, space, sizeof *copy);
   return copy;
}

mushcell mushspace_get(const mushspace* space, mushcoords c) {
   return mushstaticaabb_contains(c) ? mushstaticaabb_get(&space->box, c)
                                     : ' ';
}

void mushspace_put(mushspace* space, mushcoords p, mushcell c) {
   if (mushstaticaabb_contains(p))
      mushstaticaabb_put(&space->box, p, c);
}

void mushspace_get_loose_bounds(const mushspace* space, mushbounds* bounds) {
   (void)space;
   bounds->beg = MUSHSTATICAABB_BEG;
   bounds->end = MUSHSTATICAABB_END;
}
