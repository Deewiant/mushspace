// File created: 2012-01-27 21:55:31

#ifndef MUSHSPACE_SPACE_PUT_TEXTUAL_H
#define MUSHSPACE_SPACE_PUT_TEXTUAL_H

#include "space.all.h"

#define mushspace_put_textual MUSHSPACE_CAT(mushspace,_put_textual)

int mushspace_put_textual(const mushspace*, mushcoords, mushcoords,
                          mushcell**, size_t*, unsigned char**, size_t*,
                          void(*)(const mushcell*, size_t, void*),
                          void(*)(unsigned char, void*), void*);

#endif
