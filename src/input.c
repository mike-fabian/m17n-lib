/* input.c -- input method module.
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
    @addtogroup m17nInputMethod
    @brief API for Input method.

    An input method is an object to enable inputting various
    characters.  An input method is identified by a pair of symbols,
    LANGUAGE and NAME.  This pair decides a input method driver of the
    input method.  An input method driver is a set of functions for
    handling the input method.  There are two kinds of input methods;
    internal one and foreign one.

    <ul>
    <li> Internal Input Method

    An internal input method has non @c Mnil LANGUAGE, and the body is
    defined in the m17n database by the tag <Minput_method, LANGUAGE,
    NAME>.  For this kind of input methods, the m17n library uses two
    predefined input method drivers, one for CUI use and the other for
    GUI use.  Those driver utilize the input processing engine
    provided by the m17n library itself.  The m17n database may
    provides an input method that is not only for a specific language.
    The database uses @c Mt as LANGUAGE of such an input method.

    An internal input method accepts an input key which is a symbol
    associated with an input event.  As there is no way for the @c
    m17n @c library to know how input events are represented in an
    application program, an application programmer have to convert an
    input event to an input key by himself.  See the documentation of
    the function minput_event_to_key () for the detail.

    <li> Foreign Input Method

    A foreign input method has @c Mnil LANGUAGE, and the body is
    defined in an external resources (e.g. XIM of X Window System).
    For this kind of input methods, the symbol NAME must have a
    property of key @c Minput_driver, and the value must be a pointer
    to an input method driver.  Therefore, by preparing a proper
    driver, any kind of input method can be treated in the framework
    of the @c m17n @c library.

    For convenience, the m17n-X library provides an input method
    driver that enables the input style of OverTheSpot for XIM, and
    stores @c Minput_driver property of the symbol @c Mxim with a
    pointer to the driver.  See the documentation of m17n GUI API for
    the detail.

    </ul>

    PROCESSING FLOW

    The typical processing flow of handling an input method is: 

     @li open an input method
     @li create an input context for the input method
     @li filter an input key
     @li look up a produced text in the input context  */

/*=*/
/***ja
    @addtogroup m17nInputMethod
    @brief 入力メソッド用API.

    入力メソッドは多様な文字を入力するためのオブジェクトである。入力メ
    ソッドはシンボル LANGUAGE と NAME の組によって識別され、この組によっ
    て入力メソッドドライバが決まる。入力メソッドドライバとは、ある入力メ
    ソッドを扱うための関数の集まりである。 入力メソッドには内部メソッ
    ドと外部メソッドの二種類がある。

    <ul> 
    <li> 内部入力メソッド

    内部入力メソッドは LANGUAGE が @c Mnil 以外のものであり、本体は
    m17n データベースに <Minput_method, LANGUAGE, NAME>というタグ付き
    で定義されている。この種の入力メソッドに対して、m17n ライブラリに
    は CUI 用と GUI 用それぞれの入力メソッドドライバがあらかじめ準備さ
    れている。これらのドライバは m17n ライブラリ自身の入力処理エンジン
    を利用する。m17n データベースには、特定の言語専用でない入力メソッ
    ドも定義することができ、そのような入力メソッドの LANGUAGE は @c Mt 
    である。

    内部入力メソッドは、ユーザの入力イベントに対応したシンボルである入
    力キーを受け取る。@c m17n @c ライブラリ は入力イベントがアプリケー
    ションプログラムでどう表現されているかを知ることができないので、入力
    イベントから入力キーへの変換はアプリケーションプログラマの責任で行
    わなくてはならない。詳細については関数 minput_event_to_key () の説
    明を参照。

    <li> 外部入力メソッド

    外部入力メソッドは LANGUAGE が @c Mnil のものであり、本体は外部の
    リソースとして定義される。（たとえばX Window System のXIM など。) 
    この種の入力メソッドでは、シンボル NAME は@c Minput_driver をキー
    とするプロパティを持ち、その値は入力メソッドドライバへのポインタで
    なくてはならない。したがって、適切なドライバを準備することによって、
    いかなる種類の入力メソッドも @c m17n @c ライブラリ の枠組の中で扱
    う事ができる。

    利便性のため、m17n X ライブラリは XIM の OverTheSpot の入力スタイル
    を実現する入力メソッドドライバを提供し、またシンボル @c Mxim の @c
    Minput_driver プロパティの値としてそのドライバへのポインタを保持し
    ている。詳細については m17n GUI API のドキュメントを参照のこと。

    </ul> 

    処理の流れ

    入力メソッド処理の典型的な処理は以下のようになる。
    
    @li 入力メソッドのオープン
    @li その入力メソッドの入力コンテクストの生成
    @li 入力イベントのフィルタ
    @li 入力コンテクストでの生成テキストの検索     */

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "config.h"

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include "m17n-gui.h"
#include "m17n-misc.h"
#include "internal.h"
#include "mtext.h"
#include "input.h"
#include "symbol.h"
#include "plist.h"
#include "database.h"

static int mdebug_mask = MDEBUG_INPUT;

static MSymbol Minput_method;

/** Symbols to load an input method data.  */
static MSymbol Mtitle, Mmacro, Mmodule, Mstate;

/** Symbols for actions.  */
static MSymbol Minsert, Mdelete, Mmark, Mmove, Mpushback, Mundo, Mcall, Mshift;
static MSymbol Mselect, Mshow, Mhide;
static MSymbol Mset, Madd, Msub, Mmul, Mdiv, Mequal, Mless, Mgreater;

static MSymbol Mcandidate_list, Mcandidate_index;

static MSymbol Minit, Mfini;

/** Symbols for key events.  */
static MSymbol one_char_symbol[256];

static MSymbol M_key_alias;

static MSymbol M_description, M_command, M_variable;

/** Structure to hold a map.  */

struct MIMMap
{
  /** List of actions to take when we reach the map.  In a root map,
      the actions are executed only when there's no more key.  */
  MPlist *map_actions;

  /** List of deeper maps.  If NULL, this is a terminal map.  */
  MPlist *submaps;

  /** List of actions to take when we leave the map successfully.  In
      a root map, the actions are executed only when none of submaps
      handle the current key.  */
  MPlist *branch_actions;
};

typedef MPlist *(*MIMExternalFunc) (MPlist *plist);

typedef struct
{
  void *handle;
  MPlist *func_list;		/* function name vs (MIMExternalFunc *) */
} MIMExternalModule;

struct MIMState
{
  /** Name of the state.  */
  MSymbol name;

  /** Title of the state, or NULL.  */
  MText *title;

  /** Key translation map of the state.  Built by merging all maps of
      branches.  */
  MIMMap *map;
};


static int
marker_code (MSymbol sym)
{
  char *name;

  if (sym == Mnil)
    return -1;
  name = MSYMBOL_NAME (sym);
  return ((name[0] == '@'
	   && ((name[1] >= '0' && name[1] <= '9')
	       || name[1] == '<' || name[1] == '>'
	       || name[1] == '=' || name[1] == '+' || name[1] == '-'
	       || name[1] == '[' || name[1] == ']')
	   && name[2] == '\0')
	  ? name[1] : -1);
}

int
integer_value (MInputContext *ic, MPlist *arg)
{
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  int code;
  MText *preedit = ic->preedit;
  int len = mtext_nchars (preedit);

  if (MPLIST_INTEGER_P (arg))
    return MPLIST_INTEGER (arg);
  code = marker_code (MPLIST_SYMBOL (arg));
  if (code < 0)
    return (int) mplist_get (ic_info->vars, MPLIST_SYMBOL (arg));
  if (code >= '0' && code <= '9')
    code -= '0';
  else if (code == '=')
    code = ic->cursor_pos;
  else if (code == '-' || code == '[')
    code = ic->cursor_pos - 1;
  else if (code == '+' || code == ']')
    code = ic->cursor_pos + 1;
  else if (code == '<')
    code = 0;
  else if (code == '>')
    code = len;
  return (code >= 0 && code < len ? mtext_ref_char (preedit, code) : -1);
}


/* Parse PLIST as an action list while modifying the list to regularize
   actions.  PLIST should have this form:
      PLIST ::= ( (ACTION-NAME ACTION-ARG *) *).
   Return 0 if successfully parsed, otherwise return -1.  */

static int
parse_action_list (MPlist *plist, MPlist *macros)
{
  MPLIST_DO (plist, plist)
    {
      if (MPLIST_MTEXT_P (plist))
	{
	  /* This is a short form of (insert MTEXT).  */
	  /* if (mtext_nchars (MPLIST_MTEXT (plist)) == 0)
	     MERROR (MERROR_IM, -1); */
	}
      else if (MPLIST_PLIST_P (plist)
	       && (MPLIST_MTEXT_P (MPLIST_PLIST (plist))
		   || MPLIST_PLIST_P (MPLIST_PLIST (plist))))
	{
	  MPlist *pl;

	  /* This is a short form of (insert (GROUPS *)).  */
	  MPLIST_DO (pl, MPLIST_PLIST (plist))
	    {
	      if (MPLIST_PLIST_P (pl))
		{
		  MPlist *elt;

		  MPLIST_DO (elt, MPLIST_PLIST (pl))
		    if (! MPLIST_MTEXT_P (elt)
			|| mtext_nchars (MPLIST_MTEXT (elt)) == 0)
		      MERROR (MERROR_IM, -1);
		}
	      else
		{
		  if (! MPLIST_MTEXT_P (pl)
		      || mtext_nchars (MPLIST_MTEXT (pl)) == 0)
		    MERROR (MERROR_IM, -1);
		}
	    }
	}
      else if (MPLIST_INTEGER_P (plist))
	{
	  int c = MPLIST_INTEGER (plist);

	  if (c < 0 || c > MCHAR_MAX)
	    MERROR (MERROR_IM, -1);
	}
      else if (MPLIST_PLIST_P (plist)
	       && MPLIST_SYMBOL_P (MPLIST_PLIST (plist)))
	{
	  MPlist *pl = MPLIST_PLIST (plist);
	  MSymbol action_name = MPLIST_SYMBOL (pl);

	  pl = MPLIST_NEXT (pl);

	  if (action_name == Minsert)
	    {
	      if (MPLIST_MTEXT_P (pl))
		{
		  if (mtext_nchars (MPLIST_MTEXT (pl)) == 0)
		    MERROR (MERROR_IM, -1);
		}
	      else if (MPLIST_PLIST_P (pl))
		{
		  MPLIST_DO (pl, pl)
		    {
		      if (MPLIST_PLIST_P (pl))
			{
			  MPlist *elt;

			  MPLIST_DO (elt, MPLIST_PLIST (pl))
			    if (! MPLIST_MTEXT_P (elt)
				|| mtext_nchars (MPLIST_MTEXT (elt)) == 0)
			      MERROR (MERROR_IM, -1);
			}
		      else
			{
			  if (! MPLIST_MTEXT_P (pl)
			      || mtext_nchars (MPLIST_MTEXT (pl)) == 0)
			    MERROR (MERROR_IM, -1);
			}
		    }
		}
	      else if (! MPLIST_SYMBOL_P (pl))
		MERROR (MERROR_IM, -1); 
	    }
	  else if (action_name == Mselect
		   || action_name == Mdelete
		   || action_name == Mmove)
	    {
	      if (! MPLIST_SYMBOL_P (pl)
		  && ! MPLIST_INTEGER_P (pl))
		MERROR (MERROR_IM, -1);
	    }
	  else if (action_name == Mmark
		   || action_name == Mcall
		   || action_name == Mshift)
	    {
	      if (! MPLIST_SYMBOL_P (pl))
		MERROR (MERROR_IM, -1);
	    }
	  else if (action_name == Mshow || action_name == Mhide
		   || action_name == Mundo)
	    {
	      if (! MPLIST_TAIL_P (pl))
		MERROR (MERROR_IM, -1);
	    }
	  else if (action_name == Mpushback)
	    {
	      if (! MPLIST_INTEGER_P (pl))
		MERROR (MERROR_IM, -1);
	    }
	  else if (action_name == Mset || action_name == Madd
		   || action_name == Msub || action_name == Mmul
		   || action_name == Mdiv)
	    {
	      if (! (MPLIST_SYMBOL_P (pl)
		     && (MPLIST_INTEGER_P (MPLIST_NEXT (pl))
			 || MPLIST_SYMBOL_P (MPLIST_NEXT (pl)))))
		MERROR (MERROR_IM, -1);
	    }
	  else if (action_name == Mequal || action_name == Mless
		   || action_name == Mgreater)
	    {
	      if (! ((MPLIST_INTEGER_P (pl) || MPLIST_SYMBOL_P (pl))
		     && (MPLIST_INTEGER_P (MPLIST_NEXT (pl))
			 || MPLIST_SYMBOL_P (MPLIST_NEXT (pl)))))
		MERROR (MERROR_IM, -1);
	      pl = MPLIST_NEXT (MPLIST_NEXT (pl));
	      if (! MPLIST_PLIST_P (pl))
		MERROR (MERROR_IM, -1);
	      if (parse_action_list (MPLIST_PLIST (pl), macros) < 0)
		MERROR (MERROR_IM, -1);
	      pl = MPLIST_NEXT (pl);
	      if (MPLIST_PLIST_P (pl)
		  && parse_action_list (MPLIST_PLIST (pl), macros) < 0)
		MERROR (MERROR_IM, -1);
	    }
	  else if (! macros || ! mplist_get (macros, action_name))
	    MERROR (MERROR_IM, -1);
	}
      else
	MERROR (MERROR_IM, -1);
    }

  return 0;
}


/* Load a translation into MAP from PLIST.
   PLIST has this form:
      PLIST ::= ( KEYSEQ MAP-ACTION * )  */

static int
load_translation (MIMMap *map, MPlist *plist, MPlist *branch_actions,
		  MPlist *macros)
{
  MSymbol *keyseq;
  int len, i;

  if (MPLIST_MTEXT_P (plist))
    {
      MText *mt = MPLIST_MTEXT (plist);

      len = mtext_nchars (mt);
      if (len == 0 || len != mtext_nbytes (mt))
	MERROR (MERROR_IM, -1);
      keyseq = (MSymbol *) alloca (sizeof (MSymbol) * len);
      for (i = 0; i < len; i++)
	keyseq[i] = one_char_symbol[MTEXT_DATA (mt)[i]];
    }
  else if (MPLIST_PLIST_P (plist))
    {
      MPlist *elt = MPLIST_PLIST (plist);
	  
      len = MPLIST_LENGTH (elt);
      if (len == 0)
	MERROR (MERROR_IM, -1);
      keyseq = (MSymbol *) alloca (sizeof (int) * len);
      for (i = 0; i < len; i++, elt = MPLIST_NEXT (elt))
	{
	  if (MPLIST_INTEGER_P (elt))
	    {
	      int c = MPLIST_INTEGER (elt);

	      if (c < 0 || c >= 0x100)
		MERROR (MERROR_IM, -1);
	      keyseq[i] = one_char_symbol[c];
	    }
	  else if (MPLIST_SYMBOL_P (elt))
	    keyseq[i] = MPLIST_SYMBOL (elt);
	  else
	    MERROR (MERROR_IM, -1);
	}
    }
  else
    MERROR (MERROR_IM, -1);

  for (i = 0; i < len; i++)
    {
      MIMMap *deeper = NULL;

      if (map->submaps)
	deeper = mplist_get (map->submaps, keyseq[i]);
      else
	map->submaps = mplist ();
      if (! deeper)
	{
	  /* Fixme: It is better to make all deeper maps at once.  */
	  MSTRUCT_CALLOC (deeper, MERROR_IM);
	  mplist_put (map->submaps, keyseq[i], deeper);
	}
      map = deeper;
    }

  /* We reach a terminal map.  */
  if (map->map_actions
      || map->branch_actions)
    /* This map is already defined.  We avoid overriding it.  */
    return 0;

  plist = MPLIST_NEXT (plist);
  if (! MPLIST_TAIL_P (plist))
    {
      if (parse_action_list (plist, macros) < 0)
	MERROR (MERROR_IM, -1);
      map->map_actions = plist;
      M17N_OBJECT_REF (plist);
    }
  if (branch_actions)
    {
      map->branch_actions = branch_actions;
      M17N_OBJECT_REF (branch_actions);
    }

  return 0;
}

/* Load a branch from PLIST into MAP.  PLIST has this form:
      PLIST ::= ( MAP-NAME BRANCH-ACTION * )
   MAPS is a plist of raw maps.
   STATE is the current state.  */

static int
load_branch (MPlist *plist, MPlist *maps, MIMMap *map, MPlist *macros)
{
  MSymbol map_name;
  MPlist *branch_actions;

  if (! MPLIST_SYMBOL_P (plist))
    MERROR (MERROR_IM, -1);
  map_name = MPLIST_SYMBOL (plist);
  plist = MPLIST_NEXT (plist);
  if (MPLIST_TAIL_P (plist))
    branch_actions = NULL;
  else if (parse_action_list (plist, macros) < 0)
    MERROR (MERROR_IM, -1);
  else
    branch_actions = plist;
  if (map_name == Mnil)
    {
      map->branch_actions = branch_actions;
      if (branch_actions)
	M17N_OBJECT_REF (branch_actions);
    }
  else if (map_name == Mt)
    {
      map->map_actions = branch_actions;
      if (branch_actions)
	M17N_OBJECT_REF (branch_actions);
    }
  else
    {
      plist = (MPlist *) mplist_get (maps, map_name);
      if (! plist || ! MPLIST_PLIST_P (plist))
	MERROR (MERROR_IM, -1);
      MPLIST_DO (plist, plist)
	if (! MPLIST_PLIST_P (plist)
	    || (load_translation (map, MPLIST_PLIST (plist), branch_actions,
				  macros)
		< 0))
	  MERROR (MERROR_IM, -1);
    }

  return 0;
}

/* Load a macro from PLIST into MACROS.
   PLIST has this from:
      PLIST ::= ( MACRO-NAME ACTION * )
   MACROS is a plist of macro names vs action list.  */
static int
load_macros (MPlist *plist, MPlist *macros)
{
  MSymbol name; 

  if (! MPLIST_SYMBOL_P (plist))
    MERROR (MERROR_IM, -1);
  name = MPLIST_SYMBOL (plist);
  plist = MPLIST_NEXT (plist);
  if (MPLIST_TAIL_P (plist)
      || parse_action_list (plist, macros) < 0)
    MERROR (MERROR_IM, -1);
  mplist_put (macros, name, plist);
  M17N_OBJECT_REF (plist);
  return 0;
}

/* Load an external module from PLIST into EXTERNALS.
   PLIST has this form:
      PLIST ::= ( MODULE-NAME FUNCTION * )
   EXTERNALS is a plist of MODULE-NAME vs (MIMExternalModule *).  */

#ifndef DLOPEN_SHLIB_EXT
#define DLOPEN_SHLIB_EXT ".so"
#endif

static int
load_external_module (MPlist *plist, MPlist *externals)
{
  void *handle;
  MSymbol module;
  char *module_file;
  MIMExternalModule *external;
  MPlist *func_list;
  void *func;

  if (MPLIST_MTEXT_P (plist))
    module = msymbol ((char *) MTEXT_DATA (MPLIST_MTEXT (plist)));
  else if (MPLIST_SYMBOL_P (plist))
    module = MPLIST_SYMBOL (plist);
  module_file = alloca (strlen (MSYMBOL_NAME (module))
			+ strlen (DLOPEN_SHLIB_EXT) + 1);
  sprintf (module_file, "%s%s", MSYMBOL_NAME (module), DLOPEN_SHLIB_EXT);

  handle = dlopen (module_file, RTLD_NOW);
  if (! handle)
    {
      fprintf (stderr, "%s\n", dlerror ());
      MERROR (MERROR_IM, -1);
    }
  func_list = mplist ();
  MPLIST_DO (plist, MPLIST_NEXT (plist))
    {
      if (! MPLIST_SYMBOL_P (plist))
	MERROR_GOTO (MERROR_IM, err_label);
      func = dlsym (handle, MSYMBOL_NAME (MPLIST_SYMBOL (plist)));
      if (! func)
	MERROR_GOTO (MERROR_IM, err_label);
      mplist_add (func_list, MPLIST_SYMBOL (plist), func);
    }

  MSTRUCT_MALLOC (external, MERROR_IM);
  external->handle = handle;
  external->func_list = func_list;
  mplist_add (externals, module, external);
  return 0;

 err_label:
  dlclose (handle);
  M17N_OBJECT_UNREF (func_list);
  return -1;
}


/** Load a state from PLIST into a newly allocated state object.
    PLIST has this form:
      PLIST ::= ( STATE-NAME STATE-TITLE ? BRANCH * )
      BRANCH ::= ( MAP-NAME BRANCH-ACTION * )
   MAPS is a plist of defined maps.
   Return the state object.  */

static MIMState *
load_state (MPlist *plist, MPlist *maps, MSymbol language, MPlist *macros)
{
  MIMState *state;

  MSTRUCT_CALLOC (state, MERROR_IM);
  if (! MPLIST_SYMBOL_P (plist))
    MERROR (MERROR_IM, NULL);
  state->name = MPLIST_SYMBOL (plist);
  plist = MPLIST_NEXT (plist);
  if (MPLIST_MTEXT_P (plist))
    {
      state->title = MPLIST_MTEXT (plist);
      mtext_put_prop (state->title, 0, mtext_nchars (state->title),
		      Mlanguage, language);
      M17N_OBJECT_REF (state->title);
      plist = MPLIST_NEXT (plist);
    }
  MSTRUCT_CALLOC (state->map, MERROR_IM);
  MPLIST_DO (plist, plist)
    if (! MPLIST_PLIST_P (plist)
	|| load_branch (MPLIST_PLIST (plist), maps, state->map, macros) < 0)
      MERROR (MERROR_IM, NULL);
  return state;
}


static void
free_map (MIMMap *map)
{
  MPlist *plist;

  M17N_OBJECT_UNREF (map->map_actions);
  if (map->submaps)
    {
      MPLIST_DO (plist, map->submaps)
	free_map ((MIMMap *) MPLIST_VAL (plist));
      M17N_OBJECT_UNREF (map->submaps);
    }
  M17N_OBJECT_UNREF (map->branch_actions);
  free (map);
}

/* Load an input method from PLIST into IM_INTO, and return it.  */

static int
load_input_method (MSymbol language, MSymbol name, MPlist *plist,
		   MInputMethodInfo *im_info)
{
  MText *title = NULL;
  MPlist *maps = NULL;
  MPlist *states = NULL;
  MPlist *externals = NULL;
  MPlist *macros = NULL;
  MPlist *elt;

  for (; MPLIST_PLIST_P (plist); plist = MPLIST_NEXT (plist))
    {
      elt = MPLIST_PLIST (plist);
      if (! MPLIST_SYMBOL_P (elt))
	MERROR_GOTO (MERROR_IM, err);
      if (MPLIST_SYMBOL (elt) == Mtitle)
	{
	  elt = MPLIST_NEXT (elt);
	  if (MPLIST_MTEXT_P (elt))
	    {
	      title = MPLIST_MTEXT (elt);
	      M17N_OBJECT_REF (title);
	    }
	  else
	    MERROR_GOTO (MERROR_IM, err);
	}
      else if (MPLIST_SYMBOL (elt) == Mmap)
	{
	  maps = mplist__from_alist (MPLIST_NEXT (elt));
	  if (! maps)
	    MERROR_GOTO (MERROR_IM, err);
	}
      else if (MPLIST_SYMBOL (elt) == Mmacro)
	{
	  macros = mplist ();
	  MPLIST_DO (elt, MPLIST_NEXT (elt))
	  {
	    if (! MPLIST_PLIST_P (elt)
		|| load_macros (MPLIST_PLIST (elt), macros) < 0)
	      MERROR_GOTO (MERROR_IM, err);
	  }
	}
      else if (MPLIST_SYMBOL (elt) == Mmodule)
	{
	  externals = mplist ();
	  MPLIST_DO (elt, MPLIST_NEXT (elt))
	  {
	    if (! MPLIST_PLIST_P (elt)
		|| load_external_module (MPLIST_PLIST (elt), externals) < 0)
	      MERROR_GOTO (MERROR_IM, err);
	  }
	}
      else if (MPLIST_SYMBOL (elt) == Mstate)
	{
	  states = mplist ();
	  MPLIST_DO (elt, MPLIST_NEXT (elt))
	  {
	    MIMState *state;

	    if (! MPLIST_PLIST_P (elt))
	      MERROR_GOTO (MERROR_IM, err);
	    state = load_state (MPLIST_PLIST (elt), maps, language, macros);
	    if (! state)
	      MERROR_GOTO (MERROR_IM, err);
	    mplist_put (states, state->name, state);
	  }
	}
    }

  if (maps)
    {
      MPLIST_DO (elt, maps)
	M17N_OBJECT_UNREF (MPLIST_VAL (elt));
      M17N_OBJECT_UNREF (maps);
    }
  if (! title)
    title = mtext_from_data (MSYMBOL_NAME (name), MSYMBOL_NAMELEN (name),
			     MTEXT_FORMAT_US_ASCII);
  im_info->title = title;
  im_info->externals = externals;
  im_info->macros = macros;
  im_info->states = states;
  return 0;

 err:
  if (maps)
    {
      MPLIST_DO (elt, maps)
	M17N_OBJECT_UNREF (MPLIST_VAL (elt));
      M17N_OBJECT_UNREF (maps);
    }
  if (title)
    M17N_OBJECT_UNREF (title);
  if (states)
    {
      MPLIST_DO (plist, states)
      {
	MIMState *state = (MIMState *) MPLIST_VAL (plist);

	if (state->title)
	  M17N_OBJECT_UNREF (state->title);
	if (state->map)
	  free_map (state->map);
	free (state);
      }
      M17N_OBJECT_UNREF (states);
    }
  if (externals)
    {
      MPLIST_DO (plist, externals)
      {
	MIMExternalModule *external = MPLIST_VAL (plist);

	dlclose (external->handle);
	M17N_OBJECT_UNREF (external->func_list);
	free (external);
	MPLIST_KEY (plist) = Mt;
      }
      M17N_OBJECT_UNREF (externals);
    }
  return -1;
}



static int take_action_list (MInputContext *ic, MPlist *action_list);

static void
shift_state (MInputContext *ic, MSymbol state_name)
{
  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  MIMState *state;

  /* Find a state to shift to.  If not found, shift to the initial
     state.  */
  state = (MIMState *) mplist_get (im_info->states, state_name);
  if (! state)
    state = (MIMState *) MPLIST_VAL (im_info->states);

  MDEBUG_PRINT1 ("\n[IM] state-shift (%s)", MSYMBOL_NAME (state->name));

  /* Enter the new state.  */
  ic_info->state = state;
  ic_info->map = state->map;
  ic_info->state_key_head = ic_info->key_head;
  if (state == (MIMState *) MPLIST_VAL (im_info->states))
    {
      /* We have shifted to the initial state.  */
      MPlist *p;

      mtext_put_prop_values (ic->preedit, 0, mtext_nchars (ic->preedit),
			     Mcandidate_list, NULL, 0);
      mtext_put_prop_values (ic->preedit, 0, mtext_nchars (ic->preedit),
			     Mcandidate_index, NULL, 0);
      mtext_cat (ic->produced, ic->preedit);
      if ((mdebug__flag & mdebug_mask)
	  && mtext_nchars (ic->produced) > 0)
	{
	  int i;

	  MDEBUG_PRINT (" (produced");
	    for (i = 0; i < mtext_nchars (ic->produced); i++)
	      MDEBUG_PRINT1 (" U+%04X", mtext_ref_char (ic->produced, i));
	  MDEBUG_PRINT (")");
	}
      mtext_reset (ic->preedit);
      ic->candidate_list = NULL;
      ic->candidate_show = 0;
      ic->preedit_changed = ic->candidates_changed = 1;
      MPLIST_DO (p, ic_info->markers)
	MPLIST_VAL (p) = 0;
      MPLIST_DO (p, ic_info->vars)
	MPLIST_VAL (p) = 0;
      ic->cursor_pos = 0;
      memmove (ic_info->keys, ic_info->keys + ic_info->state_key_head,
	       sizeof (int) * (ic_info->used - ic_info->state_key_head));
      ic_info->used -= ic_info->state_key_head;
      ic_info->state_key_head = ic_info->key_head = 0;
    }
  mtext_cpy (ic_info->preedit_saved, ic->preedit);
  ic_info->state_pos = ic->cursor_pos;
  ic->status = state->title;
  if (! ic->status)
    ic->status = im_info->title;
  ic->status_changed = 1;
  if (ic_info->key_head == ic_info->used
      && ic_info->map == ic_info->state->map
      && ic_info->map->map_actions)
    {
      MDEBUG_PRINT (" init-actions:");
      take_action_list (ic, ic_info->map->map_actions);
    }
}

/* Find a candidate group that contains a candidate number INDEX from
   PLIST.  Set START_INDEX to the first candidate number of the group,
   END_INDEX to the last candidate number plus 1, GROUP_INDEX to the
   candidate group number if they are non-NULL.  If INDEX is -1, find
   the last candidate group.  */

static MPlist *
find_candidates_group (MPlist *plist, int index,
		       int *start_index, int *end_index, int *group_index)
{
  int i = 0, gidx = 0, len;

  MPLIST_DO (plist, plist)
    {
      if (MPLIST_MTEXT_P (plist))
	len = mtext_nchars (MPLIST_MTEXT (plist));
      else
	len = mplist_length (MPLIST_PLIST (plist));
      if (index < 0 ? MPLIST_TAIL_P (MPLIST_NEXT (plist))
	  : i + len > index)
	{
	  if (start_index)
	    *start_index = i;
	  if (end_index)
	    *end_index = i + len;
	  if (group_index)
	    *group_index = gidx;
	  return plist;
	}
      i += len;
      gidx++;
    }
  return NULL;
}

static void
preedit_insert (MInputContext *ic, int pos, MText *mt, int c)
{
  MInputContextInfo *ic_info = ((MInputContext *) ic)->info;
  MPlist *markers;
  int nchars = mt ? mtext_nchars (mt) : 1;

  if (mt)
    mtext_ins (ic->preedit, pos, mt);
  else
    mtext_ins_char (ic->preedit, pos, c, 1);
  MPLIST_DO (markers, ic_info->markers)
    if (MPLIST_INTEGER (markers) > pos)
      MPLIST_VAL (markers) = (void *) (MPLIST_INTEGER (markers) + nchars);
  if (ic->cursor_pos >= pos)
    ic->cursor_pos += nchars;
  ic->preedit_changed = 1;
}


static void
preedit_delete (MInputContext *ic, int from, int to)
{
  MInputContextInfo *ic_info = ((MInputContext *) ic)->info;
  MPlist *markers;

  mtext_del (ic->preedit, from, to);
  MPLIST_DO (markers, ic_info->markers)
    {
      if (MPLIST_INTEGER (markers) > to)
	MPLIST_VAL (markers)
	  = (void *) (MPLIST_INTEGER (markers) - (to - from));
      else if (MPLIST_INTEGER (markers) > from);
	MPLIST_VAL (markers) = (void *) from;
    }
  if (ic->cursor_pos >= to)
    ic->cursor_pos -= to - from;
  else if (ic->cursor_pos > from)
    ic->cursor_pos = from;
  ic->preedit_changed = 1;
}


static int
new_index (MInputContext *ic, int current, int limit, MSymbol sym, MText *mt)
{
  int code = marker_code (sym);

  if (mt && (code == '[' || code == ']'))
    {
      int pos = current;

      if (code == '[' && current > 0)
	{
	  if (mtext_prop_range (mt, Mcandidate_list, pos - 1, &pos, NULL, 1)
	      && pos > 0)
	    current = pos;
	}
      else if (code == ']' && current < mtext_nchars (mt))
	{
	  if (mtext_prop_range (mt, Mcandidate_list, pos, NULL, &pos, 1))
	    current = pos;
	}
      return current;
    }
  if (code >= 0)
    return (code == '<' ? 0
	    : code == '>' ? limit
	    : code == '-' ? current - 1
	    : code == '+' ? current + 1
	    : code == '=' ? current
	    : code - '0' > limit ? limit
	    : code - '0');
  if (! ic)  
    return 0;
  return (int) mplist_get (((MInputContextInfo *) ic->info)->markers, sym);
}

static void
update_candidate (MInputContext *ic, MTextProperty *prop, int idx)
{
  int from = mtext_property_start (prop);
  int to = mtext_property_end (prop);
  int start;
  MPlist *candidate_list = mtext_property_value (prop);
  MPlist *group = find_candidates_group (candidate_list, idx, &start,
					 NULL, NULL);
  int ingroup_index = idx - start;
  MText *mt;

  preedit_delete (ic, from, to);
  if (MPLIST_MTEXT_P (group))
    {
      mt = MPLIST_MTEXT (group);
      preedit_insert (ic, from, NULL, mtext_ref_char (mt, ingroup_index));
      to = from + 1;
    }
  else
    {
      int i;
      MPlist *plist;

      for (i = 0, plist = MPLIST_PLIST (group); i < ingroup_index;
	   i++, plist = MPLIST_NEXT (plist));
      mt = MPLIST_MTEXT (plist);
      preedit_insert (ic, from, mt, 0);
      to = from + mtext_nchars (mt);
    }
  mtext_put_prop (ic->preedit, from, to, Mcandidate_list, candidate_list);
  mtext_put_prop (ic->preedit, from, to, Mcandidate_index, (void *) idx);
  ic->cursor_pos = to;
}


static int
take_action_list (MInputContext *ic, MPlist *action_list)
{
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  MPlist *candidate_list = ic->candidate_list;
  int candidate_index = ic->candidate_index;
  int candidate_show = ic->candidate_show;
  MTextProperty *prop;

  MPLIST_DO (action_list, action_list)
    {
      MPlist *action;
      MSymbol name;
      MPlist *args;

      if (MPLIST_MTEXT_P (action_list)
	  || MPLIST_INTEGER_P (action_list))
	name = Minsert, args = action_list;
      else if (MPLIST_PLIST_P (action_list)
	       && (MPLIST_MTEXT_P (MPLIST_PLIST (action_list))
		   || MPLIST_PLIST_P (MPLIST_PLIST (action_list))))
	name = Minsert, args = action_list;
      else
	{
	  action = MPLIST_PLIST (action_list);
	  name = MPLIST_SYMBOL (action);
	  args = MPLIST_NEXT (action);
	}

      MDEBUG_PRINT1 (" %s", MSYMBOL_NAME (name));
      if (name == Minsert)
	{
	  if (MPLIST_MTEXT_P (args))
	    preedit_insert (ic, ic->cursor_pos, MPLIST_MTEXT (args), 0);
	  else if (MPLIST_INTEGER_P (args))
	    preedit_insert (ic, ic->cursor_pos, NULL, MPLIST_INTEGER (args));
	  else if (MPLIST_SYMBOL_P (args))
	    {
	      int c = integer_value (ic, args);

	      if (c >= 0 && c <= MCHAR_MAX)
		preedit_insert (ic, ic->cursor_pos, NULL, c);
	    }
	  else
	    {
	      MText *mt;
	      int len;

	      args = MPLIST_PLIST (args);
	      if (MPLIST_MTEXT_P (args))
		{
		  preedit_insert (ic, ic->cursor_pos, NULL,
				  mtext_ref_char (MPLIST_MTEXT (args), 0));
		  len = 1;
		}
	      else
		{
		  mt = MPLIST_MTEXT (MPLIST_PLIST (args));
		  preedit_insert (ic, ic->cursor_pos, mt, 0);
		  len = mtext_nchars (mt);
		}
	      mtext_put_prop (ic->preedit,
			      ic->cursor_pos - len, ic->cursor_pos,
			      Mcandidate_list, args);
	      mtext_put_prop (ic->preedit,
			      ic->cursor_pos - len, ic->cursor_pos,
			      Mcandidate_index, (void *) 0);
	    }
	}
      else if (name == Mselect)
	{
	  int start, end;
	  int code, idx, gindex;
	  int pos = ic->cursor_pos;
	  MPlist *group;

	  if (pos == 0
	      || ! (prop = mtext_get_property (ic->preedit, pos - 1,
					       Mcandidate_list)))
	    continue;
	  if (MPLIST_SYMBOL_P (args))
	    {
	      code = marker_code (MPLIST_SYMBOL (args));
	      if (code < 0)
		continue;
	    }
	  else
	    code = -1;
	  idx = (int) mtext_get_prop (ic->preedit, pos - 1, Mcandidate_index);
	  group = find_candidates_group (mtext_property_value (prop), idx,
					 &start, &end, &gindex);

	  if (code != '[' && code != ']')
	    {
	      idx = (start
		     + (code >= 0
			? new_index (NULL, ic->candidate_index - start,
				     end - start - 1, MPLIST_SYMBOL (args),
				     NULL)
			: MPLIST_INTEGER (args)));
	      if (idx < 0)
		{
		  find_candidates_group (mtext_property_value (prop), -1,
					 NULL, &end, NULL);
		  idx = end - 1;
		}
	      else if (idx >= end
		       && MPLIST_TAIL_P (MPLIST_NEXT (group)))
		idx = 0;
	    }
	  else
	    {
	      int ingroup_index = idx - start;
	      int len;

	      group = mtext_property_value (prop);
	      len = mplist_length (group);
	      if (code == '[')
		{
		  gindex--;
		  if (gindex < 0)
		    gindex = len - 1;;
		}
	      else
		{
		  gindex++;
		  if (gindex >= len)
		    gindex = 0;
		}
	      for (idx = 0; gindex > 0; gindex--, group = MPLIST_NEXT (group))
		idx += (MPLIST_MTEXT_P (group)
			? mtext_nchars (MPLIST_MTEXT (group))
			: mplist_length (MPLIST_PLIST (group)));
	      len = (MPLIST_MTEXT_P (group)
		     ? mtext_nchars (MPLIST_MTEXT (group))
		     : mplist_length (MPLIST_PLIST (group)));
	      if (ingroup_index >= len)
		ingroup_index = len - 1;
	      idx += ingroup_index;
	    }
	  update_candidate (ic, prop, idx);
	}
      else if (name == Mshow)
	ic->candidate_show = 1;
      else if (name == Mhide)
	ic->candidate_show = 0;
      else if (name == Mdelete)
	{
	  int len = mtext_nchars (ic->preedit);
	  int to = (MPLIST_SYMBOL_P (args)
		    ? new_index (ic, ic->cursor_pos, len, MPLIST_SYMBOL (args),
				 ic->preedit)
		    : MPLIST_INTEGER (args));

	  if (to < 0)
	    to = 0;
	  else if (to > len)
	    to = len;
	  if (to < ic->cursor_pos)
	    preedit_delete (ic, to, ic->cursor_pos);
	  else if (to > ic->cursor_pos)
	    preedit_delete (ic, ic->cursor_pos, to);
	}
      else if (name == Mmove)
	{
	  int len = mtext_nchars (ic->preedit);
	  int pos
	    = (MPLIST_SYMBOL_P (args)
	       ? new_index (ic, ic->cursor_pos, len, MPLIST_SYMBOL (args),
			    ic->preedit)
	       : MPLIST_INTEGER (args));

	  if (pos < 0)
	    pos = 0;
	  else if (pos > len)
	    pos = len;
	  if (pos != ic->cursor_pos)
	    {
	      ic->cursor_pos = pos;
	      ic->preedit_changed = 1;
	    }
	}
      else if (name == Mmark)
	{
	  int code = marker_code (MPLIST_SYMBOL (args));

	  if (code < 0)
	    mplist_put (ic_info->markers, MPLIST_SYMBOL (args),
			(void *) ic->cursor_pos);
	}
      else if (name == Mpushback)
	{
	  int num = MPLIST_INTEGER (args);

	  if (num > 0)
	    ic_info->key_head -= num;
	  else
	    ic_info->key_head = num;
	  if (ic_info->key_head > ic_info->used)
	    ic_info->key_head = ic_info->used;
	}
      else if (name == Mcall)
	{
	  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
	  MIMExternalFunc func = NULL;
	  MSymbol module, func_name;
	  MPlist *func_args, *val;
	  int ret = 0;

	  module = MPLIST_SYMBOL (args);
	  args = MPLIST_NEXT (args);
	  func_name = MPLIST_SYMBOL (args);

	  if (im_info->externals)
	    {
	      MIMExternalModule *external
		= (MIMExternalModule *) mplist_get (im_info->externals,
						    module);
	      if (external)
		func = (MIMExternalFunc) mplist_get (external->func_list,
						     func_name);
	    }
	  if (! func)
	    continue;
	  func_args = mplist ();
	  mplist_add (func_args, Mt, ic);
	  MPLIST_DO (args, MPLIST_NEXT (args))
	    {
	      int code;

	      if (MPLIST_KEY (args) == Msymbol
		  && MPLIST_KEY (args) != Mnil
		  && (code = marker_code (MPLIST_SYMBOL (args))) >= 0)
		{
		  code = new_index (ic, ic->cursor_pos, 
				    mtext_nchars (ic->preedit),
				    MPLIST_SYMBOL (args), ic->preedit);
		  mplist_add (func_args, Minteger, (void *) code);
		}
	      else
		mplist_add (func_args, MPLIST_KEY (args), MPLIST_VAL (args));
	    }
	  val = (func) (func_args);
	  M17N_OBJECT_UNREF (func_args);
	  if (val && ! MPLIST_TAIL_P (val))
	    ret = take_action_list (ic, val);
	  M17N_OBJECT_UNREF (val);
	  if (ret < 0)
	    return ret;
	}
      else if (name == Mshift)
	{
	  shift_state (ic, MPLIST_SYMBOL (args));
	}
      else if (name == Mundo)
	{
	  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
	  int unhandle = 0;

	  mtext_reset (ic->preedit);
	  mtext_reset (ic_info->preedit_saved);
	  ic->cursor_pos = ic_info->state_pos = 0;
	  ic_info->state_key_head = ic_info->key_head = 0;
	  ic_info->used -= 2;
	  if (ic_info->used < 0)
	    {
	      ic_info->used = 0;
	      unhandle = 1;
	    }
	  shift_state (ic, ((MIMState *) MPLIST_VAL (im_info->states))->name);
	  if (unhandle)
	    return -1;
	  break;
	}
      else if (name == Mset || name == Madd || name == Msub
	       || name == Mmul || name == Mdiv)
	{
	  MSymbol sym = MPLIST_SYMBOL (args);
	  int val1 = (int) mplist_get (ic_info->vars, sym), val2;

	  args = MPLIST_NEXT (args);
	  val2 = integer_value (ic, args);
	  if (name == Mset)
	    val1 = val2;
	  else if (name == Madd)
	    val1 += val2;
	  else if (name == Msub)
	    val1 -= val2;
	  else if (name == Mmul)
	    val1 *= val2;
	  else
	    val1 /= val2;
	  mplist_put (ic_info->vars, sym, (void *) val1);
	  MDEBUG_PRINT2 ("(%s=%d)", MSYMBOL_NAME (sym), val1);
	}
      else if (name == Mequal || name == Mless || name == Mgreater)
	{
	  int val1, val2;
	  MPlist *actions1, *actions2;
	  int ret = 0;

	  val1 = integer_value (ic, args);
	  args = MPLIST_NEXT (args);
	  val2 = integer_value (ic, args);
	  args = MPLIST_NEXT (args);
	  actions1 = MPLIST_PLIST (args);
	  args = MPLIST_NEXT (args);
	  if (MPLIST_TAIL_P (args))
	    actions2 = NULL;
	  else
	    actions2 = MPLIST_PLIST (args);
	  if (name == Mequal ? val1 == val2
	      : name == Mless ? val1 < val2
	      : val1 > val2)
	    ret = take_action_list (ic, actions1);
	  else if (actions2)
	    ret = take_action_list (ic, actions2);
	  if (ret < 0)
	    return ret;
	}
      else
	{
	  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
	  MPlist *actions;

	  if (im_info->macros
	      && (actions = mplist_get (im_info->macros, name)))
	    {
	      if (take_action_list (ic, actions) < 0)
		return -1;
	    };
	}
    }

  prop = NULL;
  ic->candidate_list = NULL;
  if (ic->cursor_pos > 0
      && (prop = mtext_get_property (ic->preedit, ic->cursor_pos - 1,
				     Mcandidate_list)))
    {
      ic->candidate_list = mtext_property_value (prop);
      ic->candidate_index
	= (int) mtext_get_prop (ic->preedit, ic->cursor_pos - 1,
				Mcandidate_index);
      ic->candidate_from = mtext_property_start (prop);
      ic->candidate_to = mtext_property_end (prop);
    }

  ic->candidates_changed |= (candidate_list != ic->candidate_list
			     || candidate_index != ic->candidate_index
			     || candidate_show != ic->candidate_show);
  return 0;
}


/* Handle the input key KEY in the current state and map specified in
   the input context IC.  If KEY is handled correctly, return 0.
   Otherwise, return -1.  */

static int
handle_key (MInputContext *ic)
{
  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  MIMMap *map = ic_info->map;
  MIMMap *submap = NULL;
  MSymbol key = ic_info->keys[ic_info->key_head];
  int i;

  MDEBUG_PRINT2 ("[IM] handle `%s' in state %s", 
		 MSYMBOL_NAME (key), MSYMBOL_NAME (ic_info->state->name));

  if (map->submaps)
    {
      submap = mplist_get (map->submaps, key);
      if (! submap && (key = msymbol_get (key, M_key_alias)) != Mnil)
	submap = mplist_get (map->submaps, key);
    }

  if (submap)
    {
      MDEBUG_PRINT (" submap-found");
      mtext_cpy (ic->preedit, ic_info->preedit_saved);
      ic->cursor_pos = ic_info->state_pos;
      ic_info->key_head++;
      ic_info->map = map = submap;
      if (map->map_actions)
	{
	  MDEBUG_PRINT (" map-actions:");
	  if (take_action_list (ic, map->map_actions) < 0)
	    return -1;
	}
      else if (map->submaps)
	{
	  for (i = ic_info->state_key_head; i < ic_info->key_head; i++)
	    {
	      MSymbol key = ic_info->keys[i];
	      char *name = msymbol_name (key);

	      if (! name[0] || ! name[1])
		mtext_ins_char (ic->preedit, ic->cursor_pos++, name[0], 1);
	    }
	  ic->preedit_changed = 1;
	}

      /* If this is the terminal map or we have shifted to another
	 state, perform branch actions (if any).  */
      if (! map->submaps || map != ic_info->map)
	{
	  if (map->branch_actions)
	    {
	      MDEBUG_PRINT (" branch-actions:");
	      if (take_action_list (ic, map->branch_actions) < 0)
		return -1;
	    }
	  /* If MAP is still not the root map, shift to the current
	     state.  */
	  if (ic_info->map != ic_info->state->map)
	    shift_state (ic, ic_info->state->name);
	}
      MDEBUG_PRINT ("\n");
    }
  else
    {
      /* MAP can not handle KEY.  */

      /* If MAP is the root map of the initial state, it means that
	 the current input method can not handle KEY.  */
      if (map == ((MIMState *) MPLIST_VAL (im_info->states))->map)
	{
	  MDEBUG_PRINT (" unhandled\n");
	  return -1;
	}

      if (map != ic_info->state->map)
	{
	  /* If MAP is not the root map... */
	  /* If MAP has branch actions, perform them.  */
	  if (map->branch_actions)
	    {
	      MDEBUG_PRINT (" branch-actions:");
	      take_action_list (ic, map->branch_actions);
	    }
	  /* If MAP is still not the root map, shift to the current
	     state. */
	  if (ic_info->map != ic_info->state->map)
	    {
	      shift_state (ic, ic_info->state->name);
	      /* If MAP has branch_actions, perform them.  */
	      if (ic_info->map->branch_actions)
		{
		  MDEBUG_PRINT (" init-actions:");
		  take_action_list (ic, ic_info->map->branch_actions);
		}
	    }
	}
      else
	{
	  /* MAP is the root map, perform branch actions (if any) or
	     shift to the initial state.  */
	  if (map->branch_actions)
	    {
	      MDEBUG_PRINT (" branch-actions:");
	      take_action_list (ic, map->branch_actions);
	    }
	  else
	    shift_state (ic,
			 ((MIMState *) MPLIST_VAL (im_info->states))->name);
	}
      MDEBUG_PRINT ("\n");
    }
  return 0;
}

static void
reset_ic (MInputContext *ic, MSymbol ignore)
{
  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;

  if (im_info->states)
    /* Shift to the initial state.  */
    shift_state (ic, Mnil);
  else
    ic_info->state = NULL;
  MLIST_RESET (ic_info);
  ic_info->map = ic_info->state ? ic_info->state->map : NULL;
  ic_info->state_key_head = ic_info->key_head = 0;
  ic_info->key_unhandled = 0;
  ic->cursor_pos = ic_info->state_pos = 0;
  ic->status = ic_info->state ? ic_info->state->title : NULL;
  if (! ic->status)
    ic->status = im_info->title;
  ic->candidate_list = NULL;
  ic->candidate_show = 0;
  ic->status_changed = ic->preedit_changed = ic->candidates_changed = 1;
  if (ic_info->map && ic_info->map->map_actions)
    take_action_list (ic, ic_info->map->map_actions);
}

static int
open_im (MInputMethod *im)
{
  MDatabase *mdb;
  MInputMethodInfo *im_info;
  MPlist *plist;
  int result;

  mdb = mdatabase_find (Minput_method, im->language, im->name, Mnil);
  if (! mdb)
    return -1;
  plist = mdatabase_load (mdb);
  if (! plist)
    MERROR (MERROR_IM, -1);
  MSTRUCT_CALLOC (im_info, MERROR_IM);
  im->info = im_info;
  result = load_input_method (im->language, im->name, plist, im_info);
  M17N_OBJECT_UNREF (plist);
  if (result < 0)
    MERROR (MERROR_IM, -1);
  return 0;
}

static void
close_im (MInputMethod *im)
{
  MInputMethodInfo *im_info = (MInputMethodInfo *) im->info;
  MPlist *plist;

  if (im_info->title)
    M17N_OBJECT_UNREF (im_info->title);
  if (im_info->states)
    {
      MPLIST_DO (plist, im_info->states)
	{
	  MIMState *state = (MIMState *) MPLIST_VAL (plist);

	  if (state->title)
	    M17N_OBJECT_UNREF (state->title);
	  if (state->map)
	    free_map (state->map);
	  free (state);
	}
      M17N_OBJECT_UNREF (im_info->states);
    }

  if (im_info->macros)
    {
      MPLIST_DO (plist, im_info->macros)
	M17N_OBJECT_UNREF (MPLIST_VAL (plist));	
      M17N_OBJECT_UNREF (im_info->macros);
    }

  if (im_info->externals)
    {
      MPLIST_DO (plist, im_info->externals)
	{
	  MIMExternalModule *external = MPLIST_VAL (plist);

	  dlclose (external->handle);
	  M17N_OBJECT_UNREF (external->func_list);
	  free (external);
	  MPLIST_KEY (plist) = Mt;
	}
      M17N_OBJECT_UNREF (im_info->externals);
    }
  free (im_info);
  im->info = NULL;
}


static int
create_ic (MInputContext *ic)
{
  MInputMethod *im = ic->im;
  MInputMethodInfo *im_info = (MInputMethodInfo *) im->info;
  MInputContextInfo *ic_info;

  if (ic->info)
    ic_info = (MInputContextInfo *) ic->info;
  else
    {
      MSTRUCT_CALLOC (ic_info, MERROR_IM);
      ic->info = ic_info;
    }
  MLIST_INIT1 (ic_info, keys, 8);
  ic_info->markers = mplist ();
  ic_info->vars = mplist ();
  ic_info->preedit_saved = mtext ();
  if (im_info->externals)
    {
      MPlist *func_args = mplist (), *plist;

      mplist_add (func_args, Mt, ic);
      MPLIST_DO (plist, im_info->externals)
	{
	  MIMExternalModule *external = MPLIST_VAL (plist);
	  MIMExternalFunc func
	    = (MIMExternalFunc) mplist_get (external->func_list, Minit);

	  if (func)
	    (func) (func_args);
	}
      M17N_OBJECT_UNREF (func_args);
    }
  reset_ic (ic, Mnil);
  return 0;
}

static void
destroy_ic (MInputContext *ic)
{
  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;

  if (im_info->externals)
    {
      MPlist *func_args = mplist (), *plist;

      mplist_add (func_args, Mt, ic);
      MPLIST_DO (plist, im_info->externals)
	{
	  MIMExternalModule *external = MPLIST_VAL (plist);
	  MIMExternalFunc func
	    = (MIMExternalFunc) mplist_get (external->func_list, Mfini);

	  if (func)
	    (func) (func_args);
	}
      M17N_OBJECT_UNREF (func_args);
    }
  MLIST_FREE1 (ic_info, keys);
  M17N_OBJECT_UNREF (ic_info->preedit_saved);
  M17N_OBJECT_UNREF (ic_info->markers);
  M17N_OBJECT_UNREF (ic_info->vars);
  free (ic->info);
}


/** Handle the input key KEY in the current state and map of IC->info.
    If KEY is handled but no text is produced, return 0, otherwise
    return 1.

    Ignore ARG.  */

static int
filter (MInputContext *ic, MSymbol key, void *arg)
{
  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  int i = 0;

  if (! ic_info->state)
    {
      ic_info->key_unhandled = 1;
      return 0;
    }
  mtext_reset (ic->produced);
  ic->status_changed = ic->preedit_changed = ic->candidates_changed = 0;
  MLIST_APPEND1 (ic_info, keys, key, MERROR_IM);
  ic_info->key_unhandled = 0;
  do {
    if (handle_key (ic) < 0)
      {
	/* KEY was not handled.  Reset the status and break the
	   loop.  */
	reset_ic (ic, Mnil);
	/* This forces returning 1.  */
	ic_info->key_unhandled = 1;
	break;
      }
    if (i++ == 100)
      {
	mdebug_hook ();
	reset_ic (ic, Mnil);
	ic_info->key_unhandled = 1;
	break;
      }
    /* Break the loop if all keys were handled.  */
  } while (ic_info->key_head < ic_info->used);

  /* If the current map is the root of the initial state, we should
     produce any preedit text in ic->produced.  */
  if (ic_info->map == ((MIMState *) MPLIST_VAL (im_info->states))->map
      && mtext_nchars (ic->preedit) > 0)
    shift_state (ic, ((MIMState *) MPLIST_VAL (im_info->states))->name);

  if (mtext_nchars (ic->produced) > 0)
    {
      MSymbol lang = msymbol_get (ic->im->language, Mlanguage);

      if (lang != Mnil)
	mtext_put_prop (ic->produced, 0, mtext_nchars (ic->produced),
			Mlanguage, ic->im->language);
    }

  return (! ic_info->key_unhandled && mtext_nchars (ic->produced) == 0);
}


/** Return 1 if the last event or key was not handled, otherwise
    return 0.

    There is no need of looking up because ic->produced should already
    contain the produced text (if any).

    Ignore KEY.  */

static int
lookup (MInputContext *ic, MSymbol key, void *arg, MText *mt)
{
  mtext_cat (mt, ic->produced);
  mtext_reset (ic->produced);
  return (((MInputContextInfo *) ic->info)->key_unhandled ? -1 : 0);
}

static MPlist *load_im_info_keys;

static MPlist *
load_im_info (MSymbol language, MSymbol name, MSymbol key)
{
  MDatabase *mdb;
  MPlist *plist;

  if (language == Mnil || name == Mnil)
    MERROR (MERROR_IM, NULL);

  mdb = mdatabase_find (Minput_method, language, name, Mnil);
  if (! mdb)
    MERROR (MERROR_IM, NULL);
  mplist_push (load_im_info_keys, key, Mt);
  plist = mdatabase__load_for_keys (mdb, load_im_info_keys);
  mplist_pop (load_im_info_keys);
  return plist;
}


/* Input method command handler.  */

/* List of all (global and local) commands. 
   (LANG:(IM-NAME:(COMMAND ...) ...) ...) ...
   COMMAND is CMD-NAME:(mtext:DESCRIPTION plist:KEYSEQ ...))
   Global commands are storead as (t (t COMMAND ...))  */
static MPlist *command_list;

/* Check if PLIST is a valid command key sequence.
   PLIST must be NULL or:
   [ symbol:KEY | integer:KEY ] ...  */

static int
check_command_keyseq (MPlist *plist)
{
  if (! plist)
    return 0;
  MPLIST_DO (plist, plist)
    {
      if (MPLIST_SYMBOL_P (plist))
	continue;
      else if (MPLIST_INTEGER_P (plist))
	{
	  int n = MPLIST_INTEGER (plist);

	  if (n < 0 || n > 9)
	    return -1;
	  MPLIST_KEY (plist) = Msymbol;
	  MPLIST_VAL (plist) = one_char_symbol['0' + 9];
	}
      else
	return -1;
    }
  return 0;
}

static MText *
get_description_advance (MPlist *plist)
{
  MText *mt;
  int pos;

  if (! MPLIST_MTEXT_P (plist))
    return NULL;
  mt = mplist_pop (plist);
  pos = mtext_chr (mt, '\n'); 
  if (pos > 0)
    {
      MText *detail = mtext_copy (mtext (), 0, mt, pos + 1, mtext_nchars (mt));
      mtext_del (mt, pos, mtext_nchars (mt));
      mtext_put_prop (mt, 0, pos, Mdetail_text, detail);
      M17N_OBJECT_UNREF (detail);
    }
  return mt;
}

static MPlist *
parse_command_list (MPlist *plist, MPlist *global_list)
{
  MPlist *val = mplist ();

  /* PLIST ::= (sym:CMD mtext:DESCRIPTION ? (sym:KEY ...) ...) ... */
  MPLIST_DO (plist, plist)
    {
      MSymbol cmd;
      MText *mt;
      MPlist *this_val, *pl, *p;

      if (! MPLIST_PLIST_P (plist))
	continue;
      pl = MPLIST_PLIST (plist);
      if (! MPLIST_SYMBOL_P (pl))
	continue;
      cmd = MPLIST_SYMBOL (pl);
      pl = MPLIST_NEXT (pl);
      mt = get_description_advance (pl);
      this_val = mplist ();

      if (! mt && global_list)
	{
	  /* Get the description from global_list.  */
	  p = mplist_get (global_list, cmd);
	  if (p && MPLIST_MTEXT (p))
	    {
	      mt = MPLIST_MTEXT (p);
	      M17N_OBJECT_REF (mt);
	    }
	}
      if (! mt)
	mt = mtext ();
      mplist_add (this_val, Mtext, mt);
      M17N_OBJECT_UNREF (mt);

      /* PL ::= (sym:KEY ...) ... */
      MPLIST_DO (pl, pl)
	{
	  if (MPLIST_PLIST_P (pl)
	      && check_command_keyseq (MPLIST_PLIST (pl)) >= 0)
	    /* All the elements are valid keys.  */
	    mplist_add (this_val, Mplist, MPLIST_PLIST (pl));
	}

      mplist_put (val, cmd, this_val);
    }
  return val;
}

static MPlist *
get_command_list (MSymbol language, MSymbol name)
{
  MPlist *per_lang;
  MPlist *plist, *pl;

  if (name == Mnil)
    language = name = Mt;

  if (! command_list)
    {
      MDatabase *mdb = mdatabase_find (msymbol ("input"), M_command,
				       Mnil, Mnil);

      if (mdb && (plist = mdatabase_load (mdb)))
	{
	  pl = parse_command_list (plist, NULL);
	  M17N_OBJECT_UNREF (plist);
	}
      else
	pl = mplist ();
      plist = mplist ();
      mplist_add (plist, Mt, pl);
      command_list = mplist ();
      mplist_add (command_list, Mt, plist);
    }

  per_lang = mplist_get (command_list, language);
  if (per_lang)
    {
      plist = mplist_find_by_key (per_lang, name);
      if (plist)
	return (MPLIST_VAL (plist));
    }
  else
    {
      per_lang = mplist ();
      mplist_add (command_list, language, per_lang);
    }

  /* Now we are sure that we are loading per-im info.  */
  /* Get the global command list.  */
  plist = load_im_info (language, name, M_command);
  if (! plist || mplist_key (plist) == Mnil)
    {
      if (! plist)
	plist = mplist ();
      mplist_add (per_lang, name, plist);
      return plist;
    }
  pl = parse_command_list (mplist_value (plist),
			   mplist_get ((MPlist *) mplist_get (command_list, Mt),
				       Mt));
  M17N_OBJECT_UNREF (plist);
  mplist_put (per_lang, name, pl);
  return pl;
}


/* Input method variable handler.  */

/* List of all variables. 
   (LANG:(IM-NAME:(VAR ...) ...) ...) ...
   VAR is VAR-NAME:(mtext:DESCRIPTION TYPE:VALUE ...))  */

static MPlist *variable_list;

static MPlist *
parse_variable_list (MPlist *plist)
{
  MPlist *val = mplist (), *pl, *p;

  /* PLIST ::= (sym:VAR mtext:DESCRIPTION TYPE:INIT-VAL ...) ...  */
  MPLIST_DO (plist, plist)
    {
      MSymbol var, type;
      MText *mt;
      MPlist *this_val;

      if (! MPLIST_PLIST_P (plist))
	continue;
      pl = MPLIST_PLIST (plist);
      if (! MPLIST_SYMBOL_P (pl))
	continue;
      var = MPLIST_SYMBOL (pl);
      pl = MPLIST_NEXT (pl);
      mt = get_description_advance (pl);
      if (! mt || MPLIST_TAIL_P (pl))
	continue;
      this_val = mplist ();
      mplist_add (this_val, Mtext, mt);
      M17N_OBJECT_UNREF (mt);
      type = MPLIST_KEY (pl);
      mplist_add (this_val, type, MPLIST_VAL (pl));
      MPLIST_DO (pl, MPLIST_NEXT (pl))
	{
	  if (type != MPLIST_KEY (pl)
	      && (type != Minteger || ! MPLIST_PLIST_P (pl)))
	    break;
	  if (MPLIST_PLIST_P (pl))
	    {
	      MPLIST_DO (p, MPLIST_PLIST (pl))
		if (! MPLIST_INTEGER_P (p))
		  break;
	      if (! MPLIST_TAIL_P (p))
		break;
	    }
	  mplist_add (this_val, MPLIST_KEY (pl), MPLIST_VAL (pl));
	}

      mplist_put (val, var, this_val);
    }
  return val;
}


static MPlist *
get_variable_list (MSymbol language, MSymbol name)
{
  MPlist *per_lang;
  MPlist *plist, *pl;

  if (language == Mnil || name == Mnil)
    MERROR (MERROR_IM, NULL);
  if (! variable_list)
    variable_list = mplist ();
  per_lang = mplist_get (variable_list, language);
  if (per_lang)
    {
      plist = mplist_find_by_key (per_lang, name);
      if (plist)
	return (MPLIST_VAL (plist));
    }
  else
    {
      per_lang = mplist ();
      mplist_add (variable_list, language, per_lang);
    }
  plist = load_im_info (language, name, M_variable);
  if (! plist || mplist_key (plist) == Mnil)
    {
      if (! plist)
	plist = mplist ();
      mplist_add (per_lang, name, plist);
      return plist;
    }
  pl = parse_variable_list (mplist_value (plist));
  M17N_OBJECT_UNREF (plist);
  mplist_put (per_lang, name, pl);
  return pl;
}

static void
input_method_hook (MSymbol tag0, MSymbol tag1, MSymbol tag2, MSymbol tag3)
{
  MPlist *plist, *pl, *p;
  char path[PATH_MAX];

  /* Cancel the hook.  */
  msymbol_put (tag0, M_database_hook, NULL);
  tag3 = Mnil;

  mplist_push (load_im_info_keys, M_description, Mt);
  MPLIST_DO (plist, mdatabase__dir_list)
    {
      char *dirname = (char *) MPLIST_VAL (plist);
      int dirlen;
      DIR *dir = opendir (dirname);
      struct dirent *dp;

      if (! dir)
	continue;
      dirlen = strlen (dirname);
      strcpy (path, dirname);
      while ((dp = readdir (dir)) != NULL)
	{
	  /* We can't trust dp->d_nameln.  */
	  int len = strlen (dp->d_name);
	  FILE *fp;

	  if (len > 4 && memcmp (dp->d_name + len - 4, ".mim", 4) == 0)
	    {
	      strcpy (path + dirlen, dp->d_name);
	      fp = fopen (path, "r");
	      if (! fp)
		continue;
	      pl = mplist__from_file (fp, load_im_info_keys);
	      fclose (fp);
	      if (pl)
		{
		  if (MPLIST_PLIST_P (pl))
		    {
		      p = MPLIST_PLIST (pl);
		      p = MPLIST_NEXT (p);
		      if (MPLIST_SYMBOL_P (p))
			{
			  tag1 = MPLIST_VAL (p);
			  p = MPLIST_NEXT (p);
			  if (MPLIST_SYMBOL_P (p))
			    {
			      tag2 = MPLIST_VAL (p);
			      mdatabase_define (tag0, tag1, tag2, tag3,
						NULL, path);
			    }
			}
		    }
		  M17N_OBJECT_UNREF (pl);
		}
	    }
	}
      closedir (dir);
    }
  mplist_pop (load_im_info_keys);
}


/* Support functions for mdebug_dump_im.  */

static void
dump_im_map (MPlist *map_list, int indent)
{
  char *prefix;
  MSymbol key = MPLIST_KEY (map_list);
  MIMMap *map = (MIMMap *) MPLIST_VAL (map_list);

  prefix = (char *) alloca (indent + 1);
  memset (prefix, 32, indent);
  prefix[indent] = '\0';

  fprintf (stderr, "(\"%s\" ", msymbol_name (key));
  if (map->map_actions)
    mdebug_dump_plist (map->map_actions, indent + 2);
  if (map->submaps)
    {
      MPLIST_DO (map_list, map->submaps)
	{
	  fprintf (stderr, "\n%s  ", prefix);
	  dump_im_map (map_list, indent + 2);
	}
    }
  if (map->branch_actions)
    {
      fprintf (stderr, "\n%s  (branch\n%s    ", prefix, prefix);
      mdebug_dump_plist (map->branch_actions, indent + 4);
      fprintf (stderr, ")");      
    }
  fprintf (stderr, ")");
}


static void
dump_im_state (MIMState *state, int indent)
{
  char *prefix;
  MPlist *map_list;

  prefix = (char *) alloca (indent + 1);
  memset (prefix, 32, indent);
  prefix[indent] = '\0';

  fprintf (stderr, "(%s", msymbol_name (state->name));
  if (state->map->submaps)
    {
      MPLIST_DO (map_list, state->map->submaps)
	{
	  fprintf (stderr, "\n%s  ", prefix);
	  dump_im_map (map_list, indent + 2);
	}
    }
  fprintf (stderr, ")");
}



int
minput__init ()
{
  char *key_names[32]
    = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"BackSpace", "Tab", "Linefeed", "Clear", NULL, "Return", NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "Escape", NULL, NULL, NULL, NULL };
  char buf[6], buf2[256];
  int i;
  MPlist *plist;

  Minput_method = msymbol ("input-method");
  msymbol_put (Minput_method, M_database_hook, (void *) input_method_hook);
  Minput_driver = msymbol ("input-driver");
  Mtitle = msymbol ("title");
  Mmacro = msymbol ("macro");
  Mmodule = msymbol ("module");
  Mmap = msymbol ("map");
  Mstate = msymbol ("state");
  Minsert = msymbol ("insert");
  Mdelete = msymbol ("delete");
  Mmove = msymbol ("move");
  Mmark = msymbol ("mark");
  Mpushback = msymbol ("pushback");
  Mundo = msymbol ("undo");
  Mcall = msymbol ("call");
  Mshift = msymbol ("shift");
  Mselect = msymbol ("select");
  Mshow = msymbol ("show");
  Mhide = msymbol ("hide");
  Mset = msymbol ("set");
  Madd = msymbol ("add");
  Msub = msymbol ("sub");
  Mmul = msymbol ("mul");
  Mdiv = msymbol ("div");
  Mequal = msymbol ("=");
  Mless = msymbol ("<");
  Mgreater = msymbol (">");

  Minput_preedit_start = msymbol ("input-preedit-start");
  Minput_preedit_done = msymbol ("input-preedit-done");
  Minput_preedit_draw = msymbol ("input-preedit-draw");
  Minput_status_start = msymbol ("input-status-start");
  Minput_status_done = msymbol ("input-status-done");
  Minput_status_draw = msymbol ("input-status-draw");
  Minput_candidates_start = msymbol ("input-candidates-start");
  Minput_candidates_done = msymbol ("input-candidates-done");
  Minput_candidates_draw = msymbol ("input-candidates-draw");
  Minput_set_spot = msymbol ("input-set-spot");
  Minput_toggle = msymbol ("input-toggle");
  Minput_reset = msymbol ("input-reset");

  Mcandidate_list = msymbol_as_managing_key ("  candidate-list");
  Mcandidate_index = msymbol ("  candidate-index");

  Minit = msymbol ("init");
  Mfini = msymbol ("fini");

  M_key_alias = msymbol ("  key-alias");
  M_description = msymbol ("description");
  M_command = msymbol ("command");
  M_variable = msymbol ("variable");

  Mdetail_text = msymbol_as_managing_key ("  detail-text");

  load_im_info_keys = mplist ();
  plist = mplist_add (load_im_info_keys, Mmap, Mnil);
  plist = mplist_add (plist, Mstate, Mnil);
  plist = mplist_add (plist, Mmacro, Mnil);
  plist = mplist_add (plist, Mmodule, Mnil);

  buf[0] = 'C';
  buf[1] = '-';
  buf[3] = '\0';
  for (i = 0, buf[2] = '@'; i < ' '; i++, buf[2]++)
    {
      one_char_symbol[i] = msymbol (buf);
      if (key_names[i])
	msymbol_put (one_char_symbol[i], M_key_alias,  msymbol (key_names[i]));
    }
  for (buf[2] = i; i < 127; i++, buf[2]++)
    one_char_symbol[i] = msymbol (buf + 2);
  one_char_symbol[i++] = msymbol ("Delete");
  buf[2] = 'M';
  buf[3] = '-';
  buf[5] = '\0';
  buf2[0] = 'M';
  buf2[1] = '-';
  for (buf[4] = '@'; i < 160; i++, buf[4]++)
    {
      one_char_symbol[i] = msymbol (buf);
      if (key_names[i - 128])
	{
	  strcpy (buf2 + 2, key_names[i - 128]);
	  msymbol_put (one_char_symbol[i], M_key_alias,  msymbol (buf2));
	}
    }
  for (buf[4] = i - 128; i < 255; i++, buf[4]++)
    one_char_symbol[i] = msymbol (buf + 2);
  one_char_symbol[i] = msymbol ("M-Delete");

  command_list = variable_list = NULL;

  minput_default_driver.open_im = open_im;
  minput_default_driver.close_im = close_im;
  minput_default_driver.create_ic = create_ic;
  minput_default_driver.destroy_ic = destroy_ic;
  minput_default_driver.filter = filter;
  minput_default_driver.lookup = lookup;
  minput_default_driver.callback_list = mplist ();
  mplist_put (minput_default_driver.callback_list, Minput_reset,
	      (void *) reset_ic);
  minput_driver = &minput_default_driver;
  return 0;
}

void
minput__fini ()
{
  MPlist *par_lang, *par_im, *p;

  if (command_list)
    {
      MPLIST_DO (par_lang, command_list)
	{
	  MPLIST_DO (par_im, MPLIST_VAL (par_lang))
	    {
	      MPLIST_DO (p, MPLIST_VAL (par_im))
		M17N_OBJECT_UNREF (MPLIST_VAL (p));
	      M17N_OBJECT_UNREF (MPLIST_VAL (par_im));
	    }
	  M17N_OBJECT_UNREF (MPLIST_VAL (par_lang));
	}
      M17N_OBJECT_UNREF (command_list);
      command_list = NULL;
    }
  if (variable_list)
    {
      MPLIST_DO (par_lang, variable_list)
	{
	  MPLIST_DO (par_im, MPLIST_VAL (par_lang))
	    {
	      MPLIST_DO (p, MPLIST_VAL (par_im))
		M17N_OBJECT_UNREF (MPLIST_VAL (p));
	      M17N_OBJECT_UNREF (MPLIST_VAL (par_im));
	    }
	  M17N_OBJECT_UNREF (MPLIST_VAL (par_lang));
	}
      M17N_OBJECT_UNREF (variable_list);
      variable_list = NULL;
    }

  if (minput_default_driver.callback_list)
    {
      M17N_OBJECT_UNREF (minput_default_driver.callback_list);
      minput_default_driver.callback_list = NULL;
    }
  if (minput_driver->callback_list)
    {
      M17N_OBJECT_UNREF (minput_driver->callback_list);
      minput_driver->callback_list = NULL;
    }

  M17N_OBJECT_UNREF (load_im_info_keys);
}

void
minput__callback (MInputContext *ic, MSymbol command)
{
  if (ic->im->driver.callback_list)
    {
      MInputCallbackFunc func
	= (MInputCallbackFunc) mplist_get (ic->im->driver.callback_list,
					   command);

      if (func)
	(func) (ic, command);
    }
}

MSymbol
minput__char_to_key (int c)
{
  if (c < 0 || c >= 0x100)
    return Mnil;

  return one_char_symbol[c];
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

/*** @addtogroup m17nInputMethod */
/*** @{ */
/*=*/

/***en
    @name Variables: Predefined symbols for callback commands.

    These are the predefined symbols that are used as the @c COMMAND
    argument of callback functions of an input method driver (see
    #MInputDriver::callback_list ).  */ 
/***ja
    @name 変数： コールバックコマンド用定義済みシンボル.

    入力メソッドドライバのコールバック関数において @c COMMAND 引数とし
    て用いられる定義済みシンボル (#MInputDriver::callback_list 参照)。
      */ 
/*** @{ */ 
/*=*/

MSymbol Minput_preedit_start;
MSymbol Minput_preedit_done;
MSymbol Minput_preedit_draw;
MSymbol Minput_status_start;
MSymbol Minput_status_done;
MSymbol Minput_status_draw;
MSymbol Minput_candidates_start;
MSymbol Minput_candidates_done;
MSymbol Minput_candidates_draw;
MSymbol Minput_set_spot;
MSymbol Minput_toggle;
MSymbol Minput_reset;
/*** @} */
/*=*/

/***en
    @brief The default driver for internal input methods.

    The variable #minput_default_driver is the default driver for
    internal input methods.

    The member MInputDriver::open_im () searches the m17n database for
    an input method that matches the tag \< #Minput_method, $LANGUAGE,
    $NAME\> and loads it.

    The member MInputDriver::callback_list () is @c NULL.  Thus, it is
    programmers responsibility to set it to a plist of proper callback
    functions.  Otherwise, no feedback information (e.g. preedit text)
    can be shown to users.

    The macro M17N_INIT () sets the variable #minput_driver to the
    pointer to this driver so that all internal input methods use it.

    Therefore, unless @c minput_driver is set differently, the driver
    dependent arguments $ARG of the functions whose name begin with
    "minput_" are all ignored.  */

/***ja
    @brief 内部入力メソッド用デフォルトドライバ.

    入力ドライバ #minput_default_driver は内部入力メソッド用のデフォル
    トのドライバである。

    メンバ MInputDriver::open_im () は m17n データベース中からタグ 
    \< #Minput_method, $LANGUAGE, $NAME\> に合致する入力メソッドを探し、
    それをロードする。

    メンバ MInputDriver::callback_list () は @c NULL なので、プログラ
    マ側で責任を持って, 適切なコールバック関数の plist に設定しなくて
    はならない。さもないと、preedit テキストなどのフィードバック情報が
    ユーザに表示されない。

    マクロ M17N_INIT () は変数 #minput_driver をこのドライバへのポイン
    タに設定し、全ての内部入力メソッドがこのドライバを使うようにする。

    したがって、@c minput_driver がデフォルト値のままであれば、minput_ 
    で始まる関数のドライバに依存する引数 $ARG はすべて無視される。  */

MInputDriver minput_default_driver;
/*=*/

/***en
    @brief The driver for internal input methods.

    The variable #minput_driver is a pointer to the input method
    driver that is used by internal input methods.  The macro
    M17N_INIT () initializes it to a pointer to #minput_default_driver
    (if <m17n<EM></EM>.h> is included) or to #minput_gui_driver (if
    <m17n-gui<EM></EM>.h> is included).  */ 
/***ja
    @brief 内部入力メソッド用ドライバ.

    変数 #minput_driver は内部入力メソッドによって使用されている入力メ
    ソッドドライバへのポインタである。マクロ M17N_INIT () はこのポイン
    タを #minput_default_driver (<m17n<EM></EM>.h> が含まれる時) または
    #minput_gui_driver ( <m17n-gui<EM></EM>.h> が含まれる時) に初期化す
    る。  */ 

MInputDriver *minput_driver;

MSymbol Minput_driver;

/*=*/

/***en
    @brief Open an input method.

    The minput_open_im () function opens an input method that matches
    language $LANGUAGE and name $NAME, and returns a pointer to the
    input method object newly allocated.

    This function at first decides an driver for the input method as
    below.

    If $LANGUAGE is not #Mnil, the driver pointed by the variable
    #minput_driver is used.

    If $LANGUAGE is #Mnil and $NAME has #Minput_driver property, the
    driver pointed to by the property value is used to open the input
    method.  If $NAME has no such property, @c NULL is returned.

    Then, the member MInputDriver::open_im () of the driver is
    called.  

    $ARG is set in the member @c arg of the structure MInputMethod so
    that the driver can refer to it.  */

/***ja
    @brief 入力メソッドをオープンする.

    関数 minput_open_im () は言語 $LANGUAGE と名前 $NAME に合致する入
    力メソッドをオープンし、新たに割り当てられた入力メソッドオブジェク
    トへのポインタを返す。
    
    この関数は、まず入力メソッド用のドライバを以下のようにして決定する。

    $LANGUAGE が #Mnil でなければ、変数 #minput_driver で指されている
    ドライバを用いる。

    $LANGUAGE が #Mnil であり、$NAME が #Minput_driver プロパティを持
    つ場合には、そのプロパティの値で指されている入力ドライバを用いて入
    力メソッドをオープンする。$NAME にそのようなプロパティが無かった場
    合は @c NULL を返す。

    次いで、ドライバのメンバ MInputDriver::open_im () が呼ばれる。

    $ARG は、ドライバが参照できるように、構造体 MInputMethod のメンバ 
    @c arg に設定される。

    @latexonly \IPAlabel{minput_open} @endlatexonly

*/

MInputMethod *
minput_open_im (MSymbol language, MSymbol name, void *arg)
{
  MInputMethod *im;
  MInputDriver *driver;

  if (language)
    driver = minput_driver;
  else
    {
      driver = (MInputDriver *) msymbol_get (name, Minput_driver);
      if (! driver)
	MERROR (MERROR_IM, NULL);
    }

  MSTRUCT_CALLOC (im, MERROR_IM);
  im->language = language;
  im->name = name;
  im->arg = arg;
  im->driver = *driver;
  if ((*im->driver.open_im) (im) < 0)
    {
      free (im);
      return NULL;
    }
  return im;
}

/*=*/

/***en
    @brief Close an input method.

    The minput_close_im () function closes the input method $IM, which
    must have been created by minput_open_im ().  */

/***ja
    @brief 入力メソッドをクローズする.

    関数 minput_close_im () は、入力メソッド $IM をクローズする。この
    入力メソッド $IM は minput_open_im () によって作られたものでなけれ
    ばならない。  */

void
minput_close_im (MInputMethod *im)
{
  (*im->driver.close_im) (im);
  free (im);
}

/*=*/

/***en
    @brief Create an input context.

    The minput_create_ic () function creates an input context object
    associated with input method $IM, and calls callback functions
    corresponding to #Minput_preedit_start, #Minput_status_start, and
    #Minput_status_draw in this order.

    @return

    If an input context is successfully created, minput_create_ic ()
    returns a pointer to it.  Otherwise it returns @c NULL.  */

/***ja
    @brief 入力コンテクストを生成する.

    関数 minput_create_ic () は入力メソッド $IM に対応する入力コンテク
    ストオブジェクトを生成し、 #Minput_preedit_start,
    #Minput_status_start, #Minput_status_draw に対応するコールバック関
    数をこの順に呼ぶ。

    @return

    入力コンテクストが生成された場合、minput_create_ic () はその入力コ
    ンテクストへのポインタを返す。失敗した場合は @c NULL を返す。
      */

MInputContext *
minput_create_ic (MInputMethod *im, void *arg)
{
  MInputContext *ic;

  MSTRUCT_CALLOC (ic, MERROR_IM);
  ic->im = im;
  ic->arg = arg;
  ic->preedit = mtext ();
  ic->candidate_list = NULL;
  ic->produced = mtext ();
  ic->spot.x = ic->spot.y = 0;
  ic->active = 1;
  ic->plist = mplist ();
  if ((*im->driver.create_ic) (ic) < 0)
    {
      M17N_OBJECT_UNREF (ic->preedit);
      M17N_OBJECT_UNREF (ic->produced);
      M17N_OBJECT_UNREF (ic->plist);
      free (ic);
      return NULL;
    };

  if (im->driver.callback_list)
    {
      minput__callback (ic, Minput_preedit_start);
      minput__callback (ic, Minput_status_start);
      minput__callback (ic, Minput_status_draw);
    }

  return ic;
}

/*=*/

/***en
    @brief Destroy an input context.

    The minput_destroy_ic () function destroys the input context $IC,
    which must have been created by minput_create_ic ().  It calls
    callback functions corresponding to #Minput_preedit_done,
    #Minput_status_done, and #Minput_candidates_done in this order.  */

/***ja
    @brief 入力コンテクストを破壊する.

    関数 minput_destroy_ic () は、入力コンテクスト $IC を破壊する。こ
    の入力コンテクストは minput_create_ic () によって作られたものでな
    ければならない。この関数は#Minput_preedit_done,
    #Minput_status_done, #Minput_candidates_done に対応するコールバッ
    ク関数をこの順に呼ぶ。
  */

void
minput_destroy_ic (MInputContext *ic)
{
  if (ic->im->driver.callback_list)
    {
      minput__callback (ic, Minput_preedit_done);
      minput__callback (ic, Minput_status_done);
      minput__callback (ic, Minput_candidates_done);
    }
  (*ic->im->driver.destroy_ic) (ic);
  M17N_OBJECT_UNREF (ic->preedit);
  M17N_OBJECT_UNREF (ic->produced);
  M17N_OBJECT_UNREF (ic->plist);
  free (ic);
}

/*=*/

/***en
    @brief Filter an input key.

    The minput_filter () function filters input key $KEY according to
    input context $IC, and calls callback functions corresponding to
    #Minput_preedit_draw, #Minput_status_draw, and
    #Minput_candidates_draw if the preedit text, the status, and the
    current candidate are changed respectively.

    @return
    If $KEY is filtered out, this function returns 1.  In that case,
    the caller should discard the key.  Otherwise, it returns 0, and
    the caller should handle the key, for instance, by calling the
    function minput_lookup () with the same key.  */

/***ja
    @brief 入力キーをフィルタする.

    関数 minput_filter () は入力キー $KEY を入力コンテクスト $IC に応
    じてフィルタし、preedit テキスト、ステータス、現時点での候補が変化
    した際にはそれぞれ #Minput_preedit_draw, #Minput_status_draw,
    #Minput_candidates_draw に対応するコールバック関数を呼ぶ。

    @return 
    $KEY がフィルタされれば、この関数は 1 を返す。この場合呼び
    出し側はこのキーを捨てるべきである。そうでなければ 0 を返し、呼び
    出し側は、たとえば同じキーで関数 minput_lookup () を呼ぶなどして、
    このキーを処理する。

    @latexonly \IPAlabel{minput_filter} @endlatexonly
*/

int
minput_filter (MInputContext *ic, MSymbol key, void *arg)
{
  int ret;

  if (! ic
      || ! ic->active)
    return 0;
  ret = (*ic->im->driver.filter) (ic, key, arg);

  if (ic->im->driver.callback_list)
    {
      if (ic->preedit_changed)
	minput__callback (ic, Minput_preedit_draw);
      if (ic->status_changed)
	minput__callback (ic, Minput_status_draw);
      if (ic->candidates_changed)
	minput__callback (ic, Minput_candidates_draw);
    }

  return ret;
}

/*=*/

/***en
    @brief Lookup a text produced in the input context.

    The minput_lookup () function looks up a text in the input context
    $IC.  $KEY must be the same one provided to the previous call of
    minput_filter ().

    If a text was produced by the input method, it is concatenated
    to M-text $MT.

    This function calls #MInputDriver::lookup .

    @return
    If $KEY was correctly handled by the input method, this function
    returns 0.  Otherwise, returns -1, even in that case, some text
    may be produced in $MT.  */

/***ja
    @brief 入力コンテクスト中のテキストの検索.

    関数 minput_lookup () は入力コンテクスト $IC 中のテキストを検索す
    る。$KEY は関数 minput_filter () への直前の呼び出しに用いられたも
    のと同じでなくてはならない。

    テキストが入力メソッドによって生成されていれば、テキストは M-text
    $MT に連結される。

    この関数は、#MInputDriver::lookup を呼ぶ。

    @return 
    $KEY が入力メソッドによって適切に処理できれば、この関数は 0 を返す。
    そうでなければ -1 を返す。この場合でも $MT に何らかのテキストが生
    成されていることがある。

    @latexonly \IPAlabel{minput_lookup} @endlatexonly  */

int
minput_lookup (MInputContext *ic, MSymbol key, void *arg, MText *mt)
{
  return (ic ? (*ic->im->driver.lookup) (ic, key, arg, mt) : -1);
}
/*=*/

/***en
    @brief Set the spot of the input context.

    The minput_set_spot () function set the spot of input context $IC
    to coordinate ($X, $Y ) with the height specified by $ASCENT and $DESCENT .
    The semantics of these values depend on the input method driver.
    $FONTSIZE specfies the fontsize of preedit text in 1/10 point.

    For instance, a driver designed to work in a CUI environment may
    use $X and $Y as column and row numbers, and ignore $ASCENT and
    $DESCENT .  A driver designed to work in a window system may
    interpret $X and $Y as pixel offsets relative to the origin of the
    client window, and may interpret $ASCENT and $DESCENT as the ascent- and
    descent pixels of the line at ($X . $Y ).

    $MT and $POS is the M-text and the character position at the spot.
    $MT may be @c NULL, in which case, the input method cannot get
    information about the text around the spot.  */

/***ja
    @brief 入力コンテクストのスポットを設定する.

    関数 minput_set_spot () は、入力コンテクスト $IC のスポットを、座
    標 ($X, $Y )に 、高さ $ASCENT、 $DESCENT で設定する。 これらの値の
    意味は入力メソッドドライバに依存する。$FONTSIZE はpreedit テキスト
    のフォントサイズを 1/10 ポイント単位で指定する。

    たとえば CUI 環境で動作するドライバは $X と $Y をそれぞれ列と行の
    番号として用い、$ASCENT と $DESCENT を無視するかもしれない。 また
    ウィンドウシステム用のドライバは $X と $Y をクライアントウィンドウ
    の原点からのオフセットをピクセル単位で表したものとして扱い、
    $ASCENT と $DESCENT を ($X . $Y ) の列のアセントとディセントをピク
    セル単位で表したものとして扱うかもしれない。

    $MT と $POS はそのスポットの M-text と文字位置である。$MT は @c
    NULL でもよく、その場合には入力メソッドはスポット周辺のテキストに
    関する情報を得ることができない。
    */

void
minput_set_spot (MInputContext *ic, int x, int y,
		 int ascent, int descent, int fontsize,
		 MText *mt, int pos)
{
  ic->spot.x = x;
  ic->spot.y = y;
  ic->spot.ascent = ascent;
  ic->spot.descent = descent;
  ic->spot.fontsize = fontsize;
  ic->spot.mt = mt;
  ic->spot.pos = pos;
  if (ic->im->driver.callback_list)
    minput__callback (ic, Minput_set_spot);
}
/*=*/

/***en
    @brief Toggle input method.

    The minput_toggle () function toggles the input method associated
    with input context $IC.  */
/***ja
    @brief 入力メソッドを切替える.

    関数 minput_toggle () は入力コンテクスト $IC に対応付けられた入力
    メソッドをトグルする。
    */

void
minput_toggle (MInputContext *ic)
{
  if (ic->im->driver.callback_list)
    minput__callback (ic, Minput_toggle);
  ic->active = ! ic->active;
}

/***en
    @brief Reset an input context.

    The minput_reset_ic () function resets input context $IC by
    calling a callback function corresponding to #Minput_reset.  It
    actually shifts the state to the initial one, and thus the current
    preediting text (if any) is committed.  If necessary, a program
    can extract that committed text by calling minput_lookup () just
    after the call of minput_reset_ic ().  In that case, the arguments
    @c KEY and @c ARG of minput_lookup () are ignored.  */
/***ja
    @brief 入力コンテクストをリセットする.

    関数 minput_reset_ic () は #Minput_reset に対応するコールバック関
    数を呼ぶことによって入力コンテクスト $IC をリセットする。実
    際には、入力メソッドを初期状態に移すことであり、したがって、もし現在入力中のテキ
    ストがあれば、それはコミットされる。必要ならば、アプリケーションプ
    ログラムは minput_lookup () を読んでそのコミットされたテキストを取
    り出すことができ、その際、minput_lookup () の引数 @c KEY と @c ARG 
    は無視される。 */
void
minput_reset_ic (MInputContext *ic)
{
  if (ic->im->driver.callback_list)
    minput__callback (ic, Minput_reset);
}

/*=*/
/***en
    @brief Key of a text property for detailed description.

    The symbol #Mdetail_text is a managing key usually used for a
    text property whose value is an M-text that contains detailed
    description.  */
/***ja
    @brief 詳細説明用テキストプロパティのキー.

    シンボル #Mdetail_text は管理キーであり、通常詳細な説明を含む 
    M-text を値として持つテキストプロパティに用いられる。
    */
MSymbol Mdetail_text;

/***en
    @brief Get description text of an input method.

    The minput_get_description () function returns an M-text that
    briefly describs the input method specified by $LANGUAGE and
    $NAME.  The returned M-text may have a text property, from its
    beginning to end, #Mdetail_text whose value is an M-text
    describing the input method in more detail.

    @return
    If the specified input method has a description text, a pointer to
    #MText is returned.  A caller have to free it by m17n_object_unref ().
    If the input method does not have a description text, @c NULL is
    returned.  */
/***ja
    @brief 入力メソッドの説明テキストを得る.

    関数 minput_get_description () は、$LANGUAGE と $NAME によって指定
    された入力メソッドを簡単に説明する M-text を返す。返される M-text 
    には、その全体に対して #Mdetail_text というテキストプロパティが付
    加されている場合があり、そのプロパティの値は入力メソッドをさらに詳
    細に説明する M-text である。

    @return 指定された入力メソッドが説明するテキストを持っていれば、 
    #MText へのポインタを返す。呼び出し側は、それを m17n_object_unref
    () を用いて解放しなくてはならない。入力メソッドに説明テキストが無
    ければ@c NULL を返す。 */

MText *
minput_get_description (MSymbol language, MSymbol name)
{
  MPlist *plist = load_im_info (language, name, M_description);
  MPlist *pl;
  MText *mt = NULL;

  if (! plist)
    return NULL;
  if (! MPLIST_PLIST_P (plist))
    {
      M17N_OBJECT_UNREF (plist);      
      return NULL;
    }
  pl = MPLIST_PLIST (plist);
  while (! MPLIST_TAIL_P (pl) && ! MPLIST_MTEXT_P (pl))
    pl = MPLIST_NEXT (pl);
  if (MPLIST_MTEXT_P (pl))
    mt = get_description_advance (pl);
  M17N_OBJECT_UNREF (plist);
  return mt;
}

/***en
    @brief Get information about input method commands.

    The minput_get_commands () function returns information about
    input method commands of the input method specified by $LANGUAGE
    and $NAME.  An input method command is a pseudo key event to which
    one or more actual input key sequences are assigned.

    There are two kinds of commands, global and local.  Global
    commands are used by multiple input methods for the same purpose,
    and have global key assignments.  Local commands are used only in
    a specific input method, and have only local key assignments.

    Each input method may locally change key assignments for glabal
    commands.  A global key assignment for a global command are
    effective only when the current input method does not have local
    key assignments for that command.

    If $NAME is #Mnil, information about global commands is returned.
    In this case $LANGUAGE is ignored.

    If $NAME is not #Mnil, information about those commands that have
    local key assignments in the input method specified by $LANGUAGE
    and $NAME is returned.

    @return
    If no input method commands are found, this function returns @c NULL.

    Otherwise, a pointer to a plist is returned.  The key of each
    element in the plist is a symbol representing a command, and the
    value is a plist of the form COMMAND-INFO described below.

    The first element of COMMAND-INFO has the key #Mtext, and the
    value is an M-text describing the command briefly.  This M-text
    may have a text property whose key is #Mdetail_text and whose
    value is an M-text describing the command in more detail.

    If there are no more elements, that means no key sequences are
    assigned to the command.  Otherwise, each of the remaining
    elements has the key #Mplist, and the value is a plist whose keys are
    #Msymbol and values are symbols representing input keys, which are
    currently assigned to the command.

    As the returned plist is kept in the library, the caller must not
    modify nor free it.  */
/***ja
    @brief 入力メソッドのコマンドに関する情報を得る.

    関数 minput_get_commands () は、 $LANGUAGE と $NAME によって指定さ
    れた入力メソッドの入力メソッドコマンドに関する情報を返す。入力メソッ
    ドコマンドとは、疑似キーイベントであり、それぞれに１つ以上の実際の
    入力キーシークエンスが割り当てられているものを指す。

    コマンドにはグローバルとローカルの２種類がある。グローバルコマンド
    は複数の入力メソッドにおいて、同じ目的で、グローバルなキー割り当て
    で用いられる。ローカルコマンドは特定の入力メソッドでのみ、ローカル
    なキー割当で使用される。

    個々の入力メソッドはグローバルコマンドのキー割当を変更することもで
    きる。グローバルコマンド用のグローバルキー割り当ては、使用する入力
    メソッドにおいてそのコマンド用のローカルなキー割当が存在しない場合
    にのみ有効である。

    $NAME が #Mnil であれば、グローバルコマンドに関する情報を返す。こ
    の場合、$LANGUAGE は無視される。

    $NAME が #Mnil でなければ、$LANGUAGE と $NAME によって指定される入
    力メソッドに置けるローカルなキー割り当てを持つコマンドに関する情報
    を返す。

    @return
    入力メソッドコマンドが見つからなければ、この関数は @c NULL を返す。

    そうでなければプロパティリストへのポインタを返す。リストの各要素の
    キーは個々のコマンドを示すシンボルであり、値は下記の COMMAND-INFO 
    の形式のプロパティリストである。

    COMMAND-INFO の第一要素はキーとして #Mtext を、値としてそのコマン
    ドを簡単に説明する M-text を持つ。この M-text は、#Mdetail_text を
    キーとするテキストプロパティを持つことができ、その値はそのコマンド
    をより詳細に説明する M-text である。

    それ以外の要素が無ければ、このコマンドに対してキーシークエンスが割
    り当てられていないことを意味する。そうでなければ、残りの各要素はキー
    として#Mplist を、値としてプロパティリストを持つ。このプロパティリ
    ストのキーは #Msymbol であり、値は現在そのコマンドに割り当てられて
    いる入力キーを表すシンボルである。

    返されるプロパティリストはライブラリによって管理されており、呼び出
    し側で変更したり解放したりしてはならない。*/

MPlist *
minput_get_commands (MSymbol language, MSymbol name)
{
  MPlist *plist = get_command_list (language, name);

  return (! plist || MPLIST_TAIL_P (plist) ? NULL : plist);
}

/***en
    @brief Assign a key sequence to an input method command.

    The minput_assign_command_keys () function assigns input key
    sequence $KEYSEQ to input method command $COMMAND for the input
    method specified by $LANGUAGE and $NAME.  If $NAME is #Mnil, the
    key sequence is assigned globally no matter what $LANGUAGE is.
    Otherwise the key sequence is assigned locally.

    Each element of $KEYSEQ must have the key $Msymbol and the value
    must be a symbol representing an input key.

    $KEYSEQ may be @c NULL, in which case, all assignments are deleted
    globally or locally.

    This assignment gets effective in a newly opened input method.

    @return
    If the operation was successful, 0 is returned.  Otherwise -1 is
    returned, and #merror_code is set to #MERROR_IM.  */
/***ja
    @brief 入力メソッドコマンドにキーシークエンスを割り当てる.

    関数 minput_assign_command_keys () は、 $LANGUAGE と $NAME によっ
    て指定された入力メソッド用の入力メソッドコマンド $COMMAND に対して、
    入力キーシークエンス $KEYSEQ を割り当てる。 $NAME が #Mnil ならば、
    $LANGUAGE に関係なく、入力キーシークエンスはグローバルに割り当てら
    れる。そうでなれば、割り当てはローカルである。

    $KEYSEQ の各要素はキーとして $Msymbol を、値として入力キーを表すシ
    ンボルを持たなくてはならない。

    $KEYSEQ は @c NULL でもよい。この場合、グローバルもしくはローカル
    なすべての割り当てが消去される。

    この割り当ては、割り当て以降新しくオープンされた入力メソッドから有
    効になる。

    @return 
    処理が成功すれば 0 を返す。そうでなければ -1 を返し、
    #merror_code を #MERROR_IM に設定する。  */

int
minput_assign_command_keys (MSymbol language, MSymbol name,
			    MSymbol command, MPlist *keyseq)
{
  MPlist *plist, *pl, *p;

  if (check_command_keyseq (keyseq) < 0
      || ! (plist = get_command_list (language, name)))
    MERROR (MERROR_IM, -1);
  pl = mplist_get (plist, command);
  if (pl)
    {
      pl = MPLIST_NEXT (pl);
      if (! keyseq)
	while ((p = mplist_pop (pl)))
	  M17N_OBJECT_UNREF (p);
      else
	{
	  keyseq = mplist_copy (keyseq);
	  mplist_push (pl, Mplist, keyseq);
	  M17N_OBJECT_UNREF (keyseq);
	}
    }
  else
    {
      if (name == Mnil)
	MERROR (MERROR_IM, -1);
      if (! keyseq)
	return 0;
      pl = get_command_list (Mnil, Mnil); /* Get global commands.  */
      pl = mplist_get (pl, command);
      if (! pl)
	MERROR (MERROR_IM, -1);
      p = mplist ();
      mplist_add (p, Mtext, mplist_value (pl));
      keyseq = mplist_copy (keyseq);
      mplist_add (p, Mplist, keyseq);
      M17N_OBJECT_UNREF (keyseq);
      mplist_push (plist, command, p);
    }
  return 0;
}

/***en
    @brief Get a list of variables of an input method.

    The minput_get_variables () function returns a plist (#MPlist) of
    variables used to control the behavior of the input method
    specified by $LANGUAGE and $NAME.  The key of an element of the
    plist is a symbol representing a variable, and the value is a
    plist of the form VAR-INFO (described below) that carries the
    information about the variable.

    The first element of VAR-INFO has the key #Mtext, and the value is
    an M-text describing the variable briefly.  This M-text may have a
    text property #Mdetail_text whose value is an M-text describing
    the variable in more detail.

    The second element of VAR-INFO is for the value of the variable.
    The key is #Minteger, #Msymbol, or #Mtext, and the value is an
    intetger, a symbol, or an M-text, respectively.  The variable is
    set to this value when an input context is created for the input
    method.

    If there are no more elements, the variable can take any value
    that matches with the above type.  Otherwise, the remaining
    elements of VAR-INFO are to specify valid values of the variable.

    If the type of the variable is integer, the following elements
    have the key #Minteger or #Mplist.  If it is #Minteger, the value
    is a valid integer value.  If it is #Mplist, the value is a plist
    of two of elements.  Both of them have the key #Minteger, and
    values are the minimum and maximum bounds of the valid value
    range.

    If the type of the variable is symbol or M-text, the following
    elements of the plist have the key #Msymbol or #Mtext,
    respectively, and the value must be a valid one.

    For instance, suppose an input method has the variables:

    <li> name:intvar, description:"value is an integer",
         initial value:0, value-range:0..3,10,20

    <li> name:symvar, description:"value is a symbol",
         initial value:nil, value-range:a, b, c, nil

    <li> name:txtvar, description:"value is an M-text",
         initial value:empty text, no value-range (i.e. any text)

    Then, the returned plist has this form ('X:Y' means X is a key and Y is
    a value, and '(...)' means a plist):

@verbatim
    plist:(intvar:(mtext:"value is an integer"
                   integer:0
		   plist:(integer:0 integer:3)
                   integer:10
                   integer:20))
           symvar:(mtext:"value is a symbol"
                   symbol:nil
                   symbol:a
                   symbol:b
                   symbol:c
                   symbol:nil))
           txtvar:(mtext:"value is an M-text"
                   mtext:""))
@endverbatim

    @return
    If the input method uses any variables, a pointer to #MPlist is
    returned.  As the plist is kept in the library, a caller must not
    modify nor free it.  If the input method does not use any
    variable, @c NULL is returned.  */
/***ja
    @brief 入力メソッドの変数リストを得る.

    関数 minput_get_variables () は、$LANGUAGE と $NAME によって指定さ
    れた入力メソッドの振る舞いを制御する変数のプロパティリスト 
    (#MPlist) を返す。リストの各要素のキーは変数を表すシンボルである。
    各要素の値は下記の VAR-INFO の形式のプロパティリストであり、各変数
    に関する情報を示している。

    VAR-INFO の第一要素はキーとして #Mtext を、値としてその変数
    を簡単に説明する M-text を持つ。この M-text は、#Mdetail_text を
    キーとするテキストプロパティを持つことができ、その値はその変数をよ
    り詳細に説明する M-text である。

    VAR-INFO の第二要素は変数の値を示す。キーは #Minteger, #Msymbol,
    #Mtext のいずれかであり、値はそれぞれ整数値、シンボル、M-text であ
    る。この入力メソッド用の入力コンテストが作られる際には、変数はこの
    値に設定される。

    VAR-INFO にそれ以外の要素が無ければ、変数は上記の型に合致する限り
    どのような値をとることもできる。そうでなければ、VAR-INFO の残りの
    要素によって変数の有効な値が指定される。

    変数の型が整数であれば、それ以降の要素は #Minteger か #Mplist をキー
    として持つ。 #Minteger であれば、値は有効な値を示す整数値である。
    #Mplist であれば、値は二つの要素を持つプロパティリストであり、各要
    素はキーとして #Minteger を、値としてそれぞれ有効な値の上限値と下
    限値をとる。

    変数の型がシンボルか M-text であれば、それ以降の要素はキーとしてそ
    れぞれ #Msymbol か #Mtext を持ち、値として型に従うものをとる。

    例として、ある入力メソッドが次のような変数を持つ場合を考えよう。

    <li> name:intvar, 説明:"value is an integer",
         初期値:0, 値の範囲:0..3,10,20

    <li> name:symvar, 説明:"value is a symbol",
         初期値:nil, 値の範囲:a, b, c, nil

    <li> name:txtvar, 説明:"value is an M-text",
         初期値:empty text, 値の範囲なし(どんな M-text でも可)

    この場合、返されるリストは以下のようになる。（'X:Y' という記法は X 
    がキーで Y が値であることを、また '(...)' はプロパティリストを示す。）

@verbatim
    plist:(intvar:(mtext:"value is an integer"
                   integer:0
		   plist:(integer:0 integer:3)
                   integer:10
                   integer:20))
           symvar:(mtext:"value is a symbol"
                   symbol:nil
                   symbol:a
                   symbol:b
                   symbol:c
                   symbol:nil))
           txtvar:(mtext:"value is an M-text"
                   mtext:""))
@endverbatim

    @return 
    入力メソッドが何らかの変数を使用していれば #MPlist への変数を返す。
    返されるプロパティリストはライブラリによって管理されており、呼
    び出し側で変更したり解放したりしてはならない。入力メソッドが変
    数を一切使用してなければ、@c NULL を返す。  */

MPlist *
minput_get_variables (MSymbol language, MSymbol name)
{
  MPlist *plist = get_variable_list (language, name);

  return (! plist || MPLIST_TAIL_P (plist) ? NULL : plist);
}

/***en
    @brief Set the initial value of an input method variable.

    The minput_set_variable () function sets the initial value of
    input method variable $VARIABLE to $VALUE for the input method
    specified by $LANGUAGE and $NAME.

    By default, the initial value is 0.

    This setting gets effective in a newly opened input method.

    @return
    If the operation was successful, 0 is returned.  Otherwise -1 is
    returned, and #merror_code is set to #MERROR_IM.  */
/***ja
    @brief 入力メソッド変数の初期値を設定する.

    関数 minput_set_variable () は、$LANGUAGE と $NAME によって指定さ
    れた入力メソッドの入力メソッド変数 $VARIABLE の初期値を、 $VALUE に
    設定する。

    デフォルトの初期値は 0 である。

    この設定は、新しくオープンされた入力メソッドから有効となる。

    @return
    処理が成功すれば 0 を返す。そうでなければ -1 を返し、
    #merror_code を #MERROR_IM に設定する。  */

int
minput_set_variable (MSymbol language, MSymbol name,
		     MSymbol variable, void *value)
{
  MPlist *plist, *val_element, *range_element;
  MSymbol type;

  if (language == Mnil || name == Mnil)
    MERROR (MERROR_IM, -1);
  plist = get_variable_list (language, name);
  if (! plist)
    MERROR (MERROR_IM, -1);
  plist = (MPlist *) mplist_get (plist, variable);
  if (! plist)
    MERROR (MERROR_IM, -1);
  val_element = MPLIST_NEXT (plist);
  type = MPLIST_KEY (val_element);
  range_element = MPLIST_NEXT (val_element);
    
  if (! MPLIST_TAIL_P (range_element))
    {
      if (type == Minteger)
	{
	  int val = (int) value, this_val;
      
	  MPLIST_DO (plist, range_element)
	    {
	      this_val = (int) MPLIST_VAL (plist);
	      if (MPLIST_PLIST_P (plist))
		{
		  int min_bound, max_bound;
		  MPlist *pl = MPLIST_PLIST (plist);

		  min_bound = (int) MPLIST_VAL (pl);
		  pl = MPLIST_NEXT (pl);
		  max_bound = (int) MPLIST_VAL (pl);
		  if (val >= min_bound && val <= max_bound)
		    break;
		}
	      else if (val == this_val)
		break;
	    }
	  if (MPLIST_TAIL_P (plist))
	    MERROR (MERROR_IM, -1);
	}
      else if (type == Msymbol)
	{
	  MPLIST_DO (plist, range_element)
	    if (MPLIST_SYMBOL (plist) == (MSymbol) value)
	      break;
	  if (MPLIST_TAIL_P (plist))
	    MERROR (MERROR_IM, -1);
	}
      else			/* type == Mtext */
	{
	  MPLIST_DO (plist, range_element)
	    if (mtext_cmp (MPLIST_MTEXT (plist), (MText *) value) == 0)
	      break;
	  if (MPLIST_TAIL_P (plist))
	    MERROR (MERROR_IM, -1);
	  M17N_OBJECT_REF (value);
	}
    }

  mplist_set (val_element, type, value);
  return 0;
}

/*** @} */
/*=*/
/*** @addtogroup m17nDebug */
/*=*/
/*** @{  */
/*=*/

/***en
    @brief Dump an input method.

    The mdebug_dump_im () function prints the input method $IM in a
    human readable way to the stderr.  $INDENT specifies how many
    columns to indent the lines but the first one.

    @return
    This function returns $IM.  */
/***ja
    @brief 入力メソッドをダンプする.

    関数 mdebug_dump_im () は入力メソッド $IM を stderr に人間に可読な
    形で印刷する。$INDENT は２行目以降のインデントを指定する。

    @return
    この関数は $IM を返す。  */

MInputMethod *
mdebug_dump_im (MInputMethod *im, int indent)
{
  MInputMethodInfo *im_info = (MInputMethodInfo *) im->info;
  char *prefix;

  prefix = (char *) alloca (indent + 1);
  memset (prefix, 32, indent);
  prefix[indent] = '\0';

  fprintf (stderr, "(input-method %s %s ", msymbol_name (im->language),
	   msymbol_name (im->name));
  mdebug_dump_mtext (im_info->title, 0, 0);
  if (im->name != Mnil)
    {
      MPlist *state;

      MPLIST_DO (state, im_info->states)
	{
	  fprintf (stderr, "\n%s  ", prefix);
	  dump_im_state (MPLIST_VAL (state), indent + 2);
	}
    }
  fprintf (stderr, ")");
  return im;
}

/*** @} */ 

/*
  Local Variables:
  coding: euc-japan
  End:
*/
