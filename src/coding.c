/* coding.c -- code conversion module.
   Copyright (C) 2003, 2004, 2005, 2007, 2008, 2009, 2010, 2011, 2012
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
   Boston, MA 02110-1301 USA.  */

/***en
    @addtogroup m17nConv
    @brief Coding system objects and API for them.

    The m17n library represents a character encoding scheme (CES) of
    coded character sets (CCS) as an object called @e coding @e
    system.  Application programs can add original coding systems.

    To @e encode means converting code-points to character codes and
    to @e decode means converting character codes back to code-points.

    Application programs can decode a byte sequence with a specified
    coding system into an M-text, and inversely, can encode an M-text
    into a byte sequence.  */

/***ja
    @addtogroup m17nConv
    @brief コード系オブジェクトとそれに関する API.

    m17n ライブラリは、符号化文字集合 (coded character set; CCS) 
    の文字符合化方式 (character encoding scheme; CES) を @e コード系 
    と呼ぶオブジェクトで表現する。
    アプリケーションプログラムは独自にコード系を追加することもできる。

    コードポイントから文字コードへの変換を @e エンコード 
    と呼び、文字コードからコードポイントへの変換を @e デコード と呼ぶ。

    アプリケーションプログラムは、指定されたコード系でバイト列をデコードすることによって
    M-text を得ることができる。また逆に、指定されたコード系で M-text 
    をエンコードしすることによってバイト列を得ることができる。  */

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "plist.h"
#include "character.h"
#include "charset.h"
#include "coding.h"
#include "mtext.h"
#include "symbol.h"
#include "mlocale.h"

#define NUM_SUPPORTED_CHARSETS 32

/** Structure for coding system object.  */

typedef struct
{
  /** Name of the coding system.  */
  MSymbol name;

  /** Type of the coding system.  */
  MSymbol type;

  /* Number of supported charsets.  */
  int ncharsets;

  /** Array of supported charsets.  */
  MCharset *charsets[NUM_SUPPORTED_CHARSETS];

  /** If non-NULL, function to call at the time of creating and
      reseting a converter.  */
  int (*resetter) (MConverter *converter);

  int (*decoder) (const unsigned char *str, int str_bytes, MText *mt,
		  MConverter *converter);

  int (*encoder) (MText *mt, int from, int to,
		  unsigned char *str, int str_bytes,
		  MConverter *converter);

  /** If non-zero, the coding system decode/encode ASCII characters as
      is.  */
  int ascii_compatible;

  /** Pointer to extra information given when the coding system is
      defined.  The meaning depends on <type>.  */
  void *extra_info;

  /** Pointer to information referred on conversion.  The meaning
      depends on <type>.  The value NULL means that the coding system
      is not yet setup.  */
  void *extra_spec;

  int ready;
} MCodingSystem;

struct MCodingList
{
  int size, inc, used;
  MCodingSystem **codings;
};

static struct MCodingList coding_list;

static MPlist *coding_definition_list;

typedef struct {
  /**en
     Pointer to a structure of a coding system.  */
  /**ja
     コード系を表わすデータ構造へのポインタ */
  MCodingSystem *coding;

  /**en
     Buffer for carryover bytes generated while decoding. */
  /**ja
     デコード中のキャリィオーバーバイト用バッファ */
  unsigned char carryover[256];

  /**en
     Number of carryover bytes. */
  /**ja
     キャリィオーバーバイト数 */
  int carryover_bytes;

  /**en
     Beginning of the byte sequence bound to this converter. */
  /**ja
     このコンバータに結び付けられたバイト列の先頭位置 */
  union {
    const unsigned char *in;
    unsigned char *out;
  } buf;

  /**en
     Size of buf. */
  /**ja
     buf の大きさ */
  int bufsize;

  /**en
     Number of bytes already consumed in buf. */
  /**ja
     buf 内ですでに消費されたバイト数 */
  int used;

  /**en
     Stream bound to this converter. */
  /**ja
     このコンバータに結び付けられたストリーム */
  FILE *fp;

  /**en
     Which of above two is in use. */
  /**ja
     上記2者のいずれが使われているか */
  int binding;

  /**en
     Buffer for unget. */
  /**ja
     Unget 用バッファ */
  MText *unread;

  /**en
    Working area. */
  /**ja
    作業領域 */
  MText *work_mt;

  int seekable;
} MConverterStatus;



/* Local macros and functions.  */

/** At first, set SRC_BASE to SRC.  Then check if we have already
    produced AT_MOST chars.  If so, set SRC_END to SRC, and jump to
    source_end.  Otherwise, get one more byte C from SRC.  In that
    case, if SRC == SRC_END, jump to the label source_end.  */

#define ONE_MORE_BASE_BYTE(c)		\
  do {					\
    src_base = src;			\
    if (nchars == at_most)		\
      {					\
	src_end = src;			\
	goto source_end;		\
      }					\
    if (src == src_stop)		\
      {					\
	if (src == src_end)		\
	  goto source_end;		\
	src_base = src = source;	\
	if (src == src_end)		\
	  goto source_end;		\
	src_stop = src_end;		\
      }					\
    (c) = *src++;			\
  } while (0)


/** Get one more byte C from SRC.  If SRC == SRC_END, jump to the
   label source_end.  */

#define ONE_MORE_BYTE(c)	\
  do {				\
    if (src == src_stop)	\
      {				\
	if (src == src_end)	\
	  goto source_end;	\
	src = source;		\
	if (src == src_end)	\
	  goto source_end;	\
	src_stop = src_end;	\
      }				\
    (c) = *src++;		\
  } while (0)


#define REWIND_SRC_TO_BASE()						\
  do {									\
    if (src_base < source || src_base >= src_end)			\
      src_stop = internal->carryover + internal->carryover_bytes;	\
    src = src_base;							\
  } while (0)


/** Push back byte C to SRC.  */

#define UNGET_ONE_BYTE(c)		\
  do {					\
    if (src > source)			\
      src--;				\
    else				\
      {					\
	internal->carryover[0] = c;	\
	internal->carryover_bytes = 1;	\
	src = internal->carryover;	\
	src_stop = src + 1;		\
      }					\
  } while  (0);


/** Store multibyte representation of character C at DST and increment
    DST to the next of the produced bytes.  DST must be a pointer to
    data area of M-text MT.  If the produced bytes are going to exceed
    DST_END, enlarge the data area of MT.  */

#define EMIT_CHAR(c)						\
  do {								\
    int bytes = CHAR_BYTES (c);					\
    int len;							\
								\
    if (dst + bytes + 1 > dst_end)				\
      {								\
	len = dst - mt->data;					\
	bytes = mt->allocated + bytes + (src_stop - src);	\
	mtext__enlarge (mt, bytes);				\
	dst = mt->data + len;					\
	dst_end = mt->data + mt->allocated;			\
      }								\
    dst += CHAR_STRING (c, dst);				\
    nchars++;							\
  } while (0)


/* Check if there is enough room to produce LEN bytes at DST.  If not,
   go to the label insufficient_destination.  */

#define CHECK_DST(len)			\
  do {					\
    if (dst + (len) > dst_end)		\
      goto insufficient_destination;	\
  } while (0)


/** Take NUM_CHARS characters (NUM_BYTES bytes) already stored at
    (MT->data + MT->nbytes) into MT, and put charset property on
    them with CHARSET->name.  */

#define TAKEIN_CHARS(mt, num_chars, num_bytes, charset)			\
  do {									\
    int chars = (num_chars);						\
									\
    if (chars > 0)							\
      {									\
	mtext__takein ((mt), chars, (num_bytes));			\
	if (charset)							\
	  mtext_put_prop ((mt), (mt)->nchars - chars, (mt)->nchars,	\
			  Mcharset, (void *) ((charset)->name));	\
      }									\
  } while (0)


#define SET_SRC(mt, format, from, to)					\
  do {									\
    if (format <= MTEXT_FORMAT_UTF_8)					\
      {									\
	src = mt->data + POS_CHAR_TO_BYTE (mt, from);			\
	src_end = mt->data + POS_CHAR_TO_BYTE (mt, to);			\
      }									\
    else if (format <= MTEXT_FORMAT_UTF_16BE)				\
      {									\
	src								\
	  = mt->data + (sizeof (short)) * POS_CHAR_TO_BYTE (mt, from);	\
	src_end								\
	  = mt->data + (sizeof (short)) * POS_CHAR_TO_BYTE (mt, to);	\
      }									\
    else								\
      {									\
	src = mt->data + (sizeof (int)) * from;				\
	src_end = mt->data + (sizeof (int)) * to;			\
      }									\
  } while (0)


#define ONE_MORE_CHAR(c, bytes, format)				\
  do {								\
    if (src == src_end)						\
      goto finish;						\
    if (format <= MTEXT_FORMAT_UTF_8)				\
      c = STRING_CHAR_AND_BYTES (src, bytes);			\
    else if (format <= MTEXT_FORMAT_UTF_16BE)			\
      {								\
	c = mtext_ref_char (mt, from++);			\
	bytes = (sizeof (short)) * CHAR_UNITS_UTF16 (c);	\
      }								\
    else							\
      {								\
	c = ((unsigned *) (mt->data))[from++];			\
	bytes = sizeof (int);					\
      }								\
  } while (0)


static int
encode_unsupporeted_char (int c, unsigned char *dst, unsigned char *dst_end,
			  MText *mt, int pos)
{
  int len;
  char *format;

  len = c < 0x10000 ? 8 : 10;
  if (dst + len > dst_end)
    return 0;

  mtext_put_prop (mt, pos, pos + 1, Mcoding, Mnil);
  format = (c < 0xD800 ? "<U+%04X>"
	    : c < 0xE000 ? "<M+%04X>"
	    : c < 0x10000 ? "<U+%04X>"
	    : c < 0x110000 ? "<U+%06X>"
	    : "<M+%06X>");
  sprintf ((char *) dst, format, c);
  return len;
}



/** Finish decoding of bytes at SOURCE (ending at SRC_END) into NCHARS
    characters by CONVERTER into M-text MT.  SRC is a pointer to the
    not-yet processed bytes.  ERROR is 1 if an invalid byte was
    found.  */

static int
finish_decoding (MText *mt, MConverter *converter, int nchars,
		 const unsigned char *source, const unsigned char *src_end,
		 const unsigned char *src,
		 int error)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;

  if (src == src_end)
    internal->carryover_bytes = 0;
  else if (error
	   || (converter->last_block
	       && ! converter->lenient))
    converter->result = MCONVERSION_RESULT_INVALID_BYTE;
  else if (! converter->last_block)
    {
      unsigned char *dst = internal->carryover;

      if (src < source || src > src_end)
	{
	  dst += internal->carryover_bytes;
	  src = source;
	}
      while (src < src_end)
	*dst++ = *src++;
      internal->carryover_bytes = dst - internal->carryover;
      converter->result = MCONVERSION_RESULT_INSUFFICIENT_SRC;
    }
  else
    {
      unsigned char *dst = mt->data + mt->nbytes;
      unsigned char *dst_end = mt->data + mt->allocated;
      const unsigned char *src_stop = src_end;
      int c;
      int last_nchars = nchars;

      if (src < source || src > src_end)
	src_stop = internal->carryover + internal->carryover_bytes;
      while (1)
	{
	  if (converter->at_most && nchars == converter->at_most)
	    break;
	  if (src == src_stop)
	    {
	      if (src == src_end)
		break;
	      src = source;
	      if (src == src_end)
		break;
	      src_stop = src_end;
	    }
	  c = *src++;
	  EMIT_CHAR (c);
	}
      TAKEIN_CHARS (mt, nchars - last_nchars, dst - (mt->data + mt->nbytes),
		    mcharset__binary);
      internal->carryover_bytes = 0;
    }

  converter->nchars += nchars;
  converter->nbytes += ((src < source || src > src_end) ? 0 : src - source);
  return (converter->result == MCONVERSION_RESULT_INVALID_BYTE ? -1 : 0);
}



/* Staffs for coding-systems of type MCODING_TYPE_CHARSET.  */

static int
setup_coding_charset (MCodingSystem *coding)
{
  int ncharsets = coding->ncharsets;
  unsigned *code_charset_table;

  if (ncharsets > 1)
    {
      /* At first, reorder charset list by dimensions (a charset of
	 smaller dimension comes first).  As the number of charsets is
	 usually very small (at most 32), we do a simple sort.  */
      MCharset **charsets;
      int idx = 0;
      int i, j;

      MTABLE_ALLOCA (charsets, NUM_SUPPORTED_CHARSETS, MERROR_CODING);
      memcpy (charsets, coding->charsets,
	      sizeof (MCharset *) * NUM_SUPPORTED_CHARSETS);
      for (i = 0; i < 4; i++)
	for (j = 0; j < ncharsets; j++)
	  if (charsets[j]->dimension == i)
	    coding->charsets[idx++] = charsets[j];
    }

  MTABLE_CALLOC (code_charset_table, 256, MERROR_CODING);
  while (ncharsets--)
    {
      int dim = coding->charsets[ncharsets]->dimension;
      int from = coding->charsets[ncharsets]->code_range[(dim - 1) * 4];
      int to = coding->charsets[ncharsets]->code_range[(dim - 1) * 4 + 1];

      if (coding->charsets[ncharsets]->ascii_compatible)
	coding->ascii_compatible = 1;
      while (from <= to)
	code_charset_table[from++] |= 1 << ncharsets;
    }

  coding->extra_spec = (void *) code_charset_table;
  return 0;
}

static int
reset_coding_charset (MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;

  if (! coding->ready
      && setup_coding_charset (coding) < 0)
    return -1;
  coding->ready = 1;
  return 0;
}

static int
decode_coding_charset (const unsigned char *source, int src_bytes, MText *mt,
		       MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;
  const unsigned char *src = internal->carryover;
  const unsigned char *src_stop = src + internal->carryover_bytes;
  const unsigned char *src_end = source + src_bytes;
  const unsigned char *src_base;
  unsigned char *dst = mt->data + mt->nbytes;
  unsigned char *dst_end = mt->data + mt->allocated;
  int nchars = 0;
  int last_nchars = 0;
  int at_most = converter->at_most > 0 ? converter->at_most : -1;

  unsigned *code_charset_table = (unsigned *) coding->extra_spec;
  MCharset **charsets = coding->charsets;
  MCharset *charset = mcharset__ascii;
  int error = 0;

  while (1)
    {
      MCharset *this_charset = NULL;
      int c;
      unsigned mask;

      ONE_MORE_BASE_BYTE (c);
      mask = code_charset_table[c];
      if (mask)
	{
	  int idx = 0;
	  unsigned code = c;
	  int nbytes = 1;
	  int dim;
	  
	  while (mask)
	    {
	      while (! (mask & 1)) mask >>= 1, idx++;
	      this_charset = charsets[idx];
	      dim = this_charset->dimension;
	      while (nbytes < dim)
		{
		  ONE_MORE_BYTE (c);
		  code = (code << 8) | c;
		  nbytes++;
		}
	      c = DECODE_CHAR (this_charset, code);
	      if (c >= 0)
		goto emit_char;
	      mask >>= 1, idx++;
	    }
	}

      if (! converter->lenient)
	break;
      REWIND_SRC_TO_BASE ();
      c = *src++;
      this_charset = mcharset__binary;

    emit_char:
      if (this_charset != mcharset__ascii
	  && this_charset != charset)
	{
	  TAKEIN_CHARS (mt, nchars - last_nchars, 
			dst - (mt->data + mt->nbytes), charset);
	  charset = this_charset;
	  last_nchars = nchars;
	}
      EMIT_CHAR (c);
    }
  /* We reach here because of an invalid byte.  */
  error = 1;

 source_end:
  TAKEIN_CHARS (mt, nchars - last_nchars,
		dst - (mt->data + mt->nbytes), charset);
  return finish_decoding (mt, converter, nchars,
			  source, src_end, src_base, error);
}

static int
encode_coding_charset (MText *mt, int from, int to,
		       unsigned char *destination, int dst_bytes,
		       MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;
  unsigned char *src, *src_end;
  unsigned char *dst = destination;
  unsigned char *dst_end = dst + dst_bytes;
  int nchars = 0;
  int ncharsets = coding->ncharsets;
  MCharset **charsets = coding->charsets;
  int ascii_compatible = coding->ascii_compatible;
  enum MTextFormat format = mt->format;

  SET_SRC (mt, format, from, to);
  while (1)
    {
      int c, bytes;

      ONE_MORE_CHAR (c, bytes, format);

      if (c < 0x80 && ascii_compatible)
	{
	  CHECK_DST (1);
	  *dst++ = c;
	}
      else
	{
	  unsigned code;
	  MCharset *charset = NULL;
	  int i = 0;

	  while (1)
	    {
	      charset = charsets[i];
	      code = ENCODE_CHAR (charset, c);
	      if (code != MCHAR_INVALID_CODE)
		break;
	      if (++i == ncharsets)
		goto unsupported_char;
	    }

	  CHECK_DST (charset->dimension);
	  if (charset->dimension == 1)
	    {
	      *dst++ = code;
	    }
	  else if (charset->dimension == 2)
	    {
	      *dst++ = code >> 8;
	      *dst++ = code & 0xFF;
	    }
	  else if (charset->dimension == 3)
	    {
	      *dst++ = code >> 16;
	      *dst++ = (code >> 8) & 0xFF;
	      *dst++ = code & 0xFF;
	    }
	  else
	    {
	      *dst++ = code >> 24;
	      *dst++ = (code >> 16) & 0xFF;
	      *dst++ = (code >> 8) & 0xFF;
	      *dst++ = code & 0xFF;
	    }
	}
      src += bytes;
      nchars++;
      continue;

    unsupported_char:
      {
	int len;

	if (! converter->lenient)
	  break;
	len = encode_unsupporeted_char (c, dst, dst_end, mt, from + nchars);
	if (len == 0)
	  goto insufficient_destination;
	dst += len;
	src += bytes;
	nchars++;
      }
    }
  /* We reach here because of an unsupported char.  */
  converter->result = MCONVERSION_RESULT_INVALID_CHAR;
  goto finish;

 insufficient_destination:
  converter->result = MCONVERSION_RESULT_INSUFFICIENT_DST;

 finish:
  converter->nchars += nchars;
  converter->nbytes += dst - destination;
  return (converter->result == MCONVERSION_RESULT_INVALID_CHAR ? -1 : 0);
}


/* Staffs for coding-systems of type MCODING_TYPE_UTF (8).  */

#define UTF8_CHARSET(p)					\
  (! ((p)[0] & 0x80) ? (mcharset__unicode)			\
   : CHAR_HEAD_P ((p) + 1) ? (mcharset__binary)			\
   : ! ((p)[0] & 0x20) ? (mcharset__unicode)			\
   : CHAR_HEAD_P ((p) + 2) ? (mcharset__binary)			\
   : ! ((p)[0] & 0x10) ? (mcharset__unicode)			\
   : CHAR_HEAD_P ((p) + 3) ? (mcharset__binary)			\
   : ! ((p)[0] & 0x08) ? ((((((p)[0] & 0x07) << 2)		\
			    & (((p)[1] & 0x30) >> 4)) <= 0x10)	\
			  ? (mcharset__unicode)			\
			  : (mcharset__m17n))			\
   : CHAR_HEAD_P ((p) + 4) ? (mcharset__binary)			\
   : ! ((p)[0] & 0x04) ? (mcharset__m17n)			\
   : CHAR_HEAD_P ((p) + 5) ? (mcharset__binary)			\
   : ! ((p)[0] & 0x02) ? (mcharset__m17n)			\
   : (mcharset__binary))


static int
decode_coding_utf_8 (const unsigned char *source, int src_bytes, MText *mt,
		     MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;
  const unsigned char *src = internal->carryover;
  const unsigned char *src_stop = src + internal->carryover_bytes;
  const unsigned char *src_end = source + src_bytes;
  const unsigned char *src_base;
  unsigned char *dst = mt->data + mt->nbytes;
  unsigned char *dst_end = mt->data + mt->allocated;
  int nchars = 0;
  int last_nchars = 0;
  int at_most = converter->at_most > 0 ? converter->at_most : -1;
  int error = 0;
  int full = converter->lenient || (coding->charsets[0] == mcharset__m17n);
  MCharset *charset = NULL;

  while (1)
    {
      int c, c1, bytes;
      MCharset *this_charset = NULL;

      ONE_MORE_BASE_BYTE (c);

      if (!(c & 0x80))
	bytes = 1;
      else if (!(c & 0x40))
	goto invalid_byte;
      else if (!(c & 0x20))
	bytes = 2, c &= 0x1F;
      else if (!(c & 0x10))
	bytes = 3, c &= 0x0F;
      else if (!(c & 0x08))
	bytes = 4, c &= 0x07;
      else if (!(c & 0x04))
	bytes = 5, c &= 0x03;
      else if (!(c & 0x02))
	bytes = 6, c &= 0x01;
      else
	goto invalid_byte;

      while (bytes-- > 1)
	{
	  ONE_MORE_BYTE (c1);
	  if ((c1 & 0xC0) != 0x80)
	    goto invalid_byte;
	  c = (c << 6) | (c1 & 0x3F);
	}

      if (full
	  || c < 0xD800 || (c >= 0xE000 && c < 0x110000))
	goto emit_char;

    invalid_byte:
      if (! converter->lenient)
	break;
      REWIND_SRC_TO_BASE ();
      c = *src++;
      this_charset = mcharset__binary;

    emit_char:
      if (this_charset != charset)
	{
	  TAKEIN_CHARS (mt, nchars - last_nchars, 
			dst - (mt->data + mt->nbytes), charset);
	  charset = this_charset;
	  last_nchars = nchars;
	}
      EMIT_CHAR (c);
    }
  /* We reach here because of an invalid byte.  */
  error = 1;

 source_end:
  TAKEIN_CHARS (mt, nchars - last_nchars,
		dst - (mt->data + mt->nbytes), charset);
  return finish_decoding (mt, converter, nchars,
			  source, src_end, src_base, error);
}

static int
encode_coding_utf_8 (MText *mt, int from, int to,
		     unsigned char *destination, int dst_bytes,
		     MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;
  unsigned char *src, *src_end;
  unsigned char *dst = destination;
  unsigned char *dst_end = dst + dst_bytes;
  int nchars = 0;
  enum MTextFormat format = mt->format;

  SET_SRC (mt, format, from, to);

  if (format <= MTEXT_FORMAT_UTF_8
      && (converter->lenient
	  || coding->charsets[0] == mcharset__m17n))
    {
      if (dst_bytes < src_end - src)
	{
	  int byte_pos = (src + dst_bytes) - mt->data;

	  to = POS_BYTE_TO_CHAR (mt, byte_pos);
	  byte_pos = POS_CHAR_TO_BYTE (mt, to);
	  src_end = mt->data + byte_pos;
	  converter->result = MCONVERSION_RESULT_INSUFFICIENT_DST;
	}
      memcpy (destination, src, src_end - src);
      nchars = to - from;
      dst += src_end - src;
      goto finish;
    }

  while (1)
    {
      int c, bytes;

      ONE_MORE_CHAR (c, bytes, format);

      if ((c >= 0xD800 && c < 0xE000) || c >= 0x110000)
	break;
      CHECK_DST (bytes);
      dst += CHAR_STRING (c, dst);
      src += bytes;
      nchars++;
    }
  /* We reach here because of an unsupported char.  */
  converter->result = MCONVERSION_RESULT_INVALID_CHAR;
  goto finish;

 insufficient_destination:
  converter->result = MCONVERSION_RESULT_INSUFFICIENT_DST;

 finish:
  converter->nchars += nchars;
  converter->nbytes += dst - destination;
  return (converter->result == MCONVERSION_RESULT_INVALID_CHAR ? -1 : 0);
}


/* Staffs for coding-systems of type MCODING_TYPE_UTF (16 & 32).  */

enum utf_bom
  {
    UTF_BOM_MAYBE,
    UTF_BOM_NO,
    UTF_BOM_YES,
    UTF_BOM_MAX
  };

enum utf_endian
  {
    UTF_BIG_ENDIAN,
    UTF_LITTLE_ENDIAN,
    UTF_ENDIAN_MAX
  };

struct utf_status
{
  int surrogate;
  enum utf_bom bom;
  enum utf_endian endian;
};

static int
setup_coding_utf (MCodingSystem *coding)
{
  MCodingInfoUTF *info = (MCodingInfoUTF *) (coding->extra_info);
  MCodingInfoUTF *spec;

  if (info->code_unit_bits == 8)
    coding->ascii_compatible = 1;
  else if (info->code_unit_bits == 16
	   || info->code_unit_bits == 32)
    {
      if (info->bom < 0 || info->bom > 2
	  || info->endian < 0 || info->endian > 1)
	MERROR (MERROR_CODING, -1);
    }
  else
    return -1;

  MSTRUCT_CALLOC (spec, MERROR_CODING);
  *spec = *info;
  coding->extra_spec = (void *) (spec);
  return 0;
}

static int
reset_coding_utf (MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;
  struct utf_status *status = (struct utf_status *) &(converter->status);

  if (! coding->ready
      && setup_coding_utf (coding) < 0)
    return -1;
  coding->ready = 1;

  status->surrogate = 0;
  status->bom = ((MCodingInfoUTF *) (coding->extra_spec))->bom;
  status->endian = ((MCodingInfoUTF *) (coding->extra_spec))->endian;
  return 0;
}

static int
decode_coding_utf_16 (const unsigned char *source, int src_bytes, MText *mt,
		      MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  const unsigned char *src = internal->carryover;
  const unsigned char *src_stop = src + internal->carryover_bytes;
  const unsigned char *src_end = source + src_bytes;
  const unsigned char *src_base;
  unsigned char *dst = mt->data + mt->nbytes;
  unsigned char *dst_end = mt->data + mt->allocated;
  int nchars = 0;
  int last_nchars = 0;
  int at_most = converter->at_most > 0 ? converter->at_most : -1;
  struct utf_status *status = (struct utf_status *) &(converter->status);
  unsigned char b1, b2;
  MCharset *charset = NULL;
  int error = 0;

  if (status->bom != UTF_BOM_NO)
    {
      int c;

      ONE_MORE_BASE_BYTE (b1);
      ONE_MORE_BYTE (b2);
      c = (b1 << 8) | b2;
      if (c == 0xFEFF)
	status->endian = UTF_BIG_ENDIAN;
      else if (c == 0xFFFE)
	status->endian = UTF_LITTLE_ENDIAN;
      else if (status->bom == UTF_BOM_MAYBE
	       || converter->lenient)
	{
	  status->endian = UTF_BIG_ENDIAN;
	  REWIND_SRC_TO_BASE ();
	}
      else
	{
	  error = 1;
	  goto source_end;
	}
      status->bom = UTF_BOM_NO;
    }

  while (1)
    {
      int c, c1;
      MCharset *this_charset = NULL;

      ONE_MORE_BASE_BYTE (b1);
      ONE_MORE_BYTE (b2);
      if (status->endian == UTF_BIG_ENDIAN)
	c = ((b1 << 8) | b2);
      else
	c = ((b2 << 8) | b1);
      if (c < 0xD800 || c >= 0xE000)
	goto emit_char;
      else if (c < 0xDC00)
	{
	  ONE_MORE_BYTE (b1);
	  ONE_MORE_BYTE (b2);
	  if (status->endian == UTF_BIG_ENDIAN)
	    c1 = ((b1 << 8) | b2);
	  else
	    c1 = ((b2 << 8) | b1);
	  if (c1 < 0xDC00 || c1 >= 0xE000)
	    goto invalid_byte;
	  c = 0x10000 + ((c - 0xD800) << 10) + (c1 - 0xDC00);
	  goto emit_char;
	}

    invalid_byte:
      if (! converter->lenient)
	break;
      REWIND_SRC_TO_BASE ();
      ONE_MORE_BYTE (b1);
      ONE_MORE_BYTE (b2);
      if (status->endian == UTF_BIG_ENDIAN)
	c = ((b1 << 8) | b2);
      else
	c = ((b2 << 8) | b1);
      this_charset = mcharset__binary;

    emit_char:
      if (this_charset != charset)
	{
	  TAKEIN_CHARS (mt, nchars - last_nchars, 
			dst - (mt->data + mt->nbytes), charset);
	  charset = this_charset;
	  last_nchars = nchars;
	}
      EMIT_CHAR (c);
    }
  /* We reach here because of an invalid byte.  */
  error = 1;

 source_end:
  TAKEIN_CHARS (mt, nchars - last_nchars,
		dst - (mt->data + mt->nbytes), charset);
  return finish_decoding (mt, converter, nchars,
			  source, src_end, src_base, error);
}


static int
decode_coding_utf_32 (const unsigned char *source, int src_bytes, MText *mt,
		      MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  const unsigned char *src = internal->carryover;
  const unsigned char *src_stop = src + internal->carryover_bytes;
  const unsigned char *src_end = source + src_bytes;
  const unsigned char *src_base;
  unsigned char *dst = mt->data + mt->nbytes;
  unsigned char *dst_end = mt->data + mt->allocated;
  int nchars = 0;
  int last_nchars = 0;
  int at_most = converter->at_most > 0 ? converter->at_most : -1;
  struct utf_status *status = (struct utf_status *) &(converter->status);
  unsigned char b1, b2, b3, b4;
  MCharset *charset = NULL;
  int error = 0;

  if (status->bom != UTF_BOM_NO)
    {
      unsigned c;

      ONE_MORE_BASE_BYTE (b1);
      ONE_MORE_BYTE (b2);
      ONE_MORE_BYTE (b3);
      ONE_MORE_BYTE (b4);
      c = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
      if (c == 0x0000FEFF)
	status->endian = UTF_BIG_ENDIAN;
      else if (c == 0xFFFE0000)
	status->endian = UTF_LITTLE_ENDIAN;
      else if (status->bom == UTF_BOM_MAYBE
	       || converter->lenient)
	{
	  status->endian = UTF_BIG_ENDIAN;
	  REWIND_SRC_TO_BASE ();
	}
      else
	{
	  error = 1;
	  goto source_end;
	}
      status->bom = UTF_BOM_NO;
    }

  while (1)
    {
      unsigned c;
      MCharset *this_charset = NULL;

      ONE_MORE_BASE_BYTE (b1);
      ONE_MORE_BYTE (b2);
      ONE_MORE_BYTE (b3);
      ONE_MORE_BYTE (b4);
      if (status->endian == UTF_BIG_ENDIAN)
	c = (b1 << 24) | (b2 << 16) | (b3 << 8) | b4;
      else
	c = (b4 << 24) | (b3 << 16) | (b2 << 8) | b1;
      if (c < 0xD800 || (c >= 0xE000 && c < 0x110000))
	goto emit_char;

      if (! converter->lenient)
	break;
      REWIND_SRC_TO_BASE ();
      ONE_MORE_BYTE (c);
      this_charset = mcharset__binary;

    emit_char:
      if (this_charset != charset)
	{
	  TAKEIN_CHARS (mt, nchars - last_nchars, 
			dst - (mt->data + mt->nbytes), charset);
	  charset = this_charset;
	  last_nchars = nchars;
	}
      EMIT_CHAR (c);
    }
  /* We reach here because of an invalid byte.  */
  error = 1;

 source_end:
  TAKEIN_CHARS (mt, nchars - last_nchars,
		dst - (mt->data + mt->nbytes), charset);
  return finish_decoding (mt, converter, nchars,
			  source, src_end, src_base, error);
}


static int
encode_coding_utf_16 (MText *mt, int from, int to,
		      unsigned char *destination, int dst_bytes,
		      MConverter *converter)
{
  unsigned char *src, *src_end;
  unsigned char *dst = destination;
  unsigned char *dst_end = dst + dst_bytes;
  int nchars = 0;
  struct utf_status *status = (struct utf_status *) &(converter->status);
  int big_endian = status->endian == UTF_BIG_ENDIAN;
  enum MTextFormat format = mt->format;

  SET_SRC (mt, format, from, to);

  if (status->bom != UTF_BOM_NO)
    {
      CHECK_DST (2);
      if (big_endian)
	*dst++ = 0xFE, *dst++ = 0xFF;
      else
	*dst++ = 0xFF, *dst++ = 0xFE;
      status->bom = UTF_BOM_NO;
    }

  while (1)
    {
      int c, bytes;

      ONE_MORE_CHAR (c, bytes, format);

      if (c < 0xD800 || (c >= 0xE000 && c < 0x10000))
	{
	  CHECK_DST (2);
	  if (big_endian)
	    *dst++ = c >> 8, *dst++ = c & 0xFF;
	  else
	    *dst++ = c & 0xFF, *dst++ = c >> 8;
	}
      else if (c >= 0x10000 && c < 0x110000)
	{
	  int c1, c2;

	  CHECK_DST (4);
	  c -= 0x10000;
	  c1 = (c >> 10) + 0xD800;
	  c2 = (c & 0x3FF) + 0xDC00;
	  if (big_endian)
	    *dst++ = c1 >> 8, *dst++ = c1 & 0xFF,
	      *dst++ = c2 >> 8, *dst++ = c2 & 0xFF;
	  else
	    *dst++ = c1 & 0xFF, *dst++ = c1 >> 8,
	      *dst++ = c2 & 0xFF, *dst++ = c2 >> 8;
	}
      else
	{
	  unsigned char buf[11];
	  int len, i;

	  if (! converter->lenient)
	    break;
	  len = encode_unsupporeted_char (c, buf, buf + (dst_end - dst),
					  mt, from + nchars);
	  if (len == 0)
	    goto insufficient_destination;
	  if (big_endian)
	    for (i = 0; i < len; i++)
	      *dst++ = 0, *dst++ = buf[i];
	  else
	    for (i = 0; i < len; i++)
	      *dst++ = buf[i], *dst++ = 0;
	}
      src += bytes;
      nchars++;
    }
  /* We reach here because of an unsupported char.  */
  converter->result = MCONVERSION_RESULT_INVALID_CHAR;
  goto finish;

 insufficient_destination:
  converter->result = MCONVERSION_RESULT_INSUFFICIENT_DST;

 finish:
  converter->nchars += nchars;
  converter->nbytes += dst - destination;
  return (converter->result == MCONVERSION_RESULT_INVALID_CHAR ? -1 : 0);
}

static int
encode_coding_utf_32 (MText *mt, int from, int to,
		      unsigned char *destination, int dst_bytes,
		      MConverter *converter)
{
  unsigned char *src, *src_end;
  unsigned char *dst = destination;
  unsigned char *dst_end = dst + dst_bytes;
  int nchars = 0;
  struct utf_status *status = (struct utf_status *) &(converter->status);
  int big_endian = status->endian == UTF_BIG_ENDIAN;
  enum MTextFormat format = mt->format;

  SET_SRC (mt, format, from, to);

  if (status->bom != UTF_BOM_NO)
    {
      CHECK_DST (4);
      if (big_endian)
	*dst++ = 0x00, *dst++ = 0x00, *dst++ = 0xFE, *dst++ = 0xFF;
      else
	*dst++ = 0xFF, *dst++ = 0xFE, *dst++ = 0x00, *dst++ = 0x00;
      status->bom = UTF_BOM_NO;
    }

  while (1)
    {
      int c, bytes;

      ONE_MORE_CHAR (c, bytes, format);

      if (c < 0xD800 || (c >= 0xE000 && c < 0x110000))
	{
	  CHECK_DST (4);
	  if (big_endian)
	    *dst++ = 0x00, *dst++ = c >> 16,
	      *dst++ = (c >> 8) & 0xFF, *dst++ = c & 0xFF;
	  else
	    *dst++ = c & 0xFF, *dst++ = (c >> 8) & 0xFF,
	      *dst++ = c >> 16, *dst++ = 0x00;
	}
      else
	{
	  unsigned char buf[11];
	  int len, i;

	  if (! converter->lenient)
	    break;
	  len = encode_unsupporeted_char (c, buf, buf + (dst_end - dst),
					  mt, from + nchars);
	  if (len == 0)
	    goto insufficient_destination;
	  if (big_endian)
	    for (i = 0; i < len; i++)
	      *dst++ = 0, *dst++ = buf[i];
	  else
	    for (i = 0; i < len; i++)
	      *dst++ = buf[i], *dst++ = 0;
	}
      src += bytes;
      nchars++;
    }
  /* We reach here because of an unsupported char.  */
  converter->result = MCONVERSION_RESULT_INVALID_CHAR;
  goto finish;

 insufficient_destination:
  converter->result = MCONVERSION_RESULT_INSUFFICIENT_DST;

 finish:
  converter->nchars += nchars;
  converter->nbytes += dst - destination;
  return (converter->result == MCONVERSION_RESULT_INVALID_CHAR ? -1 : 0);
}


/* Staffs for coding-systems of type MCODING_TYPE_ISO_2022.  */

#define ISO_CODE_STX	0x02		/* start text */
#define ISO_CODE_SO	0x0E		/* shift-out */
#define ISO_CODE_SI	0x0F		/* shift-in */
#define ISO_CODE_SS2_7	0x19		/* single-shift-2 for 7-bit code */
#define ISO_CODE_ESC	0x1B		/* escape */
#define ISO_CODE_SS2	0x8E		/* single-shift-2 */
#define ISO_CODE_SS3	0x8F		/* single-shift-3 */

/** Structure pointed by MCodingSystem.extra_spec.  */

struct iso_2022_spec
{
  unsigned flags;

  /** Initial graphic registers (0..3) invoked to each graphic
      plane left and right. */
  int initial_invocation[2];

  /** Initially designated charsets for each graphic register.  */
  MCharset *initial_designation[4];

  int n_designations;
  char *designations;

  int use_esc;
};

struct iso_2022_status
{
  int invocation[2];
  MCharset *designation[4];
  unsigned single_shifting : 1;
  unsigned bol : 1;
  unsigned r2l : 1;
  unsigned utf8_shifting : 1;
  MCharset *non_standard_charset;
  int non_standard_charset_bytes;
  int non_standard_encoding;
};

enum iso_2022_code_class {
  ISO_control_0,		/* Control codes in the range
				   0x00..0x1F and 0x7F, except for the
				   following 4 codes.  */
  ISO_shift_out,		/* ISO_CODE_SO (0x0E) */
  ISO_shift_in,			/* ISO_CODE_SI (0x0F) */
  ISO_single_shift_2_7,		/* ISO_CODE_SS2_7 (0x19) */
  ISO_escape,			/* ISO_CODE_SO (0x1B) */
  ISO_control_1,		/* Control codes in the range
				   0x80..0x9F, except for the
				   following 3 codes.  */
  ISO_single_shift_2,		/* ISO_CODE_SS2 (0x8E) */
  ISO_single_shift_3,		/* ISO_CODE_SS3 (0x8F) */
  ISO_control_sequence_introducer, /* ISO_CODE_CSI (0x9B) */
  ISO_0x20_or_0x7F,	      /* Codes of the values 0x20 or 0x7F.  */
  ISO_graphic_plane_0,	 /* Graphic codes in the range 0x21..0x7E.  */
  ISO_0xA0_or_0xFF,	      /* Codes of the values 0xA0 or 0xFF.  */
  ISO_graphic_plane_1	 /* Graphic codes in the range 0xA1..0xFE.  */
} iso_2022_code_class[256];


#define MCODING_ISO_DESIGNATION_MASK	\
  (MCODING_ISO_DESIGNATION_G0		\
   | MCODING_ISO_DESIGNATION_G1		\
   | MCODING_ISO_DESIGNATION_CTEXT	\
   | MCODING_ISO_DESIGNATION_CTEXT_EXT)

static int
setup_coding_iso_2022 (MCodingSystem *coding)
{
  MCodingInfoISO2022 *info = (MCodingInfoISO2022 *) (coding->extra_info);
  int ncharsets = coding->ncharsets;
  struct iso_2022_spec *spec;
  int designation_policy = info->flags & MCODING_ISO_DESIGNATION_MASK;
  int i;

  coding->ascii_compatible = 0;

  MSTRUCT_CALLOC (spec, MERROR_CODING);

  spec->flags = info->flags;
  spec->initial_invocation[0] = info->initial_invocation[0];
  spec->initial_invocation[1] = info->initial_invocation[1];
  for (i = 0; i < 4; i++)
    spec->initial_designation[i] = NULL;
  if (designation_policy)
    {
      spec->n_designations = ncharsets;
      if (spec->flags & MCODING_ISO_FULL_SUPPORT)
	spec->n_designations += mcharset__iso_2022_table.used;
      MTABLE_CALLOC (spec->designations, spec->n_designations, MERROR_CODING);
      for (i = 0; i < spec->n_designations; i++)
	spec->designations[i] = -1;
    }
  else
    {
      if (spec->flags & MCODING_ISO_FULL_SUPPORT)
	MERROR (MERROR_CODING, -1);
      spec->designations = NULL;
    }

  for (i = 0; i < ncharsets; i++)
    {
      int reg = info->designations[i];

      if (reg != -5
	  && coding->charsets[i]->final_byte > 0
	  && (reg < -4 || reg > 3))
	MERROR (MERROR_CODING, -1);
      if (reg >= 0)
	{
	  if (spec->initial_designation[reg])
	    MERROR (MERROR_CODING, -1);
	  spec->initial_designation[reg] = coding->charsets[i];
	}
      else if (reg >= -4)
	{
	  if (! designation_policy
	      && ! (spec->flags & MCODING_ISO_EUC_TW_SHIFT))
	    MERROR (MERROR_CODING, -1);
	  reg += 4;
	}

      if (designation_policy)
	spec->designations[i] = reg;
      if (coding->charsets[i] == mcharset__ascii)
	coding->ascii_compatible = 1;
    }

  if (coding->ascii_compatible
      && (spec->flags & (MCODING_ISO_DESIGNATION_G0
			 | MCODING_ISO_DESIGNATION_CTEXT
			 | MCODING_ISO_DESIGNATION_CTEXT_EXT
			 | MCODING_ISO_LOCKING_SHIFT)))
    coding->ascii_compatible = 0;

  if (spec->flags & MCODING_ISO_FULL_SUPPORT)
    for (i = 0; i < mcharset__iso_2022_table.used; i++)
      {
	MCharset *charset = mcharset__iso_2022_table.charsets[i];

	spec->designations[ncharsets + i]
	  = ((designation_policy == MCODING_ISO_DESIGNATION_CTEXT
	      || designation_policy == MCODING_ISO_DESIGNATION_CTEXT_EXT)
	     ? (charset->code_range[0] == 32
		|| charset->code_range[1] == 255)
	     : designation_policy == MCODING_ISO_DESIGNATION_G1);
      }

  spec->use_esc = ((spec->flags & MCODING_ISO_DESIGNATION_MASK)
		   || ((spec->flags & MCODING_ISO_LOCKING_SHIFT)
		       && (spec->initial_designation[2]
			   || spec->initial_designation[3]))
		   || (! (spec->flags & MCODING_ISO_EIGHT_BIT)
		       && (spec->flags & MCODING_ISO_SINGLE_SHIFT))
		   || (spec->flags & MCODING_ISO_ISO6429));

  coding->extra_spec = (void *) spec;

  return 0;
}

static int
reset_coding_iso_2022 (MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;
  struct iso_2022_status *status
    = (struct iso_2022_status *) &(converter->status);
  struct iso_2022_spec *spec;
  int i;

  if (! coding->ready
      && setup_coding_iso_2022 (coding) < 0)
    return -1;
  coding->ready = 1;

  spec = (struct iso_2022_spec *) coding->extra_spec;
  status->invocation[0] = spec->initial_invocation[0];
  status->invocation[1] = spec->initial_invocation[1];
  for (i = 0; i < 4; i++)
    status->designation[i] = spec->initial_designation[i];
  status->single_shifting = 0;
  status->bol = 1;
  status->r2l = 0;

  return 0;
}

#define ISO2022_DECODE_DESIGNATION(reg, dim, chars, final, rev)		  \
  do {									  \
    MCharset *charset;							  \
									  \
    if ((final) < '0' || (final) >= 128)				  \
      goto invalid_byte;						  \
    if (rev < 0)							  \
      {									  \
	charset = MCHARSET_ISO_2022 ((dim), (chars), (final));		  \
	if (! (spec->flags & MCODING_ISO_FULL_SUPPORT))			  \
	  {								  \
	    int i;							  \
									  \
	    for (i = 0; i < coding->ncharsets; i++)			  \
	      if (charset == coding->charsets[i])			  \
		break;							  \
	    if (i == coding->ncharsets)					  \
	      goto invalid_byte;					  \
	  }								  \
      }									  \
    else								  \
      {									  \
	int i;								  \
									  \
	for (i = 0; i < mcharset__iso_2022_table.used; i++)		  \
	  {								  \
	    charset = mcharset__iso_2022_table.charsets[i];		  \
	    if (charset->revision == (rev)				  \
		&& charset->dimension == (dim)				  \
		&& charset->final_byte == (final)			  \
		&& (charset->code_range[1] == (chars)			  \
		    || ((chars) == 96 && charset->code_range[1] == 255))) \
	      break;							  \
	  }								  \
	if (i == mcharset__iso_2022_table.used)				  \
	  goto invalid_byte;						  \
      }									  \
    status->designation[reg] = charset;					  \
  } while (0)


static MCharset *
find_ctext_non_standard_charset (char *charset_name)
{
  MCharset *charset;

  if (! strcmp (charset_name, "koi8-r"))
    charset = MCHARSET (msymbol ("koi8-r"));
  else if  (! strcmp (charset_name, "big5-0"))
    charset = MCHARSET (msymbol ("big5"));    
  else
    charset = NULL;
  return charset;
}

static int
decode_coding_iso_2022 (const unsigned char *source, int src_bytes, MText *mt,
		       MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;
  const unsigned char *src = internal->carryover;
  const unsigned char *src_stop = src + internal->carryover_bytes;
  const unsigned char *src_end = source + src_bytes;
  const unsigned char *src_base;
  unsigned char *dst = mt->data + mt->nbytes;
  unsigned char *dst_end = mt->data + mt->allocated;
  int nchars = 0;
  int last_nchars = 0;
  int at_most = converter->at_most > 0 ? converter->at_most : -1;
  struct iso_2022_spec *spec = (struct iso_2022_spec *) coding->extra_spec;
  struct iso_2022_status *status
    = (struct iso_2022_status *) &(converter->status);
  MCharset *charset0, *charset1, *charset;
  int error = 0;
  MCharset *cns_charsets[15];

  charset0 = (status->invocation[0] >= 0
	      ? status->designation[status->invocation[0]] : NULL);
  charset1 = (status->invocation[1] >= 0
	      ? status->designation[status->invocation[1]] : NULL);
  charset = mcharset__ascii;

  if (spec->flags & MCODING_ISO_EUC_TW_SHIFT)
    {
      int i;

      memset (cns_charsets, 0, sizeof (cns_charsets));
      for (i = 0; i < coding->ncharsets; i++)
	if (coding->charsets[i]->dimension == 2
	    && coding->charsets[i]->code_range[1] == 126)
	  {
	    int final = coding->charsets[i]->final_byte;

	    if (final >= 'G' && final <= 'M')
	      cns_charsets[final - 'G'] = coding->charsets[i];
	    else if (final < 0)
	      cns_charsets[14] = coding->charsets[i];
	  }
    }

  while (1)
    {
      MCharset *this_charset = NULL;
      int c1, c2, c3;

      ONE_MORE_BASE_BYTE (c1);

      if (status->utf8_shifting)
	{
	  int buf[6];
	  int bytes = CHAR_BYTES_BY_HEAD (c1);
	  int i;
	  
	  buf[0] = c1;
	  for (i = 1; i < bytes; i++)
	    {
	      ONE_MORE_BYTE (c1);
	      buf[i] = c1;
	    }
	  this_charset = UTF8_CHARSET (buf);
	  c1 = STRING_CHAR_UTF8 (buf);
	  goto emit_char;
	}

      if (status->non_standard_encoding > 0)
	{
	  int i;

	  this_charset = status->non_standard_charset;
	  for (i = 1; i < status->non_standard_charset_bytes; i++)
	    {
	      ONE_MORE_BYTE (c2);
	      c1 = (c1 << 8) | c2;
	    }
	  c1 = DECODE_CHAR (this_charset, c1);
	  goto emit_char;
	}

      switch (iso_2022_code_class[c1])
	{
	case ISO_graphic_plane_0:
	  this_charset = charset0;
	  break;

	case ISO_0x20_or_0x7F:
	  if (! charset0
	      || (charset0->code_range[0] != 32
		  && charset0->code_range[1] != 255))
	    /* This is SPACE or DEL.  */
	    this_charset = mcharset__ascii;
	  else
	    /* This is a graphic character of plane 0.  */	  
	    this_charset = charset0;
	  break;

	case ISO_graphic_plane_1:
	  if (!charset1)
	    goto invalid_byte;
	  this_charset = charset1;
	  break;

	case ISO_0xA0_or_0xFF:
	  if (! charset1
	      || charset1->code_range[0] == 33
	      || ! (spec->flags & MCODING_ISO_EIGHT_BIT))
	    goto invalid_byte;
	  /* This is a graphic character of plane 1. */
	  if (! charset1)
	    goto invalid_byte;
	  this_charset = charset1;
	  break;

	case ISO_control_0:
	  this_charset = mcharset__ascii;
	  break;

	case ISO_control_1:
	  goto invalid_byte;

	case ISO_shift_out:
	  if ((spec->flags & MCODING_ISO_LOCKING_SHIFT)
	      &&  status->designation[1])
	    {
	      status->invocation[0] = 1;
	      charset0 = status->designation[1];
	      continue;
	    }
	  this_charset = mcharset__ascii;
	  break;

	case ISO_shift_in:
	  if (spec->flags & MCODING_ISO_LOCKING_SHIFT)
	    {
	      status->invocation[0] = 0;
	      charset0 = status->designation[0];
	      continue;
	    }
	  this_charset = mcharset__ascii;
	  break;

	case ISO_single_shift_2_7:
	  if (! (spec->flags & MCODING_ISO_SINGLE_SHIFT_7))
	    {
	      this_charset = mcharset__ascii;
	      break;
	    }
	  c1 = 'N';
	  goto label_escape_sequence;

	case ISO_single_shift_2:
	  if (spec->flags & MCODING_ISO_EUC_TW_SHIFT)
	    {
	      ONE_MORE_BYTE (c1);
	      if (c1 < 0xA1 || (c1 > 0xA7 && c1 < 0xAF) || c1 > 0xAF
		  || ! cns_charsets[c1 - 0xA1])
		goto invalid_byte;
	      status->designation[2] = cns_charsets[c1 - 0xA1];
	    }
	  else if (! (spec->flags & MCODING_ISO_SINGLE_SHIFT))
	    goto invalid_byte;
	  /* SS2 is handled as an escape sequence of ESC 'N' */
	  c1 = 'N';
	  goto label_escape_sequence;

	case ISO_single_shift_3:
	  if (! (spec->flags & MCODING_ISO_SINGLE_SHIFT))
	    goto invalid_byte;
	  /* SS2 is handled as an escape sequence of ESC 'O' */
	  c1 = 'O';
	  goto label_escape_sequence;

	case ISO_control_sequence_introducer:
	  /* CSI is handled as an escape sequence of ESC '[' ...  */
	  c1 = '[';
	  goto label_escape_sequence;

	case ISO_escape:
	  if (! spec->use_esc)
	    {
	      this_charset = mcharset__ascii;
	      break;
	    }
	  ONE_MORE_BYTE (c1);
	label_escape_sequence:
	  /* Escape sequences handled here are invocation,
	     designation, and direction specification.  */
	  switch (c1)
	    {
	    case '&':	     /* revision of following character set */
	      if (! (spec->flags & MCODING_ISO_DESIGNATION_MASK))
		goto unused_escape_sequence;
	      ONE_MORE_BYTE (c1);
	      if (c1 < '@' || c1 > '~')
		goto invalid_byte;
	      ONE_MORE_BYTE (c1);
	      if (c1 != ISO_CODE_ESC)
		goto invalid_byte;
	      ONE_MORE_BYTE (c1);
	      goto label_escape_sequence;

	    case '$':	     /* designation of 2-byte character set */
	      if (! (spec->flags & MCODING_ISO_DESIGNATION_MASK))
		goto unused_escape_sequence;
	      ONE_MORE_BYTE (c1);
	      if (c1 >= '@' && c1 <= 'B')
		{ /* designation of JISX0208.1978, GB2312.1980, or
		     JISX0208.1980 */
		  ISO2022_DECODE_DESIGNATION (0, 2, 94, c1, -1);
		}
	      else if (c1 >= 0x28 && c1 <= 0x2B)
		{ /* designation of (dimension 2, chars 94) character set */
		  ONE_MORE_BYTE (c2);
		  ISO2022_DECODE_DESIGNATION (c1 - 0x28, 2, 94, c2, -1);
		}
	      else if (c1 >= 0x2C && c1 <= 0x2F)
		{ /* designation of (dimension 2, chars 96) character set */
		  ONE_MORE_BYTE (c2);
		  ISO2022_DECODE_DESIGNATION (c1 - 0x2C, 2, 96, c2, -1);
		}
	      else
		goto invalid_byte;
	      /* We must update these variables now.  */
	      if (status->invocation[0] >= 0)
		charset0 = status->designation[status->invocation[0]];
	      if (status->invocation[1] >= 0)
		charset1 = status->designation[status->invocation[1]];
	      continue;

	    case 'n':		/* invocation of locking-shift-2 */
	      if (! (spec->flags & MCODING_ISO_LOCKING_SHIFT)
		  || ! status->designation[2])
		goto invalid_byte;
	      status->invocation[0] = 2;
	      charset0 = status->designation[2];
	      continue;

	    case 'o':		/* invocation of locking-shift-3 */
	      if (! (spec->flags & MCODING_ISO_LOCKING_SHIFT)
		  || ! status->designation[3])
		goto invalid_byte;
	      status->invocation[0] = 3;
	      charset0 = status->designation[3];
	      continue;

	    case 'N':		/* invocation of single-shift-2 */
	      if (! ((spec->flags & MCODING_ISO_SINGLE_SHIFT)
		     || (spec->flags & MCODING_ISO_EUC_TW_SHIFT))
		  || ! status->designation[2])
		goto invalid_byte;
	      this_charset = status->designation[2];
	      ONE_MORE_BYTE (c1);
	      if (c1 < 0x20 || (c1 >= 0x80 && c1 < 0xA0))
		goto invalid_byte;
	      break;

	    case 'O':		/* invocation of single-shift-3 */
	      if (! (spec->flags & MCODING_ISO_SINGLE_SHIFT)
		  || ! status->designation[3])
		goto invalid_byte;
	      this_charset = status->designation[3];
	      ONE_MORE_BYTE (c1);
	      if (c1 < 0x20 || (c1 >= 0x80 && c1 < 0xA0))
		goto invalid_byte;
	      break;

	    case '[':		/* specification of direction */
	      if (! (spec->flags & MCODING_ISO_ISO6429))
		goto invalid_byte;
	      /* For the moment, nested direction is not supported.
		 So, (coding->mode & CODING_MODE_DIRECTION) zero means
		 left-to-right, and nonzero means right-to-left.  */
	      ONE_MORE_BYTE (c1);
	      switch (c1)
		{
		case ']':	/* end of the current direction */
		case '0':	/* end of the current direction */
		  status->r2l = 0;
		  break;

		case '1':	/* start of left-to-right direction */
		  ONE_MORE_BYTE (c1);
		  if (c1 != ']')
		    goto invalid_byte;
		  status->r2l = 0;
		  break;

		case '2':	/* start of right-to-left direction */
		  ONE_MORE_BYTE (c1);
		  if (c1 != ']')
		    goto invalid_byte;
		  status->r2l = 1;
		  break;

		default:
		  goto invalid_byte;
		}
	      continue;

	    case '%':
	      {
		char charset_name[16];
		int bytes;
		int i;

		if (! (spec->flags & MCODING_ISO_DESIGNATION_CTEXT_EXT))
		  goto invalid_byte;
		/* Compound-text uses these escape sequences:

		ESC % G  -- utf-8 bytes -- ESC % @
		ESC % / 1 M L -- charset name -- STX -- bytes --
		ESC % / 2 M L -- charset name -- STX -- bytes --
		ESC % / 3 M L -- charset name -- STX -- bytes --
		ESC % / 4 M L -- charset name -- STX -- bytes --

		It also uses this sequence but that is not yet
		supported here.

		ESC % / 0 M L -- charset name -- STX -- bytes -- */

		ONE_MORE_BYTE (c1);
		if (c1 == 'G')
		  {
		    status->utf8_shifting = 1;
		    continue;
		  }
		if (c1 == '@')
		  {
		    if (! status->utf8_shifting)
		      goto invalid_byte;
		    status->utf8_shifting = 0;
		    continue;
		  }
		if (c1 != '/')
		  goto invalid_byte;
		ONE_MORE_BYTE (c1);
		if (c1 < '1' || c1 > '4')
		  goto invalid_byte;
		status->non_standard_charset_bytes = c1 - '0';
		ONE_MORE_BYTE (c1);
		ONE_MORE_BYTE (c2);
		if (c1 < 128 || c2 < 128)
		  goto invalid_byte;
		bytes = (c1 - 128) * 128 + (c2 - 128);
		for (i = 0; i < 16; i++)
		  {
		    ONE_MORE_BYTE (c1);
		    if (c1 == ISO_CODE_STX)
		      break;
		    charset_name[i] = TOLOWER (c1);
		  }
		if (i == 16)
		  goto invalid_byte;
		charset_name[i++] = '\0';
		this_charset = find_ctext_non_standard_charset (charset_name);
		if (! this_charset)
		  goto invalid_byte;
		status->non_standard_charset = this_charset;
		status->non_standard_encoding = bytes - i;
		continue;
	      }

	    default:
	      if (! (spec->flags & MCODING_ISO_DESIGNATION_MASK))
		goto unused_escape_sequence;
	      if (c1 >= 0x28 && c1 <= 0x2B)
		{ /* designation of (dimension 1, chars 94) charset */
		  ONE_MORE_BYTE (c2);
		  ISO2022_DECODE_DESIGNATION (c1 - 0x28, 1, 94, c2, -1);
		}
	      else if (c1 >= 0x2C && c1 <= 0x2F)
		{ /* designation of (dimension 1, chars 96) charset */
		  ONE_MORE_BYTE (c2);
		  ISO2022_DECODE_DESIGNATION (c1 - 0x2C, 1, 96, c2, -1);
		}
	      else
		goto invalid_byte;
	      /* We must update these variables now.  */
	      if (status->invocation[0] >= 0)
		charset0 = status->designation[status->invocation[0]];
	      if (status->invocation[1] >= 0)
		charset1 = status->designation[status->invocation[1]];
	      continue;

	    unused_escape_sequence:
	      UNGET_ONE_BYTE (c1);
	      c1 = ISO_CODE_ESC;
	      this_charset = mcharset__ascii;
	    }
	}

      if (this_charset->dimension == 1)
	{
	  if (this_charset->code_range[1] <= 128)
	    c1 &= 0x7F;
	}
      else if (this_charset->dimension == 2)
	{
	  ONE_MORE_BYTE (c2);
	  c1 = ((c1 & 0x7F) << 8) | (c2 & 0x7F);
	}
      else			/* i.e.  (dimension == 3) */
	{
	  ONE_MORE_BYTE (c2);
	  ONE_MORE_BYTE (c3);
	  c1 = ((c1 & 0x7F) << 16) | ((c2 & 0x7F) << 8) | (c3 & 0x7F);
	}
      c1 = DECODE_CHAR (this_charset, c1);
      goto emit_char;

    invalid_byte:
      if (! converter->lenient)
	break;
      REWIND_SRC_TO_BASE ();
      c1 = *src++;
      this_charset = mcharset__binary;

    emit_char:
      if (this_charset != mcharset__ascii
	  && this_charset != charset)
	{
	  TAKEIN_CHARS (mt, nchars - last_nchars,
			dst - (mt->data + mt->nbytes), charset);
	  charset = this_charset;
	  last_nchars = nchars;
	}
      EMIT_CHAR (c1);
      if (status->non_standard_encoding > 0)
	status->non_standard_encoding -= status->non_standard_charset_bytes;
    }
  /* We reach here because of an invalid byte.  */
  error = 1;



 source_end:
  TAKEIN_CHARS (mt, nchars - last_nchars,
		dst - (mt->data + mt->nbytes), charset);
  return finish_decoding (mt, converter, nchars,
			  source, src_end, src_base, error);

}

/* Produce codes (escape sequence) for designating CHARSET to graphic
   register REG at DST, and increment DST.  If CHARSET->final-char is
   '@', 'A', or 'B' and SHORT_FORM is nonzero, produce designation
   sequence of short-form.  Update STATUS->designation.  */

#define ISO2022_ENCODE_DESIGNATION(reg, charset, spec, status)		   \
  do {									   \
    char *intermediate_char_94 = "()*+";				   \
    char *intermediate_char_96 = ",-./";				   \
									   \
    if (dst + 4 > dst_end)						   \
      goto memory_shortage;						   \
    *dst++ = ISO_CODE_ESC;						   \
    if (charset->dimension == 1)					   \
      {									   \
	if (charset->code_range[0] != 32				   \
	    && charset->code_range[1] != 255)				   \
	  *dst++ = (unsigned char) (intermediate_char_94[reg]);		   \
	else								   \
	  *dst++ = (unsigned char) (intermediate_char_96[reg]);		   \
      }									   \
    else								   \
      {									   \
	*dst++ = '$';							   \
	if (charset->code_range[0] != 32				   \
	    && charset->code_range[1] != 255)				   \
	  {								   \
	    if (spec->flags & MCODING_ISO_LONG_FORM			   \
		|| reg != 0						   \
		|| charset->final_byte < '@' || charset->final_byte > 'B') \
	      *dst++ = (unsigned char) (intermediate_char_94[reg]);	   \
	  }								   \
	else								   \
	  *dst++ = (unsigned char) (intermediate_char_96[reg]);		   \
      }									   \
    *dst++ = charset->final_byte;					   \
									   \
    status->designation[reg] = charset;					   \
  } while (0)


/* The following two macros produce codes (control character or escape
   sequence) for ISO-2022 single-shift functions (single-shift-2 and
   single-shift-3).  */

#define ISO2022_ENCODE_SINGLE_SHIFT_2(spec, status)	\
  do {							\
    if (dst + 2 > dst_end)				\
      goto memory_shortage;				\
    if (! (spec->flags & MCODING_ISO_EIGHT_BIT))	\
      *dst++ = ISO_CODE_ESC, *dst++ = 'N';		\
    else						\
      *dst++ = ISO_CODE_SS2;				\
    status->single_shifting = 1;			\
  } while (0)


#define ISO2022_ENCODE_SINGLE_SHIFT_3(spec, status)	\
  do {							\
    if (dst + 2 > dst_end)				\
      goto memory_shortage;				\
    if (! (spec->flags & MCODING_ISO_EIGHT_BIT))	\
      *dst++ = ISO_CODE_ESC, *dst++ = 'O';		\
    else						\
      *dst++ = ISO_CODE_SS3;				\
    status->single_shifting = 1;			\
  } while (0)


/* The following four macros produce codes (control character or
   escape sequence) for ISO-2022 locking-shift functions (shift-in,
   shift-out, locking-shift-2, and locking-shift-3).  */

#define ISO2022_ENCODE_SHIFT_IN(status)		\
  do {						\
    if (dst + 1 > dst_end)			\
      goto memory_shortage;			\
    *dst++ = ISO_CODE_SI;			\
    status->invocation[0] = 0;			\
  } while (0)


#define ISO2022_ENCODE_SHIFT_OUT(status)	\
  do {						\
    if (dst + 1 > dst_end)			\
      goto memory_shortage;			\
    *dst++ = ISO_CODE_SO;			\
    status->invocation[0] = 1;			\
  } while (0)


#define ISO2022_ENCODE_LOCKING_SHIFT_2(status)	\
  do {						\
    if (dst + 2 > dst_end)			\
      goto memory_shortage;			\
    *dst++ = ISO_CODE_ESC, *dst++ = 'n';	\
    status->invocation[0] = 2;			\
  } while (0)


#define ISO2022_ENCODE_LOCKING_SHIFT_3(status)	\
  do {						\
    if (dst + 2 > dst_end)			\
      goto memory_shortage;			\
    *dst++ = ISO_CODE_ESC, *dst++ = 'o';	\
    status->invocation[0] = 3;			\
  } while (0)

#define ISO2022_ENCODE_UTF8_SHIFT_START(len)	\
  do {						\
    CHECK_DST (3 + len);			\
    *dst++ = ISO_CODE_ESC;			\
    *dst++ = '%';				\
    *dst++ = 'G';				\
    status->utf8_shifting = 1;			\
  } while (0)


#define ISO2022_ENCODE_UTF8_SHIFT_END()	\
  do {					\
    CHECK_DST (3);			\
    *dst++ = ISO_CODE_ESC;		\
    *dst++ = '%';			\
    *dst++ = '@';			\
    status->utf8_shifting = 0;		\
  } while (0)


#define ISO2022_ENCODE_NON_STANDARD(name, len)			\
  do {								\
    CHECK_DST (6 + len + 1 + non_standard_charset_bytes);	\
    non_standard_begin = dst;					\
    *dst++ = ISO_CODE_ESC;					\
    *dst++ = '%';						\
    *dst++ = '/';						\
    *dst++ = '0' + non_standard_charset_bytes;			\
    *dst++ = 0, *dst++ = 0;	/* filled later */		\
    memcpy (dst, name, len);					\
    dst += len;							\
    *dst++ = ISO_CODE_STX;					\
    non_standard_bytes = len + 1;				\
  } while (0)


static char *
find_ctext_non_standard_name (MCharset *charset, int *bytes)
{
  char *name = msymbol_name (charset->name);

  if (! strcmp (name, "koi8-r"))
    *bytes = 1;
  else if (! strcmp (name, "big5"))
    name = "big5-0", *bytes = 2;
  else
    return NULL;
  return name;
}

/* Designate CHARSET to a graphic register specified in
   SPEC->designation.  If the register is not yet invoked to graphic
   left not right, invoke it to graphic left.  DSTP points to a
   variable containing a memory address where the output must go.
   DST_END is the limit of that memory.

   Return 0 if it succeeds.  Return -1 otherwise, which means that the
   memory area is too short.  By side effect, update the variable that
   DSTP points to.  */

static int
iso_2022_designate_invoke_charset (MCodingSystem *coding,
				   MCharset *charset,
				   struct iso_2022_spec *spec,
				   struct iso_2022_status *status,
				   unsigned char **dstp,
				   unsigned char *dst_end)
{
  int i;
  unsigned char *dst = *dstp;

  for (i = 0; i < 4; i++)
    if (charset == status->designation[i])
      break;

  if (i >= 4)
    {
      /* CHARSET is not yet designated to any graphic registers.  */
      for (i = 0; i < coding->ncharsets; i++)
	if (charset == coding->charsets[i])
	  break;
      if (i == coding->ncharsets)
	{
	  for (i = 0; i < mcharset__iso_2022_table.used; i++)
	    if (charset == mcharset__iso_2022_table.charsets[i])
	      break;
	  i += coding->ncharsets;
	}
      i = spec->designations[i];
      ISO2022_ENCODE_DESIGNATION (i, charset, spec, status);
    }

  if (status->invocation[0] != i
      && status->invocation[1] != i)
    {
      /* Graphic register I is not yet invoked.  */
      switch (i)
	{
	case 0:			/* graphic register 0 */
	  ISO2022_ENCODE_SHIFT_IN (status);
	  break;

	case 1:			/* graphic register 1 */
	  ISO2022_ENCODE_SHIFT_OUT (status);
	  break;

	case 2:			/* graphic register 2 */
	  if (spec->flags & MCODING_ISO_SINGLE_SHIFT)
	    ISO2022_ENCODE_SINGLE_SHIFT_2 (spec, status);
	  else
	    ISO2022_ENCODE_LOCKING_SHIFT_2 (status);
	  break;

	case 3:			/* graphic register 3 */
	  if (spec->flags & MCODING_ISO_SINGLE_SHIFT)
	    ISO2022_ENCODE_SINGLE_SHIFT_3 (spec, status);
	  else
	    ISO2022_ENCODE_LOCKING_SHIFT_3 (status);
	  break;
	}
    }
  *dstp = dst;
  return 0;

 memory_shortage:
  *dstp = dst;
  return -1;
}


/* Reset the invocation/designation status to the initial one.  SPEC
   and STATUS contain information about the current and initial
   invocation /designation status respectively.  DSTP points to a
   variable containing a memory address where the output must go.
   DST_END is the limit of that memory.

   Return 0 if it succeeds.  Return -1 otherwise, which means that the
   memory area is too short.  By side effect, update the variable that
   DSTP points to.  */

static int
iso_2022_reset_invocation_designation (struct iso_2022_spec *spec,
				       struct iso_2022_status *status,
				       unsigned char **dstp,
				       unsigned char *dst_end)
{
  unsigned char *dst = *dstp;
  int i;

  /* Reset the invocation status of GL.  We have not yet supported GR
     invocation.  */
  if (status->invocation[0] != spec->initial_invocation[0]
	&& spec->initial_invocation[0] >= 0)
    {
      if (spec->initial_invocation[0] == 0)
	ISO2022_ENCODE_SHIFT_IN (status);
      else if (spec->initial_invocation[0] == 1)
	ISO2022_ENCODE_SHIFT_OUT (status);
      else if (spec->initial_invocation[0] == 2)
	ISO2022_ENCODE_LOCKING_SHIFT_2 (status);
      else			/* i.e. spec->initial_invocation[0] == 3 */
	ISO2022_ENCODE_LOCKING_SHIFT_3 (status);
    }

  /* Reset the designation status of G0..G3.  */
  for (i = 0; i < 4; i++)
    if (status->designation[i] != spec->initial_designation[i]
	&& spec->initial_designation[i])
      {
	MCharset *charset = spec->initial_designation[i];
	
	ISO2022_ENCODE_DESIGNATION (i, charset, spec, status);
      }

  *dstp = dst;
  return 0;

 memory_shortage:
  *dstp = dst;
  return -1;
}


static int
encode_coding_iso_2022 (MText *mt, int from, int to,
		       unsigned char *destination, int dst_bytes,
		       MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;
  unsigned char *src, *src_end;
  unsigned char *dst = destination;
  unsigned char *dst_end = dst + dst_bytes;
  int nchars = 0;
  unsigned char *dst_base;
  struct iso_2022_spec *spec = (struct iso_2022_spec *) coding->extra_spec;
  int full_support = spec->flags & MCODING_ISO_FULL_SUPPORT;
  struct iso_2022_status *status
    = (struct iso_2022_status *) &(converter->status);
  MCharset *primary, *charset0, *charset1;
  int next_primary_change;
  int ncharsets = coding->ncharsets;
  MCharset **charsets = coding->charsets;
  MCharset *cns_charsets[15];
  int ascii_compatible = coding->ascii_compatible;
  MCharset *non_standard_charset = NULL;
  int non_standard_charset_bytes = 0;
  int non_standard_bytes = 0;
  unsigned char *non_standard_begin = NULL;
  enum MTextFormat format = mt->format;

  SET_SRC (mt, format, from, to);

  if (spec->flags & MCODING_ISO_EUC_TW_SHIFT)
    {
      int i;

      memset (cns_charsets, 0, sizeof (cns_charsets));
      for (i = 0; i < ncharsets; i++)
	if (charsets[i]->dimension == 2)
	  {
	    int final = charsets[i]->final_byte;

	    if (final >= 'G' && final <= 'M')
	      cns_charsets[final - 'G'] = charsets[i];
	    else if (final < 0)
	      cns_charsets[14] = charsets[i];
	  }
    }

  next_primary_change = from;
  primary = NULL;
  charset0 = status->designation[status->invocation[0]];
  charset1 = (status->invocation[1] < 0 ? NULL
	      : status->designation[status->invocation[1]]);

  while (1)
    {
      int bytes, c;

      dst_base = dst;
      ONE_MORE_CHAR (c, bytes, format);

      if (c < 128 && ascii_compatible)
	{
	  if (status->utf8_shifting)
	    ISO2022_ENCODE_UTF8_SHIFT_END ();
	  CHECK_DST (1);
	  *dst++ = c;
	}
      else if (c <= 32 || c == 127)
	{
	  if (status->utf8_shifting)
	    ISO2022_ENCODE_UTF8_SHIFT_END ();
	  if (spec->flags & MCODING_ISO_RESET_AT_CNTL
	      || (c == '\n' && spec->flags & MCODING_ISO_RESET_AT_EOL))
	    {
	      if (iso_2022_reset_invocation_designation (spec, status,
							 &dst, dst_end) < 0)
		goto insufficient_destination;
	      charset0 = status->designation[status->invocation[0]];
	      charset1 = (status->invocation[1] < 0 ? NULL
			  : status->designation[status->invocation[1]]);
	    }
	  CHECK_DST (1);
	  *dst++ = c;
	}
      else
	{
	  unsigned code = MCHAR_INVALID_CODE;
	  MCharset *charset = NULL;
	  int gr_mask;
	  int pos = from + nchars;

	  if (pos >= next_primary_change)
	    {
	      MSymbol primary_charset
		= (MSymbol) mtext_get_prop (mt, pos, Mcharset);
	      primary = MCHARSET (primary_charset);
	      if (primary && primary != mcharset__binary)
		{
		  if (primary->final_byte <= 0)
		    primary = NULL;
		  else if (! full_support)
		    {
		      int i;

		      for (i = 0; i < ncharsets; i++)
			if (primary == charsets[i])
			  break;
		      if (i == ncharsets)
			primary = NULL;
		    }
		}

	      mtext_prop_range (mt, Mcharset, pos,
				NULL, &next_primary_change, 0);
	    }

	  if (primary && primary != mcharset__binary)
	    {
	      code = ENCODE_CHAR (primary, c);
	      if (code != MCHAR_INVALID_CODE)
		charset = primary;
	    }
	  if (! charset)
	    {
	      if (c <= 32 || c == 127)
		{
		  code = c;
		  charset = mcharset__ascii;
		}
	      else 
		{
		  int i;

		  for (i = 0; i < ncharsets; i++)
		    {
		      charset = charsets[i];
		      code = ENCODE_CHAR (charset, c);
		      if (code != MCHAR_INVALID_CODE)
			break;
		    }
		  if (i == ncharsets)
		    {
		      if (spec->flags & MCODING_ISO_FULL_SUPPORT)
			{
			  for (i = 0; i < mcharset__iso_2022_table.used; i++)
			    {
			      charset = mcharset__iso_2022_table.charsets[i];
			      code = ENCODE_CHAR (charset, c);
			      if (code != MCHAR_INVALID_CODE)
				break;
			    }
			  if (i == mcharset__iso_2022_table.used)
			    {
			      if (spec->flags & MCODING_ISO_DESIGNATION_CTEXT_EXT)
				goto unsupported_char;
			      converter->result = MCONVERSION_RESULT_INVALID_CHAR;
			      goto finish;
			    }
			}
		      else
			goto unsupported_char;
		    }
		}
	    }

	  if (charset
	      && (charset->final_byte >= 0
		  || spec->flags & MCODING_ISO_EUC_TW_SHIFT))
	    {
	      if (code >= 0x80 && code < 0xA0)
		goto unsupported_char;
	      code &= 0x7F7F7F7F;
	      if (status->utf8_shifting)
		ISO2022_ENCODE_UTF8_SHIFT_END ();
	      if (charset == charset0)
		gr_mask = 0;
	      else if (charset == charset1)
		gr_mask = 0x80;
	      else
		{
		  unsigned char *p = NULL;

		  if (spec->flags & MCODING_ISO_EUC_TW_SHIFT)
		    {
		      int i;

		      if (cns_charsets[0] == charset)
			{
			  CHECK_DST (2);
			}
		      else
			{
			  for (i = 1; i < 15; i++)
			    if (cns_charsets[i] == charset)
			      break;
			  CHECK_DST (4);
			  *dst++ = ISO_CODE_SS2;
			  *dst++ = 0xA1 + i;
			}
		      status->single_shifting = 1;
		      p = dst;
		    }
		  else
		    {
		      if (iso_2022_designate_invoke_charset
			  (coding, charset, spec, status, &dst, dst_end) < 0)
			goto insufficient_destination;
		      charset0 = status->designation[status->invocation[0]];
		      charset1 = (status->invocation[1] < 0 ? NULL
				  : status->designation[status->invocation[1]]);
		    }
		  if (status->single_shifting)
		    gr_mask
		      = (spec->flags & MCODING_ISO_EIGHT_BIT) ? 0x80 : 0;
		  else if (charset == charset0)
		    gr_mask = 0;
		  else
		    gr_mask = 0x80;
		}
	      if (charset->dimension == 1)
		{
		  CHECK_DST (1);
		  *dst++ = code | gr_mask;
		}
	      else if (charset->dimension == 2)
		{
		  CHECK_DST (2);
		  *dst++ = (code >> 8) | gr_mask;
		  *dst++ = (code & 0xFF) | gr_mask;
		}
	      else
		{
		  CHECK_DST (3);
		  *dst++ = (code >> 16) | gr_mask;
		  *dst++ = ((code >> 8) & 0xFF) | gr_mask;
		  *dst++ = (code & 0xFF) | gr_mask;
		}
	      status->single_shifting = 0;
	    }
	  else if (charset && spec->flags & MCODING_ISO_DESIGNATION_CTEXT_EXT)
	    {
	      if (charset != non_standard_charset)
		{
		  char *name = (find_ctext_non_standard_name
				(charset, &non_standard_charset_bytes));

		  if (name)
		    {
		      int len = strlen (name);
		      
		      ISO2022_ENCODE_NON_STANDARD (name, len);
		      non_standard_charset = charset;
		    }
		  else
		    non_standard_charset = NULL;
		}

	      if (non_standard_charset)
		{
		  if (dst + non_standard_charset_bytes > dst_end)
		    goto insufficient_destination;
		  non_standard_bytes += non_standard_charset_bytes;
		  non_standard_begin[4] = (non_standard_bytes / 128) | 0x80;
		  non_standard_begin[5] = (non_standard_bytes % 128) | 0x80;
		  if (non_standard_charset_bytes == 1)
		    *dst++ = code;
		  else if (non_standard_charset_bytes == 2)
		    *dst++ = code >> 8, *dst++ = code & 0xFF;
		  else if (non_standard_charset_bytes == 3)
		    *dst++ = code >> 16, *dst++ = (code >> 8) & 0xFF,
		      *dst++ = code & 0xFF;
		  else		/* i.e non_standard_charset_bytes == 3 */
		    *dst++ = code >> 24, *dst++ = (code >> 16) & 0xFF,
		      *dst++ = (code >> 8) & 0xFF, *dst++ = code & 0xFF;
		}
	      else
		{
		  int len = CHAR_BYTES (c);

		  if (c >= 0x110000)
		    goto unsupported_char;
		  if (! status->utf8_shifting)
		    ISO2022_ENCODE_UTF8_SHIFT_START (len);
		  else
		    CHECK_DST (len);
		  CHAR_STRING (c, dst);
		}
	    }
	  else
	    goto unsupported_char;
	}
      src += bytes;
      nchars++;
      continue;

    unsupported_char:
      {
	int len;

	if (iso_2022_designate_invoke_charset (coding, mcharset__ascii,
					       spec, status,
					       &dst, dst_end) < 0)
	  goto insufficient_destination;
	if (! converter->lenient)
	  break;
	len = encode_unsupporeted_char (c, dst, dst_end, mt, from + nchars);
	if (len == 0)
	  goto insufficient_destination;
	dst += len;
	src += bytes;
	nchars++;
      }
    }
  /* We reach here because of an unsupported char.  */
  converter->result = MCONVERSION_RESULT_INVALID_CHAR;
  goto finish;

 insufficient_destination:
  dst = dst_base;
  converter->result = MCONVERSION_RESULT_INSUFFICIENT_DST;

 finish:
  if (converter->result == MCONVERSION_RESULT_SUCCESS
      && converter->last_block)
    {
      if (status->utf8_shifting)
	{
	  ISO2022_ENCODE_UTF8_SHIFT_END ();
	  dst_base = dst;
	}
      if (spec->flags & MCODING_ISO_RESET_AT_EOL
	  && charset0 != spec->initial_designation[0])
	{
	  if (iso_2022_reset_invocation_designation (spec, status,
						     &dst, dst_end) < 0)
	    goto insufficient_destination;
	}
    }
  converter->nchars += nchars;
  converter->nbytes += dst - destination;
  return (converter->result == MCONVERSION_RESULT_INVALID_CHAR ? -1 : 0);
}


/* Staffs for coding-systems of type MCODING_TYPE_MISC.  */

/* For SJIS handling... */

#define SJIS_TO_JIS(s1, s2)				\
  (s2 >= 0x9F						\
   ? (((s1 * 2 - (s1 >= 0xE0 ? 0x160 : 0xE0)) << 8)	\
      | (s2 - 0x7E))					\
   : (((s1 * 2 - ((s1 >= 0xE0) ? 0x161 : 0xE1)) << 8)	\
      | (s2 - ((s2 >= 0x7F) ? 0x20 : 0x1F))))

#define JIS_TO_SJIS(c1, c2)				\
  ((c1 & 1)						\
   ? (((c1 / 2 + ((c1 < 0x5F) ? 0x71 : 0xB1)) << 8)	\
      | (c2 + ((c2 >= 0x60) ? 0x20 : 0x1F)))		\
   : (((c1 / 2 + ((c1 < 0x5F) ? 0x70 : 0xB0)) << 8)	\
      | (c2 + 0x7E)))


static int
reset_coding_sjis (MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;

  if (! coding->ready)
    {
      MSymbol kanji_sym = msymbol ("jisx0208.1983");
      MCharset *kanji = MCHARSET (kanji_sym);
      MSymbol kana_sym = msymbol ("jisx0201-kana");
      MCharset *kana = MCHARSET (kana_sym);

      if (! kanji || ! kana)
	return -1;
      coding->ncharsets = 3;
      coding->charsets[1] = kanji;
      coding->charsets[2] = kana;
    }
  coding->ready = 1;
  return 0;
}

static int
decode_coding_sjis (const unsigned char *source, int src_bytes, MText *mt,
		    MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;
  const unsigned char *src = internal->carryover;
  const unsigned char *src_stop = src + internal->carryover_bytes;
  const unsigned char *src_end = source + src_bytes;
  const unsigned char *src_base;
  unsigned char *dst = mt->data + mt->nbytes;
  unsigned char *dst_end = mt->data + mt->allocated - MAX_UTF8_CHAR_BYTES;
  int nchars = 0;
  int last_nchars = 0;
  int at_most = converter->at_most > 0 ? converter->at_most : -1;

  MCharset *charset_roman = coding->charsets[0];
  MCharset *charset_kanji = coding->charsets[1];
  MCharset *charset_kana = coding->charsets[2];
  MCharset *charset = mcharset__ascii;
  int error = 0;

  while (1)
    {
      MCharset *this_charset;
      int c, c1, c2;

      ONE_MORE_BASE_BYTE (c1);

      c2 = -1;
      if (c1 < 0x80)
	{
	  this_charset = ((c1 <= 0x20 || c1 == 0x7F)
			  ? mcharset__ascii
			  : charset_roman);
	}
      else if ((c1 >= 0x81 && c1 <= 0x9F) || (c1 >= 0xE0 && c1 <= 0xEF))
	{
	  ONE_MORE_BYTE (c2);
	  if ((c2 >= 0x40 && c2 <= 0x7F) || (c2 >= 80 && c2 <= 0xFC))
	    {
	      this_charset = charset_kanji;
	      c1 = SJIS_TO_JIS (c1, c2);
	    }
	  else
	    goto invalid_byte;
	}
      else if (c1 >= 0xA1 && c1 <= 0xDF)
	{
	  this_charset = charset_kana;
	  c1 &= 0x7F;
	}
      else
	goto invalid_byte;

      c = DECODE_CHAR (this_charset, c1);
      if (c >= 0)
	goto emit_char;

    invalid_byte:
      if (! converter->lenient)
	break;
      REWIND_SRC_TO_BASE ();
      c = *src++;
      this_charset = mcharset__binary;

    emit_char:
      if (this_charset != mcharset__ascii
	  && this_charset != charset)
	{
	  TAKEIN_CHARS (mt, nchars - last_nchars, 
			dst - (mt->data + mt->nbytes), charset);
	  charset = this_charset;
	  last_nchars = nchars;
	}
      EMIT_CHAR (c);
    }
  /* We reach here because of an invalid byte.  */
  error = 1;

 source_end:
  TAKEIN_CHARS (mt, nchars - last_nchars,
		dst - (mt->data + mt->nbytes), charset);
  return finish_decoding (mt, converter, nchars,
			  source, src_end, src_base, error);
}

static int
encode_coding_sjis (MText *mt, int from, int to,
		    unsigned char *destination, int dst_bytes,
		    MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  MCodingSystem *coding = internal->coding;
  unsigned char *src, *src_end;
  unsigned char *dst = destination;
  unsigned char *dst_end = dst + dst_bytes;
  int nchars = 0;
  MCharset *charset_roman = coding->charsets[0];
  MCharset *charset_kanji = coding->charsets[1];
  MCharset *charset_kana = coding->charsets[2];
  enum MTextFormat format = mt->format;

  SET_SRC (mt, format, from, to);

  while (1)
    {
      int c, bytes, len;
      unsigned code;

      ONE_MORE_CHAR (c, bytes, format);

      if (c <= 0x20 || c == 0x7F)
	{
	  CHECK_DST (1);
	  *dst++ = c;
	}
      else
	{
	  if ((code = ENCODE_CHAR (charset_roman, c)) != MCHAR_INVALID_CODE)
	    {
	      CHECK_DST (1);
	      *dst++ = c;
	    }
	  else if ((code = ENCODE_CHAR (charset_kanji, c))
		   != MCHAR_INVALID_CODE)
	    {
	      int c1 = code >> 8, c2 = code & 0xFF;
	      code = JIS_TO_SJIS (c1, c2);
	      CHECK_DST (2);
	      *dst++ = code >> 8;
	      *dst++ = code & 0xFF;
	    }
	  else if ((code = ENCODE_CHAR (charset_kana, c))
		   != MCHAR_INVALID_CODE)
	    {
	      CHECK_DST (1);
	      *dst++ = code | 0x80;
	    }
	  else
	    {
	      if (! converter->lenient)
		break;
	      len = encode_unsupporeted_char (c, dst, dst_end,
					      mt, from + nchars);
	      if (len == 0)
		goto insufficient_destination;
	      dst += len;
	    }
	}
      src += bytes;
      nchars++;
    }
  /* We reach here because of an unsupported char.  */
  converter->result = MCONVERSION_RESULT_INVALID_CHAR;
  goto finish;

 insufficient_destination:
  converter->result = MCONVERSION_RESULT_INSUFFICIENT_DST;

 finish:
  converter->nchars += nchars;
  converter->nbytes += dst - destination;
  return (converter->result == MCONVERSION_RESULT_INVALID_CHAR ? -1 : 0);
}


static MCodingSystem *
find_coding (MSymbol name)
{
  MCodingSystem *coding = (MCodingSystem *) msymbol_get (name, Mcoding);

  if (! coding)
    {
      MPlist *plist, *pl;
      MSymbol sym = msymbol__canonicalize (name);

      plist = mplist_find_by_key (coding_definition_list, sym);
      if (! plist)
	return NULL;
      pl = MPLIST_PLIST (plist);
      name = MPLIST_VAL (pl);
      mconv_define_coding (MSYMBOL_NAME (name), MPLIST_NEXT (pl),
			   NULL, NULL, NULL, NULL);
      coding = (MCodingSystem *) msymbol_get (name, Mcoding);
      plist = mplist_pop (plist);
      M17N_OBJECT_UNREF (plist);
    }
  return coding;
}

#define BINDING_NONE 0
#define BINDING_BUFFER 1
#define BINDING_STREAM 2

#define CONVERT_WORKSIZE 0x10000


/* Internal API */

int
mcoding__init (void)
{
  int i;
  MPlist *param, *charsets, *pl;

  MLIST_INIT1 (&coding_list, codings, 128);
  coding_definition_list = mplist ();

  /* ISO-2022 specific initialize routine.  */
  for (i = 0; i < 0x20; i++)
    iso_2022_code_class[i] = ISO_control_0;
  for (i = 0x21; i < 0x7F; i++)
    iso_2022_code_class[i] = ISO_graphic_plane_0;
  for (i = 0x80; i < 0xA0; i++)
    iso_2022_code_class[i] = ISO_control_1;
  for (i = 0xA1; i < 0xFF; i++)
    iso_2022_code_class[i] = ISO_graphic_plane_1;
  iso_2022_code_class[0x20] = iso_2022_code_class[0x7F] = ISO_0x20_or_0x7F;
  iso_2022_code_class[0xA0] = iso_2022_code_class[0xFF] = ISO_0xA0_or_0xFF;
  iso_2022_code_class[0x0E] = ISO_shift_out;
  iso_2022_code_class[0x0F] = ISO_shift_in;
  iso_2022_code_class[0x19] = ISO_single_shift_2_7;
  iso_2022_code_class[0x1B] = ISO_escape;
  iso_2022_code_class[0x8E] = ISO_single_shift_2;
  iso_2022_code_class[0x8F] = ISO_single_shift_3;
  iso_2022_code_class[0x9B] = ISO_control_sequence_introducer;

  Mcoding = msymbol ("coding");

  Mutf = msymbol ("utf");
  Miso_2022 = msymbol ("iso-2022");

  Mreset_at_eol = msymbol ("reset-at-eol");
  Mreset_at_cntl = msymbol ("reset-at-cntl");
  Meight_bit = msymbol ("eight-bit");
  Mlong_form = msymbol ("long-form");
  Mdesignation_g0 = msymbol ("designation-g0");
  Mdesignation_g1 = msymbol ("designation-g1");
  Mdesignation_ctext = msymbol ("designation-ctext");
  Mdesignation_ctext_ext = msymbol ("designation-ctext-ext");
  Mlocking_shift = msymbol ("locking-shift");
  Msingle_shift = msymbol ("single-shift");
  Msingle_shift_7 = msymbol ("single-shift-7");
  Meuc_tw_shift = msymbol ("euc-tw-shift");
  Miso_6429 = msymbol ("iso-6429");
  Mrevision_number = msymbol ("revision-number");
  Mfull_support = msymbol ("full-support");
  Mmaybe = msymbol ("maybe");

  Mtype = msymbol ("type");
  Mcharsets = msymbol_as_managing_key ("charsets");
  Mflags = msymbol_as_managing_key ("flags");
  Mdesignation = msymbol_as_managing_key ("designation");
  Minvocation = msymbol_as_managing_key ("invocation");
  Mcode_unit = msymbol ("code-unit");
  Mbom = msymbol ("bom");
  Mlittle_endian = msymbol ("little-endian");

  param = mplist ();
  charsets = mplist ();
  pl = param;
  /* Setup predefined codings.  */
  mplist_set (charsets, Msymbol, Mcharset_ascii);
  pl = mplist_add (pl, Mtype, Mcharset);
  pl = mplist_add (pl, Mcharsets, charsets);
  Mcoding_us_ascii = mconv_define_coding ("us-ascii", param,
					  NULL, NULL, NULL, NULL);

  {
    MSymbol alias = msymbol ("ANSI_X3.4-1968");
    MCodingSystem *coding
      = (MCodingSystem *) msymbol_get (Mcoding_us_ascii, Mcoding);

    msymbol_put (alias, Mcoding, coding);
    alias = msymbol__canonicalize (alias);
    msymbol_put (alias, Mcoding, coding);
  }

  mplist_set (charsets, Msymbol, Mcharset_iso_8859_1);
  Mcoding_iso_8859_1 = mconv_define_coding ("iso-8859-1", param,
					    NULL, NULL, NULL, NULL);

  mplist_set (charsets, Msymbol, Mcharset_m17n);
  mplist_put (param, Mtype, Mutf);
  mplist_put (param, Mcode_unit, (void *) 8);
  Mcoding_utf_8_full = mconv_define_coding ("utf-8-full", param,
					    NULL, NULL, NULL, NULL);

  mplist_set (charsets, Msymbol, Mcharset_unicode);
  Mcoding_utf_8 = mconv_define_coding ("utf-8", param,
				       NULL, NULL, NULL, NULL);

  mplist_put (param, Mcode_unit, (void *) 16);
  mplist_put (param, Mbom, Mmaybe);
#ifndef WORDS_BIGENDIAN
  mplist_put (param, Mlittle_endian, Mt);
#endif
  Mcoding_utf_16 = mconv_define_coding ("utf-16", param,
					NULL, NULL, NULL, NULL);

  mplist_put (param, Mcode_unit, (void *) 32);
  Mcoding_utf_32 = mconv_define_coding ("utf-32", param,
					NULL, NULL, NULL, NULL);

  mplist_put (param, Mcode_unit, (void *) 16);
  mplist_put (param, Mbom, Mnil);
  mplist_put (param, Mlittle_endian, Mnil);
  Mcoding_utf_16be = mconv_define_coding ("utf-16be", param,
					  NULL, NULL, NULL, NULL);

  mplist_put (param, Mcode_unit, (void *) 32);
  Mcoding_utf_32be = mconv_define_coding ("utf-32be", param,
					  NULL, NULL, NULL, NULL);

  mplist_put (param, Mcode_unit, (void *) 16);
  mplist_put (param, Mlittle_endian, Mt);
  Mcoding_utf_16le = mconv_define_coding ("utf-16le", param,
					  NULL, NULL, NULL, NULL);

  mplist_put (param, Mcode_unit, (void *) 32);
  Mcoding_utf_32le = mconv_define_coding ("utf-32le", param,
					  NULL, NULL, NULL, NULL);

  mplist_put (param, Mtype, Mnil);
  pl = mplist ();
  mplist_add (pl, Msymbol, msymbol ("Shift_JIS"));
  mplist_put (param, Maliases, pl);
  mplist_set (charsets, Msymbol, Mcharset_ascii);
  Mcoding_sjis = mconv_define_coding ("sjis", param,
				      reset_coding_sjis,
				      decode_coding_sjis,
				      encode_coding_sjis, NULL);

  M17N_OBJECT_UNREF (charsets);
  M17N_OBJECT_UNREF (param);
  M17N_OBJECT_UNREF (pl);

  return 0;
}

void
mcoding__fini (void)
{
  int i;
  MPlist *plist;

  for (i = 0; i < coding_list.used; i++)
    {
      MCodingSystem *coding = coding_list.codings[i];

      if (coding->extra_info)
	free (coding->extra_info);
      if (coding->extra_spec)
	{
	  if (coding->type == Miso_2022)
	    free (((struct iso_2022_spec *) coding->extra_spec)->designations);
	  free (coding->extra_spec);
	}
      free (coding);
    }
  MLIST_FREE1 (&coding_list, codings);
  MPLIST_DO (plist, coding_definition_list)
    M17N_OBJECT_UNREF (MPLIST_VAL (plist));
  M17N_OBJECT_UNREF (coding_definition_list);
}

void
mconv__register_charset_coding (MSymbol sym)
{
  MSymbol name = msymbol__canonicalize (sym);

  if (! mplist_find_by_key (coding_definition_list, name))
    {
      MPlist *param = mplist (), *charsets = mplist ();

      mplist_set (charsets, Msymbol, sym);
      mplist_add (param, Msymbol, sym);
      mplist_add (param, Mtype, Mcharset);
      mplist_add (param, Mcharsets, charsets);
      mplist_put (coding_definition_list, name, param);
      M17N_OBJECT_UNREF (charsets);
    }
}


int
mcoding__load_from_database ()
{
  MDatabase *mdb = mdatabase_find (msymbol ("coding-list"), Mnil, Mnil, Mnil);
  MPlist *def_list, *plist;
  MPlist *definitions = coding_definition_list;
  int mdebug_flag = MDEBUG_CODING;

  if (! mdb)
    return 0;
  MDEBUG_PUSH_TIME ();
  def_list = (MPlist *) mdatabase_load (mdb);
  MDEBUG_PRINT_TIME ("CODING", (mdebug__output, " to load the data."));
  MDEBUG_POP_TIME ();
  if (! def_list)
    return -1;

  MDEBUG_PUSH_TIME ();
  MPLIST_DO (plist, def_list)
    {
      MPlist *pl, *aliases;
      MSymbol name, canonicalized;

      if (! MPLIST_PLIST_P (plist))
	MERROR (MERROR_CHARSET, -1);
      pl = MPLIST_PLIST (plist);
      if (! MPLIST_SYMBOL_P (pl))
	MERROR (MERROR_CHARSET, -1);
      name = MPLIST_SYMBOL (pl);
      canonicalized = msymbol__canonicalize (name);
      pl = mplist__from_plist (MPLIST_NEXT (pl));
      mplist_push (pl, Msymbol, name);
      definitions = mplist_add (definitions, canonicalized, pl);
      aliases = mplist_get (pl, Maliases);
      if (aliases)
	MPLIST_DO (aliases, aliases)
	  if (MPLIST_SYMBOL_P (aliases))
	    {
	      name = MPLIST_SYMBOL (aliases);
	      canonicalized = msymbol__canonicalize (name);
	      definitions = mplist_add (definitions, canonicalized, pl);
	      M17N_OBJECT_REF (pl);
	    }
    }

  M17N_OBJECT_UNREF (def_list);
  MDEBUG_PRINT_TIME ("CODING", (mdebug__output, " to parse the loaded data."));
  MDEBUG_POP_TIME ();
  return 0;
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

/*** @addtogroup m17nConv
     @{ */

/***en @name Variables: Symbols representing coding systems */
/***ja @name 変数: 定義済みコード系を指定するためのシンボル */
/*** @{ */
/*=*/

/***en
    @brief Symbol for the coding system US-ASCII.

    The symbol #Mcoding_us_ascii has name <tt>"us-ascii"</tt> and
    represents a coding system for the CES US-ASCII.  */

/***ja
    @brief US-ASCII コード系のシンボル.

    シンボル #Mcoding_us_ascii は <tt>"us-ascii"</tt> という名前を持ち、
    CES US-ASCII 用のコード系を示す。
    */
MSymbol Mcoding_us_ascii;
/*=*/

/***en
    @brief Symbol for the coding system ISO-8859-1.

    The symbol #Mcoding_iso_8859_1 has name <tt>"iso-8859-1"</tt> and
    represents a coding system for the CES ISO-8859-1.  */

/***ja
    @brief ISO-8859-1 コード系のシンボル.

    シンボル #Mcoding_iso_8859_1 は <tt>"iso-8859-1"</tt> 
    という名前を持ち、CES ISO-8859-1 用のコード系を示す。  */

MSymbol Mcoding_iso_8859_1;
/*=*/

/***en
    @brief Symbol for the coding system UTF-8.

    The symbol #Mcoding_utf_8 has name <tt>"utf-8"</tt> and represents
    a coding system for the CES UTF-8.  */

/***ja
    @brief UTF-8 コード系のシンボル.

    シンボル #Mcoding_utf_8 は <tt>"utf-8"</tt> という名前を持ち、CES
    UTF-8 用のコード系を示す。
     */

MSymbol Mcoding_utf_8;
/*=*/

/***en
    @brief Symbol for the coding system UTF-8-FULL.

    The symbol #Mcoding_utf_8_full has name <tt>"utf-8-full"</tt> and
    represents a coding system that is a extension of UTF-8.  This
    coding system uses the same encoding algorithm as UTF-8 but is not
    limited to the Unicode characters.  It can encode all characters
    supported by the m17n library.  */

/***ja
    @brief UTF-8-FULL コード系のシンボル.

    シンボル #Mcoding_utf_8_full は <tt>"utf-8-full"</tt> 
    という名前を持ち、<tt>"UTF-8"</tt> の拡張であるコード系を示す。
    このコード系は UTF-8 と同じエンコーディングアルゴリズムを用いるが、対象は
    Unicode 文字には限定されない。
    またm17n ライブラリが扱う全ての文字をエンコードすることができる。
    */

MSymbol Mcoding_utf_8_full;
/*=*/

/***en
    @brief Symbol for the coding system UTF-16.

    The symbol #Mcoding_utf_16 has name <tt>"utf-16"</tt> and
    represents a coding system for the CES UTF-16 (RFC 2279).  */
/***ja
    @brief UTF-16 コード系のシンボル.

    シンボル #Mcoding_utf_16 は <tt>"utf-16"</tt> という名前を持ち、
    CES UTF-16 (RFC 2279) 用のコード系を示す。
     */

MSymbol Mcoding_utf_16;
/*=*/

/***en
    @brief Symbol for the coding system UTF-16BE.

    The symbol #Mcoding_utf_16be has name <tt>"utf-16be"</tt> and
    represents a coding system for the CES UTF-16BE (RFC 2279).  */

/***ja
    @brief UTF-16BE コード系のシンボル.

    シンボル #Mcoding_utf_16be は <tt>"utf-16be"</tt> という名前を持ち、
    CES UTF-16BE (RFC 2279) 用のコード系を示す。     */

MSymbol Mcoding_utf_16be;
/*=*/

/***en
    @brief Symbol for the coding system UTF-16LE.

    The symbol #Mcoding_utf_16le has name <tt>"utf-16le"</tt> and
    represents a coding system for the CES UTF-16LE (RFC 2279).  */

/***ja
    @brief UTF-16LE コード系のシンボル.

    シンボル #Mcoding_utf_16le は <tt>"utf-16le"</tt> という名前を持ち、
    CES UTF-16LE (RFC 2279) 用のコード系を示す。     */

MSymbol Mcoding_utf_16le;
/*=*/

/***en
    @brief Symbol for the coding system UTF-32.

    The symbol #Mcoding_utf_32 has name <tt>"utf-32"</tt> and
    represents a coding system for the CES UTF-32 (RFC 2279).  */

/***ja
    @brief UTF-32 コード系のシンボル.

    シンボル #Mcoding_utf_32 は <tt>"utf-32"</tt> という名前を持ち、
    CES UTF-32 (RFC 2279) 用のコード系を示す。     */

MSymbol Mcoding_utf_32;
/*=*/

/***en
    @brief Symbol for the coding system UTF-32BE.

    The symbol #Mcoding_utf_32be has name <tt>"utf-32be"</tt> and
    represents a coding system for the CES UTF-32BE (RFC 2279).  */
/***ja
    @brief UTF-32BE コード系のシンボル.

    シンボル #Mcoding_utf_32be は <tt>"utf-32be"</tt> という名前を持ち、
    CES UTF-32BE (RFC 2279) 用のコード系を示す。     */

MSymbol Mcoding_utf_32be;
/*=*/

/***en
    @brief Symbol for the coding system UTF-32LE.

    The symbol #Mcoding_utf_32le has name <tt>"utf-32le"</tt> and
    represents a coding system for the CES UTF-32LE (RFC 2279).  */
/***ja
    @brief UTF-32LE コード系のシンボル.

    シンボル #Mcoding_utf_32le は <tt>"utf-32le"</tt> という名前を持ち、
    CES UTF-32LE (RFC 2279) 用のコード系を示す。     */

MSymbol Mcoding_utf_32le;
/*=*/

/***en
    @brief Symbol for the coding system SJIS.

    The symbol #Mcoding_sjis has name <tt>"sjis"</tt> and represents a coding
    system for the CES Shift-JIS.  */ 
/***ja
    @brief SJIS コード系のシンボル.

    シンボル #Mcoding_sjis has は <tt>"sjis"</tt> という名前を持ち、
    CES Shift-JIS用のコード系を示す。  */ 

MSymbol Mcoding_sjis;
/*** @} */ 
/*=*/

/***en
    @name Variables: Parameter keys for mconv_define_coding ().  */
/***ja
    @name 変数:  mconv_define_coding () 用パラメータキー  */
/*** @{ */
/*=*/

/***en
    Parameter key for mconv_define_coding () (which see). */ 
/***ja
    mconv_define_coding () 用パラメータキー (詳細は mconv_define_coding ()参照). */ 
MSymbol Mtype;
MSymbol Mcharsets;
MSymbol Mflags;
MSymbol Mdesignation;
MSymbol Minvocation;
MSymbol Mcode_unit;
MSymbol Mbom;
MSymbol Mlittle_endian;
/*** @} */
/*=*/

/***en
    @name Variables: Symbols representing coding system types.  */
/***ja
    @name 変数： コード系のタイプを示すシンボル.  */
/*** @{ */
/*=*/

/***en
    Symbol that can be a value of the #Mtype parameter of a coding
    system used in an argument to the mconv_define_coding () function
    (which see).  */
/***ja 
    関数 mconv_define_coding () の引数として用いられるコード系のパラメータ
    #Mtype の値となり得るシンボル。(詳細は 
    mconv_define_coding ()参照)。  */
 
MSymbol Mutf;
/*=*/
MSymbol Miso_2022;
/*=*/
/*** @} */
/*=*/

/***en
    @name Variables: Symbols appearing in the value of Mflags parameter.  */
/***ja
    @name 変数： パラメータ Mflags の値となり得るシンボル.  */
/*** @{ */
/***en
    Symbols that can be a value of the @b Mflags parameter of a coding
    system used in an argument to the mconv_define_coding () function
    (which see).  */
/***ja 
    関数 mconv_define_coding () の引数として用いられるコード系のパラメータ
    @b Mflags の値となり得るシンボル。(詳細は 
    mconv_define_coding ()参照)。  */
MSymbol Mreset_at_eol;
MSymbol Mreset_at_cntl;
MSymbol Meight_bit;
MSymbol Mlong_form;
MSymbol Mdesignation_g0;
MSymbol Mdesignation_g1;
MSymbol Mdesignation_ctext;
MSymbol Mdesignation_ctext_ext;
MSymbol Mlocking_shift;
MSymbol Msingle_shift;
MSymbol Msingle_shift_7;
MSymbol Meuc_tw_shift;
MSymbol Miso_6429;
MSymbol Mrevision_number;
MSymbol Mfull_support;
/*** @} */
/*=*/

/***en
    @name Variables: Others

    Remaining variables.  */
/***ja @name 変数: その他 

    ほかの変数。 */
/*** @{ */
/*=*/
/***en
    @brief Symbol whose name is "maybe".

    The variable #Mmaybe is a symbol of name <tt>"maybe"</tt>.  It is
    used a value of @b Mbom parameter of the function
    mconv_define_coding () (which see).  */
/***ja
    @brief "maybe"という名前を持つシンボル.

    変数 #Mmaybe は <tt>"maybe"</tt> という名前を持つ。これは関数 
    mconv_define_coding () パラメータ @b Mbom の値として用いられる。
    (詳細は mconv_define_coding () 参照)。 */

MSymbol Mmaybe;
/*=*/

/***en
    @brief The symbol @c Mcoding.

    Any decoded M-text has a text property whose key is the predefined
    symbol @c Mcoding.  The name of @c Mcoding is
    <tt>"coding"</tt>.  */

/***ja
    @brief シンボル @c Mcoding.

    デコードされた M-text はすべて、キーが定義済みシンボル @c Mcoding 
    であるようなテキストプロパティを持つ。シンボル @c Mcoding は 
    <tt>"coding"</tt> という名前を持つ。  */

MSymbol Mcoding;
/*=*/
/*** @} */ 

/***en
    @brief Define a coding system.

    The mconv_define_coding () function defines a new coding system
    and makes it accessible via a symbol whose name is $NAME.  $PLIST
    specifies parameters of the coding system as below:

    <ul>

    <li> Key is @c Mtype, value is a symbol

    The value specifies the type of the coding system.  It must be
    @b Mcharset, @b Mutf, @b Miso_2022, or @b Mnil.

    If the type is @b Mcharset, $EXTRA_INFO is ignored.

    If the type is @b Mutf, $EXTRA_INFO must be a pointer to
    #MCodingInfoUTF.

    If the type is @b Miso_2022, $EXTRA_INFO must be a pointer to
    #MCodingInfoISO2022.

    If the type is #Mnil, the argument $RESETTER, $DECODER, and
    $ENCODER must be supplied.  $EXTRA_INFO is ignored.  Otherwise,
    they can be @c NULL and the m17n library provides proper defaults.

    <li> Key is @b Mcharsets, value is a plist

    The value specifies a list charsets supported by the coding
    system.  The keys of the plist must be #Msymbol, and the values
    must be symbols representing charsets.

    <li> Key is @b Mflags, value is a plist

    If the type is @b Miso_2022, the values specifies flags to control
    the ISO 2022 interpreter.  The keys of the plist must e #Msymbol,
    and values must be one of the following.

    <ul>

    <li> @b Mreset_at_eol

    If this flag exists, designation and invocation status is reset to
    the initial state at the end of line.

    <li> @b Mreset_at_cntl

    If this flag exists, designation and invocation status is reset to
    the initial state at a control character.

    <li> @b Meight_bit

    If this flag exists, the graphic plane right is used.

    <li> @b Mlong_form

    If this flag exists, the over-long escape sequences (ESC '$' '('
    \<final_byte\>) are used for designating the CCS JISX0208.1978,
    GB2312, and JISX0208.

    <li> @b Mdesignation_g0

    If this flag and @b Mfull_support exists, designates charsets not
    listed in the charset list to the graphic register G0.

    <li> @b Mdesignation_g1

    If this flag and @b Mfull_support exists, designates charsets not
    listed in the charset list to the graphic register G1.

    <li> @b Mdesignation_ctext

    If this flag and @b Mfull_support exists, designates charsets not
    listed in the charset list to a graphic register G0 or G1 based on
    the criteria of the Compound Text.

    <li> @b Mdesignation_ctext_ext

    If this flag and @b Mfull_support exists, designates charsets not
    listed in the charset list to a graphic register G0 or G1, or use
    extended segment for such charsets based on the criteria of the
    Compound Text.

    <li> @b Mlocking_shift

    If this flag exists, use locking shift.

    <li> @b Msingle_shift

    If this flag exists, use single shift.

    <li> @b Msingle_shift_7

    If this flag exists, use 7-bit single shift code (0x19).

    <li> @b Meuc_tw_shift

    If this flag exists, use a special shifting according to EUC-TW.

    <li> @b Miso_6429

    This flag is currently ignored.

    <li> @b Mrevision_number

    If this flag exists, use a revision number escape sequence to
    designate a charset that has a revision number.

    <li> @b Mfull_support

    If this flag exists, support all charsets registered in the
    International Registry.

    </ul>

    <li> Key is @b Mdesignation, value is a plist

    If the type is @b Miso_2022, the value specifies how to designate
    each supported characters.  The keys of the plist must be 
    #Minteger, and the values must be numbers indicating a graphic
    registers.  The Nth element value is for the Nth charset of the
    charset list.  The value 0..3 means that it is assumed that a
    charset is already designated to the graphic register 0..3.  The
    negative value G (-4..-1) means that a charset is not designated
    to any register at first, and if necessary, is designated to the
    (G+4) graphic register.

    <li> Key is @b Minvocation, value is a plist

    If the type is @b Miso_2022, the value specifies how to invocate
    each graphic registers.  The plist length must be one or two.  The
    keys of the plist must be #Minteger, and the values must be
    numbers indicating a graphic register.  The value of the first
    element specifies which graphic register is invocated to the
    graphic plane left.  If the length is one, no graphic register is
    invocated to the graphic plane right.  Otherwise, the value of the
    second element specifies which graphic register is invocated to
    the graphic plane right.

    <li> Key is @b Mcode_unit, value is an integer

    If the type is @b Mutf, the value specifies the bit length of a
    code-unit.  It must be 8, 16, or 32.

    <li> Key is @b Mbom, value is a symbol

    If the type is @b Mutf and the code-unit bit length is 16 or 32,
    it specifies whether or not to use BOM (Byte Order Mark).  If the
    value is #Mnil (default), BOM is not used, else if the value is
    #Mmaybe, the existence of BOM is detected at decoding time, else
    BOM is used.

    <li> Key is @b Mlittle_endian, value is a symbol

    If the type is @b Mutf and the code-unit bit length is 16 or 32,
    it specifies whether or not the encoding is little endian.  If the
    value is #Mnil (default), it is big endian, else it is little
    endian.

    </ul>

    $RESETTER is a pointer to a function that resets a converter for
    the coding system to the initial status.  The pointed function is
    called with one argument, a pointer to a converter object.

    $DECODER is a pointer to a function that decodes a byte sequence
    according to the coding system.  The pointed function is called
    with four arguments:

	@li A pointer to the byte sequence to decode.
	@li The number of bytes to decode.
	@li A pointer to an M-text to which the decoded characters are appended.
	@li A pointer to a converter object.

    $DECODER must return 0 if it succeeds.  Otherwise it must return -1.

    $ENCODER is a pointer to a function that encodes an M-text
    according to the coding system.  The pointed function is called
    with six arguments:

	@li A pointer to the M-text to encode.
	@li The starting position of the encoding.
	@li The ending position of the encoding.
	@li A pointer to a memory area where the produced bytes are stored.
	@li The size of the memory area.
	@li A pointer to a converter object.

    $ENCODER must return 0 if it succeeds.  Otherwise it must return -1.

    $EXTRA_INFO is a pointer to a data structure that contains extra
    information about the coding system.  The type of the data
    structure depends on $TYPE.

    @return  

    If the operation was successful, mconv_define_coding () returns a
    symbol whose name is $NAME.  If an error is detected, it returns
    #Mnil and assigns an error code to the external variable #merror_code.  */

/***ja
    @brief コード系を定義する.

    関数 mconv_define_coding () は、新しいコード系を定義し、それを 
    $NAME という名前のシンボル経由でアクセスできるようにする。 $PLIST 
    では定義するコード系のパラメータを以下のように指定する。

    <ul>

    <li> キーが @c Mtype で値がシンボルの時

    値はコード系のタイプを表し、@b Mcharset, @b Mutf, @b Miso_2022, #Mnil 
    のいずれかでなくてはならない。

    タイプが @b Mcharset ならば $EXTRA_INFO は無視される。

    タイプが @b Mutf ならば $EXTRA_INFO は #MCodingInfoUTF 
    へのポインタでなくてはならない。

    タイプが @b Miso_2022ならば $EXTRA_INFO は #MCodingInfoISO2022 
    へのポインタでなくてはならない。

    タイプが #Mnil ならば、引数 $RESETTER, $DECODER, $ENCODER 
    を与えなくてはならない。$EXTRA_INFO は無視される。
    それ以外の場合にはこれらは @c NULL でよく、
    m17n ライブラリが適切なデフォルト値を与える。

    <li> キーが @b Mcharsets で値が plist の時

    値はこのコード系でサポートされる文字セットのリストである。plistのキーは
    #Msymbol、値は文字セットを示すシンボルでなくてはならない。

    <li> キーが @b Mflags 値が plist の時

    タイプが @b Miso_2022 ならば、この値は, ISO 2022 
    インタプリタ用の制御フラッグを示す。plist のキーは #Msymbol 
    であり、値は以下のいずれかである。

    <ul>

    <li> @b Mreset_at_eol

    このフラグがあれば、図形文字集合の指示や呼出は行末でリセットされて当初の状態に戻る。

    <li> @b Mreset_at_cntl

    このフラグがあれば、図形文字集合の指示や呼出は制御文字に出会った時点でリセットされて当初の状態に戻る。

    <li> @b Meight_bit

    このフラグがあれば、図形文字集合の右半面が用いられる。

    <li> @b Mlong_form

    このフラグがあれば、文字集合 JISX0208.1978, GB2312, JISX0208 
    を指示する際に over-long エスケープシーケンス (ESC '$' '('
    \<final_byte\>) が用いられる。

    <li> @b Mdesignation_g0

    このフラグと @b Mfull_support があれば、文字セットリストに現われない文字セットを
    G0 集合に指示する。

    <li> @b Mdesignation_g1

    このフラグと @b Mfull_support があれば、文字セットリストに現われない文字セットを
    G1 集合に指示する。

    <li> @b Mdesignation_ctext

    このフラグと @b Mfull_support があれば、文字セットリストに現われない文字セットを
    G0 集合または G1 集合に、コンパウンドテキストの基準にそって指示する。

    <li> @b Mdesignation_ctext_ext

    このフラグと @b Mfull_support があれば、文字セットリストに現われない文字セットを
    G0 集合または G1 集合に、あるいは拡張セグメントにコンパウンドテキストの基準にそって指示する。

    <li> @b Mlocking_shift

    このフラグがあれば、ロッキングシフトを用いる。

    <li> @b Msingle_shift

    このフラグがあれば、シングルシフトを用いる。

    <li> @b Msingle_shift_7

    このフラグがあれば、7-bit シングルシフトコード (0x19) を用いる。   
    
    <li> @b Meuc_tw_shift

    このフラグがあれば、EUC-TW に沿った特別なシフトを用いる。

    <li> @b Miso_6429

    現時点では用いられていない。

    <li> @b Mrevision_number

    このフラグがあれば、revision number を持つ文字セットを指示する際に 
    revision number エスケープシークエンスを用いる。

    <li> @b Mfull_support

    このフラグがあれば、the International Registry 
    に登録されている全文字セットをサポートする。

    </ul>

    <li> キーが @b Mdesignation で値が plist の時

    タイプが @b Miso_2022 ならば、値は各文字をどのように指示するかを示す。
    plist のキーは #Minteger、値は集合（graphic register）
    を示す数字である。N番目の要素の値は、文字セットリストの N 
    番目の文字セットに対応する。値が 0..3 であれば、文字セットがすでに 
    G0..G3 に指示 されている。

    値が負(-4..-1) であれば、初期状態では文字セットがどこにも指示されていないこと、必要な際には
    G0..G3 のそれぞれに指示することを意味する。

    <li> キーが @b Minvocation で値が plist の時

    タイプが @b Miso_2022 ならば、値は各集合をどのように呼び出すかを示す。
    plist の長さは 1 ないし 2 である。plist のキーは 
    #Minteger、値は集合（graphic register)を示す数字である。
    最初の要素の値が図形文字集合左半面に呼び出される集合を示す。
    plist の長さが 1 ならば、右半面には何も呼び出されない。
    そうでければ、２つめの要素の値が図形文字集合右半面に呼び出される集合を示す。

    <li> キーが @b Mcode_unit で値が整数値の時

    タイプが @b Mutf ならば、値はコードユニットのビット長であり、8, 16,
    32 のいずれかである。

    <li> キーが @b Mbom で値がシンボルの時

    タイプが @b Mutf でコードユニットのビット長が 16 か 32ならば、値は
    BOM (Byte Order Mark) を使用するかどうかを示す。値がデフォルト値の 
    #Mnil ならば、使用しない。値が #Mmaybe ならばデコード時に BOM 
    があるかどうかを調べる。それ以外ならば使用する。

    <li> キーが @b Mlittle_endian で値がシンボルの時 

    タイプが @b Mutf でコードユニットのビット長が 16 か 32
    ならば、値はエンコードが little endian かどうかを示す。値がデフォルト値の
    #Mnil ならば big endian であり、そうでなければ little endian である。

    </ul>

    $RESETTER 
    はこのコード系用のコンバータを初期状態にリセットする関数へのポインタである。
    この関数はコンバータオブジェクトへのポインタという１引数をとる。

    $DECODER はバイト列をこのコード系に従ってデコードする関数へのポインタである。
    この関数は以下の４引数をとる。

	@li デコードするバイト列へのポインタ
	@li デコードすべきバイト数
	@li デコード結果の文字を付加する M-text へのポインタ
	@li コンバータオブジェクトへのポインタ

    $DECODER は成功したときには 0 を、失敗したときには -1 
    を返さなくてはならない。

    $ENCODER は M-text をこのコード系に従ってエンコードする関数へのポインタである。
    この関数は以下の６引数をとる。

        @li エンコードするM-text へのポインタ
        @li M-text のエンコード開始位置
        @li M-text のエンコード終了位置
        @li 生成したバイトを保持するメモリ領域へのポインタ
        @li メモリ領域のサイズ
	@li コンバータオブジェクトへのポインタ

    $ENCODER は成功したときには 0 を、失敗したときには -1 
    を返さなくてはならない。

    $EXTRA_INFO はコーディグシステムに関する追加情報を含むデータ構造へのポインタである。
    このデータ構造の型 $TYPE に依存する。

    @return  

    処理に成功すれば mconv_define_coding () は $NAME 
    という名前のシンボルを返す。 エラーが検出された場合は #Mnil
    を返し、外部変数 #merror_code にエラーコードを設定する。
      */

/***
    @errors
    @c MERROR_CODING  */

MSymbol
mconv_define_coding (const char *name, MPlist *plist,
		     int (*resetter) (MConverter *),
		     int (*decoder) (const unsigned char *, int, MText *,
				     MConverter *),
		     int (*encoder) (MText *, int, int,
				     unsigned char *, int,
				     MConverter *),
		     void *extra_info)
{
  MSymbol sym = msymbol (name);
  int i;
  MCodingSystem *coding;
  MPlist *pl;

  MSTRUCT_MALLOC (coding, MERROR_CODING);
  coding->name = sym;
  if ((coding->type = (MSymbol) mplist_get (plist, Mtype)) == Mnil)
    coding->type = Mcharset;
  pl = (MPlist *) mplist_get (plist, Mcharsets);
  if (! pl)
    MERROR (MERROR_CODING, Mnil);
  coding->ncharsets = mplist_length (pl);
  if (coding->ncharsets > NUM_SUPPORTED_CHARSETS)
    coding->ncharsets = NUM_SUPPORTED_CHARSETS;
  for (i = 0; i < coding->ncharsets; i++, pl = MPLIST_NEXT (pl))
    {
      MSymbol charset_name;

      if (MPLIST_KEY (pl) != Msymbol)
	MERROR (MERROR_CODING, Mnil);
      charset_name = MPLIST_SYMBOL (pl);
      if (! (coding->charsets[i] = MCHARSET (charset_name)))
	MERROR (MERROR_CODING, Mnil);
    }

  coding->resetter = resetter;
  coding->decoder = decoder;
  coding->encoder = encoder;
  coding->ascii_compatible = 0;
  coding->extra_info = extra_info;
  coding->extra_spec = NULL;
  coding->ready = 0;

  if (coding->type == Mcharset)
    {
      if (! coding->resetter)
	coding->resetter = reset_coding_charset;
      if (! coding->decoder)
	coding->decoder = decode_coding_charset;
      if (! coding->encoder)
	coding->encoder = encode_coding_charset;
    }
  else if (coding->type == Mutf)
    {
      MCodingInfoUTF *info = malloc (sizeof (MCodingInfoUTF));
      MSymbol val;

      if (! coding->resetter)
	coding->resetter = reset_coding_utf;

      info->code_unit_bits = (int) mplist_get (plist, Mcode_unit);
      if (info->code_unit_bits == 8)
	{
	  if (! coding->decoder)
	    coding->decoder = decode_coding_utf_8;
	  if (! coding->encoder)
	    coding->encoder = encode_coding_utf_8;
	}
      else if (info->code_unit_bits == 16)
	{
	  if (! coding->decoder)
	    coding->decoder = decode_coding_utf_16;
	  if (! coding->encoder)
	    coding->encoder = encode_coding_utf_16;
	}
      else if (info->code_unit_bits == 32)
	{
	  if (! coding->decoder)
	    coding->decoder = decode_coding_utf_32;
	  if (! coding->encoder)
	    coding->encoder = encode_coding_utf_32;
	}
      else
	MERROR (MERROR_CODING, Mnil);
      val = (MSymbol) mplist_get (plist, Mbom);
      if (val == Mnil)
	info->bom = 1;
      else if (val == Mmaybe)
	info->bom = 0;
      else
	info->bom = 2;

      info->endian = (mplist_get (plist, Mlittle_endian) ? 1 : 0);
      coding->extra_info = info;
    }
  else if (coding->type == Miso_2022)
    {
      MCodingInfoISO2022 *info = malloc (sizeof (MCodingInfoISO2022));

      if (! coding->resetter)
	coding->resetter = reset_coding_iso_2022;
      if (! coding->decoder)
	coding->decoder = decode_coding_iso_2022;
      if (! coding->encoder)
	coding->encoder = encode_coding_iso_2022;

      info->initial_invocation[0] = 0;
      info->initial_invocation[1] = -1;
      pl = (MPlist *) mplist_get (plist, Minvocation);
      if (pl)
	{
	  if (MPLIST_KEY (pl) != Minteger)
	    MERROR (MERROR_CODING, Mnil);
	  info->initial_invocation[0] = MPLIST_INTEGER (pl);
	  if (! MPLIST_TAIL_P (pl))
	    {
	      pl = MPLIST_NEXT (pl);
	      if (MPLIST_KEY (pl) != Minteger)
		MERROR (MERROR_CODING, Mnil);
	      info->initial_invocation[1] = MPLIST_INTEGER (pl);
	    }
	}
      memset (info->designations, 0, sizeof (info->designations));
      for (i = 0, pl = (MPlist *) mplist_get (plist, Mdesignation);
	   i < 32 && pl && MPLIST_KEY (pl) == Minteger;
	   i++, pl = MPLIST_NEXT (pl))
	info->designations[i] = MPLIST_INTEGER (pl);

      info->flags = 0;
      MPLIST_DO (pl, (MPlist *) mplist_get (plist, Mflags))
	{
	  MSymbol val;

	  if (MPLIST_KEY (pl) != Msymbol)
	    MERROR (MERROR_CODING, Mnil);
	  val = MPLIST_SYMBOL (pl);
	  if (val == Mreset_at_eol)
	    info->flags |= MCODING_ISO_RESET_AT_EOL;
	  else if (val == Mreset_at_cntl)
	    info->flags |= MCODING_ISO_RESET_AT_CNTL;
	  else if (val == Meight_bit)
	    info->flags |= MCODING_ISO_EIGHT_BIT;
	  else if (val == Mlong_form)
	    info->flags |= MCODING_ISO_LOCKING_SHIFT;
	  else if (val == Mdesignation_g0)
	    info->flags |= MCODING_ISO_DESIGNATION_G0;
	  else if (val == Mdesignation_g1)
	    info->flags |= MCODING_ISO_DESIGNATION_G1;
	  else if (val == Mdesignation_ctext)
	    info->flags |= MCODING_ISO_DESIGNATION_CTEXT;
	  else if (val == Mdesignation_ctext_ext)
	    info->flags |= MCODING_ISO_DESIGNATION_CTEXT_EXT;
	  else if (val == Mlocking_shift)
	    info->flags |= MCODING_ISO_LOCKING_SHIFT;
	  else if (val == Msingle_shift)
	    info->flags |= MCODING_ISO_SINGLE_SHIFT;
	  else if (val == Msingle_shift_7)
	    info->flags |= MCODING_ISO_SINGLE_SHIFT_7;
	  else if (val == Meuc_tw_shift)
	    info->flags |= MCODING_ISO_EUC_TW_SHIFT;
	  else if (val == Miso_6429)
	    info->flags |= MCODING_ISO_ISO6429;
	  else if (val == Mrevision_number)
	    info->flags |= MCODING_ISO_REVISION_NUMBER;
	  else if (val == Mfull_support)
	    info->flags |= MCODING_ISO_FULL_SUPPORT;
	}

      coding->extra_info = info;
    }
  else
    {
      if (! coding->decoder || ! coding->encoder)
	MERROR (MERROR_CODING, Mnil);
      if (! coding->resetter)
	coding->ready = 1;
    }

  msymbol_put (sym, Mcoding, coding);
  msymbol_put (msymbol__canonicalize (sym), Mcoding, coding);
  plist = (MPlist *) mplist_get (plist, Maliases);
  if (plist)
    {
      MPLIST_DO (pl, plist)
	{
	  MSymbol alias;

	  if (MPLIST_KEY (pl) != Msymbol)
	    continue;
	  alias = MPLIST_SYMBOL (pl);
	  msymbol_put (alias, Mcoding, coding);
	  msymbol_put (msymbol__canonicalize (alias), Mcoding, coding);
	}
    }

  MLIST_APPEND1 (&coding_list, codings, coding, MERROR_CODING);

  return sym;
}

/*=*/

/***en
    @brief Resolve coding system name.

    The mconv_resolve_coding () function returns $SYMBOL if it
    represents a coding system.  Otherwise, canonicalize $SYMBOL as to
    a coding system name, and if the canonicalized name represents a
    coding system, return it.  Otherwise, return #Mnil.  */
/***ja
    @brief コード系の名前を解決する.

    関数 mconv_resolve_coding () は $SYMBOL がコード系を示していればそれを返す。
    そうでなければコード系の名前として $SYMBOL 
    を正規化し、それがコード系を表していれば正規化した $SYMBOL を返す。
    そうでなければ#Mnil を返す。  */



MSymbol
mconv_resolve_coding (MSymbol symbol)
{
  MCodingSystem *coding = find_coding (symbol);

  if (! coding)
    {
      symbol = msymbol__canonicalize (symbol);
      coding = find_coding (symbol);
    }
  return (coding ? coding->name : Mnil);
}

/*=*/


/***en
    @brief List symbols representing coding systems.

    The mconv_list_codings () function makes an array of symbols
    representing a coding system, stores the pointer to the array in a
    place pointed to by $SYMBOLS, and returns the length of the array.  */
/***ja
    @brief コード系を表わすシンボルを列挙する.

    関数 mchar_list_codings () は、コード系を示すシンボルを並べた配列を作り、
    $SYMBOLS でポイントされた場所にこの配列へのポインタを置き、配列の長さを返す。 */

int
mconv_list_codings (MSymbol **symbols)
{
  int i = coding_list.used + mplist_length (coding_definition_list);
  int j;
  MPlist *plist;

  MTABLE_MALLOC ((*symbols), i, MERROR_CODING);
  i = 0;
  MPLIST_DO (plist, coding_definition_list)
    {
      MPlist *pl = MPLIST_VAL (plist);
      (*symbols)[i++] = MPLIST_SYMBOL (pl);
    }
  for (j = 0; j < coding_list.used; j++)
    if (! mplist_find_by_key (coding_definition_list, 
			      coding_list.codings[j]->name))
      (*symbols)[i++] = coding_list.codings[j]->name;
  return i;
}

/*=*/

/***en
    @brief Create a code converter bound to a buffer.

    The mconv_buffer_converter () function creates a pointer to a code
    converter for coding system $NAME.  The code converter is bound
    to buffer area of $N bytes pointed to by $BUF.  Subsequent
    decodings and encodings are done to/from this buffer area.

    $NAME can be #Mnil.  In this case, a coding system associated
    with the current locale (LC_CTYPE) is used.

    @return
    If the operation was successful, mconv_buffer_converter () returns
    the created code converter.  Otherwise it returns @c NULL and
    assigns an error code to the external variable #merror_code.  */

/***ja
    @brief バッファに結び付けられたコードコンバータを作る.

    関数 mconv_buffer_converter () は、コード系 $NAME 
    用のコードコンバータを作る。このコードコンバータは、$BUF で示される大きさ $N 
    バイトのバッファ領域に結び付けられる。
    これ以降のデコードおよびエンコードは、このバッファ領域に対して行なわれる。

    $NAME は #Mnil であってもよい。この場合は現在のロケール 
    (LC_CTYPE) に関連付けられたコード系が使われる。

    @return
    もし処理が成功すれば mconv_buffer_converter () は 作成したコードコンバータを返す。
    そうでなければ @c NULL を返し、外部変数 #merror_code 
    にエラーコードを設定する。

    @latexonly \IPAlabel{mconverter} @endlatexonly  */

/***
    @errors
    @c MERROR_SYMBOL, @c MERROR_CODING

    @seealso 
    mconv_stream_converter ()  */

MConverter *
mconv_buffer_converter (MSymbol name, const unsigned char *buf, int n)
{
  MCodingSystem *coding;
  MConverter *converter;
  MConverterStatus *internal;

  if (name == Mnil)
    name = mlocale_get_prop (mlocale__ctype, Mcoding);
  coding = find_coding (name);
  if (! coding)
    MERROR (MERROR_CODING, NULL);
  MSTRUCT_CALLOC (converter, MERROR_CODING);
  MSTRUCT_CALLOC (internal, MERROR_CODING);
  converter->internal_info = internal;
  internal->coding = coding;
  if (coding->resetter
      && (*coding->resetter) (converter) < 0)
    {
      free (internal);
      free (converter);
      MERROR (MERROR_CODING, NULL);
    }

  internal->unread = mtext ();
  internal->work_mt = mtext ();
  mtext__enlarge (internal->work_mt, MAX_UTF8_CHAR_BYTES);
  internal->buf.in = buf;
  internal->used = 0;
  internal->bufsize = n;
  internal->binding = BINDING_BUFFER;

  return converter;
}

/*=*/

/***en
    @brief Create a code converter bound to a stream.

    The mconv_stream_converter () function creates a pointer to a code
    converter for coding system $NAME.  The code converter is bound
    to stream $FP.  Subsequent decodings and encodings are done
    to/from this stream.

    $NAME can be #Mnil.  In this case, a coding system associated
    with the current locale (LC_CTYPE) is used.

    @return 
    If the operation was successful, mconv_stream_converter ()
    returns the created code converter.  Otherwise it returns @c NULL
    and assigns an error code to the external variable 
    #merror_code.  */

/***ja
    @brief ストリームに結び付けられたコードコンバータを作る.

    関数 mconv_stream_converter () は、コード系 $NAME 
    用のコードコンバータを作る。このコードコンバータは、ストリーム $FP 
    に結び付けられる。
    これ以降のデコードおよびエンコードは、このストリームに対して行なわれる。

    $NAME は #Mnil であってもよい。この場合は現在のロケール 
    (LC_CTYPE) に関連付けられたコード系が使われる。

    @return 
    もし処理が成功すれば、mconv_stream_converter () 
    は作成したコードコンバータを返す。そうでなければ @c NULL 
    を返し、外部変数 #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mconverter} @endlatexonly  */

/***
    @errors
    @c MERROR_SYMBOL, @c MERROR_CODING

    @seealso
    mconv_buffer_converter ()  */

MConverter *
mconv_stream_converter (MSymbol name, FILE *fp)
{
  MCodingSystem *coding;
  MConverter *converter;
  MConverterStatus *internal;

  if (name == Mnil)
    name = mlocale_get_prop (mlocale__ctype, Mcoding);
  coding = find_coding (name);
  if (! coding)
    MERROR (MERROR_CODING, NULL);
  MSTRUCT_CALLOC (converter, MERROR_CODING);
  MSTRUCT_CALLOC (internal, MERROR_CODING);
  converter->internal_info = internal;
  internal->coding = coding;
  if (coding->resetter
      && (*coding->resetter) (converter) < 0)
    {
      free (internal);
      free (converter);
      MERROR (MERROR_CODING, NULL);
    }

  if (fseek (fp, 0, SEEK_CUR) < 0)
    {
      if (errno == EBADF)
	{
	  free (internal);
	  free (converter);
	  return NULL;
	}
      internal->seekable = 0;
    }
  else
    internal->seekable = 1;
  internal->unread = mtext ();
  internal->work_mt = mtext ();
  mtext__enlarge (internal->work_mt, MAX_UTF8_CHAR_BYTES);
  internal->fp = fp;
  internal->binding = BINDING_STREAM;

  return converter;
}

/*=*/

/***en
    @brief Reset a code converter.

    The mconv_reset_converter () function resets code converter
    $CONVERTER to the initial state.

    @return
    If $CONVERTER->coding has its own reseter function,
    mconv_reset_converter () returns the result of that function
    applied to $CONVERTER.  Otherwise it returns 0.  */

/***ja
    @brief コードコンバータをリセットする.

    関数 mconv_reset_converter () はコードコンバータ $CONVERTER 
    を初期状態に戻す。

    @return
    もし $CONVERTER->coding にリセット用の関数が定義されているならば、
    mconv_reset_converter () はその関数に $CONVERTER 
    を適用した結果を返し、そうでなければ0を返す。  */

int
mconv_reset_converter (MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;

  converter->nchars = converter->nbytes = 0;
  converter->result = MCONVERSION_RESULT_SUCCESS;
  internal->carryover_bytes = 0;
  internal->used = 0;
  mtext_reset (internal->unread);
  if (internal->coding->resetter)
    return (*internal->coding->resetter) (converter);
  return 0;
}

/*=*/

/***en
    @brief Free a code converter.

    The mconv_free_converter () function frees the code converter
    $CONVERTER.  */

/***ja
    @brief コードコンバータを解放する.

    関数 mconv_free_converter () はコードコンバータ $CONVERTER 
    を解放する。  */

void
mconv_free_converter (MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;

  M17N_OBJECT_UNREF (internal->work_mt);
  M17N_OBJECT_UNREF (internal->unread);
  free (internal);
  free (converter);
}

/*=*/

/***en
    @brief Bind a buffer to a code converter.

    The mconv_rebind_buffer () function binds buffer area of $N bytes
    pointed to by $BUF to code converter $CONVERTER.  Subsequent
    decodings and encodings are done to/from this newly bound buffer
    area.

    @return
    This function always returns $CONVERTER.  */

/***ja
    @brief コードコンバータにバッファ領域を結び付ける.

    関数 mconv_rebind_buffer () は、$BUF によって指された大きさ $N 
    バイトのバッファ領域をコードコンバータ $CONVERTER に結び付ける。
    これ以降のデコードおよびエンコードは、この新たに結び付けられたバッファ領域に対して行なわれるようになる。

    @return
    この関数は常に $CONVERTER を返す。

    @latexonly \IPAlabel{mconv_rebind_buffer} @endlatexonly  */

/***
    @seealso
    mconv_rebind_stream () */

MConverter *
mconv_rebind_buffer (MConverter *converter, const unsigned char *buf, int n)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;

  internal->buf.in = buf;
  internal->used = 0;
  internal->bufsize = n;
  internal->binding = BINDING_BUFFER;
  return converter;
}

/*=*/

/***en
    @brief Bind a stream to a code converter.

    The mconv_rebind_stream () function binds stream $FP to code
    converter $CONVERTER.  Following decodings and encodings are done
    to/from this newly bound stream.

    @return
    This function always returns $CONVERTER.  */

/***ja
    @brief コードコンバータにストリームを結び付ける.

    関数 mconv_rebind_stream () は、ストリーム $FP をコードコンバータ 
    $CONVERTER に結び付ける。
    これ以降のデコードおよびエンコードは、この新たに結び付けられたストリームに対して行なわれるようになる。

    @return
    この関数は常に $CONVERTER を返す。

    @latexonly \IPAlabel{mconv_rebind_stream} @endlatexonly  */

/***
    @seealso
    mconv_rebind_buffer () */

MConverter *
mconv_rebind_stream (MConverter *converter, FILE *fp)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;

  if (fseek (fp, 0, SEEK_CUR) < 0)
    {
      if (errno == EBADF)
	return NULL;
      internal->seekable = 0;
    }
  else
    internal->seekable = 1;
  internal->fp = fp;
  internal->binding = BINDING_STREAM;
  return converter;
}

/*=*/

/***en
    @brief Decode a byte sequence into an M-text.

    The mconv_decode () function decodes a byte sequence and appends
    the result at the end of M-text $MT.  The source byte sequence is
    taken from either the buffer area or the stream that is currently
    bound to $CONVERTER.

    @return
    If the operation was successful, mconv_decode () returns updated
    $MT.  Otherwise it returns @c NULL and assigns an error code to
    the external variable #merror_code.  */

/***ja
    @brief バイト列を M-text にデコードする.

    関数 mconv_decode () は、バイト列をデコードしてその結果を M-text
    $MT の末尾に追加する。デコード元のバイト列は、$CONVERTER 
    に現在結び付けられているバッファ領域あるいはストリームから取られる。

    @return
    もし処理が成功すれば、mconv_decode () は更新された $MT を返す。
    そうでなければ @c NULL を返し、外部変数 #merror_code 
    にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_IO, @c MERROR_CODING

    @seealso
    mconv_rebind_buffer (), mconv_rebind_stream (),
    mconv_encode (), mconv_encode_range (),
    mconv_decode_buffer (), mconv_decode_stream ()  */

MText *
mconv_decode (MConverter *converter, MText *mt)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  int at_most = converter->at_most > 0 ? converter->at_most : -1;
  int n;

  M_CHECK_READONLY (mt, NULL);

  if (mt->format != MTEXT_FORMAT_UTF_8)
    mtext__adjust_format (mt, MTEXT_FORMAT_UTF_8);

  if (! mt->data)
    mtext__enlarge (mt, MAX_UTF8_CHAR_BYTES);

  converter->nchars = converter->nbytes = 0;
  converter->result = MCONVERSION_RESULT_SUCCESS;

  n = mtext_nchars (internal->unread);
  if (n > 0)
    {
      int limit = n;
      int i;

      if (at_most > 0 && at_most < limit)
	limit = at_most;

      for (i = 0, n -= 1; i < limit; i++, converter->nchars++, n--)
	mtext_cat_char (mt, mtext_ref_char (internal->unread, n));
      mtext_del (internal->unread, n + 1, internal->unread->nchars);
      if (at_most > 0)
	{
	  if (at_most == limit)
	    return mt;
	  converter->at_most -= converter->nchars;
	}
    }

  if (internal->binding == BINDING_BUFFER)
    {
      (*internal->coding->decoder) (internal->buf.in + internal->used,
				    internal->bufsize - internal->used,
				    mt, converter);
      internal->used += converter->nbytes;
    }  
  else if (internal->binding == BINDING_STREAM)
    {
      unsigned char work[CONVERT_WORKSIZE];
      int last_block = converter->last_block;
      int use_fread = at_most < 0 && internal->seekable;

      converter->last_block = 0;
      while (1)
	{
	  int nbytes, prev_nbytes;

	  if (feof (internal->fp))
	    nbytes = 0;
	  else if (use_fread)
	    nbytes = fread (work, sizeof (unsigned char), CONVERT_WORKSIZE,
			    internal->fp);
	  else
	    {
	      int c = getc (internal->fp);

	      if (c != EOF)
		work[0] = c, nbytes = 1;
	      else
		nbytes = 0;
	    }

	  if (ferror (internal->fp))
	    {
	      converter->result = MCONVERSION_RESULT_IO_ERROR;
	      break;
	    }

	  if (nbytes == 0)
	    converter->last_block = last_block;
	  prev_nbytes = converter->nbytes;
	  (*internal->coding->decoder) (work, nbytes, mt, converter);
	  if (converter->nbytes - prev_nbytes < nbytes)
	    {
	      if (use_fread)
		fseek (internal->fp, converter->nbytes - prev_nbytes - nbytes,
		       SEEK_CUR);
	      else
		ungetc (work[0], internal->fp);
	      break;
	    }
	  if (nbytes == 0
	      || (converter->at_most > 0
		  && converter->nchars == converter->at_most))
	    break;
	}
      converter->last_block = last_block;
    }
  else				/* internal->binding == BINDING_NONE */
    MERROR (MERROR_CODING, NULL);

  converter->at_most = at_most;
  return ((converter->result == MCONVERSION_RESULT_SUCCESS
	   || converter->result == MCONVERSION_RESULT_INSUFFICIENT_SRC)
	  ? mt : NULL);
}

/*=*/

/***en
    @brief Decode a buffer area based on a coding system.

    The mconv_decode_buffer () function decodes $N bytes of the buffer
    area pointed to by $BUF based on the coding system $NAME.  A
    temporary code converter for decoding is automatically created
    and freed.

    @return 
    If the operation was successful, mconv_decode_buffer ()
    returns the resulting M-text.  Otherwise it returns @c NULL and
    assigns an error code to the external variable #merror_code.  */

/***ja
    @brief コード系に基づいてバッファ領域をデコードする.

    関数 mconv_decode_buffer () は、$BUF によって指された $N 
    バイトのバッファ領域を、コード系 $NAME に基づいてデコードする。
    デコードに必要なコードコンバータの作成と解放は自動的に行なわれる。

    @return
    もし処理が成功すれば、mconv_decode_buffer () は得られた M-text を返す。
    そうでなければ @c NULL を返し、外部変数 #merror_code 
    にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_IO, @c MERROR_CODING

    @seealso
    mconv_decode (), mconv_decode_stream ()  */

MText *
mconv_decode_buffer (MSymbol name, const unsigned char *buf, int n)
{
  MConverter *converter = mconv_buffer_converter (name, buf, n);
  MText *mt;

  if (! converter)
    return NULL;
  mt = mtext ();
  if (! mconv_decode (converter, mt))
    {
      M17N_OBJECT_UNREF (mt);
      mt = NULL;
    }
  mconv_free_converter (converter);
  return mt;
}

/*=*/

/***en
    @brief Decode a stream input based on a coding system.

    The mconv_decode_stream () function decodes the entire byte
    sequence read in from stream $FP based on the coding system $NAME.
    A code converter for decoding is automatically created and freed.

    @return
    If the operation was successful, mconv_decode_stream () returns
    the resulting M-text.  Otherwise it returns @c NULL and assigns an
    error code to the external variable #merror_code.  */

/***ja
    @brief コード系に基づいてストリーム入力をデコードする.

    関数 mconv_decode_stream () は、ストリーム $FP 
    から読み込まれるバイト列全体を、コード系 $NAME 
    に基づいてデコードする。デコードに必要なコードコンバータの作成と解放は自動的に行なわれる。

    @return
    もし処理が成功すれば、mconv_decode_stream () は得られた M-text 
    を返す。そうでなければ @c NULL を返し、外部変数 #merror_code 
    にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_IO, @c MERROR_CODING

    @seealso
    mconv_decode (), mconv_decode_buffer ()  */

MText *
mconv_decode_stream (MSymbol name, FILE *fp)
{
  MConverter *converter = mconv_stream_converter (name, fp);
  MText *mt;

  if (! converter)
    return NULL;
  mt = mtext ();
  if (! mconv_decode (converter, mt))
    {
      M17N_OBJECT_UNREF (mt);
      mt = NULL;
    }
  mconv_free_converter (converter);
  return mt;
}

/*=*/

/***en @brief Encode an M-text into a byte sequence.

    The mconv_encode () function encodes M-text $MT and writes the
    resulting byte sequence into the buffer area or the stream that is
    currently bound to code converter $CONVERTER.

    @return
    If the operation was successful, mconv_encode () returns the
    number of written bytes.  Otherwise it returns -1 and assigns an
    error code to the external variable #merror_code.  */

/***ja
    @brief M-text をバイト列にエンコードする.

    関数 mconv_encode () は、M-text $MT をエンコードして、コードコンバータ
    $CONVERTER に現在結び付けられているバッファ領域あるいはストリームに得られたバイト列を書き込む。

    @return
    もし処理が成功すれば、mconv_encode () は書き込まれたバイト数を返す。
    そうでなければ -1 を返し、外部変数 #merror_code 
    にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_IO, @c MERROR_CODING

    @seealso
    mconv_rebind_buffer (), mconv_rebind_stream(),
    mconv_decode (), mconv_encode_range ()  */

int
mconv_encode (MConverter *converter, MText *mt)
{
  return mconv_encode_range (converter, mt, 0, mtext_nchars (mt));
}

/*=*/

/***en
    @brief Encode a part of an M-text.

    The mconv_encode_range () function encodes the text between $FROM
    (inclusive) and $TO (exclusive) in M-text $MT and writes the
    resulting byte sequence into the buffer area or the stream that is
    currently bound to code converter $CONVERTER.

    @return
    If the operation was successful, mconv_encode_range () returns the
    number of written bytes. Otherwise it returns -1 and assigns an
    error code to the external variable #merror_code.  */

/***ja
    @brief M-text の一部をバイト列にエンコードする.

    関数 mconv_encode_range () は、M-text $MT の $FROM 
    （$FROM 自体も含む）から $TO （$TO自体は含まない）
    までの範囲のテキストをエンコードして、コードコンバータ
    $CONVERTER に現在結び付けられているバッファ領域あるいはストリームに得られたバイト列を書き込む。

    @return
    もし処理が成功すれば、mconv_encode_range () 
    は書き込まれたバイト数を返す。そうでなければ -1 
    を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_RANGE, @c MERROR_IO, @c MERROR_CODING

    @seealso
    mconv_rebind_buffer (), mconv_rebind_stream(),
    mconv_decode (), mconv_encode ()  */

int
mconv_encode_range (MConverter *converter, MText *mt, int from, int to)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;

  M_CHECK_POS_X (mt, from, -1);
  M_CHECK_POS_X (mt, to, -1);
  if (to < from)
    to = from;

  if (converter->at_most > 0 && from + converter->at_most < to)
    to = from + converter->at_most;

  converter->nchars = converter->nbytes = 0;
  converter->result = MCONVERSION_RESULT_SUCCESS;

  mtext_put_prop (mt, from, to, Mcoding, internal->coding->name);
  if (internal->binding == BINDING_BUFFER)
    {
      (*internal->coding->encoder) (mt, from, to,
				    internal->buf.out + internal->used,
				    internal->bufsize - internal->used,
				    converter);
      internal->used += converter->nbytes;
    }
  else if (internal->binding == BINDING_STREAM)
    {
      unsigned char work[CONVERT_WORKSIZE];

      while (from < to)
	{
	  int written = 0;
	  int prev_nbytes = converter->nbytes;
	  int this_nbytes;

	  (*internal->coding->encoder) (mt, from, to, work,
					CONVERT_WORKSIZE, converter);
	  this_nbytes = converter->nbytes - prev_nbytes;
	  while (written < this_nbytes)
	    {
	      int wrtn = fwrite (work + written, sizeof (unsigned char),
				 this_nbytes - written, internal->fp);

	      if (ferror (internal->fp))
		break;
	      written += wrtn;
	    }
	  if (written < this_nbytes)
	    {
	      converter->result = MCONVERSION_RESULT_IO_ERROR;
	      break;
	    }
	  from += converter->nchars;
	}
    }
  else 				/* fail safe */
    MERROR (MERROR_CODING, -1);

  return ((converter->result == MCONVERSION_RESULT_SUCCESS
	   || converter->result == MCONVERSION_RESULT_INSUFFICIENT_DST)
	  ? converter->nbytes : -1);
}

/*=*/

/***en
    @brief Encode an M-text into a buffer area.

    The mconv_encode_buffer () function encodes M-text $MT based on
    coding system $NAME and writes the resulting byte sequence into the
    buffer area pointed to by $BUF.  At most $N bytes are written.  A
    temporary code converter for encoding is automatically created
    and freed.

    @return
    If the operation was successful, mconv_encode_buffer () returns
    the number of written bytes.  Otherwise it returns -1 and assigns
    an error code to the external variable #merror_code.  */

/***ja
    @brief M-text をエンコードしてバッファ領域に書き込む.

    関数 mconv_encode_buffer () はM-text $MT をコード系 $NAME 
    に基づいてエンコードし、得られたバイト列を $BUF の指すバッファ領域に書き込む。
    $N は書き込む最大バイト数である。
    エンコードに必要なコードコンバータの作成と解放は自動的に行なわれる。

    @return
    もし処理が成功すれば、mconv_encode_buffer () は書き込まれたバイト数を返す。
    そうでなければ-1を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_IO, @c MERROR_CODING

    @seealso
    mconv_encode (), mconv_encode_stream ()  */

int
mconv_encode_buffer (MSymbol name, MText *mt, unsigned char *buf, int n)
{
  MConverter *converter = mconv_buffer_converter (name, buf, n);
  int ret;

  if (! converter)
    return -1;
  ret = mconv_encode (converter, mt);
  mconv_free_converter (converter);
  return ret;
}

/*=*/

/***en
    @brief Encode an M-text to write to a stream.

    The mconv_encode_stream () function encodes M-text $MT based on
    coding system $NAME and writes the resulting byte sequence to
    stream $FP.  A temporary code converter for encoding is
    automatically created and freed.

    @return
    If the operation was successful, mconv_encode_stream () returns
    the number of written bytes.  Otherwise it returns -1 and assigns
    an error code to the external variable #merror_code.  */

/***ja
    @brief M-text をエンコードしてストリームに書き込む.

    関数 mconv_encode_stream () はM-text $MT をコード系 $NAME 
    に基づいてエンコードし、得られたバイト列をストリーム $FP 
    に書き出す。エンコードに必要なコードコンバータの作成と解放は自動的に行なわれる。

    @return
    もし処理が成功すれば、mconv_encode_stream () 
    は書き込まれたバイト数を返す。そうでなければ -1 
    を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_IO, @c MERROR_CODING

    @seealso
    mconv_encode (), mconv_encode_buffer (), mconv_encode_file ()  */

int
mconv_encode_stream (MSymbol name, MText *mt, FILE *fp)
{
  MConverter *converter = mconv_stream_converter (name, fp);
  int ret;

  if (! converter)
    return -1;
  ret = mconv_encode (converter, mt);
  mconv_free_converter (converter);
  return ret;
}

/*=*/

/***en
    @brief Read a character via a code converter.

    The mconv_getc () function reads one character from the buffer
    area or the stream that is currently bound to code converter
    $CONVERTER.  The decoder of $CONVERTER is used to decode the byte
    sequence.  The internal status of $CONVERTER is updated
    appropriately.

    @return
    If the operation was successful, mconv_getc () returns the
    character read in.  If the input source reaches EOF, it returns @c
    EOF without changing the external variable #merror_code.  If an
    error is detected, it returns @c EOF and assigns an error code to
    #merror_code.  */

/***ja
    @brief コードコンバータ経由で一文字を読みこむ.

    関数 mconv_getc () は、コードコンバータ $CONVERTER 
    に現在結び付けられているバッファ領域あるいはストリームから文字を一つ読み込む。
    バイト列のデコードには $CONVERTER のデコーダが用いられる。
    $CONVERTER の内部状態は必要に応じて更新される。

    @return
    処理が成功すれば、mconv_getc () は読み込まれた文字を返す。入力源が 
    EOF に達した場合は、外部変数 #merror_code を変えずに @c EOF 
    を返す。エラーが検出された場合は @c EOF を返し、#merror_code 
    にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_CODING

    @seealso
    mconv_ungetc (), mconv_putc (), mconv_gets ()  */

int
mconv_getc (MConverter *converter)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;
  int at_most = converter->at_most;

  mtext_reset (internal->work_mt);
  converter->at_most = 1;
  mconv_decode (converter, internal->work_mt);
  converter->at_most = at_most;
  return (converter->nchars == 1
	  ? STRING_CHAR (internal->work_mt->data)
	  : EOF);
}

/*=*/

/***en
    @brief Push a character back to a code converter.

    The mconv_ungetc () function pushes character $C back to code
    converter $CONVERTER.  Any number of characters can be pushed
    back.  The lastly pushed back character is firstly read by the
    subsequent mconv_getc () call.  The characters pushed back are
    registered only in $CONVERTER; they are not written to the input
    source.  The internal status of $CONVERTER is updated
    appropriately.

    @return
    If the operation was successful, mconv_ungetc () returns $C.
    Otherwise it returns @c EOF and assigns an error code to the
    external variable #merror_code.  */

/***ja
    @brief コードコンバータに一文字戻す.

    関数 mconv_ungetc () は、コードコンバータ $CONVERTER に文字 $C 
    を押し戻す。戻される文字数に制限はない。この後で mconv_getc () 
    を呼び出した際には、最後に戻された文字が最初に読まれる。戻された文字は 
    $CONVERTER の内部に蓄えられるだけであり、実際に入力源に書き込まれるわけではない。
    $CONVERTER の内部状態は必要に応じて更新される。

    @return
    処理が成功すれば、mconv_ungetc () は $C を返す。そうでなければ @c
    EOF を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_CODING, @c MERROR_CHAR

    @seealso
    mconv_getc (), mconv_putc (), mconv_gets ()  */

int
mconv_ungetc (MConverter *converter, int c)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;

  M_CHECK_CHAR (c, EOF);

  converter->result = MCONVERSION_RESULT_SUCCESS;
  mtext_cat_char (internal->unread, c);
  return c;
}

/*=*/

/***en
    @brief Write a character via a code converter.

    The mconv_putc () function writes character $C to the buffer area
    or the stream that is currently bound to code converter
    $CONVERTER.  The encoder of $CONVERTER is used to encode the
    character.  The number of bytes actually written is set to the @c
    nbytes member of $CONVERTER.  The internal status of $CONVERTER
    is updated appropriately.

    @return
    If the operation was successful, mconv_putc () returns $C.
    If an error is detected, it returns @c EOF and assigns
    an error code to the external variable #merror_code.  */

/***ja
    @brief コードコンバータを経由して一文字書き出す.

    関数 mconv_putc () は、コードコンバータ $CONVERTER 
    に現在結び付けられているバッファ領域あるいはストリームに文字 $C 
    を書き出す。文字のエンコードには $CONVERTER 
    のエンコーダが用いられる。実際に書き出されたバイト数は、$CONVERTER のメンバー
    @c nbytes にセットされる。$CONVERTER の内部状態は必要に応じて更新される。

    @return
    処理が成功すれば、mconv_putc () は $C を返す。エラーが検出された場合は
    @c EOF を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_CODING, @c MERROR_IO, @c MERROR_CHAR

    @seealso
    mconv_getc (), mconv_ungetc (), mconv_gets ()  */

int
mconv_putc (MConverter *converter, int c)
{
  MConverterStatus *internal = (MConverterStatus *) converter->internal_info;

  M_CHECK_CHAR (c, EOF);
  mtext_reset (internal->work_mt);
  mtext_cat_char (internal->work_mt, c);
  if (mconv_encode_range (converter, internal->work_mt, 0, 1) < 0)
    return EOF;
  return c;
}

/*=*/

/***en
    @brief Read a line using a code converter.

    The mconv_gets () function reads one line from the buffer area or
    the stream that is currently bound to code converter $CONVERTER.
    The decoder of $CONVERTER is used for decoding.  The decoded
    character sequence is appended at the end of M-text $MT.  The
    final newline character in the original byte sequence is not
    appended.  The internal status of $CONVERTER is updated
    appropriately.

    @return
    If the operation was successful, mconv_gets () returns the
    modified $MT.  If it encounters EOF without reading a single
    character, it returns $MT without changing it.  If an error is
    detected, it returns @c NULL and assigns an error code to 
    #merror_code.  */

/***ja
    @brief コードコンバータを使って一行読み込む.

    関数 mconv_gets () は、コードコンバータ $CONVERTER 
    に現在結び付けられているバッファ領域あるいはストリームから 1 行を読み込む。
    バイト列のデコードには $CONVERTER 
    のデコーダが用いられる。デコードされた文字列は M-text $MT 
    の末尾に追加される。元のバイト列の終端改行文字は追加されない。
    $CONVERTER の内部状態は必要に応じて更新される。

    @return
    処理が成功すれば、mconv_gets () は変更された $MT
    を返す。もし1文字も読まずに EOF に遭遇した場合は、$MT 
    を変更せずにそのまま返す。エラーが検出された場合は @c NULL を返し、
    #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_CODING

    @seealso
    mconv_getc (), mconv_ungetc (), mconv_putc ()  */

MText *
mconv_gets (MConverter *converter, MText *mt)
{
  int c;

  M_CHECK_READONLY (mt, NULL);
  if (mt->format != MTEXT_FORMAT_UTF_8)
    mtext__adjust_format (mt, MTEXT_FORMAT_UTF_8);

  while (1)
    {
      c = mconv_getc (converter);
      if (c == EOF || c == '\n')
	break;
      mtext_cat_char (mt, c);
    }
  if (c == EOF && converter->result != MCONVERSION_RESULT_SUCCESS)
    /* mconv_getc () sets #merror_code */
    return NULL;
  return mt;
}

/*=*/

/*** @} */

/*
  Local Variables:
  coding: utf-8
  End:
*/
