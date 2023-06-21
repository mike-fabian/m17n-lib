/* face.c -- face module.
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

/***en
    @addtogroup m17nFace Face
    @brief A face is an object to control appearance of M-text.

    A @e face is an object of the type #MFace and controls how to
    draw M-texts.  A face has a fixed number of @e face @e properties.
    Like other types of properties, a face property consists of a key
    and a value.  A key is one of the following symbols:

    #Mforeground, #Mbackground, #Mvideomode, #Mhline, #Mbox,
    #Mfoundry, #Mfamily, #Mweight, #Mstyle, #Mstretch, #Madstyle,
    #Msize, #Mfontset, #Mratio, #Mhook_func, #Mhook_arg

    The notation "xxx property of F" means the face property that
    belongs to face F and whose key is @c Mxxx.

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
    @addtogroup m17nFace Face
    @brief フェースとは、M-text の見栄えを制御するオブジェクトである.

    @e フェース は #MFace 型のオブジェクトであり、M-text 
    の表示方法を制御する。フェースは固定個の @e フェースプロパティ を持つ。
    他のプロパティ同様フェースプロパティはキーと値からなり、キーは以下のシンボルのいずれかである。

    #Mforeground, #Mbackground, #Mvideomode, #Mhline, #Mbox, 
    #Mfoundry, #Mfamily, #Mweight, #Mstyle, #Mstretch, #Madstyle, 
    #Msize, #Mfontset, #Mratio, #Mhook_func, #Mhook_arg

    「フェース F のフェースプロパティのうちキーが @c Mxxx 
    であるもの」のことを簡単に「F の xxx プロパティ」と呼ぶことがある。

    M-text の表示関数は、まず最初にその M-text からキーがシンボル 
    #Mface であるようなテキストプロパティを探し、次にその値に従って 
    M-text を表示する。この値はフェースオブジェクトへのポインタでなければならない。

    M-text が、#Mface 
    をキーとするテキストプロパティを複数持っており、かつそれらの値が衝突しないならば、フェース情報は組み合わされて用いられる。

    あるテキスト属性がどのフェースによっても指定されていない場合は、デフォルトフェースの値が用いられる。  */

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

MSymbol Mlatin;

static MSymbol M_face_prop_index;

static MPlist *hline_prop_list;
static MPlist *box_prop_list;

/** Special hook function pointer that does nothing.  */
static MFaceHookFunc noop_hook;

/**  */
static MFaceHLineProp *
get_hline_create (MFaceHLineProp *prop)
{
  MPlist *plist;
  MFaceHLineProp *hline;

  if (prop->width == 0)
    return MPLIST_VAL (hline_prop_list);
  MPLIST_DO (plist, MPLIST_NEXT (hline_prop_list))
    {
      hline = MPLIST_VAL (plist);
      if (prop->type == hline->type
	  && prop->width == hline->width
	  && prop->color == hline->color)
	return hline;
    }
  MSTRUCT_MALLOC (hline, MERROR_FACE);
  *hline = *prop;
  mplist_push (plist, Mt, hline);
  return hline;
}

static MFaceBoxProp *
get_box_create (MFaceBoxProp *prop)
{
  MPlist *plist;
  MFaceBoxProp *box;

  if (prop->width == 0)
    return MPLIST_VAL (box_prop_list);
  MPLIST_DO (plist, MPLIST_NEXT (box_prop_list))
    {
      box = MPLIST_VAL (plist);
      if (prop->width == box->width
	  && prop->color_top == box->color_top
	  && prop->color_bottom == box->color_bottom
	  && prop->color_left == box->color_left
	  && prop->color_right == box->color_right
	  && prop->inner_hmargin == box->inner_hmargin
	  && prop->inner_vmargin == box->inner_vmargin
	  && prop->outer_hmargin == box->inner_hmargin
	  && prop->inner_vmargin == box->inner_vmargin)
	return box;
    }
  MSTRUCT_MALLOC (box, MERROR_FACE);
  *box = *prop;
  mplist_push (plist, Mt, box);
  return box;
}

/** From FRAME->realized_face_list, find a realized face based on
    FACE.  */

static MRealizedFace *
find_realized_face (MFrame *frame, MFace *face, MFont *font)
{
  MPlist *plist;

  MPLIST_DO (plist, frame->realized_face_list)
    {
      MRealizedFace *rface = MPLIST_VAL (plist);

      if (memcmp (rface->face.property, face->property,
		  sizeof face->property) == 0
	  && (rface->font
	      ? (font && ! memcmp (rface->font, font, sizeof (MFont)))
	      : ! font))
	return rface;
    }
  return NULL;
}

static void
free_face (void *object)
{
  MFace *face = (MFace *) object;

  if (face->property[MFACE_FONTSET])
    M17N_OBJECT_UNREF (face->property[MFACE_FONTSET]);
  M17N_OBJECT_UNREF (face->frame_list);
  M17N_OBJECT_UNREGISTER (face_table, face);
  free (object);
}


static MPlist *
serialize_hline (MPlist *plist, MFaceHLineProp *hline)
{
  if (hline->width > 0)
    {
      MPlist *pl = mplist ();

      mplist_add (pl, Minteger, (void *) hline->type);
      mplist_add (pl, Minteger, (void *) hline->width);
      mplist_add (pl, Msymbol, hline->color);
      plist = mplist_add (plist, Mplist, pl);
      M17N_OBJECT_UNREF (pl);
    }
  return plist;
}

static MPlist *
serialize_box (MPlist *plist, MFaceBoxProp *box)
{
  if (box->width > 0)
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
    }
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
  } serializer[MFACE_RATIO + 1]
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
	  { &Mratio,		&Minteger } };
  
  for (i = 0; i <= MFACE_RATIO; i++)
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
      if (index < 0 || index > MFACE_RATIO)
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
  MFaceHLineProp *hline;
  MFaceBoxProp *box;

  M17N_OBJECT_ADD_ARRAY (face_table, "Face");
  Mface = msymbol_as_managing_key (" face");
  msymbol_put_func (Mface, Mtext_prop_serializer,
		    M17N_FUNC (serialize_face));
  msymbol_put_func (Mface, Mtext_prop_deserializer,
		    M17N_FUNC (deserialize_face));

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
    } mface_prop_data[MFACE_HOOK_ARG + 1] =
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
	  { &Mratio,		MFACE_RATIO },
	  { &Mhook_arg,		MFACE_HOOK_ARG } };

    for (i = 0; i < MFACE_PROPERTY_MAX; i++)
      /* We add one to distinguish it from no-property.  */
      msymbol_put (*mface_prop_data[i].key, M_face_prop_index,
		   (void *) (mface_prop_data[i].index + 1));
  }

  hline_prop_list = mplist ();
  MSTRUCT_CALLOC (hline, MERROR_FACE);
  mplist_push (hline_prop_list, Mt, hline);
  box_prop_list = mplist ();
  MSTRUCT_CALLOC (box, MERROR_FACE);
  mplist_push (box_prop_list, Mt, box);

  mface__default = mface ();
  mface__default->property[MFACE_FOUNDRY] = msymbol ("misc");
  mface__default->property[MFACE_FAMILY] = msymbol ("fixed");
  mface__default->property[MFACE_WEIGHT] = msymbol ("medium");
  mface__default->property[MFACE_STYLE] = msymbol ("r");
  mface__default->property[MFACE_STRETCH] = msymbol ("normal");
  mface__default->property[MFACE_ADSTYLE] = msymbol ("");
  mface__default->property[MFACE_SIZE] = (void *) 120;
  mface__default->property[MFACE_FONTSET] = mfontset (NULL);
  mface__default->property[MFACE_FOREGROUND] = msymbol ("black");
  mface__default->property[MFACE_BACKGROUND] = msymbol ("white");
  mface__default->property[MFACE_HLINE] = hline;
  mface__default->property[MFACE_BOX] = box;
  mface__default->property[MFACE_VIDEOMODE] = Mnormal;
  mface__default->hook = noop_hook;

  mface_normal_video = mface ();
  mface_normal_video->property[MFACE_VIDEOMODE] = (void *) Mnormal;

  mface_reverse_video = mface ();
  mface_reverse_video->property[MFACE_VIDEOMODE] = (void *) Mreverse;

  {
    MFaceHLineProp hline_prop;

    hline_prop.type = MFACE_HLINE_UNDER;
    hline_prop.width = 1;
    hline_prop.color = Mnil;
    mface_underline = mface ();
    mface_put_prop (mface_underline, Mhline, &hline_prop);
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
  MPlist *plist;

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

  MPLIST_DO (plist, hline_prop_list)
    free (MPLIST_VAL (plist));
  M17N_OBJECT_UNREF (hline_prop_list);
  MPLIST_DO (plist, box_prop_list)
    free (MPLIST_VAL (plist));
  M17N_OBJECT_UNREF (box_prop_list);

  free (work_gstring.glyphs);
}

/** Return a face realized from NUM number of base faces pointed by
    FACES on the frame FRAME.  If SIZE is nonzero, it specifies the
    maximum font size.  */

MRealizedFace *
mface__realize (MFrame *frame, MFace **faces, int num, int size, MFont *font)
{
  MRealizedFace *rface;
  MRealizedFont *rfont;
  MFace merged_face = *(frame->face);
  int i, j;
  MFaceHookFunc func;
  MFont spec;

  if (num == 0 && frame->rface && ! font)
    return frame->rface;

  if (! mplist_find_by_value (frame->face->frame_list, frame))
    mplist_push (frame->face->frame_list, Mt, frame);
  for (i = 0; i < num; i++)
    if (! mplist_find_by_value (faces[i]->frame_list, frame))
      mplist_push (faces[i]->frame_list, Mt, frame);

  for (i = 0; i < MFACE_PROPERTY_MAX; i++)
    for (j = num - 1; j >= 0; j--)
      if (faces[j]->property[i])
	{
	  merged_face.property[i] = faces[j]->property[i];
	  break;
	}

  if (font)
    {
      if (font->type != MFONT_TYPE_REALIZED)
	font = mfont_copy (font);
      for (i = 0; i <= MFACE_ADSTYLE; i++)
	if (font->property[i])
	  merged_face.property[i] = FONT_PROPERTY (font, i);
      if (font->size)
	{
	  int font_size;

	  if (font->size < 0)
	    font->size = ((double) (- font->size)) * frame->dpi / 72.27 + 0.5;
	  font_size = font->size;
	  merged_face.property[MFACE_SIZE] = (void *) font_size;
	  merged_face.property[MFACE_RATIO] = (void *) 0;
	}
    }

  if (! font || ! font->size)
    {
      double font_size = (int) merged_face.property[MFACE_SIZE];
      int ifont_size;

      if (font_size < 0)
	font_size = - font_size * frame->dpi / 72.27;
      if (merged_face.property[MFACE_RATIO]
	  && (int) merged_face.property[MFACE_RATIO] != 100)
	{
	  font_size *= (int) merged_face.property[MFACE_RATIO];
	  font_size /= 100;
	}
      ifont_size = font_size + 0.5;
      merged_face.property[MFACE_SIZE] = (void *) ifont_size;
      merged_face.property[MFACE_RATIO] = (void *) 0;
    }

  merged_face.property[MFACE_FOUNDRY] = Mnil;
  rface = find_realized_face (frame, &merged_face, font);
  if (rface)
    {
      if (font && font->type != MFONT_TYPE_REALIZED)
	free (font);
      return rface;
    }

  MSTRUCT_CALLOC (rface, MERROR_FACE);
  mplist_push (frame->realized_face_list, Mt, rface);
  rface->frame = frame;
  rface->face = merged_face;
  rface->font = font;

  if (font)
    {
      if (font->type == MFONT_TYPE_SPEC)
	rfont = (MRealizedFont *) mfont_find (frame, font, NULL, 0);
      else if (font->type == MFONT_TYPE_OBJECT)
	{
	  MFONT_INIT (&spec);
	  spec.size = (int) merged_face.property[MFONT_SIZE];
	  if (font->property[MFONT_REGISTRY])
	    spec.property[MFONT_REGISTRY] = font->property[MFONT_REGISTRY];
	  else
	    mfont_put_prop (&spec, Mregistry,
			    (font->source == MFONT_SOURCE_X
			     ? Miso8859_1 : Municode_bmp));
	  rfont = mfont__open (frame, font, &spec);
	}
      else
	rfont = (MRealizedFont *) font;
    }
  else
    {
      MFontset *fontset = (MFontset *) merged_face.property[MFACE_FONTSET];

      rface->rfontset = mfont__realize_fontset (frame, fontset, &merged_face,
						font);
      rfont = NULL;
      mfont__set_spec_from_face (&spec, &merged_face);
      mfont_put_prop (&spec, Mregistry, Municode_bmp);
      spec.source = MFONT_SOURCE_FT;
      font = mfont__select (frame, &spec, 0);
      if (font)
	rfont = mfont__open (frame, font, &spec);
      if (! rfont)
	{
	  mfont_put_prop (&spec, Mregistry, Miso8859_1);
	  spec.source = MFONT_SOURCE_X;
	  font = mfont__select (frame, &spec, 0);
	  if (font)
	    rfont = mfont__open (frame, font, &spec);
	}
      if (! rfont)
	{
	  num = 0;
	  rfont = mfont__lookup_fontset (rface->rfontset, NULL, &num,
					 Mlatin, Mnil, Mnil, size, 0);
	}
    }

  if (rfont)
    {
      rface->rfont = rfont;
      rface->layouter = rfont->layouter;
      rfont->layouter = Mnil;
      work_gstring.glyphs[0].rface = rface;
      work_gstring.glyphs[0].g.code = MCHAR_INVALID_CODE;
      work_gstring.glyphs[0].g.measured = 0;
      mfont__get_metric (&work_gstring, 0, 1);
      rface->ascent = work_gstring.glyphs[0].g.ascent;
      rface->descent = work_gstring.glyphs[0].g.descent;
      work_gstring.glyphs[0].g.code
	= mfont__encode_char (frame, (MFont *) rfont, NULL, ' ');
      if (work_gstring.glyphs[0].g.code != MCHAR_INVALID_CODE)
	{
	  work_gstring.glyphs[0].g.measured = 0;
	  mfont__get_metric (&work_gstring, 0, 1);
	  rface->space_width = work_gstring.glyphs[0].g.xadv;
	}
      else
	rface->space_width = rfont->spec.size / 10;
      if (rfont->average_width)
	rface->average_width = rfont->average_width >> 6;
      else
	{
	  work_gstring.glyphs[0].g.code
	    = mfont__encode_char (frame, (MFont *) rfont, NULL, 'x');
	  if (work_gstring.glyphs[0].g.code != MCHAR_INVALID_CODE)
	    {
	      work_gstring.glyphs[0].g.measured = 0;
	      mfont__get_metric (&work_gstring, 0, 1);
	      rface->average_width = work_gstring.glyphs[0].g.xadv;
	    }
	  else
	    rface->average_width = rface->space_width;
	}
    }
  else
    {
      rface->rfont = NULL;
      rface->space_width = frame->space_width;
    }

  rface->hline = (MFaceHLineProp *) merged_face.property[MFACE_HLINE];
  if (rface->hline && rface->hline->width == 0)
    rface->hline = NULL;
  rface->box = (MFaceBoxProp *) merged_face.property[MFACE_BOX];
  if (rface->box && rface->box->width == 0)
    rface->box = NULL;
  rface->ascii_rface = rface;
  (*frame->driver->realize_face) (rface);

  func = rface->face.hook;
  if (func && func != noop_hook)
    (func) (&(rface->face), rface->info, rface->face.property[MFACE_HOOK_ARG]);

  rface->non_ascii_list = mplist ();
  if (rface->rfont)
    {
      MRealizedFace *nofont;

      MSTRUCT_CALLOC (nofont, MERROR_FACE);
      *nofont = *rface;
      nofont->non_ascii_list = NULL;
      nofont->rfont = NULL;
      mplist_add (rface->non_ascii_list, Mt, nofont);
    }

  return rface;
}


MGlyph *
mface__for_chars (MSymbol script, MSymbol language, MSymbol charset,
		  MGlyph *from_g, MGlyph *to_g, int size)
{
  MRealizedFont *rfont = from_g->rface->rfont;
  MSymbol layouter;
  int num = to_g - from_g;
  int i;

  if (from_g->rface->font)
    {
      MRealizedFace *rface = from_g->rface, *new;

      if (! rfont)
	rfont = mfontset__get_font (rface->frame,
				    rface->face.property[MFACE_FONTSET], 
				    script, language,
				    rface->font, NULL);
      else if (script != Mlatin)
	rfont = mfontset__get_font (rface->frame,
				    rface->face.property[MFACE_FONTSET],
				    script, language,
				    (MFont *) rfont, NULL);
      if (! rfont)
	{
	  for (; from_g < to_g && from_g->rface->font; from_g++)
	    from_g->g.code = MCHAR_INVALID_CODE;
	}
      else
	{
	  if (rface->rfont == rfont && rfont->layouter == Mnil)
	    new = rface;
	  else
	    {
	      MSTRUCT_MALLOC (new, MERROR_FACE);
	      mplist_push (rface->non_ascii_list, Mt, new);
	      *new = *rface;
	      new->rfont = rfont;
	      new->layouter = rfont->layouter;
	      rfont->layouter = Mnil;
	      new->non_ascii_list = NULL;
	      new->ascent = rfont->ascent >> 6;
	      new->descent = rfont->descent >> 6;
	    } 
	  for (; from_g < to_g && from_g->rface->font; from_g++)
	    {
	      from_g->rface = new;
	      if (new->layouter)
		{
		  MFLT *flt = mflt_get (new->layouter);
		  MCharTable *coverage;

		  if (! flt
		      || ((coverage = mflt_coverage (flt))
			  && ! (from_g->g.code
				= (unsigned) mchartable_lookup (coverage,
								from_g->g.c))))
		    {
		      from_g->rface = rface;
		      from_g->g.code = mfont__encode_char (rfont->frame, 
							   (MFont *) rfont,
							   NULL, from_g->g.c);
		    }
		}
	      else
		from_g->g.code = mfont__encode_char (rfont->frame, 
						     (MFont *) rfont,
						     NULL, from_g->g.c);
	    }
	}
      return from_g;
    }

  if (rfont && script == Mlatin)
    {
      for (i = 0; i < num; i++)
	{
	  unsigned code = mfont__encode_char (rfont->frame, (MFont *) rfont,
					      NULL, from_g[i].g.c);
	  if (code == MCHAR_INVALID_CODE)
	    break;
	  from_g[i].g.code = code;
	}
      if (i == num || from_g[i].rface->font)
	return from_g + i;
    }

  rfont = mfont__lookup_fontset (from_g->rface->rfontset, from_g, &num,
				 script, language, charset, size, 0);
  if (rfont)
    {
      layouter = rfont->layouter;
      rfont->layouter = Mnil;
    }
  else
    {
      from_g->g.code = MCHAR_INVALID_CODE;
      num = 1;
      rfont = NULL;
      layouter = Mnil;
    }
  
  to_g = from_g + num;
  while (from_g < to_g)
    {
      MGlyph *g = from_g;
      MRealizedFace *rface = from_g++->rface;

      while (from_g < to_g && rface == from_g->rface) from_g++;
      if (rface->rfont != rfont
	  || rface->layouter != layouter)
	{
	  MPlist *plist = mplist_find_by_value (rface->non_ascii_list, rfont);
	  MRealizedFace *new = NULL;

	  while (plist)
	    {
	      new = MPLIST_VAL (plist);
	      if (new->layouter == layouter)
		break;
	      plist = mplist_find_by_value (MPLIST_NEXT (plist), rfont);
	    }
	  if (! plist)
	    {
	      MSTRUCT_MALLOC (new, MERROR_FACE);
	      mplist_push (rface->non_ascii_list, Mt, new);
	      *new = *rface;
	      new->rfont = rfont;
	      new->layouter = layouter;
	      new->non_ascii_list = NULL;
	      if (rfont)
		{
		  new->ascent = rfont->ascent >> 6;
		  new->descent = rfont->descent >> 6;
		}
	    }
	  while (g < from_g)
	    g++->rface = new;
	}
    }
  return to_g;
}


void
mface__free_realized (MRealizedFace *rface)
{
  MPlist *plist;

  MPLIST_DO (plist, rface->non_ascii_list)
    free (MPLIST_VAL (plist));
  M17N_OBJECT_UNREF (rface->non_ascii_list);
  if (rface->font && rface->font->type != MFONT_TYPE_REALIZED)
    free (rface->font);
  free (rface);
}

void
mface__update_frame_face (MFrame *frame)
{
  frame->rface = NULL;
  frame->rface = mface__realize (frame, NULL, 0, 0, NULL);
  frame->space_width = frame->rface->space_width;
  frame->average_width = frame->rface->average_width;
  frame->ascent = frame->rface->ascent;
  frame->descent = frame->rface->descent;
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API  */
/*** @addtogroup m17nFace Face */
/*** @{ */
/*=*/

/***en @name Variables: Keys of face property */
/***ja @name 変数: フェースプロパティのキー  */
/*** @{ */
/*=*/

/***en
    @brief Key of a face property specifying foreground color.

    The variable #Mforeground is used as a key of face property.  The
    property value must be a symbol whose name is a color name, or
    #Mnil.

    #Mnil means that the face does not specify a foreground color.
    Otherwise, the foreground of an M-text is drawn by the specified
    color.  */
/***ja
    @brief 前景色を指定するフェースプロパティーのキー.

    変数 #Mforeground はフェースプロパティのキーとして用いられる。
    プロパティの値は、色名を名前として持つシンボルか #Mnil である。

    #Mnil の場合、前景色は指定されない。そうでなければ M-text 
    の前景は指定された色で表示される。  */

MSymbol Mforeground;

/***en
    @brief Key of a face property specifying background color.

    The variable #Mbackground is used as a key of face property.  The
    property value must be a symbol whose name is a color name, or
    #Mnil.

    #Mnil means that the face does not specify a background color.
    Otherwise, the background of an M-text is drawn by the specified
    color.  */
/***ja
    @brief 背景色を指定するためのフェースプロパティーのキー.

    変数 #Mbackground はフェースプロパティのキーとして用いられる。
    プロパティの値は、色名を名前として持つシンボルか #Mnil である。

    #Mnil の場合、背景色は指定されない。そうでなければ M-text 
    の背景は指定された色で表示される。  */

MSymbol Mbackground;

/***en
    @brief Key of a face property specifying video mode.

    The variable #Mvideomode is used as a key of face property.  The
    property value must be @b Mnormal, @b Mreverse, or #Mnil.

    @b Mnormal means that an M-text is drawn in normal video mode
    (i.e. the foreground is drawn by foreground color, the background
    is drawn by background color).

    @b Mreverse means that an M-text is drawn in reverse video mode
    (i.e. the foreground is drawn by background color, the background
    is drawn by foreground color).

    #Mnil means that the face does not specify a video mode.  */
/***ja
    @brief ビデオモードを指定するためのフェースプロパティーのキー.

    変数 #Mvideomode はフェースプロパティのキーとして用いられる。プロパティの値は、
    @b Mnormal, @b Mreverse, #Mnil のいずれかでなくてはならない。

    @b Mnormal の場合は、M-text 
    を標準のビデオモード（前景を前景色で、背景を背景色で）で表示する。

    @b Mreverse の場合はリバースビデオモードで（前景を背景色で、背景を前景色で）表示する。

    #Mnil の場合はビデオモードは指定されない。
    */

MSymbol Mvideomode;

/***en
    @brief Key of a face property specifying font size ratio.

    The variable #Mratio is used as a key of face property.  The value
    RATIO must be an integer.

    The value 0 means that the face does not specify a font size
    ratio.  Otherwise, an M-text is drawn by a font of size (FONTSIZE
    * RATIO / 100) where FONTSIZE is a font size specified by the face
    property #Msize.  */
/***ja
    @brief フォントのサイズの比率を指定するためのフェースプロパティーのキー.

    変数 #Mratio はフェースプロパティのキーとして用いられる。値 RATIO 
    は整数値でなくてはならない。

    値が0ならば、フォントサイズは指定されない。そうでなければ、M-text 
    は(FONTSIZE * RATIO / 100) というサイズのフォントで表示される。
    FONTSIZE はフェースプロパティー#Msize で指定されたサイズである。 */

MSymbol Mratio;

/***en
    @brief Key of a face property specifying horizontal line.

    The variable #Mhline is used as a key of face property.  The value
    must be a pointer to an object of type #MFaceHLineProp, or @c
    NULL.

    The value @c NULL means that the face does not specify this
    property.  Otherwise, an M-text is drawn with a horizontal line by
    a way specified by the object that the value points to.  */
/***ja
    @brief 水平線を指定するためのフェースプロパティーのキー.

    変数 #Mhline はフェースプロパティのキーとして用いられる。値は 
    #MFaceHLineProp 型オブジェクトへのポインタか @c NULL でなくてはならない。

    値が @c NULL ならば、このプロパティは指定されない。
    そうでなければ値が指すオブジェクトに指定されたように水平線を付加して M-text 
    を表示する。*/

MSymbol Mhline;

/***en
    @brief Key of a face property specifying box.

    The variable #Mbox is used as a key of face property.  The value
    must be a pointer to an object of type #MFaceBoxProp, or @c NULL.

    The value @c NULL means that the face does not specify a box.
    Otherwise, an M-text is drawn with a surrounding box by a way
    specified by the object that the value points to.  */
/***ja
    @brief 囲み枠を指定するためのフェースプロパティーのキー.

    変数 #Mbox はフェースプロパティのキーとして用いられる。値は 
    #MFaceBoxProp 型オブジェクトへのポインタか @c NULL でなくてはならない。

    値が @c NULL ならば、このフェースは囲み枠を指定していない。
    そうでなければ値が指すオブジェクトに指定されたように囲み枠を付加して 
    M-text を表示する。*/

MSymbol Mbox;

/***en
    @brief Key of a face property specifying fontset.

    The variable #Mfontset is used as a key of face property.  The
    value must be a pointer to an object of type #Mfontset, or @c
    NULL.

    The value @c NULL means that the face does not specify a fontset.
    Otherwise, an M-text is drawn with a font selected from what
    specified in the fontset.  */
/***ja
    @brief フォントセットを指定するためのフェースプロパティーのキー.

    変数 #Mfontset はフェースプロパティのキーとして用いられる。値は 
    #Mfontset 型オブジェクトへのポインタか @c NULL でなくてはならない。

    値が @c NULL ならば、フォントセットは指定されていない。
    そうでなければ値が指すオブジェクトに指定されたフォントセットから選んだフォントで 
    M-text を表示する。*/
    
MSymbol Mfontset;

/***en
    @brief Key of a face property specifying hook.

    The variable #Mhook_func is used as a key of face property.  The
    value must be a function of type #MFaceHookFunc, or @c NULL.

    The value @c NULL means that the face does not specify a hook.
    Otherwise, the specified function is called before the face is
    realized.  */
/***ja
    @brief フックを指定するためのフェースプロパティーのキー.

    変数 #Mhook_func はフェースプロパティのキーとして用いられる。値は 
    #MFaceHookFunc 型の関数か @c NULL でなくてはならない。

    値が @c NULL ならば、フックは指定されていない。
    そうでなければフェースを実現する前に指定された関数が呼ばれる。      */
MSymbol Mhook_func;

/***en
    @brief Key of a face property specifying argument of hook.

    The variable #Mhook_arg is used as a key of face property.  The
    value can be anything that is passed a hook function specified by
    the face property #Mhook_func.  */
/***ja
    @brief フックの引数を指定するためのフェースプロパティーのキー.

    変数 #Mhook_arg はフェースプロパティのキーとして用いられる。
    値は何でもよく、フェースプロパティ #Mhook_func で指定される関数に渡される。 */
MSymbol Mhook_arg;

/*** @} */
/*=*/

/*** @ingroup m17nFace */
/***en @name Variables: Possible values of #Mvideomode property of face */
/***ja @name 変数：  フェースの #Mvideomode プロパティの可能な値 */
/*** @{ */

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
    @brief Normal video face.

    The variable #mface_normal_video points to a face that has the
    #Mvideomode property with value @b Mnormal.  The other properties
    are not specified.  An M-text drawn with this face appear normal
    colors (i.e. the foreground is drawn by foreground color, and
    background is drawn by background color).  */
/***ja
    @brief 標準ビデオフェース.

    変数 #mface_normal_video は #Mvideomode プロパティの値が @b Mnormal 
    であるフェースを指すポインタである。他のプロパティは指定されない。
    このフェースで表示されるM-text 
    は標準の色 (すなわち前景は前景色、背景は背景色）で描かれる。  */

MFace *mface_normal_video;

/***en
    @brief Reverse video face.

    The variable #mface_reverse_video points to a face that has the
    #Mvideomode property with value @b Mreverse.  The other properties
    are not specified.  An M-text drawn with this face appear in
    reversed colors (i.e. the foreground is drawn by background
    color, and background is drawn by foreground color).  */
/***ja
    @brief リバースビデオフェース.

    変数 #mface_reverse_video は #Mvideomode プロパティの値が 
    @b Mreverse であるフェースを指すポインタである。他のプロパティは指定されない。
    このフェースで表示されるM-text 
    は前景色と背景色が入れ替わって (すなわち前景は背景色、背景は前景色）描かれる。  */

MFace *mface_reverse_video;

/***en
    @brief Underline face.

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
    @brief 下線フェース.

    変数 #mface_underline は #Mhline プロパテイの値が #MFaceHLineProp 
    型オブジェクトへのポインタであるフェースを指すポインタである。オブジェクトのメンバは以下の通り。

@verbatim
    メンバ  値
    -----   -----
    type    MFACE_HLINE_UNDER
    width   1
    color   Mnil
@endverbatim

    他のプロパティは指定されない。このフェースを持つ M-text は下線付きで表示される。*/ 

MFace *mface_underline;

/***en
    @brief Medium face.

    The variable #mface_medium points to a face that has the #Mweight
    property with value a symbol of name "medium".  The other
    properties are not specified.  An M-text that has this face is
    drawn with a font of medium weight.  */
/***ja
    @brief ミディアムフェース.

    変数 #mface_medium は #Mweight プロパテイの値が "medium" 
    という名前をもつシンボルであるようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text
    は、ミディアムウェイトのフォントで表示される。  */
MFace *mface_medium;

/***en
    @brief Bold face.

    The variable #mface_bold points to a face that has the #Mweight
    property with value a symbol of name "bold".  The other properties
    are not specified.  An M-text that has this face is drawn with a
    font of bold weight.  */
/***ja
    @brief ボールドフェース.

    変数 #mface_bold は #Mweight プロパテイの値が "bold" 
    という名前をもつシンボルであるようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text 
    は、ボールドフォントで表示される。    */

MFace *mface_bold;

/***en
    @brief Italic face.

    The variable #mface_italic points to a face that has the #Mstyle
    property with value a symbol of name "italic".  The other
    properties are not specified.  An M-text that has this face is
    drawn with a font of italic style.  */
/***ja
    @brief イタリックフェース.

    変数 #mface_italic は #Mstyle プロパテイの値が "italic" 
    という名前をもつシンボルであるようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text
    は、イタリック体で表示される。    */

MFace *mface_italic;

/***en
    @brief Bold italic face.

    The variable #mface_bold_italic points to a face that has the
    #Mweight property with value a symbol of name "bold", and #Mstyle
    property with value a symbol of name "italic".  The other
    properties are not specified.  An M-text that has this face is
    drawn with a font of bold weight and italic style.  */
/***ja
    @brief ボールドイタリックフェース.

    変数 #mface_bold_italic は、#Mweight プロパテイの値が "bold" 
    という名前をもつシンボルであり、かつ #Mstyle プロパテイの値が "italic" 
    という名前をもつシンボルであるようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text 
    は、ボールドイタリック体で表示される。    */

MFace *mface_bold_italic;

/***en
    @brief Smallest face.

    The variable #mface_xx_small points to a face that has the #Mratio
    property with value 50.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    50% of a normal font.  */
/***ja
    @brief 最小のフェース.

    変数 #mface_xx_small は、#Mratio プロパティの値が 50 
    であるフェースを指すポインタである。他のプロパティは指定されない。
    このフェースを持つ M-text は標準フォントの 50% の大きさのフォントを用いて表示される。
     */

MFace *mface_xx_small;

/***en
    @brief Smaller face.

    The variable #mface_x_small points to a face that has the #Mratio
    property with value 66.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    66% of a normal font.  */
/***ja
    @brief より小さいフェース.

    変数 #mface_x_small は、#Mratio プロパティの値が 66 
    であるフェースを指すポインタである。他のプロパティは指定されない。
    このフェースを持つ M-text は標準フォントの 66% の大きさのフォントを用いて表示される。
     */

MFace *mface_x_small;

/***en
    @brief Small face.

    The variable #mface_x_small points to a face that has the #Mratio
    property with value 75.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    75% of a normal font.  */
/***ja
    @brief 小さいフェース.

    変数 #mface_small は、#Mratio プロパティの値が 75 
    であるフェースを指すポインタである。他のプロパティは指定されない。
    このフェースを持つ M-text は標準フォントの 75% の大きさのフォントを用いて表示される。
     */

MFace *mface_small;

/***en
    @brief Normalsize face.

    The variable #mface_normalsize points to a face that has the
    #Mratio property with value 100.  The other properties are not
    specified.  An M-text that has this face is drawn with a font
    whose size is the same as a normal font.  */
/***ja
    @brief 標準の大きさのフェース.

    変数 #mface_normalsize は、#Mratio プロパティの値が 100 
    であるフェースを指すポインタである。他のプロパティは指定されない。
    このフェースを持つ M-text は標準フォントと同じ大きさのフォントを用いて表示される。
     */

MFace *mface_normalsize;

/***en
    @brief Large face.

    The variable #mface_large points to a face that has the #Mratio
    property with value 120.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    120% of a normal font.  */
/***ja
    @brief 大きいフェース.

    変数 #mface_large は、#Mratio プロパティの値が 120 
    であるフェースを指すポインタである。他のプロパティは指定されない。
    このフェースを持つ M-text は標準フォントの 120% の大きさのフォントを用いて表示される。
     */

MFace *mface_large;

/***en
    @brief Larger face.

    The variable #mface_x_large points to a face that has the #Mratio
    property with value 150.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    150% of a normal font.  */
/***ja
    @brief もっと大きいフェース.

    変数 #mface_x_large は、#Mratio プロパティの値が 150 
    であるフェースを指すポインタである。他のプロパティは指定されない。
    このフェースを持つ M-text は標準フォントの 150% の大きさのフォントを用いて表示される。
     */

MFace *mface_x_large;

/***en
    @brief Largest face.

    The variable #mface_xx_large points to a face that has the #Mratio
    property with value 200.  The other properties are not specified.
    An M-text that has this face is drawn with a font whose size is
    200% of a normal font.  */
/***ja
    @brief 最大のフェース.

    変数 #mface_xx_large は、#Mratio プロパティの値が 200 
    であるフェースを指すポインタである。他のプロパティは指定されない。
    このフェースを持つ M-text は標準フォントの 200% の大きさのフォントを用いて表示される。
     */

MFace *mface_xx_large;

/***en
    @brief Black face.

    The variable #mface_black points to a face that has the
    #Mforeground property with value a symbol of name "black".  The
    other properties are not specified.  An M-text that has this face
    is drawn with black foreground.  */
/***ja 
    @brief 黒フェース.

    変数 #mface_black は、#Mforeground プロパティの値として "black" 
    という名前のシンボルを持つようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text 
    は前景色として黒を用いて表示される。     */

MFace *mface_black;

/***en
    @brief White face.

    The variable #mface_white points to a face that has the
    #Mforeground property with value a symbol of name "white".  The
    other properties are not specified.  An M-text that has this face
    is drawn with white foreground.  */
/***ja
    @brief 白フェース.

    変数 #mface_white は、#Mforeground プロパティの値として "white" 
    という名前のシンボルを持つようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text 
    は前景色として白を用いて表示される。     */

MFace *mface_white;

/***en
    @brief Red face.

    The variable #mface_red points to a face that has the
    #Mforeground property with value a symbol of name "red".  The
    other properties are not specified.  An M-text that has this face
    is drawn with red foreground.  */
/***ja
    @brief 赤フェース.

    変数 #mface_red は、#Mforeground プロパティの値として "red" 
    という名前のシンボルを持つようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text 
    は前景色として赤を用いて表示される。  */

MFace *mface_red;

/***en
    @brief Green face.

    The variable #mface_green points to a face that has the
    #Mforeground property with value a symbol of name "green".  The
    other properties are not specified.  An M-text that has this face
    is drawn with green foreground.  */
/***ja
    @brief 緑フェース.

    変数 #mface_green は、#Mforeground プロパティの値として "green" 
    という名前のシンボルを持つようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text 
    は前景色として緑を用いて表示される。  */

MFace *mface_green;

/***en
    @brief Blue face.

    The variable #mface_blue points to a face that has the
    #Mforeground property with value a symbol of name "blue".  The
    other properties are not specified.  An M-text that has this face
    is drawn with blue foreground.  */
/***ja
    @brief 青フェース.

    変数 #mface_blue は、#Mforeground プロパティの値として "blue" 
    という名前のシンボルを持つようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text 
    は前景色として青を用いて表示される。  */

MFace *mface_blue;

/***en
    @brief Cyan face.

    The variable #mface_cyan points to a face that has the
    #Mforeground property with value a symbol of name "cyan".  The
    other properties are not specified.  An M-text that has this face
    is drawn with cyan foreground.  */
/***ja
    @brief シアンフェース.

    変数 #mface_cyan は、#Mforeground プロパティの値として "cyan"
    という名前のシンボルを持つようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text 
    は前景色としてシアンを用いて表示される。  */

MFace *mface_cyan;

/***en
    @brief yellow face.

    The variable #mface_yellow points to a face that has the
    #Mforeground property with value a symbol of name "yellow".  The
    other properties are not specified.  An M-text that has this face
    is drawn with yellow foreground.  */

/***ja
    @brief 黄フェース.

    変数 #mface_yellow は、#Mforeground プロパティの値として "yellow" 
    という名前のシンボルを持つようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text 
    は前景色として黄色を用いて表示される。  */

MFace *mface_yellow;

/***en
    @brief Magenta face.

    The variable #mface_magenta points to a face that has the
    #Mforeground property with value a symbol of name "magenta".  The
    other properties are not specified.  An M-text that has this face
    is drawn with magenta foreground.  */

/***ja
    @brief マゼンタフェース.

    変数 #mface_magenta は、#Mforeground プロパティの値として 
    "magenta" という名前のシンボルを持つようなフェースを指すポインタである。
    他のプロパティは指定されない。このフェースを持つ M-text 
    は前景色としてマゼンタを用いて表示される。  */

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
    @brief フェースを指定するテキストプロパティのキー.

    変数 #Mface は <tt>"face"</tt> 
    という名前を持つシンボルである。このシンボルをキーとするテキストプロパティは、
    #MFace 型のオブジェクトへのポインタを持たなければならない。
    これは管理キーである。  */

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
    @brief 新しいフェースをつくる.

    関数 mface () 
    はプロパティを一切持たない新しいフェースオブジェクトを作る。

    @return
    この関数は作ったフェースへのポインタを返す。  */

MFace *
mface ()
{
  MFace *face;

  M17N_OBJECT (face, free_face, MERROR_FACE);
  face->frame_list = mplist ();
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

    関数 mface_copy () はフェース $FACE 
    のコピーを作り、そのコピーへのポインタを返す。  */

MFace *
mface_copy (MFace *face)
{
  MFace *copy;

  MSTRUCT_CALLOC (copy, MERROR_FACE);
  *copy = *face;
  copy->control.ref_count = 1;
  M17N_OBJECT_REGISTER (face_table, copy);
  copy->frame_list = mplist ();
  if (copy->property[MFACE_FONTSET])
    M17N_OBJECT_REF (copy->property[MFACE_FONTSET]);
  return copy;
}

/*=*/
/***en
    @brief Compare faces.

    The mface_equal () function compares faces $FACE1 and $FACE2.

    @return
    If two faces have the same property values, return 1.
    Otherwise return 0.  */

int
mface_equal (MFace *face1, MFace *face2)
{
  MFaceHLineProp *hline1, *hline2;
  MFaceBoxProp *box1, *box2;
  int i;

  if (face1 == face2)
    return 1;
  if (memcmp (face1->property, face2->property, sizeof face1->property) == 0)
    return 1;
  for (i = MFACE_FOUNDRY; i <= MFACE_BACKGROUND; i++)
    if (face1->property[i] != face2->property[i])
      return 0;
  for (i = MFACE_VIDEOMODE; i <= MFACE_RATIO; i++)
    if (face1->property[i] != face2->property[i])
      return 0;
  hline1 = (MFaceHLineProp *) face1->property[MFACE_HLINE];
  hline2 = (MFaceHLineProp *) face2->property[MFACE_HLINE];
  if (hline1 != hline2)
    {
      if (! hline1 || ! hline2)
	return 0;
      if (memcmp (hline1, hline2, sizeof (MFaceHLineProp)) != 0)
	return 0;
    }
  box1 = (MFaceBoxProp *) face1->property[MFACE_BOX];
  box2 = (MFaceBoxProp *) face2->property[MFACE_BOX];
  if (box1 != box2)
    {
      if (! box1 || ! box2)
	return 0;
      if (memcmp (box1, box2, sizeof (MFaceBoxProp)) != 0)
	return 0;
    }
  return 1;
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
  MPlist *plist;

  for (i = 0; i < MFACE_PROPERTY_MAX; i++)
    if (src->property[i])
      {
	if (i == MFACE_FONTSET)
	  {
	    M17N_OBJECT_UNREF (dst->property[i]);
	    M17N_OBJECT_REF (src->property[i]);
	  }
	dst->property[i] = src->property[i];
      }

  MPLIST_DO (plist, dst->frame_list)
    {
      MFrame *frame = MPLIST_VAL (plist);

      frame->tick++;
      if (dst == frame->face)
	mface__update_frame_face (frame);
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

    関数 mface_from_font () はフォント $FONT 
    のプロパティをプロパティとして持つ新しいフェースを作り、それを返す。  */

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
    @brief フェースのプロパティの値を得る.

    関数 mface_get_prop () は、フェース $FACE 
    が持つフェースプロパティの内、キーが $KEY であるものの値を返す。
    $KEY は下記のいずれかでなければならない。

        #Mforeground, #Mbackground, #Mvideomode, #Mhline, #Mbox,
        #Mfoundry, #Mfamily, #Mweight, #Mstyle, #Mstretch, #Madstyle,
        #Msize, #Mfontset, #Mratio, #Mhook_arg

    @return 
    戻り値の型は $KEY に依存する。上記のキーの説明を参照すること。
    エラーが検出された場合は @c NULL を返し、外部変数 #merror_code 
    にエラーコードを設定する。  */

/***
    @seealso
    mface_put_prop (), mface_put_hook ()

    @errors
    @c MERROR_FACE  */

void *
mface_get_prop (MFace *face, MSymbol key)
{
  int index = (int) msymbol_get (key, M_face_prop_index) - 1;

  if (index < 0)
    {
      if (key == Mhook_func)
	/* This unsafe code is for backward compatiblity.  */
	return (void *) face->hook;
      MERROR (MERROR_FACE, NULL);
    }
  return face->property[index];
}

/*=*/

/***en
    @brief Get the hook function of a face.

    The mface_get_hook () function returns the hook function of face
    $FACE.  */

/***ja
    @brief フェースのフック関数を得る.

    関数 mface_get_hook () はフェース $FACE のフック関数を返す。 */

MFaceHookFunc
mface_get_hook (MFace *face)
{
  return face->hook;
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
    If the operation was successful, mface_put_prop () returns 0.
    Otherwise it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief フェースプロパティの値を設定する.

    関数 mface_put_prop () は、フェース $FACE 内でキーが $KEY 
    であるプロパティの値を $VAL に設定する。$KEY 
    は以下のいずれかでなくてはならない。

        #Mforeground, #Mbackground, #Mvideomode, #Mhline, #Mbox,
        #Mfoundry, #Mfamily, #Mweight, #Mstyle, #Mstretch, #Madstyle,
        #Msize, #Mfontset, #Mratio, #Mhook_func, #Mhook_arg.

    これらのうちの、フォント関連のプロパティ (#Mfamily から #Msize 
    まで) は、フェースのフォントセット中のフォントに関するデフォルト値となり、個々のフォントが値を指定しなかった場合に用いられる。

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
  MPlist *plist;

  if (key == Mhook_func)
    {
      /* This unsafe code is for backward compatiblity.  */
      if (face->hook == (MFaceHookFunc) val)
	return 0;
      face->hook = (MFaceHookFunc) val;
    }
  else
    {
      if (index < 0)
	MERROR (MERROR_FACE, -1);
      if (key == Mfontset)
	{
	  if (face->property[index])
	    M17N_OBJECT_UNREF (face->property[index]);
	  M17N_OBJECT_REF (val);
	}
      else if (key == Mhline)
	val = get_hline_create (val);
      else if (key == Mbox)
	val = get_box_create (val);

      if (face->property[index] == val)
	return 0;
      face->property[index] = val;
    }

  MPLIST_DO (plist, face->frame_list)
    {
      MFrame *frame = MPLIST_VAL (plist);

      frame->tick++;
      if (face == frame->face)
	mface__update_frame_face (frame);
    }

  return 0;
}

/*=*/

/***en
    @brief Set a hook function to a face.

    The mface_set_hook () function sets the hook function of face
    $FACE to $FUNC.  */

/***ja
    @brief フェースのフック関数を設定する.

    関数 mface_set_hook () は、フェース $FACE のフック関数を$FUNC に設
    定する。  */

int
mface_put_hook (MFace *face, MFaceHookFunc func)
{
  if (face->hook != func)
    {
      MPlist *plist;
      face->hook = func;

      MPLIST_DO (plist, face->frame_list)
	{
	  MFrame *frame = MPLIST_VAL (plist);

	  frame->tick++;
	  if (face == frame->face)
	    mface__update_frame_face (frame);
	}
    }
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
  MFaceHookFunc func = face->hook;
  MPlist *rface_list;
  MRealizedFace *rface;

  if (func && func != noop_hook)
    {
      MPLIST_DO (rface_list, frame->realized_face_list)
	{
	  rface = MPLIST_VAL (rface_list);
	  if (rface->face.hook == func)
	    (func) (&(rface->face), rface->face.property[MFACE_HOOK_ARG],
		    rface->info);
	}
    }
}
/*=*/

/*** @} */
/*=*/

/*** @addtogroup m17nDebug Debugging */
/*** @{  */
/*=*/

/***en
    @brief Dump a face.

    The mdebug_dump_face () function prints face $FACE in a human
    readable way to the stderr or to what specified by the environment
    variable MDEBUG_OUTPUT_FILE.  $INDENT specifies how many columns
    to indent the lines but the first one.

    @return
    This function returns $FACE.  */

/***ja
    @brief フェースをダンプする.

    関数 mdebug_dump_face () はフェース $FACE を標準エラー出力もしくは
    環境変数 MDEBUG_DUMP_FONT で指定されたファイルに人間に可読な形で印
    刷する。 $INDENT は２行目以降のインデントを指定する。

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
  fprintf (mdebug__output, "(face font:\"");
  mdebug_dump_font (&spec);
  fprintf (mdebug__output, "\"\n %s  fore:%s back:%s", prefix,
	   msymbol_name ((MSymbol) face->property[MFACE_FOREGROUND]),
	   msymbol_name ((MSymbol) face->property[MFACE_BACKGROUND]));
  if (face->property[MFACE_FONTSET])
    fprintf (mdebug__output, " non-default-fontset");
  fprintf (mdebug__output, " hline:%s",
	   face->property[MFACE_HLINE] ? "yes" : "no");
  fprintf (mdebug__output, " box:%s)",
	   face->property[MFACE_BOX] ? "yes" : "no");
  return face;
}

/*** @} */

/*
  Local Variables:
  coding: utf-8
  End:
*/
