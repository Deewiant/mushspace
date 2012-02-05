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
*
*   This file defines basic types and constants for utf.h to be
*   platform-independent. umachine.h and utf.h are included into
*   utypes.h to provide all the general definitions for ICU.
*   All of these definitions used to be in utypes.h before
*   the UTF-handling macros made this unmaintainable.
*/

#ifndef __UMACHINE_H__
#define __UMACHINE_H__


/**
 * \file
 * \brief Basic types and constants for UTF
 *
 * <h2> Basic types and constants for UTF </h2>
 *   This file defines basic types and constants for utf.h to be
 *   platform-independent. umachine.h and utf.h are included into
 *   utypes.h to provide all the general definitions for ICU.
 *   All of these definitions used to be in utypes.h before
 *   the UTF-handling macros made this unmaintainable.
 *
 */
/*==========================================================================*/
/* Include platform-dependent definitions                                   */
/* which are contained in the platform-specific file platform.h             */
/*==========================================================================*/

#include <stdint.h>

#if !defined(__MINGW32__) && (defined(WIN32) || defined(_WIN32) || defined(WIN64) || defined(_WIN64))
#include "unicod/pwin32.h"
#else
#include "unicod/platform.h"
#endif

/*
 * ANSI C headers:
 * stddef.h defines wchar_t
 */
#include <stddef.h>

/*==========================================================================*/
/* For C wrappers, we use the symbol U_STABLE.                                */
/* This works properly if the includer is C or C++.                         */
/* Functions are declared   U_STABLE return-type U_EXPORT2 function-name()... */
/*==========================================================================*/

/**
 * \def U_CFUNC
 * This is used in a declaration of a library private ICU C function.
 * @stable ICU 2.4
 */

#define U_CFUNC extern

/** This is used to declare a function as a public ICU C API @stable ICU 2.0*/
#define U_CAPI U_CFUNC U_EXPORT
/** This is used to declare a function as a stable public ICU C API*/
#define U_STABLE U_CAPI

/*==========================================================================*/
/* Boolean data type                                                        */
/*==========================================================================*/

/** The ICU boolean type @stable ICU 2.0 */
typedef int8_t UBool;

#ifndef TRUE
/** The TRUE value of a UBool @stable ICU 2.0 */
#   define TRUE  1
#endif
#ifndef FALSE
/** The FALSE value of a UBool @stable ICU 2.0 */
#   define FALSE 0
#endif


/*==========================================================================*/
/* Unicode data types                                                       */
/*==========================================================================*/

/* wchar_t-related definitions -------------------------------------------- */

/**
 * \def U_SIZEOF_WCHAR_T
 * U_SIZEOF_WCHAR_T==sizeof(wchar_t) (0 means it is not defined or autoconf could not set it)
 *
 * @stable ICU 2.0
 */
#if U_SIZEOF_WCHAR_T==0
#   undef U_SIZEOF_WCHAR_T
#   define U_SIZEOF_WCHAR_T 4
#endif

/* UChar and UChar32 definitions -------------------------------------------- */

/**
 * \var UChar
 * Define UChar to be wchar_t if that is 16 bits wide; always assumed to be unsigned.
 * If wchar_t is not 16 bits wide, then define UChar to be uint16_t or char16_t because GCC >=4.4
 * can handle UTF16 string literals.
 * This makes the definition of UChar platform-dependent
 * but allows direct string type compatibility with platforms with
 * 16-bit wchar_t types.
 *
 * @draft ICU 4.4
 */

/* Define UChar to be compatible with wchar_t if possible. */
#if U_SIZEOF_WCHAR_T==2
    typedef wchar_t UChar;
#elif U_GNUC_UTF16_STRING
#if defined _GCC_
    typedef __CHAR16_TYPE__ char16_t;
#endif
    typedef char16_t UChar;
#else
    typedef uint16_t UChar;
#endif

/**
 * Define UChar32 as a type for single Unicode code points.
 * UChar32 is a signed 32-bit integer (same as int32_t).
 *
 * The Unicode code point range is 0..0x10ffff.
 * All other values (negative or >=0x110000) are illegal as Unicode code points.
 * They may be used as sentinel values to indicate "done", "error"
 * or similar non-code point conditions.
 *
 * Before ICU 2.4 (Jitterbug 2146), UChar32 was defined
 * to be wchar_t if that is 32 bits wide (wchar_t may be signed or unsigned)
 * or else to be uint32_t.
 * That is, the definition of UChar32 was platform-dependent.
 *
 * @see U_SENTINEL
 * @stable ICU 2.4
 */
typedef int32_t UChar32;

#endif
