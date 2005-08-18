/* draw.c -- drawing module.
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
    @addtogroup m17nDraw
    @brief Drawing M-texts on a window.

    The m17n GUI API provides functions to draw M-texts.

    The fonts used for drawing are selected automatically based on the
    fontset and the properties of a face.  A face also specifies the
    appearance of M-texts, i.e. font size, color, underline, etc.

    The drawing format of M-texts can be controlled in a variety of
    ways, which provides powerful 2-dimensional layout
    facility.  */

/***ja
    @addtogroup m17nDraw
    @brief M-text をウィンドウに描画する.

    m17n-gui API には、M-text を表示するための関数が用意されている。

    表示に用いられるフォントは、フォントセットと face 
    のプロパティに基づいて自動的に決定される。また、フォントのサイズや色や下線などの見栄えも
    face によって決まる。

    M-text の描画フォーマットは多様な方法で制御できるので、強力な二次元レイアウト機能が実現できる。
    */

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>

#include "config.h"
#include "m17n-gui.h"
#include "m17n-misc.h"
#include "internal.h"
#include "symbol.h"
#include "mtext.h"
#include "textprop.h"
#include "internal-gui.h"
#include "face.h"
#include "font.h"

#ifdef HAVE_FRIBIDI
#include <fribidi/fribidi.h>
#endif /* HAVE_FRIBIDI */

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

static MSymbol M_glyph_string;

/* Special scripts */
static MSymbol Minherited, Mcommon;
/* Special categories */
static MSymbol McatCc, McatCf;

static MCharTable *linebreak_table;
static MSymbol M_break_at_space, M_break_at_word, M_break_at_any;
static MSymbol M_kinsoku_bol, M_kinsoku_eol;


/* Glyph-string composer.  */

static MSymbol MbidiR;
static MSymbol MbidiAL;
static MSymbol MbidiRLE;
static MSymbol MbidiRLO;
static MSymbol MbidiBN;
static MSymbol MbidiS;

static void
visual_order (MGlyphString *gstring)
{
  int len = gstring->used - 2;
  MGlyph *glyphs;
  int bidi_sensitive = gstring->control.orientation_reversed;
  MGlyph *g;
  int i;
#ifdef HAVE_FRIBIDI
  FriBidiCharType base = bidi_sensitive ? FRIBIDI_TYPE_RTL : FRIBIDI_TYPE_LTR;
  FriBidiChar *logical = alloca (sizeof (FriBidiChar) * len);
  FriBidiChar *visual;
  FriBidiStrIndex *indices;
  FriBidiLevel *levels;
#else  /* not HAVE_FRIBIDI */
  int *logical = alloca (sizeof (int) * len);
  int *indices;
  char *levels = alloca (len);
#endif /* not HAVE_FRIBIDI */

  for (g = MGLYPH (1), i = 0; g->type != GLYPH_ANCHOR; g++, i++)
    {
      if (! bidi_sensitive
#ifndef HAVE_FRIBIDI
	  || 1
#endif	/* not HAVE_FRIBIDI */
	  )
	{
	  MSymbol bidi = (MSymbol) mchar_get_prop (g->c, Mbidi_category);

	  if (bidi == MbidiR || bidi == MbidiAL
	      || bidi == MbidiRLE || bidi == MbidiRLO)
	    {
	      bidi_sensitive = 1;
#ifndef HAVE_FRIBIDI
	      levels[i] = 1;
#endif	/* not HAVE_FRIBIDI */
	    }
	}
      logical[i] = g->c;
    }

  if (! bidi_sensitive)
    return;

  glyphs = alloca (sizeof (MGlyph) * len);
  memcpy (glyphs, gstring->glyphs + 1, sizeof (MGlyph) * len);
#ifdef HAVE_FRIBIDI
  visual = alloca (sizeof (FriBidiChar) * (len + 1));
  indices = alloca (sizeof (FriBidiStrIndex) * (len + 1));
  levels = alloca (sizeof (FriBidiLevel) * (len + 1));

  fribidi_log2vis (logical, len, &base, visual, indices, NULL, levels);
#else  /* not HAVE_FRIBIDI */
  indices = alloca (sizeof (int) * len);
  for (i = 0; i < len; i++)
    {
      if (levels[i])
	{
	  int j, k;

	  for (j = i + 1; j < len && levels[j]; j++);
	  for (k = j--; i < k; i++, j--)
	    indices[i] = j;
	  i--;
	}
      else
	indices[i] = i;
    }
#endif /* not HAVE_FRIBIDI */

  for (i = 0; i < len;)
    {
      /* Index into gstring->glyphs plus 1 for GLYPHS[i].  */
      int j = indices[i];
      /* Length of grapheme-cluster */
      int seglen;

      g = glyphs + i;
#ifdef HAVE_FRIBIDI
      if (visual[j] != logical[i])
	{
	  /* Mirrored.  */
	  g->c = visual[j];
	  if (g->rface->rfont)
	    g->code = mfont__encode_char (NULL, (MFont *) g->rface->rfont,
					  NULL, g->c);
	}
#endif /* HAVE_FRIBIDI */
      g->bidi_level = levels[i];
      for (seglen = 1, g++;
	   i + seglen < len && (glyphs[i].pos == glyphs[i + seglen].pos
				|| glyphs[i + seglen].combining_code);
	   seglen++, g++)
	{
	  g->bidi_level = levels[i];
	  if (indices[i + seglen] < j)
	    j = indices[i + seglen];
	}
      memcpy (MGLYPH (j + 1), glyphs + i, sizeof (MGlyph) * seglen);
      i += seglen;
    }
}

static void
reorder_combining_chars (MGlyphString *gstring, int from, int to)
{
  MGlyph *g, *gbeg = MGLYPH (from + 1), *gend = MGLYPH (to), temp;
  int reordered = 1;
  
  while (reordered)
    {
      reordered = 0;
      for (g = gbeg; g != gend; g++)
	if (COMBINING_CODE_CLASS (g->combining_code) > 0
	    && (COMBINING_CODE_CLASS (g[-1].combining_code)
		> COMBINING_CODE_CLASS (g->combining_code)))
	  {
	    reordered = 1;
	    temp = *g;
	    *g = g[-1];
	    g[-1] = temp;
	  }
    }
}


/** Scan M-text MT from FROM to TO, and compose glyphs in GSTRING for
    displaying them on FRAME.

    This function fills these members:
      pos, to, c, code, rface, bidi_level, categories, type, combining_code
    The other members are filled by layout_glyph_string.  */

static void
compose_glyph_string (MFrame *frame, MText *mt, int from, int to,
		      MGlyphString *gstring)
{
  MRealizedFace *default_rface = frame->rface;
  int stop, face_change, language_change, charset_change, font_change;
  MGlyph g_tmp, *g, *last_g;
  int pos;
  MSymbol language = Mnil, script = Mnil, charset = Mnil;
  MSymbol non_latin_script = Mnil;
  MRealizedFace *rface = default_rface;
  MRealizedFont *rfont;
  int size = gstring->control.fixed_width;
  int i;

  MLIST_RESET (gstring);
  gstring->from = from;

  /* At first generate glyphs with <pos>, <to>, <c>, <type>,
     <category> and <rface> members.*/
  INIT_GLYPH (g_tmp);

  /** Put anchor glyphs at the head and tail.  */
  g_tmp.type = GLYPH_ANCHOR;
  g_tmp.pos = g_tmp.to = from;
  APPEND_GLYPH (gstring, g_tmp);
  stop = face_change = font_change = pos = from;
  while (1)
    {
      int c;
      MSymbol category;

      if (pos == stop)
	{
	  if (pos == to)
	    break;
	  if (pos < mtext_nchars (mt))
	    {
	      MFont *font = rface->font;
	      MFace *faces[64];
	      int num;

	      if (pos == font_change)
		{
		  font = mtext_get_prop (mt, pos, Mfont);
		  mtext_prop_range (mt, Mfont, pos, NULL, &font_change, 0);
		  if (font_change == mtext_nchars (mt))
		    font_change++;
		}
	      if (pos == face_change)
		{
		  num = mtext_get_prop_values (mt, pos, Mface,
					       (void **) faces, 64);
		  mtext_prop_range (mt, Mface, pos, NULL, &face_change, 1);
		  if (face_change == mtext_nchars (mt))
		    face_change++;
		}
	      else
		{
		  faces[0] = &rface->face;
		  num = 1;
		}
	      rface = mface__realize (frame, faces, num, size, font);
	    }
	  else
	    rface = default_rface;
	  stop = to;
	  if (stop > font_change)
	    stop = font_change;		
	  if (stop > face_change)
	    stop = face_change;		
	}

      if (pos < mtext_nchars (mt))
	c = mtext_ref_char (mt, pos);
      else
	c = '\n';
      g_tmp.type
	= (c == ' ' || c == '\n' || c == '\t') ? GLYPH_SPACE : GLYPH_CHAR;
      g_tmp.c = c;
      g_tmp.pos = pos++;
      g_tmp.to = pos;
      g_tmp.rface = rface;
      category = mchar_get_prop (c, Mcategory);
      if (category == McatCf)
	g_tmp.category = GLYPH_CATEGORY_FORMATTER;
      else if (category != Mnil && MSYMBOL_NAME (category)[0] == 'M')
	g_tmp.category = GLYPH_CATEGORY_MODIFIER;
      else
	g_tmp.category = GLYPH_CATEGORY_NORMAL;
      
      if ((c <= ' ' || c == 127) && g_tmp.type == GLYPH_CHAR)
	{
	  MGlyph ctrl[2];

	  ctrl[0] = ctrl[1] = g_tmp;
	  ctrl[0].c = '^';
	  ctrl[1].c = c < ' ' ? c + 0x40 : '?';
	  APPEND_GLYPH (gstring, ctrl[0]);
	  APPEND_GLYPH (gstring, ctrl[1]);
	}
      else
	APPEND_GLYPH (gstring, g_tmp);
      if (c == '\n' && gstring->control.two_dimensional)
	break;
    }
  /* Append an anchor glyph.  */
  INIT_GLYPH (g_tmp);
  g_tmp.type = GLYPH_ANCHOR;
  g_tmp.pos = g_tmp.to = pos;
  APPEND_GLYPH (gstring, g_tmp);
  gstring->to = pos;

  /* The next loop is to change each <rface> member for non-ASCII
     characters if necessary.  */
  stop = charset_change = language_change = from;
  rfont = default_rface->rfont;
  for (last_g = g = MGLYPH (1); g->type != GLYPH_ANCHOR; g++)
    {
      int c = g->c;
      MSymbol this_script;

      if (c < 0x100)
	/* Short cut for the obvious case.  */
	this_script = Mlatin;
      else
	{
	  this_script = (MSymbol) mchar_get_prop (c, Mscript);
	  if (this_script == Minherited || this_script == Mcommon)
	    this_script = script;
	  if (this_script == Mcommon)
	    this_script = non_latin_script;
	  if (this_script == Mcommon)
	    {
	      /* Search forward for a character that explicitly
		 specifies a non-latin script.  */
	      MSymbol sym;
	      MGlyph *g1;

	      for (g1 = g + 1; g1->type != GLYPH_ANCHOR; g1++)
		if (g1->c >= 0x100
		    && (sym = mchar_get_prop (g1->c, Mscript)) != Mcommon
		    && sym != Minherited)
		  {
		    this_script = sym;
		    break;
		  }
	    }
	}

      pos = g->pos;
      if (pos == stop || script != this_script || g->rface->rfont != rfont)
	{
	  while (last_g < g)
	    last_g = mface__for_chars (script, language, charset,
				       last_g, g, size);
	  script = this_script;
	  if (script != Mnil && script != Mlatin)
	    non_latin_script = script;
	  rfont = g->rface->ascii_rface->rfont;
	  if (pos == stop)
	    {
	      if (pos < mtext_nchars (mt) && pos == language_change)
		{
		  language = (MSymbol) mtext_get_prop (mt, pos, Mlanguage);
		  mtext_prop_range (mt, Mlanguage, pos, NULL,
				    &language_change, 0);
		}
	      if (pos < mtext_nchars (mt) && pos == charset_change)
		{
		  charset = (MSymbol) mtext_get_prop (mt, pos, Mcharset);
		  mtext_prop_range (mt, Mcharset, pos, NULL,
				    &charset_change, 0);
		}
	      stop = to;
	      if (stop > language_change)
		stop = language_change;
	      if (stop > charset_change)
		stop = charset_change;
	    }
	}
    }
  while (last_g < g)
    last_g = mface__for_chars (script, language, charset, last_g, g, size);

  /* The next loop is to run FLT or perform the default combining if
     necessary.  */
  for (i = 1, g = MGLYPH (1); g->type != GLYPH_ANCHOR;)
    {
      MGlyph *this = g;

      if (this->type == GLYPH_CHAR && this->rface->rfont)
	{
	  int start = i++;

	  if (this->rface->layouter != Mnil)
	    {
	      MGlyph *prev;
	      unsigned code;

	      for (prev = MGLYPH (start - 1);
		   (prev->type == GLYPH_CHAR
		    && prev->category == GLYPH_CATEGORY_FORMATTER
		    && (code = mfont__encode_char (NULL,
						   (MFont *) this->rface->rfont,
						   NULL, prev->c)
			!= MCHAR_INVALID_CODE));
		   start--, prev--)
		prev->code = code;

	      for (g++;
		   (g->type == GLYPH_CHAR
		    && g->rface->layouter == this->rface->layouter
		    && (g->rface->rfont == this->rface->rfont
			|| (g->category == GLYPH_CATEGORY_FORMATTER
			    && ((code = mfont__encode_char (NULL,
							    (MFont *) this->rface->rfont,
							    NULL,
							    g->c))
				!= MCHAR_INVALID_CODE))));
		   i++, g++)
		if (g->rface->rfont != this->rface->rfont)
		  {
		    g->rface->rfont = this->rface->rfont;
		    g->code = code;
		  }
	      i = mfont__flt_run (gstring, start, i, this->rface);
	    }
	  else
	    {
	      while (this->type == GLYPH_CHAR
		     && this->c >= 0x100
		     && this->category == GLYPH_CATEGORY_MODIFIER
		     && this->rface->rfont
		     && this->rface->rfont->layouter == Mnil)
		{
		  int class = (int) mchar_get_prop (this->c,
						    Mcombining_class);
		  this->combining_code
		    = MAKE_COMBINING_CODE_BY_CLASS (class);
		  i++, this++;
		}
	      if (start + 1 < i)
		reorder_combining_chars (gstring, start, i);
	      if (this->type == GLYPH_ANCHOR)
		break;
	    }
	  g = MGLYPH (i);
	}
      else
	i++, g++;
    }

  /* At last, reorder glyphs visually if necessary.  */
  if (gstring->control.enable_bidi)
    visual_order (gstring);
}


static int
combining_code_from_class (int class)
{
  int code;

  if (class < 200)
    code = MAKE_COMBINING_CODE (3, 1, 3, 1, 128, 128);
  else if (class == 200)	/* below left attached */
    code = MAKE_COMBINING_CODE (2, 0, 0, 1, 128, 128);
  else if (class == 202)	/* below attached*/
    code = MAKE_COMBINING_CODE (2, 1, 0, 1, 128, 128);
  else if (class == 204)	/* below right attached */
    code = MAKE_COMBINING_CODE (2, 2, 0, 1, 128, 128);
  else if (class == 208)	/* left attached */
    code = MAKE_COMBINING_CODE (3, 0, 3, 2, 128, 128);
  else if (class == 210)	/* right attached */
    code = MAKE_COMBINING_CODE (3, 2, 3, 0, 128, 128);
  else if (class == 212)	/* above left attached */
    code = MAKE_COMBINING_CODE (0, 0, 2, 1, 128, 128);
  else if (class == 214)	/* above attached */
    code = MAKE_COMBINING_CODE (0, 1, 2, 1, 128, 128);
  else if (class == 216)	/* above right attached */
    code = MAKE_COMBINING_CODE (0, 2, 2, 1, 128, 128);
  else if (class == 218)	/* below left */
    code = MAKE_COMBINING_CODE (2, 0, 0, 1, 122, 128);
  else if (class == 220)	/* below */
    code = MAKE_COMBINING_CODE (2, 1, 0, 1, 122, 128);
  else if (class == 222)	/* below right */
    code = MAKE_COMBINING_CODE (2, 2, 0, 1, 122, 128);
  else if (class == 224)	/* left */
    code = MAKE_COMBINING_CODE (3, 0, 3, 2, 128, 122);
  else if (class == 226)	/* right */
    code = MAKE_COMBINING_CODE (3, 2, 3, 0, 128, 133);
  else if (class == 228)	/* above left */
    code = MAKE_COMBINING_CODE (0, 0, 2, 1, 133, 128);
  else if (class == 230)	/* above */
    code = MAKE_COMBINING_CODE (0, 1, 2, 1, 133, 128);
  else if (class == 232)	/* above right */
    code = MAKE_COMBINING_CODE (0, 2, 2, 1, 133, 128);
  else if (class == 233)	/* double below */
    code = MAKE_COMBINING_CODE (2, 2, 0, 2, 122, 128);
  else if (class == 234)	/* double above */
    code = MAKE_COMBINING_CODE (0, 2, 2, 2, 133, 128);
  else if (class == 240)	/* iota subscript */
    code = MAKE_COMBINING_CODE (2, 1, 0, 1, 122, 128);
  else				/* unknown */
    code = MAKE_COMBINING_CODE (3, 1, 3, 1, 128, 128);
  return code;
}


typedef struct {
  int width, lbearing, rbearing;
} MSubTextExtents;

static void
layout_glyphs (MFrame *frame, MGlyphString *gstring, int from, int to,
	       MSubTextExtents *extents)
{
  int g_physical_ascent, g_physical_descent;
  MGlyph *g = MGLYPH (from);
  MGlyph *last_g = MGLYPH (to);
  int i;

  g_physical_ascent = gstring->physical_ascent;
  g_physical_descent = gstring->physical_descent;
  extents->width = extents->lbearing = extents->rbearing = 0;

  for (i = from; i < to;)
    {
      if ( MGLYPH (i)->otf_encoded)
	i++;
      else
	{
	  int j = i++;

	  while (i < to && ! MGLYPH (i)->otf_encoded) i++;
	  mfont__get_metric (gstring, j, i);
	}
    }

  g = MGLYPH (from);
  while (g < last_g)
    {
      MGlyph *base = g++;
      MRealizedFont *rfont = base->rface->rfont;
      int size = rfont->spec.size;
      int width, lbearing, rbearing;

      if (g == last_g || ! g->combining_code)
	{
	  /* No combining.  */
	  if (base->width == 0 && ! base->left_padding && ! base->right_padding
	      && GLYPH_INDEX (base) > from)
	    {
	      MGlyph *prev = base - 1; 

	      if (base->pos < prev->pos)
		prev->pos = base->pos;
	      else
		base->pos = prev->pos;
	      if (base->to > prev->to)
		prev->to = base->to;
	      else
		base->to = prev->to;
	    }

	  if (base->left_padding && base->lbearing < 0)
	    {
	      base->xoff = - base->lbearing;
	      base->width += base->xoff;
	      base->rbearing += base->xoff;
	      base->lbearing = 0;
	    }
	  if (base->right_padding && base->rbearing > base->width)
	    {
	      base->width = base->rbearing;
	    }
	  lbearing = (base->xoff + base->lbearing < 0
		      ? base->xoff + base->lbearing : 0);
	  rbearing = base->xoff + base->rbearing;
	}
      else
	{
	  /* With combining glyphs.  */
	  int left = -base->width;
	  int right = 0;
	  int top = - base->ascent;
	  int bottom = base->descent;
	  int height = bottom - top;
	  int begin = base->pos;
	  int end = base->to;
	  int i;

	  width = base->width;
	  lbearing = (base->lbearing < 0 ? base->lbearing : 0);
	  rbearing = base->rbearing;

	  while (g != last_g && g->combining_code)
	    {
	      int combining_code = g->combining_code;

	      if (begin > g->pos)
		begin = g->pos;
	      else if (end < g->to)
		end = g->to;
		
	      if (! COMBINING_PRECOMPUTED_P (combining_code))
		{
		  int base_x, base_y, add_x, add_y, off_x, off_y;

		  if (COMBINING_BY_CLASS_P (combining_code))
		    g->combining_code = combining_code
		      = combining_code_from_class (COMBINING_CODE_CLASS
						   (combining_code));

		  rfont = g->rface->rfont;
		  size = rfont->spec.size;
		  off_x = (size * (COMBINING_CODE_OFF_X (combining_code) - 128)
			   / 1000);
		  off_y = (size * (COMBINING_CODE_OFF_Y (combining_code) - 128)
			   / 1000);
		  base_x = COMBINING_CODE_BASE_X (combining_code);
		  base_y = COMBINING_CODE_BASE_Y (combining_code);
		  add_x = COMBINING_CODE_ADD_X (combining_code);
		  add_y = COMBINING_CODE_ADD_Y (combining_code);

		  g->xoff = left + (width * base_x - g->width * add_x) / 2 + off_x;
		  if (g->xoff < left)
		    left = g->xoff;
		  if (g->xoff + g->width > right)
		    right = g->xoff + g->width;
		  width = right - left;

		  if (base_y < 3)
		    g->yoff = top + height * base_y / 2;
		  else
		    g->yoff = 0;
		  if (add_y < 3)
		    g->yoff -= (g->ascent + g->descent) * add_y / 2 - g->ascent;
		  g->yoff -= off_y;
		}

	      if (g->xoff + g->lbearing < left + lbearing)
		lbearing = g->xoff + g->lbearing - left;
	      if (g->xoff + g->rbearing > left + rbearing)
		rbearing = g->xoff + g->rbearing - left;
	      if (g->yoff - g->ascent < top)
		top = g->yoff - g->ascent;
	      if (g->yoff + g->descent > bottom)
		bottom = g->yoff + g->descent;
	      height = bottom - top;

	      g->width = 0;
	      g++;
	    }

	  base->ascent = - top;
	  base->descent = bottom;
	  base->lbearing = lbearing;
	  base->rbearing = rbearing;
	  if (left < - base->width)
	    {
	      base->xoff = - base->width - left;
	      base->width += base->xoff;
	      base->rbearing += base->xoff;
	      base->lbearing += base->xoff;
	    }
	  if (right > 0)
	    {
	      base->width += right;
	      base->rbearing += right;
	      base->right_padding = 1;
	      for (i = 1; base + i != g; i++)
		base[i].xoff -= right;
	    }

	  for (i = 0; base + i != g; i++)
	    {
	      base[i].pos = begin;
	      base[i].to = end;
	    }
	}

      g_physical_ascent = MAX (g_physical_ascent, base->ascent);
      g_physical_descent = MAX (g_physical_descent, base->descent);
      extents->lbearing = MIN (extents->lbearing, extents->width + lbearing);
      extents->rbearing = MAX (extents->rbearing, extents->width + rbearing);
      extents->width += base->width;
    }

  gstring->physical_ascent = g_physical_ascent;
  gstring->physical_descent = g_physical_descent;
}


/** Decide the layout of glyphs in GSTRING.  Space glyphs are handled
    by this function directly.  Character glyphs are handled by
    layouter functions registered in font drivers.

    This function fill-in all the remaining members of glyphs.  */

static void
layout_glyph_string (MFrame *frame, MGlyphString *gstring)
{
  /* Default width of TAB.  */
  int tab_width = frame->space_width * (gstring->control.tab_width
					? gstring->control.tab_width : 8);
  int tab_found = 0;
  MGlyph *g;
  MGlyph pad;
  MDrawControl *control = &(gstring->control);
  int width;
  MFaceBoxProp *box;
  int box_line_height = 0;
  int ignore_formatting_char = control->ignore_formatting_char;

  gstring->ascent = gstring->descent = 0;
  gstring->physical_ascent = gstring->physical_descent = 0;
  gstring->width = gstring->lbearing = gstring->rbearing = 0;

  g = MGLYPH (1);
  box = NULL;
  while (g->type != GLYPH_ANCHOR)
    {
      if (box != g->rface->box)
	{
	  int gidx = GLYPH_INDEX (g);

	  if (box)
	    {
	      /* Insert the right side of the box.  That glyph belongs
		 to the previous grapheme cluster.  */
	      MGlyph box_glyph = g[-1];

	      box_glyph.type = GLYPH_BOX;
	      box_glyph.width
		= (control->fixed_width
		   ? frame->space_width
		   : box->inner_hmargin + box->width + box->outer_hmargin);
	      box_glyph.lbearing = 0;
	      box_glyph.rbearing = box_glyph.width;
	      box_glyph.xoff = 0;
	      box_glyph.right_padding = 1;
	      gstring->width += box_glyph.width;
	      gstring->rbearing += box_glyph.width;
	      INSERT_GLYPH (gstring, gidx, box_glyph);
	      gidx++;
	      g = MGLYPH (gidx);
	    }
	  box = g->rface->box;
	  if (box)
	    {
	      /* Insert the left side of the box.  That glyph belongs
		 to the following grapheme cluster.  */
	      MGlyph box_glyph = *g;
	      int box_height = (box->width
				+ box->inner_vmargin + box->outer_vmargin);

	      if (box_line_height < box_height)
		box_line_height = box_height;
	      box_glyph.type = GLYPH_BOX;
	      box_glyph.width
		= (control->fixed_width
		   ? frame->space_width
		   : box->inner_hmargin + box->width + box->outer_hmargin);
	      box_glyph.lbearing = 0;
	      box_glyph.rbearing = box_glyph.width;
	      box_glyph.xoff = 0;
	      box_glyph.left_padding = 1;
	      gstring->width += box_glyph.width;
	      gstring->rbearing += box_glyph.width;
	      INSERT_GLYPH (gstring, gidx, box_glyph);
	      gidx++;
	      g = MGLYPH (gidx);
	    }
	}

      if (g->category == GLYPH_CATEGORY_FORMATTER && ignore_formatting_char)
	g->type = GLYPH_SPACE;

      if (g->type == GLYPH_CHAR)
	{
	  MRealizedFace *rface = g->rface;
	  MRealizedFont *rfont = rface->rfont;
	  MGlyph *fromg = g;
	  int from = GLYPH_INDEX (g);

	  for (g++; g->type == GLYPH_CHAR; g++)
	    if (! rfont != ! g->rface->rfont
		|| box != g->rface->box
		|| ((fromg->code == MCHAR_INVALID_CODE)
		    != (g->code == MCHAR_INVALID_CODE))
		|| (g->category == GLYPH_CATEGORY_FORMATTER
		    && ignore_formatting_char))
	      break;
	  if (rfont && fromg->code != MCHAR_INVALID_CODE)
	    {
	      int extra_width;
	      int to = GLYPH_INDEX (g);
	      MSubTextExtents extents;

	      layout_glyphs (frame, gstring, from, to, &extents);
	      extra_width = - extents.lbearing;
	      if (extra_width > 0
		  && ! control->disable_overlapping_adjustment
		  && (! control->orientation_reversed
		      ? ((to > 1 || control->align_head)
			 && g->type != GLYPH_ANCHOR)
		      : (((g->type && GLYPH_ANCHOR) || control->align_head)
			 && to > 1)))
		{
		  g = MGLYPH (from);
		  pad = *g;
		  pad.type = GLYPH_PAD;
		  pad.xoff = 0;
		  pad.lbearing = 0;
		  pad.width = pad.rbearing = extra_width;
		  pad.left_padding = 1;
		  INSERT_GLYPH (gstring, from, pad);
		  to++;
		  extents.lbearing = 0;
		  extents.width += extra_width;
		  extents.rbearing += extra_width;

		  g = MGLYPH (from - 1);
		  if (g->type == GLYPH_SPACE)
		    {
		      /* The pad just inserted is absorbed (maybe
			 partially) by the previous space while
			 keeping at least some space width.  For the
			 moment, we use the arbitrary width 2-pixel.
			 Perhaps, it should be decided by the current
			 face, or a default value of the current
			 frame, which is, however, not yet
			 implemented.  */
		      if (extra_width + 2 < g->width)
			{
			  g->width -= extra_width;
			}
		      else
			{
			  extra_width = g->width - 2;
			  g->width = 2;
			}
		      gstring->width -= extra_width;
		      gstring->rbearing -= extra_width;
		    }
		}

	      g = MGLYPH (to);
	      extra_width = extents.rbearing - extents.width;
	      if (extra_width > 0
		  && ! control->disable_overlapping_adjustment
		  && (GLYPH_INDEX (g) < gstring->used - 1
		      || (control->orientation_reversed && control->align_head)))
		{
		  if (g->type == GLYPH_SPACE && box == g->rface->box)
		    {
		      pad = g[-1];
		      pad.type = GLYPH_PAD;
		      pad.xoff = 0;
		      pad.lbearing = 0;
		      pad.width = pad.rbearing = extra_width;
		      INSERT_GLYPH (gstring, to, pad);
		      to++;
		      g = MGLYPH (to);
		    }
		  else
		    g[-1].width += extra_width;
		  extents.width += extra_width;
		}

	      if (gstring->lbearing > gstring->width + extents.lbearing)
		gstring->lbearing = gstring->width + extents.lbearing;
	      if (gstring->rbearing < gstring->width + extents.rbearing)
		gstring->rbearing = gstring->width + extents.rbearing;
	      gstring->width += extents.width;
	      if (gstring->ascent < rface->ascent)
		gstring->ascent = rface->ascent;
	      if (gstring->descent < rface->descent)
		gstring->descent = rface->descent;
	    }
	  else
	    {
	      for (; fromg < g; fromg++)
		{
		  if ((fromg->c >= 0x200B && fromg->c <= 0x200F)
		      || (fromg->c >= 0x202A && fromg->c <= 0x202E))
		    fromg->width = fromg->rbearing = 1;
		  else
		    fromg->width = fromg->rbearing = rface->space_width;
		  fromg->xoff = fromg->lbearing = 0;
		  fromg->ascent = fromg->descent = 0;
		  gstring->width += fromg->width;
		  gstring->rbearing += fromg->width;
		}
	      if (gstring->ascent < frame->rface->ascent)
		gstring->ascent = frame->rface->ascent;
	      if (gstring->descent < frame->descent)
		gstring->descent = frame->rface->descent;
	    }
	}
      else if (g->type == GLYPH_SPACE)
	{
	  if (g->c == ' ')
	    g->width = g->rface->space_width;
	  else if (g->c == '\n')
	    {
	      g->width = control->cursor_width;
	      if (g->width)
		{
		  if (control->cursor_bidi)
		    g->width = 3;
		  else if (g->width < 0)
		    g->width = g->rface->space_width;
		}
	    }
	  else if (g->c == '\t')
	    {
	      g->width = tab_width - ((gstring->indent + gstring->width)
				      % tab_width);
	      tab_found = 1;
	    }
	  else
	    g->width = 1;
	  if (g[-1].type == GLYPH_PAD)
	    {
	      /* This space glyph absorbs (maybe partially) the
		 previous padding glyph.  */
	      g->width -= g[-1].width;
	      if (g->width < 1)
		/* But, keep at least some space width.  For the
		   moment, we use the arbitrary width 2-pixel.  */
		g->width = 2;
	    }
	  g->rbearing = g->width;
	  gstring->width += g->width;
	  gstring->rbearing += g->width;
	  if (g->rface->rfont)
	    {
	      if (gstring->ascent < g->rface->ascent)
		gstring->ascent = g->rface->ascent;
	      if (gstring->descent < g->rface->descent)
		gstring->descent = g->rface->descent;
	    }
	  g++;
	}
      else
	{
	  gstring->width += g->width;
	  gstring->rbearing += g->width;
	  g++;
	}
    }

  if (box)
    {
      /* Insert the right side of the box.  */
      int gidx = GLYPH_INDEX (g);
      MGlyph box_glyph = g[-1];

      box_glyph.type = GLYPH_BOX;
      box_glyph.width
	= (control->fixed_width
	   ? frame->space_width
	   : box->inner_hmargin + box->width + box->outer_hmargin);
      box_glyph.lbearing = 0;
      box_glyph.rbearing = box_glyph.width;
      box_glyph.xoff = 0;
      box_glyph.right_padding = 1;
      gstring->width += box_glyph.width;
      gstring->rbearing += box_glyph.width;
      INSERT_GLYPH (gstring, gidx, box_glyph);
    }

  gstring->text_ascent = gstring->ascent;
  gstring->text_descent = gstring->descent;
  if (gstring->text_ascent < gstring->physical_ascent)
    gstring->text_ascent = gstring->physical_ascent;
  if (gstring->text_descent < gstring->physical_descent)
    gstring->text_descent = gstring->physical_descent;
  gstring->line_ascent = gstring->text_ascent;
  gstring->line_descent = gstring->text_descent;
  if (box_line_height > 0)
    {
      gstring->line_ascent += box_line_height;
      gstring->physical_ascent = gstring->line_ascent;
      gstring->line_descent += box_line_height;
      gstring->physical_descent = gstring->line_descent;
    }

  if (gstring->line_ascent < control->min_line_ascent)
    gstring->line_ascent = control->min_line_ascent;
  else if (control->max_line_ascent
	   && control->max_line_ascent > control->min_line_ascent
	   && gstring->line_ascent > control->max_line_ascent)
    gstring->line_ascent = control->max_line_ascent;

  if (gstring->line_descent < control->min_line_descent)
    gstring->line_descent = control->min_line_descent;
  else if (control->max_line_descent
	   && control->max_line_descent > control->min_line_descent
	   && gstring->line_descent > control->max_line_descent)
    gstring->line_descent = control->max_line_descent;
  gstring->height = gstring->line_ascent + gstring->line_descent;

  if (control->orientation_reversed
      && tab_found)
    {
      /* We must adjust TAB width for RTL orientation.  */
      width = gstring->indent;

      for (g = MGLYPH (gstring->used - 2); g->type != GLYPH_ANCHOR; g--)
	{
	  if (g->type == GLYPH_CHAR && g->c == '\t')
	    {
	      int this_width = tab_width - (width % tab_width);

	      if (g[1].type == GLYPH_PAD)
		this_width -= g[1].width;
	      if (g[-1].type == GLYPH_PAD)
		this_width -= g[-1].width;		
	      if (this_width < 2)
		this_width = 2;
	      gstring->width += this_width - g->width;
	      gstring->rbearing += this_width - g->width;
	      g->width = this_width;
	      width += this_width;
	    }
	  else
	    width += g->width;
	}
    }
}


static MDrawRegion
draw_background (MFrame *frame, MDrawWindow win, int x, int y,
		 MGlyphString *gstring, int from, int to,
		 int *from_idx, int *to_idx, int *to_x)
{
  MGlyph *g = MGLYPH (1);
  MDrawRegion region = (MDrawRegion) NULL;
  MDrawControl *control = &gstring->control;
  int cursor_pos = -1;
  int prev_pos = -1;
  int cursor_bidi = control->cursor_bidi;

  if (control->with_cursor && control->cursor_width)
    {
      if (gstring->from <= control->cursor_pos
	  && gstring->to > control->cursor_pos)
	cursor_pos = control->cursor_pos;
      if (cursor_pos >= 0
	  && cursor_bidi
	  && gstring->from <= control->cursor_pos - 1
	  && gstring->to > control->cursor_pos - 1)
	prev_pos = control->cursor_pos - 1;
    }

  *from_idx = *to_idx = 0;
  while (g->type != GLYPH_ANCHOR)
    {
      if (g->pos >= from && g->pos < to)
	{
	  MGlyph *fromg = g, *cursor = NULL;
	  MRealizedFace *rface = g->rface;
	  int width = 0;
	  int cursor_width = 0;
	  int cursor_x;

	  if (! *from_idx)
	    *from_idx = GLYPH_INDEX (g);
	  while (g->pos >= from && g->pos < to
		 && g->rface == rface)
	    {
	      g->enabled = 1;
	      if (g->type != GLYPH_BOX
		  && g->pos <= cursor_pos && g->to > cursor_pos)
		{
		  if (! cursor)
		    cursor = g, cursor_x = x + width;
		  cursor_width += g->width;
		}
	      width += g++->width;
	    }
	  if (width > 0
	      && (control->as_image
		  || rface->face.property[MFACE_VIDEOMODE] == Mreverse))
	    {
	      int this_x = x, this_width = width;

	      if (fromg->type == GLYPH_BOX)
		this_x += fromg->width, this_width -= fromg->width;
	      if (g[-1].type == GLYPH_BOX)
		this_width -= g[-1].width;
	      (frame->driver->fill_space)
		(frame, win, rface, 0,
		 this_x, y - gstring->text_ascent, this_width,
		 gstring->text_ascent + gstring->text_descent,
		 control->clip_region);
	    }
	  if (cursor)
	    {
	      MDrawMetric rect;
	    
	      rect.x = cursor_x;
	      rect.y = y - gstring->text_ascent;
	      rect.height = gstring->text_ascent + gstring->text_descent;
	      if (! cursor_bidi)
		{
		  rect.width = ((control->cursor_width > 0
				 && control->cursor_width < cursor_width)
				? control->cursor_width : cursor_width);
		}
	      else
		rect.width = 1;
	      if (cursor->bidi_level % 2)
		rect.x += cursor_width - rect.width;
	      (*frame->driver->fill_space)
		(frame, win, rface, 1, rect.x, rect.y, rect.width, rect.height,
		 control->clip_region);
	      if (! region)
		region = (*frame->driver->region_from_rect) (&rect);
	      else
		(*frame->driver->region_add_rect) (region, &rect);
	      if (cursor_bidi)
		{
		  if (cursor->bidi_level % 2)
		    rect.x -= 3;
		  rect.height = 2;
		  rect.width = cursor_width < 4 ? cursor_width : 4;
		  (*frame->driver->fill_space)
		    (frame, win, rface, 1,
		     rect.x, rect.y, rect.width, rect.height,
		     control->clip_region);
		  (*frame->driver->region_add_rect) (region, &rect);
		}
	    }

	  if (prev_pos >= 0)
	    {
	      int temp_width = 0;

	      cursor_width = 0;
	      cursor = NULL;
	      while (fromg < g)
		{
		  if (fromg->type != GLYPH_BOX
		      && fromg->pos <= prev_pos && fromg->to > prev_pos)
		    {
		      if (! cursor)
			cursor = fromg, cursor_x = x + temp_width;
		      cursor_width += fromg->width;
		    }
		  temp_width += fromg++->width;
		}
	      if (cursor)
		{
		  MDrawMetric rect;

		  rect.x = cursor_x;
		  if (! (cursor->bidi_level % 2))
		    rect.x += cursor_width - 1;
		  rect.y = y - gstring->text_ascent;
		  rect.height = gstring->text_ascent + gstring->text_descent;
		  rect.width = 1;
		  (*frame->driver->fill_space)
		    (frame, win, rface, 1,
		     rect.x, rect.y, rect.width, rect.height,
		     control->clip_region);
		  if (! region)
		    region = (*frame->driver->region_from_rect) (&rect);
		  else
		    (*frame->driver->region_add_rect) (region, &rect);
		  rect.y += rect.height - 2;
		  rect.height = 2;
		  rect.width = cursor_width < 4 ? cursor_width : 4;
		  if (! (cursor->bidi_level % 2))
		    rect.x -= rect.width - 1;
		  (*frame->driver->fill_space) (frame, win, rface, 1,
				    rect.x, rect.y, rect.width, rect.height,
				    control->clip_region);
		  (*frame->driver->region_add_rect) (region, &rect);
		}
	    }
	  x += width;
	  *to_idx = GLYPH_INDEX (g);
	  *to_x = x;
	}
      else
	g++->enabled = 0;
    }
  return region;
}

static void
render_glyphs (MFrame *frame, MDrawWindow win, int x, int y, int width,
	       MGlyphString *gstring, int from_idx, int to_idx,
	       int reverse, MDrawRegion region)
{
  MGlyph *g = MGLYPH (from_idx), *gend = MGLYPH (to_idx);

  if (region)
    {
      MDrawMetric rect;

      (*frame->driver->region_to_rect) (region, &rect);
      if (rect.x > x)
	{
	  while (g != gend && x + g->rbearing <= rect.x)
	    {
	      x += g->width;
	      width -= g++->width;
	      while (! g->enabled && g != gend)
		g++;
	    }
	}
      rect.x += rect.width;
      if (rect.x < x + width)
	{
	  while (g != gend
		 && (x + width - gend[-1].width + gend[-1].lbearing >= rect.x))
	    {
	      width -= (--gend)->width;
	      while (! gend->enabled && g != gend)
		gend--;
	    }
	  if (g != gend)
	    while (gend[-1].to == gend->to) gend++;
	}
    }

  while (g != gend)
    {
      if (g->enabled)
	{
	  MRealizedFace *rface = g->rface;
	  int width = g->width;
	  MGlyph *from_g = g++;

	  /* Handle the glyphs of the same type/face at once.  */
	  while (g != gend
		 && g->type == from_g->type
		 && g->rface == rface
		 && ((g->code == MCHAR_INVALID_CODE)
		     == (from_g->code == MCHAR_INVALID_CODE))
		 && g->enabled)
	    width += g++->width;

	  if (from_g->type == GLYPH_CHAR)
	    {
	      if (rface->rfont && from_g->code != MCHAR_INVALID_CODE)
		(rface->rfont->driver->render) (win, x, y, gstring, from_g, g,
						reverse, region);
	      else
		(*frame->driver->draw_empty_boxes) (win, x, y, gstring, from_g, g,
					reverse, region);
	    }
	  else if (from_g->type == GLYPH_BOX)
	    {
	      /* Draw the left or right side of a box.  If
		 from_g->lbearing is nonzero, this is the left side,
		 else this is the right side.  */
	      (*frame->driver->draw_box) (frame, win, gstring, from_g, x, y, 0, region);
	    }

	  if (from_g->type != GLYPH_BOX)
	    {
	      if (rface->hline)
		(*frame->driver->draw_hline) (frame, win, gstring, rface, reverse,
				  x, y, width, region);
	      if (rface->box
		  && ! reverse)
		/* Draw the top and bottom side of a box.  */
		(*frame->driver->draw_box) (frame, win, gstring, from_g,
				   x, y, width, region);
	    }
	  x += width;
	}
      else
	g++;
    }
}


static int
find_overlapping_glyphs (MGlyphString *gstring, int *left, int *right,
			 int *from_x, int *to_x)
{
  MGlyph *g;
  int left_idx = *left, right_idx = *right;
  int left_x, right_x, x;

  for (g = MGLYPH (*left) - 1, x = 0; g->type != GLYPH_ANCHOR; g--)
    {
      x -= g->width;
      if (x + g->rbearing > 0)
	{
	  while (g[-1].pos == g->pos && g[-1].type != GLYPH_ANCHOR)
	    x -= (--g)->width;
	  left_idx = GLYPH_INDEX (g);
	  left_x = x;
	}
    }

  for (g = MGLYPH (*right), x = 0; g->type != GLYPH_ANCHOR; g++)
    {
      x += g->width;
      if (x - g->width + g->lbearing < 0)
	{
	  while (g->pos == g[1].pos && g[1].type != GLYPH_ANCHOR)
	    x += (++g)->width;
	  right_idx = GLYPH_INDEX (g) + 1;
	  right_x = x;
	}
    }

  if (*left == left_idx && *right == right_idx)
    return 0;

  if (*left != left_idx)
    {
      for (g = MGLYPH (*left) - 1; GLYPH_INDEX (g) >= left_idx; g--)
	g->enabled = 1;
      *left = left_idx;
      *from_x += left_x;
    }
  if (*right != right_idx)
    {
      for (g = MGLYPH (*right); GLYPH_INDEX (g) < right_idx; g++)
	g->enabled = 1;
      *right = right_idx;
      *to_x += right_x;
    }
  return 1;
}


static int
gstring_width (MGlyphString *gstring, int from, int to, int *rbearing)
{
  MGlyph *g;
  int width;

  if (from <= gstring->from && to >= gstring->to)
    {
      if (rbearing)
	*rbearing = gstring->rbearing;
      return gstring->width;
    }

  if (rbearing)
    *rbearing = 0;
  for (g = MGLYPH (1), width = 0; g->type != GLYPH_ANCHOR; g++)
    if (g->pos >= from && g->pos < to)
      {
	if (rbearing && width + g->rbearing > *rbearing)
	  *rbearing = width + g->rbearing;
	width += g->width;
      }
  return width;
}


static void
render_glyph_string (MFrame *frame, MDrawWindow win, int x, int y,
		     MGlyphString *gstring, int from, int to)
{
  MDrawControl *control = &gstring->control;
  MDrawMetric rect;
  MDrawRegion clip_region, cursor_region;
  int from_idx, to_idx;
  int to_x;

  if (control->orientation_reversed)
    x -= gstring->indent + gstring_width (gstring, from, to, NULL);
  else
    x += gstring->indent;

  /* At first, draw all glyphs without cursor.  */
  cursor_region = draw_background (frame, win, x, y, gstring, from, to,
				   &from_idx, &to_idx, &to_x);

  if (control->partial_update)
    {
      rect.x = x;
      rect.width = to_x - x;
      if (find_overlapping_glyphs (gstring, &from_idx, &to_idx, &x, &to_x))
	{
	  rect.y = y - gstring->line_ascent;
	  rect.height = gstring->height;
	  clip_region = (*frame->driver->region_from_rect) (&rect);
	  if (control->clip_region)
	    (*frame->driver->intersect_region) (clip_region, control->clip_region);
	}
      else
	clip_region = control->clip_region;
    }
  else
    clip_region = control->clip_region;

  render_glyphs (frame, win, x, y, to_x - x, gstring, from_idx, to_idx,
		 0, clip_region);
  if (cursor_region)
    {
      if (clip_region)
	(*frame->driver->intersect_region) (cursor_region, clip_region);
      render_glyphs (frame, win, x, y, to_x - x, gstring, from_idx, to_idx,
		     1, cursor_region);
    }
  if (clip_region != control->clip_region)
    (*frame->driver->free_region) (clip_region);
  if (cursor_region)
    (*frame->driver->free_region) (cursor_region);
  return;
}

static int gstring_num;

static void
free_gstring (void *object)
{
  MGlyphString *gstring = (MGlyphString *) object;

  if (gstring->next)
    free_gstring (gstring->next);
  if (gstring->size > 0)
    free (gstring->glyphs);
  free (gstring);
  gstring_num--;
}


static MGlyphString scratch_gstring;

static MGlyphString *
alloc_gstring (MFrame *frame, MText *mt, int pos, MDrawControl *control,
	       int line, int y)
{
  MGlyphString *gstring;

  if (pos == mt->nchars)
    {
      MGlyph *g;

      gstring = &scratch_gstring;
      if (gstring->size == 0)
	{
	  MGlyph g_tmp;

	  INIT_GLYPH (g_tmp);
	  g_tmp.type = GLYPH_ANCHOR;
	  APPEND_GLYPH (gstring, g_tmp);
	  APPEND_GLYPH (gstring, g_tmp);
	  APPEND_GLYPH (gstring, g_tmp);
	  gstring->glyphs[1].type = GLYPH_SPACE;
	  gstring->glyphs[1].c = '\n';
	  gstring->glyphs[1].code = '\n';
	}
      gstring->from = pos;
      g = MGLYPH (0);
      g->rface = frame->rface;
      g->pos = g->to = pos;
      g++;
      g->rface = frame->rface;
      g->pos = pos++, g->to = pos;
      g++;
      g->rface = frame->rface;
      g->pos = g->to = pos;
      gstring->to = pos;
    }
  else
    {
      M17N_OBJECT (gstring, free_gstring, MERROR_DRAW);
      MLIST_INIT1 (gstring, glyphs, 128);
      gstring_num++;
    }

  gstring->frame = frame;
  gstring->tick = frame->tick;
  gstring->top = gstring;
  gstring->control = *control;
  gstring->indent = gstring->width_limit = 0;
  if (control->format)
    (*control->format) (line, y, &(gstring->indent), &(gstring->width_limit));
  else
    gstring->width_limit = control->max_line_width;
  gstring->anti_alias = control->anti_alias;
  return gstring;
}

/* Truncate the line width of GSTRING to GSTRING->width_limit.  */

static void
truncate_gstring (MFrame *frame, MText *mt, MGlyphString *gstring)
{
  int width;
  int i;
  int *pos_width;
  MGlyph *g;
  int pos;

  /* Setup the array POS_WIDTH so that POS_WIDTH[I - GSTRING->from] is
     a width of glyphs for the character at I of MT.  If I is not a
     beginning of a grapheme cluster, the corresponding element is
     0.  */
  MTABLE_ALLOCA (pos_width, gstring->to - gstring->from, MERROR_DRAW);
  memset (pos_width, 0, sizeof (int) * (gstring->to - gstring->from));
  for (g = MGLYPH (1); g->type != GLYPH_ANCHOR; g++)
    pos_width[g->pos - gstring->from] += g->width;
  for (i = 0, width = 0; i < gstring->to - gstring->from; i++)
    {
      if (pos_width[i] > 0)
	{
	  if (width + pos_width[i] > gstring->width_limit)
	    break;
	}
      width += pos_width[i];
    }

  pos = gstring->from + i;
  if (gstring->control.line_break)
    {
      pos = (*gstring->control.line_break) (mt, gstring->from + i,
					    gstring->from, gstring->from + i, 0, 0);
      if (pos <= gstring->from)
	pos = gstring->from + 1;
      else if (pos >= gstring->to)
	pos = gstring->to;
    }
  else if (i == 0)
    pos++;
  compose_glyph_string (frame, mt, gstring->from, pos, gstring);
  layout_glyph_string (frame, gstring);
}


/* Return a gstring that covers a character at POS.  */

static MGlyphString *
get_gstring (MFrame *frame, MText *mt, int pos, int to, MDrawControl *control)
{
  MGlyphString *gstring = NULL;

  if (pos < mtext_nchars (mt))
    {
      MTextProperty *prop = mtext_get_property (mt, pos, M_glyph_string);

      if (prop
	  && ((prop->start != 0
	       && mtext_ref_char (mt, prop->start - 1) != '\n')
	      || (prop->end < mtext_nchars (mt)
		  && mtext_ref_char (mt, prop->end - 1) != '\n')))
	{
	  mtext_detach_property (prop);
	  prop = NULL;
	}
      if (prop)
	{
	  gstring = prop->val;
	  if (gstring->frame != frame
	      || gstring->tick != frame->tick
	      || memcmp (control, &gstring->control,
			 (char *) (&control->with_cursor)
			 - (char *) (control))
	      || control->cursor_width != gstring->control.cursor_width
	      || control->cursor_bidi != gstring->control.cursor_bidi)
	    {
	      mtext_detach_property (prop);
	      gstring = NULL;
	    }
	}
    }
  else if (! control->cursor_width)
    return NULL;

  if (gstring)
    {
      MGlyphString *gst;
      int offset;

      offset = mtext_character (mt, pos, 0, '\n');
      if (offset < 0)
	offset = 0;
      else
	offset++;
      offset -= gstring->from;
      if (offset)
	for (gst = gstring; gst; gst = gst->next)
	  {
	    int i;

	    gst->from += offset;
	    gst->to += offset;
	    for (i = 0; i < gst->used; i++)
	      {
		gst->glyphs[i].pos += offset;
		gst->glyphs[i].to += offset;
	      }
	  }
      M17N_OBJECT_REF (gstring);
    }
  else
    {
      int beg, end;
      int line = 0, y = 0;

      if (pos < mtext_nchars (mt))
	{
	  beg = mtext_character (mt, pos, 0, '\n');
	  if (beg < 0)
	    beg = 0;
	  else
	    beg++;
	}
      else
	beg = pos;
      end = mtext_nchars (mt) + (control->cursor_width != 0);
      gstring = alloc_gstring (frame, mt, beg, control, line, y);
      if (beg < mtext_nchars (mt))
	compose_glyph_string (frame, mt, beg, end, gstring);
      layout_glyph_string (frame, gstring);
      end = gstring->to;
      if (gstring->width_limit
	  && gstring->width > gstring->width_limit)
	{
	  MGlyphString *gst = gstring;

	  truncate_gstring (frame, mt, gst);
	  while (gst->to < end)
	    {
	      line++, y += gst->height;
	      gst->next = alloc_gstring (frame, mt, gst->from, control,
					 line, y);
	      gst->next->top = gstring;
	      compose_glyph_string (frame, mt, gst->to, end, gst->next);
	      gst = gst->next;
	      layout_glyph_string (frame, gst);
	      if (gst->width <= gst->width_limit)
		break;
	      truncate_gstring (frame, mt, gst);
	    }
	}

      if (! control->disable_caching && pos < mtext_nchars (mt))
	{
	  MTextProperty *prop = mtext_property (M_glyph_string, gstring,
						MTEXTPROP_VOLATILE_STRONG);

	  if (end > mtext_nchars (mt))
	    end = mtext_nchars (mt);
	  mtext_attach_property (mt, beg, end, prop);
	  M17N_OBJECT_UNREF (prop);
	}
    }

  while (gstring->to <= pos)
    {
      if (! gstring->next)
	mdebug_hook ();
      gstring = gstring->next;
    }
  gstring->control = *control;

  return gstring;
}


static MDrawControl control_noop;

#define ASSURE_CONTROL(control)	\
  if (! control)		\
    control = &control_noop;	\
  else


static int
draw_text (MFrame *frame, MDrawWindow win, int x, int y,
	   MText *mt, int from, int to,
	   MDrawControl *control)
{
  MGlyphString *gstring;

  M_CHECK_POS_X (mt, from, -1);
  ASSURE_CONTROL (control);
  if (to > mtext_nchars (mt) + (control->cursor_width != 0))
    to = mtext_nchars (mt) + (control->cursor_width != 0);
  else if (to < from)
    to = from;

  gstring = get_gstring (frame, mt, from, to, control);
  if (! gstring)
    MERROR (MERROR_DRAW, -1);
  render_glyph_string (frame, win, x, y, gstring, from, to);
  from = gstring->to;
  while (from < to)
    {
      y += gstring->line_descent;
      M17N_OBJECT_UNREF (gstring->top);
      gstring = get_gstring (frame, mt, from, to, control);
      y += gstring->line_ascent;
      render_glyph_string (frame, win, x, y, gstring, from, to);
      from = gstring->to;
    }
  M17N_OBJECT_UNREF (gstring->top);

  return 0;
}


static MGlyph *
find_glyph_in_gstring (MGlyphString *gstring, int pos, int forwardp)
{
  MGlyph *g;

  if (forwardp)
    {
      for (g = MGLYPH (1); g->type != GLYPH_ANCHOR; g++)
	if (g->pos <= pos && g->to > pos)
	  break;
    }
  else
    {
      for (g = MGLYPH (gstring->used - 2); g->type != GLYPH_ANCHOR; g--)
	if (g->pos <= pos && g->to > pos)
	  break;
    }
  return g;
}

#define GET_LB_TYPE(MT, POS, LB_TYPE)					\
  do {									\
    int c = mtext_ref_char ((MT), (POS));				\
    (LB_TYPE) = ((c == ' ' || c == '\t' || c == '\n') ? M_kinsoku_bol	\
		 : mchartable_lookup (linebreak_table, c));		\
  } while (0)

static int
find_break_backward (MText *mt, int pos, int limit)
{
  MSymbol lb_type;

  if (pos <= limit)
    return limit;

  GET_LB_TYPE (mt, pos, lb_type);
  if (lb_type == M_kinsoku_bol)
    return find_break_backward (mt, pos - 1, limit);
  if (lb_type == Mnil)
    {
      while (pos > limit)
	{
	  GET_LB_TYPE (mt, pos - 1, lb_type);
	  if (lb_type != Mnil)
	    break;
	  pos--;
	}
    }
  else if (lb_type == M_break_at_word)
    {
      int beg = limit, end = mtext_nchars (mt);
      int in_word = mtext__word_segment (mt, pos, &beg, &end);

      if (in_word)
	pos = beg;
      else if (beg > limit)
	{
	  end = beg;
	  beg = limit;
	  mtext__word_segment (mt, beg - 1, &beg, &end);
	  pos = beg;
	}
    }
  while (pos > limit)
    {
      GET_LB_TYPE (mt, pos - 1, lb_type);
      if (lb_type != M_kinsoku_eol)
	return pos;
      pos--;
    }
  return limit;
}

static int
find_break_forward (MText *mt, int pos, int limit)
{
  MSymbol lb_type;

  GET_LB_TYPE (mt, pos, lb_type);
  if (lb_type == Mnil)
    {
      while (pos < limit)
	{
	  pos++;
	  GET_LB_TYPE (mt, pos, lb_type);
	}
    }
  else if (lb_type == M_break_at_word)
    {
      int beg = 0, end = mtext_nchars (mt);
      int in_word = mtext__word_segment (mt, pos, &beg, &end);

      if (! in_word)
	pos = end;
      else if (end < limit)
	{
	  beg = end;
	  pos = end = mtext_nchars (mt);
	  mtext__word_segment (mt, pos, &beg, &end);
	  pos = end;
	}
    }
  else if (lb_type == M_kinsoku_bol)
    pos++;
  while (pos < limit)
    {
      GET_LB_TYPE (mt, pos, lb_type);
      if (lb_type != M_kinsoku_bol)
	return pos;
      pos++;
    }
  return limit;
}


/* for debugging... */
char work[16];

char *
dump_combining_code (int code)
{
  char *vallign = "tcbB";
  char *hallign = "lcr";
  char *p;
  int off_x, off_y;

  if (! code)
    return "none";
  if (COMBINING_BY_CLASS_P (code))
    code = combining_code_from_class (COMBINING_CODE_CLASS (code));
  work[0] = vallign[COMBINING_CODE_BASE_Y (code)];
  work[1] = hallign[COMBINING_CODE_BASE_X (code)];
  off_y = COMBINING_CODE_OFF_Y (code) - 128;
  off_x = COMBINING_CODE_OFF_X (code) - 128;
  if (off_y > 0)
    sprintf (work + 2, "+%d", off_y);
  else if (off_y < 0)
    sprintf (work + 2, "%d", off_y);
  else if (off_x == 0)
    sprintf (work + 2, ".");
  p = work + strlen (work);
  if (off_x > 0)
    sprintf (p, ">%d", off_x);
  else if (off_x < 0)
    sprintf (p, "<%d", -off_x);
  p += strlen (p);
  p[0] = vallign[COMBINING_CODE_ADD_Y (code)];
  p[1] = hallign[COMBINING_CODE_ADD_X (code)];
  p[2] = '\0';
  return work;
}

void
dump_gstring (MGlyphString *gstring, int indent)
{
  char *prefix = (char *) alloca (indent + 1);
  MGlyph *g, *last_g = gstring->glyphs + gstring->used;

  memset (prefix, 32, indent);
  prefix[indent] = 0;

  fprintf (stderr, "(glyph-string");

  for (g = MGLYPH (0); g < last_g; g++)
    fprintf (stderr,
	     "\n%s  (%02d %s pos:%d-%d c:%04X code:%04X face:%x cmb:%s w:%02d bidi:%d)",
	     prefix,
	     g - gstring->glyphs,
	     (g->type == GLYPH_SPACE ? "SPC": g->type == GLYPH_PAD ? "PAD"
	      : g->type == GLYPH_ANCHOR ? "ANC"
	      : g->type == GLYPH_BOX ? "BOX" : "CHR"),
	     g->pos, g->to, g->c, g->code, (unsigned) g->rface,
	     dump_combining_code (g->combining_code),
	     g->width, g->bidi_level);
  fprintf (stderr, ")");
}


/* m17n-X internal APIs */

int
mdraw__init ()
{
  M_glyph_string = msymbol_as_managing_key ("  glyph-string");

  memset (&scratch_gstring, 0, sizeof (scratch_gstring));
  MLIST_INIT1 (&scratch_gstring, glyphs, 3);

  Minherited = msymbol ("inherited");
  Mcommon = msymbol ("common");

  McatCc = msymbol ("Cc");
  McatCf = msymbol ("Cf");

  MbidiR = msymbol ("R");
  MbidiAL = msymbol ("AL");
  MbidiRLE = msymbol ("RLE");
  MbidiRLO = msymbol ("RLO");
  MbidiBN = msymbol ("BN");
  MbidiS = msymbol ("S");
#ifdef HAVE_FRIBIDI
  fribidi_set_mirroring (TRUE);
#endif

  M_break_at_space = msymbol ("bs");
  M_break_at_word = msymbol ("bw");
  M_break_at_any = msymbol ("ba");
  M_kinsoku_bol = msymbol ("kb");
  M_kinsoku_eol = msymbol ("ke");

  return 0;
}

void
mdraw__fini ()
{
  MLIST_FREE1 (&scratch_gstring, glyphs);
  M17N_OBJECT_UNREF (linebreak_table);
  linebreak_table = NULL;
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */
/*** @addtogroup m17nDraw */
/*** @{ */

/*=*/
/***en
    @brief Draw an M-text on a window.

    The mdraw_text () function draws the text between $FROM and $TO of
    M-text $MT on window $WIN of frame $FRAME at coordinate ($X, $Y).

    The appearance of the text (size, style, color, etc) is specified
    by the value of the text property whose key is @c Mface.  If the
    M-text or a part of the M-text does not have such a text property,
    the default face of $FRAME is used.

    The font used to draw a character in the M-text is selected from
    the value of the fontset property of a face by the following
    algorithm:

    <ol>

    <li> Search the text properties given to the character for the one
	 whose key is @c Mcharset; its value should be either a symbol
	 specifying a charset or #Mnil.  If the value is #Mnil,
	 proceed to the next step.

	 Otherwise, search the mapping table of the fontset for the
	 charset.  If no entry is found proceed to the next step.  

         If an entry is found, use one of the fonts in the entry that
	 has a glyph for the character and that matches best with the
	 face properties.  If no such font exists, proceed to the next
	 step.

    <li> Get the character property "script" of the character.  If it is
	 inherited, get the script property from the previous
	 characters.  If there is no previous character, or none of
	 them has the script property other than inherited, proceed to
	 the next step.

	 Search the text properties given to the character for the one
         whose key is @c Mlanguage; its value should be either a
         symbol specifying a language or @c Mnil.

	 Search the mapping table of the fontset for the combination
	 of the script and language.  If no entry is found, proceed to
	 the next step.  

         If an entry is found, use one of the fonts in the entry that
	 has a glyph for the character and that matches best with the
	 face properties.  If no such font exists, proceed to the next
	 step.

    <li> Search the fall-back table of the fontset for a font that has
	 a glyph of the character.  If such a font is found, use that
	 font.

    </ol>

    If no font is found by the algorithm above, this function draws an
    empty box for the character.

    This function draws only the glyph foreground.  To specify the
    background color, use mdraw_image_text () or
    mdraw_text_with_control ().

    This function is the counterpart of <tt>XDrawString ()</tt>,
    <tt>XmbDrawString ()</tt>, and <tt>XwcDrawString ()</tt> functions
    in the X Window System.

    @return
    If the operation was successful, mdraw_text () returns 0.  If an
    error is detected, it returns -1 and assigns an error code to the
    external variable #merror_code.  */
/***ja
    @brief ウィンドウに M-text を描画する.

    関数 mdraw_text () は、フレーム $FRAME のウィンドウ $WIN の座標 
    ($X, $Y) に、M-text $MT の $FROM から $TO までのテキストを描画する。

    テキストの見栄え（フォント、スタイル、色など）は、キーが @c Mface 
    であるテキストプロパティの値によって決まる。M-text 
    の一部あるいは全部にそのようなテキストプロパティが付いていない場合には、$FRAME 
    のデフォルトフェースを代わりに用いる。

    M-text の各文字を表示するフォントは、フェースの fontset 
    プロパティの値のうちから、以下のアルゴリズムで選ばれる。

    <ol>

    <li> その文字のテキストプロパティのうち、キーが @c Mcharset
         であるものの値を調べる。この値は文字セットを表わすシンボルか #Mnil 
         のどちらかである。#Mnil ならば、次のステップに進む。
	 そうでなければ、fontset 
	 のマッピングテーブルにその文字セット用のフォントがあるかどうかを調べる。
	 無ければ、次のステップに進む。
         
         その文字セット用のフォントがみつかれば、それらのうち現在の文字用のグリフを持ち、フェースの各プロパティに最もよく合致するものを使う。
	 そのようなフォントが無ければ次のステップに進む。

    <li> その文字の文字プロパティ "script" （スクリプト）を調べる。
         そのプロパティが継承されているならばそれ以前の文字の文字プロパティ "script" 
	 を調べる。前の文字がなかったり、その文字プロパティを持っていなかった場合には、次のステップに進む。

         その文字のテキストプロパティのうち、キーが @c Mlanguage であるものの値を調べる。
	 この値は言語を表わすシンボルか @c Mnil のいずれかである。

	 その言語とスクリプトの組み合わせが fontset
	 のマッピングテーブルにあるかどうかを調べる。見つからなければ次のステップに進む。

	 見つかったばあいには、それらのフォントのうち現在の文字用のグリフを持ち、フェースの各プロパティに最もよく合致しているものを使う。
	 そのようなフォントが無ければ次のステップに進む。

    <li> その文字のグリフを持つフォントを、フォントセットの fall-back 
         テーブルから探す。フォントが見つかればそれを使う。

    </ol>

    以上のアルゴリズムでフォントが見つからなければ、この関数はその文字として空の四角形を表示する。

    この関数が描画するのはグリフの前景だけである。背景色を指定するには、関数
    mdraw_image_text () か関数 mdraw_text_with_control () を使うこと。

    この関数は、X ウィンドウにおける関数 <tt>XDrawString ()</tt>,
    <tt>XmbDrawString ()</tt>, <tt>XwcDrawString ()</tt> に相当する。

    @return
    処理が成功した場合、mdraw_text () は 0 返す。エラーが検出された場合は
    -1 を返し、外部変数 #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mdraw_text} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mdraw_image_text ()  */

int
mdraw_text (MFrame *frame, MDrawWindow win, int x, int y,
	    MText *mt, int from, int to)
{
  MDrawControl control;

  M_CHECK_WRITABLE (frame, MERROR_DRAW, -1);
  memset (&control, 0, sizeof control);
  control.as_image = 0;
  return draw_text (frame, win, x, y, mt, from, to, &control);
}

/*=*/


/***en
    @brief Draw an M-text on a window as an image.

    The mdraw_image_text () function draws the text between $FROM and
    $TO of M-text $MT as image on window $WIN of frame $FRAME at
    coordinate ($X, $Y).

    The way to draw a text is the same as in mdraw_text () except that
    this function also draws the background with the color specified
    by faces.

    This function is the counterpart of <tt>XDrawImageString ()</tt>,
    <tt>XmbDrawImageString ()</tt>, and <tt>XwcDrawImageString ()</tt>
    functions in the X Window System.

    @return
    If the operation was successful, mdraw_image_text () returns 0.
    If an error is detected, it returns -1 and assigns an error code
    to the external variable #merror_code.  */

/***ja
    @brief ディスプレイにM-text を画像として描く.
  
    関数 mdraw_image_text () は、フレーム $FRAME のウィンドウ $WIN 
    の座標 ($X, $Y) に、M-text $MT の $FROM から $TO 
    までのテキストを画像として描く。

    テキストの描画方法は mdraw_text ()
    とほぼ同じであるが、この関数ではフェースで指定された色で背景も描く点が異なっている。

    この関数は、X ウィンドウにおける <tt>XDrawImageString ()</tt>,
    <tt>XmbDrawImageString ()</tt>, <tt>XwcDrawImageString ()</tt> 
    に相当する。

    @return
    処理が成功した場合、mdraw_image_text () は 0 
    を返す。エラーが検出された場合は -1 を返し、外部変数 #m_errro にエラーコードを設定する。

    @latexonly \IPAlabel{mdraw_image_text} @endlatexonly   */

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mdraw_text ()  */

int
mdraw_image_text (MFrame *frame, MDrawWindow win, int x, int y,
		  MText *mt, int from, int to)
{
  MDrawControl control;

  M_CHECK_WRITABLE (frame, MERROR_DRAW, -1);
  memset (&control, 0, sizeof control);
  control.as_image = 1;
  return draw_text (frame, win, x, y, mt, from, to, &control);
}

/*=*/

/***en
    @brief Draw an M-text on a window with fine control.

    The mdraw_text_with_control () function draws the text between
    $FROM and $TO of M-text $MT on windows $WIN of frame $FRAME at
    coordinate ($X, $Y).

    The way to draw a text is the same as in mdraw_text () except that
    this function also follows what specified in the drawing control
    object $CONTROL.

    For instance, if <two_dimensional> of $CONTROL is nonzero, this
    function draw an M-text 2-dimensionally, i.e., newlines in M-text
    breaks lines and the following characters are drawn in the next
    line.  See the documentation of the structure @ MDrawControl for
    more detail.  */

/***ja
    @brief ディスプレイにM-text を詳細な制御つきで描く.

    関数 mdraw_text_with_control () は、フレーム $FRAME のウィンドウ 
    $WIN の座標 ($X, $Y) に、M-text $MT の $FROM から $TO までのテキス
    トを描く。

    テキストの描画方法は mdraw_text () とほぼ同じであるが、この関数は描画制御用のオブジェクト
    $CONTROL の指示にも従う点が異なっている。

    たとえば $CONTROL の <two_dimensional> がゼロでなければ、この関数は 
    M-text を2次元的に描く。すなわち M-text 中の改行で行を改め、続く文字は次の行に描く。詳細は構造体
    @ MDrawControl の説明を参照すること。*/

int
mdraw_text_with_control (MFrame *frame, MDrawWindow win, int x, int y,
			 MText *mt, int from, int to, MDrawControl *control)
{
  M_CHECK_WRITABLE (frame, MERROR_DRAW, -1);
  return draw_text (frame, win, x, y, mt, from, to, control);
}

/*=*/

/***en
    @brief Compute text pixel width.

    The mdraw_text_extents () function computes the width of text
    between $FROM and $TO of M-text $MT when it is drawn on a window
    of frame $FRAME using the mdraw_text_with_control () function with
    the drawing control object $CONTROL.

    If $OVERALL_INK_RETURN is not @c NULL, this function also computes
    the bounding box of character ink of the M-text, and stores the
    results in the members of the structure pointed to by
    $OVERALL_INK_RETURN.  If the M-text has a face specifying a
    surrounding box, the box is included in the bounding box.

    If $OVERALL_LOGICAL_RETURN is not @c NULL, this function also
    computes the bounding box that provides minimum spacing to other
    graphical features (such as surrounding box) for the M-text, and
    stores the results in the members of the structure pointed to by
    $OVERALL_LOGICAL_RETURN.

    If $OVERALL_LINE_RETURN is not @c NULL, this function also
    computes the bounding box that provides minimum spacing to the
    other M-text drawn, and stores the results in the members of the
    structure pointed to by $OVERALL_LINE_RETURN.  This is a union of
    $OVERALL_INK_RETURN and $OVERALL_LOGICAL_RETURN if the members
    min_line_ascent, min_line_descent, max_line_ascent, and
    max_line_descent of $CONTROL are all zero.

    @return
    This function returns the width of the text to be drawn in the
    unit of pixels.  If $CONTROL->two_dimensional is nonzero and the
    text is drawn in multiple physical lines, it returns the width of
    the widest line.  If an error occurs, it returns -1 and assigns an
    error code to the external variable #merror_code.  */


/***ja 
    @brief テキストの幅（ピクセル単位）を計算する.

    関数 mdraw_text_extents () は、関数 mdraw_text_with_control () 
    が描画制御オブジェクト $CONTROL を用いて M-text $MT の $FROM から $TO 
    までをフレーム $FRAME に表示する際に必要となる幅を返す。

    $OVERALL_INK_RETURN が @c NULL でなければ、この関数は M-text 
    の文字のインクのバウンディングボックスも計算し、$OVERALL_INK_RETURN 
    が指す構造体のメンバにその結果を設定する。M-text に囲み枠 (surrounding box)
    を指定するフェースがあれば、それもバウンディングボックスに含む。

    $OVERALL_LOGICAL_RETURN が @c NULL でなければ、この関数は M-text 
    と他の graphical feature （囲み枠など）
    との間の最小のスペースを示すバウンディングボックスも計算し、$OVERALL_LOGICAL_RETURN
    が指す構造体のメンバにその結果を設定する。

    $OVERALL_LINE_RETURN が @c NULL でなければ、この関数は他の M-text 
    との間の最小のスペースを示すバウンディングボックスも計算し、
    $OVERALL_LINE_RETURN が指す構造体のメンバにその結果を設定する。オブジェクト
    $CONTROL のメンバ min_line_ascent, min_line_descent,
    max_line_ascent, max_line_descent がすべて0の時には、この値は 
    $OVERALL_INK_RETURN と$OVERALL_LOGICAL_RETURN の和となる。

    @return 
    この関数は表示に必要なテキストの幅をピクセル単位で返す。$CONTROL->two_dimensional
    が0でなく、テキストが複数の行に渡って描かれる場合には、最大の幅を返す。エラーが生じた場合は
    -1 を返し、外部変数 #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mdraw_text_extents} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE  */

int
mdraw_text_extents (MFrame *frame,
		    MText *mt, int from, int to, MDrawControl *control,
		    MDrawMetric *overall_ink_return,
		    MDrawMetric *overall_logical_return,
		    MDrawMetric *overall_line_return)
{
  MGlyphString *gstring;
  int y = 0;
  int width, rbearing;

  ASSURE_CONTROL (control);
  M_CHECK_POS_X (mt, from, -1);
  if (to > mtext_nchars (mt) + (control->cursor_width != 0))
    to = mtext_nchars (mt) + (control->cursor_width != 0);
  else if (to < from)
    to = from;

  gstring = get_gstring (frame, mt, from, to, control);
  if (! gstring)
    MERROR (MERROR_DRAW, -1);
  width = gstring_width (gstring, from, to, &rbearing);
  if (overall_ink_return)
    {
      overall_ink_return->y = - gstring->physical_ascent;
      overall_ink_return->x = gstring->lbearing;
    }
  if (overall_logical_return)
    {
      overall_logical_return->y = - gstring->ascent;
      overall_logical_return->x = 0;
    }
  if (overall_line_return)
    {
      overall_line_return->y = - gstring->line_ascent;
      overall_line_return->x = gstring->lbearing;
    }

  for (from = gstring->to; from < to; from = gstring->to)
    {
      int this_width, this_rbearing;

      y += gstring->line_descent;
      M17N_OBJECT_UNREF (gstring->top);
      gstring = get_gstring (frame, mt, from, to, control);
      this_width = gstring_width (gstring, from, to, &this_rbearing);
      y += gstring->line_ascent;
      if (width < this_width)
	width = this_width;
      if (rbearing < this_rbearing)
	rbearing = this_rbearing;
    }
  if (overall_ink_return)
    {
      overall_ink_return->width = rbearing;
      overall_ink_return->height
	= y + gstring->physical_descent - overall_ink_return->y;
    }
  if (overall_logical_return)
    {
      overall_logical_return->width = width;
      overall_logical_return->height
	= y + gstring->descent - overall_logical_return->y;
    }
  if (overall_line_return)
    {
      overall_line_return->width = MAX (width, rbearing);
      overall_line_return->height
	= y + gstring->line_descent - overall_line_return->y;
    }

  M17N_OBJECT_UNREF (gstring->top);
  return width;
}

/*=*/

/***en
    @brief Compute the text dimensions of each character of M-text.

    The mdraw_text_per_char_extents () function computes the drawn
    metric of each character between $FROM and $TO of M-text $MT
    assuming that they are drawn on a window of frame $FRAME using the
    mdraw_text_with_control () function with the drawing control
    object $CONTROL.

    $ARRAY_SIZE specifies the size of $INK_ARRAY_RETURN and
    $LOGICAL_ARRAY_RETURN.  Each successive element of
    $INK_ARRAY_RETURN and $LOGICAL_ARRAY_RETURN are set to the drawn
    ink and logical metrics of successive characters respectively,
    relative to the drawing origin of the M-text.  The number of
    elements of $INK_ARRAY_RETURN and $LOGICAL_ARRAY_RETURN that have
    been set is returned to $NUM_CHARS_RETURN.

    If $ARRAY_SIZE is too small to return all metrics, the function
    returns -1 and store the requested size in $NUM_CHARS_RETURN.
    Otherwise, it returns zero.

    If pointer $OVERALL_INK_RETURN and $OVERALL_LOGICAL_RETURN are not
    @c NULL, this function also computes the metrics of the overall
    text and stores the results in the members of the structure
    pointed to by $OVERALL_INK_RETURN and $OVERALL_LOGICAL_RETURN.

    If $CONTROL->two_dimensional is nonzero, this function computes
    only the metrics of characters in the first line.  */
/***ja
    @brief  M-text の各文字の表示範囲を計算する.

    関数 mdraw_text_per_char_extents () は、関数 mdraw_text_with_control ()
    が描画制御オブジェクト $CONTROL を用いて M-text $MT の $FROM から $TO 
    までをフレーム $FRAME に表示する際の各文字のサイズを計算する。

    $ARRAY_SIZE によって $INK_ARRAY_RETURN と$LOGICAL_ARRAY_RETURN 
    のサイズを指定する。$INK_ARRAY_RETURN と$LOGICAL_ARRAY_RETURN 
    の各要素は、それぞれ文字の描画インクと論理サイズ（M-text 
    の表示原点からの相対位値）によって順に埋められる。設定された $INK_ARRAY_RETURN と 
    $LOGICAL_ARRAY_RETURN の要素の数は、$NUM_CHARS_RETURN に戻される。
   
    $ARRAY_SIZE がすべての寸法を戻せないほど小さい場合には、関数は -1 
    を返し、必要な大きさを $NUM_CHARS_RETURN に返す。そうでなければ 0 
    を返す。

    ポインタ $OVERALL_INK_RETURN と $OVERALL_LOGICAL_RETURN が@c NULL 
    でなければ、この関数はテキスト全体のサイズも計算し、結果を
    $OVERALL_INK_RETURN と $OVERALL_LOGICAL_RETURN で指される構造のメンバに保存する。

    $CONTROL->two_dimensional が0でなければ、この関数は最初の行の文字のサイズだけを計算する。 */

int
mdraw_text_per_char_extents (MFrame *frame,
			     MText *mt, int from, int to,
			     MDrawControl *control,
			     MDrawMetric *ink_array_return,
			     MDrawMetric *logical_array_return,
			     int array_size,
			     int *num_chars_return,
			     MDrawMetric *overall_ink_return,
			     MDrawMetric *overall_logical_return)
{
  MGlyphString *gstring;
  MGlyph *g;
  int x;

  ASSURE_CONTROL (control);
  *num_chars_return = to - from;
  if (array_size < *num_chars_return)
    MERROR (MERROR_DRAW, -1);
  if (overall_logical_return)
    memset (overall_logical_return, 0, sizeof (MDrawMetric));
  if (overall_ink_return)
    memset (overall_ink_return, 0, sizeof (MDrawMetric));

  M_CHECK_RANGE (mt, from, to, -1, 0);
  gstring = get_gstring (frame, mt, from, to, control);
  if (! gstring)
    {
      *num_chars_return = 0;
      return 0;
    }

  for (g = MGLYPH (1), x = 0; g->type != GLYPH_ANCHOR; g++)
    if (g->pos >= from && g->pos < to)
      {
	int start = g->pos;
	int end = g->to;
	int width = g->width;
	int lbearing = g->lbearing;
	int rbearing = g->rbearing;
	int ascent = g->ascent;
	int descent = g->descent;
	int logical_ascent;
	int logical_descent;

	if (g->rface->rfont)
	  {
	    logical_ascent = g->rface->rfont->ascent;
	    logical_descent = g->rface->rfont->descent;
	  }
	else
	  {
	    logical_ascent = g->rface->ascent;
	    logical_descent = g->rface->descent;
	  }
	for (g++; g->type != GLYPH_ANCHOR && g->pos == start; g++)
	  {
	    if (lbearing < width + g->lbearing)
	      lbearing = width + g->lbearing;
	    if (rbearing < width + g->rbearing)
	      rbearing = width + g->rbearing;
	    width += g->width;
	    if (ascent < g->ascent)
	      ascent = g->ascent;
	    if (descent < g->descent)
	      descent = g->descent;
	  }

	if (end > to)
	  end = to;
	while (start < end)
	  {
	    if (ink_array_return)
	      {
		ink_array_return[start - from].x = x + lbearing;
		ink_array_return[start - from].y = - ascent;
		ink_array_return[start - from].width = rbearing - lbearing;
		ink_array_return[start - from].height = ascent + descent;
	      }
	    if (logical_array_return)
	      {
		logical_array_return[start - from].x = x;
		logical_array_return[start - from].y = - logical_descent;
		logical_array_return[start - from].height
		  = logical_ascent + logical_descent;
		logical_array_return[start - from].width = width;
	      }
	    start++;
	  }
	x += width;
	g--;
      }

  if (overall_ink_return)
    {
      overall_ink_return->y = - gstring->line_ascent;
      overall_ink_return->x = gstring->lbearing;
      overall_ink_return->width = x - gstring->lbearing;
      overall_ink_return->height = gstring->height;
    }
  if (overall_logical_return)
    {
      overall_logical_return->y = - gstring->ascent;
      overall_logical_return->x = 0;
      overall_logical_return->width = x;
      overall_logical_return->height = gstring->ascent + gstring->descent;
    }

  M17N_OBJECT_UNREF (gstring->top);
  return 0;
}

/*=*/

/***en
    @brief Return the character position nearest to the coordinates.

    The mdraw_coordinates_position () function checks which character
    is to be drawn at coordinate ($X, $Y) when the text between $FROM
    and $TO of M-text $MT is drawn at the coordinate (0, 0) using the
    mdraw_text_with_control () function with the drawing control
    object $CONTROL.  Here, the character position means the number of
    characters that precede the character in question in $MT, that is,
    the character position of the first character is 0.

    $FRAME is used only to get the default face information.

    @return
    If the glyph image of a character covers coordinate ($X, $Y),
    mdraw_coordinates_position () returns the character position of
    that character.\n\n
    If $Y is less than the minimum Y-coordinate of the drawn area, it
    returns $FROM.\n\n
    If $Y is greater than the maximum Y-coordinate of the drawn area,
    it returns $TO.\n\n
    If $Y fits in with the drawn area but $X is less than the minimum
    X-coordinate, it returns the character position of the first
    character drawn on the line $Y.\n\n
    If $Y fits in with the drawn area but $X is greater than the
    maximum X-coordinate, it returns the character position of the
    last character drawn on the line $Y.  */

/***ja
    @brief 指定した座標に最も近い文字の文字位置を得る.

    関数 mdraw_coordinates_position () は、関数 
    mdraw_text_with_control () が描画制御オブジェクト $CONTROL を用いて、
    M-text $MT の $FROM から $TO までを座標 (0, 0) 
    を起点として描画する際に、座標 ($X, $Y) 
    に描画される文字の文字位置を返す。ここで文字位置とは、当該
    M-text 中においてその文字が最初から何番目かを示す整数である。ただし最初の文字の文字位置は0とする。

    $FRAME はデフォルトのフェースの情報を得るためだけに用いられる。

    @return
    座標 ($X, $Y) がある文字のグリフで覆われる場合、 関数 
    mdraw_coordinates_position () はその文字の文字位置を返す。\n\n
    もし $Y が描画領域の最小Y座標よりも小さいならば $FROM を返す。\n\n
    もし $Y が描画領域の最大Y座標よりも大きいならば $TO を返す。\n\n
    もし $Y が描画領域に乗っていてかつ $X が描画領域の最小X座標よりも
    小さい場合は、直線 y = $Y 上に描画される最初の文字の文字位置を返す。\n\n
    もし $Y が描画領域に乗っていてかつ $X が描画領域の最大X座標よりも
    大きい場合は、直線 y = $Y 上に描画される最後の文字の文字位置を返す。 */

int
mdraw_coordinates_position (MFrame *frame, MText *mt, int from, int to,
			    int x_offset, int y_offset, MDrawControl *control)
{
  MGlyphString *gstring;
  int y = 0;
  int width;
  MGlyph *g;

  M_CHECK_POS_X (mt, from, -1);
  if (to > mtext_nchars (mt) + (control->cursor_width != 0))
    to = mtext_nchars (mt) + (control->cursor_width != 0);
  else if (to < from)
    to = from;

  if (from == to)
    return from;
  ASSURE_CONTROL (control);
  gstring = get_gstring (frame, mt, from, to, control);
  while (y + gstring->line_descent <= y_offset
	 && gstring->to < to)
    {
      from = gstring->to;
      y += gstring->line_descent;
      M17N_OBJECT_UNREF (gstring->top);
      gstring = get_gstring (frame, mt, from, to, control);
      y += gstring->line_ascent;
    }

  /* Accumulate width of glyphs in WIDTH until it exceeds X. */
  if (! control->orientation_reversed)
    {
      width = gstring->indent;
      for (g = MGLYPH (1); g->type != GLYPH_ANCHOR; g++)
	if (g->pos >= from && g->pos < to)
	  {
	    width += g->width;
	    if (width > x_offset)
	      break;
	  }
    }
  else
    {
      width = - gstring->indent;
      for (g = MGLYPH (gstring->used - 2); g->type != GLYPH_ANCHOR; g--)
	if (g->pos >= from && g->pos < to)
	  {
	    width -= g->width;
	    if (width < x_offset)
	      break;
	  }
    }
  if (g->type == GLYPH_ANCHOR
      && control->two_dimensional
      && g[-1].c == '\n')
    g--;
  from = g->pos;
  M17N_OBJECT_UNREF (gstring->top);

  return from;
}

/*=*/

/***en
    @brief Compute information about a glyph.

    The mdraw_glyph_info () function computes information about a
    glyph that covers a character at position $POS of the M-text $MT
    assuming that the text is drawn from the character at $FROM of $MT
    on a window of frame $FRAME using the mdraw_text_with_control ()
    function with the drawing control object $CONTROL.

    The information is stored in the members of $INFO.  */
/***ja
    @brief グリフに関する情報を計算する.

    関数 mdraw_glyph_info () は、関数 mdraw_text_with_control () 
    が描 画制御オブジェクト $CONTROL を用いてM-text $MT の $FROM から $TO 
    までをフレーム $FRAME に描画した場合、M-text の文字位置 $POS 
    の文字を覆うグリフに関する情報を計算する。

    情報は$INFO のメンバに保持される。  */

/***
    @seealso
    MDrawGlyphInfo
*/

int
mdraw_glyph_info (MFrame *frame, MText *mt, int from, int pos,
		  MDrawControl *control, MDrawGlyphInfo *info)
{
  MGlyphString *gstring;
  MGlyph *g;
  int y = 0;

  M_CHECK_RANGE_X (mt, from, pos, -1);

  ASSURE_CONTROL (control);
  gstring = get_gstring (frame, mt, from, pos + 1, control);
  if (! gstring)
    MERROR (MERROR_DRAW, -1);
  while (gstring->to <= pos)
    {
      y += gstring->line_descent;
      M17N_OBJECT_UNREF (gstring->top);
      gstring = get_gstring (frame, mt, gstring->to, pos + 1, control);
      y += gstring->line_ascent;
    }
  info->line_from = gstring->from;
  if (info->line_from < from)
    info->line_from = from;
  info->line_to = gstring->to;

  info->y = y;
  if (! control->orientation_reversed)
    {
      info->x = gstring->indent;
      for (g = MGLYPH (1); g->pos > pos || g->to <= pos; g++)
	info->x += g->width;
    }
  else
    {
      info->x = - gstring->indent;
      for (g = MGLYPH (gstring->used - 2); g->pos > pos || g->to <= pos; g--)
	info->x -= g->width;
      while (g[-1].to == g->to)
	g--;
    }
  info->from = g->pos;
  info->to = g->to;
  info->metrics.x = g->lbearing;
  info->metrics.y = - gstring->line_ascent;
  info->metrics.height = gstring->height;
  info->metrics.width = - g->lbearing + g->width;
  if (g->rface->rfont)
    info->font = (MFont *) g->rface->rfont;
  else
    info->font = NULL;
  /* info->logical_width is calculated later.  */

  if (info->from > info->line_from)
    {
      /* The logically previous glyph is on this line.  */
      MGlyph *g_tmp = find_glyph_in_gstring (gstring, info->from - 1, 1);

      info->prev_from = g_tmp->pos;
    }
  else if (info->line_from > 0)
    {
      /* The logically previous glyph is on the previous line.  */
      MGlyphString *gst = get_gstring (frame, mt, gstring->from - 1,
				       gstring->from, control);
      MGlyph *g_tmp = find_glyph_in_gstring (gst, info->from - 1, 1);

      info->prev_from = g_tmp->pos;
      M17N_OBJECT_UNREF (gst->top);
    }
  else
    info->prev_from = -1;

  if (GLYPH_INDEX (g) > 1)
    info->left_from = g[-1].pos, info->left_to = g[-1].to;
  else if (! control->orientation_reversed)
    {
      if (info->line_from > 0)
	{
	  MGlyph *g_tmp;
	  MGlyphString *gst;
	  int p = gstring->from - 1;

	  gst = get_gstring (frame, mt, p, gstring->from, control);
	  g_tmp = gst->glyphs + (gst->used - 2);
	  info->left_from = g_tmp->pos, info->left_to = g_tmp->to;
	  M17N_OBJECT_UNREF (gst->top);
	}
      else
	info->left_from = info->left_to = -1;
    }
  else
    {
      if (gstring->to + (control->cursor_width == 0) <= mtext_nchars (mt))
	{
	  MGlyph *g_tmp;
	  MGlyphString *gst;
	  int p = gstring->to;

	  gst = get_gstring (frame, mt, p, p + 1, control);
	  g_tmp = gst->glyphs + (gst->used - 2);
	  info->left_from = g_tmp->pos, info->left_to = g_tmp->to;
	  M17N_OBJECT_UNREF (gst->top);
	}
      else
	info->left_from = info->left_to = -1;
    }

  if (info->to < gstring->to)
    {
      /* The logically next glyph is on this line.   */
      MGlyph *g_tmp = find_glyph_in_gstring (gstring, info->to, 0);

      info->next_to = g_tmp->to;
    }
  else if (info->to + (control->cursor_width == 0) <= mtext_nchars (mt))
    {
      /* The logically next glyph is on the next line.   */
      int p = info->to;
      MGlyphString *gst = get_gstring (frame, mt, p, p + 1, control);
      MGlyph *g_tmp = find_glyph_in_gstring (gst, p, 0);

      info->next_to = g_tmp->to;
      M17N_OBJECT_UNREF (gst->top);
    }
  else
    info->next_to = -1;

  for (info->logical_width = (g++)->width;
       g->pos == pos && g->type != GLYPH_ANCHOR;
       info->metrics.width += g->width, info->logical_width += (g++)->width);
  info->metrics.width += g[-1].rbearing - g[-1].width;

  if (g->type != GLYPH_ANCHOR)
    info->right_from = g->pos, info->right_to = g->to;
  else if (! control->orientation_reversed)
    {
      if (gstring->to + (control->cursor_width == 0) <= mtext_nchars (mt))
	{
	  pos = gstring->to;
	  M17N_OBJECT_UNREF (gstring->top);
	  gstring = get_gstring (frame, mt, pos, pos + 1, control);
	  g = MGLYPH (1);
	  info->right_from = g->pos, info->right_to = g->to;
	}
      else
	info->right_from = info->right_to = -1;
    }
  else
    {
      if (info->line_from > 0)
	{
	  pos = gstring->from - 1;
	  M17N_OBJECT_UNREF (gstring->top);
	  gstring = get_gstring (frame, mt, pos, pos + 1, control);
	  g = MGLYPH (1);
	  info->right_from = g->pos, info->right_to = g->to;
	}
      else
	info->right_from = info->right_to = -1;
    }

  M17N_OBJECT_UNREF (gstring->top);
  return 0;
}

/*=*/

/***en
    @brief Compute information about glyph sequence.

    The mdraw_glyph_list () function computes information about glyphs
    corresponding to the text between $FROM and $TO of M-text $MT when
    it is drawn on a window of frame $FRAME using the
    mdraw_text_with_control () function with the drawing control
    object $CONTROL.  $GLYPHS is an array of objects to store the
    information, and $ARRAY_SIZE is the array size.

    If $ARRAY_SIZE is large enough to cover all glyphs, it stores the
    number of actually filled elements in the place pointed by
    $NUM_GLYPHS_RETURN, and returns 0.

    Otherwise, it stores the required array size in the place pointed
    by $NUM_GLYPHS_RETURN, and returns -1.  */

/***ja
    @brief グリフ列に関する情報を計算する.

    関数 mdraw_glyph_list () は、関数 mdraw_text_with_control () 
    が描画制御オブジェクト $CONTROL を用いてM-text $MT の $FROM から $TO
    までをフレーム $FRAME に描画した場合の、各グリフの情報を $GLYPHS 
    が指す配列に格納する。 $ARRAY_SIZE はその配列のサイズである。

    もし $ARRAY_SIZE がすべてのグリフについての情報を格納するのに十分であれば、
    $NUM_GLYPHS_RETURN が指す場所に実際に埋めた要素の数を設定し 0 を返す。

    
    そうでなければ、$NUM_GLYPHS_RETURN が指す場所に必要な配列のサイズを設定し、
    -1 を返す。
    */

/***
    @seealso
    MDrawGlyph
*/

int
mdraw_glyph_list (MFrame *frame, MText *mt, int from, int to,
		  MDrawControl *control, MDrawGlyph *glyphs,
		  int array_size, int *num_glyphs_return)
{
  MGlyphString *gstring;
  MGlyph *g;
  int n;
  int pad_width = 0;

  ASSURE_CONTROL (control);
  *num_glyphs_return = 0;
  M_CHECK_RANGE (mt, from, to, -1, 0);
  gstring = get_gstring (frame, mt, from, to, control);
  if (! gstring)
    return -1;
  for (g = MGLYPH (1), n = 0; g->type != GLYPH_ANCHOR; g++)
    {
      if (g->type == GLYPH_BOX
	  || g->pos < from || g->pos >= to)
	continue;
      if (g->type == GLYPH_PAD)
	{
	  if (g->left_padding)
	    pad_width = g->width;
	  else if (n > 0)
	    {
	      pad_width = 0;
	      glyphs[-1].x_advance += g->width;
	    }
	  continue;
	}
      if (n < array_size)
	{
	  glyphs->from = g->pos;
	  glyphs->to = g->to;
	  glyphs->glyph_code = g->code;
	  glyphs->x_off = g->xoff + pad_width;
	  glyphs->y_off = g->yoff;
	  glyphs->lbearing = g->lbearing;
	  glyphs->rbearing = g->rbearing;
	  glyphs->ascent = g->ascent;
	  glyphs->descent = g->descent;
	  glyphs->x_advance = g->width + pad_width;
	  glyphs->y_advance = 0;
	  if (g->rface->rfont)
	    {
	      glyphs->font = (MFont *) g->rface->rfont;
	      glyphs->font_type
		= (g->rface->rfont->font->type == MFONT_SOURCE_X ? Mx
		   : g->rface->rfont->driver == &mfont__ft_driver ? Mfreetype
		   : Mxft);
	      glyphs->fontp = g->rface->rfont->fontp;
	    }
	  else
	    {
	      glyphs->font = NULL;
	      glyphs->font_type = Mnil;
	      glyphs->fontp = NULL;
	    }
	  pad_width = 0;
	  glyphs++;
	}
      n++;
    }
  M17N_OBJECT_UNREF (gstring->top);

  *num_glyphs_return = n;
  return (n <= array_size ? 0 : -1);
}

/*=*/

/***en
    @brief Draw one or more textitems.

    The mdraw_text_items () function draws one or more M-texts on
    window $WIN of frame $FRAME at coordinate ($X, $Y).  $ITEMS is an array
    of the textitems to be drawn and $NITEMS is the number of
    textitems in the array.  */

/***ja
    @brief textitem を表示する.

    関数 mdraw_text_items () は、一個以上のテキストアイテムを、フレーム
    $FRAME のウィンドウ $WIN の座標 ($X, $Y) に表示する。$ITEMS 
    は表示すべきテキストアイテムの配列であり、$NITEMS はその個数である。

    @latexonly \IPAlabel{mdraw_text_items} @endlatexonly  */

/***
    @seealso
    MTextItem, mdraw_text ().  */

void
mdraw_text_items (MFrame *frame, MDrawWindow win, int x, int y,
		  MDrawTextItem *items, int nitems)
{
  if (! (frame->device_type & MDEVICE_SUPPORT_OUTPUT))
    return;
  while (nitems-- > 0)
    {
      if (items->face)
	mtext_push_prop (items->mt, 0, mtext_nchars (items->mt), Mface,
			 items->face);
      mdraw_text_with_control (frame, win, x, y,
			       items->mt, 0, mtext_nchars (items->mt),
			       items->control);
      x += mdraw_text_extents (frame, items->mt, 0, mtext_nchars (items->mt),
			       items->control, NULL, NULL, NULL);
      x += items->delta;
      if (items->face)
	mtext_pop_prop (items->mt, 0, mtext_nchars (items->mt), Mface);
    }
}

/*=*/
/***en 
      @brief   calculate a line breaking position.

      The function mdraw_default_line_break () calculates a line
      breaking position based on the line number $LINE and the
      coordinate $Y, when a line is too long to fit within the width
      limit.  $POS is the position of the character next to the last
      one that fits within the limit.  $FROM is the position of the
      first character of the line, and $TO is the position of the last
      character displayed on the line if there were not width limit.
      $LINE and $Y are reset to 0 when a line is broken by a newline
      character, and incremented each time when a long line is broken
      because of the width limit.  

      @return 
      This function returns a character position to break the
      line.
*/

/***ja 
      @brief 改行位置を計算する.

      関数 mdraw_default_line_break () は、行が最大幅中に収まらない場合の改行位置を、行番号
      $LINE と座標 $Y に基づいて計算する。
      $POS は最大幅に収まる最後の文字の次の文字の位置である。
      $FROM はその行の最初の文字の位置、$TO 
      は最大幅が指定されていなければその行に表示される最後の文字の位置である。
      $LINE と $Y は改行文字によって行が改まった際には 0
      にリセットされ、最大幅によって行が改まった場合には 1 づつ増やされる。

      @return 
      この関数は改行する文字位置を返す。
*/

int
mdraw_default_line_break (MText *mt, int pos,
			  int from, int to, int line, int y)
{
  int p;

  if (! linebreak_table)
    {
      MDatabase *mdb = mdatabase_find (Mchar_table, Msymbol,
				       msymbol ("linebreak"), Mnil);

      if (mdb)
	linebreak_table = mdatabase_load (mdb);
      if (! linebreak_table)
	linebreak_table = mchartable (Msymbol, Mnil);
    }

  if (pos > from)
    {
      p = find_break_backward (mt, pos, from);
      if (p > from)
	return p;
    }
  if (pos < to)
    {
      p = find_break_forward (mt, pos, to);
      if (p < to)
	return p;
    }
  return to;
}

/*=*/

/***en
    @brief Obtain per character dimension information.

    The mdraw_per_char_extents () function computes the text dimension
    of each character in M-text $MT.  The faces given as text
    properties in $MT and the default face of frame $FRAME determine
    the fonts to draw the text.  Each successive element in
    $ARRAY_RETURN is set to the drawn metrics of successive
    characters, which is relative to the origin of the drawing, and a
    rectangle for each character in $MT.  The number of elements of
    $ARRAY_RETURN must be equal to or greater than the number of
    characters in $MT.

    If pointer $OVERALL_RETURN is not @c NULL, this function also
    computes the extents of the overall text and stores the results in
    the members of the structure pointed to by $OVERALL_RETURN.  */

/***ja
    @brief M-text の文字毎の表示範囲情報を得る.

    関数 mdraw_per_char_extents () は、M-text $MT 
    中の各文字の表示範囲を計算する。この計算に用いるフォントは、
    $MT のテキストプロパティで指定されたフェースと、フレーム $FRAME
    のデフォルトフェースによって決まる。$ARRAY_RETURN の各要素は、$MT
    中の各文字の表示範囲情報によって順に埋められる。表示範囲情報とは、
    表示原点からの相対位置と各文字の占める長方形である。$ARRAY_RETURN 
    の要素数は、M-text 中の文字数以上でなければならない。

    ポインタ $OVERALL_RETURN が @c NULL 
    でない場合は、テキスト全体の表示範囲情報も計算し、その結果を 
    $OVERALL_RETURN の指す構造体に格納する。

    @latexonly \IPAlabel{mdraw_per_char_extents} @endlatexonly  */

void
mdraw_per_char_extents (MFrame *frame, MText *mt,
			MDrawMetric *array_return,
			MDrawMetric *overall_return)
{
  int n = mtext_nchars (mt);

  mdraw_text_per_char_extents (frame, mt, 0, n, NULL, array_return, NULL,
			       n, &n, overall_return, NULL);
}

/***en 
    @brief clear cached information.    

    The mdraw_clear_cache () function clear cached information
    on M-text $MT that was attached by any of the drawing functions.
    When the behavior of `format' or `line_break'
    member functions of MDrawControl is changed, the cache must be cleared.

    @seealso
    MDrawControl */
/***ja 
    @brief キャッシュ情報を消す.

    関数 mdraw_clear_cache () は描画関数によって M-text $MT 
    に付加されたキャッシュ情報をすべて消去する。MDrawControl の `format' 
    あるいは `line_break' 
    メンバ関数の振舞いが変わった場合にはキャッシュを消去しなくてはならない。

    @seealso
    MDrawControl */

void
mdraw_clear_cache (MText *mt)
{
  mtext_pop_prop (mt, 0, mtext_nchars (mt), M_glyph_string);
}

/*** @} */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
