/* m17n-gui.c -- body of the GUI API.
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
    @addtogroup m17nGUI
    @brief GUI support for a window system

    This section defines the m17n GUI API concerning M-text drawing
    and inputting under a window system.

    All the definitions here are independent of window systems.  An
    actual library file, however, can depend on a specific window
    system.  For instance, the library file m17n-X.so is an example of
    implementation of the m17n GUI API for the X Window System.

    Actually the GUI API is mainly for toolkit libraries or to
    implement XOM, not for direct use from application programs.
*/

/***oldja
    @addtogroup m17nGUI
    @brief ウィンドウシステム関連の多言語対応 

    このセクションはウィンドウシステム上での M-text の表示と入力を扱う 
    m17n-win API を定義する。

    ここでのすべての定義はウィンドウシステムとは独立である。しかし実装
    は個別のウィンドウシステムに依存する場合がある。m17n-X ライブラリ
    は X ウィンドウ用の実装例である。  */

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "m17n-gui.h"
#include "m17n-misc.h"
#include "internal.h"
#include "internal-gui.h"
#include "font.h"
#include "fontset.h"
#include "face.h"

static int win_initialized;

static void
free_frame (void *object)
{
  MFrame *frame = (MFrame *) object;

  M17N_OBJECT_UNREF (frame->face);
  mwin__close_device ((MFrame *) object);
  free (frame->font);
  free (object);
}


/* Internal API */

/*** @} */ 
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

void
m17n_init_win (void)
{
  int mdebug_mask = MDEBUG_INIT;

  if (win_initialized)
    return;
  m17n_init ();
  if (merror_code != MERROR_NONE)
    return;

  Mfont = msymbol ("font");
  Mfont_width = msymbol ("font-width");
  Mfont_ascent = msymbol ("font-ascent");
  Mfont_descent = msymbol ("font-descent");

  MDEBUG_PUSH_TIME ();
  MDEBUG_PUSH_TIME ();
  if (mfont__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize font module."));
  if (mwin__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize win module."));
  if (mfont__fontset_init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize fontset module."));
  if (mface__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize face module."));
  if (mdraw__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize draw module."));
  if (minput__win_init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize input-win module."));
  mframe_default = NULL;
  win_initialized = 1;

 err:
  MDEBUG_POP_TIME ();
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize the m17n GUI module."));
  MDEBUG_POP_TIME ();
  return;
}

void
m17n_fini_win (void)
{
  int mdebug_mask = MDEBUG_FINI;

  if (win_initialized)
    {
      MDEBUG_PUSH_TIME ();
      MDEBUG_PUSH_TIME ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize input-gui module."));
      minput__win_fini ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize draw module."));
      mdraw__fini ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize face module."));
      mface__fini ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize fontset module."));
      mfont__fontset_fini ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize window module."));
      mwin__fini ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize font module."));
      mfont__fini ();
      mframe_default = NULL;
      MDEBUG_POP_TIME ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize the gui modules."));
      MDEBUG_POP_TIME ();
      win_initialized = 0;
    }
  m17n_fini ();
}

/*** @addtogroup m17nFrame */
/***en
    @brief A @e frame is an object corresponding to the physical device.

    A @e frame is an object of the type #MFrame to hold various
    information about each physical display/input device.  Almost all
    m17n GUI functions require a pointer to a frame as an
    argument.  */

/***oldja
    @brief フレームとは物理的デバイスに対応するオブジェクトである

    フレームとは #MFrame 型のオブジェクトであり、個々の物理的な表示／
    入力デバイスの情報を格納するために用いられる。ほとんどすべての 
    m17n-win API は、引数としてフレームへのポインタを要求する。  */

/*** @{ */
/*=*/

/***en
    @name Variables: Keys of frame property (common).
 */ 
/*** @{ */ 
/*=*/
MSymbol Mfont;
MSymbol Mfont_width;
MSymbol Mfont_ascent;
MSymbol Mfont_descent;

/*=*/
/*** @} */ 
/*=*/

/***en
    @brief Create a new frame.

    The mframe () function creates a new frame with parameters listed
    in $PLIST.

    The recognized keys in $PLIST are window system dependent.

    The following key is always recognized.

    <ul>

    <li> #Mface, the value type must be <tt>(MFace *)</tt>.

    The value is used as the default face of the frame.

    </ul>

    In addition, in the m17n-X library, the following keys are
    recognized.  They are to specify the root window and the depth of
    drawables that can be used with the frame.

    <ul>

    <li> #Mdrawable, the value type must be <tt>Drawable</tt>

    A parameter of key #Mdisplay must also be specified.  The
    created frame can be used for drawables whose root window and
    depth are the same as those of the specified drawable on the
    specified display.

    When this parameter is specified, the parameter of key #Mscreen
    is ignored.

    <li> #Mwidget, the value type must be <tt>Widget</tt>.

    The created frame can be used for drawables whose root window and
    depth are the same as those of the specified widget.

    If a parameter of key #Mface is not specified, the default face
    is created from the resources of the widget.

    When this parameter is specified, the parameters of key #Mdisplay,
    #Mscreen, #Mdrawable, #Mdepth are ignored.

    <li> #Mdepth, the value type must be <tt>unsigned</tt>.

    The created frame can be used for drawables of the specified
    depth.

    <li> #Mscreen, the value type must be <tt>(Screen *)</tt>.

    The created frame can be used for drawables whose root window is
    the same as the root window of the specified screen, and depth is
    the same at the default depth of the screen.

    When this parameter is specified, parameter of key #Mdisplay is
    ignored.

    <li> #Mdisplay, the value type must be <tt>(Display *)</tt>.

    The created frame can be used for drawables whose root window is
    the same as the root window for the default screen of the display,
    and depth is the same as the default depth of the screen.

    <li> #Mcolormap, the value type must be <tt>(Colormap)</tt>.

    The created frame uses the specified colormap.

    </ul>

    @return
    If the operation was successful, mframe () returns a pointer to a
    newly created frame.  Otherwise, it returns @c NULL.  */

/***oldja
    @brief  新しいフレームを作る

    関数 mframe () は新しいフレームを作る。一般に、引数 $ARGC と $ARGV 
    は各アプリケーションの <tt>main ()</tt> 関数に与えられるものと同じ
    ものである。つまりコマンドライン引数を含んでいる必要がある。使用で
    きる引数はウィンドウシステムに依存する。

    m17n-X ライブラリにおけるこの関数は、<tt>XOpenDisplay (NULL)</tt> 
    を使ってデフォルトのディスプレイを開き、次いで <tt>DefaultScreen
    (DISPLAY)</tt> を使ってデフォルトのスクリーンを得た後、作られたフレーム
    にこの両者を関連付ける。

    m17n-X ライブラリは以下の引数を受け付ける。

       @li @c -fn @e font : フレームのデフォルトのフォントを @e font 
                            にセットする
       @li @c -fg @e color : フレームのデフォルトの前景色を @e color 
                             にセットする
       @li @c -bg @e color : フレームのデフォルトの背景色を @e color 
                             にセットする
       @li @c -rv : 前景色と背景色を交換する

    @return
    成功すれば mframe() は新しいフレームへのポインタを返す。そうでなけ
    れば @c NULL を返す。  */

MFrame *
mframe (MPlist *plist)
{
  MFrame *frame;
  MSymbol key;

  M17N_OBJECT (frame, free_frame, MERROR_FRAME);
  frame->device = mwin__open_device (frame, plist);
  if (! frame->device)
    {
      free (frame);
      MERROR (MERROR_WIN, NULL);
    }

  frame->face = mface_from_font (frame->font);
  frame->face->property[MFACE_FONTSET] = mfontset (NULL);
  M17N_OBJECT_REF (mface__default->property[MFACE_FONTSET]);
  if (plist)
    for (; (key = mplist_key (plist)) != Mnil; plist = mplist_next (plist))
      if (key == Mface)
	mface_merge (frame->face, (MFace *) mplist_value (plist));

  frame->rface = mface__realize (frame, NULL, 0, Mnil, Mnil, 0);
  if (! frame->rface->rfont)
    MERROR (MERROR_WIN, NULL);
  frame->space_width = frame->rface->space_width;
  frame->ascent = frame->rface->ascent;
  frame->descent = frame->rface->descent;

  if (! mframe_default)
    mframe_default = frame;

  return frame;
}

/*=*/

/***en
    @brief Return property value of frame.

    The mframe_get_prop () function returns a value of property $KEY
    of frame $FRAME.  The valid keys and the corresponding return
    values are as follows.

@verbatim

	key		type of value	meaning of value
	---             -------------   ----------------
	Mface		MFace *		The default face.

	Mfont		MFont *		The default font.

	Mfont_width	int		Width of the default font.

	Mfont_ascent	int		Ascent of the default font.

	Mfont_descent	int		Descent of the default font.

@endverbatim

    In the m17n-X library, the followings are also accepted.

@verbatim

	key		type of value	meaning of value
	---             -------------   ----------------
	Mdisplay	Display *	Display associated with the frame.

	Mscreen		int		Screen number of a screen associated
					with the frame.

	Mcolormap	Colormap	Colormap of the frame.

	Mdepth		unsigned		Depth of the frame.
@endverbatim
*/

/***oldja
    @brief フレームのMWDeviceを返す

    関数 mframe_device () はフレーム $FRAME に格納されている @c
    MWDevice 構造体へのポイタを返す。#MWDevice の形式はウィンドウシ
    ステムに依存する。  */

void *
mframe_get_prop (MFrame *frame, MSymbol key)
{
  if (key == Mface)
    return frame->face;
  if (key == Mfont)
    return &frame->rface->rfont->font;
  if (key == Mfont_width)
    return (void *) (frame->space_width);
  if (key == Mfont_ascent)
    return (void *) (frame->ascent);
  if (key == Mfont_descent)
    return (void *) (frame->descent);
  return mwin__device_get_prop (frame->device, key);
}

/*=*/

/***en
    @brief The default frame.

    The external variable #mframe_default contains a pointer to the
    default frame that is created by the first call of mframe ().  */

/***oldja
    デフォルトのフレーム 

    外部変数 #mframe_default は、デフォルトのフレームへのポインタを
    持つ。デフォルトのフレームは、最初に mframe () が呼び出されたときに
    作られる。  */

MFrame *mframe_default;

/*** @} */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
