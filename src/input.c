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
    @brief API for Input method

    An input method is an object to enable inputting various
    characters.  An input method is identified by a pair of symbols,
    LANGUAGE and NAME.  This pair decides an input driver of the input
    method.  An input driver is a set of functions for handling the
    input method.  There are two kinds of input methods; internal one
    and foreign one.

    <ul>

    <li> Internal Input Method

    An internal input method has non @c Mnil LANGUAGE, and the body is
    defined in the m17n database by the tag <Minput_method, LANGUAGE,
    NAME>.  For this kind of input methods, the m17n library uses two
    predefined input method drivers, one for CUI use and the other for
    GUI use.  Those driver utilize the input processing engine
    provided by the m17n library itself.  The m17n database may
    provides an input method that is not only for a specific language.
    The database uses @c Mt as the language of such an input method.

    An internal input method accepts an input key which is a symbol
    associated with an input event.  As there is no way for the @c
    m17n @c library to know how input events are represented in an
    application program, a application programmer have to convert an
    input event to an input key by himself.  See the documentation of
    the function minput_event_to_key () for the detail.

    <li> Foreign Input Method

    A foreign input method has @c Mnil LANGUAGE, and the body is
    defined in an external resources (e.g. XIM of X Window System).
    For this kind of input methods, the symbol NAME must have a
    property of key @c Minput_driver, and the value must be a pointer
    to an input driver.  So, by preparing a proper input 
    driver, any kind of input method can be treated in the framework
    of the @c m17n @c library.

    For convenience, the m17n-X library provides an input driver that
    enables the input style of OverTheSpot for XIM, and stores @c
    Minput_driver property of the symbol @c Mxim with a pointer to
    that driver.  See the documentation of m17n GUI API for the
    detail.

    </ul>

    PROCESSING FLOW

    The typical processing flow of handling an input method is: open
    an input method, create an input context for the input method,
    filter an input key, and looking up a produced text in the input
    context.  */

/*=*/

/***ja
    @addtogroup m17nInputMethod
    @brief 入力メソッド用API

    入力メソッドは多様な文字を入力するためのオブジェクトである。入力メ
    ソッドはシンボル LANGUAGE と NAME の組によって識別され、この組によっ
    て入力ドライバが決まる。入力ドライバとは指定の入力メソッドを扱うた
    めの関数の集まりである。

    入力メソッドには内部メソッドと外部メソッドの二通りがある。

    内部入力メソッド

    内部入力メソッドは LANGUAGE が @c Mnil 以外のものであり、本体は
    m17n 言語情報ベースに <Minput_method, LANGUAGE, NAME>というタグ付
    きで定義されている。この種の入力メソッドに対して、m17nライブラリに
    はCUI用とGUI用それぞれの入力ドライバがあらかじめ準備されている。こ
    れらのドライバはm17nライブラリ自身の入力処理エンジンを利用する。

    内部入力メソッドは、ユーザの入力イベントに対応した入力キーを受け取
    る。入力キーはシンボルである。@c m17n @c ライブラリ は入力イベント
    がアプリケーション・プログラムでどのように表現されているかを知る術
    を持たないので、入力イベントから入力キーへの変換はプログラマの責任
    で行わなくてはならない。詳細については関数 minput_event_to_key () 
    のドキュメントを参照のこと。

    外部入力メソッド

    外部入力メソッドは LANGUAGE が @c Mnil のものであり、で本体は外部
    のリソースとして定義される。（たとえばX Window System のXIM など。) 
    この種の入力メソッドではシンボル NAME は@c Minput_driver をキーと
    するプロパティを持ち、その値は入力ドライバへのポインタでなくてはな
    らない。したがって、適切な入力ドライバを準備することによって、いか
    なる入力メソッドも @c m17n @c ライブラリの枠組の中で扱う事ができる。

    簡単のため、m17n X ライブラリはXIM の OverTheSpot の入力スタイルを
    実現する入力ドライバを提供している。またシンボル @c Mxim の @c
    Minput_driver プロパティの値としてそのドライバへのポインタを保持し
    ている。詳細についてはm17n-win API のドキュメントを参照のこと。

    処理の流れ

    入力メソッド処理の典型的な処理は以下のようになる。入力メソッドのオー
    プン、その入力メソッドの入力コンテクストの生成、入力イベントのフィ
    ルタ、入力コンテクストでの生成テキストの検索。     */

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "config.h"
#include "m17n-gui.h"
#include "m17n-misc.h"
#include "internal.h"
#include "mtext.h"
#include "input.h"
#include "symbol.h"
#include "plist.h"

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
  int len = mtext_nbytes (preedit);

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
  else if (code == '<')
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

  if (! MPLIST_PLIST_P (plist))
    MERROR (MERROR_IM, -1);
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

  MPLIST_DO (elt, maps)
    M17N_OBJECT_UNREF (MPLIST_VAL (elt));
  M17N_OBJECT_UNREF (maps);
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
  MIMState *state = ic_info->state;

  /* Find a state to shift to.  If not found, shift to the initial
     state.  */
  state = (MIMState *) mplist_get (im_info->states, state_name);
  if (! state)
    state = (MIMState *) MPLIST_VAL (im_info->states);

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
    take_action_list (ic, ic_info->map->map_actions);
}


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
      if (i + len > index)
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
udpate_candidate (MInputContext *ic, MTextProperty *prop, int idx)
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
	      if (idx < 0
		  || (idx >= end
		      && MPLIST_TAIL_P (MPLIST_NEXT (group))))
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
	  udpate_candidate (ic, prop, idx);
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
	}
      else if (name == Mequal || name == Mless || name == Mgreater)
	{
	  int val1, val2;
	  MPlist *actions1, *actions2;
	  int ret;

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

  if (map->submaps)
    {
      submap = mplist_get (map->submaps, key);
      if (! submap && (key = msymbol_get (key, M_key_alias)) != Mnil)
	submap = mplist_get (map->submaps, key);
    }

  if (submap)
    {
      mtext_cpy (ic->preedit, ic_info->preedit_saved);
      ic->cursor_pos = ic_info->state_pos;
      ic_info->key_head++;
      ic_info->map = map = submap;
      if (map->map_actions)
	{
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
	      if (take_action_list (ic, map->branch_actions) < 0)
		return -1;
	    }
	  /* If MAP is still not the root map, shift to the current
	     state.  */
	  if (ic_info->map != ic_info->state->map)
	    shift_state (ic, ic_info->state->name);
	}
    }
  else
    {
      /* MAP can not handle KEY.  */

      /* If MAP is the root map of the initial state, it means that
	 the current input method can not handle KEY.  */
      if (map == ((MIMState *) MPLIST_VAL (im_info->states))->map)
	return -1;

      if (map != ic_info->state->map)
	{
	  /* If MAP is not the root map... */
	  /* If MAP has branch actions, perform them.  */
	  if (map->branch_actions)
	    take_action_list (ic, map->branch_actions);
	  /* If MAP is still not the root map, shift to the current
	     state. */
	  if (ic_info->map != ic_info->state->map)
	    {
	      shift_state (ic, ic_info->state->name);
	      /* If MAP has branch_actions, perform them.  */
	      if (ic_info->map->branch_actions)
		take_action_list (ic, ic_info->map->branch_actions);
	    }
	}
      else
	{
	  /* MAP is the root map, perform branch actions (if any) or
	     shift to the initial state.  */
	  if (map->branch_actions)
	    take_action_list (ic, map->branch_actions);
	  else
	    shift_state (ic,
			 ((MIMState *) MPLIST_VAL (im_info->states))->name);
	}
    }
  return 0;
}

static void
reset_ic (MInputContext *ic)
{
  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;

  MLIST_RESET (ic_info);
  ic_info->state = (MIMState *) MPLIST_VAL (im_info->states);
  ic_info->map = ic_info->state->map;
  ic_info->state_key_head = ic_info->key_head = 0;
  ic->cursor_pos = ic_info->state_pos = 0;
  ic->status = ic_info->state->title;
  if (! ic->status)
    ic->status = im_info->title;
  ic->candidate_list = NULL;
  ic->candidate_show = 0;
  ic->status_changed = ic->preedit_changed = ic->candidates_changed = 1;
  if (ic_info->map->map_actions)
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
  reset_ic (ic);
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

  mtext_reset (ic->produced);
  ic->status_changed = ic->preedit_changed = ic->candidates_changed = 0;
  MLIST_APPEND1 (ic_info, keys, key, MERROR_IM);
  ic_info->key_unhandled = 0;
  do {
    if (handle_key (ic) < 0)
      {
	/* KEY was not handled.  Reset the status and break the
	   loop.  */
	reset_ic (ic);
	/* This forces returning 1.  */
	ic_info->key_unhandled = 1;
	break;
      }
    if (i++ == 100)
      {
	mdebug_hook ();
	reset_ic (ic);
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

  Minput_method = msymbol ("input-method");
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

  Mcandidate_list = msymbol_as_managing_key ("  candidate-list");
  Mcandidate_index = msymbol ("  candidate-index");

  Minit = msymbol ("init");
  Mfini = msymbol ("fini");

  M_key_alias = msymbol ("  key-alias");

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

  minput_default_driver.open_im = open_im;
  minput_default_driver.close_im = close_im;
  minput_default_driver.create_ic = create_ic;
  minput_default_driver.destroy_ic = destroy_ic;
  minput_default_driver.filter = filter;
  minput_default_driver.lookup = lookup;
  minput_default_driver.callback_list = NULL;
  minput_driver = &minput_default_driver;
  return 0;
}

void
minput__fini ()
{
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
    #MInputDriver::callback_list).  */ 
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
/*** @} */
/*=*/

/***en
    @brief The default input driver for internal input methods.

    The variable #minput_default_driver is the default driver for
    internal input methods.

    The member MInputDriver::open_im () searches the m17n database for
    an input method that matches the tag \<#Minput_method, $LANGUAGE,
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
    @brief 内部入力メソッド用デフォルト入力ドライバ

    入力ドライバ minput_default_driver は内部入力メソッド用のデフォル
    トの入力ドライバである。

    このドライバの <callback> メンバは @c NULL なので、プログラマ側で
    責任を持って, 適切なコールバック関数に設定し、Preedit テキスト,
    Status テキストがユーザに表示できるようにしなくてはならない。

    関数 M17N_INIT () は変数 @c minput_driver をこのドライバへのポイン
    タに設定し、全ての内部入力メソッドがこのドライバを使うようにする。

    したがって、@c minput_driver がデフォルト値のままであれば、minput_ 
    で始まる以下の関数群のドライバに依存する引数 $ARG はどれも無視され
    る。  */

MInputDriver minput_default_driver;
/*=*/

/***en
    @brief The input driver for internal input methods.

    The variable #minput_driver is a pointer to the input method
    driver that is used by internal input methods.  The macro
    M17N_INIT () initializes it to a pointer to #minput_default_driver
    (if <m17n.h> is included) or to #minput_gui_driver (if
    <m17n-gui.h> is included).  */ 

MInputDriver *minput_driver;

MSymbol Minput_driver;

/*=*/

/***en
    @brief Open an input method.

    The minput_open_im () function opens an input method that matches
    language $LANGUAGE and name $NAME, and returns a pointer to the
    input method object newly allocated.

    This function at first decides an input driver for the input
    method as below.

    If $LANGUAGE is not #Mnil, an input driver pointed by the variable
    #minput_driver is used.

    If $LANGUAGE is #Mnil and $NAME has #Minput_driver property, the
    input driver pointed to by the property value is used to open the
    input method.  If $NAME has no such property, @c NULL is returned.

    Then, the member MInputDriver::open_im () of the input driver is
    called.  

    $ARG is set in the member @c arg of the structure MInputMethod so
    that the input driver can refer to it.  */

/***ja
    @brief 入力メソッドをオープンする     

    関数 mim_open () は言語 $LANGUAGE と名前 $NAME に適合する入力メソッ
    ドをオープンし、その新たに割り当てられた入力メソッドへのポインタを返す。

    $LANGUAGE が #Mnil でなければ、m17n 言語情報ベース中から \<@c
    Minput_method, $LANGUAGE, $NAME \> というタグに適合する入力メソッ
    ドを探す。見つかった場合は、変数 #minput_driver でポイントされて
    いる入力ドライバを用いてその入力メソッドをオープンする。見つからな
    い場合やオープンできなかった場合には @c NULL を返す。

    変数 #minput_driver は、#minput_default_driver か @c
    minput_gui_driver のどちらかをポイントしている。前者は CUI 用であ
    り、関数m17n_initialize () を呼ぶことによって #minput_driver が 
    #minput_default_driver をポイントするようになる。後者は GUI 用で
    あり、関数 m17n_initialize_win () によってポイントされる。詳細につ
    いてはこれらの変数のドキュメントを参照のこと。

    $LANGUAGE が #Mnil であり、$NAME が #Minput_driver をキーとす
    るプロパティを持つ場合には、そのプロパティの値でポイントされている
    入力ドライバを用いて入力メソッドをオープンする。$NAME にそのような
    プロパティが無かった場合やオープンできなかった場合には @c NULL を
    返す。

    $ARG は、入力ドライバが参照できるように、構造体 MInputMethod のメ
    ンバ @c arg にセットされる。

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
    @brief 入力メソッドをクローズする

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
    @brief 入力コンテクストを生成する

    関数 minput_create_ic () は入力メソッド $IM に対応する入力コンテク
    ストオブジェクトを生成する。

    @return
    処理が成功した場合、minput_create_ic () は生成した入力コンテクスト
    へのポインタを返す。失敗した場合は @c NULL を返す。  */

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
    #Minput_status_done, and #Mcandidate_done in this order.  */

/***ja
    @brief 入力コンテクストを破壊する

    関数 minput_destroy_ic () は、入力コンテクスト $IC を破壊する。こ
    の入力コンテクストは minput_create_ic () によって作られたものでな
    ければならない。  */

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
    #Minput_preedit_draw, #Minput_status_draw, and #Mcandidate_draw if
    the preedit text, the status, and the current candidate are
    changed respectively.

    @return
    If $KEY is filtered out, this function returns 1.  In that case,
    the caller should discard the key.  Otherwise, it returns 0, and
    the caller should handle the key, for instance, by calling the
    function minput_lookup () with the same $KEY.  */

/***ja
    @brief 入力キーのフィルタリングをする

    関数 minput_filter () は入力キー $KEY を入力コンテクスト $IC に応
    じてフィルタリングする。

    入力コンテクスト $IC の入力メソッドが入力キーをフィルタすれば、こ
    の関数は 1 を返す。この場合呼び出し側はこのキーを捨てるべきである。
    そうでなければ 0 を返し、呼び出し側が、たとえば同じ $KEY で関数 
    minput_lookup () を呼ぶなどして、このキーを処理する。

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
      ic->preedit_changed = ic->status_changed = ic->candidates_changed = 0;
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

    This function calls #MInputDriver::lookup.

    @return
    If $KEY was correctly handled by the input method, this function
    returns 0.  Otherwise, returns -1, even in that case, some text
    may be produced in $MT.  */

/***ja
    @brief 入力メソッドが作ったテキストの獲得

    関数 minput_lookup () は入力コンテクスト $IC 中のテキストを獲得す
    る。$KEY は関数minput_filter () への直前の呼び出しに用いられたもの
    と同じでなくてはならない。

    テキストが入力メソッドによって生成されていれば、$IC->produced に保
    持されている。

     この関数は、X ウィンドウの <tt>XLookupString ()</tt> 、
    <tt>XmbLookupString ()</tt> 、<tt>XwcLookupString ()</tt> に対応す
    る。

    @return
    $KEY が入力メソッドによって適切に処理できていれば、この関数は 0 を
    返す。そうでなければ -1 を返し、この場合でも$IC->produced に何らか
    のテキストが生成されていることがある。

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
    to coordinate ($X, $Y) with the height $ASCENT and $DESCENT.
    $FONTSIZE specfies the fontsize of a preedit text in 1/10 point.
    The semantics of these values depend on the input driver.

    For instance, an input driver designed to work in CUI environment
    may use $X and $Y as column and row numbers, and ignore $ASCENT
    and $DESCENT.  An input driver designed to work on a window system
    may treat $X and $Y as pixel offsets relative to the origin of the
    client window, and treat $ASCENT and $DESCENT as ascent and
    descent pixels of a line at ($X . $Y).

    $MT and $POS is an M-text and a character position at the spot.
    $MT may be NULL, in which case, the input method can't get
    information about the text around the spot.  */

/***ja
    @brief 入力コンテクストのスポットを設定する

    関数 minput_set_spot () は、入力コンテクスト $IC のスポットを、座
    標 ($X, $Y)に 、高さ $ASCENT、$DESCENT で設定する。これらの値の意
    味は入力ドライバに依存する。

    たとえば CUI 環境で動作する入力ドライバは $X, $Y をそれぞれ列と行
    の番号として用い、$ASCENT、$DESCENT を無視するかもしれない。 また
    ウィンドウシステム用の入力ドライバは $X,$Y をクライアントウィンド
    ウの原点からのオフセットをピクセル単位で表したものとして扱い、
    $ASCENT と $DESCENT を ($X . $Y) の列のアセントとディセントをピク
    セル単位で表したものとして扱うかもしれない。  */

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
    with the input context $IC.  */

void
minput_toggle (MInputContext *ic)
{
  if (ic->im->driver.callback_list)
    minput__callback (ic, Minput_toggle);
  ic->active = ! ic->active;
}


/*** @} */
/*=*/
/*** @addtogroup m17nDebug */
/*=*/
/*** @{  */
/*=*/

/***en
    @brief Dump an input method

    The mdebug_dump_im () function prints the input method $IM in a
    human readable way to the stderr.  $INDENT specifies how many
    columns to indent the lines but the first one.

    @return
    This function returns $IM.  */

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
