// File created: 2012-10-06 18:42:52

#ifndef TAP_H
#define TAP_H

#include <stdbool.h>
#include <stdio.h>

#include <mush/cell.h>

void tap_n(int);

void tap_ok    (const char*);
void tap_not_ok(const char*);
void tap_skip  (const char*);

void tap_skip_remaining(const char*);

void tap_bool(bool, const char*, const char*);

// TAP EQual Cells with custom message Strings
void tap_eqcs  (mushcell,   mushcell,   const char*, const char*);
void tap_eqc93s(mushcell93, mushcell93, const char*, const char*);

// TAP EQual Cell Vectors with custom message Strings
void tap_eqcvs(const mushcell*, const mushcell*, uint8_t,
               const char*, const char*);
void tap_eqc93vs(const mushcell93*, const mushcell93*, uint8_t,
                 const char*, const char*);

// Less than or EQual
void tap_leqcvs(const mushcell*, const mushcell*, uint8_t,
                const char*, const char*);
void tap_leqc93vs(const mushcell93*, const mushcell93*, uint8_t,
                  const char*, const char*);

// Greater than or EQual
void tap_geqcvs(const mushcell*, const mushcell*, uint8_t,
                const char*, const char*);
void tap_geqc93vs(const mushcell93*, const mushcell93*, uint8_t,
                  const char*, const char*);

#define tap_eqc93(a, b) tap_eqc93s((a), (b), #a " == " #b, #a " != " #b)

#if defined(MUSHSPACE_93) && MUSHSPACE_93
   #define tap_eqc tap_eqc93
   #define tap_eqcs tap_eqc93s

   // TAP EQual COordinates with custom message Strings
   #define tap_eqcos(a, b, so, sn) \
      tap_eqc93vs((a).v, (b).v, MUSHSPACE_DIM, (so), (sn))
   #define tap_leqcos(a, b, so, sn) \
      tap_leqc93vs((a).v, (b).v, MUSHSPACE_DIM, (so), (sn))
   #define tap_geqcos(a, b, so, sn) \
      tap_leqc93vs((a).v, (b).v, MUSHSPACE_DIM, (so), (sn))
#else
   #define tap_eqc(a, b) tap_eqcs((a), (b), #a " == " #b, #a " != " #b)

   #define tap_eqcos(a, b, so, sn) \
      tap_eqcvs((a).v, (b).v, MUSHSPACE_DIM, (so), (sn))
   #define tap_leqcos(a, b, so, sn) \
      tap_leqcvs((a).v, (b).v, MUSHSPACE_DIM, (so), (sn))
   #define tap_geqcos(a, b, so, sn) \
      tap_geqcvs((a).v, (b).v, MUSHSPACE_DIM, (so), (sn))
#endif

#define tap_eqco(a, b) tap_eqcos(a, b, #a " == " #b, #a " != " #b)

#endif
