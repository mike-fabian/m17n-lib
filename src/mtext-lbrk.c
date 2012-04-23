/* mtext-lbrk.c -- line break
   Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012
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

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "mtext.h"

enum LineBreakClass
  {
    LBC_OP, /* open */
    LBC_CL, /* close */
    LBC_QU, /* quotation */
    LBC_GL, /* glue */
    LBC_NS, /* no-start */
    LBC_EX, /* exclamation/interrogation */
    LBC_SY, /* Syntax (slash) */
    LBC_IS, /* infix (numeric) separator */
    LBC_PR, /* prefix */
    LBC_PO, /* postfix */
    LBC_NU, /* numeric */
    LBC_AL, /* alphabetic */
    LBC_ID, /* ideograph (atomic) */
    LBC_IN, /* inseparable */
    LBC_HY, /* hyphen */
    LBC_BA, /* break after */
    LBC_BB, /* break before */
    LBC_B2, /* break both */
    LBC_ZW, /* ZW space */
    LBC_CM, /* combining mark */
    LBC_WJ, /* word joiner */

    /* used for 4.1 pair table */
    LBC_H2, /* Hamgul 2 Jamo Syllable */
    LBC_H3, /* Hangul 3 Jamo Syllable */
    LBC_JL, /* Jamo leading consonant */
    LBC_JV, /* Jamo vowel */
    LBC_JT, /* Jamo trailing consonant */

    /* These are not handled in the pair tables. */
    LBC_SA, /* south (east) asian */
    LBC_SP, /* space */
    LBC_PS, /* paragraph and line separators */
    LBC_BK, /* hard break (newline) */
    LBC_CR, /* carriage return */
    LBC_LF, /* line feed */
    LBC_NL, /* next line */
    LBC_CB, /* contingent break opportunity */
    LBC_SG, /* surrogate */
    LBC_AI, /* ambiguous */
    LBC_XX, /* unknown */
    LBC_MAX
  };

enum LineBreakAction
  {
    LBA_DIRECT =		'_',
    LBA_INDIRECT =		'%',
    LBA_COMBINING_INDIRECT =	'#',
    LBA_COMBINING_PROHIBITED =	'@',
    LBA_PROHIBITED =		'^',
    LBA_MAX
  };

/* The pair table of line break actions.  */
static char *lba_pair_table[] =
  /* OP GL SY PO ID BA ZW H2 JV
      CL NS IS NU IN BB CM H3 JT
       QU EX PR AL HY B2 WJ JL  */
  { "^^^^^^^^^^^^^^^^^^^@^^^^^^", /* OP */
    "_^%%^^^^_%____%%__^#^_____", /* CL */
    "^^%%%^^^%%%%%%%%%%^#^%%%%%", /* QU */
    "%^%%%^^^%%%%%%%%%%^#^%%%%%", /* GL */
    "_^%%%^^^______%%__^#^_____", /* NS */
    "_^%%%^^^______%%__^#^_____", /* EX */
    "_^%%%^^^__%___%%__^#^_____", /* SY */
    "_^%%%^^^__%%__%%__^#^_____", /* IS */
    "%^%%%^^^__%%%_%%__^#^%%%%%", /* PR */
    "_^%%%^^^______%%__^#^_____", /* PO */
    "_^%%%^^^_%%%_%%%__^#^_____", /* NU */
    "_^%%%^^^__%%_%%%__^#^_____", /* AL */
    "_^%%%^^^_%___%%%__^#^_____", /* ID */
    "_^%%%^^^_____%%%__^#^_____", /* IN */
    "_^%%%^^^__%___%%__^#^_____", /* HY */
    "_^%%%^^^______%%__^#^_____", /* BA */
    "%^%%%^^^%%%%%%%%%%^#^%%%%%", /* BB */
    "_^%%%^^^______%%_^^#^_____", /* B2 */
    "__________________^_______", /* ZW */
    "_^%%%^^^__%%_%%%__^#^_____", /* CM */
    "%^%%%^^^%%%%%%%%%%^#^%%%%%", /* WJ */
    "_^%%%^^^_%___%%%__^#^___%%", /* H2 */
    "_^%%%^^^_%___%%%__^#^____%", /* H3 */
    "_^%%%^^^_%___%%%__^#^%%%%_", /* JL */
    "_^%%%^^^_%___%%%__^#^___%%", /* JV */
    "_^%%%^^^_%___%%%__^#^____%"  /* JT */
  };

static MCharTable *lbc_table;

/* Set LBC to enum LineBreakClass of the character at POS of MT
   (length is LEN) while converting LBC_AI and LBC_XX to LBC_AL,
   LBC_CB to LBC_B2, LBC_CR, LBC_LF, and LBC_NL to LBC_BK.  If POS is
   out of range, set LBC to LBC_BK.  */

#define GET_LBC(LBC, MT, LEN, POS, OPTION)				\
  do {									\
    if ((POS) < 0 || (POS) >= (LEN))					\
      (LBC) = LBC_BK;							\
    else								\
      {									\
	int c = mtext_ref_char ((MT), (POS));				\
	(LBC) = (enum LineBreakClass) mchartable_lookup (lbc_table, c);	\
	if ((LBC) == LBC_NL)						\
	  (LBC) = LBC_BK;						\
	else if ((LBC) == LBC_AI)					\
	  (LBC) = ((OPTION) & MTEXT_LBO_AI_AS_ID) ? LBC_ID : LBC_AL;	\
	else if (! ((OPTION) & MTEXT_LBO_KOREAN_SP)			\
		 && (LBC) >= LBC_H2 && (LBC) <= LBC_JT)			\
	  (LBC) = LBC_AL;						\
	else if ((LBC) == LBC_CB)					\
	  (LBC) = LBC_B2;						\
	else if ((LBC) == LBC_XX)					\
	  (LBC) = LBC_AL;						\
      }									\
  } while (0)


/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

/*** @addtogroup m17nMtext */
/*** @{ */
/*=*/

/***en
    @brief Find a linebreak postion of an M-text.

    The mtext_line_break () function checks if position $POS is a
    proper linebreak position of an M-text $MT according to the
    algorithm of The Unicode Standard 4.0 UAX#14.  It so, it returns
    $POS.  Otherwise, it returns a proper linebreak position before
    $POS.

    If $OPTION is nonzero, it controls the algorithm by logical-or of
    the members of #MTextLineBreakOption.

    If $AFTER is not NULL, a proper linebreak position after $POS is
    stored there.  */

int
mtext_line_break (MText *mt, int pos, int option, int *after)
{
  int break_before, break_after;
  int len = mtext_len (mt);
  enum LineBreakClass lbc;
  enum LineBreakClass Blbc, Albc; /* B(efore) and A(fter) lbcs.  */
  int Bpos, Apos;		  /* B(efore) and A(fter) positions.  */
  enum LineBreakAction action;
  
  if (pos >= len)
    {
      /* The end of text is an explicit break position.  */
      if (after)
	*after = pos;
      return pos;
    }

  if (! lbc_table)
    {
      MSymbol key = mchar_define_property ("linebreak", Minteger);

      lbc_table = mchar_get_prop_table (key, NULL);
    }

  GET_LBC (lbc, mt, len, pos, option);
  Apos = pos;
  Albc = lbc;
  if (Albc == LBC_SP)
    {
      if (option & MTEXT_LBO_SP_CM)
	{
	  GET_LBC (Albc, mt, len, Apos + 1, option);
	  Albc = (Albc == LBC_CM) ? LBC_ID : LBC_SP;
	}
      while (Albc == LBC_SP)
	{
	  Apos--;
	  GET_LBC (Albc, mt, len, Apos, option);
	}
    }
  if ((option & MTEXT_LBO_SP_CM) && (Albc == LBC_CM))
    {
      Apos--;
      GET_LBC (Albc, mt, len, Apos, option);
      if (Albc == LBC_SP)
	Albc = LBC_ID;
      else
	Apos++, Albc = LBC_CM;
    }

  if (Albc == LBC_CR)
    Albc = LBC_BK;
  else if (Albc == LBC_LF)
    {
      GET_LBC (Albc, mt, len, Apos - 1, option);
      if (Albc == LBC_CR)
	Apos--;
      Albc = LBC_BK;
    }
  else if (Albc == LBC_SA)
    Albc = mtext__word_segment (mt, Apos, &Apos, NULL) > 0 ? LBC_BB : LBC_AL;
  Bpos = Apos;
  /* After exiting from the following loop, if Apos is positive, it is
     the previous (including POS) break position.  */
  while (Apos > 0)
    {
      int indirect;
      int next = -1;

      /* Now Bpos == Apos.  */
      do {
	Bpos--;
	GET_LBC (Blbc, mt, len, Bpos, option);
      } while (Blbc == LBC_SP);

      if (Blbc == LBC_BK || Blbc == LBC_LF || Blbc == LBC_CR)
	{
	  /* Explicit break.  */
	  break;
	}

      indirect = Bpos + 1 < Apos;

      if (Blbc == LBC_CM)
	{
	  do {
	      Bpos--;
	      GET_LBC (Blbc, mt, len, Bpos, option);
	  } while (Blbc == LBC_CM);
	  if ((option & MTEXT_LBO_SP_CM) && (Blbc == LBC_SP))
	    Blbc = LBC_ID;
	  else if (Blbc == LBC_SP || Blbc == LBC_ZW
		   || Blbc == LBC_BK || Blbc == LBC_LF || Blbc == LBC_CR)
	    {
	      Blbc = LBC_AL;
	      Bpos++;
	    }
	}		   
      if (Blbc == LBC_SA)
	{
	  mtext__word_segment (mt, Bpos, &next, NULL);
	  Blbc = LBC_AL;
	}

      if (Albc != LBC_BK)
	{
	  action = lba_pair_table[Blbc][Albc];
	  if (action == LBA_DIRECT)
	    break;
	  else if (action == LBA_INDIRECT)
	    {
	      if (indirect)
		break;
	    }
	  else if (action == LBA_COMBINING_INDIRECT)
	    {
	      if (indirect)
		break;
	    }
	}
      if (next >= 0)
	Apos = next, Albc = LBC_BB;
      else
	Apos = Bpos, Albc = Blbc;
    }
  break_before = Apos;
  if (break_before > 0)
    {
      if (! after)
	return break_before;
      if (break_before == pos)
	{
	  if (after)
	    *after = break_before;
	  return break_before;
	}
    }

  /* Now find a break position after POS.  */
  break_after = 0;
  Bpos = pos;
  Blbc = lbc;
  if (Blbc == LBC_CM)
    {
      do {
	Bpos--;
	GET_LBC (Blbc, mt, len, Bpos, option);
      } while (Blbc == LBC_CM);
      if (Blbc == LBC_SP || Blbc == LBC_ZW
	  || Blbc == LBC_BK || Blbc == LBC_LF || Blbc == LBC_CR)
	{
	  if ((Blbc == LBC_SP) && (option & MTEXT_LBO_SP_CM))
	    Blbc = LBC_ID;
	  else
	    Blbc = LBC_AL;
	}
      Bpos = pos;
    }
  if (Blbc == LBC_SA)
    {
      mtext__word_segment (mt, Bpos, NULL, &Bpos);
      Blbc = LBC_AL;
    }
  else if (Blbc == LBC_SP)
    {
      if (option & MTEXT_LBO_SP_CM)
	{
	  GET_LBC (Blbc, mt, len, Bpos + 1, option);
	  if (Blbc == LBC_CM)
	    Blbc = LBC_ID, Bpos++;
	  else
	    Blbc = LBC_SP;
	}
      while (Blbc == LBC_SP)
	{
	  Bpos--;
	  GET_LBC (Blbc, mt, len, Bpos, option);
	}
      if (Bpos < 0)
	Bpos = pos;
    }
  Apos = Bpos;
  /* After exiting from the following loop, if Apos is positive, it is
     the next break position.  */
  while (1)
    {
      int indirect;
      int next = -1;

      /* Now Bpos == Apos.  */
      if (Blbc == LBC_LF || Blbc == LBC_BK || Blbc == LBC_CR)
	{
	  Apos++;
	  if (Blbc == LBC_CR)
	    {
	      GET_LBC (Blbc, mt, len, Bpos + 1, option);
	      if (Blbc == LBC_LF)
		Apos++;
	    }
	  break;
	}

      do {
	Apos++;
	GET_LBC (Albc, mt, len, Apos, option);
      } while (Albc == LBC_SP);
      
      if (Blbc == LBC_SP)
	break;

      if (Apos == len)
	/* Explicit break at the end of text.  */
	break;

      indirect = Bpos + 1 < Apos;

      if (Albc == LBC_SA)
	Albc = mtext__word_segment (mt, Apos, NULL, &next) ? LBC_BB : LBC_AL;

      action = lba_pair_table[Blbc][Albc];
      if (action == LBA_DIRECT)
	/* Direct break at Apos.  */
	break;
      else if (action == LBA_INDIRECT)
	{
	  if (indirect)
	    break;
	}
      else if (action == LBA_COMBINING_INDIRECT)
	{
	  if (indirect)
	    {
	      if (option & MTEXT_LBO_SP_CM)
		Apos--;
	      break;
	    }
	}
      if (next >= 0)
	Bpos = next, Blbc = LBC_AL;
      else
	{
	  Bpos = Apos;
	  if (Albc != LBC_CM)
	    Blbc = Albc;
	}
    }
  break_after = Apos;
  if (after)
    *after = break_after;

  return (break_before > 0 ? break_before : break_after);
}

/*** @} */ 

/*
  Local Variables:
  coding: euc-japan
  End:
*/
