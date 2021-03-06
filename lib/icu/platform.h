/*
******************************************************************************
*
*   Copyright (C) 1997-2011, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*
* Note: autoconf creates platform.h from platform.h.in at configure time.
*
******************************************************************************
*
*  FILE NAME : platform.h
*
*   Date        Name        Description
*   05/13/98    nos         Creation (content moved here from ptypes.h).
*   03/02/99    stephen     Added AS400 support.
*   03/30/99    stephen     Added Linux support.
*   04/13/99    stephen     Reworked for autoconf.
******************************************************************************
*/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifdef U_STATIC_IMPLEMENTATION
#define U_EXPORT
#elif defined(__GNUC__)
#define U_EXPORT __attribute__((visibility("default")))
#elif (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x550) \
   || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x550)
#define U_EXPORT __global
#else
#define U_EXPORT
#endif

#define U_EXPORT2

#endif
