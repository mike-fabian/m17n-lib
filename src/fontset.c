/* fontset.c -- fontset module.
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

/***en
    @addtogroup m17nFontset
    @brief A fontset is an object that maps a character to fonts.

    A @e fontset is an object of the type @c MFontset.  When drawing an
    M-text, a fontset provides rules to select a font for each
    character in the M-text according to the following information.

    - The script character property of a character.
    - The language text property of a character.
    - The charset text property of a character.

    The documentation of mdraw_text () describes how that information is
    used.  */

/***ja
    @addtogroup m17nFontset
    @brief フォントセットは一定のスタイルを共有するフォントの集合である

    @e フォントセット は @c MFontset 型のオブジェクトであり、多言語文
    書の表示の際、見かけの似たフォントを集めて一貫して取り扱うために使
    われる。フォントセットはまた、言語、文字セット、文字が与えられたと
    きに適切なフォントを選択するための情報も保持している。  */

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "m17n-gui.h"
#include "m17n-misc.h"
#include "internal.h"
#include "symbol.h"
#include "plist.h"
#include "character.h"
#include "charset.h"
#include "internal-gui.h"
#include "font.h"
#include "fontset.h"

struct MFontset
{
  M17NObject control;

  /* Name of the fontset.  */
  MSymbol name;

  /* Initialized to 0, and incremented by one each time the fontset is
     modified.  */
  unsigned tick;

  /* Database from which to load the contents of the fontset.  Once
     loaded, this member is set to NULL.  */
  MDatabase *mdb;

  /* SCRIPT vs PER-LANGUAGE (which is a plist LANGUAGE vs FONT-GROUP) */
  MPlist *per_script;

  /* CHARSET vs FONT-GROUP */
  MPlist *per_charset;

  /* FONT-GROUP */
  MPlist *fallback;

  /* Plist of Mt vs font specs. */
  MPlist *font_spec_list;
};

static MFontset *default_fontset;

static MPlist *fontset_list;

struct MRealizedFontset
{
  /* Fontset from which the realized fontset is realized.  */
  MFontset *fontset;

  /* Initialized to <fontset>->tick.  */
  unsigned tick;

  /* Font spec extracted from a face.  */
  MFont spec;

  /* The frame on which the realized fontset is realized.  */
  MFrame *frame;

  MPlist *per_script;

  MPlist *per_charset;

  MPlist *fallback;
};


static MPlist *
load_font_group (MPlist *plist, MPlist *elt, MPlist *spec_list)
{
  MPLIST_DO (elt, elt)
    {
      /* ELT ::= ( FONT-SPEC-LIST [ LAYOUTER ] ) ...  */
      MPlist *elt2, *p;
      MFont font, *spec = NULL;
      MSymbol layouter_name;

      if (! MPLIST_PLIST_P (elt))
	MWARNING (MERROR_FONTSET);
      elt2 = MPLIST_PLIST (elt);
      if (! MPLIST_PLIST_P (elt2))
	MWARNING (MERROR_FONTSET);
      mfont__set_spec_from_plist (&font, MPLIST_PLIST (elt2));
      MPLIST_DO (p, spec_list)
        {
	  if (! memcmp (MPLIST_VAL (p), &font, sizeof (MFont)))
	    {
	      spec = MPLIST_VAL (p);
	      break;
	    }
        }
      if (! spec)
	{
	  MSTRUCT_MALLOC (spec, MERROR_FONTSET);
	  *spec = font;
	  mplist_add (spec_list, Mt, spec);
	}
      elt2 = MPLIST_NEXT (elt2);
      layouter_name = Mt;
      if (MPLIST_SYMBOL_P (elt2))
	layouter_name = MPLIST_SYMBOL (elt2);
      if (layouter_name == Mnil)
	layouter_name = Mt;
      plist = mplist_add (plist, layouter_name, spec);
      continue;
    warning:
      /* ANSI-C requires some statement after a label.  */
      continue;
    }
  return plist;
}

/* Load FONTSET->per_script from the data in FONTSET->mdb.  */

static void
load_fontset_contents (MFontset *fontset)
{
  MPlist *per_script, *per_charset, *fallback, *spec_list, *font_group;
  MSymbol script, lang;
  MPlist *fontset_def, *plist;

  fontset->per_script = per_script = mplist ();
  fontset->per_charset = per_charset = mplist ();
  fontset->fallback = fallback = mplist ();
  fontset->font_spec_list = spec_list = mplist ();
  if (! (fontset_def = (MPlist *) mdatabase_load (fontset->mdb)))
    return;

  MPLIST_DO (plist, fontset_def)
    {
      /* PLIST ::= ( SCRIPT ( LANGUAGE FONT-SPEC-ELT ... ) ... )
		   | (CHARSET FONT-SPEC-ELT ...)
		   | FONT-SPEC-ELT  */
      MPlist *elt;

      if (! MPLIST_PLIST_P (plist))
	MWARNING (MERROR_FONTSET);
      elt = MPLIST_PLIST (plist);
      if (! MPLIST_SYMBOL_P (elt))
	MWARNING (MERROR_FONTSET);
      script = MPLIST_SYMBOL (elt);
      elt = MPLIST_NEXT (elt);
      if (! MPLIST_PLIST_P (elt))
	MWARNING (MERROR_FONTSET);
      if (script == Mnil)
	fallback = load_font_group (fallback, elt, spec_list);
      else if (MPLIST_PLIST_P (MPLIST_PLIST (elt)))
	{
	  font_group = mplist_find_by_key (fontset->per_charset, script);
	  if (! font_group)
	    {
	      font_group = mplist ();
	      per_charset = mplist_add (per_charset, script, font_group);
	    }
	  load_font_group (font_group, elt, spec_list);
	}
      else
	{
	  MPlist *per_lang = mplist_find_by_key (fontset->per_script, script);

	  if (! per_lang)
	    {
	      per_lang = mplist ();
	      per_script = mplist_add (per_script, script, per_lang);
	    }

	  MPLIST_DO (elt, elt)
	    {
	      /* ELT ::= ( LANGUAGE FONT-DEF ...) ... */
	      MPlist *elt2;

	      if (! MPLIST_PLIST_P (elt))
		MWARNING (MERROR_FONTSET);
	      elt2 = MPLIST_PLIST (elt);
	      if (! MPLIST_SYMBOL_P (elt2))
		MWARNING (MERROR_FONTSET);
	      lang = MPLIST_SYMBOL (elt2);
	      if (lang == Mnil)
		lang = Mt;
	      font_group = mplist_find_by_key (per_lang, lang);
	      if (! font_group)
		{
		  font_group = mplist ();
		  mplist_add (per_lang, lang, font_group);
		}
	      elt2 = MPLIST_NEXT (elt2);
	      load_font_group (font_group, elt2, spec_list);
	    }
	}
      continue;

    warning:
      /* ANSI-C requires some statement after a label.  */
      continue;
    }

  M17N_OBJECT_UNREF (fontset_def);
  fontset->mdb = NULL;
}

static void
free_fontset (void *object)
{
  MFontset *fontset = (MFontset *) object;
  MPlist *plist, *pl, *p;

  if (fontset->per_script)
    {
      MPLIST_DO (plist, fontset->per_script)
	{
	  MPLIST_DO (pl, MPLIST_PLIST (plist))
	    {
	      p = MPLIST_PLIST (pl);
	      M17N_OBJECT_UNREF (p);
	    }
	  pl = MPLIST_PLIST (plist);
	  M17N_OBJECT_UNREF (pl);
	}
      M17N_OBJECT_UNREF (fontset->per_script);
    }
  if (fontset->per_charset)
    {
      MPLIST_DO (plist, fontset->per_charset)
	{
	  pl = MPLIST_PLIST (plist);
	  M17N_OBJECT_UNREF (pl);
	}
      M17N_OBJECT_UNREF (fontset->per_charset);
    }
  if (fontset->fallback)
    M17N_OBJECT_UNREF (fontset->fallback);
  plist = mplist_find_by_key (fontset_list, fontset->name);
  if (! plist)
    mdebug_hook ();
  mplist_pop (plist);
  if (fontset->font_spec_list)
    {
      if (((M17NObject *) (fontset->font_spec_list))->ref_count == 1)
	MPLIST_DO (plist, fontset->font_spec_list)
	  free (MPLIST_VAL (plist));
      M17N_OBJECT_UNREF (fontset->font_spec_list);
    }
  free (object);
}

static void
realize_font_group (MFrame *frame, MFont *request, MPlist *font_group,
		    int size)
{
  MPlist *plist = MPLIST_VAL (font_group), *pl, *p;

  mplist_set (font_group, Mnil, NULL);
  MPLIST_DO (pl, plist)
    {
      MRealizedFont *rfont = mfont__select (frame, MPLIST_VAL (pl), request,
					    size);

      if (rfont)
	{
	  rfont->layouter = MPLIST_KEY (pl);
	  if (rfont->layouter == Mt)
	    rfont->layouter = Mnil;
	  MPLIST_DO (p, font_group)
	    if (((MRealizedFont *) (MPLIST_VAL (p)))->score > rfont->score)
	      break;
	  mplist_push (p, Mt, rfont);
	}
    }
}

int
check_fontset_element (MFrame *frame, MPlist *element, MGlyph *g, int size)
{
  MRealizedFont *rfont = (MRealizedFont *) MPLIST_VAL (element);

  if (! rfont)
    /* We have already failed to select this font.  */
    return 0;
  if (! rfont->frame)
    {
      rfont = mfont__select (frame, &rfont->spec, &rfont->request, size);
      free (MPLIST_VAL (element));
      MPLIST_VAL (element) = rfont;
      if (! rfont)
	/* No font matches this spec.  */
	return 0;
    }

  g->code = mfont__encode_char (rfont, g->c);
  return (g->code != MCHAR_INVALID_CODE);
}



/* Internal API */

int
mfont__fontset_init ()
{
  Mfontset = msymbol ("fontset");
  Mfontset->managing_key = 1;
  fontset_list = mplist ();
  default_fontset = mfontset ("default");
  if (! default_fontset->mdb)
    {
      MFont font;

      MFONT_INIT (&font);
      mfont_put_prop (&font, Mregistry, msymbol ("iso8859-1"));
      mfontset_modify_entry (default_fontset, Mnil, Mnil, Mnil,
			     &font, Mnil, 1);
      mfont_put_prop (&font, Mregistry, msymbol ("iso10646-1"));
      mfontset_modify_entry (default_fontset, Mnil, Mnil, Mnil,
			     &font, Mnil, 1);
    }
  return 0;
}


void
mfont__fontset_fini ()
{
  while (! MPLIST_TAIL_P (fontset_list))
    free_fontset ((MFontset *) MPLIST_VAL (fontset_list));
  M17N_OBJECT_UNREF (fontset_list);
  fontset_list = NULL;
}


MRealizedFontset *
mfont__realize_fontset (MFrame *frame, MFontset *fontset, MFace *face)
{
  MRealizedFontset *realized;
  MFont request;
  MPlist *per_script, *per_lang, *per_charset, *font_group;
  MPlist *plist, *pl, *p;

  if (fontset->mdb)
    load_fontset_contents (fontset);

  mfont__set_spec_from_face (&request, face);
  if (request.property[MFONT_SIZE] <= 0)
    {
      mdebug_hook ();
      request.property[MFONT_SIZE] = 120;
    }
  MPLIST_DO (p, frame->realized_fontset_list)
    {
      realized = (MRealizedFontset *) MPLIST_VAL (p);
      if (fontset->name == MPLIST_KEY (p)
	  && ! memcmp (&request, &realized->spec, sizeof (request)))
	return realized;
    }

  MSTRUCT_MALLOC (realized, MERROR_FONTSET);
  realized->fontset = fontset;
  realized->tick = fontset->tick;
  realized->spec = request;
  realized->frame = frame;
  realized->per_script = per_script = mplist ();
  MPLIST_DO (plist, fontset->per_script)
    {
      per_lang = mplist ();
      per_script = mplist_add (per_script, MPLIST_KEY (plist), per_lang);
      MPLIST_DO (pl, MPLIST_PLIST (plist))
	{
	  font_group = mplist ();
	  mplist_add (font_group, Mplist, MPLIST_VAL (pl));
	  per_lang = mplist_add (per_lang, MPLIST_KEY (pl), font_group);
	}
    }

  realized->per_charset = per_charset = mplist ();
  MPLIST_DO (plist, fontset->per_charset)
    {
      font_group = mplist ();
      mplist_add (font_group, Mplist, MPLIST_VAL (plist));
      per_charset = mplist_add (per_charset, MPLIST_KEY (plist), font_group);
    }

  realized->fallback = mplist ();
  mplist_add (realized->fallback, Mplist, fontset->fallback);

  mplist_add (frame->realized_fontset_list, fontset->name, realized);
  return realized;
}


void
mfont__free_realized_fontset (MRealizedFontset *realized)
{
  MPlist *plist, *pl, *p;
  MRealizedFont *rfont;

  if (realized->per_script)
    {
      MPLIST_DO (plist, realized->per_script)
	{
	  MPLIST_DO (pl, MPLIST_PLIST (plist))
	    {
	      MPLIST_DO (p, MPLIST_PLIST (pl))
		if ((rfont = MPLIST_VAL (p)) && ! rfont->frame)
		  free (rfont);
	      p = MPLIST_PLIST (pl);
	      M17N_OBJECT_UNREF (p);
	    }
	  pl = MPLIST_PLIST (plist);
	  M17N_OBJECT_UNREF (pl);
	}
      M17N_OBJECT_UNREF (realized->per_script);
    }
  if (realized->per_charset)
    {
      MPLIST_DO (plist, realized->per_charset)
	{
	  MPLIST_DO (pl, MPLIST_PLIST (plist))
	    if ((rfont = MPLIST_VAL (pl)) && ! rfont->frame)
	      free (rfont);
	  pl = MPLIST_PLIST (plist);
	  M17N_OBJECT_UNREF (pl);
	}
      M17N_OBJECT_UNREF (realized->per_charset);
    }
  if (realized->fallback)
    {
      MPLIST_DO (plist, realized->fallback)
	if ((rfont = MPLIST_VAL (plist)) && ! rfont->frame)
	  free (rfont);
      M17N_OBJECT_UNREF (realized->fallback);
    }

  free (realized);
}


MRealizedFont *
mfont__lookup_fontset (MRealizedFontset *realized, MGlyph *g, int *num,
		       MSymbol script, MSymbol language, MSymbol charset,
		       int size)
{
  MFrame *frame = realized->frame;
  MCharset *preferred_charset = (charset == Mnil ? NULL : MCHARSET (charset));
  MPlist *per_charset, *per_script, *per_lang, *font_group;
  MPlist *font_groups[256], *plist;
  int n_font_group = 0;
  MRealizedFont *first = NULL, *rfont;
  int first_len;
  int i;

  if (preferred_charset
      && (per_charset = mplist_get (realized->per_charset, charset)) != NULL)
    font_groups[n_font_group++] = per_charset;
  if (script != Mnil
      && ((per_script = mplist_find_by_key (realized->per_script, script))
	  != NULL))
    {
      /* The first loop is for matching language (if any), and the second
	 loop is for non-matching languages.  */
      if (language == Mnil)
	language = Mt;
      for (i = 0; i < 2; i++)
	{
	  MPLIST_DO (per_lang, MPLIST_PLIST (per_script))
	    if ((MPLIST_KEY (per_lang) == language) != i)
	      font_groups[n_font_group++] = MPLIST_PLIST (per_lang);
	}
    }
  font_groups[n_font_group++] = realized->fallback;

  if (n_font_group == 1)
    {
      /* As we only have a fallback font group, try all the other
	 fonts too.  */
      MPLIST_DO (per_script, realized->per_script)
	MPLIST_DO (per_lang, MPLIST_PLIST (per_script))
	  font_groups[n_font_group++] = MPLIST_PLIST (per_lang);
      MPLIST_DO (per_charset, realized->per_charset)
	font_groups[n_font_group++] = MPLIST_PLIST (per_charset);
    }

  for (i = 0; i < n_font_group; i++)
    {
      int j;
      
      if (MPLIST_PLIST_P (font_groups[i]))
	realize_font_group (frame, &realized->spec, font_groups[i], size);

      MPLIST_DO (plist, font_groups[i])
        {
	  rfont = (MRealizedFont *) MPLIST_VAL (plist);
	  g->code = mfont__encode_char (rfont, g->c);
	  if (g->code != MCHAR_INVALID_CODE)
	    break;
	}
      if (MPLIST_TAIL_P (plist))
	continue;
      for (j = 1; j < *num; j++)
	{
	  g[j].code = mfont__encode_char (rfont, g[j].c);
	  if (g[j].code == MCHAR_INVALID_CODE)
	    break;
	}
      if (! first)
	first = rfont, first_len = j;
      if (j == *num)
	/* We found a font that can display all requested
	   characters.  */
	break;

      MPLIST_DO (plist, MPLIST_NEXT (plist))
	{
	  rfont = (MRealizedFont *) MPLIST_VAL (plist);
	  for (j = 0; j < *num; j++)
	    {
	      g[j].code = mfont__encode_char (rfont, g[j].c);
	      if (g[j].code == MCHAR_INVALID_CODE)
		break;
	    }
	  if (j == *num)
	    break;
	}
      if (! MPLIST_TAIL_P (plist))
	break;
    }

  if (i == n_font_group) 
    {
      if (! first)
	return NULL;
      rfont = first, *num = first_len;
      for (i = 0; i < *num; i++)
	g[i].code = mfont__encode_char (rfont, g[i].c);
    }
  if (! rfont->status
      && mfont__open (rfont) < 0)
    {
      MPLIST_VAL (font_group) = NULL;
      return NULL;
    }
  return rfont;
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

/*** @addtogroup m17nFontset */
/*** @{ */

/*=*/
/***en
    @brief Return a fontset.

    The mfontset () function returns a pointer to a fontset object of
    name $NAME.  If $NAME is @c NULL, it returns a pointer to the
    default fontset.

    If no fontset has the name $NAME, a new one is created.  At that
    time, if there exists a data \<@c fontset, $NAME\> in the m17n
    database, the fontset contents are initialized according to the
    data.  If no such data exists, the fontset contents are left
    vacant.

    The macro M17N_INIT () creates the default fontset.  An
    application program can modify it before the first call of mframe
    ().

    @return
    This function returns a pointer to the found or newly created
    fontset.  */

MFontset *
mfontset (char *name)
{
  MSymbol sym;
  MFontset *fontset;

  if (! name)
    return default_fontset;
  sym = msymbol (name);
  fontset = mplist_get (fontset_list, sym);
  if (fontset)
    return fontset;
  M17N_OBJECT (fontset, free_fontset, MERROR_FONTSET);
  fontset->name = sym;
  fontset->mdb = mdatabase_find (Mfontset, sym, Mnil, Mnil);
  if (! fontset->mdb)
    {
      fontset->per_script = mplist ();
      fontset->per_charset = mplist ();
      fontset->fallback = mplist ();
    }
  mplist_put (fontset_list, sym, fontset);
  M17N_OBJECT_REF (fontset);
  return fontset;
}

/*=*/

/***en
    @brief Return the name of a fontset

    The mfontset_name () function returns the name of $FONTSET.  */
MSymbol
mfontset_name (MFontset *fontset)
{
  return fontset->name;
}

/*=*/

/***en
    @brief Make a copy a fontset

    The mfontset_copy () function makes a copy of $FONTSET, gives it a
    name $NAME, and returns a pointer to the created copy.  $NAME must
    not be a name of existing fontset.  Otherwise, this function
    returns NULL without making a copy.  */

MFontset *
mfontset_copy (MFontset *fontset, char *name)
{
  MSymbol sym = msymbol (name);
  MFontset *copy = mplist_get (fontset_list, sym);
  MPlist *plist, *pl;

  if (copy)
    return NULL;
  M17N_OBJECT (copy, free_fontset, MERROR_FONTSET);
  copy->name = sym;

  if (fontset->per_script)
    {
      copy->per_script = mplist ();
      MPLIST_DO (plist, fontset->per_script)
        {
	  MPlist *new = mplist ();

	  MPLIST_DO (pl, MPLIST_PLIST (plist))
	    mplist_add (new, MPLIST_KEY (pl), mplist_copy (MPLIST_PLIST (pl)));
	  mplist_add (copy->per_script, MPLIST_KEY (plist), new);
	}
    }
  if (fontset->per_charset)
    {
      copy->per_charset = mplist ();
      MPLIST_DO (plist, fontset->per_charset)
	mplist_add (copy->per_charset, MPLIST_KEY (plist),
		    mplist_copy (MPLIST_PLIST (plist)));
    }
  if (fontset->fallback)
    copy->fallback = mplist_copy (fontset->fallback);

  copy->font_spec_list = fontset->font_spec_list;
  M17N_OBJECT_REF (copy->font_spec_list);

  mplist_put (fontset_list, sym, copy);
  M17N_OBJECT_REF (copy);
  return copy;
}

/*=*/

/***en
    @brief Modify the contents of a fontset.

    The mfontset_modify_entry () function associates, in $FONTSET, a
    copy of $FONT with the $SCRIPT / $LANGUAGE pair or with $CHARSET.

    Each font in a fontset is associated with a particular
    script/language pair, with a particular charset, or with the
    symbol @c Mnil.  The fonts that are associated with the same item
    make a group.

    If $SCRIPT is not @c Mnil, it must be a symbol identifying a
    script.  In this case, $LANGUAGE is either a symbol identifying a
    language or @c Mnil, and $FONT is associated with the $SCRIPT /
    $LANGUAGE pair.

    If $CHARSET is not @c Mnil, it must be a symbol representing a
    charset object.  In this case, $FONT is associated with that
    charset.

    If both $SCRIPT and $CHARSET are not @c Mnil, two copies of $FONT
    are created.  Then one is associated with the script/language pair
    and the other with the charset.

    If both $SCRIPT and $CHARSET are @c Mnil, $FONT is associated
    with @c Mnil.  This kind of fonts are called <i>fallback
    fonts</i>.

    The argument $HOW specifies the priority of $FONT.  If $HOW is
    positive, $FONT has the highest priority in the group of fonts
    that are associated with the same item.  If $HOW is negative,
    $FONT has the lowest priority.  If $HOW is zero, $FONT becomes the
    only available font for the associated item; all the other fonts
    are removed from the group.

    If $LAYOUTER_NAME is not @c Mnil, it must be a symbol
    representing a @ref flt.  In that case, if $FONT is selected for
    drawing an M-text, that font layout table is used to generate a
    glyph code sequence from a character sequence.

    @return
    If the operation was successful, mfontset_modify_entry () returns 0.
    Otherwise it returns -1 and assigns an error code to the external
    variable @c merror_code.  */

/***ja
    @brief 言語とスクリプトの組み合わせにフォントを関連付ける

    関数 mfontset_set_language_script () は、$LANGUAGE と $SCRIPT の組
    み合わせに対してフォント $FONT を使うよう、フォントセット $FONTSET 
    を設定する。$LANGUAGE と $SCRIPT は、それぞれ言語とスクリプトを指
    定するシンボルである。

    言語とスクリプトの組み合わせ一つに対して複数のフォントを設定するこ
    ともできる。$PREPEND_P が 0 以外ならば、$FONT はその組み合わせに対
    して最優先で用いられるものとなる。そうでなければ、優先度最低のフォ
    ントとして登録される。

    @return
    処理が成功したとき、mfontset_set_language_script () は 0 を返す。
    失敗したときは -1 を返し、外部変数 @c merror_code にエラーコードを
    設定する。  */

/***
    @errors
    @c MERROR_SYMBOL  */

int
mfontset_modify_entry (MFontset *fontset,
		       MSymbol script, MSymbol language, MSymbol charset,
		       MFont *spec, MSymbol layouter_name,
		       int how)
{
  MPlist *per_lang, *plist[3], *pl;
  MFont *font = NULL;
  int i;

  if (fontset->mdb)
    load_fontset_contents (fontset);

  MPLIST_DO (pl, fontset->font_spec_list)
    {
      if (! memcmp (MPLIST_VAL (pl), spec, sizeof (MFont)))
	{
	  font = MPLIST_VAL (pl);
	  break;
	}
    }
  if (! font)
    {
      font = mfont ();
      *font = *spec;
      mplist_add (fontset->font_spec_list, Mt, font);
    }

  i = 0;
  if (script != Mnil)
    {
      if (language == Mnil)
	language = Mt;
      per_lang = mplist_get (fontset->per_script, script);
      if (! per_lang)
	mplist_add (fontset->per_script, script, per_lang = mplist ());
      plist[i] = mplist_get (per_lang, language);
      if (! plist[i])
	mplist_add (per_lang, language, plist[i] = mplist ());
      i++;
    }
  if (charset != Mnil)
    {
      plist[i] = mplist_get (fontset->per_charset, charset);
      if (! plist[i])
	mplist_add (fontset->per_charset, charset, plist[i] = mplist ());
      i++;
    }
  if (script == Mnil && charset == Mnil)
    {
      plist[i++] = fontset->fallback;
    }

  if (layouter_name == Mnil)
    layouter_name = Mt;
  for (i--; i >= 0; i--)
    {
      if (how == -1)
	mplist_push (plist[i], layouter_name, font);
      else if (how == 1)
	mplist_add (plist[i], layouter_name, font);
      else
	{
	  mplist_set (plist[i], Mnil, NULL);
	  mplist_add (plist[i], layouter_name, font);
	}
    }

  return 0;
}

/*** @} */

/*** @addtogroup m17nDebug */
/*=*/
/*** @{  */

/***en
    @brief Dump a fontset

    The mdebug_dump_fontset () function prints $FONTSET in a human readable
    way to the stderr.  $INDENT specifies how many columns to indent
    the lines but the first one.

    @return
    This function returns $FONTSET.  */

MFontset *
mdebug_dump_fontset (MFontset *fontset, int indent)
{
  char *prefix = (char *) alloca (indent + 1);
  MPlist *plist, *pl, *p;

  memset (prefix, 32, indent);
  prefix[indent] = 0;

  fprintf (stderr, "(fontset %s", fontset->name->name);
  if (fontset->per_script)
    MPLIST_DO (plist, fontset->per_script)
      {
	fprintf (stderr, "\n  %s(%s", prefix, MPLIST_KEY (plist)->name);
	MPLIST_DO (pl, MPLIST_PLIST (plist))
	  {
	    fprintf (stderr, "\n    %s(%s", prefix, MPLIST_KEY (pl)->name);
	    MPLIST_DO (p, MPLIST_PLIST (pl))
	      {
		fprintf (stderr, "\n      %s(%s ", prefix,
			 MPLIST_KEY (p)->name);
		mdebug_dump_font (MPLIST_VAL (p));
		fprintf (stderr, ")");
	      }
	    fprintf (stderr, ")");
	  }
	fprintf (stderr, ")");
      }
  if (fontset->per_charset)
    MPLIST_DO (pl, fontset->per_charset)
      {
	fprintf (stderr, "\n  %s(%s", prefix, MPLIST_KEY (pl)->name);
	MPLIST_DO (p, MPLIST_PLIST (pl))
	  {
	    fprintf (stderr, "\n    %s(%s ", prefix, MPLIST_KEY (p)->name);
	    mdebug_dump_font (MPLIST_VAL (p));
	    fprintf (stderr, ")");
	  }
	fprintf (stderr, ")");
      }

  if (fontset->fallback)
    MPLIST_DO (p, fontset->fallback)
      {
	fprintf (stderr, "\n  %s(%s ", prefix, MPLIST_KEY (p)->name);
	mdebug_dump_font (MPLIST_VAL (p));
	fprintf (stderr, ")");
      }

  fprintf (stderr, ")");
  return fontset;
}

/*** @} */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
