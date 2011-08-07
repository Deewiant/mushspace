// File created: 2011-08-06 17:55:19

#include "lib/tommyds/tommyhashdyn.h"

#include "typenames.h"
#include "coords.h"

typedef struct mush_bakaabb {
	tommy_hashdyn data;
	mushcoords beg, end;
} mush_bakaabb;

#define mush_bakaabb_init MUSHSPACE_CAT(mush_bakaabb,_init)

void mush_bakaabb_init(mush_bakaabb*, mushcoords);
