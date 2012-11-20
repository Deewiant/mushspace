// File created: 2011-08-06 16:12:12

#ifndef MUSHSPACE_COORDS_H
#define MUSHSPACE_COORDS_H

#include <stdbool.h>

#include "cell.both.h"
#include "compilers.any.h"
#include "typenames.any.h"

#define mushcoords MUSHSPACE_NAME(mushcoords)

typedef union mushcoords {
   MUSH_PACKED_STRUCT
   {
      mushcell x;
#if MUSHSPACE_DIM >= 2
      mushcell y;
#endif
#if MUSHSPACE_DIM >= 3
      mushcell z;
#endif
   };
   MUSH_PACKED_STRUCT_END

   mushcell v[MUSHSPACE_DIM];
} mushcoords;

#define MUSHCOORDS_INIT MUSHSPACE_CAT(MUSHCOORDS_INIT,MUSHSPACE_DIM)
#define MUSHCOORDS_INIT1(a,b,c) {{.x = a}}
#define MUSHCOORDS_INIT2(a,b,c) {{.x = a, .y = b}}
#define MUSHCOORDS_INIT3(a,b,c) {{.x = a, .y = b, .z = c}}

#define MUSHCOORDS(a,b,c) ((mushcoords)MUSHCOORDS_INIT(a,b,c))

#define mushcoords_add          MUSHSPACE_CAT(mushcoords,_add)
#define mushcoords_sub          MUSHSPACE_CAT(mushcoords,_sub)
#define mushcoords_add_into     MUSHSPACE_CAT(mushcoords,_add_into)
#define mushcoords_sub_into     MUSHSPACE_CAT(mushcoords,_sub_into)
#define mushcoords_equal        MUSHSPACE_CAT(mushcoords,_equal)
#define mushcoords_muls         MUSHSPACE_CAT(mushcoords,_muls)
#define mushcoords_adds_clamped MUSHSPACE_CAT(mushcoords,_adds_clamped)
#define mushcoords_subs_clamped MUSHSPACE_CAT(mushcoords,_subs_clamped)
#define mushcoords_max_into     MUSHSPACE_CAT(mushcoords,_max_into)
#define mushcoords_min_into     MUSHSPACE_CAT(mushcoords,_min_into)

mushcoords mushcoords_add(mushcoords, mushcoords);
mushcoords mushcoords_sub(mushcoords, mushcoords);

void mushcoords_add_into(mushcoords*, mushcoords);
void mushcoords_sub_into(mushcoords*, mushcoords);

void mushcoords_max_into(mushcoords*, mushcoords);
void mushcoords_min_into(mushcoords*, mushcoords);

bool mushcoords_equal(mushcoords, mushcoords);

#if !MUSHSPACE_93
// "s" for "scalar".
mushcoords mushcoords_muls(mushcoords, mushcell);

mushcoords mushcoords_adds_clamped(mushcoords, mushcell);
mushcoords mushcoords_subs_clamped(mushcoords, mushcell);
#endif

#endif
