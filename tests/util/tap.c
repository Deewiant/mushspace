// File created: 2012-10-06 18:52:39

#include "tap.h"

static int tap_cur = 0, tap_max;

void tap_n(int n) { printf("1..%d\n", tap_max = n); }

void tap_ok    (const char* s) { printf(    "ok %d - %s\n", ++tap_cur, s); }
void tap_not_ok(const char* s) { printf("not ok %d - %s\n", ++tap_cur, s); }
void tap_skip  (const char* s) { printf(  "skip %d - %s\n", ++tap_cur, s); }

void tap_skip_remaining(const char* s) {
   while (tap_cur < tap_max)
      tap_skip(s);
}

void tap_bool(bool b, const char* so, const char* sn) {
   if (b)
      tap_ok(so);
   else
      tap_not_ok(sn);
}

#define tap_eqis_gen(N, T, P) \
   void N(T a, T b, const char* so, const char* sn) { \
      if (a == b) { \
         tap_ok(so); \
         return; \
      } \
      tap_not_ok(sn); \
      printf("  ---\n" \
             "  first  was: %" P "\n" \
             "  second was: %" P "\n" \
             "  ...\n", a, b); \
   }

tap_eqis_gen(tap_eqcs,   mushcell,   MUSHCELL_PRI)
tap_eqis_gen(tap_eqc93s, mushcell93, MUSHCELL93_PRI)

#define tap_opivs_gen(N, op, T, P) \
   void N(const T* a, const T* b, uint8_t d, const char* so, const char* sn) {\
      for (uint8_t i = 0; i < d; ++i) { \
         if (a[i] op b[i]) \
            continue; \
         tap_not_ok(sn); \
         printf("  ---\n" \
                "  first  pos %" PRIu8 ": %" P "\n" \
                "  second pos %" PRIu8 ": %" P "\n" \
                "  ...\n", i, a[i], i, b[i]); \
         return; \
      } \
      tap_ok(so); \
   }

tap_opivs_gen(tap_eqcvs,    ==, mushcell,   MUSHCELL_PRI)
tap_opivs_gen(tap_leqcvs,   <=, mushcell,   MUSHCELL_PRI)
tap_opivs_gen(tap_geqcvs,   >=, mushcell,   MUSHCELL_PRI)
tap_opivs_gen(tap_eqc93vs,  ==, mushcell93, MUSHCELL93_PRI)
tap_opivs_gen(tap_leqc93vs, <=, mushcell93, MUSHCELL93_PRI)
tap_opivs_gen(tap_geqc93vs, >=, mushcell93, MUSHCELL93_PRI)
