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

extern void m17n_init_win (void);
#undef M17N_INIT
#define M17N_INIT() m17n_init_win ()

extern void m17n_fini_win (void);
#undef M17N_FINI
#define M17N_FINI() m17n_fini_win ()

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

    The internal structure of the type #MFrame is concealed from
    application code, and its contents depend on the window system in
    use.  In the m17n-X library, it contains the information about @e
    display and @e screen in the X Window System.  */

/***ja
    @brief フレーム用構造体

    #MFrame 型は、フレームオブジェクト用の構造体である。個々のフレー
    ムは、それが対応する物理デバイスの各種情報を保持する。

    #MFrame 型の内部構造は、使用するウィンドウシステムに依存し、また
    アプリケーションプログラムからは見えない。m17n-X ライブラリにおけ
    るフレームは、X ウィンドウの display と screen に関する情報を持つ。
      */

typedef struct MFrame MFrame;

/*=*/

extern MSymbol Mfont;
extern MSymbol Mfont_width;
extern MSymbol Mfont_ascent;
extern MSymbol Mfont_descent;
extern MFrame *mframe_default;

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

    The internal structure is concealed from application code.  */

/***ja
    @brief フォントの構造

    #MFont 型はフォント指定用の構造体であり、フォントのプロ
    パティとして family, weight, style, stretch, adstyle, registry,
    size, resolution を持つ。

    この構造体はフォントセット内のフォントを指定する場合と、使用可能な
    システムフォントの情報を格納する場合の両方で用いられる。

    内部構造はアプリケーションプログラムからは見えない。  */

/***
    @seealso
    mfont (), mfont_from_name (), mfont_find ().  */

typedef struct MFont MFont;

/*=*/

extern MSymbol Mfont;

extern MPlist *mfont_freetype_path;

extern MFont *mfont ();

extern MFont *mfont_from_name (char *name);

extern MFont *mfont_copy (MFont *font);

extern char *mfont_name (MFont *font);

extern MFont *mfont_from_spec (char *family, char *weight, char *slant,
			       char *swidth, char *adstyle, char *registry,
			       unsigned short point, unsigned short res);

extern MSymbol Mfoundry;
extern MSymbol Mfamily;
extern MSymbol Mweight;
extern MSymbol Mstyle;
extern MSymbol Mstretch;
extern MSymbol Madstyle;
extern MSymbol Mregistry;
extern MSymbol Msize;
extern MSymbol Mresolution;

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
    @brief フォントを探す

    関数 mfont_find () は、フレーム $FRAME 上でフォント定義 $SPEC にもっ
    とも近いフォントへのポインタを返す。  */

extern MFont *mfont_find (MFrame *frame, MFont *spec,
			  int *score, int limited_size);

extern MSymbol *mfont_selection_priority ();

extern int mfont_set_selection_priority (MSymbol *keys);

/* end of font module */
/*=*/

/*** @ingroup m17nGUI  */
/***en @defgroup m17nFontset Fontset */
/***ja @defgroup m17nFontset フォントセット */
/*=*/

typedef struct MFontset MFontset;

extern MFontset *mfontset (char *name);

extern MSymbol mfontset_name (MFontset *fontset);

extern MFontset *mfontset_copy (MFontset *fontset, char *name);

extern int mfontset_modify_entry (MFontset *fontset,
				  MSymbol language, MSymbol script,
				  MSymbol charset,
				  MFont *spec, MSymbol layouter_name,
				  int how);

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
    structure is concealed from application code.  */

/***ja
    @brief フェース用構造体

    #MFace 型はフェースオブジェクトのための構造体である。内部構造は
    アプリケーションプログラムからは見えない。  */

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

typedef struct
{
  /***en Type of the horizontal line.  */
  enum MFaceHLineType
    {
      MFACE_HLINE_BOTTOM,      
      MFACE_HLINE_UNDER,
      MFACE_HLINE_STRIKE_THROUGH,
      MFACE_HLINE_OVER,
      MFACE_HLINE_TOP
    } type;

  /***en Width of the line in pixels.  */
  unsigned width;

  /***en Color of the line.  If the value is Mnil, foreground color of
      a merged face is used.  */
  MSymbol color;
} MFaceHLineProp;
/*=*/

/*** @ingroup m17nFace */
/***en
    @brief Type of box spec of face.

    The type #MFaceBoxProp is to specify the detail of #Mbox property
    of a face.  The value of the property must be a pointer to an
    object of this type.  */

typedef struct
{
  /***en Width of the box line in pixels.  */
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
typedef void *(*MFaceHookFunc) (MFace *face, void *arg, void *info);
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

    The type MDrawWindow is for a window; a rectangular area that
    works in several ways like a miniature screen.

    What it actually points depends on a window system.  A program
    that uses the m17n-X library must coerce the type @c Drawable to
    this type.  */

/***ja ウィンドウシステムに依存する、ウィンドウを表すオブジェクト用の型。

       m17n X ライブラリでは、@c Window 型と同じ.  */

typedef void *MDrawWindow;
/*=*/

/*** @ingroup m17nDraw */
/***en
    @brief Window system dependent type for a region.

    The type MDrawRegion is for a region; an arbitrary set of pixels
    on the screen (typically a rectangular area).

    What it actually points depends on a window system.  A program
    that uses the m17n-X library must coerce the type @c Region to
    this type.  */

typedef void *MDrawRegion;
/*=*/

/*** @ingroup m17nDraw */
/***en
    @brief Type of a text drawing control.

    The type #MDrawControl is the structure that controls how to draw
    an M-text.  */

typedef struct
{
  /***en If nonzero, draw an M-text as image, i.e. with background
      filled with background colors of faces put on the M-text.
      Otherwise, the background is not changed.  */
  unsigned as_image : 1;

  /***en If nonzero and the first glyph of each line has negative
      lbearing, shift glyphs horizontally to right so that no pixel is
      drawn to the left of the specified position.  */
  unsigned align_head : 1;

  /***en If nonzero, draw an M-text two-dimensionally, i.e., newlines
      in M-text breaks lines and the following characters are drawn in
      the next line.  If <format> is non-NULL, and the function
      returns nonzero line width, a line longer than that width is
      also broken.  */
  unsigned two_dimensional : 1;

  /***en If nonzero, draw an M-text to the right of a specified
      position.  */
  unsigned orientation_reversed : 1;

  /***en If nonzero, reorder glyphs correctly for bidi text.  */
  unsigned enable_bidi : 1;

  /***en If nonzero, don't draw characters whose general category (in
      Unicode) is Cf (Other, format).  */
  unsigned ignore_formatting_char : 1;

  /***en If nonzero, draw glyphs suitable for a terminal.  Not yet
      implemented.  */
  unsigned fixed_width : 1;

  /***en If nonzero, the values are minimum line ascent and descent
      pixels.  */
  unsigned int min_line_ascent;
  unsigned int min_line_descent;

  /***en If nonzero, the values are maximum line ascent and descent
      pixels.  */
  unsigned int max_line_ascent;
  unsigned int max_line_descent;

  /***en If nonzero, the value specifies how many pixels each line can
      occupy on the display.  The value zero means that there is no
      limit.  It is ignored if <format> is non-NULL.  */
  unsigned int max_line_width;

  /***en If nonzero, the value specifies the distance between tab
      stops in columns (the width of one column is the width of a
      space in the default font of the frame).  The value zero means
      8.  */
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
  void (*format) (int line, int y, int *indent, int *width);

  /***en If non-NULL, the value is a function that calculates a line
      breaking position when a line is too long to fit within the
      width limit.  POS is a position of the character next to the
      last one that fits within the limit.  FROM is a position of the
      first character of the line, and TO is a position of the last
      character displayed on the line if there were not width limit.
      LINE and Y are the same as the arguments to <format>.

      The function must return a character position to break the
      line.

      The function should not modify MT.

      The mdraw_default_line_break () function is useful for such a
      script that uses SPACE as a word separator.  */
  int (*line_break) (MText *mt, int pos, int from, int to, int line, int y);

  int with_cursor;

  /***en Specifies the character position to display a cursor.  If it
      is greater than the maximum character position, the cursor is
      displayed next to the last character of an M-text.  If the value
      is negative, even if <cursor_width> is nonzero, cursor is not
      displayed.  */
  int cursor_pos;

  /***en If nonzero, display a cursor at the character position
      <cursor_pos>.  If the value is positive, it is the pixel width
      of the cursor.  If the value is negative, the cursor width is
      the same as the underlining glyph(s).  */
  int cursor_width;

  /***en If nonzero and <cursor_width> is also nonzero, display double
      bar cursors; at the character position <cursor_pos> and at the
      logically previous character.  Both cursors have one pixel width
      with horizontal fringes at upper or lower positions.  HOW TO
      EXPLAIN THE DOUBLE CURSORS?  */
  int cursor_bidi;

  /***en If nonzero, on drawing partial text, pixels of surrounding
      texts that intrude into the drawing area are also drawn.  For
      instance, some CVC sequence of Thai text (C is consonant, V is
      upper vowel) is drawn so that V is placed over the middle of two
      Cs.  If this CVC sequence is already drawn and only the last C
      is drawn again (for instance by updating cursor position), the
      left half of V is erased if this member is zero.  By setting
      this member to nonzero, even with such a drawing, we can keep
      this CVC sequence correctly displayed.  */
  int partial_update;

  /***en If nonzero, don't cache the result of any drawing information
      of an M-text.  */
  int disable_caching;

  /* If non-NULL, limit the drawing effect to the specified region.  */
  MDrawRegion clip_region;

} MDrawControl;

/*=*/

/*** @ingroup m17nDraw */
/***en
    @brief Type of metric for gylphs and texts.

    The type #MDrawMetric is for a metric of a glyph and a drawn text.
    It is also used to represent a rectangle area of a graphic
    device.  */

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

typedef struct
{
  /***en Character range corresponding to the glyph.  */
  int from, to;

  /***en Character ranges corresponding to the line of the glyph.  */
  int line_from, line_to;

  /***en X/Y coordinates of the glyph.  */
  int x, y;

  /***en Metric of the glyph.  */
  MDrawMetric this;

  /***en Font used for the glyph.  Set to NULL if no font is found for
      the glyph.  */
  MFont *font;

  /***en Character ranges corresponding to logically previous and next
      glyphs.  Note that we do not need the members prev_to and
      next_from because they must be the same as the memberse from and
      to respectively.  */
  int prev_from, next_to;

  /***en Character ranges corresponding to visually left and right
      glyphs. */
  int left_from, left_to;
  int right_from, right_to;

} MDrawGlyphInfo;

/*=*/

/***en
    @brief Type of text items.

    The type #MDrawTextItem is for @e textitem objects.
    Each textitem contains an M-text and some other information to
    control the drawing of the M-text.  */

/***ja
    @brief textitem 用構造体

    型 #MDrawTextItem は @e テキストアイテム オブジェクト用の構造体であ
    る。各テキストアイテムは、1個の M-text と、その表示を制御するため
    の各種情報を含んでいる。

    @latexonly \IPAlabel{MTextItem} @endlatexonly  */

typedef struct
{
  /***en M-text. */
  /***ja M-text */
  MText *mt;                      

  /***en Optional change in the position (in the unit of pixel) along
      the X-axis before the M-text is drawn.  */
  /***ja 描画前に行なうX軸方向の位置調整 (ピクセル単位) */
  int delta;                     

  /***en Pointer to a face object.  Each property of the face, if not
      Mnil, overrides the same property of face(s) specified as a text
      property in <mt>.  */
  /***ja フォントセットオブジェクトへのポインタ。これは M-text 内で指
      定されたフェースのフォントセットに優先する*/
  MFace *face;

  /***en Pointer to a draw control object.  The M-text <mt> is drawn
      by mdraw_text_with_control () with this control object.  */
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
    @brief 関数 minput_create_ic () の引数 $ARG で指される構造体

    #MInputGUIArgIC 型は、関数 minput_create_ic () が内部入力メソッ
    ドを生成する際に、引数 $ARG によって指される構造体である。  */

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
