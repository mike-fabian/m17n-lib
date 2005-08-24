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
    MFONT_RESY,
    /* The follwoings should not be used as an index to
       MFont->propperty.  */
    MFONT_SIZE,
    MFONT_SPACING,
    /* anchor */
    MFONT_PROPERTY_MAX = MFONT_SIZE
  };

enum MFontType
  {
    MFONT_TYPE_SPEC,
    MFONT_TYPE_OBJECT,
    MFONT_TYPE_REALIZED,
    MFONT_TYPE_FAILURE
  };

enum MFontSource
  {
    MFONT_SOURCE_UNDECIDED = 0,
    MFONT_SOURCE_X = 1,
    MFONT_SOURCE_FT = 2
  };

enum MFontSpacing
  {
    MFONT_SPACING_UNDECIDED,
    MFONT_SPACING_PROPORTIONAL,
    MFONT_SPACING_MONO,
    MFONT_SPACING_CHARCELL
  };

typedef struct MFontEncoding MFontEncoding;
typedef struct MFontDriver MFontDriver;

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

      For FONT-OBJ: If the value is 0, the font is scalable or
      auto-scaled.

      For FONT-OPENED: The actual size of opened font.

      <property>[MFONT_RESY] is the designed resolution of the font in
      DPI, or zero.  Zero means that the font is scalable.

      For the time being, we mention only Y-resolution (resy) and
      assume that resx is always equal to resy.  */
  unsigned short property[MFONT_PROPERTY_MAX];
  enum MFontType type : 2;
  enum MFontSource source : 2;
  enum MFontSpacing spacing : 2;
  /* Pixel size of the font in 1/10 pixels.  The negative value means
     a point size in 1/10 pt.  */
  int size : 26;
  MSymbol file;
  MSymbol capability;
  MFontEncoding *encoding;
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
  (mfont__property_table[(n)].names[(font)->property[(n)]])

struct MRealizedFont
{
  /* Font spec used to find the font.
     MRealizedFont::spec.property[MFONT_TYPE] is MFONT_TYPE_REALIZED
     so that this object can be distingushed from MFont.  */
  MFont spec;

  /* Frame on which the font is realized.  */
  MFrame *frame;

  /* The found font.  */
  MFont *font;

  MFontDriver *driver;

  /* Font Layout Table for the font.  This is just a container to
     carry the infomation.  */
  MSymbol layouter;

  /* Extra information set by MRealizedFont::driver->open.  If
     non-NULL, it must be a pointer to a managed object.  */
  void *info;

  int ascent, descent, max_advance;

  /* Pointer to the font structure.  */
  void *fontp;

  MRealizedFont *next;
};

typedef struct {
  MFont *font;
  int score;
} MFontScore;

/** Structure to hold a list of fonts.  */

typedef struct
{
  MFont object;
  MFontScore *fonts;
  int nfonts;
} MFontList;

struct MFontDriver
{
  /** Return a font best matching with FONT.  */
  MFont *(*select) (MFrame *frame, MFont *font, int limited_size);

  /** Open a font specified by RFONT.  */
  MRealizedFont *(*open) (MFrame *frame, MFont *font, MFont *spec,
			  MRealizedFont *rfont);

  /** Set metrics of glyphs in GSTRING from FROM to TO.  */
  void (*find_metric) (MRealizedFont *rfont, MGlyphString *gstring,
		       int from, int to);

  /** Check if the font has a glyph for CODE.  CODE is a code point of
      a character in font->encoder->encoding_charset.  Return nonzero
      iff the font has the glyph.  */
  int (*has_char) (MFrame *frame, MFont *font, MFont *spec,
		   int c, unsigned code);

  /** Encode CODE into a glyph code the font.  CODE is a code point of
      a character in rfont->encoder->encoding_charset.  If the font
      has no glyph for CODE, return MCHAR_INVALID_CODE.  */
  unsigned (*encode_char) (MFrame *frame, MFont *font, MFont *spec,
			   unsigned code);

  /** Draw glyphs from FROM to TO (exclusive) on window WIN of FRAME
      at coordinate (X, Y) relative to WIN.  */
  void (*render) (MDrawWindow win, int x, int y,
		  MGlyphString *gstring, MGlyph *from, MGlyph *to,
		  int reverse, MDrawRegion region);

  /** Push to PLIST fonts matching with FONT.  MAXNUM if greater than
      0 limits the number of listed fonts.  Return the number of fonts
      listed.  */
  int (*list) (MFrame *frame, MPlist *plist, MFont *font, int maxnum);
};

/** Initialize the members of FONT.  */

#define MFONT_INIT(font) memset ((font), 0, sizeof (MFont))

extern MSymbol Mlayouter;
extern MSymbol Miso8859_1, Miso10646_1, Municode_bmp, Municode_full;
extern MSymbol Mapple_roman;

extern int mfont__flt_init ();

extern void mfont__flt_fini ();

#ifdef HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#ifdef HAVE_OTF
#include <otf.h>
#else  /* not HAVE_OTF */
typedef unsigned OTF_Tag;
#endif /* not HAVE_OTF */

enum MFontOpenTypeTable
  {
    MFONT_OTT_GSUB,
    MFONT_OTT_GPOS,
    MFONT_OTT_MAX
  };

typedef struct
{
  M17NObject control;
  MSymbol *lang;
  MSymbol script;
  OTF_Tag script_tag;
#ifdef HAVE_OTF
  OTF_Tag langsys_tag;
  struct {
    char *str;
    int nfeatures;
    OTF_Tag *tags;
  } features[MFONT_OTT_MAX];
#endif
} MFontCapability;

extern MFontDriver mfont__ft_driver;

extern int mfont__ft_init ();

extern void mfont__ft_fini ();

extern int mfont__ft_parse_name (const char *name, MFont *font);

extern char *mfont__ft_unparse_name (MFont *font);

#ifdef HAVE_OTF

extern int mfont__ft_drive_otf (MGlyphString *gstring, int from, int to,
				MFontCapability *capability);

extern int mfont__ft_decode_otf (MGlyph *g);

#endif	/* HAVE_OTF */

#endif /* HAVE_FREETYPE */

extern void mfont__free_realized (MRealizedFont *rfont);

extern int mfont__match_p (MFont *font, MFont *spec, int prop);

extern int mfont__merge (MFont *dst, MFont *src, int error_on_conflict);

extern void mfont__set_spec_from_face (MFont *spec, MFace *face);

extern MSymbol mfont__set_spec_from_plist (MFont *spec, MPlist *plist);

extern void mfont__resize (MFont *spec, MFont *request);

extern int mfont__has_char (MFrame *frame, MFont *font, MFont *spec, int c);

extern unsigned mfont__encode_char (MFrame *frame, MFont *font, MFont *spec,
				    int c);

extern MFont *mfont__select (MFrame *frame, MFont *font, int max_size);

extern MFontList *mfont__list (MFrame *frame, MFont *spec, MFont *request,
			       int limited_size);

extern MRealizedFont *mfont__open (MFrame *frame, MFont *font, MFont *spec);

extern void mfont__get_metric (MGlyphString *gstring, int from, int to);

extern void mfont__set_property (MFont *font, enum MFontProperty key,
				 MSymbol val);

extern int mfont__split_name (char *name, int *property_idx,
			      unsigned short *point, unsigned short *resy);

extern int mfont__parse_name_into_font (const char *name, MSymbol format,
					MFont *font);

extern MPlist *mfont__encoding_list (void);

extern MFontCapability *mfont__get_capability (MSymbol sym);

extern unsigned mfont__flt_encode_char (MSymbol layouter_name, int c);

extern int mfont__flt_run (MGlyphString *gstring, int from, int to,
			   MRealizedFace *rface);

#endif /* _M17N_FONT_H_ */
