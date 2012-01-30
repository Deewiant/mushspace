// File created: 2012-01-27 21:54:47

#ifndef MUSHSPACE_SPACE_LOAD_STRING_H
#define MUSHSPACE_SPACE_LOAD_STRING_H

#include "space.all.h"

#define mushspace_load_string MUSHSPACE_CAT(mushspace,_load_string)

// Returns 0 on success or one of the following possible error codes:
//
// MUSHERR_OOM:     Ran out of memory somewhere.
// MUSHERR_NO_ROOM: The string doesn't fit in the space, i.e. it would overlap
//                  with itself. For instance, trying to binary-load 5
//                  gigabytes of non-space data into a 32-bit space would
//                  cause this error.
int mushspace_load_string
   ( mushspace*, const unsigned char*, size_t len
#ifndef MUSHSPACE_93
   , mushcoords* end, mushcoords target, bool binary
#endif
   );

#endif
