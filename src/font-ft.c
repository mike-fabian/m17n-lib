/* font-ft.c -- FreeType interface sub-module.
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <ctype.h>

#include "m17n-gui.h"
#include "m17n-misc.h"
#include "internal.h"
#include "plist.h"
#include "symbol.h"
#include "language.h"
#include "internal-flt.h"
#include "internal-gui.h"
#include "font.h"
#include "face.h"

#ifdef HAVE_FREETYPE

#ifdef HAVE_FTBDF_H
#include FT_BDF_H
#endif

static int mdebug_flag = MDEBUG_FONT;

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fcfreetype.h>

static FcConfig *fc_config;
static MSymbol Mgeneric_family;
#endif	/* HAVE_FONTCONFIG */

/* Font properties; Mnormal is already defined in face.c.  */
static MSymbol Mmedium, Mr, Mnull;

static MSymbol M0[5], M3_1, M1_0;

static FT_Library ft_library;

#ifdef HAVE_OTF
static OTF *invalid_otf = (OTF *) "";
static OTF *get_otf (MFLTFont *font, FT_Face *ft_face);
#endif /* HAVE_OTF */

typedef struct
{
  MFont font;
#ifdef HAVE_OTF
  /* NULL if not yet opened.  invalid_otf if not OTF.  */
  OTF *otf;
#endif /* HAVE_OTF */
#ifdef HAVE_FONTCONFIG
  FcLangSet *langset;
  FcCharSet *charset;
#endif	/* HAVE_FONTCONFIG */
} MFontFT;

typedef struct
{
  M17NObject control;
  FT_Face ft_face;		/* This must be the 2nd member. */
  MPlist *charmap_list;
  int face_encapsulated;
} MRealizedFontFT;

typedef struct
{
  char *ft_style;
  int len;
  enum MFontProperty prop;
  char *val;
} MFTtoProp;

static MFTtoProp ft_to_prop[] =
  { { "italic", 0, MFONT_STYLE, "i" },
    { "roman", 0, MFONT_STYLE, "r" },
    { "oblique", 0, MFONT_STYLE, "o" },
    { "regular", 0, MFONT_WEIGHT, "normal" },
    { "normal", 0, MFONT_WEIGHT, "normal" },
    /* We need this entry even if "bold" is in commone_weight[] to
       handle such style names as "bolditalic" and "boldoblique".  */
    { "bold", 0, MFONT_WEIGHT, "bold" },
    { "demi bold", 0, MFONT_WEIGHT, "demibold" },
    { "demi", 0, MFONT_WEIGHT, "demibold" } };
static int ft_to_prop_size = sizeof ft_to_prop / sizeof ft_to_prop[0];

/** List of FreeType fonts.  Keys are family names, values are plists
    containing fonts of the corresponding family.  In the deeper
    plist, keys are file names, values are (MFontFT *).  */
static MPlist *ft_font_list;

/** List of FreeType fonts.  Keys are script names, values are plists
    containing fonts supporting the corresponding script.  In the
    deeper plist, keys are family names, values are (MFontFT *).  */
static MPlist *ft_script_list;

/** List of FreeType fonts.  Keys are language names, values are
    plists containing fonts supporting the corresponding language.  In
    the deeper plist, keys are family names, values are (MFontFT *).  */
static MPlist *ft_language_list;

static MPlist *ft_file_list;

static int all_fonts_scaned;

#define STRDUP_LOWER(s1, size, s2)				\
  do {								\
    int len = strlen (s2) + 1;					\
    char *p1, *p2;						\
								\
    if ((size) < len)						\
      (s1) = alloca (len), (size) = len;			\
    for (p1 = (s1), p2 = (s2); *p2; p1++, p2++)			\
      *p1 = (*p2 >= 'A' && *p2 <= 'Z' ? *p2 + 'a' - 'A' : *p2);	\
    *p1 = '\0';							\
  } while (0)


static MPlist *ft_list_family (MSymbol, int, int);

static void
free_ft_rfont (void *object)
{
  MRealizedFontFT *ft_rfont = object;

  if (! ft_rfont->face_encapsulated)
    {
      M17N_OBJECT_UNREF (ft_rfont->charmap_list);
      FT_Done_Face (ft_rfont->ft_face);
    }
  free (ft_rfont);
}

static void
free_ft_info (MFontFT *ft_info)
{
#ifdef HAVE_OTF
  if (ft_info->otf && ft_info->otf != invalid_otf)
    OTF_close (ft_info->otf);
#endif /* HAVE_OTF */
#ifdef HAVE_FONTCONFIG
  if (ft_info->langset)
    FcLangSetDestroy (ft_info->langset);
  if (ft_info->charset)
    FcCharSetDestroy (ft_info->charset);
#endif	/* HAVE_FONTCONFIG */
  free (ft_info);
}

static MPlist *
ft_get_charmaps (FT_Face ft_face)
{
  MPlist *plist = mplist ();
  int unicode_bmp = -1, unicode_full = -1;
  int i;

  mplist_add (plist, Mt, (void *) -1);
  for (i = 0; i < ft_face->num_charmaps; i++)
    {
      MSymbol registry = Mnil;

      if (ft_face->charmaps[i]->platform_id == 0)
	{
	  if (ft_face->charmaps[i]->encoding_id <= 4)
	    registry = M0[ft_face->charmaps[i]->encoding_id], unicode_bmp = i;
	  if (ft_face->charmaps[i]->encoding_id == 4)
	    unicode_bmp = unicode_full = i;
	}
      else if (ft_face->charmaps[i]->platform_id == 3)
	{
	  if (ft_face->charmaps[i]->encoding_id == 1)
	    registry = M3_1, unicode_bmp = i;
	  else if (ft_face->charmaps[i]->encoding_id == 10)
	    unicode_bmp = unicode_full = i;
	}
      else if (ft_face->charmaps[i]->platform_id == 1
	       && ft_face->charmaps[i]->encoding_id == 0)
	{
	  registry = M1_0;
	  mplist_add (plist, Mapple_roman, (void *) i);
	}
      if (registry == Mnil)
	{
	  char registry_buf[16];

	  sprintf (registry_buf, "%d-%d",
		   ft_face->charmaps[i]->platform_id,
		   ft_face->charmaps[i]->encoding_id);
	  registry = msymbol (registry_buf);
	}
      mplist_add (plist, registry, (void *) i);
    }
  if (unicode_full >= 0)
    mplist_add (plist, Municode_full, (void *) unicode_full);
  if (unicode_bmp >= 0)
    {
      int i;

      mplist_add (plist, Municode_bmp, (void *) unicode_bmp);
      FT_Set_Charmap (ft_face, ft_face->charmaps[unicode_bmp]);
      for (i = 0x21; i < 0x7F && FT_Get_Char_Index (ft_face, i) > 0; i++);
      if (i == 0x7F)
	{
	  for (i = 0xC0; i < 0x100 && FT_Get_Char_Index (ft_face, i) > 0; i++);
	  if (i == 0x100)
	    mplist_add (plist, Miso8859_1, (void *) unicode_bmp);
	}
    }

  return plist;
}

static MFontFT *
ft_gen_font (FT_Face ft_face)
{
  MFontFT *ft_info;
  MFont *font;
  char *buf;
  int bufsize = 0;
  char *stylename;
  MSymbol family;
  int size;

  if (FT_IS_SCALABLE (ft_face))
    size = ft_face->size->metrics.y_ppem;
  else if (ft_face->num_fixed_sizes == 0)
    return NULL;
  else
    size = ft_face->available_sizes[0].height;

  MSTRUCT_CALLOC (ft_info, MERROR_FONT_FT);
  font = &ft_info->font;
  STRDUP_LOWER (buf, bufsize, ft_face->family_name);
  family = msymbol (buf);
  mfont__set_property (font, MFONT_FAMILY, family);
  mfont__set_property (font, MFONT_WEIGHT, Mmedium);
  mfont__set_property (font, MFONT_STYLE, Mr);
  mfont__set_property (font, MFONT_STRETCH, Mnormal);
  mfont__set_property (font, MFONT_ADSTYLE, Mnull);
  mfont__set_property (font, MFONT_REGISTRY, Municode_bmp);
  font->size = size * 10;
  font->type = MFONT_TYPE_OBJECT;
  font->source = MFONT_SOURCE_FT;
  font->file = NULL;

  stylename = ft_face->style_name;
  while (*stylename)
    {
      int i;

      for (i = 0; i < ft_to_prop_size; i++)
	if (! strncasecmp (ft_to_prop[i].ft_style, stylename,
			   ft_to_prop[i].len))
	  {
	    mfont__set_property (font, ft_to_prop[i].prop,
				 msymbol (ft_to_prop[i].val));
	    stylename += ft_to_prop[i].len;
	    break;
	  }
      if (i == ft_to_prop_size)
	{
	  char *p1 = stylename + 1;
	  MSymbol sym;

	  while (*p1 >= 'a' && *p1 <= 'z') p1++;
	  sym = msymbol__with_len (stylename, p1 - stylename);
	  for (i = MFONT_WEIGHT; i <= MFONT_STRETCH; i++)
	    if (msymbol_get (sym, mfont__property_table[i].property))
	      {
		mfont__set_property (font, i, sym);
		break;
	      }
	  stylename = p1;
	}
      while (*stylename && ! isalpha (*stylename))
	stylename++;
    }
  return ft_info;
}

#ifdef HAVE_FONTCONFIG

typedef struct
{
  int fc_value;
  char *m17n_value;
  MSymbol sym;
} FC_vs_M17N_font_prop;

static FC_vs_M17N_font_prop fc_weight_table[] =
  { { FC_WEIGHT_THIN, "thin" },
    { FC_WEIGHT_ULTRALIGHT, "extralight" },
    { FC_WEIGHT_LIGHT, "light" },
#ifdef FC_WEIGHT_BOOK
    { FC_WEIGHT_BOOK, "book" },
#endif	/* FC_WEIGHT_BOOK */
    { FC_WEIGHT_REGULAR, "normal" },
    { FC_WEIGHT_NORMAL, "normal" },
    { FC_WEIGHT_MEDIUM, "medium" },
    { FC_WEIGHT_DEMIBOLD, "demibold" },
    { FC_WEIGHT_BOLD, "bold" },
    { FC_WEIGHT_EXTRABOLD, "extrabold" },
    { FC_WEIGHT_BLACK, "black" },
    { FC_WEIGHT_HEAVY, "heavy" },
    { FC_WEIGHT_MEDIUM, NULL } };
int fc_weight_table_size =
  sizeof fc_weight_table / sizeof (FC_vs_M17N_font_prop);

static FC_vs_M17N_font_prop fc_slant_table[] =
  { { FC_SLANT_ROMAN, "r" },
    { FC_SLANT_ITALIC, "i" },
    { FC_SLANT_OBLIQUE, "o" },
    { FC_SLANT_ROMAN, NULL } };
int fc_slant_table_size =
  sizeof fc_slant_table / sizeof (FC_vs_M17N_font_prop);

static FC_vs_M17N_font_prop fc_width_table[] =
  { { FC_WIDTH_ULTRACONDENSED, "ultracondensed" },
    { FC_WIDTH_EXTRACONDENSED, "extracondensed" },
    { FC_WIDTH_CONDENSED, "condensed" },
    { FC_WIDTH_SEMICONDENSED, "semicondensed" },
    { FC_WIDTH_NORMAL, "normal" },
    { FC_WIDTH_SEMIEXPANDED, "semiexpanded" },
    { FC_WIDTH_EXPANDED, "expanded" },
    { FC_WIDTH_EXTRAEXPANDED, "extraexpanded" },
    { FC_WIDTH_ULTRAEXPANDED, "ultraexpanded" },
    { FC_WIDTH_NORMAL, NULL } };
int fc_width_table_size =
  sizeof fc_width_table / sizeof (FC_vs_M17N_font_prop);


static FC_vs_M17N_font_prop *fc_all_table[] =
  { fc_weight_table, fc_slant_table, fc_width_table };

static MSymbol
fc_decode_prop (int val, FC_vs_M17N_font_prop *table, int size)
{
  int i = size / 2;

  if (val < table[i].fc_value)
    {
      for (i--; i >= 0; i--)
	if (val > table[i].fc_value)
	  break;
      i++;
    }
  else if (val > table[i].fc_value)
    {
      for (i++; i < size; i++)
	if (val < table[i].fc_value)
	  break;
      i--;
    }
  return table[i].sym;
}

static int
fc_encode_prop (MSymbol sym, FC_vs_M17N_font_prop *table)
{
  int i;

  for (i = 0; table[i].m17n_value; i++)
    if (table[i].sym == sym)
      break;
  return table[i].fc_value;
}

FcPattern *
fc_get_pattern (MFont *font)
{
  FcPattern *pat = FcPatternCreate ();
  MSymbol sym, weight, style, stretch;


  if ((sym = (MSymbol) FONT_PROPERTY (font, MFONT_FOUNDRY)) != Mnil)
    FcPatternAddString (pat, FC_FOUNDRY, (FcChar8 *) MSYMBOL_NAME (sym));
  if ((sym = (MSymbol) FONT_PROPERTY (font, MFONT_FAMILY)) != Mnil)
    FcPatternAddString (pat, FC_FAMILY, (FcChar8 *) MSYMBOL_NAME (sym));
  if ((weight = (MSymbol) FONT_PROPERTY (font, MFONT_WEIGHT)) != Mnil)
    FcPatternAddInteger (pat, FC_WEIGHT,
			 fc_encode_prop (weight, fc_weight_table));
  if ((style = (MSymbol) FONT_PROPERTY (font, MFONT_STYLE)) != Mnil)
    FcPatternAddInteger (pat, FC_SLANT,
			 fc_encode_prop (style, fc_slant_table));
  if ((stretch = (MSymbol) FONT_PROPERTY (font, MFONT_STRETCH)) != Mnil)
    FcPatternAddInteger (pat, FC_WIDTH,
			 fc_encode_prop (stretch, fc_width_table));
  if (font->size > 0)
    {
      double size = font->size;
      FcPatternAddDouble (pat, FC_PIXEL_SIZE, size / 10);
    }
  else if (font->size < 0)
    {
      double size = - font->size;
      FcPatternAddDouble (pat, FC_SIZE, size / 10);
    }
  return pat;
}

static void
fc_parse_pattern (FcPattern *pat, char *family, MFontFT *ft_info)
{
  FcChar8 *str;
  int val;
  double size;
  char *buf;
  int bufsize = 0;
  MSymbol sym;
  FcLangSet *ls;
  FcCharSet *cs;
  MFont *font = &ft_info->font;

  MFONT_INIT (font);
  if (FcPatternGetString (pat, FC_FOUNDRY, 0, &str) == FcResultMatch)
    {
      STRDUP_LOWER (buf, bufsize, (char *) str);
      mfont__set_property (font, MFONT_FOUNDRY, msymbol (buf));
    }
  if (family)
    mfont__set_property (font, MFONT_FAMILY, msymbol (family));
  else if (FcPatternGetString (pat, FC_FAMILY, 0, &str) == FcResultMatch)
    {
      STRDUP_LOWER (buf, bufsize, (char *) str);
      mfont__set_property (font, MFONT_FAMILY, msymbol (buf));
    }
  if (FcPatternGetInteger (pat, FC_WEIGHT, 0, &val) == FcResultMatch)
    {
      sym = fc_decode_prop (val, fc_weight_table, fc_weight_table_size);
      mfont__set_property (font, MFONT_WEIGHT, sym);
    }
  if (FcPatternGetInteger (pat, FC_SLANT, 0, &val) == FcResultMatch)
    {
      sym = fc_decode_prop (val, fc_slant_table, fc_slant_table_size);
      mfont__set_property (font, MFONT_STYLE, sym);
    }
  if (FcPatternGetInteger (pat, FC_WIDTH, 0, &val) == FcResultMatch)
    {
      sym = fc_decode_prop (val, fc_width_table, fc_width_table_size);
      mfont__set_property (font, MFONT_STRETCH, sym);
    }
  if (FcPatternGetLangSet (pat, FC_LANG, 0, &ls) == FcResultMatch)
    {
      if (FcLangSetHasLang (ls, (FcChar8 *) "ja") != FcLangDifferentLang
	  || FcLangSetHasLang (ls, (FcChar8 *) "zh") != FcLangDifferentLang
	  || FcLangSetHasLang (ls, (FcChar8 *) "ko") != FcLangDifferentLang)
	font->for_full_width = 1;
      ft_info->langset = FcLangSetCopy (ls);
    }
  if (FcPatternGetCharSet (pat, FC_CHARSET, 0, &cs) == FcResultMatch)
    ft_info->charset = FcCharSetCopy (cs);

  mfont__set_property (font, MFONT_REGISTRY, Municode_bmp);
  font->type = MFONT_TYPE_SPEC;
  font->source = MFONT_SOURCE_FT;
  if (FcPatternGetDouble (pat, FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
    font->size = size * 10;
  if (FcPatternGetString (pat, FC_FILE, 0, &str) == FcResultMatch)
    font->file = msymbol ((char *) str);
}


static MFontFT *
fc_gen_font (FcPattern *pat, char *family)
{
  MFontFT *ft_info;

  MSTRUCT_CALLOC (ft_info, MERROR_FONT_FT);
  fc_parse_pattern (pat, family, ft_info);
  ft_info->font.type = MFONT_TYPE_OBJECT;
  return ft_info;
}

static void
fc_init_font_list (void)
{
  FcPattern *pattern = FcPatternCreate ();
  FcObjectSet *os = FcObjectSetBuild (FC_FAMILY, NULL);
  FcFontSet *fs = FcFontList (fc_config, pattern, os);
  MPlist *plist = mplist ();
  char *buf;
  int bufsize = 0;
  int i;

  ft_font_list = plist;
  for (i = 0; i < fs->nfont; i++)
    {
      char *fam;

      if (FcPatternGetString (fs->fonts[i], FC_FAMILY, 0,
			      (FcChar8 **) &fam) != FcResultMatch)
	continue;
      STRDUP_LOWER (buf, bufsize, fam);
      plist = mplist_add (plist, msymbol (buf), NULL);
    }
  FcFontSetDestroy (fs);
  FcObjectSetDestroy (os);
  FcPatternDestroy (pattern);
}

static MPlist *
fc_list_pattern (FcPattern *pattern)
{
  FcObjectSet *os = NULL;
  FcFontSet *fs = NULL;
  MSymbol last_family = Mnil;
  MPlist *plist = NULL, *pl = NULL;
  char *buf;
  int bufsize = 0;
  int i;

  if (! (os = FcObjectSetBuild (FC_FAMILY, FC_FILE, NULL)))
    goto err;
  if (! (fs = FcFontList (fc_config, pattern, os)))
    goto err;

  for (i = 0; i < fs->nfont; i++)
    {
      MSymbol family, file;
      char *fam, *filename;
      MFontFT *ft_info;

      if (FcPatternGetString (fs->fonts[i], FC_FAMILY, 0,
			      (FcChar8 **) &fam) != FcResultMatch)
	continue;
      if (FcPatternGetString (fs->fonts[i], FC_FILE, 0,
			      (FcChar8 **) &filename) != FcResultMatch)
	continue;
      STRDUP_LOWER (buf, bufsize, fam);
      family = msymbol (buf);
      file = msymbol (filename);
      if (family != last_family)
	{
	  pl = MPLIST_PLIST (ft_list_family (family, 0, 1));
	  last_family = family;
	}
      ft_info = mplist_get (pl, file);
      if (ft_info)
	{
	  if (! plist)
	    plist = mplist ();
	  mplist_add (plist, family, ft_info);
	}
    }

 err:
  if (fs) FcFontSetDestroy (fs);
  if (os) FcObjectSetDestroy (os);
  return plist;
}

/* Return FcCharSet object built from CHAR_LIST or MT.  In the latter
   case, it is assured that the M-text contains at least one
   character.  */

static FcCharSet *
fc_build_charset (MPlist *char_list, MText *mt)
{
  FcCharSet *cs = FcCharSetCreate ();

  if (! cs)
    return NULL;
  if (char_list)
    {
      for (; ! MPLIST_TAIL_P (char_list); char_list = MPLIST_NEXT (char_list))
	if (! FcCharSetAddChar (cs, (FcChar32) MPLIST_INTEGER (char_list)))
	  {
	    FcCharSetDestroy (cs);
	    return NULL;
	  }
    }
  else
    {
      int i;

      for (i = mtext_nchars (mt) - 1; i >= 0; i--)
	if (! FcCharSetAddChar (cs, (FcChar32) mtext_ref_char (mt, i)))
	  {
	    FcCharSetDestroy (cs);
	    return NULL;
	  }
      if (mtext_nchars (mt) > 0
	  && (mt = mtext_get_prop (mt, 0, Mtext)))
	for (i = mtext_nchars (mt) - 1; i >= 0; i--)
	  if (! FcCharSetAddChar (cs, (FcChar32) mtext_ref_char (mt, i)))
	    {
	      FcCharSetDestroy (cs);
	      return NULL;
	    }
      }
  return cs;
}

#else	/* not HAVE_FONTCONFIG */

static MPlist *
ft_add_font (char *filename)
{
  FT_Face ft_face;
  char *stylename;
  int size = 0;
  MSymbol family;
  MFontFT *ft_info;
  MFont *font;
  MPlist *plist;
  int i;
  char *buf;
  int bufsize = 0;

  if (FT_New_Face (ft_library, filename, 0, &ft_face) != 0)
    return NULL;
  ft_info = ft_gen_font (ft_face);
  FT_Done_Face (ft_face);
  if (! ft_info)
    return NULL;

  font = &ft_info->font;
  font->file = msymbol (filename);

  plist = mplist_find_by_key (ft_font_list, family);
  if (plist)
    mplist_push (MPLIST_PLIST (plist), font->file, ft_info);
  else
    {
      plist = mplist ();
      mplist_add (plist, font->file, ft_info);
      plist = mplist_push (ft_font_list, family, plist);
    }
  return plist;
}

static void
ft_init_font_list (void)
{
  MPlist *plist;
  struct stat buf;
  char *pathname;
  char *path;
  USE_SAFE_ALLOCA;

  ft_font_list = mplist ();
  MPLIST_DO (plist, mfont_freetype_path)
    if (MPLIST_STRING_P (plist)
	&& (pathname = MPLIST_STRING (plist))
	&& stat (pathname, &buf) == 0)
      {
	if (S_ISREG (buf.st_mode))
	  ft_add_font (pathname);
	else if (S_ISDIR (buf.st_mode))
	  {
	    DIR *dir = opendir (pathname);

	    if (dir)
	      {
		int len = strlen (pathname);
		struct dirent *dp;

		while ((dp = readdir (dir)) != NULL)
		  {
		    SAFE_ALLOCA (path, len + strlen (dp->d_name) + 2);
		    strcpy (path, pathname);
		    path[len] =  '/';
		    strcpy (path + len + 1, dp->d_name);
		    ft_add_font (path);
		  }
		closedir (dir);
	      }
	  }
      }
  SAFE_FREE (path);
}

/* Return 1 if the font pointed by FT_INFO has all characters in
   CHAR_LIST.  */

static int
ft_has_char_list_p (MFontFT *ft_info, MPlist *char_list)
{
  FT_Face ft_face;
  MPlist *cl;

  if (FT_New_Face (ft_library, MSYMBOL_NAME (ft_info->font.file), 0, &ft_face))
    return 0;
  MPLIST_DO (cl, char_list)
    if (FT_Get_Char_Index (ft_face, (FT_ULong) MPLIST_INTEGER (cl)) == 0)
      break;
  FT_Done_Face (ft_face);
  return MPLIST_TAIL_P (cl);
}

/* Return ((FAMILY . FONT) ...) where FONT is a pointer to MFontFT
   that supports characters in CHAR_LIST or MT.  One of CHAR_LIST or
   MT must be NULL.  */

static MPlist *
ft_list_char_list (MPlist *char_list, MText *mt)
{
  MPlist *plist = NULL, *pl, *p;

  if (! ft_font_list)
    ft_list_family (Mnil, 0, 1);

  if (mt)
    {
      int len = mtext_nchars (mt);
      MText *extra = mtext_get_prop (mt, 0, Mtext);
      int total_len = len + (extra ? mtext_nchars (extra) : 0);
      int i;

      char_list = mplist ();
      for (i = 0; i < total_len; i++)
	{
	  int c = (i < len ? mtext_ref_char (mt, i)
		   : mtext_ref_char (extra, i - len));

	  if (! mplist_find_by_value (char_list, (void *) c))
	    mplist_push (char_list, Minteger, (void *) c);
	}
    }

  MPLIST_DO (pl, ft_font_list)
    {
      MPLIST_DO (p, MPLIST_PLIST (pl))
	{
	  MFontFT *ft_info = MPLIST_VAL (p);

	  if (ft_has_char_list_p (ft_info, char_list))
	    {
	      MSymbol family = mfont_get_prop (&ft_info->font, Mfamily);

	      if (! plist)
		plist = mplist ();
	      mplist_push (plist, family, ft_info);
	    }
	}
    }
  if (mt)
    M17N_OBJECT_UNREF (char_list);
  return plist;
}
#endif	/* not HAVE_FONTCONFIG */


/* Return an element of ft_font_list for FAMILY.  If FAMILY is Mnil,
   scan all fonts and return ft_font_list.  */

static MPlist *
ft_list_family (MSymbol family, int check_generic, int check_alias)
{
  MPlist *plist;
#ifdef HAVE_FONTCONFIG
  char *fam;
  MPlist *pl, *p;
  FcPattern *pattern;
  FcObjectSet *os;
  FcFontSet *fs;
  int i;
  char *buf;
  int bufsize = 0;
  MSymbol generic;

  if (! ft_font_list)
    {
      MSymbol sym;

      plist = ft_font_list = mplist ();
      pattern = FcPatternCreate ();
      os = FcObjectSetBuild (FC_FAMILY, NULL);
      fs = FcFontList (fc_config, pattern, os);
      for (i = 0; i < fs->nfont; i++)
	{
	  if (FcPatternGetString (fs->fonts[i], FC_FAMILY, 0,
				  (FcChar8 **) &fam) != FcResultMatch)
	    continue;
	  STRDUP_LOWER (buf, bufsize, fam);
	  sym = msymbol (buf);
	  if (! mplist_find_by_key (ft_font_list, sym))
	    plist = mplist_add (plist, sym, NULL);
	}
      FcFontSetDestroy (fs);
      FcObjectSetDestroy (os);
      FcPatternDestroy (pattern);
    }

  if (family == Mnil)
    {
      if (! all_fonts_scaned)
	{
	  MPLIST_DO (plist, ft_font_list)
	    {
	      if (! MPLIST_VAL (plist))
		ft_list_family (MPLIST_KEY (plist), 0, 1);
	    }
	  all_fonts_scaned = 1;
	}
      return ft_font_list;
    }

  plist = mplist_find_by_key (ft_font_list, family);
  if (plist)
    {
      if (! MPLIST_VAL (plist))
	{
	  fam = MSYMBOL_NAME (family);
	  pattern = FcPatternCreate ();
	  FcPatternAddString (pattern, FC_FAMILY, (FcChar8 *) fam);
	  os = FcObjectSetBuild (FC_FOUNDRY, FC_WEIGHT, FC_SLANT, FC_WIDTH,
				 FC_PIXEL_SIZE, FC_LANG, FC_CHARSET, FC_FILE,
				 NULL);
	  fs = FcFontList (fc_config, pattern, os);
	  p = pl = mplist ();
	  for (i = 0; i < fs->nfont; i++)
	    {
	      MFontFT *ft_info = fc_gen_font (fs->fonts[i], fam);
	      p = mplist_add (p, ft_info->font.file, ft_info);
	    }
	  MPLIST_VAL (plist) = pl;
	  FcFontSetDestroy (fs);
	  FcObjectSetDestroy (os);
	  FcPatternDestroy (pattern);
	}
    }
  else if (check_generic
	   && (generic = msymbol_get (family, Mgeneric_family)) != Mnil)
    {
      /* Check if FAMILY is a geneneric family (e.g. `serif').  */
      FcChar8 *fam8;
      
      if (family != generic)
	plist = ft_list_family (generic, 1, 1);
      else
	{
	  fam = MSYMBOL_NAME (family);
	  plist = mplist ();
	  mplist_push (ft_font_list, family, plist);
	  pattern = FcPatternBuild (NULL, FC_FAMILY, FcTypeString, fam, NULL);
	  FcConfigSubstitute (fc_config, pattern, FcMatchPattern);
	  for (i = 0; 1; i++)
	    {
	      if (FcPatternGetString (pattern, FC_FAMILY, i, &fam8)
		  != FcResultMatch)
		break;
	      STRDUP_LOWER (buf, bufsize, (char *) fam8);
	      family = msymbol (buf);
	      if (msymbol_get (family, Mgeneric_family))
		break;
	      pl = ft_list_family (family, 0, 1);
	      if (! pl)
		continue;
	      MPLIST_DO (pl, MPLIST_PLIST (pl))
		plist = mplist_add (plist, Mt, MPLIST_VAL (pl));
	    }
	  plist = ft_font_list;
	}
    }
  else if (check_alias)
    {
      /* Check if there exist an alias.  */
      pl = mplist ();
      plist = mplist_add (ft_font_list, family, pl);

      pattern = FcPatternBuild (NULL,
				FC_FAMILY, FcTypeString, MSYMBOL_NAME (family),
				NULL);
      FcConfigSubstitute (fc_config, pattern, FcMatchPattern);

      for (i = 0; FcPatternGetString (pattern, FC_FAMILY, i,
				      (FcChar8 **) &fam) == FcResultMatch;
	   i++);
      if (i > 0)
	{
	  /* The last one is a generic family.  */
	  MSymbol sym;
	  int j;
	  FcPattern *pat = FcPatternBuild (NULL, FC_FAMILY, FcTypeString, fam,
					   NULL);

	  FcConfigSubstitute (fc_config, pat, FcMatchPattern);
	  for (j = 0; FcPatternGetString (pat, FC_FAMILY, j,
					  (FcChar8 **) &fam) == FcResultMatch;
	       j++);

	  /* Now we know that the last J fonts in PATTERN are from
	     generic font, and the first one is not available.  So,
	     the remaining ones are aliases.  */
	  j = i - j;
	  for (i = 1; i < j; i++)
	    {
	      FcPatternGetString (pattern, FC_FAMILY, i, (FcChar8 **) &fam);
	      STRDUP_LOWER (buf, bufsize, fam);
	      sym = msymbol (buf);
	      p = MPLIST_PLIST (ft_list_family (sym, 0, 0));
	      if (! MPLIST_TAIL_P (p))
		MPLIST_DO (p, p)
		  mplist_push (pl, Mt, MPLIST_VAL (p));
	    }
	}
    }
  else
    {
      pl = mplist ();
      plist = mplist_add (ft_font_list, family, pl);
    }

#else  /* not HAVE_FONTCONFIG */

  if (! all_fonts_scaned)
    {
      ft_init_font_list ();
      all_fonts_scaned = 1;
    }
  if (family == Mnil)
    plist = ft_font_list;
  else
    {
      plist = mplist_find_by_key (ft_font_list, family);
      if (! plist)
	plist = mplist_push (ft_font_list, family, mplist ());
    }
#endif  /* not HAVE_FONTCONFIG */

  return plist;
}

static MPlist *
ft_list_language (MSymbol language)
{
  MPlist *plist = NULL;
  MText *mt;

  if (! ft_language_list)
    ft_language_list = mplist ();
  else if ((plist = mplist_find_by_key (ft_language_list, language)))
    return (MPLIST_VAL (plist) ? MPLIST_PLIST (plist) : NULL);

  mt = mlanguage_text (language);

#ifdef HAVE_FONTCONFIG
  {
    FcPattern *pattern = NULL;
    FcCharSet *cs = NULL;
    FcLangSet *ls = NULL;

    if (! (pattern = FcPatternCreate ()))
      goto err;

    if (mt && mtext_nchars (mt) > 0)
      {
	cs = fc_build_charset (NULL, mt);	
	if (cs && ! FcPatternAddCharSet (pattern, FC_CHARSET, cs))
	  goto err;
      }
    else
      {
	if (! (ls = FcLangSetCreate ()))
	  goto err;
	if (! FcLangSetAdd (ls, (FcChar8 *) MSYMBOL_NAME (language))
	    || ! FcPatternAddLangSet (pattern, FC_LANG, ls))
	  goto err;
      }

    plist = fc_list_pattern (pattern);
  err:
    if (cs) FcCharSetDestroy (cs);
    if (ls) FcLangSetDestroy (ls);
    if (pattern) FcPatternDestroy (pattern);
  }
#else	/* not HAVE_FONTCONFIG */
  if (mt && mtext_nchars (mt) > 0)
    plist = ft_list_char_list (NULL, mt);
#endif  /* not HAVE_FONTCONFIG */

  mplist_push (ft_language_list, language, plist);
  return plist;
}

static MPlist *
ft_list_script (MSymbol script)
{
  MPlist *plist = NULL;
  MPlist *char_list;

  if (! ft_script_list)
    ft_script_list = mplist ();
  else if ((plist = mplist_find_by_key (ft_script_list, script)))
    return (MPLIST_VAL (plist) ? MPLIST_PLIST (plist) : NULL);

  char_list = mscript__char_list (script);

#ifdef HAVE_FONTCONFIG
  if (char_list)
    {
      FcPattern *pattern = NULL;
      FcCharSet *cs;

      if (! (pattern = FcPatternCreate ()))
	goto err;
      cs = fc_build_charset (char_list, NULL);
      if (cs && ! FcPatternAddCharSet (pattern, FC_CHARSET, cs))
	  goto err;
      plist = fc_list_pattern (pattern);
    err:
      if (cs) FcCharSetDestroy (cs);
      if (pattern) FcPatternDestroy (pattern);
    }
#else  /* not HAVE_FONTCONFIG */
  if (char_list)
    plist = ft_list_char_list (char_list, NULL);
#endif	/* not HAVE_FONTCONFIG */

  mplist_push (ft_script_list, script, plist);
  return (plist);
}

static int
ft_check_cap_otf (MFontFT *ft_info, MFontCapability *cap, FT_Face ft_face)
{
#ifdef HAVE_OTF
  if (ft_info->otf == invalid_otf)
    return -1;
  if (! ft_info->otf)
    {
#if (LIBOTF_MAJOR_VERSION > 0 || LIBOTF_MINOR_VERSION > 9 || LIBOTF_RELEASE_NUMBER > 4)
      if (ft_face)
	ft_info->otf = OTF_open_ft_face (ft_face);
      else
#endif
	ft_info->otf = OTF_open (MSYMBOL_NAME (ft_info->font.file));
      if (! ft_info->otf)
	{
	  ft_info->otf = invalid_otf;
	  return -1;
	}
    }
  if (cap->features[MFONT_OTT_GSUB].nfeatures
      && (OTF_check_features
	  (ft_info->otf, 1,
	   cap->script_tag, cap->langsys_tag,
	   cap->features[MFONT_OTT_GSUB].tags,
	   cap->features[MFONT_OTT_GSUB].nfeatures) != 1))
    return -1;
  if (cap->features[MFONT_OTT_GPOS].nfeatures
      && (OTF_check_features
	  (ft_info->otf, 0,
	   cap->script_tag, cap->langsys_tag,
	   cap->features[MFONT_OTT_GPOS].tags,
	   cap->features[MFONT_OTT_GPOS].nfeatures) != 1))
    return -1;
  return 0;
#else	/* not HAVE_OTF */
  return -1;
#endif	/* not HAVE_OTF */
}

static int
ft_check_language (MFontFT *ft_info, MSymbol language, FT_Face ft_face)
{
  MText *mt;
  MText *extra;
  int ft_face_allocaed = 0;
  int len, total_len;
  int i;

#ifdef HAVE_FONTCONFIG
  if (ft_info->langset
      && (FcLangSetHasLang (ft_info->langset,
			    (FcChar8 *) MSYMBOL_NAME (language))
	  != FcLangDifferentLang))
    return 0;
#endif	/* HAVE_FONTCONFIG */

  mt = mlanguage_text (language);
  if (! mt || mtext_nchars (mt) == 0)
    return -1;

  if (! ft_face)
    {
      char *filename = MSYMBOL_NAME (ft_info->font.file);

      if (FT_New_Face (ft_library, filename, 0, &ft_face))
	return -1;
      ft_face_allocaed = 1;
    }

  len = mtext_nchars (mt);
  extra = mtext_get_prop (mt, 0, Mtext);
  total_len = len + (extra ? mtext_nchars (extra) : 0);

  for (i = 0; i < total_len; i++)
    {
      int c = (i < len ? mtext_ref_char (mt, i)
	       : mtext_ref_char (extra, i - len));

#ifdef HAVE_FONTCONFIG
      if (ft_info->charset
	  && FcCharSetHasChar (ft_info->charset, (FcChar32) c) == FcFalse)
	break;
#endif	/* HAVE_FONTCONFIG */
      if (FT_Get_Char_Index (ft_face, (FT_ULong) c) == 0)
	break;
    }

  if (ft_face_allocaed)
    FT_Done_Face (ft_face);

  return (i == total_len ? 0 : -1);
}

static int
ft_check_script (MFontFT *ft_info, MSymbol script, FT_Face ft_face)
{
  MPlist *char_list = mscript__char_list (script);

  if (! char_list)
    return -1;
#ifdef HAVE_FONTCONFIG
  if (ft_info->charset)
    {
      MPLIST_DO (char_list, char_list)
	if (FcCharSetHasChar (ft_info->charset,
			      (FcChar32) MPLIST_INTEGER (char_list)) == FcFalse)
	  break;
    }
  else
#endif	/* HAVE_FONTCONFIG */
    {
      int ft_face_allocaed = 0;

      if (! ft_face)
	{
	  char *filename = MSYMBOL_NAME (ft_info->font.file);

	  if (FT_New_Face (ft_library, filename, 0, &ft_face))
	    return -1;
	  ft_face_allocaed = 1;
	}

      MPLIST_DO (char_list, char_list)
	if (FT_Get_Char_Index (ft_face, (FT_ULong) MPLIST_INTEGER (char_list))
	    == 0)
	  break;
      if (ft_face_allocaed)
	FT_Done_Face (ft_face);
    }

  return (MPLIST_TAIL_P (char_list) ? 0 : -1);
}

static MPlist *ft_default_list;

static MPlist *
ft_list_default ()
{
  if (ft_default_list)
    return ft_default_list;
  ft_default_list = mplist ();
#ifdef HAVE_FONTCONFIG
  {
    FcPattern *pat = FcPatternCreate ();
    FcChar8 *fam;
    char *buf;
    int bufsize = 0;
    int i;

    FcConfigSubstitute (fc_config, pat, FcMatchPattern);
    for (i = 0; FcPatternGetString (pat, FC_FAMILY, i, &fam) == FcResultMatch;
	 i++)
      {
	MSymbol family;
	MPlist *plist;

	STRDUP_LOWER (buf, bufsize, (char *) fam);
	family = msymbol (buf);
	if (msymbol_get (family, Mgeneric_family))
	  continue;
	plist = MPLIST_PLIST (ft_list_family (family, 0, 1));
	MPLIST_DO (plist, plist)
	  mplist_add (ft_default_list, family, MPLIST_VAL (plist));
      }
  }
#else  /* not HAVE_FONTCONFIG */
  {
    MPlist *plist, *pl;

    MPLIST_DO (plist, ft_list_family (Mnil, 0, 1))
      {
	pl = MPLIST_PLIST (plist);
	if (! MPLIST_TAIL_P (pl))
	  mplist_add (ft_default_list, MPLIST_KEY (plist), pl);
      }
  }
#endif	/* not HAVE_FONTCONFIG */
  return ft_default_list;
}


static MPlist *ft_capability_list;

static MPlist *
ft_list_capability (MSymbol capability)
{
  MFontCapability *cap;
  MPlist *plist = NULL, *pl;

  if (! ft_capability_list)
    ft_capability_list = mplist ();
  else if ((plist = mplist_find_by_key (ft_capability_list, capability)))
    return (MPLIST_VAL (plist) ? MPLIST_VAL (plist) : NULL);

  cap = mfont__get_capability (capability);

  if (cap && cap->language != Mnil)
    {
      plist = ft_list_language (cap->language);
      if (! plist)
	return NULL;
      plist = mplist_copy (plist);
    }

  if (cap && cap->script != Mnil)
    {
      if (! plist)
	{
	  plist = ft_list_script (cap->script);
	  if (! plist)
	    return NULL;
	  plist = mplist_copy (plist);
	}
      else
	{
	  for (pl = plist; ! MPLIST_TAIL_P (pl);)
	    {
	      if (ft_check_script (MPLIST_VAL (pl), cap->script, NULL) < 0)
		mplist_pop (pl);
	      else
		pl = MPLIST_NEXT (pl);
	    }
	}

      if (cap->script_tag)
	{
	  for (pl = plist; ! MPLIST_TAIL_P (pl);)
	    {
	      if (ft_check_cap_otf (MPLIST_VAL (pl), cap, NULL) < 0)
		mplist_pop (pl);
	      else
		pl = MPLIST_NEXT (pl);
	    }
	}

      if (MPLIST_TAIL_P (plist))
	{
	  M17N_OBJECT_UNREF (plist);
	  plist = NULL;
	}
    }

  mplist_push (ft_capability_list, capability, plist);
  return plist;
}


static MPlist *
ft_list_file (MSymbol filename)
{
  MPlist *plist = NULL;

  if (! ft_file_list)
    ft_file_list = mplist ();
  else if ((plist = mplist_find_by_key (ft_file_list, filename)))
    return (MPLIST_VAL (plist) ? MPLIST_PLIST (plist) : NULL);

#ifdef HAVE_FONTCONFIG
  {
    FcPattern *pattern = FcPatternCreate ();
    FcObjectSet *os;
    FcFontSet *fs;

    FcPatternAddString (pattern, FC_FILE, (FcChar8 *) MSYMBOL_NAME (filename));
    os = FcObjectSetBuild (FC_FAMILY, NULL);
    fs = FcFontList (fc_config, pattern, os);
    if (fs->nfont > 0)
      {
	char *fam;
	char *buf;
	int bufsize = 0;

	if (FcPatternGetString (fs->fonts[0], FC_FAMILY, 0,
				(FcChar8 **) &fam) == FcResultMatch)
	  {
	    MSymbol family;
	    MPlist *pl;

	    STRDUP_LOWER (buf, bufsize, fam);
	    family = msymbol (buf);
	    pl = ft_list_family (family, 0, 1);
	    MPLIST_DO (pl, MPLIST_PLIST (pl))
	      {
		MFontFT *ft_info = MPLIST_VAL (pl);

		if (ft_info->font.file == filename)
		  {
		    plist = mplist ();
		    mplist_add (plist, family, ft_info);
		    break;
		  }
	      }
	  }
      }
  }
#else  /* not HAVE_FONTCONFIG */
  {
    MPlist *pl, *p;

    MPLIST_DO (pl, ft_list_family (Mnil, 0, 1))
      {
	MPLIST_DO (p, MPLIST_PLIST (pl))
	  {
	    MFontFT *ft_info = MPLIST_VAL (pl);

	    if (ft_info->font.file == filename)
	      {
		plist = mplist ();
		mplist_add (plist, MPLIST_KEY (pl), ft_info);
		break;
	      }
	  }
	if (plist)
	  break;
      }
  }
#endif	/* not HAVE_FONTCONFIG */

  mplist_push (ft_file_list, filename, plist);
  return plist;
}

/* The FreeType font driver function SELECT.  */

static MFont *
ft_select (MFrame *frame, MFont *font, int limited_size)
{
  MFont *found = NULL;
#ifdef HAVE_FONTCONFIG
  MPlist *plist, *pl;
  MFontFT *ft_info;
  int check_font_property = 1;

  if (font->file != Mnil)
    {
      plist = ft_list_file (font->file);
      if (! plist)
	return NULL;
      check_font_property = 0;
    }
  else
    {
      MSymbol family = FONT_PROPERTY (font, MFONT_FAMILY);

      if (family)
	plist = MPLIST_PLIST (ft_list_family (family, 1, 1));
      else
	plist = ft_list_default ();
      if (MPLIST_TAIL_P (plist))
	return NULL;
    }

  plist = mplist_copy (plist);

  if (font->capability != Mnil)
    {
      MFontCapability *cap = mfont__get_capability (font->capability);

      for (pl = plist; ! MPLIST_TAIL_P (pl);)
	{
	  if (cap->script_tag && ft_check_cap_otf (MPLIST_VAL (pl), cap, NULL) < 0)
	    {
	      mplist_pop (pl);
	      continue;
	    }
	  if (cap->language
	      && ft_check_language (MPLIST_VAL (pl), cap->language, NULL) < 0)
	    mplist_pop (pl);
	  else
	    pl = MPLIST_NEXT (pl);
	}
    }

  if (check_font_property)
    {
      MSymbol weight = FONT_PROPERTY (font, MFONT_WEIGHT);
      MSymbol style = FONT_PROPERTY (font, MFONT_STYLE);
      MSymbol stretch = FONT_PROPERTY (font, MFONT_STRETCH);
      MSymbol alternate_weight = Mnil;

      if (weight == Mnormal)
	alternate_weight = Mmedium;
      else if (weight == Mmedium)
	alternate_weight = Mnormal;
      if (weight != Mnil || style != Mnil || stretch != Mnil || font->size > 0)
	for (pl = plist; ! MPLIST_TAIL_P (pl); )
	  {
	    ft_info = MPLIST_VAL (pl);
	    if ((weight != Mnil
		 && (weight != FONT_PROPERTY (&ft_info->font, MFONT_WEIGHT)
		     && alternate_weight != FONT_PROPERTY (&ft_info->font, MFONT_WEIGHT)))
		|| (style != Mnil
		    && style != FONT_PROPERTY (&ft_info->font, MFONT_STYLE))
		|| (stretch != Mnil
		    && stretch != FONT_PROPERTY (&ft_info->font,
						 MFONT_STRETCH))
		|| (font->size > 0
		    && ft_info->font.size > 0
		    && ft_info->font.size != font->size))
	      mplist_pop (pl);
	    else
	      pl = MPLIST_NEXT (pl);
	  }
    }

  MPLIST_DO (pl, plist)
    {
      font = MPLIST_VAL (plist);
      if (limited_size == 0
	  || font->size == 0
	  || font->size <= limited_size)
	{
	  found = font;
	  break;
	}
    }
  M17N_OBJECT_UNREF (plist);
#endif	/* HAVE_FONTCONFIG */
  return found;
}


static MRealizedFont *
ft_open (MFrame *frame, MFont *font, MFont *spec, MRealizedFont *rfont)
{
  MFontFT *ft_info = (MFontFT *) font;
  int reg = spec->property[MFONT_REGISTRY];
  MSymbol registry = FONT_PROPERTY (spec, MFONT_REGISTRY);
  MRealizedFontFT *ft_rfont;
  FT_Face ft_face;
  MPlist *plist, *charmap_list = NULL;
  int charmap_index;
  int size;

  if (font->size)
    /* non-scalable font */
    size = font->size;
  else if (spec->size)
    {
      int ratio = mfont_resize_ratio (font);

      size = ratio == 100 ? spec->size : spec->size * ratio / 100;
    }
  else
    size = 120;

  if (rfont)
    {
      charmap_list = ((MRealizedFontFT *) rfont->info)->charmap_list;
      for (; rfont; rfont = rfont->next)
	if (rfont->font == font
	    && (rfont->font->size ? rfont->font->size == size
		: rfont->spec.size == size)
	    && rfont->spec.property[MFONT_REGISTRY] == reg
	    && rfont->driver == &mfont__ft_driver)
	  return rfont;
    }

  MDEBUG_DUMP (" [FONT-FT] opening ", "", mdebug_dump_font (&ft_info->font));

  if (FT_New_Face (ft_library, MSYMBOL_NAME (ft_info->font.file), 0,
		   &ft_face))
    {
      font->type = MFONT_TYPE_FAILURE;
      MDEBUG_PRINT ("  no (FT_New_Face)\n");
      return NULL;
    }
  if (charmap_list)
    M17N_OBJECT_REF (charmap_list);
  else
    charmap_list = ft_get_charmaps (ft_face);
  if (registry == Mnil)
    registry = Municode_bmp;
  plist = mplist_find_by_key (charmap_list, registry);
  if (! plist)
    {
      FT_Done_Face (ft_face);
      M17N_OBJECT_UNREF (charmap_list);
      MDEBUG_PRINT1 ("  no (%s)\n", MSYMBOL_NAME (registry));
      return NULL;
    }
  charmap_index = (int) MPLIST_VAL (plist);
  if ((charmap_index >= 0
       && FT_Set_Charmap (ft_face, ft_face->charmaps[charmap_index]))
      || FT_Set_Pixel_Sizes (ft_face, 0, size / 10))
    {
      FT_Done_Face (ft_face);
      M17N_OBJECT_UNREF (charmap_list);
      font->type = MFONT_TYPE_FAILURE;
      MDEBUG_PRINT1 ("  no (size %d)\n", size);
      return NULL;
    }

  M17N_OBJECT (ft_rfont, free_ft_rfont, MERROR_FONT_FT);
  ft_rfont->ft_face = ft_face;
  ft_rfont->charmap_list = charmap_list;
  MSTRUCT_CALLOC (rfont, MERROR_FONT_FT);
  rfont->id = ft_info->font.file;
  rfont->spec = *font;
  rfont->spec.type = MFONT_TYPE_REALIZED;
  rfont->spec.property[MFONT_REGISTRY] = reg;
  rfont->spec.size = size;
  rfont->frame = frame;
  rfont->font = font;
  rfont->driver = &mfont__ft_driver;
  rfont->info = ft_rfont;
  rfont->fontp = ft_face;
  rfont->ascent = ft_face->size->metrics.ascender;
  rfont->descent = - ft_face->size->metrics.descender;
  rfont->max_advance = ft_face->size->metrics.max_advance;
  rfont->baseline_offset = 0;
  rfont->x_ppem = ft_face->size->metrics.x_ppem;
  rfont->y_ppem = ft_face->size->metrics.y_ppem;
#ifdef HAVE_FTBDF_H
  {
    BDF_PropertyRec prop;

    if (! FT_IS_SCALABLE (ft_face)
	&& FT_Get_BDF_Property (ft_face, "_MULE_BASELINE_OFFSET", &prop) == 0)
      {
	rfont->baseline_offset = prop.u.integer << 6;
	rfont->ascent += prop.u.integer << 6;
	rfont->descent -= prop.u.integer << 6;
      }
  }
#endif	/* HAVE_FTBDF_H */
  if (FT_IS_SCALABLE (ft_face))
    rfont->average_width = 0;
  else
    rfont->average_width = ft_face->available_sizes->width << 6;
  rfont->next = MPLIST_VAL (frame->realized_font_list);
  MPLIST_VAL (frame->realized_font_list) = rfont;
  MDEBUG_PRINT ("  ok\n");
  return rfont;
}

/* The FreeType font driver function FIND_METRIC.  */

static void
ft_find_metric (MRealizedFont *rfont, MGlyphString *gstring,
		int from, int to)
{
  FT_Face ft_face = rfont->fontp;
  MGlyph *g = MGLYPH (from), *gend = MGLYPH (to);

  for (; g != gend; g++)
    {
      if (g->g.measured)
	continue;
      if (g->g.code == MCHAR_INVALID_CODE)
	{
	  if (FT_IS_SCALABLE (ft_face))
	    {
	      g->g.lbearing = 0;
	      g->g.rbearing = ft_face->size->metrics.max_advance;
	      g->g.xadv = g->g.rbearing;
	      g->g.ascent = ft_face->size->metrics.ascender;
	      g->g.descent = - ft_face->size->metrics.descender;
	    }
	  else
	    {
#ifdef HAVE_FTBDF_H
	      BDF_PropertyRec prop;
#endif	/* HAVE_FTBDF_H */

	      g->g.lbearing = 0;
	      g->g.rbearing = g->g.xadv = ft_face->available_sizes->width << 6;
#ifdef HAVE_FTBDF_H
	      if (FT_Get_BDF_Property (ft_face, "ASCENT", &prop) == 0)
		{
		  g->g.ascent = prop.u.integer << 6;
		  FT_Get_BDF_Property (ft_face, "DESCENT", &prop);
		  g->g.descent = prop.u.integer << 6;
		  if (FT_Get_BDF_Property (ft_face, "_MULE_BASELINE_OFFSET",
					   & prop) == 0)
		    {
		      g->g.ascent += prop.u.integer << 6;
		      g->g.descent -= prop.u.integer << 6;
		    }
		}
	      else
#endif	/* HAVE_FTBDF_H */
		{
		  g->g.ascent = ft_face->available_sizes->height << 6;
		  g->g.descent = 0;
		}
	    }
	}
      else
	{
	  FT_Glyph_Metrics *metrics;

	  FT_Load_Glyph (ft_face, (FT_UInt) g->g.code, FT_LOAD_DEFAULT);
	  metrics = &ft_face->glyph->metrics;
	  g->g.lbearing = metrics->horiBearingX;
	  g->g.rbearing = metrics->horiBearingX + metrics->width;
	  g->g.xadv = metrics->horiAdvance;
	  g->g.ascent = metrics->horiBearingY;
	  g->g.descent = metrics->height - metrics->horiBearingY;
	}
      g->g.yadv = 0;
      g->g.ascent += rfont->baseline_offset;
      g->g.descent -= rfont->baseline_offset;
      g->g.measured = 1;
    }
}

static int
ft_has_char (MFrame *frame, MFont *font, MFont *spec, int c, unsigned code)
{
  MRealizedFont *rfont = NULL;
  MRealizedFontFT *ft_rfont;
  FT_UInt idx;

  if (font->type == MFONT_TYPE_REALIZED)
    rfont = (MRealizedFont *) font;
  else if (font->type == MFONT_TYPE_OBJECT)
    {
      for (rfont = MPLIST_VAL (frame->realized_font_list); rfont;
	   rfont = rfont->next)
	if (rfont->font == font && rfont->driver == &mfont__ft_driver)
	  break;
      if (! rfont)
	{
#ifdef HAVE_FONTCONFIG
	  MFontFT *ft_info = (MFontFT *) font;

	  if (! ft_info->charset)
	    {
	      FcPattern *pat = FcPatternBuild (NULL, FC_FILE, FcTypeString,
					       MSYMBOL_NAME (font->file),
					       NULL);
	      FcObjectSet *os = FcObjectSetBuild (FC_CHARSET, NULL);
	      FcFontSet *fs = FcFontList (fc_config, pat, os);

	      if (fs->nfont > 0
		  && FcPatternGetCharSet (fs->fonts[0], FC_CHARSET, 0,
					  &ft_info->charset) == FcResultMatch)
		ft_info->charset = FcCharSetCopy (ft_info->charset);
	      else
		ft_info->charset = FcCharSetCreate ();
	      FcFontSetDestroy (fs);
	      FcObjectSetDestroy (os);
	      FcPatternDestroy (pat);
	    }
	  return (FcCharSetHasChar (ft_info->charset, (FcChar32) c) == FcTrue);
#else  /* not HAVE_FONTCONFIG */
	  rfont = ft_open (frame, font, spec, NULL);
#endif	/* not HAVE_FONTCONFIG */
	}
    }
  else
    MFATAL (MERROR_FONT_FT);

  if (! rfont)
    return 0;
  ft_rfont = rfont->info;
  idx = FT_Get_Char_Index (ft_rfont->ft_face, (FT_ULong) code);
  return (idx != 0);
}

/* The FreeType font driver function ENCODE_CHAR.  */

static unsigned
ft_encode_char (MFrame *frame, MFont *font, MFont *spec, unsigned code)
{
  MRealizedFont *rfont;
  MRealizedFontFT *ft_rfont;
  FT_UInt idx;

  if (font->type == MFONT_TYPE_REALIZED)
    rfont = (MRealizedFont *) font;
  else if (font->type == MFONT_TYPE_OBJECT)
    {
      for (rfont = MPLIST_VAL (frame->realized_font_list); rfont;
	   rfont = rfont->next)
	if (rfont->font == font && rfont->driver == &mfont__ft_driver)
	  break;
      if (! rfont)
	{
	  rfont = ft_open (frame, font, spec, NULL);
	  if (! rfont)
	    return -1;
	}
    }
  else
    MFATAL (MERROR_FONT_FT);

  ft_rfont = rfont->info;
  idx = FT_Get_Char_Index (ft_rfont->ft_face, (FT_ULong) code);
  return (idx ? (unsigned) idx : MCHAR_INVALID_CODE);
}

/* The FreeType font driver function RENDER.  */

#define NUM_POINTS 0x1000

typedef struct {
  MDrawPoint points[NUM_POINTS];
  MDrawPoint *p;
} MPointTable;

static void
ft_render (MDrawWindow win, int x, int y,
	   MGlyphString *gstring, MGlyph *from, MGlyph *to,
	   int reverse, MDrawRegion region)
{
  FT_Face ft_face;
  MRealizedFace *rface = from->rface;
  MFrame *frame = rface->frame;
  FT_Int32 load_flags = FT_LOAD_RENDER;
  MGlyph *g;
  int i, j;
  MPointTable point_table[8];
  int baseline_offset;
  int pixel_mode = -1;

  if (from == to)
    return;

  /* It is assured that the all glyphs in the current range use the
     same realized face.  */
  ft_face = rface->rfont->fontp;
  baseline_offset = rface->rfont->baseline_offset >> 6;

  if (! gstring->anti_alias)
    {
#ifdef FT_LOAD_TARGET_MONO
      load_flags |= FT_LOAD_TARGET_MONO;
#else
      load_flags |= FT_LOAD_MONOCHROME;
#endif
    }

  for (i = 0; i < 8; i++)
    point_table[i].p = point_table[i].points;

  for (g = from; g < to; x += g++->g.xadv)
    {
      unsigned char *bmp;
      int intensity;
      MPointTable *ptable;
      int xoff, yoff;
      int width, pitch;

      FT_Load_Glyph (ft_face, (FT_UInt) g->g.code, load_flags);
      if (pixel_mode < 0)
	pixel_mode = ft_face->glyph->bitmap.pixel_mode;
      yoff = y - ft_face->glyph->bitmap_top + g->g.yoff;
      bmp = ft_face->glyph->bitmap.buffer;
      width = ft_face->glyph->bitmap.width;
      pitch = ft_face->glyph->bitmap.pitch;

      if (pixel_mode != FT_PIXEL_MODE_MONO)
	for (i = 0; i < ft_face->glyph->bitmap.rows;
	     i++, bmp += ft_face->glyph->bitmap.pitch, yoff++)
	  {
	    xoff = x + ft_face->glyph->bitmap_left + g->g.xoff;
	    for (j = 0; j < width; j++, xoff++)
	      {
		intensity = bmp[j] >> 5;
		if (intensity)
		  {
		    ptable = point_table + intensity;
		    ptable->p->x = xoff;
		    ptable->p->y = yoff - baseline_offset;
		    ptable->p++;
		    if (ptable->p - ptable->points == NUM_POINTS)
		      {
			(*frame->driver->draw_points)
			  (frame, win, rface,
			   reverse ? 7 - intensity : intensity,
			   ptable->points, NUM_POINTS, region);
			ptable->p = ptable->points;
		      }
		  }
	      }
	  }
      else
	for (i = 0; i < ft_face->glyph->bitmap.rows;
	     i++, bmp += ft_face->glyph->bitmap.pitch, yoff++)
	  {
	    xoff = x + ft_face->glyph->bitmap_left + g->g.xoff;
	    for (j = 0; j < width; j++, xoff++)
	      {
		intensity = bmp[j / 8] & (1 << (7 - (j % 8)));
		if (intensity)
		  {
		    ptable = point_table;
		    ptable->p->x = xoff;
		    ptable->p->y = yoff - baseline_offset;
		    ptable->p++;
		    if (ptable->p - ptable->points == NUM_POINTS)
		      {
			(*frame->driver->draw_points) (frame, win, rface,
					   reverse ? 0 : 7,
					   ptable->points, NUM_POINTS, region);
			ptable->p = ptable->points;
		      }		    
		  }
	      }
	}
    }

  if (pixel_mode != FT_PIXEL_MODE_MONO)
    {
      for (i = 1; i < 8; i++)
	if (point_table[i].p != point_table[i].points)
	  (*frame->driver->draw_points) (frame, win, rface, reverse ? 7 - i : i,
			     point_table[i].points,
			     point_table[i].p - point_table[i].points, region);
    }
  else
    {
      if (point_table[0].p != point_table[0].points)
	(*frame->driver->draw_points) (frame, win, rface, reverse ? 0 : 7,
			   point_table[0].points,
			   point_table[0].p - point_table[0].points, region);
    }
}

static int
ft_list (MFrame *frame, MPlist *plist, MFont *font, int maxnum)
{
  MPlist *pl = NULL, *p;
  int num = 0;
  MPlist *file_list = NULL;
  MPlist *family_list = NULL, *capability_list = NULL;
  MSymbol registry = Mnil;

  MDEBUG_DUMP (" [FONT-FT] listing ", "", mdebug_dump_font (font));

  if (font)
    {
      MSymbol family;

      registry = FONT_PROPERTY (font, MFONT_REGISTRY);
      if (registry != Mnil && registry != Miso8859_1)
	{
	  char *reg = MSYMBOL_NAME (registry);

	  if (strncmp (reg, "unicode-", 8)
	      && strncmp (reg, "apple-roman", 11)
	      && (reg[0] < '0' || reg[0] > '9' || reg[1] != '-'))
	    goto done;
	}

      if (font->file != Mnil
	  && ! (file_list = ft_list_file (font->file)))
	goto done;
      family = FONT_PROPERTY (font, MFONT_FAMILY);
      if (family != Mnil
	  && (family_list = MPLIST_PLIST (ft_list_family (family, 1, 1)))
	  && MPLIST_TAIL_P (family_list))
	goto done;
      if (font->capability != Mnil)
	{
	  capability_list = ft_list_capability (font->capability);
	  if (! capability_list || MPLIST_TAIL_P (capability_list))
	    goto done;
	}
    }

  if (! file_list && ! family_list && ! capability_list)
    {
      /* No restriction.  Get all fonts.  */
      pl = mplist ();
      MPLIST_DO (family_list, ft_list_family (Mnil, 0, 1))
	{
	  MPLIST_DO (p, MPLIST_PLIST (family_list))
	    mplist_push (pl, MPLIST_KEY (p), MPLIST_VAL (p));
	}
    }
  else
    {
      if (file_list)
	{
	  pl = mplist ();
	  mplist_push (pl, MPLIST_KEY (file_list), MPLIST_VAL (file_list));
	}
      if (family_list)
	{
	  if (pl)
	    for (p = pl; ! MPLIST_TAIL_P (p);)
	      {
		if (mplist_find_by_value (family_list, MPLIST_VAL (p)))
		  p = MPLIST_NEXT (p);
		else
		  mplist_pop (p);
	      }
	  else
	    {
	      pl = mplist ();
	      MPLIST_DO (p, family_list)
		mplist_push (pl, MPLIST_KEY (p), MPLIST_VAL (p));
	    }
	}
      if (capability_list)
	{
	  if (pl)
	    for (p = pl; ! MPLIST_TAIL_P (p);)
	      {
		if (mplist_find_by_value (capability_list, MPLIST_VAL (p)))
		  p = MPLIST_NEXT (p);
		else
		  mplist_pop (p);
	      }
	  else
	    {
	      pl = mplist ();
	      MPLIST_DO (p, capability_list)
		mplist_push (pl, MPLIST_KEY (p), MPLIST_VAL (p));
	    }
	}
    }
	      
  if (font
      && (font->property[MFONT_WEIGHT] + font->property[MFONT_STYLE]
	  + font->property[MFONT_STRETCH] + font->size) > 0)
    {
      MSymbol weight = FONT_PROPERTY (font, MFONT_WEIGHT);
      MSymbol style = FONT_PROPERTY (font, MFONT_STYLE);
      MSymbol stretch = FONT_PROPERTY (font, MFONT_STRETCH);
      int size = font->size;

      for (p = pl; ! MPLIST_TAIL_P (p); )
	{
	  MFontFT *ft_info = MPLIST_VAL (p);

	  if ((weight != Mnil
	       && weight != FONT_PROPERTY (&ft_info->font, MFONT_WEIGHT))
	      || (style != Mnil
		  && style != FONT_PROPERTY (&ft_info->font, MFONT_STYLE))
	      || (stretch != Mnil
		  && stretch != FONT_PROPERTY (&ft_info->font,
					       MFONT_STRETCH))
	      || (size > 0
		  && ft_info->font.size > 0
		  && ft_info->font.size != size))
	    mplist_pop (p);
	  else
	    p = MPLIST_NEXT (p);
	}
    }

  MPLIST_DO (p, pl)
    {
      mplist_push (plist, MPLIST_KEY (p), MPLIST_VAL (p));
      num++;
      if (maxnum && maxnum <= num)
	break;
    }
  M17N_OBJECT_UNREF (pl);

 done:
  MDEBUG_PRINT1 ("  %d found\n", num);
  return num;
}

static void
ft_list_family_names (MFrame *frame, MPlist *plist)
{
  MPlist *pl;

  if (! ft_font_list)
    {
#ifdef HAVE_FONTCONFIG
      fc_init_font_list ();
#else  /* not HAVE_FONTCONFIG */
      ft_init_font_list ();
#endif	/* not HAVE_FONTCONFIG */
    }

  MPLIST_DO (pl, ft_font_list)
    {
      MSymbol family = MPLIST_KEY (pl);
      MPlist *p;

#ifdef HAVE_FONTCONFIG
      if (msymbol_get (family, Mgeneric_family) != Mnil)
	continue;
#endif	/* HAVE_FONTCONFIG */
      MPLIST_DO (p, plist)
	{
	  MSymbol sym = MPLIST_SYMBOL (p);

	  if (sym == family)
	    break;
	  if (strcmp (MSYMBOL_NAME (sym), MSYMBOL_NAME (family)) > 0)
	    {
	      mplist_push (p, Msymbol, family);
	      break;
	    }
	}
      if (MPLIST_TAIL_P (p))
	mplist_push (p, Msymbol, family);
    }
}

static int 
ft_check_capability (MRealizedFont *rfont, MSymbol capability)
{
  MFontFT *ft_info = (MFontFT *) rfont->font;
  MRealizedFontFT *ft_rfont = rfont->info;
  MFontCapability *cap = mfont__get_capability (capability);

  if (cap->script_tag)
    {
      if (ft_check_cap_otf (ft_info, cap, ft_rfont->ft_face) < 0)
	return -1;
    }
  else if (cap->script != Mnil
	   && ft_check_script (ft_info, cap->script, ft_rfont->ft_face) < 0)
    return -1;
  if (cap->language != Mnil
      && ft_check_language (ft_info, cap->language, ft_rfont->ft_face) < 0)
    return -1;
  return 0;
}

static MRealizedFont *
ft_encapsulate (MFrame *frame, MSymbol data_type, void *data)
{
  MFontFT *ft_info;
  MRealizedFont *rfont;
  MRealizedFontFT *ft_rfont;
  FT_Face ft_face;

  if (data_type == Mfontconfig)
    {
#ifdef HAVE_FONTCONFIG
      FcPattern *pattern = data;

      if (FcPatternGetFTFace (pattern, FC_FT_FACE, 0, &ft_face)
	  != FcResultMatch)
	return NULL;
      ft_info = fc_gen_font (pattern, NULL);
#else  /* not HAVE_FONTCONFIG */
      return NULL;
#endif	/* not HAVE_FONTCONFIG */
    }
  else if (data_type == Mfreetype)
    {
      ft_face = data;
      ft_info = ft_gen_font (ft_face);
    }
  else
    return NULL;

  M17N_OBJECT (ft_rfont, free_ft_rfont, MERROR_FONT_FT);
  ft_rfont->ft_face = ft_face;
  ft_rfont->face_encapsulated = 1;

  MDEBUG_PRINT1 (" [FONT-FT] encapsulating %s", (char *) ft_face->family_name);

  MSTRUCT_CALLOC (rfont, MERROR_FONT_FT);
  rfont->id = ft_info->font.file;
  rfont->font = (MFont *) ft_info;
  rfont->info = ft_rfont;
  rfont->fontp = ft_face;
  rfont->driver = &mfont__ft_driver;
  rfont->spec = ft_info->font;
  rfont->spec.type = MFONT_TYPE_REALIZED;
  rfont->frame = frame;
  rfont->ascent = ft_face->size->metrics.ascender >> 6;
  rfont->descent = - ft_face->size->metrics.descender >> 6;
  rfont->max_advance = ft_face->size->metrics.max_advance >> 6;
  rfont->baseline_offset = 0;
  rfont->x_ppem = ft_face->size->metrics.x_ppem;
  rfont->y_ppem = ft_face->size->metrics.y_ppem;
#ifdef HAVE_FTBDF_H
  {
    BDF_PropertyRec prop;

    if (! FT_IS_SCALABLE (ft_face)
	&& FT_Get_BDF_Property (ft_face, "_MULE_BASELINE_OFFSET", &prop) == 0)
      {
	rfont->baseline_offset = prop.u.integer << 6;
	rfont->ascent += prop.u.integer << 6;
	rfont->descent -= prop.u.integer << 6;
      }
  }
#endif	/* HAVE_FTBDF_H */
  if (FT_IS_SCALABLE (ft_face))
    rfont->average_width = 0;
  else
    rfont->average_width = ft_face->available_sizes->width << 6;
  rfont->next = MPLIST_VAL (frame->realized_font_list);
  MPLIST_VAL (frame->realized_font_list) = rfont;

  return rfont;
}

static void
ft_close (MRealizedFont *rfont)
{
  if (! rfont->encapsulating)
    return;
  free (rfont->font);
  M17N_OBJECT_UNREF (rfont->info);
  free (rfont);
}

/* See the comment of parse_otf_command (m17n-flt.c).  */

static int 
ft_check_otf (MFLTFont *font, MFLTOtfSpec *spec)
{
#define FEATURE_NONE(IDX) (! spec->features[IDX])

#define FEATURE_ANY(IDX)	\
  (spec->features[IDX]		\
   && spec->features[IDX][0] == 0xFFFFFFFF && spec->features[IDX][1] == 0)

#ifdef HAVE_OTF
  OTF_Tag *tags;
  int i, n, negative;
  OTF *otf = get_otf (font, NULL);
  
  if (FEATURE_ANY (0) && FEATURE_ANY (1))
    /* Return 1 if any of GSUB or GPOS support the script (and language).  */
    return (otf
	    && (OTF_check_features (otf, 0, spec->script, spec->langsys,
				    NULL, 0) > 0
		|| OTF_check_features (otf, 1, spec->script, spec->langsys,
				       NULL, 0) > 0));

  for (i = 0; i < 2; i++)
    if (! FEATURE_ANY (i))
      {
	if (FEATURE_NONE (i))
	  {
	    if (otf
		&& OTF_check_features (otf, i == 0, spec->script, spec->langsys,
				       NULL, 0) > 0)
	      return 0;
	    continue;
	  }
	if (spec->features[i][0] == 0xFFFFFFFF)
	  {
	    if (! otf
		|| OTF_check_features (otf, i == 0, spec->script, spec->langsys,
				       NULL, 0) <= 0)
	      continue;
	  }
	else if (! otf)
	  return 0;
	for (n = 1; spec->features[i][n]; n++);
	tags = alloca (sizeof (OTF_Tag) * n);
	for (n = 0, negative = 0; spec->features[i][n]; n++)
	  {
	    if (spec->features[i][n] == 0xFFFFFFFF)
	      negative = 1;
	    else if (negative)
	      tags[n - 1] = spec->features[i][n] | 0x80000000;
	    else
	      tags[n] = spec->features[i][n];
	  }
	if (OTF_check_features (otf, i == 0, spec->script, spec->langsys,
				tags, n - negative) != 1)
	  return 0;
      }
  return 1;

#endif	/* HAVE_OTF */
  return (FEATURE_NONE (0) ? (FEATURE_NONE (1) || FEATURE_ANY (1))
	  : (FEATURE_NONE (1) && FEATURE_ANY (0)));
}

#ifdef HAVE_OTF

static OTF *
get_otf (MFLTFont *font, FT_Face *ft_face)
{
  MRealizedFont *rfont = ((MFLTFontForRealized *) font)->rfont;
  MFontFT *ft_info = (MFontFT *) rfont->font;
  MRealizedFontFT *ft_rfont = rfont->info;
  OTF *otf = ft_info->otf;

  if (! otf)
    {
#if (LIBOTF_MAJOR_VERSION > 0 || LIBOTF_MINOR_VERSION > 9 || LIBOTF_RELEASE_NUMBER > 4)
      otf = OTF_open_ft_face (ft_rfont->ft_face);
#else
      otf = OTF_open (MSYMBOL_NAME (ft_info->font.file));
#endif
      if (! otf || OTF_get_table (otf, "head") < 0)
	otf = invalid_otf;
      ft_info->otf = otf;
    }
  if (ft_face)
    *ft_face = ft_rfont->ft_face;
  return (otf == invalid_otf ? NULL : otf);
}

#define DEVICE_DELTA(table, size)				\
  (((size) >= (table).StartSize && (size) <= (table).EndSize)	\
   ? (table).DeltaValue[(size) - (table).StartSize] << 6	\
   : 0)

void
adjust_anchor (OTF_Anchor *anchor, FT_Face ft_face,
	       unsigned code, int x_ppem, int y_ppem, int *x, int *y)
{
  if (anchor->AnchorFormat == 2)
    {
      FT_Outline *outline;
      int ap = anchor->f.f1.AnchorPoint;

      FT_Load_Glyph (ft_face, (FT_UInt) code, FT_LOAD_MONOCHROME);
      outline = &ft_face->glyph->outline;
      if (ap < outline->n_points)
	{
	  *x = outline->points[ap].x << 6;
	  *y = outline->points[ap].y << 6;
	}
    }
  else if (anchor->AnchorFormat == 3)
    {
      if (anchor->f.f2.XDeviceTable.offset)
	*x += DEVICE_DELTA (anchor->f.f2.XDeviceTable, x_ppem);
      if (anchor->f.f2.YDeviceTable.offset)
	*y += DEVICE_DELTA (anchor->f.f2.YDeviceTable, y_ppem);
    }
}
#endif	/* HAVE_OTF */

#ifdef HAVE_OTF

static int 
ft_drive_otf (MFLTFont *font, MFLTOtfSpec *spec,
	      MFLTGlyphString *in, int from, int to,
	      MFLTGlyphString *out, MFLTGlyphAdjustment *adjustment)
{
  int len = to - from;
  int i, j, gidx;
  MGlyph *in_glyphs = (MGlyph *) (in->glyphs);
  MGlyph *out_glyphs = out ? (MGlyph *) (out->glyphs) : NULL;
  OTF *otf;
  FT_Face face;
  OTF_GlyphString otf_gstring;
  OTF_Glyph *otfg;
  char script[5], *langsys = NULL;
  char *gsub_features = NULL, *gpos_features = NULL;
  unsigned int tag;

  if (len == 0)
    return from;
  otf = get_otf (font, &face);
  if (! otf)
    goto simple_copy;
  OTF_tag_name (spec->script, script);
  if (spec->langsys)
    {
      langsys = alloca (5);
      OTF_tag_name (spec->langsys, langsys);
    }
  for (i = 0; i < 2; i++)
    {
      char *p;

      if (spec->features[i])
	{
	  for (j = 0; spec->features[i][j]; j++);
	  if (i == 0)
	    p = gsub_features = alloca (6 * j);
	  else
	    p = gpos_features = alloca (6 * j);
	  for (j = 0; spec->features[i][j]; j++)
	    {
	      if (spec->features[i][j] == 0xFFFFFFFF)
		*p++ = '*', *p++ = ',';
	      else
		{
		  OTF_tag_name (spec->features[i][j], p);
		  p[4] = ',';
		  p += 5;
		}
	    }
	  *--p = '\0';
	}
    }

  /* Extra room from pseudo glyphs.  OTF_drive_features () may need
     more rooms.  In that case, otf_gstring.glyphs is reallocated.  */
  otf_gstring.size = len * 2;
  if (! MTABLE_CALLOC_SAFE (otf_gstring.glyphs, otf_gstring.size))
    MERROR (MERROR_MEMORY, -1);
  for (i = 0; i < len; i++)
    {
      otf_gstring.glyphs[i].c = ((MGlyph *)in->glyphs)[from + i].g.c & 0x11FFFF;
      otf_gstring.glyphs[i].glyph_id = ((MGlyph *)in->glyphs)[from + i].g.code;
      otf_gstring.glyphs[i].positioning_type = ((MGlyph *)in->glyphs)[from + i].libotf_positioning_type;
    }
  otf_gstring.used = len;

  OTF_drive_gdef (otf, &otf_gstring);
  gidx = out ? out->used : from;

  if (gsub_features)
    {
      OTF_Feature *features;
      MGlyph *g;

#ifdef OTF_POSITIONING_TYPE_GET_FORMAT
      if (OTF_drive_gsub_features (otf, &otf_gstring, script, langsys,
				   gsub_features) < 0)
	goto simple_copy;
#else
      if (OTF_drive_gsub_with_log (otf, &otf_gstring, script, langsys,
				   gsub_features) < 0)
	goto simple_copy;
#endif
      features = otf->gsub->FeatureList.Feature;
      if (out)
	{
	  if (out->allocated < gidx + otf_gstring.used)
	    return -2;
	  for (i = 0, otfg = otf_gstring.glyphs, g = out_glyphs + gidx;
	       i < otf_gstring.used; i++, otfg++, g++, out->used++)
	    {
	      int j;
	      int min_from, max_to;
	      int feature_idx;

#ifdef OTF_POSITIONING_TYPE_GET_FORMAT
	      feature_idx = OTF_POSITIONING_TYPE_GET_FEATURE (otfg);
#else
	      feature_idx = otfg->positioning_type >> 4;
#endif

	      *g = in_glyphs[from + otfg->f.index.from];
	      min_from = g->g.from, max_to = g->g.to;
	      g->g.c = 0;
	      for (j = otfg->f.index.from; j <= otfg->f.index.to; j++)
		if (in_glyphs[from + j].g.code == otfg->glyph_id)
		  {
		    g->g.c = in_glyphs[from + j].g.c;
		    break;
		  }
	      if (feature_idx)
		{
		  tag = features[feature_idx - 1].FeatureTag;
		  tag = PACK_OTF_TAG (tag);
		  g->g.internal = (g->g.internal & ~0x1FFFFFFF) | tag;
		}
	      for (j = otfg->f.index.from + 1; j <= otfg->f.index.to; j++)
		{
		  if (min_from > in_glyphs[from + j].g.from)
		    min_from = in_glyphs[from + j].g.from;
		  if (max_to < in_glyphs[from + j].g.to)
		    max_to = in_glyphs[from + j].g.to;
		}
	      if (g->g.code != otfg->glyph_id)
		{
		  g->g.code = otfg->glyph_id;
		  g->g.measured = 0;
		}
	      g->g.from = min_from, g->g.to = max_to;
	    }
	}
      else
	{
	  for (i = 0, otfg = otf_gstring.glyphs; i < otf_gstring.used;
	       i++, otfg++)
	    {
	      int feature_idx;

#ifdef OTF_POSITIONING_TYPE_GET_FORMAT
	      feature_idx = OTF_POSITIONING_TYPE_GET_FEATURE (otfg);
#else
	      feature_idx = otfg->positioning_type >> 4;
#endif
	      if (feature_idx)
		{
		  tag = features[feature_idx - 1].FeatureTag;
		  tag = PACK_OTF_TAG (tag);
		  for (j = otfg->f.index.from; j <= otfg->f.index.to; j++)
		    {
		      g = in_glyphs + (from + j);
		      g->g.internal = (g->g.internal & ~0x1FFFFFFF) | tag;
		    }
		}
	    }
	}
    }
  else if (out)
    {
      if (out->allocated < gidx + len)
	return -2;
      for (i = 0; i < len; i++)
	out_glyphs[out->used++] = in_glyphs[from + i];
    }

  if (gpos_features)
    {
      OTF_Feature *features;
      MGlyph *g;

#ifdef OTF_POSITIONING_TYPE_GET_FORMAT
      if (OTF_drive_gpos_features (otf, &otf_gstring, script, langsys,
				   gpos_features) < 0)
	return to;
#else
      if (OTF_drive_gpos_with_log (otf, &otf_gstring, script, langsys,
				   gpos_features) < 0)
	return to;
#endif
      features = otf->gpos->FeatureList.Feature;
      if (out)
	{
	  MGlyph *base = NULL, *mark = NULL;
	  int x_ppem = face->size->metrics.x_ppem;
	  int y_ppem = face->size->metrics.y_ppem;
	  int x_scale = face->size->metrics.x_scale;
	  int y_scale = face->size->metrics.y_scale;

	  for (i = j = 0, otfg = otf_gstring.glyphs, g = out_glyphs + gidx;
	       i < otf_gstring.used; i++, otfg++)
	    {
	      MGlyph *prev;
	      int adjust_idx = otfg->glyph_id ? j : j - 1;
	      int positioning_type, feature_idx;

#ifdef OTF_POSITIONING_TYPE_GET_FORMAT
	      positioning_type = OTF_POSITIONING_TYPE_GET_FORMAT (otfg);
	      feature_idx = OTF_POSITIONING_TYPE_GET_FEATURE (otfg);
#else
	      positioning_type = otfg->positioning_type & 0xF;
	      feature_idx = otfg->positioning_type >> 4;
#endif

	      if (feature_idx)
		{
		  tag = features[feature_idx - 1].FeatureTag;
		  tag = PACK_OTF_TAG (tag);
		  g->g.internal = (g->g.internal & ~0x1FFFFFFF) | tag;
		}

	      switch (positioning_type)
		{
		case 0:
		  break;
		case 1: 		/* Single */
		case 2: 		/* Pair */
		  {
		    int format = otfg->f.f1.format;

		    if (format & OTF_XPlacement)
		      adjustment[adjust_idx].xoff
			+= otfg->f.f1.value->XPlacement * x_scale / 0x10000;
		    if (format & OTF_XPlaDevice)
		      adjustment[adjust_idx].xoff
			+= DEVICE_DELTA (otfg->f.f1.value->XPlaDevice, x_ppem);
		    if (format & OTF_YPlacement)
		      adjustment[adjust_idx].yoff
			-= otfg->f.f1.value->YPlacement * y_scale / 0x10000;
		    if (format & OTF_YPlaDevice)
		      adjustment[adjust_idx].yoff
			-= DEVICE_DELTA (otfg->f.f1.value->YPlaDevice, y_ppem);
		    if (format & OTF_XAdvance)
		      adjustment[adjust_idx].xadv
			+= otfg->f.f1.value->XAdvance * x_scale / 0x10000;
		    if (format & OTF_XAdvDevice)
		      adjustment[adjust_idx].xadv
			+= DEVICE_DELTA (otfg->f.f1.value->XAdvDevice, x_ppem);
		    if (format & OTF_YAdvance)
		      adjustment[adjust_idx].yadv
			+= otfg->f.f1.value->YAdvance * y_scale / 0x10000;
		    if (format & OTF_YAdvDevice)
		      adjustment[adjust_idx].yadv
			+= DEVICE_DELTA (otfg->f.f1.value->YAdvDevice, y_ppem);
		    adjustment[adjust_idx].set = 1;
		  }
		  break;
		case 3:		/* Cursive */
		  /* Not yet supported.  */
		  break;
		case 4:		/* Mark-to-Base */
		case 5:		/* Mark-to-Ligature */
		  if (! base)
		    break;
		  prev = base;
		  goto label_adjust_anchor;
		default:		/* i.e. case 6 Mark-to-Mark */
		  if (! mark)
		    break;
		  prev = mark;
#ifdef OTF_POSITIONING_TYPE_GET_FORMAT
		  {
		    int distance = OTF_POSITIONING_TYPE_GET_MARKDISTANCE (otfg);

		    if (distance > 0)
		      {
			prev = g - distance;
			if (prev < out_glyphs)
			  prev = mark;
		      }
		  }
#endif
		label_adjust_anchor:
		  {
		    int base_x, base_y, mark_x, mark_y;
		    int this_from, this_to;
		    int k;

		    base_x = otfg->f.f4.base_anchor->XCoordinate * x_scale / 0x10000;
		    base_y = otfg->f.f4.base_anchor->YCoordinate * y_scale / 0x10000;
		    mark_x = otfg->f.f4.mark_anchor->XCoordinate * x_scale / 0x10000;
		    mark_y = otfg->f.f4.mark_anchor->YCoordinate * y_scale / 0x10000;;

		    if (otfg->f.f4.base_anchor->AnchorFormat != 1)
		      adjust_anchor (otfg->f.f4.base_anchor, face, prev->g.code,
				     x_ppem, y_ppem, &base_x, &base_y);
		    if (otfg->f.f4.mark_anchor->AnchorFormat != 1)
		      adjust_anchor (otfg->f.f4.mark_anchor, face, g->g.code,
				     x_ppem, y_ppem, &mark_x, &mark_y);
		    adjustment[adjust_idx].xoff = base_x - mark_x;
		    adjustment[adjust_idx].yoff = - (base_y - mark_y);
		    adjustment[adjust_idx].back = (g - prev);
		    adjustment[adjust_idx].xadv = 0;
		    adjustment[adjust_idx].advance_is_absolute = 1;
		    adjustment[adjust_idx].set = 1;
		    this_from = g->g.from;
		    this_to = g->g.to;
		    for (k = 0; prev + k < g; k++)
		      {
			if (this_from > prev[k].g.from)
			  this_from = prev[k].g.from;
			if (this_to < prev[k].g.to)
			  this_to = prev[k].g.to;
		      }
		    for (; prev <= g; prev++)
		      {
			prev->g.from = this_from;
			prev->g.to = this_to;
		      }
		  }
		}
	      if (otfg->glyph_id)
		{
		  if (otfg->GlyphClass == OTF_GlyphClass0)
		    base = mark = g;
		  else if (otfg->GlyphClass == OTF_GlyphClassMark)
		    mark = g;
		  else
		    base = g;
		  j++, g++;
		}
	    }
	}
      else
	{
	  for (i = 0, otfg = otf_gstring.glyphs; i < otf_gstring.used;
	       i++, otfg++)
	    if (otfg->positioning_type & 0xF)
	      {
		int feature_idx;

#ifdef OTF_POSITIONING_TYPE_GET_FORMAT
		feature_idx = OTF_POSITIONING_TYPE_GET_FEATURE (otfg);
#else
		feature_idx = otfg->positioning_type >> 4;
#endif
		if (feature_idx)
		  {
		    tag = features[feature_idx - 1].FeatureTag;
		    tag = PACK_OTF_TAG (tag);
		    for (j = otfg->f.index.from; j <= otfg->f.index.to; j++)
		      {
			g = in_glyphs + (from + j);
			g->g.internal = (g->g.internal & ~0x1FFFFFFF) | tag;
		      }
		  }
	      }
	}
    }

  free (otf_gstring.glyphs);
  return to;

 simple_copy:
  if (otf_gstring.glyphs)
    free (otf_gstring.glyphs);
  if (out)
    {
      if (out->allocated < out->used + len)
	return -2;
      font->get_metrics (font, in, from, to);
      memcpy ((MGlyph *)out->glyphs + out->used, (MGlyph *) in->glyphs + from,
	      sizeof (MGlyph) * len);
      out->used += len;
    }
  return to;
}

#else  /* not HAVE_OTF */

static int 
ft_drive_otf (MFLTFont *font, MFLTOtfSpec *spec,
	      MFLTGlyphString *in, int from, int to,
	      MFLTGlyphString *out, MFLTGlyphAdjustment *adjustment)
{
  int len = to - from;
  if (out)
    {
      if (out->allocated < out->used + len)
	return -2;
      font->get_metrics (font, in, from, to);
      memcpy ((MGlyph *)out->glyphs + out->used, (MGlyph *) in->glyphs + from,
	      sizeof (MGlyph) * len);
      out->used += len;
    }
  return to;
}

#endif	/* not HAVE_OTF */


static int 
ft_try_otf (MFLTFont *font, MFLTOtfSpec *spec,
	    MFLTGlyphString *in, int from, int to)
{
  return ft_drive_otf (font, spec, in, from, to, NULL, NULL);
}


#ifdef HAVE_OTF
static unsigned char *iterate_bitmap;

static int
iterate_callback (OTF *otf, const char *feature, unsigned glyph_id)
{
  if (glyph_id <= otf->cmap->max_glyph_id)
    iterate_bitmap[glyph_id / 8] |= 1 << (glyph_id % 8);
  return 0;
}

static int
ft_iterate_otf_feature (MFLTFont *font, MFLTOtfSpec *spec,
			int from, int to, unsigned char *table)
{
  OTF *otf = get_otf (font, NULL);
  char id[13];
  int bmp_size;
  unsigned char *bitmap = NULL;
  int i, j;
  char script[5], *langsys = NULL;

  if (! otf)
    return -1;
  if (OTF_get_table (otf, "cmap") < 0)
    return -1;
  if (! spec->features[0])
    return -1;
  strcpy (id, "feature-");
  id[12] = '\0';
  OTF_tag_name (spec->script, script);
  if (spec->langsys)
    {
      langsys = alloca (5);
      OTF_tag_name (spec->langsys, langsys);
    }
  bmp_size = (otf->cmap->max_glyph_id / 8) + 1;
  for (i = 0; spec->features[0][i]; i++)
    {
      unsigned char *bmp;

      OTF_tag_name (spec->features[0][i], id + 8);
      bmp = OTF_get_data (otf, id);
      if (! bmp)
	{
	  iterate_bitmap = bmp = calloc (bmp_size, 1);
	  OTF_iterate_gsub_feature (otf, iterate_callback,
				    script, langsys, id + 8);
	  OTF_put_data (otf, id, bmp, free);
	}
      if (i == 0 && ! spec->features[0][1])
	/* Single feature */
	bitmap = bmp;
      else
	{
	  if (! bitmap)
	    {
	      bitmap = alloca (bmp_size);
	      memcpy (bitmap, bmp, bmp_size);
	    }
	  else
	    {
	      int j;

	      for (j = 0; j < bmp_size; j++)
		bitmap[j] &= bmp[j];
	    }
	}
    }
  for (i = 0; i < bmp_size; i++)
    if (bitmap[i])
      {
	for (j = 0; j < 8; j++)
	  if (bitmap[i] & (1 << j))
	    {
	      int c = OTF_get_unicode (otf, (i * 8) + j);

	      if (c >= from && c <= to)
		table[c - from] = 1;
	    }
      }
  return 0;
}
#endif



/* Internal API */

MFontDriver mfont__ft_driver =
  { ft_select, ft_open, ft_find_metric, ft_has_char, ft_encode_char,
    ft_render, ft_list, ft_list_family_names, ft_check_capability,
    ft_encapsulate, ft_close, ft_check_otf, ft_drive_otf, ft_try_otf,
#ifdef HAVE_OTF
    ft_iterate_otf_feature
#endif	/* HAVE_OTF */
  };

int
mfont__ft_init ()
{
  int i;

  if (FT_Init_FreeType (&ft_library) != 0)
    MERROR (MERROR_FONT_FT, -1);

  for (i = 0; i < ft_to_prop_size; i++)
    ft_to_prop[i].len = strlen (ft_to_prop[i].ft_style);

  Mmedium = msymbol ("medium");
  Mr = msymbol ("r");
  Mnull = msymbol ("");

  M0[0] = msymbol ("0-0");
  M0[1] = msymbol ("0-1");
  M0[2] = msymbol ("0-2");
  M0[3] = msymbol ("0-3");
  M0[4] = msymbol ("0-4");
  M3_1 = msymbol ("3-1");
  M1_0 = msymbol ("1-0");

#ifdef HAVE_FONTCONFIG
  for (i = 0; i < (sizeof (fc_all_table) / sizeof fc_all_table[0]); i++)
    {
      FC_vs_M17N_font_prop *table = fc_all_table[i];
      int j;

      for (j = 0; table[j].m17n_value; j++)
	table[j].sym = msymbol (table[j].m17n_value);
      table[j].sym = table[j - 1].sym;
    }

  {
    char *pathname;
    struct stat buf;
    MPlist *plist;
    MSymbol serif, sans_serif, monospace;

    fc_config = FcInitLoadConfigAndFonts ();
    if (mfont_freetype_path)
      {
	MPLIST_DO (plist, mfont_freetype_path)
	  if (MPLIST_STRING_P (plist)
	      && (pathname = MPLIST_STRING (plist))
	      && stat (pathname, &buf) == 0)
	    {
	      FcStrList *strlist = FcConfigGetFontDirs (fc_config);
	      FcChar8 *dir;

	      while ((dir = FcStrListNext (strlist)))
		if (strcmp ((char *) dir, pathname) == 0)
		  break;
	      if (! dir)
		FcConfigAppFontAddDir (fc_config, (FcChar8 *) pathname);
	      FcStrListDone (strlist);
	    }
      }
    Mgeneric_family = msymbol ("generic famly");
    serif = msymbol ("serif");
    msymbol_put (serif, Mgeneric_family, serif);
    sans_serif = msymbol ("sans-serif");
    msymbol_put (sans_serif, Mgeneric_family, sans_serif);
    msymbol_put (msymbol ("sans serif"), Mgeneric_family, sans_serif);
    msymbol_put (msymbol ("sans"), Mgeneric_family, sans_serif);
    monospace = msymbol ("monospace");
    msymbol_put (monospace, Mgeneric_family, monospace);
    msymbol_put (msymbol ("mono"), Mgeneric_family, monospace);
  }
#endif	/* HAVE_FONTCONFIG */

  return 0;
}

void
mfont__ft_fini ()
{
  MPlist *plist, *p;

  if (ft_default_list)
    {
      M17N_OBJECT_UNREF (ft_default_list);
      ft_default_list = NULL;
    }

  if (ft_font_list)
    {
      MPLIST_DO (plist, ft_font_list)
	{
	  if (MPLIST_VAL (plist))
	    MPLIST_DO (p, MPLIST_VAL (plist))
	      {
		if (MPLIST_KEY (p) != Mt)
		  free_ft_info (MPLIST_VAL (p));
	      }
	  M17N_OBJECT_UNREF (MPLIST_VAL (plist));
	}
      M17N_OBJECT_UNREF (ft_font_list);
      ft_font_list = NULL;

      if (ft_language_list)
	{
	  MPLIST_DO (plist, ft_language_list)
	    M17N_OBJECT_UNREF (MPLIST_VAL (plist));
	  M17N_OBJECT_UNREF (ft_language_list);
	  ft_language_list = NULL;
	}

      if (ft_script_list)
	{
	  MPLIST_DO (plist, ft_script_list)
	    M17N_OBJECT_UNREF (MPLIST_VAL (plist));
	  M17N_OBJECT_UNREF (ft_script_list);
	  ft_script_list = NULL;
	}

      if (ft_capability_list)
	{
	  MPLIST_DO (plist, ft_capability_list)
	    M17N_OBJECT_UNREF (MPLIST_VAL (plist));
	  M17N_OBJECT_UNREF (ft_capability_list);
	  ft_capability_list = NULL;
	}

      if (ft_file_list)
	{
	  MPLIST_DO (plist, ft_file_list)
	    M17N_OBJECT_UNREF (MPLIST_VAL (plist));
	  M17N_OBJECT_UNREF (ft_file_list);
	  ft_file_list = NULL;
	}
    }
  FT_Done_FreeType (ft_library);
#ifdef HAVE_FONTCONFIG
  FcConfigDestroy (fc_config);
  fc_config = NULL;
#endif	/* HAVE_FONTCONFIG */
  all_fonts_scaned = 0;
}

#ifdef HAVE_FONTCONFIG

int
mfont__ft_parse_name (const char *name, MFont *font)
{
  FcPattern *pat = FcNameParse ((FcChar8 *) name);
  FcChar8 *str;
  int val;
  double size;
  char *buf;
  int bufsize = 0;
  
  if (! pat)
    return -1;
  if (FcPatternGetString (pat, FC_FOUNDRY, 0, &str) == FcResultMatch)
    {
      STRDUP_LOWER (buf, bufsize, (char *) str);
      mfont__set_property (font, MFONT_FOUNDRY, msymbol (buf));
    }
  if (FcPatternGetString (pat, FC_FAMILY, 0, &str) == FcResultMatch)
    {
      STRDUP_LOWER (buf, bufsize, (char *) str);
      mfont__set_property (font, MFONT_FAMILY, msymbol (buf));
    }
  if (FcPatternGetInteger (pat, FC_WEIGHT, 0, &val) == FcResultMatch)
    mfont__set_property (font, MFONT_WEIGHT,
			 fc_decode_prop (val, fc_weight_table,
					 fc_weight_table_size));
  if (FcPatternGetInteger (pat, FC_SLANT, 0, &val) == FcResultMatch)
    mfont__set_property (font, MFONT_STYLE,
			 fc_decode_prop (val, fc_slant_table,
					 fc_slant_table_size));
  if (FcPatternGetInteger (pat, FC_WIDTH, 0, &val) == FcResultMatch)
    mfont__set_property (font, MFONT_STRETCH,
			 fc_decode_prop (val, fc_width_table,
					 fc_width_table_size));
  if (FcPatternGetDouble (pat, FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
    font->size = size * 10 + 0.5;
  else if (FcPatternGetDouble (pat, FC_SIZE, 0, &size) == FcResultMatch)
    font->size = - (size * 10 + 0.5);
  if (FcPatternGetString (pat, FC_FILE, 0, &str) == FcResultMatch)
    {
      font->file = msymbol ((char *) str);
    }
  mfont__set_property (font, MFONT_REGISTRY, Municode_bmp);
  font->type = MFONT_TYPE_SPEC;
  FcPatternDestroy (pat);
  return 0;
}

char *
mfont__ft_unparse_name (MFont *font)
{
  FcPattern *pat = fc_get_pattern (font);
  char *name = (char *) FcNameUnparse (pat);

  FcPatternDestroy (pat);
  return name;
}
#endif	/* HAVE_FONTCONFIG */

#endif /* HAVE_FREETYPE */
