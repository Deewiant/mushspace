// File created: 2012-11-03 21:02:37

#include "boxen.98.h"

// No #elses so that compile errors happen if more than one is defined.
#ifdef BOXEN_RTREE
#include "boxen/r.c"
#endif
#ifdef BOXEN_ARRAY
#include "boxen/array.c"
#endif
