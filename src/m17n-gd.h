/* m17n-gd.h -- header file for the GUI API for GD Library.
   Copyright (C) 2004
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

#ifndef _M17N_GD_H_
#define _M17N_GD_H_

#ifndef _M17N_GUI_H_
#include <m17n-gui.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

extern int m17n_init_gd (void);
#undef M17N_INIT_GD
#define M17N_INIT_GD() m17n_init_gd ()

#undef M17N_INIT
#define M17N_INIT()			\
  do {					\
    if (m17n_init_win () < 0) break;	\
    if (M17N_INIT_GD () < 0) break;	\
    M17N_INIT_X ();			\
  } while (0)

#ifdef __cplusplus
}
#endif

#endif /* not _M17N_GD_H_ */
