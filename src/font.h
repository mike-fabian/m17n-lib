/* font.h -- header file for the font module.
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

#ifndef _M17N_FONT_H_
#define _M17N_FONT_H_

/** Type of font.  Now obsolete.  */

enum MFontType
  {
    /** Fonts supproted by a window system natively.  On X Window
	System, it is an X font.  */
    MFONT_TYPE_WIN,

    /** Fonts supported by FreeType library.  */
    MFONT_TYPE_FT,

    /** anchor */
    MFONT_TYPE_MAX
  };


enum MFontProperty
  {
    /* The order of MFONT_FOUNDRY to MFONT_ADSTYLE must be the same as
       enum MFaceProperty.  */
    MFONT_FOUNDRY,
    MFONT_FAMILY,
    MFONT_WEIGHT,
    MFONT_STYLE,
    MFONT_STRETCH,
    MFONT_ADSTYLE,
    MFONT_REGISTRY,
    MFONT_SIZE,
    MFONT_RESY,
    /* anchor */
    MFONT_PROPERTY_MAX
  };

/** Information about a font.  This structure is used in three ways:
    FONT-OBJ, FONT-OPENED, and FONT-SPEC.  

    FONT-OBJ: To store information of an existing font.  Instances
    appear only in <font_list> of MDisplay.

    FONT-OPENED: To store information of an opened font.  Instances
    appear only in <opend_list> of MDisplay.

    FONT-SPEC: To store specifications of a font.  Instances appear
    only in <spec_list> of MFontset.  */

struct MFont
{
  /** Numeric value of each font property.  Also used as an index to
      the table @c mfont__property_table to get the actual name of the
      property.

      For FONT-OBJ, FONT-OPENED: The value is greater than zero.

      For FONT-SPEC: The value is equal to or greater than zero.  Zero
      means that the correponding property is not specified (i.e. wild
      card).

      <property>[MFONT_SIZE] is the size of the font in 1/10 pixels.

      For FONT-OBJ: If the value is 0, the font is scalable or
      auto-scaled.

      For FONT-OPENED: The actual size of opened font.

      <prperty>[MFONT_RESY] is the designed resolution of the font in
      DPI, or zero.  Zero means that the font is scalable.

      For the time being, we mention only Y-resolution (resy) and
      assume that resx is always equal to resy.  */
  unsigned short property[MFONT_PROPERTY_MAX];
};

typedef struct
{
  int size, inc, used;
  MSymbol property;
  MSymbol *names;
} MFontPropertyTable;

extern MFontPropertyTable mfont__property_table[MFONT_REGISTRY + 1];

/** Return the symbol of the Nth font property of FONT.  */
#define FONT_PROPERTY(font, n)	\
  mfont__property_table[(n)].names[(font)->property[(n)]]

typedef struct MFontEncoding MFontEncoding;

struct MRealizedFont
{
  /* Frame on which the font is realized.  */
  MFrame *frame;

  /* Font spec used to find the font.  */
  MFont spec;

  /* Font spec requested by a face.  */
  MFont request;

  /* The found font.  */
  MFont font;

  /* How well <font> matches with <request>.  */
  int score;

  MFontDriver *driver;

  /* Font Layout Table for the font.  */
  MSymbol layouter;

  /* 0: not yet tried to open
     -1: failed to open
     1: succeessfully opened.  */
  int status;

  /* Extra information set by <driver>->select or <driver>->open.  If
     non-NULL, it must be a pointer to a managed object.  */
  void *info;

  short ascent, descent;

  MFontEncoding *encoding;
};

/** Structure to hold a list of fonts of each registry.  */

typedef struct
{
  MSymbol tag;
  int nfonts;
  MFont *fonts;
} MFontList;


struct MFontDriver
{
  /** Return a font satisfying REQUEST and best matching with SPEC.
      For the moment, LIMITTED_SIZE is ignored.  */
  MRealizedFont *(*select) (MFrame *frame, MFont *spec, MFont *request,
			    int limitted_size);

  /** Open a font specified by RFONT.  */
  int (*open) (MRealizedFont *rfont);

  /** Set metrics of glyphs in GSTRING from FROM to TO.  */
  void (*find_metric) (MRealizedFont *rfont, MGlyphString *gstring,
		       int from, int to);

  /** Encode CODE into a glyph code the font.  CODE is a code point of
      a character in rfont->encoder->encoding_charset.  If the font
      has no glyph for CODE, return MCHAR_INVALID_CODE.  */
  unsigned (*encode_char) (MRealizedFont *rfont, unsigned code);

  /** Draw glyphs from FROM to TO (exclusive) on window WIN of FRAME
      at coordinate (X, Y) relative to WIN.  */
  void (*render) (MDrawWindow win, int x, int y,
		  MGlyphString *gstring, MGlyph *from, MGlyph *to,
		  int reverse, MDrawRegion region);
};

/** Initialize the members of FONT.  */

#define MFONT_INIT(font) memset ((font), 0, sizeof (MFont))

extern MSymbol Mlayouter;

extern int mfont__flt_init ();

extern void mfont__flt_fini ();

#ifdef HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef HAVE_OTF
#include <otf.h>
#endif /* HAVE_OTF */

typedef struct
{
  M17NObject control;
  MFont font;
  char *filename;
  int otf_flag;	/* This font is OTF (1), may be OTF (0), is not OTF (-1).  */
  MPlist *charmap_list;
  int charmap_index;
  FT_Face ft_face;
#ifdef HAVE_OTF
  OTF *otf;
#endif /* HAVE_OTF */
  void *extra_info;		/* Xft uses this member.  */
} MFTInfo;

extern MFontDriver mfont__ft_driver;

extern int mfont__ft_init ();

extern void mfont__ft_fini ();

extern int mfont__ft_parse_name (char *name, MFont *font);

extern char *mfont__ft_unparse_name (MFont *font);

#ifdef HAVE_OTF

extern int mfont__ft_drive_gsub (MGlyphString *gstring, int from, int to);

extern int mfont__ft_drive_gpos (MGlyphString *gstring, int from, int to);

extern int mfont__ft_decode_otf (MGlyph *g);

#endif	/* HAVE_OTF */

#endif /* HAVE_FREETYPE */

extern void mfont__free_realized (MRealizedFont *rfont);

extern int mfont__match_p (MFont *font, MFont *spec, int prop);

extern int mfont__score (MFont *font, MFont *spec, MFont *request,
			 int limitted_size);

extern void mfont__set_spec_from_face (MFont *spec, MFace *face);

extern MSymbol mfont__set_spec_from_plist (MFont *spec, MPlist *plist);

extern void mfont__resize (MFont *spec, MFont *request);

extern unsigned mfont__encode_char (MRealizedFont *rfont, int c);

extern MRealizedFont *mfont__select (MFrame *frame, MFont *spec,
				     MFont *request, int limitted_size,
				     MSymbol layouter);

extern int mfont__open (MRealizedFont *rfont);

extern void mfont__get_metric (MGlyphString *gstring, int from, int to);

extern void mfont__set_property (MFont *font, enum MFontProperty key,
				 MSymbol val);

extern int mfont__split_name (char *name, int *property_idx,
			      unsigned short *point, unsigned short *resy);

extern void mfont__set_spec (MFont *font,
			     MSymbol attrs[MFONT_PROPERTY_MAX],
			     unsigned short size, unsigned short resy);

extern int mfont__parse_name_into_font (char *name, MSymbol format,
					MFont *font);

extern unsigned mfont__flt_encode_char (MSymbol layouter_name, int c);

extern int mfont__flt_run (MGlyphString *gstring, int from, int to,
			   MRealizedFace *rface);

#endif /* _M17N_FONT_H_ */
