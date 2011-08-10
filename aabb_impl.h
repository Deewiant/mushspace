// File created: 2011-08-06 17:40:27

#include <stdlib.h>

#include "coords.h"
#include "typenames.h"

#define mush_aabb MUSHSPACE_NAME(mush_aabb)

typedef struct mush_aabb {
	mushcell *data;
	size_t size;
	mushcoords beg, end;

#if MUSHSPACE_DIM >= 2
	size_t width;
#endif
#if MUSHSPACE_DIM >= 3
	size_t area;
#endif
} mush_aabb;

#define mush_aabb_make        MUSHSPACE_CAT(mush_aabb,_make)
#define mush_aabb_make_unsafe MUSHSPACE_CAT(mush_aabb,_make_unsafe)
#define mush_aabb_finalize    MUSHSPACE_CAT(mush_aabb,_finalize)

#define mush_aabb_volume_on MUSHSPACE_CAT(mush_aabb,_volume_on)

#define mush_aabb_get           MUSHSPACE_CAT(mush_aabb,_get)
#define mush_aabb_contains      MUSHSPACE_CAT(mush_aabb,_contains)
#define mush_aabb_safe_contains MUSHSPACE_CAT(mush_aabb,_safe_contains)

#define mush_aabb_get_idx           MUSHSPACE_CAT(mush_aabb,_get_idx)
#define mush_aabb_get_idx_no_offset MUSHSPACE_CAT(mush_aabb,_get_idx_no_offset)

#define mush_aabb_overlaps MUSHSPACE_CAT(mush_aabb,_overlaps)

#define mush_aabb_tessellate  MUSHSPACE_CAT(mush_aabb,_tessellate)
#define mush_aabb_tessellate1 MUSHSPACE_CAT(mush_aabb,_tessellate1)

void mush_aabb_make       (mush_aabb*, mushcoords, mushcoords);
void mush_aabb_make_unsafe(mush_aabb*, mushcoords, mushcoords);
void mush_aabb_finalize   (mush_aabb*);

size_t mush_aabb_volume_on(const mush_aabb*, mushucell axis);

mushcell mush_aabb_get     (const mush_aabb*, mushcoords);
bool     mush_aabb_contains(const mush_aabb*, mushcoords);

// Works even if the AABB is an unsafe box with beg > end.
bool mush_aabb_safe_contains(const mush_aabb*, mushcoords);

size_t mush_aabb_get_idx          (const mush_aabb*, mushcoords);
size_t mush_aabb_get_idx_no_offset(const mush_aabb*, mushcoords);

bool mush_aabb_overlaps(const mush_aabb*, const mush_aabb*);

void mush_aabb_tessellate(
	mushcoords, const mush_aabb*, size_t, mushcoords*, mushcoords*);

void mush_aabb_tessellate1(
	mushcoords, mushcoords, mushcoords, mushcoords*, mushcoords*);
