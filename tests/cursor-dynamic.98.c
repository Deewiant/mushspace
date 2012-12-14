// File created: 2012-10-30 18:01:21

#include <mush/cursor.h>
#include <mush/err.h>
#include <mush/space.h>

#include "coords.h"
#include "typenames.h"

#include "util/tap.h"

#define mushcoords                 NAME(mushcoords)
#define mushspace                  NAME(mushspace)
#define mushcursor                 NAME(mushcursor)
#define mushbounds                 NAME(mushbounds)
#define mushspace_init             CAT(mushspace,_init)
#define mushspace_free             CAT(mushspace,_free)
#define mushcursor_init            CAT(mushcursor,_init)
#define mushcursor_free            CAT(mushcursor,_free)
#define mushcursor_get             CAT(mushcursor,_get)
#define mushcursor_get_pos         CAT(mushcursor,_get_pos)
#define mushcursor_put             CAT(mushcursor,_put)
#define mushcursor_advance         CAT(mushcursor,_advance)
#define mushcursor_retreat         CAT(mushcursor,_retreat)
#define mushspace_get_tight_bounds CAT(mushspace,_get_tight_bounds)
#define mushcoords_sub             CAT(mushcoords,_sub)
#define mushcoords_add             CAT(mushcoords,_add)
#define mushspace_load_string      CAT(mushspace,_load_string)

int main(void) {
   tap_n(1 + 5*2 + 1 + 3 + 2*(2+3)+(2+1)+2 + 1 + 1+2*2+1 + 1+2*2+1 + 1);

   static const mushcoords beg   = MUSHCOORDS_INIT(1000000,1000000,1000000),
                           delta = MUSHCOORDS_INIT(1,1,1);

   mushspace *space = mushspace_init(NULL, NULL);
   if (!space) {
      tap_not_ok("space_init returned null");
      tap_skip_remaining("space_init failed");
      return 1;
   }
   tap_ok("space_init succeeded");

   mushcursor *cursor = mushcursor_init(NULL, space, beg, MUSHCOORDS(0,0,0));

   #define gpg(g, p) do { \
      tap_eqc(mushcursor_get(cursor), g); \
      mushcursor_put(cursor, p); \
      tap_eqc(mushcursor_get(cursor), p); \
   } while (0)

   // Basic back-and-forthing. The end result should be that we're back at beg
   // with -1 behind us, 0 on us, and 1 in front of us.

   gpg(' ', 10);
   mushcursor_advance(cursor, delta);
   gpg(' ', 1);
   mushcursor_retreat(cursor, delta);
   gpg(10, 12);
   mushcursor_retreat(cursor, delta);
   gpg(' ', -1);
   mushcursor_advance(cursor, delta);
   gpg(12, 0);

   tap_eqcos(mushcursor_get_pos(cursor), beg,
             "cursor is back where it started",
             "cursor is not back where it started");

   // Check that the tight bounds are currently correct.

   #define tight(empty, ebeg, eend) do { \
      mushbounds bs; \
      if (empty) \
         tap_bool(!mushspace_get_tight_bounds(space, &bs), \
                  "get_tight_bounds says that the space is empty", \
                  "get_tight_bounds says that the space is nonempty"); \
      else { \
         tap_bool(mushspace_get_tight_bounds(space, &bs), \
                  "get_tight_bounds says that the space is nonempty", \
                  "get_tight_bounds says that the space is empty"); \
         tap_eqcos(bs.beg, ebeg, "get_tight_bounds reports correct beg", \
                                 "get_tight_bounds reports incorrect beg"); \
         tap_eqcos(bs.end, eend, "get_tight_bounds reports correct end", \
                                 "get_tight_bounds reports incorrect end"); \
      } \
   } while (0)

   tight(false, mushcoords_sub(beg, delta), mushcoords_add(beg, delta));

   // Clear the space from left to right, and make sure it remains empty
   // afterwards by checking from right to left. The end result should be an
   // empty space with the cursor back at beg.

   #define clear(ecell, empty, ebeg, eend) do { \
      gpg(ecell, ' '); \
      tight(empty, ebeg, eend); \
   } while (0)

   mushcursor_retreat(cursor, delta);
   clear(-1, false, beg, mushcoords_add(beg, delta));
   mushcursor_advance(cursor, delta);
   clear(0, false, mushcoords_add(beg, delta), mushcoords_add(beg, delta));
   mushcursor_advance(cursor, delta);
   clear(1, true, beg, beg);
   mushcursor_retreat(cursor, delta);
   tap_eqc(mushcursor_get(cursor), ' ');
   mushcursor_retreat(cursor, delta);
   tap_eqc(mushcursor_get(cursor), ' ');
   mushcursor_advance(cursor, delta);

   tap_eqcos(mushcursor_get_pos(cursor), beg,
             "cursor is back where it started",
             "cursor is not back where it started");

   // Create a tiny box (one with volume 1), check that its value is seen by
   // the cursor, put something next to it via the cursor, and check that both
   // remain correct. End at (beg - delta).

   static const unsigned char tiny[] = "x";
   mushspace_load_string(space, tiny, 1, NULL, beg, false);

   tap_eqc(mushcursor_get(cursor), 'x');
   mushcursor_retreat(cursor, delta);
   gpg(' ', 'Y');
   mushcursor_advance(cursor, delta);
   gpg('x', 'z');
   mushcursor_retreat(cursor, delta);
   tap_eqc(mushcursor_get(cursor), 'Y');

   // Create a box overlapping both of the locations allocated above (volume
   // 2^dim), check that they were both overwritten correctly, and that they
   // can be overwritten correctly. End at beg.
   //
   // The string is such that the value overwriting the second position (beg)
   // is '0' + MUSHSPACE_DIM.

   static const unsigned char huge[] = "01\nv2\r\n\fpq\rr3";
   mushspace_load_string(space, huge, 13,
                         NULL, mushcoords_sub(beg, delta), false);

   tap_eqc(mushcursor_get(cursor), '0');
   mushcursor_advance(cursor, delta);
   gpg('0' + MUSHSPACE_DIM, 'k');
   mushcursor_retreat(cursor, delta);
   gpg('0', 'o');
   mushcursor_advance(cursor, delta);
   tap_eqc(mushcursor_get(cursor), 'k');

   // Check that we are where we expect ourselves to be.

   tap_eqcos(mushcursor_get_pos(cursor), beg,
             "cursor's final position is where it started",
             "cursor's final position is not where it started");

   mushcursor_free(cursor);
   free(cursor);
   mushspace_free(space);
   free(space);
}
