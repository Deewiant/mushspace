// File created: 2012-01-28 00:28:34

#ifndef MUSHSPACE_SPACE_JUMP_TO_BOX_H
#define MUSHSPACE_SPACE_JUMP_TO_BOX_H

#include "space.all.h"

#define mushspace_jump_to_box MUSHSPACE_CAT(mushspace,_jump_to_box)

bool mushspace_jump_to_box(mushspace*, mushcoords*, mushcoords,
                           MushCursorMode*, mushaabb**, size_t*);

#endif
