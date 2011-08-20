// File created: 2011-08-06 17:12:20

#ifndef MUSHSPACE_CELL_H
#define MUSHSPACE_CELL_H

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#if MUSHSPACE_93
typedef uint8_t mushcell;
typedef uint8_t mushucell;

#define mushcell_max   mushcell_93_max
#define mushcell_min   mushcell_93_min
#define mushcell_add   mushcell_93_add
#define mushcell_sub   mushcell_93_sub
#define mushcell_inc   mushcell_93_inc
#define mushcell_space mushcell_93_space

#define MUSHCELL_MIN 0
#define MUSHCELL_MAX UINT8_MAX
#else
typedef          long mushcell;
typedef unsigned long mushucell;

#define MUSHCELL_MIN LONG_MIN
#define MUSHCELL_MAX LONG_MAX
#endif

// For things that range from 0 to MUSHSPACE_DIM or thereabouts.
//
// This doesn't, strictly speaking, belong here, but oh well.
typedef uint8_t mushdim;

mushcell mushcell_max(mushcell, mushcell);
mushcell mushcell_min(mushcell, mushcell);

// Deal with signed overflow/underflow by wrapping around the way Gawd
// intended.
mushcell mushcell_add(mushcell, mushcell);
mushcell mushcell_sub(mushcell, mushcell);
mushcell mushcell_inc(mushcell);

#if !MUSHSPACE_93
// Don't overflow/underflow, instead clamp the result to
// MUSHCELL_MIN/MUSHCELL_MAX.
mushcell mushcell_add_clamped(mushcell, mushcell);
mushcell mushcell_sub_clamped(mushcell, mushcell);
#endif

// This doesn't really belong here but it's the best place for now.
void mushcell_space(mushcell*, size_t);

#endif
