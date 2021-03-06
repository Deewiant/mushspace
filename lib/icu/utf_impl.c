/*
******************************************************************************
*
*   Copyright (C) 1999-2006, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
******************************************************************************
*   file name:  utf_impl.c
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 1999sep13
*   created by: Markus W. Scherer
*/

#include "lib/icu/umachine.h"
#include "lib/icu/utf.h"

#define U_IS_SURROGATE(c) (((c)&0xfffff800)==0xd800)

#define U8_COUNT_TRAIL_BYTES(leadByte) (utf8_countTrailBytes[(uint8_t)leadByte])

#define U8_MASK_LEAD_BYTE(leadByte, countTrailBytes) ((leadByte)&=(1<<(6-(countTrailBytes)))-1)

#define U8_IS_TRAIL(c) (((c)&0xc0)==0x80)

/* In Unicode, all UTF-8 byte sequences with more than 4 bytes are illegal;
 * lead bytes above 0xf4 are illegal.
 * We keep them in this table for skipping long ISO 10646-UTF-8 sequences.
 */
static const uint8_t
utf8_countTrailBytes[256]={
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,

    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3,
    3, 3, 3,    /* illegal in Unicode */
    4, 4, 4, 4, /* illegal in Unicode */
    5, 5,       /* illegal in Unicode */
    0, 0        /* illegal bytes 0xfe and 0xff */
};

static const UChar32
utf8_minLegal[4]={ 0, 0x80, 0x800, 0x10000 };

U_CAPI UChar32 U_EXPORT2
utf8_nextCharPtrSafeBody(const uint8_t **ps, const uint8_t *s_end, UChar32 c) {
    const uint8_t *s=*ps;
    uint8_t count=U8_COUNT_TRAIL_BYTES(c);
    if((s)+count<=(s_end)) {
        uint8_t trail, illegal=0;

        U8_MASK_LEAD_BYTE((c), count);
        /* count==0 for illegally leading trail bytes and the illegal bytes 0xfe and 0xff */
        switch(count) {
        /* each branch falls through to the next one */
        case 5:
        case 4:
            /* count>=4 is always illegal: no more than 3 trail bytes in Unicode's UTF-8 */
            illegal=1;
            break;
        case 3:
            trail=*s++;
            (c)=((c)<<6)|(trail&0x3f);
            if(c<0x110) {
                illegal|=(trail&0xc0)^0x80;
            } else {
                /* code point>0x10ffff, outside Unicode */
                illegal=1;
                break;
            }
        case 2:
            trail=*s++;
            (c)=((c)<<6)|(trail&0x3f);
            illegal|=(trail&0xc0)^0x80;
        case 1:
            trail=*s++;
            (c)=((c)<<6)|(trail&0x3f);
            illegal|=(trail&0xc0)^0x80;
            break;
        case 0:
            return U_SENTINEL;
        /* no default branch to optimize switch()  - all values are covered */
        }

        /* All the error handling should return a value that needs count bytes
         * so that U8_GET() works right.
         *
         * Starting with Unicode 3.0.1, non-shortest forms are illegal.
         * Starting with Unicode 3.2, surrogate code points must not be
         * encoded in UTF-8, and there are no irregular sequences any more.
         */

        /* correct sequence - all trail bytes have (b7..b6)==(10)? */
        /* illegal is also set if count>=4 */
        if(illegal || (c)<utf8_minLegal[count] || (U_IS_SURROGATE(c))) {
            /* don't go beyond this sequence */
            s=*ps;
            while(count>0 && U8_IS_TRAIL(*s)) {
                ++(s);
                --count;
            }
            c=U_SENTINEL;
        }
    } else /* too few bytes left */ {
        /* don't just set (s)=(s_end) in case there is an illegal sequence */
        while((s)<(s_end) && U8_IS_TRAIL(*s)) {
            ++(s);
        }
        c=U_SENTINEL;
    }
    *ps=s;
    return c;
}
