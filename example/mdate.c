/* mdate.c -- Show system date/time in all locales.	-*- coding: euc-jp; -*-
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
    @enpage mdate display date and time

    @section mdate-synopsis SYNOPSIS

    mdate [ OPTION ... ]

    @section mdate-description DESCRIPTION

    Display the system date and time in many locales on a window.

    The following OPTIONs are available.

    <ul>

    <li> --version

    Print version number.

    <li> -h, --help

    Print this message.
    </ul>
*/
/***ja
    @japage mdate 日時を表示する

    @section mdate-synopsis SYNOPSIS

    mdate [ OPTION ... ]

    @section mdate-description DESCRIPTION

    システムの日時をさまざまなロケールでウィンドウに表示する。 

    以下のオプションが利用できる。 

    <ul>

    <li> --version

    バージョン番号を表示する。 

    <li> -h, --help

    このメッセージを表示する。

    </ul>
*/

#ifndef FOR_DOXYGEN

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <locale.h>

#include <m17n.h>
#include <m17n-misc.h>

#define VERSION "1.0"

/* Return a plist of all locales currently avairable on the system.
   The keys and values of the plist are locale name symbols and
   pointers to MLocale respectively.  */

MPlist *
list_system_locales ()
{
  FILE *p;
  char buf[1024];
  MPlist *plist;

  /* Run the command "locale -a" to get a list of locales.  */
  if (! (p = popen ("locale -a", "r")))
    {
      fprintf (stderr, "Can't run `locale -a'.\n");
      exit (1);
    }

  plist = mplist ();
  /* Read from the pipe one line by one.  */
  while (fgets (buf, 1024, p))
    {
      MLocale *locale;
      int len = strlen (buf);

      if (buf[len - 1] == '\n')
	buf[len - 1] = '\0';
      locale = mlocale_set (LC_TIME, buf);

      /* If the locale is surely usable, it is not duplicated, and we
	 know the corresponding coding system, add it to the list.  */
      if (locale
	  && ! mplist_get (plist, mlocale_get_prop (locale, Mname))
	  && mlocale_get_prop (locale, Mcoding) != Mnil)
	mplist_add (plist, mlocale_get_prop (locale, Mname), locale);
    }
  pclose (p);
  return plist;
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

  printf ("Usage: %s [ OPTION ...]\n", prog);
  printf ("Display the system date and time in many locales on a window.\n");
  printf ("The following OPTIONs are available.\n");
  printf ("  %-13s %s", "--version", "Print version number.\n");
  printf ("  %-13s %s", "-h, --help", "Print this message.\n");
  exit (exit_code);
}


/* Format MSG by FMT and print the result to the stderr, and exit.  */

#define FATAL_ERROR(fmt, arg)	\
  do {				\
    fprintf (stderr, fmt, arg);	\
    exit (1);			\
  } while (0)


int
main (int argc, char **argv)
{
  time_t current_time_t = time (NULL);
  struct tm *current_time_tm = localtime (&current_time_t);
  /* List of all locales.  */
  MPlist *locale_list, *plist;
  /* Text to be shown.  */
  MText *mt;
  int i;

  for (i = 1; i < argc; i++)
    {
      if (! strcmp (argv[i], "--help")
	       || ! strcmp (argv[i], "-h")
	       || ! strcmp (argv[i], "-?"))
	help_exit (argv[0], 0);
      else if (! strcmp (argv[i], "--version"))
	{
	  printf ("mdate (m17n library) %s\n", VERSION);
	  printf ("Copyright (C) 2003 AIST, JAPAN\n");
	  exit (0);
	}
      else
	help_exit (argv[0], 1);
    }


  /* Initialize the m17n library.  */
  M17N_INIT ();
  if (merror_code != MERROR_NONE)
    FATAL_ERROR ("%s\n", "Fail to initialize the m17n library.");

  /* Get a locale list in LOCALE_LIST, and generate an M-text that
     contains date string in each locale.  */
  locale_list = list_system_locales ();
  mt = mtext ();
  plist = locale_list;
  while (mplist_key (plist) != Mnil)
    {
      char *name = msymbol_name (mplist_key (plist));
      char fmtbuf[256];
      int len;
      MLocale *locale = mplist_value (plist);
      MSymbol coding = mlocale_get_prop (locale, Mcoding);
      /* One line text for this locale.  The format is:
	 "LOCALE-NAME:	DATE-AND-TIME-STRING".  */
      MText *thisline;

      sprintf (fmtbuf, "%16s: ", name);
      len = strlen (fmtbuf);
      thisline = mconv_decode_buffer (coding, (unsigned char *) fmtbuf, len);
      if (thisline)
	{
	  mlocale_set (LC_TIME, name);
	  if (mtext_ftime (thisline, "%c", current_time_tm, NULL) > 0)
	    {
	      mtext_cat_char (thisline, '\n');
	      mtext_cat (mt, thisline);
	    }
	  m17n_object_unref (thisline);
	}
      plist = mplist_next (plist);
    }

  /* Show the generated M-text by another example program "mview".  */
  {
    FILE *p = popen ("mview", "w");

    if (!p)
      FATAL_ERROR ("%s\n", "Can't run the program mview!");
    mconv_encode_stream (Mcoding_utf_8, mt, p);
    fclose (p);
  }

  /* Clear away.  */
  m17n_object_unref (locale_list);
  m17n_object_unref (mt);
  M17N_FINI ();

  exit (0);
}
#endif /* not FOR_DOXYGEN */
