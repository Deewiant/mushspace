// File created: 2012-10-06 18:42:52

#ifndef TAP_H
#define TAP_H

#include <stdbool.h>
#include <stdio.h>

#include <mush/cell.h>

void tap_n(int);

void tap_ok    (const char*);
void tap_not_ok(const char*);

void tap_bool(bool, const char*, const char*);

// TAP EQual Cells with custom message Strings
void tap_eqcs  (mushcell,   mushcell,   const char*, const char*);
void tap_eqc93s(mushcell93, mushcell93, const char*, const char*);

#define tap_eqc93(a, b) tap_eqc93s((a), (b), #a " == " #b, #a " != " #b)

#if MUSHSPACE_93
#define tap_eqc tap_eqc93
#define tap_eqcs tap_eqc93s
#else
#define tap_eqc(a, b) tap_eqcs((a), (b), #a " == " #b, #a " != " #b)
#endif

#endif
