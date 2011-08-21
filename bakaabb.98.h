// File created: 2011-08-06 16:12:07

#ifndef MUSHSPACE_BAKAABB_H
#define MUSHSPACE_BAKAABB_H

#include <stdlib.h>

#include "coords.all.h"
#include "bounds.all.h"
#include "typenames.any.h"

#define mush_bakaabb      MUSHSPACE_NAME(mush_bakaabb)
#define mush_bakaabb_iter MUSHSPACE_NAME(mush_bakaabb_iter)

typedef struct mush_bakaabb {
	mush_bounds bounds;
	void *data;
} mush_bakaabb;

typedef struct mush_bakaabb_iter mush_bakaabb_iter;

#define mush_bakaabb_init      MUSHSPACE_CAT(mush_bakaabb,_init)
#define mush_bakaabb_free      MUSHSPACE_CAT(mush_bakaabb,_free)
#define mush_bakaabb_get       MUSHSPACE_CAT(mush_bakaabb,_get)
#define mush_bakaabb_put       MUSHSPACE_CAT(mush_bakaabb,_put)
#define mush_bakaabb_size      MUSHSPACE_CAT(mush_bakaabb,_size)
#define mush_bakaabb_it_start  MUSHSPACE_CAT(mush_bakaabb,_it_start)
#define mush_bakaabb_it_stop   MUSHSPACE_CAT(mush_bakaabb,_it_stop)
#define mush_bakaabb_it_done   MUSHSPACE_CAT(mush_bakaabb,_it_done)
#define mush_bakaabb_it_next   MUSHSPACE_CAT(mush_bakaabb,_it_next)
#define mush_bakaabb_it_remove MUSHSPACE_CAT(mush_bakaabb,_it_remove)
#define mush_bakaabb_it_pos    MUSHSPACE_CAT(mush_bakaabb,_it_pos)
#define mush_bakaabb_it_val    MUSHSPACE_CAT(mush_bakaabb,_it_val)

bool mush_bakaabb_init(mush_bakaabb*, mushcoords);
void mush_bakaabb_free(mush_bakaabb* bak);

mushcell mush_bakaabb_get(const mush_bakaabb*, mushcoords);
bool     mush_bakaabb_put(      mush_bakaabb*, mushcoords, mushcell);

size_t mush_bakaabb_size(const mush_bakaabb*);

mush_bakaabb_iter* mush_bakaabb_it_start(const mush_bakaabb*);
void               mush_bakaabb_it_stop (mush_bakaabb_iter*);

bool mush_bakaabb_it_done(const mush_bakaabb_iter*, const mush_bakaabb*);

void mush_bakaabb_it_next  (mush_bakaabb_iter*, const mush_bakaabb*);
void mush_bakaabb_it_remove(mush_bakaabb_iter*,       mush_bakaabb*);

mushcoords mush_bakaabb_it_pos(const mush_bakaabb_iter*, const mush_bakaabb*);
mushcell   mush_bakaabb_it_val(const mush_bakaabb_iter*, const mush_bakaabb*);

#endif
