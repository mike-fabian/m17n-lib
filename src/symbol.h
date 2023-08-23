/* symbol.h -- header file for the symbol module.
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

#ifndef _M17N_SYMBOL_H_
#define _M17N_SYMBOL_H_

#include "plist.h"

struct MSymbolStruct
{
  /** 1 if a value of property (including text-property) whose key is
      the symbol is a managed object.  */
  unsigned managing_key : 1;

  /* Name of the symbol. */
  char *name;

  /* Byte length of <name>.  */
  int length;

  /* Plist of the symbol.  */
  MPlist plist;

  struct MSymbolStruct *next;
};

#define MSYMBOL_NAME(sym) ((sym)->name)
#define MSYMBOL_NAMELEN(sym) ((sym)->length - 1)

extern void msymbol__free_table ();

extern MSymbol msymbol__with_len (const char *name, int len);

extern MPlist *msymbol__list (MSymbol prop);

extern MSymbol msymbol__canonicalize (MSymbol sym);

extern MTextPropSerializeFunc msymbol__serializer;
extern MTextPropDeserializeFunc msymbol__deserializer;

#endif /* _M17N_SYMBOL_H_ */
