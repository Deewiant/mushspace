// File created: 2011-08-06 16:12:12

#ifndef MUSHSPACE_COORDS_H
#define MUSHSPACE_COORDS_H

#include <stdbool.h>

#include "cell.both.h"
#include "typenames.any.h"

#define mushcoords MUSHSPACE_NAME(mushcoords)

typedef union mushcoords {
#pragma pack(push)
#pragma pack(1)
	struct {
		mushcell x;
#if MUSHSPACE_DIM >= 2
		mushcell y;
#endif
#if MUSHSPACE_DIM >= 3
		mushcell z;
#endif
	};
#pragma pack(pop)
	mushcell v[MUSHSPACE_DIM];
} mushcoords;

#define MUSHCOORDS_INIT MUSHSPACE_CAT(MUSHCOORDS_INIT,MUSHSPACE_DIM)
#define MUSHCOORDS_INIT1(a,b,c) {{.x = a}}
#define MUSHCOORDS_INIT2(a,b,c) {{.x = a, .y = b}}
#define MUSHCOORDS_INIT3(a,b,c) {{.x = a, .y = b, .z = c}}

#define MUSHCOORDS(a,b,c) ((mushcoords)MUSHCOORDS_INIT(a,b,c))

#define mushcoords_contains MUSHSPACE_CAT(mushcoords,_contains)
#define mushcoords_overlaps MUSHSPACE_CAT(mushcoords,_overlaps)

#define mushcoords_sub         MUSHSPACE_CAT(mushcoords,_sub)
#define mushcoords_equal       MUSHSPACE_CAT(mushcoords,_equal)

#define mushcoords_adds_clamped MUSHSPACE_CAT(mushcoords,_adds_clamped)
#define mushcoords_subs_clamped MUSHSPACE_CAT(mushcoords,_subs_clamped)

#define mushcoords_max_into MUSHSPACE_CAT(mushcoords,_max_into)
#define mushcoords_min_into MUSHSPACE_CAT(mushcoords,_min_into)

#define mushcoords_get_end_of_contiguous_range \
	MUSHSPACE_CAT(mushcoords,_get_end_of_contiguous_range)

bool mushcoords_contains(mushcoords pos, mushcoords beg, mushcoords end);

#if !MUSHSPACE_93
bool mushcoords_overlaps(mushcoords, mushcoords, mushcoords, mushcoords);
#endif

mushcoords mushcoords_sub(mushcoords, mushcoords);

#if !MUSHSPACE_93
mushcoords mushcoords_adds_clamped(mushcoords, mushcell);
mushcoords mushcoords_subs_clamped(mushcoords, mushcell);

bool mushcoords_equal(mushcoords, mushcoords);

void mushcoords_max_into(mushcoords*, mushcoords);
void mushcoords_min_into(mushcoords*, mushcoords);

mushcoords mushcoords_get_end_of_contiguous_range(
	mushcoords  end_pt,
	mushcoords* from,
	mushcoords  to,
	mushcoords  orig_beg,
	bool*       reached_to,
	mushcoords  tessell_beg,
	mushcoords  area_beg);
#endif

#endif
