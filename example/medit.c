/* medit.c -- simple multilingual editor.		-*- coding: euc-jp; -*-
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
    @enpage medit edit multilingual text

    @section medit-synopsis SYNOPSIS

    medit [ XT-OPTION ...] [ OPTION ... ] FILE

    @section medit-description DESCRIPTION

    Display FILE on a window and allow users to edit it.

    XT-OPTIONs are standard Xt arguments (e.g. -fn, -fg).

    The following OPTIONs are available.

    <ul>

    <li> --version

    Print version number.

    <li> -h, --help

    Print this message.

    </ul>

    This program is to demonstrate how to use the m17n GUI API.
    Although medit directly uses the GUI API, the API is mainly for
    toolkit libraries or to implement XOM (X Outout Method), not for
    direct use from application programs.
*/
/***ja
    @japage medit 多言語テキストの編集

    @section medit-synopsis SYNOPSIS

    medit [ XT-OPTION ...] [ OPTION ... ] FILE

    @section medit-description DESCRIPTION

    FILE をウィンドウに表示し、ユーザが編集できるようにする。

    XT-OPTIONs は Xt の標準の引数である。 (e.g. -fn, -fg). 

    以下のオプションが利用できる。 

    <ul>

    <li> --version

    バージョン番号を表示する。 

    <li> -h, --help

    このメッセージを表示する。 

    </ul>

    このプログラムは m17n GUI API の使い方を示すものである。medit は直 
    接 GUI API を使っているが、この API は主にツールキットライブラリや
    XOM (X Outout Method) の実装用であり、アプリケーションプログラ ム
    からの直接の利用を意図していない。
*/

#ifndef FOR_DOXYGEN

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <locale.h>

#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/Scrollbar.h>
#include <X11/Xaw/Toggle.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/MenuButton.h>

#include <m17n-gui.h>
#include <m17n-misc.h>
#include <m17n-X.h>

#define VERSION "1.0.1"

/* Global variables.  */

char *filename;
int serialized;

/* For the X Window System.  */
Display *display;
int screen;
/* GCs for normal drawing, filling by background color, normal drawing
   on bitmap (i.e. pixmap of depth 1), filling bitmap by background
   color. */
GC gc, gc_inv, mono_gc, mono_gc_inv;
Window win;
Atom XA_TEXT, XA_COMPOUND_TEXT, XA_UTF8_STRING;	/* X Selection types.  */
XtAppContext context;
int default_font_size;

/* Widget hierarchy

Shell - Form -+- Head -- File, Cursor, Bidi, LineBreak, InputMethod, CurIM;
	      +- Face -- Size, Family, Style, Color, Misc, Pop, CurFace
	      +- Lang -- A-B, C-D, ..., U-Z, Pop, CurLang
	      +- Body -- Sbar, Text
	      +- Tail -- Message
*/

Widget ShellWidget, HeadWidget, TailWidget, MessageWidget;
Widget CursorMenus[5], BidiMenus[3], LineBreakMenus[3], *InputMethodMenus;
Widget SbarWidget, TextWidget;
Widget FileShellWidget, FileDialogWidget;
Widget FaceWidget, CurFaceWidget, LangWidget, CurLangWidget;
Widget CurIMLang, CurIMStatus;

int win_width, win_height;	/* Size of TextWidget.  */
Arg arg[10];

Pixmap input_status_pixmap;
int input_status_width, input_status_height;

/* Bitmap for "check" glyph.  */
#define check_width 9
#define check_height 8
static unsigned char check_bits[] = {
   0x00, 0x01, 0x80, 0x01, 0xc0, 0x00, 0x60, 0x00,
   0x31, 0x00, 0x1b, 0x00, 0x0e, 0x00, 0x04, 0x00 };
Pixmap CheckPixmap;

/* For the m17n library.  */
MFrame *frame;
MText *mt;
int nchars;			/* == mtext_len (mt) */
MDrawControl control, input_status_control;
MTextProperty *selection;

MFace *face_default;
MFace *face_xxx_large;
MFace *face_box;
MFace *face_courier, *face_helvetica, *face_times;
MFace *face_dv_ttyogesh, *face_freesans, *face_freeserif, *face_freemono;
MFace *face_default_fontset, *face_no_ctl_fontset;
MFace *face_input_status;

MSymbol Mcoding_compound_text;

int logical_move = 1;		/* If 0, move cursor visually.  */

typedef struct {
  int available;
  MSymbol language, name;
  MInputMethod *im;
} InputMethodInfo;

InputMethodInfo *input_method_table;

int num_input_methods;
int current_input_method = -1;	/* i.e. none */
int auto_input_method = 0;
MInputContext *current_input_context;

struct FaceRec
{
  char *name;
  MFace **face;
} face_table[] =
  { {"Menu Size", NULL},
    {"xx-small", &mface_xx_small},
    {"x-small", &mface_x_small},
    {"small", &mface_small},
    {"normalsize", &mface_normalsize},
    {"large", &mface_large},
    {"x-large", &mface_x_large},
    {"xx-large", &mface_xx_large},
    {"xxx-large", &face_xxx_large},

    {"Menu Family", NULL},
    {"courier", &face_courier},
    {"helvetica", &face_helvetica},
    {"times", &face_times},
    {"dv-ttyogesh", &face_dv_ttyogesh},
    {"freesans", &face_freesans},
    {"freeserif", &face_freeserif},
    {"freemono", &face_freemono},

    {"Menu Style", NULL},
    {"medium", &mface_medium},
    {"bold", &mface_bold},
    {"italic", &mface_italic},

    {"Menu Color", NULL},
    {"black", &mface_black},
    {"white", &mface_white},
    {"red", &mface_red},
    {"green", &mface_green},
    {"blue", &mface_blue},
    {"cyan", &mface_cyan},
    {"yello", &mface_yellow},
    {"magenta", &mface_magenta},

    {"Menu Misc", NULL},
    {"normal", &mface_normal_video},
    {"reverse", &mface_reverse_video},
    {"underline", &mface_underline},
    {"box", &face_box},
    {"No CTL", &face_no_ctl_fontset} };


int num_faces = sizeof (face_table) / sizeof (struct FaceRec);

/* Information about a physical line metric.  */
struct LineInfo
{
  int from;			/* BOL position of the line.  */
  int to;			/* BOL position of the next line.  */
  int y0, y1;		 /* Top and bottom Y position of the line.  */
  int ascent;			/* Height of the top Y position.  */
};

struct LineInfo top;		/* Topmost line.  */
struct LineInfo cur;		/* Line containing cursor.  */
struct LineInfo sel_start;     /* Line containing selection start.  */
struct LineInfo sel_end;	/* Line containing selection end.  */

MDrawGlyphInfo cursor;	    /* Information about the cursor glyph.  */

/* X position to keep on vertical (up and down) cursor motion. */
int target_x_position;

/* Interface macros for m17n-lib drawing routines. */

/* Draw a text in the range $FROM to $TO of the M-text #MT at the
   coordinate ($X, $Y)  */
#define DRAW_TEXT(x, y, from, to)				\
    mdraw_text_with_control					\
      (frame, (MDrawWindow) win,				\
       control.orientation_reversed ? x + win_width : x, y,	\
       mt, from, to, &control)

/* Store the extents of a text in the range $FROM to $TO in the
   structure $RECT (type MDrawMetric).  */
#define TEXT_EXTENTS(from, to, rect)	\
  mdraw_text_extents (frame, mt, from, (to), &control, NULL, NULL, &(rect))

/* Store the glyph information of a character at the position $POS in
   the struct $INFO (type MDrawGlyphInfo) assuming that the text from
   $FROM is written at the coordinate (0, 0).  */
#define GLYPH_INFO(from, pos, info)	\
  mdraw_glyph_info (frame, mt, from, (pos), &control, &(info))

/* Set $X and $Y to the coordinate of character at position $POS
   assuming that the text from $FROM is written at the coordinate (0,
   0).  */
#define COORDINATES_POSITION(from, pos, x, y)	\
  mdraw_coordinates_position (frame, mt, (from), (pos), (x), (y), &control)

/* Interface macros for X library.  */
#define COPY_AREA(y0, y1, to)	\
  XCopyArea (display, win, win, gc, 0, (y0), win_width, (y1) - (y0), 0, (to))

#define CLEAR_AREA(x, y, w, h)	\
  XClearArea (display, win, (x), (y), (w), (h), False)

#define SELECTEDP() \
  mtext_property_mtext (selection)

/* Format MSG by FMT and print the result to the stderr, and exit.  */
#define FATAL_ERROR(fmt, arg)	\
  do {				\
    fprintf (stderr, fmt, arg);	\
    exit (1);			\
  } while (0)


/* If POS is greater than zero, move POS back to the beginning of line
   (BOL) position.  If FORWARD is nonzero, move POS forward instead.
   Return the new position.  */
int
bol (int pos, int forward)
{
  int limit = forward ? nchars : 0;

  pos = mtext_character (mt, pos, limit, '\n');
  return (pos < 0 ? limit : pos + 1);
}

/* Update the structure #TOP (struct LineInfo) to make $POS the first
   character position of the screen.  */
void
update_top (int pos)
{
  int from = bol (pos, 0);
  MDrawGlyphInfo info;

  GLYPH_INFO (from, pos, info);
  top.from = info.line_from;
  top.to = info.line_to;
  top.y0 = 0;
  top.y1 = info.this.height;
  top.ascent = - info.this.y;
}


/* Update the scroll bar so that the text of the range $FROM to $TO
   are shown on the window.  */
void
update_scroll_bar (int from, int to)
{
  float top = (float) from / nchars;
  float shown = (float) (to - from) / nchars;
  XtArgVal *l_top = (XtArgVal *) &top;
  XtArgVal *l_shown = (XtArgVal *) &shown;

  XtSetArg (arg[0], XtNtopOfThumb, *l_top);
  XtSetArg (arg[1], XtNshown, *l_shown);
  XtSetValues (SbarWidget, arg, 2);
}


/* Redraw the window area between $Y0 and $Y1 (both Y-codinates).  If
   $CLEAR is nonzero, clear the area before drawing.  If $SCROLL_BAR
   is nonzero, update the scoll bar.  */
void
redraw (int y0, int y1, int clear, int scroll_bar)
{
  int from, to;
  int y;
  MDrawGlyphInfo info;
  int sel_y0 = SELECTEDP () ? sel_start.y0 : 0;
  struct LineInfo *line;
  
  if (clear || control.anti_alias)
    CLEAR_AREA (0, y0, win_width, y1 - y0);

  /* Find a line closest to y0.  It is a cursor line if the cursor is
     Y0, otherwise the top line.  */
  if (y0 >= cur.y0)
    line = &cur;
  else
    line = &top;
  /* If there exists a selected region, check it too.  */
  if (sel_y0 > line->y0 && y0 >= sel_y0)
    line = &sel_start;

  from = line->from;
  y = line->y0;
  info.this.height = line->y1 - y;
  info.this.y = - line->ascent;
  info.line_to = line->to;
  while (from < nchars && y + info.this.height <= y0)
    {
      y += info.this.height;
      from = info.line_to;
      GLYPH_INFO (from, from, info);
    }
  y0 = y - info.this.y;
  to = from;
  while (to < nchars && y < y1)
    {
      GLYPH_INFO (to, to, info);
      y += info.this.height;
      to = info.line_to;
    }
  if (to == nchars)
    to++;
  if (from < to)
    DRAW_TEXT (0, y0, from, to);
  if (scroll_bar)
    {
      while (to < nchars)
	{
	  GLYPH_INFO (to, to, info);
	  if (y + info.this.height >= win_height)
	    break;
	  to = info.line_to;
	  y += info.this.height;
	}
      update_scroll_bar (top.from, to);
    }
}
     

/* Set the current input method spot to the correct position.  */
void
set_input_method_spot ()
{
  int x = cursor.x + (control.orientation_reversed ? win_width : 0);
  int pos = cursor.from > 0 ? cursor.from - 1 : 0;
  MFace *faces[256];
  int n = mtext_get_prop_values (mt, pos, Mface, (void **) faces, 256);
  int size = 0, ratio = 0, i;

  for (i = n - 1; i >= 0; i--)
    {
      if (! size)
	size = (int) mface_get_prop (faces[i], Msize);
      if (! ratio)
	ratio = (int) mface_get_prop (faces[i], Mratio);
    }
  if (! size)
    size = default_font_size;
  if (ratio)
    size = size * ratio / 100;
  minput_set_spot (current_input_context, x, cur.y0 + cur.ascent,
		   cur.ascent, cur.y1 - (cur.y0 + cur.ascent), size,
		   mt, cursor.from);
}


/* Redraw the cursor.  If $CLEAR is nonzero, clear the cursor area
   before drawing.  */
void
redraw_cursor (int clear)
{
  if (control.cursor_bidi)
    {
      /* We must update the whole line of the cursor.  */
      int beg = bol (cur.from, 0);
      int end = bol (cur.to - 1, 1);
      MDrawMetric rect;
      int y0 = cur.y0, y1 = cur.y1;

      if (beg != cur.from)
	{
	  TEXT_EXTENTS (beg, cur.from, rect);
	  y0 -= rect.height;
	}
      if (end != cur.to)
	{
	  TEXT_EXTENTS (cur.to, end, rect);
	  y1 += rect.height; 
	}
      redraw (y0, y1, clear, 0);
    }
  else
    {
      if (clear)
	{
	  int x = cursor.x;

	  if (control.orientation_reversed)
	    x += win_width - cursor.logical_width;
	  CLEAR_AREA (x, cur.y0, cursor.logical_width, cursor.this.height);
	}
      DRAW_TEXT (cursor.x, cur.y0 + cur.ascent, cursor.from, cursor.to);
    }
}


/* Update the information about the location of cursor to the position
   $POS.  If $FULL is nonzero, update the information fully only from
   the information about the top line.  Otherwise, truct the current
   information in the structure $CUR.  */
void
update_cursor (int pos, int full)
{
  MDrawMetric rect;

  if (full)
    {
      /* CUR is inaccurate.  We can trust only TOP.  */
      GLYPH_INFO (top.from, pos, cursor);
      cur.y0 = top.ascent + cursor.y + cursor.this.y;
    }
  else if (pos < cur.from)
    {
      int from = bol (pos, 0);

      TEXT_EXTENTS (from, cur.from, rect);
      GLYPH_INFO (from, pos, cursor);
      cur.y0 -= (rect.height + rect.y) - (cursor.y + cursor.this.y);
    }
  else if (pos < cur.to)
    {
      GLYPH_INFO (cur.from, pos, cursor);
    }
  else
    {
      GLYPH_INFO (cur.from, pos, cursor);
      cur.y0 += cur.ascent + cursor.y + cursor.this.y;
    }

  cur.from = cursor.line_from;
  cur.to = cursor.line_to;
  cur.y1 = cur.y0 + cursor.this.height;
  cur.ascent = - cursor.this.y;
}


/* Update the information about the selected region.  */
void
update_selection ()
{
  int from, to;
  MDrawMetric rect;
  MDrawGlyphInfo info;

  if (! SELECTEDP ())
    return;
  from = mtext_property_start (selection);
  to = mtext_property_end (selection);

  if (from < top.from)
    {
      int pos = bol (from, 0);

      TEXT_EXTENTS (pos, top.from, rect);
      sel_start.y0 = top.y0 - rect.height;
      sel_start.ascent = - rect.y;
      GLYPH_INFO (pos, from, info);
      if (pos < info.line_from)
	sel_start.y0 += - rect.y + info.y + info.this.y;
    }
  else
    {
      GLYPH_INFO (top.from, from, info);
      sel_start.y0 = top.ascent + info.y + info.this.y;
    }
  sel_start.ascent = -info.this.y;
  sel_start.y1 = sel_start.y0 + info.this.height;
  sel_start.from = info.line_from;
  sel_start.to = info.line_to;

  if (to <= sel_start.to)
    {
      sel_end = sel_start;
      if (to >= sel_end.to)
	{
	  GLYPH_INFO (sel_start.from, to, info);
	  sel_end.y1 = sel_end.y0 + info.y + info.this.height;
	  sel_end.to = info.line_to;
	}
    }
  else
    {
      GLYPH_INFO (sel_start.from, to, info);
      sel_end.y0 = sel_start.y0 + sel_start.ascent + info.y + info.this.y;
      sel_end.y1 = sel_end.y0 + info.this.height;
      sel_end.ascent = - info.this.y;
      sel_end.from = info.line_from;
      sel_end.to = info.line_to;
    }
}


/* Select the text in the region from $FROM to $TO.  */
void
select_region (int from, int to)
{
  int pos;

  if (from > to)
    pos = from, from = to, to = pos;
  mtext_push_property (mt, from, to, selection);
  update_selection ();
}


/* Setup the window to display the character of $POS at the top left
   of the window.  */
void
reseat (int pos)
{
  MDrawMetric rect;
  /* Top and bottom Y positions to redraw.  */
  int y0, y1;

  if (pos + 1000 < top.from)
    y0 = 0, y1 = win_height;
  else if (pos < top.from)
    {
      y0 = 0;
      TEXT_EXTENTS (pos, top.from, rect);
      if (rect.height >= win_height * 0.9)
	y1 = win_height;
      else
	{
	  y1 = rect.height;
	  COPY_AREA (0, win_height - y1, y1);
	}
    }
  else if (pos < top.to)
    {
      /* No need of redrawing.  */
      y0 = y1 = 0;
    }
  else if (pos < top.from + 1000)
    {
      TEXT_EXTENTS (top.from, pos, rect);
      if (rect.height >= win_height * 0.9)
	y0 = 0;
      else
	{
	  y0 = win_height - rect.height;
	  COPY_AREA (rect.height, win_height, 0);
	}
      y1 = win_height;
    }
  else
    y0 = 0, y1 = win_height;

  if (y0 < y1)
    {
      update_top (pos);
      if (cur.to <= pos)
	update_cursor (pos, 1);
      else
	update_cursor (cursor.from, 1);
      update_selection ();
      redraw (y0, y1, 1, 1);
    }
}

static void MenuHelpProc (Widget, XEvent *, String *, Cardinal *);


/* Select an input method accoding to $IDX.  If $IDX is negative, turn
   off the current input method, otherwide turn on the input method
   input_method_table[$IDX].  */
void
select_input_method (idx)
{
  if (idx == current_input_method)
    return;
  if (current_input_context)
    {
      minput_destroy_ic (current_input_context);
      current_input_context = NULL;
      current_input_method = -1;
    }
  if (idx >= 0)
    {
      InputMethodInfo *im = input_method_table + idx;

      if (im->language == Mnil)
	{
	  MInputXIMArgIC arg_xic;
	  Window win = XtWindow (TextWidget);

	  arg_xic.input_style = 0;
	  arg_xic.client_win = arg_xic.focus_win = win;
	  arg_xic.preedit_attrs =  arg_xic.status_attrs = NULL;
	  current_input_context = minput_create_ic (im->im, &arg_xic);
	}
      else
	{
	  MInputGUIArgIC arg_ic;

	  arg_ic.frame = frame;
	  arg_ic.client = (MDrawWindow) XtWindow (ShellWidget);
	  arg_ic.focus = (MDrawWindow) XtWindow (TextWidget);
	  current_input_context = minput_create_ic (im->im, &arg_ic);
	}

      if (current_input_context)
	{
	  set_input_method_spot ();
	  current_input_method = idx;
	}
    }
  if (current_input_method >= 0)
    {
      char *label;
      XtSetArg (arg[0], XtNlabel, &label);
      XtGetValues (InputMethodMenus[current_input_method + 2], arg, 1);
      XtSetArg (arg[0], XtNlabel, label);
    }
  else
    XtSetArg (arg[0], XtNlabel, "");
  XtSetValues (CurIMLang, arg, 1);
}

static void MenuHelpProc (Widget w, XEvent *event, String *str, Cardinal *num);


/* Display cursor according to the current information of #CUR.
   $CLIENT_DATA is ignore.  Most callback functions add this function
   as a background processing procedure the current application (by
   XtAppAddWorkProc) via the function hide_cursor.  */
Boolean
show_cursor (XtPointer client_data)
{
  MFaceHLineProp *hline;
  MFaceBoxProp *box;

  if (cur.y0 < 0)
    {
      reseat (cur.from);
      update_cursor (cursor.from, 1);
    }
  while (cur.y1 > win_height)
    {
      reseat (top.to);
      update_cursor (cursor.from, 1);
    }

  control.cursor_pos = cursor.from;
  if (! SELECTEDP ())
    {
      control.with_cursor = 1;
      redraw_cursor (0);
    }
  if (current_input_context)
    set_input_method_spot ();

  {
    int pos = (SELECTEDP () ? mtext_property_start (selection)
	       : cursor.from > 0 ? cursor.from - 1
	       : cursor.from);
    MFace *face = mface ();
    MTextProperty *props[256];
    int n = mtext_get_properties (mt, pos, Mface, props, 256);
    int i;
    char buf[256], *p = buf;
    MSymbol sym;

    buf[0] = '\0';
    if (cursor.font)
      {
	int size = (int) mfont_get_prop (cursor.font, Msize);
	MSymbol family = mfont_get_prop (cursor.font, Mfamily);
	MSymbol weight = mfont_get_prop (cursor.font, Mweight);
	MSymbol style = mfont_get_prop (cursor.font, Mstyle);
	MSymbol registry = mfont_get_prop (cursor.font, Mregistry);

	sprintf (p, "%dpt", size / 10), p += strlen (p);
	if (family)
	  strcat (p, ","), strcat (p, msymbol_name (family)), p += strlen (p);
	if (weight)
	  strcat (p, ","), strcat (p, msymbol_name (weight)), p += strlen (p);
	if (style)
	  strcat (p, ","), strcat (p, msymbol_name (style)), p += strlen (p);
	if (registry)
	  strcat (p, ","), strcat (p, msymbol_name (registry)), p += strlen (p);
	p += strlen (p);
      }

    mface_merge (face, face_default);
    for (i = 0; i < n; i++)
      if (props[i] != selection)
	mface_merge (face, (MFace *) mtext_property_value (props[i]));
    sym = (MSymbol) mface_get_prop (face, Mforeground);
    if (sym != Mnil)
      strcat (p, ","), strcat (p, msymbol_name (sym)), p += strlen (p);
    if ((MSymbol) mface_get_prop (face, Mvideomode) == Mreverse)
      strcat (p, ",rev"), p += strlen (p);
    hline = mface_get_prop (face, Mhline);
    if (hline && hline->width > 0)
      strcat (p, ",ul"), p += strlen (p);
    box = mface_get_prop (face, Mbox);
    if (box && box->width > 0)
      strcat (p, ",box"), p += strlen (p);
    m17n_object_unref (face);

    XtSetArg (arg[0], XtNborderWidth, 1);
    XtSetArg (arg[1], XtNlabel, buf);
    XtSetValues (CurFaceWidget, arg, 2);
  }

  if (control.cursor_pos < nchars)
    {
      MSymbol sym = Mnil;

      if (control.cursor_pos > 0
	  && mtext_ref_char (mt, control.cursor_pos - 1) != '\n')
	sym = mtext_get_prop (mt, control.cursor_pos - 1, Mlanguage);
      if (sym == Mnil)
	sym = mtext_get_prop (mt, control.cursor_pos, Mlanguage);
    
      if (sym == Mnil)
	{
	  XtSetArg (arg[0], XtNborderWidth, 0);
	  XtSetArg (arg[1], XtNlabel, "");
	}
      else
	{
	  XtSetArg (arg[0], XtNborderWidth, 1);
	  XtSetArg (arg[1], XtNlabel,
		    msymbol_name (msymbol_get (sym, Mlanguage)));
	  XtSetValues (CurLangWidget, arg, 2);
	}
      XtSetValues (CurLangWidget, arg, 2);

      if (auto_input_method)
	{
	  if (sym == Mnil)
	    select_input_method (-1);
	  else
	    {
	      int i;

	      for (i = 0; i < num_input_methods; i++)
		if (input_method_table[i].language == sym)
		  break;
	      if (i < num_input_methods
		  && input_method_table[i].available >= 0)
		{
		  if (! input_method_table[i].im)
		    {
		      input_method_table[i].im = 
			minput_open_im (input_method_table[i].language,
					input_method_table[i].name, NULL);
		      if (! input_method_table[i].im)
			input_method_table[i].available = -1;
		    }
		  if (input_method_table[i].im)
		    select_input_method (i);
		  else
		    select_input_method (-1);
		}
	      else
		select_input_method (-1);
	    }
	}
    }

  MenuHelpProc (MessageWidget, NULL, NULL, NULL);

  return True;
}


/* Hide the cursor.  */
void
hide_cursor ()
{
  control.with_cursor = 0;
  redraw_cursor (1);
  XtAppAddWorkProc (context, show_cursor, NULL);
}


/* Update the window area between the Y-positions $Y0 and $OLD_Y1 to
   $Y1 and $NEW_Y1 assuming that the text in the other area is not
   changed.  */
void
update_region (int y0, int old_y1, int new_y1)
{
  if (y0 < 0)
    y0 = 0;
  if (new_y1 < old_y1)
    {
      if (old_y1 < win_height)
	{
	  COPY_AREA (old_y1, win_height, new_y1);
	  redraw (win_height - (old_y1 - new_y1), win_height, 1, 0);
	}
      else
	redraw (new_y1, win_height, 1, 0);
    }
  else if (new_y1 > old_y1)
    {
      if (new_y1 < win_height)
	COPY_AREA (old_y1, win_height, new_y1);
    }
  if (new_y1 > win_height)
    new_y1 = win_height;
  redraw (y0, new_y1, 1, 1);
}


/* Delete the next $N characters.  If $N is negative delete the
   precious (- $N) characters.  */
void
delete_char (int n)
{
  MDrawMetric rect;
  MDrawGlyphInfo info;
  int old_y1, new_y1;
  int from, to;

  if (n > 0)
    from = cursor.from, to = from + n;
  else
    {
      if (cursor.from == cur.from)
	{
	  /* We are at the beginning of line.  */
	  int pos = cursor.prev_from;
	  
	  if (cursor.from == top.from)
	    {
	      /* We are at the beginning of screen.  We must scroll
		 down.  */
	      GLYPH_INFO (bol (top.from - 1, 0), top.from - 1, info);
	      reseat (info.line_from);
	    }
	  update_cursor (pos, 1);
	  from = cursor.from;
	  to = cursor.to;
	}
      else
	{
	  from = cursor.from - 1;
	  to = cursor.from;
	}
    }

  TEXT_EXTENTS (cur.from, bol (to + 1, 1), rect);
  old_y1 = cur.y0 + rect.height;

  /* Now delete a character.  */
  mtext_del (mt, from, to);
  nchars--;
  if (from >= top.from && from < top.to)
    update_top (top.from);
  update_cursor (from, 1);

  TEXT_EXTENTS (cur.from, bol (to, 1), rect);
  new_y1 = cur.y0 + rect.height;

  update_region (cur.y0, old_y1, new_y1);
}


/* Insert M-text $NEWTEXT at the current cursor position.  */
void
insert_chars (MText *newtext)
{
  int n = mtext_len (newtext);
  MDrawMetric rect;
  int y0, old_y1, new_y1;

  if (SELECTEDP ())
    {
      int n = (mtext_property_end (selection)
	       - mtext_property_start (selection));
      mtext_detach_property (selection);
      delete_char (n);
    }

  y0 = cur.y0;
  TEXT_EXTENTS (cur.from, bol (cur.to - 1, 1), rect);
  old_y1 = y0 + rect.height;

  /* Now insert chars.  */
  mtext_ins (mt, cursor.from, newtext);
  nchars += n;
  if (cur.from == top.from)
    update_top (top.from);
  update_cursor (cursor.from + n, 1);

  TEXT_EXTENTS (cur.from, bol (cur.to - 1, 1), rect);
  new_y1 = cur.y0 + rect.height;

  update_region (y0, old_y1, new_y1);
  update_selection ();
}


/* Convert the currently selected text to UTF8-STRING or
   COMPOUND-TEXT.  It is called when someone requests the current
   value of the selection.  */
Boolean
covert_selection (Widget w, Atom *selection_atom,
		  Atom *target, Atom *return_type,
		  XtPointer *value, unsigned long *length, int *format)
{
  unsigned char *buf = (unsigned char *) XtMalloc (4096);
  MText *this_mt = mtext ();
  int from = mtext_property_start (selection);
  int to = mtext_property_end (selection);
  MSymbol coding;

  mtext_copy (this_mt, 0, mt, from, to);
  if (*target == XA_TEXT)
    {
#ifdef X_HAVE_UTF8_STRING
      coding = Mcoding_utf_8;
      *return_type = XA_UTF8_STRING;
#else
      coding = Mcoding_compound_text;
      *return_type = XA_COMPOUND_TEXT;
#endif
    }
  else if (*target == XA_STRING)
    {
      int len = to - from;
      int i;

      for (i = 0; i < len; i++)
	if (mtext_ref_char (this_mt, i) >= 0x100)
	  /* Can't encode in XA_STRING */
	  return False;
      coding = Mcoding_iso_8859_1;
      *return_type = XA_STRING;
    }
  else if (*target == XA_COMPOUND_TEXT)
    {
      coding = Mcoding_compound_text;
      *return_type = XA_COMPOUND_TEXT;
    }
  *length = mconv_encode_buffer (coding, this_mt, buf, 4096);
  m17n_object_unref (this_mt);
  if (*length == 0)
    return False;
  *value = (XtPointer) buf;
  *format = 8;
  return True;
}


/* Unselect the text.  It is called when we loose the selection.  */
void
lose_selection (Widget w, Atom *selection_atom)
{
  if (SELECTEDP ())
    {
      mtext_detach_property (selection);
      redraw (sel_start.y0, sel_end.y1, 1, 0);
    }
}

void
get_selection (Widget w, XtPointer cliend_data, Atom *selection,  Atom *type,
	       XtPointer value, unsigned long *length, int *format)
{
  MText *this_mt;
  MSymbol coding;

  if (*type == XT_CONVERT_FAIL || ! value)
    goto err;
  if (*type == XA_STRING)
    coding = Mnil;
  else if (*type == XA_COMPOUND_TEXT)
    coding = msymbol ("compound-text");
#ifdef X_HAVE_UTF8_STRING
  else if (*type == XA_UTF8_STRING)
    coding = msymbol ("utf-8");
#endif
  else
    goto err;

  this_mt = mconv_decode_buffer (coding, (unsigned char *) value, *length);
  if (this_mt)
    {
      hide_cursor ();
      insert_chars (this_mt);
      m17n_object_unref (this_mt);
    }

 err:
  if (value)
    XtFree (value);
}

static void
ExposeProc (Widget w, XEvent *event, String *str, Cardinal *num)
{
  XExposeEvent *expose = (XExposeEvent *) event;

  if (top.from < 0)
    {
      Dimension width_max, width;

      XtSetArg (arg[0], XtNwidth, &width);
      XtGetValues (XtParent (w), arg, 1);
      width_max = width;
      XtGetValues (HeadWidget, arg, 1);
      if (width_max < width)
	width_max = width;
      XtGetValues (FaceWidget, arg, 1);
      if (width_max < width)
	width_max = width;
      XtGetValues (LangWidget, arg, 1);
      if (width_max < width)
	width_max = width;
      XtSetArg (arg[0], XtNwidth, width_max);
      XtSetValues (HeadWidget, arg, 1);
      XtSetValues (FaceWidget, arg, 1);
      XtSetValues (LangWidget, arg, 1);
      XtSetValues (XtParent (w), arg, 1);
      XtSetValues (TailWidget, arg, 1);

      update_top (0);
      update_cursor (0, 1);
      redraw (0, win_height, 0, 1);
      show_cursor (NULL);
    }
  else
    {
      redraw (expose->y, expose->y + expose->height, 0, 0);
      if (current_input_context
	  && expose->y < cur.y0 && expose->y + expose->height < cur.y1)
	set_input_method_spot ();
    }
}

static void
ConfigureProc (Widget w, XEvent *event, String *str, Cardinal *num)
{
  XConfigureEvent *configure = (XConfigureEvent *) event;
  
  hide_cursor ();
  control.max_line_width = win_width = configure->width;
  win_height = configure->height;
  mdraw_clear_cache (mt);
  update_top (0);
  update_cursor (0, 1);
  redraw (0, win_height, 1, 1);
  if (current_input_context)
    set_input_method_spot ();
}

static void
ButtonProc (Widget w, XEvent *event, String *str, Cardinal *num)
{
  int pos;
  int x = event->xbutton.x;
  int y = event->xbutton.y - top.ascent;

  if (control.orientation_reversed)
    x -= win_width;
  pos = COORDINATES_POSITION (top.from, nchars + 1, x, y);
  if (SELECTEDP ())
    {
      XtDisownSelection (w, XA_PRIMARY, CurrentTime);
      mtext_detach_property (selection);
      redraw (sel_start.y0, sel_end.y1, 1, 0);
    }
  hide_cursor ();
  update_cursor (pos, 0);
}


static void
ButtonReleaseProc (Widget w, XEvent *event, String *str, Cardinal *num)
{
  if (! SELECTEDP ())
    return;

  XtOwnSelection (w, XA_PRIMARY, CurrentTime,
		  covert_selection, lose_selection, NULL);
  update_cursor (mtext_property_start (selection), 0);
}

static
void
Button2Proc (Widget w, XEvent *event, String *str, Cardinal *num)
{
  if (! SELECTEDP ())
    {
      /* We don't have a local selection.  */
      XtGetSelectionValue (w, XA_PRIMARY, XA_TEXT, get_selection, NULL,
			   CurrentTime);
    }
  else
    {
      int from = mtext_property_start (selection);
      int to = mtext_property_end (selection);
      MText *this_mt;
      int pos;
      int x = event->xbutton.x;
      int y = event->xbutton.y - top.ascent;

      if (control.orientation_reversed)
	x -= win_width;
      pos = COORDINATES_POSITION (top.from, nchars + 1, x, y);
      
      XtDisownSelection (w, XA_PRIMARY, CurrentTime);
      mtext_detach_property (selection);
      hide_cursor ();
      this_mt = mtext_copy (mtext (), 0, mt, from, to);
      update_cursor (pos, 0);
      insert_chars (this_mt);
      m17n_object_unref (this_mt);
    }
}

static void
ButtonMoveProc (Widget w, XEvent *event, String *str, Cardinal *num)
{
  int pos;
  int x = event->xbutton.x;
  int y = event->xbutton.y;

  if (control.orientation_reversed)
    x -= win_width;
  if (y < cur.y0)
    pos = top.from, y -= top.ascent;
  else
    pos = cur.from, y -= cur.y0 + cur.ascent;
  pos = COORDINATES_POSITION (pos, nchars + 1, x, y);

  if (pos == cursor.from)
    return;

  hide_cursor ();
  if (SELECTEDP ())
    {
      /* Selection range changed.  */
      int from = mtext_property_start (selection);
      int to = mtext_property_end (selection);
      int start_y0 = sel_start.y0, start_y1 = sel_start.y1;
      int end_y0 = sel_end.y0, end_y1 = sel_end.y1;

      if (cursor.from == from)
	{
	  /* Starting position changed.  */
	  if (pos <= from)
	    {
	      /* Enlarged.  We can simply overdraw.  */
	      select_region (pos, to);
	      redraw (sel_start.y0, start_y1, 0, 0);
	    }
	  else if (pos < to)
	    {
	      /* Shrunken.  Previous selection face must be cleared.  */
	      select_region (pos, to);
	      redraw (start_y0, sel_start.y1, 1, 0);
	    }
	  else if (pos == to)
	    {
	      /* Shrunken to zero.  */
	      XtDisownSelection (w, XA_PRIMARY, CurrentTime);
	      mtext_detach_property (selection);
	      redraw (start_y0, end_y1, 1, 0);
	    }
	  else
	    {
	      /* Full update is necessary.  */
	      select_region (to, pos);
	      redraw (start_y0, sel_end.y1, 1, 0);
	    }
	}
      else
	{
	  /* Ending position changed.  */
	  if (pos < from)
	    {
	      /* Full update is necessary.  */
	      select_region (pos, from);
	      redraw (sel_start.y0, end_y1, 1, 0);
	    }
	  else if (pos == from)
	    {
	      /* Shrunken to zero.  */
	      XtDisownSelection (w, XA_PRIMARY, CurrentTime);
	      mtext_detach_property (selection);
	      redraw (start_y0, end_y1, 1, 0);
	    }
	  else if (pos < to)
	    {
	      /* Shrunken.  Previous selection face must be cleared.  */
	      select_region (from, pos);
	      redraw (sel_end.y0, end_y1, 1, 0);
	    }
	  else
	    {
	      /* Enlarged.  We can simply overdraw.  */
	      select_region (from, pos);
	      redraw (end_y0, sel_end.y1, 0, 0);
	    }
	}
    }
  else
    {
      /* Newly selected.  */
      select_region (pos, cursor.from);
      redraw (sel_start.y0, sel_end.y1, 0, 0);
    }
  update_cursor (pos, 1);
}

void
ScrollProc (Widget w, XtPointer client_data, XtPointer position)
{
  int from;
  MDrawGlyphInfo info;
  int height;
  int cursor_pos = cursor.from;

  if (((int) position) < 0)
    {
      /* Scroll down.  */
      int pos;

      from = top.from;
      height = top.y1 - top.y0;
      while (from > 0)
	{
	  pos = bol (from - 1, 0);
	  GLYPH_INFO (pos, from - 1, info);
	  if (height + info.this.height > win_height)
	    break;
	  height += info.this.height;
	  from = info.line_from;
	}
      if (cursor_pos >= top.to)
	{
	  cursor_pos = top.from;
	  pos = top.to;
	  while (cursor_pos < nchars)
	    {
	      GLYPH_INFO (pos, pos, info);
	      if (height + info.this.height > win_height)
		break;
	      height += info.this.height;
	      cursor_pos = pos;
	      pos = info.line_to;
	    }
	}
    }
  else if (cur.to < nchars)
    {
      /* Scroll up, but leave at least one line.  */
      from = cur.to;
      height = cur.y1;
      while (from < nchars)
	{
	  GLYPH_INFO (from, from, info);
	  if (height + info.this.height > win_height
	      || info.line_to >= nchars)
	    break;
	  height += info.this.height;
	  from = info.line_to;
	}
      if (from == nchars)
	from = info.line_from;
      if (cursor_pos < from)
	cursor_pos = from;
    }
  else
    /* Scroll up to make the cursor line top.  */
    from = cur.from;
  hide_cursor ();
  reseat (from);
  update_cursor (cursor_pos, 1);
}

void
JumpProc (Widget w, XtPointer client_data, XtPointer persent_ptr)
{
  float persent = *(float *) persent_ptr;
  int pos1, pos2 = nchars * persent;
  MDrawGlyphInfo info;

  hide_cursor ();
  pos1 = bol (pos2, 0);
  GLYPH_INFO (pos1, pos2, info);
  pos1 = info.line_from;
  reseat (pos1);
  update_cursor (pos1, 1);
}


static void
KeyProc (Widget w, XEvent *event, String *str, Cardinal *num)
{
  XKeyEvent *key_event = (XKeyEvent *) event;
  char buf[512];
  KeySym keysym = NoSymbol;
  int ret;
  /* If set to 1, do not update target_x_position.  */
  int keep_target_x_position = 0;
  MText *produced;

  if (current_input_context
      && minput_filter (current_input_context, Mnil, event))
    return;
  if (event->type == KeyRelease)
    return;

  hide_cursor ();

  produced = mtext ();
  ret = minput_lookup (current_input_context, Mnil, event, produced);
  if (mtext_len (produced) > 0)
    insert_chars (produced);
  if (ret)
    ret = XLookupString (key_event, buf, sizeof (buf), &keysym, NULL);
  m17n_object_unref (produced);

  switch (keysym)
    {
    case XK_Delete:
      {
	int n = 0;

	if (SELECTEDP ())
	  {
	    n = (mtext_property_end (selection)
		 - mtext_property_start (selection));
	    mtext_detach_property (selection);
	  }
	else if (cursor.from < nchars)
	  {
	    /* Delete the following grapheme cluster.  */
	    n = cursor.to - cursor.from;
	  }
	if (n != 0)
	  delete_char (n);
      }
      break;

    case XK_BackSpace:
      {
	int n = 0;

	if (SELECTEDP ())
	  {
	    /* Delete selected region.  */
	    n = (mtext_property_end (selection)
		 - mtext_property_start (selection));
	    mtext_detach_property (selection);
	  }
	else if (cursor.from > 0)
	  {
	    /* Delete the preceding character.  */
	    n = -1;
	  }
	if (n != 0)
	  delete_char (n);
      }
      break;

    case XK_Left:
      if (SELECTEDP ())
	{
	  mtext_detach_property (selection);
	  redraw (sel_start.y0, sel_end.y1, 1, 0);;
	}
      if (logical_move)
	{
	  if (cursor.prev_from >= 0)
	    update_cursor (cursor.prev_from, 0);
	}
      else
	{
	  if (cursor.left_from >= 0)
	    update_cursor (cursor.left_from, 0);
	}
      break;

    case XK_Right:
      if (SELECTEDP ())
	{
	  mtext_detach_property (selection);
	  redraw (sel_start.y0, sel_end.y1, 1, 0);;
	}
      if (logical_move)
	{
	  if (cursor.next_to >= 0)
	    update_cursor (cursor.to, 0);
	}
      else
	{
	  if (cursor.right_from >= 0)
	    update_cursor (cursor.right_from, 0);
	}
      break;

    case XK_Down:
      if (SELECTEDP ())
	{
	  mtext_detach_property (selection);
	  redraw (sel_start.y0, sel_end.y1, 1, 0);;
	}
      if (cur.to <= nchars)
	{
	  MDrawGlyphInfo info;
	  int pos;

	  GLYPH_INFO (cur.from, cur.to, info);
	  pos = COORDINATES_POSITION (cur.from, nchars + 1,
				      target_x_position, info.y);
	  keep_target_x_position = 1;
	  update_cursor (pos, 0);
	}
      break;

    case XK_Up:
      if (SELECTEDP ())
	{
	  mtext_detach_property (selection);
	  redraw (sel_start.y0, sel_end.y1, 1, 0);;
	}
      if (cur.from > 0)
	{
	  MDrawMetric rect;
	  int y;
	  int pos = bol (cur.from - 1, 0);

	  TEXT_EXTENTS (pos, cur.from - 1, rect);
	  y = rect.height + rect.y - 1;
	  pos = COORDINATES_POSITION (pos, nchars,
				      target_x_position, y);
	  keep_target_x_position = 1;
	  update_cursor (pos, 0);
	}
      break;

    case XK_Page_Down:
      if (SELECTEDP ())
	{
	  mtext_detach_property (selection);
	  redraw (sel_start.y0, sel_end.y1, 1, 0);;
	}
      if (top.from < nchars)
	ScrollProc (w, NULL, (XtPointer) 1);
      break;

    case XK_Page_Up:
      if (SELECTEDP ())
	{
	  mtext_detach_property (selection);
	  redraw (sel_start.y0, sel_end.y1, 1, 0);;
	}
      if (top.from > 0)
	ScrollProc (w, NULL, (XtPointer) -1);
      break;

    default:
      if (ret > 0)
	{
	  if (buf[0] == 17) /* C-q */
	    {
	      XtAppSetExitFlag (context);
	      return;
	    }
	  else if (buf[0] == 12) /* C-l */
	    {
	      redraw (0, win_height, 1, 1);
	      return;
	    }
	  else
	    {
	      MText *temp = mtext ();

	      mtext_cat_char (temp, buf[0] == '\r' ? '\n' : buf[0]);
	      if (current_input_context)
		mtext_put_prop (temp, 0, 1, Mlanguage,
				current_input_context->im->language);
	      insert_chars (temp);
	      m17n_object_unref (temp);
	    }
	}
    }

  if (! keep_target_x_position)
    target_x_position = cursor.x;
}

void
SaveProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  char *name = (char *) client_data;
  FILE *fp;
  int from = -1, to = 0;
  
  if (name)
    {
      free (filename);
      filename = strdup (name);
    }

  fp = fopen (filename, "w");
  if (! fp)
    {
      fprintf (stderr, "Open for write fail: %s", filename);
      return;
    }

  if (SELECTEDP ())
    {
      from = mtext_property_start (selection);
      to = mtext_property_end (selection);
      mtext_detach_property (selection);
    }

  mconv_encode_stream (Mcoding_utf_8, mt, fp);
  fclose (fp);
  if (from >= 0)
    select_region (from, to);
}

void
SerializeProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  MText *new;

  hide_cursor ();
  if (SELECTEDP ())
    mtext_detach_property (selection);
  serialized = (int) client_data;
  if (! serialized)
    new = mtext_deserialize (mt);
  else
    {
      MPlist *plist = mplist ();

      mplist_push (plist, Mt, Mface);
      mplist_push (plist, Mt, Mlanguage);
      new = mtext_serialize (mt, 0, mtext_len (mt), plist);
      m17n_object_unref (plist);
    }
  if (new)
    {
      m17n_object_unref (mt);
      mt = new;
      serialized = ! serialized;
      nchars = mtext_len (mt);
      update_top (0);
    }
  update_cursor (0, 1);
  redraw (0, win_height, 1, 1);
}

void
QuitProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  XtAppSetExitFlag (context);
}

MText *
read_file ()
{
  FILE *fp = fopen (filename, "r");

  if (! fp)
    FATAL_ERROR ("Can't read \"%s\"!\n", filename);
  mt = mconv_decode_stream (Mcoding_utf_8, fp);
  fclose (fp);
  if (! mt)
    FATAL_ERROR ("Can't decode \"%s\" by UTF-8!\n", filename);
  return mt;
}

void
BidiProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  int data = (int) client_data;
  int i;

  if (data == 0)
    {
      control.enable_bidi = 0;
      control.orientation_reversed = 0;
    }
  else
    {
      control.enable_bidi = 1;
      control.orientation_reversed = data == 2;
    }
  for (i = 0; i < 3; i++)
    {
      if (i == data)
	XtSetArg (arg[0], XtNleftBitmap, CheckPixmap);
      else
	XtSetArg (arg[0], XtNleftBitmap, None);
      XtSetValues (BidiMenus[i], arg, 1);
    }

  update_cursor (cursor.from, 1);
  redraw (0, win_height, 1, 0);
}

extern int line_break (MText *mt, int pos, int from, int to, int line, int y);

void
LineBreakProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  int data = (int) client_data;
  int i;

  if (data == 0)
    control.max_line_width = 0;
  else
    {
      control.max_line_width = win_width;
      control.line_break = (data == 1 ? NULL : line_break);
    }
  for (i = 0; i < 3; i++)
    {
      if (i == data)
	XtSetArg (arg[0], XtNleftBitmap, CheckPixmap);
      else
	XtSetArg (arg[0], XtNleftBitmap, None);
      XtSetValues (LineBreakMenus[i], arg, 1);
    }

  update_cursor (cursor.from, 1);
  redraw (0, win_height, 1, 0);
}

void
CursorProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  int data = (int) client_data;
  int i, from, to;

  switch (data)
    {
    case 0:
      logical_move = 1;
      from = 0, to = 2;
      break;
    case 1:
      logical_move = 0;
      from = 0, to = 2;
      break;
    case 2:
      control.cursor_bidi = 0, control.cursor_width = -1;
      from = 2, to = 5;
      break;
    case 3:
      control.cursor_bidi = 0, control.cursor_width = 2;
      from = 2, to = 5;
      break;
    default:
      control.cursor_bidi = 1;
      from = 2, to = 5;
      break;
    }

  for (i = from; i < to; i++)
    {
      if (i == data)
	XtSetArg (arg[0], XtNleftBitmap, CheckPixmap);
      else
	XtSetArg (arg[0], XtNleftBitmap, None);
      XtSetValues (CursorMenus[i], arg, 1);
    }

  redraw (0, win_height, 1, 0);
}

static void
InputMethodProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  int idx = (int) client_data;

  if (idx == -2 ? current_input_method < 0
      : idx == -1 ? auto_input_method
      : idx == current_input_method)
    return;

  XtSetArg (arg[0], XtNleftBitmap, None);
  if (auto_input_method)
    {
      XtSetValues (InputMethodMenus[1], arg, 1);
      auto_input_method = 0;
    }
  else if (current_input_method < 0)
    XtSetValues (InputMethodMenus[0], arg, 1);
  else
    XtSetValues (InputMethodMenus[current_input_method + 2], arg, 1);

  if (idx == -1)
    {
      auto_input_method = 1;
      hide_cursor ();
    }
  else if (input_method_table[idx].available >= 0)
    {
      if (! input_method_table[idx].im)
	{
	  input_method_table[idx].im = 
	    minput_open_im (input_method_table[idx].language,
			    input_method_table[idx].name, NULL);
	  if (! input_method_table[idx].im)
	    input_method_table[idx].available = -1;
	}
      if (input_method_table[idx].im)
	select_input_method (idx);
    }
  XtSetArg (arg[0], XtNleftBitmap, CheckPixmap);
  XtSetValues (InputMethodMenus[idx + 2], arg, 1);
}

MPlist *default_face_list;

void
FaceProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  int idx = (int) client_data;
  int from, to;
  int old_y1;

  if (! SELECTEDP ())
    {
      MPlist *plist;

      if (idx >= 0)
	{
	  MFace *face = mframe_get_prop (frame, Mface);

	  for (plist = default_face_list; mplist_key (plist) != Mnil;
	       plist = mplist_next (plist)) 
	    mface_merge (face, mplist_value (plist));
	  mplist_add (plist, Mt, *face_table[idx].face);
	  mface_merge (face, *face_table[idx].face);
	}
      else if (mplist_key (mplist_next (default_face_list)) != Mnil)
	{
	  MFace *face = mframe_get_prop (frame, Mface);

	  for (plist = default_face_list;
	       mplist_key (mplist_next (plist)) != Mnil;
	       plist = mplist_next (plist)) 
	    mface_merge (face, mplist_value (plist));
	  mplist_pop (plist);
	}
      update_top (0);
      update_cursor (0, 1);
      redraw (0, win_height, 1, 1);
      show_cursor (NULL);
      return;
    }

  XtAppAddWorkProc (context, show_cursor, NULL);
  from = mtext_property_start (selection);
  to = mtext_property_end (selection);
  old_y1 = sel_end.y1;

  mtext_detach_property (selection);
  if (idx >= 0)
    {
      MTextProperty *prop = mtext_property (Mface, *face_table[idx].face,
					    MTEXTPROP_REAR_STICKY);
      mtext_push_property (mt, from, to, prop);
      m17n_object_unref (prop);
    }
  else
    mtext_pop_prop (mt, from, to, Mface);
  if (from < top.to)
    update_top (top.from);
  update_cursor (cursor.from, 1);
  select_region (from, to);
  update_region (sel_start.y0, old_y1, sel_end.y1);
  if (cur.y1 > win_height)
    {
      while (cur.y1 > win_height)
	{
	  reseat (top.to);
	  update_cursor (cursor.from, 1);
	}
    }
}

void
LangProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  MSymbol sym = (MSymbol) client_data;
  int from, to;
  int old_y1;

  if (! SELECTEDP ())
    return;

  XtAppAddWorkProc (context, show_cursor, NULL);
  from = mtext_property_start (selection);
  to = mtext_property_end (selection);
  old_y1 = sel_end.y1;

  mtext_detach_property (selection);
  if (sym != Mnil)
    mtext_put_prop (mt, from, to, Mlanguage, sym);
  else
    mtext_pop_prop (mt, from, to, Mlanguage);

  if (from < top.to)
    update_top (top.from);
  update_cursor (cursor.from, 1);
  select_region (from, to);
  update_region (sel_start.y0, old_y1, sel_end.y1);
  if (cur.y1 > win_height)
    {
      while (cur.y1 > win_height)
	{
	  reseat (top.to);
	  update_cursor (cursor.from, 1);
	}
    }
}

void
DumpImageProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  int narrowed = (int) client_data;
  FILE *mdump;
  int from, to;
  MConverter *converter;

  if (narrowed)
    {
      if (! SELECTEDP ())
	return;
      from = mtext_property_start (selection);
      to = mtext_property_end (selection);
    }
  else
    {
      from = 0;
      to = nchars;
    }

  if (! narrowed)
    mdump = popen ("mdump -q -p a4", "w");
  else
    mdump = popen ("mdump -q", "w");
  if (! mdump)
    return;
  converter = mconv_stream_converter (Mcoding_utf_8, mdump);
  mconv_encode_range (converter, mt, from, to);
  mconv_free_converter (converter);
  fclose (mdump);
}

void
input_status (MInputContext *ic, MSymbol command)
{
  XFillRectangle (display, input_status_pixmap, gc_inv,
		  0, 0, input_status_width, input_status_height);
  if (command == Minput_status_draw)
    {
      MDrawMetric rect;

      mtext_put_prop (ic->status, 0, mtext_len (ic->status),
		      Mface, face_input_status);
      if (ic->im->language != Mnil)
	mtext_put_prop (ic->status, 0, mtext_len (ic->status),
			Mlanguage, ic->im->language);
      mdraw_text_extents (frame, ic->status, 0, mtext_len (ic->status),
			  &input_status_control, NULL, NULL, &rect);
      mdraw_text_with_control (frame, (MDrawWindow) input_status_pixmap,
			       input_status_width - rect.width - 2, - rect.y,
			       ic->status, 0, mtext_len (ic->status),
			       &input_status_control);
    }
  XtSetArg (arg[0], XtNbitmap, input_status_pixmap);
  XtSetValues (CurIMStatus, arg, 1);
}

int
compare_input_method (const void *elt1, const void *elt2)
{
  const InputMethodInfo *im1 = elt1;
  const InputMethodInfo *im2 = elt2;
  MSymbol lang1, lang2;

  if (im1->language == Mnil)
    return 1;
  if (im1->language == im2->language)
    return strcmp (msymbol_name (im1->name), msymbol_name (im2->name));
  if (im1->language == Mt)
    return 1;
  if (im2->language == Mt)
    return -1;
  lang1 = msymbol_get (im1->language, Mlanguage);
  lang2 = msymbol_get (im2->language, Mlanguage);
  return strcmp (msymbol_name (lang1), msymbol_name (lang2));
}

void
setup_input_methods (int with_xim)
{
  MInputMethod *im = NULL;
  MPlist *plist = mdatabase_list (msymbol ("input-method"), Mnil, Mnil, Mnil);
  MPlist *pl;
  int i = 0;

  num_input_methods = mplist_length (plist);

  if (with_xim)
    {
      MInputXIMArgIM arg_xim;

      arg_xim.display = display;
      arg_xim.db = NULL;  
      arg_xim.res_name = arg_xim.res_class = NULL;
      arg_xim.locale = NULL;
      arg_xim.modifier_list = NULL;
      im = minput_open_im (Mnil, msymbol ("xim"), &arg_xim);
      if (im)
	num_input_methods++;
    }
  input_method_table = calloc (num_input_methods, sizeof (InputMethodInfo));
  if (im)
    {
      input_method_table[i].available = 1;
      input_method_table[i].language = Mnil;
      input_method_table[i].name = im->name;
      input_method_table[i].im = im;
      i++;
    }

  for (pl = plist; mplist_key (pl) != Mnil; pl = mplist_next (pl))
    {
      MDatabase *mdb = mplist_value (pl);
      MSymbol *tag = mdatabase_tag (mdb);

      if (tag[1] != Mnil)
	{
	  input_method_table[i].language = tag[1];
	  input_method_table[i].name = tag[2];
	  i++;
	}
    }

  m17n_object_unref (plist);
  num_input_methods = i;
  qsort (input_method_table, num_input_methods, sizeof input_method_table[0],
	 compare_input_method);
  current_input_context = NULL;

  mplist_put (minput_driver->callback_list, Minput_status_start,
	      (void *) input_status);
  mplist_put (minput_driver->callback_list, Minput_status_draw,
	      (void *) input_status);
  mplist_put (minput_driver->callback_list, Minput_status_done,
	      (void *) input_status);
}


static void
MenuHelpProc (Widget w, XEvent *event, String *str, Cardinal *num)
{
  char *msg;

  if (num && *num > 0)
    {
      int bytes = 0, i;

      for (i = 0; i < *num; i++)
	bytes += strlen (str[i]) + 1;
      msg = alloca (bytes);
      strcpy (msg, str[0]);
      for (i = 1; i < *num; i++)
	strcat (msg, " "), strcat (msg, str[i]);
    }
  else if (cursor.from < nchars)
    {
      int c = mtext_ref_char (mt, cursor.from);
      char *name = mchar_get_prop (c, Mname);

      if (! name)
	name = "";
      msg = alloca (10 + strlen (name));
      sprintf (msg, "U+%04X %s", c, name);
    }
  else
    {
      msg = "";
    }
  XtSetArg (arg[0], XtNlabel, msg);
  XtSetValues (MessageWidget, arg, 1);
}

typedef struct
{
  int type;
  char *name1, *name2;
  XtCallbackProc proc;
  XtPointer client_data;
  int status;
  Widget w;
} MenuRec;

void PopupProc (Widget w, XtPointer client_data, XtPointer call_data);

void SaveProc (Widget w, XtPointer client_data, XtPointer call_data);

MenuRec FileMenu[] =
  { { 0, "Open", NULL, PopupProc, FileMenu + 0, -1 },
    { 0, "Save", NULL, SaveProc, NULL, -1 },
    { 0, "Save as", NULL, PopupProc, FileMenu + 2, -1 },
    { 1 },
    { 0, "Serialize", NULL, SerializeProc, (void *) 1, -1 },
    { 0, "Deserialize", NULL, SerializeProc, (void *) 0, -1 },
    { 1 },
    { 0, "Dump Image Buffer", NULL, DumpImageProc, (void *) 0, -1 },
    { 0, "Dump Image Region", NULL, DumpImageProc, (void *) 1, -1 },
    { 1 },
    { 0, "Quit", NULL, QuitProc, NULL, -1 } };

void
PopupProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  MenuRec *rec = (MenuRec *) client_data;
  Position x, y;

  XtSetArg (arg[0], XtNvalue, "");
  XtSetArg (arg[1], XtNlabel, rec->name1);
  XtSetValues (FileDialogWidget, arg, 2);
  XtTranslateCoords (w, (Position) 0, (Position) 0, &x, &y);
  XtSetArg (arg[0], XtNx, x + 20);
  XtSetArg (arg[1], XtNy, y + 10);
  XtSetValues (FileShellWidget, arg, 2);
  XtPopup (FileShellWidget, XtGrabExclusive);
}

void
FileDialogProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  FILE *fp;
  char *label;

  XtPopdown (FileShellWidget);
  if ((int) client_data == 1)
    return;
  XtSetArg (arg[0], XtNlabel, &label);
  XtGetValues (FileDialogWidget, arg, 1);
  if (strcmp (label, FileMenu[0].name1) == 0)
    {
      /* Open a file */
      free (filename);
      filename = strdup ((char *) XawDialogGetValueString (FileDialogWidget));
      fp = fopen (filename, "r");
      hide_cursor ();
      m17n_object_unref (mt);
      if (fp)
	{
	  mt = mconv_decode_stream (Mcoding_utf_8, fp);
	  fclose (fp);
	  if (! mt)
	    mt = mtext ();
	}
      else
	mt = mtext ();
      serialized = 0;
      nchars = mtext_len (mt);
      update_top (0);
      update_cursor (0, 1);
      redraw (0, win_height, 1, 1);
    }
  else if (strcmp (label, FileMenu[2].name1) == 0)
    SaveProc (w, (XtPointer) XawDialogGetValueString (FileDialogWidget), NULL);
  else
    fprintf (stderr, "Invalid calling sequence: FileDialogProc\n");
}

#define SetMenu(MENU, TYPE, NAME1, NAME2, PROC, DATA, STATUS)		 \
  ((MENU).type = (TYPE), (MENU).name1 = (NAME1), (MENU).name2 = (NAME2), \
   (MENU).proc = (PROC), (MENU).client_data = (XtPointer) (DATA),	 \
   (MENU).status = (STATUS))


Widget
create_menu_button (Widget top, Widget parent, Widget left, char *button_name,
		    char *menu_name, MenuRec *menus, int num_menus, char *help)
{
  Widget button, menu;
  char *fmt = "<EnterWindow>: highlight() MenuHelp(%s)\n\
	       <LeaveWindow>: reset() MenuHelp()\n\
	       <BtnDown>: reset() PopupMenu()\n\
	       <BtnUp>: highlight()"; 
  int i;
  MenuRec *m;
  char *trans;
  int max_width = 0;

  menu = XtCreatePopupShell (menu_name, simpleMenuWidgetClass, top, NULL, 0);
  for (i = 0; i < num_menus; i++)
    {
      m = menus + i;
      if (m->type == 0)
	{
	  if (m->proc)
	    {
	      int n = 0;

	      if (m->status >= 0)
		{
		  XtSetArg (arg[n], XtNleftMargin, 20), n++;
		  if (m->status > 0)
		    XtSetArg (arg[n], XtNleftBitmap, CheckPixmap), n++;
		}
	      m->w = XtCreateManagedWidget (m->name1, smeBSBObjectClass,
					    menu, arg, n);
	      XtAddCallback (m->w, XtNcallback, m->proc, m->client_data);
	    }
	  else
	    {
	      XtSetArg (arg[0], XtNsensitive, False);
	      m->w = XtCreateManagedWidget (m->name1, smeBSBObjectClass,
					    menu, arg, 2);
	    }
	}
      else
	{
	  XtCreateManagedWidget (m->name1, smeLineObjectClass, menu, NULL, 0);
	}
      if (m->name2)
	max_width = 1;
    }
  trans = alloca (strlen (fmt) + strlen (help));
  sprintf (trans, fmt, help);
  XtSetArg (arg[0], XtNmenuName, menu_name);
  XtSetArg (arg[1], XtNtranslations, XtParseTranslationTable ((String) trans));
  XtSetArg (arg[2], XtNinternalWidth, 2);
  XtSetArg (arg[3], XtNhighlightThickness, 1);
  XtSetArg (arg[4], XtNleft, XawChainLeft);
  XtSetArg (arg[5], XtNright, XawChainLeft);
  i = 6;
  if (left)
    XtSetArg (arg[i], XtNfromHoriz, left), i++;
  button = XtCreateManagedWidget (button_name, menuButtonWidgetClass, parent,
				  arg, i);

  if (max_width)
    {
      int height, ascent, *width = alloca (sizeof (int) * num_menus);
      int *len = alloca (sizeof (int) * num_menus);

      XFontSet font_set;
      XFontSetExtents *fontset_extents;

      XtSetArg (arg[0], XtNfontSet, &font_set);
      XtGetValues (button, arg, 1);

      fontset_extents = XExtentsOfFontSet (font_set);
      height = fontset_extents->max_logical_extent.height;
      ascent = - fontset_extents->max_logical_extent.y;

      for (i = 0; i < num_menus; i++)
	if (menus[i].name2)
	  {
	    len[i] = strlen (menus[i].name2);
	    width[i] = XmbTextEscapement (font_set, menus[i].name2, len[i]);
	    if (max_width < width[i])
	      max_width = width[i];
	  }
      for (i = 0; i < num_menus; i++)
	if (menus[i].name2)
	  {
	    Pixmap pixmap = XCreatePixmap (display,
					   RootWindow (display, screen),
					   max_width, height, 1);
	    XFillRectangle (display, pixmap, mono_gc_inv,
			    0, 0, max_width, height);
	    XmbDrawString (display, pixmap, font_set, mono_gc,
			   max_width - width[i], ascent,
			   menus[i].name2, len[i]);
	    XtSetArg (arg[0], XtNrightBitmap, pixmap);
	    XtSetArg (arg[1], XtNrightMargin, max_width + 20);
	    XtSetValues (menus[i].w, arg, 2);
	  }
    }

  return button;
}


XtActionsRec actions[] = {
  {"Expose", ExposeProc},
  {"Configure", ConfigureProc},
  {"Key", KeyProc},
  {"ButtonPress", ButtonProc},
  {"ButtonRelease", ButtonReleaseProc},
  {"ButtonMotion", ButtonMoveProc},
  {"Button2Press", Button2Proc},
  {"MenuHelp", MenuHelpProc}
};


/* Print the usage of this program (the name is PROG), and exit with
   EXIT_CODE.  */

void
help_exit (char *prog, int exit_code)
{
  char *p = prog;

  while (*p)
    if (*p++ == '/')
      prog = p;

  printf ("Usage: %s [ XT-OPTION ...] [ OPTION ...] FILE\n", prog);
  printf ("Display FILE on a window and allow users to edit it.\n");
  printf ("XT-OPTIONs are standard Xt arguments (e.g. -fn, -fg).\n");
  printf ("The following OPTIONs are available.\n");
  printf ("  %-13s %s", "--version", "print version number\n");
  printf ("  %-13s %s", "-h, --help", "print this message\n");
  exit (exit_code);
}

int
main (int argc, char **argv)
{
  Widget form, BodyWidget, w;
  char *fontset_name = NULL;
  int col = 80, row = 32;
  /* Translation table for TextWidget.  */
  String trans = "<Expose>: Expose()\n\
		  <Configure>: Configure()\n\
		  <Key>: Key()\n\
		  <KeyUp>: Key()\n\
		  <Btn1Down>: ButtonPress()\n\
		  <Btn1Up>: ButtonRelease()\n\
		  <Btn1Motion>: ButtonMotion()\n\
		  <Btn2Down>: Button2Press()";
  /* Translation table for the top form widget.  */
  String trans2 = "<Key>: Key()\n\
		   <KeyUp>: Key()";
  String pop_face_trans
    = "<EnterWindow>: MenuHelp(Pop face property) highlight()\n\
       <LeaveWindow>: MenuHelp() reset()\n\
       <Btn1Down>: set()\n\
       <Btn1Up>: notify() unset()"; 
  String pop_lang_trans
    = "<EnterWindow>: MenuHelp(Pop language property) highlight()\n\
       <LeaveWindow>: MenuHelp() reset()\n\
       <Btn1Down>: set()\n\
       <Btn1Up>: notify() unset()"; 
  int font_width, font_ascent, font_descent;
  int with_xim = 0;
  int i, j;

  setlocale (LC_ALL, "");
  /* Create the top shell.  */
  XtSetLanguageProc (NULL, NULL, NULL);
  ShellWidget = XtOpenApplication (&context, "MEdit", NULL, 0, &argc, argv,
				   NULL, sessionShellWidgetClass, NULL, 0);
  display = XtDisplay (ShellWidget);
  screen = XScreenNumberOfScreen (XtScreen (ShellWidget));

  /* Parse the remaining command line arguments.  */
  for (i = 1; i < argc; i++)
    {
      if (! strcmp (argv[i], "--help")
	  || ! strcmp (argv[i], "-h"))
	help_exit (argv[0], 0);
      else if (! strcmp (argv[i], "--version"))
	{
	  printf ("medit (m17n library) %s\n", VERSION);
	  printf ("Copyright (C) 2003 AIST, JAPAN\n");
	  exit (0);
	}
      else if (! strcmp (argv[i], "--geometry"))
	{
	  i++;
	  if (sscanf (argv[i], "%dx%d", &col, &row) != 2)
	    help_exit (argv[0], 1);
	}
      else if (! strcmp (argv[i], "--fontset"))
	{
	  i++;
	  fontset_name = strdup (argv[i]);
	}
      else if (! strcmp (argv[i], "--with-xim"))
	{
	  with_xim = 1;
	}
      else if (argv[i][0] != '-')
	{
	  filename = strdup (argv[i]);
	}
      else
	{
	  fprintf (stderr, "Unknown option: %s", argv[i]);
	  help_exit (argv[0], 1);
	}
    }
  if (! filename)
    help_exit (argv[0], 1);

  mdatabase_dir = ".";
  /* Initialize the m17n library.  */
  M17N_INIT ();
  if (merror_code != MERROR_NONE)
    FATAL_ERROR ("%s\n", "Fail to initialize the m17n library!");

  mt = read_file (filename);
  serialized = 0;

  nchars = mtext_len (mt);

  {
    MFace *face = mface ();

    mface_put_prop (face, Mforeground, msymbol ("blue"));
    mface_put_prop (face, Mbackground, msymbol ("yellow"));
    mface_put_prop (face, Mvideomode, Mreverse);
    selection = mtext_property (Mface, face, MTEXTPROP_NO_MERGE);
    m17n_object_unref (face);
  }

  /* This tells ExposeProc to initialize everything.  */
  top.from = -1;
  
  XA_TEXT = XInternAtom (display, "TEXT", False);
  XA_COMPOUND_TEXT = XInternAtom (display, "COMPOUND_TEXT", False);
  XA_UTF8_STRING = XInternAtom (display, "UTF8_STRING", False);
  Mcoding_compound_text = mconv_resolve_coding (msymbol ("compound-text"));
  if (Mcoding_compound_text == Mnil)
    FATAL_ERROR ("%s\n", "Don't know about COMPOUND-TEXT encoding!");

  {
    MPlist *plist = mplist ();
    MFace *face;
    MFont *font;

    mplist_put (plist, msymbol ("widget"), ShellWidget);
    if (fontset_name)
      {
	MFontset *fontset = mfontset (fontset_name);
	
	face = mface ();
	mface_put_prop (face, Mfontset, fontset);
	m17n_object_unref (fontset);
	mplist_add (plist, Mface, face);
	m17n_object_unref (face);
      }
    frame = mframe (plist);
    if (! frame)
      FATAL_ERROR ("%s\n", "Fail to create a frame!");
    m17n_object_unref (plist);
    face_default = mface_copy ((MFace *) mframe_get_prop (frame, Mface));
    default_face_list = mplist ();
    mplist_add (default_face_list, Mt, face_default);
    face_default_fontset = mface ();
    mface_put_prop (face_default_fontset, Mfontset,
		    mface_get_prop (face_default, Mfontset));

    font = (MFont *) mframe_get_prop (frame, Mfont);
    default_font_size = (int) mfont_get_prop (font, Msize);
  }

  font_width = (int) mframe_get_prop (frame, Mfont_width);
  font_ascent = (int) mframe_get_prop (frame, Mfont_ascent);
  font_descent = (int) mframe_get_prop (frame, Mfont_descent);
  win_width = font_width * col;
  win_height = (font_ascent + font_descent) * row;

  {
    MFaceBoxProp prop;

    prop.width = 4;
    prop.color_top = prop.color_left = msymbol ("magenta");
    prop.color_bottom = prop.color_right = msymbol ("red");
    prop.inner_hmargin = prop.inner_vmargin = 1;
    prop.outer_hmargin = prop.outer_vmargin = 2;

    face_box = mface ();
    mface_put_prop (face_box, Mbox, &prop);
  }

  face_courier = mface ();
  mface_put_prop (face_courier, Mfamily, msymbol ("courier"));
  face_helvetica = mface ();
  mface_put_prop (face_helvetica, Mfamily, msymbol ("helvetica"));
  face_times = mface ();
  mface_put_prop (face_times, Mfamily, msymbol ("times"));
  face_dv_ttyogesh = mface ();
  mface_put_prop (face_dv_ttyogesh, Mfamily, msymbol ("dv-ttyogesh"));
  face_freesans = mface ();
  mface_put_prop (face_freesans, Mfamily, msymbol ("freesans"));
  face_freeserif = mface ();
  mface_put_prop (face_freeserif, Mfamily, msymbol ("freeserif"));
  face_freemono = mface ();
  mface_put_prop (face_freemono, Mfamily, msymbol ("freemono"));

  face_xxx_large = mface ();
  mface_put_prop (face_xxx_large, Mratio, (void *) 300);
  {
    MFont *latin_font = mframe_get_prop (frame, Mfont);
    MFont *dev_font = mfont ();
    MFont *thai_font = mfont ();
    MFont *tib_font = mfont ();
    MFontset *fontset;
    MSymbol unicode_bmp = msymbol ("unicode-bmp");
    MSymbol no_ctl = msymbol ("no-ctl");

    mfont_put_prop (dev_font, Mfamily, msymbol ("raghindi"));
    mfont_put_prop (dev_font, Mregistry, unicode_bmp);
    mfont_put_prop (thai_font, Mfamily, msymbol ("norasi"));
    mfont_put_prop (thai_font, Mregistry, unicode_bmp);
    mfont_put_prop (tib_font, Mfamily, msymbol ("mtib"));
    mfont_put_prop (tib_font, Mregistry, unicode_bmp);

    fontset = mfontset_copy (mfontset (fontset_name), "no-ctl");
    mfontset_modify_entry (fontset, msymbol ("latin"), Mnil, Mnil,
			   latin_font, Mnil, 0);
    mfontset_modify_entry (fontset, msymbol ("devanagari"), Mnil, Mnil,
			   dev_font, no_ctl, 0);
    mfontset_modify_entry (fontset, msymbol ("thai"), Mnil, Mnil,
			   thai_font, no_ctl, 0);
    mfontset_modify_entry (fontset, msymbol ("tibetan"), Mnil, Mnil,
			   tib_font, no_ctl, 0);
    face_no_ctl_fontset = mface ();
    mface_put_prop (face_no_ctl_fontset, Mfontset, fontset);
    m17n_object_unref (fontset);

    free (dev_font);
    free (thai_font);
    free (tib_font);
  }

  setup_input_methods (with_xim);

  gc = DefaultGC (display, screen);

  XtSetArg (arg[0], XtNtranslations, XtParseTranslationTable (trans2));
  XtSetArg (arg[1], XtNdefaultDistance, 2);
  form = XtCreateManagedWidget ("form", formWidgetClass, ShellWidget, arg, 2);

  XtSetArg (arg[0], XtNborderWidth, 0);
  XtSetArg (arg[1], XtNdefaultDistance, 2);
  XtSetArg (arg[2], XtNtop, XawChainTop);
  XtSetArg (arg[3], XtNbottom, XawChainTop);
  XtSetArg (arg[4], XtNleft, XawChainLeft);
  XtSetArg (arg[5], XtNright, XawChainRight);
  XtSetArg (arg[6], XtNresizable, True);
  HeadWidget = XtCreateManagedWidget ("head", formWidgetClass, form, arg, 7);
  XtSetArg (arg[7], XtNfromVert, HeadWidget);
  FaceWidget = XtCreateManagedWidget ("face", formWidgetClass, form, arg, 8);
  XtSetArg (arg[7], XtNfromVert, FaceWidget);
  LangWidget = XtCreateManagedWidget ("lang", formWidgetClass, form, arg, 8);
  XtSetArg (arg[3], XtNbottom, XawChainBottom);
  XtSetArg (arg[7], XtNfromVert, LangWidget);
  BodyWidget = XtCreateManagedWidget ("body", formWidgetClass, form, arg, 8);
  XtSetArg (arg[2], XtNtop, XawChainBottom);
  XtSetArg (arg[7], XtNfromVert, BodyWidget);
  TailWidget = XtCreateManagedWidget ("tail", formWidgetClass, form, arg, 8);

  FileShellWidget = XtCreatePopupShell ("FileShell", transientShellWidgetClass,
					HeadWidget, NULL, 0);
  XtSetArg (arg[0], XtNvalue, "");
  FileDialogWidget = XtCreateManagedWidget ("File", dialogWidgetClass,
					    FileShellWidget, arg, 1);
  XawDialogAddButton (FileDialogWidget, "OK",
		      FileDialogProc, (XtPointer) 0);
  XawDialogAddButton (FileDialogWidget, "CANCEL",
		      FileDialogProc, (XtPointer) 1);

  CheckPixmap = XCreateBitmapFromData (display, RootWindow (display, screen),
				       (char *) check_bits,
				       check_width, check_height);
  {
    unsigned long valuemask = GCForeground;
    XGCValues values;

    values.foreground = 1;
    mono_gc = XCreateGC (display, CheckPixmap, valuemask, &values);
    values.foreground = 0;
    mono_gc_inv = XCreateGC (display, CheckPixmap, valuemask, &values);
  }

  {
    MenuRec *menus;
    int num_menus = 10;

    if (num_menus < num_input_methods + 2)
      num_menus = num_input_methods + 2;
    if (num_menus < num_faces + 1)
      num_menus = num_faces + 1;
    menus = alloca (sizeof (MenuRec) * num_menus);

    w = create_menu_button (ShellWidget, HeadWidget, NULL, "File", "File Menu",
			    FileMenu, sizeof FileMenu / sizeof (MenuRec),
			    "File I/O, Serialization, Image, Quit");

    SetMenu (menus[0], 0, "Logical Move", NULL, CursorProc, 0, 1);
    SetMenu (menus[1], 0, "Visual Move", NULL, CursorProc, 1, 0);
    SetMenu (menus[2], 1, "", NULL, NULL, NULL, 0);
    SetMenu (menus[3], 0, "Box type", NULL, CursorProc, 2, 0);
    SetMenu (menus[4], 0, "Bar type", NULL, CursorProc, 3, 1);
    SetMenu (menus[5], 0, "Bidi type", NULL, CursorProc, 4, 0);
    w = create_menu_button (ShellWidget, HeadWidget, w,
			    "Cursor", "Cursor Menu",
			    menus, 6, "Cursor Movement Mode, Cursor Shape");
    CursorMenus[0] = menus[0].w;
    CursorMenus[1] = menus[1].w;
    CursorMenus[2] = menus[3].w;
    CursorMenus[3] = menus[4].w;
    CursorMenus[4] = menus[5].w;

    SetMenu (menus[0], 0, "disable", NULL, BidiProc, 0, 0);
    SetMenu (menus[1], 0, "Left  (|--> |)", NULL, BidiProc, 1, 1);
    SetMenu (menus[2], 0, "Right (| <--|)", NULL, BidiProc, 2, 0);
    w = create_menu_button (ShellWidget, HeadWidget, w, "Bidi", "Bidi Menu",
			    menus, 3, "BIDI Processing Mode");
    for (i = 0; i < 3; i++)
      BidiMenus[i] = menus[i].w;

    SetMenu (menus[0], 0, "truncate", NULL, LineBreakProc, 0, 0);
    SetMenu (menus[1], 0, "break at edge", NULL, LineBreakProc, 1, 1);
    SetMenu (menus[2], 0, "break at word boundary", NULL, LineBreakProc, 2, 0);
    w = create_menu_button (ShellWidget, HeadWidget, w, "LineBreak",
			    "LineBreak Menu",
			    menus, 3, "How to break lines");
    for (i = 0; i < 3; i++)
      LineBreakMenus[i] = menus[i].w;

    SetMenu (menus[0], 0, "none", NULL, InputMethodProc, -2, 1);
    SetMenu (menus[1], 0, "auto", NULL, InputMethodProc, -1, 0);
    for (i = 0; i < num_input_methods; i++)
      {
	InputMethodInfo *im = input_method_table + i;
	char *name1, *name2;

	if (im->language != Mnil && im->language != Mt)
	  {
	    MSymbol sym = msymbol_get (im->language, Mlanguage);
	    if (sym == Mnil)
	      name1 = msymbol_name (im->language);
	    else
	      name1 = msymbol_name (sym);
	    name2 = msymbol_name (im->name);
	  }
	else
	  name1 = msymbol_name (im->name), name2 = NULL;

	SetMenu (menus[i + 2], 0, name1, name2, InputMethodProc, i, 0);
      }
    w = create_menu_button (ShellWidget, HeadWidget, w, "InputMethod",
			    "Input Method Menu", menus, i + 2,
			    "Select input method");

    {
      unsigned long valuemask = GCForeground;
      XGCValues values;

      XtSetArg (arg[0], XtNbackground, &values.foreground);
      XtGetValues (w, arg, 1);
      gc_inv = XCreateGC (display, RootWindow (display, screen),
			  valuemask, &values);
    }

    InputMethodMenus = malloc (sizeof (Widget) * (num_input_methods + 2));
    for (i = 0; i < num_input_methods + 2; i++)
      InputMethodMenus[i] = menus[i].w;

    input_status_width = font_width * 8;
    input_status_height = (font_ascent + font_descent) * 2.4;
    input_status_pixmap = XCreatePixmap (display, RootWindow (display, screen),
					 input_status_width,
					 input_status_height,
					 DefaultDepth (display, screen));
    {
      MFaceBoxProp prop;

      prop.width = 1;
      prop.color_top = prop.color_bottom
	= prop.color_left = prop.color_right = Mnil;
      prop.inner_hmargin = prop.inner_vmargin = 1;
      prop.outer_hmargin = prop.outer_vmargin = 0;
      face_input_status = mface_copy (face_default);
      mface_put_prop (face_input_status, Mbox, &prop);
    }

    XFillRectangle (display, input_status_pixmap, gc_inv,
		    0, 0, input_status_width, input_status_height);
    XtSetArg (arg[0], XtNfromHoriz, w);
    XtSetArg (arg[1], XtNleft, XawRubber);
    XtSetArg (arg[2], XtNright, XawChainRight);
    XtSetArg (arg[3], XtNborderWidth, 0);
    XtSetArg (arg[4], XtNlabel, "          ");
    XtSetArg (arg[5], XtNjustify, XtJustifyRight);
    CurIMLang = XtCreateManagedWidget ("CurIMLang", labelWidgetClass,
				       HeadWidget, arg, 6);
    XtSetArg (arg[0], XtNfromHoriz, CurIMLang);
    XtSetArg (arg[1], XtNleft, XawChainRight);
    XtSetArg (arg[4], XtNbitmap, input_status_pixmap);
    CurIMStatus = XtCreateManagedWidget ("CurIMStatus", labelWidgetClass,
					 HeadWidget, arg, 5);

    XtSetArg (arg[0], XtNborderWidth, 0);
    XtSetArg (arg[1], XtNleft, XawChainLeft);
    XtSetArg (arg[2], XtNright, XawChainLeft);
    w = XtCreateManagedWidget ("Face", labelWidgetClass, FaceWidget, arg, 3);
    for (i = 0; i < num_faces;)
      {
	char *label_menu = face_table[i++].name; /* "Menu Xxxx" */
	char *label = label_menu + 5;		 /* "Xxxx" */

	for (j = i; j < num_faces && face_table[j].face; j++)
	  SetMenu (menus[j - i], 0, face_table[j].name, NULL,
		   FaceProc, j, -1);
	w = create_menu_button (ShellWidget, FaceWidget, w,
				label, label_menu,
				menus, j - i, "Push face property");
	i = j;
      }

    XtSetArg (arg[0], XtNfromHoriz, w);
    XtSetArg (arg[1], XtNleft, XawChainLeft);
    XtSetArg (arg[2], XtNright, XawChainLeft);
    XtSetArg (arg[3], XtNhorizDistance, 10);
    XtSetArg (arg[4], XtNlabel, "Pop");
    XtSetArg (arg[5], XtNtranslations,
	      XtParseTranslationTable (pop_face_trans));
    w = XtCreateManagedWidget ("Pop Face", commandWidgetClass,
			       FaceWidget, arg, 6);
    XtAddCallback (w, XtNcallback, FaceProc, (void *) -1);

    XtSetArg (arg[0], XtNfromHoriz, w);
    XtSetArg (arg[1], XtNleft, XawChainLeft);
    XtSetArg (arg[2], XtNright, XawChainRight);
    XtSetArg (arg[3], XtNlabel, "");
    XtSetArg (arg[4], XtNborderWidth, 0);
    XtSetArg (arg[5], XtNjustify, XtJustifyRight);
    CurFaceWidget = XtCreateManagedWidget ("Current Face", labelWidgetClass,
					   FaceWidget, arg, 6);

    XtSetArg (arg[0], XtNborderWidth, 0);
    XtSetArg (arg[1], XtNleft, XawChainLeft);
    XtSetArg (arg[2], XtNright, XawChainLeft);
    w = XtCreateManagedWidget ("Lang", labelWidgetClass, LangWidget, arg, 3);
    {
      MPlist *plist[11], *pl;
      char langname[3];

      for (i = 0; i < 11; i++) plist[i] = NULL;
      langname[2] = '\0';
      for (langname[0] = 'a'; langname[0] <= 'z'; langname[0]++)
	for (langname[1] = 'a'; langname[1] <= 'z'; langname[1]++)
	  {
	    MSymbol sym = msymbol_exist (langname);
	    MSymbol fullname;

	    if (sym != Mnil
		&& ((fullname = msymbol_get (sym, Mlanguage)) != Mnil))
	      {
		char *name = msymbol_name (fullname);
		char c = name[0];

		if (c >= 'A' && c <= 'Z')
		  {
		    int idx = (c < 'U') ? (c - 'A') / 2 : 10;

		    pl = plist[idx];
		    if (! pl)
		      pl = plist[idx] = mplist ();
		    for (; mplist_next (pl); pl = mplist_next (pl))
		      if (strcmp (name, (char *) mplist_value (pl)) < 0)
			break;
		    mplist_push (pl, sym, fullname);
		  }
	      }
	  }

      for (i = 0; i < 11; i++)
	if (plist[i])
	  {
	    char *name = alloca (9);

	    sprintf (name, "Menu %c-%c", 'A' + i * 2, 'A' + i * 2 + 1);
	    if (i == 10)
	      name[7] = 'Z';
	    for (j = 0, pl = plist[i]; mplist_next (pl);
		 j++, pl = mplist_next (pl))
	      SetMenu (menus[j], 0, msymbol_name ((MSymbol) mplist_value (pl)),
		       msymbol_name (mplist_key (pl)),
		       LangProc, mplist_key (pl), -1);
	    w = create_menu_button (ShellWidget, LangWidget, w, name + 5, name,
				    menus, j, "Push language property");
	  }
      for (i = 0; i < 11; i++)
	if (plist[i])
	  m17n_object_unref (plist[i]);
    }
    XtSetArg (arg[0], XtNfromHoriz, w);
    XtSetArg (arg[1], XtNleft, XawChainLeft);
    XtSetArg (arg[2], XtNright, XawChainLeft);
    XtSetArg (arg[3], XtNhorizDistance, 10);
    XtSetArg (arg[4], XtNlabel, "Pop");
    XtSetArg (arg[5], XtNtranslations,
	      XtParseTranslationTable (pop_lang_trans));
    w = XtCreateManagedWidget ("Pop Lang", commandWidgetClass,
			       LangWidget, arg, 6);
    XtAddCallback (w, XtNcallback, LangProc, Mnil);

    XtSetArg (arg[0], XtNfromHoriz, w);
    XtSetArg (arg[1], XtNleft, XawChainLeft);
    XtSetArg (arg[2], XtNright, XawChainRight);
    XtSetArg (arg[3], XtNlabel, "");
    XtSetArg (arg[4], XtNborderWidth, 0);
    XtSetArg (arg[5], XtNjustify, XtJustifyRight);
    CurLangWidget = XtCreateManagedWidget ("Current Lang", labelWidgetClass,
					   LangWidget, arg, 6);
  }

  XtSetArg (arg[0], XtNheight, win_height);
  XtSetArg (arg[1], XtNwidth, 10);
  XtSetArg (arg[2], XtNleft, XawChainLeft);
  XtSetArg (arg[3], XtNright, XawChainLeft);
  SbarWidget = XtCreateManagedWidget ("sbar", scrollbarWidgetClass, BodyWidget,
				      arg, 4);
  XtAddCallback (SbarWidget, XtNscrollProc, ScrollProc, NULL);
  XtAddCallback (SbarWidget, XtNjumpProc, JumpProc, NULL);

  XtSetArg (arg[0], XtNheight, win_height);
  XtSetArg (arg[1], XtNwidth, win_width);
  XtSetArg (arg[2], XtNtranslations, XtParseTranslationTable (trans));
  XtSetArg (arg[3], XtNfromHoriz, SbarWidget);
  XtSetArg (arg[4], XtNleft, XawChainLeft);
  XtSetArg (arg[5], XtNright, XawChainRight);
  TextWidget = XtCreateManagedWidget ("text", simpleWidgetClass, BodyWidget,
				      arg, 5);

  XtSetArg (arg[0], XtNborderWidth, 0);
  XtSetArg (arg[1], XtNleft, XawChainLeft);
  XtSetArg (arg[2], XtNright, XawChainRight);
  XtSetArg (arg[3], XtNresizable, True);
  XtSetArg (arg[4], XtNjustify, XtJustifyLeft);
  MessageWidget = XtCreateManagedWidget ("message", labelWidgetClass,
					 TailWidget, arg, 5);

  memset (&control, 0, sizeof control);
  control.two_dimensional = 1;
  control.enable_bidi = 1;
  control.anti_alias = 1;
  control.min_line_ascent = font_ascent;
  control.min_line_descent = font_descent;
  control.max_line_width = win_width;
  control.with_cursor = 1;
  control.cursor_width = 2;
  control.partial_update = 1;
  control.ignore_formatting_char = 1;

  memset (&input_status_control, 0, sizeof input_status_control);
  input_status_control.enable_bidi = 1;

  XtAppAddActions (context, actions, XtNumber (actions));
  XtRealizeWidget (ShellWidget);

  win = XtWindow (TextWidget);

  XtAppMainLoop (context);

  if (current_input_context)
    minput_destroy_ic (current_input_context);
  for (i = 0; i < num_input_methods; i++)
    if (input_method_table[i].im)
      minput_close_im (input_method_table[i].im);
  m17n_object_unref (frame);
  m17n_object_unref (mt);
  m17n_object_unref (face_xxx_large);
  m17n_object_unref (face_box);
  m17n_object_unref (face_courier);
  m17n_object_unref (face_helvetica);
  m17n_object_unref (face_times);
  m17n_object_unref (face_dv_ttyogesh);
  m17n_object_unref (face_freesans);
  m17n_object_unref (face_freeserif);
  m17n_object_unref (face_freemono);
  m17n_object_unref (face_default_fontset);
  m17n_object_unref (face_no_ctl_fontset);
  m17n_object_unref (face_input_status);
  m17n_object_unref (face_default);
  m17n_object_unref (default_face_list);
  m17n_object_unref (selection);

  XFreeGC (display, mono_gc);
  XFreeGC (display, mono_gc_inv);
  XFreeGC (display, gc_inv);
  XtUninstallTranslations (form);
  XtUninstallTranslations (TextWidget);
  XtDestroyWidget (ShellWidget);
  XtDestroyApplicationContext (context);

  M17N_FINI ();

  free (fontset_name);
  free (filename);
  free (input_method_table);
  free (InputMethodMenus);

  exit (0);
}
#endif /* not FOR_DOXYGEN */
