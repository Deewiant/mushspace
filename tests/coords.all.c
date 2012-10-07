// File created: 2012-10-07 12:10:39

#include "coords.h"
#include "typenames.h"

#include "util/tap.h"

#define mushcoords          NAME(mushcoords)
#define mushcoords_add      CAT(mushcoords,_add)
#define mushcoords_sub      CAT(mushcoords,_sub)
#define mushcoords_max_into CAT(mushcoords,_max_into)
#define mushcoords_min_into CAT(mushcoords,_min_into)
#define mushcoords_equal    CAT(mushcoords,_equal)

int main(void) {
   tap_n(1 + MUSHSPACE_DIM + 5);

   tap_bool(mushcoords_equal(MUSHCOORDS(1,2,3), MUSHCOORDS(1,2,3)),
            "(1,2,3) == (1,2,3)", "(1,2,3) != (1,2,3)");

   tap_bool(!mushcoords_equal(MUSHCOORDS(1,2,3), MUSHCOORDS(0,2,3)),
            "(1,2,3) != (0,2,3)", "(1,2,3) == (0,2,3)");
#if MUSHSPACE_DIM >= 2
   tap_bool(!mushcoords_equal(MUSHCOORDS(1,2,3), MUSHCOORDS(1,0,3)),
            "(1,2,3) != (1,0,3)", "(1,2,3) == (1,0,3)");
#if MUSHSPACE_DIM >= 3
   tap_bool(!mushcoords_equal(MUSHCOORDS(1,2,3), MUSHCOORDS(1,2,0)),
            "(1,2,3) != (1,2,0)", "(1,2,3) == (1,2,0)");
#endif
#endif

   mushcoords x = MUSHCOORDS_INIT(4,5,6);

   tap_bool(mushcoords_equal(x, MUSHCOORDS(4,5,6)),
            "MUSHCOORDS agrees with MUSHCOORDS_INIT",
            "MUSHCOORDS doesn't agree with MUSHCOORDS_INIT");

   tap_eqco(mushcoords_add(MUSHCOORDS(1,2,3), MUSHCOORDS(4,5,6)),
            MUSHCOORDS(5,7,9));

   tap_eqco(mushcoords_sub(MUSHCOORDS(1,2,3), MUSHCOORDS(6,5,4)),
            MUSHCOORDS(-5,-3,-1));

   mushcoords_max_into(&x, MUSHCOORDS(5,3,7));
   tap_eqcos(x, MUSHCOORDS(5,5,7),
             "max((4,5,6), (5,3,7)) == (5,5,7)",
             "max((4,5,6), (5,3,7)) != (5,5,7)");

   x = MUSHCOORDS(4,5,6);
   mushcoords_min_into(&x, MUSHCOORDS(2,3,7));
   tap_eqcos(x, MUSHCOORDS(2,3,6),
             "min((4,5,6), (2,3,7)) == (2,3,6)",
             "min((4,5,6), (2,3,7)) != (2,3,6)");
}
