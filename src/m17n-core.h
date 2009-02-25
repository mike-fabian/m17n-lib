/* m17n-core.h -- header file for the CORE API of the m17n library.
   Copyright (C) 2003, 2004, 2005, 2006
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

#ifndef _M17N_CORE_H_
#define _M17N_CORE_H_

#ifdef __cplusplus
#define M17N_BEGIN_HEADER extern "C" {
#define M17N_END_HEADER }
#else
#define M17N_BEGIN_HEADER	/* do nothing */
#define M17N_END_HEADER		/* do nothing */
#endif

M17N_BEGIN_HEADER

/*
 * Header file for m17n library.
 */

/* (C1) Introduction */

/***en @defgroup m17nIntro Introduction  */
/***ja @defgroup m17nIntro はじめに  */
/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)

#define M17NLIB_MAJOR_VERSION 1
#define M17NLIB_MINOR_VERSION 5
#define M17NLIB_PATCH_LEVEL 4
#define M17NLIB_VERSION_NAME "1.5.4"

extern void m17n_init_core (void);
#define M17N_INIT() m17n_init_core ()
extern void m17n_fini_core (void);
#define M17N_FINI() m17n_fini_core ()

extern int merror_code;

#endif

/*=*/

/*** @ingroup m17nIntro */
/***en
    @brief Enumeration for the status of the m17n library.

    The enum #M17NStatus is used as a return value of the function
    m17n_status ().  */

/***ja
    @brief  m17n ライブラリの状態を示す列挙型.

    列挙型 #M17NStatus は関数 m17n_status () の戻り値として用いられる。 */

enum M17NStatus
  {
    /***en No modules is initialized, and all modules are finalized.  */
    M17N_NOT_INITIALIZED, 
    /***en Only the modules in CORE API are initialized.  */
    M17N_CORE_INITIALIZED,
    /***en Only the modules in CORE and SHELL APIs are initialized.  */
    M17N_SHELL_INITIALIZED, 
    /***en All modules are initialized.  */
    M17N_GUI_INITIALIZED
  };

/*=*/

extern enum M17NStatus m17n_status (void);

/***en @defgroup m17nCore CORE API
    @brief API provided by libm17n-core.so */
/***ja @defgroup m17nCore コア API
    @brief libm17n-core.so が提供する API */
/*=*/
/*** @ingroup m17nCore */
/***en @defgroup m17nObject Managed Object
    @brief Objects managed by the reference count */
/***ja @defgroup m17nObject 管理下オブジェクト
    @brief 参照回数で管理されるオブジェクト */
/*=*/

/*** @ingroup m17nObject  */
/***en
    @brief The first member of a managed object.

    When an application program defines a new structure for managed
    objects, its first member must be of the type @c struct
    #M17NObjectHead.  Its contents are used by the m17n library, and
    application programs should never touch them.  */
/***ja
    @brief 管理下オブジェクトの最初のメンバ.

    アプリケーションプログラムが新しい構造体を管理下オブジェクトとして定義する際には、最初のメンバは 
    @c #M17NObjectHead 構造体型でなくてはならない。
    @c #M17NObjectHead の内容は m17n 
    ライブラリが使用するので、アプリケーションプログラムは触れてはならない。    */

typedef struct
{
  void *filler[2];
} M17NObjectHead;

/*=*/

/* Return a newly allocated managed object.  */
extern void *m17n_object (int size, void (*freer) (void *));

/* Increment the reference count of managed object OBJECT.  */
extern int m17n_object_ref (void *object);

/* Decrement the reference count of managed object OBJECT.  */
extern int m17n_object_unref (void *object);

/*** @ingroup m17nCore */
/***en
    @brief Generic function type.

    #M17NFunc is a generic function type for setting a function
    pointer as a value of #MSymbol property or #MPlist.  */

/***ja
    @brief 汎関数型.

    #M17NFunc は汎関数型であり、関数ポインタを #MSymbol プロパティや
    #MPlist の値として設定する際用いる。  */


/***
    @seealso
    msymbol_put_func (), msymbol_get_func (),
    mplist_put_func (), mplist_get_func ().  */

typedef void (*M17NFunc) (void);

/*=*/

/*** @ingroup m17nCore */
/***en
    @brief Wrapper for a generic function type.

    The macro M17N_FUNC () casts a function to the type #M17NFunc.  */

/***ja
    @brief 汎関数型へのラッパ.

    マクロ M17N_FUNC () は関数を #M17NFunc 型へキャストする。  */


#define M17N_FUNC(func) ((M17NFunc) (func))

/*=*/

/* (C2) Symbol handling */

/*** @ingroup m17nCore */
/***en @defgroup m17nSymbol Symbol  */
/***ja @defgroup m17nSymbol シンボル */
/*=*/

/***
    @ingroup m17nSymbol */
/***en
    @brief Type of symbols.

    The type #MSymbol is for a @e symbol object.  Its internal
    structure is concealed from application programs.  */

/***ja
    @brief シンボルの型宣言.

    #MSymbol は @e シンボル (symbol) オブジェクトの型である。
    内部構造はアプリケーションプログラムからは見えない。  */

typedef struct MSymbolStruct *MSymbol;

/*=*/

/* Predefined symbols. */ 
extern MSymbol Mnil;
extern MSymbol Mt;
extern MSymbol Mstring;
extern MSymbol Msymbol;
extern MSymbol Mtext;
extern MSymbol Mcharset;

/* Return a symbol of name NAME.  */
extern MSymbol msymbol (const char *name);

/* Return a managing key of name NAME.  */
extern MSymbol msymbol_as_managing_key (const char *name);

/* Check if SYMBOL is a managing key.  */
extern int msymbol_is_managing_key (MSymbol symbol);

/* Return a symbol of name NAME if it already exists.  */
extern MSymbol msymbol_exist (const char *name);

/* Return the name of SYMBOL.  */
extern char *msymbol_name (MSymbol symbol);

/* Give SYMBOL KEY property with value VALUE.  */
extern int msymbol_put (MSymbol symbol, MSymbol key, void *val);

/*** Return KEY property value of SYMBOL.  */
extern void *msymbol_get (MSymbol symbol, MSymbol key);

extern int msymbol_put_func (MSymbol symbol, MSymbol key, M17NFunc func);

extern M17NFunc msymbol_get_func (MSymbol symbol, MSymbol key);

/* 
 *  (2-1) Property List
 */
/*=*/
/*** @ingroup m17nCore */
/***en @defgroup m17nPlist Property List */
/***ja @defgroup m17nPlist プロパティリスト */
/*=*/

/***
    @ingroup m17nPlist */ 
/***en
    @brief Type of property list objects.

    The type #MPlist is for a @e property @e list object.  Its internal
    structure is concealed from application programs.  */

/***ja
    @brief プロパティリスト・オブジェクトの型宣言.

    #MPlist は @e プロパティリスト (Property list) オブジェクトの型である。
    内部構造はアプリケーションプログラムからは見えない。  */

typedef struct MPlist MPlist;

/*=*/

extern MSymbol Mplist, Minteger;

extern MPlist *mplist ();

extern MPlist *mplist_copy (MPlist *plist);

extern MPlist *mplist_add (MPlist *plist, MSymbol key, void *val);

extern MPlist *mplist_push (MPlist *plist, MSymbol key, void *val);

extern void *mplist_pop (MPlist *plist);

extern MPlist *mplist_put (MPlist *plist, MSymbol key, void *val);

extern void *mplist_get (MPlist *plist, MSymbol key);

extern MPlist *mplist_put_func (MPlist *plist, MSymbol key, M17NFunc func);

extern M17NFunc mplist_get_func (MPlist *plist, MSymbol key);

extern MPlist *mplist_find_by_key (MPlist *plist, MSymbol key);

extern MPlist *mplist_find_by_value (MPlist *plist, void *val);

extern MPlist *mplist_next (MPlist *plist);

extern MPlist *mplist_set (MPlist *plist, MSymbol key, void *val);

extern int mplist_length (MPlist *plist);

extern MSymbol mplist_key (MPlist *plist);

extern void *mplist_value (MPlist *plist);

/* (S1) Characters */

/*=*/
/*** @ingroup m17nCore */
/***en @defgroup m17nCharacter Character */
/***ja @defgroup m17nCharacter 文字 */
/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
#define MCHAR_MAX 0x3FFFFF
/*#define MCHAR_MAX 0x7FFFFFFF*/
#endif

extern MSymbol Mscript;
extern MSymbol Mname;
extern MSymbol Mcategory;
extern MSymbol Mcombining_class;
extern MSymbol Mbidi_category;
extern MSymbol Msimple_case_folding;
extern MSymbol Mcomplicated_case_folding;
extern MSymbol Mcased, Msoft_dotted, Mcase_mapping;
extern MSymbol Mblock;

extern MSymbol mchar_define_property (const char *name, MSymbol type);

extern void *mchar_get_prop (int c, MSymbol key);

extern int mchar_put_prop (int c, MSymbol key, void *val);

/* (C3) Handling chartable */

/*** @ingroup m17nCore */
/***en @defgroup m17nChartable Chartable */
/***ja @defgroup m17nChartable 文字テーブル */
/*=*/
extern MSymbol Mchar_table;

/***
    @ingroup m17nChartable */
/***en
    @brief Type of chartables.

    The type #MCharTable is for a @e chartable objects.  Its
    internal structure is concealed from application programs.  */

/***ja
    @brief 文字テーブルの型宣言.

    #MCharTable は @e 文字テーブル (chartable) オブジェクトの型である。
    内部構造はアプリケーションプログラムからは見えない。  */

typedef struct MCharTable MCharTable;
/*=*/

extern MCharTable *mchartable (MSymbol key, void *default_value);

extern int mchartable_min_char (MCharTable *table);

extern int mchartable_max_char (MCharTable *table);

extern void *mchartable_lookup (MCharTable *table, int c);

extern int mchartable_set (MCharTable *table, int c, void *val);

extern int mchartable_set_range (MCharTable *table, int from, int to,
				 void *val);

extern int mchartable_map (MCharTable *table, void *ignore,
			   void (*func) (int, int, void *, void *), 
			   void *func_arg);

extern void mchartable_range (MCharTable *table, int *from, int *to);

extern MCharTable *mchar_get_prop_table (MSymbol key, MSymbol *type);

/*
 *  (5) Handling M-text.
 *	"M" of M-text stands for:
 *	o Multilingual
 *	o Metamorphic
 *	o More than string
 */

/*** @ingroup m17nCore */
/***en @defgroup m17nMtext M-text */
/***ja @defgroup m17nMtext M-text */
/*=*/

/*
 * (5-1) M-text basics
 */
/*=*/
/*** @ingroup m17nMtext */
/***en
    @brief Type of @e M-texts.

    The type #MText is for an @e M-text object.  Its internal
    structure is concealed from application programs.  */

/***ja
    @brief @e MText の型宣言.

    #Mtext は @e M-text オブジェクトの型である。
    内部構造はアプリケーションプログラムからは見えない。 

    @latexonly \IPAlabel{MText} @endlatexonly
    @latexonly \IPAlabel{MText->MPlist} @endlatexonly  */

typedef struct MText MText;

/*=*/

/*** @ingroup m17nMtext */
/***en
    @brief Enumeration for specifying the format of an M-text.

    The enum #MTextFormat is used as an argument of the
    mtext_from_data () function to specify the format of data from
    which an M-text is created.  */

/***ja
    @brief M-text のフォーマットを指定する列挙型.

    列挙型 #MTextFormat は関数
    mtext_from_data () の引数として用いられ、
    M-text を生成する元となるデータのフォーマットを指定する。  */

enum MTextFormat
  {
    MTEXT_FORMAT_US_ASCII,
    MTEXT_FORMAT_UTF_8,
    MTEXT_FORMAT_UTF_16LE,
    MTEXT_FORMAT_UTF_16BE,
    MTEXT_FORMAT_UTF_32LE,
    MTEXT_FORMAT_UTF_32BE,
    MTEXT_FORMAT_MAX
  };
/*=*/

extern MText *mtext ();

extern void *mtext_data (MText *mt, enum MTextFormat *fmt, int *nunits,
			 int *pos_idx, int *unit_idx);

/*=*/

/***en @name Variables: Default Endian of UTF-16 and UTF-32 */
/***ja @name 変数: UTF-16 と UTF-32 のデフォルトのエンディアン */
/*** @{ */
/*=*/

/*** @ingroup m17nMtext */
/***en
    @brief Variable of value MTEXT_FORMAT_UTF_16LE or MTEXT_FORMAT_UTF_16BE.

    The global variable #MTEXT_FORMAT_UTF_16 is initialized to
    #MTEXT_FORMAT_UTF_16LE on a "Little Endian" system (storing words
    with the least significant byte first), and to
    #MTEXT_FORMAT_UTF_16BE on a "Big Endian" system (storing words
    with the most significant byte first).  */

/***ja
    @brief 値が MTEXT_FORMAT_UTF_16LE か MTEXT_FORMAT_UTF_16BE である変数

    大域変数 #MTEXT_FORMAT_UTF_16 はリトル・エンディアン・システム
    （ワードを LSB (Least Significant Byte) を先にして格納）上では
    #MTEXT_FORMAT_UTF_16LE に初期化され、ビッグ・エンディアン・システム
    （ワードを MSB (Most Significant Byte) を先にして格納）上では
    #MTEXT_FORMAT_UTF_16BE に初期化される。  */

/***
    @seealso
    mtext_from_data ()  */

extern const enum MTextFormat MTEXT_FORMAT_UTF_16;
/*=*/

/*** @ingroup m17nMtext */
/***en
    @brief Variable of value MTEXT_FORMAT_UTF_32LE or MTEXT_FORMAT_UTF_32BE.

    The global variable #MTEXT_FORMAT_UTF_32 is initialized to
    #MTEXT_FORMAT_UTF_32LE on a "Little Endian" system (storing words
    with the least significant byte first), and to
    #MTEXT_FORMAT_UTF_32BE on a "Big Endian" system (storing
    words with the most significant byte first).  */

/***ja
    @brief 値が MTEXT_FORMAT_UTF_32LE か MTEXT_FORMAT_UTF_32BE である変数

    大域変数 #MTEXT_FORMAT_UTF_32 はリトル・エンディアン・システム
    （ワードを LSB (Least Significant Byte) を先にして格納）上では
    #MTEXT_FORMAT_UTF_32LE に初期化され、ビッグ・エンディアン・システム
    （ワードを MSB (Most Significant Byte) を先にして格納）上では
    #MTEXT_FORMAT_UTF_32BE に初期化される。  */

/***
    @seealso
    mtext_from_data ()  */

extern const int MTEXT_FORMAT_UTF_32;

/*=*/
/*** @} */
/*=*/

extern MText *mtext_from_data (const void *data, int nitems,
			       enum MTextFormat format);

/*=*/
/*** @} */

extern MSymbol Mlanguage;

/*
 *  (5-2) Functions to manipulate M-texts.  They correspond to string
 *   manipulating functions in libc.
 *   In the following functions, mtext_XXX() corresponds to strXXX().
 */

extern int mtext_len (MText *mt);

extern int mtext_ref_char (MText *mt, int pos);

extern int mtext_set_char (MText *mt, int pos, int c);

extern MText *mtext_copy (MText *mt1, int pos, MText *mt2, int from, int to);

extern int mtext_compare (MText *mt1, int from1, int to1,
			  MText *mt2, int from2, int to2);

extern int mtext_case_compare (MText *mt1, int from1, int to1,
			       MText *mt2, int from2, int to2);

extern int mtext_character (MText *mt, int from, int to, int c);

extern int mtext_del (MText *mt, int from, int to);

extern int mtext_ins (MText *mt1, int pos, MText *mt2);

extern int mtext_insert (MText *mt1, int pos, MText *mt2, int from, int to);

extern int mtext_ins_char (MText *mt, int pos, int c, int n);

extern int mtext_replace (MText *mt1, int from1, int to1,
			  MText *mt2, int from2, int to2);

extern MText *mtext_cat_char (MText *mt, int c);

extern MText *mtext_duplicate (MText *mt, int from, int to);

extern MText *mtext_dup (MText *mt);

extern MText *mtext_cat (MText *mt1, MText *mt2);

extern MText *mtext_ncat (MText *mt1, MText *mt2, int n);

extern MText *mtext_cpy (MText *mt1, MText *mt2);

extern MText *mtext_ncpy (MText *mt1, MText *mt2, int n);

extern int mtext_chr (MText *mt, int c);

extern int mtext_rchr (MText *mt, int c);

extern int mtext_cmp (MText *mt1, MText *mt2);

extern int mtext_ncmp (MText *mt1, MText *mt2, int n);

extern int mtext_spn (MText *mt1, MText *mt2);

extern int mtext_cspn (MText *mt1, MText *mt2);

extern int mtext_pbrk (MText *mt1, MText *mt2);

extern int mtext_text (MText *mt1, int pos, MText *mt2);

extern int mtext_search (MText *mt1, int from, int to, MText *mt2);

extern MText *mtext_tok (MText *mt, MText *delim, int *pos);

extern int mtext_casecmp (MText *mt1, MText *mt2);

extern int mtext_ncasecmp (MText *mt1, MText *mt2, int n);

extern int mtext_lowercase (MText *mt);

extern int mtext_titlecase (MText *mt);

extern int mtext_uppercase (MText *mt);

/***en
    @brief Enumeration for specifying a set of line breaking option.

    The enum #MTextLineBreakOption is to control the line breaking
    algorithm of the function mtext_line_break () by specifying
    logical-or of the members in the arg @e option.  */

enum MTextLineBreakOption
  {
    /***en Specify the legacy support for space character as base for
       combining marks.  See the section 8.3 of UAX#14. */
    MTEXT_LBO_SP_CM = 1,
    /***en Specify to use space characters for line breaking Korean
	text.  */
    MTEXT_LBO_KOREAN_SP = 2,
    /***en Specify to treat characters of ambiguous line-breaking
	class as of ideographic line-breaking class.  */
    MTEXT_LBO_AI_AS_ID = 4,
    MTEXT_LBO_MAX
  };

extern int mtext_line_break (MText *mt, int pos, int option, int *after);

/*** @ingroup m17nPlist */
extern MPlist *mplist_deserialize (MText *mt);

/*
 * (5-3) Text properties
 */
/*=*/
/*** @ingroup m17nCore */
/***en @defgroup m17nTextProperty Text Property */
/***ja @defgroup m17nTextProperty テキストプロパティ */
/*=*/
/*** @ingroup m17nTextProperty */
/***en
    @brief Flag bits to control text property.

    The mtext_property () function accepts logical OR of these flag
    bits as an argument.  They control the behaviour of the created
    text property as described in the documentation of each flag
    bit.  */

/***ja
    @brief テキストプロパティを制御するフラグビット.

    関数 mtext_property () は以下のフラグビットの論理
    OR を引数としてとることができる。
    フラグビットは生成されたテキストプロパティの振舞いを制御する。
    詳細は各フラグビットの説明を参照。*/

enum MTextPropertyControl
  {
    /***en If this flag bit is on, an M-text inserted at the start
	position or at the middle of the text property inherits the
	text property.  */
    /***ja このビットが on ならば、このテキストプロパティの始まる点あるいは中間に挿入された
	M-text はこのテキストプロパティを継承する。
	*/
    MTEXTPROP_FRONT_STICKY = 0x01,

    /***en If this flag bit is on, an M-text inserted at the end
	position or at the middle of the text property inherits the
	text property.  */
    /***ja このビットが on ならば、このテキストプロパティの終わる点あるいは中間に挿入された
	M-text はこのテキストプロパティを継承する。
	*/
    MTEXTPROP_REAR_STICKY = 0x02,

    /***en If this flag bit is on, the text property is removed if a
	text in its region is modified.  */
    /***ja このビットが on ならば、このテキストプロパティの範囲内のテキストが変更された場合テキストプロパティは取り除かれる。  */
    MTEXTPROP_VOLATILE_WEAK = 0x04,

    /***en If this flag bit is on, the text property is removed if a
	text or the other text property in its region is modified.  */
    /***ja このビットが on ならば、このテキストプロパティの範囲内のテキストあるいは別のテキストプロパティが変更された場合このテキ
	ストプロパティは取り除かれる。*/
    MTEXTPROP_VOLATILE_STRONG = 0x08,

    /***en If this flag bit is on, the text property is not
	automatically merged with the others.  */
    /***ja このビットが on ならば、このテキストプロパティは他のプロパティと自動的にはマージされない。 */
    MTEXTPROP_NO_MERGE = 0x10,

    MTEXTPROP_CONTROL_MAX = 0x1F
  };

/*=*/
extern MSymbol Mtext_prop_serializer;
extern MSymbol Mtext_prop_deserializer;


/*** @ingroup m17nTextProperty */
/***en
    @brief Type of serializer functions.

    This is the type of serializer functions.  If the key of a symbol
    property is #Mtext_prop_serializer, the value must be of this
    type.

    @seealso
    mtext_serialize (), #Mtext_prop_serializer
*/
/***ja
    @brief シリアライザ関数の型宣言.

    シリアライザ関数の型である。 あるシンボルのプロパティのキーが @c
    #Mtext_prop_serializer であるとき、 値はこの型でなくてはならない。

    @seealso
    mtext_serialize (), #Mtext_prop_serializer
*/

typedef MPlist *(*MTextPropSerializeFunc) (void *val);

/*** @ingroup m17nTextProperty */
/***en
    @brief Type of deserializer functions.

    This is the type of deserializer functions.  If the key of a
    symbol property is #Mtext_prop_deserializer, the value must be of
    this type.

    @seealso
    mtext_deserialize (), #Mtext_prop_deserializer
*/
/***ja
    @brief デシリアライザ関数の型宣言.

    デシリアライザ関数の型である。 あるシンボルのプロパティのキーが @c
    #Msymbol_prop_deserializer であるとき、 値はこの型でなくてはならない。

    @seealso
    Mtext_prop_deserialize (), Mtext_prop_deserializer
*/
typedef void *(*MTextPropDeserializeFunc) (MPlist *plist);

extern void *mtext_get_prop (MText *mt, int pos, MSymbol key);

extern int mtext_get_prop_values (MText *mt, int pos, MSymbol key,
				  void **values, int num);

extern int mtext_get_prop_keys (MText *mt, int pos, MSymbol **keys);

extern int mtext_put_prop (MText *mt, int from, int to,
			   MSymbol key, void *val);

extern int mtext_put_prop_values (MText *mt, int from, int to,
				  MSymbol key, void **values, int num);

extern int mtext_push_prop (MText *mt, int from, int to,
			    MSymbol key, void *val);

extern int mtext_pop_prop (MText *mt, int from, int to,
			   MSymbol key);

extern int mtext_prop_range (MText *mt, MSymbol key, int pos,
			     int *from, int *to, int deeper);

/*=*/
/***
    @ingroup m17nTextProperty */
/***en
    @brief Type of text properties.

    The type #MTextProperty is for a @e text @e property objects.  Its
    internal structure is concealed from application programs.  */
/***ja
    @brief @c テキストプロパティの型宣言.

    #MTextProperty は @e テキストプロパティ オブジェクトの型である。
    内部構造はアプリケーションプログラムからは見えない。  */

typedef struct MTextProperty MTextProperty;

/*=*/

extern MTextProperty *mtext_property (MSymbol key, void *val,
				      int control_bits);

extern MText *mtext_property_mtext (MTextProperty *prop);

extern MSymbol mtext_property_key (MTextProperty *prop);

extern void *mtext_property_value (MTextProperty *prop);

extern int mtext_property_start (MTextProperty *prop);

extern int mtext_property_end (MTextProperty *prop);

extern MTextProperty *mtext_get_property (MText *mt, int pos, MSymbol key);

extern int mtext_get_properties (MText *mt, int pos, MSymbol key,
				 MTextProperty **props, int num);

extern int mtext_attach_property (MText *mt, int from, int to,
				  MTextProperty *prop);

extern int mtext_detach_property (MTextProperty *prop);

extern int mtext_push_property (MText *mt, int from, int to,
				MTextProperty *prop);

extern MText *mtext_serialize (MText *mt, int from, int to,
			       MPlist *property_list);

extern MText *mtext_deserialize (MText *mt);

/*** @ingroup m17nCore */
/***en @defgroup m17nDatabase Database */
/***ja @defgroup m17nDatabase データベース */
/*=*/

/* Directory of an application specific databases.  */
extern char *mdatabase_dir;
/*=*/
/***
    @ingroup m17nDatabase  */ 
/***en
    @brief Type of database.

    The type #MDatabase is for a database object.  Its internal
    structure is concealed from an application program.  */
/***ja 
    @brief データベースの型宣言.

    #MDatabase 型はデータベースオブジェクト用の構造体である。
    内部構造はアプリケーションプログラムからは見えない。
    */

typedef struct MDatabase MDatabase;

/*=*/

/* Look for a data.  */
extern MDatabase *mdatabase_find (MSymbol tag1, MSymbol tag2,
				  MSymbol tag3, MSymbol tag4);

extern MPlist *mdatabase_list (MSymbol tag0, MSymbol tag1,
			       MSymbol tag2, MSymbol tag3);

/* Load a data.  */
void *mdatabase_load (MDatabase *mdb);

/* Get tags of a data.  */
extern MSymbol *mdatabase_tag (MDatabase *mdb);

/* Define a data.  */
extern MDatabase *mdatabase_define (MSymbol tag1, MSymbol tag2,
				    MSymbol tag3, MSymbol tag4,
				    void *(*loader) (MSymbol *, void *),
				    void *extra_info);

M17N_END_HEADER

#endif /* _M17N_CORE_H_ */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
