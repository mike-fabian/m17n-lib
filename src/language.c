/* language.c -- language module.
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

#include <config.h>
#include <stdlib.h>
#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "language.h"
#include "symbol.h"
#include "plist.h"

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)

static MSymbol M_script_lang_list;


/* Internal API */

int
mlang__init ()
{
  MDatabase *mdb;
  MPlist *plist, *pl;

  Mlanguage = msymbol ("language");
  msymbol_put (Mlanguage, Mtext_prop_serializer,
	       (void *) msymbol__serializer);
  msymbol_put (Mlanguage, Mtext_prop_deserializer,
	       (void *) msymbol__deserializer);
  Miso639_2 = msymbol ("iso639-2");
  Miso639_1 = msymbol ("iso639-1");
  M_script_lang_list = msymbol_as_managing_key ("  script-lang-list");

  mdb = mdatabase_find (msymbol ("standard"), Mlanguage,
			msymbol ("iso639"), Mnil);
  if (! mdb)
    return 0;
  if (! (plist = mdatabase_load (mdb)))
    MERROR (MERROR_DB, -1);

  MPLIST_DO (pl, plist)
    {
      MPlist *p;
      MSymbol code3, code2, lang;
      MText *native, *extra;

      if (! MPLIST_PLIST_P (pl))
	continue;
      p = MPLIST_PLIST (pl);
      if (! MPLIST_SYMBOL_P (p))
	continue;
      code3 = MPLIST_SYMBOL (p);
      p = MPLIST_NEXT (p);
      if (! MPLIST_SYMBOL_P (p))
	continue;
      code2 = MPLIST_SYMBOL (p);
      p = MPLIST_NEXT (p);
      if (! MPLIST_SYMBOL_P (p))
	continue;
      lang = MPLIST_SYMBOL (p);
      msymbol_put (code3, Mlanguage, lang);
      p = MPLIST_NEXT (p);      
      native = MPLIST_MTEXT_P (p) ? MPLIST_MTEXT (p) : NULL;
      if (native)
	{
	  msymbol_put (code3, Mtext, native);
	  p = MPLIST_NEXT (p);
	  extra = MPLIST_MTEXT_P (p) ? MPLIST_MTEXT (p) : NULL;
	  if (extra)
	    mtext_put_prop (native, 0, mtext_nchars (native), Mtext, extra);
	}
      if (code2 != Mnil)
	{
	  msymbol_put (code3, Miso639_1, code2);
	  msymbol_put (code2, Mlanguage, lang);
	  msymbol_put (code2, Miso639_2, code3);
	  if (native)
	    msymbol_put (code2, Mtext, native);
	}
    }
  M17N_OBJECT_UNREF (plist);
  return 0;
}

void
mlang__fini (void)
{
}


/** Return a plist of languages that use SCRIPT.  If SCRIPT is Mnil,
    return a plist of all languages.  Each element of the plist has
    3-letter language code as a key and 2-letter language code as a
    value.  A caller must unref the returned value when finished.  */

MPlist *
mlanguage__list (MSymbol script)
{
  MDatabase *mdb;
  MPlist *language_list, *plist, *pl;

  if (script)
    {
      if ((language_list = msymbol_get (script, M_script_lang_list)))
	{
	  M17N_OBJECT_REF (language_list);
	  return language_list;
	}
      mdb = mdatabase_find (msymbol ("unicode"), Mscript, Mlanguage, Mnil);
      if (! mdb
	  || ! (plist = mdatabase_load (mdb)))
	MERROR (MERROR_DB, NULL);
      MPLIST_DO (pl, plist)
	{
	  MPlist *p, *lang_list;
	  MSymbol code3, code2;

	  if (! MPLIST_PLIST_P (pl))
	    continue;
	  p = MPLIST_PLIST (pl);
	  if (! MPLIST_SYMBOL_P (p))
	    continue;
	  lang_list = mplist ();
	  if (MPLIST_SYMBOL (p) == script)
	    language_list = lang_list;
	  msymbol_put (MPLIST_SYMBOL (p), M_script_lang_list, lang_list);
	  MPLIST_DO (p, MPLIST_NEXT (p))
	    if (MPLIST_SYMBOL_P (p))
	      {
		code2 = MPLIST_SYMBOL (p);
		if (MSYMBOL_NAMELEN (code2) == 2)
		  code3 = msymbol_get (code2, Miso639_2);
		else
		  code3 = code2, code2 = Mnil;
		if (code3 != Mnil)
		  mplist_push (lang_list, code3, code2);
	      }
	  M17N_OBJECT_UNREF (lang_list);
	}
      M17N_OBJECT_UNREF (plist);
      if (language_list)
	M17N_OBJECT_REF (language_list);
      else
	{
	  language_list = mplist ();
	  msymbol_put (script, M_script_lang_list, language_list);
	}
    }
  else
    {
      mdb = mdatabase_find (msymbol ("standard"), Mlanguage,
			    msymbol ("iso639"), Mnil);
      if (! mdb
	  || ! (plist = mdatabase_load (mdb)))
	MERROR (MERROR_DB, NULL);
      MPLIST_DO (pl, plist)
	{
	  MPlist *p;
	  MSymbol code3, code2;

	  if (! MPLIST_PLIST_P (pl))
	    continue;
	  p = MPLIST_PLIST (pl);
	  if (! MPLIST_SYMBOL_P (p))
	    continue;
	  code3 = MPLIST_SYMBOL (p);
	  p = MPLIST_NEXT (p);
	  if (! MPLIST_SYMBOL_P (p))
	    continue;
	  code2 = MPLIST_SYMBOL (p);
	  mplist_push (language_list, code3, code2);
	}
      M17N_OBJECT_UNREF (plist);
    }
  return language_list;
}

#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

MSymbol Miso639_1, Miso639_2;
