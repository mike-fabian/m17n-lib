/* font.c -- font module.
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

/***en
    @addtogroup m17nFont
    @brief Font object.

    The m17n GUI API represents a font by an object of the type @c
    MFont.  A font can have @e font @e properties.  Like other types
    of properties, a font property consists of a key and a value.  The
    key of a font property must be one of the following symbols:

    @c Mfoundry, @c Mfamily, @c Mweight, @c Mstyle, @c Mstretch,
    @c Madstyle, @c Mregistry, @c Msize, @c Mresolution, @c Mspacing.

    When the key of a font property is @c Msize or @c Mresolution, its
    value is an integer.  Otherwise the value is a symbol.  

    "The font property that belongs to font F and whose key is @c
    Mxxx" may be shortened to "the xxx property of F".

    The value of a foundry property is a symbol representing font
    foundry information, e.g. adobe, misc, etc.

    The value of a family property is a symbol representing font family
    information, e.g. times, helvetica, etc.

    The value of a weight property is a symbol representing weight
    information, e.g. normal, bold, etc.

    The value of a style property is a symbol representing slant
    information, e.g. normal, italic, etc.

    The value of a stretch property is a symbol representing width
    information, e.g. normal, semicondensed, etc.

    The value of an adstyle property is a symbol representing abstract
    font family information, e.g. serif, sans-serif, etc.

    The value of a registry property is a symbol representing registry
    information, e.g. iso10646-1, iso8895-1, etc.

    The value of a size property is an integer representing design
    size in the unit of 1/10 point.

    The value of a resolution property is an integer representing
    assumed device resolution in the unit of dots per inch (dpi).

    The value of a type property is a symbol indicating a font driver;
    currently Mx or Mfreetype.

    The m17n library uses font objects for two purposes: to receive
    font specification from an application program, and to present
    available fonts to an application program.  When the m17n library
    presents an available font to an application program, all font
    properties have a concrete value.

    The m17n library supports three kinds of fonts: Window system fonts,
    FreeType fonts, and OpenType fonts.

    <ul>

    <li> Window system fonts

    The m17n-X library supports all fonts handled by an X server and
    an X font server.  The correspondence between XLFD fields and font
    properties are shown below.

@verbatim
    XLFD field                                  property
    ---------------                             --------
    FOUNDRY                                     foundry
    FAMILY_NAME                                 family
    WEIGHT_NAME                                 weight
    SLANT                                       style
    SETWIDTH_NAME                               stretch
    ADD_STYLE_NAME                              adstyle
    PIXEL_SIZE                                  size
    RESOLUTION_Y                                resolution
    CHARSET_REGISTRY-CHARSET_ENCODING           registry
@endverbatim

    XLFD fields not listed in the above table are ignored.

    <li> FreeType fonts

    The m17n library, if configured to use the FreeType library,
    supports all fonts that can be handled by the FreeType library.
    The variable #mfont_freetype_path is initialized properly according
    to the configuration of the m17n library and the environment
    variable @c M17NDIR.  See the documentation of the variable for
    details.

    If the m17n library is configured to use the fontconfig library,
    in addition to #mfont_freetype_path, all fonts available via
    fontconfig are supported.

    The family name of a FreeType font corresponds to the family
    property.  Style names of FreeType fonts correspond to the weight,
    style, and stretch properties as below.

@verbatim
    style name          weight  style   stretch
    ----------          ------  -----   -------
    Regular             medium  r       normal
    Italic              medium  i       normal
    Bold                bold    r       normal
    Bold Italic         bold    i       normal
    Narrow              medium  r       condensed
    Narrow Italic       medium  i       condensed
    Narrow Bold         bold    r       condensed
    Narrow Bold Italic  bold    i       condensed
    Black               black   r       normal
    Black Italic        black   i       normal
    Oblique             medium  o       normal
    BoldOblique         bold    o       normal
@endverbatim

    Style names not listed in the above table are treated as
    "Regular".

    Combination of a platform ID and an encoding ID corresponds to the
    registry property.  For example, if a font has the combination (1
    1), the registry property is 1-1.  Some frequent combinations have
    a predefined registry property as below.

@verbatim
    platform ID         encoding ID     registry property
    -----------         -----------     -----------------
    0                   3               unicode-bmp
    0                   4               unicode-full
    1                   0               apple-roman
    3                   1               unicode-bmp
    3                   1               unicode-full
@endverbatim

    Thus, a font that has two combinations (1 0) and (3 1) corresponds
    to four font objects whose registries are 1-0, apple-roman, 3-1,
    and unicode-bmp.

    <li> OpenType fonts 

    The m17n library, if configured to use both the FreeType library
    and the OTF library, supports any OpenType fonts.  The list of
    actually available fonts is created in the same way as in the case
    of FreeType fonts.  If a fontset instructs to use an OpenType font
    via an FLT (Font Layout Table), and the FLT has an OTF-related
    command (e.g. otf:deva), the OTF library converts a character
    sequence to a glyph code sequence according to the OpenType layout
    tables of the font, and the FreeType library gives a bitmap image
    for each glyph.

    </ul>

  */

/***ja
    @addtogroup m17nFont
    @brief フォントオブジェクト.

    m17n GUI API はフォントを @c MFont 型のオブジェクトとして表現する。
    フォントは @e フォントプロパティ を持つことができる。他のタイプのプ
    ロパティ同様、フォントプロパティはキーと値からなり、キーは以下のシ
    ンボルのいずれかである。

    @c Mfoundry, @c Mfamily, @c Mweight, @c Mstyle, @c Mstretch,
    @c Madstyle, @c Mregistry, @c Msize, @c Mresolution, @c Mspacing

    フォントプロパティのキーが @c Msize あるいは @c Mresolution 
    の場合、値は整数値であり、キーがそれ以外の場合、値はシンボルである。

    「フォント F のフォントプロパティのうちキーが @c Mxxx 
    であるもの」のことを簡単に「F の xxx プロパティ」と呼ぶことがある。

    foundry プロパティの値は、adobe, misc 
    等のフォントの開発元情報を示すシンボルである。

    family プロパティの値は、times, helvetica 
    等のフォントファミリーを示すシンボルである。

    weight プロパティの値は、normal, bold 等の太さに関する情報を示すシンボルである。

    style プロパティの値は、normal, italic 
    等のスタイルに関する情報を示すシンボルである。

    stretch プロパティの値は、normal, semicondensed 
    等の文字幅に関する情報を示すシンボルである。

    adstyle プロパティの値は、serif, sans-serif
    等の抽象的なフォントファミリーに関する情報を示すシンボルである。

    registry プロパティの値は、iso10646, iso8895-1
    等のレジストリ情報を示すシンボルである。

    size プロパティの値は、フォントのデザインサイズを表わす整数値であり、
    単位は1/10 ポイントである。

    resolution プロパティの値は、想定されているデバイスの解像度を表わす
    整数値であり、単位はdots per inch (dpi) である。

    type プロパティの値は、フォントドライバを指示し、現在 Mx もしくは
    Mfreetype である。

    m17n ライブラリはフォントオブジェクトを２つの目的で用いている。アプ
    リケーションプログラムからフォントの指定を受け取る目的と、アプリケー
    ションプログラムに利用可能なフォントを提示する目的である。アプリケー
    ションプログラムに対して提示を行う際には、フォントプロパティはすべ
    て具体的な値を持つ。

    m17n ライブラリは Window システムフォント、FreeTypeフォント、
    OpenTypeフォントの３種類をサポートしている。

    <ul>

    <li> Window システムフォント

    m17n X ライブラリは、X サーバと X フォントサーバが取り扱う全てのフォントをサポートする。
    XLFD の各フィールドとフォントプロパティの対応は以下の通り。この表にないフィールドは無視される。

@verbatim
    XLFD フィールド                             プロパティ
    ---------------                             --------
    FOUNDRY                                     foundry
    FAMILY_NAME                                 family
    WEIGHT_NAME                                 weight
    SLANT                                       style
    SETWIDTH_NAME                               stretch
    ADD_STYLE_NAME                              adstyle
    PIXEL_SIZE                                  size
    RESOLUTION_Y                                resolution
    CHARSET_REGISTRY-CHARSET_ENCODING           registry
@endverbatim

    <li> FreeType fonts

    m17n ライブラリは、FreeType ライブラリを使うように設定された場合には、
    FreeType が扱うすべてのフォントをサポートする。変数 
    #mfont_freetype_path は m17n ライブラリの設定と環境変数 @c M17NDIR 
    に応じて初期化される。詳細は変数の説明を参照のこと。

    もし m17n ライブラリが fontconfig ライブラリを使うように設定された場合には、
    #mfont_freetype_path に加えて、 fontconfig で使用可能なフォントもすべてサポートされる。

    FreeType フォントのファミリ名は family プロパティに対応する。
    FreeType フォントのスタイル名は、下の表のように weight, style,
    stretch プロパティに対応する。

@verbatim
    スタイル名          weight  style   stretch
    ----------          ------  -----   -------
    Regular             medium  r       normal
    Italic              medium  i       normal
    Bold                bold    r       normal
    Bold Italic         bold    i       normal
    Narrow              medium  r       condensed
    Narrow Italic       medium  i       condensed
    Narrow Bold         bold    r       condensed
    Narrow Bold Italic  bold    i       condensed
    Black               black   r       normal
    Black Italic        black   i       normal
    Oblique             medium  o       normal
    BoldOblique         bold    o       normal
@endverbatim

    上の表に現われないスタイル名は "Regular" として扱われる。

    platform ID と encoding ID の組み合わせが registry 
    プロパティに対応する。たとえばあるフォントが (1 1) という ID の組合せを持てば、
    registry プロパティは 1-1 となる。頻繁にあらわれる組合せには以下のような定義済み
    registry プロパティ が与えられている。

@verbatim
    platform ID         encoding ID     registry プロパティ
    -----------         -----------     -----------------
    0                   3               unicode-bmp
    0                   4               unicode-full
    1                   0               apple-roman
    3                   1               unicode-bmp
    3                   1               unicode-full
@endverbatim

    したがって、二つの組合せ (1 0) 、(3 1) を持つフォントは、それぞれ
    registry プロパティが 1-0, apple-roman, 3-1, unicode-bmp
    である４つのフォントオブジェクトに対応する。

    <li> OpenType フォント

    m17n ライブラリは、FreeType ライブラリと OTF 
    ライブラリを使用するように設定すれば、すべての OpenType 
    フォントをサポートする。実際に利用できるフォントのリストは FreeType
    フォントの場合と同様に作られる。OpenType フォントを FLT (Font Layout Table)
    経由で使用するようフォントセットに指定されており、FLT に OTF 
    関連のコマンド (たとえば otf:deva) があれば、OTF ライブラリがフォントの OpenType
    レイアウトテーブルに従って文字列をグリフコード列に変換し、FreeType
    ライブラリが各グリフのビットマップイメージを提供する。

    </ul>

*/

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m17n-gui.h"
#include "m17n-misc.h"
#include "internal.h"
#include "mtext.h"
#include "symbol.h"
#include "plist.h"
#include "charset.h"
#include "internal-gui.h"
#include "font.h"
#include "face.h"
#include "fontset.h"

MPlist *mfont__driver_list;

static MSymbol M_font_capability, M_font_list, M_font_list_len;

/** Indices to font properties sorted by their priority.  */
static int font_score_priority[] =
  { MFONT_SIZE,
    MFONT_WEIGHT,
    MFONT_STYLE,
    MFONT_STRETCH,
    MFONT_FAMILY,
    MFONT_ADSTYLE,
    MFONT_FOUNDRY
  };

#define FONT_SCORE_PRIORITY_SIZE	\
  (sizeof font_score_priority / sizeof font_score_priority[0])

/* Indexed by a font property MFONT_XXX, and the value is how many
   bits to shift the difference of property values.  */
static int font_score_shift_bits[MFONT_SIZE + 1];

/** Predefined symbols for each font property.  The order is important
    because the function font_score () decides how well a font matches
    with a spec by checking how close the index is.  */

static char *common_foundry[] =
  { "misc",
    "adobe" };
static char *common_family[] =
  { "fixed",
    "courier",
    "helvetica",
    "times" };
static char *common_weight[] =
  { "thin"
    "ultralight",
    "extralight",
    "light",
    "demilight",
    "book",
    "regular",
    "normal",
    "medium",
    "demibold",
    "semibold",
    "bold",
    "extrabold",
    "ultrabold",
    "black",
    "heavy" };
static char *common_style[] =
  { "o",
    "i",
    "slanted",
    "r",
    "rslanted",
    "ri",
    "ro" };
static char *common_stretch[] =
  { "ultracondensed",
    "extracondensed",
    "condensed",
    "narrow",
    "semicondensed",
    "normal",
    "semiexpanded",
    "expanded",
    "extraexpanded",
    "ultraexpanded" };
static char *common_adstyle[] =
  { "serif",
    "",
    "sans" };
static char *common_registry[] =
  { "iso8859-1" };

static unsigned short font_weight_regular;
static unsigned short font_weight_normal;
static unsigned short font_weight_medium;

/* Table containing all the data above.  */

struct MFontCommonNames
{
  int num;
  char **names;
};

static struct MFontCommonNames font_common_names[] =
  {
    { sizeof (common_foundry) / sizeof (char *), common_foundry},
    { sizeof (common_family) / sizeof (char *), common_family},
    { sizeof (common_weight) / sizeof (char *), common_weight},
    { sizeof (common_style) / sizeof (char *), common_style},
    { sizeof (common_stretch) / sizeof (char *), common_stretch},
    { sizeof (common_adstyle) / sizeof (char *), common_adstyle},
    { sizeof (common_registry) / sizeof (char *), common_registry}
  };


/** Table of available font property names.  */

MFontPropertyTable mfont__property_table[MFONT_REGISTRY + 1];


/** Return the numeric value of SYMBOL as the Nth font property.  */

#define FONT_PROPERTY_NUMERIC(symbol, n)	\
  ((symbol) == Mnil				\
   ? 0						\
   : ((int) msymbol_get ((symbol), mfont__property_table[(n)].property)))


/** Set the numeric value of SYMBOL as the Nth font property to NUMERIC.  */

#define SET_FONT_PROPERTY_NUMERIC(symbol, n, numeric)		\
  msymbol_put((symbol), mfont__property_table[(n)].property,	\
	      (void *) (numeric))


/* Font selector.  */

struct MFontEncoding {
  MFont spec;
  MSymbol encoding_name;
  MCharset *encoding_charset;
  MSymbol repertory_name;
  MCharset *repertory_charset;
};

static MPlist *font_encoding_list;
static MFontEncoding default_encoding;

/** Load font encoding table from the data <font encoding>.
    The data has this form:
	(FONT-SPEC ENCODING) ...
    where FONT-SPEC has this form:
	([FOUNDRY FAMILY [WEIGHT [STYLE [STRETCH [ADSTYLE]]]]] REGISTRY)
    All elements are symbols.  */

static int
load_font_encoding_table ()
{
  MDatabase *mdb;
  MPlist *encoding_list, *plist, *pl, *elt;

  font_encoding_list = pl = mplist ();

  mdb = mdatabase_find (Mfont, msymbol ("encoding"), Mnil, Mnil);
  if (! mdb
      || ! (encoding_list = (MPlist *) mdatabase_load (mdb)))
    MERROR (MERROR_FONT, -1);

  MPLIST_DO (plist, encoding_list)
    {
      MFontEncoding *encoding;
      MSymbol registry;

      MSTRUCT_CALLOC (encoding, MERROR_FONT);

      if (! MPLIST_PLIST_P (plist)
	  || (elt = MPLIST_PLIST (plist), mplist_length (elt) < 2)
	  || ! MPLIST_PLIST_P (elt))
	MWARNING (MERROR_FONT);
      registry = mfont__set_spec_from_plist (&encoding->spec,
					     MPLIST_PLIST (elt));
      elt = MPLIST_NEXT (elt);
      if (! MPLIST_SYMBOL_P (elt))
	MWARNING (MERROR_FONT);
      encoding->encoding_name = MPLIST_SYMBOL (elt);
      elt = MPLIST_NEXT (elt);
      if (MPLIST_TAIL_P (elt))
	encoding->repertory_name = encoding->encoding_name;
      else if (! MPLIST_SYMBOL_P (elt))
	MWARNING (MERROR_FONT);
      else
	encoding->repertory_name = MPLIST_SYMBOL (elt);

      if (registry == Mnil)
	mplist_push (font_encoding_list, Mt, encoding);
      else
	pl = mplist_add (pl, registry, encoding);
      continue;

    warning:
      free (encoding);
    }

  M17N_OBJECT_UNREF (encoding_list);
  return 0;
}

typedef struct {
  MFont spec;
  int resize;
} MFontResize;

static MPlist *font_resize_list;

/** Load font size table from the data <font size>.
    The data has this form:
	(FONT-SPEC RESIZE-FACTOR) ...
    where FONT-SPEC has this form:
	([FOUNDRY FAMILY [WEIGHT [STYLE [STRETCH [ADSTYLE]]]]] REGISTRY)
    All elements of FONT-SPEC are symbols.  */

static int
load_font_resize_table ()
{
  MDatabase *mdb;
  MPlist *size_adjust_list, *plist, *pl, *elt;

  font_resize_list = pl = mplist ();

  mdb = mdatabase_find (Mfont, msymbol ("resize"), Mnil, Mnil);
  if (! mdb)
    return -1;
  if (! (size_adjust_list = (MPlist *) mdatabase_load (mdb)))
    MERROR (MERROR_FONT, -1);

  MPLIST_DO (plist, size_adjust_list)
    {
      MFontResize *resize;
      MSymbol registry;

      MSTRUCT_CALLOC (resize, MERROR_FONT);

      if (! MPLIST_PLIST_P (plist)
	  || (elt = MPLIST_PLIST (plist), mplist_length (elt) != 2)
	  || ! MPLIST_PLIST_P (elt))
	MWARNING (MERROR_FONT);
      registry = mfont__set_spec_from_plist (&resize->spec,
					     MPLIST_PLIST (elt));
      elt = MPLIST_NEXT (elt);
      if (! MPLIST_INTEGER_P (elt))
	MWARNING (MERROR_FONT);
      resize->resize = MPLIST_INTEGER (elt);

      if (registry == Mnil)
	registry = Mt;
      pl = mplist_add (pl, registry, resize);
      continue;

    warning:
      free (resize);
    }

  M17N_OBJECT_UNREF (size_adjust_list);
  return 0;
}

/** Return a font encoding (and repertory) of FONT.  */

static MFontEncoding *
find_encoding (MFont *font)
{
  MSymbol registry = FONT_PROPERTY (font, MFONT_REGISTRY);
  MFontEncoding *encoding = NULL;
  MPlist *plist;

  if (! font_encoding_list)
    load_font_encoding_table ();
  plist = font_encoding_list;
  while (! MPLIST_TAIL_P (plist))
    {
      encoding = (MFontEncoding *) MPLIST_VAL (plist);
      if (mfont__match_p (font, &encoding->spec, MFONT_REGISTRY))
	{
	  if (encoding->encoding_name != Mnil
	      && ! encoding->encoding_charset)
	    {
	      encoding->encoding_charset = MCHARSET (encoding->encoding_name);
	      if (! encoding->encoding_charset)
		{
		  mplist_pop (plist);
		  continue;
		}
	    }
	  if (encoding->repertory_name == encoding->encoding_name)
	    encoding->repertory_charset = encoding->encoding_charset;
	  else if (encoding->repertory_name != Mnil)
	    {
	      encoding->repertory_charset
		= MCHARSET (encoding->repertory_name);
	      if (! encoding->repertory_charset)
		{
		  mplist_pop (plist);
		  continue;
		}
	    }
	  font->encoding = encoding;
	  return encoding;
	}

      if (registry && MPLIST_KEY (plist) != Mt)
	{
	  plist = mplist_find_by_key (plist, registry);
	  if (! plist)
	    break;
	}
      else
	plist = MPLIST_NEXT (plist);
    }
  font->encoding = &default_encoding;
  return &default_encoding;
}

#ifndef HAVE_OTF
static OTF_Tag
OTF_tag (char *name)
{
  unsigned char *p = (unsigned char *) name;

  if (! name)
    return (OTF_Tag) 0;
  return (OTF_Tag) ((p[0] << 24)
		    | (! p[1] ? 0
		       : ((p[1] << 16)
			  | (! p[2] ? 0
			     : (p[2] << 8) | p[3]))));
}
#endif	/* not HAVE_OTF */

static MPlist *otf_script_list;

static int
load_otf_script_list ()
{
  MDatabase *mdb;
  MPlist *plist, *pl;

  otf_script_list = mplist ();
  mdb = mdatabase_find (msymbol ("standard"), Mscript, msymbol ("otf"), Mnil);
  if (! mdb
      || ! (plist = mdatabase_load (mdb)))
    MERROR (MERROR_FONT, -1);
  MPLIST_DO (pl, plist)
    {
      MPlist *p;
      MSymbol script, otf_script;
      OTF_Tag tag;

      if (! MPLIST_PLIST_P (pl))
	continue;
      p = MPLIST_PLIST (pl);
      if (! MPLIST_SYMBOL_P (p))
	continue;
      script = MPLIST_SYMBOL (p);
      p = MPLIST_NEXT (p);
      if (! MPLIST_SYMBOL_P (p))
	continue;
      otf_script = MPLIST_SYMBOL (p);
      tag = OTF_tag (MSYMBOL_NAME (otf_script));
      mplist_push (otf_script_list, script, (void *) tag);
    }
  M17N_OBJECT_UNREF (plist);
  return 0;
}

static MSymbol
find_script_from_otf_tag (OTF_Tag tag)
{
  MPlist *plist;

  if (! otf_script_list)
    load_otf_script_list ();
  plist = mplist_find_by_value (otf_script_list, (void *) tag);
  return (plist ? MPLIST_KEY (plist) : Mnil);
}

/* XLFD parser/generator */

/** Indices to each field of split font name.  */

enum xlfd_field_idx
  {
    XLFD_FOUNDRY,
    XLFD_FAMILY,
    XLFD_WEIGHT,
    XLFD_SLANT,
    XLFD_SWIDTH,
    XLFD_ADSTYLE,
    XLFD_PIXEL,
    XLFD_POINT,
    XLFD_RESX,
    XLFD_RESY,
    XLFD_SPACING,
    XLFD_AVGWIDTH,
    XLFD_REGISTRY,		/* This contains ENCODING.  */
    /* anchor */
    XLFD_FIELD_MAX
  };

static int
xlfd_parse_name (const char *name, MFont *font)
{
  char *field[XLFD_FIELD_MAX];
  unsigned short resy, avgwidth;
  unsigned size;
  char copy[513];
  int i;
  char *p;

  if (name[0] != '-')
    return -1;

  field[0] = copy;
  for (i = 1, p = copy, name++; *name; p++, name++)
    {
      if (p - copy > 512)
	return -1;
      if (*name == '-'
	  && i < XLFD_FIELD_MAX)
	{
	  *p = '\0';
	  if (field[i - 1][0] == '*')
	    field[i - 1] = NULL;
	  field[i++] = p + 1;
	}
      else
	*p = tolower (*name);
    }
  *p = '\0';
  if (field[i - 1][0] == '*')
    field[i - 1] = NULL;
  while (i < XLFD_FIELD_MAX)
    field[i++] = NULL;

  resy = field[XLFD_RESY] ? atoi (field[XLFD_RESY]) : 0;
  avgwidth = ((field[XLFD_AVGWIDTH] && isdigit (field[XLFD_AVGWIDTH][0]))
	      ? atoi (field[XLFD_AVGWIDTH]) : 1);
  if (! avgwidth)
    size = 0;
  else if (! field[XLFD_PIXEL])
    size = field[XLFD_POINT] ? atoi (field[XLFD_POINT]) * resy / 72 : 0;
  else if (field[XLFD_PIXEL][0] == '[')
    {
      /* The pixel size field specifies a transformation matrix of the
	 form "[A B C D]".  The XLFD spec says that the scalar value N
	 for the pixel size is equivalent to D.  */
      char *p0 = field[XLFD_PIXEL] + 1, *p1;
      double d;

      for (i = 0; i < 4; i++, p0 = p1)
	d = strtod (p0, &p1);
      size = d * 10;
    }
  else
    size = atoi (field[XLFD_PIXEL]) * 10;

  if (field[XLFD_FOUNDRY])
    mfont__set_property (font, MFONT_FOUNDRY, msymbol (field[XLFD_FOUNDRY]));
  if (field[XLFD_FAMILY])
    mfont__set_property (font, MFONT_FAMILY, msymbol (field[XLFD_FAMILY]));
  if (field[XLFD_WEIGHT])
    mfont__set_property (font, MFONT_WEIGHT, msymbol (field[XLFD_WEIGHT]));
  if (field[XLFD_SLANT])
    mfont__set_property (font, MFONT_STYLE, msymbol (field[XLFD_SLANT]));
  if (field[XLFD_SWIDTH])
    mfont__set_property (font, MFONT_STRETCH, msymbol (field[XLFD_SWIDTH]));
  if (field[XLFD_ADSTYLE])
    mfont__set_property (font, MFONT_ADSTYLE, msymbol (field[XLFD_ADSTYLE]));
  font->property[MFONT_RESY] = resy;
  font->size = size;
  if (field[XLFD_SPACING])
    font->spacing
      = ((field[XLFD_SPACING][0] == 'p' || field[XLFD_SPACING][0] == 'P')
	 ? MFONT_SPACING_PROPORTIONAL
	 : (field[XLFD_SPACING][0] == 'm' || field[XLFD_SPACING][0] == 'M')
	 ? MFONT_SPACING_MONO : MFONT_SPACING_CHARCELL);
  if (field[XLFD_REGISTRY])
    mfont__set_property (font, MFONT_REGISTRY, msymbol (field[XLFD_REGISTRY]));
  font->type = MFONT_TYPE_SPEC;
  font->source = MFONT_SOURCE_X;
  return 0;
}

static char *
xlfd_unparse_name (MFont *font, int full_xlfd)
{
  MSymbol prop[7];
  char name[513];
  char *str[7];
  int len, i;
  char spacing;
  int size, resy;

  prop[0] = (MSymbol) mfont_get_prop (font, Mfoundry);
  prop[1] = (MSymbol) mfont_get_prop (font, Mfamily);
  prop[2] = (MSymbol) mfont_get_prop (font, Mweight);
  prop[3] = (MSymbol) mfont_get_prop (font, Mstyle);
  prop[4] = (MSymbol) mfont_get_prop (font, Mstretch);
  prop[5] = (MSymbol) mfont_get_prop (font, Madstyle);
  prop[6] = (MSymbol) mfont_get_prop (font, Mregistry);
  for (len = 0, i = 0; i < 7; i++)
    {
      if (prop[i] != Mnil)
	{
	  str[i] = msymbol_name (prop[i]);
	  len += strlen (str[i]);
	}
      else
	{
	  str[i] = "*";
	  len++;
	}
    }
  spacing = (font->spacing == MFONT_SPACING_UNDECIDED ? '*'
	     : font->spacing == MFONT_SPACING_PROPORTIONAL ? 'p'
	     : font->spacing == MFONT_SPACING_MONO ? 'm'
	     : 'c');

  if ((len
       + 13			/* 13 dashes */
       + 2			/* 2 asterisks */
       + 30			/* 3 integers (each 10 digits) */
       + 1			/* 1 spacing char */
       + 1)			/* '\0' terminal */
      > 513)
    return NULL;

  resy = (int) mfont_get_prop (font, Mresolution);
  size = font->size;
  if (size >= 0)
    {
      if ((size % 10) < 5)
	size /= 10;
      else
	size = size / 10 + 1;
    }
  else
    size = - size;

  if (full_xlfd)
    {
      if (font->size >= 0)
	sprintf (name, "-%s-%s-%s-%s-%s-%s-%d-*-%d-%d-%c-*-%s",
		 str[0], str[1], str[2], str[3], str[4], str[5],
		 size, resy, resy, spacing, str[6]);
      else
	sprintf (name, "-%s-%s-%s-%s-%s-%s-*-%d-%d-%d-%c-*-%s",
		 str[0], str[1], str[2], str[3], str[4], str[5],
		 size, resy, resy, spacing, str[6]);
    }
  else
    {
      char *p = name;

      p += sprintf (p, "-%s", str[0]);
      for (i = 1; i < 6; i++)
	if (p[-1] != '*' || str[i][0] != '*')
	  p += sprintf (p, "-%s", str[i]);
      if (p[-1] != '*' || font->size > 0)
	{
	  if (font->size > 0)
	    p += sprintf (p, "-%d-*", size);
	  else
	    p += sprintf (p, "-*");
	}
      if (str[6][0] != '*')
	sprintf (p, "-%s", str[6]);
    }

  return strdup (name);
}

/* Compare FONT with REQUEST and return how much they differs.  */

static int
font_score (MFont *font, MFont *request)
{
  int score = 0;
  int i = FONT_SCORE_PRIORITY_SIZE;

  while (--i >= 0)
    {
      enum MFontProperty prop = font_score_priority[i];
      int val;

      if (prop == MFONT_SIZE)
	{
	  if (font->size && request->size)
	    {
	      val = font->size - request->size;
	      if (val)
		{
		  if (val < 0)
		    val = - val;
		  if (val >= 0x10000)
		    val = 0xFFFF;
		  score |= (val << font_score_shift_bits[MFONT_SIZE]);
		}
	    }
	}
      else if (font->property[prop] && request->property[prop]
	       && font->property[prop] != request->property[prop])
	{
	  if (prop <= MFONT_FAMILY)
	    val = 1;
	  else if (prop == MFONT_WEIGHT)
	    {
	      unsigned short v1 = font->property[prop];
	      unsigned short v2 = request->property[prop];

	      if (v1 == font_weight_regular || v1 == font_weight_normal)
		v1 = font_weight_medium;
	      if (v2 == font_weight_regular || v2 == font_weight_normal)
		v2 = font_weight_medium;
	      val = v1 > v2 ? v1 - v2 : v2 - v1;
	    }
	  else
	    {
	      val = font->property[prop] - request->property[prop];
	      if (val < 0)
		val = - val;
	      if (val > 3)
		val = 3;
	    }
	  score |= val << font_score_shift_bits[prop];
	}
    }
  if (request->file != Mnil && request->file != font->file)
    score |= 40000000;
  return score;
}


/* Internal API */

MSymbol Miso8859_1, Miso10646_1, Municode_bmp, Municode_full, Mapple_roman;

int
mfont__init ()
{
  int i, shift;
  MSymbol regular = msymbol ("regular");
  MSymbol normal = msymbol ("normal");
  MSymbol medium = msymbol ("medium");

  M_font_capability = msymbol_as_managing_key ("  font-capability");
  M_font_list = msymbol_as_managing_key ("  font-list");
  M_font_list_len = msymbol ("  font-list-len");

  Mfoundry = msymbol ("foundry");
  mfont__property_table[MFONT_FOUNDRY].property = Mfoundry;
  Mfamily = msymbol ("family");
  mfont__property_table[MFONT_FAMILY].property = Mfamily;
  Mweight = msymbol ("weight");
  mfont__property_table[MFONT_WEIGHT].property = Mweight;
  Mstyle = msymbol ("style");
  mfont__property_table[MFONT_STYLE].property = Mstyle;
  Mstretch = msymbol ("stretch");
  mfont__property_table[MFONT_STRETCH].property = Mstretch;
  Madstyle = msymbol ("adstyle");
  mfont__property_table[MFONT_ADSTYLE].property = Madstyle;
  Mregistry = msymbol ("registry");
  mfont__property_table[MFONT_REGISTRY].property = Mregistry;

  Mspacing = msymbol ("spacing");
  Msize = msymbol ("size");
  Mresolution = msymbol ("resolution");
  Mmax_advance = msymbol ("max-advance");
  Mfontfile = msymbol ("fontfile");

  Mfontconfig = msymbol ("fontconfig");

  Mx = msymbol ("x");
  Mfreetype = msymbol ("freetype");
  Mxft = msymbol ("xft");

  Miso8859_1 = msymbol ("iso8859-1");
  Miso10646_1 = msymbol ("iso10646-1");
  Municode_bmp = msymbol ("unicode-bmp");
  Municode_full = msymbol ("unicode-full");
  Mapple_roman = msymbol ("apple-roman");

  /* The first entry of each mfont__property_table must be Mnil so
     that actual properties get positive numeric numbers.  */
  for (i = 0; i <= MFONT_REGISTRY; i++)
    {
      MLIST_INIT1 (&mfont__property_table[i], names, 8);
      MLIST_APPEND1 (&mfont__property_table[i], names, Mnil, MERROR_FONT);
    }

  /* Register predefined font property names.  */
  for (i = 0; i <= MFONT_REGISTRY; i++)
    {
      int j;

      for (j = 0; j < font_common_names[i].num; j++)
	{
	  MSymbol sym = msymbol (font_common_names[i].names[j]);

	  if (sym == Mnil)
	    return -1;
	  if (msymbol_put (sym, mfont__property_table[i].property,
			   (void *) (j + 1)) < 0)
	    return -1;
	  MLIST_APPEND1 (&mfont__property_table[i], names, sym,
			 MERROR_FONT);
	  if (i == MFONT_WEIGHT)
	    {
	      if (sym == regular)
		font_weight_regular = j + 1;
	      else if (sym == normal)
		font_weight_normal = j + 1;
	      else if (sym == medium)
		font_weight_medium = j + 1;
	    }
	}
    }

  /* Here, SHIFT starts from 1, not 0.  This is because the lowest bit
     of a score is a flag for a scalable font (see the documentation
     of font_score).  */
  i = FONT_SCORE_PRIORITY_SIZE - 1;
  for (shift = 1; i >= 0; i--)
    {
      font_score_shift_bits[font_score_priority[i]] = shift;
      if (font_score_priority[i] == MFONT_SIZE)
	shift += 16;
      else if (font_score_priority[i] <= MFONT_FAMILY)
	shift++;
      else
	shift += 2;
    }

  MFONT_INIT (&default_encoding.spec);
  default_encoding.encoding_name = Municode_full;
  default_encoding.encoding_charset = mcharset__unicode;
  default_encoding.repertory_name = Mnil;
  default_encoding.repertory_charset = NULL;
  {
    char *path, *buf;
    int bufsize;
    USE_SAFE_ALLOCA;

    mfont_freetype_path = mplist ();
    bufsize = strlen (M17NDIR) + 7;
    SAFE_ALLOCA (buf, bufsize);
    sprintf (buf, "%s/fonts", M17NDIR);
    mplist_add (mfont_freetype_path, Mstring, strdup (buf));
    path = getenv ("M17NDIR");
    if (path)
      {
	bufsize = strlen (path) + 7;
	SAFE_ALLOCA (buf, bufsize);
	sprintf (buf, "%s/fonts", path);
	mplist_push (mfont_freetype_path, Mstring, strdup (buf));
      }
    SAFE_FREE (buf);
  }

#ifdef HAVE_FREETYPE
  if (mfont__ft_init () < 0)
    return -1;
#endif /* HAVE_FREETYPE */
  if (mfont__flt_init () < 0)
    return -1;

  return 0;
}

void
mfont__fini ()
{
  MPlist *plist;
  int i;

  mfont__flt_fini ();
#ifdef HAVE_FREETYPE
  mfont__ft_fini ();
#endif /* HAVE_FREETYPE */

  MPLIST_DO (plist, mfont_freetype_path)
    free (MPLIST_VAL (plist));
  M17N_OBJECT_UNREF (mfont_freetype_path);

  if (font_resize_list)
    {
      MPLIST_DO (plist, font_resize_list)
	free (MPLIST_VAL (plist));
      M17N_OBJECT_UNREF (font_resize_list);
      font_resize_list = NULL;
    }
  if (font_encoding_list)
    {
      MPLIST_DO (plist, font_encoding_list)
	free (MPLIST_VAL (plist));
      M17N_OBJECT_UNREF (font_encoding_list);
      font_encoding_list = NULL;
    }
  if (otf_script_list)
    {
      M17N_OBJECT_UNREF (otf_script_list);
      otf_script_list = NULL;
    }

  for (i = 0; i <= MFONT_REGISTRY; i++)
    MLIST_FREE1 (&mfont__property_table[i], names);
}


MSymbol
mfont__id (MFont *font)
{
  char *buf, *p;
  int i;
  int file_len = (font->file == Mnil ? 0 : MSYMBOL_NAMELEN (font->file));
  int capability_len  = (font->capability == Mnil ? 0
			 : MSYMBOL_NAMELEN (font->capability));
  int total_len = MFONT_PROPERTY_MAX * 5 + 9 + file_len + capability_len;
  MSymbol id;
  USE_SAFE_ALLOCA;

  SAFE_ALLOCA (buf, total_len);
  p = buf;
  if (font->property[0])
    p += sprintf (p, "%X", font->property[0]);
  for (i = 1; i < MFONT_PROPERTY_MAX; i++)
    {
      if (font->property[i])
	p += sprintf (p, "-%X", font->property[i]);
      else
	*p++ = '-';
    }
  if (font->size)
    p += sprintf (p, "-%X", font->size);
  if (font->spacing)
    p += sprintf (p, "-%X", font->spacing);
  if (capability_len > 0)
    {
      *p++ = '-';
      memcpy (p, MSYMBOL_NAME (font->capability), capability_len);
      p += capability_len;
    }
  if (file_len > 0)
    {
      *p++ = '-';
      memcpy (p, MSYMBOL_NAME (font->file), file_len);
      p += file_len;
    }      
  id = msymbol__with_len (buf, p - buf);
  SAFE_FREE (buf);
  return id;
}

/** Return 1 iff FONT matches SPEC.  */

int
mfont__match_p (MFont *font, MFont *spec, int prop)
{
  if (spec->capability != font->capability
      && spec->capability != Mnil && font->capability != Mnil)
    return 0;
  if (spec->file != font->file
      && spec->file != Mnil && font->file != Mnil)
    return 0;
  for (; prop >= 0; prop--)
    if (spec->property[prop] && font->property[prop]
	&& font->property[prop] != spec->property[prop])
      return 0;
  return 1;
}

/* Merge SRC into DST.  If error_on_conflict is nonzero and a font
   property differs in SRC and DST, return -1.  */

int
mfont__merge (MFont *dst, MFont *src, int error_on_conflict)
{
  int i;

  for (i = 0; i < MFONT_PROPERTY_MAX; i++)
    {
      if (! dst->property[i])
	dst->property[i] = src->property[i];
      else if (error_on_conflict
	       && src->property[i]
	       && dst->property[i] != src->property[i])
	return -1;
    }
  if (! dst->spacing)
    dst->spacing = src->spacing;
  else if (error_on_conflict
	   && src->spacing
	   && dst->spacing != src->spacing)
    return -1;
  if (! dst->size)
    dst->size = src->size;
  else if (error_on_conflict
	   && src->size
	   && dst->size != src->size)
    return -1;
  if (dst->capability == Mnil)
    dst->capability = src->capability;
  else if (error_on_conflict
	   && src->capability
	   && dst->capability != src->capability)
    return -1;
  if (dst->file == Mnil)
    dst->file = src->file;
  else if (error_on_conflict
	   && src->file
	   && dst->file != src->file)
    return -1;
  return 0;
}

void
mfont__set_spec_from_face (MFont *spec, MFace *face)
{
  int i;

  for (i = 0; i <= MFONT_ADSTYLE; i++)
    mfont__set_property (spec, i, face->property[i]);
  spec->property[MFONT_REGISTRY] = 0;
  spec->property[MFONT_RESY] = 0;
  spec->size = (int) (face->property[MFACE_SIZE]);
  spec->type = MFONT_TYPE_SPEC;
  spec->source = MFONT_SOURCE_UNDECIDED;
  spec->file = spec->capability = Mnil;
  spec->encoding = NULL;
}


extern MSymbol
mfont__set_spec_from_plist (MFont *spec, MPlist *plist)
{
  int i;
  MSymbol spec_list[MFONT_REGISTRY + 1];
  MSymbol registry;
  char *reg;

  MFONT_INIT (spec);
  memset (spec_list, 0, sizeof spec_list);
  for (i = 0; ! MPLIST_TAIL_P (plist); i++, plist = MPLIST_NEXT (plist))
    {
      if (! MPLIST_SYMBOL_P (plist))
	MERROR (MERROR_FONT, Mnil);	
      spec_list[i] = MPLIST_SYMBOL (plist);
    }
  registry = spec_list[i - 1];
  if (i > 1 && registry != Mnil)
    {
      reg = MSYMBOL_NAME (registry);
      if (reg[0] == ':')
	{
	  mfont__get_capability (registry);
	  spec->capability = registry;
	  registry = spec_list[i - 2];
	  i--;
	}
    }
  mfont__set_property (spec, MFONT_REGISTRY, registry);
  for (i -= 2; i >= 0; i--)
    mfont__set_property (spec, i, spec_list[i]);
  spec->type = MFONT_TYPE_SPEC;

  return registry;
}


MFont *
mfont__select (MFrame *frame, MFont *font, int max_size)
{
  MFontDriver *driver;

  if (font->type == MFONT_TYPE_FAILURE)
    return NULL;
  if (font->type != MFONT_TYPE_SPEC)
    return font;
  if (font->source == MFONT_SOURCE_UNDECIDED)
    {
      if (font->file != Mnil || font->capability != Mnil)
	font->source = MFONT_SOURCE_FT;
      else if (font->property[MFONT_REGISTRY])
	{
	  MSymbol registry = FONT_PROPERTY (font, MFONT_REGISTRY);
	  char *reg = MSYMBOL_NAME (registry);

	  if (strncmp (reg, "unicode-", 8) == 0
	      || strncmp (reg, "apple-roman", 11) == 0
	      || (reg[0] >= '0' && reg[0] <= '9' && reg[1] == '-'))
	    font->source = MFONT_SOURCE_FT;
	}
    }
  if (font->source != MFONT_SOURCE_FT)
    {
      driver = mplist_get (frame->font_driver_list, Mx);
      if (driver)
	return (driver->select) (frame, font, max_size);
    }
  driver = mplist_get (frame->font_driver_list, Mfreetype);
  if (! driver)
    return NULL;
  return (driver->select) (frame, font, max_size);
}


int
mfont__available (MFrame *frame, MFont *font)
{
  return -1;
}

static int
compare_font_score (const void *e1, const void *e2)
{
  MFontScore *s1 = (MFontScore *) e1, *s2 = (MFontScore *) e2;

  return (s1->font->for_full_width == s2->font->for_full_width
	  ? s1->score > s2->score
	  : s1->font->for_full_width);
}

void
mdebug_dump_font_list (MFontList *font_list)
{
  int i;

  for (i = 0; i < font_list->nfonts; i++)
    {
      fprintf (stderr, "%04X - ", font_list->fonts[i].score);
      mdebug_dump_font (font_list->fonts[i].font);
      fprintf (stderr, "\n");
    }
}

void
mfont__free_realized (MRealizedFont *rfont)
{
  MRealizedFont *next;

  for (; rfont; rfont = next)
    {
      next = rfont->next;
      M17N_OBJECT_UNREF (rfont->info);
      free (rfont);
      rfont = next;
    }
}

MFontList *
mfont__list (MFrame *frame, MFont *spec, MFont *request, int max_size)
{
  MFontList *list;
  MSymbol id = mfont__id (spec);
  MPlist *pl, *p;
  int num, i;

  pl = msymbol_get (id, M_font_list);
  if (pl)
    num = (int) msymbol_get (id, M_font_list_len);
  else
    {
      pl = mplist ();
      num = 0;
      MPLIST_DO (p, frame->font_driver_list)
	{
	  if (spec->source == MFONT_SOURCE_X ? MPLIST_KEY (p) == Mx
	      : spec->source == MFONT_SOURCE_FT ? MPLIST_KEY (p) == Mfreetype
	      : 1)
	    {
	      MFontDriver *driver = MPLIST_VAL (p);
	      num += (driver->list) (frame, pl, spec, 0);
	    }
	}
      msymbol_put (id, M_font_list, pl);
      M17N_OBJECT_UNREF (pl);
      msymbol_put (id, M_font_list_len, (void *) num);
    }
  
  if (num == 0)
    return NULL;

  MSTRUCT_MALLOC (list, MERROR_FONT);
  MTABLE_MALLOC (list->fonts, num, MERROR_FONT);
  for (i = 0; num > 0; num--, pl = MPLIST_NEXT (pl))
    {
      MFont *font = MPLIST_VAL (pl), *adjusted = font;

      if (max_size == 0
	  || font->size == 0
	  || font->size < max_size)
	{
	  list->fonts[i].font = font;
	  if (spec == request)
	    list->fonts[i].score = 0;
	  else
	    {
	      int resize_ratio;
	      MFont resized;

	      if (font->size > 0
		  && (resize_ratio = mfont_resize_ratio (font)) != 100)
		{
		  resized = *font;
		  resized.size = font->size * 100 / resize_ratio;
		  adjusted = &resized;
		}
	      list->fonts[i].score = font_score (adjusted, request);
	    }
	  i++;
	}
    }
  if (i == 0)
    {
      free (list->fonts);
      free (list);
      return NULL;
    }
  list->nfonts = i;
  if (spec != request)
    qsort (list->fonts, i, sizeof (MFontScore), compare_font_score);
  list->object = *spec;
  mfont__merge (&list->object, request, 0);
  list->object.type = MFONT_TYPE_OBJECT;
  return list;
}

/** Open a font specified in FONT.  */

MRealizedFont *
mfont__open (MFrame *frame, MFont *font, MFont *spec)
{
  MFontDriver *driver;
  MRealizedFont *rfont;

  if (font->source == MFONT_SOURCE_UNDECIDED)
    MFATAL (MERROR_FONT);
  if (font->type != MFONT_TYPE_OBJECT)
    MFATAL (MERROR_FONT);
  for (rfont = MPLIST_VAL (frame->realized_font_list); rfont;
       rfont = rfont->next)
    {
      driver = rfont->driver;
      if (rfont->font == font
	  && mplist_find_by_value (frame->font_driver_list, driver))
	break;
    }

  if (! rfont)
    {
      driver = mplist_get (frame->font_driver_list,
			   font->source == MFONT_SOURCE_X ? Mx : Mfreetype);
      if (! driver)
	MFATAL (MERROR_FONT);
    }
  return (driver->open) (frame, font, spec, rfont);
}

int
mfont__has_char (MFrame *frame, MFont *font, MFont *spec, int c)
{
  MFontEncoding *encoding;
  unsigned code;
  MFontDriver *driver;

  if (font->source == MFONT_SOURCE_UNDECIDED)
    MFATAL (MERROR_FONT);
  encoding = (font->encoding ? font->encoding : find_encoding (font));
  if (! encoding->encoding_charset)
    return 0;
  if (encoding->repertory_charset)
    {
      code = ENCODE_CHAR (encoding->repertory_charset, c);
      return (code != MCHAR_INVALID_CODE);
    }
  code = ENCODE_CHAR (encoding->encoding_charset, c);
  if (code == MCHAR_INVALID_CODE)
    return 0;
  if (font->type == MFONT_TYPE_REALIZED)
    driver = ((MRealizedFont *) font)->driver;
  else
    {
      driver = mplist_get (frame->font_driver_list,
			   font->source == MFONT_SOURCE_X ? Mx : Mfreetype);
      if (! driver)
	MFATAL (MERROR_FONT);
    }
  return (driver->has_char) (frame, font, spec, c, code);
}

unsigned
mfont__encode_char (MFrame *frame, MFont *font, MFont *spec, int c)
{
  MFontEncoding *encoding;
  unsigned code;
  MFontDriver *driver;

  if (font->source == MFONT_SOURCE_UNDECIDED)
    MFATAL (MERROR_FONT);
  encoding = (font->encoding ? font->encoding : find_encoding (font));
  if (! encoding->encoding_charset)
    return MCHAR_INVALID_CODE;
  if (font->source == MFONT_SOURCE_X && encoding->repertory_charset)
    return (ENCODE_CHAR (encoding->repertory_charset, c));
  code = ENCODE_CHAR (encoding->encoding_charset, c);
  if (code == MCHAR_INVALID_CODE)
    return MCHAR_INVALID_CODE;
  if (font->type == MFONT_TYPE_REALIZED)
    driver = ((MRealizedFont *) font)->driver;
  else
    {
      driver = mplist_get (frame->font_driver_list,
			   font->source == MFONT_SOURCE_X ? Mx : Mfreetype);
      if (! driver)
	MFATAL (MERROR_FONT);
    }
  return (driver->encode_char) (frame, font, spec, code);
}

void
mfont__get_metric (MGlyphString *gstring, int from, int to)
{
  MGlyph *from_g = MGLYPH (from), *to_g = MGLYPH (to), *g;
  MRealizedFont *rfont = from_g->rface->rfont;

  for (g = from_g; g != to_g; g++)
    if (g->rface->rfont != rfont)
      {
	int idx = GLYPH_INDEX (g);

	(rfont->driver->find_metric) (rfont, gstring, from, idx);
	from_g = g;
	rfont = g->rface->rfont;
	from = idx;
      }
  (rfont->driver->find_metric) (rfont, gstring, from, GLYPH_INDEX (g));
}


/* KEY <= MFONT_REGISTRY */

void
mfont__set_property (MFont *font, enum MFontProperty key, MSymbol val)
{
  int numeric;

  if (val == Mnil)
    numeric = 0;
  else
    {
      numeric = FONT_PROPERTY_NUMERIC (val, key);
      if (! numeric)
	{
	  numeric = mfont__property_table[key].used;
	  MLIST_APPEND1 (mfont__property_table + key, names, val, MERROR_FONT);
	  SET_FONT_PROPERTY_NUMERIC (val, key, numeric);
	}
    }
  font->property[key] = numeric;
}

int
mfont__parse_name_into_font (const char *name, MSymbol format, MFont *font)
{
  int result = -1;

  if (format == Mx || format == Mnil)
    result = xlfd_parse_name (name, font);
#ifdef HAVE_FONTCONFIG
  if (format == Mfontconfig || (result < 0 && format == Mnil))
    result = mfont__ft_parse_name (name, font);
#endif /* HAVE_FONTCONFIG */
  return result;
}

MPlist *
mfont__encoding_list (void)
{
  if (! font_encoding_list)
    load_font_encoding_table ();
  return font_encoding_list;
}

static void
free_font_capability (void *object)
{
  MFontCapability *cap = object;
  
  if (cap->lang)
    free (cap->lang);
  if (cap->script)
    {
      int i;
      for (i = 0; i < MFONT_OTT_MAX; i++)
	{
	  if (cap->features[i].str)
	    free (cap->features[i].str);
	  if (cap->features[i].tags)
	    free (cap->features[i].tags);
	}
    }
  free (cap);
}

MFontCapability *
mfont__get_capability (MSymbol sym)
{
  MFontCapability *cap = msymbol_get (sym, M_font_capability);
  char *str, *p, *endp;

  if (cap)
    return cap;
  str = MSYMBOL_NAME (sym);
  if (str[0] != ':')
    return NULL;
  M17N_OBJECT (cap, free_font_capability, MERROR_FONT);
  msymbol_put (sym, M_font_capability, cap);
  M17N_OBJECT_UNREF (cap);
  endp = str + MSYMBOL_NAMELEN (sym);
  while (str < endp)
    {
      if (*str++ != ':')
	continue;
      if (str[0] == 'o' && str[1] == 't' && str[2] == 'f' && str[3] == '=')
	{
	  int i;

	  str += 4;
	  for (i = 0, p = str; i < 4 && p < endp; i++, p++);
	  if (i < 4)
	    break;
	  cap->script_tag = OTF_tag (str);
	  cap->script = find_script_from_otf_tag (cap->script_tag);
	  if (*p == '/')
	    {
	      for (i = 0, str = ++p; i < 4 && p < endp; i++, p++);
	      if (i < 4)
		{
		  cap->script = Mnil;
		  break;
		}
	      cap->langsys_tag = OTF_tag (str);
	    }
	  else
	    cap->langsys_tag = 0;

	  for (i = 0; i < MFONT_OTT_MAX; i++)
	    cap->features[i].nfeatures = -1;

	  while (*p == '=' || *p == '+')
	    {
	      int idx = *p == '=' ? MFONT_OTT_GSUB : MFONT_OTT_GPOS;

	      str = ++p;
	      while (p < endp && *p != '+') p++;
	      if (str < p)
		{
		  int n;
		  /* We never have more than (p - str) tags.  */
		  OTF_Tag *tags = alloca (sizeof (OTF_Tag) * (p - str));
		  char *p0;

		  cap->features[idx].str = malloc (p - str + 1);
		  for (i = n = 0, p0 = str; str + i < p; i++)
		    {
		      cap->features[idx].str[i] = str[i];
		      if (str[i] == ',' || str + i + 1 == p)
			{
			  if (*p0 == '*')
			    tags[n] = 0;
			  else if (*p0 == '~')
			    tags[n] = OTF_tag (p0 + 1) | 0x80000000;
			  else
			    tags[n] = OTF_tag (p0);
			  n++;
			  p0 = str + i + 1;
			}
		    }
		  cap->features[idx].str[i] = '\0';
		  cap->features[idx].nfeatures = n;
		  if (n > 0)
		    {
		      int size = sizeof (OTF_Tag) * n;

		      cap->features[idx].tags = malloc (size);
		      memcpy (cap->features[idx].tags, tags, size);
		    }
		}
	      else
		{
		  cap->features[idx].str = NULL;
		  cap->features[idx].nfeatures = 0;
		}
	    }

	  for (i = 0; i < MFONT_OTT_MAX; i++)
	    if (cap->features[i].nfeatures < 0)
	      {
		cap->features[i].str = strdup ("*");
		cap->features[i].nfeatures = 1;
		cap->features[i].tags = malloc (sizeof (OTF_Tag));
		cap->features[i].tags[0] = 0;
	      }
	  str = p;
	}
      else if (str[0] == 'l' && str[1] == 'a' && str[2] == 'n' && str[3] == 'g'
	  && str[4] == '=')
	{
	  int count;

	  str += 5;
	  for (p = str, count = 2; p < endp && *p != ':'; p++)
	    if (*p == ',')
	      count++;
	  MTABLE_MALLOC (cap->lang, count, MERROR_FONT);
	  for (p = str, count = 0; p < endp && *p != ':'; p++)
	    if (*p == ',')
	      {
		MSymbol lang = msymbol__with_len (str, p - str), sym;

		if (msymbol_get (lang, Miso639_2))
		  cap->lang[count++] = lang;
		else if ((sym = msymbol_get (lang, Miso639_1)) != Mnil)
		  cap->lang[count++] = sym;
		else if (msymbol_get (lang, Mlanguage))
		  cap->lang[count++] = lang;
		str = p + 1;
	      }
	  if (str < p)
	    cap->lang[count++] = msymbol__with_len (str, p - str);
	  cap->lang[count] = Mnil;
	  str = p;
	}
      else if (str[0] == 's' && str[1] == 'c' && str[2] == 'r' && str[3] == 'i'
	       && str[4] == 'p' && str[5] == 't' && str[6] == '=')
	{
	  str += 7;
	  for (p = str; p < endp && *p != ':'; p++);
	  if (str < p)
	    cap->script = msymbol__with_len (str, p - str);
	  str = p;
	}
    }
  return cap;
}

int
mfont__check_capability (MRealizedFont *rfont, MSymbol capability)
{
  return (rfont->driver->check_capability (rfont, capability));
}


/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */



/* External API */

/*** @addtogroup m17nFont */
/*** @{ */
/*=*/

/***en @name Variables: Keys of font property.  */
/***ja @name 変数: フォントプロパティを指定する定義済みシンボル */
/*** @{ */
/*=*/

/***en
    @brief Key of font property specifying foundry.

    The variable #Mfoundry is a symbol of name <tt>"foundry"</tt> and
    is used as a key of font property and face property.  The property
    value must be a symbol whose name is a foundry name of a font.  */
/***ja
    @brief 開発元を指定するフォントプロパティのキー.
    
    変数 #Mfoundry は <tt>"fonudry"</tt> 
    という名前を持つシンボルであり、フォントプロパティとフェースプロパティのキーとして用いられる。
    値は、フォントの開発元名を名前として持つシンボルである。    */

MSymbol Mfoundry;

/***en
    @brief Key of font property specifying family.

    The variable #Mfamily is a symbol of name <tt>"family"</tt> and is
    used as a key of font property and face property.  The property
    value must be a symbol whose name is a family name of a font.  */ 
/***ja
    @brief ファミリを指定するフォントプロパティのキー.
    
    変数 #Mfamily は <tt>"family"</tt> 
    という名前を持つシンボルであり、フォントプロパティとフェースプロパティのキーとして用いられる。
    値は、フォントのファミリ名を名前として持つシンボルである。    */

MSymbol Mfamily;

/***en
    @brief Key of font property specifying weight.

    The variable #Mweight is a symbol of name <tt>"weight"</tt> and is
    used as a key of font property and face property.  The property
    value must be a symbol whose name is a weight name of a font (e.g
    "medium", "bold").  */ 
/***ja
    @brief 太さを指定するフォントプロパティのキー.
    
    変数 #Mweight は <tt>"weight"</tt> 
    という名前を持つシンボルであり、フォントプロパティとフェースプロパティのキーとして用いられる。
    値は、フォントの太さ名 ( "medium", "bold" 等) を名前として持つシンボルである。    */

MSymbol Mweight;

/***en
    @brief Key of font property specifying style.

    The variable #Mstyle is a symbol of name <tt>"style"</tt> and is
    used as a key of font property and face property.  The property
    value must be a symbol whose name is a style name of a font (e.g
    "r", "i", "o").  */ 
/***ja
    @brief スタイルを指定するフォントプロパティのキー.
    
    変数 #Mstyle は <tt>"style"</tt> 
    という名前を持つシンボルであり、フォントプロパティとフェースプロパティのキーとして用いられる。
    値は、フォントのスタイル名 ("r", "i", "o" 等)を名前として持つシンボルである。    */

MSymbol Mstyle;

/***en
    @brief Key of font property specifying stretch.

    The variable #Mstretch is a symbol of name <tt>"stretch"</tt> and
    is used as a key of font property and face property.  The property
    value must be a symbol whose name is a stretch name of a font (e.g
    "normal", "condensed").  */ 
/***ja
    @brief 幅を指定するフォントプロパティのキー.
    
    変数 #Mstretch は <tt>"stretch"</tt> 
    という名前を持つシンボルであり、フォントプロパティとフェースプロパティのキーとして用いられる。
    値は、フォントの文字幅名 ( "normal", "condensed" 等)を名前として持つシンボルである。    */

MSymbol Mstretch;

/***en
    @brief Key of font property specifying additional style.

    The variable #Madstyle is a symbol of name <tt>"adstyle"</tt> and
    is used as a key of font property and face property.  The property
    value must be a symbol whose name is an additional style name of a
    font (e.g "serif", "", "sans").  */ 
/***ja
    @brief adstyle を指定するフォントプロパティのキー.
    
    変数 #Madstyle は <tt>"adstyle"</tt> 
    という名前を持つシンボルであり、フォントプロパティとフェースプロパティのキーとして用いられる。
    値は、フォントの adstyle 名("serif", "", "sans" 等)を名前として持つシンボルである。    */

MSymbol Madstyle;

/***en
    @brief Key of font property specifying spacing.

    The variable #Madstyle is a symbol of name <tt>"spacing"</tt> and
    is used as a key of font property.  The property value must be a
    symbol whose name specifies the spacing of a font (e.g "p" for
    proportional, "m" for monospaced).  */ 
/***ja
    @brief spacing を指定するフォントプロパティのキー.
    
    変数 #Mspacing は <tt>"spacing"</tt> という名前を持つシンボルであり、
    フォントプロパティのキーとして用いられる。値は、フォントの spacing
    特性を示す名前 ("p", "m" 等)を持つシンボルである。  */

MSymbol Mspacing;

/***en
    @brief Key of font property specifying registry.

    The variable #Mregistry is a symbol of name <tt>"registry"</tt>
    and is used as a key of font property.  The property value must be
    a symbol whose name is a registry name a font registry
    (e.g. "iso8859-1", "jisx0208.1983-0").  */ 
/***ja
    @brief レジストリを指定するフォントプロパティのキー.
    
    変数 #Mregistry は <tt>"registry"</tt> 
    という名前を持つシンボルであり、フォントプロパティとフェースプロパティのキーとして用いられる。
    値は、フォントのレジストリ名 ( "iso8859-1", "jisx0208.1983-0" 
    等) を名前として持つシンボルである。    */

MSymbol Mregistry;

/***en
    @brief Key of font property specifying size.

    The variable #Msize is a symbol of name <tt>"size"</tt> and is
    used as a key of font property and face property.  The property
    value must be an integer specifying a font design size in the unit
    of 1/10 point (on 100 dpi display).  */ 
/***ja
    @brief サイズを指定するフォントプロパティのキー.
    
    変数 #Msize は <tt>"size"</tt> 
    という名前を持つシンボルであり、フォントプロパティとフェースプロパティのキーとして用いられる。値は、
    100 dpi のディスプレイ上でのフォントのデザインサイズを 1/10 
    ポイント単位で示す整数値である。
    */

MSymbol Msize;

/***en
    @brief Key of font property specifying file name.

    The variable #Mfontfile is a symbol of name <tt>"fontfile"</tt>
    and is used as a key of font property.  The property value must be
    a symbol whose name is a font file name.  */ 
/***ja
    @brief フォントファイルを指定するフォントプロパティのキー.
    
    変数 #Mfontfile は <tt>"fontfile"</tt> という名前を持つシンボルであ
    り、フォントプロパティのキーとして用いられる。値は、フォントファイ
    ル名を名前として持つとするシンボルである。 */

MSymbol Mfontfile;

/***en
    @brief Key of font property specifying resolution.

    The variable #Mresolution is a symbol of name <tt>"resolution"</tt> and
    is used as a key of font property and face property.  The property
    value must be an integer to specifying a font resolution in the
    unit of dots per inch (dpi).  */ 
/***ja
    @brief 解像度を指定するフォントプロパティのキー.
    
    変数 #Mresolution は <tt>"resolution"</tt> 
    という名前を持つシンボルであり、フォントプロパティとフェースプロパティのキーとして用いられる。
    値は、フォントの解像度を dots per inch (dpi) 単位で示す整数値である。    */

MSymbol Mresolution;

/***en
    @brief Key of font property specifying max advance width.

    The variable #Mmax_advance is a symbol of name
    <tt>"max-advance"</tt> and is used as a key of font property.  The
    property value must be an integer specifying a font's max advance
    value by pixels.  */ 

MSymbol Mmax_advance;


/***en
    @brief Symbol of name "fontconfig".

    The variable #Mfontconfig is to be used as an argument of the
    functions mfont_parse_name () and mfont_unparse_name ().  */
/***ja
    @brief "fontconfig" という名前を持つシンボル.

    変数 #Mfontconfig は関数 mfont_parse_name () と mfont_unparse_name ()
    の引数として用いられる。  */

MSymbol Mfontconfig;

/***en
    @brief Symbol of name "x".

    The variable #Mx is to be used for a value of <type> member of the
    structure #MDrawGlyph to specify the type of <fontp> member is
    actually (XFontStruct *).  */
/***ja
    @brief "x" という名前を持つシンボル.

    変数 #Mx は構造 #MDrawGlyph のメンバ <type> 
    の値として用いられ、メンバ <fontp> の型が実際には (XFontStruct *) であることを表す.  */

MSymbol Mx;

/***en
    @brief Symbol of name "freetype".

    The variable #Mfreetype is to be used for a value of <type> member
    of the structure #MDrawGlyph to specify the type of <fontp> member
    is actually FT_Face.  */
/***ja
    @brief "freetype" という名前を持つシンボル.

    変数 #Mfreetype は構造 #MDrawGlyph のメンバ <type> 
    の値として用いられ、メンバ <fontp> の型が実際には FT_Face であることを表す。  */

MSymbol Mfreetype;

/***en
    @brief Symbol of name "xft".

    The variable #Mxft is to be used for a value of <type> member of the
    structure #MDrawGlyph to specify the type of <fontp> member
    is actually (XftFont *).  */
/***ja
    @brief  "xft" という名前を持つシンボル.

    変数 #Mxft は構造 #MDrawGlyph のメンバ <type> 
    の値として用いられ、メンバ <fontp> の型が実際には (XftFont *) であることを表す。  */

MSymbol Mxft;

/*=*/
/*** @} */
/*=*/

/***en
    @brief List of font files and directories that contain font files.

    The variable @c mfont_freetype_path is a plist of FreeType font
    files and directories that contain FreeType font files.  Key of
    the element is @c Mstring, and the value is a string that
    represents a font file or a directory.

    The macro M17N_INIT () sets up this variable to contain the
    sub-directory "fonts" of the m17n database and the environment
    variable "M17NDIR".  The first call of mframe () creates the
    internal list of the actually available fonts from this variable.
    Thus, an application program, if necessary, must modify the
    variable before calling mframe ().  If it is going to add a new
    element, value must be a string that can be safely freed.

    If the m17n library is not configured to use the FreeType library,
    this variable is not used.  */
/***ja
    @brief フォントファイルとフォントファイルを含むディレクトリのリスト.

    変数 @c mfont_freetype_path は、フォントファイルとフォントファイルを含むディレクトリの 
    plist である。各要素のキーは @c Mstring 
    であり、値はフォントファイルかディレクトリを示す文字列である。

    マクロ M17N_INIT () によって、この変数は m17n データベースと環境変数 
    "M17NDIR" 双方のサブディレクトリ "fonts" を含むように設定される。
    mframe () の最初の呼び出しの際に、この変数から実際に使用できるフォントの内部リストが作られる。
    そこでアプリケーションプログラムは、mframe () 
    を呼ぶ前に（必要ならば）この変数を変更しなくてはならない。
    新しい要素を追加する場合には、その値は安全に開放できる文字列でなくてはならない。

    m17n ライブラリが FreeType ライブラリを使うように設定されてない場合には、この変数は用いられない。 */

MPlist *mfont_freetype_path;

/*=*/

/***en
    @brief Create a new font.

    The mfont () function creates a new font object that has no
    property.

    @return
    This function returns a pointer to the created font object.  */
/***ja
    @brief 新しいフォントを作る.

    関数 mfont () はプロパティを一切持たない新しいフォントをオブジェクトを作る。

    @return
    この関数は作ったフォントオブジェクトへのポインタを返す。  */

MFont *
mfont ()
{
  MFont *font;

  MSTRUCT_CALLOC (font, MERROR_FONT);
  return font;
}

/*=*/

/***en
    @brief Create a font by parsing a fontname.

    The mfont_parse_name () function creates a new font object.  The
    properties are extracted fontname $NAME.

    $FORMAT specifies the format of $NAME.  If $FORMAT is #Mx, $NAME
    is parsed as XLFD (X Logical Font Description).  If $FORMAT is
    #Mfontconfig, $NAME is parsed as Fontconfig's textual
    representation of font.  If $FORMAT is #Mnil, $NAME is at first
    parsed as XLFD, and it it fails, parsed as Fontconfig's
    representation.

    @return
    If the operation was successful, this function returns a pointer
    to the created font.  Otherwise it returns @c NULL.  */

/***ja
    @brief フォント名からフォントを作る.

    関数 mfont_parse_name () は、フォント名 
    $NAME から取り出されたプロパティを持つ、新しいフォントオブジェクトを作る。

    $FORMAT は $NAME のフォーマットを指定する。$FORMAT が #Mx であれば、
    $NAME は XLFD (X Logical Font Description) に従って解析される。
    $FORMAT が #Mfontconfig であれば $NAME は Fontfonfig 
    のフォントテキスト表現に従って解析される。$FORMAT が #Mnil であれば、まず XLFD 
    に従って解析され、それに失敗したら Fontconfig に従って解析される。

    @return
    処理が成功すれば mfont_parse_name () 
    は新しく作られたフォントへのポインタを返す。そうでなければ @c NULL を返す。  */

MFont *
mfont_parse_name (const char *name, MSymbol format)
{
  MFont template, *font;
  
  MFONT_INIT (&template);
  if (mfont__parse_name_into_font (name, format, &template) < 0)
    MERROR (MERROR_FONT, NULL);
  MSTRUCT_CALLOC (font, MERROR_FONT);
  *font = template;
  return font;
}

/*=*/

/***en
    @brief Create a fontname from a font.

    The mfont_unparse_name () function creates a fontname string
    from font $FONT according to $FORMAT.

    $FORMAT must be #Mx or #Mfontconfig.  If it is #Mx, the fontname
    is in XLFD (X Logical Font Description) format.  If it is
    #Mfontconfig, the fontname is in the style of Fontconfig's text
    representation.

    @return
    This function returns a newly allocated fontname string, which is
    not freed unless the user explicitly does so by free ().  */

/***ja
    @brief フォントからフォント名を作る.

    関数 mfont_unparse_name () は フォント名の文字列をフォント $FONT 
    を元に$FORMAT に従って作る。

    $FORMAT は #Mx または #Mfontconfig である。
    #Mx ならばフォント名は XLFD (X Logical Font Description) に従う。
    #Mfontconfig ならばフォント名は Fontconfig のフォントテキスト表現に従う。

    @return 
    この関数は新たにアロケートしたフォント名の文字列を返す。文字列は、ユーザが
    free () によって明示的に解放しない限り解放されない。  */

char *
mfont_unparse_name (MFont *font, MSymbol format)
{
  char *name;

  if (format == Mx)
    name = xlfd_unparse_name (font, 1);
#ifdef HAVE_FONTCONFIG
  else if (format == Mfontconfig)
    name = mfont__ft_unparse_name (font);

#endif /* HAVE_FONTCONFIG */
  else
    MERROR (MERROR_FONT, NULL);
  return name;
}

/*=*/

/***en
    @brief Make a copy of a font.

    The mfont_copy () function returns a new copy of font $FONT.  */
/***ja
    @brief フォントのコピーを作る.

    関数 Mfont_copy () はフォント $FONT のコピーを作り、それを返す。 */

MFont *
mfont_copy (MFont *font)
{
  MFont *copy;

  MSTRUCT_MALLOC (copy, MERROR_FONT);
  *copy = *font;
  return copy;
}

/*=*/

/***en
    @brief Get a property value of a font.

    The mfont_get_prop () function gets the value of $KEY property of
    font $FONT.  $KEY must be one of the following symbols:

	@c Mfamily, @c Mweight, @c Mstyle, @c Mstretch, @c Madstyle,
	@c Mregistry, @c Msize, @c Mresolution, @c Mspacing.

    If $FONT is a return value of mfont_find (), $KEY can also be one
    of the following symbols:

	#Mfont_ascent, #Mfont_descent, #Mmax_advance.

    @return If $KEY is @c Mfamily, @c Mweight, @c Mstyle, @c Mstretch,
    @c Madstyle, @c Mregistry, or @c Mspacing, this function returns
    the corresponding value as a symbol.  If the font does not have
    $KEY property, it returns @c Mnil.  If $KEY is @c Msize, @c
    Mresolution, #Mfont_ascent, Mfont_descent, or #Mmax_advance, this
    function returns the corresponding value as an integer.  If the
    font does not have $KEY property, it returns 0.  If $KEY is
    something else, it returns @c NULL and assigns an error code to
    the external variable #merror_code.  */
 
/***ja
    @brief フォントのプロパティの値を得る.

    関数 mfont_get_prop () はフォント $FONT のプロパティのうち、キーが
    $KEY であるものの値を返す。$KEY は以下のシンボルのいずれかでなけれ
    ばならない。

	@c Mfamily, @c Mweight, @c Mstyle, @c Mstretch,
	@c Madstyle, @c Mregistry, @c Msize, @c Mresolution, @c Mspacing.

    @return $KEY が @c Mfamily, @c Mweight, @c Mstyle, @c Mstretch, @c
    Madstyle, @c Mregistry, @c Mspacing のいずれかであれば、相当する値
    をシンボルとして返す。フォントがそのプロパティを持たない場合には
    @c Mnil を返す。$KEY が @c Msize あるいは @c Mresolution の場合には、
    相当する値をは整数値として返す。フォントがそのプロパティを持たない
    場合には 0 を返す。$KEY がそれ以外のものであれば、@c NULL を返し、
    外部変数 #merror_code にエラーコードを設定する。  */

void *
mfont_get_prop (MFont *font, MSymbol key)
{
  MRealizedFont *rfont = NULL;

  if (font->type == MFONT_TYPE_REALIZED)
    rfont = (MRealizedFont *) font;

  if (key == Mfoundry)
    return (void *) FONT_PROPERTY (font, MFONT_FOUNDRY);
  if (key == Mfamily)
    return (void *) FONT_PROPERTY (font, MFONT_FAMILY);
  if (key == Mweight)
    return (void *) FONT_PROPERTY (font, MFONT_WEIGHT);
  if (key == Mstyle)
    return (void *) FONT_PROPERTY (font, MFONT_STYLE);
  if (key == Mstretch)
    return (void *) FONT_PROPERTY (font, MFONT_STRETCH);
  if (key == Madstyle)
    return (void *) FONT_PROPERTY (font, MFONT_ADSTYLE);
  if (key == Mregistry)
    return (void *) FONT_PROPERTY (font, MFONT_REGISTRY);
  if (key == Msize)
    {
      int size = font->size;
      return (void *) size;
    }
  if (key == Mresolution)
    {
      int resy = font->property[MFONT_RESY];
      return (void *) resy;
    }
  if (key == Mfontfile)
    return (void *) font->file;
  if (key == Mspacing)
    return (font->spacing == MFONT_SPACING_UNDECIDED ? Mnil
	    : msymbol (font->spacing == MFONT_SPACING_PROPORTIONAL ? "p"
		       : font->spacing == MFONT_SPACING_MONO ? "m" : "c"));
  if (rfont)
    {
      if (key == Mfont_ascent)
	return (void *) rfont->ascent;
      if (key == Mfont_descent)
	return (void *) rfont->descent;
      if (key == Mmax_advance)
	return (void *) rfont->max_advance;
    }
  MERROR (MERROR_FONT, NULL);
}


/*=*/
/***en
    @brief Put a property value to a font.

    The mfont_put_prop () function puts a font property whose key is
    $KEY and value is $VAL to font $FONT.  $KEY must be one of the
    following symbols:

	@c Mfamily, @c Mweight, @c Mstyle, @c Mstretch,
	@c Madstyle, @c Mregistry, @c Msize, @c Mresolution.

    If $KEY is @c Msize or @c Mresolution, $VAL must be an integer.
    Otherwise, $VAL must be a symbol.  */
 /***ja
    @brief フォントのプロパティに値を設定する.

    関数 mfont_put_prop () は、フォント $FONT のキーが$KEY 
    であるプロパティの値を $VAL に設定する。$KEY は以下のシンボルのいずれかである。

	@c Mfamily, @c Mweight, @c Mstyle, @c Mstretch,
	@c Madstyle, @c Mregistry, @c Msize, @c Mresolution.

    $KEY が @c Msize か @c Mresolution であれば $VAL 
    は整数値でなくてはらない。それ以外の場合、$VAL はシンボルでなくてはならない。*/

int
mfont_put_prop (MFont *font, MSymbol key, void *val)
{
  if (key == Mfoundry)
    mfont__set_property (font, MFONT_FOUNDRY, (MSymbol) val);
  else if (key == Mfamily)
    mfont__set_property (font, MFONT_FAMILY, (MSymbol) val);
  else if (key == Mweight)
    mfont__set_property (font, MFONT_WEIGHT, (MSymbol) val);
  else if (key == Mstyle)
    mfont__set_property (font, MFONT_STYLE, (MSymbol) val);
  else if (key == Mstretch)
    mfont__set_property (font, MFONT_STRETCH, (MSymbol) val);
  else if (key == Madstyle)
    mfont__set_property (font, MFONT_ADSTYLE, (MSymbol) val);
  else if (key == Mregistry)
    mfont__set_property (font, MFONT_REGISTRY, (MSymbol) val);
  else if (key == Msize)
    {
      int size = (int) val;
      font->size = size;
    }
  else if (key == Mresolution)
    {
      unsigned resy = (unsigned) val;
      font->property[MFONT_RESY] = resy;
    }
  else if (key == Mlanguage)
    {
      char *langname = MSYMBOL_NAME ((MSymbol) val);
      int len = MSYMBOL_NAMELEN ((MSymbol) val);
      
      if (len <= 3)
	{
	  char buf[10];

	  sprintf (buf, ":lang=%s", langname);
	  font->capability = msymbol (buf);
	}
    }
  else if (key == Mfontfile)
    {
      font->file = (MSymbol) val;
    }
  else
    MERROR (MERROR_FONT, -1);
  return 0;
}

/*=*/

/***en
    @brief Return the font selection priority.

    The mfont_selection_priority () function returns a newly created
    array of six symbols.  The elements are the following
    keys of font properties ordered by priority.

	@c Mfamily, @c Mweight, @c Mstyle, @c Mstretch,
	@c Madstyle, @c Msize.

   The m17n library selects the best matching font according to the
   order of this array.  A font that has a different value for a
   property of lower priority is preferred to a font that has a
   different value for a property of higher priority.  */
/***ja
    @brief フォント選択の優先度を返す.

    関数 mfont_selection_priority () は 6 つのシンボルからなる配列を作って返す。
    配列の要素は、以下のフォントプロパティのキーを優先度順に並べたものである。

	@c Mfamily, @c Mweight, @c Mstyle, @c Mstretch,
	@c Madstyle, @c Msize.

   m17n ライブラリはこの配列に従って、最も合致するフォントを選択する。
   目的のフォントと、それぞれ違うプロパティの値が合致しないフォントがあった場合、優先度の低いプロパティの値が合致しないフォント（優先度の高いプロパティの値が合致しているフォント）が選択される。

   */

MSymbol *
mfont_selection_priority ()
{
  MSymbol *keys;
  int i;

  MTABLE_MALLOC (keys, FONT_SCORE_PRIORITY_SIZE, MERROR_FONT);
  for (i = 0; i < FONT_SCORE_PRIORITY_SIZE; i++)
    {
      enum MFontProperty prop = font_score_priority[i];

      if (prop == MFONT_SIZE)
	keys[i] = Msize;
      else if (prop == MFONT_ADSTYLE)
	keys[i] = Madstyle;
      else if (prop == MFONT_FAMILY)
	keys[i] = Mfamily;
      else if (prop == MFONT_WEIGHT)
	keys[i] = Mweight;
      else if (prop == MFONT_STYLE)
	keys[i] = Mstyle;
      else if (prop == MFONT_STRETCH)
	keys[i] = Mstretch;
      else
	keys[i] = Mfoundry;
    }
  return keys;
}

/*=*/

/***en
    @brief Set the font selection priority.

    The mfont_set_selection_priority () function sets font selection
    priority according to $KEYS, which is an array of six symbols.
    Each element must be one of the below.  No two elements must be
    the same.

	@c Mfamily, @c Mweight, @c Mstyle, @c Mstretch,
	@c Madstyle, @c Msize.

    See the documentation of the function mfont_selection_priority ()
    for details.  */
/***ja
    @brief フォント選択優先度を設定する.

    関数 mfont_set_selection_priority () は、6つのシンボルの配列 $KEYS 
    にしたがってフォント選択優先度を設定する。配列は以下の各要素を適切
    な順番で並べたものである。

	@c Mfamily, @c Mweight, @c Mstyle, @c Mstretch,
	@c Madstyle, @c Msize.

    詳細は関数 mfont_selection_priority () の説明を参照のこと。
     */

int
mfont_set_selection_priority (MSymbol *keys)
{
  int priority[FONT_SCORE_PRIORITY_SIZE];
  int i, j, shift;

  for (i = 0; i < FONT_SCORE_PRIORITY_SIZE; i++, keys++)
    {
      enum MFontProperty prop;

      if (*keys == Msize)
	prop = MFONT_SIZE;
      else if (*keys == Madstyle)
	prop = MFONT_ADSTYLE;
      else if (*keys == Mfamily)
	prop = MFONT_FAMILY;
      else if (*keys == Mweight)
	prop = MFONT_WEIGHT;
      else if (*keys == Mstyle)
	prop = MFONT_STYLE;
      else if (*keys == Mstretch)
	prop = MFONT_STRETCH;
      else if (*keys == Mfoundry)
	prop = MFONT_FOUNDRY;
      else
	/* Invalid element.  */
	return -1;
      for (j = 0; j < i; j++)
	if (priority[j] == prop)
	  /* Duplicated element.  */
	  return -1;
      priority[i] = prop;
    }
  for (i = 0; i < FONT_SCORE_PRIORITY_SIZE; i++)
    font_score_priority[i] = priority[i];
  /* Here, SHIFT starts from 1, not 0.  This is because the lowest bit
     of a score is a flag for a scalable font (see the documentation
     of font_score).  */
  i = FONT_SCORE_PRIORITY_SIZE - 1;
  for (shift = 1; i >= 0; i--)
    {
      font_score_shift_bits[font_score_priority[i]] = shift;
      if (font_score_priority[i] == MFONT_SIZE)
	shift += 16;
      else if (font_score_priority[i] <= MFONT_FAMILY)
	shift++;
      else
	shift += 2;
    }
  return 0;
}

/*=*/

/***en
    @brief Find a font.

    The mfont_find () function returns a pointer to the available font
    that matches best the specification $SPEC on frame $FRAME.

    $SCORE, if not NULL, must point to a place to store the score
    value that indicates how well the found font matches to $SPEC.  The
    smaller score means a better match.  */
/***ja
    @brief フォントを探す.

    関数 mfont_find () は、フレーム $FRAME 上でフォント定義 $SPEC 
    にもっとも合致する利用可能なフォントへのポインタを返す。  

    $SCORE は NULL であるか、見つかったフォントが $SPEC 
    にどれほど合っているかを示すスコアを保存する場所へのポインタである。
    スコアが小さいほど良く合っていることを意味する。
    */

MFont *
mfont_find (MFrame *frame, MFont *spec, int *score, int max_size)
{
  MFont spec_copy;
  MFont *best;
  MFontList *list;
  MRealizedFont *rfont;
  MFont adjusted;

  if (spec->size < 0)
    {
      double pt = - spec->size;

      adjusted = *spec;
      adjusted.size = pt * frame->dpi / 72.27 + 0.5;
      spec = &adjusted;
    }
  MFONT_INIT (&spec_copy);
  spec_copy.property[MFONT_FAMILY] = spec->property[MFONT_FAMILY];
  spec_copy.property[MFONT_REGISTRY] = spec->property[MFONT_REGISTRY];
  spec_copy.capability = spec->capability;
  spec_copy.file = spec->file;

  list = mfont__list (frame, &spec_copy, spec, max_size);
  if (! list)
    return NULL;

  best = list->fonts[0].font;
  if (score)
    *score = list->fonts[0].score;
  free (list->fonts);
  free (list);
  spec_copy = *best;
  mfont__merge (&spec_copy, spec, 0);
  rfont = mfont__open (frame, best, spec);
  if (! rfont)
    return NULL;
  return (MFont *) rfont;
}

/*=*/
/***en
    @brief Set encoding of a font.

    The mfont_set_encoding () function sets the encoding information
    of font $FONT.

    $ENCODING_NAME is a symbol representing a charset that has the
    same encoding as the font.

    $REPERTORY_NAME is @c Mnil or a symbol representing a charset that
    has the same repertory as the font.  If it is @c Mnil, whether a
    specific character is supported by the font is asked to each font
    driver.

    @return
    If the operation was successful, this function returns 0.
    Otherwise it returns -1 and assigns an error code to the external
    variable #merror_code.  */
/***ja
    @brief フォントのエンコーディングを設定する.

    関数 mfont_set_encoding () はフォント $FONT のエンコーディング情報を設定する。

    $ENCODING_NAME はフォントと同じエンコーディングを持つ文字セットを示すシンボルである。

    $REPERTORY_NAME は @c Mnil であるか、フォントと同じエンコーディングを持つ文字セットを示すシンボルである。
    @c Mnil であれば、個々の文字がそのフォントでサポートされているかどうかは、各々のフォントドライバに問い合わせる。

    @return
    処理が成功すればこの関数は 0 を返す。そうでなければ -1 を返し、外部変数
    #merror_code にエラーコードを設定する。  */


int
mfont_set_encoding (MFont *font, MSymbol encoding_name, MSymbol repertory_name)
{
  MCharset *encoding_charset = MCHARSET (encoding_name);
  MCharset *repertory_charset;
  MSymbol registry;
  MFontEncoding *encoding;
  MPlist *plist;

  if (! encoding_charset)
    MERROR (MERROR_FONT, -1);
  if (repertory_name != Mnil)
    {
      repertory_charset = MCHARSET (repertory_name);
      if (! repertory_charset)
	MERROR (MERROR_FONT, -1);
    }
  else
    repertory_charset = NULL;

  MSTRUCT_CALLOC (encoding, MERROR_FONT);
  encoding->spec = *font;
  encoding->encoding_name = encoding_name;
  encoding->encoding_charset = encoding_charset;
  encoding->repertory_name = repertory_name;
  encoding->repertory_charset = repertory_charset;
  registry = FONT_PROPERTY (font, MFONT_REGISTRY);
  if (registry == Mnil)
    registry = Mt;
  if (! font_encoding_list)
    load_font_encoding_table ();
  mplist_push (font_encoding_list, registry, encoding);
  MPLIST_DO (plist, MPLIST_NEXT (font_encoding_list))
    if (! memcmp (font, &((MFontEncoding *) MPLIST_VAL (plist))->spec,
		  sizeof (MFont)))
      {
	mplist_pop (plist);
	break;
      }
  return 0;
}

/*=*/

/***en
    @brief Create a fontname from a font.

    This function is obsolete.   Use mfont_unparse_name instead. */
/***ja
    @brief フォント名からフォントを作る.

    この関数は廃止予定である。 mfont_unparse_name () を使用のこと。 */

char *
mfont_name (MFont *font)
{
  return mfont_unparse_name (font, Mx);
}

/*=*/

/***en
    @brief Create a new font from fontname.

    This function is obsolete.  Use mfont_parse_name () instead.  */

/***ja
    @brief フォントからフォント名を作る.

    これは関数は廃止予定である。 mfont_parse_name () を使用のこと。  */

MFont *
mfont_from_name (const char *name)
{
  return mfont_parse_name (name, Mx);
}

/*=*/

/***en
    @brief Get resize information of a font.

    The mfont_resize_ratio () function lookups the m17n database
    \<font, reisize\> and returns a resizing ratio (in percentage) of
    FONT.  For instance, if the return value is 150, that means that
    the m17n library uses an 1.5 time bigger font than a specified
    size.  */

/***ja
    @brief フォントのリサイズ情報を得る

    関数 mfont_resize_ratio は m17n データベース \<font, reisize\> 
    を検索し、フォント FONT のリサイズの比率（パーセンテージ）
    を返す。たとえば返す値が 150 であれば、m17n ライブラリは指定されたサイズの 1.5 
    倍のフォントを使用することを意味する。 */

int
mfont_resize_ratio (MFont *font)
{
  MSymbol registry = FONT_PROPERTY (font, MFONT_REGISTRY);
  MFontResize *resize;
  MPlist *plist;

  if (! font_resize_list)
    load_font_resize_table ();
  if (! MPLIST_TAIL_P (font_resize_list))
    while (1)
      {
	plist = font_resize_list;
	while (registry ? (plist = mplist_find_by_key (plist, registry))
	       : plist)
	  {
	    resize = (MFontResize *) MPLIST_VAL (plist);
	    if (mfont__match_p (font, &resize->spec, MFONT_ADSTYLE))
	      return resize->resize;
	    plist = MPLIST_NEXT (plist);
	  }
	if (registry == Mt)
	  break;
	registry = Mt;
      }
  return 100;
}

/*=*/

/***en
    @brief Get a list of fonts.

    The mfont_list () functions returns a list of fonts available on
    frame $FRAME.  $FONT, if not NULL, limits fonts to ones
    that match with $FONT.  $LANGUAGE, if not @c Mnil, limits fonts to
    ones that support $LANGUAGE.  $MAXNUM, if greater than 0, limits
    the number of fonts.

    @return
    This function returns a plist whose keys are family names and
    values are pointers to the object MFont.  The plist must be freed
    by m17n_object_unref ().  If no font is found, it returns
    NULL.  */

/***ja
    @brief フォントのリストを得る

    関数 mfont_list () はフレーム $FRAME で利用可能なフォントのリストを返す。
    $FONT が NULL でなければ、$FONT と合致する利用可能なフォントのリストを返す。
    $LANGUAGE が @c Mnil でなければ、$LANGUAGE をサポートする利用可能なフォントのリストを返す。
    $MAXNUM は、0 より大きい場合には、返すフォントの数の上限である。

    @return 
    この関数はキーがフォントファミリ名であり値が MFont オブジェクトへのポインタであるような
    plist を返す。plist は m17n_object_unref () 
    で解放する必要がある。フォントが見つからなければNULL を返す。  */

MPlist *
mfont_list (MFrame *frame, MFont *font, MSymbol language, int maxnum)
{
  MPlist *plist, *pl;
  MFontList *font_list;
  int i;
  MFont spec;
  
  if (font)
    spec = *font;
  else
    MFONT_INIT (&spec);

  if (spec.size < 0)
    {
      double pt = - spec.size;

      spec.size = pt * frame->dpi / 72.27 + 0.5;
    }

  if (language != Mnil)
    {
      /* ":lang=XXX" */
      char *buf = alloca (MSYMBOL_NAMELEN (language) + 7);

      sprintf (buf, ":lang=%s", MSYMBOL_NAME (language));
      spec.capability = msymbol (buf);
    }

  font_list = mfont__list (frame, &spec, &spec, 0);
  if (! font_list)
    return NULL;
  if (font_list->nfonts == 0)
    {
      free (font_list);
      return NULL;
    }

  plist = pl = mplist ();
  for (i = 0; i < font_list->nfonts; i++)
    {
      MSymbol family = FONT_PROPERTY (font_list->fonts[i].font, MFONT_FAMILY);

      if (family != Mnil)
	pl = mplist_add (pl, family, font_list->fonts[i].font);
    }
  free (font_list);
  return plist;
}


/*=*/

/***en
    @brief Check the usability of a font.

    The function mfont_check () checkes if $FONT can be used for
    $SCRIPT and $LANGUAGE in $FONTSET on $FRAME.

    @return If the font is usable, return 1.  Otherwise return 0.
 */

int
mfont_check (MFrame *frame, MFontset *fontset,
	     MSymbol script, MSymbol language, MFont *font)
{
  MRealizedFont *rfont;
  int best, score;

  if (! fontset)
    fontset = frame->face->property[MFACE_FONTSET];
  rfont = mfontset__get_font (frame, fontset, script, language, font, &best);
  if (! rfont || ! best)
    return 0;
  score = font_score (&rfont->spec, font);
  return (score == 0 ? 2 : 1);
}

/*** @} */

/*** @addtogroup m17nDebug */
/*=*/
/*** @{ */

/***en
    @brief Dump a font.

    The mdebug_dump_font () function prints font $FONT in a human readable
    way to the stderr.

    @return
    This function returns $FONT.  */
/***ja
    @brief フォントをダンプする.

    関数 mdebug_dump_font () はフォント $FONT を stderr 
    に人間に可読な形で印刷する。

    @return
    この関数は $FONT を返す。  */

MFont *
mdebug_dump_font (MFont *font)
{
  char *name;
  
  name = xlfd_unparse_name (font, 0);
  if (name)
    {
      fprintf (stderr, "%s", name);
      free (name);
    }
  if (font->file != Mnil)
    {
      char *file = MSYMBOL_NAME (font->file);
      char *lastslash = file, *p;

      for (p = file; *p; p++)
	if (*p == '/')
	  lastslash = p;
      if (name)
	fprintf (stderr, ",");
      fprintf (stderr, "%s", lastslash + 1);
    }
  if (font->capability != Mnil)
    fprintf (stderr, "%s", MSYMBOL_NAME (font->capability));
  return font;
}

/*** @} */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
