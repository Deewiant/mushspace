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

   __asm(  "addl %2,%1; movl %3,%0; cmovnol %1,%0"
         : "=r"(result), "+r"(a)
         : "r"(b), "n"(SIZE_MAX));

#elif GNU_X86_ASM && SIZE_MAX <= 0xffffffffffffffff

   __asm(  "addq %2,%1; movq %3,%0; cmovnoq %1,%0"
         : "=r"(result), "+r"(a)
         : "r"(b), "n"(SIZE_MAX));

#else
   result = a > SIZE_MAX - b ? SIZE_MAX : a + b;
#endif
   return result;
}

size_t mush_size_t_mul_clamped(size_t a, size_t b) {
   size_t result;

#if GNU_X86_ASM && SIZE_MAX <= 0xffffffff

   size_t size;
   __asm(  "movl %2,%0; mull %3; movl %4,%1; cmovol %1,%0"
         : "=a"(result), "=r"(size)
         : "r"(a), "r"(b), "i"(~0)
         : "edx");

#elif GNU_X86_ASM && SIZE_MAX <= 0xffffffffffffffff

   size_t size;
   __asm(  "movq %2,%0; mulq %3; movq %4,%1; cmovoq %1,%0"
         : "=a"(result), "=r"(size)
         : "r"(a), "r"(b), "i"(SIZE_MAX)
         : "rdx");
#else
   result = a > SIZE_MAX / b ? SIZE_MAX : a * b;
#endif
   return result;
}
