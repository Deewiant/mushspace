// File created: 2011-08-06 15:58:49

#include <stdbool.h>
#include <stdlib.h>

#include "coords.h"
#include "errors.h"
#include "stats.h"
#include "typenames.h"

#define mushspace MUSHSPACE_NAME(mushspace)

typedef struct mushspace mushspace;

#define mushspace_get MUSHSPACE_CAT(mushspace,_get)
#define mushspace_put MUSHSPACE_CAT(mushspace,_put)

extern const size_t MUSHSPACE_CAT(mushspace,_size);

mushspace *MUSHSPACE_CAT(mushspace,_allocate)(void*, mushstats*);
void       MUSHSPACE_CAT(mushspace,_free)    (mushspace*);

mushcell mushspace_get(mushspace*, mushcoords);
void     mushspace_put(mushspace*, mushcoords, mushcell);

// Returns 0 on success or one of the following possible error codes:
//
// MUSH_ERR_OOM:     Ran out of memory somewhere.
// MUSH_ERR_NO_ROOM: The string doesn't fit in the space, i.e. it would overlap
//                   with itself. For instance, trying to binary-load 5
//                   gigabytes of non-space data into a 32-bit space would
//                   cause this error.
int MUSHSPACE_CAT(mushspace,_load_string)
	( mushspace*, const char*, size_t len
#if !MUSHSPACE_93
	, mushcoords* end, mushcoords target, bool binary
#endif
	);
