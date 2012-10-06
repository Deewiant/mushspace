// File created: 2012-10-06 18:52:39

#include "tap.h"

static int tap_cur = 0;

void tap_n(int n) { printf("1..%d\n", n); }

void tap_ok    (const char* s) { printf(    "ok %d - %s\n", ++tap_cur, s); }
void tap_not_ok(const char* s) { printf("not ok %d - %s\n", ++tap_cur, s); }

void tap_bool(bool b, const char* so, const char* sn) {
   if (b)
      tap_ok(so);
   else
      tap_not_ok(sn);
}

#define tap_eqis_gen(N, T) \
   void N(T a, T b, const char* so, const char* sn) { \
      if (a == b) { \
         tap_ok(so); \
         return; \
      } \
      tap_not_ok(sn); \
      printf("  ---\n" \
             "  first  was: %lld\n" \
             "  second was: %lld\n" \
             "  ...\n", (long long)a, (long long)b); \
   }

tap_eqis_gen(tap_eqcs,   mushcell)
tap_eqis_gen(tap_eqc93s, mushcell93)
