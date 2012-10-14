// File created: 2012-01-27 21:54:47

#ifndef MUSHSPACE_SPACE_LOAD_STRING_H
#define MUSHSPACE_SPACE_LOAD_STRING_H

#include "space.all.h"

#define mushspace_load_string       MUSHSPACE_CAT(mushspace,_load_string)
#define mushspace_load_string_utf8  MUSHSPACE_CAT(mushspace,_load_string_utf8)
#define mushspace_load_string_utf16 MUSHSPACE_CAT(mushspace,_load_string_utf16)
#define mushspace_load_string_cell  MUSHSPACE_CAT(mushspace,_load_string_cell)

int mushspace_load_string
   ( mushspace*, const unsigned char*, size_t len
#ifndef MUSHSPACE_93
   , mushcoords* end, mushcoords target, bool binary
#endif
   );

int mushspace_load_string_utf8
   ( mushspace*, const uint8_t*, size_t len
#ifndef MUSHSPACE_93
   , mushcoords* end, mushcoords target, bool binary
#endif
   );

int mushspace_load_string_utf16
   ( mushspace*, const uint16_t*, size_t len
#ifndef MUSHSPACE_93
   , mushcoords* end, mushcoords target, bool binary
#endif
   );

int mushspace_load_string_cell
   ( mushspace*, const mushcell*, size_t len
#ifndef MUSHSPACE_93
   , mushcoords* end, mushcoords target, bool binary
#endif
   );

#endif
