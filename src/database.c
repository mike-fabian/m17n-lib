/* database.c -- database module.
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
    @addtogroup m17nDatabase
    @brief The m17n database and API for it.

    The m17n library dynamically acquires various kinds of information
    in need from data in the <i> m17n database</i>.  Application
    programs can also add/load their original data to/from the m17n
    database.  The m17n database contains multiple heterogeneous data,
    and each data is identified by four tags; TAG0, TAG1, TAG2, TAG3.
    Each tag must be a symbol.

    TAG0 specifies the type of data stored in the database as below.

    @li

    If TAG0 is #Mchar_table, the data is of the @e chartable @e
    type and provides information about each character.  In this case,
    TAG1 specifies the type of the information and must be #Msymbol,
    #Minteger, #Mstring, #Mtext, or #Mplist.  TAG2 and TAG3 can be any
    symbols.

    @li
    
    If TAG0 is #Mcharset, the data is of the @e charset @e type
    and provides a decode/encode mapping table for a charset.  In this
    case, TAG1 must be a symbol representing a charset.  TAG2 and TAG3
    can be any symbols.

    @li 
    
    If TAG0 is neither #Mchar_table nor #Mcharset, the data is of
    the @e plist @e type.  See the documentation of the mdatabase_load
    () function for the details.  In this case, TAG1, TAG2, and TAG3
    can be any symbols.

    The notation \<TAG0, TAG1, TAG2, TAG3\> means a data with those
    tags.

    Application programs first calls the mdatabase_find () function to
    get a pointer to an object of the type #MDatabase.  That object
    holds information about the specified data.  When it is
    successfully returned, the mdatabase_load () function loads the
    data.  The implementation of the structure #MDatabase is
    concealed from application programs.
*/

/***ja
    @addtogroup m17nDatabase
    @brief m17n データベースにとそれに関する API.

    m17n ライブラリは必要に応じて動的に @e m17n @e データベース から情
    報を取得する。また、アプリケーションプログラムも独自のデータを 
    m17n データベースに追加し、それを動的に取得することができる。m17n 
    データベースには複数の多様なデータが含まれており、各データは４つの
    タグ TAG0, TAG1, TAG2, TAG3（すべてシンボル）によって識別される。

    TAG0 はデータベース内のデータのタイプを以下のように指定する。

    @li 

    TAG0 が #Mchar_table であるデータは @e chartableタイプ と呼ばれ、
    各文字に関する情報を提供する。この場合 TAG1 は情報の種類を指定する
    シンボルであり、#Msymbol, #Minteger, #Mstring, #Mtext, #Mplist の
    いずれかである。TAG2 と TAG3 は任意のシンボルでよい。

    @li 

    TAG0 が #Mcharset であるデータは @e charsetタイプ と呼ばれ、文
    字セット用のデコード／エンコードマップを提供する。この場合 TAG1 は
    文字セットのシンボルでなければならない。TAG2 と TAG3 は任意のシン
    ボルでよい。

    @li

    TAG0 が #Mchar_table でも #Mcharset でもない場合、そのデータは @e
    plist タイプ である。詳細に関しては関数 mdatabase_load () の説明を
    参照のこと。この場合 TAG1、TAG2、TAG3 は任意のシンボルでよい。

    特定のタグを持つデータベースを \<TAG0, TAG1, TAG2, TAG3\> という形
    式で表わす。

    アプリケーションプログラムは、まず関数 mdatabase_find () を使ってデー
    タベースに関する情報を保持するオブジェクト（#MDatabase 型）への
    ポインタを得る。それに成功したら、 mdatabase_load () によって実際
    にデータベースをロードする。構造体 #MDatabase 自身がどう実装され
    ているかは、アプリケーションプログラムからは見えない。

    @latexonly \IPAlabel{database} @endlatexonly
*/

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "mtext.h"
#include "character.h"
#include "charset.h"
#include "database.h"
#include "coding.h"
#include "plist.h"

/** The file containing a list of databases.  */
#define MDB_DIR "mdb.dir"
#define MDB_DIR_LEN 8

/** List of database directories.  */ 
static MPlist *mdb_dir_list;

/** Structure for a data in the m17n database.  */

struct MDatabase
{
  /** Tags to identify the data.  <tag>[0] specifies the type of
      database.  If it is #Mchar_table, the type is @e chartable, if
      it is #Mcharset, the type is @e charset, otherwise the type is
      @e plist.  */
  MSymbol tag[4];

  void *(*loader) (MSymbol *tags, void *extra_info);

  /** The meaning of the value is dependent on <loader>.  If <loader>
      is load_database (), the value is a string of the file name that
      contains the data.  */
  void *extra_info;
};

/** List of all data.  */
struct MDatabaseList
{
  int size, inc, used;
  MDatabase *mdbs;
};

static struct MDatabaseList mdb_list;


static int
read_number (char *buf, int *i)
{
  int idx = *i;
  int c = buf[idx++];
  int n;

  if (!c)
    return -1;

  while (c && isspace (c)) c = buf[idx++];

  if (c == '0')
    {
      if (buf[idx] == 'x')
	{
	  for (idx++, c = 0; (n = hex_mnemonic[(unsigned) buf[idx]]) < 16;
	       idx++)
	    c  = (c << 4) | n;
	  *i = idx;
	  return c;
	}
      c = 0;
    }
  else if (c == '\'')
    {
      c = buf[idx++];
      if (c == '\\')
	{
	  c = buf[idx++];
	  n = escape_mnemonic[c];
	  if (n != 255)
	    c = n;
	}
      while (buf[idx] && buf[idx++] != '\'');
      *i = idx;
      return c;
    }
  else if (hex_mnemonic[c] < 10)
    c -= '0';
  else
    return -1;

  while ((n = hex_mnemonic[(unsigned) buf[idx]]) < 10)
    c = (c * 10) + n, idx++;
  *i = idx;
  return c;
}


/** Load a data of type @c chartable from the file FD, and return the
    newly created chartable.  */

static void *
load_chartable (FILE *fp, MSymbol type)
{
  int c, from, to;
  char buf[1024];
  void *val;
  MCharTable *table;

  if (! fp)
    MERROR (MERROR_DB, NULL);

  table = mchartable (type, (type == Msymbol ? (void *) Mnil
			     : type == Minteger ? (void *) -1
			     : NULL));

  while (! feof (fp))
    {
      int i, len;

      for (len = 0; len < 1023 && (c = getc (fp)) != EOF && c != '\n'; len++)
	buf[len] = c;
      buf[len] = '\0';	  
      if (hex_mnemonic[(unsigned) buf[0]] >= 10)
	/* skip comment/invalid line */
	continue;
      i = 0;
      from = read_number (buf, &i);
      if (buf[i] == '-')
	i++, to = read_number (buf, &i);
      else
	to = from;
      if (from < 0 || to < 0)
	goto label_error;

      while (buf[i] && isspace ((unsigned) buf[i])) i++;
      c = buf[i];
      if (!c)
	break;

      if (type == Mstring)
	{
	  /* VAL is a C-string.  */
	  if (! (val = strdup (buf + i)))
	    MEMORY_FULL (MERROR_DB);
	}
      else if (type == Minteger)
	{
	  /* VAL is an integer.  */
	  int positive = 1;
	  int n;

	  if (c == '-')
	    i++, positive = -1;
	  n = read_number (buf, &i);
	  if (n < 0)
	    goto label_error;
	  val = (void *) (n * positive);
	}
      else if (type == Mtext)
	{
	  /* VAL is an M-text.  */
	  MText *mt;
	  if (c == '"')
	    mt = mconv_decode_buffer (Mcoding_utf_8,
				      (unsigned char *) (buf + i),
				      len - i - 1);
	  else
	    {
	      mt = mtext ();
	      while ((c = read_number (buf, &i)) >= 0)
		mt = mtext_cat_char (mt, c);
	    }
	  val = (void *) mt;
	}
      else if (type == Msymbol)
	{
	  if (! strcmp (buf + i, "nil"))
	    val = (void *) Mnil;
	  else
	    val = (void *) msymbol (buf + i);
	}
      else if (type == Mplist)
	{
	  val = (void *) mplist__from_string ((unsigned char *) buf + i,
					      strlen (buf + i));
	}
      else
	val = NULL;

      if (from == to)
	mchartable_set (table, from, val);
      else
	mchartable_set_range (table, from, to, val);
    }
  return table;

 label_error:
  M17N_OBJECT_UNREF (table);
  MERROR (MERROR_DB, NULL);
}


/** Load a data of type @c charset from the file FD.  */

static void *
load_charset (FILE *fp, MSymbol charset_name)
{
  MCharset *charset = MCHARSET (charset_name);
  int *decoder;
  MCharTable *encoder;
  int size;
  int i, c;
  int found = 0;
  MPlist *plist;

  if (! charset)
    MERROR (MERROR_DB, NULL);
  size = (charset->code_range[15]
	  - (charset->min_code - charset->code_range_min_code));
  MTABLE_MALLOC (decoder, size, MERROR_DB);
  for (i = 0; i < size; i++)
    decoder[i] = -1;
  encoder = mchartable (Minteger, (void *) MCHAR_INVALID_CODE);

  while ((c = getc (fp)) != EOF)
    {
      unsigned code1, code2, c1, c2;
      int idx1, idx2;
      char buf[256];

      ungetc (c, fp);
      fgets (buf, 256, fp);
      if (c != '#')
	{
	  if (sscanf (buf, "0x%x-0x%x 0x%x", &code1, &code2, &c1) == 3)
	    {
	      idx1 = CODE_POINT_TO_INDEX (charset, code1);
	      if (idx1 >= size)
		continue;
	      idx2 = CODE_POINT_TO_INDEX (charset, code2);
	      if (idx2 >= size)
		idx2 = size - 1;
	      c2 = c1 + (idx2 - idx1);
	    }
	  else if (sscanf (buf, "0x%x 0x%x", &code1, &c1) == 2)
	    {
	      idx1 = idx2 = CODE_POINT_TO_INDEX (charset, code1);
	      if (idx1 >= size)
		continue;
	      c2 = c1;
	    }
	  else
	    continue;
	  if (idx1 >= 0 && idx2 >= 0)
	    {
	      decoder[idx1] = c1;
	      mchartable_set (encoder, c1, (void *) code1);
	      for (idx1++, c1++; idx1 <= idx2; idx1++, c1++)
		{
		  code1 = INDEX_TO_CODE_POINT (charset, idx1);
		  decoder[idx1] = c1;
		  mchartable_set (encoder, c1, (void *) code1);
		}
	      found++;
	    }
	}
    }

  if (! found)
    {
      free (decoder);
      M17N_OBJECT_UNREF (encoder);
      return NULL;
    }
  plist = mplist ();
  mplist_add (plist, Mt, decoder);
  mplist_add (plist, Mt, encoder);
  return plist;
}

static char *
gen_database_name (char *buf, MSymbol *tags)
{
  int i;

  strcpy (buf, msymbol_name (tags[0]));
  for (i = 1; i < 4; i++)
    {
      strcat (buf, ", ");
      strcat (buf, msymbol_name (tags[i]));
    }
  return buf;
}

static void *
load_database (MSymbol *tags, void *extra_info)
{
  FILE *fp;
  char *filename = (char *) extra_info;
  void *value;

  if (filename[0] == '/')
    fp = fopen (filename, "r");
  else
    {
      MPlist *plist;
      char path[PATH_MAX];

      MPLIST_DO (plist, mdb_dir_list)
	{
	  strcpy (path, (char *) MPLIST_VAL (plist));
	  strcat (path, filename);
	  fp = fopen (path, "r");
	  if (fp)
	    break;
	}
    }
  if (! fp)
    MERROR (MERROR_DB, NULL);

  if (tags[0] == Mchar_table)
    value = load_chartable (fp, tags[1]);
  else if (tags[0] == Mcharset)
    value = load_charset (fp, tags[1]);
  else
    value = mplist__from_file (fp);
  fclose (fp);

  if (! value)
    MERROR (MERROR_DB, NULL);
  return value;
}


/** Copy DIRNAME to a newly allocated memory and return it.  If
    DIRNAME does not end with a slash, append a slash to the new memory.  */

static char *
duplicate_dirname (char *dirname)
{
  struct stat buf;
  int len;
  char *str;

  if (! dirname
      || stat (dirname, &buf) < 0)
    return NULL;

  len = strlen (dirname);
  MTABLE_MALLOC (str, len + 2, MERROR_DB);
  memcpy (str, dirname, len + 1);
  if (str[len - 1] != '/')
    {
      str[len] = '/';
      str[len + 1] = '\0';
    }
  return str;
}


/* Internal API */

int
mdatabase__init ()
{
  char *dir;
  int i;
  MPlist *plist;
  FILE *fp;

  Mchar_table = msymbol ("char-table");

  mdb_dir_list = mplist ();
  /** The macro M17NDIR specifies a directory where the system-wide
    MDB_DIR file exists.  */
  if ((dir = duplicate_dirname (M17NDIR)))
    mplist_set (mdb_dir_list, Mt, dir);

  /* The variable mdatabase_dir specifies a directory where an
     application program specific MDB_DIR file exists.  */
  if ((dir = duplicate_dirname (mdatabase_dir)))
    mplist_push (mdb_dir_list, Mt, dir);

  /* The environment variable M17NDIR (if non-NULL) specifies a
     directory where a user specific MDB_DIR file exists.  */
  if ((dir = duplicate_dirname (getenv ("M17NDIR"))))
    mplist_push (mdb_dir_list, Mt, dir);

  MLIST_INIT1 (&mdb_list, mdbs, 256);
  MPLIST_DO (plist, mdb_dir_list)
    {
      MPlist *pl, *p;
      int len;
      char path[PATH_MAX];

      dir = (char *) MPLIST_VAL (plist);
      len = strlen (dir);
      if (len + MDB_DIR_LEN >= PATH_MAX)
	continue;
      memcpy (path, dir, len);
      memcpy (path + len, MDB_DIR, MDB_DIR_LEN);
      if (! (fp = fopen (path, "r")))
	continue;
      pl = mplist__from_file (fp);
      fclose (fp);
      if (! pl)
	continue;
      MPLIST_DO (p, pl)
	{
	  MDatabase mdb;
	  MPlist *p1;
	  int nbytes;

	  if (! MPLIST_PLIST_P (p))
	    continue;
	  for (i = 0, p1 = MPLIST_PLIST (p);
	       i < 4 && MPLIST_KEY (p1) == Msymbol;
	       i++, p1 = MPLIST_NEXT (p1))
	    mdb.tag[i] = MPLIST_SYMBOL (p1);
	  if (i == 0
	      || ! MPLIST_MTEXT_P (p1))
	    continue;
	  for (; i < 4; i++)
	    mdb.tag[i] = Mnil;
	  if (mdatabase_find (mdb.tag[0], mdb.tag[1],
			      mdb.tag[2], mdb.tag[3]))
	    continue;

	  mdb.loader = load_database;
	  nbytes = mconv_encode_buffer (Mcoding_utf_8,  MPLIST_MTEXT (p1),
					(unsigned char *) path, PATH_MAX);
	  if (nbytes < 0 || nbytes >= PATH_MAX)
	    continue;
	  path[nbytes++] = '\0';
	  mdb.extra_info = (void *) strdup (path);
	  MLIST_APPEND1 (&mdb_list, mdbs, mdb, MERROR_DB);
	}
      M17N_OBJECT_UNREF (pl);
    }

  mdatabase__finder = ((void *(*) (MSymbol, MSymbol, MSymbol, MSymbol))
		       mdatabase_find);
  mdatabase__loader = (void *(*) (void *)) mdatabase_load;

  return 0;
}

void
mdatabase__fini (void)
{
  int i;
  MPlist *plist; 

  MPLIST_DO (plist, mdb_dir_list)
    free (MPLIST_VAL (plist));
  M17N_OBJECT_UNREF (mdb_dir_list);

  for (i = 0; i < mdb_list.used; i++)
    {
      MDatabase *mdb = mdb_list.mdbs + i;

      if (mdb->loader == load_database)
	free (mdb->extra_info);
    }
  MLIST_FREE1 (&mdb_list, mdbs);
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

/*** @addtogroup m17nDatabase */
/*** @{ */

/*=*/
/***en
    @brief Directory for application specific data.

    If an application program wants to provide a data specific to the
    program or a data overriding what supplied by the m17n database,
    it must set this variable to a name of directory that contains the
    data files before it calls the macro M17N_INIT ().  The directory
    may contain a file "mdb.dir" which contains a list of data
    definitions in the format described in @ref mdbDir "mdbDir(5)".

    The default value is NULL.  */
/***ja
    @brief アプリケーション固有のデータ用ディレクトリ.

    アプリケーションプログラムが、そのプログラム固有のデータや m17n デー
    タベースを上書きするデータを提供する場合には、マクロ M17N_INIT () 
    を呼ぶ前にこの変数をデータファイルを含むディレクトリ名にセットしな
    くてはならない。ディレクトリには "mdb.dir" ファイルをおくことがで
    きる。その"mdb.dir"ファイルには、 @ref mdbDir "mdbDir(5)" で説明さ
    れているフォーマットでデータ定義のリストを記述する。

    デフォルトの値は NULL である。  */

char *mdatabase_dir;

/*=*/
/***en
    @brief Look for a data in the database.

    The mdatabase_find () function searches the m17n database for a
    data who has tags $TAG0 through $TAG3, and returns a pointer to
    the data.  If such a database is not found, it returns @c
    NULL.  */

/***ja
    @brief データベースを探す.

    関数 mdatabase_find () は、 m17n 言語情報ベース中で $TAG0 から 
    $TAG3 までのタグを持つデータベースを探し、それへのポインタを返す。
    そのようなデータベースがなければ @c NULL を返す。

    @latexonly \IPAlabel{mdatabase_find} @endlatexonly  */

MDatabase *
mdatabase_find (MSymbol tag0, MSymbol tag1, MSymbol tag2, MSymbol tag3)
{
  int i;

  for (i = 0; i < mdb_list.used; i++)
    {
      MDatabase *mdb = mdb_list.mdbs + i;

      if (tag0 == mdb->tag[0]
	  && tag1 == mdb->tag[1]
	  && tag2 == mdb->tag[2]
	  && tag3 == mdb->tag[3])
	return mdb;
    }
  return NULL;
}

/*=*/
/***en
    @brief Return a data list of the m17n database.

    The mdatabase_list () function searches the m17n database for data
    who have tags $TAG0 through $TAG3, and returns their list by a
    plist.  The value #Mnil in $TAGn means a wild card that matches
    any tag.  Each element of the plist has key #Mt and value a
    pointer to type #MDatabase.  */
/***ja
    @brief m17n データベースのデータリストを返す.

    関数 mdatabase_list () は m17n データベース中から特定のタグ$TAG0
    .. $TAG3 を持つデータを探し、そのリストをplist として返す。 $TAGn 
    が #Mnil であった場合には、任意のタグにマッチするワイルドカードと
    して取り扱われる。返される plist の各要素はキー として #Mt を、値
    として #MDatabase 型へのポインタを持つ。  */


MPlist *
mdatabase_list (MSymbol tag0, MSymbol tag1, MSymbol tag2, MSymbol tag3)
{
  int i;
  MPlist *plist = NULL, *pl;

  for (i = 0; i < mdb_list.used; i++)
    {
      MDatabase *mdb = mdb_list.mdbs + i;

      if ((tag0 == Mnil || tag0 == mdb->tag[0])
	  && (tag1 == Mnil || tag1 == mdb->tag[1])
	  && (tag2 == Mnil || tag2 == mdb->tag[2])
	  && (tag3 == Mnil || tag3 == mdb->tag[3]))
	{
	  if (! plist)
	    plist = pl = mplist ();
	  pl = mplist_add (pl, Mt, mdb);
	}
    }
  return plist;
}



/*=*/
/***en
    @brief Define a data of the m17n database.

    The mdatabase_define () function defines a data that has tags
    $TAG0 through $TAG3 and additional information $EXTRA_INFO.

    $LOADER is a pointer to a function that loads the data from the
    database.  This function is called from the mdatabase_load ()
    function with the two arguments $TAGS and $EXTRA_INFO.  Here,
    $TAGS is the array of $TAG0 through $TAG3.

    If $LOADER is @c NULL, the default loader of the m17n library is
    used.  In this case, $EXTRA_INFO must be a string specifying a
    filename that contains the data.

    @return
    If the operation was successful, mdatabase_define () returns a
    pointer to the defined data, which can be used as an argument to
    mdatabase_load ().  Otherwise, it returns @c NULL.  */

/***ja
    @brief m17n データベースのデータを定義する.

    関数 mdatabase_define () は $TAG0 から $TAG3 までのタグおよび付加
    情報 $EXTRA_INFO を持つデータを定義する。

    $LOADER はそのデータのロードに用いられる関数へのポインタである。こ
    の関数は mdatabase_load () から $TAGS と $EXTRA_INFO という2 つの
    引数付きで呼び出される。ここで $TAGS は $TAG0 から $TAG3 までの配
    列である。

    もし $LOADER が @c NULL なら、m17n ライブラリ標準のローダが使われ
    る。この場合には $EXTRA_INFO はデータを含むファイル名でなくてはな
    らない。

    @return 処理に成功すれば mdatabase_define () は定義されたデータベー
    スへのポインタを返す。このポインタは関数 mdatabase_load () の引数
    として用いることができる。そうでなければ @c NULL を返す。

    @latexonly \IPAlabel{mdatabase_define} @endlatexonly  */

/***
    @seealso
    mdatabase_load (),  mdatabase_define ()  */

MDatabase *
mdatabase_define (MSymbol tag0, MSymbol tag1, MSymbol tag2, MSymbol tag3,
		  void *(*loader) (MSymbol *, void *),
		  void *extra_info)
{
  MDatabase *mdb;

  mdb = mdatabase_find (tag0, tag1, tag2, tag3);
  if (! mdb)
    {
      MDatabase template;

      template.tag[0] = tag0, template.tag[1] = tag1;
      template.tag[2] = tag2, template.tag[3] = tag3;
      MLIST_APPEND1 (&mdb_list, mdbs, template, MERROR_DB);
      mdb = mdb_list.mdbs + (mdb_list.used - 1);
    }
  mdb->loader = loader ? loader : load_database;
  mdb->extra_info = extra_info;
  if (mdb->loader == load_database)
    mdb->extra_info = strdup ((char *) extra_info);
  return (&(mdb_list.mdbs[mdb_list.used - 1]));
}

/*=*/
/***en
    @brief Load a data from the database.

    The mdatabase_load () function loads a data specified in $MDB and
    returns the contents.  The type of contents depends on the type of
    the data.

    If the data is of the @e plist type, this function returns a
    pointer to @e plist.

    If the database is of the @e chartable type, it returns a
    chartable.  The default value of the chartable is set according to
    the second tag of the data as below:

    @li If the tag is #Msymbol, the default value is #Mnil.
    @li If the tag is #Minteger, the default value is -1.
    @li Otherwise, the default value is @c NULL.

    If the data is of the @e charset type, it returns a plist of length 2
    (keys are both #Mt).  The value of the first element is an array
    of integers that maps code points to the corresponding character
    codes.  The value of the second element is a chartable of integers
    that does the reverse mapping.  The charset must be defined in
    advance.  */


/***ja
    @brief データベースからデータをロードする.

    関数 mdatabase_load () は $MDB が指すデータをロードし、その
    中身を返す。返されるものはデータのタイプによって異なる。

    データが @e plist タイプならば、 @e plist へのポインタを返す。

    データが @e chartable タイプならば文字テーブルを返す。文字テーブル
    のデフォルト値は、データの第2タグによって以下のように決まる。

    @li タグが #Msymbol なら、デフォルト値は #Mnil
    @li タグが #Minteger なら、デフォルト値は -1
    @li それ以外なら、デフォルト値は @c NULL

    データが @e charset タイプならば長さ 2 の plist を返す（キーは共に 
    #Mt ）。最初の要素の値はコードポイントを対応する文字コードにマップ
    する整数の配列である。２番目の要素の値は逆のマップをする文字テーブ
    ルである。この文字セットは予め定義されていなければならない。

    @latexonly \IPAlabel{mdatabase_load} @endlatexonly
  */

/***
    @seealso
    mdatabase_load (),  mdatabase_define ()  */

void *
mdatabase_load (MDatabase *mdb)
{
  int mdebug_mask = MDEBUG_DATABASE;
  char buf[256];

  MDEBUG_PRINT1 (" [DATABASE] loading <%s>.\n",
		 gen_database_name (buf, mdb->tag));
  return (*mdb->loader) (mdb->tag, mdb->extra_info);
}

/*=*/
/***en
    @brief Get tags of a data.

    The mdatabase_tag () function returns an array of tags (symbols)
    that identify the data in $MDB.  The length of the array is
    four.  */

/***oldja
    @brief データのタグを得る.

    関数 mdatabase_tag () は、データ $MDB のタグ（シンボル）の配列を返
    す。配列の長さは 4 である。

    @latexonly \IPAlabel{mdatabase_tag} @endlatexonly  */

MSymbol *
mdatabase_tag (MDatabase *mdb)
{
  return mdb->tag;
}

/*** @} */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
