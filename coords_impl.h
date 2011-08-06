// File created: 2011-08-06 17:33:31

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
