// File created: 2011-08-09 19:39:28

#ifndef MUSHSPACE_STDLIB_H
#define MUSHSPACE_STDLIB_H

#include <stdlib.h>

#define MUSH_ARRAY_LEN(X) (sizeof(X) / sizeof((X)[0]))

size_t mush_size_t_max(size_t, size_t);

#endif
