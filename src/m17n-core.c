/* m17n-core.c -- body of the CORE API.
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
    @addtogroup m17nIntro
    @brief Introduction to the m17n library

    <em>API LEVELS</em>

    The API of the m17n library is divided into these four.

    <ol>
    <li> CORE API

    It provides basic modules to handle M-texts.  They don't require
    the m17n database.  To use this API, an application program must
    include <m17n-core.h> and be linked by -lm17n-core.

    <li> SHELL API

    It provides modules that utilize the m17n database.  They load
    various kinds of data from the database on demand.  To use this
    API, an application program must include <m17n.h> and be linked by
    -lm17n-core -lm17n.  With that, CORE API is also available.

    <li> GUI API

    It provides GUI modules such as drawing and inputting M-texts on a
    window system.  The API itself is independent on a window system,
    but the m17n library must be configured to use a specific window
    system.  Currently, we support only the X Window System.  To use
    this API, an application program must include <m17n-gui.h> and
    <m17n-X.h>, and be linked by -lm17n-core -lm17n -lm17n-X.  With
    that, CORE and SHELL APIs are also available.

    <li> MISC API

    It provides miscellaneous functions to support error handling and
    debugging.  This API can't be used by itself, but with one or more
    APIs listed above.  To use the API, an application program must
    include <m17n-misc.h> in addition to one of <m17n-core.h>,
    <m17n.h>, and <m17n-gui.h>.

    </ol>

    See also the section @ref m17n-config "m17n-config(1)".

    <em>ENVIRONMENT VARIABLE</em>

    The m17n library pays attention to these environment variables.

    <ul>
    <li> @c M17NDIR

    Name of a directory that contains data of the m17n database.  See
    @ref m17nDatabase for the detail.

    <li> @c MDEBUG_XXXX

    Environment variables whose name start by "MDEBUG_" controls
    printing of debug information.  See @ref m17nDebug for the detail.

    </ul>

    <em>API NAMING CONVENTION</em>

    The library exports functions, variables, macros, and types.  All
    of them start by the letter 'm' or 'M' followed by an object name
    (e.g. "symbol" and "plist", but "mtext" object is given the name
    "text" to avoid double 'm' at the head) or a module name
    (e.g. draw, input).
    
    <ul>

    <li> functions -- mobject () or mobject_xxx ()

    They start with 'm' followed by lower case object name.  For
    example, msymbol (), mtext_ref_char (), mdraw_text ().

    <li> non-symbol variables -- mobject, or mobject_xxx
    
    The naming convention is the same as functions (e.g. mface_large).

    <li> symbol variables -- Mname

    Variables of type MSymbol start with 'M' followed by their names
    (e.g. Mlanguage (name is "langauge"), Miso_2022 (name is
    "iso-2022").

    <li> macross -- MOBJECT_XXX

    They start by 'M' followed by upper case object names.

    <li> types -- MObject or MObjectXxx

    They start by 'M' followed by capitalized object names (e.g.
    MConverter, MInputDriver).

    </ul>

  */

/***ja
    @addtogroup m17nIntro

    @e m17nライブラリ は C 言語用多言語情報処理ライブラリである。
    このライブラリは、多言語文書を扱うために必要な以下の基本機能をアプ
    リケーションプログラムに提供する。

    @li 多言語テキスト用オブジェクトとしての構造体 @e M-text

    @li M-text を扱うための多くの関数・マクロ群

    @li 種々のフォーマットでエンコードされたテキストと M-text 間の変換
    を行なうデコーダ／エンコーダ

    @li Unicode の全ての文字に加え、それと同じ数の非 Unicode 文字を扱
    える巨大な文字空間

    @li 文字毎の情報を効率良く格納する構造体 @e 文字テーブル (CharTable)

    @li オプション：M-textのウィンドウシステム上での表示・入力システム。
    （このための API は m17n-win.h に含まれている。）  */
/*=*/
/*** @{ */
#ifdef FOR_DOXYGEN
/***en
    The #M17NLIB_MAJOR_VERSION macro gives the major version number
    of the m17n library.  */

/***ja
    マクロ #M17NLIB_MAJOR_VERSION は m17n ライブラリのメジャーバージョ
    ン番号を与える。  */

#define M17NLIB_MAJOR_VERSION

/*=*/

/***en
    The #M17NLIB_MINOR_VERSION macro gives the minor version number
    of the m17n library.  */

/***ja
    マクロ #M17NLIB_MINOR_VERSION は m17n ライブラリのマイナーバージョ
    ン番号を与える。  */

#define M17NLIB_MINOR_VERSION

/*=*/

/***en
    The #M17NLIB_PATCH_LEVEL macro gives the patch level number
    of the m17n library.  */

/***ja
    マクロ #M17NLIB_PATCH_LEVEL は m17n ライブラリのパッチレベル番号を
    与える。  */

#define M17NLIB_PATCH_LEVEL

/*=*/

/***en
    The #M17NLIB_VERSION_NAME macro gives the version name of the
    m17n library as a string.  */

/***ja
    マクロ #M17NLIB_VERSION_NAME は m17n ライブラリのバージョン名を
    C-string の形で与える。  */

#define M17NLIB_VERSION_NAME

/*=*/

/***en
    @brief Initialize the m17n library.

    The macro M17N_INIT () initializes the m17n library.  This
    function must be called before any m17n functions are used.

    If the initialization was successful, the external variable @c
    merror_code is set to 0.  Otherwise it is set to -1.  */

/***ja
    @brief m17n ライブラリの初期化

    マクロ M17N_INIT () は m17n ライブラリを初期化する。m17n ライブラ
    リを使うときは、最初にこの関数を呼ばなくてはならない。

    変数 #merror_code は、初期化が成功すれば 0 に、そうでなければ -1 
    に設定される。  */

#define M17N_INIT()

/*=*/

/***en
    @brief Finalize the m17n library.

    The macro M17N_FINI () finalizes the m17n library.  It frees all the
    memory area used by the m17n library.  Once this function is
    called, no m17n functions should be used until the
    macro M17N_INIT () is called again.  */

/***ja
    @brief m17n ライブラリの終了  */

#define M17N_FINI()
#endif /* FOR_DOXYGEN */
/*=*/
/*** @} */ 
/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "m17n-core.h"
#include "m17n-misc.h"
#include "internal.h"

static int core_initialized;

static void
default_error_handler (enum MErrorCode err)
{
  exit (err);
}

static struct timeval time_stack[16];
static int time_stack_index;

static int report_header_printed;


/* Internal API */

void
mdebug__report_object (char *name, M17NObjectArray *array)
{
  if (! (mdebug__flag & MDEBUG_FINI))
    return;
  if (! report_header_printed)
    {
      fprintf (stderr, "%16s %7s %7s %7s\n",
	       "object", "created", "freed", "alive");
      fprintf (stderr, "%16s %7s %7s %7s\n",
	       "------", "-------", "-----", "-----");
      report_header_printed = 1;
    }
  fprintf (stderr, "%16s %7d %7d %7d\n", name,
	   array->used, array->used - array->count, array->count);
}


void *(*mdatabase__finder) (MSymbol tag1, MSymbol tag2,
			   MSymbol tag3, MSymbol tag4);
void *(*mdatabase__loader) (void *);

int mdebug__flag;

void
mdebug__push_time ()
{
  struct timezone tz;

  gettimeofday (time_stack + time_stack_index++, &tz);
}

void
mdebug__pop_time ()
{
  time_stack_index--;
}

void
mdebug__print_time ()
{
  struct timeval tv;
  struct timezone tz;
  long diff;

  gettimeofday (&tv, &tz);
  diff = ((tv.tv_sec - time_stack[time_stack_index - 1].tv_sec) * 1000000
	  + (tv.tv_usec - time_stack[time_stack_index - 1].tv_usec));
  fprintf (stderr, "%8ld ms.", diff);
  time_stack[time_stack_index - 1] = tv;
}

#define SET_DEBUG_FLAG(env_name, mask)		\
  do {						\
    char *env_value = getenv (env_name);	\
						\
    if (env_value && env_value[0] == '1')	\
      mdebug__flag |= (mask);			\
  } while (0)



/* External API */

/* The following two are actually not exposed to a user but concealed
   by the macro M17N_INIT (). */

void
m17n_init_core (void)
{
  int mdebug_mask = MDEBUG_INIT;

  if (core_initialized)
    return;

  merror_code = MERROR_NONE;
  m17n_memory_full_handler = default_error_handler;

  mdebug__flag = 0;
  SET_DEBUG_FLAG ("MDEBUG_INIT", MDEBUG_INIT);
  SET_DEBUG_FLAG ("MDEBUG_FINI", MDEBUG_FINI);
  SET_DEBUG_FLAG ("MDEBUG_CHARSET", MDEBUG_CHARSET);
  SET_DEBUG_FLAG ("MDEBUG_CODING", MDEBUG_CODING);
  SET_DEBUG_FLAG ("MDEBUG_DATABASE", MDEBUG_DATABASE);
  SET_DEBUG_FLAG ("MDEBUG_FONT", MDEBUG_FONT); 
  SET_DEBUG_FLAG ("MDEBUG_FONT_FLT", MDEBUG_FONT_FLT);
  SET_DEBUG_FLAG ("MDEBUG_FONT_OTF", MDEBUG_FONT_OTF);
  SET_DEBUG_FLAG ("MDEBUG_INPUT", MDEBUG_INPUT);

  MDEBUG_PUSH_TIME ();
  MDEBUG_PUSH_TIME ();
  if (msymbol__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize symbol module."));
  if  (mplist__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize plist module."));
  if (mtext__init () < 0)
    goto err;
  if (mtext__prop_init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize mtext module."));
  if  (mchartable__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize chartable module."));

  mdatabase__finder = NULL;
  mdatabase__loader = NULL;
  core_initialized = 1;

 err:
  MDEBUG_POP_TIME ();
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize the core modules."));
  MDEBUG_POP_TIME ();

  report_header_printed = 0;
}

void
m17n_fini_core (void)
{
  int mdebug_mask = MDEBUG_FINI;

  if (core_initialized)
    {
      MDEBUG_PUSH_TIME ();
      MDEBUG_PUSH_TIME ();
      MDEBUG_PRINT_TIME ("INIT", (stderr, " to finalize chartable module."));
      mchartable__fini ();
      MDEBUG_PRINT_TIME ("INIT", (stderr, " to finalize textprop module."));
      mtext__prop_fini ();
      MDEBUG_PRINT_TIME ("INIT", (stderr, " to finalize mtext module."));
      mtext__fini ();
      MDEBUG_PRINT_TIME ("INIT", (stderr, " to finalize symbol module."));
      msymbol__fini ();
      MDEBUG_PRINT_TIME ("INIT", (stderr, " to finalize plist module."));
      mplist__fini ();
      core_initialized = 0;
      MDEBUG_POP_TIME ();
      MDEBUG_PRINT_TIME ("INIT", (stderr, " to finalize the core modules."));
      MDEBUG_POP_TIME ();
    }
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */
/*=*/

/***en
    @addtogroup m17nObject
    @brief Managed objects are objects managed by the reference count.

    There are some types of m17n objects that are managed by their
    reference count.  Those objects are called @e managed @e objects.
    When created, the reference count of a managed object is
    initialized to one.  The m17n_object_ref () function increments
    the reference count of a managed object by one, and the
    m17n_object_unref () function decrements by one.  A managed
    object is automatically freed when its reference count becomes
    zero.

    A property whose key is a managing key can have only a managed
    object as its value.  Such functions as msymbol_put () and
    mplist_put () pay special attention to such a property.

    In addition to the predefined managed object types, users can
    define their own managed object types.  See the documentation of
    the m17n_object () for the details.  */

/*** @{  */
/*=*/
/***en
    @brief Allocate a managed object.

    The m17n_object () function allocates a new managed object of
    $SIZE bytes and sets its reference count to 1.  $FREER is the
    function that is used to free the object when the reference count
    becomes 0.  If $FREER is NULL, the object is freed by the free ()
    function.

    The heading bytes of the allocated object is occupied by
    #M17NObjectHead.  That area is reserved for the m17n library and
    application programs should never touch it.

    @return
    This function returns a newly allocated object.

    @errors
    This function never fails.  */

#if EXAMPLE_CODE
typedef struct
{
  M17NObjectHead head;
  int mem1;
  char *mem2;
} MYStruct;

void
my_freer (void *obj)
{
  free (((MYStruct *) obj)->mem2);
  free (obj);
}

void
my_func (MText *mt, MSymbol key, int num, char *str)
{
  MYStruct *st = m17n_object (sizeof (MYStruct), my_freer);

  st->mem1 = num;
  st->mem2 = strdup (str);
  /* KEY must be a managing key.   */
  mtext_put_prop (mt, 0, mtext_len (mt), key, st);
  /* This sets the reference count of ST back to 1.  */
  m17n_object_unref (st);
}
#endif

void *
m17n_object (int size, void (*freer) (void *))
{
  M17NObject *obj = malloc (size);

  obj->ref_count = 1;
  obj->u.freer = freer;
  return obj;
}

/*=*/

/***en
    @brief Increment the reference count of a managed object.

    The m17n_object_ref () function increments the reference count of
    the managed object pointed to by $OBJECT.

    @return
    This function returns the resulting reference count if it fits in
    a 16-bit unsigned integer (i.e. less than 0x10000).  Otherwise, it
    return -1.

    @errors
    This function never fails.  */

int
m17n_object_ref (void *object)
{
  M17NObject *obj = (M17NObject *) object;
  M17NObjectRecord *record;
  unsigned *count;

  if (! obj->ref_count_extended)
    {
      if (++obj->ref_count)
	return (int) obj->ref_count;
      MSTRUCT_MALLOC (record, MERROR_OBJECT);
      record->freer = obj->u.freer;
      MLIST_INIT1 (record, counts, 1);
      MLIST_APPEND1 (record, counts, 0, MERROR_OBJECT);
      obj->u.record = record;
      obj->ref_count_extended = 1;
    }
  else
    record = obj->u.record;

  count = record->counts;
  while (*count == 0xFFFFFFFF)
    *(count++) = 0;
  (*count)++;
  if (*count == 0xFFFFFFFF)
    MLIST_APPEND1 (record, counts, 0, MERROR_OBJECT);
  return -1;
}

/*=*/

/***en
    @brief Decrement the reference count of a managed object.

    The m17n_object_unref () function decrements the reference count
    of the managed object pointed to by $OBJECT.  When the reference
    count becomes zero, the object is freed by its freer function.

    @return
    This function returns the resulting reference count if it fits in
    a 16-bit unsigned integer (i.e. less than 0x10000).  Otherwise, it
    returns -1.  Thus, the return value zero means that $OBJECT is
    freed.

    @errors
    This function never fails.  */

int
m17n_object_unref (void *object)
{
  M17NObject *obj = (M17NObject *) object;
  M17NObjectRecord *record;
  unsigned *count;

  if (! obj->ref_count_extended)
    {
      if (! --obj->ref_count)
	{
	  if (obj->u.freer)
	    (obj->u.freer) (object);
	  else
	    free (object);
	  return 0;
	}
      return (int) obj->ref_count;
    }

  record = obj->u.record;
  count = record->counts;
  while (! *count)
    *(count++) = 0xFFFFFFFF;
  (*count)--;
  if (! record->counts[0])
    {
      obj->ref_count_extended = 0;
      obj->ref_count--;
      obj->u.freer = record->freer;
      MLIST_FREE1 (record, counts);
      free (record);
    }
  return -1;
}

/*=*/

/*** @} */

/***en
    @addtogroup m17nError Error handling
    @brief Error handling of the m17n library.

    There are two types of errors that may happen in a function of
    the m17n library.

    The first type is argument errors.  When a library function is
    called with invalid arguments, it returns a value that indicates
    error and at the same time sets the external variable @e
    merror_code to a non-zero integer.

    The second type is memory allocation errors.  When the required
    amount of memory is not available on the system, m17n library
    functions call a function pointed to by the external variable @c
    m17n_memory_full_handler.  The default value of the variable is a
    pointer to the default_error_handle () function, which just calls
    exit ().  */

/***ja
    @addtogroup m17nError エラー処理
    @brief m17n ライブラリのエラー処理

    m17n ライブラリの関数では、２つの種類のエラーが起こり得る。

    一つは引数のエラーである。ライブラリの関数が妥当でない引数とともに
    呼ばれた場合、その関数はエラーを意味する値を返し、同時に外部変数 
    #merror_code にゼロでない整数をセットする。

    もう一つの種類はメモリ割当てエラーである。システムが必要な量のメモ
    リを割当てることができない場合、ライブラリ関数は外部変数 @c
    m17n_memory_full_handler が指す関数を呼ぶ。デフォルトでは、単に
    <tt>exit ()</tt> を呼ぶことになっている。
*/

/*** @{ */

/*=*/

/***en @brief External variable to hold error code of the m17n library

    The external variable #merror_code holds an error code of the
    m17n library.  When a library function is called with an invalid
    argument, it sets this variable to one of @c enum #MErrorCode.

    This variable initially has the value 0.  */

/***ja @brief m17n ライブラリのエラーコードを保持する外部変数

    外部変数 #merror_code は、m17n ライブラリのエラーコードを保持する。
    ライブラリ関数が妥当でない引数とともに呼ばれた際には、
    この変数を @c enum #MErrorCode の一つにセットする。

    この変数の初期値は０である。  */

enum MErrorCode merror_code;

/*=*/

/***en @brief Memory allocation error handler

    The external variable #m17n_memory_full_handler holds a pointer
    to the function to call when a library function failed to allocate
    memory.  $ERR is one of @c enum #MErrorCode indicating in which
    function the error occurred.

    This variable initially points a function that simply calls the
    exit () function with $ERR as an argument.

    An application program that needs a different error handling can
    change this variable to point a proper function.  */

/***ja @brief メモリ割当てエラーハンドラ

    変数 #m17n_memory_full_handler は、ライブラリ関数がメモリ割当て
    に失敗した際に呼ぶべき関数へのポインタである。$ERR は @c enum
    #MErrorCode のいずれかであり、どのライブラリ関数でエラーが起った
    かを示す。

    @anchor test

    初期設定では、この変数は単に <tt>exit ()</tt> を $ERR を引数として
    呼ぶ関数を指している。

    これとは異なるエラー処理を必要とするアプリケーションは、この変数を
    適当な関数に設定することで、目的を達成できる。  */

void (*m17n_memory_full_handler) (enum MErrorCode err);

/*** @} */

/*=*/

/***en
    @addtogroup m17nDebug
    @brief Support for m17n library users to debug their programs.

    The m17n library provides the following facilities to support the
    library users to debug their programs.

    <ul>

    <li> Environment variables to control printing of various
    information.

    <ul>

    <li> MDEBUG_INIT -- If set to 1, print information about the
    library initialization on the call of M17N_INIT ().

    <li> MDEBUG_FINI -- If set to 1, print counts of objects that are
    not yet freed on the call of M17N_FINI ().

    <li> MDEBUG_CHARSET -- If set to 1, print information about
    charsets being loaded from the m17n database.

    <li> MDEBUG_CODING -- If set to 1, print information about coding
    systems being loaded from the m17n database.

    <li> MDEBUG_DATABASE -- If set to 1, print information about
    data being loaded from the m17n database.

    <li> MDEBUG_FONT -- If set to 1, print information about fonts
    being selected and opened.

    <li> MDEBUG_FONT_FLT -- If set to 1, print information about which
    command of Font Layout Table are being executed.

    <li> MDEBUG_FONT_OTF -- If set to 1, print information about which
    feature of OpenType Layout Table are being executed.

    <li> MDEBUG_INPUT -- If set to 1, print information about how an
    input method is running.

    <li> MDEBUG_ALL -- Setting this variable to 1 is equivalent to
    setting all the above variables to 1.

    </ul>

    <li> Functions to print various objects in a human readable way.
    See the documentation of mdebug_dump_XXXX () functions.

    <li> The hook function called on an error.  See the documentation
    of mdebug_hook ().

    </ul>
*/

/*=*/
/*** @{ */
/*=*/

/***en
    @brief Hook function called on an error.

    The mdebug_hook () function is called when an error happens.  It
    returns -1q without doing anything.  It is useful to set a break
    point on this function in a debugger.  */ 

int
mdebug_hook ()
{
  return -1;
}

/*=*/

/*** @} */ 

/*
  Local Variables:
  coding: euc-japan
  End:
*/
