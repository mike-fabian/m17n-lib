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
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   02111-1307, USA.  */

/***en
    @addtogroup m17nDatabase
    @brief The m17n database and API for it.

    The m17n library acquires various kinds of information
    from data in the <i> m17n database</i> on demand.  Application
    programs can also add/load their original data to/from the m17n
    database by setting the variable #mdatabase_dir to an
    application-specific directory and storing data in it.  Users can
    overwrite those data by storing preferable data in the directory
    specified by the environment variable "M17NDIR", or if it is not
    set, in the directory "~/.m17n.d".

    The m17n database contains multiple heterogeneous data, and each
    data is identified by four tags; TAG0, TAG1, TAG2, TAG3.  Each tag
    must be a symbol.

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
    the @e plist @e type.  See the documentation of the 
    mdatabase_load () function for the details.  
    In this case, TAG1, TAG2, and TAG3 can be any symbols.

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

    m17n ライブラリは必要に応じて動的に @e m17n @e データベース 
    から情報を取得する。またアプリケーションプログラムも、独自のデータを 
    m17n データベースに追加し、それを動的に取得することができる。
    アプリケーションプログラムが独自のデータを追加・取得するには、変数 
    #mdatabase_dir にそのアプリケーション固有のディレクトリをセットし、
    その中にデータを格納する。ユーザがそのデータをオーバーライトしたい
    ときは、環境変数 "M17NDIR" で指定されるディレクトリ（指定されていな
    いときは "~/.m17n.d" というディレクトリ）に別のデータを置く。

    m17n 
    データベースには複数の多様なデータが含まれており、各データは
    TAG0, TAG1, TAG2, TAG3（すべてシンボル）の４つのタグによって識別される。

    TAG0 によって、データベース内のデータのタイプは次のように指定される。

    @li 
    TAG0 が #Mchar_table であるデータは @e chartableタイプ 
    と呼ばれ、各文字に関する情報を提供する。この場合
    TAG1 は情報の種類を指定するシンボルであり、#Msymbol, #Minteger, #Mstring,
    #Mtext, #Mplist のいずれかである。TAG2 と TAG3 は任意のシンボルでよい。

    @li 
    TAG0 が #Mcharset であるデータは @e charsetタイプ 
    と呼ばれ、文字セット用のデコード／エンコードマップを提供する。この場合 TAG1
    は文字セットのシンボルでなければならない。TAG2 と TAG3
    は任意のシンボルでよい。

    @li
    TAG0 が #Mchar_table でも #Mcharset でもない場合、そのデータは @e
    plistタイプ である。詳細に関しては関数 mdatabase_load () 
    の説明を参照のこと。この場合 TAG1、TAG2、TAG3 は任意のシンボルでよい。

    特定のタグを持つデータベースを \<TAG0, TAG1, TAG2, TAG3\> 
    という形式で表す。

    アプリケーションプログラムは、まず関数 mdatabase_find () 
    を使ってデータベースに関する情報を保持するオブジェクト（#MDatabase
    型）へのポインタを得る。それに成功したら、 mdatabase_load () 
    によって実際にデータベースをロードする。構造体 #MDatabase 
    自身がどう実装されているかは、アプリケーションプログラムからは見えない。

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
#include <glob.h>
#include <time.h>
#include <libgen.h>

#include "m17n-core.h"
#include "m17n-misc.h"
#include "internal.h"
#include "mtext.h"
#include "character.h"
#include "database.h"
#include "plist.h"

/** The file containing a list of databases.  */
#define MDB_DIR "mdb.dir"
/** Length of MDB_DIR.  */
#define MDB_DIR_LEN 7

#define MAX_TIME(TIME1, TIME2) ((TIME1) >= (TIME2) ? (TIME1) : (TIME2))

#define GEN_PATH(path, dir, dir_len, file, file_len)	\
  (dir_len + file_len > PATH_MAX ? 0			\
   : (memcpy (path, dir, dir_len),			\
      memcpy (path + dir_len, file, file_len),		\
      path[dir_len + file_len] = '\0', 1))

static MSymbol Masterisk;
static MSymbol Mversion;

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

static MPlist *mdatabase__list;

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
      if (from < 0 || to < from)
	continue;

      while (buf[i] && isspace ((unsigned) buf[i])) i++;
      c = buf[i];
      if (!c)
	continue;

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
	    mt = mtext__from_data (buf + i, len - i - 1, MTEXT_FORMAT_UTF_8, 1);
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
	  char *p = buf + i;

	  while (*p && ! isspace (*p)) 
	    {
	      if (*p == '\\' && p[1] != '\0')
		{
		  memmove (p, p + 1, buf + len - (p + 1));
		  len--;
		}
	      p++;
	    }
	  *p = '\0';
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


static char *
gen_database_name (char *buf, MSymbol *tags)
{
  int i;

  strcpy (buf, msymbol_name (tags[0]));
  for (i = 1; i < 4; i++)
    {
      strcat (buf, ",");
      strcat (buf, msymbol_name (tags[i]));
    }
  return buf;
}

/* Return the absolute file name for DB_INFO->filename or NULL if no
   absolute file name was found.  If BUF is non-NULL, store the result
   of `stat' call in it.  In that case, set *RESULT to the return
   value of `stat'.  */

char *
get_database_file (MDatabaseInfo *db_info, struct stat *buf, int *result)
{
  if (db_info->absolute_filename)
    {
      if (buf)
	*result = stat (db_info->absolute_filename, buf);
    }
  else
    {
      struct stat stat_buf;
      struct stat *statbuf = buf ? buf : &stat_buf;
      int res;
      MPlist *plist;
      char path[PATH_MAX + 1];

      MPLIST_DO (plist, mdatabase__dir_list)
	{
	  MDatabaseInfo *dir_info = MPLIST_VAL (plist);

	  if (dir_info->status != MDB_STATUS_DISABLED
	      && GEN_PATH (path, dir_info->filename, dir_info->len,
			   db_info->filename, db_info->len)
	      && (res = stat (path, statbuf)) == 0)
	    {
	      db_info->absolute_filename = strdup (path);
	      if (result)
		*result = res;
	      break;
	    }
	}
    }

  return db_info->absolute_filename;
}

static void *
load_database (MSymbol *tags, void *extra_info)
{
  MDatabaseInfo *db_info = extra_info;
  void *value;
  char *filename = get_database_file (db_info, NULL, NULL);
  FILE *fp;
  int mdebug_flag = MDEBUG_DATABASE;
  char buf[256];

  MDEBUG_PRINT1 (" [DB] <%s>", gen_database_name (buf, tags));
  if (! filename || ! (fp = fopen (filename, "r")))
    {
      if (filename)
	MDEBUG_PRINT1 (" open fail: %s\n", filename);
      else
	MDEBUG_PRINT1 (" not found: %s\n", db_info->filename);
      MERROR (MERROR_DB, NULL);
    }

  MDEBUG_PRINT1 (" from %s\n", filename);

  if (tags[0] == Mchar_table)
    value = load_chartable (fp, tags[1]);
  else if (tags[0] == Mcharset)
    {
      if (! mdatabase__load_charset_func)
	MERROR (MERROR_DB, NULL);
      value = (*mdatabase__load_charset_func) (fp, tags[1]);
    }
  else
    value = mplist__from_file (fp, NULL);
  fclose (fp);

  if (! value)
    MERROR (MERROR_DB, NULL);
  db_info->time = time (NULL);
  return value;
}


/** Return a newly allocated MDatabaseInfo for DIRNAME.  */

static MDatabaseInfo *
get_dir_info (char *dirname)
{
  MDatabaseInfo *dir_info;

  MSTRUCT_CALLOC (dir_info, MERROR_DB);
  if (dirname)
    {
      int len = strlen (dirname);

      if (len + MDB_DIR_LEN < PATH_MAX)
	{
	  MTABLE_MALLOC (dir_info->filename, len + 2, MERROR_DB);
	  memcpy (dir_info->filename, dirname, len + 1);
	  /* Append PATH_SEPARATOR if DIRNAME doesn't end with it.  */
	  if (dir_info->filename[len - 1] != PATH_SEPARATOR)
	    {
	      dir_info->filename[len] = PATH_SEPARATOR;
	      dir_info->filename[++len] = '\0';
	    }
	  dir_info->len = len;
	  dir_info->status = MDB_STATUS_OUTDATED;
	}
      else
	dir_info->status = MDB_STATUS_DISABLED;	
    }
  else
    dir_info->status = MDB_STATUS_DISABLED;
  return dir_info;
}

static void register_databases_in_files (MSymbol tags[4],
					 char *filename, int len);

static MDatabase *
find_database (MSymbol tags[4])
{
  MPlist *plist;
  int i;
  MDatabase *mdb;
  
  if (! mdatabase__list)
    return NULL;
  for (i = 0, plist = mdatabase__list; i < 4; i++)
    {
      MPlist *pl = mplist__assq (plist, tags[i]);
      MPlist *p;

      if ((p = mplist__assq (plist, Masterisk)))
	{
	  MDatabaseInfo *db_info;
	  int j;

	  p = MPLIST_PLIST (p);
	  for (j = i + 1; j < 4; j++)
	    p = MPLIST_PLIST (MPLIST_NEXT (p));
	  mdb = MPLIST_VAL (MPLIST_NEXT (p));
	  db_info = mdb->extra_info;
	  if (db_info->status != MDB_STATUS_DISABLED)
	    {
	      register_databases_in_files (mdb->tag,
					   db_info->filename, db_info->len);
	      db_info->status = MDB_STATUS_DISABLED;
	      return find_database (tags);
	    }
	}
      if (! pl)
	return NULL;
      plist = MPLIST_PLIST (pl);
      plist = MPLIST_NEXT (plist);
    }
  mdb = MPLIST_VAL (plist);
  return mdb;
}

static void
free_db_info (MDatabaseInfo *db_info)
{
  free (db_info->filename);
  if (db_info->absolute_filename
      && db_info->filename != db_info->absolute_filename)
    free (db_info->absolute_filename);
  M17N_OBJECT_UNREF (db_info->properties);
  free (db_info);
}

static int
check_version (MText *version)
{
  char *verstr = (char *) MTEXT_DATA (version);
  char *endp = verstr + mtext_nbytes (version);
  int ver[3];
  int i;

  ver[0] = ver[1] = ver[2] = 0;
  for (i = 0; verstr < endp; verstr++)
    {
      if (*verstr == '.')
	{
	  i++;
	  if (i == 3)
	    break;
	  continue;
	}
      if (! isdigit (*verstr))
	break;
      ver[i] = ver[i] * 10 + (*verstr - '0');
    }
  return (ver[0] < M17NLIB_MAJOR_VERSION
	  || (ver[0] == M17NLIB_MAJOR_VERSION
	      && (ver[1] < M17NLIB_MINOR_VERSION
		  || (ver[1] == M17NLIB_MINOR_VERSION
		      && ver[2] <= M17NLIB_PATCH_LEVEL))));
}

static MDatabase *
register_database (MSymbol tags[4],
		   void *(*loader) (MSymbol *, void *),
		   void *extra_info, enum MDatabaseStatus status,
		   MPlist *properties)
{
  MDatabase *mdb;
  MDatabaseInfo *db_info;
  int i;
  MPlist *plist;

  if (properties)
    {
      MPLIST_DO (plist, properties)
	if (MPLIST_PLIST_P (plist))
	  {
	    MPlist *p = MPLIST_PLIST (plist);

	    if (MPLIST_SYMBOL_P (p)
		&& MPLIST_SYMBOL (p) == Mversion
		&& MPLIST_MTEXT_P (MPLIST_NEXT (p)))
	      {
		if (check_version (MPLIST_MTEXT (MPLIST_NEXT (p))))
		  break;
		return NULL;
	      }
	  }
    }

  for (i = 0, plist = mdatabase__list; i < 4; i++)
    {
      MPlist *pl = mplist__assq (plist, tags[i]);

      if (pl)
	pl = MPLIST_PLIST (pl);
      else
	{
	  pl = mplist ();
	  mplist_add (pl, Msymbol, tags[i]);
	  mplist_push (plist, Mplist, pl);
	  M17N_OBJECT_UNREF (pl);
	}
      plist = MPLIST_NEXT (pl);
    }

  if (MPLIST_TAIL_P (plist))
    {
      MSTRUCT_MALLOC (mdb, MERROR_DB);
      for (i = 0; i < 4; i++)
	mdb->tag[i] = tags[i];
      mdb->loader = loader;
      if (loader == load_database)
	{
	  MSTRUCT_CALLOC (db_info, MERROR_DB);
	  mdb->extra_info = db_info;
	}
      else
	{
	  db_info = NULL;
	  mdb->extra_info = extra_info;
	}
      mplist_push (plist, Mt, mdb);
    }
  else
    {
      mdb = MPLIST_VAL (plist);
      if (loader == load_database)
	db_info = mdb->extra_info;
      else
	db_info = NULL;
    }

  if (db_info)
    {
      db_info->status = status;
      if (! db_info->filename
	  || strcmp (db_info->filename, (char *) extra_info) != 0)
	{
	  if (db_info->filename)
	    free (db_info->filename);
	  if (db_info->absolute_filename
	      && db_info->filename != db_info->absolute_filename)
	    free (db_info->absolute_filename);
	  db_info->filename = strdup ((char *) extra_info);
	  db_info->len = strlen ((char *) extra_info);
	  db_info->time = 0;
	}
      if (db_info->filename[0] == PATH_SEPARATOR)
	db_info->absolute_filename = db_info->filename;
      else
	db_info->absolute_filename = NULL;
      M17N_OBJECT_UNREF (db_info->properties);
      if (properties)
	{
	  db_info->properties = properties;
	  M17N_OBJECT_REF (properties);
	}
    }

  if (mdb->tag[0] == Mchar_table
      && mdb->tag[2] != Mnil
      && (mdb->tag[1] == Mstring || mdb->tag[1] == Mtext
	  || mdb->tag[1] == Msymbol || mdb->tag[1] == Minteger
	  || mdb->tag[1] == Mplist))
    mchar__define_prop (mdb->tag[2], mdb->tag[1], mdb);
  return mdb;
}

static void
register_databases_in_files (MSymbol tags[4], char *filename, int len)
{
  int i, j;
  MPlist *load_key = mplist ();
  FILE *fp;
  MPlist *plist, *pl;

  MPLIST_DO (plist, mdatabase__dir_list)
    {
      glob_t globbuf;
      int headlen;

      if (filename[0] == PATH_SEPARATOR)
	{
	  if (glob (filename, GLOB_NOSORT, NULL, &globbuf))
	    break;
	  headlen = 0;
	}
      else
	{
	  MDatabaseInfo *d_info = MPLIST_VAL (plist);
	  char path[PATH_MAX + 1];

	  if (d_info->status == MDB_STATUS_DISABLED)
	    continue;
	  if (! GEN_PATH (path, d_info->filename, d_info->len, filename, len))
	    continue;
	  if (glob (path, GLOB_NOSORT, NULL, &globbuf))
	    continue;
	  headlen = d_info->len;
	}

      for (i = 0; i < globbuf.gl_pathc; i++)
	{
	  if (! (fp = fopen (globbuf.gl_pathv[i], "r")))
	    continue;
	  pl = mplist__from_file (fp, load_key);
	  fclose (fp);
	  if (! pl)
	    continue;
	  if (MPLIST_PLIST_P (pl))
	    {
	      MPlist *p;
	      MSymbol tags2[4];

	      for (j = 0, p = MPLIST_PLIST (pl); j < 4 && MPLIST_SYMBOL_P (p);
		   j++, p = MPLIST_NEXT (p))
		tags2[j] = MPLIST_SYMBOL (p);
	      for (; j < 4; j++)
		tags2[j] = Mnil;
	      for (j = 0; j < 4; j++)
		if (tags[j] != Masterisk && tags[j] != tags2[j])
		  break;
	      if (j == 4)
		register_database (tags2, load_database,
				   globbuf.gl_pathv[i] + headlen,
				   MDB_STATUS_AUTO, p);
	    }
	  M17N_OBJECT_UNREF (pl);
	}
      globfree (&globbuf);
      if (filename[0] == PATH_SEPARATOR)
	break;
    }
  M17N_OBJECT_UNREF (load_key);
}

static int
expand_wildcard_database (MPlist *plist)
{
  MDatabase *mdb;
  MDatabaseInfo *db_info;

  plist = MPLIST_NEXT (plist);
  while (MPLIST_PLIST_P (plist))
    {
      plist = MPLIST_PLIST (plist);
      plist = MPLIST_NEXT (plist);
    }
  mdb = MPLIST_VAL (plist);
  if (mdb->loader == load_database
      && (db_info = mdb->extra_info)
      && db_info->status != MDB_STATUS_DISABLED)
    {
      register_databases_in_files (mdb->tag, db_info->filename, db_info->len);
      db_info->status = MDB_STATUS_DISABLED;
      return 1;
    }
  return 0;
}


/* Internal API */

/** List of database directories.  */ 
MPlist *mdatabase__dir_list;

void *(*mdatabase__load_charset_func) (FILE *fp, MSymbol charset_name);

int
mdatabase__init ()
{
  MDatabaseInfo *dir_info;
  char *path;

  mdatabase__load_charset_func = NULL;

  Mchar_table = msymbol ("char-table");
  Mcharset = msymbol ("charset");
  Masterisk = msymbol ("*");
  Mversion = msymbol ("version");

  mdatabase__dir_list = mplist ();
  /** The macro M17NDIR specifies a directory where the system-wide
    MDB_DIR file exists.  */
  mplist_set (mdatabase__dir_list, Mt, get_dir_info (M17NDIR));

  /* The variable mdatabase_dir specifies a directory where an
     application program specific MDB_DIR file exists.  */
  if (mdatabase_dir && strlen (mdatabase_dir) > 0)
    mplist_push (mdatabase__dir_list, Mt, get_dir_info (mdatabase_dir));

  /* The environment variable M17NDIR specifies a directory where a
     user specific MDB_DIR file exists.  */
  path = getenv ("M17NDIR");
  if (path && strlen (path) > 0)
    mplist_push (mdatabase__dir_list, Mt, get_dir_info (path));
  else
    {
      /* If the env var M17NDIR is not set, check "~/.m17n.d".  */
      char *home = getenv ("HOME");
      int len;

      if (home
	  && (len = strlen (home))
	  && (path = alloca (len + 9)))
	{
	  strcpy (path, home);
	  if (path[len - 1] != PATH_SEPARATOR)
	    path[len++] = PATH_SEPARATOR;
	  strcpy (path + len, ".m17n.d");
	  dir_info = get_dir_info (path);
	  mplist_push (mdatabase__dir_list, Mt, dir_info);
	}
      else
	mplist_push (mdatabase__dir_list, Mt, get_dir_info (NULL));
    }

  mdatabase__list = mplist ();
  mdatabase__update ();
  return 0;
}

void
mdatabase__fini (void)
{
  MPlist *plist, *p0, *p1, *p2, *p3;

  MPLIST_DO (plist, mdatabase__dir_list)
    free_db_info (MPLIST_VAL (plist));
  M17N_OBJECT_UNREF (mdatabase__dir_list);

  /* MDATABASE_LIST ::= ((TAG0 (TAG1 (TAG2 (TAG3 t:MDB) ...) ...) ...) ...) */
  MPLIST_DO (plist, mdatabase__list)
    {
      p0 = MPLIST_PLIST (plist);
      /* P0 ::= (TAG0 (TAG1 (TAG2 (TAG3 t:MDB) ...) ...) ...) */
      MPLIST_DO (p0, MPLIST_NEXT (p0))
	{
	  p1 = MPLIST_PLIST (p0);
	  /* P1 ::= (TAG1 (TAG2 (TAG3 t:MDB) ...) ...) */
	  MPLIST_DO (p1, MPLIST_NEXT (p1))
	    {
	      p2 = MPLIST_PLIST (p1);
	      /* P2 ::= (TAG2 (TAG3 t:MDB) ...) */
	      MPLIST_DO (p2, MPLIST_NEXT (p2))
		{
		  MDatabase *mdb;

		  p3 = MPLIST_PLIST (p2); /* P3 ::= (TAG3 t:MDB) */
		  p3 = MPLIST_NEXT (p3);
		  mdb = MPLIST_VAL (p3);
		  if (mdb->loader == load_database)
		    free_db_info (mdb->extra_info);
		  free (mdb);
		}
	    }
	}
    }
  M17N_OBJECT_UNREF (mdatabase__list);
}

void
mdatabase__update (void)
{
  MPlist *plist, *p0, *p1, *p2, *p3;
  char path[PATH_MAX + 1];
  MDatabaseInfo *dir_info;
  struct stat statbuf;
  int rescan = 0;

  /* Update elements of mdatabase__dir_list.  */
  MPLIST_DO (plist, mdatabase__dir_list)
    {
      dir_info = MPLIST_VAL (plist);
      if (dir_info->filename)
	{
	  if (stat (dir_info->filename, &statbuf) == 0
	      && (statbuf.st_mode & S_IFDIR))
	    {
	      if (dir_info->time < statbuf.st_mtime)
		{
		  rescan = 1;
		  dir_info->time = statbuf.st_mtime;
		}
	      if (GEN_PATH (path, dir_info->filename, dir_info->len,
			    MDB_DIR, MDB_DIR_LEN)
		  && stat (path, &statbuf) >= 0
		  && dir_info->time < statbuf.st_mtime)
		{
		  rescan = 1;
		  dir_info->time = statbuf.st_mtime;
		}
	      dir_info->status = MDB_STATUS_UPDATED;
	    }
	  else
	    {
	      if (dir_info->status != MDB_STATUS_DISABLED)
		{
		  rescan = 1;
		  dir_info->time = 0;
		  dir_info->status = MDB_STATUS_DISABLED;
		}
	    }
	}
    }

  if (! rescan)
    return;

  /* At first, mark all databases defined automatically from mdb.dir
     file(s) as "disabled".  */
  MPLIST_DO (plist, mdatabase__list)
    {
      p0 = MPLIST_PLIST (plist);
      /* P0 ::= (TAG0 (TAG1 (TAG2 (TAG3 MDB) ...) ...) ...) */
      MPLIST_DO (p0, MPLIST_NEXT (p0))
	{
	  p1 = MPLIST_PLIST (p0);
	  MPLIST_DO (p1, MPLIST_NEXT (p1))
	    {
	      p2 = MPLIST_PLIST (p1);
	      MPLIST_DO (p2, MPLIST_NEXT (p2))
		{
		  MDatabase *mdb;
		  MDatabaseInfo *db_info;

		  p3 = MPLIST_PLIST (p2);
		  p3 = MPLIST_NEXT (p3);
		  mdb = MPLIST_VAL (p3);
		  db_info = mdb->extra_info;
		  if (db_info->status == MDB_STATUS_AUTO)
		    db_info->status = MDB_STATUS_DISABLED;
		}
	    }
	}
    }

  plist = mplist (); 
  MPLIST_DO (p0, mdatabase__dir_list)
    mplist_push (plist, MPLIST_KEY (p0), MPLIST_VAL (p0));

  while (! MPLIST_TAIL_P (plist))
    {
      MDatabaseInfo *dir_info = mplist_pop (plist);
      MPlist *pl, *p;
      int i;
      FILE *fp;

      if (dir_info->status == MDB_STATUS_DISABLED)
	continue;
      if (! GEN_PATH (path, dir_info->filename, dir_info->len,
		      MDB_DIR, MDB_DIR_LEN))
	continue;
      if (! (fp = fopen (path, "r")))
	continue;
      pl = mplist__from_file (fp, NULL);
      fclose (fp);
      if (! pl)
	continue;
      MPLIST_DO (p, pl)
	{
	  MSymbol tags[4];
	  MPlist *p1;
	  MText *mt;
	  int nbytes;
	  int with_wildcard = 0;

	  if (! MPLIST_PLIST_P (p))
	    continue;
	  for (i = 0, p1 = MPLIST_PLIST (p); i < 4 && MPLIST_SYMBOL_P (p1);
	       i++, p1 = MPLIST_NEXT (p1))
	    with_wildcard |= ((tags[i] = MPLIST_SYMBOL (p1)) == Masterisk);
	  if (i == 0
	      || tags[0] == Masterisk
	      || ! MPLIST_MTEXT_P (p1))
	    continue;
	  for (; i < 4; i++)
	    tags[i] = with_wildcard ? Masterisk : Mnil;
	  mt = MPLIST_MTEXT (p1);
	  nbytes = mtext_nbytes (mt);
	  if (nbytes > PATH_MAX)
	    continue;
	  memcpy (path, MTEXT_DATA (mt), nbytes);
	  path[nbytes] = '\0';
	  if (with_wildcard)
	    register_database (tags, load_database, path,
			       MDB_STATUS_AUTO_WILDCARD, NULL);
	  else
	    register_database (tags, load_database, path,
			       MDB_STATUS_AUTO, p1);
	}
      M17N_OBJECT_UNREF (pl);
    }
  M17N_OBJECT_UNREF (plist);
}

MPlist *
mdatabase__load_for_keys (MDatabase *mdb, MPlist *keys)
{
  int mdebug_flag = MDEBUG_DATABASE;
  MDatabaseInfo *db_info;
  char *filename;
  FILE *fp;
  MPlist *plist;
  char name[256];

  if (mdb->loader != load_database
      || mdb->tag[0] == Mchar_table
      || mdb->tag[0] == Mcharset)
    MERROR (MERROR_DB, NULL);
  MDEBUG_PRINT1 (" [DB]  <%s>.\n",
		 gen_database_name (name, mdb->tag));
  db_info = mdb->extra_info;
  filename = get_database_file (db_info, NULL, NULL);
  if (! filename || ! (fp = fopen (filename, "r")))
    MERROR (MERROR_DB, NULL);
  plist = mplist__from_file (fp, keys);
  fclose (fp);
  return plist;
}


/* Check if the database MDB should be reloaded or not.  It returns:

	1: The database has not been updated since it was loaded last
	time.

	0: The database has never been loaded or has been updated
	since it was loaded last time.

	-1: The database is not loadable at the moment.  */

int
mdatabase__check (MDatabase *mdb)
{
  MDatabaseInfo *db_info = (MDatabaseInfo *) mdb->extra_info;
  struct stat buf;
  int result;

  if (db_info->absolute_filename != db_info->filename
      || db_info->status == MDB_STATUS_AUTO)
    mdatabase__update ();

  if (! get_database_file (db_info, &buf, &result)
      || result < 0)
    return -1;
  if (db_info->time < buf.st_mtime)
    return 0;
  return 1;
}

/* Search directories in mdatabase__dir_list for file FILENAME.  If
   the file exist, return the absolute pathname.  If FILENAME is
   already absolute, return a copy of it.  */

char *
mdatabase__find_file (char *filename)
{
  struct stat buf;
  int result;
  MDatabaseInfo db_info;

  if (filename[0] == PATH_SEPARATOR)
    return (stat (filename, &buf) == 0 ? filename : NULL);
  db_info.filename = filename;
  db_info.len = strlen (filename);
  db_info.time = 0;
  db_info.absolute_filename = NULL;
  if (! get_database_file (&db_info, &buf, &result)
      || result < 0)
    return NULL;
  return db_info.absolute_filename;
}

char *
mdatabase__file (MDatabase *mdb)
{
  MDatabaseInfo *db_info;

  if (mdb->loader != load_database)
    return NULL;
  db_info = mdb->extra_info;
  return get_database_file (db_info, NULL, NULL);
}

int
mdatabase__lock (MDatabase *mdb)
{
  MDatabaseInfo *db_info;
  struct stat buf;
  FILE *fp;
  int len;
  char *file;

  if (mdb->loader != load_database)
    return -1;
  db_info = mdb->extra_info;
  if (db_info->lock_file)
    return -1;
  file = get_database_file (db_info, NULL, NULL);
  if (! file)
    return -1;
  len = strlen (file);
  db_info->uniq_file = malloc (len + 35);
  if (! db_info->uniq_file)
    return -1;
  db_info->lock_file = malloc (len + 5);
  if (! db_info->lock_file)
    {
      free (db_info->uniq_file);
      return -1;
    }
  sprintf (db_info->uniq_file, "%s.%X.%X", db_info->absolute_filename,
	   (unsigned) time (NULL), (unsigned) getpid ());
  sprintf (db_info->lock_file, "%s.LCK", db_info->absolute_filename);

  fp = fopen (db_info->uniq_file, "w");
  if (! fp)
    {
      char *str = strdup (db_info->uniq_file);
      char *dir = dirname (str);
      
      if (stat (dir, &buf) == 0
	  || mkdir (dir, 0777) < 0
	  || ! (fp = fopen (db_info->uniq_file, "w")))
	{
	  free (db_info->uniq_file);
	  free (db_info->lock_file);
	  db_info->lock_file = NULL;
	  free (str);
	  return -1;
	}
      free (str);
    }
  fclose (fp);
  if (link (db_info->uniq_file, db_info->lock_file) < 0
      && (stat (db_info->uniq_file, &buf) < 0
	  || buf.st_nlink != 2))
    {
      unlink (db_info->uniq_file);
      unlink (db_info->lock_file);
      free (db_info->uniq_file);
      free (db_info->lock_file);
      db_info->lock_file = NULL;
      return 0;
    }
  return 1;
}

int
mdatabase__save (MDatabase *mdb, MPlist *data)
{
  MDatabaseInfo *db_info;
  FILE *fp;
  char *file;
  MText *mt;
  int ret;

  if (mdb->loader != load_database)
    return -1;
  db_info = mdb->extra_info;
  if (! db_info->lock_file)
    return -1;
  file = get_database_file (db_info, NULL, NULL);
  if (! file)
    return -1;
  mt = mtext ();
  if (mplist__serialize (mt, data, 1) < 0)
    {
      M17N_OBJECT_UNREF (mt);
      return -1;
    }
  fp = fopen (db_info->uniq_file, "w");
  if (! fp)
    {
      M17N_OBJECT_UNREF (mt);
      return -1;
    }
  if (mt->format > MTEXT_FORMAT_UTF_8)
    mtext__adjust_format (mt, MTEXT_FORMAT_UTF_8);
  fwrite (MTEXT_DATA (mt), 1, mtext_nchars (mt), fp);
  fclose (fp);
  M17N_OBJECT_UNREF (mt);
  if ((ret = rename (db_info->uniq_file, file)) < 0)
    unlink (db_info->uniq_file);
  free (db_info->uniq_file);
  db_info->uniq_file = NULL;
  return ret;
}

int
mdatabase__unlock (MDatabase *mdb)
{
  MDatabaseInfo *db_info;

  if (mdb->loader != load_database)
    return -1;
  db_info = mdb->extra_info;
  if (! db_info->lock_file)
    return -1;
  unlink (db_info->lock_file);
  free (db_info->lock_file);
  db_info->lock_file = NULL;
  if (db_info->uniq_file)
    {
      unlink (db_info->uniq_file);
      free (db_info->uniq_file);
    }
  return 0;
}

MPlist *
mdatabase__props (MDatabase *mdb)
{
  MDatabaseInfo *db_info;

  if (mdb->loader != load_database)
    return NULL;
  db_info = mdb->extra_info;
  return db_info->properties;
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

/*** @addtogroup m17nCharset */
/*** @{ */

/***en
    @brief The symbol @c Mcharset.

    Any decoded M-text has a text property whose key is the predefined
    symbol @c Mcharset.  The name of @c Mcharset is
    <tt>"charset"</tt>.  */

/***ja
    @brief シンボル @c Mcharset.

    デコードされた M-text は、キーが @c Mcharset
    であるようなテキストプロパティを持つ。
    シンボル @c Mcharset は <tt>"charset"</tt> という名前を持つ。  */

MSymbol Mcharset;
/*=*/
/*** @} */
/*=*/

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

    アプリケーションプログラムが、そのプログラム固有のデータや m17n 
    データベースを上書きするデータを提供する場合には、マクロ M17N_INIT () 
    を呼ぶ前にこの変数をデータファイルを含むディレクトリ名にセットしなくてはならない。ディレクトリには
    "mdb.dir" ファイルをおくことができる。その"mdb.dir"ファイルには、 
    @ref mdbDir "mdbDir(5)" で説明されているフォーマットでデータ定義のリストを記述する。

    デフォルトの値は NULL である。  */

char *mdatabase_dir;

/*=*/
/***en
    @brief Look for a data in the database.

    The mdatabase_find () function searches the m17n database for a
    data who has tags $TAG0 through $TAG3, and returns a pointer to
    the data.  If such a data is not found, it returns @c NULL.  */

/***ja
    @brief データベース中のデータを探す.

    関数 mdatabase_find () は、 m17n 言語情報ベース中で $TAG0 から 
    $TAG3 までのタグを持つデータを探し、それへのポインタを返す。そのようなデータがなければ
    @c NULL を返す。

    @latexonly \IPAlabel{mdatabase_find} @endlatexonly  */

MDatabase *
mdatabase_find (MSymbol tag0, MSymbol tag1, MSymbol tag2, MSymbol tag3)
{
  MSymbol tags[4];

  mdatabase__update ();
  tags[0] = tag0, tags[1] = tag1, tags[2] = tag2, tags[3] = tag3;
  return find_database (tags);
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

    関数 mdatabase_list () は m17n データベース中から $TAG0 から$TAG3 
    までのタグを持つデータを探し、そのリストをplist として返す。 $TAGn が #Mnil
    であった場合には、任意のタグにマッチするワイルドカードとして取り扱われる。返される
    plist の各要素はキー として #Mt を、値として #MDatabase 型へのポインタを持つ。  */

MPlist *
mdatabase_list (MSymbol tag0, MSymbol tag1, MSymbol tag2, MSymbol tag3)
{
  MPlist *plist = mplist (), *pl = plist;
  MPlist *p, *p0, *p1, *p2, *p3;

  mdatabase__update ();

  MPLIST_DO (p, mdatabase__list)
    {
      p0 = MPLIST_PLIST (p);
      /* P0 ::= (TAG0 (TAG1 (TAG2 (TAG3 MDB) ...) ...) ...) */
      if (MPLIST_SYMBOL (p0) == Masterisk
	  || (tag0 != Mnil && MPLIST_SYMBOL (p0) != tag0))
	continue;
      MPLIST_DO (p0, MPLIST_NEXT (p0))
	{
	  p1 = MPLIST_PLIST (p0);
	  if (MPLIST_SYMBOL (p1) == Masterisk)
	    {
	      if (expand_wildcard_database (p1))
		{
		  M17N_OBJECT_UNREF (plist);
		  return mdatabase_list (tag0, tag1, tag2, tag3);
		}
	      continue;
	    }
	  if (tag1 != Mnil && MPLIST_SYMBOL (p1) != tag1)
	    continue;
	  MPLIST_DO (p1, MPLIST_NEXT (p1))
	    {
	      p2 = MPLIST_PLIST (p1);
	      if (MPLIST_SYMBOL (p2) == Masterisk)
		{
		  if (expand_wildcard_database (p2))
		    {
		      M17N_OBJECT_UNREF (plist);
		      return mdatabase_list (tag0, tag1, tag2, tag3);
		    }
		  continue;
		}
	      if (tag2 != Mnil && MPLIST_SYMBOL (p2) != tag2)
		continue;
	      MPLIST_DO (p2, MPLIST_NEXT (p2))
		{
		  p3 = MPLIST_PLIST (p2);
		  if (MPLIST_SYMBOL (p3) == Masterisk)
		    {
		      if (expand_wildcard_database (p3))
			{
			  M17N_OBJECT_UNREF (plist);
			  return mdatabase_list (tag0, tag1, tag2, tag3);
			}
		      continue;
		    }
		  if (tag3 != Mnil && MPLIST_SYMBOL (p3) != tag3)
		    continue;
		  p3 = MPLIST_NEXT (p3);
		  pl = mplist_add (pl, Mt, MPLIST_VAL (p3));
		}
	    }
	}
    }
  if (MPLIST_TAIL_P (plist))
    M17N_OBJECT_UNREF (plist);
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

    関数 mdatabase_define () は $TAG0 から $TAG3 までのタグおよび付加情報 
    $EXTRA_INFO を持つデータを定義する。

    $LOADER はそのデータのロードに用いられる関数へのポインタである。この関数は
    mdatabase_load () から $TAGS と $EXTRA_INFO という二つの引数付きで呼び出される。ここで 
    $TAGS は $TAG0 から $TAG3 までの配列である。

    もし $LOADER が @c NULL なら、m17n ライブラリ標準のローダが使われる。この場合には
    $EXTRA_INFO はデータを含むファイル名でなくてはならない。

    @return
    処理に成功すれば mdatabase_define () 
    は定義されたデータベースへのポインタを返す。このポインタは関数 mdatabase_load () 
    の引数として用いることができる。そうでなければ @c NULL を返す。

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
  MSymbol tags[4];

  tags[0] = tag0, tags[1] = tag1, tags[2] = tag2, tags[3] = tag3;
  if (! loader)
    loader = load_database;
  mdb = register_database (tags, loader, extra_info, MDB_STATUS_EXPLICIT, NULL);
  return mdb;
}

/*=*/
/***en
    @brief Load a data from the database.

    The mdatabase_load () function loads a data specified in $MDB and
    returns the contents.  The type of contents depends on the type of
    the data.

    If the data is of the @e plist @e type, this function returns a
    pointer to @e plist.

    If the database is of the @e chartable @e type, it returns a
    chartable.  The default value of the chartable is set according to
    the second tag of the data as below:

    @li If the tag is #Msymbol, the default value is #Mnil.
    @li If the tag is #Minteger, the default value is -1.
    @li Otherwise, the default value is @c NULL.

    If the data is of the @e charset @e type, it returns a plist of length 2
    (keys are both #Mt).  The value of the first element is an array
    of integers that maps code points to the corresponding character
    codes.  The value of the second element is a chartable of integers
    that does the reverse mapping.  The charset must be defined in
    advance.  */


/***ja
    @brief データベースからデータをロードする.

    関数 mdatabase_load () は $MDB 
    が指すデータをロードし、その中身を返す。返されるものはデータのタイプによって異なる。

    データが @e plistタイプ ならば、 @e plist へのポインタを返す。

    データが @e chartableタイプ ならば文字テーブルを返す。
    文字テーブルのデフォルト値は、データの第2タグによって以下のように決まる。

    @li タグが #Msymbol なら、デフォルト値は #Mnil
    @li タグが #Minteger なら、デフォルト値は -1
    @li それ以外なら、デフォルト値は @c NULL

    データが @e charsetタイプ ならば長さ 2 の plist を返す（キーは共に#Mt ）。
    最初の要素の値はコードポイントを対応する文字コードにマップする整数の配列である。
    ２番目の要素の値は逆のマップをする文字テーブルである。
    この文字セットは予め定義されていなければならない。

    @latexonly \IPAlabel{mdatabase_load} @endlatexonly
  */

/***
    @seealso
    mdatabase_load (),  mdatabase_define ()  */

void *
mdatabase_load (MDatabase *mdb)
{
  return (*mdb->loader) (mdb->tag, mdb->extra_info);
}

/*=*/
/***en
    @brief Get tags of a data.

    The mdatabase_tag () function returns an array of tags (symbols)
    that identify the data in $MDB.  The length of the array is
    four.  */

/***ja
    @brief データのタグを得る.

    関数 mdatabase_tag () は、データ $MDB のタグ（シンボル）の配列を返す。配列の長さは
    4 である。

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
