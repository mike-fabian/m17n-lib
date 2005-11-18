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

    @li The script character property of a character.
    @li The language text property of a character.
    @li The charset text property of a character.

    The documentation of mdraw_text () describes how that information is
    used.  */

/***ja @addtogroup m17nFontset 

    @brief フォントセットは文字からフォントへの対応付けを行うオブジェクトである.

    @e フォントセット は @c MFontset 型のオブジェクトである。M-text 
    の表示の際、フォントセットは以下の情報を用いて M-text 
    中の個々の文字にどのフォントを用いるか決める規則を与える。

    @li 文字の文字プロパティ "スクリプト"
    @li 文字のテキストプロパティ "言語"
    @li 文字のテキストプロパティ "文字セット"

    これらの情報がどのように用いられるかは mdraw_text () の説明を参照のこと。

    */

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
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
};

static MFontset *default_fontset;

static MPlist *fontset_list;

struct MRealizedFontset
{
  /* Fontset from which the realized fontset is realized.  */
  MFontset *fontset;

  /* Initialized to <fontset>->tick.  */
  unsigned tick;

  /* Font spec that must be satisfied, or NULL.  */
  MFont *spec;

  /* Font spec requested by a face.  */
  MFont request;

  /* The frame on which the realized fontset is realized.  */
  MFrame *frame;

  MPlist *per_script;

  MPlist *per_charset;

  MPlist *fallback;
};


static MPlist *
load_font_group (MPlist *plist, MPlist *elt)
{
  MPLIST_DO (elt, elt)
    {
      /* ELT ::= ( FONT-SPEC [ LAYOUTER ] ) ...  */
      MPlist *elt2;
      MFont *font;
      MSymbol layouter_name;

      if (! MPLIST_PLIST_P (elt))
	MWARNING (MERROR_FONTSET);
      elt2 = MPLIST_PLIST (elt);
      if (! MPLIST_PLIST_P (elt2))
	MWARNING (MERROR_FONTSET);
      MSTRUCT_CALLOC (font, MERROR_FONTSET);
      mfont__set_spec_from_plist (font, MPLIST_PLIST (elt2));
      elt2 = MPLIST_NEXT (elt2);
      layouter_name = Mt;
      if (MPLIST_SYMBOL_P (elt2))
	layouter_name = MPLIST_SYMBOL (elt2);
      if (layouter_name == Mnil)
	layouter_name = Mt;
      plist = mplist_add (plist, layouter_name, font);
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
  MPlist *per_script, *per_charset, *font_group;
  MPlist *fontset_def, *plist;

  fontset->per_script = per_script = mplist ();
  fontset->per_charset = per_charset = mplist ();
  fontset->fallback = mplist ();
  if (! (fontset_def = (MPlist *) mdatabase_load (fontset->mdb)))
    return;

  MPLIST_DO (plist, fontset_def)
    {
      /* PLIST ::=   ( SCRIPT ( LANGUAGE ( FONT-SPEC [LAYOUTER]) ... ) ... )
		   | ( CHARSET ( FONT-SPEC [LAYOUTER] ) ...)
		   | ( nil ( FONT-SPEC [LAYOUTER] ) ...)
	 FONT-SPEC :: = ( ... ) */
      MPlist *elt;
      MSymbol sym;

      if (! MPLIST_PLIST_P (plist))
	MWARNING (MERROR_FONTSET);
      elt = MPLIST_PLIST (plist);
      if (! MPLIST_SYMBOL_P (elt))
	MWARNING (MERROR_FONTSET);
      sym = MPLIST_SYMBOL (elt);
      elt = MPLIST_NEXT (elt);
      if (! MPLIST_PLIST_P (elt))
	MWARNING (MERROR_FONTSET);
      if (sym == Mnil)
	load_font_group (fontset->fallback, elt);
      else if (MPLIST_PLIST_P (MPLIST_PLIST (elt)))
	{
	  /* SYM is a charset.  */
	  font_group = mplist ();
	  per_charset = mplist_add (per_charset, sym, font_group);
	  load_font_group (font_group, elt);
	}
      else
	{
	  /* SYM is a script */
	  MPlist *per_lang = mplist ();

	  per_script = mplist_add (per_script, sym, per_lang);
	  MPLIST_DO (elt, elt)
	    {
	      /* ELT ::= ( LANGUAGE FONT-DEF ...) ... */
	      MPlist *elt2;
	      MSymbol lang;

	      if (! MPLIST_PLIST_P (elt))
		MWARNING (MERROR_FONTSET);
	      elt2 = MPLIST_PLIST (elt);
	      if (! MPLIST_SYMBOL_P (elt2))
		MWARNING (MERROR_FONTSET);
	      lang = MPLIST_SYMBOL (elt2);
	      if (lang == Mnil)
		lang = Mt;
	      font_group = mplist ();
	      mplist_add (per_lang, lang, font_group);
	      elt2 = MPLIST_NEXT (elt2);
	      load_font_group (font_group, elt2);
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
	      MPLIST_DO (p, MPLIST_PLIST (pl))
		free (MPLIST_VAL (p));
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
      MPLIST_DO (pl, fontset->per_charset)
	{
	  MPLIST_DO (p, MPLIST_PLIST (pl))
	    free (MPLIST_VAL (p));
	  p = MPLIST_PLIST (p);
	  M17N_OBJECT_UNREF (p);
	}
      M17N_OBJECT_UNREF (fontset->per_charset);
    }
  if (fontset->fallback)
    {
      MPLIST_DO (p, fontset->fallback)
	free (MPLIST_VAL (p));
      M17N_OBJECT_UNREF (fontset->fallback);
    }

  plist = mplist_find_by_key (fontset_list, fontset->name);
  if (! plist)
    mdebug_hook ();
  mplist_pop (plist);
  if (MPLIST_TAIL_P (fontset_list))
    {
      M17N_OBJECT_UNREF (fontset_list);
      fontset_list = NULL;
    }
  free (object);
}

static void
realize_fontset_elements (MFrame *frame, MRealizedFontset *realized)
{
  MFontset *fontset = realized->fontset;
  MPlist *per_script, *per_charset, *font_group;
  MPlist *plist, *pl, *p;

  realized->per_script = per_script = mplist ();
  /* The actual elements of per_script are realized on demand.  */
#if 0
  MPLIST_DO (plist, fontset->per_script)
    {
      per_lang = mplist ();
      per_script = mplist_add (per_script, MPLIST_KEY (plist), per_lang);
      MPLIST_DO (pl, MPLIST_PLIST (plist))
	{
	  font_group = mplist ();
	  per_lang = mplist_add (per_lang, MPLIST_KEY (pl), font_group);
	  MPLIST_DO (p, MPLIST_PLIST (pl))
	    font_group = mplist_add (font_group,
				     MPLIST_KEY (p), MPLIST_VAL (p));
	}
    }
#endif

  realized->per_charset = per_charset = mplist ();
  MPLIST_DO (pl, fontset->per_charset)
    {
      font_group = mplist ();
      per_charset = mplist_add (per_charset, MPLIST_KEY (plist), font_group);
      MPLIST_DO (p, MPLIST_PLIST (pl))
	font_group = mplist_add (font_group, MPLIST_KEY (p), MPLIST_VAL (p));
    }
  realized->fallback = font_group = mplist ();
  MPLIST_DO (p, fontset->fallback)
    font_group = mplist_add (font_group, MPLIST_KEY (p), MPLIST_VAL (p));
}


/* Return a plist of fonts for SCRIPT in FONTSET.  The returned list
   is acutally a plist of languages vs font groups (which is a plist).
   If SCRIPT is nil, return a plist of fallback fonts.  If FONTSET
   doesn't record any fonts for SCRIPT, generate a proper font spec
   lists for X backend and FreeType backend.  */

MPlist *
get_per_script (MFontset *fontset, MSymbol script)
{
  MPlist *plist;

  if (script == Mnil)
    return fontset->fallback;
  plist = mplist_get (fontset->per_script, script);
  if (! plist)
    {
      int len = MSYMBOL_NAMELEN (script);
      char *cap = alloca (8 + len + 1);
      MSymbol capability;
      MFont *font;
      MPlist *pl, *p;

      sprintf (cap, ":script=%s", MSYMBOL_NAME (script));
      capability = msymbol (cap);

      pl = mplist ();
      MPLIST_DO (p, fontset->fallback)
	{
	  font = mfont_copy (MPLIST_VAL (p));
	  mfont_put_prop (font, Mregistry, Municode_bmp);
	  font->source = MFONT_SOURCE_FT;
	  font->capability = capability;
	  mplist_add (pl, Mt, font);

	  font = mfont_copy (MPLIST_VAL (p));
	  mfont_put_prop (font, Mregistry, Miso10646_1);
	  font->source = MFONT_SOURCE_X;
	  font->capability = capability;
	  mplist_add (pl, Mt, font);
	}
      plist = mplist ();
      mplist_add (plist, Mt, pl);
      mplist_add (fontset->per_script, script, plist);
    }
  return plist;
}

static void
free_realized_fontset_elements (MRealizedFontset *realized)
{
  MPlist *plist, *pl, *p;
  MFont *font;
  MFontList *font_list;

  if (realized->per_script)
    {
      MPLIST_DO (plist, realized->per_script)
	{
	  MPLIST_DO (pl, MPLIST_PLIST (plist))
	    {
	      MPLIST_DO (p, MPLIST_PLIST (pl))
		{
		  font = MPLIST_VAL (p);
		  if (font->type == MFONT_TYPE_OBJECT)
		    {
		      font_list = (MFontList *) font;
		      free (font_list->fonts);
		      free (font_list);
		    }
		  /* This is to avoid freeing rfont again by the later
		     M17N_OBJECT_UNREF (p) */
		  MPLIST_KEY (p) = Mt;
		}
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
	    {
	      font = MPLIST_VAL (pl);
	      if (font->type == MFONT_TYPE_OBJECT)
		{
		  font_list = (MFontList *) font;
		  free (font_list->fonts);
		  free (font_list);
		}
	      MPLIST_KEY (pl) = Mt;
	    }
	  pl = MPLIST_PLIST (plist);
	  M17N_OBJECT_UNREF (pl);
	}
      M17N_OBJECT_UNREF (realized->per_charset);
    }
  if (realized->fallback)
    {
      MPLIST_DO (plist, realized->fallback)
	{
	  font = MPLIST_VAL (plist);
	  if (font->type == MFONT_TYPE_OBJECT)
	    {
	      font_list = (MFontList *) font;
	      free (font_list->fonts);
	      free (font_list);
	    }
	  MPLIST_KEY (plist) = Mt;
	}
      M17N_OBJECT_UNREF (realized->fallback);
    }
}

static void
update_fontset_elements (MRealizedFontset *realized)
{
  free_realized_fontset_elements (realized);
  realize_fontset_elements (realized->frame, realized);
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
  M17N_OBJECT_UNREF (default_fontset);
  default_fontset = NULL;
}


MRealizedFontset *
mfont__realize_fontset (MFrame *frame, MFontset *fontset,
			MFace *face, MFont *spec)
{
  MRealizedFontset *realized;
  MFont request;
  MPlist *plist;

  if (fontset->mdb)
    load_fontset_contents (fontset);

  MFONT_INIT (&request);
  mfont__set_spec_from_face (&request, face);
  if (request.size <= 0)
    {
      mdebug_hook ();
      request.size = 120;
    }
  MPLIST_DO (plist, frame->realized_fontset_list)
    {
      realized = (MRealizedFontset *) MPLIST_VAL (plist);
      if (fontset->name == MPLIST_KEY (plist)
	  && ! memcmp (&request, &realized->request, sizeof (MFont))
	  && (realized->spec
	      ? (spec && ! memcmp (spec, &realized->spec, sizeof (MFont)))
	      : ! spec))
	return realized;
    }

  MSTRUCT_CALLOC (realized, MERROR_FONTSET);
  realized->fontset = fontset;
  realized->tick = fontset->tick;
  if (spec)
    {
      MSTRUCT_CALLOC (realized->spec, MERROR_FONTSET);
      *realized->spec = *spec;
    }
  realized->request = request;
  realized->frame = frame;
  realize_fontset_elements (frame, realized);
  mplist_add (frame->realized_fontset_list, fontset->name, realized);
  return realized;
}


void
mfont__free_realized_fontset (MRealizedFontset *realized)
{
  free_realized_fontset_elements (realized);
  if (realized->spec)
    free (realized->spec);
  free (realized);
}


static MRealizedFont *
try_font_list (MFrame *frame, MFontList *font_list, MFont *request,
	       MSymbol layouter, MGlyph *g, int *num, int all, int exact)
{
  int i, j;
  MFont *font;
  MRealizedFont *rfont;

  for (i = 0; i < font_list->nfonts; i++)
    {
      if (font_list->fonts[i].font->type == MFONT_TYPE_SPEC)
	MFATAL (MERROR_FONT);
      if (exact)
	{
	  if (font_list->fonts[i].score > 0)
	    break;
	}
      else
	{
	  if (font_list->fonts[i].score == 0)
	    continue;
	}
      font = font_list->fonts[i].font;
      if (font->type == MFONT_TYPE_FAILURE)
	continue;
      /* Check if this font can display all glyphs.  */
      for (j = 0; j < *num; j++)
	{
	  int c = g[j].type == GLYPH_CHAR ? g[j].c : ' ';
	  if (layouter != Mt
	      ? mfont__flt_encode_char (layouter, c) == MCHAR_INVALID_CODE
	      : ! mfont__has_char (frame, font, &font_list->object, c))
	    break;
	}
      if (j == 0 && *num > 0)
	continue;
      if (j == *num || !all)
	{
	  /* We found a font that can display the requested range of
	     glyphs.  */
	  if (font->type == MFONT_TYPE_REALIZED)
	    rfont = (MRealizedFont *) font;
	  else
	    {
	      rfont = mfont__open (frame, font, &font_list->object);
	      if (! rfont)
		continue;
	      font_list->fonts[i].font = (MFont *) rfont;
	    }
	  rfont->layouter = layouter == Mt ? Mnil : layouter;
	  *num = j;
	  for (j = 0; j < *num; j++)
	    {
	      int c = g[j].type == GLYPH_CHAR ? g[j].c : ' ';

	      g[j].code = (rfont->layouter
			   ? mfont__flt_encode_char (rfont->layouter, c)
			   : mfont__encode_char (frame, (MFont *) rfont,
						 &font_list->object, c));
	    }
	  return rfont;
	}
    }
  return NULL;
}


static MRealizedFont *
try_font_group (MRealizedFontset *realized, MFont *request,
		MPlist *font_group, MGlyph *g, int *num, int size)
{
  MFrame *frame = realized->frame;
  MFont *font;
  MFontList *font_list;
  MRealizedFont *rfont;
  MPlist *plist;
  MSymbol layouter;
  int best_score = -1, worst_score;

  for (plist = font_group; ! MPLIST_TAIL_P (plist); )
    {
      int this_score;

      layouter = MPLIST_KEY (plist);
      font = MPLIST_VAL (plist);
      if (font->type == MFONT_TYPE_SPEC)
	{
	  /* We have not yet made this entry a MFontList.  */
	  if (realized->spec)
	    {
	      MFont this = *font;
	      
	      if (mfont__merge (&this, realized->spec, 1) < 0)
		{
		  mplist_pop (plist);
		  continue;
		}
	      font_list = mfont__list (frame, &this, &this, size);
	    }
	  else
	    font_list = mfont__list (frame, font, request, size);
	  if (! font_list)
	    {
	      /* As there's no font matching this spec, remove this
		 element from the font group.  */
	      mplist_pop (plist);
	      continue;
	    }
	  MPLIST_VAL (plist) = font_list;
	}
      else
	font_list = (MFontList *) font;

      this_score = font_list->fonts[0].score;
      if ((this_score == 0)
	  && (rfont = try_font_list (frame, font_list, request,
				     layouter, g, num, 1, 1)))
	return rfont;
      if (best_score < 0)
	{
	  best_score = worst_score = this_score;
	  plist = MPLIST_NEXT (plist);
	}
      else if (this_score >= worst_score)
	{
	  worst_score = this_score;
	  plist = MPLIST_NEXT (plist);
	}
      else
	{
	  MPlist *pl;

	  MPLIST_DO (pl, font_group)
	    if (this_score < ((MFontList *) MPLIST_VAL (pl))->fonts[0].score)
	      break;
	  mplist_pop (plist);
	  mplist_push (pl, layouter, font_list);
	}
    }

  /* We couldn't find an exact matching font that can display all
     glyphs.  Find one that can at least display all glyphs.  */
  MPLIST_DO (plist, font_group)
    {
      rfont = try_font_list (frame, MPLIST_VAL (plist), request,
			     MPLIST_KEY (plist), g, num, 1, 0);
      if (rfont)
	return rfont;
    }

  /* We couldn't find a font that can display all glyphs.  Find an
     exact matching font that can at least display the first
     glyph.  */
  MPLIST_DO (plist, font_group)
    {
      rfont = try_font_list (frame, MPLIST_VAL (plist), request,
			     MPLIST_KEY (plist), g, num, 0, 1);
      if (rfont)
	return rfont;
    }

  /* Find any font that can at least display the first glyph.  */
  MPLIST_DO (plist, font_group)
    {
      rfont = try_font_list (frame, MPLIST_VAL (plist), request,
			     MPLIST_KEY (plist), g, num, 0, 0);
      if (rfont)
	return rfont;
    }

  return NULL;
}

MRealizedFont *
mfont__lookup_fontset (MRealizedFontset *realized, MGlyph *g, int *num,
		       MSymbol script, MSymbol language, MSymbol charset,
		       int size, int ignore_fallback)
{
  MCharset *preferred_charset = (charset == Mnil ? NULL : MCHARSET (charset));
  MPlist *per_charset, *per_script, *per_lang;
  MPlist *plist;
  MRealizedFont *rfont = NULL;

  if (realized->tick != realized->fontset->tick)
    update_fontset_elements (realized);

  if (preferred_charset
      && (per_charset = mplist_get (realized->per_charset, charset)) != NULL
      && (rfont = try_font_group (realized, &realized->request, per_charset,
				  g, num, size)))
    return rfont;

  if (script != Mnil)
    {
      MFont request = realized->request;

      if (script != Mlatin)
	/* These are not appropriate for non-Latin scripts.  */
	request.property[MFONT_FOUNDRY]
	  = request.property[MFONT_FAMILY]
	  = request.property[MFONT_REGISTRY] = 0;

      per_script = mplist_get (realized->per_script, script);
      if (! per_script)
	{
	  per_script = mplist_copy (get_per_script (realized->fontset, script));
	  /* PER_SCRIPT ::= (LANGUAGE:(LAYOUTER:FONT-SPEC ...) ...) */
	  MPLIST_DO (plist, per_script)
	    MPLIST_VAL (plist) = mplist_copy (MPLIST_VAL (plist));
	  mplist_add (realized->per_script, script, per_script);
	}

      /* We prefer font groups in this order:
	  (1) group matching with LANGUAGE if LANGUAGE is not Mnil
	  (2) group for generic language
	  (3) group not matching with LANGUAGE  */
      if (language == Mnil)
	language = Mt;
      if ((per_lang = mplist_get (per_script, language))
	  && (rfont = try_font_group (realized, &request, per_lang,
				      g, num, size)))
	return rfont;

      if (language == Mt)
	{
	  /* Try the above (3) */
	  MPLIST_DO (plist, per_script)
	    if (MPLIST_KEY (plist) != language
		&& (rfont = try_font_group (realized, &request,
					    MPLIST_PLIST (plist),
					    g, num, size)))
	      return rfont;
	}
      else
	{
	  /* At first try the above (2) */
	  if ((per_lang = mplist_get (per_script, Mt))
	      && (rfont = try_font_group (realized, &request, per_lang,
					  g, num, size)))
	    return rfont;

	  /* Then try the above (3) */
	  MPLIST_DO (plist, per_script)
	    if (MPLIST_KEY (plist) != language
		&& MPLIST_KEY (plist) != Mt
		&& (rfont = try_font_group (realized, &request,
					    MPLIST_PLIST (plist),
					    g, num, size)))
	      return rfont;
	}
      if (ignore_fallback)
	return NULL;
    }

  if (language != Mnil)
    /* Find a font group for this language from all scripts.  */
    MPLIST_DO (plist, realized->per_script)
      {
	MFont request = realized->request;

	if (MPLIST_KEY (plist) != Mlatin)
	  request.property[MFONT_FOUNDRY]
	    = request.property[MFONT_FAMILY]
	    = request.property[MFONT_FAMILY] = 0;
	if ((per_lang = mplist_get (MPLIST_PLIST (plist), language))
	    && (rfont = try_font_group (realized, &request, per_lang,
					g, num, size)))
	  return rfont;
      }

  /* Try fallback fonts.  */
  if ((rfont = try_font_group (realized, &realized->request,
			       realized->fallback, g, num, size)))
    return rfont;

  return NULL;

  /* At last try all fonts.  */
  MPLIST_DO (per_script, realized->per_script)
    {
      MPLIST_DO (per_lang, MPLIST_PLIST (per_script))
	if ((rfont = try_font_group (realized, &realized->request,
				     MPLIST_PLIST (per_lang), g, num, size)))
	  return rfont;
    }
  MPLIST_DO (per_charset, realized->per_charset)
    if ((rfont = try_font_group (realized, &realized->request,
				 MPLIST_PLIST (per_charset), g, num, size)))
      return rfont;

  return NULL;
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
    application program can modify it before the first call of 
    mframe ().

    @return
    This function returns a pointer to the found or newly created
    fontset.  */
/***ja 
    @brief フォントセットを返す.

    関数 mfontset () は名前 $NAME を持つフォントセットオブジェクトへのポインタを返す。 
    $NAME が @c NULL ならば、デフォルトフォントセットへのポインタを返す。

    $NAME という名前を持つフォントセットがなければ、新しいものが作られる。その際、
    m17n データベースに \<@c fontset, $NAME\> 
    というデータがあれば、フォントセットはそのデータに沿って初期化される。
    なければ、空のままにされる。

    マクロ M17N_INIT () はデフォルトのフォントセットを作る。アプリケーションプログラムは
    mframe () を初めて呼ぶまでの間はデフォルトフォントセットを変更することができる。

    @return
    この関数は見つかった、あるいは作ったフォントセットへのポインタを返す。
     */

MFontset *
mfontset (char *name)
{
  MSymbol sym;
  MFontset *fontset;

  if (! name)
    {
      fontset = default_fontset;
      M17N_OBJECT_REF (fontset);
    }
  else
    {
      sym = msymbol (name);
      fontset = mplist_get (fontset_list, sym);
      if (fontset)
	M17N_OBJECT_REF (fontset);
      else
	{
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
	}
    }
  return fontset;
}

/*=*/

/***en
    @brief Return the name of a fontset.

    The mfontset_name () function returns the name of fontset $FONTSET.  */
/***ja
    @brief フォントセットの名前を返す.

    関数 mfontset_name () はフォントセット $FONTSET の名前を返す。  */
MSymbol
mfontset_name (MFontset *fontset)
{
  return fontset->name;
}

/*=*/

/***en
    @brief Make a copy of a fontset.

    The mfontset_copy () function makes a copy of fontset $FONTSET, gives it a
    name $NAME, and returns a pointer to the created copy.  $NAME must
    not be a name of existing fontset.  In such case, this function
    returns NULL without making a copy.  */
/***ja
    @brief フォントセットのコピーを作る.

    関数 mfontset_copy () はフォントセット $FONTSET のコピーを作って、名前
    $NAME を与え、そのコピーへのポインタを返す。$NAME 
    は既存のフォントセットの名前であってはならない。そのような場合にはコピーを作らずに
    NULL を返す。  */

MFontset *
mfontset_copy (MFontset *fontset, char *name)
{
  MSymbol sym = msymbol (name);
  MFontset *copy = mplist_get (fontset_list, sym);
  MPlist *plist, *pl, *p;

  if (copy)
    return NULL;
  M17N_OBJECT (copy, free_fontset, MERROR_FONTSET);
  copy->name = sym;

  if (fontset->mdb)
    load_fontset_contents (fontset);

  if (fontset->per_script)
    {
      copy->per_script = mplist ();
      MPLIST_DO (plist, fontset->per_script)
        {
	  MPlist *per_lang = mplist ();

	  mplist_add (copy->per_script, MPLIST_KEY (plist), per_lang);
	  MPLIST_DO (pl, MPLIST_PLIST (plist))
	    {
	      MPlist *font_group = mplist ();

	      per_lang = mplist_add (per_lang, MPLIST_KEY (pl), font_group);
	      MPLIST_DO (p, MPLIST_PLIST (pl))
		font_group = mplist_add (font_group, MPLIST_KEY (p),
					 mfont_copy (MPLIST_VAL (p)));
	    }
	}
    }
  if (fontset->per_charset)
    {
      MPlist *per_charset = mplist ();

      copy->per_charset = per_charset;
      MPLIST_DO (pl, fontset->per_charset)
	{
	  MPlist *font_group = mplist ();

	  per_charset = mplist_add (per_charset, MPLIST_KEY (pl), font_group);
	  MPLIST_DO (p, MPLIST_PLIST (pl))
	    font_group = mplist_add (font_group, MPLIST_KEY (p),
				     mfont_copy (MPLIST_VAL (p)));
	}
    }
  if (fontset->fallback)
    {
      MPlist *font_group = mplist ();

      copy->fallback = font_group;
      MPLIST_DO (p, fontset->fallback)
	font_group = mplist_add (font_group, MPLIST_KEY (p),
				 mfont_copy (MPLIST_VAL (p)));
    }				 

  mplist_put (fontset_list, sym, copy);
  return copy;
}

/*=*/

/***en
    @brief Modify the contents of a fontset.

    The mfontset_modify_entry () function associates, in fontset
    $FONTSET, a copy of $FONT with the $SCRIPT / $LANGUAGE pair or
    with $CHARSET.

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
    are created.  Then one is associated with the $SCRIPT / $LANGUAGE
    pair and the other with that charset.

    If both $SCRIPT and $CHARSET are @c Mnil, $FONT is associated with
    @c Mnil.  This kind of fonts are called @e fallback @e fonts.

    The argument $HOW specifies the priority of $FONT.  If $HOW is
    positive, $FONT has the highest priority in the group of fonts
    that are associated with the same item.  If $HOW is negative,
    $FONT has the lowest priority.  If $HOW is zero, $FONT becomes the
    only available font for the associated item; all the other fonts
    are removed from the group.

    If $LAYOUTER_NAME is not @c Mnil, it must be a symbol representing
    a @ref flt (font layout table).  In that case, if $FONT is
    selected for drawing an M-text, that font layout table is used to
    generate a glyph code sequence from a character sequence.

    @return
    If the operation was successful, mfontset_modify_entry () returns 0.
    Otherwise it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief フォントセットの内容を変更する.

    関数 mfontset_modify_entry () は、$LANGUAGE と $SCRIPT の組み合わせ、または
    $CHARSET に対して $FONT のコピーを使うように、フォントセット $FONTSET を設定する。

    フォントセット中の各フォントは、特定のスクリプトと言語のペア、特定の文字セット、シンボル
    @c Mnil のいずれかと関連付けられている。同じものと関連付けられたフォントはグループを構成する。

    $SCRIPT は @c Mnil であるか、スクリプトを特定するシンボルである。
    シンボルである場合には、$LANGUAGE は言語を特定するシンボルか @c
    Mnil であり、$FONT はthe $SCRIPT / $LANGUAGE ペアに関連付けられる。

    $CHARSET は @c Mnil であるか、文字セットオブジェクトを表すシンボルである。
    シンボルである場合には $FONT はその文字セットと関連付けられる。

    $SCRIPT と $CHARSET の双方が @c Mnil でない場合には $FONT 
    のコピーが２つ作られ、それぞれ $SCRIPT / $LANGUAGE 
    ペアと文字セットに関連付けられる。

    $SCRIPT と $CHARSET の双方が @c Mnil ならば、 $FONT は @c Mnil 
    と関連付けられる。この種のフォントは @e fallback @e font と呼ばれる。

    引数 $HOW は $FONT の優先度を指定する。$HOW が正ならば、$FONT 
    は同じものと関連付けられたグループ中で最高の優先度を持つ。$HOW 
    が負ならば、最低の優先度を持つ。$HOW が 0 ならば、$FONT 
    は関連付けられたものに対する唯一の利用可能なフォントとなり、他のフォントはグループから取り除かれる。

    $LAYOUTER_NAME は @c Mnil であるか、@ref flt 
    （フォントレイアウトテーブル）を示すシンボルである。シンボルであれば、$FONT を用いて
    M-text を表示する際には、そのフォントレイアウトテーブルを使って文字列からグリフコード列を生成する。

    @return 
    処理が成功したとき、mfontset_modify_entry () は 0 を返す。
    失敗したときは -1 を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_SYMBOL  */

int
mfontset_modify_entry (MFontset *fontset,
		       MSymbol script, MSymbol language, MSymbol charset,
		       MFont *spec, MSymbol layouter_name,
		       int how)
{
  MPlist *per_lang, *plist[3];
  MFont *font = NULL;
  int i;

  if (fontset->mdb)
    load_fontset_contents (fontset);

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
      font = mfont_copy (spec);
      font->type = MFONT_TYPE_SPEC;
      if (how == 1)
	mplist_push (plist[i], layouter_name, font);
      else if (how == -1)
	mplist_add (plist[i], layouter_name, font);
      else
	{
	  MPlist *pl;

	  MPLIST_DO (pl, plist[i])
	    free (MPLIST_VAL (pl));
	  mplist_set (plist[i], Mnil, NULL);
	  mplist_add (plist[i], layouter_name, font);
	}
    }

  fontset->tick++;
  return 0;
}

/*=*/

/***en
    @brief Lookup a fontset.

    The mfontset_lookup () function lookups $FONTSET and returns a
    plist that describes the contents of $FONTSET corresponding to the
    specified script, language, and charset.

    If $SCRIPT is @c Mt, keys of the returned plist are script name
    symbols for which some fonts are specified and values are NULL.

    If $SCRIPT is a script name symbol, the returned plist is decided
    by $LANGUAGE.
    
    @li If $LANGUAGE is @c Mt, keys of the plist are language name
    symbols for which some fonts are specified and values are NULL.  A
    key may be @c Mt which means some fallback fonts are specified for
    the script.

    @li If $LANGUAGE is a language name symbol, the plist is a @c
    FONT-GROUP for the specified script and language.  @c FONT-GROUP
    is a plist whose keys are FLT (FontLayoutTable) name symbols (@c
    Mt if no FLT is associated with the font) and values are pointers
    to #MFont.

    @li If $LANGUAGE is @c Mnil, the plist is fallback @c FONT-GROUP
    for the script. 

    If $SCRIPT is @c Mnil, the returned plist is decided as below.

    @li If $CHARSET is @c Mt, keys of the returned plist are charset name
    symbols for which some fonts are specified and values are NULL.

    @li If $CHARSET is a charset name symbol, the plist is a @c FONT-GROUP for
    the charset.

    @li If $CHARSET is @c Mnil, the plist is a fallback @c FONT-GROUP.

    @return
    It returns a plist describing the contents of a fontset.  The
    plist should be freed by m17n_object_unref ().  */
/***ja
    @brief フォントセットを検索する.

    関数 mfontset_lookup () は $FONTSET を検索し、$FONTSET 
    の内容のうち指定したスクリプト、言語、文字セットに対応する部分を表す
    plist を返す。

    $SCRIPT が @c Mt ならば、返す plist 
    のキーはフォントが指定されているスクリプト名のシンボルであり、値は
    NULL である。

    $SCRIPT がスクリプト名のシンボルであれば、返す 
    plist は $LANGUAGEによって定まる。
    
    @li $LANGUAGE が @c Mt ならば、plist 
    のキーはフォントが指定されている言語名のシンボルであり、値は
    NULL である。キーは @c Mt
    であることもあり、その場合そのスクリプトにフォールバックフォントがあることを意味する。

    @li $LANGUAGE が言語名のシンボルならば、plist は指定のスクリプトと言語に対する
    @c FONT-GROUP である。@c FONT-GROUP とは、キーが FLT
    (FontLayoutTable) 名のシンボルであり、値が #MFont 
    へのポインタであるような plist である。ただしフォントに FLT 
    が対応付けられていない時には、キーは @c Mt になる。

    @li $LANGUAGE が @c Mnil ならば、plist はそのスクリプト用のフォールバック
    @c FONT-GROUP である。

    $SCRIPT が @c Mnil ならば、返す plist は以下のように定まる。

    @li $CHARSET が @c Mt ならば、plist 
    のキーはフォントが指定されている文字セット名のシンボルであり、値は
    NULL である。

    @li $CHARSET が文字セット名のシンボルならば、plist はその文字セット用の 
    @c FONT-GROUP である。

    @li $CHARSET が @c Mnil ならば、plist はフォールバック @c FONT-GROUP である。

    @return
    この関数はフォントセットの内容を表す plist を返す。
    plist は m17n_object_unref () で解放されるべきである。  */

MPlist *
mfontset_lookup (MFontset *fontset,
		 MSymbol script, MSymbol language, MSymbol charset)
{
  MPlist *plist = mplist (), *pl, *p;

  if (fontset->mdb)
    load_fontset_contents (fontset);
  if (script == Mt)
    {
      if (! fontset->per_script)
	return plist;
      p = plist;
      MPLIST_DO (pl, fontset->per_script)
	p = mplist_add (p, MPLIST_KEY (pl), NULL);
      return plist;
    }
  if (script != Mnil)
    {
      pl = get_per_script (fontset, script);
      if (MPLIST_TAIL_P (pl))
	return plist;
      if (language == Mt)
	{
	  p = plist;
	  MPLIST_DO (pl, pl)
	    p = mplist_add (p, MPLIST_KEY (pl), NULL);
	  return plist;
	}
      if (language == Mnil)
	language = Mt;
      pl = mplist_get (pl, language);
    }
  else if (charset != Mnil)
    {
      if (! fontset->per_charset)
	return plist;
      if (charset == Mt)
	{
	  p = plist;
	  MPLIST_DO (pl, fontset->per_charset)
	    p = mplist_add (p, MPLIST_KEY (pl), NULL);
	  return plist;
	}
      pl = mplist_get (fontset->per_charset, charset);
    }
  else
    pl = fontset->fallback;
  if (! pl)
    return plist;
  return mplist_copy (pl);
}


/*** @} */

/*** @addtogroup m17nDebug */
/*=*/
/*** @{  */

/***en
    @brief Dump a fontset.

    The mdebug_dump_fontset () function prints fontset $FONTSET in a human readable
    way to the stderr.  $INDENT specifies how many columns to indent
    the lines but the first one.

    @return
    This function returns $FONTSET.  */
/***ja
    @brief フォントセットをダンプする.

    関数 mdebug_dump_face () はフォントセット $FONTSET を stderr 
    に人間に可読な形で印刷する。 $INDENT は２行目以降のインデントを指定する。

    @return
    この関数は $FONTSET を返す。  */

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
		fprintf (stderr, "\n      %s(0x%X %s ", prefix,
			 (unsigned) MPLIST_VAL (p),
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
