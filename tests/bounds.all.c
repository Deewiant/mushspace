// File created: 2012-10-07 20:19:29

#include <mush/bounds.h>

#include "coords.h"
#include "typenames.h"

#include "util/tap.h"

#ifndef MUSHSPACE_93
#define MUSHSPACE_93 0
#endif

#define mushcoords                 NAME(mushcoords)
#define mushbounds                 NAME(mushbounds)
#define mushbounds_contains        CAT(mushbounds,_contains)
#define mushbounds_safe_contains   CAT(mushbounds,_safe_contains)
#define mushbounds_contains_bounds CAT(mushbounds,_contains_bounds)
#define mushbounds_overlaps        CAT(mushbounds,_overlaps)

int main(void) {
   tap_n(5*2 + 5 + 3 + 4);

   bool (*f[])(const mushbounds*, mushcoords) =
      { mushbounds_contains, mushbounds_safe_contains };

   const mushbounds x =
#if MUSHSPACE_93
      { MUSHCOORDS_INIT(1,1,1), MUSHCOORDS_INIT(4,4,4) };
#define XS "[1,4]"
#else
      { MUSHCOORDS_INIT(-3,-3,-3), MUSHCOORDS_INIT(4,4,4) };
#define XS "[-3,4]"
#endif

   for (size_t i = 0; i < sizeof f / sizeof f[0]; ++i) {
      tap_bool(f[i](&x, MUSHCOORDS(2,2,2)),
               "2 is in " XS, "2 isn't in " XS);
      tap_bool(f[i](&x, x.beg),
               "-3 is in " XS, "-3 isn't in " XS);
      tap_bool(f[i](&x, x.end),
               "4 is in " XS, "4 isn't in " XS);
      tap_bool(!f[i](&x, MUSHCOORDS(5,5,5)),
               "5 isn't in " XS, "5 is in " XS);
#if MUSHSPACE_93
      tap_bool(!f[i](&x, MUSHCOORDS(0,0,0)),
               "0 isn't in " XS, "0 is in " XS);
#else
      tap_bool(!f[i](&x, MUSHCOORDS(-5,-5,-5)),
               "-5 isn't in " XS, "-5 is in " XS);
#endif
   }

   const mushbounds y = { x.end, x.beg };
#if MUSHSPACE_93
#define YS "[4,1]"
#else
#define YS "[4,-3]"
#endif

#if MUSHSPACE_93
   tap_bool(mushbounds_safe_contains(&y, MUSHCOORDS(0,0,0)),
            "0 is in " YS, "0 isn't in " YS);
#else
   tap_bool(mushbounds_safe_contains(&y, MUSHCOORDS(-4,-4,-4)),
            "-4 is in " YS, "-4 isn't in " YS);
#endif
   tap_bool(mushbounds_safe_contains(&y, MUSHCOORDS(5,5,5)),
            "5 is in " YS, "5 isn't in " YS);
   tap_bool(mushbounds_safe_contains(&y, y.beg),
            "4 is in " YS, "4 isn't in " YS);
   tap_bool(mushbounds_safe_contains(&y, y.end),
#if MUSHSPACE_93
            "1 is in " YS, "1 isn't in " YS
#else
            "-3 is in " YS, "-3 isn't in " YS
#endif
   );
   tap_bool(!mushbounds_safe_contains(&y, MUSHCOORDS(2,2,2)),
            "2 isn't in " YS, "2 is in " YS);

   const mushbounds
      z = { MUSHCOORDS_INIT( 5, 5, 5), MUSHCOORDS_INIT(8,8,8) },
#if MUSHSPACE_93
      w = { MUSHCOORDS_INIT( 1, 1, 1), MUSHCOORDS_INIT(3,3,3) };
#define WS "[0,1]"
#else
      w = { MUSHCOORDS_INIT(-2,-2,-2), MUSHCOORDS_INIT(1,1,1) };
#define WS "[-2,1]"
#endif

   tap_bool(!mushbounds_contains_bounds(&x, &z),
            "[5,8] isn't in " XS, "[5,8] is in " XS);
   tap_bool(!mushbounds_contains_bounds(&w, &x),
            XS " isn't in " WS, XS " is in " WS);
   tap_bool(mushbounds_contains_bounds(&x, &w),
            WS " is in " XS, WS " isn't in " XS);

   tap_bool(mushbounds_overlaps(&x, &w),
            XS " and " WS " overlap", XS " and " WS " don't overlap");
   tap_bool(!mushbounds_overlaps(&x, &z),
            XS " and [5,8] don't overlap", XS " and [5,8] overlap");
   tap_bool(
      mushbounds_overlaps(&w, &x),
      "mushbounds_overlaps commutes when true",
      "mushbounds_overlaps doesn't commute when true (" WS " and " XS ")");
   tap_bool(
      !mushbounds_overlaps(&z, &x),
      "mushbounds_overlaps commutes when false",
      "mushbounds_overlaps doesn't commute when false ([5,8] and " XS ")");
}
