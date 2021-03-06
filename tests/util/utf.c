// File created: 2012-10-08 16:42:13

#include <assert.h>

#include "utf.h"

// Sorry, exotic architectures: we assume CHAR_BIT == 8 here.

codepoint_reader make_codepoint_reader(const void *data, size_t codepoints) {
   codepoint_reader reader;
   reader.data = data;
   reader.codepoints = codepoints;
   reader.last_octet = 0;
   reader.unused_bits = 0;
   return reader;
}

uint32_t next_codepoint(codepoint_reader *reader) {
   if (reader->codepoints == 0)
      return UINT32_MAX;
   --reader->codepoints;

   uint32_t cp = 0;

   // Use the unused bits from last_octet.
   cp |= reader->last_octet & ((1 << reader->unused_bits) - 1);

   uint8_t remaining = 21 - reader->unused_bits;

   // If unused_bits was 6 or 7, we can only read one full octet,
   // otherwise read two.
   if (remaining >= 8) { remaining -= 8; cp <<= 8; cp |= *reader->data++; }
   if (remaining >= 8) { remaining -= 8; cp <<= 8; cp |= *reader->data++; }

   if (remaining) {
      // We still need some bits: get them from the top of data and leave
      // the rest in last_octet.
      assert (remaining < 8);
      reader->last_octet = *reader->data++;
      reader->unused_bits = 8 - remaining;
      cp <<= remaining;
      cp |= (reader->last_octet & ~(0xff >> remaining)) >> reader->unused_bits;
   } else
      reader->unused_bits = 0;

   // If it's unencodable, make it not, by applying some arbitrary operations.
   // Since we use this for Funge-Spaces, make it whitespace: that's more
   // likely to cause issues.
   //
   // Bias plain spaces somewhat. As there are two kinds of newlines, this
   // gives us a 3:2:1 ratio of x:y:z whitespace.
   if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) switch (cp % 6) {
   case 0: case 1: case 2: cp = ' ';  break;
   case 3: cp = '\n'; break;
   case 4: cp = '\r'; break;
   case 5: cp = '\f'; break;
   }

   assert (cp <= 0x10ffff);
   assert (!(cp >= 0xd800 && cp <= 0xdfff));

   return cp;
}

size_t encode_utf8(const void* data, uint8_t* out, size_t codepoints) {
   size_t i = 0;

   codepoint_reader reader = make_codepoint_reader(data, codepoints);
   for (uint32_t cp; (cp = next_codepoint(&reader)) != UINT32_MAX;) {
      if (cp <= 0x7f) {
         out[i++] = (uint8_t)cp;
         continue;
      }
      if (cp <= 0x7ff) {
         out[i++] = 0xc0 | (uint8_t)(cp >> 6);
         out[i++] = 0x80 | (uint8_t)(cp >> 0 & ((1 << 6) - 1));
         continue;
      }
      if (cp <= 0xffff) {
         out[i++] = 0xe0 | (uint8_t) (cp >> 12);
         out[i++] = 0x80 | (uint8_t)((cp >>  6) & ((1 << 6) - 1));
         out[i++] = 0x80 | (uint8_t)((cp >>  0) & ((1 << 6) - 1));
         continue;
      }
      out[i++] = 0xf0 | (uint8_t) (cp >> 18);
      out[i++] = 0x80 | (uint8_t)((cp >> 12) & ((1 << 6) - 1));
      out[i++] = 0x80 | (uint8_t)((cp >>  6) & ((1 << 6) - 1));
      out[i++] = 0x80 | (uint8_t)((cp >>  0) & ((1 << 6) - 1));
      continue;
   }

   return i;
}

size_t encode_utf16(const void* data, uint16_t* out, size_t codepoints) {
   size_t i = 0;

   codepoint_reader reader = make_codepoint_reader(data, codepoints);
   for (uint32_t cp; (cp = next_codepoint(&reader)) != UINT32_MAX;) {
      if (cp <= 0xffff) {
         out[i++] = (uint16_t)cp;
         continue;
      }
      cp -= 0x10000;
      out[i++] = 0xd800 + (uint16_t)(cp >> 10);
      out[i++] = 0xdc00 + (uint16_t)(cp & ((1 << 10) - 1));
   }

   return i;
}
