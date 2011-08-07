// File created: 2011-08-06 17:33:31

#include <stdbool.h>

#include "typenames.h"
#include "cell.h"

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

#define mushcoords_contains MUSHSPACE_CAT(mushcoords,_contains)

#define mushcoords_sub MUSHSPACE_CAT(mushcoords,_sub)

bool mushcoords_contains(mushcoords pos, mushcoords beg, mushcoords end);

mushcoords mushcoords_sub(mushcoords, mushcoords);
