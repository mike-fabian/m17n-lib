/* m17n.c -- body of the SHELL API.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012
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

#include <config.h>
#include <stdio.h>

#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "charset.h"
#include "coding.h"

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)


/* Internal API */


/* External API */

void
m17n_init (void)
{
  int mdebug_flag = MDEBUG_INIT;

  merror_code = MERROR_NONE;
  if (m17n__shell_initialized++)
    return;
  m17n_init_core ();
  if (merror_code != MERROR_NONE)
    {
      m17n__shell_initialized--;
      return;
    }
  MDEBUG_PUSH_TIME ();
  MDEBUG_PUSH_TIME ();
  if (mcharset__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT",
		     (mdebug__output, " to initialize charset module."));
  if (mcoding__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (mdebug__output, " to initialize conv module."));
  if (mcharset__load_from_database () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (mdebug__output, " to load charset definitions."));
  if (mcoding__load_from_database () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (mdebug__output, " to load coding definitions."));
  if (mlang__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT",
		     (mdebug__output, " to initialize language module"));
  if (mlocale__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (mdebug__output, " to initialize locale module."));
  if (minput__init () < 0)
    goto err;
  MDEBUG_PRINT_TIME ("INIT", (mdebug__output, " to initialize input module."));

 err:
  MDEBUG_POP_TIME ();
  MDEBUG_PRINT_TIME ("INIT",
		     (mdebug__output, " to initialize the shell modules."));
  MDEBUG_POP_TIME ();
}

void
m17n_fini (void)
{
  int mdebug_flag = MDEBUG_FINI;

  if (m17n__shell_initialized == 0
      || --m17n__shell_initialized > 0)
    return;

  MDEBUG_PUSH_TIME ();
  MDEBUG_PUSH_TIME ();
  minput__fini ();
  MDEBUG_PRINT_TIME ("FINI", (mdebug__output, " to finalize input module."));
  mlocale__fini ();
  MDEBUG_PRINT_TIME ("FINI", (mdebug__output, " to finalize locale module."));
  mlang__fini ();
  MDEBUG_PRINT_TIME ("FINI", (mdebug__output, " to finalize language module."));
  mchar__fini ();
  MDEBUG_PRINT_TIME ("FINI",
		     (mdebug__output, " to finalize character module."));
  mdatabase__fini ();
  MDEBUG_PRINT_TIME ("FINI",
		     (mdebug__output, " to finalize database module."));
  mcoding__fini ();
  MDEBUG_PRINT_TIME ("FINI",
		     (mdebug__output, " to finalize coding module."));
  mcharset__fini ();
  MDEBUG_PRINT_TIME ("FINI",
		     (mdebug__output, " to finalize charset module."));
  MDEBUG_POP_TIME ();
  MDEBUG_PRINT_TIME ("FINI",
		     (mdebug__output, " to finalize the shell modules."));
  MDEBUG_POP_TIME ();
  m17n_fini_core ();
}

#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
