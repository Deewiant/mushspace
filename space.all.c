// File created: 2011-09-02 19:17:34

#include "space.all.h"

const size_t mushspace_sizeof = sizeof(mushspace);

void mushspace_put_binary(const mushspace* space,
                          mushcoords beg, mushcoords end,
                          void(*putcell)(mushcell, void*),
#if MUSHSPACE_DIM > 1
                          void(*put)(unsigned char, void*),
#endif
                          void* putdata
) {
	mushcoords c;
#if MUSHSPACE_DIM >= 3
	for (c.z = beg.z;;) {
#endif
#if MUSHSPACE_DIM >= 2
		for (c.y = beg.y;;) {
#endif
			for (c.x = beg.x; c.x <= end.x; ++c.x)
				putcell(mushspace_get(space, c), putdata);

#if MUSHSPACE_DIM >= 2
			if (c.y++ == end.y)
				break;
			put('\n', putdata);
		}
#endif
#if MUSHSPACE_DIM >= 3
		if (c.z++ == end.z)
			break;
		put('\f', putdata);
	}
#endif
}
