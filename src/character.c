/* character.c -- character module.
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
    @addtogroup m17nCharacter
    @brief Character objects and API for them.

    The m17n library represents a @e character by a character code (an
    integer).  The minimum character code is @c 0.  The maximum
    character code is defined by the macro #MCHAR_MAX.  It is
    assured that #MCHAR_MAX is not smaller than @c 0x3FFFFF (22
    bits).

    Characters @c 0 to @c 0x10FFFF are equivalent to the Unicode
    characters of the same code values.

    A character can have zero or more properties called @e character
    @e properties.  A character property consists of a @e key and a
    @e value, where key is a symbol and value is anything that can be
    cast to <tt>(void *)</tt>.  "The character property that belongs
    to character C and whose key is K" may be shortened to "the K
    property of C".  */

/***ja
    @addtogroup m17nCharacter
    @brief 文字オブジェクトとそれに関する API.

    m17n ライブラリは @e 文字 を文字コード（整数）で表現する。最小の文
    字コードは @c 0 で、最大の文字コードはマクロ #MCHAR_MAX によって
    定義されている。#MCHAR_MAX は @c 0x3FFFFF（22ビット） 以上である
    ことが保証されている。

    @c 0 から @c 0x10FFFF までの文字は、それと同じ値を持つ Unicode の
    文字に割り当てられている。

    各文字は @e 文字プロパティ と呼ぶプロパティを 0 個以上持つことがで
    きる。文字プロパティは @e キー と @e 値 からなる。キーはシンボルで
    あり、値は <tt>(void *)</tt> 型にキャストできるものなら何でもよい。 
    「文字 C の文字プロパティのうちキーが K であるもの」を簡単に
    「文字 C の K プロパティ」と呼ぶことがある。  */
/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <stdio.h>

#include "m17n-core.h"
#include "m17n-misc.h"
#include "internal.h"

typedef struct
{
  MSymbol type;
  void *mdb;
  MCharTable *table;
} MCharPropRecord;

static MPlist *char_prop_list;

static void
free_string (int from, int to, void *str, void *arg)
{
  free (str);
}


/* Internal API */

int
mchar__init ()
{
  char_prop_list = mplist ();

  Mname
    = mchar_define_property ("name", Mstring);
  Mcategory
    = mchar_define_property ("category", Msymbol);
  Mcombining_class
    = mchar_define_property ("combining-class", Minteger);
  Mbidi_category
    = mchar_define_property ("bidirectional-category", Msymbol);
  Msimple_case_folding
    = mchar_define_property ("simple-case-folding", Minteger);
  Mcomplicated_case_folding
    = mchar_define_property ("complicated-case-folding", Mtext);
  Mscript
    = mchar_define_property ("script", Msymbol);

  return 0;
}

void
mchar__fini (void)
{
  MPlist *p;

  for (p = char_prop_list; mplist_key (p) != Mnil; p = mplist_next (p))
    {
      MCharPropRecord *record = mplist_value (p);

      if (record->table)
	{
	  if (record->type == Mstring)
	    mchartable_map (record->table, NULL, free_string, NULL);
	  M17N_OBJECT_UNREF (record->table);
	}
      free (record);
    }
  M17N_OBJECT_UNREF (char_prop_list);
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */

/* External API */

/*** @addtogroup m17nCharacter */
/*** @{ */
/*=*/

#ifdef FOR_DOXYGEN
/***en
    @brief Maximum character code.

    The macro #MCHAR_MAX gives the maximum character code.  */

/***ja
    @brief 文字コードの最大値.

    マクロ #MCHAR_MAX は文字コードの最大値を与える。  */

#define MCHAR_MAX
/*=*/
#endif /* FOR_DOXYGEN */

/***en
    @ingroup m17nCharacter
    @name Variables: Keys of character properties

    These symbols are used as keys of character properties.  */

/***ja
     @name 変数: 文字プロパティのキー

     これらのシンボルは文字プロパティのキーとして使われる。*/
/*=*/
/*** @{ */

/***en
    @brief Key for script.

    The symbol #Mscript has the name <tt>"script"</tt> and is used as the key
    of a character property.  The value of such a property is a symbol
    representing the script to which the character belongs.

    Each symbol that represents a script has one of the names listed in
    the <em>Unicode Technical Report #24</em>.  */

/***ja
    @brief スクリプトを表わすキー.

    シンボル #Mscript は <tt>"script"</tt> という名前を持ち、文字プ
    ロパティのキーとして使われる。このプロパティの値は、この文字の属す
    るスクリプトを表わすシンボルである。

    スクリプトを表わすシンボルの名前は、<em>Unicode Technical Report
    #24</em> にリストされているもののいずれかである。  */

MSymbol Mscript;

/*=*/

/***en
    @brief Key for character name.

    The symbol #Mname has the name <tt>"name"</tt> and is used as
    the key of a character property.  The value of such a property is a
    C-string representing the name of the character.  */

/***ja
    @brief 名前を表わすキー.

    シンボル #Mname は <tt>"name"</tt> という名前を持ち、文字プロパ
    ティのキーとして使われる。このプロパティの値は C-string であり、そ
    の文字の名前を表わす。  */

MSymbol Mname;

/*=*/

/***en
    @brief Key for general category.

    The symbol #Mcategory has the name <tt>"category"</tt> and is
    used as the key of a character property.  The value of such a
    property is a symbol representing the <em>general category</em> of
    the character.

    Each symbol that represents a general category has one of the
    names listed as abbreviations for <em>General Category</em> in
    Unicode.  */

/***ja
    @brief 一般カテゴリを表わすキー.

    シンボル #Mcategory は <tt>"category"</tt> という名前を持ち、文
    字プロパティのキーとして使われる。このプロパティの値は、対応する
    <em>一般カテゴリ</em> を表わすシンボルである。

    一般カテゴリを表わすシンボルの名前は、<em>General Category</em>の
    省略形として Unicode に定義されているものである。  */

MSymbol Mcategory;

/*=*/

/***en
    @brief Key for canonical combining class.

    The symbol #Mcombining_class has the name
    <tt>"combining-class"</tt> and is used as the key of a character
    property.  The value of such a property is an integer that
    represents the <em>canonical combining class</em> of the character.

    The meaning of each integer that represents a canonical combining
    class is identical to the one defined in Unicode.  */

/***ja
    @brief 標準結合クラスを表わすキー.

    シンボル #Mcombining_class は <tt>"combining-class"</tt> という
    名前を持ち、文字プロパティのキーとして使われる。このプロパティの値
    は、対応する @e 標準結合クラス を表わす整数である。

    標準結合クラスを表わす整数の意味は、Unicode に定義されているもの
    と同じである。  */

MSymbol Mcombining_class;
/*=*/

/***en
    @brief Key for bidi category.

    The symbol #Mbidi_category has the name <tt>"bidi-category"</tt>
    and is used as the key of a character property.  The value of such
    a property is a symbol that represents the <em>bidirectional
    category</em> of the character.

    Each symbol that represents a bidirectional category has one of
    the names listed as types of <em>Bidirectional Category</em> in
    Unicode.  */

/***ja
    @brief 双方向カテゴリを表わすキー.

    シンボル #Mbidi_category は <tt>"bidi-category"</tt> という名前
    を持ち、文字プロパティのキーとして使われる。このプロパティの値は、
    対応する @e 双方向カテゴリ を表わすシンボルである。

    双方向カテゴリを表わすシンボルの名前は、<em>Bidirectional
    Category</em> の型として Unicode に定義されているものである。  */

MSymbol Mbidi_category;
/*=*/

/***en
    @brief Key for corresponding single lowercase character.

    The symbol #Msimple_case_folding has the name
    <tt>"simple-case-folding"</tt> and is used as the key of a
    character property.  The value of such a property is the
    corresponding single lowercase character that is used when
    comparing M-texts ignoring cases.

    If a character requires a complicated comparison (i.e. cannot be
    compared by simply mapping to another single character), the value
    of such a property is @c 0xFFFF.  In this case, the character has
    another property whose key is #Mcomplicated_case_folding.  */

/***ja
    @brief 対応する小文字一文字を表わすキー.

    シンボル #Msimple_case_folding は <tt>"simple-case-folding"</tt> 
    という名前を持ち、文字プロパティのキーとして使われる。このプロパティ
    の値は、対応する小文字一文字であり、大文字／小文字の区別を無視した
    文字列比較の際に使われる。

    複雑な比較方法を必要とする文字であった場合（別の一文字と対応付ける
    ことによって比較できない場合）、このプロパティの値は @c 0xFFFF に
    なる。この場合その文字は、#Mcomplicated_case_folding というキーの
    文字プロパティを持つ。  */

MSymbol Msimple_case_folding;
/***en
    @brief Key for corresponding multiple lowercase characters.

    The symbol #Mcomplicated_case_folding has the name
    <tt>"complicated-case-folding"</tt> and is used as the key of a
    character property.  The value of such a property is the
    corresponding M-text that contains a sequence of lowercase
    characters to be used for comparing M-texts ignoring case.  */

/***ja
    @brief 対応する小文字の列を表わすキー.

    シンボル #Mcomplicated_case_folding は 
    <tt>"complicated-case-folding"</tt> という名前を持ち、文字プロパティ
    のキーとして使われる。このプロパティの値は、対応する小文字列からな
    る M-text であり、大文字／小文字の区別を無視した文字列比較の際に使
    われる。
      */

MSymbol Mcomplicated_case_folding;
/*=*/
/*** @} */
/*=*/

/***en
    @brief Define a character property.

    The mchar_define_property () function searches the m17n database
    for a data whose tags are \<#Mchar_table, $TYPE, $SYM \>.
    Here, $SYM is a symbol whose name is $NAME.  $TYPE must be
    #Mstring, #Mtext, #Msymbol, #Minteger, or #Mplist.

    @return
    If the operation was successful, mchar_define_property () returns
    $SYM.  Otherwise it returns #Mnil.  */

/***ja
    @brief 文字プロパティを定義する.

    関数 mchar_define_property () は、 \<#Mchar_table, $TYPE, $SYM \>
    というタグを持ったデータベースを m17n 言語情報ベースから探す。  こ
    こで $SYM は $NAME という名前のシンボルである。$TYPE は#Mstring,
    #Mtext, #Msymbol, #Minteger, #Mplist のいずれかでなければならない。

    @return
    処理に成功すれば mchar_define_property () は$SYM を返す。
    失敗した場合は #Mnil を返す。  */

/***
    @errors
    @c MERROR_DB

    @seealso
    mchar_get_prop (), mchar_put_prop ()  */

MSymbol
mchar_define_property (const char *name, MSymbol type)
{
  MSymbol key = msymbol (name);
  MCharPropRecord *record;

  record = mplist_get (char_prop_list, key);
  if (record)
    {
      if (record->table)
	M17N_OBJECT_UNREF (record->table);
    }
  else
    {
      MSTRUCT_CALLOC (record, MERROR_CHAR);
      mplist_put (char_prop_list, key, record);
    }

  record->type = type;
  if (mdatabase__finder
      && (record->mdb = (*mdatabase__finder) (Mchar_table, type, key, Mnil)))
    {
      record->table = NULL;
    }
  else
    {
      void *default_value = NULL;

      record->mdb = NULL;
      if (type == Minteger)
	default_value = (void *) -1;
      record->table = mchartable (type, default_value);
    }

  return key;
}

/*=*/

/***en
    @brief Get the value of a character property.

    The mchar_get_prop () function searches character $C for the
    character property whose key is $KEY.

    @return
    If the operation was successful, mchar_get_prop () returns the
    value of the character property.  Otherwise it returns @c
    NULL.  */

/***ja
    @brief 文字プロパティの値を得る.

    関数 mchar_get_prop () は、文字 $C の文字プロパティのうちキーが 
    $KEY であるものを探す。

    @return
    処理が成功すれば mchar_get_prop () は見つかったプロパティの値を返
    す。失敗した場合は @c NULL を返す。

    @latexonly \IPAlabel{mchar_get_prop} @endlatexonly
*/
/***
    @errors
    @c MERROR_SYMBOL, @c MERROR_DB

    @seealso
    mchar_define_property (), mchar_put_prop ()  */

void *
mchar_get_prop (int c, MSymbol key)
{
  MCharPropRecord *record;

  record = mplist_get (char_prop_list, key);
  if (! record)
    return NULL;
  if (record->mdb)
    {
      record->table = (*mdatabase__loader) (record->mdb);
      if (! record->table)
	MERROR (MERROR_DB, NULL);
      record->mdb = NULL;
    }
  return mchartable_lookup (record->table, c);
}

/*=*/

/***en
    @brief Set the value of a character property.

    The mchar_put_prop () function searches character $C for the
    character property whose key is $KEY and assigns $VAL to the value
    of the found property.

    @return
    If the operation was successful, mchar_put_prop () returns 0.
    Otherwise, it returns -1.  */
/***ja
    @brief 文字プロパティの値を設定する.

    関数 mchar_put_prop () は、文字 $C の文字プロパティのうちキーが $KEY で
    あるものを探し、その値として $VAL を設定する。

    @return
    処理が成功すれば mchar_put_prop () は0を返す。失敗した場合は-1を返
    す。  */
/***
    @errors
    @c MERROR_SYMBOL, @c MERROR_DB

    @seealso
    mchar_define_property (), mchar_get_prop ()   */

int
mchar_put_prop (int c, MSymbol key, void *val)
{
  MCharPropRecord *record;

  record = mplist_get (char_prop_list, key);
  if (! record)
    return -1;
  if (record->mdb)
    {
      record->table = (*mdatabase__loader) (record->mdb);
      if (! record->table)
	MERROR (MERROR_DB, -1);
      record->mdb = NULL;
    }
  return mchartable_set (record->table, c, val);
}

/*** @} */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
