// File created: 2012-02-06 19:13:57

#ifndef MUSH_SPACE_H
#define MUSH_SPACE_H

#include <stdlib.h>

#include "mush/bounds.h"
#include "mush/stats.h"

typedef struct mushspace1  mushspace1;
typedef struct mushspace2  mushspace2;
typedef struct mushspace3  mushspace3;
typedef struct mushspace93 mushspace93;

extern const size_t mushspace1_sizeof;
extern const size_t mushspace2_sizeof;
extern const size_t mushspace3_sizeof;
extern const size_t mushspace93_sizeof;

mushspace1 *mushspace1_init(void*, mushstats*);
mushspace2 *mushspace2_init(void*, mushstats*);
mushspace3 *mushspace3_init(void*, mushstats*);
mushspace93 *mushspace93_init(void*);

void mushspace1_free(mushspace1*);
void mushspace2_free(mushspace2*);
void mushspace3_free(mushspace3*);
void mushspace93_free(mushspace93);

mushspace1 *mushspace1_copy(void*, const mushspace1*, mushstats1*);
mushspace2 *mushspace2_copy(void*, const mushspace2*, mushstats2*);
mushspace3 *mushspace3_copy(void*, const mushspace3*, mushstats3*);
mushspace93 *mushspace93_copy(void*, const mushspace93*);

// Returns 0 on success or one of the following possible error codes:
//
// MUSHERR_OOM:     Ran out of memory somewhere.
// MUSHERR_NO_ROOM: The string doesn't fit in the space, i.e. it would overlap
//                  with itself. For instance, trying to binary-load 5
//                  gigabytes of non-space data into a 32-bit space would
//                  cause this error.
int mushspace1_load_string
   (mushspace1*, const char*, size_t, mushcoords1*, mushcoords1, bool);
int mushspace2_load_string
   (mushspace2*, const char*, size_t, mushcoords2*, mushcoords2, bool);
int mushspace3_load_string
   (mushspace3*, const char*, size_t, mushcoords3*, mushcoords3, bool);
int mushspace93_load_string(mushspace93*, const char*, size_t);

int mushspace1_load_string_utf8
   (mushspace1*, const uint8_t*, size_t, mushcoords1*, mushcoords1, bool);
int mushspace2_load_string_utf8
   (mushspace2*, const uint8_t*, size_t, mushcoords2*, mushcoords2, bool);
int mushspace3_load_string_utf8
   (mushspace3*, const uint8_t*, size_t, mushcoords3*, mushcoords3, bool);
int mushspace93_load_string_utf8(mushspace93*, const uint8_t*, size_t);

int mushspace1_load_string_utf16
   (mushspace1*, const uint16_t*, size_t, mushcoords1*, mushcoords1, bool);
int mushspace2_load_string_utf16
   (mushspace2*, const uint16_t*, size_t, mushcoords2*, mushcoords2, bool);
int mushspace3_load_string_utf16
   (mushspace3*, const uint16_t*, size_t, mushcoords3*, mushcoords3, bool);
int mushspace93_load_string_utf16(mushspace93*, const uint16_t*, size_t);

int mushspace1_load_string_cell
   (mushspace1*, const mushcell*, size_t, mushcoords1*, mushcoords1, bool);
int mushspace2_load_string_cell
   (mushspace2*, const mushcell*, size_t, mushcoords2*, mushcoords2, bool);
int mushspace3_load_string_cell
   (mushspace3*, const mushcell*, size_t, mushcoords3*, mushcoords3, bool);
int mushspace93_load_string_cell(mushspace93*, const mushcell93*, size_t);

mushcell mushspace1_get(const mushspace1*, mushcoords1);
mushcell mushspace2_get(const mushspace2*, mushcoords2);
mushcell mushspace3_get(const mushspace3*, mushcoords3);
mushcell mushspace93_get(const mushspace93*, mushcoords93);

int mushspace1_put(mushspace1*, mushcoords1, mushcell);
int mushspace2_put(mushspace2*, mushcoords2, mushcell);
int mushspace3_put(mushspace3*, mushcoords3, mushcell);
int mushspace93_put(mushspace93*, mushcoords93, mushcell93);

void mushspace1_get_loose_bounds(const mushspace1*, mushbounds1*);
void mushspace2_get_loose_bounds(const mushspace2*, mushbounds2*);
void mushspace3_get_loose_bounds(const mushspace3*, mushbounds3*);
void mushspace93_get_loose_bounds(const mushspace93*, mushbounds93*);

bool mushspace1_get_tight_bounds(mushspace1*, mushbounds1*);
bool mushspace2_get_tight_bounds(mushspace2*, mushbounds2*);
bool mushspace3_get_tight_bounds(mushspace3*, mushbounds3*);
bool mushspace93_get_tight_bounds(const mushspace93*, mushbounds93*);

typedef struct musharr_mushcell {
   mushcell *ptr;
   size_t len;
} musharr_mushcell;
typedef struct musharr_mushcell93 {
   mushcell93 *ptr;
   size_t len;
} musharr_mushcell93;

void mushspace1_map_existing(
   mushspace1*, mushbounds1,
   void(*)(musharr_mushcell, void*), void(*)(size_t, void*), void*);
void mushspace2_map_existing(
   mushspace2*, mushbounds2,
   void(*)(musharr_mushcell, void*), void(*)(size_t, void*), void*);
void mushspace3_map_existing(
   mushspace3*, mushbounds3,
   void(*)(musharr_mushcell, void*), void(*)(size_t, void*), void*);
void mushspace93_map_existing(
   mushspace93*, mushbounds93,
   void(*)(musharr_mushcell, void*), void(*)(size_t, void*), void*);

int mushspace1_map(mushspace1*, mushbounds1,
                   void(*)(musharr_mushcell, void*), void*);
int mushspace2_map(mushspace2*, mushbounds2,
                   void(*)(musharr_mushcell, void*), void*);
int mushspace3_map(mushspace3*, mushbounds3,
                   void(*)(musharr_mushcell, void*), void*);

void mushspace1_put_binary(const mushspace1*, mushbounds1,
                           void(*)(mushcell, void*),
                           void*);
void mushspace2_put_binary(const mushspace2*, mushbounds2,
                           void(*)(mushcell, void*),
                           void(*)(unsigned char, void*),
                           void*);
void mushspace3_put_binary(const mushspace3*, mushbounds3,
                           void(*)(mushcell, void*),
                           void(*)(unsigned char, void*),
                           void*);
void mushspace93_put_binary(const mushspace93*, mushbounds93,
                            void(*)(mushcell93, void*),
                            void(*)(unsigned char, void*),
                            void*);

int mushspace1_put_textual(const mushspace1*, mushbounds1,
                           mushcell**, size_t*, unsigned char**, size_t*,
                           void(*)(const mushcell*, size_t, void*),
                           void(*)(unsigned char, void*), void*);
int mushspace2_put_textual(const mushspace2*, mushbounds2,
                           mushcell**, size_t*, unsigned char**, size_t*,
                           void(*)(const mushcell*, size_t, void*),
                           void(*)(unsigned char, void*), void*);
int mushspace3_put_textual(const mushspace3*, mushbounds3,
                           mushcell**, size_t*, unsigned char**, size_t*,
                           void(*)(const mushcell*, size_t, void*),
                           void(*)(unsigned char, void*), void*);
int mushspace93_put_textual(const mushspace93*, mushbounds93,
                            mushcell93**, size_t*, unsigned char**, size_t*,
                            void(*)(const mushcell93*, size_t, void*),
                            void(*)(unsigned char, void*), void*);

#endif
