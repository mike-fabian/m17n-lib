/* internal-gui.h -- common header file for the internal GUI API.
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

#ifndef _M_INTERNAL_GUI_H
#define _M_INTERNAL_GUI_H

typedef struct MWDevice MWDevice;

extern MSymbol Mfont;

typedef struct MRealizedFont MRealizedFont;
typedef struct MRealizedFace MRealizedFace;
typedef struct MRealizedFontset MRealizedFontset;

/** Information about a frame.  */

struct MFrame
{
  M17NObject control;

  /** Pointer to a window-system dependent device object associated
      with the frame.  */
  MWDevice *device;

  MSymbol foreground, background, videomode;

  MFont *font;

  /** The default face of the frame.  */
  MFace *face;

  /** The default realized face of the frame.  */
  MRealizedFace *rface;

  /** The default width of one-char space.  It is a width of SPACE
      character of the default face.  */
  int space_width;

  /** The default ascent and descent of a line.  It is ascent and
      descent of ASCII font of the default face.  */
  int ascent, descent;

  /** The following three members are set by mwin__open_device ().  */

  /** List of realized fonts.  */
  MPlist *realized_font_list;

  /** List of realized faces.  */
  MPlist *realized_face_list;

  /** List of realized fontsets.  */
  MPlist *realized_fontset_list;
};

enum glyph_type
  {
    GLYPH_CHAR,
    GLYPH_SPACE,
    GLYPH_PAD,
    GLYPH_BOX,
    GLYPH_ANCHOR,
    GLYPH_TYPE_MAX
  };

typedef struct
{
  int pos, to;
  int c;
  unsigned code;
  MSymbol category;
  MRealizedFace *rface;
  short width, ascent, descent, lbearing, rbearing;
  short xoff, yoff;
  unsigned enabled : 1;
  unsigned left_padding : 1;
  unsigned right_padding : 1;
  unsigned otf_encoded : 1;
  unsigned bidi_level : 6;
  enum glyph_type type : 3;
  int combining_code;
} MGlyph;

struct MGlyphString
{
  M17NObject head;

  int size, inc, used;
  MGlyph *glyphs;
  MText *mt;
  int from, to;
  short width, height, ascent, descent;
  short physical_ascent, physical_descent, lbearing, rbearing;
  short text_ascent, text_descent, line_ascent, line_descent;
  int indent, width_limit;

  /* Members to keep temporary data while layouting.  */
  short sub_width, sub_lbearing, sub_rbearing;

  /* Copied for <control>.anti_alias but never set if the frame's
     depth is less than 8.  */
  unsigned anti_alias : 1;

  MDrawControl control;

  MDrawRegion region;

  struct MGlyphString *next, *top;
};

#define MGLYPH(idx)	\
  (gstring->glyphs + ((idx) >= 0 ? (idx) : (gstring->used + (idx))))

#define GLYPH_INDEX(g)	\
  ((g) - gstring->glyphs)

#define INIT_GLYPH(g)	\
  (memset (&(g), 0, sizeof (g)))

#define APPEND_GLYPH(gstring, g)	\
  MLIST_APPEND1 ((gstring), glyphs, (g), MERROR_DRAW)

#define INSERT_GLYPH(gstring, at, g)				\
  do {								\
    MLIST_INSERT1 ((gstring), glyphs, (at), 1, MERROR_DRAW);	\
    (gstring)->glyphs[at] = g;					\
  } while (0)

#define DELETE_GLYPH(gstring, at)		\
  do {						\
    MLIST_DELETE1 (gstring, glyphs, at, 1);	\
  } while (0)

#define REPLACE_GLYPHS(gstring, from, to, len)				  \
  do {									  \
    int newlen = (gstring)->used - (from);				  \
    int diff = newlen - (len);						  \
									  \
    if (diff < 0)							  \
      MLIST_DELETE1 (gstring, glyphs, (to) + newlen, -diff);		  \
    else if (diff > 0)							  \
      MLIST_INSERT1 ((gstring), glyphs, (to) + (len), diff, MERROR_DRAW); \
    memmove ((gstring)->glyphs + to, (gstring)->glyphs + (from + diff),	  \
	     (sizeof (MGlyph)) * newlen);				  \
    (gstring)->used -= newlen;						  \
  } while (0)

#define MAKE_COMBINING_CODE(base_y, base_x, add_y, add_x, off_y, off_x)	\
  (((off_y) << 16)							\
   | ((off_x) << 8)							\
   | ((base_x) << 6)							\
   | ((base_y) << 4)							\
   | ((add_x) << 2)							\
   | (add_y))

#define COMBINING_CODE_OFF_Y(code) (((code) >> 16) & 0xFF)
#define COMBINING_CODE_OFF_X(code) (((code) >> 8) & 0xFF)
#define COMBINING_CODE_BASE_X(code) (((code) >> 6) & 0x3)
#define COMBINING_CODE_BASE_Y(code) (((code) >> 4) & 0x3)
#define COMBINING_CODE_ADD_X(code) (((code) >> 2) & 0x3)
#define COMBINING_CODE_ADD_Y(code) ((code) & 0x3)

#define MAKE_COMBINING_CODE_BY_CLASS(class) (0x1000000 | class)

#define COMBINING_BY_CLASS_P(code) ((code) & 0x1000000)

#define COMBINING_CODE_CLASS(code) ((code) & 0xFFFFFF)

typedef struct MGlyphString MGlyphString;

typedef struct MFontDriver MFontDriver;

typedef struct
{
  short x, y;
} MDrawPoint;

extern int mfont__init ();
extern void mfont__fini ();

extern int mface__init ();
extern void mface__fini ();

extern int mdraw__init ();
extern void mdraw__fini ();

extern int mfont__fontset_init ();
extern void mfont__fontset_fini ();

extern int minput__win_init ();
extern void minput__win_fini ();

extern int mwin__init ();
extern void mwin__fini ();

extern MWDevice *mwin__open_device (MFrame *frame, MPlist *plist);

extern void mwin__close_device (MFrame *frame);

extern void *mwin__device_get_prop (MWDevice *device, MSymbol key);

extern int mwin__parse_font_name (char *name, MFont *font);

extern char *mwin__build_font_name (MFont *font);

extern void mwin__realize_face (MRealizedFace *rface);

extern void mwin__free_realized_face (MRealizedFace *rface);

extern void mwin__fill_space (MFrame *frame, MDrawWindow win,
			      MRealizedFace *rface, int reverse,
			      int x, int y, int width, int height,
			      MDrawRegion region);

extern void mwin__draw_hline (MFrame *frame, MDrawWindow win,
			      MGlyphString *gstring,
			      MRealizedFace *rface, int reverse,
			      int x, int y, int width, MDrawRegion region);

extern void mwin__draw_box (MFrame *frame, MDrawWindow win,
			    MGlyphString *gstring,
			    MGlyph *g, int x, int y, int width,
			    MDrawRegion region);

extern void mwin__draw_points (MFrame *frame, MDrawWindow win,
			       MRealizedFace *rface,
			       int intensity, MDrawPoint *points, int num,
			       MDrawRegion region);

extern MDrawRegion mwin__region_from_rect (MDrawMetric *rect);

extern void mwin__union_rect_with_region (MDrawRegion region,
					  MDrawMetric *rect);

extern void mwin__intersect_region (MDrawRegion region1, MDrawRegion region2);

extern void mwin__region_add_rect (MDrawRegion region, MDrawMetric *rect);

extern void mwin__region_to_rect (MDrawRegion region, MDrawMetric *rect);

extern void mwin__free_region (MDrawRegion region);

extern void mwin__verify_region (MFrame *frame, MDrawRegion region);

extern void mwin__dump_region (MDrawRegion region);

extern MDrawWindow mwin__create_window (MFrame *frame, MDrawWindow parent);

extern void mwin__destroy_window (MFrame *frame, MDrawWindow win);

#if 0
extern MDrawWindow mwin__event_window (void *event);

extern void mwin__print_event (void *event, char *win_name);
#endif

extern void mwin__map_window (MFrame *frame, MDrawWindow win);

extern void mwin__unmap_window (MFrame *frame, MDrawWindow win);

extern void mwin__window_geometry (MFrame *frame, MDrawWindow win,
				   MDrawWindow parent, MDrawMetric *geometry);

extern void mwin__adjust_window (MFrame *frame, MDrawWindow win,
				 MDrawMetric *current, MDrawMetric *new);

extern MSymbol mwin__parse_event (MFrame *frame, void *arg, int *modifiers);

#ifdef HAVE_XFT2

#include <ft2build.h>
#include FT_FREETYPE_H

extern void *mwin__xft_open (MFrame *frame, char *fontname, int size);
extern void mwin__xft_close (void *xft_info);
extern void mwin__xft_get_metric (void *xft_info, FT_Face ft_face, MGlyph *g);
extern void mwin__xft_render (MDrawWindow win, int x, int y,
			      MGlyphString *gstring, MGlyph *from, MGlyph *to,
			      int reverse, MDrawRegion region,
			      void *xft_info, FT_Face ft_face);
#endif	/* HAVE_XFT2 */

#endif /* _M_INTERNAL_GUI_H */
