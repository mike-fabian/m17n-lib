/* face.c -- face module.
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
    @addtogroup m17nFace
    @brief A face is an object to control appearance of M-text.

    A @e face is an object of the type #MFace and controls how to
    draw M-texts.  A face has a fixed number of @e face @e properties.
    Like other types of properties, a face property consists of a key
    and a value.  A key is one of the following symbols:

    #Mforeground, #Mbackground, #Mvideomode, #Mhline, #Mbox,
    #Mfoundry, #Mfamily, #Mweight, #Mstyle, #Mstretch, #Madstyle,
    #Msize, #Mfontset, #Mratio, #Mhook_func, #Mhook_arg

    "The face property that belongs to face F and whose key is @c xxx"
    may be shortened to "the xxx property of F".

    The M-text drawing functions first search an M-text for the text
    property whose key is the symbol #Mface, then draw the M-text
    using the value of that text property.  This value must be a
    pointer to a face object.

    If there are multiple text properties whose key is @c Mface, and
    they are not conflicting one another, properties of those faces
    are merged and used.

    If no faces specify a certain property, the value of the default
    face is used.  */

/***ja
    @addtogroup m17nFace
    @brief フェースとは、M-text の表示を制御するオブジェクトである.

    @e フェース は #MFace 型のオブジェクトであり、M-text の表示方法
    を制御する。フェースは固定個数の @e フェースプロパティ を持つ。
    フェースプロパティはキーと値からなる。キーはシンボルであり、

    #Mforeground, #Mbackground, #Mvideomode, #Mhline, #Mbox, 
    #Mfoundry, #Mfamily, #Mweight, #Mstyle, #Mstretch, #Madstyle, 
    #Msize, #Mfontset, #Mratio, #Mhook_func, #Mhook_arg

    「フェース F のフェースプロパティのうちキーが @c Mxxx であるもの」
    のことを簡単に「F の xxx プロパティ」と呼ぶことがある。

    M-text の表示関数は、まず最初にその M-text からキーがシンボル 
    #Mface であるようなテキストプロパティを探し、次にその値に従って 
    M-text を表示する。この値はフェースオブジェクトへのポインタでなけ
    ればならない。

     M-text が、#Mface をキーとするテキストプロパティを複数持っており、
    かつそれらの値の間に衝突がないならば、フェース情報は組み合わされて
    用いられる。

    あるテキスト属性がどのフェースによっても指定されていない場合は、デ
    フォルトフェースの値が用いられる。  */

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
#include "charset.h"
#include "symbol.h"
#include "plist.h"
#include "mtext.h"
#include "textprop.h"
#include "internal-gui.h"
#include "face.h"
#include "font.h"
#include "fontset.h"

static M17NObjectArray face_table;

static MSymbol Mlatin;

static MSymbol M_face_prop_index;

/** Find a realized face registered on FRAME that is realized from
    FACE and using font RFONT.  If RFONT is NULL, find any that
    matches FACE.  */

static MRealizedFace *
find_realized_face (MFrame *frame, MFace *face, MRealizedFont *rfont)
{
  MPlist *rface_list;
  MRealizedFace *rface;
  int i;

  MPLIST_DO (rface_list, frame->realized_face_list)
    {
      rface = MPLIST_VAL (rface_list);
      if (! rfont
	  || rface->rfont == rfont)
	{
	  for (i = 0; i < MFACE_RATIO; i++)
	    if (rface->face.property[i] != face->property[i])
	      break;
	  if (i == MFACE_RATIO)
	    return rface;
	}
    }
  return NULL;
}

static void
free_face (void *object)
{
  MFace *face = (MFace *) object;

  if (face->property[MFACE_FONTSET])
    M17N_OBJECT_UNREF (face->property[MFACE_FONTSET]);
  if (face->property[MFACE_HLINE])
    free (face->property[MFACE_HLINE]);
  if (face->property[MFACE_BOX])
    free (face->property[MFACE_BOX]);
  M17N_OBJECT_UNREGISTER (face_table, face);
  free (object);
}


static MPlist *
serialize_hline (MPlist *plist, MFaceHLineProp *hline)
{
  MPlist *pl = mplist ();

  mplist_add (pl, Minteger, (void *) hline->type);
  mplist_add (pl, Minteger, (void *) hline->width);
  mplist_add (pl, Msymbol, hline->color);
  plist = mplist_add (plist, Mplist, pl);
  M17N_OBJECT_UNREF (pl);
  return plist;
}

static MPlist *
serialize_box (MPlist *plist, MFaceBoxProp *box)
{
  MPlist *pl = mplist ();

  mplist_add (pl, Minteger, (void *) box->width);
  mplist_add (pl, Minteger, (void *) box->inner_hmargin);
  mplist_add (pl, Minteger, (void *) box->inner_vmargin);
  mplist_add (pl, Minteger, (void *) box->outer_hmargin);
  mplist_add (pl, Minteger, (void *) box->outer_vmargin);
  mplist_add (pl, Msymbol, box->color_top);
  mplist_add (pl, Msymbol, box->color_bottom);
  mplist_add (pl, Msymbol, box->color_left);
  mplist_add (pl, Msymbol, box->color_right);
  plist = mplist_add (plist, Mplist, pl);
  M17N_OBJECT_UNREF (pl);
  return plist;
}

static MPlist *
serialize_face (void *val)
{
  MFace *face = val;
  MPlist *plist = mplist (), *pl = plist;
  int i;
  struct {
    MSymbol *key;
    MSymbol *type;
    MPlist *(*func) (MPlist *plist, void *val);
  } serializer[MFACE_PROPERTY_MAX]
      = { { &Mfoundry,		&Msymbol },
	  { &Mfamily,		&Msymbol },
	  { &Mweight,		&Msymbol },
	  { &Mstyle,		&Msymbol },
	  { &Mstretch,		&Msymbol },
	  { &Madstyle,		&Msymbol },
	  { &Msize,		&Minteger },
	  { &Mfontset,		NULL },
	  { &Mforeground,	&Msymbol },
	  { &Mbackground,	&Msymbol },
	  { &Mhline,		NULL },
	  { &Mbox,		NULL },
	  { &Mvideomode, 	&Msymbol },
	  { NULL,		NULL}, /* MFACE_HOOK_FUNC */
	  { NULL,		NULL}, /* MFACE_HOOK_ARG */
	  { &Mratio,		&Minteger } };
  
  for (i = 0; i < MFACE_PROPERTY_MAX; i++)
    if (face->property[i] && serializer[i].key)
      {
	pl = mplist_add (pl, Msymbol, *serializer[i].key);
	if (serializer[i].type)
	  pl = mplist_add (pl, *serializer[i].type, face->property[i]);
	else if (i == MFACE_FONTSET)
	  pl = mplist_add (pl, Msymbol, mfontset_name ((MFontset *)
						       face->property[i]));
	else if (i == MFACE_HLINE)
	  pl = serialize_hline (pl, (MFaceHLineProp *) face->property[i]);
	else if (i == MFACE_BOX)
	  pl = serialize_box (pl, (MFaceBoxProp *) face->property[i]);
      }

  return plist;
}

static void *
deserialize_hline (MPlist *plist)
{
  MFaceHLineProp hline, *hline_ret;

  if (! MPLIST_INTEGER_P (plist))
    MERROR (MERROR_FACE, NULL);
  hline.type = MPLIST_INTEGER_P (plist);
  plist = MPLIST_NEXT (plist);
  if (! MPLIST_INTEGER_P (plist))
    MERROR (MERROR_FACE, NULL);
  hline.width = MPLIST_INTEGER_P (plist);
  plist = MPLIST_NEXT (plist);
  if (! MPLIST_SYMBOL_P (plist))
    MERROR (MERROR_FACE, NULL);
  hline.color = MPLIST_SYMBOL (plist);
  MSTRUCT_MALLOC (hline_ret, MERROR_FACE);
  *hline_ret = hline;
  return hline_ret;
}

static void *
deserialize_box (MPlist *plist)
{
  MFaceBoxProp box, *box_ret;

  if (! MPLIST_INTEGER_P (plist))
    MERROR (MERROR_FACE, NULL);
  box.width = MPLIST_INTEGER (plist);
  plist = MPLIST_NEXT (plist);
  if (! MPLIST_INTEGER_P (plist))
    MERROR (MERROR_FACE, NULL);
  box.inner_hmargin = MPLIST_INTEGER (plist);
  plist = MPLIST_NEXT (plist);
  if (! MPLIST_INTEGER_P (plist))
    MERROR (MERROR_FACE, NULL);
  box.inner_vmargin = MPLIST_INTEGER (plist);
  plist = MPLIST_NEXT (plist);
  if (! MPLIST_INTEGER_P (plist))
    MERROR (MERROR_FACE, NULL);
  box.outer_hmargin = MPLIST_INTEGER (plist);
  plist = MPLIST_NEXT (plist);
  if (! MPLIST_INTEGER_P (plist))
    MERROR (MERROR_FACE, NULL);
  box.outer_vmargin = MPLIST_INTEGER (plist);
  plist = MPLIST_NEXT (plist);
  if (! MPLIST_SYMBOL_P (plist))
    MERROR (MERROR_FACE, NULL);
  box.color_top = MPLIST_SYMBOL (plist);
  plist = MPLIST_NEXT (plist);
  if (! MPLIST_SYMBOL_P (plist))
    MERROR (MERROR_FACE, NULL);
  box.color_bottom = MPLIST_SYMBOL (plist);
  plist = MPLIST_NEXT (plist);
  if (! MPLIST_SYMBOL_P (plist))
    MERROR (MERROR_FACE, NULL);
  box.color_left = MPLIST_SYMBOL (plist);
  plist = MPLIST_NEXT (plist);
  if (! MPLIST_SYMBOL_P (plist))
    MERROR (MERROR_FACE, NULL);
  box.color_right = MPLIST_SYMBOL (plist);
  MSTRUCT_MALLOC (box_ret, MERROR_FACE);
  *box_ret = box;
  return box_ret;
}

static void *
deserialize_face (MPlist *plist)
{
  MFace *face = mface ();

  MPLIST_DO (plist, plist)
    {
      MSymbol key;
      int index;
      void *val;

      if (! MPLIST_SYMBOL_P (plist))
	break;
      key = MPLIST_SYMBOL (plist);
      index = (int) msymbol_get (key, M_face_prop_index) - 1;
      plist = MPLIST_NEXT (plist);
      if (MPLIST_TAIL_P (plist))
	break;
      if (index < 0 || index >= MFACE_PROPERTY_MAX)
	continue;
      if (key == Mfoundry || key == Mfamily || key == Mweight || key == Mstyle
	  || key == Mstretch || key == Madstyle
	  || key == Mforeground || key == Mbackground || key == Mvideomode)
	{
	  if (! MPLIST_SYMBOL_P (plist))
	    continue;
	  val = MPLIST_VAL (plist);
	}
      else if (key == Msize || key == Mratio)
	{
	  if (! MPLIST_INTEGER_P (plist))
	    continue;
	  val = MPLIST_VAL (plist);
	}
      else if (key == Mfontset)
	{
	  if (! MPLIST_SYMBOL_P (plist))
	    continue;
	  val = mfontset (MSYMBOL_NAME (MPLIST_SYMBOL (plist)));
	}
      else if (key == Mhline)
	{
	  if (! MPLIST_PLIST_P (plist))
	    continue;
	  val = deserialize_hline (MPLIST_PLIST (plist));
	}
      else if (key == Mbox)
	{
	  if (! MPLIST_PLIST_P (plist))
	    continue;
	  val = deserialize_box (MPLIST_PLIST (plist));
	}
      face->property[index] = val;
    }
  return face;
}

static MGlyphString work_gstring;



/* Internal API */

MFace *mface__default;

int
mface__init ()
{
  int i;

  face_table.count = 0;
  Mface = msymbol_as_managing_key ("face");
  msymbol_put (Mface, Mtext_prop_serializer, (void *) serialize_face);
  msymbol_put (Mface, Mtext_prop_deserializer, (void *) deserialize_face);

  Mforeground = msymbol ("foreground");
  Mbackground = msymbol ("background");
  Mvideomode = msymbol ("videomode");
  Mnormal = msymbol ("normal");
  Mreverse = msymbol ("reverse");
  Mratio = msymbol ("ratio");
  Mhline = msymbol ("hline");
  Mbox = msymbol ("box");
  Mhook_func = msymbol ("hook-func");
  Mhook_arg = msymbol ("hook-arg");

  Mlatin = msymbol ("latin");
  M_face_prop_index = msymbol ("  face-prop-index");

  {
    struct {
      /* Pointer to the key symbol of the face property.  */
      MSymbol *key;
      /* Index (enum face_property) of the face property. */
      int index;
    } mface_prop_data[MFACE_PROPERTY_MAX] =
	{ { &Mfoundry,		MFACE_FOUNDRY },
	  { &Mfamily,		MFACE_FAMILY },
	  { &Mweight,		MFACE_WEIGHT },
	  { &Mstyle,		MFACE_STYLE },
	  { &Mstretch,		MFACE_STRETCH },
	  { &Madstyle,		MFACE_ADSTYLE },
	  { &Msize,		MFACE_SIZE },
	  { &Mfontset,		MFACE_FONTSET },
	  { &Mforeground,	MFACE_FOREGROUND },
	  { &Mbackground,	MFACE_BACKGROUND },
	  { &Mhline,		MFACE_HLINE },
	  { &Mbox,		MFACE_BOX },
	  { &Mvideomode, 	MFACE_VIDEOMODE },
	  { &Mhook_func,	MFACE_HOOK_FUNC },
	  { &Mhook_arg,		MFACE_HOOK_ARG },
	  { &Mratio,		MFACE_RATIO },
	};

    for (i = 0; i < MFACE_PROPERTY_MAX; i++)
      /* We add one to distinguish it from no-property.  */
      msymbol_put (*mface_prop_data[i].key, M_face_prop_index,
		   (void *) (mface_prop_data[i].index + 1));
  }

  mface__default = mface ();
  mface__default->property[MFACE_WEIGHT] = msymbol ("medium");
  mface__default->property[MFACE_STYLE] = msymbol ("r");
  mface__default->property[MFACE_STRETCH] = msymbol ("normal");
  mface__default->property[MFACE_SIZE] = (void *) 120;
  mface__default->property[MFACE_FONTSET] = mfontset (NULL);
  M17N_OBJECT_REF (mface__default->property[MFACE_FONTSET]);
  /* mface__default->property[MFACE_FOREGROUND] =msymbol ("black"); */
  /* mface__default->property[MFACE_BACKGROUND] =msymbol ("white"); */

  mface_normal_video = mface ();
  mface_normal_video->property[MFACE_VIDEOMODE] = (void *) Mnormal;

  mface_reverse_video = mface ();
  mface_reverse_video->property[MFACE_VIDEOMODE] = (void *) Mreverse;

  {
    MFaceHLineProp *hline_prop;

    MSTRUCT_MALLOC (hline_prop, MERROR_FACE);
    hline_prop->type = MFACE_HLINE_UNDER;
    hline_prop->width = 1;
    hline_prop->color = Mnil;
    mface_underline = mface ();
    mface_underline->property[MFACE_HLINE] = (void *) hline_prop;
  }

  mface_medium = mface ();
  mface_medium->property[MFACE_WEIGHT] = (void *) msymbol ("medium");
  mface_bold = mface ();
  mface_bold->property[MFACE_WEIGHT] = (void *) msymbol ("bold");
  mface_italic = mface ();
  mface_italic->property[MFACE_STYLE] = (void *) msymbol ("i");
  mface_bold_italic = mface_copy (mface_bold);
  mface_bold_italic->property[MFACE_STYLE]
    = mface_italic->property[MFACE_STYLE];

  mface_xx_small = mface ();
  mface_xx_small->property[MFACE_RATIO] = (void *) 50;
  mface_x_small = mface ();
  mface_x_small->property[MFACE_RATIO] = (void *) 67;
  mface_small = mface ();
  mface_small->property[MFACE_RATIO] = (void *) 75;
  mface_normalsize = mface ();
  mface_normalsize->property[MFACE_RATIO] = (void *) 100;
  mface_large = mface ();
  mface_large->property[MFACE_RATIO] = (void *) 120;
  mface_x_large = mface ();
  mface_x_large->property[MFACE_RATIO] = (void *) 150;
  mface_xx_large = mface ();
  mface_xx_large->property[MFACE_RATIO] = (void *) 200;

  mface_black = mface ();
  mface_black->property[MFACE_FOREGROUND] = (void *) msymbol ("black");
  mface_white = mface ();
  mface_white->property[MFACE_FOREGROUND] = (void *) msymbol ("white");
  mface_red = mface ();
  mface_red->property[MFACE_FOREGROUND] = (void *) msymbol ("red");
  mface_green = mface ();
  mface_green->property[MFACE_FOREGROUND] = (void *) msymbol ("green");
  mface_blue = mface ();
  mface_blue->property[MFACE_FOREGROUND] = (void *) msymbol ("blue");
  mface_cyan = mface ();
  mface_cyan->property[MFACE_FOREGROUND] = (void *) msymbol ("cyan");
  mface_yellow = mface ();
  mface_yellow->property[MFACE_FOREGROUND] = (void *) msymbol ("yellow");
  mface_magenta = mface ();
  mface_magenta->property[MFACE_FOREGROUND] = (void *) msymbol ("magenta");

  work_gstring.glyphs = malloc (sizeof (MGlyph) * 2);
  work_gstring.size = 2;
  work_gstring.used = 0;
  work_gstring.inc = 1;
  return 0;
}

void
mface__fini ()
{
  M17N_OBJECT_UNREF (mface__default);
  M17N_OBJECT_UNREF (mface_normal_video);
  M17N_OBJECT_UNREF (mface_reverse_video);
  M17N_OBJECT_UNREF (mface_underline);
  M17N_OBJECT_UNREF (mface_medium);
  M17N_OBJECT_UNREF (mface_bold);
  M17N_OBJECT_UNREF (mface_italic);
  M17N_OBJECT_UNREF (mface_bold_italic);
  M17N_OBJECT_UNREF (mface_xx_small);
  M17N_OBJECT_UNREF (mface_x_small);
  M17N_OBJECT_UNREF (mface_small);
  M17N_OBJECT_UNREF (mface_normalsize);
  M17N_OBJECT_UNREF (mface_large);
  M17N_OBJECT_UNREF (mface_x_large);
  M17N_OBJECT_UNREF (mface_xx_large);
  M17N_OBJECT_UNREF (mface_black);
  M17N_OBJECT_UNREF (mface_white);
  M17N_OBJECT_UNREF (mface_red);
  M17N_OBJECT_UNREF (mface_green);
  M17N_OBJECT_UNREF (mface_blue);
  M17N_OBJECT_UNREF (mface_cyan);
  M17N_OBJECT_UNREF (mface_yellow);
  M17N_OBJECT_UNREF (mface_magenta);
  free (work_gstring.glyphs);

  mdebug__report_object ("Face", &face_table);
}

/** Return a realized face for ASCII characters from NUM number of
    base faces pointed by FACES on the frame FRAME.  */

MRealizedFace *
mface__realize (MFrame *frame, MFace **faces, int num,
		MSymbol language, MSymbol charset, int size)
{
  MRealizedFace *rface;
  MRealizedFont *rfont;
  MFace merged_face = *(frame->face);
  void **props;
  int i, j;
  unsigned tick;
  MGlyph g;

  if (num == 0 && language == Mnil && charset == Mnil && frame->rface)
    return frame->rface;

  for (i = 0; i < MFACE_PROPERTY_MAX; i++)
    for (j = num - 1; j >= 0; j--)
      if (faces[j]->property[i])
	{
	  merged_face.property[i] = faces[j]->property[i];
	  break;
	}

  for (i = 0, tick = 0; i < num; i++)
    tick += faces[i]->tick;

  if (merged_face.property[MFACE_RATIO])
    {
      int font_size = (int) merged_face.property[MFACE_SIZE];

      font_size *= (int) merged_face.property[MFACE_RATIO];
      font_size /= 100;
      merged_face.property[MFACE_SIZE] = (void *) font_size;
    }

  rface = find_realized_face (frame, &merged_face, NULL);
  if (rface && rface->tick == tick)
    return rface->ascii_rface;

  MSTRUCT_CALLOC (rface, MERROR_FACE);
  rface->frame = frame;
  rface->face = merged_face;
  rface->tick = tick;
  props = rface->face.property;

  rface->rfontset = mfont__realize_fontset (frame,
					    (MFontset *) props[MFACE_FONTSET],
					    &merged_face);
  g.c = ' ';
  num = 1;
  rfont = mfont__lookup_fontset (rface->rfontset, &g, &num,
				 msymbol ("latin"), language, Mnil,
				 size);

  if (rfont)
    {
      rface->rfont = rfont;
      g.otf_encoded = 0;
      work_gstring.glyphs[0] = g;
      work_gstring.glyphs[0].rface = rface;
      work_gstring.glyphs[1].code = MCHAR_INVALID_CODE;
      work_gstring.glyphs[1].rface = rface;
      mfont__get_metric (&work_gstring, 0, 2);
      rface->space_width = work_gstring.glyphs[0].width;
      rface->ascent = work_gstring.glyphs[1].ascent;
      rface->descent = work_gstring.glyphs[1].descent;
    }
  else
    {
      rface->rfont = NULL;
      rface->space_width = frame->space_width;
    }

  rface->hline = (MFaceHLineProp *) props[MFACE_HLINE];
  rface->box = (MFaceBoxProp *) props[MFACE_BOX];
  rface->ascii_rface = rface;
  mwin__realize_face (rface);

  mplist_add (frame->realized_face_list, Mt, rface);

  if (rface->rfont)
    {
      MSTRUCT_CALLOC (rface->nofont_rface, MERROR_FACE);
      *rface->nofont_rface = *rface;
      rface->nofont_rface->rfont = NULL;
    }
  else
    rface->nofont_rface = rface;

  return rface;
}


MGlyph *
mface__for_chars (MSymbol script, MSymbol language, MSymbol charset,
		  MGlyph *from_g, MGlyph *to_g, int size)
{
  MRealizedFace *rface;
  MRealizedFont *rfont;
  int num = to_g - from_g, i;

  rfont = mfont__lookup_fontset (from_g->rface->rfontset, from_g, &num,
				 script, language, charset, size);
  if (! rfont)
    {
      from_g->rface = from_g->rface->nofont_rface;
      return (from_g + 1);
    }
  rface = find_realized_face (from_g->rface->frame, &(from_g->rface->face),
			      rfont);
  if (! rface)
    {
      MSTRUCT_MALLOC (rface, MERROR_FACE);
      *rface = *from_g->rface->ascii_rface;
      rface->rfont = rfont;
      {
	work_gstring.glyphs[0].code = MCHAR_INVALID_CODE;
	work_gstring.glyphs[0].rface = rface;
	mfont__get_metric (&work_gstring, 0, 1);
	rface->ascent = work_gstring.glyphs[0].ascent;
	rface->descent = work_gstring.glyphs[0].descent;
      }
      mwin__realize_face (rface);
      mplist_add (from_g->rface->frame->realized_face_list, Mt, rface);
    }

  for (i = 0; i < num; i++, from_g++)
    from_g->rface = rface;
  return from_g;
}


void
mface__free_realized (MRealizedFace *rface)
{
  mwin__free_realized_face (rface);
  if (rface == rface->ascii_rface)
    {
      if (! rface->nofont_rface)
	mdebug_hook ();
      else
	free (rface->nofont_rface);
      rface->nofont_rface = NULL;
    }
  free (rface);
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API  */
/*** @addtogroup m17nFace */
/*** @{ */
/*=*/

/***en @name Variables: Keys of face property */
/***ja @name 変数: フェースプロパティのキー  */
/*** @{ */
/*=*/

/***en
    @brief Key of a face property specifying foreground color

    The variable #Mforeground is used as a key of face property.  The
    property value must be a symbol whose name is a color name, or
    #Mnil.

    #Mnil means that the face does not specify a foreground color.
    Otherwise, the foreground of an M-text is drawn by the specified
    color.  */

/***ja
    @brief 前景色を指定するフェースプロパティーのキー

    変数 #Mforeground はフェースプロパティのキーとして用いられる。
    プロパティの値は、色名を名前として持つシンボルか #Mnil である。

    #Mnil の場合、前景色は指定されない。そうれなければ M-text の前景は
    指定された色で表示される。  */

MSymbol Mforeground;

/***en
    @brief Key of a face property specifying background color

    The variable #Mbackground is used as a key of face property.  The
    property value must be a symbol whose name is a color name, or
    #Mnil.

    #Mnil means that the face does not specify a background color.
    Otherwise, the background of an M-text is drawn by the specified
    color.  */

/***ja
    @brief 背景色を指定するためのフェースプロパティーのキー

    変数 #Mbackground はフェースプロパティのキーとして用いられる。
    プロパティの値は、色名を名前として持つシンボルか #Mnil である。

    #Mnil の場合、背景色は指定されない。そうれなければ M-text の背景は
    指定された色で表示される。  */

MSymbol Mbackground;

/***en
    @brief Key of a face property specifying video mode

    The variable #Mvideomode is used as a key of face property.  The
    property value must be #Mnormal, #Mreverse, or #Mnil.

    #Mnormal means that an M-text is drawn in normal video mode
    (i.e. the foreground is drawn by foreground color, the background
    is drawn by background color).

    #Mreverse means that an M-text is drawn in reverse video mode
    (i.e. the foreground is drawn by background color, the background
    is drawn by foreground color).

    #Mnil means that the face does not specify a video mode.  */

/***ja
    @brief ビデオモードを指定するためのフェースプロパティーのキー

    変数 #Mvideomode はフェースプロパティのキーとして用いられる。
    プロパティの値は、#Mnormal, #Mreverse, #Mnil のいずれかでなくてはならない。

    #Mnormal の場合は、M-text は標準のビデオモード（前景を前景色で、背
    景を背景色で）で表示する。

    #Mreverse の場合はリバースビデオモードで（前景を背景色で、背景を前
    景色で）表示する。

    #Mnil の場合はビデオモードは指定されない。
    */

MSymbol Mvideomode;

/***en
    @brief Key of a face property specifying font size ratio

    The variable #Mratio is used as a key of face property.  The value
    RATIO must be an integer.

    The value 0 means that the face does not specify a font size
    ratio.  Otherwise, an M-text is drawn by a font of size (FONTSIZE
    * RATIO / 100) where FONTSIZE is a font size specified by the face
    property #Msize.  */
/***ja
    @brief サイズの比率を指定するためのフェースプロパティーのキー

    変数 #Mratio はフェースプロパティのキーとして用いられる。値 RATIO 
    は整数値でなくてはならない。

    値が0ならば、フォントサイズは指定されない。そうでなければ、M-text 
    は(FONTSIZE * RATIO / 100) というサイズのフォントで表示される。こ
    こで FONTSIZE はフェースプロパティー #Msize で指定されたサイズであ
    る。 */

MSymbol Mratio;

/***en
    @brief Key of a face property specifying horizontal line

    The variable #Mhline is used as a key of face property.  The value
    must be a pointer to an object of type #MFaceHLineProp, or @c
    NULL.

    The value @c NULL means that the face does not specify this
    property.  Otherwise, an M-text is drawn with a horizontal line by
    a way specified by the object that the value points to.  */

/***ja
    @brief 水平線を指定するためのフェースプロパティーのキー

    変数 #Mhline はフェースプロパティのキーとして用いられる。値は 
    #MFaceHLineProp 型オブジェクトへのポインタか @c NULL でなくてはな
    らない。

    値が @c NULL ならば、このプロパティは指定されない。そうでければ値
   が指すオブジェクトに指定されたように水平線を付加して M-text を表示
   する。*/

MSymbol Mhline;

/***en
    @brief Key of a face property specifying box

    The variable #Mbox is used as a key of face property.  The value
    must be a pointer to an object of type #MFaceBoxProp, or @c NULL.

    The value @c NULL means that the face does not specify a box.
    Otherwise, an M-text is drawn with a surrounding box by a way
    specified by the object that the value points to.  */

/***ja
    @brief 囲み枠を指定するためのフェースプロパティーのキー

    変数 #Mbox はフェースプロパティのキーとして用いられる。値は 
    #MFaceBoxProp 型オブジェクトへのポインタか @c NULL でなくてはなら
    ない。

    値が @c NULL ならば、このプロパティは指定されない。そうでければ値
    が指すオブジェクトに指定されたように囲み枠を付加して M-text を表示
    する。*/

MSymbol Mbox;

/***en
    @brief Key of a face property specifying fontset 

    The variable #Mfontset is used as a key of face property.  The
    value must be a pointer to an object of type #Mfontset, or @c
    NULL.

    The value @c NULL means that the face does not specify a fontset.
    Otherwise, an M-text is drawn with a font selected from what
    specified in the fontset.  */

/***ja
    @brief フォントセットを指定するためのフェースプロパティーのキー

    変数 #Mfontset はフェースプロパティのキーとして用いられる。値は 
    #Mfontset 型オブジェクトへのポインタか @c NULL でなくてはならない。

    値が @c NULL ならば、フォントセットは指定されない。そうでければ値
    が指すオブジェクトに指定されたフォントセットから選んだフォントで 
    M-text を表示する。*/
    
MSymbol Mfontset;

/***en
    @brief Key of a face property specifying hook

    The variable #Mhook_func is used as a key of face property.  The
    value must be a function of type #MFaceHookFunc, or @c NULL.

    The value @c NULL means that the face does not specify a hook.
    Otherwise, the specified function is called before the face is
    realized.  */
/***ja
    @brief フックを指定するためのフェースプロパティーのキー

    変数 #Mhook_func はフェースプロパティのキーとして用いられる。値は 
    #MFaceHookFunc 型の関数か @c NULL でなくてはならない。

    値が @c NULL ならば、フックは指定されない。そうでければフェースを
    実現する前に指定した関数が呼ばれる。      */
MSymbol Mhook_func;

/***en
    @brief Key of a face property specifying argument of hook

    The variable #Mhook_arg is used as a key of face property.  The
    value can be anything that is passed a hook function specified by
    the face property #Mhook_func.  */
/***ja
    @brief フックの引数を指定するためのフェースプロパティーのキー

    変数 #Mhook_arg はフェースプロパティのキーとして用いられる。値は 
    何でもよく、フェースプロパティ #Mhook_func で指定される関数に渡さ
    れる。 */
MSymbol Mhook_arg;

/*** @} */
/*=*/

/*** @ingroup m17nFace */
/***en @name Variables: Possible values of #Mvideomode property of face */
/***ja @name 変数：  フェースの #Mvideomode プロパティの可能な値 */
/*** @{ */
/*=*/

/***en
    See the documentation of the variable #Mvideomode.  */ 
/***ja
    変数 #Mvideomode の説明を参照のこと。  */ 
MSymbol Mnormal;
MSymbol Mreverse;
/*** @} */
/*=*/

/*** @ingroup m17nFace */
/***en @name Variables: Predefined faces  */
/***ja @name 変数: 定義済みフェース  */
/*** @{ */
/*=*/

/***en
    @brief Normal video face

    The variable #mface_normal_video points to a face that has the
    #Mvideomode property with value #Mnormal.  The other properties
    are not specified.  An M-text drawn with this face appear normal
    colors (i.e. the foreground is drawn by foreground color, and
    background is drawn by background color).  */
/***en
    @brief 標準ビデオフェース

    変数 #mface_normal_video は #Mvideomode プロパティの値が #Mnormal 
    であるフェースを指すポインタである。他のプロパティは指定されない。
    このフェースで表示されるM-text は標準の色 (すなわち前景は前景色、
    背景は背景色）で描かれる。  */

MFace *mface_normal_video;

/***en
    @brief Reverse video face

    The variable #mface_reverse_video points to a face that has the
    #Mvideomode property with value #Mreverse.  The other properties
    are not specified.  An M-text drawn with this face appear in
    reversed colors (i.e. the foreground is drawn by background
    color, and background is drawn by foreground color).  */
/***ja
    @brief リバースビデオフェース

    変数 #mface_reverse_video は #Mvideomode プロパティの値が 
    #Mreverse であるフェースを指すポインタである。他のプロパティは指定
    されない。このフェースで表示されるM-text は前景色と背景色が入れ替
    わって (すなわち前景は背景色、背景は前景色）描かれる。  */

MFace *mface_reverse_video;

/***en
    @brief Underline face

    The variable #mface_underline points to a face that has the
    #Mhline property with value a pointer to an object of type
    #MFaceHLineProp.  The members of the object are as follows:

@verbatim
    member  value
    -----   -----
    type    MFACE_HLINE_UNDER
    width   1
    color   Mnil
@endverbatim

    The other properties are not specified.  An M-text that has this
    face is drawn with an underline.  */ 
/***ja
    @brief 下線フェース

    変数 #mface_underline は #Mhline プロパテイの値が #MFaceHLineProp 
    型オブジェクトへのポインタであるフェースを指すポインタである。オブ
    ジェクトのメンバは以下の通り。

@verbatim
    メンバ  値
    -----   -----
    type    MFACE_HLINE_UNDER
    width   1
    color   Mnil
@endverbatim

    他のプロパティは指定されない。このフェースを持つ M-text は下線付き
    で表示される。*/ 

MFace *mface_underline;

/***en
    @brief Medium face

    The variable #mface_medium points to a face that has the #Mweight
    property with value a symbol of name "medium".  The other
    properties are not specified.  An M-text that has this face is
    drawn with a font of medium weight.  */
/***ja
    @brief ミディアムフェース

    変数 #mface_medium は #Mweight プロパテイの値が "medium" という名
    前をもつシンボルであるようなフェースを指すポインタである。他のプロ
    パティは指定されない。このフェースを持つ M-text は、ミディアムウェ
    イトのフォントで表示される。  */
MFace *mface_medium;

/***en
    @brief Bold face

    The variable #mface_bold points to a face that has the #Mweight
    property with value a symbol of name "bold".  The other properties
    are not specified.  An M-text that has this face is drawn with a
    font of bold weight.  */

/***ja
    @brief ボールドフェース

    変数 #mface_bold は #Mweight プロパテイの値が "bold" という名前を
    もつシンボルであるようなフェースを指すポインタである。他のプロパティ
    は指定されない。このフェースを持つ M-text は、ボールドのフォントで
    表示される。
     */

MFace *mface_bold;

/***en
    @brief Italic face

    The variable #mface_italic points to a face that has the #Mstyle
    property with value a symbol of name "italic".  The other
    properties are not specified.  An M-text that has this face is
    drawn with a font of italic style.  */

/***ja
    @brief イタリックフェース

    変数 #mface_italic は #Mstyle プロパテイの値が "italic" という名前
    をもつシンボルであるようなフェースを指すポインタである。他のプロパ
    ティは指定されない。このフェースを持つ M-text は、イタリック体で表
    示される。
     */

MFace *mface_italic;

/***en
    @brief Bold italic face

    The variable #mface_bold_italic points to a face that has the
    #Mweight property with value a symbol of name "bold", and #Mstyle
    property with value a symbol of name "italic".  The other
    properties are not specified.  An M-text that has this face is
    drawn with a font of bold weight and italic style.  */

/***ja
    @brief ボールドイタリックフェース

    変数 #mface_bold_italic は、#Mweight プロパテイの値が "bold" とい
    う名前をもつシンボルであり、かつ #Mstyle プロパテイの値が "italic" 
    という名前をもつシンボルであるようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text は、ボール
    ドイタリック体で表示される。
    */

MFace *mface_bold_italic;

/***en
    @brief Smallest face

    The variable #mface_xx_small points to a face that has the #Mratio
    property with value 50.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    50% of a normal font.  */

/***ja
    @brief 最小のフェース

    変数 #mface_xx_small は、#Mratio プロパティの値が 50 であるフェー
    スを指すポインタである。他のプロパティは指定されない。このフェース
    を持つ M-text は標準の 50% の大きさのフォントを用いて表示される。
     */

MFace *mface_xx_small;

/***en
    @brief Smaller face

    The variable #mface_x_small points to a face that has the #Mratio
    property with value 66.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    66% of a normal font.  */

/***ja
    @brief もっと小さいフェース

    変数 #mface_x_small は、#Mratio プロパティの値が 66 であるフェー
    スを指すポインタである。他のプロパティは指定されない。このフェース
    を持つ M-text は標準の 66% の大きさのフォントを用いて表示される。
     */

MFace *mface_x_small;

/***en
    @brief Small face

    The variable #mface_x_small points to a face that has the #Mratio
    property with value 75.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    75% of a normal font.  */

/***ja
    @brief 小さいフェース

    変数 #mface_small は、#Mratio プロパティの値が 75 であるフェースを
    指すポインタである。他のプロパティは指定されない。このフェースを持
    つ M-text は標準の 75% の大きさのフォントを用いて表示される。
     */

MFace *mface_small;

/***en
    @brief Normalsize face

    The variable #mface_normalsize points to a face that has the
    #Mratio property with value 100.  The other properties are not
    specified.  An M-text that has this face is drawn with a font
    whose size is the same as a normal font.  */

/***ja
    @brief 標準の大きさのフェース

    変数 #mface_normalsize は、#Mratio プロパティの値が 100 であるフェー
    スを指すポインタである。他のプロパティは指定されない。このフェース
    を持つ M-text は標準と同じ大きさのフォントを用いて表示される。
     */

MFace *mface_normalsize;

/***en
    @brief Large face

    The variable #mface_large points to a face that has the #Mratio
    property with value 120.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    120% of a normal font.  */

/***ja
    @brief 大きいフェース

    変数 #mface_large は、#Mratio プロパティの値が 120 であるフェース
    を指すポインタである。他のプロパティは指定されない。このフェースを
    持つ M-text は標準の 120% の大きさのフォントを用いて表示される。
     */

MFace *mface_large;

/***en
    @brief Larger face

    The variable #mface_x_large points to a face that has the #Mratio
    property with value 150.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    150% of a normal font.  */

/***ja
    @brief もっと大きいフェース

    変数 #mface_x_large は、#Mratio プロパティの値が 150 であるフェー
    スを指すポインタである。他のプロパティは指定されない。このフェース
    を持つ M-text は標準の 150% の大きさのフォントを用いて表示される。
     */

MFace *mface_x_large;

/***en
    @brief Largest face

    The variable #mface_xx_large points to a face that has the #Mratio
    property with value 200.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    200% of a normal font.  */

/***ja
    @brief 最大のフェース

    変数 #mface_xx_large は、#Mratio プロパティの値が 200 であるフェー
    スを指すポインタである。他のプロパティは指定されない。このフェース
    を持つ M-text は標準の 200% の大きさのフォントを用いて表示される。
     */

MFace *mface_xx_large;

/***en
    @brief Black face

    The variable #mface_black points to a face that has the
    #Mforeground property with value a symbol of name "black".  The
    other properties are not specified.  An M-text that has this face
    is drawn with black foreground.  */

/***ja @brief 黒フェース

    変数 #mface_black は、#Mforeground プロパティの値として "black" と
    いう名前のシンボルを持つようなフェースを指すポインタである。他のプ
    ロパティは指定されない。このフェースを持つ M-text は前景色を黒とし
    て表示される。     */

MFace *mface_black;

/***en
    @brief White face

    The variable #mface_white points to a face that has the
    #Mforeground property with value a symbol of name "white".  The
    other properties are not specified.  An M-text that has this face
    is drawn with white foreground.  */

/***ja
    @brief 白フェース

    変数 #mface_white は、#Mforeground プロパティの値として "white" と
    いう名前のシンボルを持つようなフェースを指すポインタである。他のプ
    ロパティは指定されない。このフェースを持つ M-text は前景色を白とし
    て表示される。  */

MFace *mface_white;

/***en
    @brief Red face

    The variable #mface_red points to a face that has the
    #Mforeground property with value a symbol of name "red".  The
    other properties are not specified.  An M-text that has this face
    is drawn with red foreground.  */

/***ja
    @brief 赤フェース

    変数 #mface_red は、#Mforeground プロパティの値として "red" という
    名前のシンボルを持つようなフェースを指すポインタである。他のプロパ
    ティは指定されない。このフェースを持つ M-text は前景色を赤として表
    示される。  */

MFace *mface_red;

/***en
    @brief Green face

    The variable #mface_green points to a face that has the
    #Mforeground property with value a symbol of name "green".  The
    other properties are not specified.  An M-text that has this face
    is drawn with green foreground.  */

/***ja
    @brief 緑フェース

    変数 #mface_green は、#Mforeground プロパティの値として "green" と
    いう名前のシンボルを持つようなフェースを指すポインタである。他のプ
    ロパティは指定されない。このフェースを持つ M-text は前景色を緑とし
    て表示される。  */

MFace *mface_green;

/***en
    @brief Blue face

    The variable #mface_blue points to a face that has the
    #Mforeground property with value a symbol of name "blue".  The
    other properties are not specified.  An M-text that has this face
    is drawn with blue foreground.  */

/***ja
    @brief 青フェース

    変数 #mface_blue は、#Mforeground プロパティの値として "blue" とい
    う名前のシンボルを持つようなフェースを指すポインタである。他のプロ
    パティは指定されない。このフェースを持つ M-text は前景色を青として
    表示される。  */

MFace *mface_blue;

/***en
    @brief Cyan face

    The variable #mface_cyan points to a face that has the
    #Mforeground property with value a symbol of name "cyan".  The
    other properties are not specified.  An M-text that has this face
    is drawn with cyan foreground.  */

/***ja
    @brief シアンフェース

    変数 #mface_cyan は、#Mforeground プロパティの値として "cyan" とい
    う名前のシンボルを持つようなフェースを指すポインタである。他のプロ
    パティは指定されない。このフェースを持つ M-text は前景色をシアンと
    して表示される。  */

MFace *mface_cyan;

/***en
    @brief yellow face

    The variable #mface_yellow points to a face that has the
    #Mforeground property with value a symbol of name "yellow".  The
    other properties are not specified.  An M-text that has this face
    is drawn with yellow foreground.  */

/***ja
    @brief 黄フェース

    変数 #mface_yellow は、#Mforeground プロパティの値として "yellow" 
    という名前のシンボルを持つようなフェースを指すポインタである。他の
    プロパティは指定されない。このフェースを持つ M-text は前景色を黄色
    として表示される。  */

MFace *mface_yellow;

/***en
    @brief Magenta face

    The variable #mface_magenta points to a face that has the
    #Mforeground property with value a symbol of name "magenta".  The
    other properties are not specified.  An M-text that has this face
    is drawn with magenta foreground.  */

/***ja
    @brief マゼンタフェース

    変数 #mface_magenta は、#Mforeground プロパティの値として 
    "magenta" という名前のシンボルを持つようなフェースを指すポインタで
    ある。他のプロパティは指定されない。このフェースを持つ M-text は前
    景色をマゼンタとして表示される。  */

MFace *mface_magenta;

/*** @} */
/*=*/

/***en @name Variables: The other symbols for face handling.  */
/***ja @name 変数: フェースを取り扱うためのその他のシンボル  */
/*** @{ */
/*=*/

/***en
    @brief Key of a text property specifying a face.

    The variable #Mface is a symbol of name <tt>"face"</tt>.  A text
    property whose key is this symbol must have a pointer to an object
    of type #MFace.  This is a managing key.  */

/***ja
    @brief フェースを指定するテキストプロパティのキー

    変数 #Mface は <tt>"face"</tt> という名前を持つシンボルである。こ
    のシンボルをキーとするテキストプロパティは、#MFace 型のオブジェク
    トへのポインタを持たなければならない。これは管理キーである。  */

MSymbol Mface;
/*=*/
/*** @} */
/*=*/

/***en
    @brief Create a new face.

    The mface () function creates a new face object that specifies no
    property.

    @return
    This function returns a pointer to the created face.  */

/***ja
    @brief 新しいフェースをつくる 

    関数 mface () はプロパティを一切持たない新しいフェースオブジェクト
    を作る。

    @return
    この関数は作ったフェースへのポインタを返す。  */

MFace *
mface ()
{
  MFace *face;

  M17N_OBJECT (face, free_face, MERROR_FACE);
  M17N_OBJECT_REGISTER (face_table, face);
  return face;
}

/*=*/

/***en
    @brief Make a copy of a face.

    The mface_copy () function makes a copy of $FACE and returns a
    pointer to the created copy.  */

/***ja
    @brief フェースのコピーを作る.

    関数 mface_copy () はフェース $FACE のコピーを作り、そのコピーへの
    ポインタを返す。  */

MFace *
mface_copy (MFace *face)
{
  MFace *copy;

  MSTRUCT_CALLOC (copy, MERROR_FACE);
  *copy = *face;
  copy->control.ref_count = 1;
  M17N_OBJECT_REGISTER (face_table, copy);
  if (copy->property[MFACE_FONTSET])
    M17N_OBJECT_REF (copy->property[MFACE_FONTSET]);
  if (copy->property[MFACE_HLINE])
    {
      MFaceHLineProp *val;

      MSTRUCT_MALLOC (val, MERROR_FACE); 
      *val = *((MFaceHLineProp *) copy->property[MFACE_HLINE]);
      copy->property[MFACE_HLINE] = val;
    }
  if (copy->property[MFACE_BOX])
    {
      MFaceBoxProp *val;

      MSTRUCT_MALLOC (val, MERROR_FACE); 
      *val = *((MFaceBoxProp *) copy->property[MFACE_BOX]);
      copy->property[MFACE_BOX] = val;
    }

  return copy;
}

/*=*/
/***en
    @brief Merge faces.

    The mface_merge () functions merges the properties of face $SRC
    into $DST.

    @return
    This function returns $DST.  */

/***ja
    @brief フェースを統合する.

    関数 mface_merge () は、フェース $SRC のプロパティをフェース $DST 
    に統合する。

    @return
    この関数は $DST を返す。  */

MFace *
mface_merge (MFace *dst, MFace *src)
{
  int i;

  for (i = 0; i < MFACE_PROPERTY_MAX; i++)
    if (src->property[i])
      {
	dst->property[i] = src->property[i];
	if (i == MFACE_FONTSET)
	  M17N_OBJECT_REF (dst->property[i]);
	else if (i == MFACE_HLINE)
	  {
	    MFaceHLineProp *val;

	    MSTRUCT_MALLOC (val, MERROR_FACE); 
	    *val = *((MFaceHLineProp *) dst->property[MFACE_HLINE]);
	    dst->property[MFACE_HLINE] = val;
	  }
	else if (i == MFACE_BOX)
	  {
	    MFaceBoxProp *val;

	    MSTRUCT_MALLOC (val, MERROR_FACE); 
	    *val = *((MFaceBoxProp *) dst->property[MFACE_BOX]);
	    dst->property[MFACE_BOX] = val;
	  }
      }
  return dst;
}

/*=*/

/***en
    @brief Make a face from a font.

    The mface_from_font () function return a newly created face while
    reflecting the properties of $FONT in its properties.   */

/***ja
    @brief フォントからフェースを作る.

    関数 mface_from_font () はフォント $FONT のプロパティをプロパティ
    として持つ新しいフェースを作り、それを返す。  */

MFace *
mface_from_font (MFont *font)
{
  MFace *face = mface ();

  face->property[MFACE_FOUNDRY] = mfont_get_prop (font, Mfoundry);
  face->property[MFACE_FAMILY] = mfont_get_prop (font, Mfamily);
  face->property[MFACE_WEIGHT] = mfont_get_prop (font, Mweight);
  face->property[MFACE_STYLE] = mfont_get_prop (font, Mstyle);
  face->property[MFACE_STRETCH] = mfont_get_prop (font, Mstretch);
  face->property[MFACE_ADSTYLE] = mfont_get_prop (font, Madstyle);
  face->property[MFACE_SIZE] = mfont_get_prop (font, Msize);
  return face;
}

/*=*/

/***en
    @brief Get the value of a face property.

    The mface_get_prop () function returns the value of the face
    property whose key is $KEY in face $FACE.  $KEY must be one of the
    followings:

        #Mforeground, #Mbackground, #Mvideomode, #Mhline, #Mbox,
        #Mfoundry, #Mfamily, #Mweight, #Mstyle, #Mstretch, #Madstyle,
        #Msize, #Mfontset, #Mratio, #Mhook_func, #Mhook_arg

    @return
    The actual type of the returned value depends of $KEY.  See
    documentation of the above keys.  If an error is detected, it
    returns @c NULL and assigns an error code to the external variable
    #merror_code.  */

/***ja
    @brief フェースのプロパティの値を得る

    関数 mface_get_prop () は、フェース $FACE が持つフェースプロパティ
    の内、キーが $KEY であるものの値を返す。$KEY は下記のいずれかでな
    ければならない。

        #Mforeground, #Mbackground, #Mvideomode, #Mhline, #Mbox,
        #Mfoundry, #Mfamily, #Mweight, #Mstyle, #Mstretch, #Madstyle,
        #Msize, #Mfontset, #Mratio, #Mhook_func, #Mhook_arg

    @return 
    戻り値の型は $KEY に依存する。上記のキーの説明を参照するこ
    と。エラーが検出された場合は @c NULL を返し、外部変数 #merror_code 
    にエラーコードを設定する。  */

/***
    @seealso
    mface_put_prop ()

    @errors
    @c MERROR_FACE  */

void *
mface_get_prop (MFace *face, MSymbol key)
{
  int index = (int) msymbol_get (key, M_face_prop_index) - 1;

  if (index < 0)
    MERROR (MERROR_FACE, NULL);
  return face->property[index];
}

/*=*/

/***en
    @brief Set a value of a face property.

    The mface_put_prop () function assigns $VAL to the property whose
    key is $KEY in face $FACE.  $KEY must be one the followings:

        #Mforeground, #Mbackground, #Mvideomode, #Mhline, #Mbox,
        #Mfoundry, #Mfamily, #Mweight, #Mstyle, #Mstretch, #Madstyle,
        #Msize, #Mfontset, #Mratio, #Mhook_func, #Mhook_arg

    Among them, font related properties (#Mfoundry through #Msize) are
    used as the default values when a font in the fontset of $FACE
    does not specify those values.

    The actual type of the returned value depends of $KEY.  See
    documentation of the above keys.

    @return
    If the operation was successful, mface_put_prop returns () 0.
    Otherwise it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief フェースプロパティの値を設定する

    関数 mface_put_prop () は、フェース $FACE 内でキーが $KEY であるプ
    ロパティの値を $VAL に設定する。$KEY は以下のいずれかでなくてはな
    らない。

        #Mforeground, #Mbackground, #Mvideomode, #Mhline, #Mbox,
        #Mfoundry, #Mfamily, #Mweight, #Mstyle, #Mstretch, #Madstyle,
        #Msize, #Mfontset, #Mratio, #Mhook_func, #Mhook_arg.

    これらのうちの、フォント関連のプロパティ (#Mfamily から #Msize 
    まで) は、フェースのフォントセット中のフォントに関するデフォルト値
    となり、個々のフォントが値を指定しなかった場合に用いられる。

    戻り値の型は $KEY に依存する。上記のキーの説明を参照すること。

    @return
    処理が成功した場合、mface_put_prop () は 0 を返す。失敗した場合は 
    -1 を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @seealso
    mface_get_prop ()

    @errors
    @c MERROR_FACE  */

int
mface_put_prop (MFace *face, MSymbol key, void *val)
{
  int index = (int) msymbol_get (key, M_face_prop_index) - 1;

  if (index < 0)
    MERROR (MERROR_FACE, -1);
  if (key == Mfontset)
    M17N_OBJECT_REF (val);
  else if (key == Mhline)
    {
      MFaceHLineProp *newval;

      MSTRUCT_MALLOC (newval, MERROR_FACE);
      *newval = *((MFaceHLineProp *) val);
      val = newval;
    }
  else if (key == Mbox)
    {
      MFaceBoxProp *newval;

      MSTRUCT_MALLOC (newval, MERROR_FACE);
      *newval = *((MFaceBoxProp *) val);
      val = newval;
    }
  face->property[index] = val;
  face->tick++;

  return 0;
}

/*=*/

/***en
    @brief Update a face.

    The mface_update () function update face $FACE on frame $FRAME by
    calling a hook function of $FACE (if any).  */

/***ja
    @brief フェースを更新する.

    関数 mface_update () はフレーム $FRAME のフェース $FACE を $FACE 
    のフック関数を（あれば）呼んで更新する。  */

void
mface_update (MFrame *frame, MFace *face)
{
  MFaceHookFunc func = (MFaceHookFunc) face->property[MFACE_HOOK_FUNC];
  MPlist *rface_list;
  MRealizedFace *rface;

  if (func)
    {
      MPLIST_DO (rface_list, frame->realized_face_list)
	{
	  rface = MPLIST_VAL (rface_list);
	  if ((MFaceHookFunc) rface->face.property[MFACE_HOOK_FUNC] == func)
	    (func) (&(rface->face), rface->face.property[MFACE_HOOK_ARG],
		    rface->info);
	}
    }
}
/*=*/

/*** @} */
/*=*/

/*** @addtogroup m17nDebug */
/*** @{  */
/*=*/

/***en
    @brief Dump a face.

    The mdebug_dump_face () function prints face $FACE in a human readable
    way to the stderr.  $INDENT specifies how many columns to indent
    the lines but the first one.

    @return
    This function returns $FACE.  */

/***ja
    @brief フェースをダンプする.

    関数 mdebug_dump_face () はフェース $FACE を stderr に人間に可読な
    形で印刷する。 $INDENT は２行目以降のインデントを指定する。

    @return
    この関数は $FACE を返す。  */

MFace *
mdebug_dump_face (MFace *face, int indent)
{
  char *prefix = (char *) alloca (indent + 1);
  MFont spec;

  memset (prefix, 32, indent);
  prefix[indent] = 0;
  mfont__set_spec_from_face (&spec, face);
  fprintf (stderr, "(face font:\"");
  mdebug_dump_font (&spec);
  fprintf (stderr, "\"\n %s  fore:%s back:%s", prefix,
	   msymbol_name ((MSymbol) face->property[MFACE_FOREGROUND]),
	   msymbol_name ((MSymbol) face->property[MFACE_BACKGROUND]));
  if (face->property[MFACE_FONTSET])
    fprintf (stderr, " non-default-fontset");
  fprintf (stderr, " hline:%s", face->property[MFACE_HLINE] ? "yes" : "no");
  fprintf (stderr, " box:%s)", face->property[MFACE_BOX] ? "yes" : "no");
  return face;
}

/*** @} */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
