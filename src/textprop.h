/* textproc.h -- header file for the text property module.
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

#ifndef _M17N_TEXTPROP_H_
#define _M17N_TEXTPROP_H_

/** MTextProperty is the structure for a text property object.  While
    attached, it is stored in the stacks of intervals covering the
    range from MTextProperty->start to MTextProperty->end.  */

struct MTextProperty
{
  /** Common header for a managed object.  */
  M17NObject control;

  /** Number of intervals the property is attached.  When it becomes
      zero, the property is detached.  */
  unsigned attach_count;

  /** M-text to which the property is attaced.  The value NULL means
      that the property is detached.  */
  MText *mt;

  /** Region of <mt> if the property is attached to it.  */
  int start, end;

  /** Key of the property.  */
  MSymbol key;

  /** Value of the property.  */
  void *val;
};

#define MTEXTPROP_START(prop) (prop)->start
#define MTEXTPROP_END(prop) (prop)->end
#define MTEXTPROP_KEY(prop) (prop)->key
#define MTEXTPROP_VAL(prop) (prop)->val

extern struct MTextPlist *mtext__copy_plist (struct MTextPlist *, 
					     int from, int to,
					     MText *mt, int pos);

extern void mtext__free_plist (MText *mt);

extern void mtext__adjust_plist_for_delete (MText *, int, int);

extern void mtext__adjust_plist_for_insert (MText *, int, int,
					    struct MTextPlist *);

extern void mtext__adjust_plist_for_change (MText *mt, int pos,
					    int len1, int len2);

extern void dump_textplist (struct MTextPlist *plist, int indent);

#endif /* _M17N_TEXTPROP_H_ */
