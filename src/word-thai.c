/* word-thai.c -- Find a word segment in Thai text.
   Copyright (C) 2005
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

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <stdio.h>
#include <stdlib.h>

#include "config.h"

#ifdef HAVE_THAI_WORDSEG

#include "m17n-core.h"
#include "m17n-misc.h"
#include "internal.h"
#include "textprop.h"
#include "character.h"
#include "mtext.h"

static int init_wordseg_library (void);
static void fini_wordseg_library (void);
static MTextProperty *wordseg_propertize (MText *mt, int pos, int from, int to,
					  unsigned char *tis);

#define THAI_BEG 0x0E01
#define THAI_END 0x0E6F

static int wordseg_library_initialized;
static MSymbol Mthai_wordseg;

#ifdef HAVE_LIBTHAI

#include <thai/thbrk.h>

static int
init_wordseg_library (void)
{
  return 0;
}

static void
fini_wordseg_library (void)
{
  return;
}

static MTextProperty *
wordseg_propertize (MText *mt, int pos, int from, int to, unsigned char *tis)
{
  int len = to - from;
  int *breaks = alloca ((sizeof (int)) * len);
  int count = th_brk ((thchar_t *) tis, breaks, len);
  MTextProperty *prop = NULL;

  if (count == 0)
    {
      prop = mtext_property (Mthai_wordseg, Mt,
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
	  this = mtext_property (Mthai_wordseg, Mt,
				 MTEXTPROP_VOLATILE_WEAK | MTEXTPROP_NO_MERGE);
	  mtext_attach_property (mt, last, from + breaks[i], this);
	  if (pos >= last && pos < from + breaks[i])
	    prop = this;
	  M17N_OBJECT_UNREF (this);
	  last = from + breaks[i];
	}
      if (last < to)
	{
	  this = mtext_property (Mthai_wordseg, Mt,
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
init_wordseg_library (void)
{  
  wc_wordcut_init (&wordcut);
  return 0;
}

static void
fini_wordseg_library (void)
{
  if (word_vector)
    wc_word_vector_delete (word_vector);
  wc_wordcut_destroy (&wordcut);
  return;
}

static MTextProperty *
wordseg_propertize (MText *mt, int pos, int from, int to, unsigned char *tis)
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
	    = mtext_property (Mthai_wordseg, val,
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
init_wordseg_library (void)
{  
  return (wordcut_init (&wordcut, WORDCUT_TDICT) == 0 ? 0 : -1);
}

static void
fini_wordseg_library (void)
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
wordseg_propertize (MText *mt, int pos, int from, int to, unsigned char *tis)
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
	  this = mtext_property (Mthai_wordseg, Mnil, MTEXTPROP_VOLATILE_WEAK);
	  mtext_attach_property (mt, last, from + wordcut_result.start[i],
				 this);
	  if (pos >= last && pos < from + wordcut_result.start[i])
	    prop = this;
	  M17N_OBJECT_UNREF (this);
	}

      this = mtext_property (Mthai_wordseg, Mt,
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
thai_wordseg (MText *mt, int pos, int *from, int *to)
{
  int len = mtext_nchars (mt);
  /* TIS620 code sequence.  */
  unsigned char *tis;
  MTextProperty *prop;
  int beg, end;
  int c;

  /* It is assured that there's a Thai character at POS.  */
  prop = mtext_get_property (mt, pos, Mthai_wordseg);
  if (prop)
    {
      beg = MTEXTPROP_START (prop);
      if (beg > 0
	  && ((c = mtext_ref_char (mt, beg - 1)) < THAI_BEG || c > THAI_END))
	beg = -1;
      end = MTEXTPROP_END (prop);
      if (end < len
	  && ((c = mtext_ref_char (mt, end)) < THAI_BEG || c > THAI_END))
	end = -1;
    }
  else
    {
      int i;

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
      prop = wordseg_propertize (mt, pos, beg, end, tis);
      i = MTEXTPROP_START (prop);
      beg = (i > beg || i == 0) ? i : -1;
      i = MTEXTPROP_END (prop);
      end = (i < end || i == len) ? i : -1;
    }

  if (from)
    *from = beg;
  if (to)
    *to = end;
  return (MTEXTPROP_VAL (prop) == Mt);
}

#endif	/* HAVE_THAI_WORDSEG */


/* Internal API */

int
mtext__word_thai_init ()
{
#ifdef HAVE_THAI_WORDSEG
  if (! wordseg_library_initialized)
    {
      if (init_wordseg_library () < 0)
	return -1;
      wordseg_library_initialized = 1;
      Mthai_wordseg = msymbol ("  wordcut-wordseg");
    }
  mchartable_set_range (wordseg_func_table, THAI_BEG, THAI_END,
			(void *) thai_wordseg);
#endif
  return 0;
}

void
mtext__word_thai_fini ()
{
#ifdef HAVE_THAI_WORDSEG
  if (wordseg_library_initialized)
    {
      fini_wordseg_library ();
      wordseg_library_initialized = 0;
    }
#endif
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */
