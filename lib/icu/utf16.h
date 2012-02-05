/*
*******************************************************************************
*
*   Copyright (C) 1999-2010, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  utf16.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep09
*   created by: Markus W. Scherer
*/

/**
 * \file
 * \brief C API: 16-bit Unicode handling macros
 *
 * This file defines macros to deal with 16-bit Unicode (UTF-16) code units and strings.
 * utf16.h is included by utf.h after unicode/umachine.h
 * and some common definitions.
 *
 * For more information see utf.h and the ICU User Guide Strings chapter
 * (http://icu-project.org/userguide/strings.html).
 *
 * <em>Usage:</em>
 * ICU coding guidelines for if() statements should be followed when using these macros.
 * Compound statements (curly braces {}) must be used  for if-else-while...
 * bodies and all macro statements should be terminated with semicolon.
 */

#ifndef __UTF16_H__
#define __UTF16_H__

/* utf.h must be included first. */
#ifndef __UTF_H__
#   include "lib/icu/utf.h"
#endif

/* single-code point definitions -------------------------------------------- */

/**
 * Is this code unit a lead surrogate (U+d800..U+dbff)?
 * @param c 16-bit code unit
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U16_IS_LEAD(c) (((c)&0xfffffc00)==0xd800)

/**
 * Is this code unit a trail surrogate (U+dc00..U+dfff)?
 * @param c 16-bit code unit
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U16_IS_TRAIL(c) (((c)&0xfffffc00)==0xdc00)

/**
 * Helper constant for U16_GET_SUPPLEMENTARY.
 * @internal
 */
#define U16_SURROGATE_OFFSET ((0xd800<<10UL)+0xdc00-0x10000)

/**
 * Get a supplementary code point value (U+10000..U+10ffff)
 * from its lead and trail surrogates.
 * The result is undefined if the input values are not
 * lead and trail surrogates.
 *
 * @param lead lead surrogate (U+d800..U+dbff)
 * @param trail trail surrogate (U+dc00..U+dfff)
 * @return supplementary code point (U+10000..U+10ffff)
 * @stable ICU 2.4
 */
#define U16_GET_SUPPLEMENTARY(lead, trail) \
    (((UChar32)(lead)<<10UL)+(UChar32)(trail)-U16_SURROGATE_OFFSET)


/* definitions with forward iteration --------------------------------------- */

// Different from U16_NEXT in that it returns U_SENTINEL for lone surrogates.
#define U16_NEXT_PTR(s, s_end, c) do { \
    (c)=*(s)++; \
    if(U16_IS_LEAD(c)) { \
        uint16_t u__c2; \
        if((s)<(s_end) && U16_IS_TRAIL(u__c2=*(s))) { \
            ++(s); \
            (c)=U16_GET_SUPPLEMENTARY((c),u__c2); \
        } else { \
            (c) = U_SENTINEL; \
        } \
    } else if (U16_IS_TRAIL(c)) { \
        (c) = U_SENTINEL; \
    } \
} while (0)

#endif
