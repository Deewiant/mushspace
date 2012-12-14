// File created: 2011-09-02 23:36:15

#include "cursor.all.h"

#include <alloca.h>
#include <assert.h>
#include <string.h>

#if !MUSHSPACE_93
#include "space/jump-to-box.98.h"
#endif

#if MUSHSPACE_93
#define STATIC_BOX(sp) (&(sp)->box)
#else
#define STATIC_BOX(sp) (&(sp)->static_box)
#endif

#ifdef MUSH_ENABLE_EXPENSIVE_DEBUGGING
#define DEBUG_CHECK(cursor, c) \
   assert (mushspace_get(cursor->space, mushcursor_get_pos(cursor)) == c)
#else
#define DEBUG_CHECK(cursor, c)
#endif

#define BAD_CURSOR_MODE MUSH_UNREACHABLE("invalid cursor mode")

#if !MUSHSPACE_93
static bool mushcursor_recalibrate(void*);
#endif

const size_t mushcursor_sizeof = sizeof(mushcursor);

mushcursor* mushcursor_init(void* vp, mushspace* space, mushcoords pos) {
   mushcursor *cursor;
   if (vp)
      cursor = vp;
   else if (!(cursor = malloc(sizeof *cursor)))
      goto fail;

#if !MUSHSPACE_93
   cursor->box_iter_aux_size = mushboxen_iter_aux_size_init;
   cursor->box_iter_aux = malloc(cursor->box_iter_aux_size);
   if (!cursor->box_iter_aux && cursor->box_iter_aux_size)
      goto fail_freecursor;

   if (!mushspace_add_invalidatee(space, mushcursor_recalibrate, cursor))
      goto fail_freeaux;
#endif

   cursor->space = space;

#if MUSHSPACE_93
   cursor->rel_pos = mushcoords_sub(pos, MUSHSTATICAABB_BEG);
#else
   if (!mushcursor_get_box(cursor, pos))
      mushcursor_set_nowhere_pos(cursor, pos);
#endif
   return cursor;

#if !MUSHSPACE_93
fail_freeaux:
   free(cursor->box_iter_aux);
fail_freecursor:
   if (!vp)
      free(cursor);
#endif
fail:
   return NULL;
}

void mushcursor_free(mushcursor* cursor) {
#if MUSHSPACE_93
   (void)cursor;
#else
   free(cursor->box_iter_aux);
   mushspace_del_invalidatee(cursor->space, cursor);
#endif
}

mushcursor* mushcursor_copy(
   void* vp, const mushcursor* cursor, mushspace* space)
{
   mushcursor *copy;
   if (vp)
      copy = vp;
   else if (!(copy = malloc(sizeof *copy)))
      goto fail;

   memcpy(copy, cursor, sizeof *copy);

   if (space)
      copy->space = space;

#if !MUSHSPACE_93
   copy->box_iter_aux = malloc(copy->box_iter_aux_size);
   if (!copy->box_iter_aux && copy->box_iter_aux_size)
      goto fail_freecopy;

   if (!mushspace_add_invalidatee(copy->space, mushcursor_recalibrate, copy))
      goto fail_freeaux;
#endif

#if !MUSHSPACE_93
   // We assume that cursor was already in a valid state, so we don't need to
   // fix the position if the space doesn't change.
   if (copy->space != cursor->space)
      mushcursor_recalibrate(copy);
#endif

   return copy;

#if !MUSHSPACE_93
fail_freeaux:
   free(copy->box_iter_aux);
fail_freecopy:
   if (!vp)
      free(copy);
#endif
fail:
   return NULL;
}

mushcoords mushcursor_get_pos(const mushcursor* cursor) {
   switch (MUSHCURSOR_MODE(cursor)) {
   case MushCursorMode_static:
      return mushcoords_add(cursor->rel_pos, MUSHSTATICAABB_BEG);
#if !MUSHSPACE_93
   case MushCursorMode_dynamic:
      return mushcoords_add(cursor->rel_pos, cursor->obeg);
#if USE_BAKAABB
   case MushCursorMode_bak:
      return cursor->actual_pos;
#endif
#endif
   }
   BAD_CURSOR_MODE;
}

void mushcursor_set_pos(mushcursor* cursor, mushcoords pos) {
   switch (MUSHCURSOR_MODE(cursor)) {
   case MushCursorMode_static:
      cursor->rel_pos = mushcoords_sub(pos, MUSHSTATICAABB_BEG);
      return;
#if !MUSHSPACE_93
   case MushCursorMode_dynamic:
      cursor->rel_pos = mushcoords_sub(pos, cursor->obeg);
      return;
#if USE_BAKAABB
   case MushCursorMode_bak:
      cursor->actual_pos = pos;
      return;
#endif
#endif
   }
   BAD_CURSOR_MODE;
}

bool mushcursor_in_box(const mushcursor* cursor) {
   switch (MUSHCURSOR_MODE(cursor)) {
   case MushCursorMode_static:
      return mushbounds_contains(&MUSHSTATICAABB_REL_BOUNDS, cursor->rel_pos);

#if !MUSHSPACE_93
   case MushCursorMode_dynamic:
      return mushbounds_contains(&cursor->rel_bounds, cursor->rel_pos);

#if USE_BAKAABB
   case MushCursorMode_bak:
      return mushbounds_contains(&cursor->actual_bounds, cursor->actual_pos);
#endif
#endif
   }
   BAD_CURSOR_MODE;
}

#if !MUSHSPACE_93
bool mushcursor_get_box(mushcursor* cursor, mushcoords pos) {
   if (mushstaticaabb_contains(pos)) {
      cursor->mode = MushCursorMode_static;
      mushcursor_tessellate(cursor, pos);
      return true;
   }

   mushspace *sp = cursor->space;

   cursor->box_iter =
      mushboxen_get_iter(&sp->boxen, pos, cursor->box_iter_aux);
   if (!mushboxen_iter_is_null(cursor->box_iter)) {
      cursor->box  = mushboxen_iter_box(cursor->box_iter);
      cursor->mode = MushCursorMode_dynamic;
      mushcursor_tessellate(cursor, pos);
      return true;
   }
#if USE_BAKAABB
   if (sp->bak.data && mushbounds_contains(&sp->bak.bounds, pos)) {
      cursor->mode = MushCursorMode_bak;
      mushcursor_tessellate(cursor, pos);
      return true;
   }
#endif
   return false;
}
#endif

mushcell mushcursor_get(mushcursor* cursor) {
#if !MUSHSPACE_93
   if (!mushcursor_in_box(cursor)
    && !mushcursor_get_box(cursor, mushcursor_get_pos(cursor)))
   {
      DEBUG_CHECK(cursor, ' ');
      return ' ';
   }
#endif
   return mushcursor_get_unsafe(cursor);
}
mushcell mushcursor_get_unsafe(mushcursor* cursor) {
   assert (mushcursor_in_box(cursor));

   mushspace *sp = cursor->space;

   mushcell c;

   switch (MUSHCURSOR_MODE(cursor)) {
   case MushCursorMode_static:
      c = mushstaticaabb_get_no_offset(STATIC_BOX(sp), cursor->rel_pos);
      break;

#if !MUSHSPACE_93
   case MushCursorMode_dynamic:
      c = mushaabb_get_no_offset(cursor->box, cursor->rel_pos);
      break;

#if USE_BAKAABB
   case MushCursorMode_bak:
      c = mushbakaabb_get(&sp->bak, cursor->actual_pos);
      break;
#endif
#endif

   default: BAD_CURSOR_MODE;
   }
   DEBUG_CHECK(cursor, c);
   return c;
}

void mushcursor_put(mushcursor* cursor, mushcell c) {
#if !MUSHSPACE_93
   if (!mushcursor_in_box(cursor)) {
      mushcoords pos = mushcursor_get_pos(cursor);
      if (!mushcursor_get_box(cursor, pos)) {
         mushspace_put(cursor->space, pos, c);
         DEBUG_CHECK(cursor, c);
         return;
      }
   }
#endif
   mushcursor_put_unsafe(cursor, c);
}

void mushcursor_put_unsafe(mushcursor* cursor, mushcell c) {
   assert (mushcursor_in_box(cursor));

   mushspace *sp = cursor->space;

   switch (MUSHCURSOR_MODE(cursor)) {
   case MushCursorMode_static:
      mushstaticaabb_put_no_offset(STATIC_BOX(sp), cursor->rel_pos, c);
      break;

#if !MUSHSPACE_93
   case MushCursorMode_dynamic:
      mushaabb_put_no_offset(cursor->box, cursor->rel_pos, c);
      break;

#if USE_BAKAABB
   case MushCursorMode_bak:
      mushbakaabb_put(&sp->bak, cursor->actual_pos, c);
      break;
#endif
#endif

   default: BAD_CURSOR_MODE;
   }
   DEBUG_CHECK(cursor, c);
}

void mushcursor_advance(mushcursor* cursor, mushcoords delta) {
#if USE_BAKAABB
   if (MUSHCURSOR_MODE(cursor) == MushCursorMode_bak)
      mushcoords_add_into(&cursor->actual_pos, delta);
   else
#endif
      mushcoords_add_into(&cursor->rel_pos, delta);
}

void mushcursor_retreat(mushcursor* cursor, mushcoords delta) {
#if USE_BAKAABB
   if (MUSHCURSOR_MODE(cursor) == MushCursorMode_bak)
      mushcoords_sub_into(&cursor->actual_pos, delta);
   else
#endif
      mushcoords_sub_into(&cursor->rel_pos, delta);
}

#if !MUSHSPACE_93
static bool mushcursor_recalibrate(void* p) {
   mushcursor *cursor = p;

   const size_t aux_size = mushboxen_iter_aux_size(&cursor->space->boxen);
   if (aux_size > cursor->box_iter_aux_size) {
      void *aux = realloc(cursor->box_iter_aux, aux_size);
      if (!aux)
         return false;
      cursor->box_iter_aux      = aux;
      cursor->box_iter_aux_size = aux_size;
   }

   mushcoords pos = mushcursor_get_pos(cursor);
   if (!mushcursor_get_box(cursor, pos))
      mushcursor_set_nowhere_pos(cursor, pos);
   return true;
}
#endif

#if MUSHSPACE_93
void mushcursor93_wrap(mushcursor* cursor) {
   cursor->rel_pos.x %= MUSHSTATICAABB_SIZE.x;
   cursor->rel_pos.y %= MUSHSTATICAABB_SIZE.y;
}
#else
void mushcursor_tessellate(mushcursor* cursor, mushcoords pos) {
   mushspace *sp = cursor->space;

   void *aux = alloca(mushboxen_iter_aux_size(&sp->boxen));

   switch (MUSHCURSOR_MODE(cursor)) {
   case MushCursorMode_static:
      cursor->rel_pos = mushcoords_sub(pos, MUSHSTATICAABB_BEG);
      break;

#if USE_BAKAABB
   case MushCursorMode_bak:
      cursor->actual_pos    = pos;
      cursor->actual_bounds = sp->bak.bounds;

      // bak is the lowest, so we tessellate with all boxes.
      mushbounds_tessellate(&cursor->actual_bounds, pos,
                            &MUSHSTATICAABB_BOUNDS);

      for (mushboxen_iter it = mushboxen_iter_init(&sp->boxen, aux);
           !mushboxen_iter_done( it, &sp->boxen);
            mushboxen_iter_next(&it, &sp->boxen))
      {
         mushbounds_tessellate(&cursor->actual_bounds, pos,
                               &mushboxen_iter_box(it)->bounds);
      }
      break;
#endif

   case MushCursorMode_dynamic: {
      // cursor->box now becomes only a view. it shares its data array with the
      // original box, but has different bounds. In addition, it is weird: its
      // width and height are not its own, so that index calculation in the
      // _no_offset functions works correctly.
      //
      // BE CAREFUL! Only the *_no_offset functions work properly on it, since
      // the others (notably, _get_idx and thereby _get and _put) tend to
      // depend on the bounds matching the data and the width/height being
      // sensible.

      mushbounds bounds = cursor->box->bounds;
      cursor->obeg = bounds.beg;

      // Here we need to tessellate only with the boxes above cursor->box.
      mushbounds_tessellate(&bounds, pos, &MUSHSTATICAABB_BOUNDS);
      for (mushboxen_iter_above it =
              mushboxen_iter_above_init(&sp->boxen, cursor->box_iter, aux);
           !mushboxen_iter_above_done( it, &sp->boxen);
            mushboxen_iter_above_next(&it, &sp->boxen))
      {
         mushbounds_tessellate(
            &bounds, pos, &mushboxen_iter_above_box(it)->bounds);
      }

      cursor->rel_pos    = mushcoords_sub(pos, cursor->obeg);
      cursor->rel_bounds =
         (mushbounds){mushcoords_sub(bounds.beg, cursor->obeg),
                      mushcoords_sub(bounds.end, cursor->obeg)};
      break;
   }

   default: BAD_CURSOR_MODE;
   }
}
#endif

void mushcursor_set_nowhere_pos(mushcursor* cursor, mushcoords pos) {
#if !MUSHSPACE_93
   // Since we are "nowhere", we can set an arbitrary mode: any functionality
   // that cares about the mode handles the not-in-a-box case anyway. Prefer
   // static because it's the fastest to work with.
   cursor->mode = MushCursorMode_static;
#endif
   mushcursor_set_pos(cursor, pos);
}
