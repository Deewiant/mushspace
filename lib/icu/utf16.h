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

#ifndef __UTF16_H__
#define __UTF16_H__

#ifndef __UTF_H__
#   include "lib/icu/utf.h"
#endif

#define U16_IS_LEAD(c) (((c)&0xfffffc00)==0xd800)
#define U16_IS_TRAIL(c) (((c)&0xfffffc00)==0xdc00)

#define U16_SURROGATE_OFFSET ((0xd800<<10UL)+0xdc00-0x10000)

#define U16_GET_SUPPLEMENTARY(lead, trail) \
    (((UChar32)(lead)<<10UL)+(UChar32)(trail)-U16_SURROGATE_OFFSET)

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
