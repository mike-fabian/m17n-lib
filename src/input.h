/* input.h -- header file for the input method module.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012
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

#ifndef _M17N_INPUT_H_
#define _M17N_INPUT_H_

typedef struct _MInputMethodInfo MInputMethodInfo;

struct _MInputMethodInfo
{
  MDatabase *mdb;
  MSymbol language, name, extra;
  MPlist *cmds, *configured_cmds, *bc_cmds;
  MPlist *vars, *configured_vars, *bc_vars;
  MText *description;
  MText *title;
  MPlist *maps;
  MPlist *states;
  MPlist *macros;
  MPlist *externals;
  unsigned long tick;
};

typedef struct MIMState MIMState;

typedef struct MIMMap MIMMap;

typedef struct
{
  /** The current state.  */
  MIMState *state;

  /** The previous state.  */
  MIMState *prev_state;

  /** The current map.  */
  MIMMap *map;

  /** Table of typed keys.  */
  int size, inc, used;
  MSymbol *keys;

  /** Index of the key handled firstly in the current state.  */
  int state_key_head;

  /** Index of the key not yet handled.  */
  int key_head;

  /** Index of the key at the time of committing.  */
  int commit_key_head;

  /** Saved M-text when entered in the current state.  */
  MText *preedit_saved;

  /** The insertion position when shifted to the current state.  */
  int state_pos;

  /** List of markers.  */
  MPlist *markers;

  /** List of variables. */
  MPlist *vars;

  MPlist *vars_saved;

  MText *preceding_text, *following_text;

  int key_unhandled;

  /** Used by minput_win_driver (input-win.c).  */
  void *win_info;

  MPlist *state_hook;

  unsigned long tick;
} MInputContextInfo;

#define MINPUT_KEY_SHIFT_MODIFIER	(1 << 0)
#define MINPUT_KEY_CONTROL_MODIFIER	(1 << 1)
#define MINPUT_KEY_META_MODIFIER	(1 << 2)
#define MINPUT_KEY_ALT_MODIFIER		(1 << 3)
#define MINPUT_KEY_SUPER_MODIFIER	(1 << 4)
#define MINPUT_KEY_HYPER_MODIFIER	(1 << 5)
#define MINPUT_KEY_ALTGR_MODIFIER	(1 << 6)

extern MSymbol minput__char_to_key (int c);

#endif /* not _M17N_INPUT_H_ */
