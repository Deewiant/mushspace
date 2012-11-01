// File created: 2011-08-06 17:12:20

#ifndef MUSHSPACE_CELL_H
#define MUSHSPACE_CELL_H

#include "config/config.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#if MUSHSPACE_93
typedef uint8_t mushcell;
typedef uint8_t mushucell;

#define mushcell_max      mushcell93_max
#define mushcell_min      mushcell93_min
#define mushcell_max_into mushcell93_max_into
#define mushcell_min_into mushcell93_min_into
#define mushcell_add      mushcell93_add
#define mushcell_sub      mushcell93_sub
#define mushcell_mul      mushcell93_mul
#define mushcell_inc      mushcell93_inc
#define mushcell_dec      mushcell93_dec
#define mushcell_add_into mushcell93_add_into
#define mushcell_sub_into mushcell93_sub_into
#define mushcell_space    mushcell93_space

#define MUSHCELL_MIN  0
#define MUSHCELL_MAX  UINT8_MAX
#define MUSHUCELL_MAX MUSHCELL_MAX

#elif defined(MUSH_CELL_IS_LONG)

typedef          long mushcell;
typedef unsigned long mushucell;

#define  MUSHCELL_MIN  LONG_MIN
#define  MUSHCELL_MAX  LONG_MAX
#define MUSHUCELL_MAX ULONG_MAX

#elif defined(MUSH_CELL_IS_32_BIT)

typedef  int32_t mushcell;
typedef uint32_t mushucell;

#define  MUSHCELL_MIN  INT32_MIN
#define  MUSHCELL_MAX  INT32_MAX
#define MUSHUCELL_MAX UINT32_MAX

#elif defined(MUSH_CELL_IS_64_BIT)

typedef  int64_t mushcell;
typedef uint64_t mushucell;

#define  MUSHCELL_MIN  INT64_MIN
#define  MUSHCELL_MAX  INT64_MAX
#define MUSHUCELL_MAX UINT64_MAX

#endif

// For things that range from 0 to MUSHSPACE_DIM or thereabouts.
//
// This doesn't, strictly speaking, belong here, but oh well.
typedef uint8_t mushdim;

mushcell mushcell_max     (mushcell,  mushcell);
mushcell mushcell_min     (mushcell,  mushcell);
void     mushcell_max_into(mushcell*, mushcell);
void     mushcell_min_into(mushcell*, mushcell);

// Deal with signed overflow/underflow by wrapping around the way Gawd
// intended.
mushcell mushcell_add (mushcell,  mushcell);
mushcell mushcell_sub (mushcell,  mushcell);
mushcell mushcell_mul (mushcell,  mushcell);
mushcell mushcell_inc (mushcell);
mushcell mushcell_dec (mushcell);
void mushcell_add_into(mushcell*, mushcell);
void mushcell_sub_into(mushcell*, mushcell);

#if !MUSHSPACE_93
// Don't overflow/underflow, instead clamp the result to
// MUSHCELL_MIN/MUSHCELL_MAX.
mushcell mushcell_add_clamped(mushcell, mushcell);
mushcell mushcell_sub_clamped(mushcell, mushcell);
#endif

// This doesn't really belong here but it's the best place for now.
void mushcell_space(mushcell*, size_t);

#if !MUSHSPACE_93
// Mathy stuff used for ray intersection calculations, also somewhat
// questionably belonging here.

// Solves for x in the equation ax = b (mod 2^(sizeof(mushucell) * 8)), given a
// and b. a must be nonzero.
//
// Returns false if there was no solution.
//
// If there is a solution, returns true. A solution is stored in *x.
//
// The second pointer parameter, "gcd_lg", holds the binary logarithm of the
// number of solutions: that is, lg(gcd(a, 2^(sizeof(mushucell) * 8))). The
// solution count іtself can be constructed by raising two to that power, since
// it is guaranteed to be a power of two.
//
// Further solutions can be formed by adding 2^(sizeof(mushucell) * 8 - gcd_lg)
// to the one solution given.
bool mushucell_mod_div(mushucell a, mushucell b, mushucell* x,
                       uint_fast8_t* gcd_lg);

// gcd(2^(sizeof(mushucell)*8), n) = 2^m: this returns m, i.e. the binary
// logarithm of the gcd. Asserts if n is zero.
uint_fast8_t mushucell_gcd_lg(mushucell n);
#endif

#endif
