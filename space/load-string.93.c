// File created: 2012-01-29 11:05:32

#include "space/load-string.all.h"

#include "lib/icu/utf.h"

static bool newline(bool* got_cr, mushcoords* pos) {
   *got_cr = false;
   pos->x = 0;
   return ++pos->y >= 25;
}

#define define_load_string(SUF, C, NEXT) \
   void MUSHSPACE_CAT(mushspace_load_string, SUF)( \
      mushspace* space, const C* str, size_t len) \
   { \
      bool got_cr = false; \
      mushcoords pos = MUSHCOORDS(0,0,0); \
\
      for (const C *str_end = str + len; str < str_end;) { \
         UChar32 c32; \
         NEXT(str, str_end, c32); \
         mushcell c = (mushcell)c32; \
\
         switch (c) { \
         case '\r': got_cr = true; break; \
         case '\n': if (newline(&got_cr, &pos)) return; else break; \
         default: \
            if (got_cr && newline(&got_cr, &pos)) \
               return; \
\
            if (c != ' ') \
               mushstaticaabb_put(&space->box, pos, c); \
\
            if (++pos.x < 80) \
               break; \
\
            /* Skip to and past EOL after column 80. */ \
            while (str < str_end) { \
               c = (mushcell)*str++; \
               switch (c) { \
               case '\r': got_cr = true; break; \
               default:   if (!got_cr) break; \
               case '\n': if (newline(&got_cr, &pos)) return; \
                          else                        goto skipped; \
               } \
            } \
   skipped: \
            break; \
         } \
      } \
   }

#define PLAIN_NEXT(s, s_end, c) do { (void)s_end; (c = (*(s)++)); } while (0)
define_load_string(, unsigned char,  PLAIN_NEXT)
define_load_string(_utf8,  uint8_t,  U8_NEXT_PTR)
define_load_string(_utf16, uint16_t, U16_NEXT_PTR)
define_load_string(_cell,  mushcell, PLAIN_NEXT)
