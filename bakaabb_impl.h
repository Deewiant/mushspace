// File created: 2011-08-06 17:55:19

#include <stdlib.h>

#include "coords.h"
#include "typenames.h"

#define mush_bakaabb      MUSHSPACE_NAME(mush_bakaabb)
#define mush_bakaabb_iter MUSHSPACE_NAME(mush_bakaabb_iter)

typedef struct mush_bakaabb {
	void *data;
	mushcoords beg, end;
} mush_bakaabb;

typedef struct mush_bakaabb_iter mush_bakaabb_iter;

#define mush_bakaabb_init      MUSHSPACE_CAT(mush_bakaabb,_init)
#define mush_bakaabb_get       MUSHSPACE_CAT(mush_bakaabb,_get)
#define mush_bakaabb_size      MUSHSPACE_CAT(mush_bakaabb,_size)
#define mush_bakaabb_it_start  MUSHSPACE_CAT(mush_bakaabb,_it_start)
#define mush_bakaabb_it_stop   MUSHSPACE_CAT(mush_bakaabb,_it_stop)
#define mush_bakaabb_it_done   MUSHSPACE_CAT(mush_bakaabb,_it_done)
#define mush_bakaabb_it_next   MUSHSPACE_CAT(mush_bakaabb,_it_next)
#define mush_bakaabb_it_remove MUSHSPACE_CAT(mush_bakaabb,_it_remove)
#define mush_bakaabb_it_pos    MUSHSPACE_CAT(mush_bakaabb,_it_pos)
#define mush_bakaabb_it_val    MUSHSPACE_CAT(mush_bakaabb,_it_val)

void mush_bakaabb_init(mush_bakaabb*, mushcoords);

mushcell mush_bakaabb_get(const mush_bakaabb*, mushcoords);

size_t mush_bakaabb_size(const mush_bakaabb*);

mush_bakaabb_iter* mush_bakaabb_it_start(const mush_bakaabb*);
void               mush_bakaabb_it_stop (mush_bakaabb_iter*);

bool mush_bakaabb_it_done(const mush_bakaabb_iter*, const mush_bakaabb*);

void mush_bakaabb_it_next  (mush_bakaabb_iter*, const mush_bakaabb*);
void mush_bakaabb_it_remove(mush_bakaabb_iter*,       mush_bakaabb*);

mushcoords mush_bakaabb_it_pos(const mush_bakaabb_iter*, const mush_bakaabb*);
mushcell   mush_bakaabb_it_val(const mush_bakaabb_iter*, const mush_bakaabb*);
