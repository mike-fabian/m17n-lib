/* font-flt.c -- Font Layout Table sub-module.
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <regex.h>

#include "m17n-gui.h"
#include "m17n-misc.h"
#include "internal.h"
#include "mtext.h"
#include "symbol.h"
#include "plist.h"
#include "internal-gui.h"
#include "font.h"
#include "face.h"

/* Font Layouter */

/* Font Layout Table (FLT)

Predefined terms: SYMBOL, INTEGER, STRING

FLT ::=	'(' STAGE + ')'

STAGE ::= CATEGORY-TABLE ? FONT-LAYOUT-RULE

;; Each STAGE consumes a source (code sequence) and produces another
;; code sequence that is given to the next STAGE as a source.  The
;; source given to the first stage is a sequence of character codes
;; that are assigned category codes by CATEGORY-TABLE.  The output of
;; the last stage is a glyph code sequence given to the renderer.

CATEGORY-TABLE ::=
	'(' 'category' CATEGORY-SPEC + ')'
CATEGORY-SPEC ::=
	'(' CODE [ CODE ] CATEGORY ')'
CODE ::= INTEGER
CATEGORY ::= INTEGER
;; ASCII character codes of alphabet ('A' .. 'Z' 'a' .. 'z').
;; Ex: CATEGORY-TABLE
;; (category
;;   (0x0900 0x097F	?E)	; All Devanagari characters
;;   (0x093C		?N))	; DEVANAGARI-LETTER NUKTA
;;	Assign the category 'E' to all Devanagari characters but 0x093C,
;;	assign the category 'N' to 0x093C.

FONT-LAYOUT-RULE ::=
	'(' 'generator' RULE MACRO-DEF * ')'

RULE ::= COMMAND | REGEXP-RULE | MATCH-RULE | MAP-RULE
	 | COND-STRUCT | MACRO-NAME

COMMAND ::=
	DIRECT-CODE | COMBINING | PREDEFIND-COMMAND | OTF-COMMAND

DIRECT-CODE ::= INTEGER
;; Always succeed.  Produce the code.  Consume no source.

PREDEFIND-COMMAND ::=
	'=' | '*' | '<' | '>' | '|'

;; '=': Succeed when the current run contains at least one code.
;; Consume the first code in the current run, and produce it as is.

;; '*': If the the previous command succeeded, repeat it until it
;; fails.  

;; '<': Produce a special code that indicates the start of grapheme
;; cluster.  Succeed always, consume nothing.

;; '>': Produce a special code that indicates the end of grapheme
;; cluster.  Succeed always, consume nothing.

;; '|': Produce a special code whose category is ' '.  Succeed always,
;; consume nothing.

OTF-COMMAND ::=
	'otf:''SCRIPT'[':'['LANGSYS'][':'[GSUB-FEATURES][':'GPOS-FEATURES]]]
;; Run the Open Type Layout Table on the current run.  Succeed always,
;; consume nothing.

SCRIPT ::= OTF-TAG
;;	OTF's ScriptTag name (four letters) listed at:
;;	<http://www.microsoft.om/typograph/otspec/scripttags.htm>
LANGSYS ::= OTF-TAG
;;	OTF's Language System name (four letters) listed at:
;;	<http://www.microsoft.om/typograph/otspec/languagetags.htm>

GSUB-FEATURES ::= [FEATURE[,FEATURE]*] | ' '
GPOS-FEATURES ::= [FEATURE[,FEATURE]*] | ' '
FEATURE ::= OTF-TAG
;;	OTF's Feature name (four letters) listed at:
;;	<http://www.microsoft.om/typograph/otspec/???.htm>

OTF-TAG ::= PRINTABLE-CHAR PRINTABLE-CHAR PRINTABLE-CHAR PRINTABLE-CHAR

;; Ex. OTF-COMMAND
;; 'otf:deva'
;;	Run all features in the default langsys of 'deva' script.
;; 'otf:deva::nukt:haln'
;;	Run all GSUB features, run 'nukt' and 'haln' GPOS features.
;; 'otf:deva:: :'
;;	Run all GSUB features, run no GPOS features.

REGEXP-RULE ::=
	'(' REGEXP RULE * ')'

;; Succeed if REGXP matches the head of source.  Run RULEs while
;; limiting the source to the matching part.  Consume that part.

REGEXP ::= STRING
;; Must be composed only from ASCII characters.  'A' - 'Z', 'a' - 'z'
;; correspond to CATEGORY.

;; Ex: REGEXP-RULE
;; ("VA?"
;;   < | vowel * | >)

MATCH-RULE ::=
	'(' MATCH-IDX RULE * ')'

;; Succeed if the previous REGEXP-RULE found a matching part for
;; MATCH-IDX.  Run RULEs while limiting the source to the matching
;; part.  If MATCH-IDX is zero, consume the whole part, else consume
;; nothing.

MATCH-IDX ::= INTEGER
;; Must be 0..20.

;; Ex. MATCH-RULE
;; (2 consonant *)

MAP-RULE ::=
	'(' ( SOURCE-SEQ | SOURCE-RANGE ) RULE * ')'

;; Succeed if the source matches SOURCE-SEQ or SOURCE-RANGE.  Run
;; RULEs while limiting the source to the matching part.  Consume that
;; part.

SOURCE-SEQ ::=
	'(' CODE + ')'
SOURCE-RANGE ::=
	'(' 'range' CODE CODE ')'
;; Ex. MAP-RULE
;; ((0x0915 0x094D)		0x43)
;;	If the source code sequence is 0x0915 0x094D, produce 0x43.
;; ((range 0x0F40 0x0F6A)	0x2221)
;;	If the first source code CODE is in the range 0x0F40..0x0F6A, 
;;	produce (0x2221 + (CODE - 0x0F40)).

COND-STRUCT ::=
	'(' 'cond' RULE + ')'

;; Try each rule in sequence until one succeeds.  Succeed if one
;; succeeds.  Consume nothing.

;; Ex. COND-STRUCT
;; (cond
;;  ((0x0915 0x094D)		0x43)
;;  ((range 0x0F40 0x0F6A)	0x2221)
;;  = )

COMBINING ::= 'V''H''O''V''H'
V ::= ( 't' | 'c' | 'b' | 'B' )
H ::= ( 'l' | 'c' | 'r' )
O ::= ( '.' | XOFF | YOFF | XOFF YOFF )
XOFF ::= '<'INTEGER | '>'INTEGER 
YOFF ::= '+'INTEGER | '-'INTEGER
;; INTEGER must be integer 0..127

;; VH pair indicates 12 reference points of a glyph as below:
;;
;;   0----1----2 <---- ascent	 0:tl (top-left)
;;   |         |		 1:tc (top-center)
;;   |         |		 2:tr (top-right)
;;   |         |		 3:Bl (base-left)
;;   9   10   11 <---- center	 4:Bc (base-center)
;;   |         |		 5:Br (base-right)
;; --3----4----5-- <-- baseline	 6:bl (bottom-left)
;;   |         |		 7:bc (bottom-center)
;;   6----7----8 <---- descent	 8:br (bottom-right)
;;				 9:cl (center-left)
;;   |    |    |		10:cc (center-center)
;; left center right		11:cr (center-right)
;;
;; Ex. COMBINING
;; 'tc.bc':
;;	Align top-left point of the previous glyph and bottom-center
;;	point of the current glyph.
;; 'Bl<20-10Br'
;;	Align 20% left and 10% below of base-left point of the previous
;;	glyph and base-right point of the current glyph.

MACRO-DEF ::=
	'(' MACRO-NAME RULE + ')'
MACRO-NAME ::= SYMBOL

*/

static int mdebug_mask = MDEBUG_FONT_FLT;

MSymbol Mlayouter;

static MPlist *flt_list;

/* Command ID:
	         0 ...		: direct code
	           -1		: invalid
	     -0x0F .. -2	: builtin commands
	-0x100000F .. -0x10	: combining code
	          ... -0x1000010: index to FontLayoutStage->cmds
 */

#define INVALID_CMD_ID -1
#define CMD_ID_OFFSET_BUILTIN	-2
#define CMD_ID_OFFSET_COMBINING	-0x10
#define CMD_ID_OFFSET_INDEX	-0x1000010

/* Builtin commands. */
#define CMD_ID_COPY		-2 /* '=' */
#define CMD_ID_REPEAT		-3 /* '*' */
#define CMD_ID_CLUSTER_BEGIN	-4 /* '<' */
#define CMD_ID_CLUSTER_END	-5 /* '>' */
#define CMD_ID_SEPARATOR	-6 /* '|' */
#define CMD_ID_LEFT_PADDING	-7 /* '[' */
#define CMD_ID_RIGHT_PADDING	-8 /* ']' */

#define CMD_ID_TO_COMBINING_CODE(id) (CMD_ID_OFFSET_COMBINING - (id))
#define COMBINING_CODE_TO_CMD_ID(code) (CMD_ID_OFFSET_COMBINING - (code))

#define CMD_ID_TO_INDEX(id) (CMD_ID_OFFSET_INDEX - (id))
#define INDEX_TO_CMD_ID(idx) (CMD_ID_OFFSET_INDEX - (idx))

static MSymbol Mcond, Mrange;

#define GLYPH_CODE_P(code)	\
  ((code) >= GLYPH_CODE_MIN && (code) <= GLYPH_CODE_MAX)

#define GLYPH_CODE_INDEX(code) ((code) - GLYPH_CODE_MIN)

enum FontLayoutCmdRuleSrcType
  {
    SRC_REGEX,
    SRC_INDEX,
    SRC_SEQ,
    SRC_RANGE
  };

typedef struct
{
  enum FontLayoutCmdRuleSrcType src_type;
  union {
    struct {
      char *pattern;
      regex_t preg;
    } re;
    int match_idx;
    struct {
      int n_codes;
      int *codes;
    } seq;
    struct {
      int from, to;
    } range;
  } src;

  int n_cmds;
  int *cmd_ids;
} FontLayoutCmdRule;

typedef struct
{
  /* Beginning and end indices of series of SEQ commands.  */
  int seq_beg, seq_end;
  /* Range of the first character appears in the above series.  */
  int seq_from, seq_to;

  int n_cmds;
  int *cmd_ids;
} FontLayoutCmdCond;

enum FontLayoutCmdType
  {
    FontLayoutCmdTypeRule,
    FontLayoutCmdTypeCond,
    FontLayoutCmdTypeOTF,
    FontLayoutCmdTypeMAX
  };

typedef struct
{
  enum FontLayoutCmdType type;
  union {
    FontLayoutCmdRule rule;
    FontLayoutCmdCond cond;
    FontLayoutCmdOTF otf;
  } body;
} FontLayoutCmd;

typedef struct 
{
  MCharTable *category;
  int size, inc, used;
  FontLayoutCmd *cmds;
} FontLayoutStage;

typedef MPlist MFontLayoutTable; /* t vs FontLayoutStage */

/* Font layout table loader */

/* Load a category table from PLIST.  PLIST has this form:
      PLIST ::= ( FROM-CODE TO-CODE ? CATEGORY-CHAR ) *
*/

static MCharTable *
load_category_table (MPlist *plist)
{
  MCharTable *table;

  table = mchartable (Minteger, (void *) 0);

  MPLIST_DO (plist, plist)
    {
      MPlist *elt;
      int from, to, category_code;

      if (! MPLIST_PLIST (plist))
	MERROR (MERROR_FONT, NULL);
      elt = MPLIST_PLIST (plist);
      if (! MPLIST_INTEGER_P (elt))
	MERROR (MERROR_FONT, NULL);
      from = MPLIST_INTEGER (elt);
      elt = MPLIST_NEXT (elt);
      if (! MPLIST_INTEGER_P (elt))
	MERROR (MERROR_FONT, NULL);
      to = MPLIST_INTEGER (elt);
      elt = MPLIST_NEXT (elt);
      if (MPLIST_TAIL_P (elt))
	{
	  category_code = to;
	  to = from;
	}
      else
	{
	  if (! MPLIST_INTEGER_P (elt))
	    MERROR (MERROR_FONT, NULL);
	  category_code = MPLIST_INTEGER (elt);
	}
      if (! isalpha (category_code))
	MERROR (MERROR_FONT, NULL);

      if (from == to)
	mchartable_set (table, from, (void *) category_code);
      else
	mchartable_set_range (table, from, to, (void *) category_code);
    }

  return table;
}


/* Parse OTF command name NAME and store the result in CMD.
   NAME has this form:
	:SCRIPT[/[LANGSYS][=[GSUB-FEATURES][+GPOS-FEATURES]]]
   where GSUB-FEATURES and GPOS-FEATURES have this form:
	[FEATURE[,FEATURE]*] | ' '  */

static int
load_otf_command (FontLayoutCmd *cmd, char *name)
{
  char *p = name, *beg;

  cmd->type = FontLayoutCmdTypeOTF;
  cmd->body.otf.script = cmd->body.otf.langsys = Mnil;
  cmd->body.otf.gsub_features = cmd->body.otf.gpos_features = Mt;

  while (*p)
    {
      if (*p == ':')
	{
	  for (beg = ++p; *p && *p != '/' && *p != '=' && *p != '+'; p++);
	  if (beg < p)
	    cmd->body.otf.script = msymbol__with_len (beg, p - beg);
	}
      else if (*p == '/')
	{
	  for (beg = ++p; *p && *p != '=' && *p != '+'; p++);
	  if (beg < p)
	    cmd->body.otf.langsys = msymbol__with_len (beg, p - beg);
	}
      else if (*p == '=')
	{
	  for (beg = ++p; *p && *p != '+'; p++);
	  if (beg < p)
	    cmd->body.otf.gsub_features = msymbol__with_len (beg, p - beg);
	  else
	    cmd->body.otf.gsub_features = Mnil;
	}
      else if (*p == '+')
	{
	  for (beg = ++p; *p && *p != '+'; p++);
	  if (beg < p)
	    cmd->body.otf.gpos_features = msymbol__with_len (beg, p - beg);
	  else
	    cmd->body.otf.gpos_features = Mnil;
	}
      else
	p++;
    }

  return (cmd->body.otf.script == Mnil ? -1 : 0);
}


/* Read a decimal number from STR preceded by one of "+-><".  '+' and
   '>' means a plus sign, '-' and '<' means a minus sign.  If the
   number is greater than 127, limit it to 127.  */

static int
read_decimal_number (char **str)
{
  char *p = *str;
  int sign = (*p == '-' || *p == '<') ? -1 : 1;
  int n = 0;

  p++;
  while (*p >= '0' && *p <= '9')
    n = n * 10 + *p++ - '0';
  *str = p;
  if (n == 0)
    n = 5;
  return (n < 127 ? n * sign : 127 * sign);
}


/* Read a horizontal and vertical combining positions from STR, and
   store them in the place pointed by X and Y.  The horizontal
   position left, center, and right are represented by 0, 1, and 2
   respectively.  The vertical position top, center, bottom, and base
   are represented by 0, 1, 2, and 3 respectively.  If successfully
   read, return 0, else return -1.  */

static int
read_combining_position (char *str, int *x, int *y)
{
  int c = *str++;
  int i;

  /* Vertical position comes first.  */
  for (i = 0; i < 4; i++)
    if (c == "tcbB"[i])
      {
	*y = i;
	break;
      }
  if (i == 4)
    return -1;
  c = *str;
  /* Then comse horizontal position.  */
  for (i = 0; i < 3; i++)
    if (c == "lcr"[i])
      {
	*x = i;
	return 0;
      }
  return -1;
}


/* Return a combining code corresponding to SYM.  */

static int
get_combining_command (MSymbol sym)
{
  char *str = msymbol_name (sym);
  int base_x, base_y, add_x, add_y, off_x, off_y;
  int c;

  if (read_combining_position (str, &base_x, &base_y) < 0)
    return 0;
  str += 2;
  c = *str;
  if (c == '.')
    {
      off_x = off_y = 128;
      str++;
    }
  else
    {
      if (c == '+' || c == '-')
	{
	  off_y = read_decimal_number (&str) + 128;
	  c = *str;
	}
      else
	off_y = 128;
      if (c == '<' || c == '>')
	off_x = read_decimal_number (&str) + 128;
      else
	off_x = 128;
    }
  if (read_combining_position (str, &add_x, &add_y) < 0)
    return 0;

  c = MAKE_COMBINING_CODE (base_y, base_x, add_y, add_x, off_y, off_x);
  return (COMBINING_CODE_TO_CMD_ID (c));
}


/* Load a command from PLIST into STAGE, and return that
   identification number.  If ID is not INVALID_CMD_ID, that means we
   are loading a top level command or a macro.  In that case, use ID
   as the identification number of the command.  Otherwise, generate a
   new id number for the command.  MACROS is a list of raw macros.  */

static int
load_command (FontLayoutStage *stage, MPlist *plist,
	      MPlist *macros, int id)
{
  int i;

  if (MPLIST_INTEGER_P (plist))
    {
      int code = MPLIST_INTEGER (plist);

      if (code < 0)
	MERROR (MERROR_DRAW, INVALID_CMD_ID);
      return code;
    }
  else if (MPLIST_PLIST_P (plist))
    {
      /* PLIST ::= ( cond ... ) | ( STRING ... ) | ( INTEGER ... )
 	           | ( ( INTEGER INTEGER ) ... )
		   | ( ( range INTEGER INTEGER ) ... )  */
      MPlist *elt = MPLIST_PLIST (plist);
      int len = MPLIST_LENGTH (elt) - 1;
      FontLayoutCmd *cmd;

      if (id == INVALID_CMD_ID)
	{
	  FontLayoutCmd dummy;
	  id = INDEX_TO_CMD_ID (stage->used);
	  MLIST_APPEND1 (stage, cmds, dummy, MERROR_DRAW);
	}
      cmd = stage->cmds + CMD_ID_TO_INDEX (id);

      if (MPLIST_SYMBOL_P (elt))
	{
	  FontLayoutCmdCond *cond;

	  if (MPLIST_SYMBOL (elt) != Mcond)
	    MERROR (MERROR_DRAW, INVALID_CMD_ID);
	  elt = MPLIST_NEXT (elt);
	  cmd->type = FontLayoutCmdTypeCond;
	  cond = &cmd->body.cond;
	  cond->seq_beg = cond->seq_end = -1;
	  cond->seq_from = cond->seq_to = 0;
	  cond->n_cmds = len;
	  MTABLE_CALLOC (cond->cmd_ids, len, MERROR_DRAW);
	  for (i = 0; i < len; i++, elt = MPLIST_NEXT (elt))
	    {
	      int this_id = load_command (stage, elt, macros, INVALID_CMD_ID);

	      if (this_id == INVALID_CMD_ID)
		MERROR (MERROR_DRAW, INVALID_CMD_ID);
	      /* The above load_command may relocate stage->cmds.  */
	      cmd = stage->cmds + CMD_ID_TO_INDEX (id);
	      cond = &cmd->body.cond;
	      cond->cmd_ids[i] = this_id;
	      if (this_id <= CMD_ID_OFFSET_INDEX)
		{
		  FontLayoutCmd *this_cmd
		    = stage->cmds + CMD_ID_TO_INDEX (this_id);

		  if (this_cmd->type == FontLayoutCmdTypeRule
		      && this_cmd->body.rule.src_type == SRC_SEQ)
		    {
		      int first_char = this_cmd->body.rule.src.seq.codes[0];

		      if (cond->seq_beg < 0)
			{
			  /* The first SEQ command.  */
			  cond->seq_beg = i;
			  cond->seq_from = cond->seq_to = first_char;
			}
		      else if (cond->seq_end < 0)
			{
			  /* The following SEQ command.  */
			  if (cond->seq_from > first_char)
			    cond->seq_from = first_char;
			  else if (cond->seq_to < first_char)
			    cond->seq_to = first_char;
			}
		    }
		  else
		    {
		      if (cond->seq_beg >= 0 && cond->seq_end < 0)
			/* The previous one is the last SEQ command.  */
			cond->seq_end = i;
		    }
		}
	      else
		{
		  if (cond->seq_beg >= 0 && cond->seq_end < 0)
		    /* The previous one is the last SEQ command.  */
		    cond->seq_end = i;
		}
	    }
	  if (cond->seq_beg >= 0 && cond->seq_end < 0)
	    /* The previous one is the last SEQ command.  */
	    cond->seq_end = i;
	}
      else
	{
	  cmd->type = FontLayoutCmdTypeRule;
	  if (MPLIST_MTEXT_P (elt))
	    {
	      char *str = (char *) MTEXT_DATA (MPLIST_MTEXT (elt));

	      if (regcomp (&cmd->body.rule.src.re.preg, str, REG_EXTENDED))
		MERROR (MERROR_FONT, INVALID_CMD_ID);
	      cmd->body.rule.src_type = SRC_REGEX;
	      cmd->body.rule.src.re.pattern = strdup (str);
	    }
	  else if (MPLIST_INTEGER_P (elt))
	    {
	      cmd->body.rule.src_type = SRC_INDEX;
	      cmd->body.rule.src.match_idx = MPLIST_INTEGER (elt);
	    }
	  else if (MPLIST_PLIST_P (elt))
	    {
	      MPlist *pl = MPLIST_PLIST (elt);
	      int size = MPLIST_LENGTH (pl);

	      if (MPLIST_INTEGER_P (pl))
		{
		  int i;

		  cmd->body.rule.src_type = SRC_SEQ;
		  cmd->body.rule.src.seq.n_codes = size;
		  MTABLE_CALLOC (cmd->body.rule.src.seq.codes, size,
				 MERROR_FONT);
		  for (i = 0; i < size; i++, pl = MPLIST_NEXT (pl))
		    {
		      if (! MPLIST_INTEGER_P (pl))
			MERROR (MERROR_DRAW, INVALID_CMD_ID);
		      cmd->body.rule.src.seq.codes[i]
			= (unsigned) MPLIST_INTEGER (pl);
		    }
		}
	      else if (MPLIST_SYMBOL_P (pl) && size == 3)
		{
		  cmd->body.rule.src_type = SRC_RANGE;
		  pl = MPLIST_NEXT (pl);
		  if (! MPLIST_INTEGER_P (pl))
		    MERROR (MERROR_DRAW, INVALID_CMD_ID);
		  cmd->body.rule.src.range.from
		    = (unsigned) MPLIST_INTEGER (pl);
		  pl = MPLIST_NEXT (pl);
		  if (! MPLIST_INTEGER_P (pl))
		    MERROR (MERROR_DRAW, INVALID_CMD_ID);
		  cmd->body.rule.src.range.to
		    = (unsigned) MPLIST_INTEGER (pl);
		}
	      else
		MERROR (MERROR_DRAW, INVALID_CMD_ID);
	    }
	  else
	    MERROR (MERROR_DRAW, INVALID_CMD_ID);

	  elt = MPLIST_NEXT (elt);
	  cmd->body.rule.n_cmds = len;
	  MTABLE_CALLOC (cmd->body.rule.cmd_ids, len, MERROR_DRAW);
	  for (i = 0; i < len; i++, elt = MPLIST_NEXT (elt))
	    {
	      int this_id = load_command (stage, elt, macros, INVALID_CMD_ID);

	      if (this_id == INVALID_CMD_ID)
		MERROR (MERROR_DRAW, INVALID_CMD_ID);
	      /* The above load_command may relocate stage->cmds.  */
	      cmd = stage->cmds + CMD_ID_TO_INDEX (id);
	      cmd->body.rule.cmd_ids[i] = this_id;
	    }
	}
    }
  else if (MPLIST_SYMBOL_P (plist))
    {
      MPlist *elt;
      MSymbol sym = MPLIST_SYMBOL (plist);
      char *name = msymbol_name (sym);
      int len = strlen (name);
      FontLayoutCmd cmd;

      if (len > 4
	  && ! strncmp (name, "otf:", 4)
	  && load_otf_command (&cmd, name + 3) >= 0)
	{
	  if (id == INVALID_CMD_ID)
	    {
	      id = INDEX_TO_CMD_ID (stage->used);
	      MLIST_APPEND1 (stage, cmds, cmd, MERROR_DRAW);
	    }
	  else
	    stage->cmds[CMD_ID_TO_INDEX (id)] = cmd;
	  return id;
	}

      if (len == 1)
	{
	  if (*name == '=')
	    return CMD_ID_COPY;
	  else if (*name == '*')
	    return CMD_ID_REPEAT;
	  else if (*name == '<')
	    return CMD_ID_CLUSTER_BEGIN;
	  else if (*name == '>')
	    return CMD_ID_CLUSTER_END;
	  else if (*name == '|')
	    return CMD_ID_SEPARATOR;
	  else if (*name == '[')
	    return CMD_ID_LEFT_PADDING;
	  else if (*name == ']')
	    return CMD_ID_RIGHT_PADDING;
	  else
	    id = 0;
	}
      else
	{
	  id = get_combining_command (sym);
	  if (id)
	    return id;
	}

      i = 1;
      MPLIST_DO (elt, macros)
	{
	  if (sym == MPLIST_SYMBOL (MPLIST_PLIST (elt)))
	    {
	      id = INDEX_TO_CMD_ID (i);
	      if (stage->cmds[i].type == FontLayoutCmdTypeMAX)
		id = load_command (stage, MPLIST_NEXT (MPLIST_PLIST (elt)),
				   macros, id);
	      return id;
	    }
	  i++;
	}
      MERROR (MERROR_DRAW, INVALID_CMD_ID);
    }
  else
    MERROR (MERROR_DRAW, INVALID_CMD_ID);

  return id;
}

static void
free_flt_command (FontLayoutCmd *cmd)
{
  if (cmd->type == FontLayoutCmdTypeRule)
    {
      FontLayoutCmdRule *rule = &cmd->body.rule;

      if (rule->src_type == SRC_REGEX)
	{
	  free (rule->src.re.pattern);
	  regfree (&rule->src.re.preg);
	}
      else if (rule->src_type == SRC_SEQ)
	free (rule->src.seq.codes);
      free (rule->cmd_ids);
    }
  else if (cmd->type == FontLayoutCmdTypeCond)
    free (cmd->body.cond.cmd_ids);
}

/* Load a generator from PLIST into a newly allocated FontLayoutStage,
   and return it.  PLIST has this form:
      PLIST ::= ( COMMAND ( CMD-NAME COMMAND ) * )
*/

static FontLayoutStage *
load_generator (MPlist *plist)
{
  FontLayoutStage *stage;
  MPlist *elt, *pl;
  FontLayoutCmd dummy;

  MSTRUCT_CALLOC (stage, MERROR_DRAW);
  MLIST_INIT1 (stage, cmds, 32);
  dummy.type = FontLayoutCmdTypeMAX;
  MLIST_APPEND1 (stage, cmds, dummy, MERROR_FONT);
  MPLIST_DO (elt, MPLIST_NEXT (plist))
    {
      if (! MPLIST_PLIST_P (elt))
	MERROR (MERROR_FONT, NULL);
      pl = MPLIST_PLIST (elt);
      if (! MPLIST_SYMBOL_P (pl))
	MERROR (MERROR_FONT, NULL);
      MLIST_APPEND1 (stage, cmds, dummy, MERROR_FONT);
    }

  /* Load the first command from PLIST into STAGE->cmds[0].  Macros
     called in the first command are also loaded from MPLIST_NEXT
     (PLIST) into STAGE->cmds[n].  */
  if (load_command (stage, plist, MPLIST_NEXT (plist), INDEX_TO_CMD_ID (0))
      == INVALID_CMD_ID)
    {
      MLIST_FREE1 (stage, cmds);
      free (stage);
      MERROR (MERROR_DRAW, NULL);
    }

  return stage;
}


/* Load FLT of name LAYOUTER_NAME from the m17n database into a newly
   allocated memory, and return it.  */

static MFontLayoutTable *
load_flt (MSymbol layouter_name)
{
  MDatabase *mdb;
  MPlist *top = NULL, *plist;
  MSymbol Mcategory = msymbol ("category");
  MSymbol Mgenerator = msymbol ("generator");
  MSymbol Mend = msymbol ("end");
  MFontLayoutTable *layouter = NULL;
  MCharTable *category = NULL;

  if (! (mdb = mdatabase_find (Mfont, Mlayouter, layouter_name, Mnil)))
    MERROR_GOTO (MERROR_FONT, finish);
  if (! (top = (MPlist *) mdatabase_load (mdb)))
    MERROR_GOTO (0, finish);
  if (! MPLIST_PLIST_P (top))
    MERROR_GOTO (MERROR_FONT, finish);

  MPLIST_DO (plist, top)
    {
      MSymbol sym;
      MPlist *elt;

      if (MPLIST_SYMBOL_P (plist)
	  && MPLIST_SYMBOL (plist) == Mend)
	break;
      if (! MPLIST_PLIST (plist))
	MERROR_GOTO (MERROR_FONT, finish);
      elt = MPLIST_PLIST (plist);
      if (! MPLIST_SYMBOL_P (elt))
	MERROR_GOTO (MERROR_FONT, finish);
      sym = MPLIST_SYMBOL (elt);
      elt = MPLIST_NEXT (elt);
      if (! elt)
	MERROR_GOTO (MERROR_FONT, finish);
      if (sym == Mcategory)
	{
	  if (category)
	    M17N_OBJECT_UNREF (category);
	  category = load_category_table (elt);
	}
      else if (sym == Mgenerator)
	{
	  FontLayoutStage *stage;

	  if (! category)
	    MERROR_GOTO (MERROR_FONT, finish);
	  stage = load_generator (elt);
	  if (! stage)
	    MERROR_GOTO (MERROR_FONT, finish);
	  stage->category = category;
	  M17N_OBJECT_REF (category);
	  if (! layouter)
	    {
	      layouter = mplist ();
	      /* Here don't do M17N_OBJECT_REF (category) because we
		 don't unref the value of the element added below.  */
	      mplist_add (layouter, Mcategory, category);
	    }
	  mplist_add (layouter, Mt, stage);
	}
      else
	MERROR_GOTO (MERROR_FONT, finish);
    }

  if (category)
    M17N_OBJECT_UNREF (category);

 finish:
  M17N_OBJECT_UNREF (top);
  mplist_add (flt_list, layouter_name, layouter);
  return layouter;
}


static void
free_flt_stage (FontLayoutStage *stage)
{
  int i;

  M17N_OBJECT_UNREF (stage->category);
  for (i = 0; i < stage->used; i++)
    free_flt_command (stage->cmds + i);
  MLIST_FREE1 (stage, cmds);
  free (stage);
}


static MFontLayoutTable *
get_font_layout_table (MSymbol layouter_name)
{
  MPlist *plist = mplist_find_by_key (flt_list, layouter_name);

  return (plist ? MPLIST_VAL (plist) : load_flt (layouter_name));
}


/* FLS (Font Layout Service) */

/* Structure to hold information about a context of FLS.  */

typedef struct
{
  /* Pointer to the current stage.  */
  FontLayoutStage *stage;

  /* Encode each MGlyph->code by the current category table into this
     array.  An element is a category.  */
  char *encoded;
  /* <encoded>[GIDX - <encoded_offset>] gives a category for the glyph
     index GIDX.  */
  int encoded_offset;
  int *match_indices;
  int code_offset;
  int cluster_begin_idx;
  int cluster_begin_pos;
  int cluster_end_pos;
  int combining_code;
  int left_padding;
} FontLayoutContext;

static int run_command (int depth,
			int, MGlyphString *, int, int, FontLayoutContext *);

#define NMATCH 20

static int
run_rule (int depth,
	  FontLayoutCmdRule *rule, MGlyphString *gstring, int from, int to,
	  FontLayoutContext *ctx)
{
  int *saved_match_indices = ctx->match_indices;
  int match_indices[NMATCH * 2];
  int consumed;
  int i;
  int orig_from = from;

  if (ctx->cluster_begin_idx)
    {
      if (ctx->cluster_begin_pos > MGLYPH (from)->pos)
	ctx->cluster_begin_pos = MGLYPH (from)->pos;
      if (ctx->cluster_end_pos < MGLYPH (to)->pos)
	ctx->cluster_end_pos = MGLYPH (to)->pos;
    }

  if (rule->src_type == SRC_SEQ)
    {
      int len;

      len = rule->src.seq.n_codes;
      if (len > (to - from))
	return 0;
      for (i = 0; i < len; i++)
	if (rule->src.seq.codes[i] != gstring->glyphs[from + i].code)
	  break;
      if (i < len)
	return 0;
      to = from + len;
      MDEBUG_PRINT1 (" (SEQ 0x%X", rule->src.seq.codes[0]);
    }
  else if (rule->src_type == SRC_RANGE)
    {
      int head;

      if (from >= to)
	return 0;
      head = gstring->glyphs[from].code;
      if (head < rule->src.range.from || head > rule->src.range.to)
	return 0;
      ctx->code_offset = head - rule->src.range.from;
      to = from + 1;
      MDEBUG_PRINT2 (" (RANGE 0x%X-0x%X",
		     rule->src.range.from, rule->src.range.to);
    }
  else if (rule->src_type == SRC_REGEX)
    {
      regmatch_t pmatch[NMATCH];
      char saved_code;
      int result;

      if (from > to)
	return 0;
      saved_code = ctx->encoded[to - ctx->encoded_offset];
      ctx->encoded[to - ctx->encoded_offset] = '\0';
      result = regexec (&(rule->src.re.preg),
			ctx->encoded + from - ctx->encoded_offset,
			NMATCH, pmatch, 0);
      if (result == 0 && pmatch[0].rm_so == 0)
	{
	  MDEBUG_PRINT3 (" (REGEX \"%s\" \"%s\" %d",
			 rule->src.re.pattern,
			 ctx->encoded + from - ctx->encoded_offset,
			 pmatch[0].rm_eo);
	  ctx->encoded[to - ctx->encoded_offset] = saved_code;
	  for (i = 0; i < NMATCH; i++)
	    {
	      if (pmatch[i].rm_so < 0)
		match_indices[i * 2] = match_indices[i * 2 + 1] = -1;
	      else
		{
		  match_indices[i * 2] = from + pmatch[i].rm_so;
		  match_indices[i * 2 + 1] = from + pmatch[i].rm_eo;
		}
	    }
	  ctx->match_indices = match_indices;
	  to = match_indices[1];
	}
      else
	{
	  ctx->encoded[to - ctx->encoded_offset] = saved_code;
	  return 0;
	}
    }
  else if (rule->src_type == SRC_INDEX)
    {
      if (rule->src.match_idx >= NMATCH)
	return 0;
      from = ctx->match_indices[rule->src.match_idx * 2];
      if (from < 0)
	return 0;
      to = ctx->match_indices[rule->src.match_idx * 2 + 1];
      MDEBUG_PRINT1 (" (INDEX %d", rule->src.match_idx);
    }

  consumed = 0;
  depth++;
  for (i = 0; i < rule->n_cmds; i++)
    {
      int pos;

      if (rule->cmd_ids[i] == CMD_ID_REPEAT)
	{
	  if (! consumed)
	    continue;
	  i--;
	}
      pos = run_command (depth, rule->cmd_ids[i], gstring, from, to, ctx);
      if (pos < 0)
	MERROR (MERROR_DRAW, -1);
      consumed = pos > from;
      if (consumed)
	from = pos;
    }

  ctx->match_indices = saved_match_indices;
  MDEBUG_PRINT (")");
  return (rule->src_type == SRC_INDEX ? orig_from : to);
}

static int
run_cond (int depth,
	  FontLayoutCmdCond *cond, MGlyphString *gstring, int from, int to,
	  FontLayoutContext *ctx)
{
  int i, pos = 0;

  MDEBUG_PRINT2 ("\n [FLT] %*s(COND", depth, "");
  depth++;
  for (i = 0; i < cond->n_cmds; i++)
    {
      /* TODO: Write a code for optimization utilizaing the info
	 cond->seq_XXX.  */
      if ((pos = run_command (depth, cond->cmd_ids[i], gstring, from, to, ctx))
	  != 0)
	break;
    }
  if (pos < 0)
    MERROR (MERROR_DRAW, -1);
  MDEBUG_PRINT (")");
  return (pos);
}

static int
run_otf (int depth,
	 FontLayoutCmdOTF *otf_cmd, MGlyphString *gstring, int from, int to,
	 FontLayoutContext *ctx)
{
#ifdef HAVE_OTF
  int gidx = gstring->used;
  MGlyph *g = MGLYPH (from), *gend = MGLYPH (to);

  for (; g < gend; g++)
    g->otf_cmd = otf_cmd;

  to = mfont__ft_drive_gsub (gstring, from, to);
  if (gidx < gstring->used)
    MGLYPH (gidx)->left_padding = ctx->left_padding;
#endif
  return to;
}

static int
run_command (int depth, int id, MGlyphString *gstring, int from, int to,
	     FontLayoutContext *ctx)
{
  MGlyph g;

  if (id >= 0)
    {
      int i;

      /* Direct code (== id + ctx->code_offset) output.
	 The source is not consumed.  */
      if (from < to)
	g = *(MGLYPH (from));
      else
	g = *(MGLYPH (from - 1));
      g.type = GLYPH_CHAR;
      g.code = ctx->code_offset + id;
      MDEBUG_PRINT1 (" (DIRECT 0x%X", g.code);
      if (ctx->combining_code)
	g.combining_code = ctx->combining_code;
      if (ctx->left_padding)
	g.left_padding = ctx->left_padding;
      for (i = from; i < to; i++)
	{
	  MGlyph *tmp = MGLYPH (i);

	  if (g.pos > tmp->pos)
	    g.pos = tmp->pos;
	  else if (g.to < tmp->to)
	    g.to = tmp->to;
	}
      APPEND_GLYPH (gstring, g);
      ctx->code_offset = ctx->combining_code = ctx->left_padding = 0;
      MDEBUG_PRINT (")");
      return (from);
    }

  if (id <= CMD_ID_OFFSET_INDEX)
    {
      int idx = CMD_ID_TO_INDEX (id);
      FontLayoutCmd *cmd;

      if (idx >= ctx->stage->used)
	MERROR (MERROR_DRAW, -1);
      cmd = ctx->stage->cmds + idx;
      if (cmd->type == FontLayoutCmdTypeRule)
	to = run_rule (depth, &cmd->body.rule, gstring, from, to, ctx);
      else if (cmd->type == FontLayoutCmdTypeCond)
	to = run_cond (depth, &cmd->body.cond, gstring, from, to, ctx);
      else if (cmd->type == FontLayoutCmdTypeOTF)
	to = run_otf (depth, &cmd->body.otf, gstring, from, to, ctx);

      if (to < 0)
	return -1;
      return to;
    }

  if (id <= CMD_ID_OFFSET_COMBINING)
    {
      ctx->combining_code = CMD_ID_TO_COMBINING_CODE (id);
      return from;
    }

  switch (id)
    {
    case CMD_ID_COPY:
      {
	if (from >= to)
	  return from;
	g = *(MGLYPH (from));
	if (ctx->combining_code)
	  g.combining_code = ctx->combining_code;
	if (ctx->left_padding)
	  g.left_padding = ctx->left_padding;
	APPEND_GLYPH (gstring, g);
	ctx->code_offset = ctx->combining_code = ctx->left_padding = 0;
	return (from + 1);
      }

    case CMD_ID_CLUSTER_BEGIN:
      if (! ctx->cluster_begin_idx)
	{
	  MDEBUG_PRINT1 (" <%d", MGLYPH (from)->pos);
	  ctx->cluster_begin_idx = gstring->used;
	  ctx->cluster_begin_pos = MGLYPH (from)->pos;
	  ctx->cluster_end_pos = MGLYPH (from)->to;
	}
      return from;

    case CMD_ID_CLUSTER_END:
      if (ctx->cluster_begin_idx && ctx->cluster_begin_idx < gstring->used)
	{
	  int i;

	  MDEBUG_PRINT1 (" %d>", ctx->cluster_end_pos);
	  for (i = ctx->cluster_begin_idx; i < gstring->used; i++)
	    {
	      MGLYPH (i)->pos = ctx->cluster_begin_pos;
	      MGLYPH (i)->to = ctx->cluster_end_pos;
	    }
	  ctx->cluster_begin_idx = 0;
	}
      return from;

    case CMD_ID_SEPARATOR:
      {
	if (from < to)
	  g = *(MGLYPH (from));
	else
	  g = *(MGLYPH (from - 1));
	g.type = GLYPH_PAD;
	/* g.c = g.code = 0; */
	g.width = 0;
	APPEND_GLYPH (gstring, g);
	return from;
      }

    case CMD_ID_LEFT_PADDING:
      ctx->left_padding = 1;
      return from;

    case CMD_ID_RIGHT_PADDING:
      if (gstring->used > 0)
	gstring->glyphs[gstring->used - 1].right_padding = 1;
      return from;
    }

  MERROR (MERROR_DRAW, -1);
}


/* Internal API */

int
mfont__flt_init (void)
{
  Mcond = msymbol ("cond");
  Mrange = msymbol ("range");
  Mlayouter = msymbol ("layouter");
  flt_list = mplist ();
  return 0;
}

void
mfont__flt_fini (void)
{
  MPlist *plist, *pl;

  MPLIST_DO (plist, flt_list)
    {
      pl = MPLIST_PLIST (plist);
      if (pl)
	{
	  MPLIST_DO (pl, MPLIST_NEXT (pl))
	    free_flt_stage (MPLIST_VAL (pl));
	  pl = MPLIST_PLIST (plist);
	  M17N_OBJECT_UNREF (pl);
	}
    }
  M17N_OBJECT_UNREF (flt_list);
}

unsigned
mfont__flt_encode_char (MSymbol layouter_name, int c)
{
  MFontLayoutTable *layouter = get_font_layout_table (layouter_name);
  MCharTable *table;
  unsigned code;

  if (! layouter)
    return MCHAR_INVALID_CODE;
  table = MPLIST_VAL (layouter);
  code = (unsigned) mchartable_lookup (table, c);
  return (code ? code : MCHAR_INVALID_CODE);
}

int
mfont__flt_run (MGlyphString *gstring, int from, int to, MRealizedFace *rface)
{
  int stage_idx = 0;
  int gidx;
  int i;
  FontLayoutContext ctx;
  MCharTable *table;
  int encoded_len;
  int match_indices[NMATCH];
  MSymbol layouter_name = rface->rfont->layouter;
  MFontLayoutTable *layouter = get_font_layout_table (layouter_name);
  MRealizedFace *ascii_rface = rface->ascii_rface;
  FontLayoutStage *stage;
  int from_pos, to_pos;
  MGlyph dummy;

  if (! layouter)
    {
      /* FLT not found.  Make all glyphs invisible.  */
      while (from < to)
	gstring->glyphs[from++].code = MCHAR_INVALID_CODE;
      return to;
    }

  dummy = gstring->glyphs[from];
  MDEBUG_PRINT1 (" [FLT] (%s", msymbol_name (layouter_name));

  /* Setup CTX.  */
  memset (&ctx, 0, sizeof ctx);
  table = MPLIST_VAL (layouter);
  layouter = MPLIST_NEXT (layouter);
  stage = (FontLayoutStage *) MPLIST_VAL (layouter);
  gidx = from;
  /* Find previous glyphs that are also supported by the layouter.  */
  while (gidx > 1
	 && mchartable_lookup (table, MGLYPH (gidx - 1)->c))
    gidx--;
  /* + 2 is for a separator ' ' and a terminator '\0'.  */
  encoded_len = gstring->used - gidx + 2;
  ctx.encoded = (char *) alloca (encoded_len);

  for (i = 0; gidx < from; i++, gidx++)
    ctx.encoded[i] = (int) mchartable_lookup (table, MGLYPH (gidx)->c);

  ctx.encoded[i++] = ' ';
  ctx.encoded_offset = from - i;

  /* Now each MGlyph->code contains encoded char.  Set it in
     ctx.encoded[], and set MGlyph->c to MGlyph->code.  */
  for (gidx = from; gidx < to ; i++, gidx++)
    {
      ctx.encoded[i] = (int) MGLYPH (gidx)->code;
      MGLYPH (gidx)->code = (unsigned) MGLYPH (gidx)->c;
    }
  ctx.encoded[i++] = '\0';

  match_indices[0] = from;
  match_indices[1] = to;
  for (i = 2; i < NMATCH; i++)
    match_indices[i] = -1;
  ctx.match_indices = match_indices;

  from_pos = MGLYPH (from)->pos;
  to_pos = MGLYPH (to)->pos;

  for (stage_idx = 0; 1; stage_idx++)
    {
      int len = to - from;
      int result;

      MDEBUG_PRINT1 ("\n [FLT]   (STAGE %d", stage_idx);
      gidx = gstring->used;
      ctx.stage = stage;

      result = run_command (2, INDEX_TO_CMD_ID (0), gstring,
			    ctx.encoded_offset, to, &ctx);
      MDEBUG_PRINT (")");
      if (result < 0)
	return -1;
      to = from + (gstring->used - gidx);
      REPLACE_GLYPHS (gstring, gidx, from, len);

      layouter = MPLIST_NEXT (layouter);
      /* If this is the last stage, break the loop. */
      if (MPLIST_TAIL_P (layouter))
	break;

      /* Otherwise, prepare for the next iteration.   */
      stage = (FontLayoutStage *) MPLIST_VAL (layouter);
      table = stage->category;
      if (to - from >= encoded_len)
	{
	  encoded_len = to + 1;
	  ctx.encoded = (char *) alloca (encoded_len);
	}

      for (i = from; i < to; i++)
	{
	  MGlyph *g = MGLYPH (i);

	  if (g->type == GLYPH_PAD)
	    ctx.encoded[i - from] = ' ';
	  else if (! g->otf_encoded)
	    ctx.encoded[i - from] = (int) mchartable_lookup (table, g->code);
#if defined (HAVE_FREETYPE) && defined (HAVE_OTF)
	  else
	    {
	      int c = mfont__ft_decode_otf (g);

	      if (c >= 0)
		{
		  c = (int) mchartable_lookup (table, c);
		  if (! c)
		    c = -1;
		}
	      ctx.encoded[i - from] = (c >= 0 ? c : 1);
	    }
#endif	/* HAVE_FREETYPE && HAVE_OTF */
	}
      ctx.encoded[i - from] = '\0';
      ctx.encoded_offset = from;
      ctx.match_indices[0] = from;
      ctx.match_indices[1] = to;
    }

  MDEBUG_PRINT (")\n");

  if (from == to)
    {
      /* Somehow there's no glyph contributing to characters between
	 FROM_POS and TO_POS.  We must add one dummy space glyph for
	 those characters.  */
      MGlyph g;

      g.type = GLYPH_SPACE;
      g.c = ' ', g.code = ' ';
      g.pos = from_pos, g.to = to_pos;
      g.rface = ascii_rface;
      INSERT_GLYPH (gstring, from, g);
      to = from + 1;
    }
  else
    {
      /* Get actual glyph IDs of glyphs.  Also check if all characters
	 in the range is covered by some glyph(s).  If not, change
	 <pos> and <to> of glyphs to cover uncovered characters.  */
      int len = to_pos - from_pos;
      int pos;
      MGlyph **glyphs = alloca (sizeof (MGlyph) * len);
      MGlyph *g, *gend = MGLYPH (to);
      MGlyph *latest = gend;

      for (g = MGLYPH (from); g != gend; g++)
	if (g->type == GLYPH_CHAR && ! g->otf_encoded)
	  g->code
	    = (rface->rfont->driver->encode_char) (rface->rfont, g->code);
      for (i = 0; i < len; i++)
	glyphs[i] = NULL;
      for (g = MGLYPH (from); g != gend; g++)
	{
	  if (g->pos < latest->pos)
	    latest = g;
	  if (! glyphs[g->pos - from_pos])
	    {
	      for (i = g->pos; i < g->to; i++)
		glyphs[i - from_pos] = g;
	    }
	}
      i = 0;
      if (! glyphs[0])
	{
	  pos = latest->pos;
	  for (g = latest; g->pos == pos; g++)
	    g->pos = from_pos;
	  i++;
	}
      for (; i < len; i++)
	{
	  if (! glyphs[i])
	    {
	      for (g = latest; g->pos == latest->pos; g++)
		g->to = from_pos + i + 1;
	    }
	  else
	    latest = glyphs[i];
	}
    }
  return to;
}


/* for debugging... */

static void
dump_flt_cmd (FontLayoutStage *stage, int id, int indent)
{
  char *prefix = (char *) alloca (indent + 1);

  memset (prefix, 32, indent);
  prefix[indent] = 0;

  if (id >= 0)
    fprintf (stderr, "0x%02X", id);
  else if (id <= CMD_ID_OFFSET_INDEX)
    {
      int idx = CMD_ID_TO_INDEX (id);
      FontLayoutCmd *cmd = stage->cmds + idx;

      if (cmd->type == FontLayoutCmdTypeRule)
	{
	  FontLayoutCmdRule *rule = &cmd->body.rule;
	  int i;

	  fprintf (stderr, "(rule ");
	  if (rule->src_type == SRC_REGEX)
	    fprintf (stderr, "\"%s\"", rule->src.re.pattern);
	  else if (rule->src_type == SRC_INDEX)
	    fprintf (stderr, "%d", rule->src.match_idx);
	  else if (rule->src_type == SRC_SEQ)
	    fprintf (stderr, "(seq)");
	  else if (rule->src_type == SRC_RANGE)
	    fprintf (stderr, "(range)");
	  else
	    fprintf (stderr, "(invalid src)");

	  for (i = 0; i < rule->n_cmds; i++)
	    {
	      fprintf (stderr, "\n%s  ", prefix);
	      dump_flt_cmd (stage, rule->cmd_ids[i], indent + 2);
	    }
	  fprintf (stderr, ")");
	}
      else if (cmd->type == FontLayoutCmdTypeCond)
	{
	  FontLayoutCmdCond *cond = &cmd->body.cond;
	  int i;

	  fprintf (stderr, "(cond");
	  for (i = 0; i < cond->n_cmds; i++)
	    {
	      fprintf (stderr, "\n%s  ", prefix);
	      dump_flt_cmd (stage, cond->cmd_ids[i], indent + 2);
	    }
	  fprintf (stderr, ")");
	}
      else if (cmd->type == FontLayoutCmdTypeOTF)
	{
	  fprintf (stderr, "(otf)");
	}
      else
	fprintf (stderr, "(error-command)");
    }
  else if (id <= CMD_ID_OFFSET_COMBINING)
    fprintf (stderr, "cominging-code");
  else
    fprintf (stderr, "(predefiend %d)", id);
}

void
dump_flt (MFontLayoutTable *flt, int indent)
{
  char *prefix = (char *) alloca (indent + 1);
  MPlist *plist;
  int stage_idx = 0;

  memset (prefix, 32, indent);
  prefix[indent] = 0;
  fprintf (stderr, "(flt");
  MPLIST_DO (plist, flt)
    {
      FontLayoutStage *stage = (FontLayoutStage *) MPLIST_VAL (plist);
      int i;

      fprintf (stderr, "\n%s  (stage %d", prefix, stage_idx);
      for (i = 0; i < stage->used; i++)
	{
	  fprintf (stderr, "\n%s    ", prefix);
	  dump_flt_cmd (stage, INDEX_TO_CMD_ID (i), indent + 4);
	}
      fprintf (stderr, ")");
      stage_idx++;
    }
  fprintf (stderr, ")");
}
