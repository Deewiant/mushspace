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

/**
 * \file
 * \brief Basic types for the platform
 */

/*===========================================================================*/
/** @{ Symbol import-export control                                              */
/*===========================================================================*/

#ifdef U_STATIC_IMPLEMENTATION
#define U_EXPORT
#elif defined(__GNUC__)
#define U_EXPORT __attribute__((visibility("default")))
#elif (defined(__SUNPRO_CC) && __SUNPRO_CC >= 0x550) \
   || (defined(__SUNPRO_C) && __SUNPRO_C >= 0x550)
#define U_EXPORT __global
/*#elif defined(__HP_aCC) || defined(__HP_cc)
#define U_EXPORT __declspec(dllexport)*/
#else
#define U_EXPORT
#endif

/* U_CALLCONV is releated to U_EXPORT2 */
#define U_EXPORT2

/* cygwin needs to export/import data */
#if defined(U_CYGWIN) && !defined(__GNUC__)
#define U_IMPORT __declspec(dllimport)
#else
#define U_IMPORT
#endif

/* @} */

#endif
