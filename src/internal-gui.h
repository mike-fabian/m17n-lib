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
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   02111-1307, USA.  */

#ifndef _M_INTERNAL_GUI_H
#define _M_INTERNAL_GUI_H

enum MDeviceType
  {
    MDEVICE_SUPPORT_OUTPUT = 1,
    MDEVICE_SUPPORT_INPUT = 2
  };

typedef struct MRealizedFont MRealizedFont;
typedef struct MRealizedFace MRealizedFace;
typedef struct MRealizedFontset MRealizedFontset;
typedef struct MDeviceDriver MDeviceDriver;

/** Information about a frame.  */

struct MFrame
{
  M17NObject control;

  MSymbol foreground, background, videomode;

  MFont *font;

  /** The default face of the frame.  */
  MFace *face;

  /** The default realized face of the frame.  */
  MRealizedFace *rface;

  /** The default width of one-char space.  It is a width of SPACE
      character of the default face.  */
  int space_width;

  int average_width;

  /** The default ascent and descent of a line.  It is ascent and
      descent of ASCII font of the default face.  */
  int ascent, descent;

  /** Initialized to 0 and incremented on each modification of a face
      on which one of the realized faces is based.  */
  unsigned tick;

  /** Pointer to device dependent information associated with the
      frame.  */
  void *device;

  /** The following members are set by "device_open" function of a
      device dependent library.  */

  /** Logical OR of enum MDeviceType.  */
  int device_type;

  /** Resolution (dots per inch) of the device.  */
  int dpi;

  /** Correction of functions to manipulate the device.  */
  MDeviceDriver *driver;

  /** List of font drivers.  */
  MPlist *font_driver_list;

  /** List of realized fonts.  */
  MPlist *realized_font_list;

  /** List of realized faces.  */
  MPlist *realized_face_list;

  /** List of realized fontsets.  */
  MPlist *realized_fontset_list;
};

#define M_CHECK_WRITABLE(frame, err, ret)			\
  do {								\
    if (! ((frame)->device_type & MDEVICE_SUPPORT_OUTPUT))	\
      MERROR ((err), (ret));					\
  } while (0)

#define M_CHECK_READABLE(frame, err, ret)			\
  do {								\
    if (! ((frame)->device_type & MDEVICE_SUPPORT_INPUT))	\
      MERROR ((err), (ret));					\
  } while (0)

enum glyph_type
  {
    GLYPH_CHAR,
    GLYPH_SPACE,
    GLYPH_PAD,
    GLYPH_BOX,
    GLYPH_ANCHOR,
    GLYPH_TYPE_MAX
  };

enum glyph_category
  {
    GLYPH_CATEGORY_NORMAL,
    GLYPH_CATEGORY_MODIFIER,
    GLYPH_CATEGORY_FORMATTER
  };

typedef struct
{
  int pos, to;
  int c;
  unsigned code;
  MRealizedFace *rface;
  short width, ascent, descent, lbearing, rbearing;
  short xoff, yoff;
  unsigned enabled : 1;
  unsigned left_padding : 1;
  unsigned right_padding : 1;
  unsigned otf_encoded : 1;
  unsigned bidi_level : 6;
  unsigned category : 2;
  unsigned type : 3;
  int combining_code;
} MGlyph;

struct MGlyphString
{
  M17NObject head;

  MFrame *frame;
  int tick;

  int size, inc, used;
  MGlyph *glyphs;
  int from, to;
  short width, height, ascent, descent;
  short physical_ascent, physical_descent, lbearing, rbearing;
  short text_ascent, text_descent, line_ascent, line_descent;
  int indent, width_limit;

  /* Copied for <control>.anti_alias but never set if the frame's
     depth is less than 8.  */
  unsigned anti_alias : 1;

  MDrawControl control;

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

#define MAKE_PRECOMPUTED_COMBINDING_CODE() (0x2000000)

#define COMBINING_PRECOMPUTED_P(code) ((code) & 0x2000000)

typedef struct MGlyphString MGlyphString;

typedef struct
{
  short x, y;
} MDrawPoint;

struct MDeviceDriver
{
  void (*close) (MFrame *frame);
  void *(*get_prop) (MFrame *frame, MSymbol key);
  void (*realize_face) (MRealizedFace *rface);
  void (*free_realized_face) (MRealizedFace *rface);
  void (*fill_space) (MFrame *frame, MDrawWindow win,
		      MRealizedFace *rface, int reverse,
		      int x, int y, int width, int height,
		      MDrawRegion region);
  void (*draw_empty_boxes) (MDrawWindow win, int x, int y,
			    MGlyphString *gstring,
			    MGlyph *from, MGlyph *to,
			    int reverse, MDrawRegion region);
  void (*draw_hline) (MFrame *frame, MDrawWindow win,
		      MGlyphString *gstring,
		      MRealizedFace *rface, int reverse,
		      int x, int y, int width, MDrawRegion region);
  void (*draw_box) (MFrame *frame, MDrawWindow win,
		    MGlyphString *gstring,
		    MGlyph *g, int x, int y, int width,
		    MDrawRegion region);

  void (*draw_points) (MFrame *frame, MDrawWindow win,
		       MRealizedFace *rface,
		       int intensity, MDrawPoint *points, int num,
		       MDrawRegion region);
  MDrawRegion (*region_from_rect) (MDrawMetric *rect);
  void (*union_rect_with_region) (MDrawRegion region, MDrawMetric *rect);
  void (*intersect_region) (MDrawRegion region1, MDrawRegion region2);
  void (*region_add_rect) (MDrawRegion region, MDrawMetric *rect);
  void (*region_to_rect) (MDrawRegion region, MDrawMetric *rect);
  void (*free_region) (MDrawRegion region);
  void (*dump_region) (MDrawRegion region);
  MDrawWindow (*create_window) (MFrame *frame, MDrawWindow parent);
  void (*destroy_window) (MFrame *frame, MDrawWindow win);
  void (*map_window) (MFrame *frame, MDrawWindow win);
  void (*unmap_window) (MFrame *frame, MDrawWindow win);
  void (*window_geometry) (MFrame *frame, MDrawWindow win,
			   MDrawWindow parent, MDrawMetric *geometry);
  void (*adjust_window) (MFrame *frame, MDrawWindow win,
			 MDrawMetric *current, MDrawMetric *new);
  MSymbol (*parse_event) (MFrame *frame, void *arg, int *modifiers);
};

extern MSymbol Mlatin;

extern MSymbol Mgd;

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

#endif /* _M_INTERNAL_GUI_H */
