/* mtext.h -- header file for the M-text module.
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

#ifndef _M17N_MTEXT_H_
#define _M17N_MTEXT_H_

/** @file mtext.h
    @brief Header for M-text handling.
*/

#define POS_CHAR_TO_BYTE(mt, pos)				\
  (mtext_nchars (mt) == mtext_nbytes (mt) ? (pos)		\
   : (pos) == (mt)->cache_char_pos ? (mt)->cache_byte_pos	\
   : mtext__char_to_byte ((mt), (pos)))

#define POS_BYTE_TO_CHAR(mt, pos_byte)				\
  (mtext_nchars (mt) == mtext_nbytes (mt) ? (pos_byte)		\
   : (pos_byte) == (mt)->cache_byte_pos ? (mt)->cache_char_pos	\
   : mtext__byte_to_char ((mt), (pos_byte)))


#define MTEXT_DATA(mt) ((mt)->data)

extern int mtext__char_to_byte (MText *mt, int pos);

extern int mtext__byte_to_char (MText *mt, int pos_byte);

extern void mtext__enlarge (MText *mt, int nbytes);

extern int mtext__takein (MText *mt, int nchars, int nbytes);

extern int mtext__cat_data (MText *mt, unsigned char *p, int nbytes,
			    enum MTextFormat format);

#define MTEXT_CAT_ASCII(mt, str)				\
  mtext__cat_data ((mt), (unsigned char *) (str), strlen (str),	\
		   MTEXT_FORMAT_US_ASCII)

extern MText *mtext__from_data (void *data, int nitems,
				enum MTextFormat format, int need_copy);

extern void mtext__adjust_format (MText *mt, enum MTextFormat format);

extern int mtext__bol (MText *mt, int pos);

extern int mtext__eol (MText *mt, int pos);

#endif /* _M17N_MTEXT_H_ */
