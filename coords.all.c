// File created: 2011-08-07 18:20:32

#include "coords.all.h"

void mushcoords_max_into(mushcoords* a, mushcoords b) {
   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
      a->v[i] = mushcell_max(a->v[i], b.v[i]);
}
void mushcoords_min_into(mushcoords* a, mushcoords b) {
   for (mushdim i = 0; i < MUSHSPACE_DIM; ++i)
      a->v[i] = mushcell_min(a->v[i], b.v[i]);
}

#define DEFINE_OP(NAME) \
   mushcoords mushcoords_##NAME(mushcoords a, mushcoords b) { \
      mushcoords x = a; \
      for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) \
         x.v[i] = mushcell_##NAME(x.v[i], b.v[i]); \
      return x; \
   } \
   void mushcoords_##NAME##_into(mushcoords* a, mushcoords b) { \
      for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) \
         a->v[i] = mushcell_##NAME(a->v[i], b.v[i]); \
   } \
   mushcoords mushcoords_##NAME##s(mushcoords a, mushcell b) { \
      mushcoords x = a; \
      for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) \
         x.v[i] = mushcell_##NAME(x.v[i], b); \
      return x; \
   }

#define DEFINE_CLAMPED_OP(NAME) \
   mushcoords mushcoords_##NAME##s_clamped(mushcoords a, mushcell b) { \
      mushcoords x = a; \
      for (mushdim i = 0; i < MUSHSPACE_DIM; ++i) \
         x.v[i] = mushcell_##NAME##_clamped(x.v[i], b); \
      return x; \
   }

DEFINE_OP(add)
DEFINE_OP(sub)
DEFINE_OP(mul)

#if !MUSHSPACE_93
DEFINE_CLAMPED_OP(add)
DEFINE_CLAMPED_OP(sub)
#endif

#if !MUSHSPACE_93 || defined(MUSH_ENABLE_INFINITE_LOOP_DETECTION)

bool mushcoords_equal(mushcoords a, mushcoords b) {
   // Yes, peeling the loop is worth it.
   return a.v[0] == b.v[0]
#if MUSHSPACE_DIM >= 2
       && a.v[1] == b.v[1]
#if MUSHSPACE_DIM >= 3
       && a.v[2] == b.v[2]
#endif
#endif
   ;
}

#endif
