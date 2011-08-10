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

#define mushcoords_sub MUSHSPACE_CAT(mushcoords,_sub)

#define mushcoords_max_into MUSHSPACE_CAT(mushcoords,_max_into)

bool mushcoords_contains(mushcoords pos, mushcoords beg, mushcoords end);

mushcoords mushcoords_sub(mushcoords, mushcoords);

void mushcoords_max_into(mushcoords*, mushcoords);
