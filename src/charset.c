/* charset.c -- charset module.
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
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.  */
/***en
    @addtogroup m17nCharset
    @brief Charset objects and API for them.

    The m17n library uses @e charset objects to represent a coded
    character sets (CCS).  The m17n library supports many predefined
    coded character sets.  Moreover, application programs can add
    other charsets.  A character can belong to multiple charsets.

    The m17n library distinguishes the following three concepts:

    @li A @e code-point is a number assigned by the CCS to each
    character.  Code-points may or may not be continuous.  The type
    @c unsigned is used to represent a code-point.  An invalid
    code-point is represented by the macro @c MCHAR_INVALID_CODE.

    @li A @e character @e index is the canonical index of a character
    in a CCS.  The character that has the character index N occupies
    the Nth position when all the characters in the current CCS are
    sorted by their code-points.  Character indices in a CCS are
    continuous and start with 0.

    @li A @e character @e code is the internal representation in the
    m17n library of a character.  A character code is a signed integer
    of 21 bits or longer.

    Each charset object defines how characters are converted between
    code-points and character codes.  To @e encode means converting
    code-points to character codes and to @e decode means converting
    character codes to code-points.  */

/***ja
    @addtogroup m17nCharset
    @brief 文字セットオブジェクトとそれに関する API.

    m17n ライブラリは、符号化文字集合 (CCS) を @e 文字セット 
    と呼ぶオブジェクトで表現する。
    m17n ライブラリは多くの符号化文字集合をあらかじめサポートしているし、アプリケーションプログラムが独自に文字セットを追加することも可能である。
    一つの文字は複数の文字セットに属してもよい。

    m17n ライブラリは、以下の概念を区別している:

    @li @e コードポイント とは、CCS がその中の個々の文字に対して定義する数値である。
    コードポイントは連続しているとは限らない。コードポイントは
    @c unsigned 型によって表される。無効なコードポイントはマクロ
    @c MCHAR_INVALID_CODE で表される。

    @li @e 文字インデックス とは、CCS 内で各文字に割り当てられる正規化されたインデックスである。
    文字インデックスが N の文字は、CCS 中の全文字をコードポイント順に並べたときに N 番目に現われる。
    CCS 中の文字インデックスは連続しており、0 から始まる。

    @li @e 文字コード とは、m17n ライブラリ内における文字の内部表現であり、21 ビット以上の長さを持つ符合付き整数である。

    各文字セットオブジェクトは、その文字セットに属する文字のコードポイントと文字コードとの間の変換を規定する。
    コードポイントから文字コードへの変換を @e デコード
    と呼び、文字コードからコードポイントへの変換を @e エンコード と呼ぶ。  */

/*=*/
#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "symbol.h"
#include "charset.h"
#include "coding.h"
#include "chartab.h"
#include "plist.h"

static int unified_max;

/** List of all charsets ever defined.  */

struct MCharsetList
{
  int size, inc, used;
  MCharset **charsets;
};

static struct MCharsetList charset_list;

static MPlist *charset_definition_list;

/** Make a charset object from the template of MCharset structure
    CHARSET, and return a pointer to the new charset object.
    CHARSET->code_range[4N + 2] and CHARSET->code_range[4N + 3] are
    not yet set.  */

static MCharset *
make_charset (MCharset *charset)
{
  unsigned min_code, max_code;
  int i, n;
  int *range = charset->code_range;

  if (charset->dimension < 1 || charset->dimension > 4)
    MERROR (MERROR_CHARSET, NULL);
  if ((charset->final_byte > 0 && charset->final_byte < '0')
      || charset->final_byte > 127)
    MERROR (MERROR_CHARSET, NULL);

  for (i = 0, n = 1; i < 4; i++)
    {
      if (range[i * 4] > range[i * 4 + 1])
	MERROR (MERROR_CHARSET, NULL);
      range[i * 4 + 2] = range[i * 4 + 1] - range[i * 4] + 1;
      n *= range[i * 4 + 2];
      range[i * 4 + 3] = n;
    }

  min_code = range[0] | (range[4] << 8) | (range[8] << 16) | (range[12] << 24);
  if (charset->min_code == 0)
    charset->min_code = min_code;
  else if (charset->min_code < min_code)
    MERROR (MERROR_CHARSET, NULL);
  max_code = range[1] | (range[5] << 8) | (range[9] << 16) | (range[13] << 24);
  if (charset->max_code == 0)  
    charset->max_code = max_code;
  else if (charset->max_code > max_code)
    MERROR (MERROR_CHARSET, NULL);

  charset->code_range_min_code = min_code;
  charset->fully_loaded = 0;
  charset->simple = 0;

  if (charset->method == Msubset)
    {
      MCharset *parent;

      if (charset->nparents != 1)
	MERROR (MERROR_CHARSET, NULL);
      parent = charset->parents[0];
      if (parent->method == Msuperset
	  || charset->min_code - charset->subset_offset < parent->min_code
	  || charset->max_code - charset->subset_offset > parent->max_code)
	MERROR (MERROR_CHARSET, NULL);
    }
  else if (charset->method == Msuperset)
    {
      if (charset->nparents < 2)
	MERROR (MERROR_CHARSET, NULL);
      for (i = 0; i < charset->nparents; i++)
	if (charset->min_code > charset->parents[i]->min_code
	    || charset->max_code < charset->parents[i]->max_code)
	  MERROR (MERROR_CHARSET, NULL);
    }
  else
    {
      charset->no_code_gap
	= (charset->dimension == 1
	   || (range[2] == 256
	       && (charset->dimension == 2
		   || (range[6] == 256
		       && (charset->dimension == 3
			   || range[10] == 256)))));

      if (! charset->no_code_gap)
	{
	  int j;

	  memset (charset->code_range_mask, 0,
		  sizeof charset->code_range_mask);
	  for (i = 0; i < 4; i++)
	    for (j = range[i * 4]; j <= range[i * 4 + 1]; j++)
	      charset->code_range_mask[j] |= (1 << i);
	}

      if (charset->method == Moffset)
	{
	  charset->max_char = charset->min_char + range[15] - 1;
	  if (charset->min_char < 0
	      || charset->max_char < 0 || charset->max_char > unified_max)
	    MERROR (MERROR_CHARSET, NULL);
	  charset->simple = charset->no_code_gap;
	  charset->fully_loaded = 1;
	}
      else if (charset->method == Munify)
	{
	  /* The magic number 12 below is to align to the SUB_BITS_2
	     (defined in chartab.c) boundary in a char-table.  */
	  unified_max -= ((range[15] >> 12) + 1) << 12;
	  charset->unified_max = unified_max;
	}
      else if (charset->method != Mmap)
	MERROR (MERROR_CHARSET, NULL);
    }

  MLIST_APPEND1 (&charset_list, charsets, charset, MERROR_CHARSET);

  if (charset->final_byte > 0)
    {
      MLIST_APPEND1 (&mcharset__iso_2022_table, charsets, charset,
		     MERROR_CHARSET);
      if (charset->revision <= 0)
	{
	  int chars = range[2];

	  if (chars == 128)	/* ASCII case */
	    chars = 94;
	  else if (chars == 256) /* ISO-8859-X case */
	    chars = 96;
	  MCHARSET_ISO_2022 (charset->dimension, chars, charset->final_byte)
	    = charset;
	}
    }

  return charset;
}

static int
load_charset_fully (MCharset *charset)
{
  if (charset->method == Msubset)
    {
      MCharset *parent = charset->parents[0];

      if (! parent->fully_loaded
	  && load_charset_fully (parent) < 0)
	MERROR (MERROR_CHARSET, -1);
      if (parent->method == Moffset)
	{
	  unsigned code;

	  code = charset->min_code - charset->subset_offset;
	  charset->min_char = DECODE_CHAR (parent, code);
	  code = charset->max_code - charset->subset_offset;
	  charset->max_char = DECODE_CHAR (parent, code);
	}
      else
	{
	  unsigned min_code = charset->min_code - charset->subset_offset;
	  unsigned max_code = charset->max_code - charset->subset_offset;
	  int min_char = DECODE_CHAR (parent, min_code);
	  int max_char = min_char;
	  
	  for (++min_code; min_code <= max_code; min_code++)
	    {
	      int c = DECODE_CHAR (parent, min_code);

	      if (c >= 0)
		{
		  if (c < min_char)
		    min_char = c;
		  else if (c > max_char)
		    max_char = c;
		}
	    }
	  charset->min_char = min_char;
	  charset->max_char = max_char;
	}
    }
  else if (charset->method == Msuperset)
    {
      int min_char = 0, max_char = 0;
      int i;

      for (i = 0; i < charset->nparents; i++)
	{
	  MCharset *parent = charset->parents[i];

	  if (! parent->fully_loaded
	      && load_charset_fully (parent) < 0)
	    MERROR (MERROR_CHARSET, -1);
	  if (i == 0)
	    min_char = parent->min_char, max_char = parent->max_char;
	  else if (parent->min_char < min_char)
	    min_char = parent->min_char;
	  else if (parent->max_char > max_char)
	    max_char = parent->max_char;
	}
      charset->min_char = min_char;
      charset->max_char = max_char;
    }
  else 				/* charset->method is Mmap or Munify */
    {
      MDatabase *mdb = mdatabase_find (Mcharset, charset->name, Mnil, Mnil);
      MPlist *plist;

      if (! mdb || ! (plist = mdatabase_load (mdb)))
	MERROR (MERROR_CHARSET, -1);
      charset->decoder = mplist_value (plist);
      charset->encoder = mplist_value (mplist_next (plist));
      M17N_OBJECT_UNREF (plist);
      mchartable_range (charset->encoder,
			&charset->min_char, &charset->max_char);
      if (charset->method == Mmap)
	charset->simple = charset->no_code_gap;
      else
	charset->max_char = charset->unified_max + 1 + charset->code_range[15];
    }

  charset->fully_loaded = 1;
  return 0;
}


/* Internal API */

MPlist *mcharset__cache;

/* Predefined charsets.  */
MCharset *mcharset__ascii;
MCharset *mcharset__binary;
MCharset *mcharset__m17n;
MCharset *mcharset__unicode;

MCharsetISO2022Table mcharset__iso_2022_table;

/** Initialize charset handler.  */

int
mcharset__init ()
{
  MPlist *param, *pl;

  unified_max = MCHAR_MAX;

  mcharset__cache = mplist ();
  mplist_set (mcharset__cache, Mt, NULL);

  MLIST_INIT1 (&charset_list, charsets, 128);
  MLIST_INIT1 (&mcharset__iso_2022_table, charsets, 128);
  charset_definition_list = mplist ();

  memset (mcharset__iso_2022_table.classified, 0,
	  sizeof (mcharset__iso_2022_table.classified));

  Mcharset = msymbol ("charset");

  Mmethod = msymbol ("method");
  Moffset = msymbol ("offset");
  Mmap = msymbol ("map");
  Munify = msymbol ("unify");
  Msubset = msymbol ("subset");
  Msuperset = msymbol ("superset");

  Mdimension = msymbol ("dimension");
  Mmin_range = msymbol ("min-range");
  Mmax_range = msymbol ("max-range");
  Mmin_code = msymbol ("min-code");
  Mmax_code = msymbol ("max-code");
  Mascii_compatible = msymbol ("ascii-compatible");
  Mfinal_byte = msymbol ("final-byte");
  Mrevision = msymbol ("revision");
  Mmin_char = msymbol ("min-char");
  Mmapfile = msymbol_as_managing_key ("mapfile");
  Mparents = msymbol_as_managing_key ("parents");
  Msubset_offset = msymbol ("subset-offset");
  Mdefine_coding = msymbol ("define-coding");
  Maliases = msymbol_as_managing_key ("aliases");

  param = mplist ();
  pl = param;
  /* Setup predefined charsets.  */
  pl = mplist_add (pl, Mmethod, Moffset);
  pl = mplist_add (pl, Mmin_range, (void *) 0);
  pl = mplist_add (pl, Mmax_range, (void *) 0x7F);
  pl = mplist_add (pl, Mascii_compatible, Mt);
  pl = mplist_add (pl, Mfinal_byte, (void *) 'B');
  pl = mplist_add (pl, Mmin_char, (void *) 0);
  Mcharset_ascii = mchar_define_charset ("ascii", param);

  mplist_put (param, Mmax_range, (void *) 0xFF);
  mplist_put (param, Mfinal_byte, NULL);
  Mcharset_iso_8859_1 = mchar_define_charset ("iso-8859-1", param);

  mplist_put (param, Mmax_range, (void *) 0x10FFFF);
  Mcharset_unicode = mchar_define_charset ("unicode", param);

  mplist_put (param, Mmax_range, (void *) MCHAR_MAX);
  Mcharset_m17n = mchar_define_charset ("m17n", param);

  mplist_put (param, Mmax_range, (void *) 0xFF);
  Mcharset_binary = mchar_define_charset ("binary", param);

  M17N_OBJECT_UNREF (param);

  mcharset__ascii = MCHARSET (Mcharset_ascii);
  mcharset__binary = MCHARSET (Mcharset_binary);
  mcharset__m17n = MCHARSET (Mcharset_m17n);
  mcharset__unicode = MCHARSET (Mcharset_unicode);

  return 0;
}

void
mcharset__fini (void)
{
  int i;
  MPlist *plist;

  for (i = 0; i < charset_list.used; i++)
    {
      MCharset *charset = charset_list.charsets[i];

      if (charset->decoder)
	free (charset->decoder);
      if (charset->encoder)
	M17N_OBJECT_UNREF (charset->encoder);
      free (charset);
    }
  M17N_OBJECT_UNREF (mcharset__cache);
  MLIST_FREE1 (&charset_list, charsets);
  MLIST_FREE1 (&mcharset__iso_2022_table, charsets);
  MPLIST_DO (plist, charset_definition_list)
    M17N_OBJECT_UNREF (MPLIST_VAL (plist));
  M17N_OBJECT_UNREF (charset_definition_list);
}


MCharset *
mcharset__find (MSymbol name)
{
  MCharset *charset;

  charset = msymbol_get (name, Mcharset);
  if (! charset)
    {
      MPlist *param = mplist_get (charset_definition_list, name);

      MPLIST_KEY (mcharset__cache) = Mt;
      if (! param)
	return NULL;
      param = mplist__from_plist (param);
      mchar_define_charset (MSYMBOL_NAME (name), param);
      charset = msymbol_get (name, Mcharset);
      M17N_OBJECT_UNREF (param);
    }
  MPLIST_KEY (mcharset__cache) = name;
  MPLIST_VAL (mcharset__cache) = charset;
  return charset;
}


/** Return the character corresponding to code-point CODE in CHARSET.
    If CODE is invalid for CHARSET, return -1.  */

int
mcharset__decode_char (MCharset *charset, unsigned code)
{
  int idx;

  if (code < 128 && charset->ascii_compatible)
    return (int) code;
  if (code < charset->min_code || code > charset->max_code)
    return -1;

  if (! charset->fully_loaded
      && load_charset_fully (charset) < 0)
    MERROR (MERROR_CHARSET, -1);

  if (charset->method == Msubset)
    {
      MCharset *parent = charset->parents[0];

      code -= charset->subset_offset;
      return DECODE_CHAR (parent, code);
    }

  if (charset->method == Msuperset)
    {
      int i;

      for (i = 0; i < charset->nparents; i++)
	{
	  MCharset *parent = charset->parents[i];
	  int c = DECODE_CHAR (parent, code);

	  if (c >= 0)
	    return c;
	}
      return -1;
    }

  idx = CODE_POINT_TO_INDEX (charset, code);
  if (idx < 0)
    return -1;

  if (charset->method == Mmap)
    return charset->decoder[idx];

  if (charset->method == Munify)
    {
      int c = charset->decoder[idx];

      if (c < 0)
	c = charset->unified_max + 1 + idx;
      return c;
    }

  /* Now charset->method should be Moffset.  */
  return (charset->min_char + idx);
}


/** Return the code point of character C in CHARSET.  If CHARSET does not
    contain C, return MCHAR_INVALID_CODE.  */

unsigned
mcharset__encode_char (MCharset *charset, int c)
{
  if (! charset->fully_loaded
      && load_charset_fully (charset) < 0)
    MERROR (MERROR_CHARSET, MCHAR_INVALID_CODE);

  if (charset->method == Msubset)
    {
      MCharset *parent = charset->parents[0];
      unsigned code = ENCODE_CHAR (parent, c);

      if (code == MCHAR_INVALID_CODE)
	return code;
      code += charset->subset_offset;
      if (code >= charset->min_code && code <= charset->max_code)
	return code;
      return MCHAR_INVALID_CODE;
    }

  if (charset->method == Msuperset)
    {
      int i;

      for (i = 0; i < charset->nparents; i++)
	{
	  MCharset *parent = charset->parents[i];
	  unsigned code = ENCODE_CHAR (parent, c);

	  if (code != MCHAR_INVALID_CODE)
	    return code;
	}
      return MCHAR_INVALID_CODE;
    }

  if (c < charset->min_char || c > charset->max_char)
    return MCHAR_INVALID_CODE;

  if (charset->method == Mmap)
    return (unsigned) mchartable_lookup (charset->encoder, c);

  if (charset->method == Munify)
    {
      if (c > charset->unified_max)
	{
	  c -= charset->unified_max - 1;
	  return INDEX_TO_CODE_POINT (charset, c);
	}
      return (unsigned) mchartable_lookup (charset->encoder, c);
    }

  /* Now charset->method should be Moffset */
  c -= charset->min_char;
  return INDEX_TO_CODE_POINT (charset, c);
}

int
mcharset__load_from_database ()
{
  MDatabase *mdb = mdatabase_find (msymbol ("charset-list"), Mnil, Mnil, Mnil);
  MPlist *def_list, *plist;
  MPlist *definitions = charset_definition_list;
  int mdebug_mask = MDEBUG_CHARSET;

  if (! mdb)
    return 0;
  MDEBUG_PUSH_TIME ();
  def_list = (MPlist *) mdatabase_load (mdb);
  MDEBUG_PRINT_TIME ("CHARSET", (stderr, " to load data."));
  MDEBUG_POP_TIME ();
  if (! def_list)
    return -1;

  MDEBUG_PUSH_TIME ();
  MPLIST_DO (plist, def_list)
    {
      MPlist *pl, *p;
      MSymbol name;

      if (! MPLIST_PLIST_P (plist))
	MERROR (MERROR_CHARSET, -1);
      pl = MPLIST_PLIST (plist);
      if (! MPLIST_SYMBOL_P (pl))
	MERROR (MERROR_CHARSET, -1);
      name = MPLIST_SYMBOL (pl);
      pl = MPLIST_NEXT (pl);
      definitions = mplist_add (definitions, name, pl);
      M17N_OBJECT_REF (pl);
      p = mplist__from_plist (pl);
      mchar_define_charset (MSYMBOL_NAME (name), p);
      M17N_OBJECT_UNREF (p);
    }

  M17N_OBJECT_UNREF (def_list);
  MDEBUG_PRINT_TIME ("CHARSET", (stderr, " to parse the loaded data."));
  MDEBUG_POP_TIME ();
  return 0;
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

/*** @addtogroup m17nCharset */
/*** @{ */
/*=*/

#ifdef FOR_DOXYGEN
/***en
    @brief Invalid code-point.

    The macro #MCHAR_INVALID_CODE gives the invalid code-point.  */

/***ja
    @brief 無効なコードポイント.

    マクロ #MCHAR_INVALID_CODE は無効なコードポイントを示す。  */

#define MCHAR_INVALID_CODE
#endif
/*=*/
/***en
    @brief The symbol @c Mcharset.

    Any decoded M-text has a text property whose key is the predefined
    symbol @c Mcharset.  The name of @c Mcharset is
    <tt>"charset"</tt>.  */

/***ja
    @brief シンボル @c Mcharset.

    デコードされた M-text は、キーが @c Mcharset
    であるようなテキストプロパティを持つ。
    シンボル @c Mcharset は <tt>"charset"</tt> という名前を持つ。  */

MSymbol Mcharset;
/*=*/

/***en
    @name Variables: Symbols representing a charset.

    Each of the following symbols represents a predefined charset.  */

/***ja
    @name 変数: 文字セットを表現する定義済みシンボル.

    以下の各シンボルは、定義済み文字セットを表現する。  */
/*=*/
/*** @{ */
/*=*/
/***en
    @brief Symbol representing the charset ASCII.

    The symbol #Mcharset_ascii has name <tt>"ascii"</tt> and represents
    the charset ISO 646, USA Version X3.4-1968 (ISO-IR-6).  */
/***ja
    @brief ASCII 文字セットを表現するシンボル.

    シンボル #Mcharset_ascii は <tt>"ascii"</tt> という名前を持ち、 
    ISO 646, USA Version X3.4-1968 (ISO-IR-6) 文字セットを表現する。
     */

MSymbol Mcharset_ascii;

/*=*/
/***en
    @brief Symbol representing the charset ISO/IEC 8859/1.

    The symbol #Mcharset_iso_8859_1 has name <tt>"iso-8859-1"</tt>
    and represents the charset ISO/IEC 8859-1:1998.  */
/***ja
    @brief ISO/IEC 8859-1:1998 文字セットを表現するシンボル.

    シンボル #Mcharset_iso_8859_1 は <tt>"iso-8859-1"</tt> 
    という名前を持ち、ISO/IEC 8859-1:1998 文字セットを表現する。
    */

MSymbol Mcharset_iso_8859_1;

/***en
    @brief Symbol representing the charset Unicode.

    The symbol #Mcharset_unicode has name <tt>"unicode"</tt> and
    represents the charset Unicode.  */
/***ja
    @brief Unicode 文字セットを表現するシンボル.

    シンボル #Mcharset_unicode は <tt>"unicode"</tt> 
    という名前を持ち、Unicode 文字セットを表現する。 */

MSymbol Mcharset_unicode;

/*=*/
/***en
    @brief Symbol representing the largest charset.

    The symbol #Mcharset_m17n has name <tt>"m17n"</tt> and
    represents the charset that contains all characters supported by
    the m17n library.  */ 
/***ja
    @brief 全文字を含む文字セットを表現するシンボル.

    シンボル #Mcharset_m17n は <tt>"m17n"</tt> という名前を持ち、
    m17n ライブラリが扱う全ての文字を含む文字セットを表現する。 */

MSymbol Mcharset_m17n;

/*=*/
/***en
    @brief Symbol representing the charset for ill-decoded characters.

    The symbol #Mcharset_binary has name <tt>"binary"</tt> and
    represents the fake charset which the decoding functions put to an
    M-text as a text property when they encounter an invalid byte
    (sequence).  

    See @ref m17nConv for more details.  */

/***ja
    @brief 正しくデコードできない文字の文字セットを表現するシンボル.

    シンボル #Mcharset_binary は <tt>"binary"</tt> 
    という名前を持ち、偽の (fake) 文字セットを表現する。
    デコード関数は、M-text のテキストプロパティとして、無効なバイト（シークエンス）に遭遇した位置を付加する。

     詳細は @ref m17nConv 参照のこと。 */

MSymbol Mcharset_binary;

/*=*/
/*** @} */
/*=*/

/***en
    @name Variables: Parameter keys for mchar_define_charset ().

    These are the predefined symbols to use as parameter keys for the
    function mchar_define_charset () (which see).  */

/***ja
    @name 変数: mchar_define_charset 用のパラメータ・キー

    これらは、関数 mchar_define_charset () 用のパラメータ・キーとして使われるシンボルである。 
    詳しくはこの関数の解説を参照のこと。*/
/*** @{ */
/*=*/

/***en
    Parameter key for mchar_define_charset () (which see). */ 

/***ja
    関数 mchar_define_charset () 用のパラメータ・キー. 
    詳しくはこの関数の解説を参照のこと。*/ 

MSymbol Mmethod;
MSymbol Mdimension;
MSymbol Mmin_range;
MSymbol Mmax_range;
MSymbol Mmin_code;
MSymbol Mmax_code;
MSymbol Mascii_compatible;
MSymbol Mfinal_byte;
MSymbol Mrevision;
MSymbol Mmin_char;
MSymbol Mmapfile;
MSymbol Mparents;
MSymbol Msubset_offset;
MSymbol Mdefine_coding;
MSymbol Maliases;
/*=*/
/*** @} */
/*=*/

/***en
    @name Variables: Symbols representing charset methods.

    These are the predefined symbols that can be a value of the
    #Mmethod parameter of a charset used in an argument to the
    mchar_define_charset () function.

    A method specifies how code-points and character codes are
    converted.  See the documentation of the mchar_define_charset ()
    function for the details.  */

/***ja
    @name 変数: 文字セットのメソッド指定に使われるシンボル

    これらは、文字セットの @e メソッド を指定するための定義済みシンボルであり、文字セットの
    #Mmethod パラメータの値となることができる。
    この値は関数 mchar_define_charset () の引数として使われる。

    メソッドとは、コードポイントと文字コードを相互変換する際の方式のことである。
    詳しくは関数 mchar_define_charset () の解説を参照のこと。  */
/*** @{ */
/*=*/
/***en
    @brief Symbol for the offset type method of charset.

    The symbol #Moffset has the name <tt>"offset"</tt> and, when used
    as a value of #Mmethod parameter of a charset, it means that the
    conversion of code-points and character codes of the charset is
    done by this calculation:

@verbatim
CHARACTER-CODE = CODE-POINT - MIN-CODE + MIN-CHAR
@endverbatim

    where, MIN-CODE is a value of #Mmin_code parameter of the charset,
    and MIN-CHAR is a value of #Mmin_char parameter.  */

/***ja
    @brief オフセット型のメソッドを示すシンボル.

    シンボル #Moffset は <tt>"offset"</tt> という名前を持ち、文字セットの
    #Mmethod パラメータの値として用いられた場合には、コードポイントと文字セットの文字コードの間の変換が以下の式に従って行われることを意味する。

@verbatim
文字コード = コードポイント - MIN-CODE + MIN-CHAR
@endverbatim

    ここで、MIN-CODE は文字セットの #Mmin_code パラメータの値であり、MIN-CHAR は
    #Mmin_char パラメータの値である。 */

MSymbol Moffset;
/*=*/

/***en @brief Symbol for the map type method of charset.

    The symbol #Mmap has the name <tt>"map"</tt> and, when used as a
    value of #Mmethod parameter of a charset, it means that the
    conversion of code-points and character codes of the charset is
    done by map looking up.  The map must be given by #Mmapfile
    parameter.  */

/***ja @brief マップ型のメソッドを示すシンボル.

    シンボル #Mmap は <tt>"map"</tt> という名前を持ち、文字セットの 
    #Mmethod パラメータの値として用いられた場合には、コードポイントと文字セットの文字コードの間の変換がマップを参照することによって行われることを意味する。
    マップは #Mmapfile パラメータとして与えなければならない。 */

MSymbol Mmap;
/*=*/

/***en @brief Symbol for the unify type method of charset.

    The symbol #Munify has the name <tt>"unify"</tt> and, when used as
    a value of #Mmethod parameter of a charset, it means that the
    conversion of code-points and character codes of the charset is
    done by map looking up and offsetting.  The map must be given by
    #Mmapfile parameter.  For this kind of charset, a unique
    continuous character code space for all characters is assigned.

    If the map has an entry for a code-point, the conversion is done
    by looking up the map.  Otherwise, the conversion is done by this
    calculation:

@verbatim
CHARACTER-CODE = CODE-POINT - MIN-CODE + LOWEST-CHAR-CODE
@endverbatim

    where, MIN-CODE is a value of #Mmin_code parameter of the charset,
    and LOWEST-CHAR-CODE is the lowest character code of the assigned
    code space.  */

/***ja @brief ユニファイ型のメソッドを示すシンボル.

    シンボル #Minherit は <tt>"unify"</tt> という名前を持ち、文字セットの 
    #Mmethod パラメータの値として用いられた場合には、コードポイントと文字セットの文字コードの間の変換が、マップの参照とオフセットの組み合わせによって行われることを意味する。
    マップは #Mmapfile パラメータとして与えなければならない。
    この種の各文字セットには、全文字に対して連続するコードスペースがそれぞれ割り当てられる。

    コードポイントがマップに含まれていれば、変換はマップ参照によって行われる。
    そうでなければ、以下の式に従う。

@verbatim
CHARACTER-CODE = CODE-POINT - MIN-CODE + LOWEST-CHAR-CODE
@endverbatim
    
    ここで、MIN-CODE は文字セットの #Mmin_code パラメータの値であり、
    LOWEST-CHAR-CODE は割り当てられたコードスペースの最も小さい文字コードである。
    */

MSymbol Munify;
/*=*/

/***en
    @brief Symbol for the subset type method of charset.

    The symbol #Msubset has the name <tt>"subset"</tt> and, when used
    as a value of #Mmethod parameter of a charset, it means that the
    charset is a subset of a parent charset.  The parent charset must
    be given by #Mparents parameter.  The conversion of code-points
    and character codes of the charset is done conceptually by this
    calculation:

@verbatim
CHARACTER-CODE = PARENT-CODE (CODE-POINT) + SUBSET-OFFSET
@endverbatim

    where, PARENT-CODE is a pseudo function that returns a character
    code of CODE-POINT in the parent charset, and SUBSET-OFFSET is a
    value given by #Msubset_offset parameter.  */

/***ja @brief サブセット型のメソッドを示すシンボル.

    シンボル #Msubset は <tt>"subset"</tt> という名前を持ち、文字セットの
    #Mmethod パラメータの値として用いられた場合には、この文字セットが別の文字セット（親文字セット）の部分集合であることを意味する。
    親文字セットは #Mparents パラメータによって与えられなくてはならない。
    コードポイントと文字セットの文字コードの間の変換は、概念的には以下の式に従う。

@verbatim
CHARACTER-CODE = PARENT-CODE (CODE-POINT) + SUBSET-OFFSET
@endverbatim

    ここで PARENT-CODE は CODE-POINT 
    の親文字セット中での文字コードを返す擬関数であり、SUBSET-OFFSET は 
    #Msubset_offset パラメータで与えられる値である。
    */

MSymbol Msubset;
/*=*/

/***en
    @brief Symbol for the superset type method of charset.

    The symbol #Msuperset has the name <tt>"superset"</tt> and, when
    used as a value of #Mmethod parameter of a charset, it means that
    the charset is a superset of parent charsets.  The parent charsets
    must be given by #Mparents parameter.  */

/***ja
    @brief スーパーセット型のメソッドを示すシンボル.

    シンボル #Msuperset は <tt>"superset"</tt> という名前を持ち、文字セットの
    #Mmethod パラメータの値として用いられた場合には、この文字セットが別の文字セット（親文字セット）の上位集合であることを意味する。
    親文字セットは #Mparents パラメータによって与えられなくてはならない。
    */

MSymbol Msuperset;
/*=*/
/*** @}  */ 

/***en
    @brief Define a charset.

    The mchar_define_charset () function defines a new charset and
    makes it accessible via a symbol whose name is $NAME.  $PLIST
    specifies parameters of the charset as below:

    <ul>

    <li> Key is #Mmethod, value is a symbol.

    The value specifies the method for decoding/encoding code-points
    in the charset.  It must be #Moffset, #Mmap (default), #Munify,
    #Msubset, or #Msuperset.

    <li> Key is #Mdimension, value is an integer

    The value specifies the dimension of code-points of the charset.
    It must be 1 (default), 2, 3, or 4.

    <li> Key is #Mmin_range, value is an unsigned integer

    The value specifies the minimum range of a code-point, which means
    that the Nth byte of the value is the minimum Nth byte of
    code-points of the charset.   The default value is 0.

    <li> Key is #Mmax_range, value is an unsigned integer

    The value specifies the maximum range of a code-point, which means
    that the Nth byte of the value is the maximum Nth byte of
    code-points of the charset.  The default value is 0xFF, 0xFFFF,
    0xFFFFFF, or 0xFFFFFFFF if the dimension is 1, 2, 3, or 4
    respectively.

    <li> Key is #Mmin_code, value is an unsigned integer

    The value specifies the minimum code-point of
    the charset.  The default value is the minimum range.

    <li> Key is #Mmax_code, value is an unsigned integer

    The value specifies the maximum code-point of
    the charset.  The default value is the maximum range.

    <li> Key is #Mascii_compatible, value is a symbol

    The value specifies whether the charset is ASCII compatible or
    not.  If the value is #Mnil (default), it is not ASCII
    compatible, else compatible.

    <li> Key is #Mfinal_byte, value is an integer

    The value specifies the @e final @e byte of the charset registered
    in The International Registry.  It must be 0 (default) or 32..127.
    The value 0 means that the charset is not in the registry.

    <li> Key is #Mrevision, value is an integer

    The value specifies the @e revision @e number of the charset
    registered in The International Registry.  It must be 0..127.  If
    the charset is not in The International Registry, the value is
    ignored.  The value 0 means that the charset has no revision
    number.

    <li> Key is #Mmin_char, value is an integer

    The value specifies the minimum character code of the charset.
    The default value is 0.

    <li> Key is #Mmapfile, value is an M-text

    If the method is #Mmap or #Munify, a data that contains
    mapping information is added to the m17n database by calling
    the function mdatabase_define () with the value as an argument $EXTRA_INFO,
    i.e. the value is used as a file name of the data.

    Otherwise, this parameter is ignored.

    <li> Key is #Mparents, value is a plist

    If the method is #Msubset, the value must is a plist of length
    1, and the value of the plist must be a symbol representing a
    parent charset.

    If the method is #Msuperset, the value must be a plist of length
    less than 9, and the values of the plist must be symbols
    representing subset charsets.

    Otherwise, this parameter is ignored.

    <li> Key is #Mdefine_coding, value is a symbol

    If the dimension of the charset is 1, the value specifies whether
    or not to define a coding system of the same name whose type is
    #Mcharset.  A coding system is defined if the value is not #Mnil.

    Otherwise, this parameter is ignored.

    </ul>

    @return
    If the operation was successful, mchar_define_charset () returns a
    symbol whose name is $NAME.  Otherwise it returns #Mnil and
    assigns an error code to the external variable #merror_code.  */

/***ja
    @brief 文字セットを定義する.

    関数 mchar_define_charset () は新しい文字セットを定義し、それを 
    $NAME という名前を持つシンボル経由でアクセスできるようにする。
    $PLIST は定義される文字セットのパラメータを以下のように指定する。

    <ul>

    <li> キーが #Mmethod で値がシンボルの時

    値は、#Moffset, #Mmap (デフォルト値), #Munify, #Msubset,
    #Msuperset のいずれかであり、文字セットのコードポイントをデコード／エンコードする際のメソッドを指定する。

    <li> キーが #Mdimension で値が整数値の時

    値は、1 (デフォルト値), 2, 3, 4 
    のいずれかであり、文字セットのコードポイントの次元である。

    <li> キーが #Mmin_range で値が非負整数値の時

    値はコードポイントの最小の値である。すなわち、この値の N 
    番目のバイトはこの文字セットのコードポイントの N 番目のバイトの最小のものとなる。
    デフォルト値は 0 。

    <li> キーが #Mmax_range で値が非負整数値の時

    値はコードポイントの最大の値である。すなわち、この値の N 
    番目のバイトはこの文字セットのコードポイントの N 番目のバイトの最大のものとなる。
    デフォルト値は、コードポイントの次元が 1, 2, 3, 4 の時、それぞれ
    0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF 。

    <li> キーが #Mmin_code で値が非負整数値の時

    値はこの文字セットの最小のコードポイントである。デフォルト値は 
    #Mmin_range の値。

    <li> キーが #Mmax_code で値が非負整数値の時

    値はこの文字セットの最大のコードポイントである。デフォルト値は 
    #Mmax_range の値。

    <li> キーが  #Mascii_compatible で値がシンボルの時

    値はこの文字セットが ASCII 互換であるかどうかを示す。デフォルト値の
    #Mnil であれば互換ではなく、それ以外の場合は互換である。

    <li> キーが  #Mfinal_byte で値が整数値の時

    値はこの文字セットの The International Registry に登録されている 
    @e 終端バイト であり、0 (デフォルト値) であるか 32..127 である。0 
    は登録されていないことを意味する。

    <li> キーが  #Mrevision で値が整数値の時

    値は The International Registry に登録されている @e revision @e
    number であり、0..127 である。
    文字セットが登録されていない場合にはこの値は無視される。
    0 は revision number が存在しないことを意味する。

    <li> キーが  #Mmin_char で値が整数値の時

    値はこの文字セットの最小の文字コードである。デフォルト値は 0 。

    <li> キーが #Mmapfile で値が M-text の時

    メソッドが #Mmap か #Munify の時、関数 mdatabase_define () 
    をこの値を引数 $EXTRA_INFO として呼ぶことによって、マッピングに関するデータが
    m17n データベースに追加される。
    すなわち、この値はデータファイルの名前である。

    そうでなければ、このパラメータは無視される。

    <li> キーが #Mparents で値が plist の時

    メソッドが #Msubset ならば、値は長さ 1 の plist 
    であり、その値はこの文字セットの上位集合となる文字セットを示すシンボルである。

    メソッドが #Msuperset ならば、値は長さ 8 以下の plist 
    であり、それらの値はこの文字セットの下位集合である文字セットを示すシンボルである。

    そうでなければ、このパラメータは無視される。

    <li> キーが  #Mdefine_coding で値がシンボルの時

    文字セットの次元が 1 ならば、値が #Mnil 以外の場合に #Mcharset 型
    で同じ名前を持つコード系を定義する。

    そうでなければ、このパラメータは無視される。

    </ul>

    @return 
    処理が成功すれば、mchar_define_charset() は $NAME 
    という名前のシンボルを返す。そうでなければ #Mnil を返し、外部変数 
    #merror_code にエラーコードを設定する。*/

/***
    @errors
    @c MERROR_CHARSET  */

MSymbol
mchar_define_charset (const char *name, MPlist *plist)
{
  MSymbol sym = msymbol (name);
  MCharset *charset;
  int i;
  unsigned min_range, max_range;
  MPlist *pl;
  MText *mapfile = (MText *) mplist_get (plist, Mmapfile);

  MSTRUCT_CALLOC (charset, MERROR_CHARSET);
  charset->name = sym;
  charset->method = (MSymbol) mplist_get (plist, Mmethod);
  if (! charset->method)
    {
      if (mapfile)
	charset->method = Mmap;
      else
	charset->method = Moffset;
    }
  if (charset->method == Mmap || charset->method == Munify)
    {
      if (! mapfile)
	MERROR (MERROR_CHARSET, Mnil);
      mdatabase_define (Mcharset, sym, Mnil, Mnil, NULL, mapfile->data);
    }
  if (! (charset->dimension = (int) mplist_get (plist, Mdimension)))
    charset->dimension = 1;

  min_range = (unsigned) mplist_get (plist, Mmin_range);
  if ((pl = mplist_find_by_key (plist, Mmax_range)))
    {
      max_range = (unsigned) MPLIST_VAL (pl);
      if (max_range >= 0x1000000)
	charset->dimension = 4;
      else if (max_range >= 0x10000 && charset->dimension < 3)
	charset->dimension = 3;
      else if (max_range >= 0x100 && charset->dimension < 2)
	charset->dimension = 2;
    }
  else if (charset->dimension == 1)
    max_range = 0xFF;
  else if (charset->dimension == 2)
    max_range = 0xFFFF;
  else if (charset->dimension == 3)
    max_range = 0xFFFFFF;
  else
    max_range = 0xFFFFFFFF;

  memset (charset->code_range, 0, sizeof charset->code_range);
  for (i = 0; i < charset->dimension; i++, min_range >>= 8, max_range >>= 8)
    {
      charset->code_range[i * 4] = min_range & 0xFF;
      charset->code_range[i * 4 + 1] = max_range & 0xFF;
    }
  if ((charset->min_code = (int) mplist_get (plist, Mmin_code)) < min_range)
    charset->min_code = min_range;
  if ((charset->max_code = (int) mplist_get (plist, Mmax_code)) > max_range)
    charset->max_code = max_range;
  charset->ascii_compatible
    = (MSymbol) mplist_get (plist, Mascii_compatible) != Mnil;
  charset->final_byte = (int) mplist_get (plist, Mfinal_byte);
  charset->revision = (int) mplist_get (plist, Mrevision);
  charset->min_char = (int) mplist_get (plist, Mmin_char);
  pl = (MPlist *) mplist_get (plist, Mparents);
  charset->nparents = pl ? mplist_length (pl) : 0;
  if (charset->nparents > 8)
    charset->nparents = 8;
  for (i = 0; i < charset->nparents; i++, pl = MPLIST_NEXT (pl))
    {
      MSymbol parent_name;

      if (MPLIST_KEY (pl) != Msymbol)
	MERROR (MERROR_CHARSET, Mnil);
      parent_name = MPLIST_SYMBOL (pl);
      if (! (charset->parents[i] = MCHARSET (parent_name)))
	MERROR (MERROR_CHARSET, Mnil);
    }

  charset->subset_offset = (int) mplist_get (plist, Msubset_offset);

  msymbol_put (sym, Mcharset, charset);
  charset = make_charset (charset);
  if (! charset)
    return Mnil;
  msymbol_put (msymbol__canonicalize (sym), Mcharset, charset);

  for (pl = (MPlist *) mplist_get (plist, Maliases);
       pl && MPLIST_KEY (pl) == Msymbol;
       pl = MPLIST_NEXT (pl))
    {
      MSymbol alias = MPLIST_SYMBOL (pl);

      msymbol_put (alias, Mcharset, charset);
      msymbol_put (msymbol__canonicalize (alias), Mcharset, charset);
    }

  if (mplist_get (plist, Mdefine_coding)
      && charset->dimension == 1
      && charset->code_range[0] == 0 && charset->code_range[1] == 255)
    mconv__register_charset_coding (sym);
  return (sym);
}

/*=*/

/***en
    @brief Resolve charset name.

    The mchar_resolve_charset () function returns $SYMBOL if it
    represents a charset.  Otherwise, canonicalize $SYMBOL as to a
    charset name, and if the canonicalized name represents a charset,
    return it.  Otherwise, return #Mnil.  */

/***ja
    @brief 文字セット名を解決する.

    関数 mchar_resolve_charset () は $SYMBOL 
    が文字セットを示していればそれを返す。

    そうでなければ、$SYMBOL を文字セット名として正規化し、それが文字セットを示していていれば正規化したものを返す。
    そうでなければ、#Mnil を返す。 */

MSymbol
mchar_resolve_charset (MSymbol symbol)
{
  MCharset *charset = (MCharset *) msymbol_get (symbol, Mcharset);

  if (! charset)
    {
      symbol = msymbol__canonicalize (symbol);
      charset = (MCharset *) msymbol_get (symbol, Mcharset);
    }

  return (charset ? charset->name : Mnil);
}

/*=*/

/***en
    @brief List symbols representing charsets.

    The mchar_list_charsets () function makes an array of symbols
    representing a charset, stores the pointer to the array in a place
    pointed to by $SYMBOLS, and returns the length of the array.  */

/***ja
    @brief 文字セットを表わすシンボルを列挙する.

    関数 mchar_list_charsets () 
    は、文字セットを示すシンボルを並べた配列を作り、$SYMBOLS 
    でポイントされた場所にこの配列へのポインタを置き、配列の長さを返す。 */

int
mchar_list_charset (MSymbol **symbols)
{
  int i;

  MTABLE_MALLOC ((*symbols), charset_list.used, MERROR_CHARSET);
  for (i = 0; i < charset_list.used; i++)
    (*symbols)[i] = charset_list.charsets[i]->name;
  return i;
}

/*=*/

/***en
    @brief Decode a code-point.

    The mchar_decode () function decodes code-point $CODE in the
    charset represented by the symbol $CHARSET_NAME to get a character
    code.

    @return
    If decoding was successful, mchar_decode () returns the decoded
    character code.  Otherwise it returns -1.  */

/***ja
    @brief コードポイントをデコードする.

    関数 mchar_decode () は、シンボル $CHARSET_NAME で示される文字セット内の
    $CODE というコードポイントをデコードして文字コードを得る。

    @return
    デコードが成功すれば、mchar_decode () はデコードされた文字コードを返す。
    そうでなければ -1 を返す。  */

/***
    @seealso
    mchar_encode ()  */

int
mchar_decode (MSymbol charset_name, unsigned code)
{
  MCharset *charset = MCHARSET (charset_name);

  if (! charset)
    return MCHAR_INVALID_CODE;
  return DECODE_CHAR (charset, code);
}

/*=*/

/***en
    @brief Encode a character code.

    The mchar_encode () function encodes character code $C to get a
    code-point in the charset represented by the symbol $CHARSET_NAME.

    @return
    If encoding was successful, mchar_encode () returns the encoded
    code-point.  Otherwise it returns #MCHAR_INVALID_CODE.  */

/***ja
    @brief 文字コードをエンコードする.

    関数 mchar_encode () は、文字コード $C をエンコードしてシンボル
    $CHARSET_NAME で示される文字セット内におけるコードポイントを得る。

    @return
    エンコードが成功すれば、mchar_encode () はエンードされたコードポイントを返す。
    そうでなければ #MCHAR_INVALID_CODE を返す。  */

/***
    @seealso
    mchar_decode ()  */

unsigned
mchar_encode (MSymbol charset_name, int c)
{
  MCharset *charset = MCHARSET (charset_name);

  if (! charset)
    return MCHAR_INVALID_CODE;
  return ENCODE_CHAR (charset, c);
}

/*=*/

/***en
    @brief Call a function for all the characters in a specified charset.

    The mcharset_map_chars () function calls $FUNC for all the
    characters in the charset named $CHARSET_NAME.  A call is done for
    a chunk of consecutive characters rather than character by
    character.

    $FUNC receives three arguments: $FROM, $TO, and $ARG.  $FROM and
    $TO specify the range of character codes in $CHARSET.  $ARG is the
    same as $FUNC_ARG.

    @return
    If the operation was successful, mcharset_map_chars () returns 0.
    Otherwise, it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief 指定した文字セットのすべての文字に対して関数を呼ぶ.

    関数 mcharset_map_chars () は $CHARSET_NAME 
    という名前を持つ文字セット中のすべての文字に対して $FUNC を呼ぶ。
    呼び出しは一文字毎ではなく、連続した文字のまとまり単位で行なわれる。

    関数 $FUNC には$FROM, $TO, $ARG の３引数が渡される。$FROM と $TO 
    は $CHARSET 中の文字コードの範囲を指定する。$ARG は $FUNC_ARG 
    と同じである。

    @return
    処理に成功すれば mcharset_map_chars () は 0 を返す。
    そうでなければ -1 を返し、外部変数 #merror_code にエラーコードを設定する。  */

/*** 
    @errors
    @c MERROR_CHARSET */

int
mchar_map_charset (MSymbol charset_name,
		   void (*func) (int from, int to, void *arg),
		   void *func_arg)
{
  MCharset *charset;

  charset = MCHARSET (charset_name);
  if (! charset)
    MERROR (MERROR_CHARSET, -1);

  if (charset->encoder)
    {
      int c = charset->min_char;
      int next_c;

      if ((int) mchartable__lookup (charset->encoder, c, &next_c, 1) < 0)
	c = next_c;
      while (c <= charset->max_char)
	{
	  if ((int) mchartable__lookup (charset->encoder, c, &next_c, 1) >= 0)
	    (*func) (c, next_c - 1, func_arg);	  
	  c = next_c;
	}
    }
  else
    (*func) (charset->min_char, charset->max_char, func_arg);
  return 0;
}

/*=*/

/*** @} */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
