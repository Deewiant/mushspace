// File created: 2012-01-29 11:05:32

#include "space/load-string.all.h"

static bool newline(bool* got_cr, mushcoords* pos) {
	*got_cr = false;
	pos->x = 0;
	return ++pos->y >= 25;
}
int mushspace_load_string(
	mushspace* space, const unsigned char* str, size_t len)
{
	bool got_cr = false;
	mushcoords pos = MUSHCOORDS(0,0,0);

	for (size_t i = 0; i < len; ++i) {
		unsigned char c = str[i];

		switch (c) {
		case '\r': got_cr = true; break;
		case '\n': if (newline(&got_cr, &pos)) goto end; else break;
		default:
			if (got_cr && newline(&got_cr, &pos))
				goto end;

			if (c != ' ')
				mushstaticaabb_put(&space->box, pos, c);

			if (++pos.x < 80)
				break;

			// Skip to and past EOL after column 80.
			while (++i < len) {
				c = str[i];
				switch (c) {
				case '\r': got_cr = true; break;
				default:   if (!got_cr) break;
				case '\n': if (newline(&got_cr, &pos)) goto end; else goto skipped;
				}
			}
skipped:
			break;
		}
	}
end:
	return MUSHERR_NONE;
}
