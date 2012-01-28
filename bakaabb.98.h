// File created: 2011-08-06 16:12:07

#ifndef MUSHSPACE_BAKAABB_H
#define MUSHSPACE_BAKAABB_H

#include <stdlib.h>

#include "coords.all.h"
#include "bounds.all.h"
#include "typenames.any.h"

#define mushbakaabb      MUSHSPACE_NAME(mushbakaabb)
#define mushbakaabb_iter MUSHSPACE_NAME(mushbakaabb_iter)

typedef struct mushbakaabb {
	mushbounds bounds;
	void *data;
} mushbakaabb;

typedef struct mushbakaabb_iter mushbakaabb_iter;

#define mushbakaabb_iter_sizeof    MUSHSPACE_CAT(mushbakaabb_iter,_sizeof)
#define mushbakaabb_init           MUSHSPACE_CAT(mushbakaabb,_init)
#define mushbakaabb_free           MUSHSPACE_CAT(mushbakaabb,_free)
#define mushbakaabb_copy           MUSHSPACE_CAT(mushbakaabb,_copy)
#define mushbakaabb_get            MUSHSPACE_CAT(mushbakaabb,_get)
#define mushbakaabb_get_ptr_unsafe MUSHSPACE_CAT(mushbakaabb,_get_ptr_unsafe)
#define mushbakaabb_put            MUSHSPACE_CAT(mushbakaabb,_put)
#define mushbakaabb_size           MUSHSPACE_CAT(mushbakaabb,_size)
#define mushbakaabb_it_start       MUSHSPACE_CAT(mushbakaabb,_it_start)
#define mushbakaabb_it_stop        MUSHSPACE_CAT(mushbakaabb,_it_stop)
#define mushbakaabb_it_done        MUSHSPACE_CAT(mushbakaabb,_it_done)
#define mushbakaabb_it_next        MUSHSPACE_CAT(mushbakaabb,_it_next)
#define mushbakaabb_it_remove      MUSHSPACE_CAT(mushbakaabb,_it_remove)
#define mushbakaabb_it_pos         MUSHSPACE_CAT(mushbakaabb,_it_pos)
#define mushbakaabb_it_val         MUSHSPACE_CAT(mushbakaabb,_it_val)

extern const size_t mushbakaabb_iter_sizeof;

bool mushbakaabb_init(mushbakaabb*, mushcoords);
void mushbakaabb_free(mushbakaabb*);
bool mushbakaabb_copy(mushbakaabb*, const mushbakaabb*);

mushcell  mushbakaabb_get           (const mushbakaabb*, mushcoords);
mushcell* mushbakaabb_get_ptr_unsafe(      mushbakaabb*, mushcoords);
bool      mushbakaabb_put           (      mushbakaabb*, mushcoords, mushcell);

size_t mushbakaabb_size(const mushbakaabb*);

mushbakaabb_iter* mushbakaabb_it_start(const mushbakaabb*, void*);
void              mushbakaabb_it_stop (mushbakaabb_iter*);

bool mushbakaabb_it_done(const mushbakaabb_iter*, const mushbakaabb*);

void mushbakaabb_it_next  (mushbakaabb_iter*, const mushbakaabb*);
void mushbakaabb_it_remove(mushbakaabb_iter*,       mushbakaabb*);

mushcoords mushbakaabb_it_pos(const mushbakaabb_iter*, const mushbakaabb*);
mushcell   mushbakaabb_it_val(const mushbakaabb_iter*, const mushbakaabb*);

#endif
