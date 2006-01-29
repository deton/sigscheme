/*===========================================================================
 *  FileName : string.c
 *  About    : R5RS strings
 *
 *  Copyright (C) 2005-2006 Kazuki Ohta <mover AT hct.zaq.ne.jp>
 *
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of authors nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 *  IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 *  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 *  PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 *  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================*/

#include "config.h"
#include "config-nonstd-string.h"

/*=======================================
  System Include
=======================================*/
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/*=======================================
  Local Include
=======================================*/
#include "sigscheme.h"
#include "sigschemeinternal.h"

/*=======================================
  File Local Struct Declarations
=======================================*/

/*=======================================
  File Local Macro Declarations
=======================================*/
#define STRING_CMP(str1, str2)                                               \
    (string_cmp(SCM_MANGLE(name), (str1), (str2), scm_false))
#define STRING_CI_CMP(str1, str2)                                            \
    (string_cmp(SCM_MANGLE(name), (str1), (str2), scm_true))

/*
 * SigScheme's case-insensitive comparison conforms to the foldcase'ed
 * comparison described in SRFI-75 and SRFI-13, although R5RS does not specify
 * comparison between alphabetic and non-alphabetic char.
 *
 * This specification is needed to produce natural result on sort functions
 * with these case-insensitive predicates as comparator.
 *
 *   (a-sort '(#\a #\c #\B #\D #\1 #\[ #\$ #\_) char-ci<?)
 *     => (#\$ #\1 #\a #\B #\c #\D #\[ #\_)  ;; the "natural result"
 *
 *     => (#\$ #\1 #\B #\D #\[ #\_ #\a #\c)  ;; "unnatural result"
 *
 * See also:
 *
 *   - Description around 'char-foldcase' in SRFI-75
 *   - "Case mapping and case-folding" and "Comparison" section of SRFI-13
 */
/* FIXME: support SRFI-75 */
#define ICHAR_DOWNCASE(c) ((isascii((int)(c))) ? tolower((int)(c)) : (c))
#define ICHAR_UPCASE(c)   ((isascii((int)(c))) ? toupper((int)(c)) : (c))
/* foldcase for case-insensitive character comparison is done by downcase as
 * described in SRFI-75. Although SRFI-13 expects (char-downcase (char-upcase
 * c)), this implementation is sufficient for ASCII range. */
#define ICHAR_FOLDCASE(c) (ICHAR_DOWNCASE(c))

/*=======================================
  Variable Declarations
=======================================*/

/*=======================================
  File Local Function Declarations
=======================================*/
#if (!HAVE_STRCASECMP && !SCM_USE_MULTIBYTE_CHAR)
static int strcasecmp(const char *s1, const char *s2);
#endif
static int string_cmp(const char *funcname,
                      ScmObj str1, ScmObj str2, scm_bool case_insensitive);

/*=======================================
  Function Implementations
=======================================*/

/*===========================================================================
  R5RS : 6.3 Other data types : 6.3.4 Characters
===========================================================================*/
ScmObj
scm_p_charp(ScmObj obj)
{
    DECLARE_FUNCTION("char?", procedure_fixed_1);

    return MAKE_BOOL(CHARP(obj));
}

ScmObj
scm_p_char_equalp(ScmObj ch1, ScmObj ch2)
{
    DECLARE_FUNCTION("char=?", procedure_fixed_2);

    ENSURE_CHAR(ch1);
    ENSURE_CHAR(ch2);

#if SCM_HAS_IMMEDIATE_CHAR_ONLY
    return MAKE_BOOL(EQ(ch1, ch2));
#else
    return MAKE_BOOL(SCM_CHAR_VALUE(ch1) == SCM_CHAR_VALUE(ch2));
#endif
}

#define CHAR_CMP_BODY(op, ch1, ch2)                                          \
    do {                                                                     \
        ENSURE_CHAR(ch1);                                                    \
        ENSURE_CHAR(ch2);                                                    \
                                                                             \
        return MAKE_BOOL(SCM_CHAR_VALUE(ch1) op SCM_CHAR_VALUE(ch2));        \
    } while (/* CONSTCOND */ 0)

ScmObj
scm_p_char_lessp(ScmObj ch1, ScmObj ch2)
{
    DECLARE_FUNCTION("char<?", procedure_fixed_2);

    CHAR_CMP_BODY(<, ch1, ch2);
}

ScmObj
scm_p_char_greaterp(ScmObj ch1, ScmObj ch2)
{
    DECLARE_FUNCTION("char>?", procedure_fixed_2);

    CHAR_CMP_BODY(>, ch1, ch2);
}

ScmObj
scm_p_char_less_equalp(ScmObj ch1, ScmObj ch2)
{
    DECLARE_FUNCTION("char<=?", procedure_fixed_2);

    CHAR_CMP_BODY(<=, ch1, ch2);
}

ScmObj
scm_p_char_greater_equalp(ScmObj ch1, ScmObj ch2)
{
    DECLARE_FUNCTION("char>=?", procedure_fixed_2);

    CHAR_CMP_BODY(>=, ch1, ch2);
}

#define CHAR_CI_CMP_BODY(op, ch1, ch2)                                       \
    do {                                                                     \
        scm_ichar_t val1, val2;                                              \
                                                                             \
        ENSURE_CHAR(ch1);                                                    \
        ENSURE_CHAR(ch2);                                                    \
                                                                             \
        val1 = ICHAR_FOLDCASE(SCM_CHAR_VALUE(ch1));                          \
        val2 = ICHAR_FOLDCASE(SCM_CHAR_VALUE(ch2));                          \
                                                                             \
        return MAKE_BOOL(val1 op val2);                                      \
    } while (/* CONSTCOND */ 0)

ScmObj
scm_p_char_ci_equalp(ScmObj ch1, ScmObj ch2)
{
    DECLARE_FUNCTION("char-ci=?", procedure_fixed_2);

    CHAR_CI_CMP_BODY(==, ch1, ch2);
}

ScmObj
scm_p_char_ci_lessp(ScmObj ch1, ScmObj ch2)
{
    DECLARE_FUNCTION("char-ci<?", procedure_fixed_2);

    CHAR_CI_CMP_BODY(<, ch1, ch2);
}

ScmObj
scm_p_char_ci_greaterp(ScmObj ch1, ScmObj ch2)
{
    DECLARE_FUNCTION("char-ci>?", procedure_fixed_2);

    CHAR_CI_CMP_BODY(>, ch1, ch2);
}

ScmObj
scm_p_char_ci_less_equalp(ScmObj ch1, ScmObj ch2)
{
    DECLARE_FUNCTION("char-ci<=?", procedure_fixed_2);

    CHAR_CI_CMP_BODY(<=, ch1, ch2);
}

ScmObj
scm_p_char_ci_greater_equalp(ScmObj ch1, ScmObj ch2)
{
    DECLARE_FUNCTION("char-ci>=?", procedure_fixed_2);

    CHAR_CI_CMP_BODY(>=, ch1, ch2);
}

#undef CHAR_CMP_BODY
#undef CHAR_CI_CMP_BODY

ScmObj
scm_p_char_alphabeticp(ScmObj ch)
{
    scm_ichar_t val;
    DECLARE_FUNCTION("char-alphabetic?", procedure_fixed_1);

    ENSURE_CHAR(ch);

    val = SCM_CHAR_VALUE(ch);

    return MAKE_BOOL(isascii(val) && isalpha(val));
}

ScmObj
scm_p_char_numericp(ScmObj ch)
{
    scm_ichar_t val;
    DECLARE_FUNCTION("char-numeric?", procedure_fixed_1);

    ENSURE_CHAR(ch);

    val = SCM_CHAR_VALUE(ch);

    return MAKE_BOOL(isascii(val) && isdigit(val));
}

ScmObj
scm_p_char_whitespacep(ScmObj ch)
{
    scm_ichar_t val;
    DECLARE_FUNCTION("char-whitespace?", procedure_fixed_1);

    ENSURE_CHAR(ch);

    val = SCM_CHAR_VALUE(ch);

    return MAKE_BOOL(isascii(val) && isspace(val));
}

ScmObj
scm_p_char_upper_casep(ScmObj ch)
{
    scm_ichar_t val;
    DECLARE_FUNCTION("char-upper-case?", procedure_fixed_1);

    ENSURE_CHAR(ch);

    val = SCM_CHAR_VALUE(ch);

    return MAKE_BOOL(isascii(val) && isupper(val));
}

ScmObj
scm_p_char_lower_casep(ScmObj ch)
{
    scm_ichar_t val;
    DECLARE_FUNCTION("char-lower-case?", procedure_fixed_1);

    ENSURE_CHAR(ch);

    val = SCM_CHAR_VALUE(ch);

    return MAKE_BOOL(isascii(val) && islower(val));
}

ScmObj
scm_p_char2integer(ScmObj ch)
{
    DECLARE_FUNCTION("char->integer", procedure_fixed_1);

    ENSURE_CHAR(ch);

    return MAKE_INT(SCM_CHAR_VALUE(ch));
}

ScmObj
scm_p_integer2char(ScmObj n)
{
    scm_int_t val;
    DECLARE_FUNCTION("integer->char", procedure_fixed_1);

    ENSURE_INT(n);

    val = SCM_INT_VALUE(n);
#if SCM_USE_MULTIBYTE_CHAR
    if (!SCM_CHARCODEC_CHAR_LEN(scm_current_char_codec, val))
#else
    if (!isascii(val))
#endif
        ERR_OBJ("invalid char value", n);

    return MAKE_CHAR(val);
}

ScmObj
scm_p_char_upcase(ScmObj ch)
{
    scm_ichar_t val;
    DECLARE_FUNCTION("char-upcase", procedure_fixed_1);

    ENSURE_CHAR(ch);

    val = SCM_CHAR_VALUE(ch);
    SCM_CHAR_SET_VALUE(ch, ICHAR_UPCASE(val));

    return ch;
}

ScmObj
scm_p_char_downcase(ScmObj ch)
{
    scm_ichar_t val;
    DECLARE_FUNCTION("char-downcase", procedure_fixed_1);

    ENSURE_CHAR(ch);

    val = SCM_CHAR_VALUE(ch);
    SCM_CHAR_SET_VALUE(ch, ICHAR_DOWNCASE(val));

    return ch;
}

/*===========================================================================
  R5RS : 6.3 Other data types : 6.3.5 Strings
===========================================================================*/
ScmObj
scm_p_stringp(ScmObj obj)
{
    DECLARE_FUNCTION("string?", procedure_fixed_1);

    return MAKE_BOOL(STRINGP(obj));
}

ScmObj
scm_p_make_string(ScmObj length, ScmObj args)
{
    ScmObj filler;
    scm_ichar_t filler_val;
    size_t len;
    int ch_len;
    char *str, *dst;
#if SCM_USE_MULTIBYTE_CHAR
    const char *next;
    char ch_str[SCM_MB_MAX_LEN + sizeof("")];
#endif
    DECLARE_FUNCTION("make-string", procedure_variadic_1);

    ENSURE_STATELESS_CODEC(scm_current_char_codec);
    ENSURE_INT(length);
    len = SCM_INT_VALUE(length);
    if (len == 0)
        return MAKE_STRING_COPYING("", 0);
    if (len < 0)
        ERR_OBJ("length must be a positive integer", length);

    /* extract filler */
    if (NULLP(args)) {
        filler_val = ' ';
        ch_len = sizeof((char)' ');
    } else {
        filler = POP(args);
        ASSERT_NO_MORE_ARG(args);
        ENSURE_CHAR(filler);
        filler_val = SCM_CHAR_VALUE(filler);
#if SCM_USE_MULTIBYTE_CHAR
        ch_len = SCM_CHARCODEC_CHAR_LEN(scm_current_char_codec, filler_val);
#endif
    }
#if !SCM_USE_NULL_CAPABLE_STRING
    if (filler_val == '\0')
        ERR("make-string: " SCM_ERRMSG_NULL_IN_STRING);
#endif

#if SCM_USE_MULTIBYTE_CHAR
    next = SCM_CHARCODEC_INT2STR(scm_current_char_codec, ch_str, filler_val,
                                 SCM_MB_STATELESS);
    if (!next)
        ERR("make-string: invalid char 0x%x for encoding %s",
            (int)filler_val, SCM_CHARCODEC_ENCODING(scm_current_char_codec));

    str = scm_malloc(ch_len * len + sizeof(""));
    for (dst = str; dst < &str[ch_len * len]; dst += ch_len)
        memcpy(dst, ch_str, ch_len);
#else
    SCM_ASSERT(isascii(filler_val));
    str = scm_malloc(len + sizeof(""));
    for (dst = str; dst < &str[len];)
        *dst++ = filler_val;
#endif
    *dst = '\0';

    return MAKE_STRING(str, len);
}

ScmObj
scm_p_string(ScmObj args)
{
    DECLARE_FUNCTION("string", procedure_variadic_0);

    return scm_p_list2string(args);
}

ScmObj
scm_p_string_length(ScmObj str)
{
    scm_int_t len;
    DECLARE_FUNCTION("string-length", procedure_fixed_1);

    ENSURE_STRING(str);

#if SCM_USE_MULTIBYTE_CHAR
    len = scm_mb_bare_c_strlen(scm_current_char_codec, SCM_STRING_STR(str));
#else
    len = SCM_STRING_LEN(str);
#endif

    return MAKE_INT(len);
}

ScmObj
scm_p_string_ref(ScmObj str, ScmObj k)
{
    scm_int_t idx;
    scm_ichar_t ch;
#if SCM_USE_MULTIBYTE_CHAR
    ScmMultibyteString mbs;
#endif
    DECLARE_FUNCTION("string-ref", procedure_fixed_2);

    ENSURE_STRING(str);
    ENSURE_INT(k);

    idx = SCM_INT_VALUE(k);
    if (idx < 0 || SCM_STRING_LEN(str) <= idx)
        ERR_OBJ("index out of range", k);

#if SCM_USE_MULTIBYTE_CHAR
    SCM_MBS_INIT2(mbs, SCM_STRING_STR(str), strlen(SCM_STRING_STR(str)));
    mbs = scm_mb_strref(scm_current_char_codec, mbs, idx);

    ch = SCM_CHARCODEC_STR2INT(scm_current_char_codec, SCM_MBS_GET_STR(mbs),
                               SCM_MBS_GET_SIZE(mbs), SCM_MBS_GET_STATE(mbs));
    if (ch == EOF)
        ERR("string-ref: invalid char sequence");
#else
    ch = ((unsigned char *)SCM_STRING_STR(str))[idx];
#endif

    return MAKE_CHAR(ch);
}

ScmObj
scm_p_string_setd(ScmObj str, ScmObj k, ScmObj ch)
{
    scm_int_t idx;
    scm_ichar_t ch_val;
    char *c_str;
#if SCM_USE_MULTIBYTE_CHAR
    int ch_len, orig_ch_len;
    size_t prefix_len, suffix_len, new_str_len;
    const char *suffix_src, *ch_end;
    char *new_str, *suffix_dst;
    char ch_buf[SCM_MB_MAX_LEN + sizeof("")];
    ScmMultibyteString mbs_ch;
#endif
    DECLARE_FUNCTION("string-set!", procedure_fixed_3);

    ENSURE_STATELESS_CODEC(scm_current_char_codec);
    ENSURE_STRING(str);
    ENSURE_MUTABLE_STRING(str);
    ENSURE_INT(k);
    ENSURE_CHAR(ch);

    idx = SCM_INT_VALUE(k);
    c_str = SCM_STRING_STR(str);
    if (idx < 0 || SCM_STRING_LEN(str) <= idx)
        ERR_OBJ("index out of range", k);

#if SCM_USE_MULTIBYTE_CHAR
    /* point at the char that to be replaced */
    SCM_MBS_INIT2(mbs_ch, c_str, strlen(c_str));
    mbs_ch = scm_mb_strref(scm_current_char_codec, mbs_ch, idx);
    orig_ch_len = SCM_MBS_GET_SIZE(mbs_ch);
    prefix_len = SCM_MBS_GET_STR(mbs_ch) - c_str;

    /* prepare new char */
    ch_val = SCM_CHAR_VALUE(ch);
    ch_end = SCM_CHARCODEC_INT2STR(scm_current_char_codec, ch_buf, ch_val,
                                   SCM_MB_STATELESS);
    if (!ch_end)
        ERR("string-set!: invalid char 0x%x for encoding %s",
            (int)ch_val, SCM_CHARCODEC_ENCODING(scm_current_char_codec));
    ch_len = ch_end - ch_buf;

    /* prepare the space for new char */
    if (ch_len == orig_ch_len) {
        new_str = c_str;
    } else {
        suffix_src = &SCM_MBS_GET_STR(mbs_ch)[orig_ch_len];
        suffix_len = strlen(suffix_src);

        new_str_len = prefix_len + ch_len + suffix_len;
        new_str = scm_realloc(c_str, new_str_len + sizeof(""));

        suffix_src = &new_str[prefix_len + orig_ch_len];
        suffix_dst = &new_str[prefix_len + ch_len];
        memmove(suffix_dst, suffix_src, suffix_len);
        new_str[new_str_len] = '\0';
    }

    /* set new char */
    memcpy(&new_str[prefix_len], ch_buf, ch_len);

    SCM_STRING_SET_STR(str, new_str);
#else
    ch_val = SCM_CHAR_VALUE(ch);
    SCM_ASSERT(isascii(ch_val));
    c_str[idx] = ch_val;
#endif

    return str;
}

#if (!HAVE_STRCASECMP && !SCM_USE_MULTIBYTE_CHAR)
static int
strcasecmp(const char *s1, const char *s2)
{
    unsigned char c1, c2;

    for (;;) {
        c1 = *(const unsigned char *)s1;
        c2 = *(const unsigned char *)s2;

        if (c1 && !c2)
            return 1;
        if (!c1 && c2)
            return -1;
        if (!c1 && !c2)
            return 0;

        if (isascii(c1))
            c1 = tolower(c1);
        if (isascii(c2))
            c2 = tolower(c2);
        
        if (c1 > c2)
            return 1;
        if (c1 < c2)
            return -1;
    }
}
#endif

/* Upper case letters are less than lower. */
static int
string_cmp(const char *funcname,
           ScmObj str1, ScmObj str2, scm_bool case_insensitive)
{
    const char *c_str1, *c_str2;
#if SCM_USE_MULTIBYTE_CHAR
    scm_ichar_t c1, c2;
    ScmMultibyteString mbs1, mbs2;
#endif
    DECLARE_INTERNAL_FUNCTION("string_cmp");

    /* dirty hack to replace internal function name */
    SCM_MANGLE(name) = funcname;

    ENSURE_STRING(str1);
    ENSURE_STRING(str2);

    c_str1 = SCM_STRING_STR(str1);
    c_str2 = SCM_STRING_STR(str2);
#if SCM_USE_MULTIBYTE_CHAR
    SCM_MBS_INIT2(mbs1, c_str1, strlen(c_str1));
    SCM_MBS_INIT2(mbs2, c_str2, strlen(c_str2));
    for (;;) {
        if (SCM_MBS_GET_SIZE(mbs1) && !SCM_MBS_GET_SIZE(mbs2))
            return 1;
        if (!SCM_MBS_GET_SIZE(mbs1) && SCM_MBS_GET_SIZE(mbs2))
            return -1;
        if (!SCM_MBS_GET_SIZE(mbs1) && !SCM_MBS_GET_SIZE(mbs2))
            return 0;

        c1 = SCM_CHARCODEC_READ_CHAR(scm_current_char_codec, mbs1);
        c2 = SCM_CHARCODEC_READ_CHAR(scm_current_char_codec, mbs2);
        if (case_insensitive) {
            c1 = ICHAR_FOLDCASE(c1);
            c2 = ICHAR_FOLDCASE(c2);
        }
        
        if (c1 > c2)
            return 1;
        if (c1 < c2)
            return -1;
    }
#else /* SCM_USE_MULTIBYTE_CHAR */
    if (case_insensitive) {
        return strcasecmp(c_str1, c_str2);
    } else {
        return strcmp(c_str1, c_str2);
    }
#endif /* SCM_USE_MULTIBYTE_CHAR */
}

ScmObj
scm_p_stringequalp(ScmObj str1, ScmObj str2)
{
    DECLARE_FUNCTION("string=?", procedure_fixed_2);

    ENSURE_STRING(str1);
    ENSURE_STRING(str2);

    return MAKE_BOOL(STRING_EQUALP(str1, str2));
}

ScmObj
scm_p_string_ci_equalp(ScmObj str1, ScmObj str2)
{
    DECLARE_FUNCTION("string-ci=?", procedure_fixed_2);

    ENSURE_STRING(str1);
    ENSURE_STRING(str2);

    return MAKE_BOOL(EQ((str1), (str2))                                     
                     || (SCM_STRING_LEN(str1) == SCM_STRING_LEN(str2)
                         && STRING_CI_CMP(str1, str2) == 0));
}

ScmObj
scm_p_string_greaterp(ScmObj str1, ScmObj str2)
{
    DECLARE_FUNCTION("string>?", procedure_fixed_2);

    return MAKE_BOOL(STRING_CMP(str1, str2) > 0);
}

ScmObj
scm_p_string_lessp(ScmObj str1, ScmObj str2)
{
    DECLARE_FUNCTION("string<?", procedure_fixed_2);

    return MAKE_BOOL(STRING_CMP(str1, str2) < 0);
}

ScmObj
scm_p_string_greater_equalp(ScmObj str1, ScmObj str2)
{
    DECLARE_FUNCTION("string>=?", procedure_fixed_2);

    return MAKE_BOOL(STRING_CMP(str1, str2) >= 0);
}

ScmObj
scm_p_string_less_equalp(ScmObj str1, ScmObj str2)
{
    DECLARE_FUNCTION("string<=?", procedure_fixed_2);

    return MAKE_BOOL(STRING_CMP(str1, str2) <= 0);
}

ScmObj
scm_p_string_ci_greaterp(ScmObj str1, ScmObj str2)
{
    DECLARE_FUNCTION("string-ci>?", procedure_fixed_2);

    return MAKE_BOOL(STRING_CI_CMP(str1, str2) > 0);
}

ScmObj
scm_p_string_ci_lessp(ScmObj str1, ScmObj str2)
{
    DECLARE_FUNCTION("string-ci<?", procedure_fixed_2);

    return MAKE_BOOL(STRING_CI_CMP(str1, str2) < 0);
}

ScmObj
scm_p_string_ci_greater_equalp(ScmObj str1, ScmObj str2)
{
    DECLARE_FUNCTION("string-ci>=?", procedure_fixed_2);

    return MAKE_BOOL(STRING_CI_CMP(str1, str2) >= 0);
}

ScmObj
scm_p_string_ci_less_equalp(ScmObj str1, ScmObj str2)
{
    DECLARE_FUNCTION("string-ci<=?", procedure_fixed_2);

    return MAKE_BOOL(STRING_CI_CMP(str1, str2) <= 0);
}

ScmObj
scm_p_substring(ScmObj str, ScmObj start, ScmObj end)
{
    scm_int_t c_start, c_end, len, sub_len;
    const char *c_str;
    char *new_str;
#if SCM_USE_MULTIBYTE_CHAR
    ScmMultibyteString mbs;
#endif
    DECLARE_FUNCTION("substring", procedure_fixed_3);

    ENSURE_STRING(str);
    ENSURE_INT(start);
    ENSURE_INT(end);

    c_start = SCM_INT_VALUE(start);
    c_end   = SCM_INT_VALUE(end);
    len     = SCM_STRING_LEN(str);

    if (c_start < 0 || len < c_start)
        ERR_OBJ("start index out of range", start);
    if (c_end < 0 || len < c_end)
        ERR_OBJ("end index out of range", end);
    if (c_start > c_end)
        ERR_OBJ("start index exceeded end index", LIST_2(start, end));

    c_str = SCM_STRING_STR(str);
    sub_len = c_end - c_start;

#if SCM_USE_MULTIBYTE_CHAR
    /* substring */
    SCM_MBS_INIT2(mbs, c_str, strlen(c_str));
    mbs = scm_mb_substring(scm_current_char_codec, mbs, c_start, sub_len);

    /* copy the substring */
    new_str = scm_malloc(SCM_MBS_GET_SIZE(mbs) + sizeof(""));
    memcpy(new_str, SCM_MBS_GET_STR(mbs), SCM_MBS_GET_SIZE(mbs));
    new_str[SCM_MBS_GET_SIZE(mbs)] = '\0';
#else
    new_str = scm_malloc(sub_len + sizeof(""));
    memcpy(new_str, &c_str[c_start], sub_len);
    new_str[sub_len] = '\0';
#endif

#if SCM_USE_NULL_CAPABLE_STRING
    /* FIXME: the result is truncated at null and incorrect */
    return MAKE_STRING(new_str, STRLEN_UNKNOWN);
#else
    return MAKE_STRING(new_str, sub_len);
#endif
}

/* FIXME: support stateful encoding */
ScmObj
scm_p_string_append(ScmObj args)
{
    ScmObj rest, str;
    size_t byte_len;
    scm_int_t mb_len;
    char  *new_str, *dst;
    const char *src;
    DECLARE_FUNCTION("string-append", procedure_variadic_0);

    if (NULLP(args))
        return MAKE_STRING_COPYING("", 0);

    /* count total size of the new string */
    byte_len = mb_len = 0;
    rest = args;
    FOR_EACH (str, rest) {
        ENSURE_STRING(str);
        mb_len   += SCM_STRING_LEN(str);
#if SCM_USE_MULTIBYTE_CHAR
        byte_len += strlen(SCM_STRING_STR(str));
#else
        byte_len = mb_len;
#endif
    }

    new_str = scm_malloc(byte_len + sizeof(""));

    /* copy all strings into new_str */
    dst = new_str;
    FOR_EACH (str, args) {
        for (src = SCM_STRING_STR(str); *src;)
            *dst++ = *src++;
    }
    *dst = '\0';

#if SCM_USE_NULL_CAPABLE_STRING
    /* each string is chopped at first null and the result is incorrect */
    return MAKE_STRING(new_str, STRLEN_UNKNOWN);
#else
    return MAKE_STRING(new_str, mb_len);
#endif
}

ScmObj
scm_p_string2list(ScmObj str)
{
#if SCM_USE_MULTIBYTE_CHAR
    ScmMultibyteString mbs;
    ScmQueue q;
#endif
    ScmObj res;
    scm_ichar_t ch;
    scm_int_t mb_len;
    const char *c_str;
    DECLARE_FUNCTION("string->list", procedure_fixed_1);

    ENSURE_STRING(str);

    c_str = SCM_STRING_STR(str);
    mb_len = SCM_STRING_LEN(str);

    res = SCM_NULL;
#if SCM_USE_MULTIBYTE_CHAR
    SCM_QUEUE_POINT_TO(q, res);
    SCM_MBS_INIT2(mbs, c_str, strlen(c_str));
    while (mb_len--) {
        if (SCM_MBS_GET_SIZE(mbs)) {
            ch = SCM_CHARCODEC_READ_CHAR(scm_current_char_codec, mbs);
        } else {
#if SCM_USE_NULL_CAPABLE_STRING
            /* CAUTION: this code may crash when (scm_current_char_codec !=
             * orig_codec) */
            ch = '\0';
            c_str = &SCM_MBS_GET_STR(mbs)[1];
            SCM_MBS_INIT2(mbs, c_str, strlen(c_str));
#else
            break;
#endif /* SCM_USE_NULL_CAPABLE_STRING */
        }
        SCM_QUEUE_ADD(q, MAKE_CHAR(ch));
    }
#else /* SCM_USE_MULTIBYTE_CHAR */
    while (mb_len) {
        ch = ((unsigned char *)c_str)[--mb_len];
        res = CONS(MAKE_CHAR(ch), res);
    }
#endif /* SCM_USE_MULTIBYTE_CHAR */

    return res;
}

ScmObj
scm_p_list2string(ScmObj lst)
{
    ScmObj rest, ch;
    size_t str_size;
    scm_int_t len;
    char *str, *dst;
#if SCM_USE_MULTIBYTE_CHAR
    scm_ichar_t ch_val;
#endif
    DECLARE_FUNCTION("list->string", procedure_fixed_1);

    ENSURE_STATELESS_CODEC(scm_current_char_codec);
    ENSURE_LIST(lst);

    if (NULLP(lst))
        return MAKE_STRING_COPYING("", 0);

    str_size = sizeof("");
    rest = lst;
    len = 0;
    FOR_EACH (ch, rest) {
        ENSURE_CHAR(ch);
#if SCM_USE_MULTIBYTE_CHAR
        ch_val = SCM_CHAR_VALUE(ch);
        str_size += SCM_CHARCODEC_CHAR_LEN(scm_current_char_codec, ch_val);
#else
        str_size++;
#endif
        len++;
    }
    ENSURE_PROPER_LIST_TERMINATION(rest, lst);

    dst = str = scm_malloc(str_size);
    FOR_EACH (ch, lst) {
#if !SCM_USE_NULL_CAPABLE_STRING
        if (ch == '\0')
            ERR("list->string: " SCM_ERRMSG_NULL_IN_STRING);
#endif
#if SCM_USE_MULTIBYTE_CHAR
        dst = SCM_CHARCODEC_INT2STR(scm_current_char_codec, dst,
                                    SCM_CHAR_VALUE(ch), SCM_MB_STATELESS);
#else
        *dst++ = SCM_CHAR_VALUE(ch);
#endif
    }
#if !SCM_USE_MULTIBYTE_CHAR
    *dst = '\0';
#endif

    return MAKE_STRING(str, len);
}

ScmObj
scm_p_string_copy(ScmObj str)
{
    DECLARE_FUNCTION("string-copy", procedure_fixed_1);

    ENSURE_STRING(str);

#if SCM_USE_NULL_CAPABLE_STRING
    /* result is truncated at first null and incorrect */
    return MAKE_STRING_COPYING(SCM_STRING_STR(str), STRLEN_UNKNOWN);
#else
    return MAKE_STRING_COPYING(SCM_STRING_STR(str), SCM_STRING_LEN(str));
#endif
}

ScmObj
scm_p_string_filld(ScmObj str, ScmObj ch)
{
    size_t str_len;
    char *dst;
#if SCM_USE_MULTIBYTE_CHAR
    int ch_len;
    char *new_str;
    char ch_str[SCM_MB_MAX_LEN + sizeof("")];
    const char *next;
#else
    scm_ichar_t ch_val;
    char *c_str;
#endif
    DECLARE_FUNCTION("string-fill!", procedure_fixed_2);

    ENSURE_STATELESS_CODEC(scm_current_char_codec);
    ENSURE_STRING(str);
    ENSURE_MUTABLE_STRING(str);
    ENSURE_CHAR(ch);

    str_len = SCM_STRING_LEN(str);
    if (str_len == 0)
        return MAKE_STRING_COPYING("", 0);

#if SCM_USE_MULTIBYTE_CHAR
    next = SCM_CHARCODEC_INT2STR(scm_current_char_codec, ch_str,
                                 SCM_CHAR_VALUE(ch), SCM_MB_STATELESS);
    if (!next)
        ERR("string-fill!: invalid char 0x%x for encoding %s",
            (int)SCM_CHAR_VALUE(ch),
            SCM_CHARCODEC_ENCODING(scm_current_char_codec));

    /* create new str */
    ch_len = next - ch_str;
    new_str = scm_realloc(SCM_STRING_STR(str), str_len * ch_len + sizeof(""));
    for (dst = new_str; dst < &new_str[ch_len * str_len]; dst += ch_len)
        memcpy(dst, ch_str, ch_len);
    *dst = '\0';

    SCM_STRING_SET_STR(str, new_str);
#else
    ch_val = SCM_CHAR_VALUE(ch);
    SCM_ASSERT(isascii(ch_val));
    c_str = SCM_STRING_STR(str);
    for (dst = c_str; dst < &c_str[str_len]; dst++)
        *dst = ch_val;
#endif

    return str;
}
