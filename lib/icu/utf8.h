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

#ifndef __UTF8_H__
#define __UTF8_H__

#ifndef __UTF_H__
#   include "lib/icu/utf.h"
#endif

U_CAPI UChar32 U_EXPORT2
utf8_nextCharPtrSafeBody(const uint8_t **s, const uint8_t *s_end, UChar32 c);

#define U8_IS_LEAD(c) ((uint8_t)((c)-0xc0)<0x3e)

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
            (c)=utf8_nextCharPtrSafeBody((const uint8_t**)&s, s_end, (UChar32)c); \
        } else { \
            (c)=U_SENTINEL; \
        } \
    } \
} while (0)

#endif
