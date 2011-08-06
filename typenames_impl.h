// File created: 2011-08-06 17:42:46

#undef mushspace
#undef mushcoords
#undef MUSHSPACE_CAT
#undef MUSHSPACE_CAT2

#ifndef MUSHSPACE_UNDEFINE

#define MUSHSPACE_CATHELPER(a,b) a##b
#define MUSHSPACE_CAT(a,b)  MUSHSPACE_CATHELPER(a,b)
#define MUSHSPACE_NAME(x)   MUSHSPACE_CAT(MUSHSPACE_CAT(x,MUSHSPACE_DIM),\
                                          MUSHSPACE_STRUCT_SUFFIX)

#define mushspace  MUSHSPACE_NAME(mushspace)
#define mushcoords MUSHSPACE_NAME(mushcoords)

#define mush_aabb       MUSHSPACE_NAME(mush_aabb)
#define mush_bakaabb    MUSHSPACE_NAME(mush_bakaabb)
#define mush_staticaabb MUSHSPACE_NAME(mush_staticaabb)

#define mush_anamnesic_ring MUSHSPACE_NAME(mush_anamnesic_ring)

#endif
