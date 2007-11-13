/* m17n-flt.h -- header file for the FLT API of the m17n library.
   Copyright (C) 2007
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

#ifndef _M17N_FLT_H_
#define _M17N_FLT_H_

#ifndef _M17N_CORE_H_
#include <m17n-core.h>
#endif

M17N_BEGIN_HEADER

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)

extern void m17n_init_flt (void);
#undef M17N_INIT
#define M17N_INIT() m17n_init_flt ()

extern void m17n_fini_flt (void);
#undef M17N_FINI
#define M17N_FINI() m17n_fini_flt ()

#endif

/***en @defgroup m17nFLT FLT API */
/***ja @defgroup m17nFLT FLT API */
/*=*/

/*** @addtogroup m17nFLT */
/*** @{ */
/*=*/

/***en
    @brief Type of information about a glyph.

    The type #MFLTGlyph is the structure that contains information
    about a glyph.  */

typedef struct _MFLTGlyph MFLTGlyph;

struct _MFLTGlyph
{
  /***en @brief Character code (Unicode) of the glyph.  This is the sole
      member to be set before calling the functions mflt_find () and
      mflt_run ().  */
  int c;
  /***en Glyph-id of the font of the glyph.  */
  unsigned int code;
  /***en Glyph indices indicating the start of the original glyphs.  */
  int from;
  /***en Glyph indices indicating the end of the original glyphs.  */
  int to;
  /***en Advance width for horizontal layout expressed in 26.6
      fractional pixel format.  */
  int xadv;
  /***en Advance height for vertical layout expressed in 26.6
      fractional pixel format.  */
  int yadv;
  /***en Ink metrics of the glyph expressed in 26.6 fractional pixel
      format.  */
  int ascent, descent, lbearing, rbearing;
  /***en Horizontal and vertical adjustments for the glyph positioning
      expressed in 26.6 fractional pixel format.  */
  int xoff, yoff;
  /***en Flag to tell if the member <code> is already encoded into a
      glyph ID of a font.  */
  unsigned encoded : 1;
  /***en Flag to tell if the metrics of the glyph (members <xadv> thru
      <rbearing>) are already calculated.  */
  unsigned measured : 1;
  /***en For m17n-lib's internal use only.  */
  unsigned internal : 30;

  /* Arbitrary data can follow.  */
};

/*=*/

/***en
    @brief Type of information about a glyph position adjustment.

    The type #MFLTGlyphAdjustment is the structure to store
    information about a glyph metrics/position adjustment.  It is
    given to the callback function #drive_otf of #MFLTFont.  */

typedef struct _MFLTGlyphAdjustment MFLTGlyphAdjustment;

struct _MFLTGlyphAdjustment
{
  /***en Adjustments for advance width for horizontal layout and
      advance height for vertical layout expressed in 26.6 fractional
      pixel format.  */
  int xadv, yadv;
  /***en Horizontal and vertical adjustments for a glyph positioning
      expressed in 26.6 fractional pixel format.  */
  int xoff, yoff;
  /***en Number of glyphs to go back for drawing a glyph.  */
  short back;
  /***en If nonzero, the member <xadv> and <yadv> are absolute, i.e.,
      they should not be added to a glyph's origianl advance width and
      height.  */
  unsigned advance_is_absolute : 1;
  /***en Should be set to 1 iff at least one of the other members has
      a nonzero value.  */
  unsigned set : 1;
};

/***en
    @brief Type of information about a glyph sequence.

    The type #MFLTGlyphString is the structure that contains
    information about a sequence of glyphs.  */

typedef struct _MFLTGlyphString MFLTGlyphString;

struct _MFLTGlyphString
{
  /***en The actual byte size of elements of the array pointed by the
      member #glyphs.  It must be equal to or greater than "sizeof
      (MFLTGlyph)".  */
  int glyph_size;
  /***en Array of glyphs.  */
  MFLTGlyph *glyphs;
  /***en How many elements are allocated in #glyphs.  */
  int allocated;
  /***en How many elements in #glyphs are in use.  */
  int used;
  /***en Flag to tell if the glyphs should be drawn from right-to-left
      or not.  */
  unsigned int r2l;
};

/***en
    @brief Type of specification of GSUB and GPOS OpenType tables.

    The type #MFLTOtfSpec is the structure that contains information
    about GSUB and GPOS features of a specific script and language
    system to be applied to a glyph sequence.  */

typedef struct _MFLTOtfSpec MFLTOtfSpec;

struct _MFLTOtfSpec
{
  /***en Unique symbol representing the spec.  This is the same as the
      #OTF-SPEC of the FLT.  */
  MSymbol sym;

  /***en Tags for script and language system.  */
  unsigned int script, langsys;

  /***en Array of GSUB (1st element) and GPOS (2nd element) features.
      Each array is terminated by 0.  If an element is 0xFFFFFFFF,
      apply the previous features in that order, and apply all the
      other features except those appearing in the following elements.
      It may be NULL if there's no feature.  */
  unsigned int *features[2];
};

/***en
    @brief Type of font to be used by the FLT driver.

    The type #MFLTFont is the structure that contains information
    about a font used by the FLT driver.  */

typedef struct _MFLTFont MFLTFont;

struct _MFLTFont
{
  /***en Family name of the font.  It may be #Mnil if the family name
     is not important in finding a Font Layout Table suitable for the
     font (for instance, in the case that the font is an OpenType
     font).  */
  MSymbol family;

  /***en Horizontal and vertical font sizes in pixel per EM.  */
  int x_ppem, y_ppem;

  /***en Callback function to get glyph IDs for glyphs between FROM
     (inclusive) and TO (exclusive) of GSTRING.  If <encoded> member
     of a glyph is zero, the <code> member of the glyph is a character
     code.  The function must convert it to the glyph ID of FONT.  */
  int (*get_glyph_id) (MFLTFont *font, MFLTGlyphString *gstring,
		       int from, int to);

  /*** Callback function to get metrics of glyphs between FROM
     (inclusive) and TO (exclusive) of GSTRING.  If <measured> member
     of a glyph is zero, the function must set members <xadv>, <yadv>,
     <ascent>, <descent>, <lbearing>, and <rbearing> of the glyph.  */
  int (*get_metrics) (MFLTFont *font, MFLTGlyphString *gstring,
		     int from, int to);

  /***en Callback function to check if the font has OpenType GSUB/GPOS
     features for a specific script/language.  The function must
     return 1 if the font satisfy SPEC, else return 0.  It must be
     NULL if the font doesn't have OpenType tables.  */
  int (*check_otf) (MFLTFont *font, MFLTOtfSpec *spec);

  /*** Callback function to apply OpenType features in SPEC to glyphs
     between FROM (inclusive) and TO (exclusive) of IN.  The resulting
     glyphs should be appended to the tail of OUT.  If OUT doesn't
     have a room to store all resulting glyphs, it must return -2.
     It must be NULL if the font doesn't have OpenType tables.  */
  int (*drive_otf) (MFLTFont *font, MFLTOtfSpec *spec,
		    MFLTGlyphString *in, int from, int to,
		    MFLTGlyphString *out, MFLTGlyphAdjustment *adjustment);

  /***en For m17n-lib's internal use only.  It should be initialized
      to NULL.  */
  void *internal;
};

/***en
    @brief Type of FLT (Font Layout Table).

    The type #MFLT is for a FLT object.  Its internal structure is
    concealed from application program.  */

typedef struct _MFLT MFLT;

extern MFLT *mflt_get (MSymbol name);

extern MFLT *mflt_find (int c, MFLTFont *font);

extern const char *mflt_name (MFLT *flt);

extern MCharTable *mflt_coverage (MFLT *flt);

extern int mflt_run (MFLTGlyphString *gstring, int from, int to,
		     MFLTFont *font, MFLT *flt);

/*=*/
/*** @} */

M17N_END_HEADER

#endif /* _M17N_FLT_H_ */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
