// File created: 2012-10-07 22:15:25

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

#include <mush/space.h>

#include "coords.h"
#include "typenames.h"

#include "util/random.h"
#include "util/tap.h"
#include "util/utf.h"

#define mushcoords                  NAME(mushcoords)
#define mushcoords_add              CAT(mushcoords,_add)
#define mushbounds                  NAME(mushbounds)
#define mushspace                   NAME(mushspace)
#define mushspace_sizeof            CAT(mushspace,_sizeof)
#define mushspace_init              CAT(mushspace,_init)
#define mushspace_free              CAT(mushspace,_free)
#define mushspace_load_string       CAT(mushspace,_load_string)
#define mushspace_load_string_cell  CAT(mushspace,_load_string_cell)
#define mushspace_load_string_utf8  CAT(mushspace,_load_string_utf8)
#define mushspace_load_string_utf16 CAT(mushspace,_load_string_utf16)
#define mushspace_get               CAT(mushspace,_get)
#define mushspace_get_loose_bounds  CAT(mushspace,_get_loose_bounds)
#define mushspace_get_tight_bounds  CAT(mushspace,_get_tight_bounds)
#define mushspace_put               CAT(mushspace,_put)
#define mushspace_copy              CAT(mushspace,_copy)

#define DATA_LEN ((1 << 20) * 21)

static size_t dummy(const void* a, const void* b, size_t c) {
   (void)a; (void)b;
   return c;
}

int main(int argc, char **argv) {
   tap_n((4+5)*4 + (3+5)*2 + (2+5));

   if (argc > 1) {
      long s = atol(argv[1]);
      init_by_array((uint32_t*)&s, sizeof s / sizeof(uint32_t));
   } else {
      time_t s = time(NULL);
      init_by_array((uint32_t*)&s, sizeof s / sizeof(uint32_t));
   }

   unsigned char *data = malloc(DATA_LEN);
   random_fill(data, DATA_LEN);

   const size_t codepoints = DATA_LEN * CHAR_BIT / 21;

   mushcoords beg = MUSHCOORDS(1000000,1000000,1000000), end;

   void *space_buf = malloc(mushspace_sizeof);
   mushspace *space;

   bool ok;
   int err;
   mushbounds bounds, expected_loose, expected_tight;

   size_t spaces_beg, spaces_end;

   codepoint_reader cp_reader;

#define BOUNDS_CHECK \
   mushspace_get_loose_bounds(space, &bounds); \
   \
   tap_leqcos(bounds.beg, expected_loose.beg, \
              "get_loose_bounds reports appropriate beg", \
              "get_loose_bounds reports too large beg"); \
   tap_geqcos(bounds.end, expected_loose.end, \
              "get_loose_bounds reports appropriate end", \
              "get_loose_bounds reports too small end"); \
   \
   ok = mushspace_get_tight_bounds(space, &bounds); \
   tap_bool(ok, "get_tight_bounds says that the space is nonempty", \
                "get_tight_bounds says that the space is empty"); \
   \
   tap_eqcos(bounds.beg, expected_tight.beg, \
             "get_tight_bounds reports correct beg", \
             "get_tight_bounds reports incorrect beg"); \
   tap_eqcos(bounds.end, expected_tight.end, \
             "get_tight_bounds reports correct end", \
             "get_tight_bounds reports incorrect end"); \

#define LOAD_STRING(suf, T, ENCODER, BLOWUP, FOREACH_CP) \
   space = mushspace_init(space_buf, NULL); \
   if (!space) { \
      tap_not_ok("init returned null"); \
      tap_skip_remaining("init failed"); \
      return 1; \
   } \
   tap_ok("init succeeded"); \
   \
   T *encoded_data##suf = (T*)data; \
   size_t encoded_len##suf = DATA_LEN / sizeof *encoded_data##suf; \
   size_t cells##suf = encoded_len##suf; \
   if (BLOWUP) { \
      encoded_data##suf = \
         malloc((codepoints * BLOWUP) * sizeof *encoded_data##suf); \
      encoded_len##suf = ENCODER(data, encoded_data##suf, codepoints); \
      cells##suf = codepoints; \
   } \
   \
   err = mushspace_load_string##suf( \
      space, encoded_data##suf, encoded_len##suf, &end, beg, true); \
   \
   if (err) { \
      tap_not_ok("load_string" #suf " returned an error"); \
      printf("  ---\n" \
             "  code: %d\n" \
             "  ...\n", \
             err); \
      tap_skip_remaining("load_string" #suf " failed"); \
      return 1; \
   } else \
      tap_ok("load_string" #suf " returned ok"); \
   \
   spaces_beg = 0; \
   spaces_end = 0; \
   \
   while (encoded_data##suf[spaces_beg] == ' ') { \
      if (++spaces_beg == encoded_len##suf) { \
         tap_skip_remaining("space is all spaces"); \
         return 0; \
      } \
   } \
   \
   expected_loose.beg = expected_tight.beg = beg; \
   expected_loose.end = expected_tight.end = \
      mushcoords_add(beg, MUSHCOORDS(cells##suf-1, 0, 0)); \
   \
   size_t s##suf = encoded_len##suf; \
   while (s##suf-- > 0 && encoded_data##suf[s##suf] == ' '); \
   spaces_end = encoded_len##suf - s##suf - 1; \
   \
   expected_loose.beg.x = expected_tight.beg.x += spaces_beg; \
   expected_loose.end.x = expected_tight.end.x -= spaces_end; \
   \
   if (BLOWUP) \
      free(encoded_data##suf); \
   \
   tap_eqcos(end, expected_tight.end, \
             "load_string" #suf " reports correct end", \
             "load_string" #suf " reports incorrect end"); \
   \
   ok = true; \
   FOREACH_CP(suf) { \
      mushcell gc = \
         mushspace_get(space, mushcoords_add(beg, MUSHCOORDS(ii##suf,0,0))); \
      if (gc == cp##suf) \
         continue; \
      ok = false; \
      tap_not_ok("get doesn't match data given to load_string" #suf); \
      printf("  ---\n" \
             "  failed index: %zu\n" \
             "  expected: %" MUSHCELL_PRIx "\n" \
             "  got: %" MUSHCELL_PRIx "\n" \
             "  ...\n", \
             ii##suf, cp##suf, gc); \
      break; \
   } \
   if (ok) \
      tap_ok("get matches data given to load_string" #suf); \
   \
   BOUNDS_CHECK; \
   mushspace_free(space);

#define DIRECT_FOREACH_CP(s) \
   size_t ii##s = 0; \
   mushcell cp##s; \
   for (; ii##s < encoded_len##s && (cp##s = encoded_data##s[ii##s], true); \
        ++ii##s)

#define READER_FOREACH_CP(s) \
   cp_reader = make_codepoint_reader(data, codepoints); \
   size_t ii##s = 0; \
   for (mushcell cp##s; (cp##s = next_codepoint(&cp_reader)) != UINT32_MAX; \
        ++ii##s)

   LOAD_STRING(, unsigned char,  dummy, 0,        DIRECT_FOREACH_CP);
   LOAD_STRING(_cell,  mushcell, dummy, 0,        DIRECT_FOREACH_CP);
   LOAD_STRING(_utf8,  uint8_t,  encode_utf8,  4, READER_FOREACH_CP);
   LOAD_STRING(_utf16, uint16_t, encode_utf16, 2, READER_FOREACH_CP);

   const mushcell *data_cell       = (const mushcell*)data;
   const size_t    data_cell_count = DATA_LEN / sizeof *data_cell;

   spaces_beg = 0;
   spaces_end = 0;

   for (size_t i = 0; i < data_cell_count && data_cell[i++] == ' ';)
      ++spaces_beg;
   for (size_t i = data_cell_count; i-- > 0 && data_cell[i] == ' ';)
      ++spaces_end;

   expected_loose.beg = expected_tight.beg = beg;
   expected_loose.end = expected_tight.end =
      mushcoords_add(beg, MUSHCOORDS(data_cell_count - 1, 0, 0));
   expected_loose.beg.x = expected_tight.beg.x += spaces_beg;
   expected_loose.end.x = expected_tight.end.x -= spaces_beg;

#define PUT(FOREACH_CELL, S) \
   space = mushspace_init(space_buf, NULL); \
   if (!space) { \
      tap_not_ok("init returned null"); \
      tap_skip_remaining("init failed"); \
      return 1; \
   } \
   tap_ok("init succeeded"); \
   FOREACH_CELL { \
      err = mushspace_put(space, mushcoords_add(beg, MUSHCOORDS(i, 0, 0)), \
                          data_cell[i]); \
      if (!err) \
         continue; \
      \
      tap_not_ok("put returned an error" S); \
      printf("  ---\n" \
             "  failed index: %zu\n" \
             "  error code: %d\n" \
             "  ...\n", \
             i, err); \
      tap_skip_remaining("put failed"); \
      return 1; \
   } \
   tap_ok("every put succeeded" S); \
   \
   ok = true; \
   for (size_t i = 0; i < data_cell_count; ++i) { \
      mushcell dc = data_cell[i], \
               gc = mushspace_get(space, \
                                  mushcoords_add(beg, MUSHCOORDS(i, 0, 0))); \
      if (gc == dc) \
         continue; \
      ok = false; \
      tap_not_ok("get doesn't match what was put" S); \
      printf("  ---\n" \
             "  failed index: %zu\n" \
             "  expected: %" MUSHCELL_PRIx "\n" \
             "  got: %" MUSHCELL_PRIx "\n" \
             "  ...\n", \
             i, dc, gc); \
      break; \
   } \
   if (ok) \
      tap_ok("get matches what was put" S); \
   \
   BOUNDS_CHECK;

#define FORWARD for (size_t i = 0; i < data_cell_count; ++i)
#define REVERSE for (size_t i = data_cell_count; i--;)
   PUT(FORWARD, " (forward order)");
   mushspace_free(space);
   PUT(REVERSE, " (reverse order)");

   if (!ok) {
      tap_skip_remaining("won't copy bad space");
      return 1;
   }

   void *space_buf2 = malloc(mushspace_sizeof);

   mushspace *space2 = mushspace_copy(space_buf2, space, NULL);
   mushspace_free(space);
   free(space_buf);

   space = space2;

   if (!space) {
      tap_not_ok("copy returned null");
      tap_skip_remaining("copy failed");
      return 1;
   }
   tap_ok("copy succeeded");

   ok = true;
   for (size_t i = 0; i < DATA_LEN / sizeof *data_cell; ++i) {
      mushcell dc = data_cell[i],
               gc = mushspace_get(space,
                                  mushcoords_add(beg, MUSHCOORDS(i, 0, 0)));
      if (gc == dc)
         continue;
      ok = false;
      tap_not_ok("get in copy doesn't match data");
      printf("  ---\n"
             "  failed index: %zu\n"
             "  expected: %" MUSHCELL_PRIx "\n"
             "  got: %" MUSHCELL_PRIx "\n"
             "  ...\n",
             i, dc, gc);
      break;
   }
   if (ok)
      tap_ok("get in copy matched data");

   BOUNDS_CHECK;

   free(data);
   mushspace_free(space);
   free(space_buf2);
}
