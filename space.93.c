// File created: 2011-08-14 00:10:39

#include "space.all.h"

#include <string.h>

#include "staticaabb.all.h"
#include "stdlib.any.h"

struct mushspace {
	mushstats *stats;
	mush_staticaabb box;
};

const size_t mushspace_size = sizeof(mushspace);

mushspace* mushspace_allocate(void* vp, mushstats* stats) {
	mushspace *space = vp ? vp : malloc(sizeof *space);
	if (space) {
		space->stats = stats ? stats : malloc(sizeof *space->stats);
		mushcell_space(space->box.array, MUSH_ARRAY_LEN(space->box.array));
	}
	return space;
}

void mushspace_free(mushspace* space) { free(space->stats); }

mushcell mushspace_get(mushspace* space, mushcoords c) {
	mushstats_add(space->stats, MushStat_lookups, 1);

	return mush_staticaabb_contains(c) ? mush_staticaabb_get(&space->box, c)
	                                   : ' ';
}

int mushspace_put(mushspace* space, mushcoords p, mushcell c) {
	mushstats_add(space->stats, MushStat_assignments, 1);

	if (mush_staticaabb_contains(p))
		mush_staticaabb_put(&space->box, p, c);
	return MUSH_ERR_NONE;
}

static bool mushspace2_93_newline(bool* got_cr, mushcoords* pos) {
	*got_cr = false;
	pos->x = 0;
	return ++pos->y >= 25;
}
int mushspace_load_string(mushspace* space, const char* str, size_t len) {
	bool got_cr = false;
	mushcoords pos = MUSHCOORDS(0,0,0);

	for (size_t i = 0; i < len; ++i) {
		char c = str[i];

		switch (c) {
		case '\r': got_cr = true; break;
		case '\n': if (mushspace2_93_newline(&got_cr, &pos)) goto end; break;
		default:
			if (got_cr && mushspace2_93_newline(&got_cr, &pos))
				goto end;

			if (c != ' ')
				mush_staticaabb_put(&space->box, pos, c);

			if (++pos.x < 80)
				break;

			// Skip to and past EOL after column 80.
			while (++i < len) {
				c = str[i];
				switch (c) {
				case '\r': got_cr = true; break;
				default:   if (!got_cr) break;
				case '\n': if (mushspace2_93_newline(&got_cr, &pos)) goto end;
				           goto skipped;
				}
			}
skipped:
			break;
		}
	}
end:
	return MUSH_ERR_NONE;
}
