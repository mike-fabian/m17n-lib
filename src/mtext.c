/* mtext.c -- M-text module.
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
    @addtogroup m17nMtext
    @brief M-text objects and API for them.

    In the m17n library, text is represented as an object called @e
    M-text rather than as a C-string (<tt>char *</tt> or <tt>unsigned
    char *</tt>).  An M-text is a sequence of characters whose length
    is equals to or more than 0, and can be coined from various
    character sources, e.g. C-strings, files, character codes, etc.

    M-texts are more useful than C-strings in the following points.

    @li M-texts can handle mixture of characters of various scripts,
    including all Unicode characters and more.  This is an
    indispensable facility when handling multilingual text.

    @li Each character in an M-text can have properties called @e text
    @e properties. Text properties store various kinds of information
    attached to parts of an M-text to provide application programs
    with a unified view of those information.  As rich information can
    be stored in M-texts in the form of text properties, functions in
    application programs can be simple.

    In addition, the library provides many functions to manipulate an
    M-text just the same way as a C-string.  */

/***ja
    @addtogroup m17nMtext

    @brief M-text オブジェクトとそれに関する API.

    m17n ライブラリは、 C-string（<tt>char *</tt> や <tt>unsigned
    char *</tt>）ではなく @e M-text と呼ぶオブジェクトでテキストを表現する。
    M-text は長さ 0 以上の文字列であり、種々の文字ソース（たとえば 
    C-string、ファイル、文字コード等）から作成できる。

    M-text には、C-string にない以下の特徴がある。     

    @li M-text は非常に多くの種類の文字を、同時に、混在させて、同等に扱うことができる。
    Unicode の全ての文字はもちろん、より多くの文字までも扱うことができる。
    これは多言語テキストを扱う上では必須の機能である。

    @li M-text 内の各文字は、@e テキストプロパティ 
    と呼ばれるプロパティを持ち、
    テキストプロパティによって、テキストの各部位に関する様々な情報を
    M-text 内に保持することが可能になる。
    そのため、それらの情報をアプリケーションプログラム内で統一的に扱うことが可能になる。
    また、M-text 
    自体が豊富な情報を持つため、アプリケーションプログラム中の各関数を簡素化することができる。

    さらにm17n ライブラリは、 C-string 
    を操作するために提供される種々の関数と同等のものを M-text 
    を操作するためにサポートしている。  */

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "textprop.h"
#include "character.h"
#include "mtext.h"
#include "plist.h"

static M17NObjectArray mtext_table;

static MSymbol M_charbag;

/** Increment character position CHAR_POS and unit position UNIT_POS
    so that they point to the next character in M-text MT.  No range
    check for CHAR_POS and UNIT_POS.  */

#define INC_POSITION(mt, char_pos, unit_pos)			\
  do {								\
    int c;							\
								\
    if ((mt)->format <= MTEXT_FORMAT_UTF_8)			\
      {								\
	c = (mt)->data[(unit_pos)];				\
	(unit_pos) += CHAR_UNITS_BY_HEAD_UTF8 (c);		\
      }								\
    else if ((mt)->format <= MTEXT_FORMAT_UTF_16BE)		\
      {								\
	c = ((unsigned short *) ((mt)->data))[(unit_pos)];	\
								\
	if ((mt)->format != MTEXT_FORMAT_UTF_16)		\
	  c = SWAP_16 (c);					\
	(unit_pos) += CHAR_UNITS_BY_HEAD_UTF16 (c);		\
      }								\
    else							\
      (unit_pos)++;						\
    (char_pos)++;						\
  } while (0)


/** Decrement character position CHAR_POS and unit position UNIT_POS
    so that they point to the previous character in M-text MT.  No
    range check for CHAR_POS and UNIT_POS.  */

#define DEC_POSITION(mt, char_pos, unit_pos)				\
  do {									\
    if ((mt)->format <= MTEXT_FORMAT_UTF_8)				\
      {									\
	unsigned char *p1 = (mt)->data + (unit_pos);			\
	unsigned char *p0 = p1 - 1;					\
									\
	while (! CHAR_HEAD_P (p0)) p0--;				\
	(unit_pos) -= (p1 - p0);					\
      }									\
    else if ((mt)->format <= MTEXT_FORMAT_UTF_16BE)			\
      {									\
	int c = ((unsigned short *) ((mt)->data))[(unit_pos) - 1];	\
									\
	if ((mt)->format != MTEXT_FORMAT_UTF_16)			\
	  c = SWAP_16 (c);						\
	(unit_pos) -= 2 - (c < 0xD800 || c >= 0xE000);			\
      }									\
    else								\
      (unit_pos)--;							\
    (char_pos)--;							\
  } while (0)


/* Compoare sub-texts in MT1 (range FROM1 and TO1) and MT2 (range
   FROM2 to TO2). */

static int
compare (MText *mt1, int from1, int to1, MText *mt2, int from2, int to2)
{
  if (mt1->format == mt2->format
      && (mt1->format <= MTEXT_FORMAT_UTF_8))
    {
      unsigned char *p1, *pend1, *p2, *pend2;
      int unit_bytes = UNIT_BYTES (mt1->format);
      int nbytes;
      int result;

      p1 = mt1->data + mtext__char_to_byte (mt1, from1) * unit_bytes;
      pend1 = mt1->data + mtext__char_to_byte (mt1, to1) * unit_bytes;

      p2 = mt2->data + mtext__char_to_byte (mt2, from2) * unit_bytes;
      pend2 = mt2->data + mtext__char_to_byte (mt2, to2) * unit_bytes;

      if (pend1 - p1 < pend2 - p2)
	nbytes = pend1 - p1;
      else
	nbytes = pend2 - p2;
      result = memcmp (p1, p2, nbytes);
      if (result)
	return result;
      return ((pend1 - p1) - (pend2 - p2));
    }
  for (; from1 < to1 && from2 < to2; from1++, from2++)
    {
      int c1 = mtext_ref_char (mt1, from1);
      int c2 = mtext_ref_char (mt2, from2);

      if (c1 != c2)
	return (c1 > c2 ? 1 : -1);
    }
  return (from2 == to2 ? (from1 < to1) : -1);
}


/* Return how many units are required in UTF-8 to represent characters
   between FROM and TO of MT.  */

static int
count_by_utf_8 (MText *mt, int from, int to)
{
  int n, c;

  for (n = 0; from < to; from++)
    {
      c = mtext_ref_char (mt, from);
      n += CHAR_UNITS_UTF8 (c);
    }
  return n;
}


/* Return how many units are required in UTF-16 to represent
   characters between FROM and TO of MT.  */

static int
count_by_utf_16 (MText *mt, int from, int to)
{
  int n, c;

  for (n = 0; from < to; from++)
    {
      c = mtext_ref_char (mt, from);
      n += CHAR_UNITS_UTF16 (c);
    }
  return n;
}


/* Insert text between FROM and TO of MT2 at POS of MT1.  */

static MText *
insert (MText *mt1, int pos, MText *mt2, int from, int to)
{
  int pos_unit = POS_CHAR_TO_BYTE (mt1, pos);
  int from_unit = POS_CHAR_TO_BYTE (mt2, from);
  int new_units = POS_CHAR_TO_BYTE (mt2, to) - from_unit;
  int unit_bytes;

  if (mt1->nchars == 0)
    mt1->format = mt2->format;
  else if (mt1->format != mt2->format)
    {
      /* Be sure to make mt1->format sufficient to contain all
	 characters in mt2.  */
      if (mt1->format == MTEXT_FORMAT_UTF_8
	  || mt1->format == MTEXT_FORMAT_UTF_32
	  || (mt1->format == MTEXT_FORMAT_UTF_16
	      && mt2->format <= MTEXT_FORMAT_UTF_16BE
	      && mt2->format != MTEXT_FORMAT_UTF_8))
	;
      else if (mt1->format == MTEXT_FORMAT_US_ASCII)
	{
	  if (mt2->format == MTEXT_FORMAT_UTF_8)
	    mt1->format = MTEXT_FORMAT_UTF_8;
	  else if (mt2->format == MTEXT_FORMAT_UTF_16
		   || mt2->format == MTEXT_FORMAT_UTF_32)
	    mtext__adjust_format (mt1, mt2->format);
	  else
	    mtext__adjust_format (mt1, MTEXT_FORMAT_UTF_8);
	}
      else
	{
	  mtext__adjust_format (mt1, MTEXT_FORMAT_UTF_8);
	  pos_unit = POS_CHAR_TO_BYTE (mt1, pos);
	}
    }

  unit_bytes = UNIT_BYTES (mt1->format);

  if (mt1->format == mt2->format)
    {
      int pos_byte = pos_unit * unit_bytes;
      int total_bytes = (mt1->nbytes + new_units) * unit_bytes;
      int new_bytes = new_units * unit_bytes;

      if (total_bytes + unit_bytes > mt1->allocated)
	{
	  mt1->allocated = total_bytes + unit_bytes;
	  MTABLE_REALLOC (mt1->data, mt1->allocated, MERROR_MTEXT);
	}
      if (pos < mt1->nchars)
	memmove (mt1->data + pos_byte + new_bytes, mt1->data + pos_byte,
		 (mt1->nbytes - pos_unit + 1) * unit_bytes);
      memcpy (mt1->data + pos_byte, mt2->data + from_unit * unit_bytes,
	      new_bytes);
    }
  else if (mt1->format == MTEXT_FORMAT_UTF_8)
    {
      unsigned char *p;
      int total_bytes, i, c;

      new_units = count_by_utf_8 (mt2, from, to);
      total_bytes = mt1->nbytes + new_units;

      if (total_bytes + 1 > mt1->allocated)
	{
	  mt1->allocated = total_bytes + 1;
	  MTABLE_REALLOC (mt1->data, mt1->allocated, MERROR_MTEXT);
	}
      p = mt1->data + pos_unit;
      memmove (p + new_units, p, mt1->nbytes - pos_unit + 1);
      for (i = from; i < to; i++)
	{
	  c = mtext_ref_char (mt2, i);
	  p += CHAR_STRING_UTF8 (c, p);
	}
    }
  else if (mt1->format == MTEXT_FORMAT_UTF_16)
    {
      unsigned short *p;
      int total_bytes, i, c;

      new_units = count_by_utf_16 (mt2, from, to);
      total_bytes = (mt1->nbytes + new_units) * USHORT_SIZE;

      if (total_bytes + USHORT_SIZE > mt1->allocated)
	{
	  mt1->allocated = total_bytes + USHORT_SIZE;
	  MTABLE_REALLOC (mt1->data, mt1->allocated, MERROR_MTEXT);
	}
      p = (unsigned short *) mt1->data + pos_unit;
      memmove (p + new_units, p,
	       (mt1->nbytes - pos_unit + 1) * USHORT_SIZE);
      for (i = from; i < to; i++)
	{
	  c = mtext_ref_char (mt2, i);
	  p += CHAR_STRING_UTF16 (c, p);
	}
    }
  else				/* MTEXT_FORMAT_UTF_32 */
    {
      unsigned int *p;
      int total_bytes, i;

      new_units = to - from;
      total_bytes = (mt1->nbytes + new_units) * UINT_SIZE;

      if (total_bytes + UINT_SIZE > mt1->allocated)
	{
	  mt1->allocated = total_bytes + UINT_SIZE;
	  MTABLE_REALLOC (mt1->data, mt1->allocated, MERROR_MTEXT);
	}
      p = (unsigned *) mt1->data + pos_unit;
      memmove (p + new_units, p,
	       (mt1->nbytes - pos_unit + 1) * UINT_SIZE);
      for (i = from; i < to; i++)
	*p++ = mtext_ref_char (mt2, i);
    }

  mtext__adjust_plist_for_insert
    (mt1, pos, to - from,
     mtext__copy_plist (mt2->plist, from, to, mt1, pos));
  mt1->nchars += to - from;
  mt1->nbytes += new_units;
  if (mt1->cache_char_pos > pos)
    {
      mt1->cache_char_pos += to - from;
      mt1->cache_byte_pos += new_units;
    }

  return mt1;
}


static MCharTable *
get_charbag (MText *mt)
{
  MTextProperty *prop = mtext_get_property (mt, 0, M_charbag);
  MCharTable *table;
  int i;

  if (prop)
    {
      if (prop->end == mt->nchars)
	return ((MCharTable *) prop->val);
      mtext_detach_property (prop);
    }

  table = mchartable (Msymbol, (void *) 0);
  for (i = mt->nchars - 1; i >= 0; i--)
    mchartable_set (table, mtext_ref_char (mt, i), Mt);
  prop = mtext_property (M_charbag, table, MTEXTPROP_VOLATILE_WEAK);
  mtext_attach_property (mt, 0, mtext_nchars (mt), prop);
  M17N_OBJECT_UNREF (prop);
  return table;
}


/* span () : Number of consecutive chars starting at POS in MT1 that
   are included (if NOT is Mnil) or not included (if NOT is Mt) in
   MT2.  */

static int
span (MText *mt1, MText *mt2, int pos, MSymbol not)
{
  int nchars = mtext_nchars (mt1);
  MCharTable *table = get_charbag (mt2);
  int i;

  for (i = pos; i < nchars; i++)
    if ((MSymbol) mchartable_lookup (table, mtext_ref_char (mt1, i)) == not)
      break;
  return (i - pos);
}


static int
count_utf_8_chars (const void *data, int nitems)
{
  unsigned char *p = (unsigned char *) data;
  unsigned char *pend = p + nitems;
  int nchars = 0;

  while (p < pend)
    {
      int i, n;

      for (; p < pend && *p < 128; nchars++, p++);
      if (p == pend)
	return nchars;
      if (! CHAR_HEAD_P_UTF8 (p))
	return -1;
      n = CHAR_UNITS_BY_HEAD_UTF8 (*p);
      if (p + n > pend)
	return -1;
      for (i = 1; i < n; i++)
	if (CHAR_HEAD_P_UTF8 (p + i))
	  return -1;
      p += n;
      nchars++;
    }
  return nchars;
}

static int
count_utf_16_chars (const void *data, int nitems, int swap)
{
  unsigned short *p = (unsigned short *) data;
  unsigned short *pend = p + nitems;
  int nchars = 0;
  int prev_surrogate = 0;

  for (; p < pend; p++)
    {
      int c = *p;

      if (swap)
	c = SWAP_16 (c);
      if (prev_surrogate)
	{
	  if (c < 0xDC00 || c >= 0xE000)
	    /* Invalid surrogate */
	    nchars++;
	}
      else
	{
	  if (c >= 0xD800 && c < 0xDC00)
	    prev_surrogate = 1;
	  nchars++;
	}
    }
  if (prev_surrogate)
    nchars++;
  return nchars;
}


static int
find_char_forward (MText *mt, int from, int to, int c)
{
  int from_byte = POS_CHAR_TO_BYTE (mt, from);

  if (mt->format <= MTEXT_FORMAT_UTF_8)
    {
      unsigned char *p = mt->data + from_byte;

      while (from < to && STRING_CHAR_ADVANCE_UTF8 (p) != c) from++;
    }
  else if (mt->format <= MTEXT_FORMAT_UTF_16BE)
    {
      unsigned short *p = (unsigned short *) (mt->data) + from_byte;

      if (mt->format == MTEXT_FORMAT_UTF_16)
	while (from < to && STRING_CHAR_ADVANCE_UTF16 (p) != c) from++;
      else if (c < 0x10000)
	{
	  c = SWAP_16 (c);
	  while (from < to && *p != c)
	    {
	      from++;
	      p += ((*p & 0xFF) < 0xD8 || (*p & 0xFF) >= 0xE0) ? 1 : 2;
	    }
	}
      else if (c < 0x110000)
	{
	  int c1 = (c >> 10) + 0xD800;
	  int c2 = (c & 0x3FF) + 0xDC00;

	  c1 = SWAP_16 (c1);
	  c2 = SWAP_16 (c2);
	  while (from < to && (*p != c1 || p[1] != c2))
	    {
	      from++;
	      p += ((*p & 0xFF) < 0xD8 || (*p & 0xFF) >= 0xE0) ? 1 : 2;
	    }
	}
      else
	from = to;
    }
  else
    {
      unsigned *p = (unsigned *) (mt->data) + from_byte;
      unsigned c1 = c;

      if (mt->format != MTEXT_FORMAT_UTF_32)
	c1 = SWAP_32 (c1);
      while (from < to && *p++ != c1) from++;
    }

  return (from < to ? from : -1);
}


static int
find_char_backward (MText *mt, int from, int to, int c)
{
  int to_byte = POS_CHAR_TO_BYTE (mt, to);

  if (mt->format <= MTEXT_FORMAT_UTF_8)
    {
      unsigned char *p = mt->data + to_byte;

      while (from < to)
	{
	  for (p--; ! CHAR_HEAD_P (p); p--);
	  if (c == STRING_CHAR (p))
	    break;
	  to--;
	}
    }
  else if (mt->format <= MTEXT_FORMAT_UTF_16LE)
    {
      unsigned short *p = (unsigned short *) (mt->data) + to_byte;

      if (mt->format == MTEXT_FORMAT_UTF_16)
	{
	  while (from < to)
	    {
	      p--;
	      if (*p >= 0xDC00 && *p < 0xE000)
		p--;
	      if (c == STRING_CHAR_UTF16 (p))
		break;
	      to--;
	    }
	}
      else if (c < 0x10000)
	{
	  c = SWAP_16 (c);
	  while (from < to && p[-1] != c)
	    {
	      to--;
	      p -= ((p[-1] & 0xFF) < 0xD8 || (p[-1] & 0xFF) >= 0xE0) ? 1 : 2;
	    }
	}
      else if (c < 0x110000)
	{
	  int c1 = (c >> 10) + 0xD800;
	  int c2 = (c & 0x3FF) + 0xDC00;

	  c1 = SWAP_16 (c1);
	  c2 = SWAP_16 (c2);
	  while (from < to && (p[-1] != c2 || p[-2] != c1))
	    {
	      to--;
	      p -= ((p[-1] & 0xFF) < 0xD8 || (p[-1] & 0xFF) >= 0xE0) ? 1 : 2;
	    }
	}
    }
  else
    {
      unsigned *p = (unsigned *) (mt->data) + to_byte;
      unsigned c1 = c;

      if (mt->format != MTEXT_FORMAT_UTF_32)
	c1 = SWAP_32 (c1);
      while (from < to && p[-1] != c1) to--, p--;
    }

  return (from < to ? to - 1 : -1);
}


static void
free_mtext (void *object)
{
  MText *mt = (MText *) object;

  if (mt->plist)
    mtext__free_plist (mt);
  if (mt->data && mt->allocated >= 0)
    free (mt->data);
  M17N_OBJECT_UNREGISTER (mtext_table, mt);
  free (object);
}

/** Structure for an iterator used in case-fold comparison.  */

struct casecmp_iterator {
  MText *mt;
  int pos;
  MText *folded;
  unsigned char *foldedp;
  int folded_len;
};

static int
next_char_from_it (struct casecmp_iterator *it)
{
  int c, c1;

  if (it->folded)
    {
      c = STRING_CHAR_AND_BYTES (it->foldedp, it->folded_len);
      return c;
    }

  c = mtext_ref_char (it->mt, it->pos);
  c1 = (int) mchar_get_prop (c, Msimple_case_folding);
  if (c1 == 0xFFFF)
    {
      it->folded
	= (MText *) mchar_get_prop (c, Mcomplicated_case_folding);
      it->foldedp = it->folded->data;
      c = STRING_CHAR_AND_BYTES (it->foldedp, it->folded_len);
      return c;
    }

  if (c1 >= 0)
    c = c1;
  return c;
}

static void
advance_it (struct casecmp_iterator *it)
{
  if (it->folded)
    {
      it->foldedp += it->folded_len;
      if (it->foldedp == it->folded->data + it->folded->nbytes)
	it->folded = NULL;
    }
  if (! it->folded)
    {
      it->pos++;
    }
}

static int
case_compare (MText *mt1, int from1, int to1, MText *mt2, int from2, int to2)
{
  struct casecmp_iterator it1, it2;

  it1.mt = mt1, it1.pos = from1, it1.folded = NULL;
  it2.mt = mt2, it2.pos = from2, it2.folded = NULL;

  while (it1.pos < to1 && it2.pos < to2)
    {
      int c1 = next_char_from_it (&it1);
      int c2 = next_char_from_it (&it2);

      if (c1 != c2)
	return (c1 > c2 ? 1 : -1);
      advance_it (&it1);
      advance_it (&it2);
    }
  return (it2.pos == to2 ? (it1.pos < to1) : -1);
}


/* Internal API */

int
mtext__init ()
{
  M_charbag = msymbol_as_managing_key ("  charbag");
  mtext_table.count = 0;
  return 0;
}


void
mtext__fini (void)
{
  mdebug__report_object ("M-text", &mtext_table);
}


int
mtext__char_to_byte (MText *mt, int pos)
{
  int char_pos, byte_pos;
  int forward;

  if (pos < mt->cache_char_pos)
    {
      if (mt->cache_char_pos == mt->cache_byte_pos)
	return pos;
      if (pos < mt->cache_char_pos - pos)
	{
	  char_pos = byte_pos = 0;
	  forward = 1;
	}
      else
	{
	  char_pos = mt->cache_char_pos;
	  byte_pos = mt->cache_byte_pos;
	  forward = 0;
	}
    }
  else
    {
      if (mt->nchars - mt->cache_char_pos == mt->nbytes - mt->cache_byte_pos)
	return (mt->cache_byte_pos + (pos - mt->cache_char_pos));
      if (pos - mt->cache_char_pos < mt->nchars - pos)
	{
	  char_pos = mt->cache_char_pos;
	  byte_pos = mt->cache_byte_pos;
	  forward = 1;
	}
      else
	{
	  char_pos = mt->nchars;
	  byte_pos = mt->nbytes;
	  forward = 0;
	}
    }
  if (forward)
    while (char_pos < pos)
      INC_POSITION (mt, char_pos, byte_pos);
  else
    while (char_pos > pos)
      DEC_POSITION (mt, char_pos, byte_pos);
  mt->cache_char_pos = char_pos;
  mt->cache_byte_pos = byte_pos;
  return byte_pos;
}

/* mtext__byte_to_char () */

int
mtext__byte_to_char (MText *mt, int pos_byte)
{
  int char_pos, byte_pos;
  int forward;

  if (pos_byte < mt->cache_byte_pos)
    {
      if (mt->cache_char_pos == mt->cache_byte_pos)
	return pos_byte;
      if (pos_byte < mt->cache_byte_pos - pos_byte)
	{
	  char_pos = byte_pos = 0;
	  forward = 1;
	}
      else
	{
	  char_pos = mt->cache_char_pos;
	  byte_pos = mt->cache_byte_pos;
	  forward = 0;
	}
    }
  else
    {
      if (mt->nchars - mt->cache_char_pos == mt->nbytes - mt->cache_byte_pos)
	return (mt->cache_char_pos + (pos_byte - mt->cache_byte_pos));
      if (pos_byte - mt->cache_byte_pos < mt->nbytes - pos_byte)
	{
	  char_pos = mt->cache_char_pos;
	  byte_pos = mt->cache_byte_pos;
	  forward = 1;
	}
      else
	{
	  char_pos = mt->nchars;
	  byte_pos = mt->nbytes;
	  forward = 0;
	}
    }
  if (forward)
    while (byte_pos < pos_byte)
      INC_POSITION (mt, char_pos, byte_pos);
  else
    while (byte_pos > pos_byte)
      DEC_POSITION (mt, char_pos, byte_pos);
  mt->cache_char_pos = char_pos;
  mt->cache_byte_pos = byte_pos;
  return char_pos;
}

/* Estimated extra bytes that malloc will use for its own purpose on
   each memory allocation.  */
#define MALLOC_OVERHEAD 4
#define MALLOC_MININUM_BYTES 12

void
mtext__enlarge (MText *mt, int nbytes)
{
  nbytes += MAX_UTF8_CHAR_BYTES;
  if (mt->allocated >= nbytes)
    return;
  if (nbytes < MALLOC_MININUM_BYTES)
    nbytes = MALLOC_MININUM_BYTES;
  while (mt->allocated < nbytes)
    mt->allocated = mt->allocated * 2 + MALLOC_OVERHEAD;
  MTABLE_REALLOC (mt->data, mt->allocated, MERROR_MTEXT);
}

int
mtext__takein (MText *mt, int nchars, int nbytes)
{
  if (mt->plist)
    mtext__adjust_plist_for_insert (mt, mt->nchars, nchars, NULL);
  mt->nchars += nchars;
  mt->nbytes += nbytes;
  mt->data[mt->nbytes] = 0;
  return 0;
}


int
mtext__cat_data (MText *mt, unsigned char *p, int nbytes,
		 enum MTextFormat format)
{
  int nchars = -1;

  if (mt->format > MTEXT_FORMAT_UTF_8)
    MERROR (MERROR_MTEXT, -1);
  if (format == MTEXT_FORMAT_US_ASCII)
    nchars = nbytes;
  else if (format == MTEXT_FORMAT_UTF_8)
    nchars = count_utf_8_chars (p, nbytes);
  if (nchars < 0)
    MERROR (MERROR_MTEXT, -1);
  mtext__enlarge (mt, mtext_nbytes (mt) + nbytes + 1);
  memcpy (MTEXT_DATA (mt) + mtext_nbytes (mt), p, nbytes);
  mtext__takein (mt, nchars, nbytes);
  return nchars;
}

MText *
mtext__from_data (const void *data, int nitems, enum MTextFormat format,
		  int need_copy)
{
  MText *mt;
  int nchars, nbytes, unit_bytes;

  if (format == MTEXT_FORMAT_US_ASCII)
    {
      const char *p = (char *) data, *pend = p + nitems;

      while (p < pend)
	if (*p++ < 0)
	  MERROR (MERROR_MTEXT, NULL);
      nchars = nbytes = nitems;
      unit_bytes = 1;
    }
  else if (format == MTEXT_FORMAT_UTF_8)
    {
      if ((nchars = count_utf_8_chars (data, nitems)) < 0)
	MERROR (MERROR_MTEXT, NULL);
      nbytes = nitems;
      unit_bytes = 1;
    }
  else if (format <= MTEXT_FORMAT_UTF_16BE)
    {
      if ((nchars = count_utf_16_chars (data, nitems,
					format != MTEXT_FORMAT_UTF_16)) < 0)
	MERROR (MERROR_MTEXT, NULL);
      nbytes = USHORT_SIZE * nitems;
      unit_bytes = USHORT_SIZE;
    }
  else				/* MTEXT_FORMAT_UTF_32XX */
    {
      nchars = nitems;
      nbytes = UINT_SIZE * nitems;
      unit_bytes = UINT_SIZE;
    }

  mt = mtext ();
  mt->format = format;
  mt->allocated = need_copy ? nbytes + unit_bytes : -1;
  mt->nchars = nchars;
  mt->nbytes = nitems;
  if (need_copy)
    {
      MTABLE_MALLOC (mt->data, mt->allocated, MERROR_MTEXT);
      memcpy (mt->data, data, nbytes);
      mt->data[nbytes] = 0;
    }
  else
    mt->data = (unsigned char *) data;
  return mt;
}


void
mtext__adjust_format (MText *mt, enum MTextFormat format)
{
  int i, c;

  if (mt->nchars > 0)
    switch (format)
      {
      case MTEXT_FORMAT_US_ASCII:
	{
	  unsigned char *p = mt->data;

	  for (i = 0; i < mt->nchars; i++)
	    *p++ = mtext_ref_char (mt, i);
	  mt->nbytes = mt->nchars;
	  mt->cache_byte_pos = mt->cache_char_pos;
	  break;
	}

      case MTEXT_FORMAT_UTF_8:
	{
	  unsigned char *p0, *p1;

	  i = count_by_utf_8 (mt, 0, mt->nchars) + 1;
	  MTABLE_MALLOC (p0, i, MERROR_MTEXT);
	  mt->allocated = i;
	  for (i = 0, p1 = p0; i < mt->nchars; i++)
	    {
	      c = mtext_ref_char (mt, i);
	      p1 += CHAR_STRING_UTF8 (c, p1);
	    }
	  *p1 = '\0';
	  free (mt->data);
	  mt->data = p0;
	  mt->nbytes = p1 - p0;
	  mt->cache_char_pos = mt->cache_byte_pos = 0;
	  break;
	}

      default:
	if (format == MTEXT_FORMAT_UTF_16)
	  {
	    unsigned short *p0, *p1;

	    i = (count_by_utf_16 (mt, 0, mt->nchars) + 1) * USHORT_SIZE;
	    MTABLE_MALLOC (p0, i, MERROR_MTEXT);
	    mt->allocated = i;
	    for (i = 0, p1 = p0; i < mt->nchars; i++)
	      {
		c = mtext_ref_char (mt, i);
		p1 += CHAR_STRING_UTF16 (c, p1);
	      }
	    *p1 = 0;
	    free (mt->data);
	    mt->data = (unsigned char *) p0;
	    mt->nbytes = p1 - p0;
	    mt->cache_char_pos = mt->cache_byte_pos = 0;
	    break;
	  }
	else
	  {
	    unsigned int *p;

	    mt->allocated = (mt->nchars + 1) * UINT_SIZE;
	    MTABLE_MALLOC (p, mt->allocated, MERROR_MTEXT);
	    for (i = 0; i < mt->nchars; i++)
	      p[i] = mtext_ref_char (mt, i);
	    p[i] = 0;
	    free (mt->data);
	    mt->data = (unsigned char *) p;
	    mt->nbytes = mt->nchars;
	    mt->cache_byte_pos = mt->cache_char_pos;
	  }
      }
  mt->format = format;
}


/* Find the position of a character at the beginning of a line of
   M-Text MT searching backward from POS.  */

int
mtext__bol (MText *mt, int pos)
{
  int byte_pos;

  if (pos == 0)
    return pos;
  byte_pos = POS_CHAR_TO_BYTE (mt, pos);
  if (mt->format <= MTEXT_FORMAT_UTF_8)
    {
      unsigned char *p = mt->data + byte_pos;

      if (p[-1] == '\n')
	return pos;
      p--;
      while (p > mt->data && p[-1] != '\n')
	p--;
      if (p == mt->data)
	return 0;
      byte_pos = p - mt->data;
      return POS_BYTE_TO_CHAR (mt, byte_pos);
    }
  else if (mt->format <= MTEXT_FORMAT_UTF_16BE)
    {
      unsigned short *p = ((unsigned short *) (mt->data)) + byte_pos;
      unsigned short newline = (mt->format == MTEXT_FORMAT_UTF_16
				? 0x0A00 : 0x000A);

      if (p[-1] == newline)
	return pos;
      p--;
      while (p > (unsigned short *) (mt->data) && p[-1] != newline)
	p--;
      if (p == (unsigned short *) (mt->data))
	return 0;
      byte_pos = p - (unsigned short *) (mt->data);
      return POS_BYTE_TO_CHAR (mt, byte_pos);;
    }
  else
    {
      unsigned *p = ((unsigned *) (mt->data)) + byte_pos;
      unsigned newline = (mt->format == MTEXT_FORMAT_UTF_32
			  ? 0x0A000000 : 0x0000000A);

      if (p[-1] == newline)
	return pos;
      p--, pos--;
      while (p > (unsigned *) (mt->data) && p[-1] != newline)
	p--, pos--;
      return pos;
    }
}


/* Find the position of a character at the end of a line of M-Text MT
   searching forward from POS.  */

int
mtext__eol (MText *mt, int pos)
{
  int byte_pos;

  if (pos == mt->nchars)
    return pos;
  byte_pos = POS_CHAR_TO_BYTE (mt, pos);
  if (mt->format <= MTEXT_FORMAT_UTF_8)
    {
      unsigned char *p = mt->data + byte_pos;
      unsigned char *endp;

      if (*p == '\n')
	return pos + 1;
      p++;
      endp = mt->data + mt->nbytes;
      while (p < endp && *p != '\n')
	p++;
      if (p == endp)
	return mt->nchars;
      byte_pos = p + 1 - mt->data;
      return POS_BYTE_TO_CHAR (mt, byte_pos);
    }
  else if (mt->format <= MTEXT_FORMAT_UTF_16BE)
    {
      unsigned short *p = ((unsigned short *) (mt->data)) + byte_pos;
      unsigned short *endp;
      unsigned short newline = (mt->format == MTEXT_FORMAT_UTF_16
				? 0x0A00 : 0x000A);

      if (*p == newline)
	return pos + 1;
      p++;
      endp = (unsigned short *) (mt->data) + mt->nbytes;
      while (p < endp && *p != newline)
	p++;
      if (p == endp)
	return mt->nchars;
      byte_pos = p + 1 - (unsigned short *) (mt->data);
      return POS_BYTE_TO_CHAR (mt, byte_pos);
    }
  else
    {
      unsigned *p = ((unsigned *) (mt->data)) + byte_pos;
      unsigned *endp;
      unsigned newline = (mt->format == MTEXT_FORMAT_UTF_32
			  ? 0x0A000000 : 0x0000000A);

      if (*p == newline)
	return pos + 1;
      p++, pos++;
      endp = (unsigned *) (mt->data) + mt->nbytes;
      while (p < endp && *p != newline)
	p++, pos++;
      return pos;
    }
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

#ifdef WORDS_BIGENDIAN
const int MTEXT_FORMAT_UTF_16 = MTEXT_FORMAT_UTF_16BE;
#else
const int MTEXT_FORMAT_UTF_16 = MTEXT_FORMAT_UTF_16LE;
#endif

#ifdef WORDS_BIGENDIAN
const int MTEXT_FORMAT_UTF_32 = MTEXT_FORMAT_UTF_32BE;
#else
const int MTEXT_FORMAT_UTF_32 = MTEXT_FORMAT_UTF_32LE;
#endif

/*** @addtogroup m17nMtext */
/*** @{ */
/*=*/

/***en
    @brief Allocate a new M-text.

    The mtext () function allocates a new M-text of length 0 and
    returns a pointer to it.  The allocated M-text will not be freed
    unless the user explicitly does so with the m17n_object_free ()
    function.  */

/***ja
    @brief 新しいM-textを割り当てる.

    関数 mtext () は、長さ 0 の新しい M-text 
    を割り当て、それへのポインタを返す。割り当てられた M-text は、関数
    m17n_object_free () によってユーザが明示的に行なわない限り、解放されない。

    @latexonly \IPAlabel{mtext} @endlatexonly  */

/***
    @seealso
    m17n_object_free ()  */

MText *
mtext ()
{
  MText *mt;

  M17N_OBJECT (mt, free_mtext, MERROR_MTEXT);
  mt->format = MTEXT_FORMAT_UTF_8;
  M17N_OBJECT_REGISTER (mtext_table, mt);
  return mt;
}

/***en
    @brief Allocate a new M-text with specified data.

    The mtext_from_data () function allocates a new M-text whose
    character sequence is specified by array $DATA of $NITEMS
    elements.  $FORMAT specifies the format of $DATA.

    When $FORMAT is either #MTEXT_FORMAT_US_ASCII or
    #MTEXT_FORMAT_UTF_8, the contents of $DATA must be of the type @c
    unsigned @c char, and $NITEMS counts by byte.

    When $FORMAT is either #MTEXT_FORMAT_UTF_16LE or
    #MTEXT_FORMAT_UTF_16BE, the contents of $DATA must be of the type
    @c unsigned @c short, and $NITEMS counts by unsigned short.

    When $FORMAT is either #MTEXT_FORMAT_UTF_32LE or
    #MTEXT_FORMAT_UTF_32BE, the contents of $DATA must be of the type
    @c unsigned, and $NITEMS counts by unsigned.

    The character sequence of the M-text is not modifiable.  
    The contents of $DATA must not be modified while the M-text is alive.

    The allocated M-text will not be freed unless the user explicitly
    does so with the m17n_object_unref () function.  Even in that case,
    $DATA is not freed.

    @return
    If the operation was successful, mtext_from_data () returns a
    pointer to the allocated M-text.  Otherwise it returns @c NULL and
    assigns an error code to the external variable #merror_code.  */
/***ja
    @brief 指定のデータを元に新しい M-text を割り当てる.

    関数 mtext_from_data () は、要素数 $NITEMS の配列 $DATA 
    で指定された文字列を持つ新しい M-text を割り当てる。$FORMAT は $DATA
    のフォーマットを示す。

    $FORMAT が #MTEXT_FORMAT_US_ASCII か #MTEXT_FORMAT_UTF_8 ならば、
    $DATA の内容は @c unsigned @c char 型であり、$NITEMS 
    はバイト単位で表されている。

    $FORMAT が #MTEXT_FORMAT_UTF_16LE か #MTEXT_FORMAT_UTF_16BE ならば、
    $DATA の内容は @c unsigned @c short 型であり、$NITEMS は unsigned
    short 単位である。

    $FORMAT が #MTEXT_FORMAT_UTF_32LE か #MTEXT_FORMAT_UTF_32BE ならば、
    $DATA の内容は@c unsigned 型であり、$NITEMS は unsigned 単位である。

    割り当てられた M-text の文字列は変更できない。$DATA の内容は 
    M-text が有効な間は変更してはならない。

    割り当てられた M-text は、関数 m17n_object_unref () 
    によってユーザが明示的に行なわない限り、解放されない。その場合でも $DATA は解放されない。

    @return 
    処理が成功すれば、mtext_from_data () は割り当てられたM-text 
    へのポインタを返す。そうでなければ @c NULL を返し外部変数 #merror_code 
    にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_MTEXT  */

MText *
mtext_from_data (const void *data, int nitems, enum MTextFormat format)
{
  if (nitems < 0
      || format < MTEXT_FORMAT_US_ASCII || format >= MTEXT_FORMAT_MAX)
    MERROR (MERROR_MTEXT, NULL);
  return mtext__from_data (data, nitems, format, 0);
}

/*=*/

/***en
    @brief Number of characters in M-text.

    The mtext_len () function returns the number of characters in
    M-text $MT.  */

/***ja
    @brief M-text 中の文字の数.

    関数 mtext_len () は M-text $MT 中の文字の数を返す。

    @latexonly \IPAlabel{mtext_len} @endlatexonly  */

int
mtext_len (MText *mt)
{
  return (mt->nchars);
}

/*=*/

/***en
    @brief Return the character at the specified position in an M-text.

    The mtext_ref_char () function returns the character at $POS in
    M-text $MT.  If an error is detected, it returns -1 and assigns an
    error code to the external variable #merror_code.  */

/***ja
    @brief M-text 中の指定された位置の文字を返す.

    関数 mtext_ref_char () は、M-text $MT の位置 $POS 
    の文字を返す。エラーが検出された場合は -1 を返し、外部変数 #merror_code
    にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_ref_char} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE  */

int
mtext_ref_char (MText *mt, int pos)
{
  int c;

  M_CHECK_POS (mt, pos, -1);
  if (mt->format <= MTEXT_FORMAT_UTF_8)
    {
      unsigned char *p = mt->data + POS_CHAR_TO_BYTE (mt, pos);

      c = STRING_CHAR_UTF8 (p);
    }
  else if (mt->format <= MTEXT_FORMAT_UTF_16BE)
    {
      unsigned short *p
	= (unsigned short *) (mt->data) + POS_CHAR_TO_BYTE (mt, pos);
      unsigned short p1[2];

      if (mt->format != MTEXT_FORMAT_UTF_16)
	{
	  p1[0] = SWAP_16 (*p);
	  if (p1[0] >= 0xD800 || p1[0] < 0xDC00)
	    p1[1] = SWAP_16 (p[1]);
	  p = p1;
	}
      c = STRING_CHAR_UTF16 (p);
    }
  else
    {
      c = ((unsigned *) (mt->data))[pos];
      if (mt->format != MTEXT_FORMAT_UTF_32)
	c = SWAP_32 (c);
    }
  return c;
}

/*=*/

/***en
    @brief Store a character into an M-text.

    The mtext_set_char () function sets character $C, which has no
    text properties, at $POS in M-text $MT.

    @return
    If the operation was successful, mtext_set_char () returns 0.
    Otherwise it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief M-text に一文字を設定する.

    関数 mtext_set_char () は、テキストプロパティ無しの文字 $C を 
    M-text $MT の位置 $POS に設定する。

    @return
    処理に成功すれば mtext_set_char () は 0 を返す。失敗すれば -1 
    を返し、外部変数 #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_set_char} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE */

int
mtext_set_char (MText *mt, int pos, int c)
{
  int pos_unit;
  int old_units, new_units;
  int delta;
  unsigned char *p;
  int unit_bytes;

  M_CHECK_POS (mt, pos, -1);
  M_CHECK_READONLY (mt, -1);

  mtext__adjust_plist_for_change (mt, pos, pos + 1);

  if (mt->format <= MTEXT_FORMAT_UTF_8)
    {
      if (c >= 0x80)
	mt->format = MTEXT_FORMAT_UTF_8;
    }
  else if (mt->format <= MTEXT_FORMAT_UTF_16BE)
    {
      if (c >= 0x110000)
	mtext__adjust_format (mt, MTEXT_FORMAT_UTF_8);
      else if (mt->format != MTEXT_FORMAT_UTF_16)
	mtext__adjust_format (mt, MTEXT_FORMAT_UTF_16);
    }
  else if (mt->format != MTEXT_FORMAT_UTF_32)
    mtext__adjust_format (mt, MTEXT_FORMAT_UTF_32);

  unit_bytes = UNIT_BYTES (mt->format);
  pos_unit = POS_CHAR_TO_BYTE (mt, pos);
  p = mt->data + pos_unit * unit_bytes;
  old_units = CHAR_UNITS_AT (mt, p);
  new_units = CHAR_UNITS (c, mt->format);
  delta = new_units - old_units;

  if (delta)
    {
      if (mt->cache_char_pos > pos)
	mt->cache_byte_pos += delta;

      if ((mt->nbytes + delta + 1) * unit_bytes > mt->allocated)
	{
	  mt->allocated = (mt->nbytes + delta + 1) * unit_bytes;
	  MTABLE_REALLOC (mt->data, mt->allocated, MERROR_MTEXT);
	}

      memmove (mt->data + (pos_unit + new_units) * unit_bytes, 
	       mt->data + (pos_unit + old_units) * unit_bytes,
	       (mt->nbytes - pos_unit - old_units + 1) * unit_bytes);
      mt->nbytes += delta;
      mt->data[mt->nbytes * unit_bytes] = 0;
    }
  switch (mt->format)
    {
    case MTEXT_FORMAT_US_ASCII:
      mt->data[pos_unit] = c;
      break;
    case MTEXT_FORMAT_UTF_8:
      {
	unsigned char *p = mt->data + pos_unit;
	CHAR_STRING_UTF8 (c, p);
	break;
      }
    default:
      if (mt->format == MTEXT_FORMAT_UTF_16)
	{
	  unsigned short *p = (unsigned short *) mt->data + pos_unit;

	  CHAR_STRING_UTF16 (c, p);
	}
      else
	((unsigned *) mt->data)[pos_unit] = c;
    }
  return 0;
}

/*=*/

/***en
    @brief  Append a character to an M-text.

    The mtext_cat_char () function appends character $C, which has no
    text properties, to the end of M-text $MT.

    @return
    This function returns a pointer to the resulting M-text $MT.  If
    $C is an invalid character, it returns @c NULL.  */

/***ja
    @brief M-text に一文字追加する.

    関数 mtext_cat_char () は、テキストプロパティ無しの文字 $C を 
    M-text $MT の末尾に追加する。

    @return
    この関数は変更された M-text $MT へのポインタを返す。$C 
    が正しい文字でない場合には @c NULL を返す。  */

/***
    @seealso
    mtext_cat (), mtext_ncat ()  */

MText *
mtext_cat_char (MText *mt, int c)
{
  int nunits;
  int unit_bytes = UNIT_BYTES (mt->format);

  M_CHECK_READONLY (mt, NULL);
  if (c < 0 || c > MCHAR_MAX)
    return NULL;
  mtext__adjust_plist_for_insert (mt, mt->nchars, 1, NULL);

  if (c >= 0x80
      && (mt->format == MTEXT_FORMAT_US_ASCII
	  || (c >= 0x10000
	      && (mt->format == MTEXT_FORMAT_UTF_16LE
		  || mt->format == MTEXT_FORMAT_UTF_16BE))))

    {
      mtext__adjust_format (mt, MTEXT_FORMAT_UTF_8);
      unit_bytes = 1;
    }
  else if (mt->format >= MTEXT_FORMAT_UTF_32LE)
    {
      if (mt->format != MTEXT_FORMAT_UTF_32)
	mtext__adjust_format (mt, MTEXT_FORMAT_UTF_32);
    }
  else if (mt->format >= MTEXT_FORMAT_UTF_16LE)
    {
      if (mt->format != MTEXT_FORMAT_UTF_16)
	mtext__adjust_format (mt, MTEXT_FORMAT_UTF_16);
    }

  nunits = CHAR_UNITS (c, mt->format);
  if ((mt->nbytes + nunits + 1) * unit_bytes > mt->allocated)
    {
      mt->allocated = (mt->nbytes + nunits + 1) * unit_bytes;
      MTABLE_REALLOC (mt->data, mt->allocated, MERROR_MTEXT);
    }
  
  if (mt->format <= MTEXT_FORMAT_UTF_8)
    {
      unsigned char *p = mt->data + mt->nbytes;
      p += CHAR_STRING_UTF8 (c, p);
      *p = 0;
    }
  else if (mt->format == MTEXT_FORMAT_UTF_16)
    {
      unsigned short *p = (unsigned short *) mt->data + mt->nbytes;
      p += CHAR_STRING_UTF16 (c, p);
      *p = 0;
    }
  else
    {
      unsigned *p = (unsigned *) mt->data + mt->nbytes;
      *p++ = c;
      *p = 0;
    }

  mt->nchars++;
  mt->nbytes += nunits;
  return mt;
}

/*=*/

/***en
    @brief  Create a copy of an M-text.

    The mtext_dup () function creates a copy of M-text $MT while
    inheriting all the text properties of $MT.

    @return
    This function returns a pointer to the created copy.  */

/***ja
    @brief M-text のコピーを作る.

    関数 mtext_dup () は、M-text $MT のコピーを作る。$MT 
    のテキストプロパティはすべて継承される。

    @return
    この関数は作られたコピーへのポインタを返す。

     @latexonly \IPAlabel{mtext_dup} @endlatexonly  */

/***
    @seealso
    mtext_duplicate ()  */

MText *
mtext_dup (MText *mt)
{
  MText *new = mtext ();
  int unit_bytes = UNIT_BYTES (mt->format);

  *new = *mt;
  if (mt->nchars > 0)
    {
      new->allocated = (mt->nbytes + 1) * unit_bytes;
      MTABLE_MALLOC (new->data, new->allocated, MERROR_MTEXT);
      memcpy (new->data, mt->data, new->allocated);
      if (mt->plist)
	new->plist = mtext__copy_plist (mt->plist, 0, mt->nchars, new, 0);
    }
  return new;
}

/*=*/

/***en
    @brief  Append an M-text to another.

    The mtext_cat () function appends M-text $MT2 to the end of M-text
    $MT1 while inheriting all the text properties.  $MT2 itself is not
    modified.

    @return
    This function returns a pointer to the resulting M-text $MT1.  */

/***ja
    @brief 2個の M-textを連結する.

    関数 mtext_cat () は、 M-text $MT2 を M-text $MT1 
    の末尾に付け加える。$MT2 のテキストプロパティはすべて継承される。$MT2 は変更されない。

    @return
    この関数は変更された M-text $MT1 へのポインタを返す。

    @latexonly \IPAlabel{mtext_cat} @endlatexonly  */

/***
    @seealso
    mtext_ncat (), mtext_cat_char ()  */

MText *
mtext_cat (MText *mt1, MText *mt2)
{
  M_CHECK_READONLY (mt1, NULL);

  if (mt2->nchars > 0)
    insert (mt1, mt1->nchars, mt2, 0, mt2->nchars);
  return mt1;
}


/*=*/

/***en
    @brief Append a part of an M-text to another.

    The mtext_ncat () function appends the first $N characters of
    M-text $MT2 to the end of M-text $MT1 while inheriting all the
    text properties.  If the length of $MT2 is less than $N, all
    characters are copied.  $MT2 is not modified.  

    @return 
    If the operation was successful, mtext_ncat () returns a
    pointer to the resulting M-text $MT1.  If an error is detected, it
    returns @c NULL and assigns an error code to the global variable
    #merror_code.  */

/***ja
    @brief M-text の一部を別の M-text に付加する.

    関数 mtext_ncat () は、M-text $MT2 のはじめの $N 文字を M-text
    $MT1 の末尾に付け加える。$MT2 のテキストプロパティはすべて継承される。$MT2
    の長さが $N 以下ならば、$MT2 のすべての文字が付加される。 $MT2 は変更されない。

    @return
    処理が成功した場合、mtext_ncat () は変更された M-text $MT1
    へのポインタを返す。エラーが検出された場合は @c NULL を返し、外部変数
    #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_ncat} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mtext_cat (), mtext_cat_char ()  */

MText *
mtext_ncat (MText *mt1, MText *mt2, int n)
{
  M_CHECK_READONLY (mt1, NULL);
  if (n < 0)
    MERROR (MERROR_RANGE, NULL);
  if (mt2->nchars > 0)
    insert (mt1, mt1->nchars, mt2, 0, mt2->nchars < n ? mt2->nchars : n);
  return mt1;
}


/*=*/

/***en
    @brief Copy an M-text to another.

    The mtext_cpy () function copies M-text $MT2 to M-text $MT1 while
    inheriting all the text properties.  The old text in $MT1 is
    overwritten and the length of $MT1 is extended if necessary.  $MT2
    is not modified.

    @return
    This function returns a pointer to the resulting M-text $MT1.  */

/***ja
    @brief M-text を別の M-text にコピーする.

    関数 mtext_cpy () は M-text $MT2 を M-text $MT1 に上書きコピーする。
    $MT2 のテキストプロパティはすべて継承される。$MT1 
    の長さは必要に応じて伸ばされる。$MT2 は変更されない。

    @return
    この関数は変更された M-text $MT1 へのポインタを返す。

    @latexonly \IPAlabel{mtext_cpy} @endlatexonly  */

/***
    @seealso
    mtext_ncpy (), mtext_copy ()  */

MText *
mtext_cpy (MText *mt1, MText *mt2)
{
  M_CHECK_READONLY (mt1, NULL);
  mtext_del (mt1, 0, mt1->nchars);
  if (mt2->nchars > 0)
    insert (mt1, 0, mt2, 0, mt2->nchars);
  return mt1;
}

/*=*/

/***en
    @brief Copy the first some characters in an M-text to another.

    The mtext_ncpy () function copies the first $N characters of
    M-text $MT2 to M-text $MT1 while inheriting all the text
    properties.  If the length of $MT2 is less than $N, all characters
    of $MT2 are copied.  The old text in $MT1 is overwritten and the
    length of $MT1 is extended if necessary.  $MT2 is not modified.

    @return
    If the operation was successful, mtext_ncpy () returns a pointer
    to the resulting M-text $MT1.  If an error is detected, it returns
    @c NULL and assigns an error code to the global variable 
    #merror_code.  */

/***ja
    @brief M-text に含まれる最初の何文字かをコピーする.

    関数 mtext_ncpy () は、M-text $MT2 の最初の $N 文字を M-text $MT1 
    に上書きコピーする。$MT2 のテキストプロパティはすべて継承される。もし $MT2
    の長さが $N よりも小さければ $MT2 のすべての文字をコピーする。$MT1 
    の長さは必要に応じて伸ばされる。$MT2 は変更されない。

    @return 
    処理が成功した場合、mtext_ncpy () は変更された M-text $MT1 
    へのポインタを返す。エラーが検出された場合は @c NULL を返し、外部変数 
    #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_ncpy} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mtext_cpy (), mtext_copy ()  */

MText *
mtext_ncpy (MText *mt1, MText *mt2, int n)
{
  M_CHECK_READONLY (mt1, NULL);
  if (n < 0)
    MERROR (MERROR_RANGE, NULL);
  mtext_del (mt1, 0, mt1->nchars);
  if (mt2->nchars > 0)
    insert (mt1, 0, mt2, 0, mt2->nchars < n ? mt2->nchars : n);
  return mt1;
}

/*=*/

/***en
    @brief Create a new M-text from a part of an existing M-text.

    The mtext_duplicate () function creates a copy of sub-text of
    M-text $MT, starting at $FROM (inclusive) and ending at $TO
    (exclusive) while inheriting all the text properties of $MT.  $MT
    itself is not modified.

    @return
    If the operation was successful, mtext_duplicate () returns a
    pointer to the created M-text.  If an error is detected, it returns 0
    and assigns an error code to the external variable #merror_code.  */

/***ja
    @brief 既存の M-text の一部から新しい M-text をつくる.

    関数 mtext_duplicate () は、M-text $MT の $FROM （$FROM 自体も含む）から
    $TO （$TO 自体は含まない）までの部分のコピーを作る。このとき $MT 
    のテキストプロパティはすべて継承される。$MT そのものは変更されない。

    @return
    処理が成功すれば、mtext_duplicate () は作られた M-text 
    へのポインタを返す。エラーが検出された場合は @c NULL を返し、外部変数 
    #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_duplicate} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mtext_dup ()  */

MText *
mtext_duplicate (MText *mt, int from, int to)
{
  MText *new;

  M_CHECK_RANGE_X (mt, from, to, NULL);
  new = mtext ();
  new->format = mt->format;
  if (from < to)
    insert (new, 0, mt, from, to);
  return new;
}

/*=*/

/***en
    @brief Copy characters in the specified range into an M-text.

    The mtext_copy () function copies the text between $FROM
    (inclusive) and $TO (exclusive) in M-text $MT2 to the region
    starting at $POS in M-text $MT1 while inheriting the text
    properties.  The old text in $MT1 is overwritten and the length of
    $MT1 is extended if necessary.  $MT2 is not modified.

    @return
    If the operation was successful, mtext_copy () returns a pointer
    to the modified $MT1.  Otherwise, it returns @c NULL and assigns
    an error code to the external variable #merror_code.  */

/***ja
    @brief M-text に指定範囲の文字をコピーする.

    関数 mtext_copy () は、 M-text $MT2 の $FROM （$FROM 自体も含む）から 
    $TO （$TO 自体は含まない）までの範囲のテキストを M-text $MT1 の位置 $POS
    から上書きコピーする。$MT2 のテキストプロパティはすべて継承される。$MT1 
    の長さは必要に応じて伸ばされる。$MT2 は変更されない。

    @latexonly \IPAlabel{mtext_copy} @endlatexonly

    @return
    処理が成功した場合、mtext_copy () は変更された $MT1 
    へのポインタを返す。そうでなければ @c NULL を返し、外部変数 #merror_code 
    にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mtext_cpy (), mtext_ncpy ()  */

MText *
mtext_copy (MText *mt1, int pos, MText *mt2, int from, int to)
{
  M_CHECK_POS_X (mt1, pos, NULL);
  M_CHECK_READONLY (mt1, NULL);
  M_CHECK_RANGE_X (mt2, from, to, NULL);
  mtext_del (mt1, pos, mt1->nchars);
  return insert (mt1, pos, mt2, from, to);
}

/*=*/


/***en
    @brief Delete characters in the specified range destructively.

    The mtext_del () function deletes the characters in the range
    $FROM (inclusive) and $TO (exclusive) from M-text $MT
    destructively.  As a result, the length of $MT shrinks by ($TO -
    $FROM) characters.

    @return
    If the operation was successful, mtext_del () returns 0.
    Otherwise, it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief 指定範囲の文字を破壊的に取り除く.

    関数 mtext_del () は、M-text $MT の $FROM （$FROM 自体も含む）から $TO
    （$TO 自体は含まない）までの文字を破壊的に取り除く。結果的に $MT は長さが ($TO @c
    - $FROM) だけ縮むことになる。

    @return
    処理が成功すれば mtext_del () は 0 を返す。そうでなければ -1 
    を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mtext_ins ()  */

int
mtext_del (MText *mt, int from, int to)
{
  int from_byte, to_byte;
  int unit_bytes = UNIT_BYTES (mt->format);

  M_CHECK_READONLY (mt, -1);
  M_CHECK_RANGE (mt, from, to, -1, 0);

  from_byte = POS_CHAR_TO_BYTE (mt, from);
  to_byte = POS_CHAR_TO_BYTE (mt, to);

  if (mt->cache_char_pos >= to)
    {
      mt->cache_char_pos -= to - from;
      mt->cache_byte_pos -= to_byte - from_byte;
    }
  else if (mt->cache_char_pos > from)
    {
      mt->cache_char_pos -= from;
      mt->cache_byte_pos -= from_byte;
    }

  mtext__adjust_plist_for_delete (mt, from, to - from);
  memmove (mt->data + from_byte * unit_bytes, 
	   mt->data + to_byte * unit_bytes,
	   (mt->nbytes - to_byte + 1) * unit_bytes);
  mt->nchars -= (to - from);
  mt->nbytes -= (to_byte - from_byte);
  mt->cache_char_pos = from;
  mt->cache_byte_pos = from_byte;
  return 0;
}


/*=*/

/***en
    @brief Insert an M-text into another M-text.

    The mtext_ins () function inserts M-text $MT2 into M-text $MT1, at
    position $POS.  As a result, $MT1 is lengthen by the length of
    $MT2.  On insertion, all the text properties of $MT2 are
    inherited.  The original $MT2 is not modified.

    @return
    If the operation was successful, mtext_ins () returns 0.
    Otherwise, it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief M-text を別の M-text に挿入する.

    関数 mtext_ins () は M-text $MT1 の $POS の位置に 別の M-text $MT2 
    を挿入する。この結果 $MT1 の長さは $MT2 の長さ分だけ増える。挿入の際、$MT2
    のテキストプロパティはすべて継承される。$MT2 そのものは変更されない。

    @return
    処理が成功すれば mtext_ins () は 0 を返す。そうでなければ -1 
    を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mtext_del ()  */

int
mtext_ins (MText *mt1, int pos, MText *mt2)
{
  M_CHECK_READONLY (mt1, -1);
  M_CHECK_POS_X (mt1, pos, -1);

  if (mt2->nchars == 0)
    return 0;
  insert (mt1, pos, mt2, 0, mt2->nchars);
  return 0;
}


/*=*/

/***en
    @brief Insert a character into an M-text.

    The mtext_ins_char () function inserts $N copies of character $C
    into M-text $MT at position $POS.  As a result, $MT is lengthen by
    $N.

    @return
    If the operation was successful, mtext_ins () returns 0.
    Otherwise, it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief M-text に文字を挿入する.

    関数 mtext_ins_char () は M-text $MT の $POS の位置に文字 $C のコピーを $N
    個挿入する。この結果 $MT1 の長さは $N だけ増える。

    @return
    処理が成功すれば mtext_ins_char () は 0 を返す。そうでなければ -1
    を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mtext_ins, mtext_del ()  */

int
mtext_ins_char (MText *mt, int pos, int c, int n)
{
  int nunits;
  int unit_bytes = UNIT_BYTES (mt->format);
  int pos_unit;
  int i;

  M_CHECK_READONLY (mt, -1);
  M_CHECK_POS_X (mt, pos, -1);
  if (c < 0 || c > MCHAR_MAX)
    MERROR (MERROR_MTEXT, -1);
  if (n <= 0)
    return 0;
  mtext__adjust_plist_for_insert (mt, pos, n, NULL);

  if (c >= 0x80
      && (mt->format == MTEXT_FORMAT_US_ASCII
	  || (c >= 0x10000 && (mt->format == MTEXT_FORMAT_UTF_16LE
			       || mt->format == MTEXT_FORMAT_UTF_16BE))))
    {
      mtext__adjust_format (mt, MTEXT_FORMAT_UTF_8);
      unit_bytes = 1;
    }
  else if (mt->format >= MTEXT_FORMAT_UTF_32LE)
    {
      if (mt->format != MTEXT_FORMAT_UTF_32)
	mtext__adjust_format (mt, MTEXT_FORMAT_UTF_32);
    }
  else if (mt->format >= MTEXT_FORMAT_UTF_16LE)
    {
      if (mt->format != MTEXT_FORMAT_UTF_16)
	mtext__adjust_format (mt, MTEXT_FORMAT_UTF_16);
    }

  nunits = CHAR_UNITS (c, mt->format);
  if ((mt->nbytes + nunits * n + 1) * unit_bytes > mt->allocated)
    {
      mt->allocated = (mt->nbytes + nunits * n + 1) * unit_bytes;
      MTABLE_REALLOC (mt->data, mt->allocated, MERROR_MTEXT);
    }
  pos_unit = POS_CHAR_TO_BYTE (mt, pos);
  if (mt->cache_char_pos > pos)
    {
      mt->cache_char_pos += n;
      mt->cache_byte_pos += nunits + n;
    }
  memmove (mt->data + (pos_unit + nunits * n) * unit_bytes,
	   mt->data + pos_unit * unit_bytes,
	   (mt->nbytes - pos_unit + 1) * unit_bytes);
  if (mt->format <= MTEXT_FORMAT_UTF_8)
    {
      unsigned char *p = mt->data + pos_unit;

      for (i = 0; i < n; i++)
	p += CHAR_STRING_UTF8 (c, p);
    }
  else if (mt->format == MTEXT_FORMAT_UTF_16)
    {
      unsigned short *p = (unsigned short *) mt->data + pos_unit;

      for (i = 0; i < n; i++)
	p += CHAR_STRING_UTF16 (c, p);
    }
  else
    {
      unsigned *p = (unsigned *) mt->data + pos_unit;

      for (i = 0; i < n; i++)
	*p++ = c;
    }
  mt->nchars += n;
  mt->nbytes += nunits * n;
  return 0;
}

/*=*/

/***en
    @brief Search a character in an M-text.

    The mtext_character () function searches M-text $MT for character
    $C.  If $FROM is less than $TO, the search begins at position $FROM
    and goes forward but does not exceed ($TO - 1).  Otherwise, the search
    begins at position ($FROM - 1) and goes backward but does not
    exceed $TO.  An invalid position specification is regarded as both
    $FROM and $TO being 0.

    @return
    If $C is found, mtext_character () returns the position of its
    first occurrence.  Otherwise it returns -1 without changing the
    external variable #merror_code.  If an error is detected, it returns -1 and
    assigns an error code to the external variable #merror_code.  */

/***ja
    @brief M-text 中で文字を探す.

    関数 mtext_character () は M-text $MT 中で文字 $C を探す。もし 
    $FROM が $TO より小さければ、探索は位置 $FROM から末尾方向へ、最大 
    ($TO - 1) まで進む。そうでなければ位置 ($FROM - 1) から先頭方向へ、最大
    $TO まで進む。位置の指定に誤りがある場合は、$FROM と $TO 
    の両方に 0 が指定されたものとみなす。

    @return
    もし $C が見つかれば、mtext_character () 
    はその最初の出現位置を返す。見つからなかった場合は外部変数 #merror_code
    を変更せずに -1 を返す。エラーが検出された場合は -1 を返し、外部変数
    #merror_code にエラーコードを設定する。  */

/***
    @seealso
    mtext_chr(), mtext_rchr ()  */

int
mtext_character (MText *mt, int from, int to, int c)
{
  if (from < to)
    {
      /* We do not use M_CHECK_RANGE () because this function should
	 not set merror_code.  */
      if (from < 0 || to > mt->nchars)
	return -1;
      return find_char_forward (mt, from, to, c);
    }
  else
    {
      /* ditto */
      if (to < 0 || from > mt->nchars)
	return -1;
      return find_char_backward (mt, to, from, c);
    }
}


/*=*/

/***en
    @brief Return the position of the first occurrence of a character in an M-text.

    The mtext_chr () function searches M-text $MT for character $C.
    The search starts from the beginning of $MT and goes toward the end.

    @return
    If $C is found, mtext_chr () returns its position; otherwise it
    returns -1.  */

/***ja
    @brief M-text 中で指定された文字が最初に現れる位置を返す.

    関数 mtext_chr () は M-text $MT 中で文字 $C を探す。探索は $MT 
    の先頭から末尾方向に進む。

    @return
    もし $C が見つかれば、mtext_chr () 
    はその出現位置を返す。見つからなかった場合は -1 を返す。

    @latexonly \IPAlabel{mtext_chr} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mtext_rchr (), mtext_character ()  */

int
mtext_chr (MText *mt, int c)
{
  return find_char_forward (mt, 0, mt->nchars, c);
}

/*=*/

/***en
    @brief Return the position of the last occurrence of a character in an M-text.

    The mtext_rchr () function searches M-text $MT for character $C.
    The search starts from the end of $MT and goes backwardly toward the
    beginning.

    @return
    If $C is found, mtext_rchr () returns its position; otherwise it
    returns -1.  */

/***ja
    @brief M-text 中で指定された文字が最後に現れる位置を返す.

    関数 mtext_rchr () は M-text $MT 中で文字 $C を探す。探索は $MT 
    の最後から先頭方向へと後向きに進む。

    @return
    もし $C が見つかれば、mtext_rchr () 
    はその出現位置を返す。見つからなかった場合は -1 を返す。

    @latexonly \IPAlabel{mtext_rchr} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mtext_chr (), mtext_character ()  */

int
mtext_rchr (MText *mt, int c)
{
  return find_char_backward (mt, mt->nchars, 0, c);
}


/*=*/

/***en
    @brief Compare two M-texts character-by-character.

    The mtext_cmp () function compares M-texts $MT1 and $MT2 character
    by character.

    @return
    This function returns 1, 0, or -1 if $MT1 is found greater than,
    equal to, or less than $MT2, respectively.  Comparison is based on
    character codes.  */

/***ja
    @brief 二つの M-text を文字単位で比較する.

    関数 mtext_cmp () は、 M-text $MT1 と $MT2 を文字単位で比較する。

    @return
    この関数は、$MT1 と $MT2 が等しければ 0、$MT1 が $MT2 より大きければ
    1、$MT1 が $MT2 より小さければ -1 を返す。比較は文字コードに基づく。

    @latexonly \IPAlabel{mtext_cmp} @endlatexonly  */

/***
    @seealso
    mtext_ncmp (), mtext_casecmp (), mtext_ncasecmp (),
    mtext_compare (), mtext_case_compare ()  */

int
mtext_cmp (MText *mt1, MText *mt2)
{
  return compare (mt1, 0, mt1->nchars, mt2, 0, mt2->nchars);
}


/*=*/

/***en
    @brief Compare initial parts of two M-texts character-by-character.

    The mtext_ncmp () function is similar to mtext_cmp (), but
    compares at most $N characters from the beginning.

    @return
    This function returns 1, 0, or -1 if $MT1 is found greater than,
    equal to, or less than $MT2, respectively.  */

/***ja
    @brief 二つの M-text の先頭部分を文字単位で比較する.

    関数 mtext_ncmp () は、関数 mtext_cmp () 同様の M-text 
    同士の比較を先頭から最大 $N 文字までに関して行なう。

    @return
    この関数は、$MT1 と $MT2 が等しければ 0、$MT1 が $MT2 より大きければ 
    1、$MT1 が $MT2 より小さければ -1 を返す。

    @latexonly \IPAlabel{mtext_ncmp} @endlatexonly  */

/***
    @seealso
    mtext_cmp (), mtext_casecmp (), mtext_ncasecmp ()
    mtext_compare (), mtext_case_compare ()  */

int
mtext_ncmp (MText *mt1, MText *mt2, int n)
{
  if (n < 0)
    return 0;
  return compare (mt1, 0, (mt1->nchars < n ? mt1->nchars : n),
		  mt2, 0, (mt2->nchars < n ? mt2->nchars : n));
}

/*=*/

/***en
    @brief Compare specified regions of two M-texts.

    The mtext_compare () function compares two M-texts $MT1 and $MT2,
    character-by-character.  The compared regions are between $FROM1
    and $TO1 in $MT1 and $FROM2 to $TO2 in MT2.  $FROM1 and $FROM2 are
    inclusive, $TO1 and $TO2 are exclusive.  $FROM1 being equal to
    $TO1 (or $FROM2 being equal to $TO2) means an M-text of length
    zero.  An invalid region specification is regarded as both $FROM1
    and $TO1 (or $FROM2 and $TO2) being 0.

    @return
    This function returns 1, 0, or -1 if $MT1 is found greater than,
    equal to, or less than $MT2, respectively.  Comparison is based on
    character codes.  */

/***ja
    @brief 二つの M-text の指定した領域同士を比較する.

    関数 mtext_compare () は二つの M-text $MT1 と $MT2 
    を文字単位で比較する。比較の対象は $MT1 のうち $FROM1 から $TO1 までと、$MT2 
    のうち $FROM2 から $TO2 までである。$FROM1 と $FROM2 は含まれ、$TO1 
    と $TO2 は含まれない。$FROM1 と $TO1 （あるいは $FROM2 と $TO2 
    ）が等しい場合は長さゼロの M-text を意味する。範囲指定に誤りがある場合は、
    $FROM1 と $TO1 （あるいは $FROM2 と $TO2 ） 両方に 0 が指定されたものとみなす。

    @return
    この関数は、$MT1 と $MT2 が等しければ 0、$MT1 が $MT2 より大きければ
    1 、$MT1 が $MT2 より小さければ -1 を返す。比較は文字コードに基づく。  */

/***
    @seealso
    mtext_cmp (), mtext_ncmp (), mtext_casecmp (), mtext_ncasecmp (),
    mtext_case_compare ()  */

int
mtext_compare (MText *mt1, int from1, int to1, MText *mt2, int from2, int to2)
{
  if (from1 < 0 || from1 > to1 || to1 > mt1->nchars)
    from1 = to1 = 0;

  if (from2 < 0 || from2 > to2 || to2 > mt2->nchars)
    from2 = to2 = 0;

  return compare (mt1, from1, to1, mt2, from2, to2);
}

/*=*/

/***en
    @brief Search an M-text for a set of characters.

    The mtext_spn () function returns the length of the initial
    segment of M-text $MT1 that consists entirely of characters in
    M-text $MT2.  */

/***ja
    @brief ある集合の文字を M-text の中で探す.

    関数 mtext_spn () は、M-text $MT1 の先頭から M-text $MT2 
    に含まれる文字だけでできている部分の長さを返す。

    @latexonly \IPAlabel{mtext_spn} @endlatexonly  */

/***
    @seealso
    mtext_cspn ()  */

int
mtext_spn (MText *mt, MText *accept)
{
  return span (mt, accept, 0, Mnil);
}

/*=*/

/***en
    @brief Search an M-text for the complement of a set of characters.

    The mtext_cspn () returns the length of the initial segment of
    M-text $MT1 that consists entirely of characters not in M-text $MT2.  */

/***ja
    @brief ある集合に属さない文字を M-text の中で探す.

    関数 mtext_cspn () は、M-text $MT1 の先頭部分で M-text $MT2
    に含まれない文字だけでできている部分の長さを返す。

    @latexonly \IPAlabel{mtext_cspn} @endlatexonly  */

/***
    @seealso
    mtext_spn ()  */

int
mtext_cspn (MText *mt, MText *reject)
{
  return span (mt, reject, 0, Mt);
}

/*=*/

/***en
    @brief Search an M-text for any of a set of characters.

    The mtext_pbrk () function locates the first occurrence in M-text
    $MT1 of any of the characters in M-text $MT2.

    @return
    This function returns the position in $MT1 of the found character.
    If no such character is found, it returns -1. */

/***ja
    @brief ある集合に属す文字を M-text の中から探す.

    関数 mtext_pbrk () は、M-text $MT1 中で M-text $MT2 
    の文字のどれかが最初に現れる位置を調べる。

    @return 
    見つかった文字の、$MT1 
    内における出現位置を返す。もしそのような文字がなければ -1 を返す。

    @latexonly \IPAlabel{mtext_pbrk} @endlatexonly  */

int
mtext_pbrk (MText *mt, MText *accept)
{
  int nchars = mtext_nchars (mt);
  int len = span (mt, accept, 0, Mt);

  return (len == nchars ? -1 : len);
}

/*=*/

/***en
    @brief Look for a token in an M-text.

    The mtext_tok () function searches a token that firstly occurs
    after position $POS in M-text $MT.  Here, a token means a
    substring each of which does not appear in M-text $DELIM.  Note
    that the type of $POS is not @c int but pointer to @c int.

    @return
    If a token is found, mtext_tok () copies the corresponding part of
    $MT and returns a pointer to the copy.  In this case, $POS is set
    to the end of the found token.  If no token is found, it returns
    @c NULL without changing the external variable #merror_code.  If an
    error is detected, it returns @c NULL and assigns an error code
    to the external variable #merror_code. */

/***ja
    @brief M-text 中のトークンを探す.

    関数 mtext_tok () は、M-text $MT の中で位置 $POS 
    以降最初に現れるトークンを探す。ここでトークンとは M-text $DELIM
    の中に現われない文字だけからなる部分文字列である。$POS の型が @c int ではなくて @c
    int へのポインタであることに注意。

    @return
    もしトークンが見つかれば mtext_tok ()はそのトークンに相当する部分の 
    $MT をコピーし、そのコピーへのポインタを返す。この場合、$POS 
    は見つかったトークンの終端にセットされる。トークンが見つからなかった場合は外部変数
    #merror_code を変えずに @c NULL を返す。エラーが検出された場合は 
    @c NULL を返し、変部変数 #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_tok} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE  */

MText *
mtext_tok (MText *mt, MText *delim, int *pos)
{
  int nchars = mtext_nchars (mt);
  int pos2;

  M_CHECK_POS (mt, *pos, NULL);

  /*
    Skip delimiters starting at POS in MT.
    Never do *pos += span(...), or you will change *pos
    even though no token is found.
   */
  pos2 = *pos + span (mt, delim, *pos, Mnil);

  if (pos2 == nchars)
    return NULL;

  *pos = pos2 + span (mt, delim, pos2, Mt);
  return (insert (mtext (), 0, mt, pos2, *pos));
}

/*=*/

/***en
    @brief Locate an M-text in another.

    The mtext_text () function finds the first occurrence of M-text
    $MT2 in M-text $MT1 after the position $POS while ignoring
    difference of the text properties.

    @return
    If $MT2 is found in $MT1, mtext_text () returns the position of it
    first occurrence.  Otherwise it returns -1.  If $MT2 is empty, it
    returns 0.  */

/***ja
    @brief M-text 中で別の M-text を探す.

    関数 mtext_text () は、M-text $MT1 中で位置 $POS 以降に現われる 
    M-text $MT2 の最初の位置を調べる。テキストプロパティの違いは無視される。

    @return
    $MT1 中に $MT2 が見つかれば、mtext_text() 
    はその最初の出現位置を返す。見つからない場合は -1 を返す。もし $MT2 が空ならば 0 を返す。

    @latexonly \IPAlabel{mtext_text} @endlatexonly  */

int
mtext_text (MText *mt1, int pos, MText *mt2)
{
  int from = pos;
  int pos_byte = POS_CHAR_TO_BYTE (mt1, pos);
  int c = mtext_ref_char (mt2, 0);
  int nbytes1 = mtext_nbytes (mt1);
  int nbytes2 = mtext_nbytes (mt2);
  int limit;
  int use_memcmp = (mt1->format == mt2->format
		    || (mt1->format < MTEXT_FORMAT_UTF_8
			&& mt2->format == MTEXT_FORMAT_UTF_8));
  int unit_bytes = UNIT_BYTES (mt1->format);

  if (nbytes2 > pos_byte + nbytes1)
    return -1;
  pos_byte = nbytes1 - nbytes2;
  limit = POS_BYTE_TO_CHAR (mt1, pos_byte);

  while (1)
    {
      if ((pos = mtext_character (mt1, from, limit, c)) < 0)
	return -1;
      pos_byte = POS_CHAR_TO_BYTE (mt1, pos);
      if (use_memcmp
	  ? ! memcmp (mt1->data + pos_byte * unit_bytes, 
		      mt2->data, nbytes2 * unit_bytes)
	  : ! compare (mt1, pos, mt2->nchars, mt2, 0, mt2->nchars))
	break;
      from = pos + 1;
    }
  return pos;
}

/***en
    @brief Locate an M-text in a specific range of another.

    The mtext_search () function searches for the first occurrence of
    M-text $MT2 in M-text $MT1 in the region $FROM and $TO while
    ignoring difference of the text properties.  If $FROM is less than
    $TO, the forward search starts from $FROM, otherwise the backward
    search starts from $TO.

    @return
    If $MT2 is found in $MT1, mtext_search () returns the position of the
    first occurrence.  Otherwise it returns -1.  If $MT2 is empty, it
    returns 0.  */

/***ja
    @brief M-text 中の特定の領域で別の M-text を探す.

    関数 mtext_search () は、M-text $MT1 中の $FROM から $TO 
    までの間の領域でM-text $MT2 
    が最初に現われる位置を調べる。テキストプロパティの違いは無視される。もし
    $FROM が $TO より小さければ探索は位置 $FROM から末尾方向へ、そうでなければ
    $TO から先頭方向へ進む。

    @return
    $MT1 中に $MT2 が見つかれば、mtext_search() 
    はその最初の出現位置を返す。見つからない場合は -1 を返す。もし $MT2 が空ならば 0 を返す。
    */

int
mtext_search (MText *mt1, int from, int to, MText *mt2)
{
  int c = mtext_ref_char (mt2, 0);
  int from_byte;
  int nbytes2 = mtext_nbytes (mt2);

  if (mt1->format > MTEXT_FORMAT_UTF_8
      || mt2->format > MTEXT_FORMAT_UTF_8)
    MERROR (MERROR_MTEXT, -1);

  if (from < to)
    {
      to -= mtext_nchars (mt2);
      if (from > to)
	return -1;
      while (1)
	{
	  if ((from = find_char_forward (mt1, from, to, c)) < 0)
	    return -1;
	  from_byte = POS_CHAR_TO_BYTE (mt1, from);
	  if (! memcmp (mt1->data + from_byte, mt2->data, nbytes2))
	    break;
	  from++;
	}
    }
  else if (from > to)
    {
      from -= mtext_nchars (mt2);
      if (from < to)
	return -1;
      while (1)
	{
	  if ((from = find_char_backward (mt1, from, to, c)) < 0)
	    return -1;
	  from_byte = POS_CHAR_TO_BYTE (mt1, from);
	  if (! memcmp (mt1->data + from_byte, mt2->data, nbytes2))
	    break;
	  from--;
	}
    }

  return from;
}

/*=*/

/***en
    @brief Compare two M-texts ignoring cases.

    The mtext_casecmp () function is similar to mtext_cmp (), but
    ignores cases on comparison.

    @return
    This function returns 1, 0, or -1 if $MT1 is found greater than,
    equal to, or less than $MT2, respectively.  */

/***ja
    @brief 二つの M-text を大文字／小文字の区別を無視して比較する.

    関数 mtext_casecmp () は、関数 mtext_cmp () 同様の M-text 
    同士の比較を、大文字／小文字の区別を無視して行なう。

    @return
    この関数は、$MT1 と $MT2 が等しければ 0、$MT1 が $MT2 
    より大きければ 1、$MT1 が $MT2 より小さければ -1 を返す。

    @latexonly \IPAlabel{mtext_casecmp} @endlatexonly  */

/***
    @seealso
    mtext_cmp (), mtext_ncmp (), mtext_ncasecmp ()
    mtext_compare (), mtext_case_compare ()  */

int
mtext_casecmp (MText *mt1, MText *mt2)
{
  return case_compare (mt1, 0, mt1->nchars, mt2, 0, mt2->nchars);
}

/*=*/

/***en
    @brief Compare initial parts of two M-texts ignoring cases.

    The mtext_ncasecmp () function is similar to mtext_casecmp (), but
    compares at most $N characters from the beginning.

    @return
    This function returns 1, 0, or -1 if $MT1 is found greater than,
    equal to, or less than $MT2, respectively.  */

/***ja
    @brief 二つの M-text の先頭部分を大文字／小文字の区別を無視して比較する.

    関数 mtext_ncasecmp () は、関数 mtext_casecmp () 同様の M-text 
    同士の比較を先頭から最大 $N 文字までに関して行なう。

    @return
    この関数は、$MT1 と $MT2 が等しければ 0、$MT1 が $MT2 
    より大きければ 1、$MT1 が $MT2 より小さければ -1 を返す。

    @latexonly \IPAlabel{mtext_ncasecmp} @endlatexonly  */

/***
    @seealso
    mtext_cmp (), mtext_casecmp (), mtext_casecmp ()
    mtext_compare (), mtext_case_compare ()  */

int
mtext_ncasecmp (MText *mt1, MText *mt2, int n)
{
  if (n < 0)
    return 0;
  return case_compare (mt1, 0, (mt1->nchars < n ? mt1->nchars : n),
		       mt2, 0, (mt2->nchars < n ? mt2->nchars : n));
}

/*=*/

/***en
    @brief Compare specified regions of two M-texts ignoring cases. 

    The mtext_case_compare () function compares two M-texts $MT1 and
    $MT2, character-by-character, ignoring cases.  The compared
    regions are between $FROM1 and $TO1 in $MT1 and $FROM2 to $TO2 in
    MT2.  $FROM1 and $FROM2 are inclusive, $TO1 and $TO2 are
    exclusive.  $FROM1 being equal to $TO1 (or $FROM2 being equal to
    $TO2) means an M-text of length zero.  An invalid region
    specification is regarded as both $FROM1 and $TO1 (or $FROM2 and
    $TO2) being 0.

    @return
    This function returns 1, 0, or -1 if $MT1 is found greater than,
    equal to, or less than $MT2, respectively.  Comparison is based on
    character codes.  */

/***ja 
    @brief 二つの M-text の指定した領域を、大文字／小文字の区別を無視して比較する.

    関数 mtext_compare () は二つの M-text $MT1 と $MT2 
    を、大文字／小文字の区別を無視して文字単位で比較する。比較の対象は $MT1 
    の $FROM1 から $TO1 まで、$MT2 の $FROM2 から $TO2 までである。
    $FROM1 と $FROM2 は含まれ、$TO1 と $TO2 は含まれない。$FROM1 と $TO1
    （あるいは $FROM2 と $TO2 ）が等しい場合は長さゼロの M-text 
    を意味する。範囲指定に誤りがある場合は、$FROM1 と $TO1 （あるいは 
    $FROM2 と $TO2 ）両方に 0 が指定されたものと見なす。

    @return
    この関数は、$MT1 と $MT2 が等しければ 0、$MT1 が $MT2 より大きければ
    1、$MT1 が $MT2 より小さければ -1を返す。比較は文字コードに基づく。

  @latexonly \IPAlabel{mtext_case_compare} @endlatexonly
*/

/***
    @seealso
    mtext_cmp (), mtext_ncmp (), mtext_casecmp (), mtext_ncasecmp (),
    mtext_compare ()  */

int
mtext_case_compare (MText *mt1, int from1, int to1,
		    MText *mt2, int from2, int to2)
{
  if (from1 < 0 || from1 > to1 || to1 > mt1->nchars)
    from1 = to1 = 0;

  if (from2 < 0 || from2 > to2 || to2 > mt2->nchars)
    from2 = to2 = 0;

  return case_compare (mt1, from1, to1, mt2, from2, to2);
}

/*** @} */

#include <stdio.h>

/*** @addtogroup m17nDebug */
/*=*/
/*** @{  */

/***en
    @brief Dump an M-text.

    The mdebug_dump_mtext () function prints the M-text $MT in a human
    readable way to the stderr.  $INDENT specifies how many columns to
    indent the lines but the first one.  If $FULLP is zero, this
    function prints only a character code sequence.  Otherwise, it
    prints the internal byte sequence and text properties as well.

    @return
    This function returns $MT.  */
/***ja
    @brief M-text をダンプする.

    関数 mdebug_dump_mtext () は M-text $MT を stderr 
    に人間に可読な形で印刷する。 $INDENT は２行目以降のインデントを指定する。
    $FULLP が 0 ならば、文字コード列だけを印刷する。
    そうでなければ、内部バイト列とテキストプロパティも印刷する。

    @return
    この関数は $MT を返す。  */

MText *
mdebug_dump_mtext (MText *mt, int indent, int fullp)
{
  char *prefix = (char *) alloca (indent + 1);
  int i;
  unsigned char *p;

  memset (prefix, 32, indent);
  prefix[indent] = 0;

  fprintf (stderr,
	   "(mtext (size %d %d %d) (cache %d %d)",
	   mt->nchars, mt->nbytes, mt->allocated,
	   mt->cache_char_pos, mt->cache_byte_pos);
  if (! fullp)
    {
      fprintf (stderr, " \"");
      for (i = 0; i < mt->nchars; i++)
	{
	  int c = mtext_ref_char (mt, i);
	  if (c >= ' ' && c < 127)
	    fprintf (stderr, "%c", c);
	  else
	    fprintf (stderr, "\\x%02X", c);
	}
      fprintf (stderr, "\"");
    }
  else if (mt->nchars > 0)
    {
      fprintf (stderr, "\n%s (bytes \"", prefix);
      for (i = 0; i < mt->nbytes; i++)
	fprintf (stderr, "\\x%02x", mt->data[i]);
      fprintf (stderr, "\")\n");
      fprintf (stderr, "%s (chars \"", prefix);
      p = mt->data;
      for (i = 0; i < mt->nchars; i++)
	{
	  int len;
	  int c = STRING_CHAR_AND_BYTES (p, len);

	  if (c >= ' ' && c < 127 && c != '\\' && c != '\"')
	    fputc (c, stderr);
	  else
	    fprintf (stderr, "\\x%X", c);
	  p += len;
	}
      fprintf (stderr, "\")");
      if (mt->plist)
	{
	  fprintf (stderr, "\n%s ", prefix);
	  dump_textplist (mt->plist, indent + 1);
	}
    }
  fprintf (stderr, ")");
  return mt;
}

/*** @} */ 

/*
  Local Variables:
  coding: euc-japan
  End:
*/
