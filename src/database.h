/* database.h -- header file for the database module.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
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

#ifndef _M17N_DATABASE_H_
#define _M17N_DATABASE_H_

#ifndef M17NDIR
#define M17NDIR "/usr/local/share/m17n"
#endif

#ifdef PATH_MAX
#undef PATH_MAX
#endif
#define PATH_MAX 1024

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR '/'
#endif

enum MDatabaseType
  {
    /* The database was defined automatically (from mdb.dir file(s))
       with no wildcard tag.  */
    MDB_TYPE_AUTO,
    /* The database was defined automatically (from mdb.dir file(s))
       with a wildcard tag to define multiple databases of the same
       kind.  */
    MDB_TYPE_AUTO_WILDCARD,
    /* The database was defined explicitely (by mdatabase_define ()). */
    MDB_TYPE_EXPLICIT
  };

enum MDatabaseStatus
  {
    /* The database file is currently disabled.  It means that the
       database file is not readable or the database is deleted by the
       modification of "mdb.dir".  */
    MDB_STATUS_DISABLED,
    /* The database file has not yet been loaded, or was modified
       after the previous loading.  */
    MDB_STATUS_OUTDATED,
    /* The database file has not been modified after the previous
       loading.  */
    MDB_STATUS_UPDATED,
    /* The database file is updated but the validation was failed.  If
       this is for a database directory, the directory is readable but
       "mdb.dir" doesn't exist in it.  */
    MDB_STATUS_INVALID
  };

typedef struct
{
  /* 1 iff the database is for the system wide directory.  Such an
     database is already validated at the installation time.  */
  int system_database;
  /* Name of the directory containing the database file with trailing
     PATH_SEPARATOR.  It is NULL if `filename' is an absolute
     path.  */
  MText *dirname;
  /* Name of the file containing the database.  If this is for a
     database directory, "mdb.xml" or "mdb.dir".  */
  MText *filename;
  /* Validater filename.  */
  MText *validater;
  int major_version, minor_version, release_version;
  enum MDatabaseType type;
  /* The current status of the database.  */
  enum MDatabaseStatus status;
  /* The format of the database file.  */
  MSymbol format;
  MSymbol schema;
  /* The last modification time of the database file.  */
  time_t mtime;
  /* When the database was loaded last.  0 if it has never been
     loaded.  If this is for a database directory, this is a time of
     whichever newer, the direcotry modification or the loading of
     "mdb.xml" (or "mdb.dir") in it.  */
  time_t time;
  char *lock_file, *uniq_file;

  MPlist *properties;
} MDatabaseInfo;

typedef MPlist *(*MDatabaseLoaderXML) (MDatabaseInfo *, char *filename);

extern MPlist *mdatabase__dir_list;

extern void mdatabase__update (void);

extern MPlist *mdatabase__load_for_keys (MDatabase *mdb, MPlist *keys);

extern int mdatabase__check (MDatabase *mdb);

extern char *mdatabase__find_file (char *filename);

extern char *mdatabase__file (MDatabase *mdb);

extern int mdatabase__lock (MDatabase *mdb);

extern int mdatabase__save (MDatabase *mdb, MPlist *data);

extern int mdatabase__unlock (MDatabase *mdb);

extern MPlist *mdatabase__props (MDatabase *mdb);

extern void *(*mdatabase__load_charset_func) (FILE *fp, MSymbol charset_name);

#include <libxml/xmlreader.h>
extern int mdatabase__validate (xmlDocPtr doc, MDatabaseInfo *db_info);

#endif /* not _M17N_DATABASE_H_ */
