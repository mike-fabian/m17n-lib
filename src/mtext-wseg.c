/* mtext-wseg.c -- word segmentation
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

#include "config.h"
#include "m17n-core.h"
#include "m17n-misc.h"
#include "internal.h"
#include "textprop.h"
#include "character.h"

typedef struct _MWordseg_Function MWordseg_Function;

struct _MWordseg_Function
{
  int initialized;
  int (*init) (void);
  void (*fini) (void);
  int (*wseg) (MText *mt, int pos, int *from, int *to,
	       MWordseg_Function *wordseg);
  MWordseg_Function *next;
};

static MWordseg_Function *wordseg_function_list;

static MCharTable *wordseg_function_table;

static int
generic_wordseg (MText *mt, int pos, int *from, int *to,
		 MWordseg_Function *wordseg)
{
  int len = mtext_nchars (mt);
  int c = mtext_ref_char (mt, pos);
  MSymbol category = mchar_get_prop (c, Mcategory);
  char cathead = msymbol_name (category)[0];
  int in_word = (cathead == 'L' || cathead == 'M' || cathead == 'N');
  int beg, end;

  for (beg = pos; beg > 0; beg--)
    {
      c = mtext_ref_char (mt, beg - 1);
      category = mchar_get_prop (c, Mcategory);
      cathead = msymbol_name (category)[0];
      if (in_word != (cathead == 'L' || cathead == 'M' || cathead == 'N'))
	break;
      if (mchartable_lookup (wordseg_function_table, c) != wordseg)
	break;
    }
  for (end = pos; end < len; end++)
    {
      c = mtext_ref_char (mt, end);
      category = mchar_get_prop (c, Mcategory);
      cathead = msymbol_name (category)[0];
      if (in_word != (cathead == 'L' || cathead == 'M' || cathead == 'N'))
	break;
      if (mchartable_lookup (wordseg_function_table, c) != wordseg)
	break;
    }
  if (from)
    *from = beg;
  if (to)
    *to = end;
  return in_word;
}

#ifdef HAVE_THAI_WORDSEG

#define THAI_BEG 0x0E01
#define THAI_END 0x0E6F

static MSymbol M_thai_wordseg;

/* We have libthai, wordcut, or wordcut-old.  Each of them provides
   the following three functions.  */

static int thai_wordseg_init (void);
static void thai_wordseg_fini (void);
static MTextProperty *thai_wordseg_propertize (MText *mt, int pos,
					       int from, int to,
					       unsigned char *tis);

#ifdef HAVE_LIBTHAI

#include <thai/thbrk.h>

static int
thai_wordseg_init (void)
{
  return 0;
}

static void
thai_wordseg_fini (void)
{
  return;
}

static MTextProperty *
thai_wordseg_propertize (MText *mt, int pos, int from, int to,
			 unsigned char *tis)
{
  int len = to - from;
  int *breaks = alloca ((sizeof (int)) * len);
  int count = th_brk ((thchar_t *) tis, breaks, len);
  MTextProperty *prop = NULL;

  if (count == 0)
    {
      prop = mtext_property (M_thai_wordseg, Mt,
			     MTEXTPROP_VOLATILE_WEAK | MTEXTPROP_NO_MERGE);
      mtext_attach_property (mt, from, to, prop);
      M17N_OBJECT_UNREF (prop);
    }
  else
    {
      int last, i;
      MTextProperty *this;

      for (i = 0, last = from; i < count; i++)
	{
	  this = mtext_property (M_thai_wordseg, Mt,
				 MTEXTPROP_VOLATILE_WEAK | MTEXTPROP_NO_MERGE);
	  mtext_attach_property (mt, last, from + breaks[i], this);
	  if (pos >= last && pos < from + breaks[i])
	    prop = this;
	  M17N_OBJECT_UNREF (this);
	  last = from + breaks[i];
	}
      if (last < to)
	{
	  this = mtext_property (M_thai_wordseg, Mt,
				 MTEXTPROP_VOLATILE_WEAK | MTEXTPROP_NO_MERGE);
	  mtext_attach_property (mt, last, to, this);
	  if (pos >= last && pos < to)
	    prop = this;
	  M17N_OBJECT_UNREF (this);
	}
    }

  if (! prop)
    mdebug_hook ();
  return prop;
}

#elif HAVE_WORDCUT

#include <wordcut/wcwordcut.h>

static WcWordcut wordcut;
static WcWordVector *word_vector;

static int
thai_wordseg_init (void)
{  
  wc_wordcut_init (&wordcut);
  return 0;
}

static void
thai_wordseg_fini (void)
{
  if (word_vector)
    wc_word_vector_delete (word_vector);
  wc_wordcut_destroy (&wordcut);
  return;
}

static MTextProperty *
thai_wordseg_propertize (MText *mt, int pos, int from, int to,
			 unsigned char *tis)
{
  gulong i, count;
  MTextProperty *prop = NULL;

  if (! word_vector)
    word_vector = wc_word_vector_new ();
  else
    {
      wc_word_vector_destroy (word_vector);
      wc_word_vector_init (word_vector);
    }

  wc_wordcut_cut (&wordcut, (gchar *) tis, (gint) (to - from),
		  word_vector);
  count = wc_word_vector_get_count (word_vector);
  for (i = 0; i < count; i++)
    {
      WcWord *word = wc_word_vector_get_word (word_vector, i);

      if (word->type != WC_WORDTYPE_DELETED)
	{
	  MSymbol val = ((word->type == WC_WORDTYPE_DICTIONARY
			  || word->type == WC_WORDTYPE_WORDUNIT
			  || word->type == WC_WORDTYPE_JOINED)
			 ? Mt : Mnil);
	  MTextProperty *this
	    = mtext_property (M_thai_wordseg, val,
			      MTEXTPROP_VOLATILE_WEAK | MTEXTPROP_NO_MERGE);

	  mtext_attach_property (mt, from, from + word->len, this);
	  if (pos >= from && pos < from + word->len)
	    prop = this;
	  M17N_OBJECT_UNREF (this);
	  from += word->len;
	}
    }
  return prop;
}

#else  /* HAVE_WORDCUT_OLD */

#include <wordcut/wordcut.h>

static Wordcut wordcut;
static WordcutResult wordcut_result;
static int wordcut_result_used;

static int
thai_wordseg_init (void)
{  
  return (wordcut_init (&wordcut, WORDCUT_TDICT) == 0 ? 0 : -1);
}

static void
thai_wordseg_fini (void)
{
  if (wordcut_result_used)
    {
      wordcut_result_close (&wordcut_result);
      wordcut_result_used = 0;
    }
  wordcut_close (&wordcut);
  return;
}

static MTextProperty *
thai_wordseg_propertize (MText *mt, int pos, int from, int to,
			 unsigned char *tis)
{
  int i, last;
  MTextProperty *prop = NULL;

  wordcut_cut (&wordcut, (char *) tis, &wordcut_result);
  wordcut_result_used = 1;
  for (i = 0, last = from; i < wordcut_result.count; i++)
    {
      MTextProperty *this;

      if (last < from + wordcut_result.start[i])
	{
	  this = mtext_property (M_thai_wordseg, Mnil,
				 MTEXTPROP_VOLATILE_WEAK);
	  mtext_attach_property (mt, last, from + wordcut_result.start[i],
				 this);
	  if (pos >= last && pos < from + wordcut_result.start[i])
	    prop = this;
	  M17N_OBJECT_UNREF (this);
	}

      this = mtext_property (M_thai_wordseg, Mt,
			     MTEXTPROP_VOLATILE_WEAK | MTEXTPROP_NO_MERGE);
      last = from + wordcut_result.start[i];
      mtext_attach_property (mt, last, last + wordcut_result.offset[i], this);
      if (pos >= last && pos < last + wordcut_result.offset[i])
	prop = this;
      m17n_object_unref (this);
      last += wordcut_result.offset[i];
    }
  return prop;
}

#endif  /* not HAVE_LIBTHA, HAVE_WORDCUT nor HAVE_WORDCUT_OLD */

int
thai_wordseg (MText *mt, int pos, int *from, int *to,
	      MWordseg_Function *wordseg)
{
  MTextProperty *prop;

  /* It is assured that there's a Thai character at POS.  */
  prop = mtext_get_property (mt, pos, M_thai_wordseg);
  if (! prop)
    {
      /* TIS620 code sequence.  */
      unsigned char *tis;
      int len = mtext_nchars (mt);
      int beg, end;
      int c, i;

      for (beg = pos; beg > 0; beg--)
	if ((c = mtext_ref_char (mt, beg - 1)) < THAI_BEG || c > THAI_END)
	  break;
      for (end = pos + 1; end < len; end++)
	if ((c = mtext_ref_char (mt, end)) < THAI_BEG || c > THAI_END)
	  break;

      /* Extra 1-byte for 0 terminating.  */
      tis = alloca ((end - beg) + 1);

      for (i = beg; i < end; i++)
	tis[i - beg] = 0xA1 + (mtext_ref_char (mt, i) - THAI_BEG);
      tis[i - beg] = 0;
      prop = thai_wordseg_propertize (mt, pos, beg, end, tis);
    }

  if (from)
    *from = MTEXTPROP_START (prop);
  if (to)
    *to = MTEXTPROP_END (prop);
  return (MTEXTPROP_VAL (prop) == Mt);
}

#endif	/* HAVE_THAI_WORDSEG */


/* Internal API */

void
mtext__wseg_fini ()
{
  if (wordseg_function_list)
    {
      while (wordseg_function_list)
	{
	  MWordseg_Function *next = wordseg_function_list->next;

	  if (wordseg_function_list->initialized > 0
	      && wordseg_function_list->fini)
	    wordseg_function_list->fini ();
	  free (wordseg_function_list);
	  wordseg_function_list = next;
	}
      M17N_OBJECT_UNREF (wordseg_function_table);
    }
}

/* Find word boundaries around POS of MT.  Set *FROM to the word
   boundary position at or previous to POS, and update *TO to the word
   boundary position after POS.

   @return 
   If word boundaries were found successfully, return 1 (if
   the character at POS is a part of a word) or 0 (otherwise).  If the
   operation was not successful, return -1 without setting *FROM and
   *TO.  */

int
mtext__word_segment (MText *mt, int pos, int *from, int *to)
{
  int c = mtext_ref_char (mt, pos);
  MWordseg_Function *wordseg;

  if (! wordseg_function_table)
    {
      wordseg_function_table = mchartable (Mnil, NULL);

      MSTRUCT_CALLOC (wordseg, MERROR_MTEXT);
      wordseg->wseg = generic_wordseg;
      wordseg->next = wordseg_function_list;
      wordseg_function_list = wordseg;
      mchartable_set_range (wordseg_function_table, 0, MCHAR_MAX, wordseg);

#ifdef HAVE_THAI_WORDSEG
      MSTRUCT_CALLOC (wordseg, MERROR_MTEXT);
      wordseg->init = thai_wordseg_init;
      wordseg->fini = thai_wordseg_fini;
      wordseg->wseg = thai_wordseg;
      wordseg->next = wordseg_function_list;
      wordseg_function_list = wordseg;
      mchartable_set_range (wordseg_function_table, THAI_BEG, THAI_END,
			    wordseg);
      M_thai_wordseg = msymbol ("  thai-wordseg");
#endif
    }

  wordseg = mchartable_lookup (wordseg_function_table, c);
  if (wordseg && wordseg->initialized >= 0)
    {
      if (! wordseg->initialized)
	{
	  if (wordseg->init
	      && wordseg->init () < 0)
	    {
	      wordseg->initialized = -1;
	      return -1;
	    }
	  wordseg->initialized = 1;
	}
      return wordseg->wseg (mt, pos, from, to, wordseg);
    }
  return -1;
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */
