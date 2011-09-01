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

#define mushspace_sizeof           MUSHSPACE_CAT(mushspace,_sizeof)
#define mushspace_allocate         MUSHSPACE_CAT(mushspace,_allocate)
#define mushspace_free             MUSHSPACE_CAT(mushspace,_free)
#define mushspace_get              MUSHSPACE_CAT(mushspace,_get)
#define mushspace_put              MUSHSPACE_CAT(mushspace,_put)
#define mushspace_get_loose_bounds MUSHSPACE_CAT(mushspace,_get_loose_bounds)
#define mushspace_get_tight_bounds MUSHSPACE_CAT(mushspace,_get_tight_bounds)
#define mushspace_load_string      MUSHSPACE_CAT(mushspace,_load_string)

extern const size_t mushspace_sizeof;

mushspace *mushspace_allocate(void*, mushstats*);
void       mushspace_free    (mushspace*);

mushcell mushspace_get(
#ifndef MUSH_ENABLE_STATS
	const
#endif
	mushspace*, mushcoords);

int mushspace_put(mushspace*, mushcoords, mushcell);

void mushspace_get_loose_bounds(const mushspace*, mushcoords*, mushcoords*);

#if MUSHSPACE_93
// Returns false if the space is completely empty. In that case, the values of
// *beg and *end are undefined.
bool
#else
// If the space is completely empty, beg->x > end->x.
int
#endif
mushspace_get_tight_bounds(
#if MUSHSPACE_93
	const
#endif
	mushspace*, mushcoords* beg, mushcoords* end);

// Returns 0 on success or one of the following possible error codes:
//
// MUSH_ERR_OOM:     Ran out of memory somewhere.
// MUSH_ERR_NO_ROOM: The string doesn't fit in the space, i.e. it would overlap
//                   with itself. For instance, trying to binary-load 5
//                   gigabytes of non-space data into a 32-bit space would
//                   cause this error.
int mushspace_load_string
	( mushspace*, const char*, size_t len
#ifndef MUSHSPACE_93
	, mushcoords* end, mushcoords target, bool binary
#endif
	);

#endif
