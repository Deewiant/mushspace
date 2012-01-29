// File created: 2012-01-27 20:25:04

#include "space/put-textual.all.h"

static bool put_textual_row(
	const mushcell*, size_t*, const unsigned char*, size_t*,
	void(*)(const mushcell*, size_t, void*), void(*)(unsigned char, void*),
	void*);

static void put_textual_page(
	const mushcell*, size_t*, const unsigned char*, size_t*,
	void(*)(const mushcell*, size_t, void*), void(*)(unsigned char, void*),
	void*);

static bool put_textual_add_ws(
	unsigned char**, size_t*, size_t*, unsigned char);

int mushspace_put_textual(
	const mushspace* space, mushbounds bounds,
	mushcell     **   bufp, size_t*   buflenp,
	unsigned char** wsbufp, size_t* wsbuflenp,
	void(*putrow)(const mushcell*, size_t, void*),
	void(*put)   (unsigned char, void*),
	void* pdat)
{
	if (!bufp != !buflenp) {
		bufp    = NULL;
		buflenp = NULL;
	}
	if (!wsbufp != !wsbuflenp) {
		wsbufp    = NULL;
		wsbuflenp = NULL;
	}

	mushcell      *  buf    =   bufp    ? *  bufp    : NULL;
	unsigned char *wsbuf    = wsbufp    ? *wsbufp    : NULL;
	size_t           buflen =   buflenp ? *  buflenp : 0,
	               wsbuflen = wsbuflenp ? *wsbuflenp : 0;

	// Clamp end to loose bounds: no point in going beyond them. Don't clamp the
	// beginning: leading whitespace is not invisible.
	mushcoords lbeg, lend;
	mushspace_get_loose_bounds(space, &lbeg, &lend);
	mushcoords_min_into(&bounds.end, lend);

	int ret = MUSHERR_OOM;

	mushcoords c;
	size_t i = 0, w = 0;

#if MUSHSPACE_DIM >= 3
	for (c.z = bounds.beg.z; c.z <= bounds.end.z; ++c.z) {
#endif
#if MUSHSPACE_DIM >= 2
		for (c.y = bounds.beg.y; c.y <= bounds.end.y; ++c.y) {
#endif
			for (c.x = bounds.beg.x; c.x <= bounds.end.x; ++c.x) {
				if (i == buflen) {
					mushcell *p = realloc(buf,
						(buflen ? (buflen *= 2) : (buflen += 1024)) * sizeof *buf);
					if (!p)
						goto end;
					buf = p;
				}

				switch (buf[i++] = mushspace_get(space, c)) {
				case '\r':
					if (c.x < bounds.end.x) {
						++c.x;
						if (mushspace_get(space, c) != '\n')
							--c.x;
					}
				case '\n':
					if (!put_textual_row(buf, &i, wsbuf, &w, putrow, put, pdat)
					 &&  put_textual_add_ws(&wsbuf, &wsbuflen, &w, '\n'))
						goto end;
					break;

				case '\f': {
					put_textual_page(buf, &i, wsbuf, &w, putrow, put, pdat);

					// Always buffer this instead of outputting it: form feeds go
					// between pages, not after each one.
					if (!put_textual_add_ws(&wsbuf, &wsbuflen, &w, '\f'))
						goto end;
				}}
			}
#if MUSHSPACE_DIM >= 2
			if (!put_textual_row(buf, &i, wsbuf, &w, putrow, put, pdat))
				if (!put_textual_add_ws(&wsbuf, &wsbuflen, &w, '\n'))
					goto end;
		}
#endif
#if MUSHSPACE_DIM >= 3
		put_textual_page(buf, &i, wsbuf, &w, putrow, put, pdat);

		// Don't possibly force a reallocation for something that we know we
		// won't use: don't add a form feed at end.z.
		if (c.z < bounds.end.z
		 && !put_textual_add_ws(&wsbuf, &wsbuflen, &w, '\f'))
			goto end;
	}
#endif
	put_textual_row(buf, &i, wsbuf, &w, putrow, put, pdat);
	ret = MUSHERR_NONE;

end:
	if (bufp) {
		*bufp    = buf;
		*buflenp = buflen;
	} else
		free(buf);

	if (wsbufp) {
		*wsbufp    = wsbuf;
		*wsbuflenp = wsbuflen;
	} else
		free(wsbuf);

	return ret;
}

static bool put_textual_row(
	const mushcell     *   buf, size_t* i,
	const unsigned char* wsbuf, size_t* w,
	void(*putrow)(const mushcell*, size_t, void*),
	void(*put)   (unsigned char, void*),
	void* pdat)
{
	if (!*i)
		return false;

	// Drop spaces before EOL.
	size_t j;
	for (j = *i-1; j-- > 0 && buf[j] == ' ';);

	if (!(*i = ++j))
		return false;

	for (size_t k = 0; k < *w; ++k)
		put(wsbuf[k], pdat);
	putrow(buf, j, pdat);
	put('\n', pdat);
	*i = *w = 0;
	return true;
}

static void put_textual_page(
	const mushcell     *   buf, size_t* i,
	const unsigned char* wsbuf, size_t* w,
	void(*putrow)(const mushcell*, size_t, void*),
	void(*put)   (unsigned char, void*),
	void* pdat)
{
	// Drop trailing newlines at EOP.
	size_t j;
	for (j = *w; j-- > 0 && wsbuf[j] == '\n';);
	*w = j + 1;

	put_textual_row(buf, i, wsbuf, w, putrow, put, pdat);
}

static bool put_textual_add_ws(
	unsigned char** wsbuf, size_t* wsbuflen, size_t* w, unsigned char ws)
{
	if (*w == *wsbuflen) {
		unsigned char *p =
			realloc(*wsbuf, *wsbuflen ? (*wsbuflen *= 2) : (*wsbuflen += 64));
		if (!p)
			return false;
		*wsbuf = p;
	}
	(*wsbuf)[(*w)++] = ws;
	return true;
}
