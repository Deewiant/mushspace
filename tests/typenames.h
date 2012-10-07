// File created: 2012-10-07 12:11:22

#ifndef TYPENAMES_H
#define TYPENAMES_H

#define CATHELPER(a,b) a##b
#define CAT(a,b) CATHELPER(a,b)

#if MUSHSPACE_93
#define NAME(x) CAT(x,93)
#else
#define NAME(x) CAT(x,MUSHSPACE_DIM)
#endif

#endif
