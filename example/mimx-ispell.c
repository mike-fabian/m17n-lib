/* imx-ispell.c -- Input method external module for Ispell.
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
    @page mimx-ispell external module for the input method <en, ispell>

    @section mimx-ispell-description DESCRIPTION

    The shared library mimx-ispell.so is an external module used by the
    input method <en, ispell>.  It exports these functions.

    <ul>
    <li> init

    Initialize this library.

    <li> fini

    Finalize this library.

    <li> ispell_word

    Check the spell of the current preedit text (English) and, if the
    spell is incorrect,  return a list of candidates.

    </ul>

    This program is just for demonstrating how to write an external
    module for an m17n input method, not for an actual use.

    @section mimx-ispell-seealso See also
    @ref mdbIM
*/

#ifndef FOR_DOXYGEN

#include <stdio.h>
#include <string.h>
#include <config.h>
#include <m17n-gui.h>

#ifdef HAVE_ISPELL

static int initialized = 0;
static MFace *mface_overstrike = NULL;

static MPlist *
add_action (MPlist *actions, MSymbol name, MSymbol key, void *val)
{
  MPlist *action = mplist ();

  mplist_add (action, Msymbol, name);
  if (key != Mnil)
    mplist_add (action, key, val);
  mplist_add (actions, Mplist, action);
  m17n_object_unref (action);
  return actions;
}

MPlist *
init (MPlist *args)
{
  if (! initialized++)
    {
      MFaceHLineProp hline;

      hline.type = MFACE_HLINE_STRIKE_THROUGH;
      hline.width = 1;
      hline.color = msymbol ("black");
      mface_overstrike = mface ();
      mface_put_prop (mface_overstrike, Mhline, &hline);
    }
  return NULL;
}

MPlist *
fini (MPlist *args)
{
  if (! --initialized)
    m17n_object_unref (mface_overstrike);
  return NULL;
}

MPlist *
ispell_word (MPlist *args)
{
  MInputContext *ic;
  unsigned char buf[256];
  int nbytes;
  MPlist *actions, *candidates, *plist;
  char command[256];
  char **words;
  FILE *ispell;
  char *p = (char *) buf;
  int i, n;
  MSymbol init_state;
  MSymbol select_state;
  MText *mt;

  ic = mplist_value (args);
  args = mplist_next (args);
  init_state = (MSymbol) mplist_value (args);
  args = mplist_next (args);
  select_state = (MSymbol) mplist_value (args);
  nbytes = mconv_encode_buffer (Mcoding_us_ascii, ic->preedit, buf, 256);

  actions = mplist ();

  if (nbytes < 3)
    return add_action (actions, msymbol ("shift"), Msymbol, init_state);

  buf[nbytes] = '\0';
  sprintf (command, "echo %s | ispell -a -m", (char *) buf);
  ispell = popen (command, "r");
  if (! ispell)
    return add_action (actions, msymbol ("shift"), Msymbol, init_state);

  /* Skip the heading line.  */
  fgets (p, 256, ispell);
  /* Read just 256 bytes, thus candidates listed after the first 256
     bytes are just ignored.  */
  fgets (p, 256, ispell);
  pclose (ispell);
  p[strlen (p) - 1] = '\0';
  if (*p != '&' && *p != '#')
    return add_action (actions, msymbol ("shift"), Msymbol, init_state);

  add_action (actions, msymbol ("delete"), Msymbol,  msymbol ("@<"));
  if (*p == '#')
    {
      mt = mtext_dup (ic->preedit);
      mtext_push_prop (mt, 0, mtext_len (mt), Mface, mface_overstrike);
      mplist_add (actions, Mtext, mt);
      add_action (actions, msymbol ("shift"), Msymbol, init_state);
      m17n_object_unref (mt);
      return actions;
    }

  p = strchr (p + 2, ' ');
  if (sscanf (p, "%d", &n) != 1)
    return add_action (actions, msymbol ("shift"), Msymbol, init_state);
  words = alloca (sizeof (char *) * n);
  p = strchr (p + 1, ' ');
  p = strchr (p + 1, ' ');
  for (i = 0; i < n - 1; i++)
    {
      words[i] = ++p;
      p = strchr (p, ',');
      if (! p)
	{
	  n = i - 1;
	  break;
	}
      *p++ = '\0';
    }
  words[i] = ++p;
  candidates = mplist ();
  for (i = 0; i < n; i++)
    {
      mt = mconv_decode_buffer (Mcoding_us_ascii, (unsigned char *) words[i],
				strlen (words[i]));
      mplist_add (candidates, Mtext, mt);
      m17n_object_unref (mt);
    }
  mt = mtext_dup (ic->preedit);
  mtext_push_prop (mt, 0, mtext_len (mt), Mface, mface_overstrike);
  mplist_add (candidates, Mtext, mt);
  m17n_object_unref (mt);
  plist = mplist_add (mplist (), Mplist, candidates);
  m17n_object_unref (candidates);
  mplist_add (actions, Mplist, plist);
  m17n_object_unref (plist);
  add_action (actions, msymbol ("show"), Mnil, NULL);
  add_action (actions, msymbol ("shift"), Msymbol, select_state);
  return actions;
}

#else  /* not HAVE_ISPELL */

MPlist *ispell_word (MPlist *args) { return NULL; }

#endif /* not HAVE_ISPELL */
#endif /* not FOR_DOXYGEN */
