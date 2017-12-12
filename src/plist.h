/* plist.h -- header file for the plist module.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012
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

#ifndef _M17N_PLIST_H_
#define _M17N_PLIST_H_

struct MPlist
{
  /** Header for a managed object.  */
  M17NObject control;

  /**en Key of the first element of the plist.  If the value is Mnil,
     this is the tail of the plist.  In that case, <val> and <next> is
     NULL.  If the value is a managing key, <val> is a managed
     object.  */
  MSymbol key;

  /**en Value of the first element of the plist.  */
  union {
    void *pointer;
    M17NFunc func;
  } val;

  /**en Plist for the next element. */
  MPlist *next;
};

/** Macros to access each member of PLIST.  */

#define MPLIST_KEY(plist) ((plist)->key)
#define MPLIST_VAL(plist) ((plist)->val.pointer)
#define MPLIST_FUNC(plist) ((plist)->val.func)
#define MPLIST_NEXT(plist) ((plist)->next)
#define MPLIST_TAIL_P(plist) ((plist)->key == Mnil)

#define MPLIST_SYMBOL_P(plist) (MPLIST_KEY (plist) == Msymbol)
#define MPLIST_STRING_P(plist) (MPLIST_KEY (plist) == Mstring)
#define MPLIST_MTEXT_P(plist) (MPLIST_KEY (plist) == Mtext)
#define MPLIST_INTEGER_P(plist) (MPLIST_KEY (plist) == Minteger)
#define MPLIST_PLIST_P(plist) (MPLIST_KEY (plist) == Mplist)

#define MPLIST_NESTED_P(plist)	\
  ((plist)->control.flag & 1)
#define MPLIST_SET_NESTED_P(plist)	\
  ((plist)->control.flag |= 1)

#define MPLIST_VAL_FUNC_P(plist)	\
  ((plist)->control.flag & 2)
#define MPLIST_SET_VAL_FUNC_P(plist)	\
  ((plist)->control.flag |= 2)

#define MPLIST_SYMBOL(plist) ((MSymbol) MPLIST_VAL (plist))
#define MPLIST_STRING(plist) ((char *) MPLIST_VAL (plist))
#define MPLIST_MTEXT(plist) ((MText *) MPLIST_VAL (plist))
#define MPLIST_INTEGER(plist) ((int) MPLIST_VAL (plist))
#define MPLIST_PLIST(plist) ((MPlist *) MPLIST_VAL (plist))

#define MPLIST_FIND(plist, key)						\
  do {									\
    while (! MPLIST_TAIL_P (plist) && MPLIST_KEY (plist) != (key))	\
      (plist) = (plist)->next;						\
  } while (0)


#define MPLIST_DO(elt, plist)	\
  for ((elt) = (plist); ! MPLIST_TAIL_P (elt); (elt) = MPLIST_NEXT (elt))

#define MPLIST_LENGTH(plist)			\
  (MPLIST_TAIL_P (plist) ? 0			\
   : MPLIST_TAIL_P ((plist)->next) ? 1		\
   : MPLIST_TAIL_P ((plist)->next->next) ? 2	\
   : mplist_length (plist))

#define MPLIST_ADD_PLIST(PLIST, KEY, VAL) \
  MPLIST_SET_NESTED_P (mplist_add ((PLIST), (KEY), (VAL)))
#define MPLIST_PUSH_PLIST(PLIST, KEY, VAL) \
  MPLIST_SET_NESTED_P (mplist_push ((PLIST), (KEY), (VAL)))
#define MPLIST_PUT_PLIST(PLIST, KEY, VAL) \
  MPLIST_SET_NESTED_P (mplist_put ((PLIST), (KEY), (VAL)))


extern unsigned char hex_mnemonic[256];
extern unsigned char escape_mnemonic[256];

extern MPlist *mplist__from_file (FILE *fp, MPlist *keys);

extern MPlist *mplist__from_plist (MPlist *plist);

extern MPlist *mplist__from_alist (MPlist *plist);

extern MPlist *mplist__from_string (unsigned char *str, int n);

extern int mplist__serialize (MText *mt, MPlist *plist, int pretty);

extern MPlist *mplist__conc (MPlist *plist, MPlist *tail);

extern void mplist__pop_unref (MPlist *plist);

extern MPlist *mplist__assq (MPlist *plist, MSymbol key);

#endif  /* _M17N_PLIST_H_ */
