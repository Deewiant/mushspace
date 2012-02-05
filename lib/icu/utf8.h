/*
*******************************************************************************
*
*   Copyright (C) 1999-2009, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  utf8.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep13
*   created by: Markus W. Scherer
*/

/**
 * \file
 * \brief C API: 8-bit Unicode handling macros
 *
 * This file defines macros to deal with 8-bit Unicode (UTF-8) code units (bytes) and strings.
 * utf8.h is included by utf.h after unicode/umachine.h
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

#ifndef __UTF8_H__
#define __UTF8_H__

/* utf.h must be included first. */
#ifndef __UTF_H__
#   include "lib/icu/utf.h"
#endif

/* internal definitions ----------------------------------------------------- */

/**
 * Count the trail bytes for a UTF-8 lead byte.
 *
 * This is internal since it is not meant to be called directly by external clients;
 * however it is called by public macros in this file and thus must remain stable.
 * @internal
 */
#define U8_COUNT_TRAIL_BYTES(leadByte) (utf8_countTrailBytes[(uint8_t)leadByte])

/**
 * Mask a UTF-8 lead byte, leave only the lower bits that form part of the code point value.
 *
 * This is internal since it is not meant to be called directly by external clients;
 * however it is called by public macros in this file and thus must remain stable.
 * @internal
 */
#define U8_MASK_LEAD_BYTE(leadByte, countTrailBytes) ((leadByte)&=(1<<(6-(countTrailBytes)))-1)

U_STABLE UChar32 U_EXPORT2
utf8_nextCharPtrSafeBody(const uint8_t **s, const uint8_t *s_end, UChar32 c);

/* single-code point definitions -------------------------------------------- */

/**
 * Is this code unit (byte) a UTF-8 lead byte?
 * @param c 8-bit code unit (byte)
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U8_IS_LEAD(c) ((uint8_t)((c)-0xc0)<0x3e)

/**
 * Is this code unit (byte) a UTF-8 trail byte?
 * @param c 8-bit code unit (byte)
 * @return TRUE or FALSE
 * @stable ICU 2.4
 */
#define U8_IS_TRAIL(c) (((c)&0xc0)==0x80)

/* definitions with forward iteration --------------------------------------- */

#define U8_NEXT_PTR(s, s_end, c) do { \
    (c)=(uint8_t)*(s)++; \
    if((c)>=0x80) { \
        uint8_t u__t1, u__t2; \
        if( /* handle U+1000..U+CFFF inline */ \
            (0xe0<(c) && (c)<=0xec) && \
            ((s)+1<(s_end)) && \
            (u__t1=(uint8_t)((s)[0]-0x80))<= 0x3f && \
            (u__t2=(uint8_t)((s)[1]-0x80))<= 0x3f \
        ) { \
            /* no need for (c&0xf) because the upper bits are truncated after <<12 in the cast to (UChar) */ \
            (c)=(UChar)(((c)<<12)|(u__t1<<6)|u__t2); \
            (s)+=2; \
        } else if( /* handle U+0080..U+07FF inline */ \
            ((c)<0xe0 && (c)>=0xc2) && \
            ((s)<(s_end)) && \
            (u__t1=(uint8_t)(*(s)-0x80))<=0x3f \
        ) { \
            (c)=(UChar)((((c)&0x1f)<<6)|u__t1); \
            ++(s); \
        } else if(U8_IS_LEAD(c)) { \
            /* function call for "complicated" and error cases */ \
            (c)=utf8_nextCharPtrSafeBody((const uint8_t**)&s, s_end, c); \
        } else { \
            (c)=U_SENTINEL; \
        } \
    } \
} while (0)

#endif
