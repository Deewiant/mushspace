// File created: 2012-10-20 20:34:02

#ifndef MUSHSPACE_DOUBLE_SIZE_T_H
#define MUSHSPACE_DOUBLE_SIZE_T_H

#include <stdlib.h>

// An integer with precision twice that of size_t.
typedef struct mush_double_size_t { size_t hi, lo; } mush_double_size_t;

// The operations we need, and no more.
void mush_double_size_t_add_into (mush_double_size_t*, mush_double_size_t);
void mush_double_size_t_sub1_into(mush_double_size_t*, size_t);
void mush_double_size_t_mul1_into(mush_double_size_t*, size_t);

#endif
