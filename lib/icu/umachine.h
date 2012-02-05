/*
******************************************************************************
*
*   Copyright (C) 1999-2011, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  umachine.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep13
*   created by: Markus W. Scherer
*/

#ifndef __UMACHINE_H__
#define __UMACHINE_H__

#include <stdint.h>

#if !defined(__MINGW32__) && (defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64))
#include "lib/icu/pwin32.h"
#else
#include "lib/icu/platform.h"
#endif

#define U_CAPI extern U_EXPORT

typedef uint16_t UChar;
typedef int32_t UChar32;

#endif
