/* symbol.c -- symbol module.
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
    @addtogroup m17nSymbol

    @brief Symbol objects and API for them.

    The m17n library uses objects called @e symbols as unambiguous
    identifiers.  Symbols are similar to atoms in the X library, but a
    symbol can have zero or more @e symbol @e properties.  A symbol
    property consists of a @e key and a @e value, where key is also a
    symbol and value is anything that can be cast to <tt>(void *)</tt>.  
    "The symbol property that belongs to the symbol S and
    whose key is K" may be shortened to "K property of S".

    Symbols are used mainly in the following three ways.

    @li As keys of symbol properties and other properties.

    @li To represent various objects, e.g. charsets, coding systems,
    fontsets.

    @li As arguments of the m17n library functions to control
    their behavior.

    There is a special kind of symbol, a @e managing @e key.  The
    value of a property whose key is a managing key must be a @e
    managed @e object.  See @ref m17nObject for the detail.
*/
/***ja
    @addtogroup m17nSymbol シンボル

    @brief シンボルオブジェクトとそれに関する API.

    m17n ライブラリは一意に決まる識別子として @e シンボル 
    と呼ぶオブジェクトを用いる。シンボルは X ライブラリのアトムと似ているが、
    0 個以上の @e シンボルプロパティ を持つことができる。シンボルプロパティは
    @e キー と @e 値 からなる。キーはそれ自体シンボルであり、値は 
    <tt>(void *)</tt> 型にキャストできるものなら何でもよい。「シンボル 
    S が持つシンボルプロパティのうちキーが K のもの」を簡単に「S の K 
    プロパティ」と呼ぶことがある。

    シンボルの用途は主に以下の3通りである。

    @li シンボルプロパティおよび他のプロパティのキーを表す。

    @li 文字セット、コード系、フォントセットなどの各種オブジェクトを表す。

    @li m17n ライブラリ関数の引数となり、関数の挙動を制御する。

    @e 管理キー と呼ばれる特別なシンボルがあり、管理キーをキーとして持つプロパティの値は
    @e 管理下オブジェクト でなくてはならない。詳細は @ref m17nObject 参照。
*/

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "symbol.h"
#include "character.h"
#include "mtext.h"
#include "plist.h"

static int num_symbols;

#define SYMBOL_TABLE_SIZE 1024

static MSymbol symbol_table[SYMBOL_TABLE_SIZE];

static unsigned
hash_string (const char *str, int len)
{
  unsigned hash = 0;
  const char *end = str + len;
  unsigned c;

  while (str < end)
    {
      c = *((unsigned char *) str++);
      if (c >= 0140)
	c -= 40;
      hash = ((hash << 3) + (hash >> 28) + c);
    }
  return hash & (SYMBOL_TABLE_SIZE - 1);
}


static MPlist *
serialize_symbol (void *val)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, val);
  return plist;
}

static void *
deserialize_symbol (MPlist *plist)
{
  return (MPLIST_SYMBOL_P (plist) ? MPLIST_SYMBOL (plist) : Mnil);
}


/* Internal API */

int
msymbol__init ()
{
  num_symbols = 0;
  Mnil = (MSymbol) 0;
  Mt = msymbol ("t");
  Msymbol = msymbol ("symbol");
  Mstring = msymbol ("string");
  return 0;
}

void
msymbol__fini ()
{
  int i;
  MSymbol sym, next;
  int freed_symbols = 0;

  for (i = 0; i < SYMBOL_TABLE_SIZE; i++)
    for (sym = symbol_table[i]; sym; sym = sym->next)
      if (! MPLIST_TAIL_P (&sym->plist))
	{
	  if (sym->plist.key->managing_key)
	    M17N_OBJECT_UNREF (sym->plist.val);
	  M17N_OBJECT_UNREF (sym->plist.next);
	}
  for (i = 0; i < SYMBOL_TABLE_SIZE; i++)
    {
      for (sym = symbol_table[i]; sym; sym = next)
	{
	  next = sym->next;
	  free (sym->name);
	  free (sym);
	  freed_symbols++;
	}
      symbol_table[i] = NULL;
    }
  if (mdebug__flag & MDEBUG_FINI)
    fprintf (stderr, "%16s %7d %7d %7d\n", "Symbol",
	     num_symbols, freed_symbols, num_symbols - freed_symbols);
  num_symbols = 0;
}


MSymbol
msymbol__with_len (const char *name, int len)
{
  char *p = alloca (len + 1);

  memcpy (p, name, len);
  p[len] = '\0';
  return msymbol (p);
}

/** Return a plist of symbols that has non-NULL property PROP.  If
    PROP is Mnil, return a plist of all symbols.  Values of the plist
    is NULL.  */

MPlist *
msymbol__list (MSymbol prop)
{
  MPlist *plist = mplist ();
  int i;
  MSymbol sym;

  for (i = 0; i < SYMBOL_TABLE_SIZE; i++)
    for (sym = symbol_table[i]; sym; sym = sym->next)
      if (prop == Mnil || msymbol_get (sym, prop))
	mplist_push (plist, sym, NULL);
  return plist;
}


/** Canonicalize the name of SYM, and return a symbol of the
    canonicalized name.  Canonicalization is done by this rule:
	o convert all uppercase characters to lowercase.
	o remove all non alpha-numeric characters.
	o change the leading "ibm" to "cp".
	o change the leading "cp" to "ibm"
	o remove the leading "iso".
    For instance:
	"ISO-8859-2" -> "88592"
	"euc-JP" -> "eucjp"
	"IBM851" -> "cp851"
	"CP1250" -> "ibm1250"

    This function is used to canonicalize charset and coding system
    names.  */

MSymbol
msymbol__canonicalize (MSymbol sym)
{
  char *name = sym->name;
  /* Extra 2 bytes are for changing "cpXXX" to "ibmXXX" and
     terminating '\0'.  */
  char *canon = (char *) alloca (strlen (name) + 2);
  char *p = canon;

  for (; *name; name++)
    if (ISALNUM (*name))
      *p++ = TOLOWER (*name);
  *p = '\0';
  if (p - canon > 3 && canon[0] == 'i')
    {
      if (canon[1] == 'b' && canon[2] == 'm' && isdigit (canon[3]))
	{
	  /* Change "ibmXXX" to "cpXXX".  */
	  canon++;
	  canon[0] = 'c';
	  canon[1] = 'p';
	}
      else if (canon[1] == 's' && canon[2] == 'o')
	{
	  /* Change "isoXXX" to "XXX".  */
	  canon += 3;
	}
    }
  else if (p - canon > 2
	   && canon[0] == 'c' && canon[1] == 'p' && isdigit (canon[2]))
    {
      /* Change "cpXXX" to "ibmXXX".  */
      for (; p >= canon + 2; p--)
	p[1] = p[0];
      canon[0] = 'i';
      canon[1] = 'b';
      canon[2] = 'm';
    }

  return msymbol (canon);
}

MTextPropSerializeFunc msymbol__serializer = serialize_symbol;
MTextPropDeserializeFunc msymbol__deserializer = deserialize_symbol;

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

/*** @addtogroup m17nSymbol */
/*** @{ */

/*=*/
/***en
    @brief Symbol whose name is "nil".

    The symbol #Mnil has the name <tt>"nil"</tt> and, in general,
    represents @e false or @e no.  When coerced to "int", its value is
    zero.  #Mnil can't have any symbol property.  */
/***ja
    @brief "nil" を名前として持つシンボル.

    シンボル #Mnil は <tt>"nil"</tt> 
    という名前を持ち、一般に「偽」または「否定」を意味する。
    "int" に変換された場合、値は 0 である。
    #Mnil 自身はいかなるシンボルプロパティも持たない。  */

MSymbol Mnil;

/*=*/

/***en
    @brief Symbol whose name is "t".

    The symbol #Mt has the name <tt>"t"</tt> and, in general,
    represents @e true or @e yes.  */
/***ja
    @brief "t" を名前として持つシンボル.

    シンボル #Mt は <tt>"t"</tt> という名前を持ち、一般に「真」または「肯定」を意味する。  */

MSymbol Mt;

/*=*/

/***en
    @brief Symbol whose name is "string".

    The symbol #Mstring has the name <tt>"string"</tt> and is used
    as an argument of the functions mchar_define_property (),
    etc.  */
/***ja
    @brief "string" を名前として持つシンボル.

    シンボル #Mstring は <tt>"string"</tt> という名前を持ち、関数 
    mchar_define_property () などの引数として用いられる。  */

MSymbol Mstring;

/*=*/

/***en
    @brief Symbol whose name is "symbol".

    The symbol #Msymbol has the name <tt>"symbol"</tt> and is used
    as an argument of the functions mchar_define_property (),
    etc.  */
/***ja
    @brief "symbol" を名前として持つシンボル.

    定義済みシンボル #Msymbol は <tt>"symbol"</tt> という名前を持ち、関数
    mchar_define_property () などの引数として使われる。  */

MSymbol Msymbol;

/*=*/

/***en
    @brief Get a symbol.

    The msymbol () function returns the canonical symbol whose name is
    $NAME.  If there is none, one is created.  The created one is not
    a managing key.

    Symbols whose name starts by two spaces are reserved by the m17n
    library, and are used by the library only internally.

    @return
    This function returns the found or created symbol.

    @errors
    This function never fails.  */
/***ja
    @brief シンボルを得る.

    関数 msymbol () は $NAME 
    という名前を持つ正規化されたシンボルを返す。そのようなシンボルが存在しない場合には、生成する。生成されたシンボルは管理キーではない。

    空白文字二つで始まるシンボルは m17n ライブラリ用であり、内部的にのみ用いられる。

    @return
    この関数は見つけたか生成したかしたシンボルを返す。

    @errors
    この関数は決して失敗しない。

    @latexonly \IPAlabel{msymbol} @endlatexonly  */

/***
    @seealso
    msymbol_as_managing_key (), msymbol_name (), msymbol_exist ()  */

MSymbol
msymbol (const char *name)
{
  MSymbol sym;
  int len;
  unsigned hash;

  len = strlen (name);
  if (len == 3 && name[0] == 'n' && name[1] == 'i' && name[2] == 'l')
    return Mnil;
  hash = hash_string (name, len);
  len++;
  for (sym = symbol_table[hash]; sym; sym = sym->next)
    if (len == sym->length
	&& *name == *(sym->name)
	&& ! memcmp (name, sym->name, len))
      return sym;

  num_symbols++;
  MTABLE_CALLOC (sym, 1, MERROR_SYMBOL);
  MTABLE_MALLOC (sym->name, len, MERROR_SYMBOL);
  memcpy (sym->name, name, len);
  sym->length = len;
  sym->next = symbol_table[hash];
  symbol_table[hash] = sym;
  return sym;
}

/***en
    @brief Create a managing key.

    The msymbol_as_managing_key () function returns a newly created
    managing key whose name is $NAME.  It there already exists a
    symbol of name $NAME, it returns #Mnil.

    Symbols whose name starts by two spaces are reserved by the m17n
    library, and are used by the library only internally.

    @return
    If the operation was successful, this function returns the created
    symbol.  Otherwise, it returns #Mnil.  */
/***ja
    @brief 管理キーを作る.

    関数 msymbol_as_managing_key () は名前 $NAME 
    を持つ新しく作られた管理キーを返す。すでに名前 $NAME を持つシンボルがあれば、
    #Mnil を返す。

    空白文字二つで始まるシンボルは m17n ライブラリ用であり、内部的にのみ用いられる。

    @return
    処理に成功すれば、この関数は生成したシンボルを返す。そうでなければ
    #Mnil を返す。   */

/***
    @errors
    MERROR_SYMBOL

    @seealso
    msymbol (), msymbol_exist ()  */

MSymbol
msymbol_as_managing_key (const char *name)
{
  MSymbol sym;
  int len;
  unsigned hash;

  len = strlen (name);
  if (len == 3 && name[0] == 'n' && name[1] == 'i' && name[2] == 'l')
    MERROR (MERROR_SYMBOL, Mnil);
  hash = hash_string (name, len);
  len++;
  for (sym = symbol_table[hash]; sym; sym = sym->next)
    if (len == sym->length
	&& *name == *(sym->name)
	&& ! memcmp (name, sym->name, len))
      MERROR (MERROR_SYMBOL, Mnil);

  num_symbols++;
  MTABLE_CALLOC (sym, 1, MERROR_SYMBOL);
  sym->managing_key = 1;
  MTABLE_MALLOC (sym->name, len, MERROR_SYMBOL);
  memcpy (sym->name, name, len);
  sym->length = len;
  sym->next = symbol_table[hash];
  symbol_table[hash] = sym;
  return sym;
}

/*=*/

/***en
    @brief Search for a symbol that has a specified name.

    The msymbol_exist () function searches for the symbol whose name
    is $NAME.

    @return
    If such a symbol exists, msymbol_exist () returns that symbol.
    Otherwise it returns the predefined symbol #Mnil.

    @errors
    This function never fails.  */
/***ja
    @brief 指定された名前を持つシンボルを探す.

    関数 msymbol_exist () は $NAME という名前を持つシンボルを探す。

    @return
    もしそのようなシンボルが存在するならばそのシンボルを返す。そうでなければ、定義済みシンボル
    #Mnil を返す。  

    @errors
    この関数は決して失敗しない。     */

/***@seealso
    msymbol_name (), msymbol ()  */

MSymbol
msymbol_exist (const char *name)
{
  MSymbol sym;
  int len;
  unsigned hash;

  len = strlen (name);
  if (len == 3 && name[0] == 'n' && name[1] == 'i' && name[2] == 'l')
    return Mnil;
  hash = hash_string (name, len);
  len++;
  for (sym = symbol_table[hash]; sym; sym = sym->next)
    if (len == sym->length
	&& *name == *(sym->name)
	&& ! memcmp (name, sym->name, len))
      return sym;
  return Mnil;
}

/*=*/

/***en
    @brief Get symbol name.

    The msymbol_name () function returns a pointer to a string
    containing the name of $SYMBOL.

    @errors
    This function never fails.  */
/***ja
    @brief シンボルの名前を得る.

    関数 msymbol_name () は指定されたシンボル $SYMBOL 
    の名前を含む文字列へのポインタを返す。

    @errors
    この関数は決して失敗しない。     */

/***@seealso
    msymbol (), msymbol_exist ()  */

char *
msymbol_name (MSymbol symbol)
{
  return (symbol == Mnil ? "nil" : symbol->name);
}

/*=*/
/***en
    @brief Set the value of a symbol property.

    The msymbol_put () function assigns $VAL to the value of the
    symbol property that belongs to $SYMBOL and whose key is $KEY.  If
    the symbol property already has a value, $VAL overwrites the old
    one.  Both $SYMBOL and $KEY must not be #Mnil.

    If $KEY is a managing key, $VAL must be a managed object.  In this
    case, the reference count of the old value, if not @c NULL, is
    decremented by one, and that of $VAL is incremented by one.

    @return
    If the operation was successful, msymbol_put () returns 0.
    Otherwise it returns -1 and assigns an error code to the external
    variable #merror_code.  */
/***ja
    @brief シンボルプロパティに値を設定する.

    関数 msymbol_put () は、シンボル $SYMBOL 中でキーが $KEY であるシンボルプロパティの値を
    $VAL に設定する。そのシンボルプロパティにすでに値があれば上書きする。
    $SYMBOL, $KEY とも #Mnil であってはならない。

    $KEY が管理キーならば、$VAL は管理下オブジェクトでなくてはならない。この場合、古い値の参照数は
    @c NULL でなければ 1 減らされ、$VAL の参照数は 1 増やされる。

    @return
    処理が成功すれば、msymbol_put () は 0 を返す。そうでなければ -1 
    を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_SYMBOL

    @seealso
    msymbol_get () */

int
msymbol_put (MSymbol symbol, MSymbol key, void *val)
{
  if (symbol == Mnil || key == Mnil)
    MERROR (MERROR_SYMBOL, -1);
  mplist_put (&symbol->plist, key, val);
  return 0;
}

/*=*/

/***en
    @brief Get the value of a symbol property.

    The msymbol_get () function searches for the value of the symbol
    property that belongs to $SYMBOL and whose key is $KEY.  If
    $SYMBOL has such a symbol property, its value is returned.
    Otherwise @c NULL is returned.

    @return
    If an error is detected, msymbol_get () returns @c NULL and
    assigns an error code to the external variable #merror_code.  */
/***ja
    @brief シンボルプロパティの値を得る.

    関数 msymbol_get () は、シンボル $SYMBOL 
    が持つシンボルプロパティのうち、キーが $KEY 
    であるものを探す。もし該当するシンボルプロパティが存在すれば、それの値を返す。そうでなければ
    @c NULL を返す。

    @return 
    エラーが検出された場合、msymbol_get () は @c NULL 
    を返し、外部変数 #merror_code にエラーコードを設定する。  */

/***
    @errors
    @c MERROR_SYMBOL

    @seealso
    msymbol_put () */

void *
msymbol_get (MSymbol symbol, MSymbol key)
{
  MPlist *plist;

  if (symbol == Mnil || key == Mnil)
    return NULL;
  plist = &symbol->plist;
  MPLIST_FIND (plist, key);
  return (MPLIST_TAIL_P (plist) ? NULL : MPLIST_VAL (plist));
}

/*** @} */

#include <stdio.h>

/*** @addtogroup m17nDebug */
/*=*/
/*** @{ */

/***en
    @brief Dump a symbol.

    The mdebug_dump_symbol () function prints symbol $SYMBOL in a human
    readable way to the stderr.  $INDENT specifies how many columns to
    indent the lines but the first one.

    @return
    This function returns $SYMBOL.

    @errors
    MERROR_DEBUG  */
/***ja
    @brief シンボルをダンプする.

    関数 mdebug_dump_symbol () はシンボル $symbol を stderr 
    に人間に可読な形で印刷する。 $INDENT は２行目以降のインデントを指定する。

    @return
    この関数は $SYMBOL を返す。 

    @errors
    MERROR_DEBUG  */

MSymbol
mdebug_dump_symbol (MSymbol symbol, int indent)
{
  char *prefix;
  MPlist *plist;
  char *name;

  if (indent < 0)
    MERROR (MERROR_DEBUG, Mnil);
  prefix = (char *) alloca (indent + 1);
  memset (prefix, 32, indent);
  prefix[indent] = 0;

  if (symbol == Mnil)
    plist = NULL, name = "nil";
  else
    plist = &symbol->plist, name = symbol->name;

  fprintf (stderr, "%s%s", prefix, name);
  while (plist && MPLIST_KEY (plist) != Mnil)
    {
      fprintf (stderr, ":%s", MPLIST_KEY (plist)->name);
      plist = MPLIST_NEXT (plist);
    }
  return symbol;
}

/***en
    @brief Dump all symbol names.

    The mdebug_dump_all_symbols () function prints names of all
    symbols to the stderr.  $INDENT specifies how many columns to
    indent the lines but the first one.

    @return
    This function returns #Mnil.

    @errors
    MERROR_DEBUG  */
/***ja
    @brief すべてのシンボル名をダンプする.

    関数 mdebug_dump_all_symbols () は、すべてのシンボルの名前を 
    stderr に印刷する。 $INDENT は２行目以降のインデントを指定する。

    @return
    この関数は #Mnil を返す。 

    @errors
    MERROR_DEBUG  */


MSymbol
mdebug_dump_all_symbols (int indent)
{
  char *prefix;
  int i, n;
  MSymbol sym;

  if (indent < 0)
    MERROR (MERROR_DEBUG, Mnil);
  prefix = (char *) alloca (indent + 1);
  memset (prefix, 32, indent);
  prefix[indent] = 0;

  fprintf (stderr, "(symbol-list");
  for (i = n = 0; i < SYMBOL_TABLE_SIZE; i++)
    if ((sym = symbol_table[i]))
      {
	fprintf (stderr, "\n%s  (%4d", prefix, i);
	for (; sym; sym = sym->next, n++)
	  fprintf (stderr, " '%s'", sym->name);
	fprintf (stderr, ")");
      }
  fprintf (stderr, "\n%s  (total %d)", prefix, n);
  fprintf (stderr, ")");
  return Mnil;
}

/*** @} */ 

/*
  Local Variables:
  coding: euc-japan
  End:
*/
