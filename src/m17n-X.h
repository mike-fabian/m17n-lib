/* m17n-X.h -- header file for the GUI API on X Windows.
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

#ifndef _M17N_X_H_
#define _M17N_X_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* For drawing.  */

extern MSymbol Mdisplay;
extern MSymbol Mscreen;
extern MSymbol Mdrawable;
extern MSymbol Mwidget;
extern MSymbol Mdepth;
extern MSymbol Mcolormap;

/* For inputting.  */

extern MInputDriver minput_xim_driver;
extern MSymbol Mxim;

/*** @ingroup m17nInputMethodWin */
/***en
    @brief Structure pointed to by the argument $ARG of the function.
    input_open_im ().

    The type #MInputXIMArgIM is the structure pointed to by the
    argument $ARG of the function minput_open_im () for the foreign
    input method of name #Mxim.  */

/***ja
    @brief 関数 minput_open_im () の引数 $ARG によって指される構造体.

    #MInputXIMArgIM 型は、関数 minput_open_im () が名前 #Mxim を持
    つ外部入力メソッドを生成する際に引数 $ARG によって指される構造体で
    ある。  */

typedef struct
{
  /***en The meaning of the following four members are the same as
      arguments to XOpenIM ().  */
  /***ja 以下の４つのメンバの意味は、XOpenIM () の引数の意味と同じであ
      る。  */

  /***en Display of the client.  */
  /***ja クライアントのディスプレイ  */
  Display *display;

  /***en Pointer to the X resource database.  */
  /***ja X リソース・データベースへのポインタ  */
  XrmDatabase db;

  /***en Full class name of the application.  */
  /***ja アプリケーションの完全なクラス名  */
  char *res_class;

  /***en Full resource name of the application.  */
  /***ja アプリケーションの完全なリソース名  */
  char *res_name;

  /***en Locale name under which an XIM is opened.  */
  /***ja XIMがオープンされたロケール名  */
  char *locale;

  /***en Arguments to XSetLocaleModifiers ().  */
  /***ja XSetLocaleModifiers () の引数  */
  char *modifier_list;
} MInputXIMArgIM;
  /*=*/

/*** @ingroup m17nInputMethodWin */
/***en
    @brief Structure pointed to by the argument $ARG of the function.
    minput_create_ic.

    The type #MInputXIMArgIC is the structure pointed to by the
    argument $ARG of the function minput_create_ic () for the foreign
    input method of name #Mxim.  */

/***ja
    @brief 関数 minput_create_ic () の引数 $ARG によって指される構造体.

    #MInputXIMArgIC 型は、関数 minput_create_ic () が名前 #Mxim を
    持つ外部入力メソッド用に呼ばれる際に、引数 $ARG によって指される構
    造体である。 */

typedef struct
{
  /***en Used as the arguments of @c XCreateIC following @c
      XNInputStyle.  If this is zero, ( @c XIMPreeditNothing | @c
      XIMStatusNothing) is used, and <preedit_attrs> and
      <status_attrs> are set to @c NULL.  */
  /***ja @c XCreateIC の @c XNInputStyle に続く引数として用いられる。
      ゼロならば、 ( @c XIMPreeditNothing | @c XIMStatusNothing) が用
      いられ、 <preedit_attrs> と <status_attrs> は @c NULL に設定され
      る。 */

  XIMStyle input_style;
  /***en Used as the argument of @c XCreateIC following @c XNClientWindow.  */
  /***ja @c XCreateIC の @c XNClientWindow に続く引数として用いられる。  */


  Window client_win;
  /***en Used as the argument of @c XCreateIC following @c XNFocusWindow.  */
  /***ja @c XCreateIC の @c XNFocusWindow に続く引数として用いられる。  */

  Window focus_win;
  /***en If non- @c NULL, used as the argument of @c XCreateIC following
      @c XNPreeditAttributes.  */
  /***ja @c NULLでなければ、 @c XCreateIC following の@c
      XNPreeditAttributes に続く引数として用いられる。  */

  XVaNestedList preedit_attrs;
  /***en If non- @c NULL, used as the argument of @c XCreateIC following
      @c XNStatusAttributes.  */ 
  /***ja @c NULLでなければ、 @c XCreateIC following の @c
      XNStatusAttributes に続く引数として用いられる。  */

  XVaNestedList status_attrs;
} MInputXIMArgIC;
/*=*/

#ifdef __cplusplus
}
#endif

#endif /* not _M17N_X_H_ */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
