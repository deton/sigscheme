/*===========================================================================
 *  Filename : storage-compact.h
 *  About    : Storage abstraction (compact representation)
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
#ifndef __STORAGE_COMPACT_H
#define __STORAGE_COMPACT_H

/*
 * Internal representation defined in this file MUST NOT directly touched by
 * libsscm users. Use abstract public APIs defined in sigscheme.h.
 */

/*
 * Object Representation
 *
 * First, we assume ScmObj "S" which contains two ScmObj "X" and "Y" (e.g.
 * ScmObj S { X, Y }).
 *
 * (0) LSB(Least Significant Bit) of "S" is called G-bit.
 *
 * (1) if S == "...00G", S is ConsCell. G-bit of S->car is used as the
 *     GC mark bit.  S->cdr's G bit is always set to 0, which helps
 *     determine the finalization semantics without a pointer.
 *
 * (2) if S == "...01G", S is Closure. G-bit of S->car is used as marking bit
 *     of GC. S->cdr's G bit is always set to 0, which helps determine the
 *     finalization semantics without a pointer.
 *
 * (4) if S == "...10G", S is other types. Type is separated by the value of
 *     least n bits of S->cdr. S->cdr's G bit is always set to 1, which helps
 *     determine the finalization semantics without a pointer.
 *
 *        S->car   |     Type     |             content of S->car
 *     -------------------------------------------------------------------------
 *     .......|I|G : String       : I bit is used to represent mutable or
 *                                  immutable string. G bit is used to GC mark
 *                                  information (Mutation Bit). The other bits
 *                                  are used to store string pointer value.
 *     .........|G : Otherwise    : LSB is used to GC mark information. The value
 *                                  of each type is stored in the other bits.
 *
 *        S->cdr   |     Type     |             content of S->cdr
 *     -------------------------------------------------------------------------
 *     ......|00|1 : Symbol       : symbol name
 *     ......|01|1 : String       : string length
 *     ......|10|1 : Vector       : vector length
 *     ...000|11|1 : Values       : all 0 (for efficiency)
 *     ...001|11|1 : Func         : ScmFuncTypeCode and LSB of stored Func address
 *     ...010|11|1 : Port         : ScmPortDirection
 *     ...011|11|1 : Continuation : tag
 *     ...100|11|1 : Pointer      : pointer type id
 *     .00100|11|1 :  - C Ptr     :
 *     .01100|11|1 :  - C FuncPtr :
 *     .10100|11|1 :  - Reserved  :
 *     .11100|11|1 :  - Reserved  :
 *     ...101|11|1 : Wrapper      : depends on type
#if SCM_USE_HYGIENIC_MACRO
 *     .00101|11|1 : subpat       : metainformation about the wrapped object
 *     .01101|11|1 : far symbol   : [#if !SCM_USE_UNHYGIENIC_MACRO] env depth
 *     .10101|11|1 : macro        : [#if !SCM_USE_UNHYGIENIC_MACRO] env depth
#endif / * SCM_USE_HYGIENIC_MACRO * /
 *     .11101|11|1 : Reserved     :
 *     ...110|11|1 : Reserved6    :
 *     ...111|11|1 : FreeCell     : all 0 (for efficiency)
 *
 *     Misc. types' tags come in several levels, including the GC bit:
 *
 *     .|..|...|ZZ|Z : level 1
 *     .|..|ZZZ|ZZ|Z : level 2
 *     .|ZZ|ZZZ|ZZ|Z : level 3
 *
 * (4) if S == "...11G", S is an immediate value. Immediate values are
 *     separated into these types by the value of least 1-5 bits of
 *     ((unsigned int S) >> 3).
 *
 *           S        Type
 *     ......0|11G : Integer
 *     .....01|11G : Char
 *     .....11|11G : Constant
 *     ------------------------------
 *     Constants
 *     .000|11|11G : ()
 *     .001|11|11G : INVALID
 *     .010|11|11G : UNBOUND
 *     .011|11|11G : #f
 *     .100|11|11G : #t
 *     .101|11|11G : EOF
 *     .110|11|11G : UNDEF
 *
 * Notice:
 *   Some data must be aligned properly for compaction.
 *   Required Alignments are listed below.
 *
 * Required Data Aligment:
 *
 *     Symbol
 *         name (char *)        : 8 byte
 *     String
 *         str (char *)         : 4 byte
 *     Vector
 *         vec (ScmObj *)       : 2 byte
 *     Port
 *         impl (ScmCharPort *) : 2 byte
 *     Continuation
 *         opaque (void *)      : 2 byte
 */

#include <limits.h>
#include <stddef.h>

/* Don't include scmport.h. The implementations are internal and should not be
 * exposed to libsscm users via installation of this file. */

#ifdef __cplusplus
/* extern "C" { */
#endif


/* Aux. */
#define SCM_MAKE_MASK(offset, width) \
    ((scm_uintobj_t)((1 << ((offset) + (width))) - (1 << (offset))))

#if HAVE_ARITHMETIC_SHIFT
#define SCM_RSHIFT(x, n)    ((scm_intobj_t)(x) >> (n))
#else  /* not HAVE_ARITHMETIC_SHIFT */
/* Emulate a right arithmetic shift. */
#define SCM_RSHIFT(x, n)                        \
   (((scm_uintobj_t)(x) >> (n)) |               \
    (-((scm_uintobj_t)(x) >> (n))               \
     & (~0U << (sizeof(n) * CHAR_BIT - (n)))))
#endif /* not HAVE_ARITHMETIC_SHIFT */


/* ------------------------------------------------------------
 * Crude representation.
 */

typedef struct ScmCell_ ScmCell;

/* Note that this is unsigned.  Signed operations are desirable only
 * in a few, specific cases. */
typedef scm_uintobj_t ScmObj;

struct ScmCell_ {
    /* The field names have some redundancy to avoid conflict with
     * macros' formal arguments and stuff. */
    ScmObj obj_x;
    ScmObj obj_y;
};

typedef ScmObj (*ScmFuncType)();

/* ScmObj = .....|PP|G
 * G = GC bit
 * P = Primary tag (ptag)
 */
#define SCM_GCBIT_OFFSET     0
#define SCM_GCBIT_WIDTH      1
#define SCM_GCBIT_MASK       SCM_MAKE_MASK(SCM_GCBIT_OFFSET, SCM_GCBIT_WIDTH)
#define SCM_GCBIT(o)         ((o) & SCM_GCBIT_MASK)
#define SCM_PTAG_OFFSET      (SCM_GCBIT_WIDTH + SCM_GCBIT_OFFSET)
#define SCM_PTAG_WIDTH       2
#define SCM_PTAG_MASK        SCM_MAKE_MASK(SCM_PTAG_OFFSET, SCM_PTAG_WIDTH)
#define SCM_MAKE_PTAG(id)    ((id) << SCM_PTAG_OFFSET)
#define SCM_PTAG(o)          ((o) & SCM_PTAG_MASK)
#define SCM_PTAG_SET(o, tag) ((o) = ((o) & ~SCM_PTAG_MASK) | (tag))
#define SCM_PTAG_EQ(o, tag)  (SCM_PTAG(o) == (tag))

#define SCM_STRIP_PTAG(o)     ((o) & ~SCM_PTAG_MASK)
#define SCM_STRIP_GCBIT(o)    ((o) & ~SCM_GCBIT_MASK)
#define SCM_STRIP_TAG(o)      ((o) & ~(SCM_GCBIT_MASK | SCM_PTAG_MASK))

#define SCM_UNTAGGEDP(o)      ((o) == SCM_UNTAG(o))

/* A ScmObj lying within a marked cell is never dereferenced and thus
 * no stripping is necessary if 0 means unmarked. */
#if SCM_GCBIT_UNMARKED
#define SCM_PTR(o)       ((ScmCell*)SCM_STRIP_GCBIT(o))
#define SCM_SAL_EQ(a, b) (SCM_STRIP_GCBIT(a) == SCM_STRIP_GCBIT(b))
#else
#define SCM_PTR(o)       ((ScmCell*)(o))
#define SCM_SAL_EQ(a, b) ((a) == (b))
#endif

#define SCM_X(o)        (SCM_PTR(o)->obj_x)
#define SCM_Y(o)        (SCM_PTR(o)->obj_y)
#define SCM_SET_X(o, x) (SCM_X(o) = (x))
#define SCM_SET_Y(o, y) (SCM_Y(o) = (y))
#define SCM_INIT(o, x, y, ptag)                 \
    (SCM_ASSERT(SCM_UNTAGGEDP(o)),              \
     SCM_SET_X((o), (x)),                       \
     SCM_SET_Y((o), (y)),                       \
     (o) |= (ptag))

/* Returns a dereferencable pointer. */
#define SCM_STRIPPED_PTR(o) ((ScmCell*)SCM_STRIP_TAG(o))

#if SCM_USE_ACCESSOR_ASSERT
#define SCM_ACCESS_ASSERT(cond) SCM_ASSERT(cond)
#else
#define SCM_ACCESS_ASSERT(cond) SCM_EMPTY_EXPR
#endif

/* ------------------------------------------------------------
 * Garbage collection
 */

#define SCM_GCBIT_MARKED     ((scm_uintobj_t)1)  /* More or less hard-coded. */
#define SCM_GCBIT_UNMARKED   ((scm_uintobj_t)0)  /* Ditto. */

/* Always invoked on unmarked objects. */
#define SCM_SAL_MARK(o) SCM_SET_X((o), SCM_X(o) | SCM_GCBIT_MARKED)

#define SCM_SAL_UNMARK(o)                                                \
    SCM_SET_X((o), (SCM_X(o) & ~SCM_GCBIT_MASK) | SCM_GCBIT_UNMARKED)
#define SCM_SAL_MARKEDP(o)   (SCM_GCBIT(SCM_X(o)) == SCM_GCBIT_MARKED)
#define SCM_SAL_UNMARKEDP(o) (!SCM_GC_MARKEDP(o))

/* See if O's tag and the content of the cell it references are
 * consistent.  O must be a tagged pointer into the heap. */
#define SCM_GC_VALID_REFP(o) (!!SCM_SYMMETRICP(o)                           \
                              != !!SCM_CELL_MISCP(SCM_STRIPPED_PTR(o)))

/* ------------------------------------------------------------
 * Symmetric types (both obj_x and obj_y point to some other ScmCell).
 * Pairs and closures are chosen for their prevalence.
 */

/* SCM_GC_VALID_REFP() needs this. */
#define SCM_SYMMETRICP(o)       (!(SCM_PTAG(o) & SCM_MAKE_PTAG(2)))

/* Pairs. */
#define SCM_PTAG_CONS           SCM_MAKE_PTAG(0)
/* Bypass ptag stripping. */
#define SCM_CONS_PTR(o)         SCM_PTR(SCM_AS_CONS(o))

#define SCM_SAL_ENTYPE_CONS(o)  SCM_PTAG_SET((o), SCM_PTAG_CONS)
#define SCM_SAL_CONSP(o)        SCM_PTAG_EQ((o), SCM_PTAG_CONS)
#define SCM_SAL_CAR(o)          SCM_X(SCM_CONS_PTR(o))
#define SCM_SAL_CDR(o)          SCM_Y(SCM_CONS_PTR(o))
#define SCM_SAL_SET_CAR(o, kar) (SCM_SAL_CAR(o) = (kar))
#define SCM_SAL_SET_CDR(o, kdr) (SCM_SAL_CDR(o) = (kdr))
#define SCM_SAL_CONS_INIT(o, ar, dr) SCM_INIT((o), (ar), (dr), SCM_PTAG_CONS)

/* Closures. */
#define SCM_PTAG_CLOSURE               SCM_MAKE_PTAG(1)
#define SCM_CLOSURE_PTR(o)             SCM_STRIPPED_PTR(SCM_AS_CLOSURE(o))

#define SCM_SAL_ENTYPE_CLOSURE(o)      SCM_PTAG_SET((o), SCM_PTAG_CLOSURE)
#define SCM_SAL_CLOSUREP(o)            SCM_PTAG_EQ((o), SCM_PTAG_CLOSURE)
#define SCM_SAL_CLOSURE_CODE(o)        SCM_X(SCM_CLOSURE_PTR(o))
#define SCM_SAL_CLOSURE_ENV(o)         SCM_Y(SCM_CLOSURE_PTR(o))
#define SCM_SAL_CLOSURE_SET_CODE(o, c) SCM_SET_X(SCM_CLOSURE_PTR(o), (c))
#define SCM_SAL_CLOSURE_SET_ENV(o, e)  SCM_SET_Y(SCM_CLOSURE_PTR(o), (e))
#define SCM_SAL_CLOSURE_INIT(o, c, e)  SCM_INIT((o), (c), (e), SCM_PTAG_CLOSURE)
/* ------------------------------------------------------------
 * Immediate types (ones that fit on the pointer including type tags).
 */

/* Immediate ScmObj = VVVVIIPPG
 * V = Numerical value of the object.
 * I = Immediate type ID; further distinguishes types.  Only 1 bit
 *     wide for integers, 2 bits for others.
 * P = 3 (signature for immediates)
 */
#define SCM_PTAG_IMM                SCM_MAKE_PTAG(3)
#define SCM_IMMP(o)                 SCM_PTAG_EQ((o), SCM_PTAG_IMM)
#define SCM_IMMID_OFFSET            (SCM_PTAG_OFFSET + SCM_PTAG_WIDTH)
#define SCM_MAKE_IMMID(val)         ((val) << SCM_IMMID_OFFSET)
#define SCM_MAKE_ITAG(id)           ((id) | SCM_PTAG_IMM)
#define SCM_MAKE_ITAG_MASK(id_w)    SCM_MAKE_MASK(SCM_PTAG_OFFSET,         \
                                                  (id_w) + SCM_PTAG_WIDTH)
#define SCM_MAKE_VAL_OFFSET(id_w)   (SCM_IMMID_OFFSET + (id_w))
#define SCM_ITAG_EQ(o, tag, mask)   (((o) & (mask)) == (tag))

/* Integers. */
#define SCM_IMMID_INT            SCM_MAKE_IMMID(0)
#define SCM_IMMID_WIDTH_INT      1
#define SCM_ITAG_INT             SCM_MAKE_ITAG(SCM_IMMID_INT)
#define SCM_ITAG_MASK_INT        SCM_MAKE_ITAG_MASK(SCM_IMMID_WIDTH_INT)
#define SCM_INT_VAL_OFFSET       (SCM_IMMID_OFFSET + SCM_IMMID_WIDTH_INT)
#define SCM_SAL_INTP(o)          SCM_ITAG_EQ((o), SCM_ITAG_INT, \
                                             SCM_ITAG_MASK_INT)
#define SCM_SAL_MAKE_INT(i)      ((ScmObj)((i) << SCM_INT_VAL_OFFSET))
#define SCM_SAL_INT_VALUE(o)     ((scm_int_t)                             \
                                  SCM_RSHIFT((scm_intobj_t)SCM_AS_INT(o), \
                                             SCM_INT_VAL_OFFSET))

/* Characters. */
#define SCM_IMMID_CHAR          SCM_MAKE_IMMID(1)
#define SCM_IMMID_WIDTH_CHAR    2
#define SCM_ITAG_CHAR           SCM_MAKE_ITAG(SCM_IMMID_CHAR)
#define SCM_ITAG_MASK_CHAR      SCM_MAKE_ITAG_MASK(SCM_IMMID_WIDTH_CHAR)
#define SCM_SAL_CHARP(o)        SCM_ITAG_EQ((o), SCM_ITAG_CHAR,        \
                                            SCM_ITAG_MASK_CHAR)
#define SCM_SAL_MAKE_CHAR(c)    (((c) << SCM_CHAR_VAL_OFFSET) | SCM_ITAG_CHAR)
/* FIXME: this should be cast to something along the lines of scm_char_t. */
#define SCM_SAL_CHAR_VALUE(o)   ((int)(SCM_AS_CHAR(c) >> SCM_CHAR_VAL_OFFSET))

/* Singleton constants. */
#define SCM_IMMID_CONST         SCM_MAKE_IMMID(3)
#define SCM_IMMID_WIDTH_CONST   2
#define SCM_ITAG_CONST          SCM_MAKE_ITAG(SCM_IMMID_CONST)
/* #define SCM_ITAG_MASK_CONST */
#define SCM_CONST_VAL_OFFSET    SCM_MAKE_VAL_OFFSET(SCM_IMMID_WIDTH_CONST)
#define SCM_MAKE_CONST(i)       ((i) << SCM_CONST_VAL_OFFSET | SCM_ITAG_CONST)

#define SCM_SAL_NULL        SCM_MAKE_CONST(0)
#define SCM_SAL_INVALID     SCM_MAKE_CONST(1)
#define SCM_SAL_UNBOUND     SCM_MAKE_CONST(2)
#define SCM_SAL_FALSE       SCM_MAKE_CONST(3)
#define SCM_SAL_TRUE        SCM_MAKE_CONST(4)
#define SCM_SAL_EOF         SCM_MAKE_CONST(5)
#define SCM_SAL_UNDEF       SCM_MAKE_CONST(6)

#define SCM_SAL_NULLP(o)    SCM_SAL_EQ((o), SCM_SAL_NULL)
#define SCM_SAL_VALIDP(o)   (!SCM_SAL_EQ((o), SCM_SAL_INVALID))
#define SCM_SAL_UNBOUNDP(o) SCM_SAL_EQ((o), SCM_SAL_UNBOUND)
#define SCM_SAL_FALSEP(o)   SCM_SAL_EQ((o), SCM_SAL_FALSE)
#define SCM_SAL_TRUEP(o)    SCM_SAL_EQ((o), SCM_SAL_TRUE)
#define SCM_SAL_EOFP(o)     SCM_SAL_EQ((o), SCM_SAL_EOF)
#define SCM_SAL_UNDEFP(o)   SCM_SAL_EQ((o), SCM_SAL_UNDEF)


/* ------------------------------------------------------------
 * Miscellaneous types; most refer to one ScmCell or less, or
 * otherwise uncommon enough to warrant the use of a pair to hold the
 * two (perhaps more) ScmObj references.
 */
#define SCM_PTAG_MISC       SCM_MAKE_PTAG(2)
#define SCM_MISCP(o)        SCM_PTAG_EQ((o), SCM_PTAG_MISC)
#define SCM_MISC_Y_GCBIT    SCM_GCBIT_MARKED
#define SCM_CELL_MISCP(o)   (SCM_GCBIT(SCM_Y(o)) == SCM_MISC_Y_GCBIT)

/* scmobj_y = ...CC|BBB|AA|G
 * G       = GC bit
 * A,G     = L1 Misc tag bits
 * A,B,G   = L2 Misc tag bits
 * A,B,C,G = L3 Misc tag bits
 * Note that misc tags include the GC bit (which is always 1).
 */
#define SCM_MTAG_OFFSET      SCM_GCBIT_OFFSET
#define SCM_MTAG_L1_WIDTH    (SCM_GCBIT_WIDTH + 2)
#define SCM_MTAG_L2_WIDTH    (SCM_MTAG_L1_WIDTH + 3)
#define SCM_MTAG_L3_WIDTH    (SCM_MTAG_L2_WIDTH + 2)
#define SCM_MTAG_WIDTH(lv)   (((lv) == 1) ? SCM_MTAG_L1_WIDTH : \
                              ((lv) == 2) ? SCM_MTAG_L2_WIDTH : \
                              SCM_MTAG_L3_WIDTH)
#define SCM_MTAG_L1_MASK     SCM_MAKE_MASK(SCM_MTAG_OFFSET, SCM_MTAG_L1_WIDTH)
#define SCM_MTAG_L2_MASK     SCM_MAKE_MASK(SCM_MTAG_OFFSET, SCM_MTAG_L2_WIDTH)
#define SCM_MTAG_L3_MASK     SCM_MAKE_MASK(SCM_MTAG_OFFSET, SCM_MTAG_L3_WIDTH)
#define SCM_MTAG_MASK(lv)    (((lv) == 1) ? SCM_MTAG_L1_MASK :  \
                              ((lv) == 2) ? SCM_MTAG_L2_MASK :  \
                              SCM_MTAG_L3_MASK)

#define SCM_MTAG(o, lv)        (SCM_Y(o) & SCM_MTAG_MASK(lv))
#define SCM_MTAG_SET(o, lv, t) SCM_SET_Y((o), (SCM_Y(o) & ~SCM_MTAG_MASK(lv)) \
                                          | (t))
#define SCM_MTAG_EQ(o, lv, tc) (SCM_MTAG((o), (lv)) == (tc))

#define SCM_MAKE_MTAG_L1(t)                             \
    (((t) << (SCM_MTAG_OFFSET + SCM_GCBIT_WIDTH))       \
     | SCM_MISC_Y_GCBIT)
#define SCM_MAKE_MTAG_L2(t2, t1)                        \
    (((t2) << (SCM_MTAG_OFFSET + SCM_MTAG_L1_WIDTH))    \
     | SCM_MAKE_MTAG_L1(t1))
#define SCM_MAKE_MTAG_L3(t3, t2, t1)                    \
    (((t3) << (SCM_MTAG_OFFSET + SCM_MTAG_L2_WIDTH))    \
     | SCM_MAKE_MTAG_L2((t2), (t1)))

#define SCM_MISC_Y_OFFSET(lv)   (SCM_MTAG_WIDTH(lv) + SCM_MTAG_OFFSET)

/* A cell is always unmarked on access. */
#define SCM_MISC_X(o)        (SCM_X(o) & ~SCM_GCBIT_UNMARKED)
#define SCM_MISC_SET_X(o, x)                            \
    (SCM_ACCESS_ASSERT(!((x) & SCM_GCBIT_MASK)),        \
     SCM_SET_X((o), (x) | SCM_GCBIT_UNMARKED))

/* Getter and setter that keeps the mtag intact.  These are auxiliary;
 * don't use them directly. */
#define SCM_MISC_Y(o, lv)                    \
    (SCM_Y(o) >> SCM_MISC_Y_OFFSET(lv))
#define SCM_MISC_SET_Y(o, y, lv)                     \
    (SCM_SET_Y((o), ((y) << SCM_MISC_Y_OFFSET(lv))      \
               | (SCM_Y(o) & SCM_MTAG_MASK(lv))))

#define SCM_MISC_INIT(o, x, y, lv, tag)                 \
    (SCM_INIT((o),                                      \
              (x) | SCM_GCBIT_UNMARKED,                 \
              ((y) << SCM_MISC_Y_OFFSET(lv)) | (tag),   \
              SCM_PTAG_MISC))

/* Split X at B bits from LSB, store the upper half in obj_x, and
 * multiplex the remainder with obj_y.  It's tempting to use
 * SCM_MISC_Y() in these macros, but it appears that GCC can't
 * optimize the overhead away. */
#define SCM_MISC_SET_X_SPLITX_AT(o, x, lv, b)                                 \
    (SCM_SET_X((o), ((x) & ~SCM_MAKE_MASK(0, (b))) | SCM_GCBIT_UNMARKED),     \
     SCM_SET_Y((o),                                                           \
               (SCM_Y(o) & ~(SCM_MAKE_MASK(0, (b)) << SCM_MISC_Y_OFFSET(lv))) \
               | (((x) & SCM_MAKE_MASK(0, (b))) << SCM_MISC_Y_OFFSET(lv))))

#define SCM_MISC_SET_Y_SPLITX_AT(o, y, lv, b)           \
    SCM_SET_Y((o),                                      \
              (SCM_Y(o) & ((SCM_MTAG_MASK(lv) << (b))   \
                           | SCM_MAKE_MASK(0, (b))))    \
              | ((y) << (SCM_MISC_Y_OFFSET(lv) + (b))))

#define SCM_MISC_X_SPLITX_AT(o, lv, b)          \
    ((SCM_MISC_X(o) & ~SCM_GCBIT_UNMARKED)      \
     | (SCM_MISC_Y((o), (lv)) & (b)))

#define SCM_MISC_Y_SPLITX_AT(o, lv, b)       \
    (SCM_MISC_Y(o, lv) >> (b))

#define SCM_MISC_INIT_SPLITX_AT(o, x, y, lv, tag, b)            \
    SCM_MISC_INIT((o), (x) & ~SCM_MAKE_MASK(0, (b)),            \
                  ((y) << (b)) | ((x) & SCM_MAKE_MASK(0, (b))), \
                  (lv), (tag))


/* A convenient declarator for misc. subtypes.  This macro covertly
 * defines parameters for the macros defined below.
 *
 * name   - Name of the type in uppercase.  STRING, SYMBOL, etc.
 *
 * lv    - The level "invoked" with tag values.  L2(1, 3) for example.
 *
 * xtype  - The type to be stored in obj_x.  void*, ScmFuncType, etc.
 *
 * xalign - Base-2 logarithm of the minimum alignment guaranteed for
 *          values stored in x.  0 means not aligned.
 */
#define SCM_MISC_DECLARE_TYPE(name, lv, xtype, xalign)                       \
    enum {                                                                   \
        SCM_MISC_##name##_LV = SCM_MISC_LEVEL_##lv,                          \
        SCM_MISC_##name##_X_UNUSED_BITS                                      \
            = (SIZEOF_SCM_INTOBJ_T - sizeof(xtype)) * CHAR_BIT,              \
        SCM_MISC_##name##_XALIGN = (xalign),                                 \
        SCM_MISC_##name##_XSPILL = SCM_GCBIT_WIDTH - (xalign),               \
        SCM_MTAG_##name = SCM_MAKE_MTAG_##lv,                                \
        SCM_MISC_##name##_XDIRECTP = (SCM_MISC_##name##_XSPILL <= 0),        \
        SCM_MISC_##name##_XSHIFTP = (!SCM_MISC_##name##_XDIRECTP             \
                                     && (SCM_MISC_##name##_XSPILL            \
                                         < SCM_MISC_##name##_X_UNUSED_BITS)) \
    };                                                                       \
    typedef xtype SCM_MISC_##name##_XTYPE /* No semicolon here. */

#define SCM_MISC_LEVEL_L1(dummy)      1
#define SCM_MISC_LEVEL_L2(d1, d2)     2
#define SCM_MISC_LEVEL_L3(d1, d2, d3) 3

/* Dummies to make the declaration more verbose. */
#define SCM_MISC_XTYPE(t)       t
#define SCM_MISC_XALIGN(n)      n
#define SCM_MISC_XALIGN_SCMOBJ  SCM_GCBIT_WIDTH /* If storing ScmObj. */

/* Type-conscious dispatchers. */
#define SCM_MISC_XVAL_BY_PTR(o, typ)                    \
    (SCM_MISC_##typ##_XDIRECTP                          \
     ? SCM_MISC_X(o)                                    \
     : SCM_MISC_##typ##_XSHIFTP                         \
       ? SCM_MISC_X(o) >> SCM_MISC_##typ##_XSPILL       \
       : SCM_MISC_X_SPLITX_AT((o),                      \
                              SCM_MISC_##typ##_LV,      \
                              SCM_MISC_##typ##_XSPILL))

#define SCM_MISC_SET_XVAL_BY_PTR(o, x, typ)                     \
    do {                                                        \
        ScmCell *_p = (o);                                      \
        SCM_ACCESS_ASSERT(SCM_UNTAGGEDP(_p));                   \
        if (SCM_MISC_##typ##_XDIRECTP)                          \
            SCM_MISC_SET_X(_p, (x));                            \
        else if (SCM_MISC_##typ##_XSHIFTP)                      \
            SCM_MISC_SET_X(_p, (x) << SCM_MISC_##typ##_XSPILL); \
        else                                                    \
            SCM_MISC_SET_X_SPLITX_AT(_p, (x),                   \
                                     SCM_MISC_##typ##_LV,       \
                                     SCM_MISC_##typ##_XSPILL);  \
    } while (0)

#define SCM_MISC_YVAL_BY_PTR(o, typ)                    \
    (SCM_MISC_##typ##_XDIRECTP                          \
     ? SCM_MISC_Y((o), SCM_MISC_##typ##_LV)             \
     : SCM_MISC_##typ##_XSHIFTP                         \
       ? SCM_MISC_Y((o), SCM_MISC_##typ##_LV)           \
       : SCM_MISC_Y_SPLITX_AT((o),                      \
                              SCM_MISC_##typ##_LV,      \
                              SCM_MISC_##typ##_XSPILL))

#define SCM_MISC_SET_YVAL_BY_PTR(o, y, typ)                     \
    do {                                                        \
        ScmCell *_p = (o);                                      \
        SCM_ACCESS_ASSERT(SCM_UNTAGGEDP(_p));                   \
        if (SCM_MISC_##typ##_XDIRECTP)                          \
            SCM_MISC_SET_Y(_p, (y), SCM_MISC_##typ##_LV);       \
        else if (SCM_MISC_##typ##_XSHIFTP)                      \
            SCM_MISC_SET_Y(_p, (y),                             \
                           SCM_MISC_##typ##_LV);                \
        else                                                    \
            SCM_MISC_SET_Y_SPLITX_AT(_p, (y),                   \
                                     SCM_MISC_##typ##_LV,       \
                                     SCM_MISC_##typ##_XSPILL);  \
    } while (0)

#define SCM_MISC_XVAL(o, typ) SCM_MISC_XVAL_BY_PTR(SCM_##typ##_PTR(o), typ)
#define SCM_MISC_SET_XVAL(o, x, typ)                            \
    SCM_MISC_SET_XVAL_BY_PTR(SCM_##typ##_PTR(o), (x), typ)
#define SCM_MISC_YVAL(o, typ) SCM_MISC_YVAL_BY_PTR(SCM_##typ##_PTR(o), typ)
#define SCM_MISC_SET_YVAL(o, y, typ)                            \
    SCM_MISC_SET_YVAL_BY_PTR(SCM_##typ##_PTR(o), (y), typ)

#define SCM_MISC_ENTYPE(o, typ) (SCM_MTAG_SET(SCM_STRIPPED_PTR(o),      \
                                              SCM_MISC_##typ##_LV,      \
                                              SCM_MTAG_##typ),          \
                                 SCM_PTAG_SET((o), SCM_PTAG_MISC))

#define SCM_MISC_PTR(o, typ)    SCM_STRIPPED_PTR(SCM_AS_##typ(o))
#define SCM_MISC_TYPEP_BY_PTR(c, typ)                           \
    (SCM_MTAG_EQ((c), SCM_MISC_##typ##_LV, SCM_MTAG_##typ))
#define SCM_MISC_TYPEP(o, typ)                                          \
    (SCM_MISCP(o) && SCM_MISC_TYPEP_BY_PTR(SCM_STRIPPED_PTR(o), typ))


/* ------------------------------
 * And finally, the types....
 */
/* Symbols. */
SCM_MISC_DECLARE_TYPE(SYMBOL, L1(0), SCM_MISC_XTYPE(ScmObj),
                      SCM_MISC_XALIGN_SCMOBJ);

#define SCM_SYMBOL_PTR(o)              SCM_MISC_PTR((o), SYMBOL)
#define SCM_SAL_ENTYPE_SYMBOL(o)       SCM_MISC_ENTYPE((o), SYMBOL)
#define SCM_SAL_SYMBOLP(o)             SCM_MISC_TYPEP((o), SYMBOL)
#define SCM_SAL_SYMBOL_VCELL(o)        SCM_MISC_XVAL((o), SYMBOL)
#define SCM_SAL_SYMBOL_SET_VCELL(o, c) SCM_MISC_SET_XVAL((o), (c), SYMBOL)
#define SCM_SAL_SYMBOL_NAME(o)         ((char*)SCM_MISC_YVAL((o), SYMBOL))
#define SCM_SAL_SYMBOL_SET_NAME(o, n)  SCM_MISC_SET_YVAL((o), (ScmObj)(n), \
                                                         SYMBOL)
#define SCM_SAL_SYMBOL_INIT(o, c, n)   SCM_MISC_INIT((o), (c), (ScmObj)(n), \
                                                     SYMBOL)
#define SCM_CELL_SYMBOLP(c)            SCM_MISC_TYPEP_BY_PTR(&(c), SYMBOL)
#define SCM_CELL_SYMBOL_FIN(c)                                  \
    do {                                                        \
        char *_s = (char*)SCM_MISC_YVAL_BY_PTR(&(c), SYMBOL);   \
        if (_s) free(_s);                                       \
    } while (0)

/* Strings. */
SCM_MISC_DECLARE_TYPE(STRING, L1(1), SCM_MISC_XTYPE(char *),
                      SCM_MISC_XALIGN(1));

#define SCM_STRING_PTR(o)         SCM_MISC_PTR((o), STRING)
#define SCM_SAL_ENTYPE_STRING(o)  SCM_MISC_ENTYPE((o), STRING)
#define SCM_SAL_STRINGP(o)        SCM_MISC_TYPEP((o), STRING)
#define SCM_STR_MUTABLE_BIT       1
#define SCM_SAL_STRING_MUTABILITY(o)                    \
    (SCM_MISC_YVAL((o), STRING) & SCM_STR_MUTABLE_BIT)
#define SCM_SAL_STRING_MUTABLEP(o)              \
    SCM_SAL_STRING_MUTABILITY(o)
#define SCM_SAL_STRING_SET_MUTABLE(o)                   \
    SCM_MISC_SET_YVAL((o), SCM_MISC_YVAL((o), STRING)   \
                      | SCM_STR_MUTABLE_BIT, STRING)
#define SCM_SAL_STRING_SET_IMMUTABLE(o)                 \
    SCM_MISC_SET_YVAL((o), SCM_MISC_YVAL((o), STRING)   \
                      & ~SCM_STR_MUTABLE_BIT, STRING)
#define SCM_SAL_STRING_STR(o)        ((char*)SCM_MISC_XVAL(o))
#define SCM_SAL_STRING_LEN(o)        ((scm_int_t)(SCM_MISC_YVAL(o, STRING)>>1))
#define SCM_SAL_STRING_SET_STR(o, s) SCM_SET_XVAL((o), (ScmObj)(s))
#define SCM_SAL_STRING_SET_LEN(o, l)                                    \
    SCM_MISC_SET_YVAL((o), (l) | SCM_SAL_STRING_MUTABILITY(o), STRING)
#define SCM_SAL_MUTABLE_STRING_INIT(o, s, l)                    \
    SCM_MISC_INIT((o), (ScmObj)(s),                             \
                  ((ScmObj)(l) << 1) | SCM_STRING_MUTABLE_BIT,  \
                  STRING)
#define SCM_CELL_STRINGP(c)      SCM_MISC_TYPEP_BY_PTR(&(c), STRING)
#define SCM_CELL_STRING_FIN(c)                                 \
    do {                                                       \
        char *_s = (char*)SCM_MISC_XVAL_BY_PTR(&(c), STRING);  \
        if (_s) free (_s);                                     \
    } while (0)


/* TODO */

/* Builtin functions. */
SCM_MISC_DECLARE_TYPE(FUNC, L2(1, 3), SCM_MISC_XTYPE(ScmFuncType),
                      SCM_MISC_XALIGN(0));

#define SCM_FUNC_PTR(o)          SCM_MISC_PTR((o), FUNC)
#define SCM_SAL_FUNCP(o)         SCM_MISC_TYPEP((o), FUNC)
#define SCM_SAL_ENTYPE_FUNC(o)   SCM_MISC_ENTYPE((o), FUNC)
#define SCM_SAL_FUNC_CFUNC(o)    ((ScmFuncType)SCM_MISC_XVAL((o), FUNC))
#define SCM_SAL_FUNC_TYPECODE(o)                        \
    ((enum ScmFuncTypeCode)(SCM_MISC_YVAL((o), FUNC)))
#define SCM_SAL_FUNC_SET_CFUNC(o, f)            \
    SCM_MISC_SET_XVAL((o), (ScmObj)(f), FUNC)
#define SCM_SAL_FUNC_SET_TYPECODE(o, t)                 \
    SCM_MISC_SET_YVAL((o), (ScmObj)(t), FUNC)
#define SCM_SAL_FUNC_INIT(o, f, t) SCM_MISC_INIT((o), (ScmObj)(f),      \
                                                 (ScmObj)(t), FUNC)

/* TODO */

/* Each argument must be an untagged pointer to a cell. */
#define SCM_MTAG_FREECELL         SCM_MAKE_MTAG_L2(7, 3)
#define SCM_SAL_FREECELL_NEXT(o)  SCM_X(o)
#define SCM_SAL_FREECELLP(o)      (SCM_Y(o) == SCM_MTAG_FREECELL)
#define SCM_SAL_RECLAIM_CELL(o, next)                           \
    (SCM_SET_X((o), (next)), SCM_SET_Y((o), SCM_MTAG_FREECELL))



/*===========================================================================
  Predefined Symbols
===========================================================================*/
/* for list construction */
#define SCM_SAL_SYM_QUOTE            scm_sym_quote
#define SCM_SAL_SYM_QUASIQUOTE       scm_sym_quasiquote
#define SCM_SAL_SYM_UNQUOTE          scm_sym_unquote
#define SCM_SAL_SYM_UNQUOTE_SPLICING scm_sym_unquote_splicing

/* syntax.c */
SCM_GLOBAL_VARS_BEGIN(syntax);
ScmObj scm_sym_quote, scm_sym_quasiquote;
ScmObj scm_sym_unquote, scm_sym_unquote_splicing;
SCM_GLOBAL_VARS_END(syntax);
#define scm_sym_quote            SCM_GLOBAL_VAR(syntax, scm_sym_quote)
#define scm_sym_quasiquote       SCM_GLOBAL_VAR(syntax, scm_sym_quasiquote)
#define scm_sym_unquote          SCM_GLOBAL_VAR(syntax, scm_sym_unquote)
#define scm_sym_unquote_splicing SCM_GLOBAL_VAR(syntax, scm_sym_unquote_splicing)
SCM_DECLARE_EXPORTED_VARS(syntax);

#ifdef __cplusplus
}
#endif

#endif /* __STORAGE_COMPACT_H */