// File created: 2011-08-09 19:51:08

#include "stdlib.any.h"

#include <stdint.h>

size_t mush_size_t_max(size_t a, size_t b) { return a > b ? a : b; }

int mush_size_t_qsort_cmp(const void* p, const void* q) {
   const size_t *a = p, *b = q;
   return *a < *b ? -1 : *a > *b ? 1 : 0;
}

size_t mush_size_t_add_clamped(size_t a, size_t b) {
   return a > SIZE_MAX - b ? SIZE_MAX : a + b;
}

size_t mush_size_t_mul_clamped(size_t a, size_t b) {
   return a > SIZE_MAX / b ? SIZE_MAX : a * b;
}
