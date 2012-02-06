// File created: 2012-02-06 19:13:57

#ifndef MUSH_BOUNDS_H
#define MUSH_BOUNDS_H

#include "mush/coords.h"

typedef struct mushbounds1  { mushcoords1  beg, end; } mushbounds1;
typedef struct mushbounds2  { mushcoords2  beg, end; } mushbounds2;
typedef struct mushbounds3  { mushcoords3  beg, end; } mushbounds3;
typedef struct mushbounds93 { mushcoords93 beg, end; } mushbounds93;

bool mushbounds1_contains(const mushbounds1*, mushcoords1);
bool mushbounds2_contains(const mushbounds2*, mushcoords2);
bool mushbounds3_contains(const mushbounds3*, mushcoords3);
bool mushbounds93_contains(const mushbounds93*, mushcoords93);

bool mushbounds1_safe_contains(const mushbounds1*, mushcoords1);
bool mushbounds2_safe_contains(const mushbounds2*, mushcoords2);
bool mushbounds3_safe_contains(const mushbounds3*, mushcoords3);
bool mushbounds93_safe_contains(const mushbounds93*, mushcoords93);

bool mushbounds1_contains_bounds(const mushbounds1*, const mushbounds1*);
bool mushbounds2_contains_bounds(const mushbounds2*, const mushbounds2*);
bool mushbounds3_contains_bounds(const mushbounds3*, const mushbounds3*);
bool mushbounds93_contains_bounds(const mushbounds93*, const mushbounds93*);

bool mushbounds1_overlaps(const mushbounds1*, const mushbounds1*);
bool mushbounds2_overlaps(const mushbounds2*, const mushbounds2*);
bool mushbounds3_overlaps(const mushbounds3*, const mushbounds3*);
bool mushbounds93_overlaps(const mushbounds93*, const mushbounds93*);

#endif
