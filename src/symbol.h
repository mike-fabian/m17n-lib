/* symbol.h -- header file for the symbol module.
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

#ifndef _M17N_SYMBOL_H_
#define _M17N_SYMBOL_H_

#include "plist.h"

struct MSymbol
{
  /** 1 iff a value of property (including text-property) whose key is
      the symbol is a managed object.  */
  unsigned managing_key : 1;

  /* Name of the symbol. */
  char *name;

  /* Byte length of <name>.  */
  int length;

  /* Plist of the symbol.  */
  MPlist plist;

  MSymbol next;
};

#define MSYMBOL_NAME(sym) ((sym)->name)
#define MSYMBOL_NAMELEN(sym) ((sym)->length - 1)

extern MSymbol msymbol__with_len (const char *name, int len);

extern MSymbol msymbol__canonicalize (MSymbol sym);

extern MTextPropSerializeFunc msymbol__serializer;
extern MTextPropDeserializeFunc msymbol__deserializer;

#endif /* _M17N_SYMBOL_H_ */
