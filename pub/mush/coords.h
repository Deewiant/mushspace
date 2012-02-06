// File created: 2012-02-06 19:13:57

#ifndef MUSH_COORDS_H
#define MUSH_COORDS_H

#include <stdbool.h>

#include "mush/cell.h"

typedef union mushcoords1 {
#pragma pack(push)
#pragma pack(1)
   struct { mushcell x; };
#pragma pack(pop)
   mushcell v[1];
} mushcoords1;
typedef union mushcoords2 {
#pragma pack(push)
#pragma pack(1)
   struct { mushcell x, y; };
#pragma pack(pop)
   mushcell v[2];
} mushcoords2;
typedef union mushcoords3 {
#pragma pack(push)
#pragma pack(1)
   struct { mushcell x, y, z; };
#pragma pack(pop)
   mushcell v[3];
} mushcoords3;
typedef union mushcoords93 {
#pragma pack(push)
#pragma pack(1)
   struct { mushcell93 x, y; };
#pragma pack(pop)
   mushcell93 v[2];
} mushcoords93;

#define MUSHCOORDS1_INIT(a)     {{.x = a}}
#define MUSHCOORDS2_INIT(a,b)   {{.x = a, .y = b}}
#define MUSHCOORDS3_INIT(a,b,c) {{.x = a, .y = b, .z = c}}
#define MUSHCOORDS93_INIT(a,b)  {{.x = a, .y = b}}

#define MUSHCOORDS1(a)     ((mushcoords)MUSHCOORDS1_INIT(a))
#define MUSHCOORDS2(a,b)   ((mushcoords)MUSHCOORDS2_INIT(a,b))
#define MUSHCOORDS3(a,b,c) ((mushcoords)MUSHCOORDS3_INIT(a,b,c))
#define MUSHCOORDS93(a,b)  ((mushcoords)MUSHCOORDS93_INIT(a,b))

mushcoords1 mushcoords1_add(mushcoords1, mushcoords1);
mushcoords2 mushcoords2_add(mushcoords2, mushcoords2);
mushcoords3 mushcoords3_add(mushcoords3, mushcoords3);
mushcoords93 mushcoords93_add(mushcoords93, mushcoords93);

mushcoords1 mushcoords1_sub(mushcoords1, mushcoords1);
mushcoords2 mushcoords2_sub(mushcoords2, mushcoords2);
mushcoords3 mushcoords3_sub(mushcoords3, mushcoords3);
mushcoords93 mushcoords93_sub(mushcoords93, mushcoords93);

void mushcoords1_max_into(mushcoords1*, mushcoords1);
void mushcoords2_max_into(mushcoords2*, mushcoords2);
void mushcoords3_max_into(mushcoords3*, mushcoords3);
void mushcoords93_max_into(mushcoords93*, mushcoords93);

void mushcoords1_min_into(mushcoords1*, mushcoords1);
void mushcoords2_min_into(mushcoords2*, mushcoords2);
void mushcoords3_min_into(mushcoords3*, mushcoords3);
void mushcoords93_min_into(mushcoords93*, mushcoords93);

bool mushcoords1_equal(mushcoords1, mushcoords1);
bool mushcoords2_equal(mushcoords2, mushcoords2);
bool mushcoords3_equal(mushcoords3, mushcoords3);
bool mushcoords93_equal(mushcoords93, mushcoords93);

#endif
