/* face.h -- header file for the face module.
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

#ifndef _M17N_FACE_H_
#define _M17N_FACE_H_

enum MFaceProperty
  {
    /** The font related properties.  */
    /* The order of MFACE_FOUNDRY to MFACE_ADSTYLE must be the same as
       enum MFontProperty.  */
    MFACE_FOUNDRY,
    MFACE_FAMILY,
    MFACE_WEIGHT,
    MFACE_STYLE,
    MFACE_STRETCH,
    MFACE_ADSTYLE,
    MFACE_SIZE,
    MFACE_FONTSET,

    /** The color related properties.   */
    MFACE_FOREGROUND,
    MFACE_BACKGROUND,

    /** The other properties.   */
    MFACE_HLINE,
    MFACE_BOX,
    MFACE_VIDEOMODE,

    /** Extention by applications.  */
    MFACE_HOOK_FUNC,
    MFACE_HOOK_ARG,

    /* In a realized face, this is already reflected in MFACE_SIZE,
       thus is ignored.  */
    MFACE_RATIO,

    MFACE_PROPERTY_MAX
  };

struct MFace
{
  M17NObject control;
  /* Initialized to 0, and incremented by one each the face is
     modified.  */
  unsigned tick;
  void *property[MFACE_PROPERTY_MAX];
};


/** A realized face is registered in MFrame->face_list, thus it does
    not have to be a managed object.  */

struct MRealizedFace
{
  /** Frame on which this realized face is created.  */
  MFrame *frame;

  /** Properties of all stacked faces are merged into here.  */
  MFace face;

  /** From what faces this is realized.  Keys are Mface and values are
     (MFace *).  */
  MPlist *base_face_list;

  /* Initialized to the sum of ticks of the above faces.  */
  unsigned tick;

  /* Realized font, one of <frame>->realized_font_list.  */
  MRealizedFont *rfont;

  /* Realized fontset, one of <frame>->realized_fontset_list.  */
  MRealizedFontset *rfontset;

  MSymbol layouter;

  MFaceHLineProp *hline;

  MFaceBoxProp *box;

  /** Realized face for ASCII chars that has the same face
      properties. */
  MRealizedFace *ascii_rface;

  /** Realized face for undisplayable chars (no font found) that has
      the same face properties. */
  MRealizedFace *nofont_rface;

  int ascent, descent;
  int space_width;

  /** Pointer to a window system dependent object.  */
  void *info;
};


extern MFace *mface__default;

extern MRealizedFace *mface__realize (MFrame *frame, MFace **faces, int num,
				      MSymbol language, MSymbol charset,
				      int limitted_size);

extern MGlyph *mface__for_chars (MSymbol script, MSymbol language,
				 MSymbol charset, MGlyph *from_g, MGlyph *to_g,
				 int size);

extern void mface__free_realized (MRealizedFace *rface);

#endif /* _M17N_FACE_H_ */
