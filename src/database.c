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
    から情報を取得する。また、アプリケーションプログラムも独自のデータを 
    m17n データベースに追加し、それを動的に取得することができる。m17n 
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

#define MAX_TIME(TIME1, TIME2) ((TIME1) >= (TIME2) ? (TIME1) : (TIME2))

static MSymbol Masterisk;

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

typedef struct
{
  char *filename;
  time_t time;
} MDatabaseInfo;

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

static FILE *
get_database_stream (MDatabaseInfo *db_info)
{
  FILE *fp = NULL;
  struct stat buf;

  if (db_info->filename[0] == '/')
    {
      if (stat (db_info->filename, &buf) == 0
	  && (fp = fopen (db_info->filename, "r")))
	db_info->time = MAX_TIME (buf.st_mtime, buf.st_ctime);
    }
  else
    {
      MPlist *plist;
      char *path;
      int filelen = strlen (db_info->filename);
      USE_SAFE_ALLOCA;

      MPLIST_DO (plist, mdatabase__dir_list)
	{
	  MDatabaseInfo *dir_info = MPLIST_VAL (plist);
	  int require = strlen (dir_info->filename) + filelen + 1;

	  SAFE_ALLOCA (path, require);
	  strcpy (path, dir_info->filename);
	  strcat (path, db_info->filename);
	  if (stat (path, &buf) == 0
	      && (fp = fopen (path, "r")))
	    {
	      free (db_info->filename);
	      db_info->filename = strdup (path);
	      db_info->time = MAX_TIME (buf.st_mtime, buf.st_ctime);
	      break;
	    }
	}
      SAFE_FREE (path);
    }
  return fp;
}

static void *
load_database (MSymbol *tags, void *extra_info)
{
  FILE *fp = get_database_stream ((MDatabaseInfo *) extra_info);
  void *value;

  if (! fp)
    MERROR (MERROR_DB, NULL);

  if (tags[0] == Mchar_table)
    value = load_chartable (fp, tags[1]);
  else if (tags[0] == Mcharset)
    value = load_charset (fp, tags[1]);
  else
    value = mplist__from_file (fp, NULL);
  fclose (fp);

  if (! value)
    MERROR (MERROR_DB, NULL);
  return value;
}


/** If DIRNAME is a readable directory, allocate MDatabaseInfo and
    copy DIRNAME to a newly allocated memory and return it.  If
    DIRNAME does not end with a slash, append a slash to the new memory.  */

static MDatabaseInfo *
get_dir_info (char *dirname)
{
  struct stat buf;
  int len;
  MDatabaseInfo *dir_info;

  if (! dirname
      || stat (dirname, &buf) < 0
      || ! (buf.st_mode & S_IFDIR))
    return NULL;

  MSTRUCT_MALLOC (dir_info, MERROR_DB);
  len = strlen (dirname);
  MTABLE_MALLOC (dir_info->filename, len + 2, MERROR_DB);
  memcpy (dir_info->filename, dirname, len + 1);
  if (dir_info->filename[len - 1] != '/')
    {
      dir_info->filename[len] = '/';
      dir_info->filename[len + 1] = '\0';
    }
  /* Set this to zero so that the first call of update_database_list
     surely checks this directory.  */
  dir_info->time = 0;
  return dir_info;
}

static MDatabase *
find_database (MSymbol tag0, MSymbol tag1, MSymbol tag2, MSymbol tag3)
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

static void
update_database_list ()
{
  MPlist *plist;
  char *path;
  USE_SAFE_ALLOCA;

  /* Usually this avoids path to be reallocated.  */
  SAFE_ALLOCA (path, 256);

  MPLIST_DO (plist, mdatabase__dir_list)
    {
      MDatabaseInfo *dir_info = MPLIST_VAL (plist);
      struct stat statbuf;
      MPlist *pl, *p;
      int i, j, len;
      FILE *fp;

      if (stat (dir_info->filename, &statbuf) < 0)
	continue;
      if (dir_info->time >= statbuf.st_ctime
	  && dir_info->time >= statbuf.st_mtime)
	continue;
      dir_info->time = MAX_TIME (statbuf.st_ctime, statbuf.st_mtime);
      len = strlen (dir_info->filename);
#ifdef PATH_MAX
      if (len + MDB_DIR_LEN >= PATH_MAX)
	continue;
#endif	/* PATH_MAX */
      SAFE_ALLOCA (path, len + MDB_DIR_LEN + 1);
      memcpy (path, dir_info->filename, len);
      memcpy (path + len, MDB_DIR, MDB_DIR_LEN);
      path[len + MDB_DIR_LEN] = '\0';
      if (! (fp = fopen (path, "r")))
	continue;
      pl = mplist__from_file (fp, NULL);
      fclose (fp);
      if (! pl)
	continue;
      MPLIST_DO (p, pl)
	{
	  MDatabase mdb;
	  MPlist *p1;
	  MText *mt;
	  int nbytes;
	  int with_wildcard = 0;

	  if (! MPLIST_PLIST_P (p))
	    continue;
	  p1 = MPLIST_PLIST (p);
	  if (! MPLIST_SYMBOL_P (p1))
	      continue;
	  for (i = 0, p1 = MPLIST_PLIST (p);
	       i < 4 && MPLIST_KEY (p1) == Msymbol;
	       i++, p1 = MPLIST_NEXT (p1))
	    with_wildcard |= ((mdb.tag[i] = MPLIST_SYMBOL (p1)) == Masterisk);
	  if (i == 0
	      || mdb.tag[0] == Masterisk
	      || ! MPLIST_MTEXT_P (p1))
	    continue;
	  for (; i < 4; i++)
	    mdb.tag[i] = Mnil;
	  mdb.loader = load_database;
	  mt = MPLIST_MTEXT (p1);
	  if (mt->format >= MTEXT_FORMAT_UTF_16LE)
	    mtext__adjust_format (mt, MTEXT_FORMAT_UTF_8);
	  nbytes = mtext_nbytes (mt);
#ifdef PATH_MAX
	  if (nbytes > PATH_MAX)
	    continue;
#endif	/* PATH_MAX */
	  if (with_wildcard
	      && mdb.tag[0] != Mchar_table
	      && mdb.tag[0] != Mcharset)
	    {
	      glob_t globbuf;
	      MPlist *load_key;

	      SAFE_ALLOCA (path, len + nbytes + 1);
	      memcpy (path, dir_info->filename, len);
	      memcpy (path + len, mt->data, nbytes);
	      path[len + nbytes] = '\0';

	      if (glob (path, GLOB_NOSORT | GLOB_NOCHECK, NULL, &globbuf) != 0)
		continue;
	      load_key = mplist ();
	      for (i = 0; i < globbuf.gl_pathc; i++)
		{
		  if (! (fp = fopen (globbuf.gl_pathv[i], "r")))
		    continue;
		  p1 = mplist__from_file (fp, load_key);
		  fclose (fp);
		  if (! p1)
		    continue;
		  if (MPLIST_PLIST_P (p1))
		    {
		      MPlist *p0;
		      MDatabase mdb2;

		      for (j = 0, p0 = MPLIST_PLIST (p1);
			   j < 4 && MPLIST_SYMBOL_P (p0);
			   j++, p0 = MPLIST_NEXT (p0))
			mdb2.tag[j] = MPLIST_SYMBOL (p0);
		      for (; j < 4; j++)
			mdb2.tag[j] = Mnil;
		      for (j = 0; j < 4; j++)
			if (mdb.tag[j] == Masterisk
			    ? mdb2.tag[j] == Mnil
			    : (mdb.tag[j] != Mnil && mdb.tag[j] != mdb2.tag[j]))
			  break;
		      if (j == 4
			  && ! find_database (mdb2.tag[0], mdb2.tag[1],
					      mdb2.tag[2], mdb2.tag[3]))
			{
			  mdb2.loader = load_database;
			  mdb2.extra_info = calloc (1, sizeof (MDatabaseInfo));
			  if (! mdb2.extra_info)
			    MEMORY_FULL (MERROR_DB);
			  ((MDatabaseInfo*) mdb2.extra_info)->filename
			    = strdup (globbuf.gl_pathv[i]);
			  MLIST_APPEND1 (&mdb_list, mdbs, mdb2, MERROR_DB);
			}
		    }
		  M17N_OBJECT_UNREF (p1);
		}
	      M17N_OBJECT_UNREF (load_key);
	      globfree (&globbuf);
	    }
	  else
	    {
	      if (find_database (mdb.tag[0], mdb.tag[1],
				 mdb.tag[2], mdb.tag[3]))
		continue;
	      SAFE_ALLOCA (path, nbytes + 1);
	      memcpy (path, mt->data, nbytes);
	      path[nbytes] = '\0';
	      mdb.extra_info = calloc (1, sizeof (MDatabaseInfo));
	      if (! mdb.extra_info)
		MEMORY_FULL (MERROR_DB);
	      ((MDatabaseInfo*) mdb.extra_info)->filename = strdup (path);
	      MLIST_APPEND1 (&mdb_list, mdbs, mdb, MERROR_DB);
	    }
	}
      M17N_OBJECT_UNREF (pl);
    }

  SAFE_FREE (path);
}


/* Internal API */

/** List of database directories.  */ 
MPlist *mdatabase__dir_list;

int
mdatabase__init ()
{
  MDatabaseInfo *dir_info;

  Mchar_table = msymbol ("char-table");
  Masterisk = msymbol ("*");

  mdatabase__dir_list = mplist ();
  /** The macro M17NDIR specifies a directory where the system-wide
    MDB_DIR file exists.  */
  if ((dir_info = get_dir_info (M17NDIR)))
    mplist_set (mdatabase__dir_list, Mt, dir_info);

  /* The variable mdatabase_dir specifies a directory where an
     application program specific MDB_DIR file exists.  */
  if ((dir_info = get_dir_info (mdatabase_dir)))
    mplist_push (mdatabase__dir_list, Mt, dir_info);

  /* The environment variable M17NDIR (if non-NULL) specifies a
     directory where a user specific MDB_DIR file exists.  */
  if ((dir_info = get_dir_info (getenv ("M17NDIR"))))
    mplist_push (mdatabase__dir_list, Mt, dir_info);

  MLIST_INIT1 (&mdb_list, mdbs, 256);
  update_database_list ();

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

  MPLIST_DO (plist, mdatabase__dir_list)
    {
      MDatabaseInfo *dir_info = MPLIST_VAL (plist);

      free (dir_info->filename);
      free (dir_info);
    }
  M17N_OBJECT_UNREF (mdatabase__dir_list);

  for (i = 0; i < mdb_list.used; i++)
    {
      MDatabase *mdb = mdb_list.mdbs + i;

      if (mdb->loader == load_database)
	{
	  MDatabaseInfo *db_info = mdb->extra_info;

	  free (db_info->filename);
	  free (db_info);
	}
    }
  MLIST_FREE1 (&mdb_list, mdbs);
}

MPlist *
mdatabase__load_for_keys (MDatabase *mdb, MPlist *keys)
{
  int mdebug_mask = MDEBUG_DATABASE;
  FILE *fp;
  MPlist *plist;
  char buf[256];

  if (mdb->loader != load_database
      || mdb->tag[0] == Mchar_table
      || mdb->tag[0] == Mcharset)
    MERROR (MERROR_DB, NULL);
  MDEBUG_PRINT1 (" [DATABASE] loading <%s>.\n",
		 gen_database_name (buf, mdb->tag));
  fp = get_database_stream ((MDatabaseInfo *) mdb->extra_info);
  if (! fp)
    MERROR (MERROR_DB, NULL);
  plist = mplist__from_file (fp, keys);
  fclose (fp);
  return plist;
}


/* Check if the database MDB should be reloaded or not.  */

int
mdatabase__check (MDatabase *mdb)
{
  MDatabaseInfo *db_info = (MDatabaseInfo *) mdb->extra_info;
  struct stat buf;

  if (stat (db_info->filename, &buf) < 0)
    return -1;
  return (db_info->time >= buf.st_ctime
	  && db_info->time >= buf.st_mtime);
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
  update_database_list ();
  return find_database (tag0, tag1, tag2, tag3);
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
  int i;
  MPlist *plist = NULL, *pl;

  update_database_list ();

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

  mdb = mdatabase_find (tag0, tag1, tag2, tag3);
  if (! mdb)
    {
      MDatabase template;

      template.tag[0] = tag0, template.tag[1] = tag1;
      template.tag[2] = tag2, template.tag[3] = tag3;
      template.extra_info = NULL;
      MLIST_APPEND1 (&mdb_list, mdbs, template, MERROR_DB);
      mdb = mdb_list.mdbs + (mdb_list.used - 1);
    }
  mdb->loader = loader ? loader : load_database;
  if (mdb->loader == load_database)
    {
      MDatabaseInfo *db_info = mdb->extra_info;

      if (db_info)
	free (db_info->filename);
      else
	db_info = mdb->extra_info = malloc (sizeof (MDatabaseInfo));
      db_info->filename = strdup ((char *) extra_info);
      db_info->time = 0;
    }
  else
    mdb->extra_info = extra_info;
  return (&(mdb_list.mdbs[mdb_list.used - 1]));
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
