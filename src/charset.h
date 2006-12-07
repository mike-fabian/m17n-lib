/* charset.h -- header file for the charset module.
   Copyright (C) 2003, 2004
     National Institute of Advanced Industrial Science and Technology (AIST)
     Registration Number H15PRO112

   This file is part of the m17n library.

   The m17n library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   The m17n library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the m17n library; if not, write to the Free
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   02111-1307, USA.  */

#ifndef _M17N_CHARSET_H_
#define _M17N_CHARSET_H_

/** @file charset.h
    @brief Header for charset handlers.
*/

enum mcharset_method
  {
    MCHARSET_METHOD_OFFSET,
    MCHARSET_METHOD_MAP,
    MCHARSET_METHOD_DEFERRED,
    MCHARSET_METHOD_SUBSET,
    MCHARSET_METHOD_SUPERSET,
    MCHARSET_METHOD_MAX
  };

/** Structure for charset.  */

typedef struct MCharset MCharset;

struct MCharset
{
  /** The value is always 0 because all charsets are static.  */
  unsigned ref_count;

  /** Symbol indicating the name of the charset.  */
  MSymbol name;

  /** Number of dimensions of the charset.  It must be 1, 2, 3, or
      4.  */
  int dimension;

  /** Byte code range of each dimension.  <code_range>[4N] is a
      minimum byte code of the (N+1)th dimension, <code_range>[4N+1]
      is a maximum byte code of the (N+1)th dimension,
      <code_range>[4N+2] is (<code_range>[4N+1] - <code_range>[4N] +
      1), <code_range>[4N+3] is a number of characters contained in the
      first to (N+1)th dimensions.  We get "char-index" of a
      "code-point" from this information.  */
  int code_range[16];

  /** The minimum code-point calculated from <code_range>.  It may be
      smaller than <min_code>.  */
  int code_range_min_code;

  /** Nonzero means there is no gap in code points of the charset.  If
      <dimension> is 1, <no_code_gap> is always 1.  Otherwise,
      <no_code_gap> is 1 iff <code_range>[4N] is zero and
      <code_range>[4N+1] is 256 for N = 0..<dimension>-2.  If
      <no_code_gap> is nonzero, "char-index" is "code-point" -
      <min_code>.  */
  int no_code_gap;

  /** If the byte code B is valid in the (N+1)th dimension,
      (<code_range_mask>[B] & (1 << N)) is 1.  Otherwise,
      (<code_range_mask>[B] & (1 << N)) is 0.  */
  unsigned char code_range_mask[256];

  /** Minimum and maximum code-point of the charset.  */
  unsigned min_code, max_code;

  /** Nonzero means the charset encodes ASCII characters as is.  */
  int ascii_compatible;

  /** Minimum and maximum character of the charset.  If
      <ascii_compatible> is nonzero, <min_char> is actually the
      minimum non-ASCII character of the charset.  */
  int min_char, max_char;

  /** ISO 2022 final byte of the charset.  It must be in the range
      48..127, or -1.  The value -1 means that the charset is not
      encodable by ISO 2022 based coding systems.  */
  int final_byte;

  /** ISO 2022 revision number of the charset, or -1.  The value -1
      means that the charset has no revision number.  Used only when
      <final_byte> is not -1.  */
  int revision;

  /** Specify how to encode/decode code-point of the charset.  It must
      be Moffset, Mmap, Munify, Msubset, or Msuperset.  */
  MSymbol method;

  /** Array of integers to decode a code-point of the charset.  It is
      indexed by a "char-index" of the code-point, and the
      corresponding element is a character of the charset, or -1 if
      the code point is not valid in the charset.  Used only when
      <method> is Mmap or Munify.  */
  int *decoder;

  /** Char-table to encode a character of the charset.  It is indexed
      by a character code, and the corresponding element is a code
      point of the character in the charset, or
      MCHAR_INVALID_CODE if the character is not included in the
      charset.  Used only when <method> is Mmap or Munify.  */
  MCharTable *encoder;

  int unified_max;

  /** Array of pointers to parent charsets.  Used only when <method>
      is Msubset or Msuperset.  Atmost 8 parents are supported.  */
  MCharset *parents[8];

  /* Number of parent charsets.  */
  int nparents;

  unsigned subset_min_code, subset_max_code;
  int subset_offset;

  int simple;

  /** If the charset is fully loaded (i.e. all the above member are
      set to correct values), the value is 1.  Otherwise, the value is
      0.  */
  int fully_loaded;
};

extern MPlist *mcharset__cache;

/** Return a charset associated with the symbol CHARSET_SYM.  */

#define MCHARSET(charset_sym)					\
  (((charset_sym) == MPLIST_KEY (mcharset__cache)		\
    || (MPLIST_KEY (mcharset__cache) = (charset_sym),		\
	MPLIST_VAL (mcharset__cache)				\
	= (MCharset *) msymbol_get ((charset_sym), Mcharset)))	\
   ? MPLIST_VAL (mcharset__cache)				\
   : mcharset__find (charset_sym))


/** Return index of a character whose code-point in CHARSET is CODE.
    If CODE is not valid, return -1.  */

#define CODE_POINT_TO_INDEX(charset, code)				\
  ((charset)->no_code_gap						\
   ? (code) - (charset)->min_code					\
   : (((charset)->code_range_mask[(code) >> 24] & 0x8)			\
      && ((charset)->code_range_mask[((code) >> 16) & 0xFF] & 0x4)	\
      && ((charset)->code_range_mask[((code) >> 8) & 0xFF] & 0x2)	\
      && ((charset)->code_range_mask[(code) & 0xFF] & 0x1))		\
   ? (((((code) >> 24) - (charset)->code_range[12])			\
       * (charset)->code_range[11])					\
      + (((((code) >> 16) & 0xFF) - (charset)->code_range[8])		\
	 * (charset)->code_range[7])					\
      + (((((code) >> 8) & 0xFF) - (charset)->code_range[4])		\
	 * (charset)->code_range[3])					\
      + (((code) & 0xFF) - (charset)->code_range[0])			\
      - ((charset)->min_code - (charset)->code_range_min_code))		\
   : -1)


/* Return code-point of a character whose index is IDX.  
   The validness of IDX is not checked.  IDX may be modified.  */

#define INDEX_TO_CODE_POINT(charset, idx)				     \
  ((charset)->no_code_gap						     \
   ? (idx) + (charset)->min_code					     \
   : (idx += (charset)->min_code - (charset)->code_range_min_code,	     \
      (((charset)->code_range[0] + (idx) % (charset)->code_range[2])	     \
       | (((charset)->code_range[4]					     \
	   + ((idx) / (charset)->code_range[3] % (charset)->code_range[6]))  \
	  << 8)								     \
       | (((charset)->code_range[8]					     \
	   + ((idx) / (charset)->code_range[7] % (charset)->code_range[10])) \
	  << 16)							     \
       | (((charset)->code_range[12] + ((idx) / (charset)->code_range[11]))  \
	  << 24))))


/** Return a character whose code-point in CHARSET is CODE.  If CODE
    is invalid, return -1.  */

#define DECODE_CHAR(charset, code)					\
  (((code) < 128 && (charset)->ascii_compatible)			\
   ? (int) (code)							\
   : ((code) < (charset)->min_code || (code) > (charset)->max_code)	\
   ? -1									\
   : ! (charset)->simple						\
   ? mcharset__decode_char ((charset), (code))				\
   : (charset)->method == Moffset					\
   ? (code) - (charset)->min_code + (charset)->min_char			\
   : (charset)->decoder[(code) - (charset)->min_code])


/** Return a code-point in CHARSET for character C.  If CHARSET
    does not contain C, return MCHAR_INVALID_CODE.  */

#define ENCODE_CHAR(charset, c)					\
  (! (charset)->simple						\
   ? mcharset__encode_char ((charset), (c))			\
   : ((c) < (charset)->min_char || (c) > (charset)->max_char)	\
   ? MCHAR_INVALID_CODE						\
   : (charset)->method == Moffset				\
   ? (c) - (charset)->min_char + (charset)->min_code		\
   : (unsigned) mchartable_lookup ((charset)->encoder, (c)))


extern MCharset *mcharset__ascii;
extern MCharset *mcharset__binary;
extern MCharset *mcharset__m17n;
extern MCharset *mcharset__unicode;

#define ISO_MAX_DIMENSION 3
#define ISO_MAX_CHARS 2
#define ISO_MAX_FINAL 0x80	/* only 0x30..0xFF are used */

typedef struct
{
  /* Table of ISO-2022 charsets.  */
  int size, inc, used;
  MCharset **charsets;

  /** A 3-dimensional table indexed by "dimension", "chars", and
      "final byte" of an ISO-2022 charset to get the correponding
      charset.  A charset that has a revision number is not stored in
      this table.  */
  MCharset *classified[ISO_MAX_DIMENSION][ISO_MAX_CHARS][ISO_MAX_FINAL];
} MCharsetISO2022Table;

extern MCharsetISO2022Table mcharset__iso_2022_table;

#define MCHARSET_ISO_2022(dim, chars, final) \
  mcharset__iso_2022_table.classified[(dim) - 1][(chars) == 96][(final)]

extern MCharset *mcharset__find (MSymbol name);
extern int mcharset__decode_char (MCharset *charset, unsigned code);
extern unsigned mcharset__encode_char (MCharset *charset, int c);
extern int mcharset__load_from_database ();

#endif /* _M17N_CHARSET_H_ */
