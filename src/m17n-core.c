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
    @brief Introduction to the m17n library.

    <em>API LEVELS</em>

    The API of the m17n library is divided into these four.

    <ol>
    <li> CORE API

    It provides basic modules to handle M-texts.  They do not require
    the m17n database.  To use this API, an application program must
    include <m17n-core.h> and be linked by -lm17n-core.

    <li> SHELL API

    It provides modules that utilize the m17n database (code
    conversion, character property, etc).  They load various kinds of
    data from the database on demand.  To use this API, an application
    program must include <m17n.h> and be linked by -lm17n-core -lm17n.

    When you use this API, CORE API is also available.

    <li> GUI API

    It provides GUI modules such as drawing and inputting M-texts on a
    graphic device.  The API itself is independent on a graphic device
    but most functions require an argument MFrame which is created for
    a specific type of graphic device.  Currently, a null device, X
    Window System, and an image date (gdImagePtr) of GD library are
    supported as a graphic device.

    On a frame of the null device, you can't draw text nor use an
    input method (but, for instance, the funciton mdraw_glyph_list ()
    is available).

    On a frame of the X Window System, you can use the whole GUI API.

    On a frame of GD library, you can use all drawing API but can't
    use input method.

    To use this device, an application program must include
    <m17n-gui.h> and be linked by -lm17n-core -lm17n -lm17n-gui.

    When you use this API, CORE and SHELL API are also available.

    <li> MISC API

    It provides miscellaneous functions to support error handling and
    debugging.  This API cannot be used by itself, but with one or
    more APIs listed above.  To use the API, an application program
    must include <m17n-misc.h> in addition to one of a header file
    described above.

    </ol>

    See also the section @ref m17n-config "m17n-config(1)".

    <em>ENVIRONMENT VARIABLE</em>

    The m17n library pays attention to these environment variables.

    <ul>
    <li> @c M17NDIR

    Name of a directory that contains data of the m17n database.  See
    @ref m17nDatabase for more details.

    <li> @c MDEBUG_XXXX

    Environment variables whose name start by "MDEBUG_" controls
    printing of debug information.  See @ref m17nDebug for more
    details.

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
    (e.g. Mlanguage (name is "language"), Miso_2022 (name is
    "iso-2022").

    <li> macros -- MOBJECT_XXX

    They start by 'M' followed by upper case object names.

    <li> types -- MObject or MObjectXxx

    They start by 'M' followed by capitalized object names (e.g.
    MConverter, MInputDriver).

    </ul>

  */

/***ja
    @addtogroup m17nIntro
    @brief m17n ライブラリ イントロダクション.

    @em APIのレベル

    m17n ライブラリの API は以下の４種に分類されている。

    <ol>
    <li> コア API

    M-text を扱うための基本的なモジュールを提供する。この分類の API は
    m17n データベースを必要としない。利用するためには、アプリケーショ
    ンプログラムは<m17n-core.h> を include し、 -lm17n-core でリンクさ
    れなくてはならない。

    <li> シェル API

    m17n データベースを利用するモジュール（コード変換、文字プロパティ
    等）を提供する。モジュールはデータベースから必要に応じて多様なデー
    タをロードする。利用するためには、アプリケーションプログラムは
    <m17n.h> を include し、 -lm17n-core -lm17n でリンクされなくてはな
    らない。

    この API を使用する際にはコア API も使用できる。

    <li> GUI API

    M-texts をグラフィックデバイス上で表示したり入力したりするといった、
    グラフィックユーザインタフェースモジュールを提供する。API 自体はグ
    ラフィックデバイスとは独立であるが、多くの関数は特定のグラフィック
    デバイス上に作成された MFrame を引数に取る。現時点では、ヌルデバイ
    ス、X ウィンドウシステム、GD ライブラリのイメージ（gdImagePtr）、が
    グラフィックデバイスとしてサポートされている。

    ヌルデバイス上では表示も入力もできない（ただしたとえば
    mdraw_glyph_list () などの関数は使用可能）。

    X ウィンドウシステム上ではすべての GUI API が使える。

    GD ライブラリのイメージ上では、描画用の API はすべて使えるが入力は
    できない。

    このデバイスを使用するためには、アプリケーションプログラムは
    <m17n-gui.h> を include し、-lm17n-core -lm17n -lm17n-gui でリンク
    されなくてはならない。

    この API を使用する際には、コア API とシェル API も使用できる。

    <li> その他の API

    エラー処理、デバッグ用のその他の関数を提供する。この API はそれだけ
    では使用できず、上記の他のものと共に使う。利用するためには、上記の
    いずれかの include ファイルに加えて、 <m17n-misc.h> を include しな
    くてはならない。

    </ol>

    @ref m17n-config "m17n-config(1)" 節も参照。

    @em 環境変数

    m17n ライブラリは次の環境変数を考慮する。

    <ul>
    <li> @c M17NDIR

    m17n データベースのデータを含むディレクトリの名前。詳細は@ref
    m17nDatabase 参照。

    <li> @c MDEBUG_XXXX

    "MDEBUG_" で始まる名前を持つ環境変数はデバッグ情報を制御する。詳細
    は@ref m17nDebug 参照。

    </ul>

    @em API @em の命名規則 コマンド

    ライブラリは、関数、変数、マクロ、型を export する。それらは'm' ま
    たは 'M' のあとにオブジェクト名("symbol" や "plist" など。ただし 
    "mtext" オブジェクトははじめの 'm' の重複を避けるため "text" を用
    いる。) またはモジュール名(draw, input など) を続けたものである。
    
    <ul>

    <li> 関数 -- mobject () または mobject_xxx ()

    'm' のあとに小文字でオブジェクト名が続く。たとえば、msymbol (),
     mtext_ref_char (), mdraw_text () など。

    <li> シンボルでない変数 -- mobject,  または mobject_xxx
    
    関数と同じ命名規則に従う。(たとえば  mface_large)

    <li> シンボル変数 -- Mname

    MSymbol 型変数は、'M' の後に名前が続く。たとえば Mlanguage (名前は 
    "language"), Miso_2022 (名前は"iso-2022")など。

    <li> マクロ -- MOBJECT_XXX

    'M' の後に大文字でオブジェクト名が続く。

    <li> タイプ -- MObject または MObjectXxx

    'M' の後に大文字で始めてオブジェクト名をが続く。 (たとえば
    MConverter, MInputDriver)

    </ul>
    
    */
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
    文字列として与える。  */

#define M17NLIB_VERSION_NAME

/*=*/

/***en
    @brief Initialize the m17n library.

    The macro M17N_INIT () initializes the m17n library.  This macro
    must be called before any m17n functions are used.

    If the initialization was successful, the external variable
    #merror_code is set to 0.  Otherwise it is set to -1.  */

/***ja
    @brief m17n ライブラリを初期化する.

    マクロ M17N_INIT () は m17n ライブラリを初期化する。m17n の関数を
    利用する前に、このマクロをまず呼ばなくてはならない。
    
    外部変数 #merror_code は、初期化が成功すれば 0 に、そうでなければ 
    -1 に設定される。  */

#define M17N_INIT()

/*=*/

/***en
    @brief Finalize the m17n library.

    The macro M17N_FINI () finalizes the m17n library.  It frees all the
    memory area used by the m17n library.  Once this macro is
    called, no m17n functions should be used until the
    macro M17N_INIT () is called again.  */
/***ja
    @brief m17n ライブラリを終了する. 

    マクロ M17N_FINI () は m17n ライブラリを終了する。m17n ライブラリ
    が使った全てのメモリ領域は解放される。一度このマクロが呼ばれたら、
    マクロ M17N_INIT () が再度呼ばれるまで m17n 関数は使うべきでない。
 */

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
  if (array->used > 0)
    {
      free (array->objects);
      array->count = array->used = 0;
    }
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


void
mdebug__register_object (M17NObjectArray *array, void *object)
{
  if (array->used == 0)
    MLIST_INIT1 (array, objects, 256);
  array->count++;
  MLIST_APPEND1 (array, objects, object, MERROR_OBJECT);
}

void
mdebug__unregister_object (M17NObjectArray *array, void *object)
{
  array->count--;
  if (array->count >= 0)
    {
      int i = 0;

      while (i < array->used && array->objects[i] != object) i++;
      if (i < array->used)
	array->objects[i] = NULL;
      else
	mdebug_hook ();
    }
  else									\
    mdebug_hook ();
}


/* External API */

/* The following two are actually not exposed to a user but concealed
   by the macro M17N_INIT (). */

void
m17n_init_core (void)
{
  int mdebug_mask = MDEBUG_INIT;

  if (core_initialized++)
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

 err:
  MDEBUG_POP_TIME ();
  MDEBUG_PRINT_TIME ("INIT", (stderr, " to initialize the core modules."));
  MDEBUG_POP_TIME ();
}

void
m17n_fini_core (void)
{
  int mdebug_mask = MDEBUG_FINI;

  if (core_initialized > 1)
    core_initialized--;
  else
    {
      core_initialized = 0;
      MDEBUG_PUSH_TIME ();
      MDEBUG_PUSH_TIME ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize chartable module."));
      mchartable__fini ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize textprop module."));
      mtext__prop_fini ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize mtext module."));
      mtext__fini ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize symbol module."));
      msymbol__fini ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize plist module."));
      mplist__fini ();
      MDEBUG_POP_TIME ();
      MDEBUG_PRINT_TIME ("FINI", (stderr, " to finalize the core modules."));
      MDEBUG_POP_TIME ();
    }
  report_header_printed = 0;
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
    the m17n_object () for more details.  */
/***ja
    @addtogroup m17nObject
    @brief 管理下オブジェクトとは参照数によって管理されているオブジェクトである.

    m17n オブジェクトのある型のものは、参照数によって管理されている。
    それらのオブジェクトは @e 管理下オブジェクト と呼ばれる。生成され
    た時点での参照数は 0 に初期化されている。関数 m17n_object_ref () 
    は管理下オブジェクトの参照数を 1 増やし、関数m17n_object_unref () 
    は 1 減らす。参照数が 0 になった管理下オブジェクトは自動的に解放さ
    れる。

    キーが管理キーであるプロパティは、値として管理下オブジェクトだけを
    取る。関数 msymbol_put () や mplist_put () などはそれらのプロパティ
    を特別扱いする。

    定義済み管理下オブジェクトタイプの他に、ユーザは必要な管理下オブジェ
    クトタイプを自分で定義することができる。詳細はm17n_object () の説
    明を参照。  */

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

/***ja
    @brief 管理下オブジェクトを割り当てる.

    関数 m17n_object () は$SIZE バイトの新しい管理下オブジェクトを割り
    当て、その参照数を 1 とする。 $FREER は参照数が 0 になった際にその
    オブジェクトを解放するために用いられる関数である。$FREER が NULLな
    らば、オブジェクトは関数 free () によって解放される。

    割り当てられたオブジェクト冒頭のバイトは、#M17NObjectHead が占める。 
    この領域は m17n ライブラリが使用するので、アプリケーションプログラ
    ムは触れてはならない。

    @return
    この関数は新しく割り当てられたオブジェクトを返す。

    @errors
    この関数は失敗しない。    */

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
/***ja
    @brief 管理下オブジェクトの参照数を 1 増やす.

    関数 m17n_object_ref () は $OBJECT で指される管理下オブジェクトの
    参照数を 1 増やす。

    @return 
    この関数は、増やした参照数が 16 ビットの符号無し整数値(すなわち 
    0x10000 未満)におさまれば、それを返す。そうでなければ -1 を返す。

    @errors
    この関数は失敗しない。    */

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
/***ja
    @brief 管理下オブジェクトの参照数を 1 減らす.

    関数 m17n_object_unref () は $OBJECT で指される管理下オブジェクト
    の参照数を 1 減らす。参照数が 0 になれば、オブジェクトは解放関数に
    よって解放される。

    @return 
    この関数は、減らした参照数が 16 ビットの符号無し整数値(すなわち 
    0x10000 未満)におさまれば、それを返す。そうでなければ -1 を返す。
    つまり、0 が返って来た場合は$OBJECT は解放されている。

    @errors
    この関数は失敗しない。    */
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
    error and at the same time sets the external variable #merror_code
    to a non-zero integer.

    The second type is memory allocation errors.  When the required
    amount of memory is not available on the system, m17n library
    functions call a function pointed to by the external variable @c
    m17n_memory_full_handler.  The default value of the variable is a
    pointer to the default_error_handle () function, which just calls
    <tt> exit ()</tt>.  */

/***ja
    @addtogroup m17nError エラー処理
    @brief m17n ライブラリのエラー処理.

    m17n ライブラリの関数では、２つの種類のエラーが起こり得る。

    一つは引数のエラーである。ライブラリの関数が妥当でない引数とともに
    呼ばれた場合、その関数はエラーを意味する値を返し、同時に外部変数 
    #merror_code にゼロでない整数をセットする。

    もう一つの種類はメモリ割当てエラーである。システムが必要な量のメモ
    リを割当てることができない場合、ライブラリ関数は外部変数 @c
    m17n_memory_full_handler が指す関数を呼ぶ。デフォルトでは、関数 
    default_error_handle () を指しており、この関数は単に <tt>exit
    ()</tt> を呼ぶ。
*/

/*** @{ */

/*=*/

/***en 
    @brief External variable to hold error code of the m17n library.

    The external variable #merror_code holds an error code of the
    m17n library.  When a library function is called with an invalid
    argument, it sets this variable to one of @c enum #MErrorCode.

    This variable initially has the value 0.  */

/***ja 
    @brief m17n ライブラリのエラーコードを保持する外部変数.

    外部変数 #merror_code は、m17n ライブラリのエラーコードを保持する。
    ライブラリ関数が妥当でない引数とともに呼ばれた際には、この変数を 
    @c enum #MErrorCode の一つにセットする。

    この変数の初期値は 0 である。  */

enum MErrorCode merror_code;

/*=*/

/***en 
    @brief Memory allocation error handler.

    The external variable #m17n_memory_full_handler holds a pointer
    to the function to call when a library function failed to allocate
    memory.  $ERR is one of @c enum #MErrorCode indicating in which
    function the error occurred.

    This variable initially points a function that simply calls the
    <tt>exit </tt>() function with $ERR as an argument.

    An application program that needs a different error handling can
    change this variable to point a proper function.  */

/***ja 
    @brief メモリ割当てエラーハンドラ.

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
/***ja
    @addtogroup m17nDebug
    @brief m17n ライブラリユーザのためのプログラムデバッグサポート.

    m17n ライブラリは、そのユーザが自分のプログラムをデバッグするため
    に、以下の機能をサポートしている。

    <ul>

    <li> さまざまな情報のプリントを制御する環境変数。

    <ul>

    <li> MDEBUG_INIT -- 1 ならば、M17N_INIT () が呼ばれた時点で、ライ
    ブラリの初期化に関する情報をプリントする。

    <li> MDEBUG_FINI -- 1 ならば、M17N_FINI () が呼ばれた時点で、まだ
    解放されていないオブジェクトの参照数をプリントする。

    <li> MDEBUG_CHARSET -- 1 ならば、m17n データベースからロードされた
    文字セットについての情報をプリントする。

    <li> MDEBUG_CODING --  1 ならば、m17n データベースからロードされた
    コード系についての情報をプリントする。

    <li> MDEBUG_DATABASE -- 1 ならば、m17n データベースからロードされ
    たデータについての情報をプリントする。

    <li> MDEBUG_FONT -- 1 ならば、選択されてオープンされたフォントにつ
    いての情報をプリントする。

    <li> MDEBUG_FONT_FLT -- 1 ならば、Font Layout Table のどのコマンド
    が実行中かについてのをプリントする。

    <li> MDEBUG_FONT_OTF -- 1 ならば、OpenType Layout Table のどの属性
    が実行中かについてのをプリントする。

    <li> MDEBUG_INPUT -- 1 ならば、実行中の入力メソッドの状態に付いて
    の情報をプリントする。

    <li> MDEBUG_ALL -- 1 ならば、上記すべての変数を 1 にしたのと同じ効
    果を持つ。

    </ul>

    <li> 種々のオブジェクトを人間に可読な形でプリントする関数。詳細は
    関数 mdebug_dump_XXXX () の説明参照。

    <li> エラー発生時に呼ばれるフック関数。mdebug_hook () の説明参照。

    </ul>
*/

/*=*/
/*** @{ */
/*=*/

/***en
    @brief Hook function called on an error.

    The mdebug_hook () function is called when an error happens.  It
    returns -1 without doing anything.  It is useful to set a break
    point on this function in a debugger.  */ 
/***ja
    @brief エラーの際に呼ばれるフック関数.

    関数 mdebug_hook () はエラーが起こった際に呼ばれ、何もせずに-1 を
    返す。デバッガ内でブレークポイントを設定するために用いることができ
    る。
    */ 

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
