// File created: 2011-08-09 19:39:28

#ifndef MUSHSPACE_STDLIB_H
#define MUSHSPACE_STDLIB_H

#include <stdlib.h>

#define MUSH_ARRAY_LEN(X) (sizeof(X) / sizeof((X)[0]))

#define MUSH_DECL_DYN_ARRAY(T) \
	typedef struct mush_arr_##T { T *ptr; size_t len; } mush_arr_##T;

#define MUSH_DECL_CONST_DYN_ARRAY(T) \
	typedef struct mush_carr_##T { const T *ptr; size_t len; } mush_carr_##T;

size_t mush_size_t_max(size_t, size_t);

int mush_size_t_qsort_cmp(const void*, const void*);

// Don't overflow, instead clamp the result to SIZE_MAX.
size_t mush_size_t_add_clamped(size_t, size_t);
size_t mush_size_t_mul_clamped(size_t, size_t);

#endif
