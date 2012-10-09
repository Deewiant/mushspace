// File created: 2012-10-08 16:42:16

#ifndef UTF_H
#define UTF_H

#include <stdint.h>
#include <stdlib.h>

// Data is eaten in 21-bit chunks.

typedef struct {
   const uint8_t *data;
   size_t         codepoints;
   uint8_t        last_octet, unused_bits;
} codepoint_reader;

codepoint_reader make_codepoint_reader(const void *data, size_t codepoints);
uint32_t next_codepoint(codepoint_reader *reader);

size_t encode_utf8 (const void* data, uint8_t*  out, size_t codepoints);
size_t encode_utf16(const void* data, uint16_t* out, size_t codepoints);

#endif
