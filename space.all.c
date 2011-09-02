// File created: 2011-09-02 19:17:34

#include "space.all.h"

#define mushspace_put_textual_row    MUSHSPACE_CAT(mushspace,_put_textual_row)
#define mushspace_put_textual_page   MUSHSPACE_CAT(mushspace,_put_textual_page)
#define mushspace_put_textual_add_ws \
	MUSHSPACE_CAT(mushspace,_put_textual_add_ws)

static bool mushspace_put_textual_row(
	const mushcell*, size_t*, const unsigned char*, size_t*,
	void(*)(const mushcell*, size_t, void*), void(*)(unsigned char, void*),
	void*);

static void mushspace_put_textual_page(
	const mushcell*, size_t*, const unsigned char*, size_t*,
	void(*)(const mushcell*, size_t, void*), void(*)(unsigned char, void*),
	void*);

static bool mushspace_put_textual_add_ws(
	unsigned char**, size_t*, size_t*, unsigned char);

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
				putcell(mushspace_get_nostats(space, c), putdata);

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

int mushspace_put_textual(
	const mushspace* space, mushcoords beg, mushcoords end,
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
	mushcoords_min_into(&end, lend);

	int ret = MUSH_ERR_OOM;

	mushcoords c;
	size_t i = 0, w = 0;

#if MUSHSPACE_DIM >= 3
	for (c.z = beg.z; c.z <= end.z; ++c.z) {
#endif
#if MUSHSPACE_DIM >= 2
		for (c.y = beg.y; c.y <= end.y; ++c.y) {
#endif
			for (c.x = beg.x; c.x <= end.x; ++c.x) {
				if (i == buflen) {
					mushcell *p = realloc(buf,
						(buflen ? (buflen *= 2) : (buflen += 1024)) * sizeof *buf);
					if (!p)
						goto end;
					buf = p;
				}

				switch (buf[i++] = mushspace_get_nostats(space, c)) {
				case '\r':
					if (c.x < end.x) {
						++c.x;
						if (mushspace_get_nostats(space, c) != '\n')
							--c.x;
					}
				case '\n':
					if (!mushspace_put_textual_row(buf, &i, wsbuf, &w,
					                               putrow, put, pdat)
					 &&  mushspace_put_textual_add_ws(&wsbuf, &wsbuflen, &w, '\n'))
						goto end;
					break;

				case '\f': {
					mushspace_put_textual_page(buf, &i, wsbuf, &w,
					                           putrow, put, pdat);

					// Always buffer this instead of outputting it: form feeds go
					// between pages, not after each one.
					if (!mushspace_put_textual_add_ws(&wsbuf, &wsbuflen, &w, '\f'))
						goto end;
				}}
			}
#if MUSHSPACE_DIM >= 2
			if (!mushspace_put_textual_row(buf, &i, wsbuf, &w, putrow, put, pdat))
				if (!mushspace_put_textual_add_ws(&wsbuf, &wsbuflen, &w, '\n'))
					goto end;
		}
#endif
#if MUSHSPACE_DIM >= 3
		mushspace_put_textual_page(buf, &i, wsbuf, &w, putrow, put, pdat);

		// Don't possibly force a reallocation for something that we know we
		// won't use: don't add a form feed at end.z.
		if (c.z < end.z
		 && !mushspace_put_textual_add_ws(&wsbuf, &wsbuflen, &w, '\f'))
			goto end;
	}
#endif
	mushspace_put_textual_row(buf, &i, wsbuf, &w, putrow, put, pdat);
	ret = MUSH_ERR_NONE;

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
static bool mushspace_put_textual_row(
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
static void mushspace_put_textual_page(
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

	mushspace_put_textual_row(buf, i, wsbuf, w, putrow, put, pdat);
}
static bool mushspace_put_textual_add_ws(
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
