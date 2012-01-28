// File created: 2011-08-06 16:12:02

#ifndef MUSHSPACE_AABB_H
#define MUSHSPACE_AABB_H

#include <stdlib.h>

#include "bounds.all.h"
#include "coords.all.h"
#include "typenames.any.h"

#define mushaabb MUSHSPACE_NAME(mushaabb)

typedef struct mushaabb {
	mushbounds bounds;
	mushcell *data;
	size_t size;

#if MUSHSPACE_DIM >= 2
	size_t width;
#endif
#if MUSHSPACE_DIM >= 3
	size_t area;
#endif
} mushaabb;

#define mushaabb_make              MUSHSPACE_CAT(mushaabb,_make)
#define mushaabb_make_unsafe       MUSHSPACE_CAT(mushaabb,_make_unsafe)
#define mushaabb_finalize          MUSHSPACE_CAT(mushaabb,_finalize)
#define mushaabb_alloc             MUSHSPACE_CAT(mushaabb,_alloc)
#define mushaabb_volume_on         MUSHSPACE_CAT(mushaabb,_volume_on)
#define mushaabb_get               MUSHSPACE_CAT(mushaabb,_get)
#define mushaabb_put               MUSHSPACE_CAT(mushaabb,_put)
#define mushaabb_get_no_offset     MUSHSPACE_CAT(mushaabb,_get_no_offset)
#define mushaabb_put_no_offset     MUSHSPACE_CAT(mushaabb,_put_no_offset)
#define mushaabb_getter_no_offset  MUSHSPACE_CAT(mushaabb,_getter_no_offset)
#define mushaabb_get_idx           MUSHSPACE_CAT(mushaabb,_get_idx)
#define mushaabb_get_idx_no_offset MUSHSPACE_CAT(mushaabb,_get_idx_no_offset)
#define mushaabb_can_direct_copy   MUSHSPACE_CAT(mushaabb,_can_direct_copy)
#define mushaabb_can_direct_copy_area \
	MUSHSPACE_CAT(mushaabb,_can_direct_copy_area)

void mushaabb_make       (mushaabb*, const mushbounds*);
void mushaabb_make_unsafe(mushaabb*, const mushbounds*);
void mushaabb_finalize   (mushaabb*);
bool mushaabb_alloc      (mushaabb*);

size_t mushaabb_volume_on(const mushaabb*, mushdim);

mushcell mushaabb_get(const mushaabb*, mushcoords);
void     mushaabb_put(      mushaabb*, mushcoords, mushcell);

mushcell mushaabb_get_no_offset(const mushaabb*, mushcoords);
void     mushaabb_put_no_offset(      mushaabb*, mushcoords, mushcell);

mushcell mushaabb_getter_no_offset(const void*, mushcoords);

size_t mushaabb_get_idx          (const mushaabb*, mushcoords);
size_t mushaabb_get_idx_no_offset(const mushaabb*, mushcoords);

bool mushaabb_can_direct_copy(const mushaabb*, const mushaabb*);
bool mushaabb_can_direct_copy_area(
	const mushaabb*, const mushaabb*, const mushaabb*);

#endif
