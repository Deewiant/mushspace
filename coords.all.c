// File created: 2011-08-07 18:20:32

#include "coords.all.h"

#include <stdlib.h>
#include <string.h>

void mushcoords_max_into(mushcoords* a, mushcoords b) {
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
		a->v[i] = mushcell_max(a->v[i], b.v[i]);
}
void mushcoords_min_into(mushcoords* a, mushcoords b) {
	for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
		a->v[i] = mushcell_min(a->v[i], b.v[i]);
}

#define DEFINE_OP(NAME) \
	mushcoords mushcoords_##NAME(mushcoords a, mushcoords b) { \
		mushcoords x = a; \
		for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) \
			x.v[i] = mushcell_##NAME(x.v[i], b.v[i]); \
		return x; \
	} \
	void mushcoords_##NAME##_into(mushcoords* a, mushcoords b) { \
		for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) \
			a->v[i] = mushcell_##NAME(a->v[i], b.v[i]); \
	} \
	mushcoords mushcoords_##NAME##s(mushcoords a, mushcell b) { \
		mushcoords x = a; \
		for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) \
			x.v[i] = mushcell_##NAME(x.v[i], b); \
		return x; \
	}

#define DEFINE_CLAMPED_OP(NAME) \
	mushcoords mushcoords_##NAME##s_clamped(mushcoords a, mushcell b) { \
		mushcoords x = a; \
		for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) \
			x.v[i] = mushcell_##NAME##_clamped(x.v[i], b); \
		return x; \
	}

DEFINE_OP(add)
DEFINE_OP(sub)

#if !MUSHSPACE_93

DEFINE_CLAMPED_OP(add)
DEFINE_CLAMPED_OP(sub)

bool mushcoords_equal(mushcoords a, mushcoords b) {
	// Yes, peeling the loop is worth it.
	return a.v[0] == b.v[0]
#if MUSHSPACE_DIM >= 2
	    && a.v[1] == b.v[1]
#if MUSHSPACE_DIM >= 3
	    && a.v[2] == b.v[2]
#endif
#endif
	;
}

mushcoords mushcoords_get_end_of_contiguous_range(
	mushcoords  end_pt,
	mushcoords* from,
	mushcoords  to,
	mushcoords  orig_beg,
	bool*       reached_to,
	mushcoords  tessell_beg,
	mushcoords  area_beg)
{
	static const mushucell D = MUSHSPACE_DIM;
#if MUSHSPACE_DIM >= 2
	mushcell orig_from[D-1];
	memcpy(orig_from, from->v + 1, sizeof orig_from);
#else
	(void)tessell_beg; (void)area_beg;
#endif

	for (mushucell i = 0; i < D-1; ++i) {
		if (end_pt.v[i] == to.v[i]) {
			// Hit the end point exactly: we'll be going to the next line/page on
			// this axis.
			from->v[i] = orig_beg.v[i];
			continue;
		}

		const size_t remaining_bytes = (D - (i+1)) * sizeof(mushcell);

		// Did not reach the end point or the box is too big to go any further as
		// a contiguous block. The remaining axes won't be changing.
		memcpy(end_pt.v + i+1, from->v + i+1, remaining_bytes);

		if (end_pt.v[i] < to.v[i] || from->v[i] > to.v[i]) {
			// Did not reach the endpoint: either ordinarily or because "to" is
			// wrapped around with respect to "from", so it's not possible to
			// reach the endpoint from "from" without going to another box.
			//
			// The next point will be one up on this axis.
			from->v[i] = mushcell_inc(end_pt.v[i]);
		} else {
			// Reached "to" on this axis, but the box is too big to go any
			// further.
			end_pt.v[i] = to.v[i];

			// If we're at "to" on the other axes as well, we're there.
			if (!memcmp(end_pt.v + i+1, to.v + i+1, remaining_bytes))
				*reached_to = true;
			else {
				// We should go further on the next axis next time around, since
				// we're done along this one.
				from->v[i]   = orig_beg.v[i];
				from->v[i+1] = mushcell_inc(from->v[i+1]);
			}
		}
		goto end;
	}
	// All the coords but the last were the same: check the last one too.
	if (end_pt.v[D-1] == to.v[D-1])
		*reached_to = true;
	else {
		if (end_pt.v[D-1] < to.v[D-1] || from->v[D-1] > to.v[D-1])
			from->v[D-1] = mushcell_inc(end_pt.v[D-1]);
		else {
			end_pt.v[D-1] = to.v[D-1];
			*reached_to = true;
		}
	}
end:
#if MUSHSPACE_DIM >= 2
	for (mushucell i = 0; i < D-1; ++i) {
		// If we were going to cross a line/page but we're actually in a box
		// tessellated in such a way that we can't, wibble things so that we just
		// go to the end of the line/page.
		if (end_pt.v[i+1] > orig_from[i] && tessell_beg.v[i] != area_beg.v[i]) {

			memcpy(end_pt.v + i+1, orig_from + i,   (D-(i+1)) * sizeof(mushcell));
			memcpy(from->v  + i+2, orig_from + i+1, (D-(i+2)) * sizeof(mushcell));
			from->v[i+1] = mushcell_inc(orig_from[i]);

			*reached_to = false;
			break;
		}
	}
#endif
	return end_pt;
}
#endif // !MUSHSPACE_93
