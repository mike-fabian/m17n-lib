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
#ifdef _M17N_H_
#define M17N_INIT()			\
  do {					\
    m17n_init ();			\
    if (merror_code == MERROR_NONE)	\
      m17n_init_flt ();			\
  } while (0)
#else  /* not _M17N_H_ */
#define M17N_INIT() m17n_init_flt ()
#endif	/* not _M17N_H_ */

extern void m17n_fini_flt (void);
#undef M17N_FINI
#ifdef _M17N_H_
#define M17N_FINI()	\
  do {			\
    m17n_fini_flt ();	\
    m17n_fini ();	\
  } while (0)
#else  /* not _M17N_H_ */
#define M17N_FINI() m17n_fini_flt ()
#endif	/* not _M17N_H_ */

#endif

/***en @defgroup m17nFLT FLT API
    @brief API provided by libm17n-flt.so */
/***ja @defgroup m17nFLT FLT API
    @brief libm17n-flt.so が提供する API */
/*=*/

/*** @addtogroup m17nFLT */
/*** @{ */
/*=*/

/***en
    @brief Type of information about a glyph.

    The type #MFLTGlyph is the structure that contains information
    about a glyph.  */

/***ja
    @brief グリフに関する情報の型.

    型 #MFLTGlyph は、グリフに関する情報を格納する構造体である。  */

typedef struct
{
  /***en Character code (Unicode) of the glyph.  This is the sole
      member to be set before calling the functions mflt_find () and
      mflt_run ().  */
  /***ja グリフの (Unicode における) 文字コード。関数
      mflt_find () と mflt_run ()
      を呼び出す前セットすべき唯一のメンバーである。  */
  int c;
  /***en Glyph ID of the glyph in the font.  */
  /***ja フォント内におけるそのグリフの ID。  */
  unsigned int code;
  /***en Starting index of the run in #MFLTGlyphString that is
      replaced by this glyph.  */
  /***ja #MFLTGlyphString
      の中で、このグリフによって置き換えられる部分の先頭のインデクス。  */
  int from;
  /***en Ending index of the run in #MFLTGlyphString that is
      replaced by this glyph.  */
  /***ja #MFLTGlyphString
      の中で、このグリフによって置き換えられる部分の末尾のインデクス。  */
  int to;
  /***en Advance width for horizontal layout expressed in 26.6
      fractional pixel format.  */
  /***ja 横書き時の送り幅を 26.6 fractional pixel format で表現したもの。  */
  int xadv;
  /***en Advance height for vertical layout expressed in 26.6
      fractional pixel format.  */
  /***ja 縦書き時の送り高を 26.6 fractional pixel format で表現したもの。  */
  int yadv;
  /***en Ink metrics of the glyph expressed in 26.6 fractional pixel
      format.  */
  /***ja このグリフのインクメトリックを 26.6 fractional pixel format
      で表現したもの。  */
  int ascent, descent, lbearing, rbearing;
  /***en Horizontal and vertical adjustments for the glyph positioning
      expressed in 26.6 fractional pixel format.  */
  /***ja グリフ位置決めの際の水平・垂直調整値を、
      26.6 fractional pixel format で表現したもの。  */
  int xoff, yoff;
  /***en Flag to tell whether the member \<code\> has already been set
      to a glyph ID in the font.  */
  /***ja メンバー \<code\> に既にグリフ ID
      がセットされているか否かを示すフラグ。  */
  unsigned encoded : 1;
  /***en Flag to tell if the metrics of the glyph (members \<xadv\> thru
      \<rbearing\>) are already calculated.  */
  /***ja メンバー \<xadv\> から \<rbearing\>
      までの各メトリックが既に計算済か否かを示すフラグ。  */
  unsigned measured : 1;
  /***en Flag to tell if the metrics of the glyph is adjusted,
      i.e. \<xadv\> or \<yadv\> is different from the normal size, or
      \<xoff\> or \<yoff\> is nonzero.  */
  /***ja グリフのメトリックが調整済みか否か、
      すなわち以下のうち1つ以上が成立していることを示すフラグ。
      \<xadv\> が標準の値と異なる、
      \<yadv\> が標準の値と異なる、
      \<xoff\> がゼロでない、
      \<yoff\> がゼロでない。  */
  unsigned adjusted : 1;
  /***en For m17n-lib's internal use only.  */
  /***ja m17n-lib 内部作業用。  */
  unsigned internal : 30;

  /* Arbitrary data can follow.  */
} MFLTGlyph;

/*=*/

/***en
    @brief Type of information about a glyph position adjustment.

    The type #MFLTGlyphAdjustment is the structure to store
    information about a glyph metrics/position adjustment.  It is
    given to the callback function #drive_otf of #MFLTFont.  */

/***ja
    @brief グリフ位置調整情報のための型.

    型 #MFLTGlyphAdjustment
    は、グリフのメトリック/位置の調整に関する情報を格納するための構造体であり、
    #MFLTFont の callback 関数 #drive_otf に渡される。  */

typedef struct
{
  /***en Adjustments for advance width for horizontal layout and
      advance height for vertical layout expressed in 26.6 fractional
      pixel format.  */
  /***ja 水平・垂直方向の送り量の調整値を 26.6 fractional pixel format
      で表現したもの。  */
  int xadv, yadv;
  /***en Horizontal and vertical adjustments for glyph positioning
      expressed in 26.6 fractional pixel format.  */
  /***ja グリフ位置決めための水平・垂直調整値を 26.6 fractional pixel
      format で表現したもの。  */
  int xoff, yoff;
  /***en Number of glyphs to go back for drawing a glyph.  */
  /***ja グリフ描画のために戻るべきグリフ数。  */
  short back;
  /***en If nonzero, the member \<xadv\> and \<yadv\> are absolute, i.e.,
      they should not be added to a glyph's origianl advance width and
      height.  */
  /***ja 非ゼロのとき、メンバー \<xadv\> と \<yadv\> は絶対値である。
      すなわちその値をグリフ本来の送り幅に加算してはならない。  */
  unsigned advance_is_absolute : 1;
  /***en Should be set to 1 iff at least one of the other members has
      a nonzero value.  */
  /***ja 他のメンバーのうち最低1個が非ゼロのときのみ、1にセットされる。  */
  unsigned set : 1;
} MFLTGlyphAdjustment;

/*=*/

/***en
    @brief Type of information about a glyph sequence.

    The type #MFLTGlyphString is the structure that contains
    information about a sequence of glyphs.  */

/***ja
    @brief グリフ列の情報のための型.

    型 #MFLTGlyphString は、グリフ列の情報を格納するための構造体である。  */

typedef struct
{
  /***en The actual byte size of elements of the array pointed by the
      member #glyphs.  It must be equal to or greater than "sizeof
      (MFLTGlyph)".  */
  /***ja メンバー #glyphs の指す配列の要素が占める実バイト数。
      この値は "sizeof (MFLTGlyph)" 以上でなければならない。  */
  int glyph_size;
  /***en Array of glyphs.  */
  /***ja グリフの配列。  */
  MFLTGlyph *glyphs;
  /***en Number of elements allocated in #glyphs.  */
  /***ja #glyphs 内に配置されている要素の数。  */
  int allocated;
  /***en Number of elements in #glyphs in use.  */
  /***ja #glyphs 内で使用中の要素の数。  */
  int used;
  /***en Flag to tell if the glyphs should be drawn from right-to-left
      or not.  */
  /***ja グリフが右から左へと描かれるべきか否かを示すフラグ。  */
  unsigned int r2l;
} MFLTGlyphString;

/*=*/

/***en
    @brief Type of specification of GSUB and GPOS OpenType tables.

    The type #MFLTOtfSpec is the structure that contains information
    about the GSUB and GPOS features of a specific script and language
    system to be applied to a glyph sequence.  */

/***ja
    @brief GSUB および GPOS OpenType テーブルの仕様のための型.

    型 #MFLTOtfSpec は、GSUB および GPOSフィーチャーの情報を格納するた
    めの構造体である。これらフィーチャーは特定のスクリプトおよび言語シ
    ステムのものであり、グリフ列に適用される。  */

typedef struct
{
  /***en Unique symbol representing the spec.  This is the same as the
      #OTF-SPEC of the FLT.  */
  /***ja この仕様を表わすユニークなシンボル。
      FLT の #OTF-SPEC と同一の値である。  */
  MSymbol sym;

  /***en Tags for script and language system.  */
  /***ja スクリプトおよび言語システムのタグ。  */
  unsigned int script, langsys;

  /***en Array of GSUB (1st element) and GPOS (2nd element) features.
      Each array is terminated by 0.  If an element is 0xFFFFFFFF
      apply the previous features in that order, and apply all the
      other features except those that appear in the following elements.
      It may be NULL if there are no features.  */
  /***ja GSUB フィーチャーを第1要素、GPOS フィーチャーを第2要素とする配
      列。各配列の末尾は0で示される。もしある要素が 0xFFFFFFFFならば、
      以前の全フィーチャーをその順序で適用し、更に以降の要素として現わ
      れるフィチャー以外のすべてを適用する。フィーチャーが1つもない場合
      は NULL でもよい。  */
  unsigned int *features[2];
} MFLTOtfSpec;

/*=*/

/***en
    @brief Type of font to be used by the FLT driver.

    The type #MFLTFont is the structure that contains information
    about a font used by the FLT driver.  */

/***ja
    @brief FLT ドライバが使うフォントの型.

    型 #MFLTFont は、FLTドライバが使うフォントに関する情報を格納するた
    めの構造体である。  */

typedef struct _MFLTFont
{
  /***en Family name of the font.  It may be #Mnil if the family name
     is not important in finding a Font Layout Table suitable for the
     font (for instance, in the case that the font is an OpenType
     font).  */
  /***ja フォントのファミリー名。フォントに適した FLTを探す際に重要でな
      い場合 (たとえば OpenTypeフォントの場合など) は、#Mnil でよい。 */
  MSymbol family;

  /***en Horizontal and vertical font sizes in pixels per EM.  */
  /***ja フォントの水平・垂直サイズを pixels per EM で表現したもの。  */
  int x_ppem, y_ppem;

  /***en Callback function to get glyph IDs for glyphs between FROM
     (inclusive) and TO (exclusive) of GSTRING.  If the member \<encoded\>
     of a glyph is zero, the member \<code\> of that glyph is a character
     code.  The function must convert it to the glyph ID of FONT.  */
  /***ja GSTRING 内の FROM から TO 直前までの各グリフに対応するグリフ
      IDを取得するための callback 関数。もしあるグリフのメンバー
      \<encoded\>がゼロならば、そのグリフのメンバー \<code\> は文字コードで
      ある。この関数はその文字コードを FONT のグリフ IDに変換しなくては
      ならない。  */
  int (*get_glyph_id) (struct _MFLTFont *font, MFLTGlyphString *gstring,
		       int from, int to);

  /***en Callback function to get metrics of glyphs between FROM
     (inclusive) and TO (exclusive) of GSTRING.  If the member \<measured\>
     of a glyph is zero, the function must set the members \<xadv\>, \<yadv\>,
     \<ascent\>, \<descent\>, \<lbearing\>, and \<rbearing\> of the glyph.  */
  /***ja GSTRING 内の FROM から TO直前までの各グリフに対応するメトリッ
      クを取得するための callback 関数。もしあるグリフのメンバー
      \<measured\>がゼロならば、この関数はそのグリフのメンバー \<xadv\>,
      \<yadv\>, \<ascent\>, \<descent\>, \<lbearing\>, および \<rbearing\>をセッ
      トしなければならない。  */
  int (*get_metrics) (struct _MFLTFont *font, MFLTGlyphString *gstring,
		     int from, int to);

  /***en Callback function to check if the font has OpenType GSUB/GPOS
     features for a specific script/language.  The function must
     return 1, if the font satisfies SPEC, or 0.  It must be
     NULL if the font does not have OpenType tables.  */
  /***ja フォントがある特定のスクリプト/言語に対する GSUB/GPOS
      OpenTypeフィーチャーを持つか否かを調べる callback 関数。この関数
      はフォントがSPEC を満たすときは 1 を、そうでないときは 0を返さな
      ければならない。フォントが OpenType テーブルを持たないときはNULL
      でなければならない。  */
  int (*check_otf) (struct _MFLTFont *font, MFLTOtfSpec *spec);

  /***en Callback function to apply OpenType features in SPEC to glyphs
     between FROM (inclusive) and TO (exclusive) of IN.  The resulting
     glyphs are appended to the tail of OUT.  If OUT does not
     have a room to store all the resulting glyphs, it must return -2.
     It must be NULL if the font does not have OpenType tables.  */
  /***ja IN 内の FROM から TO 直前までの各グリフに SPEC内の各 OpenType
      フィーチャーを適用するための callback 関数。適用結果のグリフ列は
      OUT の末尾に追加される。OUT が短か過ぎて結果を追加し切れない場合
      は -2 を返さなくてはならない。フォントが OpenType テーブルを持た
      ない場合は NULLでなければならない。  */
  int (*drive_otf) (struct _MFLTFont *font, MFLTOtfSpec *spec,
		    MFLTGlyphString *in, int from, int to,
		    MFLTGlyphString *out, MFLTGlyphAdjustment *adjustment);

  /***en For m17n-lib's internal use only.  It should be initialized
      to NULL.  */
  /***ja m17n-lib の内部作業用。NULL に初値化される。  */
  void *internal;
} MFLTFont;

/*=*/

/***en
    @brief Type of FLT (Font Layout Table).

    The type #MFLT is for an FLT object.  Its internal structure is
    concealed from application programs.  */

/***ja
    @brief FLT (Font Layout Table) の型.

    型 #MFLT は FLT オブジェクトのための型である。
    この内部構造は、アプリケーションプログラムからは隠蔽されている。  */

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
