/* face.h -- header file for the face module.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
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

#ifndef _M17N_FACE_H_
#define _M17N_FACE_H_

enum MFaceProperty
  {
    /** The font related properties.  */
    /* The order of MFACE_FOUNDRY to MFACE_ADSTYLE must be the same as
       MFONT_FOUNDRY to MFONT_ADSTYLE of enum MFontProperty.  */
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

    /* In a realized face, this is ignored because it is already
       reflected in MFACE_SIZE.  */
    MFACE_RATIO,

    MFACE_HOOK_ARG,

    MFACE_PROPERTY_MAX
  };

struct MFace
{
  M17NObject control;

  /** Properties of the face.  */
  void *property[MFACE_PROPERTY_MAX];

  MFaceHookFunc hook;

  /** List of frames affected by the face modification.  */
  MPlist *frame_list;
};


/** A realized face is registered in MFrame->face_list, thus it does
    not have to be a managed object.  */

struct MRealizedFace
{
  /** Frame on which this realized face is created.  */
  MFrame *frame;

  /** Properties of all stacked faces are merged into here.  */
  MFace face;

  /** Font explicitly specified for the face (maybe NULL).  */
  MFont *font;

  /** From what faces this is realized.  Keys are Mface and values are
     (MFace *).  */
  MPlist *base_face_list;

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

  /** List of realized faces that have the same face properties.  */
  MPlist *non_ascii_list;

  int ascent, descent;
  int space_width;
  int average_width;

  /** Pointer to a window system dependent object.  */
  void *info;
};


extern MFace *mface__default;

extern MRealizedFace *mface__realize (MFrame *frame, MFace **faces, int num,
				      int limitted_size, MFont *font);

extern MGlyph *mface__for_chars (MSymbol script, MSymbol language,
				 MSymbol charset, MGlyph *from_g, MGlyph *to_g,
				 int size);

extern void mface__free_realized (MRealizedFace *rface);

extern void mface__update_frame_face (MFrame *frame);

#endif /* _M17N_FACE_H_ */
