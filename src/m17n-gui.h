/* m17n-gui.h -- header file for the GUI API of the m17n library.
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

#ifndef _M17N_GUI_H_
#define _M17N_GUI_H_

#ifndef _M17N_H_
#include <m17n.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)

extern void m17n_init_win (void);
#undef M17N_INIT
#define M17N_INIT() m17n_init_win ()

extern void m17n_fini_win (void);
#undef M17N_FINI
#define M17N_FINI() m17n_fini_win ()

#endif

/***en @defgroup m17nGUI GUI API */
/***ja @defgroup m17nGUI GUI API */
/*=*/

/*** @ingroup m17nGUI */
/***en @defgroup m17nFrame Frame */
/***ja @defgroup m17nFrame フレーム */
/*=*/

/*** @ingroup m17nFrame */
/***en
    @brief Type of frames.

    The type #MFrame is for a @e frame object.  Each frame holds
    various information about the corresponding physical display/input
    device.

    The internal structure of the type #MFrame is concealed from an
    application program, and its contents depend on the window system
    in use.  In the m17n-X library, it contains the information about
    @e display and @e screen in the X Window System.  */

/***ja
    @brief フレームの型宣言.

    #MFrame は、@e フレーム オブジェクト用の型である。
    個々のフレームは、それが対応する物理的な表示／入力デバイスの各種情報を保持する。

    #MFrame 型の内部構造は、アプリケーションプログラムからは見えない。
    またその内容は使用するウィンドウシステムに依存する。また m17n-X 
    ライブラリにおけるフレームは、X ウィンドウの @e display と @e screen 
    に関する情報を持つ。      */

typedef struct MFrame MFrame;

/*=*/

extern MSymbol Mdevice;

extern MSymbol Mfont;
extern MSymbol Mfont_width;
extern MSymbol Mfont_ascent;
extern MSymbol Mfont_descent;
extern MFrame *mframe_default;

extern MSymbol Mdisplay;
extern MSymbol Mscreen;
extern MSymbol Mdrawable;
extern MSymbol Mwidget;
extern MSymbol Mdepth;
extern MSymbol Mcolormap;

extern MFrame *mframe (MPlist *plist);

extern void *mframe_get_prop (MFrame *frame, MSymbol key);

/* end of frame module */
/*=*/

/*** @ingroup m17nGUI  */
/***en @defgroup m17nFont Font */
/***ja @defgroup m17nFont フォント */
/*=*/

/*** @ingroup m17nFont */
/***en
    @brief Type of fonts.

    The type #MFont is the structure defining fonts.  It contains
    information about the following properties of a font: foundry,
    family, weight, style, stretch, adstyle, registry, size, and
    resolution.

    This structure is used both for specifying a font in a fontset
    and for storing information about available system fonts.

    The internal structure is concealed from an application program.  */

/***ja
    @brief フォントの型宣言.

    #MFont 型はフォント指定用の構造体であり、フォントのプロパティである
    foundry, family, weight, style, stretch, adstyle, registry,
    size, resolution に関する情報を含む。

    この構造体はフォントセット内のフォントを指定する際と、使用可能なシステムフォントの情報を格納する際の両方に用いられる。

    内部構造はアプリケーションプログラムからは見えない。  */

/***
    @seealso
    mfont (), mfont_from_name (), mfont_find ().  */

typedef struct MFont MFont;

/*=*/

extern MSymbol Mx, Mfreetype, Mxft;

extern MPlist *mfont_freetype_path;

extern MFont *mfont ();

extern MFont *mfont_copy (MFont *font);

extern MFont *mfont_parse_name (const char *name, MSymbol format);

extern char *mfont_unparse_name (MFont *font, MSymbol format);

/* These two are obsolete (from 1.1.0).  */
extern char *mfont_name (MFont *font);
extern MFont *mfont_from_name (const char *name);

extern MSymbol Mfoundry;
extern MSymbol Mfamily;
extern MSymbol Mweight;
extern MSymbol Mstyle;
extern MSymbol Mstretch;
extern MSymbol Madstyle;
extern MSymbol Mspacing;
extern MSymbol Mregistry;
extern MSymbol Msize;
extern MSymbol Mresolution;
extern MSymbol Mmax_advance;
extern MSymbol Mfontfile;

extern MSymbol Mfontconfig;

extern void *mfont_get_prop (MFont *font, MSymbol key);

extern int mfont_put_prop (MFont *font, MSymbol key, void *val);

extern int mfont_set_encoding (MFont *font,
			       MSymbol encoding_name, MSymbol repertory_name);


/*=*/

/***en
    @brief Find a font.

    The mfont_find () function returns a pointer to the available font
    that matches best with the specification $SPEC in frame $FRAME.

    $SCORE, if not NULL, must point to a place to store the score
    value which indicates how well the found font matches $SPEC.  The
    smaller score means a better match.

    $LIMITED_SIZE, if nonzero, forces the font selector to find a
    font not greater than the #Msize property of $SPEC.  */

/***ja
    @brief フォントを探す.

    関数 mfont_find () は、フレーム $FRAME 上でフォント定義 $SPEC 
    にもっとも合致する使用可能なフォントへのポインタを返す。  

    $SCORE は NULL であるか、見つかったフォントが $SPEC 
    にどれほど合っているかを示すスコアを保存する場所へのポインタである。
    スコアが小さいほど良く合っていることを意味する。

    $LIMITED_SIZE が 0 でなければ、$SPEC のプロパティ #Msize 
    より大きくないフォントだけが探される。
*/

extern MFont *mfont_find (MFrame *frame, MFont *spec,
			  int *score, int limited_size);

extern MSymbol *mfont_selection_priority ();

extern int mfont_set_selection_priority (MSymbol *keys);

extern int mfont_resize_ratio (MFont *font);

extern MPlist *mfont_list (MFrame *frame, MFont *font, MSymbol language,
			   int maxnum);

typedef struct MFontset MFontset;

extern int mfont_check (MFrame *frame, MFontset *fontset,
			MSymbol script, MSymbol language, MFont *font);

/* end of font module */
/*=*/

/*** @ingroup m17nGUI  */
/***en @defgroup m17nFontset Fontset */
/***ja @defgroup m17nFontset フォントセット */
/*=*/
/*** @addtogroup m17nFontset
     @{   */
extern MFontset *mfontset (char *name);

extern MSymbol mfontset_name (MFontset *fontset);

extern MFontset *mfontset_copy (MFontset *fontset, char *name);

extern int mfontset_modify_entry (MFontset *fontset,
				  MSymbol language, MSymbol script,
				  MSymbol charset,
				  MFont *spec, MSymbol layouter_name,
				  int how);

extern MPlist *mfontset_lookup (MFontset *fontset, MSymbol script,
				MSymbol language, MSymbol charset);
/*** @}   */
/* end of fontset module */
/*=*/

/*** @ingroup m17nGUI */
/***en @defgroup m17nFace Face */
/***ja @defgroup m17nFace フェース */
/*=*/

/*** @ingroup m17nFace */
/***en
    @brief Type of faces.

    The type #MFace is the structure of face objects.  The internal
    structure is concealed from an application program.  */

/***ja
    @brief フェースの型宣言.

    #MFace 型はフェースオブジェクトのための構造体である。
    内部構造はアプリケーションプログラムからは見えない。  */

typedef struct MFace MFace;
/*=*/

extern MSymbol Mforeground;
extern MSymbol Mbackground;
extern MSymbol Mvideomode;
extern MSymbol Mnormal;
extern MSymbol Mreverse;
extern MSymbol Mhline;
extern MSymbol Mbox;
extern MSymbol Mfontset;
extern MSymbol Mratio;
extern MSymbol Mhook_func;
extern MSymbol Mhook_arg;

/* Predefined faces.  */
extern MFace *mface_normal_video;
extern MFace *mface_reverse_video;
extern MFace *mface_underline;
extern MFace *mface_medium;
extern MFace *mface_bold;
extern MFace *mface_italic;
extern MFace *mface_bold_italic;
extern MFace *mface_xx_small;
extern MFace *mface_x_small;
extern MFace *mface_small;
extern MFace *mface_normalsize;
extern MFace *mface_large;
extern MFace *mface_x_large;
extern MFace *mface_xx_large;
extern MFace *mface_black;
extern MFace *mface_white;
extern MFace *mface_red;
extern MFace *mface_green;
extern MFace *mface_blue;
extern MFace *mface_cyan;
extern MFace *mface_yellow;
extern MFace *mface_magenta;

/* etc */
extern MSymbol Mface;

extern MFace *mface ();

extern int mface_equal (MFace *face1, MFace *face2);

extern MFace *mface_copy (MFace *face);

extern MFace *mface_merge (MFace *dst, MFace *src);

extern MFace *mface_from_font (MFont *font);

/*=*/

/*** @ingroup m17nFace */
/***en
    @brief Type of horizontal line spec of face.

    The type #MFaceHLineProp is to specify the detail of #Mhline
    property of a face.  The value of the property must be a pointer
    to an object of this type.  */
/***ja
    @brief フェースの水平線指定用型宣言.

    #MFaceHLineProp はフェースの #Mhline 
    プロパティの詳細を指定する型である。このプロパティの値はこの型のオブジェクトでなくてはならない。
      */

typedef struct
{
  /***en Type of the horizontal line.  */
  /***ja 水平線のタイプ.  */
  enum MFaceHLineType
    {
      MFACE_HLINE_BOTTOM,      
      MFACE_HLINE_UNDER,
      MFACE_HLINE_STRIKE_THROUGH,
      MFACE_HLINE_OVER,
      MFACE_HLINE_TOP
    } type;

  /***en Width of the line in pixels.  */
  /***ja 線幅（ピクセル単位）.  */
  unsigned width;

  /***en Color of the line.  If the value is Mnil, foreground color of
      a merged face is used.  */
  /***ja 線の色.  Mnil ならば、統合したフェースの前景色が使われる。  */
  
  MSymbol color;
} MFaceHLineProp;
/*=*/

/*** @ingroup m17nFace */
/***en
    @brief Type of box spec of face.

    The type #MFaceBoxProp is to specify the detail of #Mbox property
    of a face.  The value of the property must be a pointer to an
    object of this type.  */
/***ja
    @brief フェースの囲み枠指定用型宣言.

    #MFaceBoxProp はフェースの #Mbox プロパティの詳細を指定する型である。
    このプロパティの値はこの型のオブジェクトへのポインタでなくてはならない。
      */

typedef struct
{
  /***en Width of the box line in pixels.  */
  /***ja 線幅（ピクセル単位）.  */
  unsigned width;

  MSymbol color_top;
  MSymbol color_bottom;
  MSymbol color_left;
  MSymbol color_right;

  unsigned inner_hmargin;
  unsigned inner_vmargin;
  unsigned outer_hmargin;
  unsigned outer_vmargin;

} MFaceBoxProp;
/*=*/

/*** @ingroup m17nFace */
/***en
    @brief Type of hook function of face.

    The type #MFaceHookFunc is to specify the #Mhook property of a
    face.  The value of the property must be function of this
    type.  */
/***ja
    @brief フェースのフック関数の型宣言.

    #MFaceHookFunc はフェースの #Mhook プロパティを指定する型である。
    このプロパティの値は、この型の関数でなくてはならない。
      */
typedef void (*MFaceHookFunc) (MFace *face, void *arg, void *info);
/*=*/

extern void *mface_get_prop (MFace *face, MSymbol key);

extern int mface_put_prop (MFace *face, MSymbol key, void *val);

extern void mface_update (MFrame *frame, MFace *face);

/* end of face module */
/*=*/

/*** @ingroup m17nGUI */
/***en @defgroup m17nDraw Drawing */
/***ja @defgroup m17nDraw 表示 */
/*=*/

/*** @ingroup m17nDraw */
/***en
    @brief Window system dependent type for a window.

    The type #MDrawWindow is for a window; a rectangular area that
    works in several ways like a miniature screen.

    What it actually points depends on a window system.  A program
    that uses the m17n-X library must coerce the type @c Drawable to
    this type.  */
/***ja 
    @brief ウィンドウシステムに依存する、ウィンドウの型宣言.

    #MDrawWindow はウィンドウ、すなわち幾つかの点でスクリーンのミニチュアとして働く矩形領域用の型である。

    実際に何を指すかはウィンドウシステムに依存する。 m17n X 
    ライブラリを利用するプログラムは @c Drawable 型をこの型に変換しなくてはならない。 */

typedef void *MDrawWindow;
/*=*/

/*** @ingroup m17nDraw */
/***en
    @brief Window system dependent type for a region.

    The type #MDrawRegion is for a region; an arbitrary set of pixels
    on the screen (typically a rectangular area).

    What it actually points depends on a window system.  A program
    that uses the m17n-X library must coerce the type @c Region to
    this type.  */
/***ja
    @brief ウィンドウシステムに依存する、領域の型宣言.

    #MDrawRegion は領域、すなわちスクリーン上の任意のピクセルの集合（典型的には矩形領域）用の型である。

    実際に何を指すかはウィンドウシステムに依存する。 m17n X 
    ライブラリを利用するプログラムは @c Region 型をこの型に変換しなくてはならない。  */

typedef void *MDrawRegion;
/*=*/

/*** @ingroup m17nDraw */
/***en
    @brief Type of a text drawing control.

    The type #MDrawControl is the structure that controls how to draw
    an M-text.  */
/***ja
    @brief テキスト表示制御の型宣言.

    #MDrawControl 型は、M-text をどう表示するかを制御する構造体である。
      */


typedef struct
{
  /***en If nonzero, draw an M-text as image, i.e. with background
      filled with background colors of faces put on the M-text.
      Otherwise, the background is not changed.  */
  /***ja 0 でなければ、 M-text を画像として、すなわち背景を M-text 
      のフェースで指定されている背景色で埋めて表示する。そうでなければ背景は変わらない。  */
  unsigned as_image : 1;

  /***en If nonzero and the first glyph of each line has negative
      lbearing, shift glyphs horizontally to right so that no pixel is
      drawn to the left of the specified position.  */
  /***ja 0 でなく、各行の最初のグリフの lbearing 
      が負ならば、グリフを水平に右にずらして、指定した位置より左にピクセルが描かれないようにする。  */
  unsigned align_head : 1;

  /***en If nonzero, draw an M-text two-dimensionally, i.e., newlines
      in M-text breaks lines and the following characters are drawn in
      the next line.  If <format> is non-NULL, and the function
      returns nonzero line width, a line longer than that width is
      also broken.  */
  /***ja 0 でなければ、M-text を２次元的に、すなわち M-text 中の 
      newline で改行し、続く文字は次の行に表示する。もし <format> が 
      NULL でなく、その関数が 0 でない行幅を返せば、その幅より長い行も改行される。  */
  unsigned two_dimensional : 1;

  /***en If nonzero, draw an M-text to the right of a specified
      position.  */
  /***ja 0 でなければ、M-text を指定した位置の右に表示する。  */
  unsigned orientation_reversed : 1;

  /***en If nonzero, reorder glyphs correctly for bidi text.  */ 
  /***ja 0 なければ、bidi テキスト用にグリフを正しく整列する。  */
  unsigned enable_bidi : 1;

  /***en If nonzero, don't draw characters whose general category (in
      Unicode) is Cf (Other, format).  */
  /***ja 0 でなければ、ユニコードに置ける一般カテゴリが Cf (Other,
      format) である文字を表示しない。  */
  unsigned ignore_formatting_char : 1;

  /***en If nonzero, draw glyphs suitable for a terminal.  Not yet
      implemented.  */
  /***ja 0 でなければ、端末用のグリフを表示する。未実装。  */
  unsigned fixed_width : 1;

  /***en If nonzero, draw glyphs with anti-aliasing if a backend font
      driver supports it.  */
  /***ja 0 でなければ、アンチエーリアスでグリフを表示する。
      （バックエンドのフォントドライバがアンチエーリアス機能を持つ場合のみ。） */
  unsigned anti_alias : 1;

  /***en If nonzero, disable the adjustment of glyph positions to
      avoid horizontal overlapping at font boundary.  */
  /***ja 0 でなければ、フォント境界での水平方向のグリフの重なりを避けるためのグリフ位置の調整を無効にする。  */
  unsigned disable_overlapping_adjustment : 1;

  /***en If nonzero, the values are minimum line ascent and descent
      pixels.  */
  /***ja 0 でなければ、値は行の ascent と descent の最小値を示す。  */
  unsigned int min_line_ascent;
  unsigned int min_line_descent;

  /***en If nonzero, the values are maximum line ascent and descent
      pixels.  */
  /***ja 0 でなければ、値は行の ascent と descent の最大値を示す。  */
  unsigned int max_line_ascent;
  unsigned int max_line_descent;

  /***en If nonzero, the value specifies how many pixels each line can
      occupy on the display.  The value zero means that there is no
      limit.  It is ignored if <format> is non-NULL.  */
  /***ja 0 でなければ、値はこのディスプレイ上で各行が占めることのできるピクセル数を示す。
      0 は限定されないことを意味する。<format> が NULL でなければ無視される。   */
  unsigned int max_line_width;

  /***en If nonzero, the value specifies the distance between tab
      stops in columns (the width of one column is the width of a
      space in the default font of the frame).  The value zero means
      8.  */
  /***ja 0 でなければ、値はタブストップ間の距離をコラム単位
      （コラムはフレームのデフォルトフォントにおける空白文字の幅である）で示す。 
      0 は 8 を意味する。 */
  unsigned int tab_width;

  /***en If non-NULL, the value is a function that calculates the
      indentation and width limit of each line based on the line
      number LINE and the coordinate Y.  The function store the
      indentation and width limit at the place pointed by INDENT and
      WIDTH respectively.

      The indentation specifies how many pixels the first glyph of
      each line is shifted to the right (if the member
      <orientation_reversed> is zero) or to the left (otherwise).  If
      the value is negative, each line is shifted to the reverse
      direction.

      The width limit specifies how many pixels each line can occupy
      on the display.  The value 0 means that there is no limit.

      LINE and Y are reset to 0 when a line is broken by a newline
      character, and incremented each time when a long line is broken
      because of the width limit.

      This has an effect only when <two_dimensional> is nonzero.  */
  /***ja 0 でなければ、値は関数であり、その関数は行番号 LINE と座標 Y 
      に基づいて各行のインデントと最大幅を計算し、それぞれをINDENT と
      WIDTH で指される場所に保存する。

      インデントは、各行の最初のグリフを右（メンバ 
      <orientation_reversed> が 0 
      の時）あるいは左（それ以外の時）に何ピクセルずらすかを指定する。値が負ならば逆方向にずらす。

      最大幅は、各行がディスプレイ上で占めることのできるピクセル数の最大値である。値が
      0 の場合は制限を受けないことを意味する。

      LINE と Y は改行文字によって行が改まった際には 0 
      にリセットされ、長い行が最大幅の制限によって改行されるたびに 1 増やされる。

      これは <two_dimensional> が 0 でない場合にのみ有効である。  */
  void (*format) (int line, int y, int *indent, int *width);

  /***en If non-NULL, the value is a function that calculates a line
      breaking position when a line is too long to fit within the
      width limit.  POS is the position of the character next to the
      last one that fits within the limit.  FROM is the position of the
      first character of the line, and TO is the position of the last
      character displayed on the line if there were not width limit.
      LINE and Y are the same as the arguments to <format>.

      The function must return a character position to break the
      line.

      The function should not modify MT.

      The mdraw_default_line_break () function is useful for such a
      script that uses SPACE as a word separator.  */
  /***ja NULL でなければ、値は行が最大幅中に収まらない場合に行を改める位置を計算する関数である。
      POS は最大幅に収まる最後の文字の次の文字の位置である。FROM
      は行の最初の文字の位置、TO 
      は最大幅が指定されていなければその行に表示される最後の文字の位置である。LINE 
      と Y は <format> の引数と同様である。

      この関数は行を改める文字位置を返さなくてはならない。また MT を変更してはならない。

      関数 mdraw_default_line_break ()
      は、空白を語の区切りとして用いるスクリプト用として有用である。  */
  int (*line_break) (MText *mt, int pos, int from, int to, int line, int y);

  int with_cursor;

  /***en Specifies the character position to display a cursor.  If it
      is greater than the maximum character position, the cursor is
      displayed next to the last character of an M-text.  If the value
      is negative, even if <cursor_width> is nonzero, cursor is not
      displayed.  */
  /***ja カーソルを表示する文字位置を示す。最大の文字位置より大きければ、カーソルは 
      M-text の最後の文字の隣に表示される。負ならば、
      <cursor_width> が 0 でなくてもカーソルは表示されない。
        */
  int cursor_pos;

  /***en If nonzero, display a cursor at the character position
      <cursor_pos>.  If the value is positive, it is the pixel width
      of the cursor.  If the value is negative, the cursor width is
      the same as the underlining glyph(s).  */
  /***ja 0 でなければ、<cursor_pos> にカーソルを表示する。
      値が正ならば、カーソルの幅はその値（ピクセル単位）である。
      負ならば、カーソルのあるグリフと同じ幅である。  */
  int cursor_width;

  /***en If nonzero and <cursor_width> is also nonzero, display double
      bar cursors; at the character position <cursor_pos> and at the
      logically previous character.  Both cursors have one pixel width
      with horizontal fringes at upper or lower positions.  */
  /***ja If 0 でなく、かつ <cursor_width> も 0 でなければ、バーカーソルを文字位置
      <cursor_pos> と論理的にそれの前にある文字の２ヶ所に表示する。
      双方とも１ピクセル幅で、上か下に水平の飾りがつく。*/
  int cursor_bidi;

  /***en If nonzero, on drawing partial text, pixels of surrounding
      texts that intrude into the drawing area are also drawn.  For
      instance, some CVC sequence of Thai text (C is consonant, V is
      upper vowel) is drawn so that V is placed over the middle of two
      Cs.  If this CVC sequence is already drawn and only the last C
      is drawn again (for instance by updating cursor position), the
      right half of V is erased if this member is zero.  By setting
      this member to nonzero, even with such a drawing, we can keep
      this CVC sequence correctly displayed.  */
  /***ja 0 でなければ、テキストの一部分を表示する際に、前後のテキストのうちその表示領域に侵入する部分も表示する。
      たとえば、タイ語テキスト 子音-母音-子音 
      というシークエンスのいくつかは、母音が二つの子音の間に上にのるように描かれる。
      このようなシークエンスがすでに描かれており、最後の子音だけを描き直す場合
      （たとえば、カーソル位置を更新する際など）このメンバが 0 
      であれば、母音の右半分が消されてしまう。これを 0 以外にすることによって、そのような際にも
      子音-母音-子音 のシークエンスを正しく表示し続けることができる。  */
  int partial_update;

  /***en If nonzero, don't cache the result of any drawing information
      of an M-text.  */
  /***ja 0 でなければ、M-text の表示に関する情報をキャッシュしない。
       */
  int disable_caching;

  /* If non-NULL, limit the drawing effect to the specified region.  */
  MDrawRegion clip_region;

} MDrawControl;

extern int mdraw_line_break_option;

/*=*/

/*** @ingroup m17nDraw */
/***en
    @brief Type of metric for glyphs and texts.

    The type #MDrawMetric is for a metric of a glyph and a drawn text.
    It is also used to represent a rectangle area of a graphic
    device.  */
/***ja
    @brief グリフとテキストの寸法の型宣言.

    #MDrawMetric はグリフと表示されたテキストの寸法用の型である。
    また、表示デバイスの矩形領域を表すのにも用いられる。 */

typedef struct {
  int x, y;
  unsigned int width, height;
} MDrawMetric;

/*=*/

/*** @ingroup m17nDraw */
/***en
    @brief Type of information about a glyph.

    The type #MDrawGlyphInfo is the structure that contains
    information about a glyph.  It is used by mdraw_glyph_info ().  */
/***ja
    @brief グリフに関する情報の型宣言.

    #MDrawGlyphInfo 型はグリフに関する情報を含む構造体である。
    mdraw_glyph_info () はこれを用いる。  */

typedef struct
{
  /***en Character range corresponding to the glyph.  */
  /***ja グリフに対応する文字の範囲.  */
  int from, to;

  /***en Character ranges corresponding to the line of the glyph.  */
  /***ja  グリフの列に対応する文字の範囲.  */
  int line_from, line_to;

  /***en X/Y coordinates of the glyph.  */
  /***ja グリフの X/Y 座標.  */
  int x, y;

  /***en Metric of the glyph.  */
  /***ja グリフの寸法.  */
  MDrawMetric metrics;

  /***en Font used for the glyph.  Set to NULL if no font is found for
      the glyph.  */
  /***ja グリフに使われるフォント。見つからなければ NULL。 */
      
  MFont *font;

  /***en Character ranges corresponding to logically previous and next
      glyphs.  Note that we do not need the members prev_to and
      next_from because they must be the same as the members from and
      to respectively.  */
  /***ja 論理的な前後のグリフに対応する文字の範囲。メンバ prev_to と
      next_from は、それぞれメンバ from と to と同じであるはずなので不
      要である。  */
  int prev_from, next_to;

  /***en Character ranges corresponding to visually left and right
      glyphs. */
  /***ja 表示上の左右のグリフに対応する文字の範囲。  */
  int left_from, left_to;
  int right_from, right_to;

  /***en Logical width of the glyph.  Nominal distance to the next
      glyph.  */
  /***ja グリフの論理的幅。次のグリフとの名目上の距離。  */
  int logical_width;
} MDrawGlyphInfo;

/*=*/

/*** @ingroup m17nDraw */
/***en
    @brief Type of information about a glyph metric and font.

    The type #MDrawGlyph is the structure that contains information
    about a glyph metric and font.  It is used by the function
    mdraw_glyph_list ().  */
/***ja
    @brief グリフの寸法とフォントに関する情報の型宣言.

    #MDrawGlyph 型はグリフの寸法とフォントに関する情報を含む構造体である。
    mdraw_glyph_list () はこれを用いる。  */

typedef struct
{
  /***en Character range corresponding to the glyph.  */
  /***ja グリフに対応する文字の範囲.  */
  int from, to;

  /***en Font glyph code of the glyph.  */
  /***ja フォント内のグリフコード。  */
  int glyph_code;

  /***en Logical width of the glyph.  Nominal distance to the next
      glyph.  */
  /***ja グリフの論理的幅。次のグリフとの名目上の距離。  */
  int x_advance, y_advance;

  /***en X/Y offset relative to the glyph position.  */
  /***ja グリフの位置に対する X/Y オフセット.  */
  int x_off, y_off;

  /***en Metric of the glyph.  */
  /***ja グリフの寸法.  */
  int lbearing, rbearing, ascent, descent;

  /***en Font used for the glyph.  Set to NULL if no font is found for
      the glyph.  */
  /***ja グリフに使われるフォント。見つからなければ NULL。  */
  MFont *font;

  /***en Type of the font.  One of Mx, Mfreetype, Mxft.  */
  /***ja フォントのタイプ。Mx、Mfreetype、Mxft のいずれか。  */
  MSymbol font_type;

  /***en Pointer to the font structure.  The actual type is
      (XFontStruct *) if <font_type> member is Mx, FT_Face if
      <font_type> member is Mfreetype, and (XftFont *) if <font_type>
      member is Mxft.  */
  /***ja フォントの構造体へのポインタ。実際の型は <font_type> メンバが
      Mx なら (XFontStruct *)、 Mfreetype なら FT_Face、Mxft 
      なら (XftFont *)。 */
  void *fontp;

} MDrawGlyph;

/*=*/

/***en
    @brief Type of textitems.

    The type #MDrawTextItem is for @e textitem objects.
    Each textitem contains an M-text and some other information to
    control the drawing of the M-text.  */

/***ja
    @brief textitem の型宣言.

    #MDrawTextItem は @e テキストアイテム オブジェクト用の型である。
    各テキストアイテムは、 1 個の M-text と、その表示を制御するための情報を含んでいる。

    @latexonly \IPAlabel{MTextItem} @endlatexonly  */

typedef struct
{
  /***en M-text. */
  /***ja M-text. */
  MText *mt;                      

  /***en Optional change in the position (in the unit of pixel) along
      the X-axis before the M-text is drawn.  */
  /***ja M-text 表示前に行なうX軸方向の位置調整 (ピクセル単位) */
  int delta;                     

  /***en Pointer to a face object.  Each property of the face, if not
      Mnil, overrides the same property of face(s) specified as a text
      property in <mt>.  */
  /***ja フェースオブジェクトへのポインタ。フェースの各プロパティは 
      Mnil でなければ <mt> で指定されたフェースの同じプロパティに優先する*/
  MFace *face;

  /***en Pointer to a draw control object.  The M-text <mt> is drawn
      by mdraw_text_with_control () with this control object.  */
  /***ja 表示制御オブジェクトへのポインタ。 mdraw_text_with_control () 
      はこのオブジェクトを用いて M-text <mt> を表示する。  */
  MDrawControl *control;

} MDrawTextItem;

/*=*/

extern int mdraw_text (MFrame *frame, MDrawWindow win, int x, int y,
		       MText *mt, int from, int to);

extern int mdraw_image_text (MFrame *frame, MDrawWindow win, int x, int y,
			     MText *mt, int from, int to);

extern int mdraw_text_with_control (MFrame *frame, MDrawWindow win,
				    int x, int y, MText *mt, int from, int to,
				    MDrawControl *control);

extern int mdraw_coordinates_position (MFrame *frame,
				       MText *mt, int from, int to,
				       int x, int y, MDrawControl *control);

extern int mdraw_text_extents (MFrame *frame,
			       MText *mt, int from, int to,
			       MDrawControl *control,
			       MDrawMetric *overall_ink_return,
			       MDrawMetric *overall_logical_return,
			       MDrawMetric *overall_line_return);

extern int mdraw_text_per_char_extents (MFrame *frame,
					MText *mt, int from, int to,
					MDrawControl *control,
					MDrawMetric *ink_array_return,
					MDrawMetric *logical_array_return,
					int array_size,
					int *num_chars_return,
					MDrawMetric *overall_ink_return,
					MDrawMetric *overall_logical_return);

extern int mdraw_glyph_info (MFrame *frame, MText *mt, int from, int pos,
			     MDrawControl *control, MDrawGlyphInfo *info);

extern int mdraw_glyph_list (MFrame *frame, MText *mt, int from, int to,
			     MDrawControl *control, MDrawGlyph *glyphs,
			     int array_size, int *num_glyphs_return);

extern void mdraw_text_items (MFrame *frame, MDrawWindow win, int x, int y,
			      MDrawTextItem *items, int nitems);

extern void mdraw_per_char_extents (MFrame *frame, MText *mt,
				    MDrawMetric *array_return,
				    MDrawMetric *overall_return);

extern int mdraw_default_line_break (MText *mt, int pos,
				     int from, int to, int line, int y);

extern void mdraw_clear_cache (MText *mt);

/* end of drawing module */
/*=*/

/*** @ingroup m17nGUI */
/***en @defgroup m17nInputMethodWin Input Method (GUI) */
/***ja @defgroup m17nInputMethodWin 入力メソッド (GUI) */
/*=*/

extern MInputDriver minput_gui_driver;

/*=*/
/*** @ingroup m17nInputMethodWin */
/***en 
    @brief Type of the argument to the function minput_create_ic ().

    The type #MInputGUIArgIC is for the argument $ARG of the function
    minput_create_ic () to create an input context of an internal
    input method.  */

/***ja
    @brief 関数 minput_create_ic () の引数の型宣言.

    #MInputGUIArgIC は、関数 minput_create_ic () 
    が内部入力メソッドの入力コンテクストを生成する際の、引数 $ARG 用の型である。  */

typedef struct
{
  /***en Frame of the client.  */
  /***ja クライアントのフレーム  */
  MFrame *frame;

  /***en Window on which to display the preedit and status text.  */
  /***ja preedit テキストと status テキストを表示するウィンドウ  */
  MDrawWindow client;

  /***en Window that the input context has a focus on.  */
  /***ja 入力コンテクストがフォーカスをおいているウィンドウ  */
  MDrawWindow focus;
} MInputGUIArgIC;

/*=*/

extern MSymbol minput_event_to_key (MFrame *frame, void *event);

/* end of input module */
/*=*/
/* end of window modules */
/*=*/

extern MFace *mdebug_dump_face (MFace *face, int indent);
extern MFont *mdebug_dump_font (MFont *font);
extern MFontset *mdebug_dump_fontset (MFontset *fontset, int indent);

#ifdef __cplusplus
}
#endif

#endif /* _M17N_GUI_H_ */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
