// File created: 2011-08-06 17:33:31

#include <stdbool.h>

#include "cell.h"
#include "typenames.h"

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

#define MUSHCOORDS MUSHSPACE_CAT(MUSHCOORDS,MUSHSPACE_DIM)
#define MUSHCOORDS1(a,b,c) (mushcoords){{.x = a}}
#define MUSHCOORDS2(a,b,c) (mushcoords){{.x = a, .y = b}}
#define MUSHCOORDS3(a,b,c) (mushcoords){{.x = a, .y = b, .z = c}}

#define mushcoords_contains MUSHSPACE_CAT(mushcoords,_contains)

#define mushcoords_sub         MUSHSPACE_CAT(mushcoords,_sub)
#define mushcoords_equal       MUSHSPACE_CAT(mushcoords,_equal)

#define mushcoords_adds_clamped MUSHSPACE_CAT(mushcoords,_adds_clamped)
#define mushcoords_subs_clamped MUSHSPACE_CAT(mushcoords,_subs_clamped)

#define mushcoords_max_into MUSHSPACE_CAT(mushcoords,_max_into)

#define mushcoords_get_end_of_contiguous_range \
	MUSHSPACE_CAT(mushcoords,_get_end_of_contiguous_range)

bool mushcoords_contains(mushcoords pos, mushcoords beg, mushcoords end);

mushcoords mushcoords_sub(mushcoords, mushcoords);

mushcoords mushcoords_adds_clamped(mushcoords, mushcell);
mushcoords mushcoords_subs_clamped(mushcoords, mushcell);

bool mushcoords_equal(mushcoords, mushcoords);

void mushcoords_max_into(mushcoords*, mushcoords);

mushcoords mushcoords_get_end_of_contiguous_range(
	mushcoords  end_pt,
	mushcoords* from,
	mushcoords  to,
	mushcoords  orig_beg,
	bool*       reached_to,
	mushcoords  tessell_beg,
	mushcoords  area_beg);
