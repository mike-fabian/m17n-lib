/* m17n-X.c -- implementation of the GUI API on X Windows.
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

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <locale.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xlocale.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

#include "m17n-gui.h"
#include "m17n-X.h"
#include "m17n-misc.h"
#include "internal.h"
#include "internal-gui.h"
#include "symbol.h"
#include "input.h"
#include "font.h"
#include "fontset.h"
#include "face.h"

typedef struct
{
  /* Common header for the m17n object.  */
  M17NObject control;

  Display *display;

  /* If nonzero, <display> is opened by this library.  Thus it should
     be closed on freeing this structure.  */
  int auto_display;

  /** List of available fonts on the display (except for iso8859-1 and
      iso10646-1 fonts).  Keys are font registries, values are
      (MFontList *).  */
  MPlist *font_registry_list;

  MPlist *iso8859_1_family_list;

  MPlist *iso10646_1_family_list;

  /* List of information about each font.  Keys are font registries,
     values are (MFontInfo *).  */
  MPlist *realized_font_list;

 /** Modifier bit masks of the display.  */
  int meta_mask;
  int alt_mask;
  int super_mask;
  int hyper_mask;
} MDisplayInfo;

/* Anchor of the chain of MDisplayInfo objects.  */
static MPlist *display_info_list;

struct MWDevice
{
  /* Common header for the m17n object.  */
  M17NObject control;

  MDisplayInfo *display_info;

  int screen_num;

  Drawable drawable;

  unsigned depth;

  Colormap cmap;

  unsigned long foreground, background;

  /** List of pointers to realized faces on the frame.  */
  MPlist *realized_face_list;

  /** List of pointers to realized fontsets on the frame.  */
  MPlist *realized_fontset_list;

};

static MPlist *device_list;

static MSymbol M_iso8859_1, M_iso10646_1;

#define FRAME_DISPLAY(frame) (frame->device->display_info->display)
#define FRAME_SCREEN(frame) (frame->device->screen_num)

static void
free_display_info (void *object)
{
  MDisplayInfo *disp_info = (MDisplayInfo *) object;
  MPlist *plist;

  for (plist = disp_info->font_registry_list;
       mplist_key (plist) != Mnil; plist = mplist_next (plist))
    {
      MFontList *registry_list = mplist_value (plist);

      if (registry_list->fonts)
	free (registry_list->fonts);
      free (registry_list);
    }
  M17N_OBJECT_UNREF (disp_info->font_registry_list);

  for (plist = disp_info->iso8859_1_family_list;
       mplist_key (plist) != Mnil; plist = mplist_next (plist))
    {
      MFontList *family_list = mplist_value (plist);

      if (family_list->fonts)
	free (family_list->fonts);
      free (family_list);
    }
  M17N_OBJECT_UNREF (disp_info->iso8859_1_family_list);

  for (plist = disp_info->iso10646_1_family_list;
       mplist_key (plist) != Mnil; plist = mplist_next (plist))
    {
      MFontList *family_list = mplist_value (plist);

      if (family_list->fonts)
	free (family_list->fonts);
      free (family_list);
    }
  M17N_OBJECT_UNREF (disp_info->iso10646_1_family_list);

  for (plist = disp_info->realized_font_list;
       mplist_key (plist) != Mnil; plist = mplist_next (plist))
    mfont__free_realized ((MRealizedFont *) mplist_value (plist));
  M17N_OBJECT_UNREF (disp_info->realized_font_list);

  if (disp_info->auto_display)
    XCloseDisplay (disp_info->display);

  free (object);
}

static void
free_device (void *object)
{
  MWDevice *device = (MWDevice *) object;
  MPlist *plist;

  for (plist = device->realized_fontset_list;
       mplist_key (plist) != Mnil; plist = mplist_next (plist))
    mfont__free_realized_fontset ((MRealizedFontset *) mplist_value (plist));
  M17N_OBJECT_UNREF (device->realized_fontset_list);

  for (plist = device->realized_face_list;
       mplist_key (plist) != Mnil; plist = mplist_next (plist))
    mface__free_realized ((MRealizedFace *) mplist_value (plist));
  M17N_OBJECT_UNREF (device->realized_face_list);

  XFreePixmap (device->display_info->display, device->drawable);
  M17N_OBJECT_UNREF (device->display_info);
  free (object);
}


static void
find_modifier_bits (MDisplayInfo *disp_info)
{
  Display *display = disp_info->display;
  XModifierKeymap *mods;
  KeyCode meta_l = XKeysymToKeycode (display, XK_Meta_L);
  KeyCode meta_r = XKeysymToKeycode (display, XK_Meta_R);
  KeyCode alt_l = XKeysymToKeycode (display, XK_Alt_L);
  KeyCode alt_r = XKeysymToKeycode (display, XK_Alt_R);
  KeyCode super_l = XKeysymToKeycode (display, XK_Super_L);
  KeyCode super_r = XKeysymToKeycode (display, XK_Super_R);
  KeyCode hyper_l = XKeysymToKeycode (display, XK_Hyper_L);
  KeyCode hyper_r = XKeysymToKeycode (display, XK_Hyper_R);
  int i, j;

  mods = XGetModifierMapping (display);
  /* We skip the first three sets for Shift, Lock, and Control.  The
     remaining sets are for Mod1, Mod2, Mod3, Mod4, and Mod5.  */
  for (i = 3; i < 8; i++)
    for (j = 0; j < mods->max_keypermod; j++)
      {
	KeyCode code = mods->modifiermap[i * mods->max_keypermod + j];

	if (! code)
	  continue;
	if (code == meta_l || code == meta_r)
	  disp_info->meta_mask |= (1 << i);
	else if (code == alt_l || code == alt_r)
	  disp_info->alt_mask |= (1 << i);
	else if (code == super_l || code == super_r)
	  disp_info->super_mask |= (1 << i);
	else if (code == hyper_l || code == hyper_r)
	  disp_info->hyper_mask |= (1 << i);
      }

  /* If meta keys are not in any modifier, use alt keys as meta
     keys.  */
  if (! disp_info->meta_mask)
    {
      disp_info->meta_mask = disp_info->alt_mask;
      disp_info->alt_mask = 0;
    }
  /* If both meta and alt are assigned to the same modifier, give meta
     keys priority.  */
  if (disp_info->meta_mask & disp_info->alt_mask)
    disp_info->alt_mask &= ~disp_info->meta_mask;

  XFreeModifiermap (mods);
}

static unsigned long
get_color (Display *display, Colormap cmap,
	   MSymbol color_name, MSymbol default_name,
	   unsigned long default_pixel)
{
  XColor exact_def;

  if (XParseColor (display, cmap, msymbol_name (color_name), &exact_def)
      && XAllocColor (display, cmap, &exact_def))
    return exact_def.pixel;

  if (XParseColor (display, cmap, msymbol_name (default_name), &exact_def)
      && XAllocColor (display, cmap, &exact_def))
    return exact_def.pixel;

  return default_pixel;
}


/** X font handler */

/** Indices to each field of split font name.  */

enum xlfd_field_idx
  {
    XLFD_FOUNDRY,
    XLFD_FAMILY,
    XLFD_WEIGHT,
    XLFD_SLANT,
    XLFD_SWIDTH,
    XLFD_ADSTYLE,
    XLFD_PIXEL,
    XLFD_POINT,
    XLFD_RESX,
    XLFD_RESY,
    XLFD_SPACING,
    XLFD_AVGWIDTH,
    XLFD_REGISTRY,
    XLFD_ENCODING,
    /* anchor */
    XLFD_FIELD_MAX
  };

/** Split the fontname NAME into each XLFD field destructively.  Set
    each element of the table pointed by PROPERTY_IDX to a pointer to
    the corresponding font property name.  Store the point size and
    the resolution-Y of the font to the place pointed by POINT and
    RESY respectively.

    If NAME does not contain all XLFD fields, the unspecified fields is
    treated as wild cards.  */

static int
split_font_name (char *name, char **field,
		 unsigned short *size, unsigned short *resy)
{
  int i;
  char *p;

  for (i = 0, p = name; *p; p++)
    {
      *p = tolower (*p);
      if (*p == '-' && i < XLFD_FIELD_MAX)
	{
	  field[i] = p + 1;
	  if (i != XLFD_ENCODING)
	    *p = '\0';
	  i++;
	}
    }
  if (i < XLFD_REGISTRY)
    return -1;
  for (; i < XLFD_FIELD_MAX; i++)
    field[i] = "*";

  if (*(field[XLFD_RESY]) == '*')
    *resy = 0;
  else
    *resy = atoi (field[XLFD_RESY]);
  if (*(field[XLFD_PIXEL]) == '*')
    {
      if (*(field[XLFD_POINT]) != '*')
	*size = atoi (field[XLFD_POINT]) * *resy / 72;
      else
	*size = 0;
    }
  else if (*(field[XLFD_PIXEL]) == '[')
    {
      /* The pixel size field specifies a transformation matrix of the
	 form "[A B C D]".  The XLFD spec says that the scalar value N
	 for the pixel size is equivalent to D.  */
      char *p0 = field[XLFD_PIXEL] + 1, *p1;
      double d;

      for (i = 0; i < 4; i++, p0 = p1)
	d = strtod (p0, &p1);
      *size = d * 10;
    }
  else
    *size = atoi (field[XLFD_PIXEL]) * 10;
  if (*size == 0 && *(field[XLFD_POINT]) != '*')
    {
      *size = atoi (field[XLFD_POINT]);
      if (*resy)
	*size = *size * *resy / 72;
      else
	*size = *size * 100 / 72;
    }

  return 0;
}

static int
build_font_name (MFont *font, char *name, int limit)
{
  MSymbol prop[7];
  char *str[7];
  int len, i;
  unsigned short size, resy;

  prop[0] = (MSymbol) mfont_get_prop (font, Mfoundry);
  prop[1] = (MSymbol) mfont_get_prop (font, Mfamily);
  prop[2] = (MSymbol) mfont_get_prop (font, Mweight);
  prop[3] = (MSymbol) mfont_get_prop (font, Mstyle);
  prop[4] = (MSymbol) mfont_get_prop (font, Mstretch);
  prop[5] = (MSymbol) mfont_get_prop (font, Madstyle);
  prop[6] = (MSymbol) mfont_get_prop (font, Mregistry);
  for (len = 0, i = 0; i < 7; i++)
    {
      if (prop[i] != Mnil)
	{
	  str[i] = msymbol_name (prop[i]);
	  len += strlen (str[i]);
	}
      else
	{
	  str[i] = "*";
	  len++;
	}
    }
  if ((len
       + 12			/* 12 dashes */
       + 3			/* 3 asterisks */
       + 30			/* 3 integers (each 10 digits) */
       + 1)			/* '\0' terminal */
      > limit)
    return -1;

  size = (int) mfont_get_prop (font, Msize);
  if ((size % 10) < 5)
    size /= 10;
  else
    size = size / 10 + 1;
  resy = (int) mfont_get_prop (font, Mresolution);

  sprintf (name, "-%s-%s-%s-%s-%s-%s-%d-*-%d-%d-*-*-%s",
	   str[0], str[1], str[2], str[3], str[4], str[5],
	   size, resy, resy,  str[6]);
  return 0;
}

static MFontList *
build_font_list (MFrame *frame, MSymbol family, MSymbol registry,
		 MPlist *plist)
{
  char pattern[1024];
  MFontList *font_list;
  char **fontnames;
  int nfonts;
  int i, j;

  MSTRUCT_CALLOC (font_list, MERROR_WIN);

  if (family == Mnil)
    {
      sprintf (pattern, "-*-*-*-*-*-*-*-*-*-*-*-*-%s",
	       msymbol_name (registry));
      font_list->tag = registry;
    }
  else
    {
      sprintf (pattern, "-*-%s-*-*-*-*-*-*-*-*-*-*-%s",
	       msymbol_name (family), msymbol_name (registry));
      font_list->tag = family;
    }

  fontnames = XListFonts (FRAME_DISPLAY (frame), pattern, 0x8000, &nfonts);
  if (nfonts > 0)
    {
      MTABLE_MALLOC (font_list->fonts, nfonts, MERROR_WIN);
      for (i = j = 0; i < nfonts; i++)
	if (mwin__parse_font_name (fontnames[i], font_list->fonts + j) >= 0
	    && (font_list->fonts[j].property[MFONT_SIZE] != 0
		|| font_list->fonts[j].property[MFONT_RESY] == 0))
	  {
	    font_list->fonts[j].property[MFONT_TYPE] = MFONT_TYPE_WIN + 1;
	    j++;
	  }
      XFreeFontNames (fontnames);
      font_list->nfonts = j;
    }
  mplist_add (plist, font_list->tag, font_list);
  return (nfonts > 0 ? font_list : NULL);
}


static MRealizedFont *xfont_select (MFrame *, MFont *, MFont *, int);
static int xfont_open (MRealizedFont *);
static void xfont_close (MRealizedFont *);
static void xfont_find_metric (MRealizedFont *, MGlyph *);
static unsigned xfont_encode_char (MRealizedFont *, int, unsigned);
static void xfont_render (MDrawWindow, int, int, MGlyphString *,
			  MGlyph *, MGlyph *, int, MDrawRegion);

MFontDriver xfont_driver =
  { xfont_select, xfont_open, xfont_close,
    xfont_find_metric, xfont_encode_char, xfont_render };

/* The X font driver function SELECT.  */

static MRealizedFont *
xfont_select (MFrame *frame, MFont *spec, MFont *request, int limited_size)
{
  MSymbol registry = FONT_PROPERTY (spec, MFONT_REGISTRY);
  MRealizedFont *rfont;
  MFontList *font_list = NULL;
  int i;
  MFont *best_font;
  int best_score, score;

  if (registry == Mnil
      || ! strchr (MSYMBOL_NAME (registry), '-'))
    return NULL;

  /* We handles iso8859-1 and iso10646-1 fonts specially because there
     exists so many such fonts.  */
  if (registry == M_iso8859_1 || registry == M_iso10646_1)
    {
      MPlist *family_list
	= (registry == M_iso8859_1
	   ? frame->device->display_info->iso8859_1_family_list
	   : frame->device->display_info->iso10646_1_family_list);
      MSymbol family = FONT_PROPERTY (spec, MFONT_FAMILY);

      if (family != Mnil)
	{
	  font_list = (MFontList *) mplist_get (family_list, family);
	  if (! font_list)
	    font_list = build_font_list (frame, family, registry, family_list);
	}
      if (! font_list)
	{
	  family = FONT_PROPERTY (request, MFONT_FAMILY);
	  font_list = (MFontList *) mplist_get (family_list, family);
	  if (! font_list)
	    font_list = build_font_list (frame, family, registry, family_list);
	}
    }
  if (! font_list)
    {
      MPlist *registry_list
	= frame->device->display_info->font_registry_list;

      font_list = (MFontList *) mplist_get (registry_list, registry);
      if (! font_list)
	font_list = build_font_list (frame, Mnil, registry, registry_list);
    }
  if (! font_list)
    return NULL;

  for (i = 0, best_score = -1, best_font = NULL; i < font_list->nfonts; i++)
    if ((best_score = mfont__score (font_list->fonts + i, spec, request,
				    limited_size)) >= 0)
      break;
  if (best_score < 0)
    return NULL;
  best_font = font_list->fonts + i;
  for (; best_score > 0 && i < font_list->nfonts ; i++)
    {
      score = mfont__score (font_list->fonts + i, spec, request,
			    limited_size);
      if (score >= 0 && score < best_score)
	{
	  best_font = font_list->fonts + i;
	  best_score = score;
	}
    }

  MSTRUCT_CALLOC (rfont, MERROR_WIN);
  rfont->frame = frame;
  rfont->spec = *spec;
  rfont->request = *request;
  rfont->font = *best_font;
  if (best_font->property[MFONT_SIZE] == 0)
    rfont->font.property[MFONT_SIZE] = request->property[MFONT_SIZE];
  rfont->score = best_score;
  rfont->driver = &xfont_driver;
  return rfont;
}

typedef struct
{
  M17NObject control;
  MFrame *frame;
  XFontStruct *f;
} MXFontInfo;

static void
close_xfont (void *object)
{
  MXFontInfo *xfont = (MXFontInfo *) object;

  if (xfont->f)
    XFreeFont (FRAME_DISPLAY (xfont->frame), xfont->f);
  free (object);
}


/* The X font driver function OPEN.  */

static int
xfont_open (MRealizedFont *rfont)
{
  char name[1024];
  MXFontInfo *xfont;
  MFrame *frame = rfont->frame;
  int mdebug_mask = MDEBUG_FONT;

  /* This never fail to generate a valid fontname because open_spec
     should correspond to a font available on the system.  */
  build_font_name (&rfont->font, name, 1024);
  M17N_OBJECT (xfont, close_xfont, MERROR_WIN);
  rfont->info = xfont;
  xfont->frame = frame;
  xfont->f = XLoadQueryFont (FRAME_DISPLAY (frame), name);
  if (! xfont->f)
    {
      rfont->status = -1;
      MDEBUG_PRINT1 (" [XFONT] x %s\n", name);
      return -1;
    }
  MDEBUG_PRINT1 (" [XFONT] o %s\n", name);
  rfont->status = 1;
  rfont->ascent = xfont->f->ascent;
  rfont->descent = xfont->f->descent;
  return 0;
}


/* The X font driver function CLOSE.  */

static void
xfont_close (MRealizedFont *rfont)
{
  M17N_OBJECT_UNREF (rfont->info);
}

/* The X font driver function FIND_METRIC.  */

static void
xfont_find_metric (MRealizedFont *rfont, MGlyph *g)
{
  XCharStruct *pcm = NULL;
  MXFontInfo *xfont = (MXFontInfo *) rfont->info;
  XFontStruct *f = xfont->f;
  int byte1, byte2;

  if (g->code == MCHAR_INVALID_CODE)
    {
      g->lbearing = f->max_bounds.lbearing;
      g->rbearing = f->max_bounds.rbearing;
      g->width = f->max_bounds.width;
      g->ascent = f->ascent;
      g->descent = f->descent;
      return;
    }

  byte1 = g->code >> 8;
  byte2 = g->code & 0xFF;

  if (f->per_char != NULL)
    {
      if (f->min_byte1 == 0 && f->max_byte1 == 0)
	{
	  if (byte1 == 0
	      && byte2 >= f->min_char_or_byte2
	      && byte2 <= f->max_char_or_byte2)
	    pcm = f->per_char + byte2 - f->min_char_or_byte2;
	}
      else
	{
	  if (byte1 >= f->min_byte1
	      && byte1 <= f->max_byte1
	      && byte2 >= f->min_char_or_byte2
	      && byte2 <= f->max_char_or_byte2)
	    {
	      pcm = (f->per_char
		     + ((f->max_char_or_byte2-f->min_char_or_byte2 + 1)
			* (byte1 - f->min_byte1))
		     + (byte2 - f->min_char_or_byte2));
	    }
	}
    }

  if (pcm)
    {
      g->lbearing = pcm->lbearing;
      g->rbearing = pcm->rbearing;
      g->width = pcm->width;
      g->ascent = pcm->ascent;
      g->descent = pcm->descent;
    }
  else
    {
      /* If the per_char pointer is null, all glyphs between the first
	 and last character indexes inclusive have the same
	 information, as given by both min_bounds and max_bounds.  */
      g->lbearing = 0;
      g->rbearing = f->max_bounds.width;
      g->width = f->max_bounds.width;
      g->ascent = f->ascent;
      g->descent = f->descent;
    }
}


/* The X font driver function ENCODE_CHAR.  */

static unsigned
xfont_encode_char (MRealizedFont *rfont, int c, unsigned code)
{
  MXFontInfo *xfont;
  XFontStruct *f;
  unsigned min_byte1, max_byte1, min_byte2, max_byte2;
  int all_chars_exist;

  if (rfont->status < 0)
    return -1;
  if (rfont->status == 0)
    {
      if (xfont_open (rfont) < 0)
	return -1;
    }
  xfont = (MXFontInfo *) rfont->info;
  f = xfont->f;
  all_chars_exist = (! f->per_char || f->all_chars_exist == True);
  min_byte1 = f->min_byte1;
  max_byte1 = f->max_byte1;
  min_byte2 = f->min_char_or_byte2;
  max_byte2 = f->max_char_or_byte2;

  if (min_byte1 == 0 && max_byte1 == 0)
    {
      XCharStruct *pcm;

      if (all_chars_exist)
	return ((code >= min_byte2 && code <= max_byte2)
		? code : MCHAR_INVALID_CODE);
      pcm = f->per_char + (code - min_byte2);
      return ((pcm->width > 0 || pcm->rbearing != pcm->lbearing)
	      ? code : MCHAR_INVALID_CODE);
    }
  else
    {
      unsigned byte1 = code >> 8, byte2 = code & 0xFF;
      XCharStruct *pcm;

      if (all_chars_exist)
	return ((byte1 >= min_byte1 && byte1 <= max_byte1
		 && byte2 >= min_byte2 && byte2 <= max_byte2)
		? code : MCHAR_INVALID_CODE);
      pcm = f->per_char + ((byte1 - min_byte1) * (max_byte2 - min_byte2 + 1)
			   + (byte2 - min_byte2));
      return ((pcm->width > 0 || pcm->rbearing != pcm->lbearing)
	      ? code : MCHAR_INVALID_CODE);
    }
}

static GC
set_region (Display *display, MRealizedFace *rface, GC gc, MDrawRegion region)
{
  GC gc1;
  XRectangle xrect;

  XClipBox (region, &xrect);  
  gc1 = ((GC *) rface->info)[MFACE_GC_SCRATCH];
  XCopyGC (display, gc, GCFont | GCForeground | GCBackground, gc1);
  XSetRegion (display, gc1, region);
  return gc1;
}

/* The X font driver function RENDER.  */

static void
xfont_render (MDrawWindow win, int x, int y, MGlyphString *gstring,
	      MGlyph *from, MGlyph *to, int reverse, MDrawRegion region)
{
  MRealizedFace *rface = from->rface;
  Display *display;
  XChar2b *code;
  GC *gcs = rface->info;
  GC gc = gcs[reverse ? MFACE_GC_INVERSE : MFACE_GC_NORMAL];
  MGlyph *g;
  int i;

  if (from == to)
    return;

  /* It is assured that the all glyphs in the current range use the
     same realized face.  */
  display = FRAME_DISPLAY (rface->frame);

  if (region)
    gc = set_region (display, rface, gc, region);

  if (from->code == MCHAR_INVALID_CODE)
    {
      int x0 = x;

      for (; from < to; from++)
	{
	  XDrawRectangle (display, (Window) win, gc,
			  x0, y - gstring->ascent + 1, from->width - 1,
			  gstring->ascent + gstring->descent - 2);
	  x0 += from->width;
	}
      return;
    }

  code = (XChar2b *) alloca (sizeof (XChar2b) * (to - from));
  for (i = 0, g = from; g < to; i++, g++)
    {
      code[i].byte1 = g->code >> 8;
      code[i].byte2 = g->code & 0xFF;
    }

  g = from;
  while (g < to)
    {
      if (g->type == GLYPH_PAD)
	x += g++->width;
      else if (g->type == GLYPH_SPACE)
	for (; g < to && g->type == GLYPH_SPACE; g++)
	  x += g->width;
      else if (! g->rface->rfont)
	{
	  if ((g->c >= 0x200B && g->c <= 0x200F)
	      || (g->c >= 0x202A && g->c <= 0x202E))
	    x += g++->width;
	  else
	    {
	      /* As a font is not found for this character, draw an
		 empty box.  */
	      int box_width = g->width;
	      int box_height = gstring->ascent + gstring->descent;

	      if (box_width > 4)
		box_width -= 2;
	      if (box_height > 4)
		box_height -= 2;
	      XDrawRectangle (display, (Window) win, gc,
			      x, y - gstring->ascent, box_width, box_height);
	      x += g++->width;
	    }
	}
      else if (g->xoff != 0 || g->yoff != 0 || g->right_padding)
	{
	  XDrawString16 (display, (Window) win, gc,
			 x + g->xoff, y + g->yoff, code + (g - from), 1);
	  x += g->width;
	  g++;
	}
      else
	{
	  int orig_x = x;
	  int code_idx = g - from;

	  for (i = 0;
	       g < to && g->type == GLYPH_CHAR && g->xoff == 0 && g->yoff == 0;
	       i++, g++)
	      x += g->width;
	  XDrawString16 (display, (Window) win, gc, orig_x, y,
			 code + code_idx, i);
	}
    }
}



/* XIM (X Input Method) handler */

typedef struct MInputXIMMethodInfo
{
  Display *display;
  XIM xim;
  MSymbol language;
  MSymbol coding;
} MInputXIMMethodInfo;

typedef struct MInputXIMContextInfo
{
  XIC xic;
  Window win;
  MConverter *converter;
} MInputXIMContextInfo;

static int
xim_open_im (MInputMethod *im)
{
  MInputXIMArgIM *arg = (MInputXIMArgIM *) im->arg;
  MLocale *saved, *this;
  char *save_modifier_list;
  XIM xim;
  MInputXIMMethodInfo *im_info;

  saved = mlocale_set (LC_CTYPE, NULL);
  this = mlocale_set (LC_CTYPE, arg->locale ? arg->locale : "");
  if (! this)
    /* The specified locale is not supported.  */
    MERROR (MERROR_LOCALE, -1);
  if (mlocale_get_prop (this, Mcoding) == Mnil)
    {
      /* Unable to decode the output of XIM.  */
      mlocale_set (LC_CTYPE, msymbol_name (mlocale_get_prop (saved, Mname)));
      MERROR (MERROR_LOCALE, -1);
    }

  if (arg->modifier_list)
    save_modifier_list = XSetLocaleModifiers (arg->modifier_list);
  else
    save_modifier_list = XSetLocaleModifiers ("");
  if (! save_modifier_list)
    {
      /* The specified locale is not supported by X.  */
      mlocale_set (LC_CTYPE, msymbol_name (mlocale_get_prop (saved, Mname)));
      MERROR (MERROR_LOCALE, -1);
    }

  xim = XOpenIM (arg->display, arg->db, arg->res_name, arg->res_class);
  if (! xim)
    {
      /* No input method is available in the current locale.  */
      XSetLocaleModifiers (save_modifier_list);
      mlocale_set (LC_CTYPE, msymbol_name (mlocale_get_prop (saved, Mname)));
      MERROR (MERROR_WIN, -1);
    }

  MSTRUCT_MALLOC (im_info, MERROR_WIN);
  im_info->display = arg->display;
  im_info->xim = xim;
  im_info->language = mlocale_get_prop (this, Mlanguage);
  im_info->coding = mlocale_get_prop (this, Mcoding);
  im->info = im_info;

  XSetLocaleModifiers (save_modifier_list);
  mlocale_set (LC_CTYPE, msymbol_name (mlocale_get_prop (saved, Mname)));

  return 0;
}

static void
xim_close_im (MInputMethod *im)
{
  MInputXIMMethodInfo *im_info = (MInputXIMMethodInfo *) im->info;

  XCloseIM (im_info->xim);
  free (im_info);
}

static int
xim_create_ic (MInputContext *ic)
{
  MInputXIMArgIC *arg = (MInputXIMArgIC *) ic->arg;
  MInputXIMMethodInfo *im_info = (MInputXIMMethodInfo *) ic->im->info;
  MInputXIMContextInfo *ic_info;
  XIC xic;

  if (! arg->input_style)
    {
      /* By default, use Root style.  */
      arg->input_style = XIMPreeditNothing | XIMStatusNothing;
      arg->preedit_attrs = NULL;
      arg->status_attrs = NULL;
    }

  if (! arg->preedit_attrs && ! arg->status_attrs)
    xic = XCreateIC (im_info->xim,
		     XNInputStyle, arg->input_style,
		     XNClientWindow, arg->client_win,
		     XNFocusWindow, arg->focus_win,
		     NULL);
  else if (arg->preedit_attrs && ! arg->status_attrs)
    xic = XCreateIC (im_info->xim,
		     XNInputStyle, arg->input_style,
		     XNClientWindow, arg->client_win,
		     XNFocusWindow, arg->focus_win,
		     XNPreeditAttributes, arg->preedit_attrs,
		     NULL);
  else if (! arg->preedit_attrs && arg->status_attrs)
    xic = XCreateIC (im_info->xim,
		     XNInputStyle, arg->input_style,
		     XNClientWindow, arg->client_win,
		     XNFocusWindow, arg->focus_win,
		     XNStatusAttributes, arg->status_attrs,
		     NULL);
  else
    xic = XCreateIC (im_info->xim,
		     XNInputStyle, arg->input_style,
		     XNClientWindow, arg->client_win,
		     XNFocusWindow, arg->focus_win,
		     XNPreeditAttributes, arg->preedit_attrs,
		     XNStatusAttributes, arg->status_attrs,
		     NULL);
  if (! xic)
    MERROR (MERROR_WIN, -1);

  MSTRUCT_MALLOC (ic_info, MERROR_WIN);
  ic_info->xic = xic;
  ic_info->win = arg->focus_win;
  ic_info->converter = mconv_buffer_converter (im_info->coding, NULL, 0);
  ic->info = ic_info;
  return 0;
}

static void
xim_destroy_ic (MInputContext *ic)
{
  MInputXIMContextInfo *ic_info = (MInputXIMContextInfo *) ic->info;

  XDestroyIC (ic_info->xic);
  mconv_free_converter (ic_info->converter);
  free (ic_info);
  ic->info = NULL;
}

static int
xim_filter (MInputContext *ic, MSymbol key, void *event)
{
  MInputXIMContextInfo *ic_info = (MInputXIMContextInfo *) ic->info;

  return (XFilterEvent ((XEvent *) event, ic_info->win) == True);
}


static int
xim_lookup (MInputContext *ic, MSymbol key, void *arg, MText *mt)
{
  MInputXIMMethodInfo *im_info = (MInputXIMMethodInfo *) ic->im->info;
  MInputXIMContextInfo *ic_info = (MInputXIMContextInfo *) ic->info;
  XKeyPressedEvent *ev = (XKeyPressedEvent *) arg;
  KeySym keysym;
  Status status;
  char *buf;
  int len;

  buf = (char *) alloca (512);
  len = XmbLookupString (ic_info->xic, ev, buf, 512, &keysym, &status);
  if (status == XBufferOverflow)
    {
      buf = (char *) alloca (len);
      len = XmbLookupString (ic_info->xic, ev, buf, len, &keysym, &status);
    }

  mtext_reset (ic->produced);
  if (len == 0)
    return 1;

  mconv_reset_converter (ic_info->converter);
  mconv_rebind_buffer (ic_info->converter, (unsigned char *) buf, len);
  mconv_decode (ic_info->converter, ic->produced);
  mtext_put_prop (ic->produced, 0, mtext_nchars (ic->produced),
		  Mlanguage, (void *) im_info->language);
  mtext_cpy (mt, ic->produced);
  mtext_reset (ic->produced);  
  return 0;
}



#if 1
static int
x_error_handler (Display *display, XErrorEvent *error)
{
  mdebug_hook ();
  return 0;
}

static int
x_io_error_handler (Display *display)
{
  mdebug_hook ();
  return 0;
}
#endif



int
mwin__init ()
{
  Mdisplay = msymbol ("display");
  Mscreen = msymbol ("screen");
  Mdrawable = msymbol ("drawable");
  Mdepth = msymbol ("depth");
  Mwidget = msymbol ("widget");
  M_iso8859_1 = msymbol ("iso8859-1");
  M_iso10646_1 = msymbol ("iso10646-1");

  display_info_list = mplist ();
  device_list = mplist ();

  mfont__driver_list[MFONT_TYPE_WIN] = &xfont_driver;

  Mxim = msymbol ("xim");
  msymbol_put (Mxim, Minput_driver, &minput_xim_driver);

  return 0;
}

void
mwin__fini ()
{
  M17N_OBJECT_UNREF (display_info_list);
  M17N_OBJECT_UNREF (device_list);
}

typedef struct
{
  String font;
  String foreground;
  String background;
  Boolean reverse_video;
} AppData, *AppDataPtr;


int
mwin__parse_font_name (char *name, MFont *font)
{
  char *field[XLFD_FIELD_MAX];
  unsigned short size, resy;
  MSymbol attrs[MFONT_PROPERTY_MAX];
  char *copy = (char *) alloca (512);
  int i, len;
  char *p, *last = NULL;

  len = strlen (name) + 1;
  for (i = 0, p = name; *p; p++)
    {
      if (*p == '-')
	i++;
      else if (p > name && *p == '*' && p[-1] == '-')
	last = p + 1;
    }
  if (i == 14)
    memcpy (copy, name, len);
  else if (last)
    {
      memcpy (copy, name, last - name);
      for (; i < 14; i++)
	strcat (copy, "-*");
      strcat (copy, last);
    }

  if (split_font_name (copy, field, &size, &resy) < 0)
    return -1;
  attrs[MFONT_FOUNDRY]
    = *(field[XLFD_FOUNDRY]) != '*' ? msymbol (field[XLFD_FOUNDRY]) : Mnil;
  attrs[MFONT_FAMILY]
    = *(field[XLFD_FAMILY]) != '*' ? msymbol (field[XLFD_FAMILY]) : Mnil;
  attrs[MFONT_WEIGHT]
    = *(field[XLFD_WEIGHT]) != '*' ? msymbol (field[XLFD_WEIGHT]) : Mnil;
  attrs[MFONT_STYLE]
    = *(field[XLFD_SLANT]) != '*' ? msymbol (field[XLFD_SLANT]) : Mnil;
  attrs[MFONT_STRETCH]
    = *(field[XLFD_SWIDTH]) != '*' ? msymbol (field[XLFD_SWIDTH]) : Mnil;
  attrs[MFONT_ADSTYLE]
    = *(field[XLFD_ADSTYLE]) != '*' ? msymbol (field[XLFD_ADSTYLE]) : Mnil;
  attrs[MFONT_REGISTRY]
    = *(field[XLFD_REGISTRY]) != '*' ? msymbol (field[XLFD_REGISTRY]) : Mnil;
  mfont__set_spec (font, attrs, size, resy);
  return 0;
}


char *
mwin__build_font_name (MFont *font)
{
  char name[1024];

  if (build_font_name (font, name, 1024) < 0)
    return NULL;
  return strdup (name);
}

/** Return an MWDevice object corresponding to a display specified in
    PLIST.

    It searches device_list for a device matching the display.  If
    found, return the found object.  Otherwise, return a newly created
    object.  */

MWDevice *
mwin__open_device (MFrame *frame, MPlist *param)
{
  Display *display = NULL;
  Screen *screen = NULL;
  int screen_num;
  Drawable drawable = 0;
  Widget widget = NULL;
  Colormap cmap = 0;
  int auto_display = 0;
  MDisplayInfo *disp_info = NULL;
  MWDevice *device = NULL;
  MSymbol key;
  XWindowAttributes attr;
  unsigned depth = 0;
  MPlist *plist;

  if (param)
    for (plist = param; (key = mplist_key (plist)) != Mnil;
	 plist = mplist_next (plist))
      {
	if (key == Mdisplay)
	  display = (Display *) mplist_value (plist);
	else if (key == Mscreen)
	  screen = mplist_value (plist);
	else if (key == Mdrawable)
	  drawable = (Drawable) mplist_value (plist);
	else if (key == Mdepth)
	  depth = (unsigned) mplist_value (plist);
	else if (key == Mwidget)
	  widget = (Widget) mplist_value (plist);
	else if (key == Mcolormap)
	  cmap = (Colormap) mplist_value (plist);
      }

  if (widget)
    {
      display = XtDisplay (widget);
      screen_num = XScreenNumberOfScreen (XtScreen (widget));
      depth = DefaultDepth (display, screen_num);
    }
  else if (drawable)
    {
      Window root_window;
      int x, y;
      unsigned width, height, border_width;

      if (! display)
	MERROR (MERROR_WIN, NULL);
      XGetGeometry (display, drawable, &root_window,
		    &x, &y, &width, &height, &border_width, &depth);
      XGetWindowAttributes (display, root_window, &attr);
      screen_num = XScreenNumberOfScreen (attr.screen);
    }
  else
    {
      if (screen)
	display = DisplayOfScreen (screen);
      else
	{
	  if (! display)
	    {
	      display = XOpenDisplay (NULL);
	      if (! display)
		MERROR (MERROR_WIN, NULL);
	      auto_display = 1;
	    }
	  screen = DefaultScreenOfDisplay (display);
	}
      screen_num = XScreenNumberOfScreen (screen);
      if (! depth)
	depth = DefaultDepth (display, screen_num);
    }

  if (! cmap)
    cmap = DefaultColormap (display, screen_num);

  for (plist = display_info_list; mplist_key (plist) != Mnil;
       plist = mplist_next (plist))
    {
      disp_info = (MDisplayInfo *) mplist_value (plist);
      if (disp_info->display == display)
	break;
    }

  if (mplist_key (plist) != Mnil)
    M17N_OBJECT_REF (disp_info);
  else
    {
      M17N_OBJECT (disp_info, free_display_info, MERROR_WIN);
      disp_info->display = display;
      disp_info->auto_display = auto_display;
      disp_info->font_registry_list = mplist ();
      disp_info->iso8859_1_family_list = mplist ();
      disp_info->iso10646_1_family_list = mplist ();
      disp_info->realized_font_list = mplist ();
      find_modifier_bits (disp_info);
      mplist_add (display_info_list, Mt, disp_info);
    }  

  for (plist = device_list; mplist_key (plist) != Mnil;
       plist = mplist_next (plist))
    {
      device = (MWDevice *) mplist_value (plist);
      if (device->display_info == disp_info
	  && device->depth == depth
	  && device->cmap == cmap)
	break;
    }

  if (mplist_key (plist) != Mnil)
    M17N_OBJECT_REF (device);
  else
    {
      M17N_OBJECT (device, free_device, MERROR_WIN);
      device->display_info = disp_info;
      device->screen_num = screen_num;
      /* A drawable on which to create GCs.  */
      device->drawable = XCreatePixmap (display,
					RootWindow (display, screen_num),
					1, 1, depth);
      device->depth = depth;
      device->cmap = cmap;
      device->realized_face_list = mplist ();
      device->realized_fontset_list = mplist ();
      device->foreground = BlackPixel (display, screen_num);
      device->background = WhitePixel (display, screen_num);
    }

  frame->realized_font_list = disp_info->realized_font_list;
  frame->realized_face_list = device->realized_face_list;
  frame->realized_fontset_list = device->realized_fontset_list;

  if (widget)
    {
      AppData app_data;
      XtResource resources[] = {
	{ XtNfont, XtCFont, XtRString, sizeof (String),
	  XtOffset (AppDataPtr, font), XtRString,
	  "-misc-fixed-medium-r-normal--*-120-*-*-*-*-iso8859-1" },
	{ XtNforeground, XtCForeground, XtRString, sizeof (String),
	  XtOffset (AppDataPtr, foreground), XtRString, "black" },
	{ XtNbackground, XtCBackground, XtRString, sizeof (String),
	  XtOffset (AppDataPtr, background), XtRString, "white" },
	{ XtNreverseVideo, XtCReverseVideo, XtRBoolean, sizeof (Boolean),
	  XtOffset (AppDataPtr, reverse_video), XtRImmediate, (caddr_t) FALSE }
      };
      MFace *face = NULL;
      MFont font;
      char **names;
      int nfonts;

      XtGetApplicationResources (widget, &app_data,
				 resources, XtNumber (resources), NULL, 0);
      names = XListFonts (display, app_data.font, 1, &nfonts);
      if (nfonts == 1)
	{
	  if (mwin__parse_font_name (names[0], &font) >= 0)
	    face = mface_from_font (&font);
	  else
	    {
	      /* The font name does not conform to XLFD.  Try to open the
		 font and get XA_FONT property.  */
	      XFontStruct *xfont = XLoadQueryFont (display, names[0]);

	      if (xfont)
		{
		  unsigned long value;
		  char *name;

		  if (XGetFontProperty (xfont, XA_FONT, &value)
		      && (name = ((char *)
				  XGetAtomName (display, (Atom) value))))
		    {
		      if (mwin__parse_font_name (name, &font) >= 0)
			face = mface_from_font (&font);
		    }
		  XFreeFont (display, xfont);
		}
	    }
	  XFreeFontNames (names);
	}

      if (app_data.reverse_video == True)
	{
	  if (! face)
	    face = mface ();
	  mface_put_prop (face, Mvideomode, Mreverse);
	}
      if (face)
	{
	  mplist_push (param, Mface, face);
	  M17N_OBJECT_UNREF (face);
	}
      device->foreground
	= get_color (display, cmap, msymbol (app_data.foreground), Mnil,
		     device->foreground);
      device->background
	= get_color (display, cmap, msymbol (app_data.background), Mnil,
		     device->background);
    }
  XSetErrorHandler (x_error_handler);
  /* XSetIOErrorHandler (x_io_error_handler); */

  return device;
}

void
mwin__close_device (MFrame *frame)
{
  M17N_OBJECT_UNREF (frame->device);
}

void *
mwin__device_get_prop (MWDevice *device, MSymbol key)
{
  if (key == Mdisplay)
    return (void *) device->display_info->display;
  if (key == Mscreen)
    return (void *) ScreenOfDisplay(device->display_info->display,
				    device->screen_num);
  if (key == Mcolormap)
    return (void *) device->cmap;
  if (key == Mdepth)
    return (void *) device->depth;
  return NULL;
}

struct {
  int size, inc, used;
  GC *gc_table;
} gc_list;

#define REGISTER_GC(gc)					\
  do {							\
    if (! gc_list.size)					\
      MLIST_INIT1 (&gc_list, gc_table, 100);		\
    MLIST_APPEND1 (&gc_list, gc_table, gc, MERROR_WIN);	\
  } while (0)


#define UNREGISTER_GC(gc)			\
  do {						\
    int j;					\
    for (j = 0; j < gc_list.used; j++)		\
      if (gc_list.gc_table[j] == gc)		\
	gc_list.gc_table[j] = (GC) NULL;	\
  } while (0)


void
mwin__realize_face (MRealizedFace *rface)
{
  MFrame *frame = rface->frame;
  MWDevice *device = frame->device;
  Display *display = FRAME_DISPLAY (frame);
  XGCValues values;
  int mask = GCForeground | GCBackground;
  MSymbol foreground = rface->face.property[MFACE_FOREGROUND];
  MSymbol background = rface->face.property[MFACE_BACKGROUND];
  MFaceHLineProp *hline = rface->hline;
  MFaceBoxProp *box = rface->box;
  MFaceHookFunc func = (MFaceHookFunc) rface->face.property[MFACE_HOOK_FUNC];
  MSymbol default_foreground
    = (MSymbol) mface_get_prop (frame->face, Mforeground);
  MSymbol default_background
    = (MSymbol) mface_get_prop (frame->face, Mbackground);
  GC *gcs;
  unsigned long pixel;

  MTABLE_CALLOC (gcs, MFACE_GCS, MERROR_WIN);

  values.foreground = get_color (display, device->cmap, foreground,
				 default_foreground, device->foreground);
  values.background = get_color (display, device->cmap, background,
				 default_background, device->background);
  if (rface->face.property[MFACE_VIDEOMODE] == Mreverse)
    pixel = values.foreground,
      values.foreground = values.background,
      values.background = pixel;

  if (rface->rfont
      && rface->rfont->font.property[MFONT_TYPE] - 1 == MFONT_TYPE_WIN)
    {
      values.font = ((MXFontInfo *) (rface->rfont->info))->f->fid;
      mask |= GCFont;
    }

  gcs[MFACE_GC_NORMAL] = XCreateGC (display, device->drawable, mask, &values);
  REGISTER_GC (gcs[MFACE_GC_NORMAL]);

  gcs[MFACE_GC_SCRATCH] = XCreateGC (display, device->drawable, mask, &values);
  REGISTER_GC (gcs[MFACE_GC_SCRATCH]);

  pixel = values.foreground;
  values.foreground = values.background;
  values.background = pixel;
  gcs[MFACE_GC_INVERSE] = XCreateGC (display, device->drawable, mask, &values);
  REGISTER_GC (gcs[MFACE_GC_INVERSE]);
  values.background = values.foreground;
  values.foreground = pixel;

  mask &= ~GCFont;

  if (rface == rface->ascii_rface)
    {
      /* This realized face is for ASCII.  Setup GCs for hline and
	 box.  */
      if (hline && hline->color != foreground)
	{
	  values.foreground
	    = get_color (display, device->cmap, hline->color,
			 default_foreground, device->foreground);
	  gcs[MFACE_GC_HLINE]
	    = XCreateGC (display, device->drawable, mask, &values);
	  REGISTER_GC (gcs[MFACE_GC_HLINE]);
	  values.foreground = pixel;
	}

      if (box)
	{
	  if (box->color_top)
	    {
	      values.foreground
		= get_color (display, device->cmap, box->color_top,
			     default_foreground, device->foreground);
	      gcs[MFACE_GC_BOX_TOP]
		= XCreateGC (display, device->drawable, mask, &values);
	      REGISTER_GC (gcs[MFACE_GC_BOX_TOP]);
	    }

	  if (box->color_left
	      && box->color_left != box->color_top)
	    {
	      values.foreground
		= get_color (display, device->cmap, box->color_left,
			     default_foreground, device->foreground);
	      gcs[MFACE_GC_BOX_LEFT]
		= XCreateGC (display, device->drawable, mask, &values);
	      REGISTER_GC (gcs[MFACE_GC_BOX_LEFT]);
	    }

	  if (box->color_right
	      && box->color_right != box->color_top)
	    {
	      values.foreground
		= get_color (display, device->cmap, box->color_right,
			     default_foreground, device->foreground);
	      gcs[MFACE_GC_BOX_RIGHT]
		= XCreateGC (display, device->drawable, mask, &values);
	      REGISTER_GC (gcs[MFACE_GC_BOX_RIGHT]);
	    }

	  if (box->color_bottom
	      && box->color_bottom != box->color_top)
	    {
	      values.foreground
		= get_color (display, device->cmap, box->color_bottom,
			     default_foreground, device->foreground);
	      gcs[MFACE_GC_BOX_BOTTOM]
		= XCreateGC (display, device->drawable, mask, &values);
	      REGISTER_GC (gcs[MFACE_GC_BOX_BOTTOM]);
	    }
	}
    }
  else
    {
      /* This realized face is not for ASCII.  GCs for hline and box
	 are shared with that of the corresponding ASCII face.  */
      GC *ascii_gcs = rface->ascii_rface->info;
      int i;

      for (i = MFACE_GC_HLINE; i < MFACE_GCS; i++)
	gcs[i] = ascii_gcs[i];
    }

  rface->info = gcs;
  if (func)
    (func) (&(rface->face), rface->info, rface->face.property[MFACE_HOOK_ARG]);
}


void
mwin__free_realized_face (MRealizedFace *rface)
{
  GC *gcs = rface->info;
  enum face_gc limit
    = rface == rface->ascii_rface ? MFACE_GCS : MFACE_GC_HLINE;
  int i;

  for (i = 0; i < limit; i++)
    if (gcs[i])
      {
	UNREGISTER_GC (gcs[i]);
	XFreeGC (FRAME_DISPLAY (rface->frame), gcs[i]);
      }	    
  free (gcs);
}


void
mwin__fill_space (MFrame *frame, MDrawWindow win, MRealizedFace *rface,
		  int reverse,
		  int x, int y, int width, int height, MDrawRegion region)
{
  GC *gcs = rface->info;
  GC gc = gcs[reverse ? MFACE_GC_NORMAL : MFACE_GC_INVERSE];

  if (region)
    gc = set_region (FRAME_DISPLAY (frame), rface, gc, region);

  XFillRectangle (FRAME_DISPLAY (frame), (Window) win, gc,
		  x, y, width, height);
}


void
mwin__draw_hline (MFrame *frame, MDrawWindow win, MGlyphString *gstring,
		 MRealizedFace *rface, int reverse,
		 int x, int y, int width, MDrawRegion region)
{
  enum MFaceHLineType type = rface->hline->type;
  GC *gcs = rface->info;
  GC gc;
  int i;

  y = (type == MFACE_HLINE_BOTTOM
       ? y + gstring->text_descent - rface->hline->width
       : type == MFACE_HLINE_UNDER
       ? y + 1
       : type == MFACE_HLINE_STRIKE_THROUGH
       ? y - ((gstring->ascent + gstring->descent) / 2)
       : y - gstring->text_ascent);
  if (reverse)
    gc = gcs[MFACE_GC_INVERSE];
  else if (gcs[MFACE_GC_HLINE])
    gc = gcs[MFACE_GC_HLINE];
  else
    gc = gcs[MFACE_GC_NORMAL];

  if (region)
    gc = set_region (FRAME_DISPLAY (frame), rface, gc, region);

  for (i = 0; i < rface->hline->width; i++)
    XDrawLine (FRAME_DISPLAY (frame), (Window) win, gc,
	       x, y + i, x + width - 1, y + i);
}


void
mwin__draw_box (MFrame *frame, MDrawWindow win, MGlyphString *gstring,
		MGlyph *g, int x, int y, int width, MDrawRegion region)
{
  Display *display = FRAME_DISPLAY (frame);
  MRealizedFace *rface = g->rface;
  MFaceBoxProp *box = rface->box;
  GC *gcs = rface->info;
  GC gc_top, gc_left, gc_right, gc_btm;
  int y0, y1;
  int i;

  y0 = y - (gstring->text_ascent
	    + rface->box->inner_vmargin + rface->box->width);
  y1 = y + (gstring->text_descent
	    + rface->box->inner_vmargin + rface->box->width - 1);

  gc_top = gcs[MFACE_GC_BOX_TOP];
  if (! gc_top)
    gc_top = gcs[MFACE_GC_NORMAL];
  if (region)
    gc_top = set_region (FRAME_DISPLAY (frame), rface, gc_top, region);
  gc_btm = gcs[MFACE_GC_BOX_BOTTOM];
  if (! gc_btm)
    gc_btm = gc_top;

  if (g->type == GLYPH_BOX)
    {
      int x0, x1;

      if (g->left_padding)
	x0 = x + box->outer_hmargin, x1 = x + g->width - 1;
      else
	x0 = x, x1 = x + g->width - box->outer_hmargin - 1;

      /* Draw the top side.  */
      for (i = 0; i < box->width; i++)
	XDrawLine (display, (Window) win, gc_top, x0, y0 + i, x1, y0 + i);

      /* Draw the bottom side.  */
      if (region)
	gc_btm = set_region (display, rface, gc_btm, region);
      for (i = 0; i < box->width; i++)
	XDrawLine (display, (Window) win, gc_btm, x0, y1 - i, x1, y1 - i);

      if (g->left_padding > 0)
	{
	  /* Draw the left side.  */
	  gc_left = gcs[MFACE_GC_BOX_LEFT];
	  if (! gc_left)
	    gc_left = gc_top;
	  else if (region)
	    gc_left = set_region (display, rface, gc_left, region);
	  for (i = 0; i < rface->box->width; i++)
	    XDrawLine (display, (Window) win, gc_left,
		       x0 + i, y0 + i, x0 + i, y1 - i);
	}
      else
	{
	  /* Draw the right side.  */
	  gc_right = gcs[MFACE_GC_BOX_RIGHT];
	  if (! gc_right)
	    gc_right = gc_top;
	  else if (region)
	    gc_right = set_region (display, rface, gc_right, region);
	  for (i = 0; i < rface->box->width; i++)
	    XDrawLine (display, (Window) win, gc_right,
		       x1 - i, y0 + i, x1 - i, y1 - i);
	}
    }
  else
    {
      /* Draw the top side.  */
      for (i = 0; i < box->width; i++)
	XDrawLine (display, (Window) win, gc_top,
		   x, y0 + i, x + width - 1, y0 + i);

      /* Draw the bottom side.  */
      if (region)
	gc_btm = set_region (display, rface, gc_btm, region);
      for (i = 0; i < box->width; i++)
	XDrawLine (display, (Window) win, gc_btm,
		   x, y1 - i, x + width - 1, y1 - i);
    }
}


void
mwin__draw_bitmap (MFrame *frame, MDrawWindow win, MRealizedFace *rface,
		   int reverse, int x, int y,
		   int width, int height, int row_bytes, unsigned char *bmp,
		   MDrawRegion region)
{
  Display *display = FRAME_DISPLAY (frame);
  int i, j;
  GC *gcs = rface->info;
  GC gc = gcs[reverse ? MFACE_GC_INVERSE : MFACE_GC_NORMAL];

  if (region)
    gc = set_region (FRAME_DISPLAY (frame), rface, gc, region);

  for (i = 0; i < height; i++, bmp += row_bytes)
    for (j = 0; j < width; j++)
      if (bmp[j / 8] & (1 << (7 - (j % 8))))
	XDrawPoint (display, (Window) win, gc, x + j, y + i);
}


MDrawRegion
mwin__region_from_rect (MDrawMetric *rect)
{
  MDrawRegion region1 = XCreateRegion ();
  MDrawRegion region2 = XCreateRegion ();
  XRectangle xrect;

  xrect.x = rect->x;
  xrect.y = rect->y;
  xrect.width = rect->width;
  xrect.height = rect->height;
  XUnionRectWithRegion (&xrect, region1, region2);
  XDestroyRegion (region1);
  return region2;
}

void
mwin__union_rect_with_region (MDrawRegion region, MDrawMetric *rect)
{
  MDrawRegion region1 = XCreateRegion ();
  XRectangle xrect;

  xrect.x = rect->x;
  xrect.y = rect->y;
  xrect.width = rect->width;
  xrect.height = rect->height;

  XUnionRegion (region, region, region1);
  XUnionRectWithRegion (&xrect, region1, region);
  XDestroyRegion (region1);
}

void
mwin__intersect_region (MDrawRegion region1, MDrawRegion region2)
{
  MDrawRegion region = XCreateRegion ();

  XUnionRegion (region1, region1, region);
  XIntersectRegion (region, region2, region1);
  XDestroyRegion (region);
}

void
mwin__region_add_rect (MDrawRegion region, MDrawMetric *rect)
{
  MDrawRegion region1 = XCreateRegion ();
  XRectangle xrect;

  xrect.x = rect->x;
  xrect.y = rect->y;
  xrect.width = rect->width;
  xrect.height = rect->height;
  XUnionRectWithRegion (&xrect, region1, region);
  XDestroyRegion (region1);
}

void
mwin__region_to_rect (MDrawRegion region, MDrawMetric *rect)
{
  XRectangle xrect;

  XClipBox (region, &xrect);
  rect->x = xrect.x;
  rect->y = xrect.y;
  rect->width = xrect.width;
  rect->height = xrect.height;
}

void
mwin__free_region (MDrawRegion region)
{
  XDestroyRegion (region);
}

void
mwin__dump_region (MDrawRegion region)
{
  XRectangle rect;
  XClipBox (region, &rect);
  fprintf (stderr, "(%d %d %d %d)\n", rect.x, rect.y, rect.width, rect.height);
}

void
mwin__verify_region (MFrame *frame, MDrawRegion region)
{
  set_region (FRAME_DISPLAY (frame), frame->rface,
	      ((GC *) frame->rface->info)[MFACE_GC_NORMAL], region);
}

MDrawWindow
mwin__create_window (MFrame *frame, MDrawWindow parent)
{
  MWDevice *device = frame->device;
  Display *display = FRAME_DISPLAY (frame);
  int screen = FRAME_SCREEN (frame);
  Window win;
  XWMHints wm_hints = { InputHint, False };
  XClassHint class_hints = { "M17N-IM", "m17n-im" };
  XSetWindowAttributes attrs;
  unsigned long mask;
  MSymbol background = mface_get_prop (frame->face, Mbackground);

  attrs.background_pixel = get_color (display, device->cmap,
				      background, background,
				      WhitePixel (display, screen));
  attrs.backing_store = Always;
  attrs.override_redirect = True;
  attrs.save_under = True;
  mask = CWBackPixel | CWBackingStore | CWOverrideRedirect | CWSaveUnder;
  if (! parent)
    parent = (MDrawWindow) RootWindow (display, screen);
  win = XCreateWindow (display, (Window) parent, 0, 0, 1, 1, 0,
		       CopyFromParent, InputOutput, CopyFromParent,
		       mask, &attrs);
  XSetWMProperties (display, (Window) win, NULL, NULL, NULL, 0,
		    NULL, &wm_hints, &class_hints);
  XSelectInput (display, (Window) win, StructureNotifyMask | ExposureMask);
  return (MDrawWindow) win;
}

void
mwin__destroy_window (MFrame *frame, MDrawWindow win)
{
  XDestroyWindow (FRAME_DISPLAY (frame), (Window) win);
}

#if 0
MDrawWindow
mwin__event_window (void *event)
{
  return ((MDrawWindow) ((XEvent *) event)->xany.window);
}

void
mwin__print_event (void *arg, char *win_name)
{
  char *event_name;
  XEvent *event = (XEvent *) arg;

  switch (event->xany.type)
    {
    case 2: event_name = "KeyPress"; break;
    case 3: event_name = "KeyRelease"; break;
    case 4: event_name = "ButtonPress"; break;
    case 5: event_name = "ButtonRelease"; break;
    case 6: event_name = "MotionNotify"; break;
    case 7: event_name = "EnterNotify"; break;
    case 8: event_name = "LeaveNotify"; break;
    case 9: event_name = "FocusIn"; break;
    case 10: event_name = "FocusOut"; break;
    case 11: event_name = "KeymapNotify"; break;
    case 12: event_name = "Expose"; break;
    case 13: event_name = "GraphicsExpose"; break;
    case 14: event_name = "NoExpose"; break;
    case 15: event_name = "VisibilityNotify"; break;
    case 16: event_name = "CreateNotify"; break;
    case 17: event_name = "DestroyNotify"; break;
    case 18: event_name = "UnmapNotify"; break;
    case 19: event_name = "MapNotify"; break;
    case 20: event_name = "MapRequest"; break;
    case 21: event_name = "ReparentNotify"; break;
    case 22: event_name = "ConfigureNotify"; break;
    case 23: event_name = "ConfigureRequest"; break;
    case 24: event_name = "GravityNotify"; break;
    case 25: event_name = "ResizeRequest"; break;
    case 26: event_name = "CirculateNotify"; break;
    case 27: event_name = "CirculateRequest"; break;
    case 28: event_name = "PropertyNotify"; break;
    case 29: event_name = "SelectionClear"; break;
    case 30: event_name = "SelectionRequest"; break;
    case 31: event_name = "SelectionNotify"; break;
    case 32: event_name = "ColormapNotify"; break;
    case 33: event_name = "ClientMessage"; break;
    case 34: event_name = "MappingNotify"; break;
    default: event_name = "unknown";
    }

  fprintf (stderr, "%s: %s\n", win_name, event_name);
}
#endif

void
mwin__map_window (MFrame *frame, MDrawWindow win)
{
  XMapRaised (FRAME_DISPLAY (frame), (Window) win);
}

void
mwin__unmap_window (MFrame *frame, MDrawWindow win)
{
  XUnmapWindow (FRAME_DISPLAY (frame), (Window) win);
}

void
mwin__window_geometry (MFrame *frame, MDrawWindow win, MDrawWindow parent_win,
		       MDrawMetric *geometry)
{
  Display *display = FRAME_DISPLAY (frame);
  XWindowAttributes attr;
  Window parent = (Window) parent_win, root;

  XGetWindowAttributes (display, (Window) win, &attr);
  geometry->x = attr.x + attr.border_width;
  geometry->y = attr.y + attr.border_width;
  geometry->width = attr.width;
  geometry->height = attr.height; 

  if (! parent)
    parent = RootWindow (display, FRAME_SCREEN (frame));
  while (1)
    {
      Window this_parent, *children;
      unsigned n;

      XQueryTree (display, (Window) win, &root, &this_parent, &children, &n);
      if (children)
	XFree (children);
      if (this_parent == parent || this_parent == root)
	break;
      win = (MDrawWindow) this_parent;
      XGetWindowAttributes (display, (Window) win, &attr);
      geometry->x += attr.x + attr.border_width;
      geometry->y += attr.y + attr.border_width;
    }
}

void
mwin__adjust_window (MFrame *frame, MDrawWindow win,
		     MDrawMetric *current, MDrawMetric *new)
{
  Display *display = FRAME_DISPLAY (frame);
  unsigned int mask = 0;
  XWindowChanges values;

  if (current->width != new->width)
    {
      mask |= CWWidth;
      if (new->width <= 0)
	new->width = 1;
      values.width = current->width = new->width;
    }
  if (current->height != new->height)
    {
      mask |= CWHeight;
      if (new->height <= 0)
	new->height = 1;
      values.height = current->height = new->height;
    }
  if (current->x != new->x)
    {
      mask |= CWX;
      values.x = current->x = new->x;
    }
  if (current->y != new->y)
    {
      mask |= CWY;
      current->y = new->y;
      values.y = current->y = new->y;
    }
  if (mask)
    XConfigureWindow (display, (Window) win, mask, &values);
}

MSymbol
mwin__parse_event (MFrame *frame, void *arg, int *modifiers)
{
  XEvent *event = (XEvent *) arg;
  MDisplayInfo *disp_info = frame->device->display_info;
  int len;
  char buf[512];
  KeySym keysym;
  MSymbol key;

  *modifiers = 0;
  if (event->xany.type != KeyPress
      /* && event->xany.type != KeyRelease */
      )
    return Mnil;
  len = XLookupString ((XKeyEvent *) event, (char *) buf, 512, &keysym, NULL);
  if (len > 1)
    return Mnil;
  if (len == 1)
    {
      int c = buf[0];

      if ((c == ' ' || c == 127) && ((XKeyEvent *) event)->state & ShiftMask)
	*modifiers |= MINPUT_KEY_SHIFT_MODIFIER;
      if (((XKeyEvent *) event)->state & ControlMask)
	{
	  c &= 0x1F;
	  if (((XKeyEvent *) event)->state & ShiftMask)
	    *modifiers |= MINPUT_KEY_SHIFT_MODIFIER;
	}
      if (((XKeyEvent *) event)->state & disp_info->meta_mask)
	c |= 0x80;
      key = minput__char_to_key (c);
    }
  else if (keysym >= XK_Shift_L && keysym <= XK_Hyper_R)
    return Mnil;
  else
    {
      char *str = XKeysymToString (keysym);

      if (! str)
	return Mnil;
      key = msymbol (str);
      if (((XKeyEvent *) event)->state & ShiftMask)
	*modifiers |= MINPUT_KEY_SHIFT_MODIFIER;
      if (((XKeyEvent *) event)->state & ControlMask)
	*modifiers |= MINPUT_KEY_CONTROL_MODIFIER;
    }
  if (((XKeyEvent *) event)->state & disp_info->meta_mask)
    *modifiers |= MINPUT_KEY_META_MODIFIER;
  if (((XKeyEvent *) event)->state & disp_info->alt_mask)
    *modifiers |= MINPUT_KEY_ALT_MODIFIER;
  if (((XKeyEvent *) event)->state & disp_info->super_mask)
    *modifiers |= MINPUT_KEY_SUPER_MODIFIER;
  if (((XKeyEvent *) event)->state & disp_info->hyper_mask)
    *modifiers |= MINPUT_KEY_HYPER_MODIFIER;

  return key;
}


MText *
mwin__get_selection_text (MFrame *frame)
{
  return NULL;
}


void
mwin__dump_gc (MFrame *frame, MRealizedFace *rface)
{
  unsigned long valuemask = GCForeground | GCBackground | GCClipMask;
  XGCValues values;
  Display *display = FRAME_DISPLAY (frame);
  GC *gcs = rface->info;
  int i;

  for (i = 0; i <= MFACE_GC_SCRATCH; i++)
    {
      XGetGCValues (display, gcs[i], valuemask, &values);
      fprintf (stderr, "GC%d: fore/#%lX back/#%lX", i,
	       values.foreground, values.background);
      fprintf (stderr, "\n");
    }
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */

/* External API */

/*=*/
/*** @addtogroup m17nFrame */
/*** @{ */
/*=*/

/***en
    @name Variables: Keys of frame parameter (X specific).

    These are the symbols to use as parameter keys for the function
    mframe () (which see).  They are also keys of a frame property
    (except for #Mwidget).  */
/*=*/
/*** @{ */ 
/* Keywords for mwin__open_device ().  */
MSymbol Mdisplay, Mscreen, Mdrawable, Mdepth, Mwidget, Mcolormap;

/*** @} */
/*** @} */

/*=*/
/*** @addtogroup m17nInputMethodWin */
/*=*/
/*** @{ */

/***en
    @brief Input driver for XIM.

    The input driver #minput_xim_driver is for the foreign input
    method of name #Mxim.  It uses XIM (X Input Methods) as a
    background input engine.

    As the symbol #Mxim has property #Minput_driver whose value is
    a pointer to this driver, the input method of language #Mnil
    and name #Mxim uses this driver.

    Therefore, for such input methods, the driver dependent arguments
    to the functions whose name begin with minput_ must be as follows.

    The argument $ARG of the function minput_open_im () must be a
    pointer to the structure #MInputXIMArgIM.  See the documentation
    of #MInputXIMArgIM for more detail.

    The argument $ARG of the function minput_create_ic () must be a
    pointer to the structure #MInputXIMArgIC. See the documentation
    of #MInputXIMArgIC for more detail.

    The argument $ARG of the function minput_filter () must be a
    pointer to the structure @c XEvent.  The argument $KEY is ignored.

    The argument $ARG of the function minput_lookup () must be the
    same one as that of the function minput_filter ().  The argument
    $KEY is ignored.  */

/***ja
    @brief XIM用入力ドライバ

    入力ドライバ #minput_xim_driver は #Mxim を名前として持つ外部
    入力メソッド用であり、 XIM (X Input Methods) をバックグラウンドの
    入力エンジンとして使用する。

    シンボル #Mxim はこのドライバへのポインタを値とするプロパティ
    #Minput_driver を持つため、LANGUAGE が #Mnil で名前が #Mxim で
    ある入力メソッドはこのドライバを利用する。

    したがって、それらの入力メソッド用には、minput_ で始まる名前を持つ
    以下の関数群のドライバに依存する引数は次のようなものでなくてはなら
    ない。

    関数 minput_open_im () の引数 $ARG は構造体 #MInputXIMArgIM への
    ポインタでなくてはならない。詳細については #MInputXIMArgIM のド
    キュメントを参照のこと。

    関数 minput_create_ic () の引数 $ARG は構造体 #MInputXIMArgIC へ
    のポインタでなくてはならない。詳細については #MInputXIMArgIC の
    ドキュメントを参照のこと。

    関数 minput_filter () の引数 %ARG は構造体 @c XEvent へのポインタ
    でなくてはならない。引数 $KEY は無視される。

    関数 minput_lookup () の引数 $ARG は関数 function minput_filter () 
    の引数 $ARG と同じものでなくてはならない。 引数 $KEY は、無視され
    る。  */

MInputDriver minput_xim_driver =
  { xim_open_im, xim_close_im, xim_create_ic, xim_destroy_ic,
    xim_filter, xim_lookup, NULL };

/*=*/

/***en
    @brief Symbol of the name "xim".

    The variable Mxim is a symbol of name "xim".  It is a name of the
    input method driver #minput_xim_driver.  */ 

MSymbol Mxim;

/*** @} */ 

/*
  Local Variables:
  coding: euc-japan
  End:
*/
