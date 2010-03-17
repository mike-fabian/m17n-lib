/* input-xml.c -- XML decoder for input method module.
   Copyright (C) 2009
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xinclude.h>

#include "config.h"
#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "database.h"
#include "plist.h"

#define ADD_STRING(pl)							\
  do {									\
    xmlChar *ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1); \
    MText *mt = mtext_from_data (ptr, xmlStrlen (ptr), MTEXT_FORMAT_UTF_8); \
    mplist_add (pl, Mtext, mt);						\
    M17N_OBJECT_UNREF (mt);						\
  } while (0)

#define ADD_SYMBOL(pl)						      \
  do {								      \
  xmlChar *ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1); \
  mplist_add (pl, Msymbol, msymbol ((char *) ptr));		      \
  xmlFree (ptr);						      \
  } while (0)

#define ADD_SYMBOL_PROP(pl, prop)				\
  do {								\
    xmlChar *ptr = xmlGetProp (cur, (xmlChar *) prop);		\
    mplist_add (pl, Msymbol, msymbol ((char *) ptr));		\
    xmlFree (ptr);						\
  } while (0)

#define CUR_NAME(str) xmlStrEqual (cur->name, (xmlChar *) str)

static int try_decode_keyseq (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_marker (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_selector (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_integer (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_string (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_symbol (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_error (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_varref (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_funcall (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);

static int try_decode_keyseqterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_markerterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_selectorterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_intterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_strterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_symterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);
static int try_decode_listterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);

static void decode_term (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);

static MPlist *external_name;

static int
try_decode_keyseq (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("keyseq"))
    cur = cur->xmlChildrenNode;

  if (try_decode_listterm (doc, cur, parent))
    return 1;
  if (try_decode_strterm (doc, cur, parent))
    return 1;
  return 0;
}

static int
try_decode_marker (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("marker"))
    {
      ADD_SYMBOL (parent);
      return 1;
    }
  return 0;
}

static int
try_decode_selector (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("selector"))
    {
      xmlChar *ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      char str[3] = "@ \0";

      if (xmlStrEqual (ptr, (xmlChar *) "@first"))
	str [1] = '<';
      else if (xmlStrEqual (ptr, (xmlChar *) "@current"))
	str [1] = '=';
      else if (xmlStrEqual (ptr, (xmlChar *) "@last"))
	str [1] = '>';
      else if (xmlStrEqual (ptr, (xmlChar *) "@previous"))
	str [1] = '-';
      else if (xmlStrEqual (ptr, (xmlChar *) "@next"))
	str [1] = '+';
      else if (xmlStrEqual (ptr, (xmlChar *) "@previous-group"))
	str [1] = '[';
      else				/* @next-group */
	str [1] = ']';

      mplist_add (parent, Msymbol, msymbol (str));
      return 1;
    }
  return 0;
}

/* Sometimes isspace () returns non-zero values for chars between 255
   and EOF. */
static int
Isspace (int ch)
{
  if (ch == 32 || ch == 8 || ch == 10 || ch == 13)
    return 1;
  else
    return 0;
}

static int
try_decode_integer (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("integer"))
    {
      xmlChar *ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);

      while (Isspace (*ptr))
	ptr++;

      if (xmlStrlen (ptr) >= 3
	  && (ptr[0] == '0' || ptr[0] == '#')
	  && ptr[1] == 'x')
	{
	  int val;

	  sscanf ((char *) ptr + 2, "%x", &val);
	  mplist_add (parent, Minteger, (void *) val);
	}
      else if (ptr[0] == '?')
	{
	  int val, len = 4;

	  val = xmlGetUTF8Char (ptr + 1, &len);
	  mplist_add (parent, Minteger, (void *) val);
	}
      else
	mplist_add (parent, Minteger, (void *) atoi ((char *) ptr));

      xmlFree (ptr);
      return 1;
    }
  return 0;
}

static int
try_decode_string (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("string"))
    {
      ADD_STRING (parent);
      return 1;
    }
  return 0;
}

static int
try_decode_symbol (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("symbol"))
    {
      ADD_SYMBOL (parent);
      return 1;
    }
  return 0;
}

static int
try_decode_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("list"))
    {
      MPlist *plist = mplist ();

      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	decode_term (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }
  return 0;
}

static int
try_decode_error (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  /* not implemented yet */
  return 0;
}

static int
try_decode_varref (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("varref"))
    {
      ADD_SYMBOL_PROP (parent, "vname");
      return 1;
    }
  return 0;
}

static int
try_decode_command_reference (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("command"))
    {
      xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "cname");

      /* 8 for "command-" */
      mplist_add (parent, Msymbol, msymbol ((char *) ptr + 8));
      xmlFree (ptr);
      return 1;
    }
  return 0;
}

static int
try_decode_predefined (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("set"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("set"));
      ADD_SYMBOL_PROP (plist, "vname");
      decode_term (doc, cur->xmlChildrenNode, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("and"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("&"));
      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	decode_term (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("or"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("|"));
      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	decode_term (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("not"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("!"));
      decode_term (doc, cur->xmlChildrenNode, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("eq"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("="));
      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	decode_term (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("noteq"))
    {
      /* not implemented yet */
      return 1;
    }

  else if (CUR_NAME ("equal"))
    {
      /* not implemented yet */
      return 1;
    }

  else if (CUR_NAME ("match"))
    {
      /* not implemented yet */
      return 1;
    }

  else if (CUR_NAME ("lt"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("<"));
      cur = cur->xmlChildrenNode;
      try_decode_intterm (doc, cur, plist);
      cur = cur->next;
      try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("le"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("<="));
      cur = cur->xmlChildrenNode;
      try_decode_intterm (doc, cur, plist);
      cur = cur->next;
      try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("gt"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol (">"));
      cur = cur->xmlChildrenNode;
      try_decode_intterm (doc, cur, plist);
      cur = cur->next;
      try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("ge"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol (">="));
      cur = cur->xmlChildrenNode;
      try_decode_intterm (doc, cur, plist);
      cur = cur->next;
      try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("add"))
    {
      MPlist *plist = mplist ();
      xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "vname");

      if (ptr)
	{
	  mplist_add (plist, Msymbol, msymbol ("add"));
	  mplist_add (plist, Msymbol, msymbol ((char *) ptr));
	  xmlFree (ptr);
	}
      else
	mplist_add (plist, Msymbol, msymbol ("+"));
      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("sub"))
    {
      MPlist *plist = mplist ();
      xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "vname");

      if (ptr)
	{
	  mplist_add (plist, Msymbol, msymbol ("sub"));
	  mplist_add (plist, Msymbol, msymbol ((char *) ptr));
	  xmlFree (ptr);
	}
      else
	mplist_add (plist, Msymbol, msymbol ("-"));
      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("mul"))
    {
      MPlist *plist = mplist ();
      xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "vname");

      if (ptr)
	{
	  mplist_add (plist, Msymbol, msymbol ("mul"));
	  mplist_add (plist, Msymbol, msymbol ((char *) ptr));
	  xmlFree (ptr);
	}
      else
	mplist_add (plist, Msymbol, msymbol ("*"));
      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("div"))
    {
      /* not used */
      return 1;
    }

  else if (CUR_NAME ("mod"))
    {
      /* not used */
      return 1;
    }

  else if (CUR_NAME ("logand"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("&"));
      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("logior"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("|"));
      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("progn"))
    {
      MPlist *plist = mplist ();

      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	decode_term (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("if"))
    {
      MPlist *plist = mplist ();

      /* The CONDITION term: only <gt>, <lt> and <eq> are used in the
	 current mimx files. */
      cur = cur->xmlChildrenNode;
      if (CUR_NAME ("gt"))
	mplist_add (plist, Msymbol, msymbol (">"));
      else if (CUR_NAME ("lt"))
	mplist_add (plist, Msymbol, msymbol ("<"));
      else
	mplist_add (plist, Msymbol, msymbol ("="));
      decode_term (doc, cur->xmlChildrenNode, plist);
      decode_term (doc, cur->xmlChildrenNode->next, plist);

      /* The "THEN" term */
      cur = cur->next;
      decode_term (doc, cur, plist);

      /* The "ELSE" term */
      if ((cur = cur->next))
	decode_term (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("cond"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("cond"));
      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	try_decode_list (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }
  return 0;
}

static int
try_decode_funcall (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (CUR_NAME ("insert"))
    {
      cur = cur->xmlChildrenNode;
      if (try_decode_intterm (doc, cur, parent))
	return 1;
      if (try_decode_strterm (doc, cur, parent))
	return 1;
    }

  else if (CUR_NAME ("insert-candidates"))
    {
      MPlist *plist = mplist ();

      cur = cur->xmlChildrenNode;
      if (try_decode_listterm (doc, cur, plist))
	;
      else
	try_decode_strterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("delete"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("delete"));
      cur = cur->xmlChildrenNode;
      if (try_decode_markerterm (doc, cur, plist))
	;
      else
	try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("select"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("select"));
      cur = cur->xmlChildrenNode;
      if (try_decode_selectorterm (doc, cur, plist))
	;
      else
	try_decode_intterm (doc, cur, plist);
      
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("show-candidates"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("show"));
      
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("hide-candidates"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("hide"));
      
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("move"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("move"));
      cur = cur->xmlChildrenNode;
      if (try_decode_markerterm (doc, cur, plist))
	;
      else
	try_decode_intterm (doc, cur, plist);
      
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("mark"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("mark"));
      try_decode_markerterm (doc, cur->xmlChildrenNode, plist);
      
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("pushback"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("pushback"));
      cur = cur->xmlChildrenNode;
      if (try_decode_keyseqterm (doc, cur, plist))
	;
      else
	try_decode_intterm (doc, cur, plist);
      
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("pop"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("pop"));
      
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("undo"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("undo"));
      if ((cur = cur->xmlChildrenNode))
	try_decode_intterm (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("commit"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("commit"));
      
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("unhandle"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("unhandle"));
      
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("shift"))
    {
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ("shift"));
      try_decode_symterm (doc, cur->xmlChildrenNode, plist);
      
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else if (CUR_NAME ("shiftback"))
    {
      /* not implemented yet */
      return 1;
    }

  else if (CUR_NAME ("char-at"))
    {
      try_decode_markerterm (doc, cur->xmlChildrenNode, parent);
      return 1;
    }

  else if (CUR_NAME ("key-count"))
    {
      mplist_add (parent, Msymbol, msymbol ("@@"));
      return 1;
    }

  else if (CUR_NAME ("surrounding-text-flag"))
    {
      mplist_add (parent, Msymbol, msymbol ("@-0"));
      return 1;
    }

  else if (CUR_NAME ("funcall"))
    {
      MPlist *plist = mplist (), *pl;
      xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "fname");
      MSymbol sym = msymbol ((char *) ptr);

      xmlFree (ptr);
      pl =  mplist__assq (external_name, sym);
      if (pl)
	{
	  mplist_add (plist, Msymbol, msymbol ("call"));
	  pl = mplist_next ((MPlist *) mplist_value (pl));
	  mplist_add (plist, Msymbol, (MSymbol) mplist_value (pl));
	  pl = mplist_next (pl);
	  mplist_add (plist, Msymbol, (MSymbol) mplist_value (pl));
	}
      else
	mplist_add (plist, Msymbol, sym);

      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	decode_term (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
      return 1;
    }

  else
    return (try_decode_predefined (doc, cur, parent));

  return 0;			/* suppress compiler's warning */
}

static int
try_decode_keyseqterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (try_decode_keyseq (doc, cur, parent))
    return 1;
  if (try_decode_funcall (doc, cur, parent))
    return 1;
  if (try_decode_varref (doc, cur, parent))
    return 1;
  return 0;
}

static int
try_decode_markerterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (try_decode_marker (doc, cur, parent))
    return 1;
  if (try_decode_funcall (doc, cur, parent))
    return 1;
  if (try_decode_varref (doc, cur, parent))
    return 1;
  return 0;
}

static int
try_decode_selectorterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (try_decode_selector (doc, cur, parent))
    return 1;
  if (try_decode_funcall (doc, cur, parent))
    return 1;
  if (try_decode_varref (doc, cur, parent))
    return 1;
  return 0;
}

static int
try_decode_intterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (try_decode_integer (doc, cur, parent))
    return 1;
  if (try_decode_varref (doc, cur, parent))
    return 1;
  if (try_decode_funcall (doc, cur, parent))
    return 1;
  return 0;
}

static int
try_decode_strterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (try_decode_string (doc, cur, parent))
    return 1;
  if (try_decode_varref (doc, cur, parent))
    return 1;
  if (try_decode_funcall (doc, cur, parent))
    return 1;
  return 0;
}

static int
try_decode_symterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (try_decode_symbol (doc, cur, parent))
    return 1;
  if (try_decode_varref (doc, cur, parent))
    return 1;
  if (try_decode_funcall (doc, cur, parent))
    return 1;
  return 0;
}

static int
try_decode_listterm (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (try_decode_list (doc, cur, parent))
    return 1;
  if (try_decode_varref (doc, cur, parent))
    return 1;
  if (try_decode_funcall (doc, cur, parent))
    return 1;
  return 0;
}

static void
decode_term (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (try_decode_keyseq (doc, cur, parent))
    return;
  if (try_decode_marker (doc, cur, parent))
    return;
  if (try_decode_selector (doc, cur, parent))
    return;
  if (try_decode_integer (doc, cur, parent))
    return;
  if (try_decode_string (doc, cur, parent))
    return;
  if (try_decode_symbol (doc, cur, parent))
    return;
  if (try_decode_list (doc, cur, parent))
    return;
  if (try_decode_error (doc, cur, parent))
    return;
  if (try_decode_varref (doc, cur, parent))
    return;
  if (try_decode_funcall (doc, cur, parent))
    return;
}

static void
decode_im_declaration (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();
  xmlNodePtr backup = cur->next;

  /* tags */
  mplist_add (plist, Msymbol, msymbol ("input-method"));

  /* language */
  cur = cur->xmlChildrenNode;
  ADD_SYMBOL (plist);

  /* name */
  cur = cur->next;
  ADD_SYMBOL (plist);
  cur = cur->next;

  /* extra-id */
  if (cur && CUR_NAME ("extra-id"))
    {
      ADD_SYMBOL (plist);
      cur = cur->next;
    }

  /* m17n-version */
  if ((cur = backup) && CUR_NAME ("m17n-version"))
    {
      MPlist *plist0 = mplist ();

      mplist_add (plist0, Msymbol, msymbol ("version"));
      ADD_STRING (plist0);
      mplist_add (plist, Mplist, plist0);
      M17N_OBJECT_UNREF (plist0);
    }

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_description (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (cur->xmlChildrenNode
      && xmlStrEqual (cur->xmlChildrenNode->name, (xmlChar *) "gettext"))
    {
      MPlist *plist = mplist ();

      cur = cur->xmlChildrenNode;
      mplist_add (plist, Msymbol, msymbol ("_"));
      ADD_STRING (plist);
      mplist_add (parent, Mplist, plist);
      M17N_OBJECT_UNREF (plist);
    }
  else
      ADD_STRING (parent);
}

static void
decode_im_description (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("description"));
  decode_description (doc, cur, plist);
  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_title (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("title"));
  ADD_STRING (plist);
  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_defvar (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  ADD_SYMBOL_PROP (plist, "vname");

  if ((cur = cur->xmlChildrenNode))
    {
      /* description */
      if (CUR_NAME ("description"))
	{
	  decode_description (doc, cur, plist);
	  cur = cur->next;
	}
      else
	mplist_add (plist, Msymbol, Mnil);
    }

  if (cur)
    {
      try_decode_integer (doc, cur, plist);
      try_decode_string (doc, cur, plist);
      try_decode_symbol (doc, cur, plist);

      if ((cur = cur->next)) /* if exist, must be "possible-value" */
	for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	  {
	    try_decode_integer (doc, cur, plist);
	    /* not implemented yet
	       try_decode_range (doc, cur, plist); */
	    try_decode_string (doc, cur, plist);
	    try_decode_symbol (doc, cur, plist);
	  }
    }

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_variable_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("variable"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_defvar (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

void
decode_defcmd (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();
  xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "cname");

  /* 8 for "command-" */
  mplist_add (plist, Msymbol, msymbol ((char *) ptr + 8));
  xmlFree (ptr);

  if ((cur = cur->xmlChildrenNode))
    {
      /* description */
      if (CUR_NAME ("description"))
	{
	  decode_description (doc, cur, plist);
	  cur = cur->next;
	}
      else
	mplist_add (plist, Msymbol, Mnil);

      /* keyseq */
      for (; cur; cur = cur->next)
	try_decode_keyseq (doc, cur, plist);
    }

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_command_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("command"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_defcmd (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_module (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();
  xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "id");
  MSymbol module = msymbol ((char *) ptr);
  int prefix = xmlStrlen (ptr) + 10; /* 10 for "-function-" */

  xmlFree (ptr);
  mplist_add (plist, Msymbol, module);

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    {
      xmlChar *ptr0 = xmlGetProp (cur, (xmlChar *) "fname");
      MSymbol longname = msymbol ((char *) ptr0);
      MSymbol shortname = msymbol ((char *) ptr0 + prefix);
      MPlist *plist0 = mplist ();

      xmlFree (ptr0);
      mplist_add (plist, Msymbol, shortname);

      mplist_add (plist0, Msymbol, longname);
      mplist_add (plist0, Msymbol, module);
      mplist_add (plist0, Msymbol, shortname);
      mplist_add (external_name, Mplist, plist0);
      M17N_OBJECT_UNREF (plist0);
    }

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_module_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("module"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_module (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_defmacro (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  ADD_SYMBOL_PROP (plist, "fname");

  /* no <args> are used now */

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_term (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_macro_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("macro"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_defmacro (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_rule (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  cur = cur->xmlChildrenNode; 
  if (try_decode_keyseq (doc, cur, plist))
    ;
  else
    try_decode_command_reference (doc, cur, plist);

  for (cur = cur->next; cur; cur = cur->next)
    try_decode_funcall (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_map (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  ADD_SYMBOL_PROP (plist, "mname");

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_rule (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_map_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("map"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_map (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_state_hook (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, Mt);

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    try_decode_funcall (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_catch_all_branch (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, Mnil);

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    try_decode_funcall (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_branch (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  ADD_SYMBOL_PROP (plist, "mname");

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    try_decode_funcall (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_state (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  ADD_SYMBOL_PROP (plist, "sname");

  if ((cur = cur->xmlChildrenNode) && CUR_NAME ("title"))
    {
      ADD_STRING (plist);
      cur = cur->next;
    }

  for (; cur; cur = cur->next)
    {
      if (CUR_NAME ("state-hook"))
	decode_state_hook (doc, cur, plist);
      else if (CUR_NAME ("catch-all-branch"))
	decode_catch_all_branch (doc, cur, plist);
      else
	decode_branch (doc, cur, plist);
    }

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static void
decode_state_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("state"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_state (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
  M17N_OBJECT_UNREF (plist);
}

static int
rewrite_include (xmlNodePtr cur)
{
  xmlChar *filename, *fullname;

  filename = xmlGetProp (cur, (xmlChar *) "href");
  fullname = (xmlChar *) mdatabase__find_file ((char *) filename);
  if (! fullname)
    {
      xmlFree (filename);
      return -1;
    }
  else
    {
      xmlSetProp (cur, (xmlChar *) "href", (xmlChar *) fullname);
      xmlFree (filename);
      free (fullname);
      return 0;
    }
}

static int
prepare_include (xmlNodePtr cur)
{
  xmlNodePtr cur0;

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    if (CUR_NAME ("macro-list")	|| CUR_NAME ("map-list")
	|| CUR_NAME ("state-list"))
      for (cur0 = cur->xmlChildrenNode; cur0; cur0 = cur0->next)
	if (xmlStrEqual (cur0->name, (xmlChar *) "include"))
	  if (rewrite_include (cur0) == -1)
	    return -1;
  return 0;
}

MPlist *
minput__load_xml (MDatabaseInfo *db_info, char *filename)
{
  xmlDocPtr doc;
  xmlNodePtr cur;
  MPlist *xml = mplist ();

  doc = xmlReadFile (filename, NULL, XML_PARSE_NOENT | XML_PARSE_NOBLANKS);
  if (! doc)
    return NULL;

  cur = xmlDocGetRootElement (doc);
  if (! cur)
    {
      xmlFreeDoc (doc);
      return NULL;
    }

  if (xmlStrcmp (cur->name, (xmlChar *) "input-method"))
    {
      xmlFreeDoc (doc);
      return NULL;
    }

  if (prepare_include (cur) == -1)
    {
      xmlFreeDoc (doc);
      return NULL;
    }

  xmlXIncludeProcessFlags (doc, XML_PARSE_NOENT | XML_PARSE_NOBLANKS
			   | XML_PARSE_NOXINCNODE);
  if (! mdatabase__validate (doc, db_info))
    {
      xmlFreeDoc (doc);
      MERROR (MERROR_IM, NULL);
    }

  external_name = mplist ();
  cur = xmlDocGetRootElement (doc)->xmlChildrenNode;
  decode_im_declaration (doc, cur, xml);
  cur = cur->next;

  if (cur && CUR_NAME ("m17n-version"))
    /* This should have been processed in decode_im_declaration (). */
    cur = cur->next;

  if (cur && CUR_NAME ("description"))
    {
      decode_im_description (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && CUR_NAME ("title"))
    {
      decode_title (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && CUR_NAME ("variable-list"))
    {
      decode_variable_list (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && CUR_NAME ("command-list"))
    {
      decode_command_list (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && CUR_NAME ("module-list"))
    {
      decode_module_list (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && CUR_NAME ("macro-list"))
    {
      decode_macro_list (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && CUR_NAME ("map-list"))
    {
      decode_map_list (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && CUR_NAME ("state-list"))
    {
      decode_state_list (doc, cur, xml);
      cur = cur->next;
    }

  xmlFreeDoc (doc);
  return xml;
}
