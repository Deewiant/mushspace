// File created: 2011-08-06 17:12:20

#ifndef MUSHSPACE_CELL_H
#define MUSHSPACE_CELL_H

#include <limits.h>

typedef          long mushcell;
typedef unsigned long mushucell;

#define MUSHCELL_MIN LONG_MIN
#define MUSHCELL_MAX LONG_MAX

mushcell mushcell_max(mushcell, mushcell);
mushcell mushcell_min(mushcell, mushcell);

#endif
