// File created: 2012-01-27 20:47:48

#include "space/put-binary.all.h"

void mushspace_put_binary(const mushspace* space, mushbounds bounds,
                          void(*putcell)(mushcell, void*),
#if MUSHSPACE_DIM > 1
                          void(*put)(unsigned char, void*),
#endif
                          void* putdata
) {
	mushcoords c;
#if MUSHSPACE_DIM >= 3
	for (c.z = bounds.beg.z;;) {
#endif
#if MUSHSPACE_DIM >= 2
		for (c.y = bounds.beg.y;;) {
#endif
			for (c.x = bounds.beg.x; c.x <= bounds.end.x; ++c.x)
				putcell(mushspace_get(space, c), putdata);

#if MUSHSPACE_DIM >= 2
			if (c.y++ == bounds.end.y)
				break;
			put('\n', putdata);
		}
#endif
#if MUSHSPACE_DIM >= 3
		if (c.z++ == bounds.end.z)
			break;
		put('\f', putdata);
	}
#endif
}
