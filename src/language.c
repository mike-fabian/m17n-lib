/* language.c -- language (and script) module.
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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
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
static MPlist *langname_list;

static MPlist *
load_lang_script_list (MSymbol tag0, MSymbol tag1, MSymbol tag2, MSymbol tag3)
{
  MDatabase *mdb = mdatabase_find (tag0, tag1, tag2, tag3);
  MPlist *plist, *pl, *p;

  if (! mdb
      || ! (plist = mdatabase_load (mdb)))
    return NULL;
  /* Check at least if the plist is ((SYMBOL ...) ...).  */
  for (pl = plist; ! MPLIST_TAIL_P (pl);)
    {
      if (! MPLIST_PLIST_P (pl))
	mplist__pop_unref (pl);
      else
	{
	  p = MPLIST_PLIST (pl);
	  if (! MPLIST_SYMBOL_P (p))
	    mplist__pop_unref (pl);
	  else
	    pl = MPLIST_NEXT (pl);
	}
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

static MPlist *
load_lang_name (MSymbol target3, MSymbol target2)
{
  MPlist *plist, *pl, *p;

  plist = mplist ();
  mplist_add (plist, Msymbol, target3);
  pl = mdatabase_list (Mlanguage, Mname, target3, Mnil);
  if (! pl && target2 != Mnil)
    pl = mdatabase_list (Mlanguage, Mname, target2, Mnil);
  if (pl)
    {
      MPLIST_DO (p, pl)
	{
	  MDatabase *mdb = MPLIST_VAL (p);
	  MPlist *p0 = mdatabase_load (mdb), *p1, *territories;
	  MSymbol script = mdatabase_tag (mdb)[3];
	  
	  if (MPLIST_PLIST_P (p0))
	    {
	      p1 = MPLIST_PLIST (p0);
	      if (MPLIST_SYMBOL_P (p1) && MPLIST_SYMBOL (p1) == Mlanguage)
		{
		  p1 = MPLIST_NEXT (MPLIST_NEXT (MPLIST_NEXT (MPLIST_NEXT (p1))));
		  territories = p1;
		  while (! MPLIST_TAIL_P (p1))
		    {
		      if (MPLIST_SYMBOL_P (p1))
			p1 = MPLIST_NEXT (p1);
		      else
			mplist__pop_unref (p1);
		    }
		  M17N_OBJECT_REF (territories);
		  mplist__pop_unref (p0);
		}
	      else
		territories = mplist ();
	      mplist_push (p0, Mplist, territories);
	      M17N_OBJECT_UNREF (territories);
	      mplist_push (p0, Msymbol, script);
	      mplist_add (plist, Mplist, p0);
	      M17N_OBJECT_UNREF (p0);
	    }
	}
      M17N_OBJECT_UNREF (pl);
    }
  mplist_push (langname_list, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
  return plist;
}


/* Internal API */

int
mlang__init ()
{
  msymbol_put_func (Mlanguage, Mtext_prop_serializer,
		    M17N_FUNC (msymbol__serializer));
  msymbol_put_func (Mlanguage, Mtext_prop_deserializer,
		    M17N_FUNC (msymbol__deserializer));
  Miso639_2 = msymbol ("iso639-2");
  Miso639_1 = msymbol ("iso639-1");

  language_list = script_list = langname_list = NULL;
  return 0;
}

void
mlang__fini (void)
{
  M17N_OBJECT_UNREF (language_list);
  M17N_OBJECT_UNREF (script_list);
  M17N_OBJECT_UNREF (langname_list);
}

/*=*/

/***en
    @brief Get information about a language.

    The mlanguage_info () function returns a well-formed @e plist that
    contains information about $LANGUAGE.  $LANGUAGE is a symbol whose
    name is an ISO639-2 3-letter language code, an ISO639-1 2-letter
    language codes, or an English word.

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

      if (MPLIST_SYMBOL (pl) == language)
	return pl;
      if (MPLIST_TAIL_P (pl))
	continue;
      pl = MPLIST_NEXT (pl);
      if (MPLIST_SYMBOL_P (pl) && MPLIST_SYMBOL (pl) == language)
	return MPLIST_PLIST (plist);
      if (MPLIST_TAIL_P (pl))
	continue;
      pl = MPLIST_NEXT (pl);
      if (MPLIST_MTEXT_P (pl))
	{
	  MText *mt = MPLIST_MTEXT (pl);

	  if (mtext_nbytes (mt) == MSYMBOL_NAMELEN (language)
	      && strncasecmp ((char *) MTEXT_DATA (MPLIST_MTEXT (pl)),
			      MSYMBOL_NAME (language),
			      MSYMBOL_NAMELEN (language)) == 0)
	    return MPLIST_PLIST (plist);
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
      && ! MPLIST_TAIL_P (plist)
      && (plist = MPLIST_NEXT (plist)) /* char list */
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
	  && ! MPLIST_TAIL_P (p))
	{
	  if (MPLIST_SYMBOL_P (p))
	    {
	      if (otf_tag == MPLIST_SYMBOL (p))
		  return MPLIST_SYMBOL (pl);
	    }
	  else if (MPLIST_PLIST (p))
	    {
	      MPlist *p0;

	      MPLIST_DO (p0, MPLIST_PLIST (p))
		if (MPLIST_SYMBOL_P (p0) && otf_tag == MPLIST_SYMBOL (p0))
		  return MPLIST_SYMBOL (pl);
	    }
	}
    }
  return script;
}

#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

/*** @addtogroup m17nLocale */
/*** @{ */
/*=*/


MSymbol Miso639_1, Miso639_2;

/*=*/

/***en
    @brief List 3-letter language codes.

    The mlanguage_list () funciton returns a well-formed plist whose
    keys are #Msymbol and values are symbols whose names are ISO639-2
    3-letter language codes.

    @return
    This function returns a plist.  The caller should free it by
    m17n_object_unref ().

    @seealso
    mscript_list ().  */

/***ja
    @brief 3文字言語コードをリストする.

    関数 mlanguage_list () は、整形式 (well-formed) plist を返す。各キー
    は #Msymbol であり、個々の値は ISO639-2 に定められた3文字言語コー
    ドを名前とするシンボルである。

    @return
    この関数が返す plist は、呼び出し側が m17n_object_unref () を使っ
    て解放する必要がある。

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

    The mlanguage_code () function returns a symbol whose name is the
    ISO639 language code of $LANGUAGE. $LANGUAGE is a symbol whose
    name is an ISO639-2 3-letter language code, an ISO639-1 2-letter
    language codes, or an English word.

    $LEN specifies the type of the returned language code.  If it is
    3, an ISO639-2 3-letter language code is returned.  If it is 2, an
    ISO639-1 2-letter language code is returned when defined;
    otherwise #Mnil is returned.  If it is 0, a 2-letter code is
    returned when defined; otherwise a 3-letter code is returned.

    @return
    If the information is available, this function returns a non-#Mnil
    symbol.  Otherwise, it returns #Mnil.

    @seealso
    mlanguage_name_list (), mlanguage_text ().  */

/***ja
    @brief 言語コードを得る.

    関数 mlanguage_code () は、$LANGUAGE に対応した ISO-639 言語コード
    が名前であるようなシンボルを返す。$LANGUAGE はシンボルであり、その
    名前は、ISO639-2 3文字言語コード、ISO639-1 2文字言語コード、英語名、
    のいずれかである。

    $LEN は返される言語コードの種類を決定する。$LEN が3の場合は 
    ISO639-2 3文字言語コードが返される。2の場合は、もし定義されていれ
    ば ISO639-1 2文字言語コードが、そうでなければ #Mnil が返される。0 
    の場合は、もし定義されていれば2文字コードが、そうでなければ3文字コー
    ドが返される。

    @return
    もし情報が得られれば、この関数は #Mnil 以外のシンボルを返す。そう
    でなければ #Mnil を返す。

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
    @brief Return the language names written in the specified language.

    The mlanguage_name_list () function returns a plist of LANGUAGE's
    names written in TARGET language.  SCRIPT and TERRITORY, if not #Mnil,
    specifies which script and territory to concern at first.

    LANGUAGE and TARGET must be a symbol whose name is an ISO639-2
    3-letter language code or an ISO639-1 2-letter language codes.
    TARGET may be #Mnil, in which case, the language of the current
    locale is used.  If locale is not set or is C, English is used.

    SCRIPT and TERRITORY must be a symbol whose name is a script and
    territory name of a locale (e.g. "TW", "SG") respectively.

    @return
    If the translation is available, this function returns a non-empty
    plist.  The first element has key #MText and the value is an
    M-text of a translated language name.  If the succeeding elements
    also have key #MText, their values are M-texts of alternate
    translations.

    If no translation is available, @c NULL is returned.

    The returned plist should not be modified nor freed.

    @seealso
    mlanguage_code (), mlanguage_text ().  */

MPlist *
mlanguage_name_list (MSymbol language, MSymbol target,
		     MSymbol script, MSymbol territory)
{
  MPlist *plist, *pl;
  MSymbol language2, target2;

  plist = mlanguage__info (language);
  if (! plist)
    return NULL;
  language = MPLIST_SYMBOL (plist);
  language2 = MPLIST_SYMBOL (MPLIST_NEXT (plist));
  if (target != Mnil)
    {
      plist = mlanguage__info (target);
      if (! plist)
	return NULL;
      target = MPLIST_SYMBOL (plist);
      target2 = MPLIST_SYMBOL (MPLIST_NEXT (plist));
    }
  else
    {
      MLocale *locale = mlocale_set (LC_MESSAGES, NULL);

      if (! locale)
	{
	  target = msymbol ("eng");
	  target2 = msymbol ("en");
	  script = territory = Mnil;
	}
      else
	{
	  target = mlocale_get_prop (locale, Mlanguage);
	  plist = mlanguage__info (target);
	  if (! plist)
	    return NULL;
	  target = MPLIST_SYMBOL (plist);
	  target2 = MPLIST_SYMBOL (MPLIST_NEXT (plist));
	  script = mlocale_get_prop (locale, Mscript);
	  territory = mlocale_get_prop (locale, Mterritory);
	}
    }

  /* Now both LANGUAGE and TARGET are 3-letter codes.  */
  if (! langname_list)
    langname_list = mplist ();
  plist = mplist__assq (langname_list, target);
  if (plist)
    plist = MPLIST_PLIST (plist);
  else
    plist = load_lang_name (target, target2);

  /* PLIST = (TARGET (SCRIPT (TERRITORY ...) (LANG-CODE NAME ...) ...) ...) */
  plist = MPLIST_NEXT (plist);
  if (MPLIST_TAIL_P (plist))
    return NULL;

  MPLIST_DO (pl, plist)
    {
      MPlist *p = MPLIST_PLIST (pl), *p0;

      if (MPLIST_SYMBOL (p) == script
	  && (territory == Mnil
	      || mplist_find_by_value (MPLIST_PLIST (MPLIST_NEXT (p)),
				       territory))
	  && (p = MPLIST_NEXT (MPLIST_NEXT (p)))
	  && ((p0 = mplist__assq (p, language))
	      || (p0 = mplist__assq (p, language2))))
	{
	  plist = p0;
	  break;
	}
    }
  if (MPLIST_TAIL_P (pl))
    {
      MPLIST_DO (pl, plist)
	{
	  MPlist *p = MPLIST_NEXT (MPLIST_PLIST (pl)), *p0;

	  if ((territory == Mnil
	       || mplist_find_by_value (MPLIST_VAL (p), territory))
	      && (p = MPLIST_NEXT (MPLIST_NEXT (p)))
	      && ((p0 = mplist__assq (p, language))
		  || (p0 = mplist__assq (p, language2))))
	    {
	      plist = p0;
	      break;
	    }
	}
      if (MPLIST_TAIL_P (pl))
	{
	  MPLIST_DO (pl, plist)
	    {
	      MPlist *p = MPLIST_NEXT (MPLIST_NEXT (MPLIST_PLIST (pl))), *p0;

	      if ((p0 = mplist__assq (p, language))
		  || (p0 = mplist__assq (p, language2)))
		{
		  plist = p0;
		  break;
		}
	    }
	  if (MPLIST_TAIL_P (pl))
	    return NULL;
	}
    }

  plist = MPLIST_NEXT (MPLIST_PLIST (plist));
  if (territory != Mnil)
    {
      MPLIST_DO (pl, MPLIST_NEXT (plist))
	if (MPLIST_SYMBOL_P (pl) && MPLIST_SYMBOL (pl) == territory)
	  break;
      if (! MPLIST_TAIL_P (pl)
	  && (pl = MPLIST_NEXT (pl))
	  && MPLIST_MTEXT_P (pl))
	return pl;
    }
  MPLIST_DO (plist, plist)
    if (MPLIST_MTEXT_P (plist))
      break;
  return (MPLIST_MTEXT_P (plist) ? plist : NULL);
}

/*=*/

/***en
    @brief Return the language name written in that language.

    The mlanguage_text () function returns, in the form of M-text, the
    language name of $LANGUAGE written in $LANGUAGE.  If the
    representative characters of the language are known, the
    characters of the returned M-text has a text property whose key is
    #Mtext and whose value is an M-text that contains the
    representative characters.

    @return
    If the information is available, this function returns an M-text
    that should not be modified nor freed.  Otherwise, it returns @c
    NULL.

    @seealso
    mlanguage_code (), mlanguage_name ().  */

/***ja
    @brief 与えられた言語自身で書かれた言語名を返す.

    関数 mlanguage_text () は、言語 $LANGUAGE で書かれた $LANGUAGE の
    名前を M-text の形式で返す。その言語の代表的な文字がわかっている場
    合は、返される M-text の各文字に、キーが #Mtext で値がその代表的な
    文字を含む M-text であるようなテキストプロパティが付加される。

    @return
    求める情報が得られた場合、この関数が返す M-text を変更したり解放し
    たりしてはいけない。情報が得られなかった場合は @c NULL が返される。

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
    This function returns a plist.  The caller should free it by
    m17n_object_unref ().

    @seealso
    mscript_language_list (), mlanguage_list ().  */

/***ja
    @brief スクリプト名をリストする.

    関数 mscript_list () は、整形式 (well-formed) plist を返す。各キー
    は #Msymbol であり、個々の値はスクリプト名を名前とするシンボルであ
    る。

    @return
    この関数が返す plist は、呼び出し側が m17n_object_unref () を使っ
    て解放する必要がある。

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

    The mscript_language_list () function lists languages that use
    $SCRIPT.  $SCRIPT is a symbol whose name is the lower-cased
    version of a script name that appears in the Unicode Character
    Database.

    @return

    This function returns a well-formed plist whose keys are #Msymbol
    and values are symbols whose names are ISO639-1 2-letter codes (or
    ISO639-2 3-letter codes, if the former is not available).  The
    caller should not modify nor free it.  If the m17n library does
    not know about $SCRIPT, it returns @ c NULL.

    @seealso
    mscript_list (), mlanguage_list ().  */

/***ja
    @brief 与えられたスクリプトを用いる言語をリストする.

    関数 mscript_language_list () は、$SCRIPT を用いる言語をリストする。
    $SCRIPT はシンボルで、その名前は Unicode Character Database に示さ
    れているスクリプト名をすべて小文字にしたものである。

    @return 
    この関数は、整形式 (well-formed) plist を返す。各キーは 
    #Msymbol であり、個々の値は ISO639-1 に定められた2文字言語コード
    (定義されていない場合は ISO639-2 に定められた3文字言語コード) を名
    前とするシンボルである。返される plist は変更したり解放したりして
    はならない。$SCRIPT が未知の場合は @c NULL が返される。

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

/*** @} */
/*=*/
/***en
    @name Obsolete functions
*/
/***ja
    @name Obsolete な関数
*/
/*** @{ */

/***en
    @brief Get an English language name.

    This function is obsolete.  Use mlanguage_name_list () instead.

    The mlanguage_name () function returns a symbol whose name is an
    English name of $LANGUAGE.  $LANGUAGE is a symbol whose name is an
    ISO639-2 3-letter language code, an ISO639-1 2-letter language
    codes, or an English word.

    @return
    If the information is available, this function returns a non-#Mnil
    symbol.  Otherwise, it returns #Mnil.

    @seealso
    mlanguage_code (), mlanguage_text ().  */

/***ja
    @brief 言語の英語名を得る.

    関数 mlanguage_name () は、$LANGUAGE の英語名を名前とするようなシ
    ンボルを返す。$LANGUAGE はシンボルであり、その名前は、ISO639-2 3文
    字言語コード、ISO639-1 2文字言語コード、英語名、のいずれかである。

    @return
    求めている情報が得られるなら、この関数は #Mnil 以外のシンボルを返
    す。そうでなければ #Mnil を返す。

    @seealso
    mlanguage_code (), mlanguage_text ().  */

MSymbol
mlanguage_name (MSymbol language)
{
  MPlist *plist = mlanguage__info (language);
  MText *mt;
  char *str;

  if (! plist)			/* 3-letter code */
    return Mnil;
  plist = MPLIST_NEXT (plist);	/* 2-letter code */
  if (MPLIST_TAIL_P (plist))
    return Mnil;
  plist = MPLIST_NEXT (plist);	/* english name */
  if (! MPLIST_MTEXT_P (plist))
    return Mnil;
  mt = MPLIST_MTEXT (plist);
  str = alloca (mtext_nbytes (mt));
  memcpy (str, MTEXT_DATA (mt), mtext_nbytes (mt));
  str[0] = tolower (str[0]);
  return msymbol__with_len (str, mtext_nbytes (mt));
}

/*=*/
/*** @} */

/*
  Local Variables:
  coding: utf-8
  End:
*/
