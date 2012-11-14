// File created: 2011-09-02 23:36:23

#ifndef MUSHSPACE_CURSOR_H
#define MUSHSPACE_CURSOR_H

#include "space.all.h"

#define mushcursor MUSHSPACE_NAME(mushcursor)

#if MUSHSPACE_93
#define MUSHCURSOR_MODE(x) MushCursorMode_static
#else
#define MUSHCURSOR_MODE(x) ((x)->mode)
#endif

typedef struct mushcursor {
#if !MUSHSPACE_93
   MushCursorMode mode;
#endif
   mushspace *space;
   union {
      // For dynamic mode.
      struct {
         // For static mode (only rel_pos).
         mushcoords rel_pos;

#if !MUSHSPACE_93
         mushbounds rel_bounds;
         mushcoords obeg;
         mushaabb      *box;
         mushboxen_iter box_iter;
         void          *box_iter_aux;
         size_t         box_iter_aux_size;
#endif
      };
#if USE_BAKAABB
      // For bak mode.
      struct {
         mushcoords actual_pos;
         mushbounds actual_bounds;
      };
#endif
   };
} mushcursor;

#define mushcursor_sizeof          MUSHSPACE_CAT(mushcursor,_sizeof)
#define mushcursor_init            MUSHSPACE_CAT(mushcursor,_init)
#define mushcursor_free            MUSHSPACE_CAT(mushcursor,_free)
#define mushcursor_copy            MUSHSPACE_CAT(mushcursor,_copy)
#define mushcursor_get_pos         MUSHSPACE_CAT(mushcursor,_get_pos)
#define mushcursor_set_pos         MUSHSPACE_CAT(mushcursor,_set_pos)
#define mushcursor_get             MUSHSPACE_CAT(mushcursor,_get)
#define mushcursor_get_unsafe      MUSHSPACE_CAT(mushcursor,_get_unsafe)
#define mushcursor_put             MUSHSPACE_CAT(mushcursor,_put)
#define mushcursor_put_unsafe      MUSHSPACE_CAT(mushcursor,_put_unsafe)
#define mushcursor_advance         MUSHSPACE_CAT(mushcursor,_advance)
#define mushcursor_retreat         MUSHSPACE_CAT(mushcursor,_retreat)
#define mushcursor_in_box          MUSHSPACE_CAT(mushcursor,_in_box)
#define mushcursor_get_box         MUSHSPACE_CAT(mushcursor,_get_box)
#define mushcursor_tessellate      MUSHSPACE_CAT(mushcursor,_tessellate)
#define mushcursor_set_nowhere_pos MUSHSPACE_CAT(mushcursor,_set_nowhere_pos)

#if MUSHSPACE_93
void mushcursor93_wrap(mushcursor*);
#endif

extern const size_t mushcursor_sizeof;

int mushcursor_init(mushcursor**, mushspace*, mushcoords
#if !MUSHSPACE_93
                   , mushcoords
#endif
                   );
int mushcursor_copy(mushcursor**, const mushcursor*, mushspace*
#if !MUSHSPACE_93
                   , mushcoords
#endif
                   );
void mushcursor_free(mushcursor*);

mushcoords mushcursor_get_pos(const mushcursor*);
void       mushcursor_set_pos(      mushcursor*, mushcoords);

mushcell mushcursor_get       (mushcursor*);
mushcell mushcursor_get_unsafe(mushcursor*);
int      mushcursor_put       (mushcursor*, mushcell);
int      mushcursor_put_unsafe(mushcursor*, mushcell);

void mushcursor_advance(mushcursor*, mushcoords);
void mushcursor_retreat(mushcursor*, mushcoords);

bool mushcursor_in_box(const mushcursor*);

#if !MUSHSPACE_93
bool mushcursor_get_box(mushcursor*, mushcoords);

void mushcursor_tessellate(mushcursor*, mushcoords);
#endif

void mushcursor_set_nowhere_pos(mushcursor*, mushcoords);

#endif
