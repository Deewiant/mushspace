// File created: 2012-01-27 21:55:31

#ifndef MUSHSPACE_SPACE_PUT_TEXTUAL_H
#define MUSHSPACE_SPACE_PUT_TEXTUAL_H

#include "space.all.h"

#define mushspace_put_textual MUSHSPACE_CAT(mushspace,_put_textual)

#if MUSHSPACE_93
void mushspace_put_textual(const mushspace*, mushbounds,
                           void(*)(const mushcell*, size_t, void*),
                           void(*)(char, void*), void*);
#else
int mushspace_put_textual(const mushspace*, mushbounds,
                          mushcell**, size_t*, char**, size_t*,
                          void(*)(const mushcell*, size_t, void*),
                          void(*)(char, void*), void*);
#endif

#endif
