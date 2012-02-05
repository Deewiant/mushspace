/*
*******************************************************************************
*
*   Copyright (C) 2002-2008, International Business Machines
*   Corporation and others.  All Rights Reserved.
*
*******************************************************************************
*   file name:  utf.h
*   encoding:   US-ASCII
*   tab size:   8 (not used)
*   indentation:4
*
*   created on: 2002sep21
*   created by: Markus W. Scherer
*/

/**
 * \file
 * \brief C API: Deprecated macros for Unicode string handling
 */

/**
 *
 * The macros in utf_old.h are all deprecated and their use discouraged.
 * Some of the design principles behind the set of UTF macros
 * have changed or proved impractical.
 * Almost all of the old "UTF macros" are at least renamed.
 * If you are looking for a new equivalent to an old macro, please see the
 * comment at the old one.
 *
 * utf_old.h is included by utf.h after unicode/umachine.h
 * and some common definitions, to not break old code.
 *
 * Brief summary of reasons for deprecation:
 * - Switch on UTF_SIZE (selection of UTF-8/16/32 default string processing)
 *   was impractical.
 * - Switch on UTF_SAFE etc. (selection of unsafe/safe/strict default string processing)
 *   was of little use and impractical.
 * - Whole classes of macros became obsolete outside of the UTF_SIZE/UTF_SAFE
 *   selection framework: UTF32_ macros (all trivial)
 *   and UTF_ default and intermediate macros (all aliases).
 * - The selection framework also caused many macro aliases.
 * - Change in Unicode standard: "irregular" sequences (3.0) became illegal (3.2).
 * - Change of language in Unicode standard:
 *   Growing distinction between internal x-bit Unicode strings and external UTF-x
 *   forms, with the former more lenient.
 *   Suggests renaming of UTF16_ macros to U16_.
 * - The prefix "UTF_" without a width number confused some users.
 * - "Safe" append macros needed the addition of an error indicator output.
 * - "Safe" UTF-8 macros used legitimate (if rarely used) code point values
 *   to indicate error conditions.
 * - The use of the "_CHAR" infix for code point operations confused some users.
 *
 * More details:
 *
 * Until ICU 2.2, utf.h theoretically allowed to choose among UTF-8/16/32
 * for string processing, and among unsafe/safe/strict default macros for that.
 *
 * It proved nearly impossible to write non-trivial, high-performance code
 * that is UTF-generic.
 * Unsafe default macros would be dangerous for default string processing,
 * and the main reason for the "strict" versions disappeared:
 * Between Unicode 3.0 and 3.2 all "irregular" UTF-8 sequences became illegal.
 * The only other conditions that "strict" checked for were non-characters,
 * which are valid during processing. Only during text input/output should they
 * be checked, and at that time other well-formedness checks may be
 * necessary or useful as well.
 * This can still be done by using U16_NEXT and U_IS_UNICODE_NONCHAR
 * or U_IS_UNICODE_CHAR.
 *
 * The old UTF8_..._SAFE macros also used some normal Unicode code points
 * to indicate malformed sequences.
 * The new UTF8_ macros without suffix use negative values instead.
 *
 * The entire contents of utf32.h was moved here without replacement
 * because all those macros were trivial and
 * were meaningful only in the framework of choosing the UTF size.
 *
 * See Jitterbug 2150 and its discussion on the ICU mailing list
 * in September 2002.
 *
 * <hr>
 *
 * <em>Obsolete part</em> of pre-ICU 2.4 utf.h file documentation:
 *
 * <p>The original concept for these files was for ICU to allow
 * in principle to set which UTF (UTF-8/16/32) is used internally
 * by defining UTF_SIZE to either 8, 16, or 32. utf.h would then define the UChar type
 * accordingly. UTF-16 was the default.</p>
 *
 * <p>This concept has been abandoned.
 * A lot of the ICU source code assumes UChar strings are in UTF-16.
 * This is especially true for low-level code like
 * conversion, normalization, and collation.
 * The utf.h header enforces the default of UTF-16.
 * The UTF-8 and UTF-32 macros remain for now for completeness and backward compatibility.</p>
 *
 * <p>Accordingly, utf.h defines UChar to be an unsigned 16-bit integer. If this matches wchar_t, then
 * UChar is defined to be exactly wchar_t, otherwise uint16_t.</p>
 *
 * <p>UChar32 is defined to be a signed 32-bit integer (int32_t), large enough for a 21-bit
 * Unicode code point (Unicode scalar value, 0..0x10ffff).
 * Before ICU 2.4, the definition of UChar32 was similarly platform-dependent as
 * the definition of UChar. For details see the documentation for UChar32 itself.</p>
 *
 * <p>utf.h also defines a number of C macros for handling single Unicode code points and
 * for using UTF Unicode strings. It includes utf8.h, utf16.h, and utf32.h for the actual
 * implementations of those macros and then aliases one set of them (for UTF-16) for general use.
 * The UTF-specific macros have the UTF size in the macro name prefixes (UTF16_...), while
 * the general alias macros always begin with UTF_...</p>
 *
 * <p>Many string operations can be done with or without error checking.
 * Where such a distinction is useful, there are two versions of the macros, "unsafe" and "safe"
 * ones with ..._UNSAFE and ..._SAFE suffixes. The unsafe macros are fast but may cause
 * program failures if the strings are not well-formed. The safe macros have an additional, boolean
 * parameter "strict". If strict is FALSE, then only illegal sequences are detected.
 * Otherwise, irregular sequences and non-characters are detected as well (like single surrogates).
 * Safe macros return special error code points for illegal/irregular sequences:
 * Typically, U+ffff, or values that would result in a code unit sequence of the same length
 * as the erroneous input sequence.<br>
 * Note that _UNSAFE macros have fewer parameters: They do not have the strictness parameter, and
 * they do not have start/length parameters for boundary checking.</p>
 *
 * <p>Here, the macros are aliased in two steps:
 * In the first step, the UTF-specific macros with UTF16_ prefix and _UNSAFE and _SAFE suffixes are
 * aliased according to the UTF_SIZE to macros with UTF_ prefix and the same suffixes and signatures.
 * Then, in a second step, the default, general alias macros are set to use either the unsafe or
 * the safe/not strict (default) or the safe/strict macro;
 * these general macros do not have a strictness parameter.</p>
 *
 * <p>It is possible to change the default choice for the general alias macros to be unsafe, safe/not strict or safe/strict.
 * The default is safe/not strict. It is not recommended to select the unsafe macros as the basis for
 * Unicode string handling in ICU! To select this, define UTF_SAFE, UTF_STRICT, or UTF_UNSAFE.</p>
 *
 * <p>For general use, one should use the default, general macros with UTF_ prefix and no _SAFE/_UNSAFE suffix.
 * Only in some cases it may be necessary to control the choice of macro directly and use a less generic alias.
 * For example, if it can be assumed that a string is well-formed and the index will stay within the bounds,
 * then the _UNSAFE version may be used.
 * If a UTF-8 string is to be processed, then the macros with UTF8_ prefixes need to be used.</p>
 *
 * <hr>
 *
 * @deprecated ICU 2.4. Use the macros in utf.h, utf16.h, utf8.h instead.
 */

#ifndef __UTF_OLD_H__
#define __UTF_OLD_H__

/* utf.h must be included first. */
#ifndef __UTF_H__
#   include "unicod/utf.h"
#endif

/* Formerly utf.h, part 1 --------------------------------------------------- */

/**
 * UTF8_ERROR_VALUE_1 and UTF8_ERROR_VALUE_2 are special error values for UTF-8,
 * which need 1 or 2 bytes in UTF-8:
 * \code
 * U+0015 = NAK = Negative Acknowledge, C0 control character
 * U+009f = highest C1 control character
 * \endcode
 *
 * These are used by UTF8_..._SAFE macros so that they can return an error value
 * that needs the same number of code units (bytes) as were seen by
 * a macro. They should be tested with UTF_IS_ERROR() or UTF_IS_VALID().
 *
 * @deprecated ICU 2.4. Obsolete, see utf_old.h.
 */
#define UTF8_ERROR_VALUE_1 0x15

/**
 * See documentation on UTF8_ERROR_VALUE_1 for details.
 *
 * @deprecated ICU 2.4. Obsolete, see utf_old.h.
 */
#define UTF8_ERROR_VALUE_2 0x9f

/**
 * Error value for all UTFs. This code point value will be set by macros with error
 * checking if an error is detected.
 *
 * @deprecated ICU 2.4. Obsolete, see utf_old.h.
 */
#define UTF_ERROR_VALUE 0xffff

/**
 * Is this code unit or code point a surrogate (U+d800..U+dfff)?
 * @deprecated ICU 2.4. Renamed to U_IS_SURROGATE and U16_IS_SURROGATE, see utf_old.h.
 */
#define UTF_IS_SURROGATE(uchar) (((uchar)&0xfffff800)==0xd800)

/**
 * Is a given 32-bit code point a Unicode noncharacter?
 *
 * @deprecated ICU 2.4. Renamed to U_IS_UNICODE_NONCHAR, see utf_old.h.
 */
#define UTF_IS_UNICODE_NONCHAR(c) \
    ((c)>=0xfdd0 && \
     ((uint32_t)(c)<=0xfdef || ((c)&0xfffe)==0xfffe) && \
     (uint32_t)(c)<=0x10ffff)

/* Formerly utf8.h ---------------------------------------------------------- */

/**
 * Count the trail bytes for a UTF-8 lead byte.
 * @deprecated ICU 2.4. Renamed to U8_COUNT_TRAIL_BYTES, see utf_old.h.
 */
#define UTF8_COUNT_TRAIL_BYTES(leadByte) (utf8_countTrailBytes[(uint8_t)leadByte])

/**
 * Mask a UTF-8 lead byte, leave only the lower bits that form part of the code point value.
 * @deprecated ICU 2.4. Renamed to U8_MASK_LEAD_BYTE, see utf_old.h.
 */
#define UTF8_MASK_LEAD_BYTE(leadByte, countTrailBytes) ((leadByte)&=(1<<(6-(countTrailBytes)))-1)

/** Is this this code unit a trailing code unit (byte) of a code point? @deprecated ICU 2.4. Renamed to U8_IS_TRAIL, see utf_old.h. */
#define UTF8_IS_TRAIL(uchar) (((uchar)&0xc0)==0x80)

/** @deprecated ICU 2.4. Renamed to U8_APPEND_UNSAFE, see utf_old.h. */
#define UTF8_APPEND_CHAR_UNSAFE(s, i, c) { \
    if((uint32_t)(c)<=0x7f) { \
        (s)[(i)++]=(uint8_t)(c); \
    } else { \
        if((uint32_t)(c)<=0x7ff) { \
            (s)[(i)++]=(uint8_t)(((c)>>6)|0xc0); \
        } else { \
            if((uint32_t)(c)<=0xffff) { \
                (s)[(i)++]=(uint8_t)(((c)>>12)|0xe0); \
            } else { \
                (s)[(i)++]=(uint8_t)(((c)>>18)|0xf0); \
                (s)[(i)++]=(uint8_t)((((c)>>12)&0x3f)|0x80); \
            } \
            (s)[(i)++]=(uint8_t)((((c)>>6)&0x3f)|0x80); \
        } \
        (s)[(i)++]=(uint8_t)(((c)&0x3f)|0x80); \
    } \
}

#endif
