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

static void decode_saction (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent);

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

static MSymbol
decode_predefined (xmlChar *ptr)
{
  char str[3];

  str[0] = '@';
  str[2] = '\0';
  if (xmlStrEqual (ptr, (xmlChar *) "first"))
    str [1] = '<';
  else if (xmlStrEqual (ptr, (xmlChar *) "current"))
    str [1] = '=';
  else if (xmlStrEqual (ptr, (xmlChar *) "last"))
    str [1] = '>';
  else if (xmlStrEqual (ptr, (xmlChar *) "previous"))
    str [1] = '-';
  else if (xmlStrEqual (ptr, (xmlChar *) "next"))
    str [1] = '+';
  else if (xmlStrEqual (ptr, (xmlChar *) "previous_candidate_list"))
    str [1] = '[';
  else if (xmlStrEqual (ptr, (xmlChar *) "next_candidate_list"))
    str [1] = ']';
  else
    str [1] = *ptr;
  return msymbol (str);
}

static void
decode_marker (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "position");

  if (ptr)
    mplist_add (parent, Msymbol, decode_predefined (ptr + 1));
  else
    {
      ptr = xmlGetProp (cur, (xmlChar *) "markerID");
      mplist_add (parent, Msymbol, msymbol ((char *) ptr));
    }
  xmlFree (ptr);
}

static void
decode_variable_reference (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  /*
  xmlChar *id = xmlGetProp (cur, (xmlChar *) "id");
  xmlChar *type = xmlGetProp (cur, (xmlChar *) "type");

  if (type)
    {
      xmlFree (type);
      if (xmlStrEqual (id, (xmlChar *) "handled-keys"))
	mplist_add (parent, Msymbol, msymbol ("@@"));
      else if (xmlStrEqual (id, (xmlChar *) "predefined-surround-text-flag"))
	mplist_add (parent, Msymbol, msymbol ("@-0"));
      else
	mplist_add (parent, Msymbol, msymbol ((char *) id));
    }
  else
    mplist_add (parent, Msymbol, msymbol ((char *) id));

  xmlFree (id);
  */

  xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "id");

  if (xmlStrEqual (ptr, (xmlChar *) "handled-keys"))
    mplist_add (parent, Msymbol, msymbol ("@@"));
  else if (xmlStrEqual (ptr, (xmlChar *) "predefined-surround-text-flag"))
    mplist_add (parent, Msymbol, msymbol ("@-0"));
  else
    mplist_add (parent, Msymbol, msymbol ((char *) ptr));
  xmlFree (ptr);
}

static void
decode_integer (xmlChar *ptr, MPlist *parent)
{
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
}

static void
decode_keyseq (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;

  if (xmlStrEqual (cur->name, (xmlChar *) "command-reference"))
    {
      ptr = xmlGetProp (cur, (xmlChar *) "id");
      /* +8 to skip "command-" */
      mplist_add (parent, Msymbol, msymbol ((char *) ptr + 8));
      xmlFree (ptr);
    }

  else if ((ptr = xmlGetProp (cur, (xmlChar *) "keys")))
    mplist_add (parent, Mtext,
		mtext_from_data (ptr, xmlStrlen (ptr), MTEXT_FORMAT_UTF_8));

  else
    {
      MPlist *plist = mplist ();

      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	{
	  ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  if (xmlStrEqual (cur->name, (xmlChar *) "key-event"))
	    /*
	      {
	      char *p;

	      sscanf ((char *) ptr, " %ms ", &p);
	      mplist_add (plist, Msymbol, msymbol (p));
	      printf ("(%s)", p);
	      free (p);
	      }
	    */
	    mplist_add (plist, Msymbol, msymbol ((char *) ptr));
	  else			/* character-code */
	    decode_integer (ptr, plist);
	  xmlFree (ptr);
	}    
      mplist_add (parent, Mplist, plist);
    }
}

static void
decode_plist (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  /* to be written */
}

static void
decode_expr (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (xmlStrEqual (cur->name, (xmlChar *) "expr"))
    {
      xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "operator");
      MPlist *plist = mplist ();

      mplist_add (plist, Msymbol, msymbol ((char *) ptr));
      xmlFree (ptr);

      for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	decode_expr (doc, cur, plist);

      mplist_add (parent, Mplist, plist);
    }
  else if (xmlStrEqual (cur->name, (xmlChar *) "int-val"))
    {
      xmlChar *ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);

      decode_integer (ptr, parent);
      xmlFree (ptr);
    }
  else if (xmlStrEqual (cur->name, (xmlChar *) "predefined-nth-previous-or-following-character"))
    {
      xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "position");
      char str[8];

      snprintf (str, 8, "@%+d", atoi ((char *) ptr));
      xmlFree (ptr);
      mplist_add (parent, Msymbol, msymbol (str));
    }
  else				/* variable-reference */
      decode_variable_reference (doc, cur, parent);
}
    
static void
decode_insert (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("insert"));

  if ((ptr = xmlGetProp (cur, (xmlChar *) "string")))
    mplist_add (plist, Mtext,
		mtext_from_data (ptr, xmlStrlen (ptr), MTEXT_FORMAT_UTF_8));

  else if ((ptr = xmlGetProp (cur, (xmlChar *) "character")))
    {
      decode_integer (ptr, plist);
      xmlFree (ptr);
    }

  else if ((ptr = xmlGetProp (cur, (xmlChar *) "character-or-string")))
    {
      xmlFree (ptr);
      ptr = xmlGetProp (cur->xmlChildrenNode, (xmlChar *) "id");
      mplist_add (plist, Msymbol, msymbol ((char *) ptr));
      xmlFree (ptr);
    }

  else if ((cur = cur->xmlChildrenNode)
  	   && xmlStrEqual (cur->name, (xmlChar *) "candidates"))
    {
      MPlist *pl = mplist ();

      mplist_add (plist, Mplist, pl);
      M17N_OBJECT_UNREF (pl);
      while (cur)
	{
	  ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  pl = mplist_add (pl, Mtext,
			   mtext_from_data (ptr, xmlStrlen (ptr),
					    MTEXT_FORMAT_UTF_8));
	  cur = cur->next;
	}
    }

  else				/* list-of-candidates */
    while (cur)
      {
	xmlChar *start;
	MPlist *plist0 = mplist (), *plist1 = mplist ();
	int ch, len = 4, skipping = 1;

  	ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);

	while ((ch = xmlGetUTF8Char (ptr, &len)))
	  {
	    if (skipping && ! Isspace (ch))
	      {
		start = ptr;
		skipping = 0;
	      }
	    else if (! skipping && Isspace (ch))
	      {
		*ptr = '\0';
		mplist_add (plist0, Mtext,
			    mtext_from_data (start, xmlStrlen (start),
					     MTEXT_FORMAT_UTF_8));
		skipping = 1;
	      }
	    ptr += len;
	    len = 4;
	  }
	if (!skipping)
	  {
	    mplist_add (plist0, Mtext,
			mtext_from_data (start, xmlStrlen (start),
					 MTEXT_FORMAT_UTF_8));
	  }

	mplist_add (plist1, Mplist, plist0);
	mplist_add (plist, Mplist, plist1);
  	cur = cur->next;
      }

  mplist_add (parent, Mplist, plist);
}

static void
decode_delete (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("delete"));

  if (xmlStrEqual (cur->name, (xmlChar *) "delete-to-marker"))
    decode_marker (doc, cur, plist);
  else if (xmlStrEqual (cur->name,
			(xmlChar *) "delete-to-character-position"))
    {
      ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      decode_integer (ptr, plist);
      xmlFree (ptr);
    }
  else				/* delete-n-characters */
    {
      char str[8];

      ptr = xmlGetProp (cur, (xmlChar *) "n");
      snprintf (str, 8, "@%+d", atoi ((char *) ptr));
      xmlFree (ptr);
      mplist_add (plist, Msymbol, msymbol (str));
    }

  mplist_add (parent, Mplist, plist);
}

static void
decode_select (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("select"));
  
  if ((ptr = xmlGetProp (cur, (xmlChar *) "selector")))
    mplist_add (plist, Msymbol, decode_predefined (ptr + 1));
  else
    {
      ptr = xmlGetProp (cur, (xmlChar *) "index");
      if (xmlStrEqual (ptr, (xmlChar *) "variable"))
	{
	  xmlFree (ptr);
	  ptr = xmlGetProp (cur->xmlChildrenNode, (xmlChar *) "id");
	  mplist_add (plist, Msymbol, msymbol ((char *) ptr));
	}
      else
	decode_integer (ptr, plist);
    }
  xmlFree (ptr);

  mplist_add (parent, Mplist, plist);
}

static void
decode_move (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("move"));

  if (xmlStrEqual (cur->name, (xmlChar *) "move-to-marker"))
    decode_marker (doc, cur, plist);

  else				/* move-to-character-position */
    {
      xmlChar *ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);

      decode_integer (ptr, plist);
      xmlFree (ptr);
    }

  mplist_add (parent, Mplist, plist);
}

static void
decode_mark (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("mark"));

  decode_marker (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
}

static void
decode_pushback (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("pushback"));

  if (xmlStrEqual (cur->name, (xmlChar *) "pushback-n-events"))
    {
      xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "n");

      decode_integer (ptr, plist);
      xmlFree (ptr);
    }

  else				/* pushback-keyseq */
    decode_keyseq (doc, cur->xmlChildrenNode, plist);

  mplist_add (parent, Mplist, plist);
}

static void
decode_undo (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("undo"));

  if ((ptr = xmlGetProp (cur, (xmlChar *) "target-of-undo")))
    {
      decode_integer (ptr, plist);
      xmlFree (ptr);
    }
  else if (cur->xmlChildrenNode)
    decode_variable_reference (doc, cur->xmlChildrenNode, plist);

  mplist_add (parent, Mplist, plist);
}

static void
decode_call (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("call"));

  ptr = xmlGetProp (cur, (xmlChar *) "id");
  /* +7 to skip "module-" */
  mplist_add (plist, Msymbol, msymbol ((char *) ptr + 7));
  xmlFree (ptr);

  cur = cur->xmlChildrenNode;
  ptr = xmlGetProp (cur, (xmlChar *) "id");
  /* +9 to skip "function-" */
  mplist_add (plist, Msymbol, msymbol ((char *) ptr + 9));
  xmlFree (ptr);

  for (cur = cur->next; cur; cur = cur->next)
    {
      ptr = xmlGetProp (cur, (xmlChar *) "type");
      if (xmlStrEqual (ptr, (xmlChar *) "string"))
	{
	  xmlFree (ptr);
	  ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  mplist_add (plist, Mtext, mtext_from_data (ptr, xmlStrlen (ptr),
						     MTEXT_FORMAT_UTF_8));
	}
      else if (xmlStrEqual (ptr, (xmlChar *) "integer"))
	{
	  xmlFree (ptr);
	  ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  decode_integer (ptr, plist);
	  xmlFree (ptr);
	}
      else if (xmlStrEqual (ptr, (xmlChar *) "plist"))
	{
	  xmlFree (ptr);
	  decode_plist (doc, cur->xmlChildrenNode, plist);
	}
      else			/* symbol */
	{
	  xmlFree (ptr);
	  decode_variable_reference (doc, cur->xmlChildrenNode, plist);
	}
    }
  mplist_add (parent, Mplist, plist);
}

static void
decode_set (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ((char *) cur->name));

  ptr = xmlGetProp (cur, (xmlChar *) "id");
  mplist_add (plist, Msymbol, msymbol ((char *) ptr));
  xmlFree (ptr);

  decode_expr (doc, cur->xmlChildrenNode, plist);

  mplist_add (parent, Mplist, plist);
}

static void
decode_if (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  xmlNodePtr cur0;
  MPlist *plist = mplist (), *plist0 = mplist ();

  ptr = xmlGetProp (cur, (xmlChar *) "condition");
  mplist_add (plist, Msymbol, msymbol ((char *) ptr));
  xmlFree (ptr);

  cur = cur->xmlChildrenNode;	/* 1st arg */
  decode_expr (doc, cur, plist);

  cur =  cur->next;		/* 2nd arg */
  decode_expr (doc, cur, plist);

  cur = cur->next;		/* then */
  plist0 = mplist ();
  for (cur0 = cur->xmlChildrenNode; cur0; cur0 = cur0->next)
    decode_saction (doc, cur0, plist0);
  mplist_add (plist, Mplist, plist0);

  cur = cur->next;		/* else */
  if (cur)
    {
      plist0 = mplist ();
      for (cur0 = cur->xmlChildrenNode; cur0; cur0 = cur0->next)
	decode_saction (doc, cur0, plist0);
      mplist_add (plist, Mplist, plist0);
    }

  mplist_add (parent, Mplist, plist);
}

static void
decode_conditional (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("cond"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    {
      xmlNodePtr cur0 = cur->xmlChildrenNode;
      MPlist *plist0 = mplist ();

      decode_expr (doc, cur0, plist0);

      while ((cur0 = cur0->next))
	decode_saction (doc, cur0, plist0);

      mplist_add (plist, Mplist, plist0);
    }

  mplist_add (parent, Mplist, plist);
}

static void
decode_action (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  if (xmlStrEqual (cur->name, (xmlChar *) "insert"))
    decode_insert (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "delete-to-marker")
	   || xmlStrEqual (cur->name,
			   (xmlChar *) "delete-to-character-position")
	   || xmlStrEqual (cur->name, (xmlChar *) "delete-n-characters"))
    decode_delete (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "select"))
    decode_select (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "show-candidates"))
    {
      mplist_add (plist, Msymbol, msymbol ("show"));
      mplist_add (parent, Mplist, plist);
    }

  else if (xmlStrEqual (cur->name, (xmlChar *) "hide-candidates"))
    {
      mplist_add (plist, Msymbol, msymbol ("hide"));
      mplist_add (parent, Mplist, plist);
    }

  else if (xmlStrEqual (cur->name, (xmlChar *) "move-to-marker")
	   || xmlStrEqual (cur->name, (xmlChar *) "move-to-character-position"))
    decode_move (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "mark-current-position"))
    decode_mark (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "pushback-n-events")
	   || xmlStrEqual (cur->name, (xmlChar *) "pushback-keyseq"))
      decode_pushback (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "pop"))
    {
      mplist_add (plist, Msymbol, msymbol ((char *) cur->name));
      mplist_add (parent, Mplist, plist);
    }

  else if (xmlStrEqual (cur->name, (xmlChar *) "undo"))
    decode_undo (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "commit"))
    {
      mplist_add (plist, Msymbol, msymbol ((char *) cur->name));
      mplist_add (parent, Mplist, plist);
    }

  else if (xmlStrEqual (cur->name, (xmlChar *) "unhandle"))
    {
      mplist_add (plist, Msymbol, msymbol ((char *) cur->name));
      mplist_add (parent, Mplist, plist);
    }

  else if (xmlStrEqual (cur->name, (xmlChar *) "call"))
    decode_call (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "set")
	   || xmlStrEqual (cur->name, (xmlChar *) "add")
	   || xmlStrEqual (cur->name, (xmlChar *) "sub")
	   || xmlStrEqual (cur->name, (xmlChar *) "mul")
	   || xmlStrEqual (cur->name, (xmlChar *) "div"))
    decode_set (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "if"))
    decode_if (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "conditional"))
    decode_conditional (doc, cur, parent);

  else if (xmlStrEqual (cur->name, (xmlChar *) "macro-reference"))
    {
      xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "id");

      /* +6 to skip "macro-" */
      mplist_add (plist, Msymbol, msymbol ((char *) ptr + 6));
      mplist_add (parent, Mplist, plist);
      xmlFree (ptr);
    }

  else
    {
      /* never comes here */
    }
}

static void
decode_saction (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  if (xmlStrEqual (cur->name, (xmlChar *) "shift-to"))
    {
      xmlChar *ptr = xmlGetProp (cur, (xmlChar *) "id");
      MPlist *plist = mplist ();

      /* +6 to skip "state-" */
      mplist_add (plist, Msymbol, msymbol ("shift"));
      mplist_add (plist, Msymbol, msymbol ((char *) ptr + 6));
      mplist_add (parent, Mplist, plist);
      xmlFree (ptr);
    }
  else
    decode_action (doc, cur, parent);
}

static void
decode_description (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;

  if (xmlStrEqual (cur->name, (xmlChar *) "get-text"))
    {
      MPlist *plist = mplist ();

      ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      mplist_add (plist, Msymbol, msymbol ("_"));
      mplist_add (plist, Mtext,
		  mtext_from_data (ptr, xmlStrlen (ptr), MTEXT_FORMAT_UTF_8));
      mplist_add (parent, Mplist, plist);
    }
  else
    {
      ptr = xmlNodeListGetString (doc, cur, 1);
      mplist_add (parent, Mtext,
		  mtext_from_data (ptr, xmlStrlen (ptr), MTEXT_FORMAT_UTF_8));
    }
}

/***/

static void
decode_im_declaration (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  /* tags */
  mplist_add (plist, Msymbol, msymbol ("input-method"));

  /* language */
  cur = cur->xmlChildrenNode;
  ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
  mplist_add (plist, Msymbol, msymbol ((char *) ptr));
  xmlFree (ptr);

  /* name */
  cur = cur->next;
  ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
  mplist_add (plist, Msymbol, msymbol ((char *) ptr));
  xmlFree (ptr);
  cur = cur->next;

  /* extra-id */
  if (cur && xmlStrEqual (cur->name, (xmlChar *) "extra-id"))
    {
      ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      mplist_add (plist, Msymbol, msymbol ((char *) ptr));
      xmlFree (ptr);
      cur = cur->next;
    }

  /* m17n-version */
  if (cur && xmlStrEqual (cur->name, (xmlChar *) "m17n-version"))
    {
      MPlist *plist0 = mplist ();

      ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
      mplist_add (plist0, Msymbol, msymbol ("version"));
      mplist_add (plist0, Mtext,
		  mtext_from_data (ptr, xmlStrlen (ptr), MTEXT_FORMAT_UTF_8));
      mplist_add (plist, Mplist, plist0);
    }

  mplist_add (parent, Mplist, plist);
}

static void
decode_im_description (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("description"));
  decode_description (doc, cur->xmlChildrenNode, plist);
  mplist_add (parent, Mplist, plist);
}

static void
decode_title (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("title"));
  mplist_add (plist, Mtext,
	      mtext_from_data (ptr, xmlStrlen (ptr), MTEXT_FORMAT_UTF_8));
  mplist_add (parent, Mplist, plist);
}

static void
decode_variable (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  ptr = xmlGetProp (cur, (xmlChar *) "id");
  mplist_add (plist, Msymbol, msymbol ((char *) ptr));
  xmlFree (ptr);

  if ((cur = cur->xmlChildrenNode))
    {
      /* description */
      if (xmlStrEqual (cur->name, (xmlChar *) "description"))
	{
	  decode_description (doc, cur->xmlChildrenNode, plist);
	  cur = cur->next;
	}
      else
	mplist_add (plist, Msymbol, Mnil);

      /* value */
      if (cur && xmlStrEqual (cur->name, (xmlChar *) "value"))
	{
	  xmlChar *valuetype;

	  ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  valuetype = xmlGetProp (cur, (xmlChar *) "type");
	  if (xmlStrEqual (valuetype, (xmlChar *) "string"))
	    mplist_add (plist, Mtext,
			mtext_from_data (ptr, xmlStrlen (ptr),
					 MTEXT_FORMAT_UTF_8));
	  else if (xmlStrEqual (valuetype, (xmlChar *) "symbol"))
	    {
	      mplist_add (plist, Msymbol, msymbol ((char *) ptr));
	      xmlFree (ptr);
	    }
	  else			/* integer */
	    {
	      decode_integer (ptr, plist);
	      xmlFree (ptr);
	    }
	  xmlFree (valuetype);
	  cur = cur->next;
	}

      /* variable-value-candidate */
      if (cur)
	for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
	  {
	    if (xmlStrEqual (cur->name, (xmlChar *) "c-value"))
	      {
		xmlChar *valuetype;

		ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
		valuetype = xmlGetProp (cur, (xmlChar *) "type");
		if (xmlStrEqual (valuetype, (xmlChar *) "string"))
		  mplist_add (plist, Mtext,
			      mtext_from_data (ptr, xmlStrlen (ptr),
					       MTEXT_FORMAT_UTF_8));
		else if (xmlStrEqual (valuetype, (xmlChar *) "symbol"))
		  {
		    mplist_add (plist, Msymbol, msymbol ((char *) ptr));
		    xmlFree (ptr);
		  }
		else		/* integer */
		  {
		    decode_integer (ptr, plist);
		    xmlFree (ptr);
		  }
		xmlFree (valuetype);
	      }
	    else			/* c-range */
	      {
		MPlist *range = mplist ();

		ptr = xmlGetProp (cur, (xmlChar *) "from");
		decode_integer (ptr, range);
		xmlFree (ptr);
		ptr = xmlGetProp (cur, (xmlChar *) "to");
		decode_integer (ptr, range);
		xmlFree (ptr);
		mplist_add (plist, Mplist, range);
	      }
	  }
    }

  mplist_add (parent, Mplist, plist);
}

static void
decode_variable_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("variable"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_variable (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
}

void
decode_command (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  ptr = xmlGetProp (cur, (xmlChar *) "id");
  /* +8 to skip "command-" */
  mplist_add (plist, Msymbol, msymbol ((char *) ptr + 8));
  xmlFree (ptr);

  if ((cur = cur->xmlChildrenNode))
    {
      /* description */
      if (xmlStrEqual (cur->name, (xmlChar *) "description"))
	{
	  decode_description (doc, cur->xmlChildrenNode, plist);
	  cur = cur->next;
	}
      else
	mplist_add (plist, Msymbol, Mnil);

      /* keyseq */
      for (; cur; cur = cur->next)
	decode_keyseq (doc, cur, plist);
    }

  mplist_add (parent, Mplist, plist);
}

static void
decode_command_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("command"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_command (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
}

static void
decode_module (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  ptr = xmlGetProp (cur, (xmlChar *) "id");
  /* +7 to skip "module-" */
  mplist_add (plist, Msymbol, msymbol ((char *) ptr + 7));
  xmlFree (ptr);

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    {
      ptr = xmlGetProp (cur, (xmlChar *) "id");
      /* +9 to skip "function" */
      mplist_add (plist, Msymbol, msymbol ((char *) ptr + 9));
      xmlFree (ptr);
    }

  mplist_add (parent, Mplist, plist);
}

static void
decode_module_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("module"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_module (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
}

static void
decode_macro (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  ptr = xmlGetProp (cur, (xmlChar *) "id");
  /* +6 to skip "macro-" */
  mplist_add (plist, Msymbol, msymbol ((char *) ptr + 6));
  xmlFree (ptr);

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_action (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
}

static void
decode_macro_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("macro"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_macro (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
}

static void
decode_rule (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  cur = cur->xmlChildrenNode;
  decode_keyseq (doc, cur, plist);
  cur = cur->next;

  for (; cur; cur = cur->next)
    decode_action (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
}      

static void
decode_map (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  ptr = xmlGetProp (cur, (xmlChar *) "id");
  /* +4 to skip "map-" */
  mplist_add (plist, Msymbol, msymbol ((char *) ptr + 4));
  xmlFree (ptr);

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_rule (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
}

static void
decode_map_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("map"));
  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_map (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
}

static void
decode_branch (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  if (xmlStrEqual (cur->name, (xmlChar *) "state-hook"))
    mplist_add (plist, Msymbol, Mt);
  else if (xmlStrEqual (cur->name, (xmlChar *) "catch-all-branch"))
    mplist_add (plist, Msymbol, Mnil);
  else		/* branch */
    {
      ptr = xmlGetProp (cur, (xmlChar *) "branch-selecting-map");
      /* +4 to skip "map-" */
      mplist_add (plist, Msymbol, msymbol ((char *) ptr + 4));
      xmlFree (ptr);
    }

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_saction (doc, cur, plist);

  if (mplist_length (plist))
    mplist_add (parent, Mplist, plist);
}

static void
decode_state (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  xmlChar *ptr;
  MPlist *plist = mplist ();

  ptr = xmlGetProp (cur, (xmlChar *) "id");
  /* +6 to skip "state-" */
  mplist_add (plist, Msymbol, msymbol ((char *) ptr + 6));
  xmlFree (ptr);

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    {
      if (xmlStrEqual (cur->name, (xmlChar *) "state-title-text"))
	{
	  ptr = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
	  mplist_add (plist, Mtext, mtext_from_data (ptr, xmlStrlen (ptr),
						     MTEXT_FORMAT_UTF_8));
	}
      else
	decode_branch (doc, cur, plist);
    }

  mplist_add (parent, Mplist, plist);
}

static void
decode_state_list (xmlDocPtr doc, xmlNodePtr cur, MPlist *parent)
{
  MPlist *plist = mplist ();

  mplist_add (plist, Msymbol, msymbol ("state"));

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    decode_state (doc, cur, plist);

  mplist_add (parent, Mplist, plist);
}

static int
rewrite_include (xmlNodePtr cur)
{
  xmlChar *ptr, *suffix, *filename, *fullname, *newvalue;
  int len;

  ptr = xmlGetProp (cur, (xmlChar *) "href");
  suffix = (xmlChar *) xmlStrstr (ptr, (xmlChar *) "#xmlns");

  len = suffix - ptr;
  filename = malloc (len + 1);
  filename[0] = '\0';
  xmlStrncat (filename, ptr, len);
  fullname = (xmlChar *) mdatabase__find_file ((char *) filename);
  if (! fullname)
    {
      xmlFree (ptr);
      free (filename);
      return -1;
    }
  else
    {
      newvalue = xmlStrncatNew (fullname, suffix, -1);
      xmlSetProp (cur, (xmlChar *) "href", newvalue);
      xmlFree (ptr);
      free (filename);
      free (fullname);
      xmlFree (newvalue);
      return 0;
    }
}

static int
prepare_include (xmlNodePtr cur)
{
  xmlNodePtr cur0;

  for (cur = cur->xmlChildrenNode; cur; cur = cur->next)
    if (xmlStrEqual (cur->name, (xmlChar *) "macro-list")
	|| xmlStrEqual (cur->name, (xmlChar *) "map-list")
	|| xmlStrEqual (cur->name, (xmlChar *) "state-list"))
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

  cur = xmlDocGetRootElement (doc)->xmlChildrenNode;
  decode_im_declaration (doc, cur, xml);
  cur = cur->next;

  if (cur && xmlStrEqual (cur->name, (xmlChar *) "description"))
    {
      decode_im_description (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && xmlStrEqual (cur->name, (xmlChar *) "title"))
    {
      decode_title (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && xmlStrEqual (cur->name, (xmlChar *) "variable-list"))
    {
      decode_variable_list (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && xmlStrEqual (cur->name, (xmlChar *) "command-list"))
    {
      decode_command_list (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && xmlStrEqual (cur->name, (xmlChar *) "module-list"))
    {
      decode_module_list (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && xmlStrEqual (cur->name, (xmlChar *) "macro-list"))
    {
      decode_macro_list (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && xmlStrEqual (cur->name, (xmlChar *) "map-list"))
    {
      decode_map_list (doc, cur, xml);
      cur = cur->next;
    }

  if (cur && xmlStrEqual (cur->name, (xmlChar *) "state-list"))
    {
      decode_state_list (doc, cur, xml);
      cur = cur->next;
    }

  xmlFreeDoc (doc);
  return xml;
}
