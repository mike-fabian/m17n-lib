/* font-ft.c -- FreeType interface sub-module.
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
#include "internal-gui.h"
#include "font.h"
#include "face.h"

#ifdef HAVE_FREETYPE

#include <freetype/ftbdf.h>

#ifdef HAVE_FONTCONFIG
static FcConfig *fc_config;
#endif	/* HAVE_FONTCONFIG */

/* Registries.  */
static MSymbol Municode_bmp, Municode_full, Miso10646_1, Miso8859_1;

/* Font properties; Mnormal is already defined in face.c.  */
static MSymbol Mmedium, Mr, Mnull;

static FT_Library ft_library;

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
    { "oblique", 0, MFONT_STYLE, "p" },
    { "regular", 0, MFONT_WEIGHT, "medium" },
    { "normal", 0, MFONT_WEIGHT, "medium" },
    /* We need this entry even if "bold" is in commone_weight[] to
       handle such style names as "bolditalic" and "boldoblique".  */
    { "bold", 0, MFONT_WEIGHT, "bold" },
    { "demi bold", 0, MFONT_WEIGHT, "demibold" },
    { "demi", 0, MFONT_WEIGHT, "demibold" } };
static int ft_to_prop_size = sizeof ft_to_prop / sizeof ft_to_prop[0];

/** List of FreeType fonts.  Keys are family names, values are plists
    containing fonts of the corresponding family.  In the deeper
    plist, keys are Mt, values are (MFTInfo *).  */
static MPlist *ft_font_list;

/** List of FreeType base fonts.  Keys are family names, values are
    (MFTInfo *).  */
static MPlist *ft_family_list;

static int all_fonts_scaned;

static MSymbol M_generic_family_info;

enum GenericFamilyType {
  GENERIC_FAMILY_SERIF,
  GENERIC_FAMILY_SANS_SERIF,
  GENERIC_FAMILY_MONOSPACE,
  GENERIC_FAMILY_MAX
};

/** Table for each generic family.  */

typedef struct
{
  char *name;
  MPlist *list;
} GenericFamilyInfo;

static GenericFamilyInfo generic_family_table[GENERIC_FAMILY_MAX] =
  { { "serif" }, { "sans-serif" }, { "monospace" } };

/** Return 0 if NAME implies TrueType or OpenType fonts.  Othersize
    return -1.  */

static int
check_otf_filename (const char *name)
{
  int len = strlen (name);
  const char *ext = name + (len - 4);

  if (len < 5
      || (memcmp (ext, ".ttf", 4)
	  && memcmp (ext, ".TTF", 4)
	  && memcmp (ext, ".otf", 4)
	  && memcmp (ext, ".OTF", 4)))
    return -1;
  return 0;
}

#define STRDUP_LOWER(s1, size, s2)				\
  do {								\
    int len = strlen (s2) + 1;					\
    char *p1, *p2;						\
								\
    if (size < len)						\
      s1 = alloca (len), size = len;				\
    for (p1 = s1, p2 = s2; *p2; p1++, p2++)			\
      *p1 = (*p2 >= 'A' && *p2 <= 'Z' ? *p2 + 'a' - 'A' : *p2);	\
    *p1 = *p2;							\
  } while (0)

/** Setup members of FT_INFO from FT_FACE.  If the font is a base one
    (i.e. medium-r-normal), set BASEP to 1.  Otherwise set BASEP to 0.
    Return the family name.  */

static MSymbol
set_font_info (FT_Face ft_face, MFTInfo *ft_info,
	       MSymbol family, MSymbol style, int *basep)
{
  MFont *font = &ft_info->font;
  MPlist *charmap_list;
  int unicode_bmp = -1, unicode_full = -1, unicode = -1;
  int i;

  MFONT_INIT (font);

  mfont__set_property (font, MFONT_FAMILY, family);
  mfont__set_property (font, MFONT_WEIGHT, Mmedium);
  mfont__set_property (font, MFONT_STYLE, Mr);
  mfont__set_property (font, MFONT_STRETCH, Mnormal);
  mfont__set_property (font, MFONT_ADSTYLE, Mnull);
  *basep = 1;

  if (style != Mnull)
    {
      char *p = MSYMBOL_NAME (style);

      while (*p)
	{
	  for (i = 0; i < ft_to_prop_size; i++)
	    if (! strncmp (ft_to_prop[i].ft_style, p, ft_to_prop[i].len))
	      {
		mfont__set_property (font, ft_to_prop[i].prop,
				     msymbol (ft_to_prop[i].val));
		p += ft_to_prop[i].len;
		break;
	      }
	  if (i == ft_to_prop_size)
	    {
	      char *p1 = p + 1;
	      MSymbol sym;

	      while (*p1 >= 'a' && *p1 <= 'z') p1++;
	      sym = msymbol__with_len (p, p1 - p);
	      for (i = MFONT_WEIGHT; i <= MFONT_STYLE; i++)
		if (msymbol_get (sym, mfont__property_table[i].property))
		  {
		    mfont__set_property (font, i, sym);
		    break;
		  }
	      p = p1;
	    }
	  while (*p && (*p < 'a' || *p > 'z')) p++;
	}
      *basep = (FONT_PROPERTY (font, MFONT_WEIGHT) == Mmedium
		&& FONT_PROPERTY (font, MFONT_STYLE) == Mr
		&& FONT_PROPERTY (font, MFONT_STRETCH) == Mnormal);
    }

  charmap_list = mplist ();
  mplist_add (charmap_list, Mt, (void *) -1);
  for (i = 0; i < ft_face->num_charmaps; i++)
    {
      char registry_buf[16];
      MSymbol registry;

      sprintf (registry_buf, "%d-%d",
	       ft_face->charmaps[i]->platform_id,
	       ft_face->charmaps[i]->encoding_id);
      registry = msymbol (registry_buf);
      mplist_add (charmap_list, registry, (void *) i);
      
      if (ft_face->charmaps[i]->platform_id == 0)
	{
	  if (ft_face->charmaps[i]->encoding_id == 3)
	    unicode_bmp = i;
	  else if (ft_face->charmaps[i]->encoding_id == 4)
	    unicode_full = i;
	}
      else if (ft_face->charmaps[i]->platform_id == 3)
	{
	  if (ft_face->charmaps[i]->encoding_id == 1)
	    unicode_bmp = i;
	  else if (ft_face->charmaps[i]->encoding_id == 10)
	    unicode_full = i;
	}
      else if (ft_face->charmaps[i]->platform_id == 1
	       && ft_face->charmaps[i]->encoding_id == 0)
	mplist_add (charmap_list, msymbol ("apple-roman"), (void *) i);
    }
  if (unicode_full >= 0)
    {
      mplist_add (charmap_list, Municode_full, (void *) unicode_full);
      mplist_add (charmap_list, Municode_bmp, (void *) unicode_full);
      mplist_add (charmap_list, Miso10646_1, (void *) unicode_full);
      unicode = unicode_full;
    }
  else if (unicode_bmp >= 0)
    {
      mplist_add (charmap_list, Municode_bmp, (void *) unicode_bmp);
      mplist_add (charmap_list, Miso10646_1, (void *) unicode_bmp);
      unicode = unicode_bmp;
    }
  if (unicode >= 0)
    {
      FT_Set_Charmap (ft_face, ft_face->charmaps[unicode]);
      for (i = 255; i >= 32; i--)
	{
	  if (i == 192)
	    i = 126;
	  if (FT_Get_Char_Index (ft_face, (FT_ULong) i) == 0)
	    break;
	}
      if (i == 31)
	mplist_add (charmap_list, Miso8859_1, (void *) unicode);
    }

  ft_info->charmap_list = charmap_list;

  if (! FT_IS_SCALABLE (ft_face))
    {
      BDF_PropertyRec prop;
      
      FT_Get_BDF_Property (ft_face, "PIXEL_SIZE", &prop);
      font->property[MFONT_SIZE] = prop.u.integer * 10;
      FT_Get_BDF_Property (ft_face, "RESOLUTION_Y", &prop);
      font->property[MFONT_RESY] = prop.u.integer;
    }

  return family;
}


static void
close_ft (void *object)
{
  MFTInfo *ft_info = object;

  if (ft_info->ft_face)
    {
      if (ft_info->extra_info)
	M17N_OBJECT_UNREF (ft_info->extra_info);
      FT_Done_Face (ft_info->ft_face);
#ifdef HAVE_OTF
      if (ft_info->otf)
	OTF_close (ft_info->otf);
#endif /* HAVE_OTF */
    }
  free (ft_info->filename);
  if (ft_info->languages)
    free (ft_info->languages);
  M17N_OBJECT_UNREF (ft_info->charmap_list);
  free (ft_info);
}

static void
add_font_info (char *filename, MSymbol family, void *langset, MPlist *plist)
{
  FT_Face ft_face;
  BDF_PropertyRec prop;

  if (FT_New_Face (ft_library, filename, 0, &ft_face) == 0)
    {
      char *buf;
      int bufsize = 0;
      MSymbol style;

      if (family == Mnil)
	{
	  if (ft_face->family_name)
	    {
	      STRDUP_LOWER (buf, bufsize, ft_face->family_name);
	      family = msymbol (buf);
	    }
	  else
	    family = Mnull;
	  if (! (plist = mplist_get (ft_font_list, family)))
	    {
	      plist = mplist ();
	      mplist_add (ft_font_list, family, plist);
	    }
	}
      if (ft_face->style_name)
	{
	  STRDUP_LOWER (buf, bufsize, ft_face->style_name);
	  style = msymbol (buf);
	}
      else
	style = Mnull;

      if (! mplist_get (plist, style)
	  && (FT_IS_SCALABLE (ft_face)
	      || FT_Get_BDF_Property (ft_face, "PIXEL_SIZE", &prop) == 0))
	{
	  int basep;
	  MFTInfo *ft_info;

	  M17N_OBJECT (ft_info, close_ft, MERROR_FONT_FT);
	  ft_info->filename = strdup (filename);
	  ft_info->otf_flag = check_otf_filename (filename);
#ifdef HAVE_FONTCONFIG
	  if (langset)
	    ft_info->langset = FcLangSetCopy (langset);
#endif
	  set_font_info (ft_face, ft_info, family, style, &basep);
	  mplist_add (plist, style, ft_info);

	  if (basep)
	    mplist_put (ft_family_list, family, ft_info);
	  else if (! mplist_get (ft_family_list, family))
	    mplist_add (ft_family_list, family, ft_info);
	}
      FT_Done_Face (ft_face);
    }
}

/* Return an element of ft_font_list for FAMILY.  If FAMILY is Mnil,
   scan all fonts and return ft_font_list.  */
static MPlist *
ft_list_family (MSymbol family)
{
  MPlist *plist, *head;

  if (! ft_font_list)
    {
      ft_font_list = mplist ();
      ft_family_list = mplist ();
    }

  if (family == Mnil)
    head = ft_font_list;
  else
    {
      head = mplist_find_by_key (ft_font_list, family);
      if (head)
	return head;
      head = mplist_add (ft_font_list, family, mplist ());
    }

#ifdef HAVE_FONTCONFIG
  if (! all_fonts_scaned)
    {
      FcPattern *pattern;
      FcObjectSet *os;
      FcFontSet *fs;
      char *buf;
      int bufsize = 0;
      int i;

      pattern = FcPatternCreate ();
      if (family != Mnil)
	{
	  FcPatternAddString (pattern, FC_FAMILY,
			      (FcChar8 *) (msymbol_name (family)));
	  plist = head;
	  os = FcObjectSetBuild (FC_FILE, FC_LANG, NULL);
	}
      else
	{
	  plist = NULL;
	  os = FcObjectSetBuild (FC_FILE, FC_FAMILY, FC_LANG, NULL);
	}
      fs = FcFontList (fc_config, pattern, os);
      for (i = 0; i < fs->nfont; i++)
	{
	  char *filename;
	  FcLangSet *langset;

	  if (FcPatternGetString (fs->fonts[i], FC_FILE, 0,
				  (FcChar8 **) &filename) != FcResultMatch)
	    continue;
	  if (FcPatternGetLangSet (fs->fonts[i], FC_LANG, 0,
				   &langset) != FcResultMatch)
	    langset = NULL;
	  if (family == Mnil)
	    {
	      MSymbol fam;
	      char *fname;

	      if (FcPatternGetString (fs->fonts[i], FC_FAMILY, 0,
				      (FcChar8 **) &fname) == FcResultMatch)
		{
		  STRDUP_LOWER (buf, bufsize, fname);
		  fam = msymbol (buf);
		  if (! plist || MPLIST_KEY (plist) != fam)
		    {
		      plist = mplist_find_by_key (ft_font_list, fam);
		      if (! plist)
			plist = mplist_add (ft_font_list, fam, mplist ());
		    }
		  add_font_info (filename, fam, langset, MPLIST_PLIST (plist));
		}
	    }
	  else
	    add_font_info (filename, family, langset, MPLIST_PLIST (plist));
	}

      FcFontSetDestroy (fs);
      FcObjectSetDestroy (os);
      FcPatternDestroy (pattern);
      all_fonts_scaned = family == Mnil;
    }

#else  /* not HAVE_FONTCONFIG */

  if (! all_fonts_scaned)
    {
      struct stat buf;
      char *pathname;

      MPLIST_DO (plist, mfont_freetype_path)
	if (MPLIST_STRING_P (plist)
	    && (pathname = MPLIST_STRING (plist))
	    && stat (pathname, &buf) == 0)
	  {
	    if (S_ISREG (buf.st_mode))
	      add_font_info (pathname, Mnil, NULL, NULL);
	    else if (S_ISDIR (buf.st_mode))
	      {
		int len = strlen (pathname);
		char path[PATH_MAX];
		DIR *dir = opendir (pathname);
		struct dirent *dp;

		if (dir)
		  {
		    strcpy (path, pathname);
		    strcpy (path + len, "/");
		    len++;
		    while ((dp = readdir (dir)) != NULL)
		      {
			strcpy (path + len, dp->d_name);
			add_font_info (path, Mnil, NULL, NULL);
		      }
		    closedir (dir);
		  }
	      }
	  }
      all_fonts_scaned = 1;
    }

#endif  /* not HAVE_FONTCONFIG */

  return head;
}

static MPlist *
ft_list_generic (MSymbol generic)
{
#ifdef HAVE_FONTCONFIG
  GenericFamilyInfo *info = msymbol_get (generic, M_generic_family_info);
  FcPattern *pattern;

  if (! info)
    return NULL;
  if (info->list)
    return info->list;

  info->list = mplist ();
  pattern = FcPatternCreate ();
  FcPatternAddString (pattern, FC_FAMILY,
		      (FcChar8 *) info->name);
  if (FcConfigSubstitute (fc_config, pattern, FcMatchPattern) == FcTrue)
    {
      int i = 0;
      char *family, *buf;
      int bufsize = 0;
    
      while (FcPatternGetString (pattern, FC_FAMILY, i, (FcChar8 **) &family)
	     == FcResultMatch)
	{
	  MPlist *plist;

	  STRDUP_LOWER (buf, bufsize, family);
	  plist = ft_list_family (msymbol (buf));
	  mplist_add (info->list, MPLIST_KEY (plist), MPLIST_VAL (plist));
	  i++;
	}
    }
  return info->list;
#else   /* not HAVE_FONTCONFIG */
  return NULL;
#endif	/* not HAVE_FONTCONFIG */
}


/* The FreeType font driver function SELECT.  */

static MRealizedFont *
ft_select (MFrame *frame, MFont *spec, MFont *request, int limited_size)
{
  MPlist *plist, *pl, *p;
  MFTInfo *best_font;
  int best_score;
  MRealizedFont *rfont;
  MSymbol registry, family;
  unsigned short spec_family_id, request_family_id;

  registry = FONT_PROPERTY (spec, MFONT_REGISTRY);
  if (registry == Mnil)
    registry = Mt;
  family = FONT_PROPERTY (spec, MFONT_FAMILY);
  spec_family_id = spec->property[MFONT_FAMILY];
  request_family_id = request->property[MFONT_FAMILY];

  if (spec_family_id)
    {
      plist = ft_list_generic (family);
      if (plist)
	{
	  family = Mnil;
	  spec->property[MFONT_FAMILY] = 0;
	  if (spec_family_id == request_family_id)
	    request->property[MFONT_FAMILY] = 0;
	}
      else
	{
	  if (request_family_id
	      && (plist
		  = ft_list_generic (FONT_PROPERTY (request, MFONT_FAMILY)))
	      && mplist_get (plist, family))
	    request->property[MFONT_FAMILY] = 0;
	  plist = ft_list_family (family);
	}
    }
  else
    {
      if (request_family_id
	  && (plist = ft_list_generic (FONT_PROPERTY (request, MFONT_FAMILY))))
	request->property[MFONT_FAMILY] = 0;
      else
	plist = ft_list_family (FONT_PROPERTY (request, MFONT_FAMILY));
    }

  best_font = NULL;
  best_score = -1;
  MPLIST_DO (pl, plist)
    {
      MPLIST_DO (p, MPLIST_VAL (pl))
	{
	  MFTInfo *ft_info = MPLIST_VAL (p);
	  int score;

	  if (! mplist_find_by_key (ft_info->charmap_list, registry))
	    continue;

	  /* Always ignore FOUNDRY.  */
	  ft_info->font.property[MFONT_FOUNDRY] = spec->property[MFONT_FOUNDRY];
	  score = mfont__score (&ft_info->font, spec, request, limited_size);
	  if (score >= 0
	      && (! best_font
		  || best_score > score))
	    {
	      best_font = ft_info;
	      best_score = score;
	      if (best_score == 0)
		break;
	    }
	}
      if (best_score == 0 || family != Mnil)
	break;
    }
  spec->property[MFONT_FAMILY] = spec_family_id;
  request->property[MFONT_FAMILY] = request_family_id;
  if (! best_font)
    return NULL;

  MSTRUCT_CALLOC (rfont, MERROR_FONT_FT);
  rfont->frame = frame;
  rfont->spec = *spec;
  rfont->request = *request;
  rfont->font = best_font->font;
  rfont->font.property[MFONT_SIZE] = request->property[MFONT_SIZE];
  rfont->font.property[MFONT_REGISTRY] = spec->property[MFONT_REGISTRY];
  rfont->score = best_score;
  rfont->info = best_font;
  M17N_OBJECT_REF (best_font);
  return rfont;
}


/* The FreeType font driver function OPEN.  */

static int
ft_open (MRealizedFont *rfont)
{
  MFTInfo *base = rfont->info, *ft_info;
  MSymbol registry = FONT_PROPERTY (&rfont->font, MFONT_REGISTRY);
  int mdebug_mask = MDEBUG_FONT;
  int size;

  M17N_OBJECT (ft_info, close_ft, MERROR_FONT_FT);
  ft_info->font = base->font;
  ft_info->filename = strdup (base->filename);
  ft_info->otf_flag = base->otf_flag;
  ft_info->charmap_list = base->charmap_list;
  M17N_OBJECT_REF (ft_info->charmap_list);
  M17N_OBJECT_UNREF (base);
  rfont->info = ft_info;

  rfont->status = -1;
  ft_info->ft_face = NULL;
  if (FT_New_Face (ft_library, ft_info->filename, 0, &ft_info->ft_face))
    goto err;
  if (registry == Mnil)
    registry = Mt;
  ft_info->charmap_index
    = (int) mplist_get (((MFTInfo *) rfont->info)->charmap_list, registry);
  if (ft_info->charmap_index >= 0
      && FT_Set_Charmap (ft_info->ft_face, 
			 ft_info->ft_face->charmaps[ft_info->charmap_index]))
    goto err;
  size = rfont->font.property[MFONT_SIZE] / 10;
  if (FT_Set_Pixel_Sizes (ft_info->ft_face, 0, size))
    goto err;

  MDEBUG_PRINT1 (" [FT-FONT] o %s\n", ft_info->filename);
  rfont->status = 1;
  rfont->ascent = ft_info->ft_face->size->metrics.ascender >> 6;
  rfont->descent = - (ft_info->ft_face->size->metrics.descender >> 6);
  rfont->type = Mfreetype;
  rfont->fontp = ft_info->ft_face;
  return 0;

 err:
  MDEBUG_PRINT1 (" [FT-FONT] x %s\n", ft_info->filename);
  if (ft_info->ft_face)
    FT_Done_Face (ft_info->ft_face);
  M17N_OBJECT_UNREF (ft_info->charmap_list);
  free (ft_info->filename);
  free (ft_info);
  rfont->info = NULL;
  return -1;
}

/* The FreeType font driver function FIND_METRIC.  */

static void
ft_find_metric (MRealizedFont *rfont, MGlyphString *gstring,
		int from, int to)
{
  MFTInfo *ft_info = (MFTInfo *) rfont->info;
  FT_Face ft_face = ft_info->ft_face;
  MGlyph *g = MGLYPH (from), *gend = MGLYPH (to);

  for (; g != gend; g++)
    {
      if (g->code == MCHAR_INVALID_CODE)
	{
	  if (FT_IS_SCALABLE (ft_face))
	    {
	      unsigned unitsPerEm = ft_face->units_per_EM;
	      int size = rfont->font.property[MFONT_SIZE] / 10;

	      g->lbearing = 0;
	      g->rbearing = ft_face->max_advance_width * size / unitsPerEm;
	      g->width = ft_face->max_advance_width * size / unitsPerEm;
	      g->ascent = ft_face->ascender * size / unitsPerEm;
	      g->descent = (- ft_face->descender) * size / unitsPerEm;
	    }
	  else
	    {
	      BDF_PropertyRec prop;

	      g->lbearing = 0;
	      g->rbearing = g->width = ft_face->available_sizes->width;
	      if (FT_Get_BDF_Property (ft_face, "ASCENT", &prop) == 0)
		{
		  g->ascent = prop.u.integer;
		  FT_Get_BDF_Property (ft_face, "DESCENT", &prop);
		  g->descent = prop.u.integer;
		}
	      else
		{
		  g->ascent = ft_face->available_sizes->height;
		  g->descent = 0;
		}
	    }
	}
      else
	{
	  FT_Glyph_Metrics *metrics;

	  FT_Load_Glyph (ft_face, (FT_UInt) g->code, FT_LOAD_DEFAULT);
	  metrics = &ft_face->glyph->metrics;
	  g->lbearing = (metrics->horiBearingX >> 6);
	  g->rbearing = (metrics->horiBearingX + metrics->width) >> 6;
	  g->width = metrics->horiAdvance >> 6;
	  g->ascent = metrics->horiBearingY >> 6;
	  g->descent = (metrics->height - metrics->horiBearingY) >> 6;
	}
    }
}

/* The FreeType font driver function ENCODE_CHAR.  */

static unsigned
ft_encode_char (MRealizedFont *rfont, unsigned code)
{
  MFTInfo *ft_info;

  if (rfont->status == 0)
    {
      if ((rfont->driver->open) (rfont) < 0)
	return -1;
    }
  ft_info = (MFTInfo *) rfont->info;
  code = (unsigned) FT_Get_Char_Index (ft_info->ft_face, (FT_ULong) code);
  if (! code)
    return MCHAR_INVALID_CODE;
  return (code);

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
  MFTInfo *ft_info;
  FT_Face ft_face;
  MRealizedFace *rface = from->rface;
  MFrame *frame = rface->frame;
  FT_Int32 load_flags = FT_LOAD_RENDER;
  MGlyph *g;
  int i, j;
  MPointTable point_table[8];

  if (from == to)
    return;

  /* It is assured that the all glyphs in the current range use the
     same realized face.  */
  ft_info = (MFTInfo *) rface->rfont->info;
  ft_face = ft_info->ft_face;

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

  for (g = from; g < to; x += g++->width)
    {
      unsigned char *bmp;
      int intensity;
      MPointTable *ptable;
      int xoff, yoff;
      int width, pitch;

      FT_Load_Glyph (ft_face, (FT_UInt) g->code, load_flags);
      yoff = y - ft_face->glyph->bitmap_top + g->yoff;
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
	    xoff = x + ft_face->glyph->bitmap_left + g->xoff;
	    for (j = 0; j < width; j++, xoff++)
	      {
		intensity = bmp[j] >> 5;
		if (intensity)
		  {
		    ptable = point_table + intensity;
		    ptable->p->x = xoff;
		    ptable->p->y = yoff;
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
	    xoff = x + ft_face->glyph->bitmap_left + g->xoff;
	    for (j = 0; j < width; j++, xoff++)
	      {
		intensity = bmp[j / 8] & (1 << (7 - (j % 8)));
		if (intensity)
		  {
		    ptable = point_table;
		    ptable->p->x = xoff;
		    ptable->p->y = yoff;
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

  if (gstring->anti_alias)
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
ft_list (MFrame *frame, MPlist *plist, MFont *font, MSymbol language,
	 int maxnum)
{
  MPlist *pl, *p;
#ifdef HAVE_FONTCONFIG
  FcChar8 *lang = (language != Mnil ? (FcChar8 *) MSYMBOL_NAME (language)
		   : NULL);
#endif
  int num = 0;

  if (font)
    {
      MSymbol family = FONT_PROPERTY (font, MFONT_FAMILY);
      pl = ft_list_generic (family);
      if (pl)
	family = Mnil;
      else
	pl = ft_list_family (family);
      MPLIST_DO (pl, pl)
	{
	  MPLIST_DO (p, MPLIST_PLIST (pl))
	    {
	      MFTInfo *ft_info = MPLIST_VAL (p);

#ifdef HAVE_FONTCONFIG
	      if (lang && ft_info->langset
		  && FcLangSetHasLang (ft_info->langset, lang) != FcLangEqual)
		continue;
#endif
	      mplist_add (plist, MPLIST_KEY (pl), &ft_info->font);
	      num++;
	      if (num == maxnum)
		return num;
	    }
	  if (family != Mnil)
	    break;
	}
    }
  else
    {
      ft_list_family (Mnil);
      MPLIST_DO (p, ft_family_list)
	{
	  MFTInfo *ft_info = MPLIST_VAL (p);

#ifdef HAVE_FONTCONFIG
	  if (lang && ft_info->langset
	      && FcLangSetHasLang (ft_info->langset, lang) != FcLangEqual)
	    continue;
#endif
	  mplist_add (plist, MPLIST_KEY (p), &ft_info->font);
	  num++;
	  if (num == maxnum)
	    break;
	}
    }
  return num;
}


/* Internal API */

MFontDriver mfont__ft_driver =
  { ft_select, ft_open, ft_find_metric, ft_encode_char, ft_render, ft_list };

int
mfont__ft_init ()
{
  int i;

  if (FT_Init_FreeType (&ft_library) != 0)
    MERROR (MERROR_FONT_FT, -1);

  for (i = 0; i < ft_to_prop_size; i++)
    ft_to_prop[i].len = strlen (ft_to_prop[i].ft_style);

  Municode_bmp = msymbol ("unicode-bmp");
  Municode_full = msymbol ("unicode-full");
  Miso10646_1 = msymbol ("iso10646-1");
  Miso8859_1 = msymbol ("iso8859-1");

  Mmedium = msymbol ("medium");
  Mr = msymbol ("r");
  Mnull = msymbol ("");

  for (i = 0; i < GENERIC_FAMILY_MAX; i++)
    generic_family_table[i].list = NULL;
  M_generic_family_info = msymbol ("  generic_family_info");
  msymbol_put (msymbol ("serif"), M_generic_family_info,
	       generic_family_table + GENERIC_FAMILY_SERIF);
  msymbol_put (msymbol ("sans-serif"), M_generic_family_info,
	       generic_family_table + GENERIC_FAMILY_SANS_SERIF);
  msymbol_put (msymbol ("sans"), M_generic_family_info,
	       generic_family_table + GENERIC_FAMILY_SANS_SERIF);
  msymbol_put (msymbol ("sans serif"), M_generic_family_info,
	       generic_family_table + GENERIC_FAMILY_SANS_SERIF);
  msymbol_put (msymbol ("monospace"), M_generic_family_info,
	       generic_family_table + GENERIC_FAMILY_MONOSPACE);
  msymbol_put (msymbol ("mono"), M_generic_family_info,
	       generic_family_table + GENERIC_FAMILY_MONOSPACE);
  msymbol_put (msymbol ("m"), M_generic_family_info,
	       generic_family_table + GENERIC_FAMILY_MONOSPACE);

#ifdef HAVE_FONTCONFIG
  if (! fc_config)
    {
      char *pathname;
      struct stat buf;
      MPlist *plist;

      FcInit ();
      fc_config = FcConfigGetCurrent ();
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
#endif

  return 0;
}

void
mfont__ft_fini ()
{
  MPlist *plist, *p;
  int i;

  for (i = 0; i < GENERIC_FAMILY_MAX; i++)
    if (generic_family_table[i].list)
      M17N_OBJECT_UNREF (generic_family_table[i].list);
  if (ft_font_list)
    {
      MPLIST_DO (plist, ft_font_list)
	{
	  MPLIST_DO (p, MPLIST_VAL (plist))
	    {
	      MFTInfo *ft_info = MPLIST_VAL (p);

	      M17N_OBJECT_UNREF (ft_info);
	    }
	  M17N_OBJECT_UNREF (MPLIST_VAL (plist));
	}
      M17N_OBJECT_UNREF (ft_font_list);
      ft_font_list = NULL;

      M17N_OBJECT_UNREF (ft_family_list);
      ft_family_list = NULL;
    }
  FT_Done_FreeType (ft_library);
  all_fonts_scaned = 0;
}


#ifdef HAVE_FONTCONFIG
typedef struct
{
  int fc_value;
  char *m17n_value;
} FC_vs_M17N_font_prop;

static FC_vs_M17N_font_prop fc_weight_table[] =
  { { FC_WEIGHT_ULTRALIGHT, "extralight" },
    { FC_WEIGHT_LIGHT, "light" },
    { FC_WEIGHT_NORMAL, "normal" },
    { FC_WEIGHT_MEDIUM, "medium" },
    { FC_WEIGHT_DEMIBOLD, "demibold" },
    { FC_WEIGHT_EXTRABOLD, "extrabold" },
    { FC_WEIGHT_BLACK, "black" },
    { FC_WEIGHT_MEDIUM, NULL } };

static FC_vs_M17N_font_prop fc_slant_table[] =
  { { FC_SLANT_ROMAN, "r" },
    { FC_SLANT_ITALIC, "i" },
    { FC_SLANT_OBLIQUE, "o" },
    { FC_SLANT_ROMAN, NULL } };

static FC_vs_M17N_font_prop fc_width_table[] =
  { { FC_WIDTH_CONDENSED, "condensed" },
    { FC_WIDTH_SEMIEXPANDED, "semicondensed" },
    { FC_WIDTH_NORMAL, "normal" },
    { FC_WIDTH_SEMIEXPANDED, "semiexpanded" },
    { FC_WIDTH_EXPANDED, "expanded" },
    { FC_WIDTH_NORMAL, NULL } };


static MSymbol
fc_decode_prop (int val, FC_vs_M17N_font_prop *table)
{
  int i;

  for (i = 0; table[i].m17n_value; i++)
    if (val <= table[i].fc_value)
      return msymbol ("table[i].m17n_value");
  return msymbol ("table[i - 1].m17n_value");
}

static int
fc_encode_prop (char *name, FC_vs_M17N_font_prop *table)
{
  int i;

  for (i = 0; table[i].m17n_value && strcmp (name, table[i].m17n_value); i++);
  return table[i].fc_value;
}

int
mfont__ft_parse_name (char *name, MFont *font)
{
  FcPattern *pat = FcNameParse ((FcChar8 *) name);
  FcChar8 *str;
  int val;
  double size;
  
  if (! pat)
    return -1;
  if (FcPatternGetString (pat, FC_FOUNDRY, 0, &str) == FcResultMatch)
    mfont__set_property (font, MFONT_FOUNDRY, msymbol ((char *) str));
  if (FcPatternGetString (pat, FC_FAMILY, 0, &str) == FcResultMatch)
    mfont__set_property (font, MFONT_FAMILY, msymbol ((char *) str));
  if (FcPatternGetInteger (pat, FC_WEIGHT, 0, &val) == FcResultMatch)
    mfont__set_property (font, MFONT_WEIGHT,
			 fc_decode_prop (val, fc_weight_table));
  if (FcPatternGetInteger (pat, FC_SLANT, 0, &val) == FcResultMatch)
    mfont__set_property (font, MFONT_STYLE,
			 fc_decode_prop (val, fc_slant_table));
  if (FcPatternGetInteger (pat, FC_WIDTH, 0, &val) == FcResultMatch)
    mfont__set_property (font, MFONT_STRETCH,
			 fc_decode_prop (val, fc_width_table));
  if (FcPatternGetDouble (pat, FC_PIXEL_SIZE, 0, &size) == FcResultMatch)
    font->property[MFONT_SIZE] = size * 10;
  FcPatternDestroy (pat);
  return 0;
}

char *
mfont__ft_unparse_name (MFont *font)
{
  FcPattern *pat = FcPatternCreate ();
  MSymbol sym, weight, style, stretch;
  char *name;

  if ((sym = (MSymbol) FONT_PROPERTY (font, MFONT_FOUNDRY)) != Mnil)
    FcPatternAddString (pat, FC_FOUNDRY, (FcChar8 *) MSYMBOL_NAME (sym));
  if ((sym = (MSymbol) FONT_PROPERTY (font, MFONT_FAMILY)) != Mnil)
    FcPatternAddString (pat, FC_FAMILY, (FcChar8 *) MSYMBOL_NAME (sym));
  if ((weight = (MSymbol) FONT_PROPERTY (font, MFONT_WEIGHT)) != Mnil)
    FcPatternAddInteger (pat, FC_WEIGHT, fc_encode_prop (MSYMBOL_NAME (weight),
							 fc_weight_table));
  if ((style = (MSymbol) FONT_PROPERTY (font, MFONT_STYLE)) != Mnil)
    FcPatternAddInteger (pat, FC_SLANT, fc_encode_prop (MSYMBOL_NAME (style),
							fc_slant_table));
  if ((stretch = (MSymbol) FONT_PROPERTY (font, MFONT_STRETCH)) != Mnil)
    FcPatternAddInteger (pat, FC_WIDTH, fc_encode_prop (MSYMBOL_NAME (stretch),
							fc_width_table));
  name = (char *) FcNameUnparse (pat);
  FcPatternDestroy (pat);
  return name;
}
#endif	/* HAVE_FONTCONFIG */


#ifdef HAVE_OTF

#define DEVICE_DELTA(table, size)				\
  (((size) >= (table).StartSize && (size) <= (table).EndSize)	\
   ? (table).DeltaValue[(size) >= (table).StartSize]		\
   : 0)

void
adjust_anchor (OTF_Anchor *anchor, FT_Face ft_face,
	       unsigned code, int size, int *x, int *y)
{
  if (anchor->AnchorFormat == 2)
    {
      FT_Outline *outline;
      int ap = anchor->f.f1.AnchorPoint;

      FT_Load_Glyph (ft_face, (FT_UInt) code, FT_LOAD_MONOCHROME);
      outline = &ft_face->glyph->outline;
      if (ap < outline->n_points)
	{
	  *x = outline->points[ap].x;
	  *y = outline->points[ap].y;
	}
    }
  else if (anchor->AnchorFormat == 3)
    {
      *x += DEVICE_DELTA (anchor->f.f2.XDeviceTable, size);
      *y += DEVICE_DELTA (anchor->f.f2.YDeviceTable, size);
    }
}

int
mfont__ft_drive_otf (MGlyphString *gstring, int from, int to,
		     MSymbol script, MSymbol langsys,
		     MSymbol gsub_features, MSymbol gpos_features)
{
  int len = to - from;
  MGlyph *g = MGLYPH (from);
  int i, gidx;
  MRealizedFont *rfont;
  MFTInfo *ft_info;
  OTF *otf;
  OTF_GlyphString otf_gstring;
  OTF_Glyph *otfg;
  char *script_name, *language_name;
  char *gsub_feature_names, *gpos_feature_names;
  int need_cmap;

  if (len == 0)
    return from;

  rfont = g->rface->rfont;
  ft_info = rfont->info;
  if (ft_info->otf_flag < 0)
    goto simple_copy;
  otf = ft_info->otf;
  if (! otf)
    {
      otf = OTF_open (ft_info->filename);
      if (otf && OTF_get_table (otf, "head") < 0)
	{
	  OTF_close (otf);
	  otf = NULL;
	}
      if (! otf)
	{
	  ft_info->otf_flag = -1;
	  goto simple_copy;
	}
      ft_info->otf = otf;
    }

  if (script != Mnil)
    script_name = msymbol_name (script);
  else
    script_name = NULL;
  if (langsys != Mnil)
    language_name = msymbol_name (langsys);
  else
    language_name = NULL;
  gsub_feature_names
    = (gsub_features == Mt ? "*"
       : gsub_features == Mnil ? NULL
       : msymbol_name (gsub_features));
  if (gsub_feature_names && OTF_check_table (otf, "GSUB") < 0)
    gsub_feature_names = NULL;
  gpos_feature_names
    = (gpos_features == Mt ? "*"
       : gpos_features == Mnil ? NULL
       : msymbol_name (gpos_features));
  if (gpos_feature_names && OTF_check_table (otf, "GPOS") < 0)
    gpos_feature_names = NULL;

  otf_gstring.size = otf_gstring.used = len;
  otf_gstring.glyphs = (OTF_Glyph *) malloc (sizeof (OTF_Glyph) * len);
  memset (otf_gstring.glyphs, 0, sizeof (OTF_Glyph) * len);
  for (i = 0, need_cmap = 0; i < len; i++)
    {
      if (gstring->glyphs[from + i].otf_encoded)
	{
	  otf_gstring.glyphs[i].c = gstring->glyphs[from + i].c;
	  otf_gstring.glyphs[i].glyph_id = gstring->glyphs[from + i].code;
	}
      else
	{
	  otf_gstring.glyphs[i].c = gstring->glyphs[from + i].code;
	  need_cmap++;
	}
    }
  if (need_cmap
      && OTF_drive_cmap (otf, &otf_gstring) < 0)
    goto simple_copy;

  OTF_drive_gdef (otf, &otf_gstring);
  gidx = gstring->used;

  if (gsub_feature_names)
    {
      if (OTF_drive_gsub (otf, &otf_gstring, script_name, language_name,
			  gsub_feature_names) < 0)
	goto simple_copy;
      for (i = 0, otfg = otf_gstring.glyphs; i < otf_gstring.used; i++, otfg++)
	{
	  MGlyph temp = *(MGLYPH (from + otfg->f.index.from));

	  temp.c = otfg->c;
	  temp.combining_code = 0;
	  if (otfg->glyph_id)
	    {
	      temp.code = otfg->glyph_id;
	      temp.otf_encoded = 1;
	    }
	  else
	    {
	      temp.code = temp.c;
	      temp.otf_encoded = 0;
	    }
	  temp.to = MGLYPH (from + otfg->f.index.to)->to;
	  MLIST_APPEND1 (gstring, glyphs, temp, MERROR_FONT_OTF);
	}
    }
  else
    for (i = 0; i < len; i++)
      {
	MGlyph temp = gstring->glyphs[from + i];

	if (otf_gstring.glyphs[i].glyph_id)
	  {
	    temp.code = otf_gstring.glyphs[i].glyph_id;
	    temp.otf_encoded = 1;
	  }
	MLIST_APPEND1 (gstring, glyphs, temp, MERROR_FONT_OTF);
      }

  ft_find_metric (rfont, gstring, gidx, gstring->used);

  if (gpos_feature_names)
    {
      int u;
      int size10, size;
      MGlyph *base = NULL, *mark = NULL;

      if (OTF_drive_gpos (otf, &otf_gstring, script_name, language_name,
			  gpos_feature_names) < 0)
	return to;

      u = otf->head->unitsPerEm;
      size10 = rfont->font.property[MFONT_SIZE];
      size = size10 / 10;

      for (i = 0, otfg = otf_gstring.glyphs, g = MGLYPH (gidx);
	   i < otf_gstring.used; i++, otfg++, g++)
	{
	  MGlyph *prev;

	  if (! otfg->glyph_id)
	    continue;
	  switch (otfg->positioning_type)
	    {
	    case 0:
	      break;
	    case 1: case 2:
	      {
		int format = otfg->f.f1.format;

		if (format & OTF_XPlacement)
		  g->xoff = otfg->f.f1.value->XPlacement * size10 / u / 10;
		if (format & OTF_XPlaDevice)
		  g->xoff += DEVICE_DELTA (otfg->f.f1.value->XPlaDevice, size);
		if (format & OTF_YPlacement)
		  g->yoff = - (otfg->f.f1.value->YPlacement * size10 / u / 10);
		if (format & OTF_YPlaDevice)
		  g->yoff -= DEVICE_DELTA (otfg->f.f1.value->YPlaDevice, size);
		if (format & OTF_XAdvance)
		  g->width += otfg->f.f1.value->XAdvance * size10 / u / 10;
		if (format & OTF_XAdvDevice)
		  g->width += DEVICE_DELTA (otfg->f.f1.value->XAdvDevice, size);
	      }
	      break;
	    case 3:
	      /* Not yet supported.  */
	      break;
	    case 4: case 5:
	      if (! base)
		break;
	      prev = base;
	      goto label_adjust_anchor;
	    default:		/* i.e. case 6 */
	      if (! mark)
		break;
	      prev = mark;

	    label_adjust_anchor:
	      {
		int base_x, base_y, mark_x, mark_y;

		base_x = otfg->f.f4.base_anchor->XCoordinate * size10 / u / 10;
		base_y = otfg->f.f4.base_anchor->YCoordinate * size10 / u / 10;
		mark_x = otfg->f.f4.mark_anchor->XCoordinate * size10 / u / 10;
		mark_y = otfg->f.f4.mark_anchor->YCoordinate * size10 / u / 10;

		if (otfg->f.f4.base_anchor->AnchorFormat != 1)
		  adjust_anchor (otfg->f.f4.base_anchor, ft_info->ft_face,
				 prev->code, size, &base_x, &base_y);
		if (otfg->f.f4.mark_anchor->AnchorFormat != 1)
		  adjust_anchor (otfg->f.f4.mark_anchor, ft_info->ft_face,
				 g->code, size, &mark_x, &mark_y);
		g->xoff = prev->xoff + (base_x - prev->width) - mark_x;
		g->yoff = prev->yoff + mark_y - base_y;
		g->combining_code = MAKE_PRECOMPUTED_COMBINDING_CODE ();
	      }
	    }
	  if (otfg->GlyphClass == OTF_GlyphClass0)
	    base = mark = g;
	  else if (otfg->GlyphClass == OTF_GlyphClassMark)
	    mark = g;
	  else
	    base = g;
	}
    }
  free (otf_gstring.glyphs);
  return to;

 simple_copy:
  ft_find_metric (rfont, gstring, from, to);
  for (i = 0; i < len; i++)
    {
      MGlyph temp = gstring->glyphs[from + i];
      MLIST_APPEND1 (gstring, glyphs, temp, MERROR_FONT_OTF);
    }
  free (otf_gstring.glyphs);
  return to;
}


int
mfont__ft_decode_otf (MGlyph *g)
{
  MFTInfo *ft_info = (MFTInfo *) g->rface->rfont->info;
  int c = OTF_get_unicode (ft_info->otf, (OTF_GlyphID) g->code);

  return (c ? c : -1);
}

#endif	/* HAVE_OTF */

#endif /* HAVE_FREETYPE */
