// File created: 2012-01-27 20:25:04

#include "space/put-textual.all.h"

#include <assert.h>

static bool put_textual_row(
   const mushcell*, size_t*, const char*, size_t*,
   void(*)(const mushcell*, size_t, void*), void(*)(char, void*),
   void*);

static void put_textual_page(
   const mushcell*, size_t*, const char*, size_t*,
   void(*)(const mushcell*, size_t, void*), void(*)(char, void*),
   void*);

#if !MUSHSPACE_93
static bool put_textual_add_ws(char**, size_t*, size_t*, char);
#endif

#if MUSHSPACE_93
void
#else
int
#endif
mushspace_put_textual(
   const mushspace* space, mushbounds bounds,
#if !MUSHSPACE_93
   mushcell**   bufp, size_t*   buflenp,
   char    ** wsbufp, size_t* wsbuflenp,
#endif
   void(*putrow)(const mushcell*, size_t, void*),
   void(*put)   (char, void*),
   void* pdat)
{
#if MUSHSPACE_93
   static const size_t buflen = 80*25, wsbuflen = 80*25;
   mushcell buf[  buflen];
   char   wsbuf[wsbuflen];
#else
   // If we have a buffer but not a length or vice versa, we have neither.
   if (!  bufp != !  buflenp) {   bufp = NULL;   buflenp = NULL; }
   if (!wsbufp != !wsbuflenp) { wsbufp = NULL; wsbuflenp = NULL; }

   mushcell*  buf    =   bufp    ? *  bufp    : NULL;
   char    *wsbuf    = wsbufp    ? *wsbufp    : NULL;
   size_t     buflen =   buflenp ? *  buflenp : 0,
            wsbuflen = wsbuflenp ? *wsbuflenp : 0;
#endif

   // Clamp end to loose bounds: no point in going beyond them. Don't clamp the
   // beginning: leading whitespace is not invisible.
   mushbounds lbounds;
   mushspace_get_loose_bounds(space, &lbounds);
   mushcoords_min_into(&bounds.end, lbounds.end);

#if !MUSHSPACE_93
   int ret = MUSHERR_OOM;
#endif

   mushcoords c;
   size_t i = 0, w = 0;

#if MUSHSPACE_93
#define ADD_WS(ws) do { \
   assert (w < wsbuflen); \
   wsbuf[w++] = (ws); \
} while (0)
#else
#define ADD_WS(ws) do { \
   if (!put_textual_add_ws(&wsbuf, &wsbuflen, &w, ws)) \
      goto end; \
} while (0)
#endif

#if MUSHSPACE_DIM >= 3
   for (c.z = bounds.beg.z; c.z <= bounds.end.z; ++c.z) {
#endif
#if MUSHSPACE_DIM >= 2
      for (c.y = bounds.beg.y; c.y <= bounds.end.y; ++c.y) {
#endif
         for (c.x = bounds.beg.x; c.x <= bounds.end.x; ++c.x) {
#if MUSHSPACE_93
            assert (i < buflen);
#else
            if (i == buflen) {
               mushcell *p = realloc(buf,
                  (buflen ? (buflen *= 2) : (buflen += 1024)) * sizeof *buf);
               if (!p)
                  goto end;
               buf = p;
            }
#endif
            switch (buf[i++] = mushspace_get(space, c)) {
            case '\r':
               if (c.x < bounds.end.x) {
                  ++c.x;
                  if (mushspace_get(space, c) != '\n')
                     --c.x;
               }
            case '\n':
               if (!put_textual_row(buf, &i, wsbuf, &w, putrow, put, pdat))
                  ADD_WS('\n');
               break;

            case '\f': {
               put_textual_page(buf, &i, wsbuf, &w, putrow, put, pdat);

               // Always buffer this instead of outputting it: form feeds go
               // between pages, not after each one.
               ADD_WS('\f');
            }}
         }
#if MUSHSPACE_DIM >= 2
         if (!put_textual_row(buf, &i, wsbuf, &w, putrow, put, pdat))
            ADD_WS('\n');
      }
#endif
#if MUSHSPACE_DIM >= 3
      put_textual_page(buf, &i, wsbuf, &w, putrow, put, pdat);

      // Don't possibly force a reallocation for something that we know we
      // won't use: don't add a form feed at end.z.
      if (c.z < bounds.end.z)
         ADD_WS('\f');
   }
#endif
   put_textual_row(buf, &i, wsbuf, &w, putrow, put, pdat);

#if !MUSHSPACE_93
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
#endif
}

static bool put_textual_row(
   const mushcell*   buf, size_t* i,
   const char*     wsbuf, size_t* w,
   void(*putrow)(const mushcell*, size_t, void*),
   void(*put)   (char, void*),
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
   const mushcell*   buf, size_t* i,
   const char*     wsbuf, size_t* w,
   void(*putrow)(const mushcell*, size_t, void*),
   void(*put)   (char, void*),
   void* pdat)
{
   // Drop trailing newlines at EOP.
   size_t j;
   for (j = *w; j-- > 0 && wsbuf[j] == '\n';);
   *w = j + 1;

   put_textual_row(buf, i, wsbuf, w, putrow, put, pdat);
}

#if !MUSHSPACE_93
static bool put_textual_add_ws(
   char** wsbuf, size_t* wsbuflen, size_t* w, char ws)
{
   if (*w == *wsbuflen) {
      char *p =
         realloc(*wsbuf, *wsbuflen ? (*wsbuflen *= 2) : (*wsbuflen += 64));
      if (!p)
         return false;
      *wsbuf = p;
   }
   (*wsbuf)[(*w)++] = ws;
   return true;
}
#endif
