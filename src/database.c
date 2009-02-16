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
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <glob.h>
#include <time.h>
#include <libgen.h>

#include <libxml/xmlreader.h>

#include "m17n-core.h"
#include "m17n-misc.h"
#include "internal.h"
#include "mtext.h"
#include "character.h"
#include "database.h"
#include "plist.h"

/** The file containing a list of databases.  */
static MText *mdb_xml, *mdb_dir, *mdb_rng;
static MText *work;

/* Return a binary mtext for the filename NAME.  */
#define MTEXT_FOR_FILE(name)	\
  mtext__from_data ((name), strlen ((char *) name), MTEXT_FORMAT_BINARY, 1)

/* Return a pointer (char *) to the text data of M-text MT. */
#define MTEXT_STR(mt) ((char *) MTEXT_DATA (mt))

/* Return the later time.  */
#define MAX_TIME(TIME1, TIME2) ((TIME1) >= (TIME2) ? (TIME1) : (TIME2))

#define GEN_PATH(dir, file)	\
  (mtext_cpy (work, dir), mtext_cat (work, file))

#define ABSOLUTE_PATH_P(file)	\
  (MTEXT_STR (file)[0] == PATH_SEPARATOR)

static MSymbol Mfilename, Mformat, Mvalidater;
static MSymbol Mxml, Mdtd, Mxml_schema, Mrelaxng, Mschematron;
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
      is load_database (), the value is a pointer to MDatabaseInfo.  */
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
	    mt = mtext__from_data (buf + i, len - i - 1,
				   MTEXT_FORMAT_UTF_8, 1);
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

static MText *
file_readble_p (MText *dir, MText *file, struct stat *statbuf)
{
  if (file)
    {
      char *p;
      int fd;

      if (! ABSOLUTE_PATH_P (file))
	file = GEN_PATH (dir, file);
      p = MTEXT_STR (file);
      if (stat (p, statbuf) == 0
	  && (fd = open (p, O_RDONLY)) >= 0)
	{
	  close (fd);
	  return file;
	}
    }
  else
    {
      DIR *directory;

      if (stat (MTEXT_STR (dir), statbuf) == 0
	  && (directory = opendir (MTEXT_STR (dir))))
	{
	  closedir (directory);
	  return dir;
	}
    }
  return NULL;
}


static MDatabaseInfo *
find_database_dir_info (MText *filename)
{
  MPlist *plist;

  MPLIST_DO (plist, mdatabase__dir_list)
    {
      MDatabaseInfo *dir_info = MPLIST_VAL (plist);;
      struct stat statbuf;

      if (dir_info->status != MDB_STATUS_DISABLED
	  && file_readble_p (dir_info->dirname, filename, &statbuf))
	return dir_info;
    }
  return NULL;
}


/* Return the absolute file for the database DB_INFO, and update
   DB_INFO->dirname, DB_INFO->filename, and DB_INFO->mtime.  If
   DB_INFO is NULL, check FILENAME instead.  Caller must NOT unref the
   returned filename.

   If no absolute file name was found, return NULL.  */

MText *
get_database_file (MDatabaseInfo *db_info, MText *filename)
{
  struct stat statbuf;
  MText *dirname = NULL, *path = NULL;
  int system_database = 0;

  if (db_info)
    filename = db_info->filename;
  if (ABSOLUTE_PATH_P (filename))
    {
      path = file_readble_p (NULL, filename, &statbuf);
      system_database
	= strncmp (M17NDIR, MTEXT_STR (filename), strlen (M17NDIR)) == 0;
    }
  else
    {
      MDatabaseInfo *dir_info = find_database_dir_info (filename);

      if (dir_info)
	{
	  dirname = dir_info->dirname;
	  system_database = dir_info->system_database;
	  path = GEN_PATH (dirname, filename);
	}
    }

  if (! db_info)
    return path;

  if ((path == NULL) == (db_info->status == MDB_STATUS_DISABLED)
      && db_info->dirname != dirname)
    {
      db_info->system_database = system_database;
      M17N_OBJECT_UNREF (db_info->dirname);
      if (dirname)
	{
	  db_info->dirname = dirname;
	  M17N_OBJECT_REF (dirname);
	}
      db_info->status = MDB_STATUS_OUTDATED;
    }
  else
    {
      db_info->status = MDB_STATUS_DISABLED;
    }
  return path;
}


/* List of XML loaders
   ((TAG0 [func|(TAG1 [func|(TAG2 [func|(TAG3 func) ... ]) ... ]) ... ]) ...) */
static MPlist *xml_loader_list;

static MDatabaseLoaderXML
find_xml_loader (MSymbol tags[4])
{
  MPlist *plist;
  int i;

  for (i = 0, plist = xml_loader_list; i < 4 && tags[i] != Mnil; i++)
    {
      MPlist *pl = mplist__assq (plist, tags[i]);
      
      if (! pl)
	return NULL;
      pl = MPLIST_PLIST (pl);
      plist = MPLIST_NEXT (pl);
      if (MPLIST_VAL_FUNC_P (plist))
	return ((MDatabaseLoaderXML) MPLIST_FUNC (plist));
    }
  return NULL;
}

static int xml_validate (MDatabaseInfo *);

static void *
load_database (MSymbol *tags, void *extra_info)
{
  MDatabaseInfo *db_info = extra_info;
  void *value;
  MText *path = get_database_file (db_info, NULL);
  FILE *fp;
  int mdebug_flag = MDEBUG_DATABASE;
  char buf[256];

  MDEBUG_PRINT1 (" [DB] <%s>", gen_database_name (buf, tags));
  if (! path || ! (fp = fopen (MTEXT_STR (path), "r")))
    {
      if (path)
	MDEBUG_PRINT1 (" open fail: %s\n", MTEXT_STR (path));
      else
	MDEBUG_PRINT1 (" not found: %s\n", MTEXT_STR (db_info->filename));
      db_info->status = MDB_STATUS_DISABLED;
      db_info->time = 0;
      MERROR (MERROR_DB, NULL);
    }

#if 0
  if (db_info->status != MDB_STATUS_UPDATED)
    {
      if (! xml_validate (db_info))
	{
	  db_info->status = MDB_STATUS_INVALID;
	  MDEBUG_PRINT1 (" fail to validate: %s", MTEXT_STR (path));
	  MERROR (MERROR_DB, NULL);
	}
      db_info->status = MDB_STATUS_UPDATED;
    }
#endif
  MDEBUG_PRINT1 (" from %s\n", MTEXT_STR (path));

  if (tags[0] == Mchar_table)
    value = load_chartable (fp, tags[1]);
  else if (tags[0] == Mcharset)
    {
      if (! mdatabase__load_charset_func)
	MERROR (MERROR_DB, NULL);
      value = (*mdatabase__load_charset_func) (fp, tags[1]);
    }
  else
    {
      int c;
      unsigned char buf[4];

      if (fgets (buf, 4, fp) == NULL)
	c = EOF;
      else
	{
	  /* Skip BOM of UTF-8.  */
	  if ((c = buf[0]) != 0xEF)
	    fseek (fp, 0L, SEEK_SET);
	  else if (buf[1] != 0xBB)
	    fseek (fp, 0L, SEEK_SET);
	  else if (buf[2] != 0xBF)
	    fseek (fp, 0L, SEEK_SET);
	}
      if (c == '<')
	{
	  MDatabaseLoaderXML loader = find_xml_loader (tags);

	  if (! loader)
	    MERROR (MERROR_DB, NULL);
	  fclose (fp);
	  fp = NULL;
	  value = loader (db_info, MTEXT_STR (path));
	}
      else if (c != EOF)
	{
	  value = mplist__from_file (fp, NULL);
	}
      else
	value = NULL;
    }
  if (fp)
    fclose (fp);

  if (! value)
    MERROR (MERROR_DB, NULL);
  db_info->time = time (NULL);
  return value;
}


/** Return a newly allocated MDatabaseInfo for DIRNAME which is a
    directory containing the database files.  SYSTEM_DATABSE is 1 iff
    the direcoty is for the system wide directory
    (e.g. /usr/share/m17n).  */

static MDatabaseInfo *
get_dir_info (char *dirname, int system_database)
{
  MDatabaseInfo *dir_info;

  MSTRUCT_CALLOC (dir_info, MERROR_DB);
  dir_info->system_database = system_database;
  dir_info->dirname = MTEXT_FOR_FILE (dirname);
  if (dirname[mtext_nbytes (dir_info->dirname) - 1] != PATH_SEPARATOR)
    mtext_cat_char (dir_info->dirname, PATH_SEPARATOR);
  dir_info->status = MDB_STATUS_OUTDATED;
  return dir_info;
}

static int
update_dir_info (MDatabaseInfo *dir_info)
{
  struct stat statbuf;
  MText *mdb_file;

  if (! file_readble_p (dir_info->dirname, NULL, &statbuf))
    {
      if (dir_info->status == MDB_STATUS_DISABLED)
	return 0;
      dir_info->status = MDB_STATUS_DISABLED;
      dir_info->filename = NULL;
      dir_info->mtime = dir_info->time = 0;
      return 1;
    }

  for (mdb_file = mdb_xml; mdb_file;
       mdb_file = (mdb_file == mdb_dir ? NULL : mdb_dir))
    if (file_readble_p (dir_info->dirname, mdb_file, &statbuf))
      {
	if (dir_info->filename == mdb_file
	    && dir_info->time >= statbuf.st_mtime)
	  return 0;
	dir_info->status = MDB_STATUS_OUTDATED;
	M17N_OBJECT_UNREF (dir_info->filename);
	dir_info->filename = mdb_file;
	M17N_OBJECT_REF (mdb_file);
	dir_info->format = mdb_file == mdb_xml ? Mxml : Mplist;
	dir_info->time = dir_info->mtime = statbuf.st_mtime;
	return 1;
      }
  dir_info->status = MDB_STATUS_INVALID;
  if (! dir_info->filename)
    return 0;
  /* The directory is readable but doesn't have mdb.xml nor
     mdb.dir.  */
  dir_info->filename = NULL;
  dir_info->mtime = dir_info->time = 0;
  return 1;
}


static void register_databases_in_files (MSymbol tags[4],
					 MDatabaseInfo *db_info);

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
	  if (db_info->status == MDB_STATUS_OUTDATED)
	    {
	      register_databases_in_files (mdb->tag, db_info);
	      db_info->status = MDB_STATUS_UPDATED;
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
  M17N_OBJECT_UNREF (db_info->dirname);
  M17N_OBJECT_UNREF (db_info->filename);
  M17N_OBJECT_UNREF (db_info->validater);
  M17N_OBJECT_UNREF (db_info->properties);
  free (db_info);
}

static int
check_version (MPlist *version)
{
  char *verstr;
  char *endp;
  int ver[3];
  int i;

  if (! MPLIST_MTEXT_P (version))
    return 0;
  verstr = MTEXT_STR (MPLIST_MTEXT (version));
  endp = verstr + mtext_nbytes (MPLIST_MTEXT (version));
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

/* If LOADER == load_database, extra_info is a pointer to MDatabaseInfo.  */

static MDatabase *
register_database (MSymbol tags[4],
		   void *(*loader) (MSymbol *, void *),
		   void *extra_info, enum MDatabaseType type)
{
  MDatabase *mdb;
  MDatabaseInfo *db_info;
  int i;
  MPlist *plist;

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
      MSTRUCT_CALLOC (mdb, MERROR_DB);
      for (i = 0; i < 4; i++)
	mdb->tag[i] = tags[i];
      mdb->loader = loader;
      mplist_push (plist, Mt, mdb);
    }
  else
    {
      mdb = MPLIST_VAL (plist);
    }

  if (loader == load_database)
    {
      if (mdb->extra_info)
	{
	  db_info = mdb->extra_info;
	  M17N_OBJECT_UNREF (db_info->filename);
	  M17N_OBJECT_UNREF (db_info->validater);
	  M17N_OBJECT_UNREF (db_info->properties);
	}
      else
	{
	  MSTRUCT_CALLOC (db_info, MERROR_DB);
	  mdb->extra_info = db_info;
	}
      *db_info = *(MDatabaseInfo *) extra_info;
      db_info->type = type;
      db_info->status = MDB_STATUS_OUTDATED;
      if (db_info->filename)
	M17N_OBJECT_REF (db_info->filename);
      if (db_info->validater)
	M17N_OBJECT_REF (db_info->validater);	
    }
  else
    {
      mdb->extra_info = extra_info;
    }

  if (mdb->tag[0] == Mchar_table
      && mdb->tag[2] != Mnil
      && (mdb->tag[1] == Mstring || mdb->tag[1] == Mtext
	  || mdb->tag[1] == Msymbol || mdb->tag[1] == Minteger
	  || mdb->tag[1] == Mplist))
    mchar__define_prop (mdb->tag[2], mdb->tag[1], mdb);
  return mdb;
}

#define STR_EQ(s1, s2) strcmp ((char *) s1, (char *) s2) == 0

static MPlist *
parse_mdb_xml_item (xmlNodePtr node)
{
  MPlist *plist, *pl, *p;
  MSymbol tags[4];
  xmlAttrPtr attr = node->properties;
  xmlNodePtr cur;
  int i;

  plist = pl = mplist ();
  for (i = 0; i < 4; i++)
    tags[i] = Mnil;
  while (attr)
    {
      if (STR_EQ (attr->name, "key0"))
	tags[0] = msymbol ((char *) attr->children->content);
      else if (STR_EQ (attr->name, "key1"))
	tags[1] = msymbol ((char *) attr->children->content);
      else if (STR_EQ (attr->name, "key2"))
	tags[2] = msymbol ((char *) attr->children->content);
      else if (STR_EQ (attr->name, "key3"))
	tags[3] = msymbol ((char *) attr->children->content);
      attr = attr->next;
    }
  for (i = 0; i < 4; i++)
    pl = mplist_add (pl, Msymbol, tags[i]);

  for (node = node->children; node; node = node->next)
    {
      if (STR_EQ (node->name, "source"))
	{
	  MText *filename = NULL, *schema_file = NULL;
	  MSymbol format = Mnil, schema = Mnil;
	  char *content;

	  for (cur = node->children; cur; cur = cur->next)
	    {
	      if (cur->type != XML_ELEMENT_NODE)
		continue;
	      content = (char *) cur->children->content;
	      if (STR_EQ (cur->name, "filename"))
		filename = mtext__from_data (content, strlen (content),
					     MTEXT_FORMAT_UTF_8, 1);
	      else if (STR_EQ (cur->name, "format"))
		format = msymbol (content);
	      else if (STR_EQ (cur->name, "schema"))
		{
		  attr = xmlHasProp (cur, "type");
		  schema = msymbol ((char *) attr->children->content);
		  content = cur->children->children->content;
		  schema_file = mtext__from_data (content, strlen (content),
						  MTEXT_FORMAT_UTF_8, 1);
		}
	    }
	  if (format == Mnil)
	    pl = mplist_add (pl, Mtext, filename);
	  else
	    {
	      p = mplist ();
	      pl = mplist_add (pl, Mplist, p);
	      M17N_OBJECT_UNREF (p);
	      p = mplist_add (p, Mtext, filename);
	      p = mplist_add (p, Msymbol, format);
	      p = mplist_add (p, Msymbol, schema);
	      if (schema_file)
		{
		  mplist_add (p, Mtext, schema_file);
		  M17N_OBJECT_UNREF (schema_file);
		}
	    }
	  M17N_OBJECT_UNREF (filename);
	}
      else if (STR_EQ (node->name, "properties"))
	{
	  for (cur = node->children; cur; cur = cur->next)
	    {
	      if (cur->type != XML_ELEMENT_NODE)
		continue;
	      if (STR_EQ (cur->name, "font"))
		{
		  MPlist *p = mplist ();

		  for (attr = cur->properties; attr; attr = attr->next)
		    mplist_add (p, Msymbol, msymbol ((char *)
						     attr->children->content));
		}
	    }
	}
    }
  return plist;
}

static MPlist *
parse_mdb_xml (char *filename, int need_validation)
{
  xmlDocPtr doc;
  xmlNodePtr node;
  MPlist *plist, *pl, *p;

  doc = xmlReadFile (filename, "UTF-8", (XML_PARSE_NOBLANKS | XML_PARSE_NOENT
					 | XML_PARSE_XINCLUDE));

  if (need_validation)
    {
      xmlRelaxNGParserCtxtPtr context = NULL;
      xmlRelaxNGPtr schema = NULL;
      xmlRelaxNGValidCtxtPtr valid_context = NULL;
      MText *path;
      int result;

      path = get_database_file (NULL, mdb_rng);
      if (! path)
	{
	  xmlFreeDoc (doc);
	  return NULL;
	}
      result = ((context = xmlRelaxNGNewParserCtxt (MTEXT_STR (path)))
		&& (schema = xmlRelaxNGParse (context))
		&& (valid_context = xmlRelaxNGNewValidCtxt (schema))
		&& xmlRelaxNGValidateDoc (valid_context, doc) == 0);
      if (valid_context)
	xmlRelaxNGFreeValidCtxt (valid_context);
      if (schema)
	xmlRelaxNGFree (schema);
      if (context)
	xmlRelaxNGFreeParserCtxt (context);
      if (! result)
	{
	  xmlFreeDoc (doc);
	  return NULL;
	}
    }
  plist = pl = mplist ();
  for (node = xmlDocGetRootElement (doc)->children; node; node = node->next)
    {
      if (node->type != XML_ELEMENT_NODE)
	continue;
      p = parse_mdb_xml_item (node);
      pl = mplist_add (pl, Mplist, p);
      M17N_OBJECT_UNREF (p);
    }      
  xmlFreeDoc (doc);
  return plist;
}

static MPlist *
parse_mdb_dir (char *filename)
{
  FILE *fp;
  MPlist *plist;

  fp = fopen (filename, "r");
  if (! fp)
    return NULL;
  plist = mplist__from_file (fp, NULL);
  fclose (fp);
  return plist;
}

static int
xml_validate (MDatabaseInfo *db_info)
{
  xmlDocPtr doc = NULL;
  MText *path = NULL;
  char *file;
  int result;

  if (db_info->schema == Mnil)
    return 1;

  path = get_database_file (db_info, NULL);
  if (! path)
    MERROR (MERROR_DB, 0);
  file = MTEXT_STR (path);
  doc = xmlReadFile (file, NULL, (XML_PARSE_DTDLOAD | XML_PARSE_DTDVALID
				  | XML_PARSE_XINCLUDE | XML_PARSE_COMPACT));
  if (! doc)
    MERROR (MERROR_DB, 0);
  result = mdatabase__validate (doc, db_info);
  xmlFreeDoc (doc);
  return result;
}

static int
parse_header_xml (xmlTextReaderPtr reader, MSymbol tags[4],
		  MDatabaseInfo *db_info)
{
  MSymbol sym;
  MText *mt;
  const xmlChar *name, *value;
  int depth;
  int i;
  int with_wildcard = 0;

  /* Read TAG0..3 (TAG1..3 are optional) from XML of this format:
     <TAG0 ...>
       <tags>
         <XXX>TAG1</XXX>
	 <YYY>TAG2</YYY> 
	 <ZZZ>TAG3</ZZZ>
       </tags>
       ...
     </TAG0> */
  xmlTextReaderRead (reader);	/* <TAG0 ...> */
  tags[0] = msymbol ((char *) xmlTextReaderConstLocalName (reader));
  xmlTextReaderRead (reader);	/* <tags> */
  depth = xmlTextReaderDepth (reader);
  for (i = 1;
       xmlTextReaderRead (reader) && depth < xmlTextReaderDepth (reader);
       i++)
    {
      xmlTextReaderRead (reader);
      tags[i] = msymbol ((char *) xmlTextReaderConstValue (reader));
      with_wildcard |= (tags[i] == Masterisk);
      xmlTextReaderRead (reader);
    }
  for (; i < 4; i++)
    tags[i] = with_wildcard ? Masterisk : Mnil;
  db_info->type = with_wildcard ? MDB_TYPE_AUTO_WILDCARD : MDB_TYPE_AUTO;
  xmlTextReaderRead (reader);
  if (strcmp ((char *) xmlTextReaderConstLocalName (reader), "source") == 0)
    {
      /* Read a file specification from XML of this format:
	 <source>
	   [ <filename>FILENAME</filename> ]
	   [ <format>FORMAT</format> ]
	   [ <schema>SCHEMA</schema> ]
	   [ <validater>VALIDATER</validater> ]
	 </source>  */
      depth = xmlTextReaderDepth (reader);
      while (xmlTextReaderRead (reader) && depth < xmlTextReaderDepth (reader))
	{
	  name = xmlTextReaderConstLocalName (reader);
	  xmlTextReaderRead (reader);
	  value = xmlTextReaderConstValue (reader);
	  if (strcmp ((char *) name, "filename") == 0)
	    db_info->filename = MTEXT_FOR_FILE (value);
	  else if (strcmp ((char *) name, "format") == 0)
	    db_info->format = msymbol ((char *) value);
	  else if (strcmp ((char *) name, "schema") == 0)
	    db_info->schema = msymbol ((char *) value);
	  else if (strcmp ((char *) name, "validater") == 0)
	    db_info->validater = MTEXT_FOR_FILE (value);
	  xmlTextReaderRead (reader);
	}
    }
  if (strcmp ((char *) xmlTextReaderConstLocalName (reader), "properties") == 0)
    {
      /* Read properties from XML of this format:
	 <properties>
	   <PROP>VAL</PROP>
	   ...
	 </properties>  */
      MPlist *plist = mplist (), *pl; 

      db_info->properties = plist;
      depth = xmlTextReaderDepth (reader);
      while (xmlTextReaderRead (reader) && depth < xmlTextReaderDepth (reader))
	{
	  name = xmlTextReaderConstLocalName (reader);
	  sym = msymbol ((char *) name);
	  xmlTextReaderRead (reader);
	  value = xmlTextReaderConstValue (reader);
	  mt = MTEXT_FOR_FILE (value);
	  pl = mplist ();
	  plist = mplist_add (plist, Mplist, pl);
	  M17N_OBJECT_UNREF (pl);
	  mplist_add (pl, Msymbol, sym);
	  mplist_add (pl, Mtext, mt);
	  M17N_OBJECT_UNREF (mt);
	  xmlTextReaderRead (reader);
	}
    }
  return 1;
}


static int
parse_database_info (MPlist *plist, MSymbol tags[4], MDatabaseInfo *db_info)
{
  int with_wildcard = 0;
  int i;

  if (! MPLIST_PLIST_P (plist))
    MERROR (MERROR_DB, 0);
  plist = MPLIST_PLIST (plist);
  for (i = 0; i < 4 && MPLIST_SYMBOL_P (plist);
       i++, plist = MPLIST_NEXT (plist))
    {
      tags[i] = MPLIST_SYMBOL (plist);
      with_wildcard |= (tags[i] == Masterisk);
    }
  if (i == 0)
    MERROR (MERROR_DB, 0);
  for (; i < 4; i++)
    tags[i] = with_wildcard ? Masterisk : Mnil;
  memset (db_info, 0, sizeof (MDatabaseInfo));
  db_info->type = with_wildcard ? MDB_TYPE_AUTO_WILDCARD : MDB_TYPE_AUTO;
  if (MPLIST_MTEXT_P (plist))
    {
      db_info->filename = MPLIST_MTEXT (plist);
      M17N_OBJECT_REF (db_info->filename);
      plist = MPLIST_NEXT (plist);
    }
  else if (MPLIST_PLIST_P (plist))
    {
      MPlist *pl = MPLIST_PLIST (plist);

      /* If PL == (FILENAME ...) || (nil ...), this is a source
	 specification.  */
      if (MPLIST_MTEXT_P (pl)
	  || (MPLIST_SYMBOL_P (pl) && MPLIST_SYMBOL (pl) == Mnil))
	{
	  if (MPLIST_MTEXT_P (pl))
	    {
	      db_info->filename = MPLIST_MTEXT (pl);
	      M17N_OBJECT_REF (db_info->filename);
	    }
	  pl = MPLIST_NEXT (pl);
	  if (! MPLIST_SYMBOL_P (pl))
	    {
	      M17N_OBJECT_UNREF (db_info->filename);
	      MERROR (MERROR_DB, 0);
	    }
	  db_info->format = MPLIST_SYMBOL (pl);
	  if (db_info->format == Mxml)
	    {
	      pl = MPLIST_NEXT (pl);
	      if (MPLIST_SYMBOL_P (pl))
		{
		  db_info->schema = MPLIST_SYMBOL (pl);
		  pl = MPLIST_NEXT (pl);		  
		  if (MPLIST_MTEXT_P (pl))
		    {
		      db_info->validater = MPLIST_MTEXT (pl);
		      M17N_OBJECT_REF (db_info->validater);
		    }
		}
	    }
	  plist = MPLIST_NEXT (plist);
	}
    }
  
  if (! MPLIST_TAIL_P (plist))
    {
      if (! db_info->properties)
	db_info->properties = mplist ();
      MPLIST_DO (plist, plist)
	if (MPLIST_PLIST_P (plist))
	  {
	    MPlist *p = MPLIST_PLIST (plist);

	    if (MPLIST_SYMBOL_P (p)
		&& MPLIST_SYMBOL (p) == Mversion
		&& ! check_version (MPLIST_NEXT (p)))
	      continue;
	    mplist_put (db_info->properties, Mplist, p);
	  }
    }
  return 1;
}


static int
parse_header_sexp (char *filename, MSymbol tags[4], MDatabaseInfo *db_info)
{
  MPlist *load_key, *plist;
  FILE *fp;

  if (! (fp = fopen (filename, "r")))
    MERROR (MERROR_DB, 0);
  load_key = mplist ();
  plist = mplist__from_file (fp, load_key);
  fclose (fp);
  M17N_OBJECT_UNREF (load_key);
  if (! parse_database_info (plist, tags, db_info))
    {
      M17N_OBJECT_UNREF (plist);
      MERROR (MERROR_DB, 0);
    }
  if (db_info->filename)
    MERROR (MERROR_DB, 0);
  db_info->filename = MTEXT_FOR_FILE (filename);
  M17N_OBJECT_UNREF (plist);
  return 1;
}

static int
merge_info (MSymbol tags1[4], MDatabaseInfo *info1,
	    MSymbol tags2[4], MDatabaseInfo *info2)
{
  int i;

  for (i = 0; i < 4; i++)
    if (tags1[i] && tags1[i] != Masterisk && tags1[i] != tags2[i])
      goto err;
  if (info2->format == Mnil)
    info2->format = info1->format;
  else if (info1->format != info2->format)
    goto err;
  if (info2->schema == Mnil)
    info2->schema = info1->schema;
  else if (info1->schema != info2->schema)
    goto err;
  if (! info2->filename)
    {
      info2->filename = info1->filename;
      M17N_OBJECT_REF (info2->filename);
    }
  if (! info2->validater && info1->validater)
    {
      info2->validater = info1->validater;
      M17N_OBJECT_REF (info2->validater);
    }
  return 1;

 err:
  if (info2->filename)
    M17N_OBJECT_UNREF (info2->filename);
  if (info2->validater)
    M17N_OBJECT_UNREF (info2->validater);
  return 0;
}

static void
register_databases_in_files (MSymbol tags[4], MDatabaseInfo *db_info)
{
  int check[3];
  glob_t globbuf[3];
  MText *dirname[3];
  int i, j;
  MPlist *plist;

  check[0] = check[1] = check[2] = 0;

  if (ABSOLUTE_PATH_P (db_info->filename))
    {
      if (glob (MTEXT_STR (db_info->filename), GLOB_NOSORT, NULL, globbuf) == 0)
	{
	  check[0] = 1;
	  dirname[0] = NULL;
	}
    }
  else
    {
      MDatabaseInfo *dir_info;

      j = 2;
      MPLIST_DO (plist, mdatabase__dir_list)
	{
	  MText *path;

	  dir_info = MPLIST_VAL (plist);
	  if (dir_info->status == MDB_STATUS_DISABLED)
	    continue;
	  path = GEN_PATH (dir_info->dirname, db_info->filename);
	  if (glob (MTEXT_STR (path), GLOB_NOSORT, NULL, globbuf + j) == 0)
	    {
	      check[j] = 1;
	      dirname[j] = dir_info->dirname;
	    }
	  j--;
	}
    }

  for (j = 0; j < 3; j++)
    if (check[j])
      {
	for (i = 0; i < globbuf[j].gl_pathc; i++)
	  {
	    MDatabaseInfo this;
	    MSymbol tags2[4];	  

	    memset (&this, 0, sizeof (MDatabaseInfo));
	    if (dirname[j])
	      this.filename = MTEXT_FOR_FILE (globbuf[j].gl_pathv[i]
					      + mtext_nbytes (dirname[j]));
	    else
	      this.filename = MTEXT_FOR_FILE (globbuf[j].gl_pathv[i]);
	    if (db_info->format == Mxml)
	      {
		xmlTextReaderPtr reader
		  = xmlReaderForFile (globbuf[j].gl_pathv[i], "utf-8",
				      XML_PARSE_NOBLANKS
				      | XML_PARSE_NOENT
				      | XML_PARSE_XINCLUDE);
		if (reader
		    && parse_header_xml (reader, tags2, &this)
		    && merge_info (tags, db_info, tags2, &this))
		  register_database (tags2, load_database, &this,
				     MDB_TYPE_AUTO);
	      }
	    else if (parse_header_sexp (globbuf[j].gl_pathv[i], tags2, &this)
		     && merge_info (tags, db_info, tags2, &this))
	      register_database (tags2, load_database, &this, MDB_TYPE_AUTO);
	    M17N_OBJECT_UNREF (this.filename);
	    M17N_OBJECT_UNREF (this.validater);
	  }
	globfree (globbuf + j);
      }
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
      && db_info->status != MDB_STATUS_UPDATED)
    {
      register_databases_in_files (mdb->tag, db_info);
      db_info->status = MDB_STATUS_UPDATED;
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
  char *path;

  mdatabase__load_charset_func = NULL;

  Mchar_table = msymbol ("char-table");
  Mcharset = msymbol ("charset");
  Mfilename = msymbol ("filename");
  Mformat = msymbol ("format");
  Mvalidater = msymbol ("validater");
  Mxml = msymbol ("xml");
  Mdtd = msymbol ("dtd");
  Mxml_schema = msymbol ("xml-schema");
  Mrelaxng = msymbol ("relaxng");
  Mschematron = msymbol ("schematron");

  Masterisk = msymbol ("*");
  Mversion = msymbol ("version");

  mdb_xml = mtext__from_data ("mdb.xml", 7, MTEXT_FORMAT_BINARY, 0);
  mdb_dir = mtext__from_data ("mdb.dir", 7, MTEXT_FORMAT_BINARY, 0);
  mdb_rng = mtext__from_data ("mdb.rng", 7, MTEXT_FORMAT_BINARY, 0);
  work = mtext ();

  mdatabase__dir_list = mplist ();
  /* The environment variable M17N_SYSTEM_DIR specifies the directory
     where a system wide MDB_DIR file exists.  If the variable is not
     specified, use the macro M17NDIR.  */
  path = getenv ("M17N_SYSTEM_DIR");
  if (! path)
    path = M17NDIR;
  mplist_set (mdatabase__dir_list, Mt, get_dir_info (path, 1));

  /* The variable mdatabase_dir specifies a directory where an
     application program specific MDB_DIR file exists.  */
  if (mdatabase_dir && strlen (mdatabase_dir) > 0)
    mplist_push (mdatabase__dir_list, Mt, get_dir_info (mdatabase_dir, 0));

  /* The environment variable M17NDIR specifies a directory where a
     user specific MDB_DIR file exists.  */
  path = getenv ("M17NDIR");
  if (path && strlen (path) > 0)
    mplist_push (mdatabase__dir_list, Mt, get_dir_info (path, 0));
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
	  mplist_push (mdatabase__dir_list, Mt, get_dir_info (path, 0));
	}
    }

  mdatabase__list = mplist ();
  xml_loader_list = mplist ();
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
  M17N_OBJECT_UNREF (xml_loader_list);
  M17N_OBJECT_UNREF (mdb_xml);
  M17N_OBJECT_UNREF (mdb_dir);
  M17N_OBJECT_UNREF (mdb_rng);
  M17N_OBJECT_UNREF (work);
}

void
mdatabase__update (void)
{
  MPlist *plist, *p0, *p1, *p2, *p3;
  int rescan = 0;
  int mdebug_flag = MDEBUG_DATABASE;

  /* Update elements of mdatabase__dir_list.  */
  MPLIST_DO (plist, mdatabase__dir_list)
    if (update_dir_info (MPLIST_VAL (plist)))
      rescan = 1;
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
		  if (mdb->loader == load_database)
		    {
		      db_info = mdb->extra_info;
		      if (db_info->type == MDB_TYPE_AUTO
			  || db_info->type == MDB_TYPE_AUTO_WILDCARD)
			db_info->status = MDB_STATUS_DISABLED;
		    }
		}
	    }
	}
    }

  /* Get a reverse list of mdatabase__dir_list.  */
  plist = mplist (); 
  MPLIST_DO (p0, mdatabase__dir_list)
    mplist_push (plist, MPLIST_KEY (p0), MPLIST_VAL (p0));

  while (! MPLIST_TAIL_P (plist))
    {
      MDatabaseInfo *dir_info = mplist_pop (plist);
      MText *path;

      if (dir_info->status == MDB_STATUS_DISABLED
	  || dir_info->status == MDB_STATUS_INVALID)
	continue;
      path = GEN_PATH (dir_info->dirname, dir_info->filename);
      MDEBUG_PRINT1 (" [DB] Parsing <%s>", MTEXT_STR (path));
      p0 = (dir_info->format == Mxml
	    ? parse_mdb_xml (MTEXT_STR (path), ! dir_info->system_database)
	    : parse_mdb_dir (MTEXT_STR (path)));
      if (! p0)
	{
	  MDEBUG_PRINT (" fail\n");
	  continue;
	}
      MDEBUG_PRINT ("\n");
      MPLIST_DO (p1, p0)
	{
	  MSymbol tags[4];
	  MDatabaseInfo db_info;

	  if (parse_database_info (p1, tags, &db_info))
	    {
	      if (db_info.filename)
		register_database (tags, load_database, &db_info, db_info.type);
	      M17N_OBJECT_UNREF (db_info.filename);
	      M17N_OBJECT_UNREF (db_info.validater);
	      M17N_OBJECT_UNREF (db_info.properties);
	    }
	}
      M17N_OBJECT_UNREF (p0);
    }
  M17N_OBJECT_UNREF (plist);
}

MPlist *
mdatabase__load_for_keys (MDatabase *mdb, MPlist *keys)
{
  int mdebug_flag = MDEBUG_DATABASE;
  MDatabaseInfo *db_info;
  MText *path;
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
  path = get_database_file (db_info, NULL);
  if (! path)
    {
      db_info->status = MDB_STATUS_INVALID;
      MERROR (MERROR_DB, NULL);
    }
  plist = NULL;
  if ((fp = fopen (MTEXT_STR (path), "r")))
    {
      int c;

      while ((c = getc (fp)) != EOF && isspace (c));
      if (c == '<')
	{
	  MDatabaseLoaderXML loader = find_xml_loader (mdb->tag);

	  if (! loader)
	    MERROR (MERROR_DB, NULL);
	  plist = loader (db_info, MTEXT_STR (path));
	}
      else if (c != EOF)
	plist = mplist__from_file (fp, keys);
      fclose (fp);
    }
  return plist;
}


/* Check if the database MDB should be reloaded or not.  It returns:

	1: The database has not been modified since it was loaded last
	time.

	0: The database has never been loaded or has been updated
	since it was loaded last time.

	-1: The database is not loadable at the moment.  */

int
mdatabase__check (MDatabase *mdb)
{
  MDatabaseInfo *db_info = (MDatabaseInfo *) mdb->extra_info;
  MText *path;

  if (db_info->type != MDB_TYPE_EXPLICIT)
    mdatabase__update ();
  path = get_database_file (db_info, NULL);
  if (! path)
    return -1;
  if (db_info->time < db_info->mtime)
    return 0;
  return 1;
}

/* Search directories in mdatabase__dir_list for file FILENAME.  If
   the file exist, return a copy of the absolute pathname.  If
   FILENAME is already absolute, return a copy of it.  */

char *
mdatabase__find_file (char *filename)
{
  MText *file, *path;

  file = MTEXT_FOR_FILE (filename);
  path = get_database_file (NULL, file);
  if (! path)
    filename = NULL;
  else
    filename = strdup (MTEXT_STR (path));
  M17N_OBJECT_UNREF (file);
  return filename;
}

char *
mdatabase__file (MDatabase *mdb)
{
  MDatabaseInfo *db_info;
  MText *path;

  if (mdb->loader != load_database)
    return NULL;
  db_info = mdb->extra_info;
  path = get_database_file (db_info, NULL);
  return path ? MTEXT_STR (path) : NULL;
}

int
mdatabase__lock (MDatabase *mdb)
{
  MDatabaseInfo *db_info;
  struct stat stat_buf;
  FILE *fp;
  int len;
  char *file;
  MText *path;

  if (mdb->loader != load_database)
    return -1;
  db_info = mdb->extra_info;
  if (db_info->lock_file)
    return -1;
  path = get_database_file (db_info, NULL);
  if (! path)
    return -1;
  len = mtext_nbytes (path);
  db_info->uniq_file = malloc (len + 35);
  if (! db_info->uniq_file)
    return -1;
  db_info->lock_file = malloc (len + 5);
  if (! db_info->lock_file)
    {
      free (db_info->uniq_file);
      return -1;
    }
  sprintf (db_info->uniq_file, "%s.%X.%X", MTEXT_STR (path),
	   (unsigned) time (NULL), (unsigned) getpid ());
  sprintf (db_info->lock_file, "%s.LCK", MTEXT_STR (path));

  fp = fopen (db_info->uniq_file, "w");
  if (! fp)
    {
      char *str = strdup (db_info->uniq_file);
      char *dir = dirname (str);
      
      if (stat (dir, &stat_buf) == 0
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
      && (stat (db_info->uniq_file, &stat_buf) < 0
	  || stat_buf.st_nlink != 2))
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
  MText *path, *mt;
  int ret;

  if (mdb->loader != load_database)
    return -1;
  db_info = mdb->extra_info;
  if (! db_info->lock_file)
    return -1;
  path = get_database_file (db_info, NULL);
  if (! path)
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
  if ((ret = rename (db_info->uniq_file, MTEXT_STR (path))) < 0)
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


/*  Register the XML file loader LOADER for the database of TAGS */

void
mdatabase__register_xml_loader (MSymbol tags[4], MDatabaseLoaderXML loader)
{
  MPlist *plist;
  int i;

  for (i = 0, plist = xml_loader_list; i < 4 && tags[i] != Mnil; i++)
    {
      MPlist *pl = mplist__assq (plist, tags[i]);
      
      if (pl)
	{
	  /* pl = (TAGn [ func | (TAGn+1 ..)]) */
	  pl = MPLIST_PLIST (pl);
	  plist = MPLIST_NEXT (pl);
	  if (MPLIST_VAL_FUNC_P (plist))
	    {
	      pl = mplist ();
	      mplist_add (pl, Msymbol, tags[i]);
	      mplist_set (plist, Mplist, pl);
	      M17N_OBJECT_UNREF (pl);
	      plist = MPLIST_NEXT (pl);
	    }
	}
      else
	{
	  pl = mplist ();
	  mplist_add (pl, Msymbol, tags[i]);
	  mplist_push (plist, Mplist, pl);
	  M17N_OBJECT_UNREF (pl);
	  plist = MPLIST_NEXT (pl);
	}
    }
  mplist_add (plist, Mt, NULL);
  MPLIST_FUNC (plist) = (M17NFunc) loader;
  MPLIST_SET_VAL_FUNC_P (plist);
}

int
mdatabase__validate (xmlDocPtr doc, MDatabaseInfo *db_info)
{
  int result = 0;
  MText *path = NULL;
  char *file;

  if (db_info->schema == Mdtd)
    {
      if (db_info->validater)
	/* Not yet supported. */
	result = 0;
      else
	result = 1;
    }
  else if (db_info->validater)
    {
      path = get_database_file (NULL, db_info->validater);
      if (! path)
	result = 0;
      else
	{
	  file = MTEXT_STR (path);
	  if (db_info->schema == Mxml_schema)
	    {
	      xmlSchemaParserCtxtPtr context = NULL;
	      xmlSchemaPtr schema = NULL;
	      xmlSchemaValidCtxtPtr valid_context = NULL;

	      if ((context = xmlSchemaNewParserCtxt (file))
		  && (schema = xmlSchemaParse (context))
		  && (valid_context = xmlSchemaNewValidCtxt (schema))
		  && xmlSchemaValidateDoc (valid_context, doc) == 0)
		result = 1;
	      if (valid_context)
		xmlSchemaFreeValidCtxt (valid_context);
	      if (schema)
		xmlSchemaFree (schema);
	      if (context)
		xmlSchemaFreeParserCtxt (context);
	    }
	  else if (db_info->schema == Mrelaxng)
	    {
	      xmlRelaxNGParserCtxtPtr context = NULL;
	      xmlRelaxNGPtr schema = NULL;
	      xmlRelaxNGValidCtxtPtr valid_context = NULL;

	      if ((context = xmlRelaxNGNewParserCtxt (file))
		  && (schema = xmlRelaxNGParse (context))
		  && (valid_context = xmlRelaxNGNewValidCtxt (schema))
		  && xmlRelaxNGValidateDoc (valid_context, doc) == 0)
		result = 1;
	      if (valid_context)
		xmlRelaxNGFreeValidCtxt (valid_context);
	      if (schema)
		xmlRelaxNGFree (schema);
	      if (context)
		xmlRelaxNGFreeParserCtxt (context);
	      if (! result)
		MERROR (MERROR_DB, result);
	    }
	}
    }
  return result;
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
  MPlist *plist, *pl, *p, *p0, *p1, *p2, *p3;

  mdatabase__update ();

 retry:
  plist = mplist (), pl = plist;
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
		  goto retry;
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
		      goto retry;
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
			  goto retry;
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
  if (loader != load_database)
    mdb = register_database (tags, loader, extra_info, MDB_TYPE_EXPLICIT);
  else
    {
      MDatabaseInfo db_info;

      memset (&db_info, 0, sizeof (MDatabaseInfo));
      db_info.filename = MTEXT_FOR_FILE (extra_info);
      mdb = register_database (tags, loader, &db_info, MDB_TYPE_EXPLICIT);
      M17N_OBJECT_UNREF (db_info.filename);
    }
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
