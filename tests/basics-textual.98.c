// File created: 2012-10-13 22:59:38

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
#define mushspace                   NAME(mushspace)
#define mushspace_sizeof            CAT(mushspace,_sizeof)
#define mushspace_init              CAT(mushspace,_init)
#define mushspace_free              CAT(mushspace,_free)
#define mushspace_load_string       CAT(mushspace,_load_string)
#define mushspace_load_string_cell  CAT(mushspace,_load_string_cell)
#define mushspace_load_string_utf8  CAT(mushspace,_load_string_utf8)
#define mushspace_load_string_utf16 CAT(mushspace,_load_string_utf16)
#define mushspace_get               CAT(mushspace,_get)
#define mushspace_put               CAT(mushspace,_put)
#define mushspace_copy              CAT(mushspace,_copy)

#define DATA_LEN ((1 << 6) * 21)

static size_t dummy(const void* a, const void* b, size_t c) {
   (void)a; (void)b;
   return c;
}

int main(int argc, char **argv) {
   // We don't check bounds here: we'd end up essentially duplicating the logic
   // from load_string, and there's little point in that.
   tap_n(3*4 + 3*2 + 2);

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

   codepoint_reader cp_reader;

   mushcoords pos, pos_next;
   bool cr;

#define POS_INIT \
   pos_next = beg; \
   cr = false;

#define CP_POS(cp) \
   pos = pos_next; \
   switch (cp) { \
   default: \
      if (MUSHSPACE_DIM > 1 && cr) { \
         cr = false; \
         pos = MUSHCOORDS(beg.x, pos.y + 1, pos.z); \
      } \
      pos_next = MUSHCOORDS(pos.x + 1, pos.y, pos.z); \
      break; \
   case '\r': \
      if (MUSHSPACE_DIM > 1) { \
         if (cr) \
            pos_next = pos = MUSHCOORDS(beg.x, pos.y + 1, pos.z); \
         cr = true; \
      } \
      continue; \
   case '\n': \
      if (MUSHSPACE_DIM > 1) { \
         cr = false; \
         pos_next = pos = MUSHCOORDS(beg.x, pos.y + 1, pos.z); \
      } \
      continue; \
   case '\f': \
      if (MUSHSPACE_DIM > 2) { \
         cr = false; \
         pos_next = pos = MUSHCOORDS(beg.x, beg.y, pos.z + 1); \
      } else if (cr) { \
         cr = false; \
         pos_next = pos = MUSHCOORDS(beg.x, pos.y + 1, pos.z); \
      } \
      continue; \
   }

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
   if (BLOWUP) { \
      encoded_data##suf = \
         malloc((codepoints * BLOWUP) * sizeof *encoded_data##suf); \
      encoded_len##suf = ENCODER(data, encoded_data##suf, codepoints); \
   } \
   \
   err = mushspace_load_string##suf( \
      space, encoded_data##suf, encoded_len##suf, &end, beg, false); \
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
   if (BLOWUP) \
      free(encoded_data##suf); \
   \
   ok = true; \
   POS_INIT; \
   FOREACH_CP(suf) { \
      CP_POS(cp##suf); \
      mushcell gc = mushspace_get(space, pos); \
      if (gc == cp##suf) \
         continue; \
      ok = false; \
      tap_not_ok("get doesn't match data given to load_string" #suf); \
      printf("  ---\n" \
             "  expected: %" MUSHCELL_PRIx "\n" \
             "  got: %" MUSHCELL_PRIx "\n" \
             "  failed index: %zu\n", \
             cp##suf, gc, ii##suf); \
      printf("  failed pos: ("); \
      for (uint8_t i = 0; i < MUSHSPACE_DIM; ++i) \
         printf(" %" MUSHCELL_PRId, pos.v[i]); \
      printf(" )\n" \
             "  ...\n"); \
      break; \
   } \
   if (ok) \
      tap_ok("get matches data given to load_string" #suf); \
   \
   mushspace_free(space);

#define DIRECT_FOREACH_CP(s) \
   mushcell cp##s; \
   size_t ii##s = 0; \
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

   mushcoords *saved_pos = malloc(data_cell_count * sizeof *saved_pos);

#define PUT(FOREACH_CELL, S, GET_POS, SAVE_POS) \
   space = mushspace_init(space_buf, NULL); \
   if (!space) { \
      tap_not_ok("init returned null"); \
      tap_skip_remaining("init failed"); \
      free(saved_pos); \
      return 1; \
   } \
   tap_ok("init succeeded"); \
   POS_INIT; \
   FOREACH_CELL { \
      GET_POS(data_cell[i]); \
      if (SAVE_POS) \
         saved_pos[i] = pos; \
      err = mushspace_put(space, pos, data_cell[i]); \
      if (!err) \
         continue; \
      \
      tap_not_ok("put returned an error" S); \
      printf("  ---\n" \
             "  error code: %d\n" \
             "  failed index: %zu\n", \
             err, i); \
      printf("  failed pos: ("); \
      for (uint8_t j = 0; j < MUSHSPACE_DIM; ++j) \
         printf(" %" MUSHCELL_PRId, pos.v[j]); \
      printf(" )\n" \
             "  ...\n"); \
      tap_skip_remaining("put failed"); \
      free(saved_pos); \
      return 1; \
   } \
   tap_ok("every put succeeded" S); \
   \
   ok = true; \
   POS_INIT; \
   for (size_t i = 0; i < data_cell_count; ++i) { \
      mushcell dc = data_cell[i]; \
      GET_POS(dc); \
      mushcell gc = mushspace_get(space, pos); \
      if (gc == dc) \
         continue; \
      ok = false; \
      tap_not_ok("get doesn't match what was put" S); \
      printf("  ---\n" \
             "  failed index: %zu\n" \
             "  expected: %" MUSHCELL_PRIx "\n" \
             "  got: %" MUSHCELL_PRIx "\n", \
             i, dc, gc); \
      printf("  failed pos: ("); \
      for (uint8_t j = 0; j < MUSHSPACE_DIM; ++j) \
         printf(" %" MUSHCELL_PRId, pos.v[i]); \
      printf(" )\n" \
             "  ...\n"); \
      break; \
   } \
   if (ok) \
      tap_ok("get matches what was put" S);

#define FORWARD for (size_t i = 0; i < data_cell_count; ++i)
#define REVERSE for (size_t i = data_cell_count; i--;)

#define SAVED_POS(_) pos = saved_pos[i];

   PUT(FORWARD, " (forward order)", CP_POS, true);
   mushspace_free(space);
   PUT(REVERSE, " (reverse order)", SAVED_POS, false);
   free(saved_pos);

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
   POS_INIT;
   for (size_t i = 0; i < DATA_LEN / sizeof *data_cell; ++i) {
      mushcell dc = data_cell[i];
      CP_POS(dc);
      mushcell gc = mushspace_get(space, pos);
      if (gc == dc)
         continue;
      ok = false;
      tap_not_ok("get in copy doesn't match data");
      printf("  ---\n"
             "  failed index: %zu\n"
             "  expected: %" MUSHCELL_PRIx "\n"
             "  got: %" MUSHCELL_PRIx "\n",
             i, dc, gc);
      printf("  failed pos: (");
      for (uint8_t j = 0; j < MUSHSPACE_DIM; ++j)
         printf(" %" MUSHCELL_PRId, pos.v[i]);
      printf(" )\n"
             "  ...\n");
      break;
   }
   if (ok)
      tap_ok("get in copy matched data");

   free(data);
   mushspace_free(space);
   free(space_buf2);
}
