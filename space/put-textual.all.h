// File created: 2012-01-27 21:55:31

#ifndef MUSHSPACE_SPACE_PUT_TEXTUAL_H
#define MUSHSPACE_SPACE_PUT_TEXTUAL_H

#include "space.all.h"

#define mushspace_put_textual MUSHSPACE_CAT(mushspace,_put_textual)

void mushspace_put_textual(mushspace*, mushbounds,
#if !MUSHSPACE_93
                           mushcell**, size_t*, char**, size_t*,
#endif
                           void(*)(const mushcell*, size_t, void*),
                           void(*)(char, void*), void*);

#endif
