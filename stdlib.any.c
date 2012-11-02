// File created: 2011-08-09 19:51:08

#include "stdlib.any.h"

#include <stdint.h>

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define GNU_X86_ASM 1
#else
#define GNU_X86_ASM 0
#endif

size_t mush_size_t_max(size_t a, size_t b) { return a > b ? a : b; }

int mush_size_t_qsort_cmp(const void* p, const void* q) {
   const size_t *a = p, *b = q;
   return *a < *b ? -1 : *a > *b ? 1 : 0;
}

size_t mush_size_t_add_clamped(size_t a, size_t b) {
   size_t result;

#if GNU_X86_ASM && SIZE_MAX <= 0xffffffff

   __asm(  "addl %2,%1; movl %1,%0; cmovcl %3,%0"
         : "=&r"(result), "+&%r"(a)
         : "r"(b), "r"(SIZE_MAX)
         : "cc");

#elif GNU_X86_ASM && SIZE_MAX <= 0xffffffffffffffff

   __asm(  "addq %2,%1; movq %1,%0; cmovcq %3,%0"
         : "=&r"(result), "+&%r"(a)
         : "r"(b), "r"(SIZE_MAX)
         : "cc");

#else
   result = a > SIZE_MAX - b ? SIZE_MAX : a + b;
#endif
   return result;
}

size_t mush_size_t_mul_clamped(size_t a, size_t b) {
   size_t result;

#if GNU_X86_ASM && SIZE_MAX <= 0xffffffff

   __asm(  "movl %1,%0; mull %2; cmovcl %3,%0"
         : "=&a"(result)
         : "%dr"(a), "dr"(b), "r"(SIZE_MAX)
         : "edx", "cc");

#elif GNU_X86_ASM && SIZE_MAX <= 0xffffffffffffffff

   __asm(  "movq %1,%0; mulq %2; cmovcq %3,%0"
         : "=&a"(result)
         : "%dr"(a), "dr"(b), "r"(SIZE_MAX)
         : "rdx", "cc");
#else
   result = a > SIZE_MAX / b ? SIZE_MAX : a * b;
#endif
   return result;
}
