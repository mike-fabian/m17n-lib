/* mview.c -- File viewer				-*- coding: euc-jp; -*-
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
    @enpage m17n-view view file

    @section m17n-view-synopsis SYNOPSIS

    m17n-view [ XT-OPTION ...] [ OPTION ... ] [ FILE ]

    @section m17n-view-description DESCRIPTION

    Display FILE on a window.

    If FILE is omitted, the input is taken from standard input.

    XT-OPTIONs are standard Xt arguments (e.g. -fn, -fg).

    The following OPTIONs are available.

    <ul>

    <li> -e ENCODING

    ENCODING is the encoding of FILE (defaults to UTF-8).

    <li> -s FONTSIZE

    FONTSIZE is the fontsize in point.  If ommited, it defaults to the
    size of the default font defined in X resource.

    <li> --version

    Print version number.

    <li> -h, --help

    Print this message.

    </ul>
*/
/***ja
    @japage m17n-view ファイルを見る

    @section m17n-view-synopsis SYNOPSIS

    m17n-view [ XT-OPTION ...] [ OPTION ... ] [ FILE ]

    @section m17n-view-description DESCRIPTION

    FILE をウィンドウに表示する。 

    FILE が省略された場合は、標準入力からとる。 

    XT-OPTIONs は Xt の標準の引数である。 (e.g. -fn, -fg). 

    以下のオプションが利用できる。 

    <ul>

    <li> -e ENCODING

    ENCODING は FILE のコード系である。(デフォルトは UTF-8) 

    <li> -s FONTSIZE

    FONTSIZE はフォントの大きさをポイント単位で示したものである。省略 
    された場合は、X のリソースで定義されたデフォルトフォントの大きさと 
    なる。

    <li> --version

    バージョン番号を表示する。

    <li> -h, --help

   このメッセージを表示する。 

    </ul>
*/
#ifndef FOR_DOXYGEN

#include <stdio.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <m17n-gui.h>
#include <m17n-misc.h>
#include <m17n-X.h>

#include <config.h>

#ifdef HAVE_X11_XAW_COMMAND_H

#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Viewport.h>

#define VERSION "1.2.0"

/* Global m17n variables.  */
MFrame *frame;
MText *mt;
MDrawMetric metric;
MDrawControl control;


/* Callback procedure for "quit".  */

void
QuitProc (Widget w, XtPointer client_data, XtPointer call_data)
{
  XtAppSetExitFlag (XtWidgetToApplicationContext (w));
}


/* Move POS to the next line head in M-text MT whose length is LEN.
   If POS is already on the last line, set POS to LEN.  */

#define NEXTLINE(pos, len)			\
  do {						\
    pos = mtext_character (mt, pos, len, '\n');	\
    if (pos < 0)				\
      pos = len;				\
    else					\
      pos++;					\
  } while (0)


/* Action procedure for expose event.  Redraw partial area of the
   widget W.  The area is specified in EVENT.  */

static void
ExposeProc (Widget w, XEvent *event, String *str, Cardinal *num)
{
  XExposeEvent *expose = (XExposeEvent *) event;
  int len = mtext_len (mt);
  int pos, from, to;
  int y = 0, yoff = 0;
  MDrawMetric rect;

  /* We must update the area between the Y-positions expose->y and
     (expose->y + expose->height).  We ignore X-positions.  */

  /* At first, find the line that occupies the Y-position expose->y.
     That is the first line to draw.  */
  to = 0;
  while (1)
    {
      from = to;
      NEXTLINE (to, len);
      mdraw_text_extents (frame, mt, from, to, &control, NULL, NULL, &rect);
      if (to == len || y + rect.height > expose->y)
	break;
      y += rect.height;
    }
  /* The first character to draw is at position FROM.  Remeber the
     Y-position to start drawing.  */
  yoff = y - rect.y;

  /* Next, find the line that occupies the Y-position (expose->y +
     expose->height).  That is the last line to draw.  This time, we
     enable caching to utilize it in the later drawing.  */
  y += rect.height;
  control.disable_caching = 0;
  while (to < len && y < expose->y + expose->height)
    {
      pos = to;
      NEXTLINE (to, len);
      mdraw_text_extents (frame, mt, pos, to, &control, NULL, NULL, &rect);
      y += rect.height;
    }

  /* It is decided that we must draw from FROM to the previous
     character of TO.  */
  mdraw_text_with_control (frame, (MDrawWindow) XtWindow (w),
			   0, yoff, mt, from, to, &control);

  /* Disable caching again.  */
  control.disable_caching = 1;

  /* If the widget was vertically enlarged too much, shrink it.  */
  if (metric.height < expose->y + expose->height)
    {
      Arg arg;

      XtSetArg (arg, XtNheight, metric.height);
      XtSetValues (w, &arg, 0);
    }
}


/* Print the usage of this program (the name is PROG), and exit with
   EXIT_CODE.  */

void
help_exit (char *prog, int exit_code)
{
  char *p = prog;

  while (*p)
    if (*p++ == '/')
      prog = p;

  printf ("Usage: %s [ XT-OPTION ...] [ OPTION ...] [ FILE ]\n", prog);
  printf ("Display FILE on a window.\n");
  printf ("  If FILE is omitted, the input is taken from standard input.\n");
  printf ("  XT-OPTIONs are standard Xt arguments (e.g. -fn, -fg).\n");
  printf ("The following OPTIONs are available.\n");
  printf ("  %-13s %s", "-e ENCODING",
	  "ENCODING is the encoding of FILE (defaults to UTF-8).\n");
  printf ("  %-13s %s", "-s FONTSIZE",
	  "FONTSIZE is the fontsize in point.\n");
  printf ("\t\tIf ommited, it defaults to the size\n");
  printf ("\t\tof the default font defined in X resource.\n");
  printf ("  %-13s %s", "--version", "print version number\n");
  printf ("  %-13s %s", "-h, --help", "print this message\n");
  exit (exit_code);
}


/* Format MSG by FMT and print the result to the stderr, and exit.  */

#define FATAL_ERROR(fmt, arg)	\
  do {				\
    fprintf (stderr, fmt, arg);	\
    exit (1);			\
  } while (0)


/* Adjust FONTSIZE for the resolution of the screen of widget W.  */

int
adjust_fontsize (Widget w, int fontsize)
{
  Display *display = XtDisplay (w);
  int screen_number = XScreenNumberOfScreen (XtScreen (w));
  double pixels = DisplayHeight (display, screen_number);
  double mm = DisplayHeightMM (display, screen_number);

  return (fontsize * pixels * 25.4 / mm / 100);
}


int
main (int argc, char **argv)
{
  XtAppContext context;
  Widget shell, form, quit, viewport, text;
  String quit_action = "<KeyPress>q: set() notify() unset()";
  XtActionsRec actions[] = { {"Expose", ExposeProc} };
  Arg arg[10];
  int i;
  int viewport_width, viewport_height;
  char *coding_name = NULL;
  FILE *fp = stdin;
  MSymbol coding;
  int fontsize = 0;

  /* Open an application context.  */
  XtSetLanguageProc (NULL, NULL, NULL);
  shell = XtOpenApplication (&context, "M17NView", NULL, 0, &argc, argv, NULL,
			     sessionShellWidgetClass, NULL, 0);
  XtAppAddActions (context, actions, XtNumber (actions));

  /* Parse the remaining command line arguments.  */
  for (i = 1; i < argc; i++)
    {
      if (! strcmp (argv[i], "--help")
	  || ! strcmp (argv[i], "-h"))
	help_exit (argv[0], 0);
      else if (! strcmp (argv[i], "--version"))
	{
	  printf ("m17n-view (m17n library) %s\n", VERSION);
	  printf ("Copyright (C) 2003 AIST, JAPAN\n");
	  exit (0);
	}
      else if (! strcmp (argv[i], "-e"))
	{
	  i++;
	  coding_name = argv[i];
	}
      else if (! strcmp (argv[i], "-s"))
	{
	  double n;
	  i++;
	  n = atof (argv[i]);
	  if (n <= 0)
	    FATAL_ERROR ("Invalid fontsize %s!\n", argv[i]);
	  fontsize = adjust_fontsize (shell, (int) (n * 10));
	}
      else if (argv[i][0] != '-')
	{
	  fp = fopen (argv[i], "r");
	  if (! fp)
	    FATAL_ERROR ("Fail to open the file %s!\n", argv[i]);
	}
      else
	{
	  printf ("Unknown option: %s", argv[i]);
	  help_exit (argv[0], 1);
	}
    }

  /* Initialize the m17n library.  */
  M17N_INIT ();
  if (merror_code != MERROR_NONE)
    FATAL_ERROR ("%s\n", "Fail to initialize the m17n library.");

  /* Decide how to decode the input stream.  */
  if (coding_name)
    {
      coding = mconv_resolve_coding (msymbol (coding_name));
      if (coding == Mnil)
	FATAL_ERROR ("Invalid coding: %s\n", coding_name);
    }
  else
    coding = Mcoding_utf_8;

  mt = mconv_decode_stream (coding, fp);
  fclose (fp);
  if (! mt)
    FATAL_ERROR ("%s\n", "Fail to decode the input file or stream!");

  {
    MPlist *param = mplist ();
    MFace *face = mface ();

    if (fontsize)
      mface_put_prop (face, Msize, (void *) fontsize);
    mplist_put (param, Mwidget, shell);
    mplist_put (param, Mface, face);
    frame = mframe (param);
    m17n_object_unref (param);
    m17n_object_unref (face);
  }

  /* Create this widget hierarchy.
     Shell - form -+- quit
		   |
		   +- viewport - text  */

  form = XtCreateManagedWidget ("form", formWidgetClass, shell, NULL, 0);
  XtSetArg (arg[0], XtNleft, XawChainLeft);
  XtSetArg (arg[1], XtNright, XawChainLeft);
  XtSetArg (arg[2], XtNtop, XawChainTop);
  XtSetArg (arg[3], XtNbottom, XawChainTop);
  XtSetArg (arg[4], XtNaccelerators, XtParseAcceleratorTable (quit_action));
  quit = XtCreateManagedWidget ("quit", commandWidgetClass, form, arg, 5);
  XtAddCallback (quit, XtNcallback, QuitProc, NULL);

  viewport_width = (int) mframe_get_prop (frame, Mfont_width) * 80;
  viewport_height
    = ((int) mframe_get_prop (frame, Mfont_ascent)
       + (int) mframe_get_prop (frame, Mfont_descent)) * 24;
  XtSetArg (arg[0], XtNallowVert, True);
  XtSetArg (arg[1], XtNforceBars, False);
  XtSetArg (arg[2], XtNfromVert, quit);
  XtSetArg (arg[3], XtNtop, XawChainTop);
  XtSetArg (arg[4], XtNbottom, XawChainBottom);
  XtSetArg (arg[5], XtNright, XawChainRight);
  XtSetArg (arg[6], XtNwidth, viewport_width);
  XtSetArg (arg[7], XtNheight, viewport_height);
  viewport = XtCreateManagedWidget ("viewport", viewportWidgetClass, form,
				    arg, 8);

  /* Before creating the text widget, we must calculate the height of
     the M-text to draw.  */
  control.two_dimensional = 1;
  control.enable_bidi = 1;
  control.disable_caching = 1;
  control.max_line_width = viewport_width;
  mdraw_text_extents (frame, mt, 0, mtext_len (mt), &control,
		      NULL, NULL, &metric);

  {
    /* Decide the size of the text widget.  */
    XtSetArg (arg[0], XtNwidth, viewport_width);
    if (viewport_height > metric.height)
      /* The outer viewport is tall enough.  */
      XtSetArg (arg[1], XtNheight, viewport_height);
    else if (metric.height < 0x8000)
      /* The M-text height is within the limit of X.  */
      XtSetArg (arg[1], XtNheight, metric.height);
    else
      /* We can't make such a tall widget.  Truncate it.  */
      XtSetArg (arg[1], XtNheight, 0x7FFF);

    /* We must provide our own expose event handler.  */
    XtSetArg (arg[2], XtNtranslations,
	      XtParseTranslationTable ((String) "<Expose>: Expose()"));
    text = XtCreateManagedWidget ("text", simpleWidgetClass, viewport, arg, 3);
  }

  /* Realize the top widget, and dive into an even loop.  */
  XtInstallAllAccelerators (form, form);
  XtRealizeWidget (shell);
  XtAppMainLoop (context);

  /* Clear away.  */
  m17n_object_unref (mt);
  m17n_object_unref (frame);
  M17N_FINI ();

  exit (0);
}

#else  /* not HAVE_X11_XAW_COMMAND_H */

int
main (int argc, char **argv)
{
  fprintf (stderr,
	   "Building of this program failed (lack of some header files)\n");
  exit (1);
}

#endif /* not HAVE_X11_XAW_COMMAND_H */

#endif /* not FOR_DOXYGEN */
