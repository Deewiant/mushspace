// File created: 2012-01-27 21:55:55

#ifndef MUSHSPACE_SPACE_PUT_BINARY_H
#define MUSHSPACE_SPACE_PUT_BINARY_H

#include "space.all.h"

#define mushspace_put_binary MUSHSPACE_CAT(mushspace,_put_binary)

void mushspace_put_binary(const mushspace*, mushbounds,
                          void(*)(mushcell, void*),
#if MUSHSPACE_DIM > 1
                          void(*)(char, void*),
#endif
                          void*);

#endif
