/* language.c -- language (and script) module.
   Copyright (C) 2003, 2004, 2006
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
#include "mtext.h"

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)

static MPlist *language_list;
static MPlist *script_list;

static MPlist *
load_lang_script_list (MSymbol tag0, MSymbol tag1, MSymbol tag2, MSymbol tag3)
{
  MDatabase *mdb = mdatabase_find (tag0, tag1, tag2, tag3);
  MPlist *plist, *pl, *p;

  if (! mdb
      || ! (plist = mdatabase_load (mdb)))
    return NULL;
  /* Check at least if the plist is ((SYMBOL ...) ...).  */
  MPLIST_DO (pl, plist)
    {
      if (! MPLIST_PLIST_P (pl))
	break;
      p = MPLIST_PLIST (pl);
      if (! MPLIST_SYMBOL_P (p))
	break;
    }
  if (! MPLIST_TAIL_P (pl))
    {
      M17N_OBJECT_UNREF (plist);
      return NULL;
    }
  return plist;
}

static int
init_language_list (void)
{
  language_list = load_lang_script_list (msymbol ("standard"), Mlanguage,
					 msymbol ("iso639"), Mnil);
  if (! language_list)
    {
      language_list = mplist ();
      MERROR (MERROR_DB, -1);
    }
  return 0;
}


static int
init_script_list (void)
{
  script_list = load_lang_script_list (msymbol ("standard"), Mscript,
				       msymbol ("unicode"), Mnil);
  if (! script_list)
    {
      script_list = mplist ();
      MERROR (MERROR_DB, -1);
    }
  return 0;
}


/* Internal API */

int
mlang__init ()
{
  Mlanguage = msymbol ("language");
  msymbol_put (Mlanguage, Mtext_prop_serializer,
	       (void *) msymbol__serializer);
  msymbol_put (Mlanguage, Mtext_prop_deserializer,
	       (void *) msymbol__deserializer);
  Miso639_2 = msymbol ("iso639-2");
  Miso639_1 = msymbol ("iso639-1");

  language_list = script_list = NULL;
  return 0;
}

void
mlang__fini (void)
{
  MPlist *plist, *p;

  M17N_OBJECT_UNREF (language_list);
  language_list = NULL;
  M17N_OBJECT_UNREF (script_list);
  script_list = NULL;
}

/*=*/

/***en
    @brief Get information about a language.

    The mlanguage_info () function returns a well-formed @e plist that
    contains information about $LANGUAGE.  $LANGUAGE is a symbol whose
    name is an ISO639-2 3-letter language code, an ISO639-1 2-letter
    language codes, or an English name.

    The format of the plist is:

@verbatim
        (ISO639-2 [ISO639-1 | nil] ENGLISH-NAME ["NATIVE-NAME" | nil]
                  ["REPRESENTATIVE-CHARACTERS"])
@endverbatim

    where, ISO639-2 is a symbol whose name is 3-letter language code
    of ISO639-2, ISO639-1 is a symbol whose name is 2-letter language
    code of ISO639-1, ENGLISH-NAME is a symbol whose name is the
    English name of the language, "NATIVE-NAME" is an M-text written
    by the most natural way in the language,
    "REPRESENTATIVE-CHARACTERS" is an M-text that contains
    representative characters used by the language.

    It is assured that the formats of both M-texts are
    #MTEXT_FORMAT_UTF_8.

    @return
    If the information is available, this function returns a plist
    that should not be modified nor freed.  Otherwise, it returns
    @c NULL.

    @seealso
    mlanguage_list ()  */

MPlist *
mlanguage__info (MSymbol language)
{
  MPlist *plist;

  if (! language_list
      && init_language_list () < 0)
    return NULL;

  MPLIST_DO (plist, language_list)
    {
      MPlist *pl = MPLIST_PLIST (plist);

      if (MPLIST_SYMBOL_P (pl) && MPLIST_SYMBOL (pl) == language)
	return MPLIST_PLIST (plist);
      if (! MPLIST_TAIL_P (pl))
	{
	  pl = MPLIST_NEXT (pl);
	  if (MPLIST_SYMBOL_P (pl) && MPLIST_SYMBOL (pl) == language)
	    return MPLIST_PLIST (plist);
	  if (! MPLIST_TAIL_P (pl))
	    {
	      pl = MPLIST_NEXT (pl);
	      if (MPLIST_SYMBOL_P (pl) && MPLIST_SYMBOL (pl) == language)
		return MPLIST_PLIST (plist);
	    }
	}
    }
  return NULL;
}

static MPlist *
mscript__info (MSymbol script)
{
  MPlist *plist;

  if (! script_list
      && init_script_list () < 0)
    return NULL;
  MPLIST_DO (plist, script_list)
    {
      MPlist *pl = MPLIST_PLIST (plist);

      if (MPLIST_SYMBOL (pl) == script)
	return pl;
    }
  return NULL;
}

MPlist *
mscript__char_list (MSymbol name)
{
  MPlist *plist = mscript__info (name);

  if (plist			/* script name */
      && (plist = MPLIST_NEXT (plist)) /* language list */
      && MPLIST_PLIST_P (plist))
    return MPLIST_PLIST (plist);
  return NULL;
}

MSymbol
mscript__otf_tag (MSymbol script)
{
  MPlist *plist = mscript__info (script);

  if (plist			/* script name */
      && (plist = MPLIST_NEXT (plist)) /* language list */
      && ! MPLIST_TAIL_P (plist)
      && (plist = MPLIST_NEXT (plist)) /* char list */
      && ! MPLIST_TAIL_P (plist)
      && (plist = MPLIST_NEXT (plist)) /* otf tag */
      && MPLIST_SYMBOL_P (plist))
    return MPLIST_SYMBOL (plist);
  return NULL;
}

MSymbol
mscript__from_otf_tag (MSymbol otf_tag)
{
  MPlist *plist;
  /* As it is expected that this function is called in a sequence with
     the same argument, we use a cache.  */
  static MSymbol last_otf_tag, script;

  if (! script_list)
    {
      last_otf_tag = script = Mnil;
      if (init_script_list () < 0)
	return Mnil;
    }
  if (otf_tag == last_otf_tag)
    return script;
  last_otf_tag = otf_tag;
  script = Mnil;
  MPLIST_DO (plist, script_list)
    {
      MPlist *pl = MPLIST_PLIST (plist), *p;

      if (pl			       /* script name */
	  && (p = MPLIST_NEXT (pl))    /* language tag */
	  && ! MPLIST_TAIL_P (p)
	  && (p = MPLIST_NEXT (p)) /* char list */
	  && ! MPLIST_TAIL_P (p)
	  && (p = MPLIST_NEXT (p)) /* otf tag */
	  && MPLIST_SYMBOL_P (p)
	  && otf_tag == MPLIST_SYMBOL (p))
	{
	  script = MPLIST_SYMBOL (pl);
	  break;
	}
    }
  return script;
}

#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

MSymbol Miso639_1, Miso639_2;

/*=*/

/***en
    @brief List 3-letter language codes.

    The mlanguage_list () funciton returns a well-formed plist
    whose keys are #Msymbol and values are symbols whose names
    are ISO639-2 3-letter language codes.

    @return
    This function returns a plist.  The caller should free it by
    m17n_object_unref ().

    @seealso
    mscript_list ().  */

MPlist *
mlanguage_list (void)
{
  MPlist *plist, *pl, *p, *p0;

  if (! language_list
      && init_language_list () < 0)
    return NULL;
  plist = pl = mplist ();
  MPLIST_DO (p, language_list)
    {
      p0 = MPLIST_PLIST (p);
      pl = mplist_add (pl, Msymbol, MPLIST_VAL (p0));
    }
  return plist;
}

/*=*/

/***en
    @brief Get a language code.

    The mlanguage_code () function returns a symbol whose name is an
    ISO639 language code of $LANGUAGE. $LANGUAGE is a symbol whose
    name is an ISO639-2 3-letter language code, an ISO639-1 2-letter
    language codes, or an English name.

    $LEN specifies which type of language code to return.  If it is 3,
    ISO639-2 3-letter language code is returned.  If it is 2, ISO639-1
    2-letter language code (if available) or #Mnil is returned.  If it
    is 0, 2-letter code (if available) or 3-letter code is returned.

    @return
    If the information is available, this function returns a non-#Mnil
    symbol.  Otherwise, it returns #Mnil.

    @seealso
    mlanguage_name (), mlanguage_text ().  */

MSymbol
mlanguage_code (MSymbol language, int len)
{
  MPlist *plist = mlanguage__info (language);
  MSymbol code;

  if (! plist)
    return Mnil;
  if (! MPLIST_SYMBOL_P (plist))
    return Mnil;
  code = MPLIST_SYMBOL (plist);
  if (len == 3)
    return code;
  plist = MPLIST_NEXT (plist);
  return ((MPLIST_SYMBOL_P (plist) && MPLIST_SYMBOL (plist) != Mnil)
	  ? MPLIST_SYMBOL (plist)
	  : len == 0 ? code : Mnil);
}

/*=*/

/***en
    @brief Get an English language name.

    The mlanguage_name () function returns a symbol whose name is an
    English name of $LANGUAGE.

    @return
    If the information is available, this function returns a non-#Mnil
    symbol.  Otherwise, it returns #Mnil.

    @seealso
    mlanguage_code (), mlanguage_text ().  */

MSymbol
mlanguage_name (MSymbol language)
{
  MPlist *plist = mlanguage__info (language);

  if (! plist)			/* 3-letter code */
    return Mnil;
  plist = MPLIST_NEXT (plist);	/* 2-letter code */
  if (MPLIST_TAIL_P (plist))
    return Mnil;
  plist = MPLIST_NEXT (plist);	/* english name */
  if (! MPLIST_SYMBOL_P (plist))
    return Mnil;
  return MPLIST_SYMBOL (plist);
}

/*=*/

/***en
    @brief Return a language name written in the language.

    The mlanguage_text () function returns by M-text a language name
    of $LANGUAGE written $LANGUAGE.  If the representative characters
    of the language is known, the first character of the returned
    M-text has text-property #Mtext whose value is an M-text contains
    the representative characters.

    @return
    If the information available, this function returns an M-text that
    should not modified nor freed.  Otherwise, it returns @c NULL.

    @seealso
    mlanguage_code (), mlanguage_name ().  */

MText *
mlanguage_text (MSymbol language)
{
  MPlist *plist = mlanguage__info (language);
  MText *mt;

  if (! plist)
    return NULL;
  plist = MPLIST_NEXT (plist);
  if (MPLIST_TAIL_P (plist))
    return NULL;
  plist = MPLIST_NEXT (plist);
  if (MPLIST_TAIL_P (plist))
    return NULL;
  plist = MPLIST_NEXT (plist);		  
  if (! MPLIST_MTEXT_P (plist))
    return NULL;
  mt = MPLIST_MTEXT (plist);
  if (mtext_nchars (mt) == 0)
    return NULL;
  plist = MPLIST_NEXT (plist);
  if (MPLIST_MTEXT_P (plist)
      && ! mtext_get_prop (mt, 0, Mtext))
    mtext_put_prop (mt, 0, mtext_nchars (mt), Mtext, MPLIST_MTEXT (plist));
  return mt;
}

/***en
    @brief List script names.

    The mscript_list () funciton returns a well-formed plist whose
    keys are #Msymbol and values are symbols whose names are script
    names.

    @return
    This function returns plist.  The caller has to free it by
    m17n_object_unref ().

    @seealso
    mscript_language_list (), mlanguage_list ().  */

MPlist *
mscript_list (void)
{
  MPlist *plist, *pl, *p, *p0;

  if (! script_list
      && init_script_list () < 0)
    return NULL;
  plist = pl = mplist ();
  MPLIST_DO (p, script_list)
    {
      p0 = MPLIST_PLIST (p);
      pl = mplist_add (pl, Msymbol, MPLIST_VAL (p0));
    }
  return plist;
}

/*=*/

/***en
    @brief List languages that use a specified script.

    The mscript_language_list () function lists language names that
    use $SCRIPT.

    @return
    This function returns a well-formed @e plist whose keys are
    #Msymbol and values are symbols representing language names
    (ISO639-1 2-letter code (if defined) or ISO639-2 3-letter code).
    The caller should not modify nor free it.  If the m17n library
    doesn't know about $SCRIPT, it returns NULL.

    @seealso
    mscript_list (), mlanguage_list ().  */

MPlist *
mscript_language_list (MSymbol script)
{
  MPlist *plist = mscript__info (script);

  if (plist			/* script name */
      && (plist = MPLIST_NEXT (plist)) /* language list */
      && MPLIST_PLIST_P (plist))
    return MPLIST_PLIST (plist);
  return NULL;
}
