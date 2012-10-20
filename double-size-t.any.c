// File created: 2012-10-20 20:34:19

#include "double-size-t.any.h"

#include <limits.h>
#include <stdint.h>

void mush_double_size_t_add_into(mush_double_size_t* a, mush_double_size_t b) {
   a->hi += b.hi;
   if (a->lo > SIZE_MAX - b.lo)
      ++a->hi;
   a->lo += b.lo;
}

void mush_double_size_t_sub1_into(mush_double_size_t* a, size_t b) {
   if (a->lo < b)
      --a->hi;
   a->lo -= b;
}

void mush_double_size_t_mul1_into(mush_double_size_t* a, size_t b) {
   a->hi *= b;

   const size_t SHIFT = sizeof(size_t) * CHAR_BIT / 2,
                BITS  = ((size_t)1 << SHIFT) - 1;

   // All are half of size_t in size.
   size_t lo1 = a->lo >> SHIFT,
          lo0 = a->lo &  BITS,
          b1  = b     >> SHIFT,
          b0  = b     &  BITS;

   // (lo1 shift + lo0) * (b1 shift + b0) ==
   // lo1 b1 shift shift + lo1 shift b0 + lo0 b1 shift + lo0 b0

   a->hi += lo1 * b1;

   const size_t n = lo1 * b0;
   const size_t m = lo0 * b1;
   a->hi += n >> SHIFT;
   a->hi += m >> SHIFT;

   a->lo = lo0 * b0;
   mush_double_size_t_add_into(a, (mush_double_size_t){0, n << SHIFT});
   mush_double_size_t_add_into(a, (mush_double_size_t){0, m << SHIFT});
}
