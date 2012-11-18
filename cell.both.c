// File created: 2011-08-09 19:22:21

#include "cell.both.h"

#include <assert.h>

#if !MUSHSPACE_93
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define GNU_X86_ASM 1
#else
#define GNU_X86_ASM 0
#endif

static mushucell mushucell_mod_inv(mushucell);
#endif

mushcell mushcell_max(mushcell a, mushcell b) { return a > b ? a : b; }
mushcell mushcell_min(mushcell a, mushcell b) { return a < b ? a : b; }

void mushcell_max_into(mushcell* a, mushcell b) { *a = mushcell_max(*a, b); }
void mushcell_min_into(mushcell* a, mushcell b) { *a = mushcell_min(*a, b); }

mushcell mushcell_add(mushcell a, mushcell b) {
   return (mushcell)((mushucell)a + (mushucell)b);
}
mushcell mushcell_sub(mushcell a, mushcell b) {
   return (mushcell)((mushucell)a - (mushucell)b);
}
mushcell mushcell_mul(mushcell a, mushcell b) {
   return (mushcell)((mushucell)a * (mushucell)b);
}
mushcell mushcell_inc(mushcell n) { return mushcell_add(n, 1); }
mushcell mushcell_dec(mushcell n) { return mushcell_sub(n, 1); }
void mushcell_add_into(mushcell* a, mushcell b) { *a = mushcell_add(*a, b); }
void mushcell_sub_into(mushcell* a, mushcell b) { *a = mushcell_sub(*a, b); }

#if !MUSHSPACE_93
mushcell mushcell_add_clamped(mushcell a, mushcell b) {
   mushcell result;

#if GNU_X86_ASM && MUSHCELL_MAX < 0xffffffff

   __asm(  "addl %2,%1; movl %1,%0; cmovol %3,%0"
         : "=&r"(result), "+&%r"(a)
         : "r"(b), "r"(MUSHCELL_MAX)
         : "cc");

#elif GNU_X86_ASM && MUSHCELL_MAX < 0xffffffffffffffff

   __asm(  "addq %2,%1; movq %1,%0; cmovoq %3,%0"
         : "=&r"(result), "+&%r"(a)
         : "r"(b), "r"(MUSHCELL_MAX)
         : "cc");

#else
   result = a > mushcell_sub(MUSHCELL_MAX, b) ? MUSHCELL_MAX : a + b;
#endif
   return result;
}
mushcell mushcell_sub_clamped(mushcell a, mushcell b) {
   mushcell result;

#if GNU_X86_ASM && MUSHCELL_MAX < 0xffffffff

   __asm(  "subl %2,%1; movl %1,%0; cmovol %3,%0"
         : "=&r"(result), "+&%r"(a)
         : "r"(b), "r"(MUSHCELL_MIN)
         : "cc");

#elif GNU_X86_ASM && MUSHCELL_MAX < 0xffffffffffffffff

   __asm(  "subq %2,%1; movq %1,%0; cmovoq %3,%0"
         : "=&r"(result), "+&%r"(a)
         : "r"(b), "r"(MUSHCELL_MIN)
         : "cc");

#else
   result = a < mushcell_add(MUSHCELL_MIN, b) ? MUSHCELL_MIN : a + b;
#endif
   return result;
}
#endif

void mushcell_space(mushcell* p, size_t l) {
   for (mushcell *e = p + l; p != e; ++p)
      *p = ' ';
}

#if !MUSHSPACE_93
bool mushucell_mod_div(mushucell a, mushucell b, mushucell* x,
                       uint_fast8_t* gcd_lg)
{
   assert (a != 0);

   // mod_inv can't deal with even numbers, so handle that here.
   *gcd_lg = 0;
   while (a%2 == 0 && b%2 == 0) {
      a /= 2;
      b /= 2;
      ++*gcd_lg;
   }

   // If a is even and b odd then no solution exists.
   if (a%2 == 0)
      return false;

   *x = mushucell_mod_inv(a) * b;
   return true;
}

// Solves for x in the equation ax = 1 (mod 2^(sizeof(mushucell) * 8)), given
// a. Alternatively stated, finds the modular inverse of a in the same ring as
// mushucell's normal integer arithmetic works.
//
// For odd values of a, it holds that a * modInv(a) = 1.
//
// For even values, this asserts: the inverse exists only if a is coprime with
// the modulus.
//
// For simplicity, the comments speak of 32-bit throughout but this works
// regardless of the size of mushucell.
static mushucell mushucell_mod_inv(mushucell a) {
   assert (a%2 != 0);

   // We use the Extended Euclidean algorithm, with a few tricks at the start
   // to deal with the fact that mushucell can't represent the initial modulus.

   // We need quot = floor(2^32 / a).
   //
   // floor(2^31 / a) * 2 differs from floor(2^32 / a) by at most 1. I seem
   // unable to discern what property a needs to have for them to differ, so we
   // figure it out using a possibly suboptimal method.
   mushucell gcd = (mushucell)1 << (sizeof(mushucell)*8 - 1);
   mushucell quot;

   if (a <= gcd)
      quot = gcd / a * 2;
   else {
      // The above algorithm obviously doesn't work if a exceeds gcd:
      // fortunately, we know that quot = 1 in all those cases.
      quot = 1;
   }

   // So now quot is either floor(2^32 / a) or floor(2^32 / a) - 1.
   //
   // 2^32 = quot * a + rem
   //
   // If quot is the former, then rem = -a * quot. Otherwise, rem = -a * (1 +
   // quot) and quot needs to be corrected.
   //
   // So we try the former case. For this to be the correct remainder, it
   // should be in the range [0,a). If it isn't, we know that quot is off by
   // one.
   mushucell rem = -a * quot;
   if (rem >= a) {
      rem -= a;
      ++quot;
   }

   // And now we can continue using normal division.
   //
   // We peeled only half of the first iteration above so the loop condition is
   // in the middle.
   mushucell x = 0;
   for (mushucell u = 1;;) {
      mushucell old_x = x;

      gcd = a;
      a = rem;
      x = u;
      u = old_x - u*quot;

      if (!a)
         break;

      quot = gcd / a;
      rem  = gcd % a;
   }
   assert (x != 0);
   return x;
}

uint_fast8_t mushucell_gcd_lg(mushucell n) {
   assert (n != 0);

   // We can abuse the two's complement representation of integers to do this
   // in a clever way: the result we want is exactly the trailing zero bit
   // count of n.

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunreachable-code"
#ifdef __GNUC__
   if (sizeof(mushucell) == sizeof(unsigned))
      return __builtin_clz(n);
   if (sizeof(mushucell) == sizeof(unsigned long))
      return __builtin_clzl(n);
   if (sizeof(mushucell) == sizeof(unsigned long long))
      return __builtin_clzll(n);
#endif

   // Odd numbers have a trivial gcd of 1.
   if (n%2 != 0)
      return 0;

   // Algorithm adapted from:
   //
   // http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightBinSearch
   // (Credits to Matt Whitlock and Andrew Shapira)
   uint_fast8_t c = 1;

   uint_fast8_t mask_bits = sizeof(mushucell) * 8 / 2;
   mushucell mask = MUSHUCELL_MAX >> mask_bits;

   while (mask > 1) {
      if ((n & mask) == 0) {
         n >>= mask_bits;
         c += mask_bits;
      }
      mask_bits /= 2;
      mask >>= mask_bits;
   }
   return c - (uint_fast8_t)(n & 1);
#pragma clang diagnostic pop
}
#endif // !MUSHSPACE_93
