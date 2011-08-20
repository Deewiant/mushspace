// File created: 2011-08-06 17:12:20

#ifndef MUSHSPACE_CELL_H
#define MUSHSPACE_CELL_H

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

typedef          long mushcell;
typedef unsigned long mushucell;

// For things that range from 0 to MUSHSPACE_DIM or thereabouts.
//
// This doesn't, strictly speaking, belong here, but oh well.
typedef uint8_t mushdim;

#define MUSHCELL_MIN LONG_MIN
#define MUSHCELL_MAX LONG_MAX

mushcell mushcell_max(mushcell, mushcell);
mushcell mushcell_min(mushcell, mushcell);

// Deal with signed overflow/underflow by wrapping around the way Gawd
// intended.
mushcell mushcell_add(mushcell, mushcell);
mushcell mushcell_sub(mushcell, mushcell);
mushcell mushcell_inc(mushcell);

// Don't overflow/underflow, instead clamp the result to
// MUSHCELL_MIN/MUSHCELL_MAX.
mushcell mushcell_add_clamped(mushcell, mushcell);
mushcell mushcell_sub_clamped(mushcell, mushcell);

// This doesn't really belong here but it's the best place for now.
void mushcell_space(mushcell*, size_t);

#endif
