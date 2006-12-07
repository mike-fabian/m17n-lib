/* linkcore.c -- test program for linking with m17n-core.so.
   Copyright (C) 2003
     National Institute of Advanced Industrial Science and Technology (AIST)
     Registration Number H15PRO112

This file is part of the m17n library.

The m17n library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2, or (at
your option) any later version.

The m17n library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the m17n library; see the file COPYING.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.  */

#include <stdlib.h>
#include "m17n-core.h"

int
main ()
{
  M17N_INIT ();
  M17N_FINI ();
  exit (0);
}
