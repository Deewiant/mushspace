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
#define mushspace_copy             MUSHSPACE_CAT(mushspace,_copy)
#define mushspace_get              MUSHSPACE_CAT(mushspace,_get)
#define mushspace_get_nostats      MUSHSPACE_CAT(mushspace,_get_nostats)
#define mushspace_put              MUSHSPACE_CAT(mushspace,_put)
#define mushspace_put_nostats      MUSHSPACE_CAT(mushspace,_put_nostats)
#define mushspace_get_loose_bounds MUSHSPACE_CAT(mushspace,_get_loose_bounds)
#define mushspace_get_tight_bounds MUSHSPACE_CAT(mushspace,_get_tight_bounds)
#define mushspace_load_string      MUSHSPACE_CAT(mushspace,_load_string)
#define mushspace_put_binary       MUSHSPACE_CAT(mushspace,_put_binary)
#define mushspace_put_textual      MUSHSPACE_CAT(mushspace,_put_textual)
#define mushspace_add_invalidatee  MUSHSPACE_CAT(mushspace,_add_invalidatee)

extern const size_t mushspace_sizeof;

mushspace *mushspace_allocate(void*, mushstats*);
void       mushspace_free    (mushspace*);
mushspace *mushspace_copy    (void*, const mushspace*, mushstats*);

mushcell mushspace_get        (      mushspace*, mushcoords);
mushcell mushspace_get_nostats(const mushspace*, mushcoords);
int      mushspace_put        (      mushspace*, mushcoords, mushcell);
int      mushspace_put_nostats(      mushspace*, mushcoords, mushcell);

void mushspace_get_loose_bounds(const mushspace*, mushcoords*, mushcoords*);

// Returns false if the space is completely empty. In that case, the values of
// *beg and *end are undefined.
bool mushspace_get_tight_bounds(
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

void mushspace_put_binary(const mushspace*, mushcoords, mushcoords,
                          void(*)(mushcell, void*),
#if MUSHSPACE_DIM > 1
                          void(*)(unsigned char, void*),
#endif
                          void*);
int mushspace_put_textual(const mushspace*, mushcoords, mushcoords,
                          mushcell**, size_t*, unsigned char**, size_t*,
                          void(*)(const mushcell*, size_t, void*),
                          void(*)(unsigned char, void*), void*);

#if !MUSHSPACE_93
bool mushspace_add_invalidatee(mushspace*, void(*)(void*), void*);
#endif

#endif
