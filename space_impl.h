// File created: 2011-08-06 15:58:49

#include <stdlib.h>

#include "typenames.h"

#include "stats.h"

typedef struct mushspace mushspace;

size_t     MUSHSPACE_CAT(mushspace,_size)     (void);
mushspace *MUSHSPACE_CAT(mushspace,_allocate) (void*);
void       MUSHSPACE_CAT(mushspace,_set_stats)(mushspace*, mush_stats*);
