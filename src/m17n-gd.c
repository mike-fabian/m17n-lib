/* m17n-gd.c -- implementation of the GUI API on GD Library.
   Copyright (C) 2004, 2005, 2006
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
   02111-1307, USA.  */

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include "config.h"

#if defined (HAVE_FREETYPE) && defined (HAVE_GD)

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <gd.h>

#include "m17n-gui.h"
#include "m17n-misc.h"
#include "internal.h"
#include "internal-gui.h"
#include "symbol.h"
#include "font.h"
#include "fontset.h"
#include "face.h"

static MPlist *realized_fontset_list;
static MPlist *realized_font_list;
static MPlist *realized_face_list;

/* The first element is for 256 color, the second for true color.  */
static gdImagePtr scratch_images[2];

enum ColorIndex
  {
    COLOR_NORMAL,
    COLOR_INVERSE,
    COLOR_HLINE,
    COLOR_BOX_TOP,
    COLOR_BOX_BOTTOM,
    COLOR_BOX_LEFT,
    COLOR_BOX_RIGHT,
    COLOR_MAX
  };

static MSymbol M_rgb;

static void
read_rgb_txt ()
{
  FILE *fp;
  int r, g, b, i;

  /* At first, support HTML 4.0 color names. */
  msymbol_put (msymbol ("black"), M_rgb, (void *) 0x000000);
  msymbol_put (msymbol ("silver"), M_rgb, (void *) 0xC0C0C0);
  msymbol_put (msymbol ("gray"), M_rgb, (void *) 0x808080);
  msymbol_put (msymbol ("white"), M_rgb, (void *) 0xFFFFFF);
  msymbol_put (msymbol ("maroon"), M_rgb, (void *) 0x800000);
  msymbol_put (msymbol ("red"), M_rgb, (void *) 0xFF0000);
  msymbol_put (msymbol ("purple"), M_rgb, (void *) 0x800080);
  msymbol_put (msymbol ("fuchsia"), M_rgb, (void *) 0xFF00FF);
  msymbol_put (msymbol ("green"), M_rgb, (void *) 0x008000);
  msymbol_put (msymbol ("lime"), M_rgb, (void *) 0x00FF00);
  msymbol_put (msymbol ("olive"), M_rgb, (void *) 0x808000);
  msymbol_put (msymbol ("yellow"), M_rgb, (void *) 0xFFFF00);
  msymbol_put (msymbol ("navy"), M_rgb, (void *) 0x000080);
  msymbol_put (msymbol ("blue"), M_rgb, (void *) 0x0000FF);
  msymbol_put (msymbol ("teal"), M_rgb, (void *) 0x008080);
  msymbol_put (msymbol ("aqua"), M_rgb, (void *) 0x00FFFF);

  {
    char *rgb_path[]
      =  {"/usr/lib/X11/rgb.txt", "/usr/X11R6/lib/X11/rgb.txt",
	  "/etc/X11/rgb.txt" };

    fp = NULL;
    for (i = 0; i < (sizeof rgb_path) / (sizeof rgb_path[0]); i++)
      if ((fp = fopen ("/usr/lib/X11/rgb.txt", "r")))
	break;
    if (! fp)
      return;
  }
  while (1)
    {
      char buf[256];
      int c, len;

      if ((c = getc (fp)) == EOF) 
	break;
      if (c == '!')
	{
	  while ((c = getc (fp)) != EOF && c != '\n');
	  continue;
	}
      ungetc (c, fp);
      if (fscanf (fp, "%d %d %d", &r, &g, &b) != 3)
	break;
      while ((c = getc (fp)) != EOF && isspace (c));
      if (c == EOF)
	break;
      buf[0] = c;
      fgets (buf + 1, 255, fp);
      len = strlen (buf);
      for (i = 0; i < len; i++)
	buf[i] = tolower (buf[i]);
      if (buf[len - 1] == '\n')
	buf[len - 1] = '\0';
      b |= (r << 16) | (g << 8);
      msymbol_put (msymbol (buf), M_rgb, (void *) b);
    }
  fclose (fp);
}
   

static int
parse_color (MSymbol sym)
{
  char *name = MSYMBOL_NAME (sym);
  unsigned r = 0x80, g = 0x80, b = 0x80;
  int i;
  
  do {
    if (strncmp (name , "rgb:", 4) == 0)
      {
	name += 4;
	if (sscanf (name, "%x", &r) < 1)
	  break;
	for (i = 0; *name != '/'; i++, name++);
	r = (i == 1 ? ((r << 1) | r) : (r >> (i - 2)));
	name++;
	if (sscanf (name, "%x", &g) < 1)
	  break;
	for (i = 0; *name != '/'; i++, name++);
	g = (i == 1 ? ((g << 1) | g) : (g >> (i - 2)));
	name += 4;
	if (sscanf (name, "%x", &b) < 1)
	  break;
	for (i = 0; *name; i++, name++);
	b = (i == 1 ? ((b << 1) | b) : (b >> (i - 2)));
      }
    else if (*name == '#')
      {
	name++;
	i = strlen (name);
	if (i == 3)
	  {
	    if (sscanf (name, "%1x%1x%1x", &r, &g, &b) < 3)
	      break;
	    r <<= 4, g <<= 4, b <<= 4;
	  }
	else if (i == 6)
	  {
	    if (sscanf (name, "%2x%2x%2x", &r, &g, &b) < 3)
	      break;
	  }
	else if (i == 9)
	  {
	    if (sscanf (name, "%3x%3x%3x", &r, &g, &b) < 3)
	      break;
	    r >>= 1, g >>= 1, b >>= 1;
	  }
	else if (i == 12)
	  {
	    if (sscanf (name, "%4x%4x%4x", &r, &g, &b) < 3)
	      break;
	    r >>= 2, g >>= 2, b >>= 2;
	  }
      }
    else
      return (int) msymbol_get (sym, M_rgb);
  } while (0);

  return ((r << 16) | (g << 8) | b);
}


static gdImagePtr
get_scrach_image (gdImagePtr img, int width, int height)
{
  int index;
  gdImagePtr scratch;

#if HAVE_GD == 1
  index = 0;
#else
  index = img->trueColor ? 1 : 0;
#endif
  scratch = scratch_images[index];

  if (scratch)
    {
      if (scratch->sx <= width && scratch->sy <= height)
	return scratch;
      gdImageDestroy (scratch);
    }
#if HAVE_GD > 1
  if (img->trueColor)
    scratch = scratch_images[1] = gdImageCreateTrueColor (width, height);
  else
#endif
    scratch = scratch_images[0] = gdImageCreate (width, height);
  return scratch;
}  

static int
intersect_rectangle (MDrawMetric *r1, MDrawMetric *r2, MDrawMetric *rect)
{
  *rect = *r1;
  if (rect->x < r2->x)
    rect->width -= (r2->x - rect->x), rect->x = r2->x;
  if (rect->x + rect->width > r2->x + r2->width)
    rect->width -= (r2->x + r2->width - rect->x - rect->width);
  if (rect->y < r2->y)
    rect->height -= (r2->y - rect->y), rect->y = r2->y;
  if (rect->y + rect->height > r2->y + r2->height)
    rect->height -= (r2->y + r2->height - rect->y - rect->height);
  return 1;
}


#define INTERSECT_RECTANGLE(r1, r2, rect)	\
  (((r1)->x + (r1->width) <= (r2)->x		\
    || (r2)->x + (r2)->width <= (r1)->x		\
    || (r1)->y + (r1->height) <= (r2)->y	\
    || (r2)->y + (r2)->height <= (r1)->y)	\
   ? 0						\
   : intersect_rectangle (r1, r2, rect))


#define RESOLVE_COLOR(img, color)					\
  gdImageColorResolve ((img), (color) >> 16, ((color) >> 8) & 0xFF,	\
		       (color) & 0xFF)

static MRealizedFont *gd_font_open (MFrame *, MFont *, MFont *,
				    MRealizedFont *);
static void gd_render (MDrawWindow, int, int, MGlyphString *,
		       MGlyph *, MGlyph *, int, MDrawRegion);

static MFontDriver gd_font_driver =
  { NULL, gd_font_open, NULL, NULL, NULL, gd_render, NULL };

static MRealizedFont *
gd_font_open (MFrame *frame, MFont *font, MFont *spec, MRealizedFont *rfont)
{
  double size = font->size ? font->size : spec->size;
  int reg = spec->property[MFONT_REGISTRY];
  MRealizedFont *new;

  if (rfont)
    {
      MRealizedFont *save = NULL;

      for (; rfont; rfont = rfont->next)
	if (rfont->font == font
	    && (rfont->font->size ? rfont->font->size == size
		: rfont->spec.size == size)
	    && rfont->spec.property[MFONT_REGISTRY] == reg)
	  {
	    if (! save)
	      save = rfont;
	    if (rfont->driver == &gd_font_driver)
	      return rfont;
	  }
      rfont = save;
    }
  rfont = (mfont__ft_driver.open) (frame, font, spec, rfont);
  if (! rfont)
    return NULL;
  M17N_OBJECT_REF (rfont->info);
  MSTRUCT_CALLOC (new, MERROR_GD);
  *new = *rfont;
  new->driver = &gd_font_driver;
  new->next = MPLIST_VAL (frame->realized_font_list);
  MPLIST_VAL (frame->realized_font_list) = new;
  return new;
}

static void
gd_render (MDrawWindow win, int x, int y,
	   MGlyphString *gstring, MGlyph *from, MGlyph *to,
	   int reverse, MDrawRegion region)
{
  gdImagePtr img = (gdImagePtr) win;
  FT_Face ft_face;
  MRealizedFace *rface = from->rface;
  FT_Int32 load_flags = FT_LOAD_RENDER;
  int i, j;
  int color, pixel;
  int r, g, b;
  
  if (from == to)
    return;

  /* It is assured that the all glyphs in the current range use the
     same realized face.  */
  ft_face = rface->rfont->fontp;
  color = ((int *) rface->info)[reverse ? COLOR_INVERSE : COLOR_NORMAL];
  pixel = RESOLVE_COLOR (img, color);

  if (gstring->anti_alias)
    r = color >> 16, g = (color >> 8) & 0xFF, b = color & 0xFF;
  else
    {
#ifdef FT_LOAD_TARGET_MONO
      load_flags |= FT_LOAD_TARGET_MONO;
#else
      load_flags |= FT_LOAD_MONOCHROME;
#endif
    }

  for (; from < to; x += from++->g.xadv)
    {
      unsigned char *bmp;
      int xoff, yoff;
      int width, pitch;

      FT_Load_Glyph (ft_face, (FT_UInt) from->g.code, load_flags);
      yoff = y - ft_face->glyph->bitmap_top + from->g.yoff;
      bmp = ft_face->glyph->bitmap.buffer;
      width = ft_face->glyph->bitmap.width;
      pitch = ft_face->glyph->bitmap.pitch;
      if (! gstring->anti_alias)
	pitch *= 8;
      if (width > pitch)
	width = pitch;

      if (gstring->anti_alias)
	for (i = 0; i < ft_face->glyph->bitmap.rows;
	     i++, bmp += ft_face->glyph->bitmap.pitch, yoff++)
	  {
	    xoff = x + ft_face->glyph->bitmap_left + from->g.xoff;
	    for (j = 0; j < width; j++, xoff++)
	      if (bmp[j] > 0)
		{
		  int pixel1 = pixel;
#if HAVE_GD > 1
		  int alpha = gdAlphaTransparent * (255 - bmp[j]) / 255;

		  if (alpha > 0)
		    pixel1 = gdImageColorResolveAlpha (img, r, g, b, alpha);
#else
		  int f = bmp[j] >> 5;

		  if (f < 7)
		    {
		      int r1, g1, b1, color1;

		      pixel1 = gdImageGetPixel (img, xoff, yoff);
		      r1 = gdImageRed (img, pixel1);
		      g1 = gdImageGreen (img, pixel1);
		      b1 = gdImageBlue (img, pixel1);
		      color1 = ((((r * f + r1 * (7 - f)) / 7) << 16)
				| (((g * f + g1 * (7 - f)) / 7) << 8)
				| ((b * f + b1 * (7 - f)) / 7));
		      pixel1 = RESOLVE_COLOR (img, color1);
		    }
#endif
		  gdImageSetPixel (img, xoff, yoff, pixel1);
		}
	  }
      else
	for (i = 0; i < ft_face->glyph->bitmap.rows;
	     i++, bmp += ft_face->glyph->bitmap.pitch, yoff++)
	  {
	    xoff = x + ft_face->glyph->bitmap_left + from->g.xoff;
	    for (j = 0; j < width; j++, xoff++)
	      if (bmp[j / 8] & (1 << (7 - (j % 8))))
		gdImageSetPixel (img, xoff, yoff, pixel);
	  }
    }
}

static void
gd_close (MFrame *frame)
{
}

static void *
gd_get_prop (MFrame *frame, MSymbol key)
{
  return NULL;
}

static void
gd_realize_face (MRealizedFace *rface)
{
  int *colors;
  MFaceHLineProp *hline;
  MFaceBoxProp *box;
  MSymbol *props = (MSymbol *) rface->face.property;

  if (rface != rface->ascii_rface)
    {
      rface->info = rface->ascii_rface->info;
      return;
    }
  colors = malloc (sizeof (int) * COLOR_MAX);
  colors[COLOR_NORMAL] = parse_color (props[MFACE_FOREGROUND]);
  colors[COLOR_INVERSE] = parse_color (props[MFACE_BACKGROUND]);
  if (rface->face.property[MFACE_VIDEOMODE] == Mreverse)
    {
      colors[COLOR_HLINE] = colors[COLOR_NORMAL];
      colors[COLOR_NORMAL] = colors[COLOR_INVERSE];
      colors[COLOR_INVERSE] = colors[COLOR_HLINE];
    }
  colors[COLOR_HLINE] = 0;

  hline = rface->hline;
  if (hline)
    {
      if (hline->color)
	colors[COLOR_HLINE] = parse_color (hline->color);
      else
	colors[COLOR_HLINE] = colors[COLOR_NORMAL];
    }

  box = rface->box;
  if (box)
    {
      if (box->color_top)
	colors[COLOR_BOX_TOP] = parse_color (box->color_top);
      else
	colors[COLOR_BOX_TOP] = colors[COLOR_NORMAL];

      if (box->color_left && box->color_left != box->color_top)
	colors[COLOR_BOX_LEFT] = parse_color (box->color_left);
      else
	colors[COLOR_BOX_LEFT] = colors[COLOR_BOX_TOP];

      if (box->color_bottom && box->color_bottom != box->color_top)
	colors[COLOR_BOX_BOTTOM] = parse_color (box->color_bottom);
      else
	colors[COLOR_BOX_BOTTOM] = colors[COLOR_BOX_TOP];

      if (box->color_right && box->color_right != box->color_bottom)
	colors[COLOR_BOX_RIGHT] = parse_color (box->color_right);
      else
	colors[COLOR_BOX_RIGHT] = colors[COLOR_BOX_BOTTOM];
    }

  rface->info = colors;
}

static void
gd_free_realized_face (MRealizedFace *rface)
{
  free (rface->info);
}

static void
gd_fill_space (MFrame *frame, MDrawWindow win, MRealizedFace *rface,
	       int reverse,
	       int x, int y, int width, int height, MDrawRegion region)
{
  gdImagePtr img = (gdImagePtr) win;
  int *colors = rface->info;
  int color = colors[reverse ? COLOR_NORMAL : COLOR_INVERSE];
  MPlist *region_list = region, *plist;

  color = RESOLVE_COLOR (img, color);
  if (! region)
    gdImageFilledRectangle (img, x, y, x + width - 1, y + height - 1, color);
  else
    {
      MDrawMetric rect;;

      rect.x = x, rect.y = y, rect.width = width, rect.height = height;
      MPLIST_DO (plist, region_list)
	{
	  MDrawMetric *r = MPLIST_VAL (plist), new;

	  if (INTERSECT_RECTANGLE (r, &rect, &new))
	    gdImageFilledRectangle (img, new.x, new.y, new.x + new.width - 1,
				    new.y + new.height - 1, color);
	}
    }
}

static void
gd_draw_empty_boxes (MDrawWindow win, int x, int y,
		     MGlyphString *gstring, MGlyph *from, MGlyph *to,
		     int reverse, MDrawRegion region)
{
  gdImagePtr img = (gdImagePtr) win;
  int *colors = from->rface->info;
  int color = colors[reverse ? COLOR_INVERSE : COLOR_NORMAL];
  MPlist *region_list = region, *plist;
  int height;

  if (from == to)
    return;

  color = RESOLVE_COLOR (img, color);
  y -= gstring->ascent - 1;
  height = gstring->ascent + gstring->descent - 2;
  if (! region)
    for (; from < to; x += from++->g.xadv)
      gdImageRectangle (img, x, y, x + from->g.xadv - 2, y + height - 1, color);
  else
    {
      gdImagePtr cpy;
      MGlyph *g;
      int width, x1;

      for (g = from, width = 0; g < to; width += g++->g.xadv);
      cpy = get_scrach_image (img, width, height);
      MPLIST_DO (plist, region_list)
	{
	  MDrawMetric *rect = MPLIST_VAL (plist);
	  gdImageCopy (cpy, img, rect->x - x, rect->y - y, rect->x, rect->y,
		       rect->x + rect->width, rect->y + rect->height);
	}
      for (x1 = 0; from < to; x1 += from++->g.xadv)
	gdImageRectangle (cpy, x1, 0, x1 + from->g.xadv - 2, height - 1, color);
      MPLIST_DO (plist, region_list)
	{
	  MDrawMetric *rect = MPLIST_VAL (plist);
	  gdImageCopy (img, cpy, rect->x, rect->y, rect->x - x, rect->y - y,
		       rect->x + rect->width, rect->y + rect->height);
	}
    }
}


static void
gd_draw_hline (MFrame *frame, MDrawWindow win, MGlyphString *gstring,
	       MRealizedFace *rface, int reverse,
	       int x, int y, int width, MDrawRegion region)
{
  enum MFaceHLineType type = rface->hline->type;
  int height = rface->hline->width;
  gdImagePtr img = (gdImagePtr) win;
  int *colors = rface->info;
  int color = colors[COLOR_HLINE];
  MPlist *region_list = region, *plist;

  color = RESOLVE_COLOR (img, color);
  y = (type == MFACE_HLINE_BOTTOM
       ? y + gstring->text_descent - height
       : type == MFACE_HLINE_UNDER
       ? y + 1
       : type == MFACE_HLINE_STRIKE_THROUGH
       ? y - ((gstring->ascent + gstring->descent) / 2)
       : y - gstring->text_ascent);
  if (! region)
    gdImageFilledRectangle (img, x, y, x + width - 1, y + height - 1, color);
  else
    {
      MDrawMetric rect;

      rect.x = x, rect.y = y, rect.width = width, rect.height = height;
      MPLIST_DO (plist, region_list)
	{
	  MDrawMetric *r = MPLIST_VAL (plist), new;

	  if (INTERSECT_RECTANGLE (r, &rect, &new))
	    gdImageFilledRectangle (img, new.x, new.y, new.x + new.width - 1,
				    new.y + new.height - 1, color);
	}
    }
}


static void
gd_draw_box (MFrame *frame, MDrawWindow win, MGlyphString *gstring,
	     MGlyph *g, int x, int y, int width, MDrawRegion region)
{
  gdImagePtr img = (gdImagePtr) win;
  int *colors = g->rface->info;
  int color;
  MRealizedFace *rface = g->rface;
  MFaceBoxProp *box = rface->box;
  MPlist *region_list = region, *plist;
  int y0, y1;
  int i;

  y0 = y - (gstring->text_ascent
	    + rface->box->inner_vmargin + rface->box->width);
  y1 = y + (gstring->text_descent
	    + rface->box->inner_vmargin + rface->box->width - 1);

  if (region)
    {
      int height = y1 - y0;
      gdImagePtr cpy;

      if (g->type == GLYPH_BOX)
	width = g->g.xadv;
      cpy = get_scrach_image (img, width, height);
      MPLIST_DO (plist, region_list)
	{
	  MDrawMetric *rect = MPLIST_VAL (plist);
	  gdImageCopy (cpy, img, rect->x - x, rect->y - y, rect->x, rect->y,
		       rect->x + rect->width, rect->y + rect->height);
	}
      gd_draw_box (frame, win, gstring, g, 0, y - y0, width, NULL);
      MPLIST_DO (plist, region_list)
	{
	  MDrawMetric *rect = MPLIST_VAL (plist);
	  gdImageCopy (img, cpy, rect->x, rect->y, rect->x - x, rect->y - y,
		       rect->x + rect->width, rect->y + rect->height);
	}
      return;
    }

  if (g->type == GLYPH_BOX)
    {
      int x0, x1;

      if (g->left_padding)
	x0 = x + box->outer_hmargin, x1 = x + g->g.xadv - 1;
      else
	x0 = x, x1 = x + g->g.xadv - box->outer_hmargin - 1;

      /* Draw the top side.  */
      color = RESOLVE_COLOR (img, colors[COLOR_BOX_TOP]);
      for (i = 0; i < box->width; i++)
	gdImageLine (img, x0, y0 + i, x1, y0 + i, color);

      /* Draw the bottom side.  */
      color = RESOLVE_COLOR (img, colors[COLOR_BOX_BOTTOM]);
      for (i = 0; i < box->width; i++)
	gdImageLine (img, x0, y1 - i, x1, y1 - i, color);

      if (g->left_padding > 0)
	{
	  /* Draw the left side.  */
	  color = RESOLVE_COLOR (img, colors[COLOR_BOX_LEFT]);
	  for (i = 0; i < rface->box->width; i++)
	    gdImageLine (img, x0 + i, y0 + i, x0 + i, y1 - i, color);
	}
      else
	{
	  /* Draw the right side.  */
	  color = RESOLVE_COLOR (img, colors[COLOR_BOX_RIGHT]);
	  for (i = 0; i < rface->box->width; i++)
	    gdImageLine (img, x1 - i, y0 + i, x1 - i, y1 - i, color);
	}

    }
  else
    {
      /* Draw the top side.  */
      color = RESOLVE_COLOR (img, colors[COLOR_BOX_TOP]);
      for (i = 0; i < box->width; i++)
	gdImageLine (img, x, y0 + i, x + width - 1, y0 + i, color);

      /* Draw the bottom side.  */
      color = RESOLVE_COLOR (img, colors[COLOR_BOX_BOTTOM]);
      for (i = 0; i < box->width; i++)
	gdImageLine (img, x, y1 - i, x + width - 1, y1 - i, color);
    }
}


static MDrawRegion
gd_region_from_rect (MDrawMetric *rect)
{
  MDrawMetric *new;
  MPlist *plist = mplist ();

  MSTRUCT_MALLOC (new, MERROR_GD);
  *new = *rect;
  mplist_add (plist, Mt, new);
  return (MDrawRegion) plist;
}

static void
gd_union_rect_with_region (MDrawRegion region, MDrawMetric *rect)
{
  MPlist *plist = (MPlist *) region;
  MDrawMetric *r;

  MSTRUCT_MALLOC (r, MERROR_GD);
  *r = *rect;
  mplist_push (plist, Mt, r);
}

static void
gd_intersect_region (MDrawRegion region1, MDrawRegion region2)
{
  MPlist *plist1 = (MPlist *) region1, *p1 = plist1;
  MPlist *plist2 = (MPlist *) region2;
  MPlist *p2;
  MDrawMetric rect, *rect1, *rect2, *r;

  while (! MPLIST_TAIL_P (p1))
    {
      rect1 = mplist_pop (p1);
      MPLIST_DO (p2, plist2)
	{
	  rect2 = MPLIST_VAL (p2);
	  if (INTERSECT_RECTANGLE (rect1, rect2, &rect))
	    {
	      MSTRUCT_MALLOC (r, MERROR_GD);
	      *r = rect;
	      mplist_push (p1, Mt, r);
	      p1 = MPLIST_NEXT (p1);
	    }
	}
      free (rect1);
    }
}

static void
gd_region_add_rect (MDrawRegion region, MDrawMetric *rect)
{
  MPlist *plist = (MPlist *) region;
  MDrawMetric *new;

  MSTRUCT_MALLOC (new, MERROR_GD);
  *new = *rect;
  mplist_push (plist, Mt, new);
}

static void
gd_region_to_rect (MDrawRegion region, MDrawMetric *rect)
{
  MPlist *plist = (MPlist *) region;
  MDrawMetric *r = MPLIST_VAL (plist);
  int min_x = r->x, max_x = min_x + r->width;
  int min_y = r->y, max_y = min_y + r->height;
  
  MPLIST_DO (plist, MPLIST_NEXT (plist))
    {
      r = MPLIST_VAL (plist);
      if (r->x < min_x)
	min_x = r->x;
      if (r->x + r->width > max_x)
	max_x = r->x + r->width;
      if (r->y < min_y)
	min_y = r->y;
      if (r->y + r->height > max_y)
	max_y = r->y + r->height;
    }
  rect->x = min_x;
  rect->y = min_y;
  rect->width = max_x - min_x;
  rect->height =max_y - min_y;
}

static void
gd_free_region (MDrawRegion region)
{
  MPlist *plist = (MPlist *) region;

  MPLIST_DO (plist, plist)
    free (MPLIST_VAL (plist));
  M17N_OBJECT_UNREF (region);
}

static void
gd_dump_region (MDrawRegion region)
{
  MDrawMetric rect;

  gd_region_to_rect (region, &rect);
  fprintf (stderr, "(%d %d %d %d)\n", rect.x, rect.y, rect.width, rect.height);
}

static MDeviceDriver gd_driver =
  {
    gd_close,
    gd_get_prop,
    gd_realize_face,
    gd_free_realized_face,
    gd_fill_space,
    gd_draw_empty_boxes,
    gd_draw_hline,
    gd_draw_box,
    NULL,
    gd_region_from_rect,
    gd_union_rect_with_region,
    gd_intersect_region,
    gd_region_add_rect,
    gd_region_to_rect,
    gd_free_region,
    gd_dump_region,
  };

/* Functions to be stored in MDeviceLibraryInterface by dlsym ().  */

int
device_init ()
{
  M_rgb = msymbol ("  rgb");
  read_rgb_txt ();
  realized_fontset_list = mplist ();
  realized_font_list = mplist ();
  realized_face_list = mplist ();  
  scratch_images[0] = scratch_images[1] = NULL;

  gd_font_driver.select = mfont__ft_driver.select;
  gd_font_driver.find_metric = mfont__ft_driver.find_metric;
  gd_font_driver.has_char = mfont__ft_driver.has_char;
  gd_font_driver.encode_char = mfont__ft_driver.encode_char;
  gd_font_driver.list = mfont__ft_driver.list;
  gd_font_driver.check_otf = mfont__ft_driver.check_otf;
  gd_font_driver.drive_otf = mfont__ft_driver.drive_otf;

  return 0;
}

int
device_fini ()
{
  MPlist *plist;
  int i;

  MPLIST_DO (plist, realized_fontset_list)
    mfont__free_realized_fontset ((MRealizedFontset *) MPLIST_VAL (plist));
  M17N_OBJECT_UNREF (realized_fontset_list);

  MPLIST_DO (plist, realized_face_list)
    {
      MRealizedFace *rface = MPLIST_VAL (plist);

      free (rface->info);
      mface__free_realized (rface);
    }
  M17N_OBJECT_UNREF (realized_face_list);

  if (MPLIST_VAL (realized_font_list))
    mfont__free_realized (MPLIST_VAL (realized_font_list));
  M17N_OBJECT_UNREF (realized_font_list);

  for (i = 0; i < 2; i++)
    if (scratch_images[i])
      gdImageDestroy (scratch_images[i]);
  return 0;
}

int
device_open (MFrame *frame, MPlist *param)
{
  MFace *face;

  frame->device = NULL;
  frame->device_type = MDEVICE_SUPPORT_OUTPUT;
  frame->dpi = (int) mplist_get (param, Mresolution);
  if (frame->dpi == 0)
    frame->dpi = 100;
  frame->driver = &gd_driver;
  frame->font_driver_list = mplist ();
  mplist_add (frame->font_driver_list, Mfreetype, &gd_font_driver);
  frame->realized_font_list = realized_font_list;
  frame->realized_face_list = realized_face_list;
  frame->realized_fontset_list = realized_fontset_list;
  face = mface_copy (mface__default);
  mface_put_prop (face, Mfoundry, Mnil);
  mface_put_prop (face, Mfamily, Mnil);
  mplist_push (param, Mface, face);
  M17N_OBJECT_UNREF (face);
  return 0;
}

#else	/* not HAVE_GD nor HAVE_FREETYPE */

int device_open () { return -1; }

#endif	/* not HAVE_GD nor HAVE_FREETYPE */

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */
