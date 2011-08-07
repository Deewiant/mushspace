// File created: 2011-08-06 15:58:49

#include <stdlib.h>

#include "typenames.h"
#include "coords.h"
#include "stats.h"

typedef struct mushspace mushspace;

#define mushspace_get MUSHSPACE_CAT(mushspace,_get)

size_t     MUSHSPACE_CAT(mushspace,_size)    (void);
mushspace *MUSHSPACE_CAT(mushspace,_allocate)(void*, mush_stats*);
void       MUSHSPACE_CAT(mushspace,_free)    (mushspace*);

mushcell mushspace_get(mushspace*, mushcoords);
