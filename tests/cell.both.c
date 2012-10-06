// File created: 2012-10-06 18:41:51

#include "tap/tap.h"

#if MUSHSPACE_93
#undef MUSHCELL_MIN
#undef MUSHCELL_MAX
#define MUSHCELL_MIN MUSHCELL93_MIN
#define MUSHCELL_MAX MUSHCELL93_MAX
#define mushcell_add mushcell93_add
#define mushcell_sub mushcell93_sub
#define mushcell_mul mushcell93_mul
#endif

int main(void) {
   tap_n(13);

   tap_eqc(mushcell_add(MUSHCELL_MAX, 1), MUSHCELL_MIN);
   tap_eqc(mushcell_sub(MUSHCELL_MIN, 1), MUSHCELL_MAX);
   tap_eqc(mushcell_mul(MUSHCELL_MAX, 11), MUSHCELL_MAX - 10);
   tap_eqc(mushcell_add(MUSHCELL_MAX, 1), mushcell_inc(MUSHCELL_MAX));
   tap_eqc(mushcell_sub(MUSHCELL_MIN, 1), mushcell_dec(MUSHCELL_MIN));

   mushcell x;

   x = MUSHCELL_MAX;
   mushcell_add_into(&x, 1);
   tap_eqcs(x, mushcell_add(MUSHCELL_MAX, 1),
            "mushcell_add_into() matches mushcell_add()",
            "mushcell_add_into() doesn't match mushcell_add()");

   x = MUSHCELL_MIN;
   mushcell_sub_into(&x, 1);
   tap_eqcs(x, mushcell_sub(MUSHCELL_MIN, 1),
            "mushcell_sub_into() matches mushcell_sub()",
            "mushcell_sub_into() doesn't match mushcell_sub()");

   tap_eqc(mushcell_max(MUSHCELL_MAX, MUSHCELL_MIN), MUSHCELL_MAX);
   tap_eqc(mushcell_max(MUSHCELL_MIN, MUSHCELL_MAX), MUSHCELL_MAX);
   tap_eqc(mushcell_min(MUSHCELL_MIN, MUSHCELL_MAX), MUSHCELL_MIN);
   tap_eqc(mushcell_min(MUSHCELL_MAX, MUSHCELL_MIN), MUSHCELL_MIN);

   x = MUSHCELL_MIN;
   mushcell_max_into(&x, MUSHCELL_MAX);
   tap_eqcs(x, mushcell_max(MUSHCELL_MIN, MUSHCELL_MAX),
            "mushcell_max_into() matches mushcell_max()",
            "mushcell_max_into() doesn't match mushcell_max()");

   x = MUSHCELL_MAX;
   mushcell_min_into(&x, MUSHCELL_MIN);
   tap_eqcs(x, mushcell_min(MUSHCELL_MAX, MUSHCELL_MIN),
            "mushcell_min_into() matches mushcell_min()",
            "mushcell_min_into() doesn't match mushcell_min()");
}
