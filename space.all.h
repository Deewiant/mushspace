// File created: 2011-08-06 15:58:45

#ifndef MUSHSPACE_H
#define MUSHSPACE_H

#include <stdbool.h>
#include <stdlib.h>

#include "config.h"
#include "coords.all.h"
#include "errors.any.h"
#include "stats.any.h"
#include "typenames.any.h"

#define mushspace MUSHSPACE_NAME(mushspace)

typedef struct mushspace mushspace;

#define mushspace_get MUSHSPACE_CAT(mushspace,_get)
#define mushspace_put MUSHSPACE_CAT(mushspace,_put)

extern const size_t MUSHSPACE_CAT(mushspace,_size);

mushspace *MUSHSPACE_CAT(mushspace,_allocate)(void*, mushstats*);
void       MUSHSPACE_CAT(mushspace,_free)    (mushspace*);

mushcell mushspace_get(
#ifndef MUSH_ENABLE_STATS
	const
#endif
	mushspace*, mushcoords);

int mushspace_put(mushspace*, mushcoords, mushcell);

// Returns 0 on success or one of the following possible error codes:
//
// MUSH_ERR_OOM:     Ran out of memory somewhere.
// MUSH_ERR_NO_ROOM: The string doesn't fit in the space, i.e. it would overlap
//                   with itself. For instance, trying to binary-load 5
//                   gigabytes of non-space data into a 32-bit space would
//                   cause this error.
int MUSHSPACE_CAT(mushspace,_load_string)
	( mushspace*, const char*, size_t len
#ifndef MUSHSPACE_93
	, mushcoords* end, mushcoords target, bool binary
#endif
	);

#endif
