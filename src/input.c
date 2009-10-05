/* input.c -- input method module.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009
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

/***en
    @addtogroup m17nInputMethod
    @brief API for Input method.

    An input method is an object to enable inputting various
    characters.  An input method is identified by a pair of symbols,
    LANGUAGE and NAME.  This pair decides an input method driver of the
    input method.  An input method driver is a set of functions for
    handling the input method.  There are two kinds of input methods;
    internal one and foreign one.

    <ul>
    <li> Internal Input Method

    An internal input method has non @c Mnil LANGUAGE, and its body is
    defined in the m17n database by the tag <Minput_method, LANGUAGE,
    NAME>.  For this kind of input methods, the m17n library uses two
    predefined input method drivers, one for CUI use and the other for
    GUI use.  Those drivers utilize the input processing engine
    provided by the m17n library itself.  The m17n database may
    provide input methods that are not limited to a specific language.
    The database uses @c Mt as LANGUAGE of those input methods.

    An internal input method accepts an input key which is a symbol
    associated with an input event.  As there is no way for the @c
    m17n @c library to know how input events are represented in an
    application program, an application programmer has to convert an
    input event to an input key by himself.  See the documentation of
    the function minput_event_to_key () for the detail.

    <li> Foreign Input Method @anchor foreign-input-method

    A foreign input method has @c Mnil LANGUAGE, and its body is
    defined in an external resource (e.g. XIM of X Window System).
    For this kind of input methods, the symbol NAME must have a
    property of key #Minput_driver, and the value must be a pointer
    to an input method driver.  Therefore, by preparing a proper
    driver, any kind of input method can be treated in the framework
    of the @c m17n @c library.

    For convenience, the m17n-X library provides an input method
    driver that enables the input style of OverTheSpot for XIM, and
    stores #Minput_driver property of the symbol @c Mxim with a
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

    入力メソッドは多様な文字を入力するためのオブジェクトである。
    入力メソッドはシンボル LANGUAGE と NAME の組によって識別され、
    この組合せによって入力メソッドドライバが決定する。
    入力メソッドドライバとは、ある入力メソッドを扱うための関数の集まりである。
    入力メソッドには内部メソッドと外部メソッドの二種類がある。

    <ul> 
    <li> 内部入力メソッド

    内部入力メソッドとは LANGUAGE が @c Mnil 以外のものであり、その本体
    はm17n データベースに<Minput_method, LANGUAGE, NAME> というタグを付
    けて定義されている。この種の入力メソッドに対して、m17n ライブラリで
    はCUI 用と GUI 用それぞれの入力メソッドドライバをあらかじめ定義して
    いる。これらのドライバは m17n ライブラリ自体の入力処理エンジンを利
    用する。m17n データベースには、特定の言語専用でない入力メソッドを定
    義することもでき、そのような入力メソッドの LANGUAGE は @c Mt である。

    内部入力メソッドは、ユーザの入力イベントに対応したシンボルである入
    力キーを受け取る。@c m17n @c ライブラリ は入力イベントがアプリケー
    ションプログラムでどう表現されているかを知ることができないので、入
    力イベントから入力キーへの変換はアプリケーションプログラマの責任で
    行わなくてはならない。詳細については関数 minput_event_to_key () の
    説明を参照。

    <li> 外部入力メソッド @anchor foreign-input-method

    外部入力メソッドとは LANGUAGE が @c Mnil のものであり、その本体は外
    部のリソースとして定義される。（たとえばX Window System のXIM な
    ど。) この種の入力メソッドでは、シンボル NAME は #Minput_driver を
    キーとするプロパティを持ち、その値は入力メソッドドライバへのポイン
    タである。このことにより、適切なドライバを準備することによって、い
    かなる種類の入力メソッドも@c m17n @c ライブラリ の枠組の中で扱う事
    ができる。

    利便性の観点から、m17n X ライブラリは XIM の OverTheSpot の入力スタ
    イルを実現する入力メソッドドライバを提供し、またシンボル @c Mxim の
    #Minput_driver プロパティの値としてそのドライバへのポインタを保持
    している。詳細については m17n GUI API のドキュメントを参照のこと。

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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "config.h"

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "mtext.h"
#include "input.h"
#include "symbol.h"
#include "plist.h"
#include "database.h"
#include "charset.h"

static int mdebug_flag = MDEBUG_INPUT;

static int fully_initialized;

/** Symbols to load an input method data.  */
static MSymbol Mtitle, Mmacro, Mmodule, Mstate, Minclude;

/** Symbols for actions.  */
static MSymbol Minsert, Mdelete, Mmark, Mmove, Mpushback, Mundo, Mcall, Mshift;
static MSymbol Mselect, Mshow, Mhide, Mcommit, Munhandle, Mpop;
static MSymbol Mset, Madd, Msub, Mmul, Mdiv, Mequal, Mless, Mgreater;
static MSymbol Mless_equal, Mgreater_equal;
static MSymbol Mcond;
static MSymbol Mplus, Mminus, Mstar, Mslash, Mand, Mor, Mnot;

/** Special action symbol.  */
static MSymbol Mat_reload;

static MSymbol M_candidates;

static MSymbol Mcandidate_list, Mcandidate_index;

static MSymbol Minit, Mfini;

/** Symbols for variables.  */
static MSymbol Mcandidates_group_size, Mcandidates_charset;

/** Symbols for key events.  */
static MSymbol one_char_symbol[256];

static MSymbol M_key_alias;

static MSymbol Mdescription, Mcommand, Mvariable, Mglobal, Mconfig;

static MSymbol M_gettext;

/** Structure to hold a map.  */

struct MIMMap
{
  /** List of actions to take when we reach the map.  In a root map,
      the actions are executed only when there is no more key.  */
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
  M17NObject control;

  /** Name of the state.  */
  MSymbol name;

  /** Title of the state, or NULL.  */
  MText *title;

  /** Key translation map of the state.  Built by merging all maps of
      branches.  */
  MIMMap *map;
};

#define CUSTOM_FILE "config.mic"

static MPlist *load_im_info_keys;

/* List of input method information.  The format is:
     (LANGUAGE NAME t:IM_INFO ... ... ...)  */
static MPlist *im_info_list;

/* Database for user's customization file.  */
static MDatabase *im_custom_mdb;

/* List of input method information loaded from im_custom_mdb.  The
   format is the same as im_info_list.  */
static MPlist *im_custom_list;

/* List of input method information configured by
   minput_config_command and minput_config_variable.  The format is
   the same as im_info_list.  */
static MPlist *im_config_list;

/* Global input method information.  It points into the element of
   im_info_list corresponding to LANGUAGE == `nil' and NAME ==
   `global'.  */
static MInputMethodInfo *global_info;

static int update_global_info (void);
static int update_custom_info (void);
static MInputMethodInfo *get_im_info (MSymbol, MSymbol, MSymbol, MSymbol);


void
fully_initialize ()
{
  char *key_names[32]
    = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"BackSpace", "Tab", "Linefeed", "Clear", NULL, "Return", NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "Escape", NULL, NULL, NULL, NULL };
  char buf[6], buf2[32], buf3[2];
  int i, j;
  /* Maximum case: '\215', C-M-m, C-M-M, M-Return, C-A-m, C-A-M, A-Return
     plus one for cyclic alias.  */
  MSymbol alias[8];

  M_key_alias = msymbol ("  key-alias");

  buf3[1] = '\0';

  /* Aliases for 0x00-0x1F */
  buf[0] = 'C';
  buf[1] = '-';
  buf[3] = '\0';
  for (i = 0, buf[2] = '@'; i < ' '; i++, buf[2]++)
    {
      j = 0;
      buf3[0] = i;
      alias[j++] = msymbol (buf3);
      alias[j++] = one_char_symbol[i] = msymbol (buf);
      if (key_names[i] || (buf[2] >= 'A' && buf[2] <= 'Z'))
	{
	  if (key_names[i])
	    {
	      /* Ex: `Escape' == `C-['  */
	      alias[j++] = msymbol (key_names[i]);
	    }
	  if (buf[2] >= 'A' && buf[2] <= 'Z')
	    {
	      /* Ex: `C-a' == `C-A'  */
	      buf[2] += 32;
	      alias[j++] = msymbol (buf);
	      buf[2] -= 32;
	    }
	}
      /* Establish cyclic alias chain.  */
      alias[j] = alias[0];
      while (--j >= 0)
	msymbol_put (alias[j], M_key_alias, alias[j + 1]);
    }

  /* Aliases for 0x20-0x7E */
  buf[0] = 'S';
  for (i = buf[2] = ' '; i < 127; i++, buf[2]++)
    {
      one_char_symbol[i] = msymbol (buf + 2);
      if (i >= 'A' && i <= 'Z')
	{
	  /* Ex: `A' == `S-A' == `S-a'.  */
	  alias[0] = alias[3] = one_char_symbol[i];
	  alias[1] = msymbol (buf);
	  buf[2] += 32;
	  alias[2] = msymbol (buf);
	  buf[2] -= 32;
	  for (j = 0; j < 3; j++)
	    msymbol_put (alias[j], M_key_alias, alias[j + 1]);
	}
    }

  /* Aliases for 0x7F */
  buf3[0] = 0x7F;
  alias[0] = alias[3] = msymbol (buf3);
  alias[1] = one_char_symbol[127] = msymbol ("Delete");
  alias[2] = msymbol ("C-?");
  for (j = 0; j < 3; j++)
    msymbol_put (alias[j], M_key_alias, alias[j + 1]);

  /* Aliases for 0x80-0x9F */
  buf[0] = 'C';
  /* buf[1] = '-'; -- already done */
  buf[3] = '-';
  buf[5] = '\0';
  buf2[1] = '-';
  for (i = 128, buf[4] = '@'; i < 160; i++, buf[4]++)
    {
      j = 0;
      buf3[0] = i;
      alias[j++] = msymbol (buf3);
      /* `C-M-a' == `C-A-a' */
      buf[2] = 'M';
      alias[j++] = one_char_symbol[i] = msymbol (buf);
      buf[2] = 'A';
      alias[j++] = msymbol (buf);
      if (key_names[i - 128])
	{
	  /* Ex: `M-Escape' == `A-Escape' == `C-M-['.  */
	  buf2[0] = 'M';
	  strcpy (buf2 + 2, key_names[i - 128]);
	  alias[j++] = msymbol (buf2);
	  buf2[0] = 'A';
	  alias[j++] = msymbol (buf2);
	}
      if (buf[4] >= 'A' && buf[4] <= 'Z')
	{
	  /* Ex: `C-M-a' == `C-M-A'.  */
	  buf[4] += 32;
	  buf[2] = 'M';
	  alias[j++] = msymbol (buf);
	  buf[2] = 'A';
	  alias[j++] = msymbol (buf);
	  buf[4] -= 32;
	}

      /* Establish cyclic alias chain.  */
      alias[j] = alias[0];
      while (--j >= 0)
	msymbol_put (alias[j], M_key_alias, alias[j + 1]);
    }

  /* Aliases for 0xA0-0xFF */
  for (i = 160, buf[4] = ' '; i < 255; i++, buf[4]++)
    {
      j = 0;
      buf3[0] = i;
      alias[j++] = msymbol (buf3);
      buf[2] = 'M';
      alias[j++] = one_char_symbol[i] = msymbol (buf + 2);
      buf[2] = 'A';
      alias[j++] = msymbol (buf + 2);
      alias[j]= alias[0];
      while (--j >= 0)
	msymbol_put (alias[j], M_key_alias, alias[j + 1]);
    }

  buf3[0] = 255;
  alias[0] = alias[3] = msymbol (buf3);
  alias[1] = one_char_symbol[255] = msymbol ("M-Delete");
  alias[2] = msymbol ("A-Delete");
  for (j = 0; j < 3; j++)
    msymbol_put (alias[j], M_key_alias, alias[j + 1]);

  /* Aliases for keys that can't be mapped to one-char-symbol
     (e.g. C-A-1) */
  /* buf is already set to "C-?-".  */
  for (i = ' '; i <= '~'; i++)
    {
      if (i == '@')
	{
	  i = '_';
	  continue;
	}
      if (i == 'a')
	{
	  i = 'z';
	  continue;
	}
      buf[2] = 'M';
      buf[4] = i;
      alias[0] = alias[2] = msymbol (buf);
      buf[2] = 'A';
      alias[1] = msymbol (buf);
      for (j = 0; j < 2; j++)
	msymbol_put (alias[j], M_key_alias, alias[j + 1]);
    }

  Minput_method = msymbol ("input-method");
  Mtitle = msymbol ("title");
  Mmacro = msymbol ("macro");
  Mmodule = msymbol ("module");
  Mmap = msymbol ("map");
  Mstate = msymbol ("state");
  Minclude = msymbol ("include");
  Minsert = msymbol ("insert");
  M_candidates = msymbol ("  candidates");
  Mdelete = msymbol ("delete");
  Mmove = msymbol ("move");
  Mmark = msymbol ("mark");
  Mpushback = msymbol ("pushback");
  Mpop = msymbol ("pop");
  Mundo = msymbol ("undo");
  Mcall = msymbol ("call");
  Mshift = msymbol ("shift");
  Mselect = msymbol ("select");
  Mshow = msymbol ("show");
  Mhide = msymbol ("hide");
  Mcommit = msymbol ("commit");
  Munhandle = msymbol ("unhandle");
  Mset = msymbol ("set");
  Madd = msymbol ("add");
  Msub = msymbol ("sub");
  Mmul = msymbol ("mul");
  Mdiv = msymbol ("div");
  Mequal = msymbol ("=");
  Mless = msymbol ("<");
  Mgreater = msymbol (">");
  Mless_equal = msymbol ("<=");
  Mgreater_equal = msymbol (">=");
  Mcond = msymbol ("cond");
  Mplus = msymbol ("+");
  Mminus = msymbol ("-");
  Mstar = msymbol ("*");
  Mslash = msymbol ("/");
  Mand = msymbol ("&");
  Mor = msymbol ("|");
  Mnot = msymbol ("!");

  Mat_reload = msymbol ("-reload");

  Mcandidates_group_size = msymbol ("candidates-group-size");
  Mcandidates_charset = msymbol ("candidates-charset");

  Mcandidate_list = msymbol_as_managing_key ("  candidate-list");
  Mcandidate_index = msymbol ("  candidate-index");

  Minit = msymbol ("init");
  Mfini = msymbol ("fini");

  Mdescription = msymbol ("description");
  Mcommand = msymbol ("command");
  Mvariable = msymbol ("variable");
  Mglobal = msymbol ("global");
  Mconfig = msymbol ("config");
  M_gettext = msymbol ("_");

  load_im_info_keys = mplist ();
  mplist_add (load_im_info_keys, Mstate, Mnil);
  mplist_push (load_im_info_keys, Mmap, Mnil);

  im_info_list = mplist ();
  im_config_list = im_custom_list = NULL;
  im_custom_mdb = NULL;
  update_custom_info ();
  global_info = NULL;
  update_global_info ();

  fully_initialized = 1;
}

#define MINPUT__INIT()		\
  do {				\
    if (! fully_initialized)	\
      fully_initialize ();	\
  } while (0)


static int
marker_code (MSymbol sym, int surrounding)
{
  char *name;

  if (sym == Mnil)
    return -1;
  name = MSYMBOL_NAME (sym);
  return (name[0] != '@' ? -1
	  : (((name[1] >= '0' && name[1] <= '9')
	      || name[1] == '<' || name[1] == '>' || name[1] == '='
	      || name[1] == '[' || name[1] == ']'
	      || name[1] == '@')
	     && name[2] == '\0') ? name[1]
	  : (name[1] != '+' && name[1] != '-') ? -1
	  : (name[2] == '\0' || surrounding) ? name[1]
	  : -1);
}


/* Return a plist containing an integer value of VAR.  The plist must
   not be UNREFed. */

static MPlist *
resolve_variable (MInputContextInfo *ic_info, MSymbol var)
{
  MPlist *plist = mplist__assq (ic_info->vars, var);

  if (plist)
    {
      plist = MPLIST_PLIST (plist);
      return MPLIST_NEXT (plist);
    }

  plist = mplist ();
  mplist_push (ic_info->vars, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
  plist = mplist_add (plist, Msymbol, var);
  plist = mplist_add (plist, Minteger, (void *) 0);
  return plist;
}

static MText *
get_surrounding_text (MInputContext *ic, int len)
{
  MText *mt = NULL;

  mplist_push (ic->plist, Minteger, (void *) len);
  if (minput_callback (ic, Minput_get_surrounding_text) >= 0
      && MPLIST_MTEXT_P (ic->plist))
    mt = MPLIST_MTEXT (ic->plist);
  mplist_pop (ic->plist);
  return mt;
}

static void
delete_surrounding_text (MInputContext *ic, int pos)
{
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;

  mplist_push (ic->plist, Minteger, (void *) pos);
  minput_callback (ic, Minput_delete_surrounding_text);
  mplist_pop (ic->plist);
  if (pos < 0)
    {
      M17N_OBJECT_UNREF (ic_info->preceding_text);
      ic_info->preceding_text = NULL;
    }
  else if (pos > 0)
    {
      M17N_OBJECT_UNREF (ic_info->following_text);
      ic_info->following_text = NULL;
    }
}

static int
get_preceding_char (MInputContext *ic, int pos)
{
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  MText *mt;
  int len;

  if (pos && ic_info->preceding_text)
    {
      len = mtext_nchars (ic_info->preceding_text);
      if (pos <= len)
	return mtext_ref_char (ic_info->preceding_text, len - pos);
    }
  mt = get_surrounding_text (ic, - pos);
  if (! mt)
    return -2;
  len = mtext_nchars (mt);
  if (ic_info->preceding_text)
    {
      if (mtext_nchars (ic_info->preceding_text) < len)
	{
	  M17N_OBJECT_UNREF (ic_info->preceding_text);
	  ic_info->preceding_text = mt;
	}
      else
	M17N_OBJECT_UNREF (mt);
    }
  else
    ic_info->preceding_text = mt;
  if (pos > len)
    return -1;
  return mtext_ref_char (ic_info->preceding_text, len - pos);
}

static int
get_following_char (MInputContext *ic, int pos)
{
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  MText *mt;
  int len;

  if (ic_info->following_text)
    {
      len = mtext_nchars (ic_info->following_text);
      if (pos < len)
	return mtext_ref_char (ic_info->following_text, pos);
    }
  mt = get_surrounding_text (ic, pos + 1);
  if (! mt)
    return -2;
  len = mtext_nchars (mt);
  if (ic_info->following_text)
    {
      if (mtext_nchars (ic_info->following_text) < len)
	{
	  M17N_OBJECT_UNREF (ic_info->following_text);
	  ic_info->following_text = mt;
	}
      else
	M17N_OBJECT_UNREF (mt);
    }
  else
    ic_info->following_text = mt;
  if (pos >= len)
    return -1;
  return mtext_ref_char (ic_info->following_text, pos);
}

static int
surrounding_pos (MSymbol sym)
{
  char *name;

  if (sym == Mnil)
    return 0;
  name = MSYMBOL_NAME (sym);
  if (name[0] == '@'
      && (name[1] == '-' || name[1] == '+')
      && name[2] >= '1' && name[2] <= '9')
    return (name[1] == '-' ? - atoi (name + 2) : atoi (name + 2));
  return 0;
}

static int
integer_value (MInputContext *ic, MPlist *arg, int surrounding)
{
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  int code, pos;
  MText *preedit = ic->preedit;
  int len = mtext_nchars (preedit);

  if (MPLIST_INTEGER_P (arg))
    return MPLIST_INTEGER (arg);

  code = marker_code (MPLIST_SYMBOL (arg), surrounding);
  if (code < 0)
    {
      MPlist *val = resolve_variable (ic_info, MPLIST_SYMBOL (arg));

      return (MPLIST_INTEGER_P (val) ? MPLIST_INTEGER (val) : 0);
    }
  if (code == '@')
    return ic_info->key_head;
  if ((code == '-' || code == '+'))
    {
      char *name = MSYMBOL_NAME (MPLIST_SYMBOL (arg));

      if (name[2])
	{
	  pos = atoi (name + 1);
	  if (pos == 0)
	    return get_preceding_char (ic, 0);
	  if (pos < 0)
	    pos = ic->cursor_pos + pos;
	  else
	    pos = ic->cursor_pos + pos - 1;
	  if (pos < 0)
	    {
	      if (ic->produced && mtext_len (ic->produced) + pos >= 0)
		return mtext_ref_char (ic->produced,
				       mtext_len (ic->produced) + pos);
	      return get_preceding_char (ic, - pos);
	    }
	  else if (pos >= len)
	    return get_following_char (ic, pos - len);
	}
      else
	pos = ic->cursor_pos + (code == '+' ? 1 : -1);
    }
  else if (code >= '0' && code <= '9')
    pos = code - '0';
  else if (code == '=')
    pos = ic->cursor_pos;
  else if (code == '[')
    pos = ic->cursor_pos - 1;
  else if (code == ']')
    pos = ic->cursor_pos + 1;
  else if (code == '<')
    pos = 0;
  else if (code == '>')
    pos = len - 1;
  return (pos >= 0 && pos < len ? mtext_ref_char (preedit, pos) : -1);
}

static int
parse_expression (MPlist *plist)
{
  MSymbol op;

  if (MPLIST_INTEGER_P (plist) || MPLIST_SYMBOL_P (plist))
    return 0;
  if (! MPLIST_PLIST_P (plist))
    return -1;
  plist = MPLIST_PLIST (plist);
  op = MPLIST_SYMBOL (plist);
  if (op != Mplus && op != Mminus && op != Mstar && op != Mslash
      && op != Mand && op != Mor && op != Mnot
      && op != Mless && op != Mgreater && op != Mequal
      && op != Mless_equal && op != Mgreater_equal)
    MERROR (MERROR_IM, -1);
  MPLIST_DO (plist, MPLIST_NEXT (plist))
    if (parse_expression (plist) < 0)
      return -1;
  return 0;
}

static int
resolve_expression (MInputContext *ic, MPlist *plist)
{
  int val;
  MSymbol op;
  
  if (MPLIST_INTEGER_P (plist))
    return MPLIST_INTEGER (plist);
  if (MPLIST_SYMBOL_P (plist))
    return integer_value (ic, plist, 1);
  if (! MPLIST_PLIST_P (plist))
    return 0;
  plist = MPLIST_PLIST (plist);
  if (! MPLIST_SYMBOL_P (plist))
    return 0;
  op = MPLIST_SYMBOL (plist);
  plist = MPLIST_NEXT (plist);
  val = resolve_expression (ic, plist);
  if (op == Mplus)
    MPLIST_DO (plist, MPLIST_NEXT (plist))
      val += resolve_expression (ic, plist);
  else if (op == Mminus)
    MPLIST_DO (plist, MPLIST_NEXT (plist))
      val -= resolve_expression (ic, plist);
  else if (op == Mstar)
    MPLIST_DO (plist, MPLIST_NEXT (plist))
      val *= resolve_expression (ic, plist);
  else if (op == Mslash)
    MPLIST_DO (plist, MPLIST_NEXT (plist))
      val /= resolve_expression (ic, plist);
  else if (op == Mand)
    MPLIST_DO (plist, MPLIST_NEXT (plist))
      val &= resolve_expression (ic, plist);
  else if (op == Mor)
    MPLIST_DO (plist, MPLIST_NEXT (plist))
      val |= resolve_expression (ic, plist);
  else if (op == Mnot)
    val = ! val;
  else if (op == Mless)
    val = val < resolve_expression (ic, MPLIST_NEXT (plist));
  else if (op == Mequal)
    val = val == resolve_expression (ic, MPLIST_NEXT (plist));
  else if (op == Mgreater)
    val = val > resolve_expression (ic, MPLIST_NEXT (plist));
  else if (op == Mless_equal)
    val = val <= resolve_expression (ic, MPLIST_NEXT (plist));
  else if (op == Mgreater_equal)
    val = val >= resolve_expression (ic, MPLIST_NEXT (plist));
  return val;
}

/* Parse PLIST as an action list.  PLIST should have this form:
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

	  if (action_name == M_candidates)
	    {
	      /* This is an already regularised action.  */
	      continue;
	    }
	  if (action_name == Minsert)
	    {
	      if (MPLIST_MTEXT_P (pl))
		{
		  if (mtext_nchars (MPLIST_MTEXT (pl)) == 0)
		    MERROR (MERROR_IM, -1);
		}
	      else if (MPLIST_INTEGER_P (pl))
		{
		  int c = MPLIST_INTEGER (pl);

		  if (c < 0 || c > MCHAR_MAX)
		    MERROR (MERROR_IM, -1);
		}
	      else if (MPLIST_PLIST_P (pl))
		{
		  MPLIST_DO (pl, MPLIST_PLIST (pl))
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
	      if (parse_expression (pl) < 0)
		return -1;
	    }
	  else if (action_name == Mmark
		   || action_name == Mcall
		   || action_name == Mshift)
	    {
	      if (! MPLIST_SYMBOL_P (pl))
		MERROR (MERROR_IM, -1);
	    }
	  else if (action_name == Mundo)
	    {
	      if (! MPLIST_TAIL_P (pl))
		{
		  if (! MPLIST_SYMBOL_P (pl)
		      && ! MPLIST_INTEGER_P (pl))
		    MERROR (MERROR_IM, -1);		    
		}
	    }
	  else if (action_name == Mpushback)
	    {
	      if (MPLIST_MTEXT_P (pl))
		{
		  MText *mt = MPLIST_MTEXT (pl);

		  if (mtext_nchars (mt) != mtext_nbytes (mt))
		    MERROR (MERROR_IM, -1);		    
		}
	      else if (MPLIST_PLIST_P (pl))
		{
		  MPlist *p;

		  MPLIST_DO (p, MPLIST_PLIST (pl))
		    if (! MPLIST_SYMBOL_P (p))
		      MERROR (MERROR_IM, -1);
		}
	      else if (! MPLIST_INTEGER_P (pl))
		MERROR (MERROR_IM, -1);
	    }
	  else if (action_name == Mset || action_name == Madd
		   || action_name == Msub || action_name == Mmul
		   || action_name == Mdiv)
	    {
	      if (! MPLIST_SYMBOL_P (pl))
		MERROR (MERROR_IM, -1);
	      if (parse_expression (MPLIST_NEXT (pl)) < 0)
		return -1;
	    }
	  else if (action_name == Mequal || action_name == Mless
		   || action_name == Mgreater || action_name == Mless_equal
		   || action_name == Mgreater_equal)
	    {
	      if (parse_expression (pl) < 0
		  || parse_expression (MPLIST_NEXT (pl)) < 0)
		return -1;
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
	  else if (action_name == Mshow || action_name == Mhide
		   || action_name == Mcommit || action_name == Munhandle
		   || action_name == Mpop)
	    ;
	  else if (action_name == Mcond)
	    {
	      MPLIST_DO (pl, pl)
		if (! MPLIST_PLIST_P (pl))
		  MERROR (MERROR_IM, -1);
	    }
	  else if (! macros || ! mplist_get (macros, action_name))
	    MERROR (MERROR_IM, -1);
	}
      else if (! MPLIST_SYMBOL_P (plist))
	MERROR (MERROR_IM, -1);
    }

  return 0;
}

static MPlist *
resolve_command (MPlist *cmds, MSymbol command)
{
  MPlist *plist;

  if (! cmds || ! (plist = mplist__assq (cmds, command)))
    return NULL;
  plist = MPLIST_PLIST (plist);	/* (NAME DESC STATUS [KEYSEQ ...]) */
  plist = MPLIST_NEXT (plist);
  plist = MPLIST_NEXT (plist);
  plist = MPLIST_NEXT (plist);
  return plist;
}

/* Load a translation into MAP from PLIST.
   PLIST has this form:
      PLIST ::= ( KEYSEQ MAP-ACTION * )  */

static int
load_translation (MIMMap *map, MPlist *keylist, MPlist *map_actions,
		  MPlist *branch_actions, MPlist *macros)
{
  MSymbol *keyseq;
  int len, i;

  if (MPLIST_MTEXT_P (keylist))
    {
      MText *mt = MPLIST_MTEXT (keylist);

      len = mtext_nchars (mt);
      if (MFAILP (len > 0 && len == mtext_nbytes (mt)))
	return -1;
      keyseq = (MSymbol *) alloca (sizeof (MSymbol) * len);
      for (i = 0; i < len; i++)
	keyseq[i] = one_char_symbol[MTEXT_DATA (mt)[i]];
    }
  else
    {
      MPlist *elt;
	  
      if (MFAILP (MPLIST_PLIST_P (keylist)))
	return -1;
      elt = MPLIST_PLIST (keylist);
      len = MPLIST_LENGTH (elt);
      if (MFAILP (len > 0))
	return -1;
      keyseq = (MSymbol *) alloca (sizeof (int) * len);
      for (i = 0; i < len; i++, elt = MPLIST_NEXT (elt))
	{
	  if (MPLIST_INTEGER_P (elt))
	    {
	      int c = MPLIST_INTEGER (elt);

	      if (MFAILP (c >= 0 && c < 0x100))
		return -1;
	      keyseq[i] = one_char_symbol[c];
	    }
	  else
	    {
	      if (MFAILP (MPLIST_SYMBOL_P (elt)))
		return -1;
	      keyseq[i] = MPLIST_SYMBOL (elt);
	    }
	}
    }

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

  if (! MPLIST_TAIL_P (map_actions))
    {
      if (parse_action_list (map_actions, macros) < 0)
	MERROR (MERROR_IM, -1);
      map->map_actions = map_actions;
    }
  if (branch_actions)
    {
      map->branch_actions = branch_actions;
      M17N_OBJECT_REF (branch_actions);
    }

  return 0;
}

/* Load a branch from PLIST into MAP.  PLIST has this form:
      PLIST ::= ( MAP-NAME BRANCH-ACTION * )  */

static int
load_branch (MInputMethodInfo *im_info, MPlist *plist, MIMMap *map)
{
  MSymbol map_name;
  MPlist *branch_actions;

  if (MFAILP (MPLIST_SYMBOL_P (plist)))
    return -1;
  map_name = MPLIST_SYMBOL (plist);
  plist = MPLIST_NEXT (plist);
  if (MPLIST_TAIL_P (plist))
    branch_actions = NULL;
  else if (MFAILP (parse_action_list (plist, im_info->macros) >= 0))
    return -1;
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
  else if (im_info->maps) 
    {
      plist = (MPlist *) mplist_get (im_info->maps, map_name);
      if (! plist && im_info->configured_vars)
	{
	  MPlist *p = mplist__assq (im_info->configured_vars, map_name);

	  if (p && MPLIST_PLIST_P (p))
	    {
	      p = MPLIST_NEXT (MPLIST_NEXT (MPLIST_NEXT (MPLIST_PLIST (p))));
	      if (MPLIST_SYMBOL_P (p))
		plist = mplist_get (im_info->maps, MPLIST_SYMBOL (p));
	    }
	}
      if (plist)
	{
	  MPLIST_DO (plist, plist)
	    {
	      MPlist *keylist, *map_actions;

	      if (! MPLIST_PLIST_P (plist))
		MERROR (MERROR_IM, -1);
	      keylist = MPLIST_PLIST (plist);
	      map_actions = MPLIST_NEXT (keylist);
	      if (MPLIST_SYMBOL_P (keylist))
		{
		  MSymbol command = MPLIST_SYMBOL (keylist);
		  MPlist *pl;

		  if (MFAILP (command != Mat_reload))
		    continue;
		  pl = resolve_command (im_info->configured_cmds, command);
		  if (MFAILP (pl))
		    continue;
		  MPLIST_DO (pl, pl)
		    load_translation (map, pl, map_actions, branch_actions,
				      im_info->macros);
		}
	      else
		load_translation (map, keylist, map_actions, branch_actions,
				  im_info->macros);
	    }
	}
    }

  return 0;
}

/* Load a macro from PLIST into IM_INFO->macros.
   PLIST has this form:
      PLIST ::= ( MACRO-NAME ACTION * )
   IM_INFO->macros is a plist of macro names vs action list.  */

static int
load_macros (MInputMethodInfo *im_info, MPlist *plist)
{
  MSymbol name; 
  MPlist *pl;

  if (! MPLIST_SYMBOL_P (plist))
    MERROR (MERROR_IM, -1);
  name = MPLIST_SYMBOL (plist);
  plist = MPLIST_NEXT (plist);
  if (MFAILP (! MPLIST_TAIL_P (plist)))
    MERROR (MERROR_IM, -1);
  pl = mplist_get (im_info->macros, name);
  M17N_OBJECT_UNREF (pl);
  mplist_put (im_info->macros, name, plist);
  M17N_OBJECT_REF (plist);
  return 0;
}

/* Load an external module from PLIST into IM_INFO->externals.
   PLIST has this form:
      PLIST ::= ( MODULE-NAME FUNCTION * )
   IM_INFO->externals is a plist of MODULE-NAME vs (MIMExternalModule *).  */

static int
load_external_module (MInputMethodInfo *im_info, MPlist *plist)
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
  if (MFAILP (handle))
    {
      fprintf (stderr, "%s\n", dlerror ());
      return -1;
    }
  func_list = mplist ();
  MPLIST_DO (plist, MPLIST_NEXT (plist))
    {
      if (! MPLIST_SYMBOL_P (plist))
	MERROR_GOTO (MERROR_IM, err_label);
      func = dlsym (handle, MSYMBOL_NAME (MPLIST_SYMBOL (plist)));
      if (MFAILP (func))
	goto err_label;
      mplist_add (func_list, MPLIST_SYMBOL (plist), func);
    }

  MSTRUCT_MALLOC (external, MERROR_IM);
  external->handle = handle;
  external->func_list = func_list;
  mplist_add (im_info->externals, module, external);
  return 0;

 err_label:
  dlclose (handle);
  M17N_OBJECT_UNREF (func_list);
  return -1;
}

static void
free_map (MIMMap *map, int top)
{
  MPlist *plist;

  if (top)
    M17N_OBJECT_UNREF (map->map_actions);
  if (map->submaps)
    {
      MPLIST_DO (plist, map->submaps)
	free_map ((MIMMap *) MPLIST_VAL (plist), 0);
      M17N_OBJECT_UNREF (map->submaps);
    }
  M17N_OBJECT_UNREF (map->branch_actions);
  free (map);
}

static void
free_state (void *object)
{
  MIMState *state = object;

  M17N_OBJECT_UNREF (state->title);
  if (state->map)
    free_map (state->map, 1);
  free (state);
}

/** Load a state from PLIST into a newly allocated state object.
    PLIST has this form:
      PLIST ::= ( STATE-NAME STATE-TITLE ? BRANCH * )
      BRANCH ::= ( MAP-NAME BRANCH-ACTION * )
   Return the state object.  */

static MIMState *
load_state (MInputMethodInfo *im_info, MPlist *plist)
{
  MIMState *state;

  if (MFAILP (MPLIST_SYMBOL_P (plist)))
    return NULL;
  M17N_OBJECT (state, free_state, MERROR_IM);
  state->name = MPLIST_SYMBOL (plist);
  plist = MPLIST_NEXT (plist);
  if (MPLIST_MTEXT_P (plist))
    {
      state->title = MPLIST_MTEXT (plist);
      mtext_put_prop (state->title, 0, mtext_nchars (state->title),
		      Mlanguage, im_info->language);
      M17N_OBJECT_REF (state->title);
      plist = MPLIST_NEXT (plist);
    }
  MSTRUCT_CALLOC (state->map, MERROR_IM);
  MPLIST_DO (plist, plist)
    {
      if (MFAILP (MPLIST_PLIST_P (plist)))
	continue;
      load_branch (im_info, MPLIST_PLIST (plist), state->map);
    }
  return state;
}

/* Return a newly created IM_INFO for an input method specified by
   LANUAGE, NAME, and EXTRA.  IM_INFO is stored in PLIST.  */

static MInputMethodInfo *
new_im_info (MDatabase *mdb, MSymbol language, MSymbol name, MSymbol extra,
	     MPlist *plist)
{
  MInputMethodInfo *im_info;
  MPlist *elt;

  if (name == Mnil && extra == Mnil)
    language = Mt, extra = Mglobal;
  MSTRUCT_CALLOC (im_info, MERROR_IM);
  im_info->mdb = mdb;
  im_info->language = language;
  im_info->name = name;
  im_info->extra = extra;

  elt = mplist ();
  mplist_add (plist, Mplist, elt);
  M17N_OBJECT_UNREF (elt);
  elt = mplist_add (elt, Msymbol, language);
  elt = mplist_add (elt, Msymbol, name);
  elt = mplist_add (elt, Msymbol, extra);
  mplist_add (elt, Mt, im_info);

  return im_info;
}

static void
fini_im_info (MInputMethodInfo *im_info)
{
  MPlist *plist;

  M17N_OBJECT_UNREF (im_info->cmds);
  M17N_OBJECT_UNREF (im_info->configured_cmds);
  M17N_OBJECT_UNREF (im_info->bc_cmds);
  M17N_OBJECT_UNREF (im_info->vars);
  M17N_OBJECT_UNREF (im_info->configured_vars);
  M17N_OBJECT_UNREF (im_info->bc_vars);
  M17N_OBJECT_UNREF (im_info->description);
  M17N_OBJECT_UNREF (im_info->title);
  if (im_info->states)
    {
      MPLIST_DO (plist, im_info->states)
	{
	  MIMState *state = (MIMState *) MPLIST_VAL (plist);

	  M17N_OBJECT_UNREF (state);
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
  if (im_info->maps)
    {
      MPLIST_DO (plist, im_info->maps)
	{
	  MPlist *p = MPLIST_PLIST (plist);

	  M17N_OBJECT_UNREF (p);
	}
      M17N_OBJECT_UNREF (im_info->maps);
    }

  im_info->tick = 0;
}

static void
free_im_info (MInputMethodInfo *im_info)
{
  fini_im_info (im_info);
  free (im_info);
}

static void
free_im_list (MPlist *plist)
{
  MPlist *pl, *elt;

  MPLIST_DO (pl, plist)
    {
      MInputMethodInfo *im_info;

      elt = MPLIST_NEXT (MPLIST_NEXT (MPLIST_NEXT (MPLIST_PLIST (pl))));
      im_info = MPLIST_VAL (elt);
      free_im_info (im_info);
    }
  M17N_OBJECT_UNREF (plist);
}

static MInputMethodInfo *
lookup_im_info (MPlist *plist, MSymbol language, MSymbol name, MSymbol extra)
{
  if (name == Mnil && extra == Mnil)
    language = Mt, extra = Mglobal;
  while ((plist = mplist__assq (plist, language)))
    {
      MPlist *elt = MPLIST_PLIST (plist);

      plist = MPLIST_NEXT (plist);
      elt = MPLIST_NEXT (elt);
      if (MPLIST_SYMBOL (elt) != name)
	continue;
      elt = MPLIST_NEXT (elt);
      if (MPLIST_SYMBOL (elt) != extra)
	continue;
      elt = MPLIST_NEXT (elt);
      return MPLIST_VAL (elt);
    }
  return NULL;
}

static void load_im_info (MPlist *, MInputMethodInfo *);

#define get_custom_info(im_info)				\
  (im_custom_list						\
   ? lookup_im_info (im_custom_list, (im_info)->language,	\
		     (im_info)->name, (im_info)->extra)		\
   : NULL)

#define get_config_info(im_info)				\
  (im_config_list						\
   ? lookup_im_info (im_config_list, (im_info)->language,	\
		     (im_info)->name, (im_info)->extra)		\
   : NULL)

static int
update_custom_info (void)
{
  MPlist *plist, *pl;

  if (im_custom_mdb)
    {
      if (mdatabase__check (im_custom_mdb) > 0)
	return 1;
    }
  else
    {
      MDatabaseInfo *custom_dir_info;
      char custom_path[PATH_MAX + 1];

      custom_dir_info = MPLIST_VAL (mdatabase__dir_list);
      if (! custom_dir_info->filename
	  || custom_dir_info->len + strlen (CUSTOM_FILE) > PATH_MAX)
	return -1;
      strcpy (custom_path, custom_dir_info->filename);
      strcat (custom_path, CUSTOM_FILE);
      im_custom_mdb = mdatabase_define (Minput_method, Mt, Mnil, Mconfig,
					NULL, custom_path);
    }

  if (im_custom_list)
    {
      free_im_list (im_custom_list);
      im_custom_list = NULL;
    }
  plist = mdatabase_load (im_custom_mdb);
  if (! plist)
    return -1;
  im_custom_list = mplist ();

  MPLIST_DO (pl, plist)
    {
      MSymbol language, name, extra;
      MInputMethodInfo *im_info;
      MPlist *im_data, *p;

      if (! MPLIST_PLIST_P (pl))
	continue;
      p = MPLIST_PLIST (pl);
      im_data = MPLIST_NEXT (p);
      if (! MPLIST_PLIST_P (p))
	continue;
      p = MPLIST_PLIST (p);
      if (! MPLIST_SYMBOL_P (p)
	  || MPLIST_SYMBOL (p) != Minput_method)
	continue;
      p = MPLIST_NEXT (p);
      if (! MPLIST_SYMBOL_P (p))
	continue;
      language = MPLIST_SYMBOL (p);
      p = MPLIST_NEXT (p);
      if (! MPLIST_SYMBOL_P (p))
	continue;
      name = MPLIST_SYMBOL (p);
      p = MPLIST_NEXT (p);
      if (MPLIST_TAIL_P (p))
	extra = Mnil;
      else if (MPLIST_SYMBOL_P (p))
	extra = MPLIST_SYMBOL (p);
      if (language == Mnil || (name == Mnil && extra == Mnil))
	continue;
      im_info = new_im_info (NULL, language, name, extra, im_custom_list);
      load_im_info (im_data, im_info);
    }
  M17N_OBJECT_UNREF (plist);
  return 0;
}

static int
update_global_info (void)
{
  MPlist *plist;

  if (global_info)
    {
      int ret = mdatabase__check (global_info->mdb);

      if (ret)
	return ret;
      fini_im_info (global_info);
    }
  else
    {
      MDatabase *mdb = mdatabase_find (Minput_method, Mt, Mnil, Mglobal);

      if (! mdb)
	return -1;
      global_info = new_im_info (mdb, Mt, Mnil, Mglobal, im_info_list);
    }
  if (! global_info->mdb
      || ! (plist = mdatabase_load (global_info->mdb)))
    return -1;

  load_im_info (plist, global_info);
  M17N_OBJECT_UNREF (plist);
  return 0;
}


/* Return an IM_INFO for the an method specified by LANGUAGE, NAME,
   and EXTRA.  KEY, if not Mnil, tells which kind of information about
   the input method is necessary, and the returned IM_INFO may contain
   only that information.  */

static MInputMethodInfo *
get_im_info (MSymbol language, MSymbol name, MSymbol extra, MSymbol key)
{
  MPlist *plist;
  MInputMethodInfo *im_info;
  MDatabase *mdb;

  if (name == Mnil && extra == Mnil)
    language = Mt, extra = Mglobal;
  im_info = lookup_im_info (im_info_list, language, name, extra);
  if (im_info)
    {
      if (key == Mnil ? im_info->states != NULL
	  : key == Mcommand ? im_info->cmds != NULL
	  : key == Mvariable ? im_info->vars != NULL
	  : key == Mtitle ? im_info->title != NULL
	  : key == Mdescription ? im_info->description != NULL
	  : 1)
	/* IM_INFO already contains required information.  */
	return im_info;
      /* We have not yet loaded required information.  */
    }
  else
    {
      mdb = mdatabase_find (Minput_method, language, name, extra);
      if (! mdb)
	return NULL;
      im_info = new_im_info (mdb, language, name, extra, im_info_list);
    }

  if (key == Mnil)
    {
      plist = mdatabase_load (im_info->mdb);
    }
  else
    {
      mplist_push (load_im_info_keys, key, Mt);
      plist = mdatabase__load_for_keys (im_info->mdb, load_im_info_keys);
      mplist_pop (load_im_info_keys);
    }
  im_info->tick = 0;
  if (! plist)
    MERROR (MERROR_IM, im_info);
  update_global_info ();
  load_im_info (plist, im_info);
  M17N_OBJECT_UNREF (plist);
  if (key == Mnil)
    {
      if (! im_info->cmds)
	im_info->cmds = mplist ();
      if (! im_info->vars)
	im_info->vars = mplist ();
      if (! im_info->states)
	im_info->states = mplist ();
    }
  if (! im_info->title
      && (key == Mnil || key == Mtitle))
    im_info->title = (name == Mnil ? mtext ()
		      : mtext_from_data (MSYMBOL_NAME (name),
					 MSYMBOL_NAMELEN (name),
					 MTEXT_FORMAT_US_ASCII));
  return im_info;
}

/* Check if IM_INFO->mdb is updated or not.  If not updated, return 0.
   If updated, but got unloadable, return -1.  Otherwise, update
   contents of IM_INFO from the new database, and return 1.  */

static int
reload_im_info (MInputMethodInfo *im_info)
{
  int check;
  MPlist *plist;

  update_custom_info ();
  update_global_info ();
  check = mdatabase__check (im_info->mdb);
  if (check < 0)
    return -1;
  plist = mdatabase_load (im_info->mdb);
  if (! plist)
    return -1;
  fini_im_info (im_info);
  load_im_info (plist, im_info);
  M17N_OBJECT_UNREF (plist);
  if (! im_info->cmds)
    im_info->cmds = mplist ();
  if (! im_info->vars)
    im_info->vars = mplist ();
  if (! im_info->title)
    {
      MSymbol name = im_info->name;

      im_info->title = (name == Mnil ? mtext ()
			: mtext_from_data (MSYMBOL_NAME (name),
					   MSYMBOL_NAMELEN (name),
					   MTEXT_FORMAT_US_ASCII));
    }
  return 1;
}

static MInputMethodInfo *
get_im_info_by_tags (MPlist *plist)
{
  MSymbol tag[3];
  int i;

  for (i = 0; i < 3 && MPLIST_SYMBOL_P (plist);
       i++, plist = MPLIST_NEXT (plist))
    tag[i] = MPLIST_SYMBOL (plist);
  if (i < 2)
    return NULL;
  for (; i < 3; i++)
    tag[i] = Mnil;
  return get_im_info (tag[0], tag[1], tag[2], Mnil);
}


static int
check_description (MPlist *plist)
{
  MText *mt;

  if (MPLIST_MTEXT_P (plist))
    return 1;
  if (MPLIST_PLIST_P (plist))
    {
      MPlist *pl = MPLIST_PLIST (plist);

      if (MFAILP (MPLIST_SYMBOL_P (pl) && MPLIST_SYMBOL (pl) == M_gettext))
	return 0;
      pl =MPLIST_NEXT (pl);
      if (MFAILP (MPLIST_MTEXT_P (pl)))
	return 0;
      mt = MPLIST_MTEXT (pl);
      M17N_OBJECT_REF (mt);
#if ENABLE_NLS
      {
	char *translated = dgettext ("m17n-db", (char *) MTEXT_DATA (mt));

	if (translated == (char *) MTEXT_DATA (mt))
	  translated = dgettext ("m17n-contrib", (char *) MTEXT_DATA (mt));
	if (translated != (char *) MTEXT_DATA (mt))
	  {
	    M17N_OBJECT_UNREF (mt);
	    mt = mtext__from_data (translated, strlen (translated),
				   MTEXT_FORMAT_UTF_8, 1);
	  }
      }
#endif
      mplist_set (plist, Mtext, mt);
      M17N_OBJECT_UNREF (mt);
      return 1;
    }
  if (MFAILP (MPLIST_SYMBOL_P (plist) && MPLIST_SYMBOL (plist) == Mnil))
    return 0;
  return 1;
}


/* Check KEYSEQ, and return 1 if it is valid as a key sequence, return
   0 if not.  */

static int
check_command_keyseq (MPlist *keyseq)
{
  if (MPLIST_PLIST_P (keyseq))
    {
      MPlist *p = MPLIST_PLIST (keyseq);

      MPLIST_DO (p, p)
	if (! MPLIST_SYMBOL_P (p) && ! MPLIST_INTEGER_P (p))
	  return 0;
      return 1;
    }
  if (MPLIST_MTEXT_P (keyseq))
    {
      MText *mt = MPLIST_MTEXT (keyseq);
      int i;
      
      for (i = 0; i < mtext_nchars (mt); i++)
	if (mtext_ref_char (mt, i) >= 256)
	  return 0;
      return 1;
    }
  return 0;
}

/* Load command defitions from PLIST into IM_INFO->cmds.

   PLIST is well-formed and has this form;
     (command (NAME [DESCRIPTION KEYSEQ ...]) ...)
   NAME is a symbol.  DESCRIPTION is an M-text or `nil'.  KEYSEQ is an
   M-text or a plist of symbols.

   The returned list has the same form, but for each element...

   (1) If DESCRIPTION and the rest are omitted, the element is not
   stored in the returned list.

   (2) If DESCRIPTION is nil, it is complemented by the corresponding
   description in global_info->cmds (if any).  */

static void
load_commands (MInputMethodInfo *im_info, MPlist *plist)
{
  MPlist *tail;

  im_info->cmds = tail = mplist ();

  MPLIST_DO (plist, MPLIST_NEXT (plist))
    {
      /* PLIST ::= ((NAME DESC KEYSEQ ...) ...) */
      MPlist *pl, *p;

      if (MFAILP (MPLIST_PLIST_P (plist)))
	continue;
      pl = MPLIST_PLIST (plist); /* PL ::= (NAME DESC KEYSEQ ...) */
      if (MFAILP (MPLIST_SYMBOL_P (pl)))
	continue;
      p = MPLIST_NEXT (pl);	/* P ::= (DESC KEYSEQ ...) */
      if (MPLIST_TAIL_P (p))	/* PL ::= (NAME) */
	{
	  if (MFAILP (im_info != global_info))
	    mplist_add (p, Msymbol, Mnil); /* PL ::= (NAME nil) */
	}
      else
	{
	  if (! check_description (p))
	    mplist_set (p, Msymbol, Mnil);
	  p = MPLIST_NEXT (p);
	  while (! MPLIST_TAIL_P (p))
	    {
	      if (MFAILP (check_command_keyseq (p)))
		mplist__pop_unref (p);
	      else
		p = MPLIST_NEXT (p);
	    }
	}
      tail = mplist_add (tail, Mplist, pl);
    }
}

static MPlist *
config_command (MPlist *plist, MPlist *global_cmds, MPlist *custom_cmds,
		MPlist *config_cmds)
{
  MPlist *global = NULL, *custom = NULL, *config = NULL;
  MSymbol name = MPLIST_SYMBOL (plist);
  MSymbol status;
  MPlist *description, *keyseq;

  if (global_cmds && (global = mplist__assq (global_cmds, name)))
    global = MPLIST_NEXT (MPLIST_PLIST (global));  

  plist = MPLIST_NEXT (plist);
  if (MPLIST_MTEXT_P (plist) || MPLIST_PLIST_P (plist))
    {
      description = plist;
      plist = MPLIST_NEXT (plist);
    }
  else
    {
      description = global;
      if (! MPLIST_TAIL_P (plist))
	plist = MPLIST_NEXT (plist);
    }
  if (MPLIST_TAIL_P (plist) && global)
    {
      keyseq = MPLIST_NEXT (global);
      status = Minherited;
    }
  else
    {
      keyseq = plist;
      status = Mnil;
    }

  if (config_cmds && (config = mplist__assq (config_cmds, name)))
    {
      status = Mconfigured;
      config = MPLIST_NEXT (MPLIST_PLIST (config));
      if (! MPLIST_TAIL_P (config))
	keyseq = config;
    }
  else if (custom_cmds && (custom = mplist__assq (custom_cmds, name)))
    {
      MPlist *this_keyseq = MPLIST_NEXT (MPLIST_NEXT (MPLIST_PLIST (custom)));

      if (MPLIST_TAIL_P (this_keyseq))
	mplist__pop_unref (custom);
      else
	{
	  status = Mcustomized;
	  keyseq = this_keyseq;
	}
    }
  
  plist = mplist ();
  mplist_add (plist, Msymbol, name);
  if (description)
    mplist_add (plist, MPLIST_KEY (description), MPLIST_VAL (description));
  else
    mplist_add (plist, Msymbol, Mnil);
  mplist_add (plist, Msymbol, status);
  mplist__conc (plist, keyseq);
  return plist;
}

static void
config_all_commands (MInputMethodInfo *im_info)
{
  MPlist *global_cmds, *custom_cmds, *config_cmds;
  MInputMethodInfo *temp;
  MPlist *tail, *plist;

  M17N_OBJECT_UNREF (im_info->configured_cmds);

  if (MPLIST_TAIL_P (im_info->cmds)
      || ! im_info->mdb)
    return;

  global_cmds = im_info != global_info ? global_info->cmds : NULL;
  custom_cmds = ((temp = get_custom_info (im_info)) ? temp->cmds : NULL);
  config_cmds = ((temp = get_config_info (im_info)) ? temp->cmds : NULL);

  im_info->configured_cmds = tail = mplist ();
  MPLIST_DO (plist, im_info->cmds)
    {
      MPlist *pl = config_command (MPLIST_PLIST (plist),
				   global_cmds, custom_cmds, config_cmds);
      if (pl)
	{
	  tail = mplist_add (tail, Mplist, pl);
	  M17N_OBJECT_UNREF (pl);
	}
    }
}

/* Check VAL's value against VALID_VALUES, and return 1 if it is
   valid, return 0 if not.  */

static int
check_variable_value (MPlist *val, MPlist *global)
{
  MSymbol type = MPLIST_KEY (val);
  MPlist *valids = MPLIST_NEXT (val);

  if (type != Minteger && type != Mtext && type != Msymbol)
    return 0;
  if (global)
    {
      if (MPLIST_KEY (global) != Mt
	  && MPLIST_KEY (global) != MPLIST_KEY (val))
	return 0;
      if (MPLIST_TAIL_P (valids))
	valids = MPLIST_NEXT (global);
    }
  if (MPLIST_TAIL_P (valids))
    return 1;

  if (type == Minteger)
    {
      int n = MPLIST_INTEGER (val);

      MPLIST_DO (valids, valids)
	{
	  if (MPLIST_INTEGER_P (valids))
	    {
	      if (n == MPLIST_INTEGER (valids))
		break;
	    }
	  else if (MPLIST_PLIST_P (valids))
	    {
	      MPlist *p = MPLIST_PLIST (valids);
	      int min_bound, max_bound;

	      if (! MPLIST_INTEGER_P (p))
		MERROR (MERROR_IM, 0);
	      min_bound = MPLIST_INTEGER (p);
	      p = MPLIST_NEXT (p);
	      if (! MPLIST_INTEGER_P (p))
		MERROR (MERROR_IM, 0);
	      max_bound = MPLIST_INTEGER (p);
	      if (n >= min_bound && n <= max_bound)
		break;
	    }
	}
    }
  else if (type == Msymbol)
    {
      MSymbol sym = MPLIST_SYMBOL (val);

      MPLIST_DO (valids, valids)
	{
	  if (! MPLIST_SYMBOL_P (valids))
	    MERROR (MERROR_IM, 0);
	  if (sym == MPLIST_SYMBOL (valids))
	    break;
	}
    }
  else
    {
      MText *mt = MPLIST_MTEXT (val);

      MPLIST_DO (valids, valids)
	{
	  if (! MPLIST_MTEXT_P (valids))
	    MERROR (MERROR_IM, 0);
	  if (mtext_cmp (mt, MPLIST_MTEXT (valids)) == 0)
	    break;
	}
    }

  return (! MPLIST_TAIL_P (valids));
}

/* Load variable defitions from PLIST into IM_INFO->vars.

   PLIST is well-formed and has this form;
     ((NAME [DESCRIPTION DEFAULT-VALUE VALID-VALUE ...])
      ...)
   NAME is a symbol.  DESCRIPTION is an M-text or `nil'.

   The returned list has the same form, but for each element...

   (1) If DESCRIPTION and the rest are omitted, the element is not
   stored in the returned list.

   (2) If DESCRIPTION is nil, it is complemented by the corresponding
   description in global_info->vars (if any).  */

static void
load_variables (MInputMethodInfo *im_info, MPlist *plist)
{
  MPlist *global_vars = ((im_info->mdb && im_info != global_info)
			 ? global_info->vars : NULL);
  MPlist *tail;

  im_info->vars = tail = mplist ();
  MPLIST_DO (plist, MPLIST_NEXT (plist))
    {
      MPlist *pl, *p;

      if (MFAILP (MPLIST_PLIST_P (plist)))
	continue;
      pl = MPLIST_PLIST (plist); /* PL ::= (NAME DESC VALUE VALID ...) */
      if (MFAILP (MPLIST_SYMBOL_P (pl)))
	continue;
      if (im_info == global_info)
	{
	  /* Loading a global variable.  */
	  p = MPLIST_NEXT (pl);
	  if (MPLIST_TAIL_P (p))
	    mplist_add (p, Msymbol, Mnil);
	  else
	    {
	      if (! check_description (p))
		mplist_set (p, Msymbol, Mnil);
	      p = MPLIST_NEXT (p);
	      if (MFAILP (! MPLIST_TAIL_P (p)
			  && check_variable_value (p, NULL)))
		mplist_set (p, Mt, NULL);
	    }
	}
      else if (im_info->mdb)
	{
	  /* Loading a local variable.  */
	  MSymbol name = MPLIST_SYMBOL (pl);
	  MPlist *global = NULL;

	  if (global_vars
	      && (p = mplist__assq (global_vars, name)))
	    {
	      /* P ::= ((NAME DESC ...) ...) */
	      p = MPLIST_PLIST (p); /* P ::= (NAME DESC ...) */
	      global = MPLIST_NEXT (p); /* P ::= (DESC VALUE ...) */
	      global = MPLIST_NEXT (global); /* P ::= (VALUE ...) */
	    }

	  p = MPLIST_NEXT (pl);	/* P ::= (DESC VALUE VALID ...) */
	  if (! MPLIST_TAIL_P (p))
	    {
	      if (! check_description (p))
		mplist_set (p, Msymbol, Mnil);
	      p = MPLIST_NEXT (p); /* P ::= (VALUE VALID ...) */
	      if (MFAILP (! MPLIST_TAIL_P (p)))
		mplist_set (p, Mt, NULL);
	      else
		{
		  MPlist *valid_values = MPLIST_NEXT (p);

		  if (! MPLIST_TAIL_P (valid_values)
		      ? MFAILP (check_variable_value (p, NULL))
		      : global && MFAILP (check_variable_value (p, global)))
		    mplist_set (p, Mt, NULL);
		}
	    }
	}
      else
	{
	  /* Loading a variable customization.  */
	  p = MPLIST_NEXT (pl);	/* P ::= (nil VALUE) */
	  if (MFAILP (! MPLIST_TAIL_P (p)))
	    continue;
	  p = MPLIST_NEXT (p);	/* P ::= (VALUE) */
	  if (MFAILP (MPLIST_INTEGER_P (p) || MPLIST_SYMBOL_P (p)
		      || MPLIST_MTEXT_P (p)))
	    continue;
	}
      tail = mplist_add (tail, Mplist, pl);
    }
}

static MPlist *
config_variable (MPlist *plist, MPlist *global_vars, MPlist *custom_vars,
		 MPlist *config_vars)
{
  MPlist *global = NULL, *custom = NULL, *config = NULL;
  MSymbol name = MPLIST_SYMBOL (plist);
  MSymbol status;
  MPlist *description = NULL, *value, *valids;

  if (global_vars)
    {
      global = mplist__assq (global_vars, name);
      if (global)
	global = MPLIST_NEXT (MPLIST_PLIST (global)); /* (DESC VALUE ...) */
    }

  plist = MPLIST_NEXT (plist);
  if (MPLIST_MTEXT_P (plist) || MPLIST_PLIST_P (plist))
    description = plist;
  else if (global)
    description = global;
  if (global)
    global = MPLIST_NEXT (global); /* (VALUE VALIDS ...) */

  if (MPLIST_TAIL_P (plist))
    {
      /* Inherit from global (if any).  */
      if (global)
	{
	  value = global;
	  if (MPLIST_KEY (value) == Mt)
	    value = NULL;
	  valids = MPLIST_NEXT (global);
	  status = Minherited;
	}
      else
	{
	  value = NULL;
	  valids = NULL;
	  status = Mnil;
	  plist = NULL;
	}
    }
  else
    {
      value = plist = MPLIST_NEXT (plist);
      valids = MPLIST_NEXT (value);
      if (MPLIST_KEY (value) == Mt)
	value = NULL;
      if (! MPLIST_TAIL_P (valids))
	global = NULL;
      else if (global)
	valids = MPLIST_NEXT (global);
      status = Mnil;
    }

  if (config_vars && (config = mplist__assq (config_vars, name)))
    {
      status = Mconfigured;
      config = MPLIST_NEXT (MPLIST_PLIST (config));
      if (! MPLIST_TAIL_P (config))
	{
	  value = config;
	  if (MFAILP (check_variable_value (value, global ? global : plist)))
	    value = NULL;
	}
    }
  else if (custom_vars && (custom = mplist__assq (custom_vars, name)))
    {
      MPlist *this_value = MPLIST_NEXT (MPLIST_NEXT (MPLIST_PLIST (custom)));

      if (MPLIST_TAIL_P (this_value))
	mplist__pop_unref (custom);
      else
	{
	  value = this_value;
	  if (MFAILP (check_variable_value (value, global ? global : plist)))
	    value = NULL;
	  status = Mcustomized;
	}
    }
  
  plist = mplist ();
  mplist_add (plist, Msymbol, name);
  if (description)
    mplist_add (plist, MPLIST_KEY (description), MPLIST_VAL (description));
  else
    mplist_add (plist, Msymbol, Mnil);
  mplist_add (plist, Msymbol, status);
  if (value)
    mplist_add (plist, MPLIST_KEY (value), MPLIST_VAL (value));
  else
    mplist_add (plist, Mt, NULL);
  if (valids && ! MPLIST_TAIL_P (valids))
    mplist__conc (plist, valids);
  return plist;
}

/* Return a configured variable definition list based on
   IM_INFO->vars.  If a variable in it doesn't contain a value, try to
   get it from global_info->vars.  */

static void
config_all_variables (MInputMethodInfo *im_info)
{
  MPlist *global_vars, *custom_vars, *config_vars;
  MInputMethodInfo *temp;
  MPlist *tail, *plist;

  M17N_OBJECT_UNREF (im_info->configured_vars);

  if (MPLIST_TAIL_P (im_info->vars)
      || ! im_info->mdb)
    return;

  global_vars = im_info != global_info ? global_info->vars : NULL;
  custom_vars = ((temp = get_custom_info (im_info)) ? temp->vars : NULL);
  config_vars = ((temp = get_config_info (im_info)) ? temp->vars : NULL);

  im_info->configured_vars = tail = mplist ();
  MPLIST_DO (plist, im_info->vars)
    {
      MPlist *pl = config_variable (MPLIST_PLIST (plist),
				    global_vars, custom_vars, config_vars);
      if (pl)
	{
	  tail = mplist_add (tail, Mplist, pl);
	  M17N_OBJECT_UNREF (pl);
	}
    }
}

/* Load an input method (LANGUAGE NAME) from PLIST into IM_INFO.
   CONFIG contains configuration information of the input method.  */

static void
load_im_info (MPlist *plist, MInputMethodInfo *im_info)
{
  MPlist *pl, *p;

  if (! im_info->cmds && (pl = mplist__assq (plist, Mcommand)))
    {
      load_commands (im_info, MPLIST_PLIST (pl));
      config_all_commands (im_info);
      pl = mplist_pop (pl);
      M17N_OBJECT_UNREF (pl);
    }

  if (! im_info->vars && (pl = mplist__assq (plist, Mvariable)))
    {
      load_variables (im_info, MPLIST_PLIST (pl));
      config_all_variables (im_info);
      pl = mplist_pop (pl);
      M17N_OBJECT_UNREF (pl);
    }

  MPLIST_DO (plist, plist)
    if (MPLIST_PLIST_P (plist))
      {
	MPlist *elt = MPLIST_PLIST (plist);
	MSymbol key;

	if (MFAILP (MPLIST_SYMBOL_P (elt)))
	  continue;
	key = MPLIST_SYMBOL (elt);
	if (key == Mtitle)
	  {
	    if (im_info->title)
	      continue;
	    elt = MPLIST_NEXT (elt);
	    if (MFAILP (MPLIST_MTEXT_P (elt)))
	      continue;
	    im_info->title = MPLIST_MTEXT (elt);
	    M17N_OBJECT_REF (im_info->title);
	  }
	else if (key == Mmap)
	  {
	    pl = mplist__from_alist (MPLIST_NEXT (elt));
	    if (MFAILP (pl))
	      continue;
	    if (! im_info->maps)
	      im_info->maps = pl;
	    else
	      {
		mplist__conc (im_info->maps, pl);
		M17N_OBJECT_UNREF (pl);
	      }
	  }
	else if (key == Mmacro)
	  {
	    if (! im_info->macros)
	      im_info->macros = mplist ();
	    MPLIST_DO (elt, MPLIST_NEXT (elt))
	      {
		if (MFAILP (MPLIST_PLIST_P (elt)))
		  continue;
		load_macros (im_info, MPLIST_PLIST (elt));
	      }
	  }
	else if (key == Mmodule)
	  {
	    if (! im_info->externals)
	      im_info->externals = mplist ();
	    MPLIST_DO (elt, MPLIST_NEXT (elt))
	      {
		if (MFAILP (MPLIST_PLIST_P (elt)))
		  continue;
		load_external_module (im_info, MPLIST_PLIST (elt));
	      }
	  }
	else if (key == Mstate)
	  {
	    MPLIST_DO (elt, MPLIST_NEXT (elt))
	      {
		MIMState *state;

		if (MFAILP (MPLIST_PLIST_P (elt)))
		  continue;
		pl = MPLIST_PLIST (elt);
		if (! im_info->states)
		  im_info->states = mplist ();
		state = load_state (im_info, MPLIST_PLIST (elt));
		if (MFAILP (state))
		  continue;
		mplist_put (im_info->states, state->name, state);
	      }
	  }
	else if (key == Minclude)
	  {
	    /* elt ::= include (tag1 tag2 ...) key item ... */
	    MSymbol key;
	    MInputMethodInfo *temp;

	    elt = MPLIST_NEXT (elt);
	    if (MFAILP (MPLIST_PLIST_P (elt)))
	      continue;
	    temp = get_im_info_by_tags (MPLIST_PLIST (elt));
	    if (MFAILP (temp))
	      continue;
	    elt = MPLIST_NEXT (elt);
	    if (MFAILP (MPLIST_SYMBOL_P (elt)))
	      continue;
	    key = MPLIST_SYMBOL (elt);
	    elt = MPLIST_NEXT (elt);
	    if (key == Mmap)
	      {
		if (! temp->maps || MPLIST_TAIL_P (temp->maps))
		  continue;
		if (! im_info->maps)
		  im_info->maps = mplist ();
		MPLIST_DO (pl, temp->maps)
		  {
		    p = MPLIST_VAL (pl);
		    MPLIST_ADD_PLIST (im_info->maps, MPLIST_KEY (pl), p);
		    M17N_OBJECT_REF (p);
		  }
	      }
	    else if (key == Mmacro)
	      {
		if (! temp->macros || MPLIST_TAIL_P (temp->macros))
		  continue;
		if (! im_info->macros)
		  im_info->macros = mplist ();
		MPLIST_DO (pl, temp->macros)
		  {
		    p = MPLIST_VAL (pl);
		    MPLIST_ADD_PLIST (im_info->macros, MPLIST_KEY (pl), p);
		    M17N_OBJECT_REF (p);
		  }
	      }
	    else if (key == Mstate)
	      {
		if (! temp->states || MPLIST_TAIL_P (temp->states))
		  continue;
		if (! im_info->states)
		  im_info->states = mplist ();
		MPLIST_DO (pl, temp->states)
		  {
		    MIMState *state = MPLIST_VAL (pl);

		    mplist_add (im_info->states, MPLIST_KEY (pl), state);
		    M17N_OBJECT_REF (state);
		  }
	      }
	  }
	else if (key == Mdescription)
	  {
	    if (im_info->description)
	      continue;
	    elt = MPLIST_NEXT (elt);
	    if (! check_description (elt))
	      continue;
	    im_info->description = MPLIST_MTEXT (elt);
	    M17N_OBJECT_REF (im_info->description);
	  }
      }
  if (im_info->macros)
    {
      MPLIST_DO (pl, im_info->macros)
	parse_action_list (MPLIST_PLIST (pl), im_info->macros);
    }

  im_info->tick = time (NULL);
}



static int take_action_list (MInputContext *ic, MPlist *action_list);
static void preedit_commit (MInputContext *ic, int need_prefix);

static void
shift_state (MInputContext *ic, MSymbol state_name)
{
  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  MIMState *orig_state = ic_info->state, *state;

  /* Find a state to shift to.  If not found, shift to the initial
     state.  */
  if (state_name == Mt)
    {
      if (! ic_info->prev_state)
	return;
      state = ic_info->prev_state;
    }
  else if (state_name == Mnil)
    {
      state = (MIMState *) MPLIST_VAL (im_info->states);
    }
  else
    {
      state = (MIMState *) mplist_get (im_info->states, state_name);
      if (! state)
	state = (MIMState *) MPLIST_VAL (im_info->states);
    }

  if (MDEBUG_FLAG ())
    {
      if (orig_state)
	MDEBUG_PRINT2 ("\n  [IM] [%s] (shift %s)\n",
		       MSYMBOL_NAME (orig_state->name),
		       MSYMBOL_NAME (state->name));
      else
	MDEBUG_PRINT1 (" (shift %s)\n", MSYMBOL_NAME (state->name));
    }

  /* Enter the new state.  */
  ic_info->state = state;
  ic_info->map = state->map;
  ic_info->state_key_head = ic_info->key_head;
  if (state == (MIMState *) MPLIST_VAL (im_info->states)
      && orig_state)
    /* We have shifted to the initial state.  */
    preedit_commit (ic, 0);
  mtext_cpy (ic_info->preedit_saved, ic->preedit);
  ic_info->state_pos = ic->cursor_pos;
  if (state != orig_state)
    {
      if (state == (MIMState *) MPLIST_VAL (im_info->states))
	{
	  /* Shifted to the initial state.  */
	  ic_info->prev_state = NULL;
	  M17N_OBJECT_UNREF (ic_info->vars_saved);
	  ic_info->vars_saved = mplist_copy (ic_info->vars);
	}
      else
	ic_info->prev_state = orig_state;

      if (state->title)
	ic->status = state->title;
      else
	ic->status = im_info->title;
      ic->status_changed = 1;
      if (ic_info->map == ic_info->state->map
	  && ic_info->map->map_actions)
	{
	  MDEBUG_PRINT1 ("  [IM] [%s] init-actions:",
			 MSYMBOL_NAME (state->name));
	  take_action_list (ic, ic_info->map->map_actions);
	}
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

/* Adjust markers for the change of preedit text.
   If FROM == TO, the change is insertion of INS chars.
   If FROM < TO and INS == 0, the change is deletion of the range.
   If FROM < TO and INS > 0, the change is replacement.  */

static void
adjust_markers (MInputContext *ic, int from, int to, int ins)
{
  MInputContextInfo *ic_info = ((MInputContext *) ic)->info;
  MPlist *markers;

  if (from == to)
    {
      MPLIST_DO (markers, ic_info->markers)
	if (MPLIST_INTEGER (markers) > from)
	  MPLIST_VAL (markers) = (void *) (MPLIST_INTEGER (markers) + ins);
      if (ic->cursor_pos >= from)
	ic->cursor_pos += ins;
    }
  else
    {
      MPLIST_DO (markers, ic_info->markers)
	{
	  if (MPLIST_INTEGER (markers) >= to)
	    MPLIST_VAL (markers)
	      = (void *) (MPLIST_INTEGER (markers) + ins - (to - from));
	  else if (MPLIST_INTEGER (markers) > from)
	    MPLIST_VAL (markers) = (void *) from;
	}
      if (ic->cursor_pos >= to)
	ic->cursor_pos += ins - (to - from);
      else if (ic->cursor_pos > from)
	ic->cursor_pos = from;
    }
}


static void
preedit_insert (MInputContext *ic, int pos, MText *mt, int c)
{
  int nchars = mt ? mtext_nchars (mt) : 1;

  if (mt)
    {
      mtext_ins (ic->preedit, pos, mt);
      MDEBUG_PRINT1 ("(\"%s\")", MTEXT_DATA (mt));
    }
  else
    {
      mtext_ins_char (ic->preedit, pos, c, 1);
      if (c < 0x7F)
	MDEBUG_PRINT1 ("('%c')", c);
      else
	MDEBUG_PRINT1 ("(U+%04X)", c);
    }
  adjust_markers (ic, pos, pos, nchars);
  ic->preedit_changed = 1;
}


static void
preedit_delete (MInputContext *ic, int from, int to)
{
  mtext_del (ic->preedit, from, to);
  adjust_markers (ic, from, to, 0);
  ic->preedit_changed = 1;
}

static void
preedit_replace (MInputContext *ic, int from, int to, MText *mt, int c)
{
  int ins;

  mtext_del (ic->preedit, from, to);
  if (mt)
    {
      mtext_ins (ic->preedit, from, mt);
      ins = mtext_nchars (mt);
    }
  else
    {
      mtext_ins_char (ic->preedit, from, c, 1);
      ins = 1;
    }
  adjust_markers (ic, from, to, ins);
  ic->preedit_changed = 1;
}


static void
preedit_commit (MInputContext *ic, int need_prefix)
{
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  int preedit_len = mtext_nchars (ic->preedit);

  if (preedit_len > 0)
    {
      MPlist *p;

      mtext_put_prop_values (ic->preedit, 0, mtext_nchars (ic->preedit),
			     Mcandidate_list, NULL, 0);
      mtext_put_prop_values (ic->preedit, 0, mtext_nchars (ic->preedit),
			     Mcandidate_index, NULL, 0);
      mtext_cat (ic->produced, ic->preedit);
      if (MDEBUG_FLAG ())
	{
	  int i;

	  if (need_prefix)
	    MDEBUG_PRINT1 ("\n  [IM] [%s]",
			   MSYMBOL_NAME (ic_info->state->name));
	  MDEBUG_PRINT (" (commit");
	  for (i = 0; i < mtext_nchars (ic->preedit); i++)
	    MDEBUG_PRINT1 (" U+%04X", mtext_ref_char (ic->preedit, i));
	  MDEBUG_PRINT (")");
	}

      mtext_reset (ic->preedit);
      mtext_reset (ic_info->preedit_saved);
      MPLIST_DO (p, ic_info->markers)
	MPLIST_VAL (p) = 0;
      ic->cursor_pos = ic_info->state_pos = 0;
      ic->preedit_changed = 1;
      ic_info->commit_key_head = ic_info->key_head;
    }
  if (ic->candidate_list)
    {
      M17N_OBJECT_UNREF (ic->candidate_list);
      ic->candidate_list = NULL;
      ic->candidate_index = 0;
      ic->candidate_from = ic->candidate_to = 0;
      ic->candidates_changed = MINPUT_CANDIDATES_LIST_CHANGED;
      if (ic->candidate_show)
	{
	  ic->candidate_show = 0;
	  ic->candidates_changed |= MINPUT_CANDIDATES_SHOW_CHANGED;
	}
    }
}

static int
new_index (MInputContext *ic, int current, int limit, MSymbol sym, MText *mt)
{
  int code = marker_code (sym, 0);

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

  candidate_list = mplist_copy (candidate_list);
  if (MPLIST_MTEXT_P (group))
    {
      mt = MPLIST_MTEXT (group);
      preedit_replace (ic, from, to, NULL, mtext_ref_char (mt, ingroup_index));
      to = from + 1;
    }
  else
    {
      int i;
      MPlist *plist;

      for (i = 0, plist = MPLIST_PLIST (group); i < ingroup_index;
	   i++, plist = MPLIST_NEXT (plist));
      mt = MPLIST_MTEXT (plist);
      preedit_replace (ic, from, to, mt, 0);
      to = from + mtext_nchars (mt);
    }
  mtext_put_prop (ic->preedit, from, to, Mcandidate_list, candidate_list);
  M17N_OBJECT_UNREF (candidate_list);
  mtext_put_prop (ic->preedit, from, to, Mcandidate_index, (void *) idx);
  ic->cursor_pos = to;
}

static MCharset *
get_select_charset (MInputContextInfo * ic_info)
{
  MPlist *plist = resolve_variable (ic_info, Mcandidates_charset);
  MSymbol sym;

  if (! MPLIST_VAL (plist))
    return NULL;
  sym = MPLIST_SYMBOL (plist);
  if (sym == Mnil)
    return NULL;
  return MCHARSET (sym);
}

/* The returned plist must be UNREFed.  */

static MPlist *
adjust_candidates (MPlist *plist, MCharset *charset)
{
  MPlist *pl;

  /* plist ::= MTEXT ... | PLIST ... */
  plist = mplist_copy (plist);
  if (MPLIST_MTEXT_P (plist))
    {
      pl = plist;
      while (! MPLIST_TAIL_P (pl))
	{
	  /* pl ::= MTEXT ... */
	  MText *mt = MPLIST_MTEXT (pl);
	  int mt_copied = 0;
	  int i, c;

	  for (i = mtext_nchars (mt) - 1; i >= 0; i--)
	    {
	      c = mtext_ref_char (mt, i);
	      if (ENCODE_CHAR (charset, c) == MCHAR_INVALID_CODE)
		{
		  if (! mt_copied)
		    {
		      mt = mtext_dup (mt);
		      mplist_set (pl, Mtext, mt);
		      M17N_OBJECT_UNREF (mt);
		      mt_copied = 1;
		    }
		  mtext_del (mt, i, i + 1);
		}
	    }
	  if (mtext_len (mt) > 0)
	    pl = MPLIST_NEXT (pl);
	  else
	    {
	      mplist_pop (pl);
	      M17N_OBJECT_UNREF (mt);
	    }
	}
    }
  else				/* MPLIST_PLIST_P (plist) */
    {
      pl = plist;
      while (! MPLIST_TAIL_P (pl))
	{
	  /* pl ::= (MTEXT ...) ... */
	  MPlist *p = MPLIST_PLIST (pl);
	  int p_copied = 0;
	  /* p ::= MTEXT ... */
	  MPlist *p0 = p;
	  int n = 0;

	  while (! MPLIST_TAIL_P (p0))
	    {
	      MText *mt = MPLIST_MTEXT (p0);
	      int i, c;

	      for (i = mtext_nchars (mt) - 1; i >= 0; i--)
		{
		  c = mtext_ref_char (mt, i);
		  if (ENCODE_CHAR (charset, c) == MCHAR_INVALID_CODE)
		    break;
		}
	      if (i < 0)
		{
		  p0 = MPLIST_NEXT (p0);
		  n++;
		}
	      else
		{
		  if (! p_copied)
		    {
		      p = mplist_copy (p);
		      mplist_set (pl, Mplist, p);
		      M17N_OBJECT_UNREF (p);
		      p_copied = 1;
		      p0 = p;
		      while (n-- > 0)
			p0 = MPLIST_NEXT (p0);
		    }	  
		  mplist_pop (p0);
		  M17N_OBJECT_UNREF (mt);
		}
	    }
	  if (! MPLIST_TAIL_P (p))
	    pl = MPLIST_NEXT (pl);
	  else
	    {
	      mplist_pop (pl);
	      M17N_OBJECT_UNREF (p);
	    }
	}
    }
  if (MPLIST_TAIL_P (plist))
    {
      M17N_OBJECT_UNREF (plist);
      return NULL;
    }      
  return plist;
}

/* The returned Plist must be UNREFed.  */

static MPlist *
get_candidate_list (MInputContextInfo *ic_info, MPlist *args)
{
  MCharset *charset = get_select_charset (ic_info);
  MPlist *plist;
  int column;
  int i, len;

  plist = resolve_variable (ic_info, Mcandidates_group_size);
  column = MPLIST_INTEGER (plist);

  plist = MPLIST_PLIST (args);
  if (! plist)
    return NULL;
  if (charset)
    {
      plist = adjust_candidates (plist, charset);
      if (! plist)
	return NULL;
    }
  else
    M17N_OBJECT_REF (plist);

  if (column == 0)
    return plist;

  if (MPLIST_MTEXT_P (plist))
    {
      MText *mt = MPLIST_MTEXT (plist);
      MPlist *next = MPLIST_NEXT (plist);

      if (MPLIST_TAIL_P (next))
	M17N_OBJECT_REF (mt);
      else
	{
	  mt = mtext_dup (mt);
	  while (! MPLIST_TAIL_P (next))
	    {
	      mt = mtext_cat (mt, MPLIST_MTEXT (next));
	      next = MPLIST_NEXT (next);
	    }
	}
      M17N_OBJECT_UNREF (plist);
      plist = mplist ();
      len = mtext_nchars (mt);
      if (len <= column)
	mplist_add (plist, Mtext, mt);
      else
	{
	  for (i = 0; i < len; i += column)
	    {
	      int to = (i + column < len ? i + column : len);
	      MText *sub = mtext_copy (mtext (), 0, mt, i, to);
						       
	      mplist_add (plist, Mtext, sub);
	      M17N_OBJECT_UNREF (sub);
	    }
	}
      M17N_OBJECT_UNREF (mt);
    }
  else if (MPLIST_PLIST_P (plist))
    {
      MPlist *tail = plist;
      MPlist *new = mplist ();
      MPlist *this = mplist ();
      int count = 0;

      MPLIST_DO (tail, tail)
	{
	  MPlist *p = MPLIST_PLIST (tail);

	  MPLIST_DO (p, p)
	    {
	      MText *mt = MPLIST_MTEXT (p);

	      if (count == column)
		{
		  mplist_add (new, Mplist, this);
		  M17N_OBJECT_UNREF (this);
		  this = mplist ();
		  count = 0;
		}
	      mplist_add (this, Mtext, mt);
	      count++;
	    }
	}
      mplist_add (new, Mplist, this);
      M17N_OBJECT_UNREF (this);
      mplist_set (plist, Mnil, NULL);
      MPLIST_DO (tail, new)
	{
	  MPlist *elt = MPLIST_PLIST (tail);

	  mplist_add (plist, Mplist, elt);
	}
      M17N_OBJECT_UNREF (new);
    }

  return plist;
}


static MPlist *
regularize_action (MPlist *action_list, MInputContextInfo *ic_info)
{
  MPlist *action = NULL;
  MSymbol name;
  MPlist *args;

  if (MPLIST_SYMBOL_P (action_list))
    {
      MSymbol var = MPLIST_SYMBOL (action_list);
      MPlist *p;

      MPLIST_DO (p, ic_info->vars)
	if (MPLIST_SYMBOL (MPLIST_PLIST (p)) == var)
	  break;
      if (MPLIST_TAIL_P (p))
	return NULL;
      action = MPLIST_NEXT (MPLIST_PLIST (p));
      mplist_set (action_list, MPLIST_KEY (action), MPLIST_VAL (action));
    }

  if (MPLIST_PLIST_P (action_list))
    {
      action = MPLIST_PLIST (action_list);
      if (MPLIST_SYMBOL_P (action))
	{
	  name = MPLIST_SYMBOL (action);
	  args = MPLIST_NEXT (action);
	  if (name == Minsert
	      && MPLIST_PLIST_P (args))
	    mplist_set (action, Msymbol, M_candidates);
	}
      else if (MPLIST_MTEXT_P (action) || MPLIST_PLIST_P (action))
	{
	  action = mplist ();
	  mplist_push (action, Mplist, MPLIST_VAL (action_list));
	  mplist_push (action, Msymbol, M_candidates);
	  mplist_set (action_list, Mplist, action);
	  M17N_OBJECT_UNREF (action);
	}
    }
  else if (MPLIST_MTEXT_P (action_list) || MPLIST_INTEGER_P (action_list))
    {
      action = mplist ();
      mplist_push (action, MPLIST_KEY (action_list), MPLIST_VAL (action_list));
      mplist_push (action, Msymbol, Minsert);
      mplist_set (action_list, Mplist, action);
      M17N_OBJECT_UNREF (action);
    }
  return action;
}

/* Perform list of actions in ACTION_LIST for the current input
   context IC.  If unhandle action was not performed, return 0.
   Otherwise, return -1.  */

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
      MPlist *action = regularize_action (action_list, ic_info);
      MSymbol name;
      MPlist *args;

      if (! action)
	continue;
      name = MPLIST_SYMBOL (action);
      args = MPLIST_NEXT (action);

      MDEBUG_PRINT1 (" %s", MSYMBOL_NAME (name));
      if (name == Minsert)
	{
	  if (MPLIST_SYMBOL_P (args))
	    {
	      args = resolve_variable (ic_info, MPLIST_SYMBOL (args));
	      if (! MPLIST_MTEXT_P (args) && ! MPLIST_INTEGER_P (args))
		continue;
	    }
	  if (MPLIST_MTEXT_P (args))
	    preedit_insert (ic, ic->cursor_pos, MPLIST_MTEXT (args), 0);
	  else			/* MPLIST_INTEGER_P (args)) */
	    preedit_insert (ic, ic->cursor_pos, NULL, MPLIST_INTEGER (args));
	}
      else if (name == M_candidates)
	{
	  MPlist *plist = get_candidate_list (ic_info, args);
	  MPlist *pl;
	  int len;

	  if (! plist)
	    continue;
	  if (MPLIST_PLIST_P (plist) && MPLIST_TAIL_P (plist))
	    {
	      M17N_OBJECT_UNREF (plist);
	      continue;
	    }
	  if (MPLIST_MTEXT_P (plist))
	    {
	      preedit_insert (ic, ic->cursor_pos, NULL,
			      mtext_ref_char (MPLIST_MTEXT (plist), 0));
	      len = 1;
	    }
	  else
	    {
	      MText * mt = MPLIST_MTEXT (MPLIST_PLIST (plist));

	      preedit_insert (ic, ic->cursor_pos, mt, 0);
	      len = mtext_nchars (mt);
	    }
	  pl = mplist_copy (plist);
	  M17N_OBJECT_UNREF (plist);
	  mtext_put_prop (ic->preedit,
			  ic->cursor_pos - len, ic->cursor_pos,
			  Mcandidate_list, pl);
	  M17N_OBJECT_UNREF (pl);
	  mtext_put_prop (ic->preedit,
			  ic->cursor_pos - len, ic->cursor_pos,
			  Mcandidate_index, (void *) 0);
	}
      else if (name == Mselect)
	{
	  int start, end;
	  int code, idx, gindex;
	  int pos = ic->cursor_pos;
	  MPlist *group;
	  int idx_decided = 0;

	  if (pos == 0
	      || ! (prop = mtext_get_property (ic->preedit, pos - 1,
					       Mcandidate_list)))
	    continue;
	  idx = (int) mtext_get_prop (ic->preedit, pos - 1, Mcandidate_index);
	  group = find_candidates_group (mtext_property_value (prop), idx,
					 &start, &end, &gindex);
	  if (MPLIST_SYMBOL_P (args))
	    {
	      code = marker_code (MPLIST_SYMBOL (args), 0);
	      if (code < 0)
		{
		  args = resolve_variable (ic_info, MPLIST_SYMBOL (args));
		  if (! MPLIST_INTEGER_P (args))
		    continue;
		  idx = start + MPLIST_INTEGER (args);
		  if (idx < start || idx >= end)
		    continue;
		  idx_decided = 1;
		}		  
	    }
	  else
	    code = -1;

	  if (code != '[' && code != ']')
	    {
	      if (! idx_decided)
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
	  MDEBUG_PRINT1 ("(%d)", idx);
	}
      else if (name == Mshow)
	ic->candidate_show = 1;
      else if (name == Mhide)
	ic->candidate_show = 0;
      else if (name == Mdelete)
	{
	  int len = mtext_nchars (ic->preedit);
	  int pos;
	  int to;

	  if (MPLIST_SYMBOL_P (args)
	      && (pos = surrounding_pos (MPLIST_SYMBOL (args))) != 0)
	    {
	      to = ic->cursor_pos + pos;
	      if (to < 0)
		{
		  delete_surrounding_text (ic, to);
		  to = 0;
		}
	      else if (to > len)
		{
		  delete_surrounding_text (ic, to - len);
		  to = len;
		}
	    }
	  else
	    {
	      to = (MPLIST_SYMBOL_P (args)
		    ? new_index (ic, ic->cursor_pos, len, MPLIST_SYMBOL (args),
				 ic->preedit)
		    : MPLIST_INTEGER (args));
	      if (to < 0)
		to = 0;
	      else if (to > len)
		to = len;
	      pos = to - ic->cursor_pos;
	    }
	  MDEBUG_PRINT1 ("(%d)", pos);
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
	  MDEBUG_PRINT1 ("(%d)", ic->cursor_pos);
	}
      else if (name == Mmark)
	{
	  int code = marker_code (MPLIST_SYMBOL (args), 0);

	  if (code < 0)
	    {
	      mplist_put (ic_info->markers, MPLIST_SYMBOL (args),
			  (void *) ic->cursor_pos);
	      MDEBUG_PRINT1 ("(%d)", ic->cursor_pos);
	    }
	}
      else if (name == Mpushback)
	{
	  if (MPLIST_INTEGER_P (args) || MPLIST_SYMBOL_P (args))
	    {
	      int num;

	      if (MPLIST_SYMBOL_P (args))
		{
		  args = resolve_variable (ic_info, MPLIST_SYMBOL (args));
		  if (MPLIST_INTEGER_P (args))
		    num = MPLIST_INTEGER (args);
		  else
		    num = 0;
		}
	      else
		num = MPLIST_INTEGER (args);

	      if (num > 0)
		ic_info->key_head -= num;
	      else if (num == 0)
		ic_info->key_head = 0;
	      else
		ic_info->key_head = - num;
	      if (ic_info->key_head > ic_info->used)
		ic_info->key_head = ic_info->used;
	    }
	  else if (MPLIST_MTEXT_P (args))
	    {
	      MText *mt = MPLIST_MTEXT (args);
	      int i, len = mtext_nchars (mt);
	      MSymbol key;

	      ic_info->key_head--;
	      for (i = 0; i < len; i++)
		{
		  key = one_char_symbol[MTEXT_DATA (mt)[i]];
		  if (ic_info->key_head + i < ic_info->used)
		    ic_info->keys[ic_info->key_head + i] = key;
		  else
		    MLIST_APPEND1 (ic_info, keys, key, MERROR_IM);
		}
	    }
	  else
	    {
	      MPlist *plist = MPLIST_PLIST (args), *pl;
	      int i = 0;
	      MSymbol key;

	      ic_info->key_head--;

	      MPLIST_DO (pl, plist)
		{
		  key = MPLIST_SYMBOL (pl);
		  if (ic_info->key_head < ic_info->used)
		    ic_info->keys[ic_info->key_head + i] = key;
		  else
		    MLIST_APPEND1 (ic_info, keys, key, MERROR_IM);
		  i++;
		}
	    }
	}
      else if (name == Mpop)
	{
	  if (ic_info->key_head < ic_info->used)
	    MLIST_DELETE1 (ic_info, keys, ic_info->key_head, 1);
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
		func = ((MIMExternalFunc)
			mplist_get_func (external->func_list, func_name));
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
		  && (code = marker_code (MPLIST_SYMBOL (args), 0)) >= 0)
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
	  int intarg = (MPLIST_TAIL_P (args)
			? ic_info->used - 2
			: integer_value (ic, args, 0));

	  mtext_reset (ic->preedit);
	  mtext_reset (ic_info->preedit_saved);
	  mtext_reset (ic->produced);
	  M17N_OBJECT_UNREF (ic_info->vars);
	  ic_info->vars = mplist_copy (ic_info->vars_saved);
	  ic->cursor_pos = ic_info->state_pos = 0;
	  ic_info->state_key_head = ic_info->key_head
	    = ic_info->commit_key_head = 0;

	  shift_state (ic, Mnil);
	  if (intarg < 0)
	    {
	      if (MPLIST_TAIL_P (args))
		{
		  ic_info->used = 0;
		  return -1;
		}
	      ic_info->used += intarg;
	    }
	  else
	    ic_info->used = intarg;
	  break;
	}
      else if (name == Mset || name == Madd || name == Msub
	       || name == Mmul || name == Mdiv)
	{
	  MSymbol sym = MPLIST_SYMBOL (args);
	  MPlist *value = resolve_variable (ic_info, sym);
	  int val1, val2;
	  char *op;

	  val1 = MPLIST_INTEGER (value);
	  args = MPLIST_NEXT (args);
	  val2 = resolve_expression (ic, args);
	  if (name == Mset)
	    val1 = val2, op = "=";
	  else if (name == Madd)
	    val1 += val2, op = "+=";
	  else if (name == Msub)
	    val1 -= val2, op = "-=";
	  else if (name == Mmul)
	    val1 *= val2, op = "*=";
	  else
	    val1 /= val2, op = "/=";
	  MDEBUG_PRINT4 ("(%s %s 0x%X(%d))",
			 MSYMBOL_NAME (sym), op, val1, val1);
	  mplist_set (value, Minteger, (void *) val1);
	}
      else if (name == Mequal || name == Mless || name == Mgreater
	       || name == Mless_equal || name == Mgreater_equal)
	{
	  int val1, val2;
	  MPlist *actions1, *actions2;
	  int ret = 0;

	  val1 = resolve_expression (ic, args);
	  args = MPLIST_NEXT (args);
	  val2 = resolve_expression (ic, args);
	  args = MPLIST_NEXT (args);
	  actions1 = MPLIST_PLIST (args);
	  args = MPLIST_NEXT (args);
	  if (MPLIST_TAIL_P (args))
	    actions2 = NULL;
	  else
	    actions2 = MPLIST_PLIST (args);
	  MDEBUG_PRINT3 ("(%d %s %d)? ", val1, MSYMBOL_NAME (name), val2);
	  if (name == Mequal ? val1 == val2
	      : name == Mless ? val1 < val2
	      : name == Mgreater ? val1 > val2
	      : name == Mless_equal ? val1 <= val2
	      : val1 >= val2)
	    {
	      MDEBUG_PRINT ("ok");
	      ret = take_action_list (ic, actions1);
	    }
	  else
	    {
	      MDEBUG_PRINT ("no");
	      if (actions2)
		ret = take_action_list (ic, actions2);
	    }
	  if (ret < 0)
	    return ret;
	}
      else if (name == Mcond)
	{
	  int idx = 0;

	  MPLIST_DO (args, args)
	    {
	      MPlist *cond;

	      idx++;
	      if (! MPLIST_PLIST (args))
		continue;
	      cond = MPLIST_PLIST (args);
	      if (resolve_expression (ic, cond) != 0)
		{
		  MDEBUG_PRINT1 ("(%dth)", idx);
		  if (take_action_list (ic, MPLIST_NEXT (cond)) < 0)
		    return -1;;
		  break;
		}
	    }
	}
      else if (name == Mcommit)
	{
	  preedit_commit (ic, 0);
	}
      else if (name == Munhandle)
	{
	  preedit_commit (ic, 0);
	  return -1;
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

  if (ic->candidate_list)
    {
      M17N_OBJECT_UNREF (ic->candidate_list);
      ic->candidate_list = NULL;
    }
  if (ic->cursor_pos > 0
      && (prop = mtext_get_property (ic->preedit, ic->cursor_pos - 1,
				     Mcandidate_list)))
    {
      ic->candidate_list = mtext_property_value (prop);
      M17N_OBJECT_REF (ic->candidate_list);
      ic->candidate_index
	= (int) mtext_get_prop (ic->preedit, ic->cursor_pos - 1,
				Mcandidate_index);
      ic->candidate_from = mtext_property_start (prop);
      ic->candidate_to = mtext_property_end (prop);
    }

  if (candidate_list != ic->candidate_list)
    ic->candidates_changed |= MINPUT_CANDIDATES_LIST_CHANGED;
  if (candidate_index != ic->candidate_index)
    ic->candidates_changed |= MINPUT_CANDIDATES_INDEX_CHANGED;
  if (candidate_show != ic->candidate_show)
    ic->candidates_changed |= MINPUT_CANDIDATES_SHOW_CHANGED;    
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
  MSymbol alias = Mnil;
  int i;

  MDEBUG_PRINT2 ("  [IM] [%s] handle `%s'", 
		 MSYMBOL_NAME (ic_info->state->name), msymbol_name (key));

  if (map->submaps)
    {
      submap = mplist_get (map->submaps, key);
      alias = key;
      while (! submap
	     && (alias = msymbol_get (alias, M_key_alias))
	     && alias != key)
	submap = mplist_get (map->submaps, alias);
    }

  if (submap)
    {
      if (! alias || alias == key)
	MDEBUG_PRINT (" submap-found");
      else
	MDEBUG_PRINT1 (" submap-found (by alias `%s')", MSYMBOL_NAME (alias));
      mtext_cpy (ic->preedit, ic_info->preedit_saved);
      ic->preedit_changed = 1;
      ic->cursor_pos = ic_info->state_pos;
      ic_info->key_head++;
      ic_info->map = map = submap;
      if (map->map_actions)
	{
	  MDEBUG_PRINT (" map-actions:");
	  if (take_action_list (ic, map->map_actions) < 0)
	    {
	      MDEBUG_PRINT ("\n");
	      return -1;
	    }
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
	}

      /* If this is the terminal map or we have shifted to another
	 state, perform branch actions (if any).  */
      if (! map->submaps || map != ic_info->map)
	{
	  if (map->branch_actions)
	    {
	      MDEBUG_PRINT (" branch-actions:");
	      if (take_action_list (ic, map->branch_actions) < 0)
		{
		  MDEBUG_PRINT ("\n");
		  return -1;
		}
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

      /* Perform branch actions if any.  */
      if (map->branch_actions)
	{
	  MDEBUG_PRINT (" branch-actions:");
	  if (take_action_list (ic, map->branch_actions) < 0)
	    {
	      MDEBUG_PRINT ("\n");
	      return -1;
	    }
	}

      if (map == ic_info->map)
	{
	  /* The above branch actions didn't change the state.  */

	  /* If MAP is the root map of the initial state, and there
	     still exist an unhandled key, it means that the current
	     input method can not handle it.  */
	  if (map == ((MIMState *) MPLIST_VAL (im_info->states))->map
	      && ic_info->key_head < ic_info->used)
	    {
	      MDEBUG_PRINT (" unhandled\n");
	      return -1;
	    }

	  if (map != ic_info->state->map)
	    {
	      /* MAP is not the root map.  Shift to the root map of the
		 current state. */
	      shift_state (ic, ic_info->state->name);
	    }
	  else if (! map->branch_actions)
	    {
	      /* MAP is the root map without any default branch
		 actions.  Shift to the initial state.  */
	      shift_state (ic, Mnil);
	    }
	}
    }
  MDEBUG_PRINT ("\n");
  return 0;
}

/* Initialize IC->ic_info.  */

static void
init_ic_info (MInputContext *ic)
{
  MInputMethodInfo *im_info = ic->im->info;
  MInputContextInfo *ic_info = ic->info;
  MPlist *plist;
  
  MLIST_INIT1 (ic_info, keys, 8);;

  ic_info->markers = mplist ();

  ic_info->vars = mplist ();
  if (im_info->configured_vars)
    MPLIST_DO (plist, im_info->configured_vars)
      {
	MPlist *pl = MPLIST_PLIST (plist);
	MSymbol name = MPLIST_SYMBOL (pl);

	pl = MPLIST_NEXT (MPLIST_NEXT (MPLIST_NEXT (pl)));
	if (MPLIST_KEY (pl) != Mt)
	  {
	    MPlist *p = mplist ();

	    mplist_push (ic_info->vars, Mplist, p);
	    M17N_OBJECT_UNREF (p);
	    mplist_add (p, Msymbol, name);
	    mplist_add (p, MPLIST_KEY (pl), MPLIST_VAL (pl));
	  }
      }
  ic_info->vars_saved = mplist_copy (ic_info->vars);

  if (im_info->externals)
    {
      MPlist *func_args = mplist (), *plist;

      mplist_add (func_args, Mt, ic);
      MPLIST_DO (plist, im_info->externals)
	{
	  MIMExternalModule *external = MPLIST_VAL (plist);
	  MIMExternalFunc func
	    = (MIMExternalFunc) mplist_get_func (external->func_list, Minit);

	  if (func)
	    (func) (func_args);
	}
      M17N_OBJECT_UNREF (func_args);
    }

  ic_info->preedit_saved = mtext ();
  ic_info->tick = im_info->tick;
}

/* Finalize IC->ic_info.  */

static void
fini_ic_info (MInputContext *ic)
{
  MInputMethodInfo *im_info = ic->im->info;
  MInputContextInfo *ic_info = ic->info;

  if (im_info->externals)
    {
      MPlist *func_args = mplist (), *plist;

      mplist_add (func_args, Mt, ic);
      MPLIST_DO (plist, im_info->externals)
	{
	  MIMExternalModule *external = MPLIST_VAL (plist);
	  MIMExternalFunc func
	    = (MIMExternalFunc) mplist_get_func (external->func_list, Mfini);

	  if (func)
	    (func) (func_args);
	}
      M17N_OBJECT_UNREF (func_args);
    }

  MLIST_FREE1 (ic_info, keys);
  M17N_OBJECT_UNREF (ic_info->preedit_saved);
  M17N_OBJECT_UNREF (ic_info->markers);
  M17N_OBJECT_UNREF (ic_info->vars);
  M17N_OBJECT_UNREF (ic_info->vars_saved);
  M17N_OBJECT_UNREF (ic_info->preceding_text);
  M17N_OBJECT_UNREF (ic_info->following_text);

  memset (ic_info, 0, sizeof (MInputContextInfo));
}

static void
re_init_ic (MInputContext *ic, int reload)
{
  MInputMethodInfo *im_info = (MInputMethodInfo *) ic->im->info;
  MInputContextInfo *ic_info = (MInputContextInfo *) ic->info;
  int status_changed, preedit_changed, cursor_pos_changed, candidates_changed;

  status_changed = ic_info->state != (MIMState *) MPLIST_VAL (im_info->states);
  preedit_changed = mtext_nchars (ic->preedit) > 0;
  cursor_pos_changed = ic->cursor_pos > 0;
  candidates_changed = 0;
  if (ic->candidate_list)
    {
      candidates_changed |= MINPUT_CANDIDATES_LIST_CHANGED;
      M17N_OBJECT_UNREF (ic->candidate_list);
      ic->candidate_list = NULL;
    }
  if (ic->candidate_show)
    {
      candidates_changed |= MINPUT_CANDIDATES_SHOW_CHANGED;
      ic->candidate_show = 0;
    }
  if (ic->candidate_index > 0)
    {
      candidates_changed |= MINPUT_CANDIDATES_INDEX_CHANGED;
      ic->candidate_index = 0;
      ic->candidate_from = ic->candidate_to = 0;
    }
  if (mtext_nchars (ic->produced) > 0)
    mtext_reset (ic->produced);
  if (mtext_nchars (ic->preedit) > 0)
    mtext_reset (ic->preedit);
  ic->cursor_pos = 0;
  M17N_OBJECT_UNREF (ic->plist);
  ic->plist = mplist ();

  fini_ic_info (ic);
  if (reload)
    reload_im_info (im_info);
  if (! im_info->states)
    {
      struct MIMState *state;

      M17N_OBJECT (state, free_state, MERROR_IM);
      state->name = msymbol ("init");
      state->title = mtext__from_data ("ERROR!", 6, MTEXT_FORMAT_US_ASCII, 0);
      MSTRUCT_CALLOC (state->map, MERROR_IM);
      im_info->states = mplist ();
      mplist_add (im_info->states, state->name, state);
    }
  init_ic_info (ic);
  shift_state (ic, Mnil);

  ic->status_changed = status_changed;
  ic->preedit_changed = preedit_changed;
  ic->cursor_pos_changed = cursor_pos_changed;
  ic->candidates_changed = candidates_changed;
}

static void
reset_ic (MInputContext *ic, MSymbol ignore)
{
  MDEBUG_PRINT ("\n  [IM] reset\n");
  re_init_ic (ic, 0);
}

static int
open_im (MInputMethod *im)
{
  MInputMethodInfo *im_info = get_im_info (im->language, im->name, Mnil, Mnil);

  if (! im_info || ! im_info->states)
    MERROR (MERROR_IM, -1);
  im->info = im_info;

  return 0;
}

static void
close_im (MInputMethod *im)
{
  im->info = NULL;
}

static int
create_ic (MInputContext *ic)
{
  MInputContextInfo *ic_info;

  MSTRUCT_CALLOC (ic_info, MERROR_IM);
  ic->info = ic_info;
  init_ic_info (ic);
  shift_state (ic, Mnil);
  return 0;
}

static void
destroy_ic (MInputContext *ic)
{
  fini_ic_info (ic);
  free (ic->info);
}

static int
check_reload (MInputContext *ic, MSymbol key)
{
  MInputMethodInfo *im_info = ic->im->info;
  MPlist *plist = resolve_command (im_info->configured_cmds, Mat_reload);

  if (! plist)
    {
      plist = resolve_command (global_info->configured_cmds, Mat_reload);
      if (! plist)
	return 0;
    }
  MPLIST_DO (plist, plist)
    {
      MSymbol this_key, alias;

      if (MPLIST_MTEXT_P (plist))
	{
	  MText *mt = MPLIST_MTEXT (plist);
	  int c = mtext_ref_char (mt, 0);

	  if (c >= 256)
	    continue;
	  this_key = one_char_symbol[c];
	}
      else
	{
	  MPlist *pl = MPLIST_PLIST (plist);
      
	  this_key = MPLIST_SYMBOL (pl);
	}
      alias = this_key;
      while (alias != key 
	     && (alias = msymbol_get (alias, M_key_alias))
	     && alias != this_key);
      if (alias == key)
	break;
    }
  if (MPLIST_TAIL_P (plist))
    return 0;

  MDEBUG_PRINT ("\n  [IM] reload");
  re_init_ic (ic, 1);
  return 1;
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

  if (check_reload (ic, key))
    return 0;

  if (! ic_info->state)
    {
      ic_info->key_unhandled = 1;
      return 0;
    }
  mtext_reset (ic->produced);
  ic->status_changed = ic->preedit_changed = ic->candidates_changed = 0;
  M17N_OBJECT_UNREF (ic_info->preceding_text);
  M17N_OBJECT_UNREF (ic_info->following_text);
  ic_info->preceding_text = ic_info->following_text = NULL;
  MLIST_APPEND1 (ic_info, keys, key, MERROR_IM);
  ic_info->key_unhandled = 0;

  do {
    if (handle_key (ic) < 0)
      {
	/* KEY was not handled.  Delete it from the current key sequence.  */
	if (ic_info->used > 0)
	  {
	    memmove (ic_info->keys, ic_info->keys + 1,
		     sizeof (int) * (ic_info->used - 1));
	    ic_info->used--;
	    if (ic_info->state_key_head > 0)
	      ic_info->state_key_head--;
	    if (ic_info->commit_key_head > 0)
	      ic_info->commit_key_head--;	      
	  }
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
  if (ic_info->map == ((MIMState *) MPLIST_VAL (im_info->states))->map)
    preedit_commit (ic, 1);

  if (mtext_nchars (ic->produced) > 0)
    {
      if (MDEBUG_FLAG ())
	{
	  MDEBUG_PRINT1 ("\n  [IM] [%s] (produced",
			 MSYMBOL_NAME (ic_info->state->name));
	  for (i = 0; i < mtext_nchars (ic->produced); i++)
	    MDEBUG_PRINT1 (" U+%04X", mtext_ref_char (ic->produced, i));
	  MDEBUG_PRINT (")");
	}

      mtext_put_prop (ic->produced, 0, mtext_nchars (ic->produced),
		      Mlanguage, ic->im->language);
    }
  if (ic_info->commit_key_head > 0)
    {
      memmove (ic_info->keys, ic_info->keys + ic_info->commit_key_head,
	       sizeof (int) * (ic_info->used - ic_info->commit_key_head));
      ic_info->used -= ic_info->commit_key_head;
      ic_info->key_head -= ic_info->commit_key_head;
      ic_info->state_key_head -= ic_info->commit_key_head;
      ic_info->commit_key_head = 0;
    }
  if (ic_info->key_unhandled)
    {
      ic_info->used = 0;
      ic_info->key_head = ic_info->state_key_head
	= ic_info->commit_key_head = 0;
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


/* Input method command handler.  */

/* List of all (global and local) commands. 
   (LANG:(IM-NAME:(COMMAND ...) ...) ...) ...
   COMMAND is CMD-NAME:(mtext:DESCRIPTION plist:KEYSEQ ...))
   Global commands are stored as (t (t COMMAND ...))  */


/* Input method variable handler.  */


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
  Minput_driver = msymbol ("input-driver");

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
  Minput_focus_move = msymbol ("input-focus-move");
  Minput_focus_in = msymbol ("input-focus-in");
  Minput_focus_out = msymbol ("input-focus-out");
  Minput_toggle = msymbol ("input-toggle");
  Minput_reset = msymbol ("input-reset");
  Minput_get_surrounding_text = msymbol ("input-get-surrounding-text");
  Minput_delete_surrounding_text = msymbol ("input-delete-surrounding-text");
  Mcustomized = msymbol ("customized");
  Mconfigured = msymbol ("configured");
  Minherited = msymbol ("inherited");

  minput_default_driver.open_im = open_im;
  minput_default_driver.close_im = close_im;
  minput_default_driver.create_ic = create_ic;
  minput_default_driver.destroy_ic = destroy_ic;
  minput_default_driver.filter = filter;
  minput_default_driver.lookup = lookup;
  minput_default_driver.callback_list = mplist ();
  mplist_put_func (minput_default_driver.callback_list, Minput_reset,
		   M17N_FUNC (reset_ic));
  minput_driver = &minput_default_driver;

  fully_initialized = 0;
  return 0;
}

void
minput__fini ()
{
  if (fully_initialized)
    {
      free_im_list (im_info_list);
      if (im_custom_list)
	free_im_list (im_custom_list);
      if (im_config_list)
	free_im_list (im_config_list);
      M17N_OBJECT_UNREF (load_im_info_keys);
    }

  M17N_OBJECT_UNREF (minput_default_driver.callback_list);
  M17N_OBJECT_UNREF (minput_driver->callback_list);

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
    @brief Symbol whose name is "input-method".
 */
/***ja
    @brief "input-method" を名前として持つシンボル.
 */
MSymbol Minput_method;

/***en
    @name Variables: Predefined symbols for callback commands.  */
/***ja
    @name 変数： コールバックコマンド用定義済みシンボル.  */
/*** @{ */ 
/***en
    These are the predefined symbols that are used as the @c COMMAND
    argument of callback functions of an input method driver (see
    #MInputDriver::callback_list).  

    Most of them do not require extra argument nor return any value;
    exceptions are these:

    @b Minput_get_surrounding_text: When a callback function assigned for
    this command is called, the first element of #MInputContext::plist
    has key #Minteger and the value specifies which portion of the
    surrounding text should be retrieved.  If the value is positive,
    it specifies the number of characters following the current cursor
    position.  If the value is negative, the absolute value specifies
    the number of characters preceding the current cursor position.
    If the value is zero, it means that the caller just wants to know
    if the surrounding text is currently supported or not.

    If the surrounding text is currently supported, the callback
    function must set the key of this element to #Mtext and the value
    to the retrieved M-text.  The length of the M-text may be shorter
    than the requested number of characters, if the available text is
    not that long.  The length can be zero in the worst case.  Or, the
    length may be longer if an application thinks it is more efficient
    to return that length.

    If the surrounding text is not currently supported, the callback
    function should return without changing the first element of
    #MInputContext::plist.

    @b Minput_delete_surrounding_text: When a callback function assigned
    for this command is called, the first element of
    #MInputContext::plist has key #Minteger and the value specifies
    which portion of the surrounding text should be deleted in the
    same way as the case of Minput_get_surrounding_text.  The callback
    function must delete the specified text.  It should not alter
    #MInputContext::plist.  */ 
/***ja
    入力メソッドドライバのコールバック関数において @c COMMAND 
    引数として用いられる定義済みシンボル (#MInputDriver::callback_list 参照)。

    ほとんどは追加の引数を必要としないし値を返さないが、以下は例外である。

    Minput_get_surrounding_text: このコマンドに割り当てられたコールバッ
    ク関数が呼ばれた際には、 #MInputContext::plist の第一要素はキーとし
    て#Minteger をとり、その値はサラウンディングテキストのうちどの部分
    を取って来るかを指定する。値が正であれば、現在のカーソル位置に続く
    値の個数分の文字を取る。負であれば、カーソル位置に先行する値の絶対
    値分の文字を取る。現在サラウンドテキストがサポートされているかどう
    かを知りたいだけであれば、この値はゼロでも良い。

    サラウンディングテキストがサポートされていれば、コールバック関数は
    この要素のキーを #Mtext に、値を取り込んだM-text に設定しなくてはな
    らない。もしテキストの長さが充分でなければ、この M-text の長さは要
    求されている文字数より短くて良い。最悪の場合 0 でもよいし、アプリケー
    ション側で必要で効率的だと思えば長くても良い。

    サラウンディングテキストがサポートされていなければ、コールバック関
    数は #MInputContext::plist の第一要素を変更してはならない。

    Minput_delete_surrounding_text: このコマンドに割り当てられたコール
    バック関数が呼ばれた際には、#MInputContext::plist の第一要素は、キー
    として#Minteger をとり、値は削除するべきサラウンディングテキストを
    Minput_get_surrounding_text と同様のやり方で指定する。コールバック
    関数は指定されたテキストを削除しなければならない。また
    #MInputContext::plist を変えてはならない。  */ 
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
MSymbol Minput_get_surrounding_text;
MSymbol Minput_delete_surrounding_text;
/*** @} */

/*=*/

/***en
    @name Variables: Predefined symbols for special input events.

    These are the predefined symbols that are used as the @c KEY
    argument of minput_filter ().  */ 
/***ja
    @name 変数: 特別な入力イベント用定義済みシンボル.

    minput_filter () の @c KEY 引数として用いられる定義済みシンボル。  */ 

/*** @{ */ 
/*=*/

MSymbol Minput_focus_out;
MSymbol Minput_focus_in;
MSymbol Minput_focus_move;

/*** @} */

/*=*/
/***en
    @name Variables: Predefined symbols used in input method information.  */
/***ja
    @name 変数: 入力メソッド情報用定義済みシンボル.  */
/*** @{ */ 
/*=*/
/***en
    These are the predefined symbols describing status of input method
    command and variable, and are used in a return value of
    minput_get_command () and minput_get_variable ().  */
/***ja
    入力メソッドのコマンドや変数の状態を表し、minput_get_command () と
    minput_get_variable () の戻り値として用いられる定義済みシンボル。  */
MSymbol Minherited;
MSymbol Mcustomized;
MSymbol Mconfigured;
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
    dependent arguments $ARG of the functions whose name begins with
    "minput_" are all ignored.  */
/***ja
    @brief 内部入力メソッド用デフォルトドライバ.

    変数 #minput_default_driver は内部入力メソッド用のデフォルトのドライバを表す。

    メンバ MInputDriver::open_im () は m17n データベース中からタグ 
    \< #Minput_method, $LANGUAGE, $NAME\> 
    に合致する入力メソッドを探し、それをロードする。

    メンバ MInputDriver::callback_list () は @c NULL であり、
    したがって、プログラマ側で責任を持って 適切なコールバック関数の plist
    に設定しなくてはならない。さもないと、preedit 
    テキストなどのフィードバック情報がユーザに表示されない。

    マクロ M17N_INIT () は変数 #minput_driver 
    をこのドライバへのポインタに設定し、全ての内部入力メソッドがこのドライバを使うようにする。

    したがって、@c minput_driver がデフォルト値のままであれば、minput_ 
    で始まる関数のドライバに依存する引数 $ARG はすべて無視される。  */

MInputDriver minput_default_driver;
/*=*/

/***en
    @brief The driver for internal input methods.

    The variable #minput_driver is a pointer to the input method
    driver that is used by internal input methods.  The macro
    M17N_INIT () initializes it to a pointer to #minput_default_driver
    if <m17n<EM></EM>.h> is included.  */ 
/***ja
    @brief 内部入力メソッド用ドライバ.

    変数 #minput_driver は内部入力メソッドによって使用されている入力メ
    ソッドドライバへのポインタである。マクロ M17N_INIT () はこのポイン
    タを#minput_default_driver (<m17n<EM></EM>.h> が include されている
    時) に初期化する。  */ 

MInputDriver *minput_driver;

/*=*/
/***
    The variable #Minput_driver is a symbol for a foreign input method.
    See @ref foreign-input-method "foreign input method" for the detail.  */
MSymbol Minput_driver;

/*=*/

/***en
    @name Functions
*/
/***ja
    @name 関数
*/
/*** @{ */

/*=*/

/***en
    @brief Open an input method.

    The minput_open_im () function opens an input method whose
    language and name match $LANGUAGE and $NAME, and returns a pointer
    to the input method object newly allocated.

    This function at first decides a driver for the input method as
    described below.

    If $LANGUAGE is not #Mnil, the driver pointed by the variable
    #minput_driver is used.

    If $LANGUAGE is #Mnil and $NAME has the property #Minput_driver, the
    driver pointed to by the property value is used to open the input
    method.  If $NAME has no such a property, @c NULL is returned.

    Then, the member MInputDriver::open_im () of the driver is
    called.  

    $ARG is set in the member @c arg of the structure MInputMethod so
    that the driver can refer to it.  */
/***ja
    @brief 入力メソッドをオープンする.

    関数 minput_open_im () は言語 $LANGUAGE と名前 $NAME 
    に合致する入力メソッドをオープンし、新たに割り当てられた入力メソッドオブジェクトへのポインタを返す。
    
    この関数は、まず入力メソッド用のドライバを以下のようにして決定する。

    $LANGUAGE が #Mnil でなければ、変数 #minput_driver 
    で指されているドライバを用いる。

    $LANGUAGE が #Mnil であり、$NAME が #Minput_driver
    プロパティを持つ場合には、そのプロパティの値で指されている入力ドライバを用いて入力メソッドをオープンする。
    $NAME にそのようなプロパティが無かった場合は @c NULL を返す。

    次いで、ドライバのメンバ MInputDriver::open_im () が呼ばれる。

    $ARG は構造体 MInputMethod のメンバ @c arg に設定され、ドライバから参照できる。

    @latexonly \IPAlabel{minput_open} @endlatexonly

*/

MInputMethod *
minput_open_im (MSymbol language, MSymbol name, void *arg)
{
  MInputMethod *im;
  MInputDriver *driver;

  MINPUT__INIT ();

  MDEBUG_PRINT2 ("  [IM] opening (%s %s) ... ",
		 msymbol_name (language), msymbol_name (name));
  if (language)
    {
      if (name == Mnil)
	MERROR (MERROR_IM, NULL);
      driver = minput_driver;
    }
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
      MDEBUG_PRINT (" failed\n");
      free (im);
      return NULL;
    }
  MDEBUG_PRINT (" ok\n");
  return im;
}

/*=*/

/***en
    @brief Close an input method.

    The minput_close_im () function closes the input method $IM, which
    must have been created by minput_open_im ().  */

/***ja
    @brief 入力メソッドをクローズする.

    関数 minput_close_im () は、入力メソッド $IM をクローズする。
    この入力メソッド $IM は minput_open_im () によって作られたものでなければならない。  */

void
minput_close_im (MInputMethod *im)
{
  MDEBUG_PRINT2 ("  [IM] closing (%s %s) ... ",
		 msymbol_name (im->name), msymbol_name (im->language));
  (*im->driver.close_im) (im);
  free (im);
  MDEBUG_PRINT (" done\n");
}

/*=*/

/***en
    @brief Create an input context.

    The minput_create_ic () function creates an input context object
    associated with input method $IM, and calls callback functions
    corresponding to @b Minput_preedit_start, @b Minput_status_start, and
    @b Minput_status_draw in this order.

    @return
    If an input context is successfully created, minput_create_ic ()
    returns a pointer to it.  Otherwise it returns @c NULL.  */

/***ja
    @brief 入力コンテクストを生成する.

    関数 minput_create_ic () は入力メソッド $IM
    に対応する入力コンテクストオブジェクトを生成し、
    @b Minput_preedit_start, @b Minput_status_start, @b Minput_status_draw
    に対応するコールバック関数をこの順に呼ぶ。

    @return
    入力コンテクストが生成された場合、minput_create_ic () 
    はその入力コンテクストへのポインタを返す。失敗した場合は @c NULL を返す。
      */

MInputContext *
minput_create_ic (MInputMethod *im, void *arg)
{
  MInputContext *ic;

  MDEBUG_PRINT2 ("  [IM] creating context (%s %s) ... ",
		 msymbol_name (im->name), msymbol_name (im->language));
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
      MDEBUG_PRINT (" failed\n");
      M17N_OBJECT_UNREF (ic->preedit);
      M17N_OBJECT_UNREF (ic->produced);
      M17N_OBJECT_UNREF (ic->plist);
      free (ic);
      return NULL;
    };

  if (im->driver.callback_list)
    {
      minput_callback (ic, Minput_preedit_start);
      minput_callback (ic, Minput_status_start);
      minput_callback (ic, Minput_status_draw);
    }

  MDEBUG_PRINT (" ok\n");
  return ic;
}

/*=*/

/***en
    @brief Destroy an input context.

    The minput_destroy_ic () function destroys the input context $IC,
    which must have been created by minput_create_ic ().  It calls
    callback functions corresponding to @b Minput_preedit_done,
    @b Minput_status_done, and @b Minput_candidates_done in this order.  */

/***ja
    @brief 入力コンテクストを破壊する.

    関数 minput_destroy_ic () は、入力コンテクスト $IC を破壊する。
    この入力コンテクストは minput_create_ic () 
    によって作られたものでなければならない。この関数は 
    @b Minput_preedit_done, @b Minput_status_done, @b Minput_candidates_done 
    に対応するコールバック関数をこの順に呼ぶ。
  */

void
minput_destroy_ic (MInputContext *ic)
{
  MDEBUG_PRINT2 ("  [IM] destroying context (%s %s) ... ",
		 msymbol_name (ic->im->name), msymbol_name (ic->im->language));
  if (ic->im->driver.callback_list)
    {
      minput_callback (ic, Minput_preedit_done);
      minput_callback (ic, Minput_status_done);
      minput_callback (ic, Minput_candidates_done);
    }
  (*ic->im->driver.destroy_ic) (ic);
  M17N_OBJECT_UNREF (ic->preedit);
  M17N_OBJECT_UNREF (ic->produced);
  M17N_OBJECT_UNREF (ic->plist);
  MDEBUG_PRINT (" done\n");
  free (ic);
}

/*=*/

/***en
    @brief Filter an input key.

    The minput_filter () function filters input key $KEY according to
    input context $IC, and calls callback functions corresponding to
    @b Minput_preedit_draw, @b Minput_status_draw, and
    @b Minput_candidates_draw if the preedit text, the status, and the
    current candidate are changed respectively.

    To make the input method commit the current preedit text (if any)
    and shift to the initial state, call this function with #Mnil as
    $KEY.

    To inform the input method about the focus-out event, call this
    function with @b Minput_focus_out as $KEY.

    To inform the input method about the focus-in event, call this
    function with @b Minput_focus_in as $KEY.

    To inform the input method about the focus-move event (i.e. input
    spot change within the same input context), call this function
    with @b Minput_focus_move as $KEY.

    @return
    If $KEY is filtered out, this function returns 1.  In that case,
    the caller should discard the key.  Otherwise, it returns 0, and
    the caller should handle the key, for instance, by calling the
    function minput_lookup () with the same key.  */

/***ja
    @brief 入力キーをフィルタする.

    関数 minput_filter () は入力キー $KEY を入力コンテクスト $IC 
    に応じてフィルタし、preedit テキスト、ステータス、現時点での候補が変化した時点で、それぞれ
    @b Minput_preedit_draw, @b Minput_status_draw,
    @b Minput_candidates_draw に対応するコールバック関数を呼ぶ。

    @return 
    $KEY がフィルタされれば、この関数は 1 を返す。
    この場合呼び出し側はこのキーを捨てるべきである。
    そうでなければ 0 を返し、呼び出し側は、たとえば同じキーで関数 minput_lookup ()
    を呼ぶなどして、このキーを処理する。

    @latexonly \IPAlabel{minput_filter} @endlatexonly
*/

int
minput_filter (MInputContext *ic, MSymbol key, void *arg)
{
  int ret;

  if (! ic
      || ! ic->active)
    return 0;
  if (ic->im->driver.callback_list
      && mtext_nchars (ic->preedit) > 0)
    minput_callback (ic, Minput_preedit_draw);

  ret = (*ic->im->driver.filter) (ic, key, arg);

  if (ic->im->driver.callback_list)
    {
      if (ic->preedit_changed)
	minput_callback (ic, Minput_preedit_draw);
      if (ic->status_changed)
	minput_callback (ic, Minput_status_draw);
      if (ic->candidates_changed)
	minput_callback (ic, Minput_candidates_draw);
    }

  return ret;
}

/*=*/

/***en
    @brief Look up a text produced in the input context.

    The minput_lookup () function looks up a text in the input context
    $IC.  $KEY must be identical to the one that was used in the previous call of
    minput_filter ().

    If a text was produced by the input method, it is concatenated
    to M-text $MT.

    This function calls #MInputDriver::lookup .

    @return
    If $KEY was correctly handled by the input method, this function
    returns 0.  Otherwise, it returns -1, even though some text
    might be produced in $MT.  */

/***ja
    @brief 入力コンテクスト中のテキストを探す.

    関数 minput_lookup () は入力コンテクスト $IC 中のテキストを探す。
    $KEY は関数 minput_filter () への直前の呼び出しに用いられたものと同じでなくてはならない。

    テキストが入力メソッドによって生成されていれば、テキストは M-text
    $MT に連結される。

    この関数は、#MInputDriver::lookup を呼ぶ。

    @return 
    $KEY が入力メソッドによって適切に処理できれば、この関数は 0 を返す。
    そうでなければ -1 を返す。
    この場合でも $MT に何らかのテキストが生成されていることがある。

    @latexonly \IPAlabel{minput_lookup} @endlatexonly  */

int
minput_lookup (MInputContext *ic, MSymbol key, void *arg, MText *mt)
{
  return (ic ? (*ic->im->driver.lookup) (ic, key, arg, mt) : -1);
}
/*=*/

/***en
    @brief Set the spot of the input context.

    The minput_set_spot () function sets the spot of input context $IC
    to coordinate ($X, $Y ) with the height specified by $ASCENT and $DESCENT .
    The semantics of these values depends on the input method driver.

    For instance, a driver designed to work in a CUI environment may
    use $X and $Y as the column- and row numbers, and may ignore $ASCENT and
    $DESCENT .  A driver designed to work in a window system may
    interpret $X and $Y as the pixel offsets relative to the origin of the
    client window, and may interpret $ASCENT and $DESCENT as the ascent- and
    descent pixels of the line at ($X . $Y ).

    $FONTSIZE specifies the fontsize of preedit text in 1/10 point.

    $MT and $POS are the M-text and the character position at the spot.
    $MT may be @c NULL, in which case, the input method cannot get
    information about the text around the spot.  */

/***ja
    @brief 入力コンテクストのスポットを設定する.

    関数 minput_set_spot () は、入力コンテクスト $IC のスポットを、座標 ($X, $Y )
    の位置に 、高さ $ASCENT、 $DESCENT 
    で設定する。 これらの値の意味は入力メソッドドライバに依存する。

    たとえば CUI 環境で動作するドライバは $X と $Y 
    をそれぞれ列と行の番号として用い、$ASCENT と $DESCENT 
    を無視するかもしれない。 またウィンドウシステム用のドライバは
    $X と $Y をクライアントウィンドウの原点からのオフセットをピクセル単位で表したものとして扱い、
    $ASCENT と $DESCENT を ($X . $Y )
    の列のアセントとディセントをピクセル単位で表したものとして扱うかもしれない。

    $FONTSIZE には preedit テキストのフォントサイズを 1/10 ポイント単位で指定する。

    $MT と $POS はそのスポットの M-text と文字位置である。$MT は @c
    NULL でもよく、その場合には入力メソッドはスポット周辺のテキストに関する情報を得ることができない。
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
    minput_callback (ic, Minput_set_spot);
}
/*=*/

/***en
    @brief Toggle input method.

    The minput_toggle () function toggles the input method associated
    with input context $IC.  */
/***ja
    @brief 入力メソッドを切替える.

    関数 minput_toggle () は入力コンテクスト $IC 
    に対応付けられた入力メソッドをトグルする。
    */

void
minput_toggle (MInputContext *ic)
{
  if (ic->im->driver.callback_list)
    minput_callback (ic, Minput_toggle);
  ic->active = ! ic->active;
}

/*=*/

/***en
    @brief Reset an input context.

    The minput_reset_ic () function resets input context $IC by
    calling a callback function corresponding to @b Minput_reset.  It
    resets the status of $IC to its initial one.  As the
    current preedit text is deleted without commitment, if necessary,
    call minput_filter () with the arg @b key #Mnil to force the input
    method to commit the preedit in advance.  */

/***ja
    @brief 入力コンテクストをリセットする.

    関数 minput_reset_ic () は @b Minput_reset に対応するコールバック関数
    を呼ぶことによって入力コンテクスト $IC をリセットする。リセットとは、
    実際には入力メソッドを初期状態に移すことである。現在入力中のテキス
    トはコミットされることなく削除されるので、アプリケーションプログラ
    ムは、必要ならば予め minput_filter () を引数 @b key #Mnil で呼んで
    強制的にプリエディットテキストをコミットさせること。  */

void
minput_reset_ic (MInputContext *ic)
{
  if (ic->im->driver.callback_list)
    minput_callback (ic, Minput_reset);
}

/*=*/

/***en
    @brief Get title and icon filename of an input method.

    The minput_get_title_icon () function returns a plist containing a
    title and icon filename (if any) of an input method specified by
    $LANGUAGE and $NAME.

    The first element of the plist has key #Mtext and the value is an
    M-text of the title for identifying the input method.  The second
    element (if any) has key #Mtext and the value is an M-text of the
    icon image (absolute) filename for the same purpose.

    @return
    If there exists a specified input method and it defines an title,
    a plist is returned.  Otherwise, NULL is returned.  The caller
    must free the plist by m17n_object_unref ().  */
/***ja
    @brief 入力メソッドのタイトルとアイコン用ファイル名を得る.

    関数 minput_get_title_icon () は、 $LANGUAGE と $NAME で指定される
    入力メソッドのタイトルと（あれば）アイコン用ファイルを含む plist を
    返す。

    plist の第一要素は、#Mtext をキーに持ち、値は入力メソッドを識別する
    タイトルを表す M-text である。第二要素があれば、キーは #Mtext であ
    り、値は識別用アイコン画像ファイルの絶対パスを表す M-text である。

    @return
    指定の入力メソッドが存在し、タイトルが定義されていれば
     plist を返す。そうでなければ NULL を返す。呼出側は
     関数 m17n_object_unref () を用いて plist を解放しなくてはならない。  */

MPlist *
minput_get_title_icon (MSymbol language, MSymbol name)
{
  MInputMethodInfo *im_info;
  MPlist *plist;
  char *file = NULL;
  MText *mt;

  MINPUT__INIT ();

  im_info = get_im_info (language, name, Mnil, Mtitle);
  if (! im_info || !im_info->title)
    return NULL;
  mt = mtext_get_prop (im_info->title, 0, Mtext);
  if (mt)
    file = mdatabase__find_file ((char *) MTEXT_DATA (mt));
  else
    {
      char *buf = alloca (MSYMBOL_NAMELEN (language) + MSYMBOL_NAMELEN (name)
			  + 12);

      sprintf (buf, "icons/%s-%s.png", (char *) MSYMBOL_NAME (language), 
	       (char *) MSYMBOL_NAME (name));
      file = mdatabase__find_file (buf);
      if (! file && language == Mt)
	{
	  sprintf (buf, "icons/%s.png", (char *) MSYMBOL_NAME (name));
	  file = mdatabase__find_file (buf);
	}
    }

  plist = mplist ();
  mplist_add (plist, Mtext, im_info->title);
  if (file)
    {
      mt = mtext__from_data (file, strlen (file), MTEXT_FORMAT_UTF_8, 1);
      free (file);
      mplist_add (plist, Mtext, mt);
      M17N_OBJECT_UNREF (mt);
    }
  return plist;
}

/*=*/

/***en
    @brief Get description text of an input method.

    The minput_get_description () function returns an M-text that
    describes the input method specified by $LANGUAGE and $NAME.

    @return
    If the specified input method has a description text, a pointer to
    #MText is returned.  The caller has to free it by m17n_object_unref ().
    If the input method does not have a description text, @c NULL is
    returned.  */
/***ja
    @brief 入力メソッドの説明テキストを得る.

    関数 minput_get_description () は、$LANGUAGE と $NAME によって指定
    された入力メソッドを説明する M-text を返す。

    @return
    指定された入力メソッドが説明するテキストを持っていれば、
    #MText へのポインタを返す。呼び出し側は、それを m17n_object_unref
    () を用いて解放しなくてはならない。入力メソッドに説明テキストが無け
    れば@c NULL を返す。 */

MText *
minput_get_description (MSymbol language, MSymbol name)
{
  MInputMethodInfo *im_info;
  MSymbol extra;

  MINPUT__INIT ();

  if (name != Mnil)
    extra = Mnil;
  else
    extra = language, language = Mt;

  im_info = get_im_info (language, name, extra, Mdescription);
  if (! im_info || ! im_info->description)
    return NULL;
  M17N_OBJECT_REF (im_info->description);
  return im_info->description;
}

/*=*/

/***en
    @brief Get information about input method command(s).

    The minput_get_command () function returns information about
    the command $COMMAND of the input method specified by $LANGUAGE and
    $NAME.  An input method command is a pseudo key event to which one
    or more actual input key sequences are assigned.

    There are two kinds of commands, global and local.  A global
    command has a global definition, and the description and the key
    assignment may be inherited by a local command.  Each input method
    defines a local command which has a local key assignment.  It may
    also declare a local command that inherits the definition of a
    global command of the same name.

    If $LANGUAGE is #Mt and $NAME is #Mnil, this function returns
    information about a global command.  Otherwise information about a
    local command is returned.

    If $COMMAND is #Mnil, information about all commands is returned.

    The return value is a @e well-formed plist (@ref m17nPlist) of this
    format:
@verbatim
  ((NAME DESCRIPTION STATUS [KEYSEQ ...]) ...)
@endverbatim
    @c NAME is a symbol representing the command name.

    @c DESCRIPTION is an M-text describing the command, or #Mnil if the
    command has no description.

    @c STATUS is a symbol representing how the key assignment is decided.
    The value is #Mnil (the default key assignment), @b Mcustomized (the
    key assignment is customized by per-user customization file), or
    @b Mconfigured (the key assignment is set by the call of
    minput_config_command ()).  For a local command only, it may also
    be @b Minherited (the key assignment is inherited from the
    corresponding global command).

    @c KEYSEQ is a plist of one or more symbols representing a key
    sequence assigned to the command.  If there's no KEYSEQ, the
    command is currently disabled (i.e. no key sequence can trigger
    actions of the command).

    If $COMMAND is not #Mnil, the first element of the returned plist
    contains the information about $COMMAND.

    @return

    If the requested information was found, a pointer to a non-empty
    plist is returned.  As the plist is kept in the library, the
    caller must not modify nor free it.

    Otherwise (the specified input method or the specified command
    does not exist), @c NULL is returned.  */
/***ja
    @brief 入力メソッドのコマンドに関する情報を得る.

    関数 minput_get_command () は、$LANGUAGE と $NAME で指定される入力
    メソッドのコマンド $COMMAND に関する情報を返す。入力メソッドのコマ
    ンドとは、疑似キーイベントであり、１つ以上の実際の入力キーシークエ
    ンスが割り当てられる。

    コマンドには、グローバルとローカルの２種類がある。グローバルなコマンド
    はグローバルに定義され、ローカルなコマンドはその説明とキー割り当て
    を継承することができる。各入力メソッドはローカルなキー割当を持つロー
    カルなコマンドを定義する。また同名のグローバルなコマンドの定義を継
    承するローカルなコマンドを宣言することもできる。

    $LANGUAGE が #Mt で $NAME が #Mnil の場合は、この関数はグローバルコ
    マンドに関する情報を返す。そうでなければローカルコマンドに関するも
    のを返す。

    $COMMAND が #Mnil の場合は、すべてのコマンドに関する情報を返す。

    戻り値は以下の形式の @e well-formed plist (@ref m17nPlist) である。

@verbatim
  ((NAME DESCRIPTION STATUS [KEYSEQ ...]) ...)
@endverbatim
    @c NAME はコマンド名を示すシンボルである。

    @c DESCRIPTION はコマンドを説明する M-text であるか、説明が無い場合に
    は #Mnil である。

    @c STATUS はキー割り当てがどのように定められるかをあらわすシンボル
    であり、その値は #Mnil （デフォルトの割り当て）, @b Mcustomized （ユー
    ザ毎のカスタマイズファイルによってカスタマイズされた割り当て）,
    @b Mconfigured （minput_config_command ()を呼ぶことによって設定される
    割り当て）のいずれかである。ローカルコマンドの場合には、
    @b Minherited （対応するグローバルコマンドからの継承による割り当て）
    でもよい。

    @c KEYSEQ は１つ以上のシンボルからなる plist であり、各シンボルはコマ
    ンドに割り当てられているキーシークエンスを表す。KEYSEQ が無い場合は、
    そのコマンドは現状で使用不能である。（すなわちコマンドの動作を起
    動できるキーシークエンスが無い。）

    $COMMAND が #Mnil でなければ、返される plist の最初の要素は、
    $COMMAND に関する情報を含む。

    @return

    求められた情報が見つかれば、空でない plist へのポインタを返す。リス
    トはライブラリが管理しているので、呼出側が変更したり解放したりする
    ことはできない。

    そうでなければ、すなわち指定の入力メソッドやコマンドが存在しなければ
    @c NULL を返す。  */

#if EXAMPLE_CODE
MText *
get_im_command_description (MSymbol language, MSymbol name, MSymbol command)
{
  /* Return a description of the command COMMAND of the input method
     specified by LANGUAGE and NAME.  */
  MPlist *cmd = minput_get_command (langauge, name, command);
  MPlist *plist;

  if (! cmds)
    return NULL;
  plist = mplist_value (cmds);	/* (NAME DESCRIPTION STATUS KEY-SEQ ...) */
  plist = mplist_next (plist);	/* (DESCRIPTION STATUS KEY-SEQ ...) */
  return  (mplist_key (plist) == Mtext
	   ? (MText *) mplist_value (plist)
	   : NULL);
}
#endif

MPlist *
minput_get_command (MSymbol language, MSymbol name, MSymbol command)
{
  MInputMethodInfo *im_info;

  MINPUT__INIT ();

  im_info = get_im_info (language, name, Mnil, Mcommand);
  if (! im_info
      || ! im_info->configured_cmds
      || MPLIST_TAIL_P (im_info->configured_cmds))
    return NULL;
  if (command == Mnil)
    return im_info->configured_cmds;
  return mplist__assq (im_info->configured_cmds, command);
}

/*=*/

/***en
    @brief Configure the key sequence of an input method command.

    The minput_config_command () function assigns a list of key
    sequences $KEYSEQLIST to the command $COMMAND of the input method
    specified by $LANGUAGE and $NAME.

    If $KEYSEQLIST is a non-empty plist, it must be a list of key
    sequences, and each key sequence must be a plist of symbols.

    If $KEYSEQLIST is an empty plist, any configuration and
    customization of the command are cancelled, and default key
    sequences become effective.

    If $KEYSEQLIST is NULL, the configuration of the command is
    canceled, and the original key sequences (what saved in per-user
    customization file, or the default one) become effective.

    In the latter two cases, $COMMAND can be #Mnil to make all the
    commands of the input method the target of the operation.

    If $NAME is #Mnil, this function configures the key assignment of a
    global command, not that of a specific input method.

    The configuration takes effect for input methods opened or
    re-opened later in the current session.  In order to make the
    configuration take effect for the future session, it must be saved
    in a per-user customization file by the function
    minput_save_config ().

    @return
    If the operation was successful, this function returns 0,
    otherwise returns -1.  The operation fails in these cases:
    <ul>
    <li>$KEYSEQLIST is not in a valid form.
    <li>$COMMAND is not available for the input method.
    <li>$LANGUAGE and $NAME do not specify an existing input method.
    </ul>

    @seealso
    minput_get_commands (), minput_save_config ().
*/
/***ja
    @brief 入力メソッドのコマンドのキーシークエンスを設定する.

    関数 minput_config_command () はキーシークエンスのリスト
    $KEYSEQLIST を、$LANGUAGE と $NAME によって指定される入力メソッドの
    コマンド $COMMAND に割り当てる。

    $KEYSEQLIST が空リストでなければ、キーシークエンスのリストであり、
    各キーシークエンスはシンボルの plist である。

    $KEYSEQLIST が空の plist ならば、そのコマンドの設定やカスタマイズは
    すべてキャンセルされ、デフォルトのキーシークエンスが有効になる。

    $KEYSEQLIST が NULL であれば、そのコマンドの設定はキャンセルされ、
    元のキーシークエンス（ユーザ毎のカスタマイズファイルに保存されてい
    るもの、あるいはデフォルトのもの）が有効になる。

    後のふたつの場合には、$COMMAND は #Mnil をとることができ、指定の入
    力メソッドの全てのコマンド設定のキャンセルを意味する。

    $NAME が #Mnil ならば、この関数は個々の入力メソッドではなくグローバ
    ルなコマンドのキー割り当てを設定する。

    これらの設定は、現行のセッション中で入力メソッドがオープン（または
    再オープン）された時点で有効になる。将来のセッション中でも有効にす
    るためには、関数 minput_save_config () を用いてユーザ毎のカスタマイ
    ズファイルに保存しなくてはならない。

    @return

    この関数は、処理が成功すれば 0 を、失敗すれば -1 を返す。失敗とは以下の場合である。
    <ul>
    <li>$KEYSEQLIST が有効な形式でない。
    <li>$COMMAND が指定の入力メソッドで利用できない。
    <li>$LANGUAGE と $NAME で指定される入力メソッドが存在しない。
    </ul>

    @seealso
    minput_get_commands (), minput_save_config ().
*/

#if EXAMPLE_CODE
/* Add "C-x u" to the "start" command of Unicode input method.  */
{
  MSymbol start_command = msymbol ("start");
  MSymbol unicode = msymbol ("unicode");
  MPlist *cmd, *plist, *key_seq_list, *key_seq;

  /* At first get the current key-sequence assignment.  */
  cmd = minput_get_command (Mt, unicode, start_command);
  if (! cmd)
    {
      /* The input method does not have the command "start".  Here
	 should come some error handling code.  */
    }
  /* Now CMD == ((start DESCRIPTION STATUS KEY-SEQUENCE ...) ...).
     Extract the part (KEY-SEQUENCE ...).  */
  plist = mplist_next (mplist_next (mplist_next (mplist_value (cmd))));
  /* Copy it because we should not modify it directly.  */
  key_seq_list = mplist_copy (plist);
  
  key_seq = mplist ();
  mplist_add (key_seq, Msymbol, msymbol ("C-x"));
  mplist_add (key_seq, Msymbol, msymbol ("u"));
  mplist_add (key_seq_list, Mplist, key_seq);
  m17n_object_unref (key_seq);

  minput_config_command (Mt, unicode, start_command, key_seq_list);
  m17n_object_unref (key_seq_list);
}
#endif

int
minput_config_command (MSymbol language, MSymbol name, MSymbol command,
		       MPlist *keyseqlist)
{
  MInputMethodInfo *im_info, *config;
  MPlist *plist;

  MINPUT__INIT ();

  im_info = get_im_info (language, name, Mnil, Mcommand);
  if (! im_info)
    MERROR (MERROR_IM, -1);
  if (command == Mnil ? (keyseqlist && ! MPLIST_TAIL_P (keyseqlist))
      : (! im_info->cmds
	 || ! mplist__assq (im_info->configured_cmds, command)))
    MERROR (MERROR_IM, -1);
  if (keyseqlist && ! MPLIST_TAIL_P (keyseqlist))
    {
      MPLIST_DO (plist, keyseqlist)
	if (! check_command_keyseq (plist))
	  MERROR (MERROR_IM, -1);
    }

  config = get_config_info (im_info);
  if (! config)
    {
      if (! im_config_list)
	im_config_list = mplist ();
      config = new_im_info (NULL, language, name, Mnil, im_config_list);
      config->cmds = mplist ();
      config->vars = mplist ();
    }

  if (! keyseqlist && MPLIST_TAIL_P (config->cmds))
    /* Nothing to do.  */
    return 0;

  if (command == Mnil)
    {
      if (! keyseqlist)
	{
	  /* Cancal the configuration. */
	  if (MPLIST_TAIL_P (config->cmds))
	    return 0;
	  mplist_set (config->cmds, Mnil, NULL);
	}
      else
	{
	  /* Cancal the customization. */
	  MInputMethodInfo *custom = get_custom_info (im_info);

	  if (MPLIST_TAIL_P (config->cmds)
	      && (! custom || ! custom->cmds || MPLIST_TAIL_P (custom->cmds)))
	    /* Nothing to do.  */
	    return 0;
	  mplist_set (config->cmds, Mnil, NULL);
	  MPLIST_DO (plist, custom->cmds)
	    {
	      command = MPLIST_SYMBOL (MPLIST_PLIST (plist));
	      plist = mplist ();
	      mplist_add (plist, Msymbol, command);
	      mplist_push (config->cmds, Mplist, plist);
	      M17N_OBJECT_UNREF (plist);
	    }
	}
    }
  else
    {
      plist = mplist__assq (config->cmds, command);
      if (! keyseqlist)
	{
	  /* Cancel the configuration.  */
	  if (! plist)
	    return 0;
	  mplist__pop_unref (plist);
	}
      else if (MPLIST_TAIL_P (keyseqlist))
	{
	  /* Cancel the customization.  */
	  MInputMethodInfo *custom = get_custom_info (im_info);
	  int no_custom = (! custom || ! custom->cmds
			   || ! mplist__assq (custom->cmds, command));
	  if (! plist)
	    {
	      if (no_custom)
		return 0;
	      plist = mplist ();
	      mplist_add (config->cmds, Mplist, plist);
	      M17N_OBJECT_UNREF (plist);
	      plist = mplist_add (plist, Msymbol, command);
	    }
	  else
	    {
	      if (no_custom)
		mplist__pop_unref (plist);
	      else
		{
		  plist = MPLIST_PLIST (plist); /* (NAME nil KEYSEQ ...) */
		  plist = MPLIST_NEXT (plist);
		  mplist_set (plist, Mnil, NULL);
		}
	    }
	}
      else
	{
	  MPlist *pl;

	  if (plist)
	    {
	      plist = MPLIST_NEXT (MPLIST_PLIST (plist));
	      if (! MPLIST_TAIL_P (plist))
		mplist_set (plist, Mnil, NULL);
	    }
	  else
	    {
	      plist = mplist ();
	      mplist_add (config->cmds, Mplist, plist);
	      M17N_OBJECT_UNREF (plist);
	      plist = mplist_add (plist, Msymbol, command);
	      plist = MPLIST_NEXT (plist);
	    }
	  MPLIST_DO (keyseqlist, keyseqlist)
	    {
	      pl = mplist_copy (MPLIST_VAL (keyseqlist));
	      plist = mplist_add (plist, Mplist, pl);
	      M17N_OBJECT_UNREF (pl);
	    }
	}
    }
  config_all_commands (im_info);
  im_info->tick = time (NULL);
  return 0;
}

/*=*/

/***en
    @brief Get information about input method variable(s).

    The minput_get_variable () function returns information about
    variable $VARIABLE of the input method specified by $LANGUAGE and $NAME.
    An input method variable controls behavior of an input method.

    There are two kinds of variables, global and local.  A global
    variable has a global definition, and the description and the value
    may be inherited by a local variable.  Each input method defines a
    local variable which has local value.  It may also declare a
    local variable that inherits definition of a global variable of
    the same name.

    If $LANGUAGE is #Mt and $NAME is #Mnil, information about a global
    variable is returned.  Otherwise information about a local variable
    is returned.

    If $VARIABLE is #Mnil, information about all variables is
    returned.

    The return value is a @e well-formed plist (@ref m17nPlist) of this
    format:
@verbatim
  ((NAME DESCRIPTION STATUS VALUE [VALID-VALUE ...]) ...)
@endverbatim
    @c NAME is a symbol representing the variable name.

    @c DESCRIPTION is an M-text describing the variable, or #Mnil if the
    variable has no description.

    @c STATUS is a symbol representing how the value is decided.  The
    value is #Mnil (the default value), @b Mcustomized (the value is
    customized by per-user customization file), or @b Mconfigured (the
    value is set by the call of minput_config_variable ()).  For a
    local variable only, it may also be @b Minherited (the value is
    inherited from the corresponding global variable).

    @c VALUE is the initial value of the variable.  If the key of this
    element is #Mt, the variable has no initial value.  Otherwise, the
    key is #Minteger, #Msymbol, or #Mtext and the value is of the
    corresponding type.

    @c VALID-VALUEs (if any) specify which values the variable can have.
    They have the same type (i.e. having the same key) as @c VALUE except
    for the case that VALUE is an integer.  In that case, @c VALID-VALUE
    may be a plist of two integers specifying the range of possible
    values.

    If there no @c VALID-VALUE, the variable can have any value as long
    as the type is the same as @c VALUE.

    If $VARIABLE is not #Mnil, the first element of the returned plist
    contains the information about $VARIABLE.

    @return

    If the requested information was found, a pointer to a non-empty
    plist is returned.  As the plist is kept in the library, the
    caller must not modify nor free it.

    Otherwise (the specified input method or the specified variable
    does not exist), @c NULL is returned.  */
/***ja
    @brief 入力メソッドの変数に関する情報を得る.

    関数 minput_get_variable () は、$LANGUAGE と $NAME で指定される入力
    メソッドの変数 $VARIABLE に関する情報を返す。入力メソッドの変数とは、
    入力メソッドの振舞を制御するものである。

    変数には、グローバルとローカルの２種類がある。グローバルな変数はグ
    ローバルに定義され、ローカルな変数はその説明と値を継承することがで
    きる。各入力メソッドはローカルな値を持つローカルな変数を定義する。
    また同名のグローバルな変数の定義を継承するローカルな変数を宣言する
    こともできる。

    $LANGUAGE が #Mt で $NAME が #Mnil の場合は、この関数はグローバル変
    数に関する情報を返す。そうでなければローカル変数に関するものを返す。

    $VARIABLE が #Mnil の場合は、すべてのコマンドに関する情報を返す。

    戻り値は以下の形式の @e well-formed plist (@ref m17nPlist) である。
@verbatim
  ((NAME DESCRIPTION STATUS VALUE [VALID-VALUE ...]) ...)
@endverbatim

    @c NAME は変数の名前を示すシンボルである。

    @c DESCRIPTION は変数を説明する M-text であるか、説明が無い場合には
    #Mnil である。

    @c STATUS は値がどのように定められるかをあらわすシンボルであり、
    @c STATUS の値は #Mnil （デフォルトの値）, @b Mcustomized （ユーザ毎の
    カスタマイズファイルによってカスタマイズされた値）, @b Mconfigured
    （minput_config_variable ()を呼ぶことによって設定される値）のいずれ
    かである。ローカル変数の場合には、@b Minherited （対応するグローバル
    変数から継承した値）でもよい。

    @c VALUE は変数の初期値である。この要素のキーが#Mt であれば初期値を持
    たない。そうでなければ、キーは #Minteger, #Msymbol, #Mtext のいずれ
    かであり、値はそれぞれ対応する型のものである。

    @c VALID-VALUE はもしあれば、変数の取り得る値を指定する。これは @c VALUE
    と同じ型(すなわち同じキーを持つ) であるが、例外として @c VALUE が
    integer の場合は @c VALID-VALUE は可能な値の範囲を示す二つの整数から
    なる plist となることができる。

    @c VALID-VALUE がなければ、変数は @c VALUE と同じ型である限りいかなる値も
    とることができる。

    $VARIABLE が #Mnil でなければ、返される plist の最初の要素は
    $VARIABLE に関する情報を含む。

    @return

    求められた情報が見つかれば、空でない plist へのポインタを返す。リス
    トはライブラリが管理しているので、呼出側が変更したり解放したりする
    ことはできない。

    そうでなければ、すなわち指定の入力メソッドや変数が存在しなければ
    @c NULL を返す。 */

MPlist *
minput_get_variable (MSymbol language, MSymbol name, MSymbol variable)
{
  MInputMethodInfo *im_info;

  MINPUT__INIT ();

  im_info = get_im_info (language, name, Mnil, Mvariable);
  if (! im_info || ! im_info->configured_vars)
    return NULL;
  if (variable == Mnil)
    return im_info->configured_vars;
  return mplist__assq (im_info->configured_vars, variable);
}

/*=*/

/***en
    @brief Configure the value of an input method variable.

    The minput_config_variable () function assigns $VALUE to the
    variable $VARIABLE of the input method specified by $LANGUAGE and
    $NAME.

    If $VALUE is a non-empty plist, it must be a plist of one element
    whose key is #Minteger, #Msymbol, or #Mtext, and the value is of
    the corresponding type.  That value is assigned to the variable.

    If $VALUE is an empty plist, any configuration and customization
    of the variable are canceled, and the default value is assigned to
    the variable.

    If $VALUE is NULL, the configuration of the variable is canceled,
    and the original value (what saved in per-user customization file,
    or the default value) is assigned to the variable.

    In the latter two cases, $VARIABLE can be #Mnil to make all the
    variables of the input method the target of the operation.

    If $NAME is #Mnil, this function configures the value of global
    variable, not that of a specific input method.

    The configuration takes effect for input methods opened or
    re-opened later in the current session.  To make the configuration
    take effect for the future session, it must be saved in a per-user
    customization file by the function minput_save_config ().

    @return

    If the operation was successful, this function returns 0,
    otherwise returns -1.  The operation fails in these cases:
    <ul>
    <li>$VALUE is not in a valid form, the type does not match the
    definition, or the value is our of range.
    <li>$VARIABLE is not available for the input method.
    <li>$LANGUAGE and $NAME do not specify an existing input method.  
    </ul>

    @seealso
    minput_get_variable (), minput_save_config ().  */
/***ja
    @brief 入力メソッドの変数の値を設定する.

    関数 minput_config_variable () は値 $VALUE を、$LANGUAGE と $NAME
    によって指定される入力メソッドの変数 $VARIABLE に割り当てる。

    $VALUE が 空リストでなければ、１要素の plist であり、そのキーは
    #Minteger, #Msymbol, #Mtext のいずれか、値は対応する型のものである。
    この値が変数 $VARIABLE に割り当てられる。

    $VALUE が 空リストであれば、変数の設定とカスタマイズがキャンセルさ
    れ、デフォルト値が変数 $VARIABLE に割り当てられる。

    $VALUE が NULL であれば、変数の設定はキャンセルされ、元の値（ユーザ
    毎のカスタマイズファイル中の値、またはデフォルトの値）が割り当てられる。

    後のふたつの場合には、$VARIABLE は #Mnil をとることができ、指定され
    た入力メソッドの全ての変数設定のキャンセルを意味する。

    $NAME が #Mnil ならば、この関数は個々の入力メソッドではなくグローバ
    ルな変数の値を設定する。

    これらの設定は、現行のセッション中で入力メソッドがオープン（または
    再オープン）された時点で有効になる。将来のセッション中でも有効にす
    るためには、関数 minput_save_config () を用いてユーザ毎のカスタマイ
    ズファイルに保存しなくてはならない。

    @return

    この関数は、処理が成功すれば 0 を、失敗すれば -1 を返す。失敗とは以下の場合である。
    <ul>
    <li>$VALUEが有効な形式でない。型が定義に合わない、または値が範囲外である。
    <li>$VARIABLE が指定の入力メソッドで利用できない。
    <li>$LANGUAGE と $NAME で指定される入力メソッドが存在しない。
    </ul>

    @seealso
    minput_get_commands (), minput_save_config ().
*/
int
minput_config_variable (MSymbol language, MSymbol name, MSymbol variable,
			MPlist *value)
{
  MInputMethodInfo *im_info, *config;
  MPlist *plist;

  MINPUT__INIT ();

  im_info = get_im_info (language, name, Mnil, Mvariable);
  if (! im_info)
    MERROR (MERROR_IM, -1);
  if (variable == Mnil ? (value && ! MPLIST_TAIL_P (value))
      : (! im_info->vars
	 || ! (plist = mplist__assq (im_info->configured_vars, variable))))
    MERROR (MERROR_IM, -1);

  if (value && ! MPLIST_TAIL_P (value))
    {
      plist = MPLIST_PLIST (plist);
      plist = MPLIST_NEXT (plist); /* (DESC STATUS VALUE VALIDS ...) */
      plist = MPLIST_NEXT (plist); /* (STATUS VALUE VALIDS ...) */
      plist = MPLIST_NEXT (plist); /* (VALUE VALIDS ...) */
      if (MPLIST_KEY (plist) != Mt
	  && ! check_variable_value (value, plist))
	MERROR (MERROR_IM, -1);
    }

  config = get_config_info (im_info);
  if (! config)
    {
      if (! im_config_list)
	im_config_list = mplist ();
      config = new_im_info (NULL, language, name, Mnil, im_config_list);
      config->cmds = mplist ();
      config->vars = mplist ();
    }

  if (! value && MPLIST_TAIL_P (config->vars))
    /* Nothing to do.  */
    return 0;

  if (variable == Mnil)
    {
      if (! value)
	{
	  /* Cancel the configuration.  */
	  if (MPLIST_TAIL_P (config->vars))
	    return 0;
	  mplist_set (config->vars, Mnil, NULL);
	}
      else
	{
	  /* Cancel the customization.  */
	  MInputMethodInfo *custom = get_custom_info (im_info);

	  if (MPLIST_TAIL_P (config->vars)
	      && (! custom || ! custom->vars || MPLIST_TAIL_P (custom->vars)))
	    /* Nothing to do.  */
	    return 0;
	  mplist_set (config->vars, Mnil, NULL);
	  MPLIST_DO (plist, custom->vars)
	    {
	      variable = MPLIST_SYMBOL (MPLIST_PLIST (plist));
	      plist = mplist ();
	      mplist_add (plist, Msymbol, variable);
	      mplist_push (config->vars, Mplist, plist);
	      M17N_OBJECT_UNREF (plist);
	    }
	}
    }
  else
    {
      plist = mplist__assq (config->vars, variable);
      if (! value)
	{
	  /* Cancel the configuration.  */
	  if (! plist)
	    return 0;
	  mplist__pop_unref (plist);
	}
      else if (MPLIST_TAIL_P (value))
	{
	  /* Cancel the customization.  */
	  MInputMethodInfo *custom = get_custom_info (im_info);
	  int no_custom = (! custom || ! custom->vars
			   || ! mplist__assq (custom->vars, variable));
	  if (! plist)
	    {
	      if (no_custom)
		return 0;
	      plist = mplist ();
	      mplist_add (config->vars, Mplist, plist);
	      M17N_OBJECT_UNREF (plist);
	      plist = mplist_add (plist, Msymbol, variable);
	    }
	  else
	    {
	      if (no_custom)
		mplist__pop_unref (plist);
	      else
		{
		  plist = MPLIST_PLIST (plist); /* (NAME nil VALUE) */
		  plist = MPLIST_NEXT (plist);	/* ([nil VALUE]) */
		  mplist_set (plist, Mnil ,NULL);
		}
	    }
	}
      else
	{
	  if (plist)
	    {
	      plist = MPLIST_NEXT (MPLIST_PLIST (plist));
	      if (! MPLIST_TAIL_P (plist))
		mplist_set (plist, Mnil, NULL);
	    }
	  else
	    {
	      plist = mplist ();
	      mplist_add (config->vars, Mplist, plist);
	      M17N_OBJECT_UNREF (plist);
	      plist = mplist_add (plist, Msymbol, variable);
	      plist = MPLIST_NEXT (plist);
	    }
	  mplist_add (plist, MPLIST_KEY (value), MPLIST_VAL (value));
	}
    }
  config_all_variables (im_info);
  im_info->tick = time (NULL);
  return 0;
}

/*=*/

/***en
    @brief Get the name of per-user customization file.
    
    The minput_config_file () function returns the absolute path name
    of per-user customization file into which minput_save_config ()
    save configurations.  It is usually @c config.mic under the
    directory <tt>${HOME}/.m17n.d</tt> (${HOME} is user's home
    directory).  It is not assured that the file of the returned name
    exists nor is readable/writable.  If minput_save_config () fails
    and returns -1, an application program might check the file, make
    it writable (if possible), and try minput_save_config () again.

    @return

    This function returns a string.  As the string is kept in the
    library, the caller must not modify nor free it.

    @seealso
    minput_save_config ()
*/
/***ja
    @brief ユーザ毎のカスタマイズファイルの名前を得る.
    
    関数 minput_config_file () は、関数 minput_save_config () が設定を
    保存するユーザ毎のカスタマイズファイルへの絶対パス名を返す。通常は、ユーザ
    のホームディレクトリの下のディレクトリ @c ".m17n.d" にある@c
    "config.mic" となる。返された名前のファイルが存在するか、読み書きで
    きるかは保証されない。関数minput_save_config () が失敗して -1 を返
    した場合には、アプリケーションプログラムはファイルの存在を確認し、
    （できれば）書き込み可能にし再度minput_save_config () を試すことが
    できる。

    @return

    この関数は文字列を返す。文字列はライブラリが管理しているので、呼出
    側が修正したり解放したりすることはできない。

    @seealso
    minput_save_config ()
*/

char *
minput_config_file ()
{
  MINPUT__INIT ();

  return mdatabase__file (im_custom_mdb);
}

/*=*/

/***en
    @brief Save configurations in per-user customization file.

    The minput_save_config () function saves the configurations done
    so far in the current session into the per-user customization
    file.

    @return

    If the operation was successful, 1 is returned.  If the per-user
    customization file is currently locked, 0 is returned.  In that
    case, the caller may wait for a while and try again.  If the
    configuration file is not writable, -1 is returned.  In that case,
    the caller may check the name of the file by calling
    minput_config_file (), make it writable if possible, and try
    again.

    @seealso
    minput_config_file ()  */
/***ja
    @brief 設定をユーザ毎のカスタマイズファイルに保存する.

    関数 minput_save_config () は現行のセッションでこれまでに行った設定
    をユーザ毎のカスタマイズファイルに保存する。

    @return

    成功すれば 1 を返す。ユーザ毎のカスタマイズファイルがロックされてい
    れば 0 を返す。この場合、呼出側はしばらく待って再試行できる。設定ファ
    イルが書き込み不可の場合、-1 を返す。この場合、minput_config_file
    () を呼んでファイル名をチェックし、できれば書き込み可能にし、再試行
    できる。

    @seealso
    minput_config_file ()  */

int
minput_save_config (void)
{
  MPlist *data, *tail, *plist, *p, *elt;
  int ret;

  MINPUT__INIT ();
  ret = mdatabase__lock (im_custom_mdb);
  if (ret <= 0)
    return ret;
  if (! im_config_list)
    return 1;
  update_custom_info ();
  if (! im_custom_list)
    im_custom_list = mplist ();

  /* At first, reflect configuration in customization.  */
  MPLIST_DO (plist, im_config_list)
    {
      MPlist *pl = MPLIST_PLIST (plist);
      MSymbol language, name, extra, command, variable;
      MInputMethodInfo *custom, *config;

      language = MPLIST_SYMBOL (pl);
      pl = MPLIST_NEXT (pl);
      name = MPLIST_SYMBOL (pl);
      pl = MPLIST_NEXT (pl);
      extra = MPLIST_SYMBOL (pl);
      pl = MPLIST_NEXT (pl);
      config = MPLIST_VAL (pl);
      custom = get_custom_info (config);
      if (! custom)
	custom = new_im_info (NULL, language, name, extra, im_custom_list);
      if (config->cmds)
	MPLIST_DO (pl, config->cmds)
	  {
	    elt = MPLIST_PLIST (pl);
	    command = MPLIST_SYMBOL (elt);
	    if (custom->cmds)
	      p = mplist__assq (custom->cmds, command);
	    else
	      custom->cmds = mplist (), p = NULL;
	    elt = MPLIST_NEXT (elt);
	    if (p)
	      {
		p = MPLIST_NEXT (MPLIST_NEXT (MPLIST_PLIST (p)));
		mplist_set (p, Mnil, NULL);
	      }
	    else
	      {
		p = mplist ();
		mplist_add (custom->cmds, Mplist, p);
		M17N_OBJECT_UNREF (p);
		mplist_add (p, Msymbol, command);
		p = mplist_add (p, Msymbol, Mnil);
		p = MPLIST_NEXT (p);
	      }
	    mplist__conc (p, elt);
	  }
      if (config->vars)
	MPLIST_DO (pl, config->vars)
	  {
	    elt = MPLIST_PLIST (pl);
	    variable = MPLIST_SYMBOL (elt);
	    if (custom->vars)
	      p = mplist__assq (custom->vars, variable);
	    else
	      custom->vars = mplist (), p = NULL;
	    elt = MPLIST_NEXT (elt);
	    if (p)
	      {
		p = MPLIST_NEXT (MPLIST_NEXT (MPLIST_PLIST (p)));
		mplist_set (p, Mnil, NULL);
	      }
	    else
	      {
		p = mplist ();
		mplist_add (custom->vars, Mplist, p);
		M17N_OBJECT_UNREF (p);
		mplist_add (p, Msymbol, variable);
		p = mplist_add (p, Msymbol, Mnil);
		p = MPLIST_NEXT (p);
	      }
	    mplist__conc (p, elt);
	  }
    }
  free_im_list (im_config_list);
  im_config_list = NULL;

  /* Next, reflect customization to the actual plist to be written.  */
  data = tail = mplist ();
  MPLIST_DO (plist, im_custom_list)
    {
      MPlist *pl = MPLIST_PLIST (plist);
      MSymbol language, name, extra;
      MInputMethodInfo *custom, *im_info;

      language = MPLIST_SYMBOL (pl);
      pl  = MPLIST_NEXT (pl);
      name = MPLIST_SYMBOL (pl);
      pl = MPLIST_NEXT (pl);
      extra = MPLIST_SYMBOL (pl);
      pl = MPLIST_NEXT (pl);
      custom = MPLIST_VAL (pl);
      if ((! custom->cmds || MPLIST_TAIL_P (custom->cmds))
	  && (! custom->vars || MPLIST_TAIL_P (custom->vars)))
	continue;
      im_info = lookup_im_info (im_info_list, language, name, extra);
      if (im_info)
	{
	  if (im_info->cmds)
	    config_all_commands (im_info);
	  if (im_info->vars)
	    config_all_variables (im_info);
	}
      
      elt = NULL;
      if (custom->cmds && ! MPLIST_TAIL_P (custom->cmds))
	{
	  MPLIST_DO (p, custom->cmds)
	    if (! MPLIST_TAIL_P (MPLIST_NEXT (MPLIST_PLIST (p))))
	      break;
	  if (! MPLIST_TAIL_P (p))
	    {
	      elt = mplist ();
	      pl = mplist ();
	      mplist_add (elt, Mplist, pl);
	      M17N_OBJECT_UNREF (pl);
	      pl = mplist_add (pl, Msymbol, Mcommand);
	      MPLIST_DO (p, custom->cmds)
		if (! MPLIST_TAIL_P (MPLIST_NEXT (MPLIST_PLIST (p))))
		  pl = mplist_add (pl, Mplist, MPLIST_PLIST (p));
	    }
	}
      if (custom->vars && ! MPLIST_TAIL_P (custom->vars))
	{
	  MPLIST_DO (p, custom->vars)
	    if (! MPLIST_TAIL_P (MPLIST_NEXT (MPLIST_PLIST (p))))
	      break;
	  if (! MPLIST_TAIL_P (p))
	    {
	      if (! elt)
		elt = mplist ();
	      pl = mplist ();
	      mplist_add (elt, Mplist, pl);
	      M17N_OBJECT_UNREF (pl);
	      pl = mplist_add (pl, Msymbol, Mvariable);
	      MPLIST_DO (p, custom->vars)
		if (! MPLIST_TAIL_P (MPLIST_NEXT (MPLIST_PLIST (p))))
		  pl = mplist_add (pl, Mplist, MPLIST_PLIST (p));
	    }
	}
      if (elt)
	{
	  pl = mplist ();
	  mplist_push (elt, Mplist, pl);
	  M17N_OBJECT_UNREF (pl);
	  pl = mplist_add (pl, Msymbol, Minput_method);
	  pl = mplist_add (pl, Msymbol, language);
	  pl = mplist_add (pl, Msymbol, name);
	  if (extra != Mnil)
	    pl = mplist_add (pl, Msymbol, extra);
	  tail = mplist_add (tail, Mplist, elt);
	  M17N_OBJECT_UNREF (elt);
	}
    }

  mplist_push (data, Mstring, ";; -*- mode:lisp; coding:utf-8 -*-");
  ret = mdatabase__save (im_custom_mdb, data);
  mdatabase__unlock (im_custom_mdb);
  M17N_OBJECT_UNREF (data);
  return (ret < 0 ? -1 : 1);
}

/*=*/
/*** @} */
/*=*/
/***en
    @name Obsolete functions
*/
/***ja
    @name Obsolete な関数
*/
/*** @{ */

/*=*/
/***en
    @brief Get a list of variables of an input method (obsolete).

    This function is obsolete.  Use minput_get_variable () instead.

    The minput_get_variables () function returns a plist (#MPlist) of
    variables used to control the behavior of the input method
    specified by $LANGUAGE and $NAME.  The plist is @e well-formed
    (@ref m17nPlist) of the following format:

@verbatim
    (VARNAME (DOC-MTEXT DEFAULT-VALUE [ VALUE ... ] )
     VARNAME (DOC-MTEXT DEFAULT-VALUE [ VALUE ... ] )
     ...)
@endverbatim

    @c VARNAME is a symbol representing the variable name.

    @c DOC-MTEXT is an M-text describing the variable.

    @c DEFAULT-VALUE is the default value of the variable.  It is a
    symbol, integer, or M-text.

    @c VALUEs (if any) specifies the possible values of the variable.
    If @c DEFAULT-VALUE is an integer, @c VALUE may be a plist (@c FROM
    @c TO), where @c FROM and @c TO specifies a range of possible
    values.

    For instance, suppose an input method has the variables:

    @li name:intvar, description:"value is an integer",
         initial value:0, value-range:0..3,10,20

    @li name:symvar, description:"value is a symbol",
         initial value:nil, value-range:a, b, c, nil

    @li name:txtvar, description:"value is an M-text",
         initial value:empty text, no value-range (i.e. any text)

    Then, the returned plist is as follows.

@verbatim
    (intvar ("value is an integer" 0 (0 3) 10 20)
     symvar ("value is a symbol" nil a b c nil)
     txtvar ("value is an M-text" ""))
@endverbatim

    @return
    If the input method uses any variables, a pointer to #MPlist is
    returned.  As the plist is kept in the library, the caller must not
    modify nor free it.  If the input method does not use any
    variable, @c NULL is returned.  */
/***ja
    @brief 入力メソッドの変数リストを得る.

    関数 minput_get_variables () は、$LANGUAGE と $NAME によって指定さ
    れた入力メソッドの振る舞いを制御する変数のプロパティリスト
    (#MPlist) を返す。このリストは @e well-formed であり(@ref m17nPlist) 以
    下の形式である。

@verbatim
    (VARNAME (DOC-MTEXT DEFAULT-VALUE [ VALUE ... ] )
     VARNAME (DOC-MTEXT DEFAULT-VALUE [ VALUE ... ] )
     ...)
@endverbatim

    @c VARNAME は変数の名前を示すシンボルである。

    @c DOC-MTEXT は変数を説明する M-text である。

    @c DEFAULT-VALUE は変数のデフォルト値であり、シンボル、整数もしくは
    M-text である。

    @c VALUE は、もし指定されていれば変数の取り得る値を示す。もし
    @c DEFAULT-VALUE が整数なら、 @c VALUE は (@c FROM @c TO) という形
    のリストでも良い。この場合 @c FROM と @c TO は可能な値の範囲を示す。

    例として、ある入力メソッドが次のような変数を持つ場合を考えよう。

    @li name:intvar, 説明:"value is an integer",
        初期値:0, 値の範囲:0..3,10,20

    @li name:symvar, 説明:"value is a symbol",
         初期値:nil, 値の範囲:a, b, c, nil

    @li name:txtvar, 説明:"value is an M-text",
        初期値:empty text, 値の範囲なし(どんな M-text でも可)

    この場合、返されるリストは以下のようになる。

@verbatim
    (intvar ("value is an integer" 0 (0 3) 10 20)
     symvar ("value is a symbol" nil a b c nil)
     txtvar ("value is an M-text" ""))
@endverbatim

    @return 
    入力メソッドが何らかの変数を使用していれば #MPlist へのポインタを返す。
    返されるプロパティリストはライブラリによって管理されており、呼び出し側で変更したり解放したりしてはならない。
    入力メソッドが変数を一切使用してなければ、@c NULL を返す。  */

MPlist *
minput_get_variables (MSymbol language, MSymbol name)
{
  MInputMethodInfo *im_info;
  MPlist *vars;

  MINPUT__INIT ();

  im_info = get_im_info (language, name, Mnil, Mvariable);
  if (! im_info || ! im_info->configured_vars)
    return NULL;

  M17N_OBJECT_UNREF (im_info->bc_vars);
  im_info->bc_vars = mplist ();
  MPLIST_DO (vars, im_info->configured_vars)
    {
      MPlist *plist = MPLIST_PLIST (vars);
      MPlist *elt = mplist ();

      mplist_push (im_info->bc_vars, Mplist, elt);
      mplist_add (elt, Msymbol, MPLIST_SYMBOL (plist));
      elt = MPLIST_NEXT (elt);
      mplist_set (elt, Mplist, mplist_copy (MPLIST_NEXT (plist)));
      M17N_OBJECT_UNREF (elt);
    }
  return im_info->bc_vars;
}

/*=*/

/***en
    @brief Set the initial value of an input method variable.

    The minput_set_variable () function sets the initial value of
    input method variable $VARIABLE to $VALUE for the input method
    specified by $LANGUAGE and $NAME.

    By default, the initial value is 0.

    This setting gets effective in a newly opened input method.

    @return
    If the operation was successful, 0 is returned.  Otherwise -1 is
    returned, and #merror_code is set to @c MERROR_IM.  */
/***ja
    @brief 入力メソッド変数の初期値を設定する.

    関数 minput_set_variable () は、$LANGUAGE と $NAME 
    によって指定された入力メソッドの入力メソッド変数 $VARIABLE
    の初期値を、 $VALUE に設定する。

    デフォルトの初期値は 0 である。

    この設定は、新しくオープンされた入力メソッドから有効となる。

    @return
    処理が成功すれば 0 を返す。そうでなければ -1 を返し、
    #merror_code を @c MERROR_IM に設定する。  */

int
minput_set_variable (MSymbol language, MSymbol name,
		     MSymbol variable, void *value)
{
  MPlist *plist, *pl;
  MInputMethodInfo *im_info;
  int ret;

  MINPUT__INIT ();

  if (variable == Mnil)
    MERROR (MERROR_IM, -1);
  plist = minput_get_variable (language, name, variable);
  plist = MPLIST_PLIST (plist);
  plist = MPLIST_NEXT (plist);
  pl = mplist ();
  mplist_add (pl, MPLIST_KEY (plist), value);
  ret = minput_config_variable (language, name, variable, pl);
  M17N_OBJECT_UNREF (pl);
  if (ret == 0)
    {
      im_info = get_im_info (language, name, Mnil, Mvariable);
      im_info->tick = 0;
    }
  return ret;
}

/*=*/

/***en
    @brief Get information about input method commands.

    The minput_get_commands () function returns information about
    input method commands of the input method specified by $LANGUAGE
    and $NAME.  An input method command is a pseudo key event to which
    one or more actual input key sequences are assigned.

    There are two kinds of commands, global and local.  Global
    commands are used by multiple input methods for the same purpose,
    and have global key assignments.  Local commands are used only by
    a specific input method, and have only local key assignments.

    Each input method may locally change key assignments for global
    commands.  The global key assignment for a global command is
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
    value is an M-text describing the command.

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

    $NAME が #Mnil であれば、グローバルコマンドに関する情報を返す。この
    場合、$LANGUAGE は無視される。

    $NAME が #Mnil でなければ、$LANGUAGE と $NAME によって指定される入
    力メソッドに置けるローカルなキー割り当てを持つコマンドに関する情報
    を返す。

    @return
    入力メソッドコマンドが見つからなければ、この関数は @c NULL を返す。

    そうでなければプロパティリストへのポインタを返す。リストの各要素の
    キーは個々のコマンドを示すシンボルであり、値は下記の COMMAND-INFO
    の形式のプロパティリストである。

    COMMAND-INFO の第一要素のキーは #Mtext または #Msymbol である。キー
    が #Mtext なら、値はそのコマンドを説明する M-text である。キーが
    #Msymbol なら値は #Mnil であり、このコマンドは説明テキストを持たな
    いことになる。

    それ以外の要素が無ければ、このコマンドに対してキーシークエンスが割
    り当てられていないことを意味する。そうでなければ、残りの各要素はキ
    ーとして#Mplist を、値としてプロパティリストを持つ。このプロパティ
    リストのキーは #Msymbol であり、値は現在そのコマンドに割り当てられ
    ている入力キーを表すシンボルである。

    返されるプロパティリストはライブラリによって管理されており、呼び出
    し側で変更したり解放したりしてはならない。*/

MPlist *
minput_get_commands (MSymbol language, MSymbol name)
{
  MInputMethodInfo *im_info;
  MPlist *cmds;

  MINPUT__INIT ();

  im_info = get_im_info (language, name, Mnil, Mcommand);
  if (! im_info || ! im_info->configured_vars)
    return NULL;
  M17N_OBJECT_UNREF (im_info->bc_cmds);
  im_info->bc_cmds = mplist ();
  MPLIST_DO (cmds, im_info->configured_cmds)
    {
      MPlist *plist = MPLIST_PLIST (cmds);
      MPlist *elt = mplist ();

      mplist_push (im_info->bc_cmds, Mplist, elt);
      mplist_add (elt, MPLIST_SYMBOL (plist),
		  mplist_copy (MPLIST_NEXT (plist)));
      M17N_OBJECT_UNREF (elt);
    }
  return im_info->bc_cmds;
}

/*=*/

/***en
    @brief Assign a key sequence to an input method command (obsolete).

    This function is obsolete.  Use minput_config_command () instead.

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
    returned, and #merror_code is set to @c MERROR_IM.  */
/***ja
    @brief 入力メソッドコマンドにキーシークエンスを割り当てる.

    関数 minput_assign_command_keys () は、 $LANGUAGE と $NAME によって
    指定された入力メソッド用の入力メソッドコマンド $COMMAND に対して、
    入力キーシークエンス $KEYSEQ を割り当てる。 $NAME が #Mnil ならば、
    $LANGUAGE に関係なく、入力キーシークエンスはグローバルに割り当てら
    れる。そうでなれば、割り当てはローカルである。

    $KEYSEQ の各要素はキーとして $Msymbol を、値として入力キーを表すシ
    ンボルを持たなくてはならない。

    $KEYSEQ は @c NULL でもよい。この場合、グローバルもしくはローカルな
    すべての割り当てが消去される。

    この割り当ては、割り当て以降新しくオープンされた入力メソッドから有
    効になる。

    @return 
    処理が成功すれば 0 を返す。そうでなければ -1 を返し、
    #merror_code を @c MERROR_IM に設定する。  */

int
minput_assign_command_keys (MSymbol language, MSymbol name,
			    MSymbol command, MPlist *keyseq)
{
  int ret;

  MINPUT__INIT ();

  if (command == Mnil)
    MERROR (MERROR_IM, -1);
  if (keyseq)
    {
      MPlist *plist;

      if  (! check_command_keyseq (keyseq))
	MERROR (MERROR_IM, -1);
      plist = mplist ();
      mplist_add (plist, Mplist, keyseq);
      keyseq = plist;
    }  
  else
    keyseq = mplist ();
  ret = minput_config_command (language, name, command, keyseq);
  M17N_OBJECT_UNREF (keyseq);
  return ret;
}

/*=*/

/***en
    @brief Call a callback function

    The minput_callback () functions calls a callback function
    $COMMAND assigned for the input context $IC.  The caller must set
    specific elements in $IC->plist if the callback function requires.

    @return
    If there exists a specified callback function, 0 is returned.
    Otherwise -1 is returned.  By side effects, $IC->plist may be
    modified.  */

int
minput_callback (MInputContext *ic, MSymbol command)
{
  MInputCallbackFunc func;

  if (! ic->im->driver.callback_list)
    return -1;
  func = ((MInputCallbackFunc)
	  mplist_get_func (ic->im->driver.callback_list, command));
  if (! func)
    return -1;
  (func) (ic, command);
  return 0;
}

/*** @} */ 
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

    関数 mdebug_dump_im () は入力メソッド $IM を stderr 
    に人間に可読な形で印刷する。$INDENT は２行目以降のインデントを指定する。

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
