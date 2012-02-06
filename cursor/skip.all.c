// File created: 2012-01-28 01:17:40

#include "cursor/skip.all.h"

#include <assert.h>

#if !MUSHSPACE_93
#include "space/jump-to-box.98.h"
#endif

#ifdef MUSH_ENABLE_INFINITE_LOOP_DETECTION

#define INFLOOP_DECLS \
   mushcoords first_exit; \
   bool is_first_exit = true;

#define INFLOOP_DETECT(pos) do { \
   if (is_first_exit) { \
      is_first_exit = false; \
      first_exit = pos; \
   } else if (mushcoords_equal(pos, first_exit)) { \
      mushcursor_set_infloop_pos(cursor, pos); \
      return MUSHERR_INFINITE_LOOP_SPACES; \
   } \
} while (0)

#define INFLOOP_CHECK_DELTA do { \
   if (mushcoords_equal(delta, MUSHCOORDS(0,0,0))) \
      return MUSHERR_INFINITE_LOOP_SPACES; \
} while (0)

#else
#define INFLOOP_DECLS
#define INFLOOP_DETECT(pos)
#define INFLOOP_CHECK_DELTA
#endif

#if MUSHSPACE_93

#define FIND_BOX(cursor, delta, error_code) do { \
   INFLOOP_DETECT(cursor->rel_pos); \
   mushcursor93_wrap(cursor); \
} while (0)

#else

#define FIND_BOX(cursor, delta, error_code) do { \
   mushcoords pos = mushcursor_get_pos(cursor); \
   if (mushcursor_get_box(cursor, pos)) \
      continue; \
\
   INFLOOP_DETECT(pos); \
\
   if (!mushspace_jump_to_box(cursor->space, &pos, delta, &cursor->mode, \
                              &cursor->box, &cursor->box_idx)) \
   { \
      mushcursor_set_infloop_pos(cursor, pos); \
      return error_code; \
   } \
   mushcursor_tessellate(cursor, pos); \
} while (0)

#endif

static bool skip_spaces_here    (mushcursor*, mushcoords);
static bool skip_semicolons_here(mushcursor*, mushcoords, bool*);

int mushcursor_skip_markers(mushcursor* cursor, mushcoords delta) {
   INFLOOP_DECLS;
   INFLOOP_CHECK_DELTA;

   if (!mushcursor_in_box(cursor))
      goto find_box;

   const mushcell c = mushcursor_get_unsafe(cursor);
   if (c == ';')
      goto semicolon;
   else if (c != ' ')
      return MUSHERR_NONE;

   do {
      while (!skip_spaces_here(cursor, delta))
         find_box: FIND_BOX(cursor, delta, MUSHERR_INFINITE_LOOP_SPACES);

      if (mushcursor_get_unsafe(cursor) != ';')
         break;
semicolon:;

      bool in_mid = false;
      while (!skip_semicolons_here(cursor, delta, &in_mid))
         FIND_BOX(cursor, delta, MUSHERR_INFINITE_LOOP_SEMICOLONS);

   } while (mushcursor_get_unsafe(cursor) == ' ');

   assert (mushcursor_get_unsafe(cursor) != ' ');
   assert (mushcursor_get_unsafe(cursor) != ';');
   return MUSHERR_NONE;
}

int mushcursor_skip_spaces(mushcursor* cursor, mushcoords delta) {
   INFLOOP_DECLS;
   INFLOOP_CHECK_DELTA;

   if (!mushcursor_in_box(cursor))
      goto find_box;

   while (!skip_spaces_here(cursor, delta))
      find_box: FIND_BOX(cursor, delta, MUSHERR_INFINITE_LOOP_SPACES);

   assert (mushcursor_get(cursor) != ' ');
   return MUSHERR_NONE;
}

int mushcursor_skip_to_last_space(mushcursor* cursor, mushcoords delta) {
#if !MUSHSPACE_93
   mushcoords pos;
#endif
   INFLOOP_DECLS;
   INFLOOP_CHECK_DELTA;

   if (!mushcursor_in_box(cursor)) {
#if MUSHSPACE_93
      goto wrap;
#else
      // We should retreat only if we saw at least one space, so don't jump
      // into the loop just because we fell out of the box: that doesn't
      // necessarily mean a space.
      if (!mushcursor_get_box(cursor, pos = mushcursor_get_pos(cursor)))
         goto jump_to_box;
#endif
   }

   if (mushcursor_get_unsafe(cursor) != ' ')
      return MUSHERR_NONE;

   while (!skip_spaces_here(cursor, delta)) {
#if MUSHSPACE_93
wrap:
      INFLOOP_DETECT(cursor->rel_pos);
      mushcursor93_wrap(cursor);
#else
      if (mushcursor_get_box(cursor, pos = mushcursor_get_pos(cursor)))
         continue;

jump_to_box:
      INFLOOP_DETECT(pos);

      if (!mushspace_jump_to_box(cursor->space, &pos, delta, &cursor->mode,
                                 &cursor->box, &cursor->box_idx))
      {
         mushcursor_set_infloop_pos(cursor, pos);
         return MUSHERR_INFINITE_LOOP_SPACES;
      }
      mushcursor_tessellate(cursor, pos);
#endif
   }
   assert (mushcursor_get(cursor) != ' ');
   mushcursor_retreat(cursor, delta);
   assert (mushcursor_get(cursor) == ' ');
   return MUSHERR_NONE;
}

static bool skip_spaces_here(mushcursor* cursor, mushcoords delta) {
   assert (mushcursor_in_box(cursor));

   while (mushcursor_get_unsafe(cursor) == ' ') {
      mushcursor_advance(cursor, delta);
      if (!mushcursor_in_box(cursor))
         return false;
   }
   return true;
}

int mushcursor_skip_semicolons(mushcursor* cursor, mushcoords delta) {
   INFLOOP_DECLS;
   INFLOOP_CHECK_DELTA;

   if (!mushcursor_in_box(cursor))
      goto find_box;

   bool in_mid = false;
   while (!skip_semicolons_here(cursor, delta, &in_mid))
      find_box: FIND_BOX(cursor, delta, MUSHERR_INFINITE_LOOP_SEMICOLONS);
   assert (mushcursor_get(cursor) != ';');
   return MUSHERR_NONE;
}

static bool skip_semicolons_here(
   mushcursor* cursor, mushcoords delta, bool* in_mid)
{
   assert (mushcursor_in_box(cursor));

   if (*in_mid)
      goto continue_prev;

   while (mushcursor_get_unsafe(cursor) == ';') {
      do {
         mushcursor_advance(cursor, delta);
         if (!mushcursor_in_box(cursor)) {
            *in_mid = true;
            return false;
         }
continue_prev:;
      } while (mushcursor_get_unsafe(cursor) != ';');

      mushcursor_advance(cursor, delta);
      if (!mushcursor_in_box(cursor)) {
         *in_mid = false;
         return false;
      }
   }
   return true;
}
