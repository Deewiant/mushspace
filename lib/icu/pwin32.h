/*
 ******************************************************************************
 *
 *   Copyright (C) 1997-2011, International Business Machines
 *   Corporation and others.  All Rights Reserved.
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

 /**
  * \file
  * \brief Configuration constants for the Windows platform
  */

/*===========================================================================*/
/** @{ Symbol import-export control                                              */
/*===========================================================================*/

#ifdef U_STATIC_IMPLEMENTATION
#define U_EXPORT
#else
#define U_EXPORT __declspec(dllexport)
#endif
#define U_EXPORT2 __cdecl
/** @} */
