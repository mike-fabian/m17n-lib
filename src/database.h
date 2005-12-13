/* database.h -- header file for the database module.
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

#ifndef _M17N_DATABASE_H_
#define _M17N_DATABASE_H_

#ifndef M17NDIR
#define M17NDIR "/usr/local/share/m17n"
#endif

extern MPlist *mdatabase__dir_list;

extern MPlist *mdatabase__load_for_keys (MDatabase *mdb, MPlist *keys);

extern int mdatabase__check (MDatabase *mdb);

extern char *mdatabase__find_file (char *filename);

#endif /* not _M17N_DATABASE_H_ */
