// File created: 2011-08-06 15:58:49

#include <stdbool.h>
#include <stdlib.h>

#include "coords.h"
#include "stats.h"
#include "typenames.h"

#define mushspace MUSHSPACE_NAME(mushspace)

typedef struct mushspace mushspace;

#define mushspace_get MUSHSPACE_CAT(mushspace,_get)

size_t     MUSHSPACE_CAT(mushspace,_size)    (void);
mushspace *MUSHSPACE_CAT(mushspace,_allocate)(void*, mushstats*);
void       MUSHSPACE_CAT(mushspace,_free)    (mushspace*);

mushcell mushspace_get(mushspace*, mushcoords);

void MUSHSPACE_CAT(mushspace,_load_string)
	( mushspace*, const char*, size_t len
#if !MUSHSPACE_93
	, mushcoords* end, mushcoords target, bool binary
#endif
	);
