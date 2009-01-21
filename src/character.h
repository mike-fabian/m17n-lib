/* character.h -- header file for the character module.
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

#ifndef _M17N_CHARACTER_H_
#define _M17N_CHARACTER_H_

/*  UTF-8 format

	 0-7F		0xxxxxxx
	80-7FF		110xxxxx 10xxxxxx
       800-FFFF		1110xxxx 10xxxxxx 10xxxxxx
     10000-1FFFFF	11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    200000-3FFFFFF	111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 
   4000000-7FFFFFFF	1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx

   Unicode range:
     0-10FFFF	0 - 11110uuu 10uuxxxx 10xxxxxx 10xxxxxx (uuuuu <= 0x10)

*/

#define MAX_UTF8_CHAR_BYTES 6
#define MAX_UNICODE_CHAR_BYTES 4

#define USHORT_SIZE (sizeof (unsigned short))
#define UINT_SIZE (sizeof (unsigned int))

/* Return how many bytes one unit (char, short, or int) in FORMAT
   occupies.  */

#define UNIT_BYTES(format)						   \
  (((format) <= MTEXT_FORMAT_UTF_8 || (format) == MTEXT_FORMAT_BINARY) ? 1 \
   : (format) <= MTEXT_FORMAT_UTF_16BE ? USHORT_SIZE			   \
   : UINT_SIZE)

/* Return how many units (char, short, or int) C will occupy in
   MText->data.  If C is not in the supported range, return 0.  */

#define CHAR_UNITS_ASCII(c) ((c) < 0x80)

#define CHAR_UNITS_UTF8(c)	\
  ((c) < 0x80 ? 1		\
   : (c) < 0x800 ? 2		\
   : (c) < 0x10000 ? 3		\
   : (c) < 0x200000 ? 4		\
   : (c) < 0x4000000 ? 5	\
   : 6)

#define CHAR_UNITS_UTF16(c) ((c) < 0x110000 ? (2 - ((c) < 0x10000)) : 0)

#define CHAR_UNITS_UTF32(c) 1

#define CHAR_UNITS(c, format)					\
  ((format) <= MTEXT_FORMAT_UTF_8 ? CHAR_UNITS_UTF8 (c)		\
   : (format) <= MTEXT_FORMAT_UTF_16BE ? CHAR_UNITS_UTF16 (c)	\
   : (format) <= MTEXT_FORMAT_UTF_32BE ? CHAR_UNITS_UTF32 (c)	\
   : 1)

#define CHAR_BYTES CHAR_UNITS_UTF8

#define CHAR_UNITS_AT_UTF8(p)	\
  (!(*(p) & 0x80) ? 1		\
   : !(*(p) & 0x20) ? 2		\
   : !(*(p) & 0x10) ? 3		\
   : !(*(p) & 0x08) ? 4		\
   : !(*(p) & 0x04) ? 5		\
   : !(*(p) & 0x02) ? 6		\
   : 0)

#define CHAR_UNITS_AT_UTF16(p)			\
  (2 - (*(unsigned short *) (p) < 0xD800	\
	|| *(unsigned short *) (p) >= 0xDC00))

#define CHAR_UNITS_AT(mt, p)						\
  ((mt)->format <= MTEXT_FORMAT_UTF_8 ? CHAR_UNITS_AT_UTF8 (p)		\
   : (mt)->format <= MTEXT_FORMAT_UTF_16BE ? CHAR_UNITS_AT_UTF16 (p)	\
   : 1)

#define CHAR_BYTES_AT CHAR_UNITS_AT_UTF8

#define CHAR_UNITS_BY_HEAD_UTF8(c)	\
  (!((c) & 0x80) ? 1			\
   : !((c) & 0x20) ? 2			\
   : !((c) & 0x10) ? 3			\
   : !((c) & 0x08) ? 4			\
   : !((c) & 0x04) ? 5			\
   : !((c) & 0x02) ? 6			\
   : 0)

#define CHAR_UNITS_BY_HEAD_UTF16(c)	\
  (2 - ((unsigned short) (c) < 0xD800 || (unsigned short) (c) >= 0xDC00))

#define CHAR_UNITS_BY_HEAD(c, format)					\
  ((format) <= MTEXT_FORMAT_UTF_8 ? CHAR_UNITS_BY_HEAD_UTF8 (c)	\
   : (format) <= MTEXT_FORMAT_UTF_16BE ? CHAR_UNITS_BY_HEAD_UTF16 (c)	\
   : 1)

#define CHAR_BYTES_BY_HEAD CHAR_UNITS_BY_HEAD_UTF8

#define STRING_CHAR_UTF8(p)				\
  (!((p)[0] & 0x80) ? (p)[0]				\
   : !((p)[0] & 0x20) ? ((((p)[0] & 0x1F) << 6)		\
			 | ((p)[1] & 0x3F))		\
   : !((p)[0] & 0x10) ? ((((p)[0] & 0x0F) << 12)	\
			 | (((p)[1] & 0x3F) << 6)	\
			 | ((p)[2] & 0x3F))		\
   : !((p)[0] & 0x08) ? ((((p)[0] & 0x07) << 18)	\
			 | (((p)[1] & 0x3F) << 12)	\
			 | (((p)[2] & 0x3F) << 6)	\
			 | ((p)[3] & 0x3F))		\
   : !((p)[0] & 0x04) ? ((((p)[0] & 0x03) << 24)	\
			 | (((p)[1] & 0x3F) << 18)	\
			 | (((p)[2] & 0x3F) << 12)	\
			 | (((p)[3] & 0x3F) << 6)	\
			 | ((p)[4] & 0x3F))		\
   : ((((p)[0] & 0x01) << 30)				\
      | (((p)[1] & 0x3F) << 24)				\
      | (((p)[2] & 0x3F) << 18)				\
      | (((p)[3] & 0x3F) << 12)				\
      | (((p)[4] & 0x3F) << 6)				\
      | ((p)[5] & 0x3F)))

#define STRING_CHAR_UTF16(p)						   \
  (((unsigned short) (p)[0] < 0xD800 || (unsigned short) (p)[0] >= 0xDC00) \
   ? (p)[0]								   \
   : ((((p)[0] - 0xD800) << 10) + ((p)[1] - 0xDC00) + 0x10000))


#define STRING_CHAR STRING_CHAR_UTF8


#define STRING_CHAR_ADVANCE_UTF8(p)			\
  (!(*(p) & 0x80) ? *(p)++				\
   : !(*(p) & 0x20) ? (((*(p)++ & 0x1F) << 6)		\
		       | (*(p)++ & 0x3F))		\
   : !(*(p) & 0x10) ? (((*(p)++ & 0x0F) << 12)		\
		       | ((*(p)++ & 0x3F) << 6)		\
		       | (*(p)++ & 0x3F))		\
   : !(*(p) & 0x08) ? (((*(p)++ & 0x07) << 18)		\
		       | ((*(p)++ & 0x3F) << 12)	\
		       | ((*(p)++ & 0x3F) << 6)		\
		       | (*(p)++ & 0x3F))		\
   : !(*(p) & 0x04) ? (((*(p)++ & 0x03) << 24)		\
		       | ((*(p)++ & 0x3F) << 18)	\
		       | ((*(p)++ & 0x3F) << 12)	\
		       | ((*(p)++ & 0x3F) << 6)		\
		       | (*(p)++ & 0x3F))		\
   : (((*(p)++ & 0x01) << 30)				\
      | ((*(p)++ & 0x3F) << 24)				\
      | ((*(p)++ & 0x3F) << 18)				\
      | ((*(p)++ & 0x3F) << 12)				\
      | ((*(p)++ & 0x3F) << 6)				\
      | (*(p)++ & 0x3F)))

#define STRING_CHAR_ADVANCE_UTF16(p)					   \
  (((unsigned short) (p)[0] < 0xD800 || (unsigned short) (p)[0] >= 0xDC00) \
   ? *(p)++								   \
   : (((*(p)++ - 0xD800) << 10) + (*(p)++ - 0xDC00) + 0x10000))

#define STRING_CHAR_ADVANCE STRING_CHAR_ADVANCE_UTF8

#define STRING_CHAR_AND_UNITS_UTF8(p, bytes)		\
  (!((p)[0] & 0x80) ? ((bytes) = 1, (p)[0])		\
   : !((p)[0] & 0x20) ? ((bytes) = 2,			\
			 ((((p)[0] & 0x1F) << 6)	\
			  | ((p)[1] & 0x3F)))		\
   : !((p)[0] & 0x10) ? ((bytes) = 3,			\
			 ((((p)[0] & 0x0F) << 12)	\
			  | (((p)[1] & 0x3F) << 6)	\
			  | ((p)[2] & 0x3F)))		\
   : !((p)[0] & 0x08) ? ((bytes) = 4,			\
			 ((((p)[0] & 0x07) << 18)	\
			  | (((p)[1] & 0x3F) << 12)	\
			  | (((p)[2] & 0x3F) << 6)	\
			  | ((p)[3] & 0x3F)))		\
   : !((p)[0] & 0x04) ? ((bytes) = 5,			\
			 ((((p)[0] & 0x03) << 24)	\
			  | (((p)[1] & 0x3F) << 18)	\
			  | (((p)[2] & 0x3F) << 12)	\
			  | (((p)[3] & 0x3F) << 6)	\
			  | ((p)[4] & 0x3F)))		\
   : ((bytes) = 6,					\
      ((((p)[0] & 0x01) << 30)				\
       | (((p)[1] & 0x3F) << 24)			\
       | (((p)[2] & 0x3F) << 18)			\
       | (((p)[3] & 0x3F) << 12)			\
       | (((p)[4] & 0x3F) << 6)				\
       | ((p)[5] & 0x3F))))

#define STRING_CHAR_AND_UNITS_UTF16(p, units)				   \
  (((unsigned short) (p)[0] < 0xD800 || (unsigned short) (p)[0] >= 0xDC00) \
   ? ((units) = 1, (p)[0])						   \
   : ((units) = 2,							   \
      (((p)[0] - 0xD800) << 10) + ((p)[1] - 0xDC00) + 0x10000))

#define STRING_CHAR_AND_UNITS(p, units, format)	\
  ((format) <= MTEXT_FORMAT_UTF_8		\
   ? STRING_CHAR_AND_UNITS_UTF8 (p, units)	\
   : (format) <= MTEXT_FORMAT_UTF_16BE		\
   ? STRING_CHAR_AND_UNITS_UTF16 (p, units)	\
   : (format) <= MTEXT_FORMAT_UTF_32BE		\
   ? ((units) = 1, ((unsigned) (p))[0])		\
   : ((units) = 1, (p)[0]))

#define STRING_CHAR_AND_BYTES STRING_CHAR_AND_UNITS_UTF8

#define CHAR_STRING_UTF8(c, p)						\
  ((c) < 0x80								\
   ? ((p)[0] = (c), 1)							\
   : (c) < 0x800 ? ((p)[0] = (0xC0 | ((c) >> 6)),			\
		    (p)[1] = (0x80 | ((c) & 0x3F)),			\
		    2)							\
   : (c) < 0x10000 ? ((p)[0] = (0xE0 | ((c) >> 12)),			\
		      (p)[1] = (0x80 | (((c) >> 6) & 0x3F)),		\
		      (p)[2] = (0x80 | ((c) & 0x3F)),			\
		      3)						\
   : (c) < 0x200000 ? ((p)[0] = (0xF0 | ((c) >> 18)),			\
		       (p)[1] = (0x80 | (((c) >> 12) & 0x3F)),		\
		       (p)[2] = (0x80 | (((c) >> 6) & 0x3F)),		\
		       (p)[3] = (0x80 | ((c) & 0x3F)),			\
		       4)						\
   : (c) < 0x4000000 ? ((p)[0] = 0xF8,					\
			(p)[1] = (0x80 | ((c) >> 18)),			\
			(p)[2] = (0x80 | (((c) >> 12) & 0x3F)),		\
			(p)[3] = (0x80 | (((c) >> 6) & 0x3F)),		\
			(p)[4] = (0x80 | ((c) & 0x3F)),			\
			5)						\
   : ((p)[0] = (0xFC | ((c) >> 30)),					\
      (p)[1] = (0x80 | (((c) >> 24) & 0x3F)),				\
      (p)[2] = (0x80 | (((c) >> 18) & 0x3F)),				\
      (p)[3] = (0x80 | (((c) >> 12) & 0x3F)),				\
      (p)[4] = (0x80 | (((c) >> 6) & 0x3F)),				\
      (p)[5] = (0x80 | ((c) & 0x3F)),					\
      6))

#define CHAR_STRING_UTF16(c, p)			\
  ((c) < 0x10000 ? (p)[0] = (c), 1			\
   : (p[0] = (((c) - 0x10000) >> 10) + 0xD800,		\
      p[1] = (((c) - 0x10000) & 0x3FF) + 0xDC00,	\
      2))

#define CHAR_STRING CHAR_STRING_UTF8

#define CHAR_HEAD_P_UTF8(p)	\
  ((*(p) & 0xC0) != 0x80)

#define CHAR_HEAD_P_UTF16(p)		\
  (*(unsigned short *) (p) < 0xDC00	\
   || *(unsigned short *) (p) >= 0xE000)

#define CHAR_HEAD_P CHAR_HEAD_P_UTF8

/** Locale-safe version of tolower ().  It works only for an ASCII
    character.  */
#define TOLOWER(c)  (((c) >= 'A' && (c) <= 'Z') ? (c) + 32 : (c))

/** Locale-safe version of toupper ().  It works only for an ASCII
    character.  */
#define TOUPPER(c)  (((c) >= 'a' && (c) <= 'z') ? (c) - 32 : (c))

/** Locale-safe version of isupper ().  It works only for an ASCII
    character.  */
#define ISUPPER(c) ((c) >= 'A' && (c) <= 'Z')

/** Locale-safe version of isalnum ().  It works only for an ASCII
    character.  */
#define ISALNUM(c)			\
  (((c) >= 'A' && (c) <= 'Z')		\
   || ((c) >= 'a' && (c) <= 'z')	\
   || ((c) >= '0' && (c) <= '9'))

extern void mchar__define_prop (MSymbol key, MSymbol type, void *mdb);

#endif /* not _M17N_CHARACTER_H_ */
