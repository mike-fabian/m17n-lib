/* textprop.c -- text property module.
   Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
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

/***en
    @addtogroup m17nTextProperty
    @brief Function to handle text properties.

    Each character in an M-text can have properties called @e text @e
    properties.  Text properties store various kinds of information
    attached to parts of an M-text to provide application programs
    with a unified view of those information.  As rich information can
    be stored in M-texts in the form of text properties, functions in
    application programs can be simple.

    A text property consists of a @e key and @e values, where key is a
    symbol and values are anything that can be cast to <tt>(void *)
    </tt>.  Unlike other types of properties, a text property can
    have multiple values.  "The text property whose key is K" may be
    shortened to "K property".  */

/***ja
    @addtogroup m17nTextProperty
    @brief テキストプロパティを操作するための関数.

    M-text 内の各文字は、@e テキストプロパティ と呼ばれるプロパティを
    持つことができる。テキストプロパティは、M-text の各部位に付加され
    たさまざまな情報を保持しており、アプリケーションプログラムはそれら
    の情報を統一的に扱うことができる。M-text 自体が豊富な情報を持つた
    め、アプリケーションプログラム中の関数を簡素化することができる。

    テキストプロパティは @e キー と @e 値 からなる。キーはシンボルであ
    り、値は <tt>(void *)</tt> 型にキャストできるものなら何でもよい。
    他のタイプのプロパティと異なり、一つのテキストプロパティが複数の値
    を持つことが許される。「キーが K であるテキストプロパティ」のこと
    を簡単に「K プロパティ」と呼ぶことがある。  */

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_XML2
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <libxml/xpath.h>
#endif

#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "symbol.h"
#include "mtext.h"
#include "textprop.h"

#define TEXT_PROP_DEBUG

#undef xassert
#ifdef TEXT_PROP_DEBUG
#define xassert(X)	do {if (!(X)) mdebug_hook ();} while (0)
#else
#define xassert(X)	(void) 0
#endif	/* not FONTSET_DEBUG */

/* Hierarchy of objects (MText, MTextPlist, MInterval, MTextProperty)

MText
  |    key/a         key/b                key/x
  +--> MTextPlist -> MTextPlist -> ... -> MTextPlist
         |             |
         |             +- tail <-----------------------------------------+
         |             |                                                 |
         |             +- head <--> MInterval <--> ... <--> MInterval <--+
         |
         +- tail --------------------------------------------------------+
         |                                                               |
         +- head --> MInterval <--> MInterval <--> ... <--> MInterval <--+
                       |               |
                       +---------------+------------> MTextProperty
                       +--> MTextProperty
                       ...


Examples:

MTextProperty a/A                    [AAAAAAAAAAAAAAAAAAAAA]
MTextProperty a/B           [BBBBBBBBBBBBBBBBB] 
MTextPlist a     |--intvl1--|-intvl2-|-intvl3-|---intvl4---|-intvl5-|
		

MTextProperty b/A                    [AAAAAAAAAA]
MTextProperty b/B         [BBBBBBBBBBBBBBBBBBBBBBBBBBBBBB] 
MTextPlist b     |-intvl1-|--intvl2--|--intvl3--|-intvl4-|--intvl5--|

    M-text       |--------------------------------------------------|

    (intvl == MInterval)

*/

/* The structure MTextProperty is defined in textprop.h.  */

/** MInterval is the structure for an interval that holds text
    properties of the same key in a specific range of M-text.
    All intervals are stored in MIntervalPool.  */

typedef struct MInterval MInterval;

struct MInterval
{
  /** Stack of pointers to text properties.  If the interval does not
      have any text properties, this member is NULL or contains random
      values.  */
  MTextProperty **stack;

  /** How many values are in <stack>.  */
  int nprops;

  /** Length of <stack>.  */
  int stack_length;

  /** Start and end character positions of the interval.  If <end> is
      negative, this interval is not in use.  */
  int start, end;

  /** Pointers to the previous and next intervals.  If <start> is 0,
      <prev> is NULL and this interval is pointed by MTextPlist->head.
      If <end> is the size of the M-text, <next> is NULL, and this
      interval is pointed by MTextPlist->tail.  */
  MInterval *prev, *next;
};  

/** MTextPlist is a structure to hold text properties of an M-text by
   chain.  Each element in the chain is for a specific key.  */

typedef struct MTextPlist MTextPlist;

struct MTextPlist
{
  /** Key of the property.  */
  MSymbol key;

  /** The head and tail intervals.  <head>->start is always 0.
      <tail->end is always MText->nchars.  */
  MInterval *head, *tail;

  /** Lastly accessed interval.  */
  MInterval *cache;

  /* Not yet implemented.  */
  int (*modification_hook) (MText *mt, MSymbol key, int from, int to);

  /** Pointer to the next property in the chain, or NULL if the
      property is the last one in the chain.  */
  MTextPlist *next;
};


/** How many intervals one interval-pool can contain. */

#define INTERVAL_POOL_SIZE 1024

typedef struct MIntervalPool MIntervalPool;


/** MIntervalPool is the structure for an interval-pool which store
    intervals.  Each interval-pool contains INTERVAL_POOL_SIZE number
    of intervals, and is chained from the root #interval_pool.  */

struct MIntervalPool
{
  /** Array of intervals.  */
  MInterval intervals[INTERVAL_POOL_SIZE];

  /** The smallest index to an unused interval.  */
  int free_slot;

  /** Pointer to the next interval-pool.  */
  MIntervalPool *next;
};


/** Root of interval-pools.  */

static MIntervalPool interval_pool_root;

/* For debugging. */

static M17NObjectArray text_property_table;

/** Return a newly allocated interval pool.  */

static MIntervalPool *
new_interval_pool ()
{
  MIntervalPool *pool;
  int i;

  MSTRUCT_CALLOC (pool, MERROR_TEXTPROP);
  for (i = 0; i < INTERVAL_POOL_SIZE; i++)
    pool->intervals[i].end = -1;
  pool->free_slot = 0;
  pool->next = NULL;
  return pool;
}


/** Return a new interval for the region START and END.  */

static MInterval *
new_interval (int start, int end)
{
  MIntervalPool *pool;
  MInterval *interval;

  for (pool = &interval_pool_root;
       pool->free_slot >= INTERVAL_POOL_SIZE;
       pool = pool->next)
    {
      if (! pool->next)
	pool->next = new_interval_pool ();
    }

  interval = &(pool->intervals[pool->free_slot]);
  interval->stack = NULL;
  interval->nprops = 0;
  interval->stack_length = 0;
  interval->prev = interval->next = NULL;
  interval->start = start;
  interval->end = end;

  pool->free_slot++;
  while (pool->free_slot < INTERVAL_POOL_SIZE
	 && pool->intervals[pool->free_slot].end >= 0)
    pool->free_slot++;

  return interval;
}


/** Free INTERVAL and return INTERVAL->next.  It assumes that INTERVAL
    has no properties.  */

static MInterval *
free_interval (MInterval *interval)
{
  MIntervalPool *pool = &interval_pool_root;
  int i;

  xassert (interval->nprops == 0);
  if (interval->stack)
    free (interval->stack);
  while ((interval < pool->intervals
	  || interval >= pool->intervals + INTERVAL_POOL_SIZE)
	 && pool->next)
    pool = pool->next;

  i = interval - pool->intervals;
  interval->end = -1;
  if (i < pool->free_slot)
    pool->free_slot = i;
  return interval->next;
}


/** If necessary, allocate a stack for INTERVAL so that it can contain
   NUM number of text properties.  */

#define PREPARE_INTERVAL_STACK(interval, num)				\
  do {									\
    if ((num) > (interval)->stack_length)				\
      {									\
	MTABLE_REALLOC ((interval)->stack, (num), MERROR_TEXTPROP);	\
	(interval)->stack_length = (num);				\
      }									\
  } while (0)


/** Return a copy of INTERVAL.  The copy still shares text properties
    with INTERVAL.  If MASK_BITS is not zero, don't copy such text
    properties whose control flags contains bits in MASK_BITS.  */

static MInterval *
copy_interval (MInterval *interval, int mask_bits)
{
  MInterval *new = new_interval (interval->start, interval->end);
  int nprops = interval->nprops;
  MTextProperty **props = alloca (sizeof (MTextProperty *) * nprops);
  int i, n;

  for (i = n = 0; i < nprops; i++)
    if (! (interval->stack[i]->control.flag & mask_bits))
      props[n++] = interval->stack[i];
  new->nprops = n;
  if (n > 0)
    {
      PREPARE_INTERVAL_STACK (new, n);
      memcpy (new->stack, props, sizeof (MTextProperty *) * n);
    }	

  return new;
}


/** Free text property OBJECT.  */

static void
free_text_property (void *object)
{
  MTextProperty *prop = (MTextProperty *) object;

  if (prop->key->managing_key)
    M17N_OBJECT_UNREF (prop->val);
  M17N_OBJECT_UNREGISTER (text_property_table, prop);
  free (object);
}


/** Return a newly allocated text property whose key is KEY and value
    is VAL.  */

static MTextProperty *
new_text_property (MText *mt, int from, int to, MSymbol key, void *val,
		   int control_bits)
{
  MTextProperty *prop;

  M17N_OBJECT (prop, free_text_property, MERROR_TEXTPROP);
  prop->control.flag = control_bits;
  prop->attach_count = 0;
  prop->mt = mt;
  prop->start = from;
  prop->end = to;
  prop->key = key;
  prop->val = val;
  if (key->managing_key)
    M17N_OBJECT_REF (val);    
  M17N_OBJECT_REGISTER (text_property_table, prop);
  return prop;
}


/** Return a newly allocated copy of text property PROP.  */

#define COPY_TEXT_PROPERTY(prop)				\
  new_text_property ((prop)->mt, (prop)->start, (prop)->end,	\
		     (prop)->key, (prop)->val, (prop)->control.flag)


/** Split text property PROP at position INTERVAL->start, and make all
    the following intervals contain the copy of PROP instead of PROP.
    It assumes that PROP starts before INTERVAL.  */

static void
split_property (MTextProperty *prop, MInterval *interval)
{
  int end = prop->end;
  MTextProperty *copy;
  int i;

  prop->end = interval->start;
  copy = COPY_TEXT_PROPERTY (prop);
  copy->start = interval->start;
  copy->end = end;
  /* Check all stacks of the following intervals, and if it contains
     PROP, change it to the copy of it.  */
  for (; interval && interval->start < end; interval = interval->next)
    for (i = 0; i < interval->nprops; i++)
      if (interval->stack[i] == prop)
	{
	  interval->stack[i] = copy;
	  M17N_OBJECT_REF (copy);
	  copy->attach_count++;
	  prop->attach_count--;
	  M17N_OBJECT_UNREF (prop);
	}
  M17N_OBJECT_UNREF (copy);
}

/** Divide INTERVAL of PLIST at POS if POS is in between the range of
    INTERVAL.  */

static void
divide_interval (MTextPlist *plist, MInterval *interval, int pos)
{
  MInterval *new;
  int i;

  if (pos == interval->start || pos == interval->end)
    return;
  new = copy_interval (interval, 0);
  interval->end = new->start = pos;
  new->prev = interval;
  new->next = interval->next;
  interval->next = new;
  if (new->next)
    new->next->prev = new;
  if (plist->tail == interval)
    plist->tail = new;
  for (i = 0; i < new->nprops; i++)
    {
      new->stack[i]->attach_count++;
      M17N_OBJECT_REF (new->stack[i]);
    }
}


/** Check if INTERVAL of PLIST can be merged with INTERVAL->next.  If
    mergeable, extend INTERVAL to the end of INTEVAL->next, free
    INTERVAL->next, and return INTERVAL.  Otherwise, return
    INTERVAL->next.  */

static MInterval *
maybe_merge_interval (MTextPlist *plist, MInterval *interval)
{
  int nprops = interval->nprops;
  MInterval *next = interval->next;
  int i, j;

  if (! next || nprops != next->nprops)
    return next;

  for (i = 0; i < nprops; i++)
    {
      MTextProperty *prop = interval->stack[i];
      MTextProperty *old = next->stack[i];

      if (prop != old
	  && (prop->val != old->val
	      || prop->end != old->start
	      || prop->control.flag & MTEXTPROP_NO_MERGE
	      || old->control.flag & MTEXTPROP_NO_MERGE))
	return interval->next;
    }


  for (i = 0; i < nprops; i++)
    {
      MTextProperty *prop = interval->stack[i];
      MTextProperty *old = next->stack[i];

      if (prop != old)
	{
	  MInterval *tail;

	  for (tail = next->next; tail && tail->start < old->end;
	       tail = tail->next)
	    for (j = 0; j < tail->nprops; j++)
	      if (tail->stack[j] == old)
		{
		  old->attach_count--;
		  xassert (old->attach_count);
		  tail->stack[j] = prop;
		  prop->attach_count++;
		  M17N_OBJECT_REF (prop);
		}
	  xassert (old->attach_count == 1);
	  old->mt = NULL;
	  prop->end = old->end;
	}
      old->attach_count--;
      M17N_OBJECT_UNREF (old);
    }

  interval->end = next->end;
  interval->next = next->next;
  if (next->next)
    next->next->prev = interval;
  if (plist->tail == next)
    plist->tail = interval;
  plist->cache = interval;
  next->nprops = 0;
  free_interval (next);
  return interval;
}


/** Adjust start and end positions of intervals between HEAD and TAIL
     (both inclusive) by diff.  Adjust also start and end positions
     of text properties belonging to those intervals.  */

static void
adjust_intervals (MInterval *head, MInterval *tail, int diff)
{
  int i;
  MTextProperty *prop;

  if (diff < 0)
    {
      /* Adjust end positions of properties starting before HEAD.  */
      for (i = 0; i < head->nprops; i++)
	{
	  prop = head->stack[i];
	  if (prop->start < head->start)
	    prop->end += diff;
	}

      /* Adjust start and end positions of properties starting at
	 HEAD, and adjust HEAD itself.  */
      while (1)
	{
	  for (i = 0; i < head->nprops; i++)
	    {
	      prop = head->stack[i];
	      if (prop->start == head->start)
		prop->start += diff, prop->end += diff;
	    }
	  head->start += diff;
	  head->end += diff;
	  if (head == tail)
	    break;
	  head = head->next;
	}
    }
  else
    {
      /* Adjust start poistions of properties ending after TAIL.  */
      for (i = 0; i < tail->nprops; i++)
	{
	  prop = tail->stack[i];
	  if (prop->end > tail->end)
	    prop->start += diff;
	}

      /* Adjust start and end positions of properties ending at
	 TAIL, and adjust TAIL itself.  */
      while (1)
	{
	  for (i = 0; i < tail->nprops; i++)
	    {
	      prop = tail->stack[i];
	      if (prop->end == tail->end)
		prop->start += diff, prop->end += diff;
	    }
	  tail->start += diff;
	  tail->end += diff;
	  if (tail == head)
	    break;
	  tail = tail->prev;
	}
    }
}

/* Return an interval of PLIST that covers the position POS.  */

static MInterval *
find_interval (MTextPlist *plist, int pos)
{
  MInterval *interval;
  MInterval *highest;

  if (pos < plist->head->end)
    return plist->head;
  if (pos >= plist->tail->start)
    return (pos < plist->tail->end ? plist->tail : NULL);

  interval = plist->cache;

  if (pos < interval->start)
    highest = interval->prev, interval = plist->head->next;
  else if (pos < interval->end)
    return interval;
  else
    highest = plist->tail->prev, interval = interval->next;

  if (pos - interval->start < highest->end - pos)
    {
      while (interval->end <= pos)
	/* Here, we are sure that POS is not included in PLIST->tail,
	   thus, INTERVAL->next always points a valid next
	   interval.  */
	interval = interval->next;
    }
  else
    {
      while (highest->start > pos)
	highest = highest->prev;
      interval = highest;
    }
  plist->cache = interval;
  return interval;
}

/* Push text property PROP on the stack of INTERVAL.  */

#define PUSH_PROP(interval, prop)		\
  do {						\
    int n = (interval)->nprops;			\
						\
    PREPARE_INTERVAL_STACK ((interval), n + 1);	\
    (interval)->stack[n] = (prop);		\
    (interval)->nprops += 1;			\
    (prop)->attach_count++;			\
    M17N_OBJECT_REF (prop);			\
    if ((prop)->start > (interval)->start)	\
      (prop)->start = (interval)->start;	\
    if ((prop)->end < (interval)->end)		\
      (prop)->end = (interval)->end;		\
  } while (0)


/* Pop the topmost text property of INTERVAL from the stack.  If it
   ends after INTERVAL->end, split it.  */

#define POP_PROP(interval)				\
  do {							\
    MTextProperty *prop;				\
							\
    (interval)->nprops--;				\
    prop = (interval)->stack[(interval)->nprops];	\
    xassert (prop->control.ref_count > 0);		\
    xassert (prop->attach_count > 0);			\
    if (prop->start < (interval)->start)		\
      {							\
	if (prop->end > (interval)->end)		\
	  split_property (prop, (interval)->next);	\
	prop->end = (interval)->start;			\
      }							\
    else if (prop->end > (interval)->end)		\
      prop->start = (interval)->end;			\
    prop->attach_count--;				\
    if (! prop->attach_count)				\
      prop->mt = NULL;					\
    M17N_OBJECT_UNREF (prop);				\
  } while (0)


#define REMOVE_PROP(interval, prop)			\
  do {							\
    int i;						\
							\
    for (i = (interval)->nprops - 1; i >= 0; i--)	\
      if ((interval)->stack[i] == (prop))		\
	break;						\
    if (i < 0)						\
      break;						\
    (interval)->nprops--;				\
    for (; i < (interval)->nprops; i++)			\
      (interval)->stack[i] = (interval)->stack[i + 1];	\
    (prop)->attach_count--;				\
    if (! (prop)->attach_count)				\
      (prop)->mt = NULL;				\
    M17N_OBJECT_UNREF (prop);				\
  } while (0)


#ifdef TEXT_PROP_DEBUG
static int
check_plist (MTextPlist *plist, int start)
{
  MInterval *interval = plist->head;
  MInterval *cache = plist->cache;
  int cache_found = 0;

  if (interval->start != start
      || interval->start >= interval->end)
    return mdebug_hook ();
  while (interval)
    {
      int i;

      if (interval == interval->next)
	return mdebug_hook ();

      if (interval == cache)
	cache_found = 1;

      if (interval->start >= interval->end)
	return mdebug_hook ();
      if ((interval->next
	   ? (interval->end != interval->next->start
	      || interval != interval->next->prev)
	   : interval != plist->tail))
	return mdebug_hook ();
      for (i = 0; i < interval->nprops; i++)
	{
	  if (interval->stack[i]->start > interval->start
	      || interval->stack[i]->end < interval->end)
	    return mdebug_hook ();

	  if (! interval->stack[i]->attach_count)
	    return mdebug_hook ();
	  if (! interval->stack[i]->mt)
	    return mdebug_hook ();
	  if (interval->stack[i]->start == interval->start)
	    {
	      MTextProperty *prop = interval->stack[i];
	      int count = prop->attach_count - 1;
	      MInterval *interval2;

	      for (interval2 = interval->next;
		   interval2 && interval2->start < prop->end;
		   count--, interval2 = interval2->next)
		if (count == 0)
		  return mdebug_hook ();
	    }	      

	  if (interval->stack[i]->end > interval->end)
	    {
	      MTextProperty *prop = interval->stack[i];
	      MInterval *interval2;
	      int j;

	      for (interval2 = interval->next;
		   interval2 && interval2->start < prop->end;
		   interval2 = interval2->next)
		{
		  for (j = 0; j < interval2->nprops; j++)
		    if (interval2->stack[j] == prop)
		      break;
		  if (j == interval2->nprops)
		    return mdebug_hook ();
		}
	    }
	  if (interval->stack[i]->start < interval->start)
	    {
	      MTextProperty *prop = interval->stack[i];
	      MInterval *interval2;
	      int j;

	      for (interval2 = interval->prev;
		   interval2 && interval2->end > prop->start;
		   interval2 = interval2->prev)
		{
		  for (j = 0; j < interval2->nprops; j++)
		    if (interval2->stack[j] == prop)
		      break;
		  if (j == interval2->nprops)
		    return mdebug_hook ();
		}
	    }
	}
      interval = interval->next;
    }
  if (! cache_found)
    return mdebug_hook ();
  if (plist->head->prev || plist->tail->next)
    return mdebug_hook ();    
  return 0;
}
#endif


/** Return a copy of plist that contains intervals between FROM and TO
    of PLIST.  The copy goes to the position POS of M-text MT.  */

static MTextPlist *
copy_single_property (MTextPlist *plist, int from, int to, MText *mt, int pos)
{
  MTextPlist *new;
  MInterval *interval1, *interval2;
  MTextProperty *prop;
  int diff = pos - from;
  int i, j;
  int mask_bits = MTEXTPROP_VOLATILE_STRONG | MTEXTPROP_VOLATILE_WEAK;

  MSTRUCT_CALLOC (new, MERROR_TEXTPROP);
  new->key = plist->key;
  new->next = NULL;

  interval1 = find_interval (plist, from);
  new->head = copy_interval (interval1, mask_bits);
  for (interval1 = interval1->next, interval2 = new->head;
       interval1 && interval1->start < to;
       interval1 = interval1->next, interval2 = interval2->next)
    {
      interval2->next = copy_interval (interval1, mask_bits);
      interval2->next->prev = interval2;
    }
  new->tail = interval2;
  new->head->start = from;
  new->tail->end = to;
  for (interval1 = new->head; interval1; interval1 = interval1->next)
    for (i = 0; i < interval1->nprops; i++)
      if (interval1->start == interval1->stack[i]->start
	  || interval1 == new->head)
	{
	  prop = interval1->stack[i];
	  interval1->stack[i] = COPY_TEXT_PROPERTY (prop);
	  interval1->stack[i]->mt = mt;
	  interval1->stack[i]->attach_count++;
	  if (interval1->stack[i]->start < from)
	    interval1->stack[i]->start = from;
	  if (interval1->stack[i]->end > to)
	    interval1->stack[i]->end = to;
	  for (interval2 = interval1->next; interval2;
	       interval2 = interval2->next)
	    for (j = 0; j < interval2->nprops; j++)
	      if (interval2->stack[j] == prop)
		{
		  interval2->stack[j] = interval1->stack[i];
		  interval1->stack[i]->attach_count++;
		  M17N_OBJECT_REF (interval1->stack[i]);
		}
	}
  adjust_intervals (new->head, new->tail, diff);
  new->cache = new->head;
  for (interval1 = new->head; interval1 && interval1->next;
       interval1 = maybe_merge_interval (new, interval1));
  xassert (check_plist (new, pos) == 0);
  if (new->head == new->tail
      && new->head->nprops == 0)
    {
      free_interval (new->head);
      free (new);
      new = NULL;
    }

  return new;
}

/** Return a newly allocated plist whose key is KEY on M-text MT.  */

static MTextPlist *
new_plist (MText *mt, MSymbol key)
{
  MTextPlist *plist;

  MSTRUCT_MALLOC (plist, MERROR_TEXTPROP);
  plist->key = key;
  plist->head = new_interval (0, mtext_nchars (mt));
  plist->tail = plist->head;
  plist->cache = plist->head;
  plist->next = mt->plist;
  mt->plist = plist;
  return plist;
}

/* Free PLIST and return PLIST->next.  */

static MTextPlist *
free_textplist (MTextPlist *plist)
{
  MTextPlist *next = plist->next;
  MInterval *interval = plist->head;

  while (interval)
    {
      while (interval->nprops > 0)
	POP_PROP (interval);
      interval = free_interval (interval);
    }
  free (plist);
  return next;
}

/** Return a plist that contains the property KEY of M-text MT.  If
    such a plist does not exist and CREATE is nonzero, create a new
    plist and return it.  */

static MTextPlist *
get_plist_create (MText *mt, MSymbol key, int create)
{
  MTextPlist *plist;

  plist = mt->plist;
  while (plist && plist->key != key)
    plist = plist->next;

  /* If MT does not have PROP, make one.  */
  if (! plist && create)
    plist = new_plist (mt, key);
  return plist;
}

/* Detach PROP.  INTERVAL (if not NULL) contains PROP.  */

static void
detach_property (MTextPlist *plist, MTextProperty *prop, MInterval *interval)
{
  MInterval *head;
  int to = prop->end;

  xassert (prop->mt);
  xassert (plist);

  M17N_OBJECT_REF (prop);
  if (interval)
    while (interval->start > prop->start)
      interval = interval->prev;
  else
    interval = find_interval (plist, prop->start);
  head = interval;
  while (1)
    {
      REMOVE_PROP (interval, prop);
      if (interval->end == to)
	break;
      interval = interval->next;
    }
  xassert (prop->attach_count == 0 && prop->mt == NULL);
  M17N_OBJECT_UNREF (prop);

  while (head && head->end <= to)
    head = maybe_merge_interval (plist, head);
  xassert (check_plist (plist, 0) == 0);
}

/* Delete text properties of PLIST between FROM and TO.  MASK_BITS
   specifies what kind of properties to delete.  If DELETING is
   nonzero, delete such properties too that are completely included in
   the region.

   If the resulting PLIST still has any text properties, return 1,
   else return 0.  */

static int
delete_properties (MTextPlist *plist, int from, int to,
		   int mask_bits, int deleting)
{
  MInterval *interval;
  int modified = 0;
  int modified_from = from;
  int modified_to = to;
  int i;

 retry:
  for (interval = find_interval (plist, from);
       interval && interval->start < to;
       interval = interval->next)
    for (i = 0; i < interval->nprops; i++)
      {
	MTextProperty *prop = interval->stack[i];

	if (prop->control.flag & mask_bits)
	  {
	    if (prop->start < modified_from)
	      modified_from = prop->start;
	    if (prop->end > modified_to)
	      modified_to = prop->end;
	    detach_property (plist, prop, interval);
	    modified++;
	    goto retry;
	  }
	else if (deleting && prop->start >= from && prop->end <= to)
	  {
	    detach_property (plist, prop, interval);
	    modified++;
	    goto retry;
	  }
      }

  if (modified)
    {
      interval = find_interval (plist, modified_from);
      while (interval && interval->start < modified_to)
	interval = maybe_merge_interval (plist, interval);
    }

  return (plist->head != plist->tail || plist->head->nprops > 0);
}

static void
pop_interval_properties (MInterval *interval)
{
  while (interval->nprops > 0)
    POP_PROP (interval);
}


MInterval *
pop_all_properties (MTextPlist *plist, int from, int to)
{
  MInterval *interval;

  /* Be sure to have interval boundary at TO.  */
  interval = find_interval (plist, to);
  if (interval && interval->start < to)
    divide_interval (plist, interval, to);

  /* Be sure to have interval boundary at FROM.  */
  interval = find_interval (plist, from);
  if (interval->start < from)
    {
      divide_interval (plist, interval, from);
      interval = interval->next;
    }

  pop_interval_properties (interval);
  while (interval->end < to)
    {
      MInterval *next = interval->next;

      pop_interval_properties (next);
      interval->end = next->end;
      interval->next = next->next;
      if (interval->next)
	interval->next->prev = interval;
      if (next == plist->tail)
	plist->tail = interval;
      if (plist->cache == next)
	plist->cache = interval;
      free_interval (next);
    }
  return interval;
}


/* Delete volatile text properties between FROM and TO.  If DELETING
   is nonzero, we are going to delete text, thus both strongly and
   weakly volatile properties must be deleted.  Otherwise we are going
   to modify a text property KEY, thus only strongly volatile
   properties whose key is not KEY must be deleted.  */

static void
prepare_to_modify (MText *mt, int from, int to, MSymbol key, int deleting)
{
  MTextPlist *plist = mt->plist, *prev = NULL;
  int mask_bits = MTEXTPROP_VOLATILE_STRONG;

  if (deleting)
    mask_bits |= MTEXTPROP_VOLATILE_WEAK;
  while (plist)
    {
      if (plist->key != key
	  && ! delete_properties (plist, from, to, mask_bits, deleting))
	{
	  if (prev)
	    plist = prev->next = free_textplist (plist);
	  else
	    plist = mt->plist = free_textplist (plist);
	}
      else
	prev = plist, plist = plist->next;
    }
}

void
extract_text_properties (MText *mt, int from, int to, MSymbol key,
			 MPlist *plist)
{
  MPlist *top;
  MTextPlist *list = get_plist_create (mt, key, 0);
  MInterval *interval;

  if (! list)
    return;
  interval = find_interval (list, from);
  if (interval->nprops == 0
      && interval->start <= from && interval->end >= to)
    return;
  top = plist;
  while (interval && interval->start < to)
    {
      if (interval->nprops == 0)
	top = mplist_find_by_key (top, Mnil);
      else
	{
	  MPlist *current = top, *place;
	  int i;

	  for (i = 0; i < interval->nprops; i++)
	    {
	      MTextProperty *prop = interval->stack[i];

	      place = mplist_find_by_value (current, prop);
	      if (place)
		current = MPLIST_NEXT (place);
	      else
		{
		  place = mplist_find_by_value (top, prop);
		  if (place)
		    {
		      mplist_pop (place);
		      if (MPLIST_NEXT (place) == MPLIST_NEXT (current))
			current = place;
		    }
		  mplist_push (current, Mt, prop);
		  current = MPLIST_NEXT (current);
		}
	    }
	}
      interval = interval->next;
    }
  return;
}

#define XML_TEMPLATE "<?xml version=\"1.0\" ?>\n\
<!DOCTYPE mtext [\n\
  <!ELEMENT mtext (property*,body+)>\n\
  <!ELEMENT property EMPTY>\n\
  <!ELEMENT body (#PCDATA)>\n\
  <!ATTLIST property key CDATA #REQUIRED>\n\
  <!ATTLIST property value CDATA #REQUIRED>\n\
  <!ATTLIST property from CDATA #REQUIRED>\n\
  <!ATTLIST property to CDATA #REQUIRED>\n\
  <!ATTLIST property control CDATA #REQUIRED>\n\
 ]>\n\
<mtext>\n\
</mtext>"


/* for debugging... */
#include <stdio.h>

void
dump_interval (MInterval *interval, int indent)
{
  char *prefix = (char *) alloca (indent + 1);
  int i;

  memset (prefix, 32, indent);
  prefix[indent] = 0;

  fprintf (stderr, "(interval %d-%d (%d)", interval->start, interval->end,
	   interval->nprops);
  for (i = 0; i < interval->nprops; i++)
    fprintf (stderr, "\n%s (%d %d/%d %d-%d 0x%x)",
	     prefix, i,
	     interval->stack[i]->control.ref_count,
	     interval->stack[i]->attach_count,
	     interval->stack[i]->start, interval->stack[i]->end,
	     (unsigned) interval->stack[i]->val);
  fprintf (stderr, ")");
}

void
dump_textplist (MTextPlist *plist, int indent)
{
  char *prefix = (char *) alloca (indent + 1);

  memset (prefix, 32, indent);
  prefix[indent] = 0;

  fprintf (stderr, "(properties");
  if (! plist)
    fprintf (stderr, ")\n");
  else
    {
      fprintf (stderr, "\n");
      while (plist)
	{
	  MInterval *interval = plist->head;

	  fprintf (stderr, "%s (%s", prefix, msymbol_name (plist->key));
	  while (interval)
	    {
	      fprintf (stderr, " (%d %d", interval->start, interval->end);
	      if (interval->nprops > 0)
		{
		  int i;

		  for (i = 0; i < interval->nprops; i++)
		    fprintf (stderr, " 0x%x", (int) interval->stack[i]->val);
		}
	      fprintf (stderr, ")");
	      interval = interval->next;
	    }
	  fprintf (stderr, ")\n");
	  xassert (check_plist (plist, 0) == 0);
	  plist = plist->next;
	}
    }
}


/* Internal API */

int
mtext__prop_init ()
{
  M17N_OBJECT_ADD_ARRAY (text_property_table, "Text property");
  Mtext_prop_serializer = msymbol ("text-prop-serializer");
  Mtext_prop_deserializer = msymbol ("text-prop-deserializer");
  return 0;
}

void
mtext__prop_fini ()
{
  MIntervalPool *pool = interval_pool_root.next;

  while (pool)
    {
      MIntervalPool *next = pool->next;
      free (pool);
      pool = next;
    }
  interval_pool_root.next = NULL;  
}


/** Free all plists.  */

void
mtext__free_plist (MText *mt){

  MTextPlist *plist = mt->plist;

  while (plist)
    plist = free_textplist (plist);
  mt->plist = NULL;
}


/** Extract intervals between FROM and TO of all properties (except
    for volatile ones) in PLIST, and make a new plist from them for
    M-text MT.  */

MTextPlist *
mtext__copy_plist (MTextPlist *plist, int from, int to, MText *mt, int pos)
{
  MTextPlist *copy, *this;

  if (from == to)
    return NULL;
  for (copy = NULL; plist && ! copy; plist = plist->next)
    copy = copy_single_property (plist, from, to, mt, pos);
  if (! plist)
    return copy;
  for (; plist; plist = plist->next)
    if ((this = copy_single_property (plist, from, to, mt, pos)))
      {
	this->next = copy;
	copy = this;
      }

  return copy;
}

void
mtext__adjust_plist_for_delete (MText *mt, int pos, int len)
{
  MTextPlist *plist;
  int to;

  if (len == 0 || pos == mt->nchars)
    return;
  if (len == mt->nchars)
    {
      mtext__free_plist (mt);
      return;
    }      

  to = pos + len;
  prepare_to_modify (mt, pos, to, Mnil, 1);
  for (plist = mt->plist; plist; plist = plist->next)
    {
      MInterval *interval = pop_all_properties (plist, pos, to);
      MInterval *prev = interval->prev, *next = interval->next;

      if (prev)
	prev->next = next;
      else
	plist->head = next;
      if (next)
	{
	  adjust_intervals (next, plist->tail, -len);
	  next->prev = prev;
	}
      else
	plist->tail = prev;
      if (prev && next)
	next = maybe_merge_interval (plist, prev);
      plist->cache = next ? next : prev;
      free_interval (interval);
      xassert (check_plist (plist, 0) == 0);
    }
}

void
mtext__adjust_plist_for_insert (MText *mt, int pos, int nchars,
				MTextPlist *plist)
{
  MTextPlist *pl, *pl_last, *pl2, *p;
  int i;
  MInterval *interval;

  if (mt->nchars == 0)
    {
      mtext__free_plist (mt);
      mt->plist = plist;
      return;
    }
  if (pos > 0 && pos < mtext_nchars (mt))
    prepare_to_modify (mt, pos, pos, Mnil, 0);

  for (pl_last = NULL, pl = mt->plist; pl; pl_last = pl, pl = pl->next)
    {
      MInterval *interval, *prev, *next, *head, *tail;

      if (pos == 0)
	prev = NULL, next = pl->head;
      else if (pos == mtext_nchars (mt))
	prev = pl->tail, next = NULL;
      else
	{
	  next = find_interval (pl, pos);
	  if (next->start < pos)
	    {
	      divide_interval (pl, next, pos);
	      next = next->next;
	    }
	  for (i = 0; i < next->nprops; i++)
	    if (next->stack[i]->start < pos)
	      split_property (next->stack[i], next);
	  prev = next->prev;
	}

      xassert (check_plist (pl, 0) == 0);
      for (p = NULL, pl2 = plist; pl2 && pl->key != pl2->key;
	   p = pl2, pl2 = p->next);
      if (pl2)
	{
	  xassert (check_plist (pl2, pl2->head->start) == 0);
	  if (p)
	    p->next = pl2->next;
	  else
	    plist = plist->next;

	  head = pl2->head;
	  tail = pl2->tail;
	  free (pl2);
	}
      else
	{
	  head = tail = new_interval (pos, pos + nchars);
	}
      head->prev = prev;
      tail->next = next;
      if (prev)
	prev->next = head;
      else
	pl->head = head;
      if (next)
	next->prev = tail;
      else
	pl->tail = tail;
      if (next)
	adjust_intervals (next, pl->tail, nchars);

      xassert (check_plist (pl, 0) == 0);
      if (prev && prev->nprops > 0)
	{
	  for (interval = prev;
	       interval->next != next && interval->next->nprops == 0;
	       interval = interval->next)
	    for (i = 0; i < interval->nprops; i++)
	      {
		MTextProperty *prop = interval->stack[i];

		if (prop->control.flag & MTEXTPROP_REAR_STICKY)
		  PUSH_PROP (interval->next, prop);
	      }
	}
      xassert (check_plist (pl, 0) == 0);
      if (next && next->nprops > 0)
	{
	  for (interval = next;
	       interval->prev != prev && interval->prev->nprops == 0;
	       interval = interval->prev)
	    for (i = 0; i < interval->nprops; i++)
	      {
		MTextProperty *prop = interval->stack[i];

		if (prop->control.flag & MTEXTPROP_FRONT_STICKY)
		  PUSH_PROP (interval->prev, prop);
	      }
	}

      interval = prev ? prev : pl->head;
      pl->cache = interval;
      while (interval && interval->start <= pos + nchars)
	interval = maybe_merge_interval (pl, interval);
      xassert (check_plist (pl, 0) == 0);
    }

  if (pl_last)
    pl_last->next = plist;
  else
    mt->plist = plist;

  for (; plist; plist = plist->next)
    {
      plist->cache = plist->head;
      if (pos > 0)
	{
	  if (plist->head->nprops)
	    {
	      interval = new_interval (0, pos);
	      interval->next = plist->head;
	      plist->head->prev = interval;
	      plist->head = interval;
	    }
	  else
	    plist->head->start = 0;
	}
      if (pos < mtext_nchars (mt))
	{
	  if (plist->tail->nprops)
	    {
	      interval = new_interval (pos + nchars,
				       mtext_nchars (mt) + nchars);
	      interval->prev = plist->tail;
	      plist->tail->next = interval;
	      plist->tail = interval;
	    }
	  else
	    plist->tail->end = mtext_nchars (mt) + nchars;
	}
      xassert (check_plist (plist, 0) == 0);
    }
}

/* len1 > 0 && len2 > 0 */

void
mtext__adjust_plist_for_change (MText *mt, int pos, int len1, int len2)
{
  int pos2 = pos + len1;

  prepare_to_modify (mt, pos, pos2, Mnil, 0);

  if (len1 < len2)
    {
      int diff = len2 - len1;
      MTextPlist *plist;

      for (plist = mt->plist; plist; plist = plist->next)
	{
	  MInterval *head = find_interval (plist, pos2);
	  MInterval *tail = plist->tail;
	  MTextProperty *prop;
	  int i;

	  if (head)
	    {
	      if (head->start == pos2)
		head = head->prev;
	      while (tail != head)
		{
		  for (i = 0; i < tail->nprops; i++)
		    {
		      prop = tail->stack[i];
		      if (prop->start == tail->start)
			prop->start += diff, prop->end += diff;
		    }
		  tail->start += diff;
		  tail->end += diff;
		  tail = tail->prev;
		}
	    }
	  for (i = 0; i < tail->nprops; i++)
	    tail->stack[i]->end += diff;
	  tail->end += diff;
	}
    }
  else if (len1 > len2)
    {
      mtext__adjust_plist_for_delete (mt, pos + len2, len1 - len2);
    }
}


/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/** External API */

/*** @addtogroup m17nTextProperty */
/*** @{  */

/*=*/
/***en
    @brief Get the value of the topmost text property.

    The mtext_get_prop () function searches the character at $POS in
    M-text $MT for the text property whose key is $KEY.

    @return
    If a text property is found, mtext_get_prop () returns the value
    of the property.  If the property has multiple values, it returns
    the topmost one.  If no such property is found, it returns @c NULL
    without changing the external variable #merror_code.

    If an error is detected, mtext_get_prop () returns @c NULL and
    assigns an error code to the external variable #merror_code.

    @note If @c NULL is returned without an error, there are two
    possibilities:

    @li  the character at $POS does not have a property whose key is $KEY, or 

    @li  the character does have such a property and its value is @c NULL.  

    If you need to distinguish these two cases, use the
    mtext_get_prop_values () function instead.  */

/***ja
    @brief テキストプロパティの一番上の値を得る.

    関数 mtext_get_prop () は、M-text $MT 内の位置 $POS にある文字のテ
    キストプロパティのうち、キーが $KEY であるものを探す。

    @return
    テキストプロパティがみつかれば、mtext_get_prop () はそのプロパティ
    の値を返す。値が複数存在するときは、一番上の値を返す。見つからなけ
    れば外部変数 #merror_code を変更することなく @c NULL を返す。

    エラーが検出された場合 mtext_get_prop () は @c NULL を返し、外部変
    数 #merror_code にエラーコードを設定する。

    @note エラーなしで @c NULL が返された場合には二つの可能性がある。

    @li $POS の位置の文字は $KEY をキーとするプロパティを持たない。

    @li その文字はそのようなプロパティを持ち、その値が @c NULL である。

    この二つを区別する必要がある場合には、関数 mtext_get_prop_values ()
    を代わりに使用すること。

     @latexonly \IPAlabel{mtext_get_prop} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE, @c MERROR_SYMBOL

    @seealso
    mtext_get_prop_values (), mtext_put_prop (), mtext_put_prop_values (),
    mtext_push_prop (), mtext_pop_prop (), mtext_prop_range ()  */

void *
mtext_get_prop (MText *mt, int pos, MSymbol key)
{
  MTextPlist *plist;
  MInterval *interval;
  void *val;

  M_CHECK_POS (mt, pos, NULL);

  plist = get_plist_create (mt, key, 0);
  if (! plist)
    return NULL;

  interval = find_interval (plist, pos);
  val = (interval->nprops
	 ? interval->stack[interval->nprops - 1]->val : NULL);
  return val;
}

/*=*/

/***en
    @brief Get multiple values of a text property.

    The mtext_get_prop_values () function searches the character at
    $POS in M-text $MT for the property whose key is $KEY.  If such
    a property is found, its values are stored in the memory area
    pointed to by $VALUES.  $NUM limits the maximum number of stored
    values.

    @return
    If the operation was successful, mtext_get_prop_values () returns
    the number of actually stored values.  If the character at $POS
    does not have a property whose key is $KEY, the return value is
    0. If an error is detected, mtext_get_prop_values () returns -1 and
    assigns an error code to the external variable #merror_code.  */

/***ja
    @brief テキストプロパティの値を複数個得る.

    関数 mtext_get_prop_values () は、M-text $MT 内で $POS という位置
    にある文字のプロパティのうち、キーが $KEY であるものを探す。もしそ
    のようなプロパティが見つかれば、それが持つ値 (複数可) を $VALUES 
    の指すメモリ領域に格納する。$NUM は格納する値の数の上限である。

    @return
    処理が成功すれば、mtext_get_prop_values () は実際にメモリに格納さ
    れた値の数を返す。$POS の位置の文字が $KEY をキーとするプロパティ
    を持たなければ 0 を返す。エラーが検出された場合は -1 を返し、外部
    変数 #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_get_prop_values} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE, @c MERROR_SYMBOL

    @seealso
    mtext_get_prop (), mtext_put_prop (), mtext_put_prop_values (),
    mtext_push_prop (), mtext_pop_prop (), mtext_prop_range ()  */

int
mtext_get_prop_values (MText *mt, int pos, MSymbol key,
		       void **values, int num)
{
  MTextPlist *plist;
  MInterval *interval;
  int nprops;
  int i;
  int offset;

  M_CHECK_POS (mt, pos, -1);

  plist = get_plist_create (mt, key, 0);
  if (! plist)
    return 0;

  interval = find_interval (plist, pos);
  /* It is assured that INTERVAL is not NULL.  */
  nprops = interval->nprops;
  if (nprops == 0 || num <= 0)
    return 0;
  if (nprops == 1 || num == 1)
    {
      values[0] = interval->stack[nprops - 1]->val;
      return 1;
    }

  if (nprops <= num)
    num = nprops, offset = 0;
  else
    offset = nprops - num;
  for (i = 0; i < num; i++)
    values[i] = interval->stack[offset + i]->val;
  return num;
}

/*=*/

/***en
    @brief Get a list of text property keys at a position of an M-text.

    The mtext_get_prop_keys () function creates an array whose
    elements are the keys of text properties found at position $POS in
    M-text $MT, and sets *$KEYS to the address of the created array.
    The user is responsible to free the memory allocated for
    the array.

    @returns
    If the operation was successful, mtext_get_prop_keys () returns
    the length of the key list.  Otherwise it returns -1 and assigns
    an error code to the external variable #merror_code.

*/

/***ja
    @brief M-text の指定した位置のテキストプロパティのキーのリストを得る.

    関数 mtext_get_prop_keys () は、M-text $MT 内で $POS の位置にある
    すべてのテキストプロパティのキーを要素とする配列を作り、その配列の
    アドレスを *$KEYS に設定する。この配列のために確保されたメモリを解
    放するのはユーザの責任である。

    @return
    処理が成功すれば mtext_get_prop_keys () は得られたリストの長さを返
    す。そうでなければ -1 を返し、外部変数 #merror_code にエラーコードを
    設定する。
*/

/***
    @errors
    @c MERROR_RANGE

    @seealso
    mtext_get_prop (), mtext_put_prop (), mtext_put_prop_values (),
    mtext_get_prop_values (), mtext_push_prop (), mtext_pop_prop ()  */

int
mtext_get_prop_keys (MText *mt, int pos, MSymbol **keys)
{
  MTextPlist *plist;
  int i;

  M_CHECK_POS (mt, pos, -1);
  for (i = 0, plist = mt->plist; plist; i++, plist = plist->next);
  if (i == 0)
    {
      *keys = NULL;
      return 0;
    }
  MTABLE_MALLOC (*keys, i, MERROR_TEXTPROP);
  for (i = 0, plist = mt->plist; plist; plist = plist->next)
    {
      MInterval *interval = find_interval (plist, pos);

      if (interval->nprops)
	(*keys)[i++] = plist->key;
    }
  return i;
}

/*=*/

/***en
    @brief Set a text property.

    The mtext_put_prop () function sets a text property to the
    characters between $FROM (inclusive) and $TO (exclusive) in M-text
    $MT.  $KEY and $VAL specify the key and the value of the text
    property.  With this function,

@verbatim
                     FROM                   TO
M-text: |<------------|-------- MT ---------|------------>|
PROP  :  <------------------ OLD_VAL -------------------->
@endverbatim

   becomes

@verbatim
                     FROM                   TO
M-text: |<------------|-------- MT ---------|------------>|
PROP  :  <-- OLD_VAL-><-------- VAL -------><-- OLD_VAL-->
@endverbatim

    @return
    If the operation was successful, mtext_put_prop () returns 0.
    Otherwise it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief テキストプロパティを設定する.

    関数 mtext_put_prop () は、M-text $MT の $FROM （含まれる）から 
    $TO （含まれない）の範囲の文字に、キーが $KEY で値が $VAL であるよ
    うなテキストプロパティを設定する。この関数によって


@verbatim
                         FROM                    TO
M-text:      |<------------|-------- MT ---------|------------>|
PROP:         <------------------ OLD_VAL -------------------->
@endverbatim

は次のようになる。

@verbatim
                         FROM                    TO
M-text:       |<------------|-------- MT ---------|------------>|
PROP:          <-- OLD_VAL-><-------- VAL -------><-- OLD_VAL-->
@endverbatim

    @return
    処理が成功すれば mtext_put_prop () は 0 を返す。そうでなければ -1 
    を返し、外部変数 #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_put_prop} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE, @c MERROR_SYMBOL

    @seealso
    mtext_put_prop_values (), mtext_get_prop (),
    mtext_get_prop_values (), mtext_push_prop (),
    mtext_pop_prop (), mtext_prop_range ()  */

int
mtext_put_prop (MText *mt, int from, int to, MSymbol key, void *val)
{
  MTextPlist *plist;
  MTextProperty *prop;
  MInterval *interval;

  M_CHECK_RANGE (mt, from, to, -1, 0);

  prepare_to_modify (mt, from, to, key, 0);
  plist = get_plist_create (mt, key, 1);
  interval = pop_all_properties (plist, from, to);
  prop = new_text_property (mt, from, to, key, val, 0);
  PUSH_PROP (interval, prop);
  M17N_OBJECT_UNREF (prop);
  if (interval->next)
    maybe_merge_interval (plist, interval);
  if (interval->prev)
    maybe_merge_interval (plist, interval->prev);
  xassert (check_plist (plist, 0) == 0);
  return 0;
}

/*=*/

/***en
    @brief Set multiple text properties with the same key.

    The mtext_put_prop_values () function sets a text property to the
    characters between $FROM (inclusive) and $TO (exclusive) in M-text
    $MT.  $KEY and $VALUES specify the key and the values of the text
    property.  $NUM specifies the number of property values to be set.

    @return
    If the operation was successful, mtext_put_prop_values () returns
    0.  Otherwise it returns -1 and assigns an error code to the
    external variable #merror_code.  */

/***ja
    @brief 同じキーのテキストプロパティを複数設定する.

    関数 mtext_put_prop_values () は、M-Text $MT の$FROM （含まれる）
    から $TO （含まれない）の範囲の文字に、テキストプロパティを設定す
    る。テキストプロパティのキーは $KEY によって、値(複数可)は $VALUES 
    によって指定される。$NUM は設定される値の個数である。

    @return
    処理が成功すれば、mtext_put_prop_values () は 0 を返す。そうでなけ
    れば -1 を返し、外部変数 #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_put_prop_values} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE, @c MERROR_SYMBOL

    @seealso
    mtext_put_prop (), mtext_get_prop (), mtext_get_prop_values (),
    mtext_push_prop (), mtext_pop_prop (), mtext_prop_range ()  */

int
mtext_put_prop_values (MText *mt, int from, int to,
		       MSymbol key, void **values, int num)
{
  MTextPlist *plist;
  MInterval *interval;
  int i;

  M_CHECK_RANGE (mt, from, to, -1, 0);

  prepare_to_modify (mt, from, to, key, 0);
  plist = get_plist_create (mt, key, 1);
  interval = pop_all_properties (plist, from, to);
  if (num > 0)
    {
      PREPARE_INTERVAL_STACK (interval, num);
      for (i = 0; i < num; i++)
	{
	  MTextProperty *prop
	    = new_text_property (mt, from, to, key, values[i], 0);
	  PUSH_PROP (interval, prop);
	  M17N_OBJECT_UNREF (prop);
	}
    }
  if (interval->next)
    maybe_merge_interval (plist, interval);
  if (interval->prev)
    maybe_merge_interval (plist, interval->prev);
  xassert (check_plist (plist, 0) == 0);
  return 0;
}

/*=*/

/***en
    @brief Push a text property.

    The mtext_push_prop () function pushes a text property whose key
    is $KEY and value is $VAL to the characters between $FROM
    (inclusive) and $TO (exclusive) in M-text $MT.  With this
    function,

@verbatim
                    FROM                    TO
M-text: |<------------|-------- MT ---------|------------>|
PROP  :  <------------------ OLD_VAL -------------------->
@endverbatim

    becomes

@verbatim 
                    FROM                    TO
M-text: |<------------|-------- MT ---------|------------>|
PROP  :  <------------------- OLD_VAL ------------------->
PROP  :               <-------- VAL ------->
@endverbatim

    @return
    If the operation was successful, mtext_push_prop () returns 0.
    Otherwise it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief テキストプロパティをプッシュする.

    関数 mtext_push_prop () は、キーが $KEY で値が $VAL であるテキスト
    プロパティを、M-text $MT 中の $FROM （含まれる）から $TO （含まれな
    い）の範囲の文字にプッシュする。この関数によって

@verbatim
                    FROM                    TO
M-text: |<------------|-------- MT ---------|------------>|
PROP  :  <------------------ OLD_VAL -------------------->
@endverbatim
 は次のようになる。
@verbatim 
                    FROM                    TO
M-text: |<------------|-------- MT ---------|------------>|
PROP  :  <------------------- OLD_VAL ------------------->
PROP  :               <-------- VAL ------->
@endverbatim

    @return
    処理が成功すれば、mtext_push_prop () は 0 を返す。そうでなければ 
    -1 を返し、外部変数 #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_push_prop} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE, @c MERROR_SYMBOL

    @seealso
    mtext_put_prop (), mtext_put_prop_values (),
    mtext_get_prop (), mtext_get_prop_values (),
    mtext_pop_prop (), mtext_prop_range ()  */

int
mtext_push_prop (MText *mt, int from, int to,
		 MSymbol key, void *val)
{
  MTextPlist *plist;
  MInterval *head, *tail, *interval;
  MTextProperty *prop;
  int check_head, check_tail;

  M_CHECK_RANGE (mt, from, to, -1, 0);

  prepare_to_modify (mt, from, to, key, 0);
  plist = get_plist_create (mt, key, 1);

  /* Find an interval that covers the position FROM.  */
  head = find_interval (plist, from);

  /* If the found interval starts before FROM, divide it at FROM.  */
  if (head->start < from)
    {
      divide_interval (plist, head, from);
      head = head->next;
      check_head = 0;
    }
  else
    check_head = 1;

  /* Find an interval that ends at TO.  If TO is not at the end of an
     interval, make one that ends at TO.  */
  if (head->end == to)
    {
      tail = head;
      check_tail = 1;
    }
  else if (head->end > to)
    {
      divide_interval (plist, head, to);
      tail = head;
      check_tail = 0;
    }
  else
    {
      tail = find_interval (plist, to);
      if (! tail)
	{
	  tail = plist->tail;
	  check_tail = 0;
	}
      else if (tail->start == to)
	{
	  tail = tail->prev;
	  check_tail = 1;
	}
      else
	{
	  divide_interval (plist, tail, to);
	  check_tail = 0;
	}
    }

  prop = new_text_property (mt, from, to, key, val, 0);

  /* Push PROP to the current values of intervals between HEAD and TAIL
     (both inclusive).  */
  for (interval = head; ; interval = interval->next)
    {
      PUSH_PROP (interval, prop);
      if (interval == tail)
	break;
    }

  M17N_OBJECT_UNREF (prop);

  /* If there is a possibility that TAIL now has the same value as the
     next one, check it and concatenate them if necessary.  */
  if (tail->next && check_tail)
    maybe_merge_interval (plist, tail);

  /* If there is a possibility that HEAD now has the same value as the
     previous one, check it and concatenate them if necessary.  */
  if (head->prev && check_head)
    maybe_merge_interval (plist, head->prev);

  xassert (check_plist (plist, 0) == 0);
  return 0;
}

/*=*/

/***en
    @brief Pop a text property.

    The mtext_pop_prop () function removes the topmost text property
    whose key is $KEY from the characters between $FROM (inclusive)
    and and $TO (exclusive) in $MT.

    This function does nothing if characters in the region have no
    such text property. With this function,

@verbatim
                    FROM                    TO
M-text: |<------------|-------- MT ---------|------------>|
PROP  :  <------------------ OLD_VAL -------------------->
@endverbatim

    becomes

@verbatim 
                    FROM                    TO
M-text: |<------------|-------- MT ---------|------------>|
PROP  :  <--OLD_VAL-->|                     |<--OLD_VAL-->|
@endverbatim

    @return
    If the operation was successful, mtext_pop_prop () return 0.
    Otherwise it returns -1 and assigns an error code to the external
    variable #merror_code.  */

/***ja
    @brief テキストプロパティをポップする.

    関数 mtext_pop_prop () は、キーが $KEY であるテキストプロパティの
    うち一番上のものを、M-text $MT の $FROM （含まれる）から $TO（含ま
    れない）の範囲の文字から取り除く。

    指定範囲の文字がそのようなプロパティを持たないならば、この関数は何
    もしない。この関数によって、

@verbatim
                    FROM                    TO
M-text: |<------------|-------- MT ---------|------------>|
PROP  :  <------------------ OLD_VAL -------------------->
@endverbatim
 は以下のようになる。
@verbatim 
                    FROM                    TO
M-text: |<------------|-------- MT ---------|------------>|
PROP  :  <--OLD_VAL-->|                     |<--OLD_VAL-->|
@endverbatim

    @return
    処理が成功すれば、mtext_pop_prop () は 0 を返す。そうでなければ -1 
    を返し、外部変数 #merror_code にエラーコードを設定する。

    @latexonly \IPAlabel{mtext_pop_prop} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE, @c MERROR_SYMBOL

    @seealso
    mtext_put_prop (), mtext_put_prop_values (),
    mtext_get_prop (), mtext_get_prop_values (),
    mtext_push_prop (), mtext_prop_range ()  */

int
mtext_pop_prop (MText *mt, int from, int to, MSymbol key)
{
  MTextPlist *plist;
  MInterval *head, *tail;
  int check_head = 1;

  if (key == Mnil)
    MERROR (MERROR_TEXTPROP, -1);
  M_CHECK_RANGE (mt, from, to, -1, 0);
  plist = get_plist_create (mt, key, 0);
  if (! plist)
    return 0;

  /* Find an interval that covers the position FROM.  */
  head = find_interval (plist, from);
  if (head->end >= to
      && head->nprops == 0)
    /* No property to pop.  */
    return 0;

  prepare_to_modify (mt, from, to, key, 0);

  /* If the found interval starts before FROM and has value(s), divide
     it at FROM.  */
  if (head->start < from)
    {
      if (head->nprops > 0)
	{
	  divide_interval (plist, head, from);
	  check_head = 0;
	}
      else
	from = head->end;
      head = head->next;
    }

  /* Pop the topmost text property from each interval following HEAD.
     Stop at an interval that ends after TO.  */
  for (tail = head; tail && tail->end <= to; tail = tail->next)
    if (tail->nprops > 0)
      POP_PROP (tail);

  if (tail)
    {
      if (tail->start < to)
	{
	  if (tail->nprops > 0)
	    {
	      divide_interval (plist, tail, to);
	      POP_PROP (tail);
	    }
	  to = tail->start;
	}
      else
	to = tail->end;
    }
  else
    to = plist->tail->start;

  /* If there is a possibility that HEAD now has the same text
     properties as the previous one, check it and concatenate them if
     necessary.  */
  if (head->prev && check_head)
    head = head->prev;
  while (head && head->end <= to)
    head = maybe_merge_interval (plist, head);

  xassert (check_plist (plist, 0) == 0);
  return 0;
}

/*=*/

/***en
    @brief Find the range where the value of a text property is the same.

    The mtext_prop_range () function investigates the extent where all
    characters have the same value for a text property.  It first
    finds the value of the property specified by $KEY of the character
    at $POS in M-text $MT.  Then it checks if adjacent characters have
    the same value for the property $KEY.  The beginning and the end
    of the found range are stored to the variable pointed to by $FROM
    and $TO.  The character position stored in $FROM is inclusive but
    that in $TO is exclusive; this fashion is compatible with the
    range specification in the mtext_put_prop () function, etc.

    If $DEEPER is not 0, not only the topmost but also all the stacked
    properties whose key is $KEY are compared.

    If $FROM is @c NULL, the beginning of range is not searched for.  If
    $TO is @c NULL, the end of range is not searched for.

    @return

    If the operation was successful, mtext_prop_range () returns the
    number of values the property $KEY has at pos.  Otherwise it
    returns -1 and assigns an error code to the external variable @c
    merror_code.  */

/***ja
    @brief テキストプロパティが同じ値をとる範囲を調べる.

    関数 mtext_prop_range () は、指定したテキストプロパティの値が同じ
    である連続した文字の範囲を調べる。まず M-text $MT の $POS の位置に
    ある文字のプロパティのうち、キー $KEY で指定されたもの値を見つけ
    る。そして前後の文字も $KEY のプロパティの値が同じであるかどうかを
    調べる。見つけた範囲の最初と最後を、それぞれ $FROM と $TO にポイン
    トされる変数に保存する。$FROM に保存される文字の位置は見つけた範囲
    に含まれるが、$TO は含まれない。（$TO の前で同じ値をとる範囲は終わ
    る。）この範囲指定法は、関数 mtext_put_prop () などと共通である。

    $DEEPER が 0 でなければ、$KEY というキーを持つプロパティのうち一番
    上のものだけでなく、スタック中のすべてのものが比較される。

    $FROM が @c NULL ならば、範囲の始まりは探索しない。$TO が @c NULL 
    ならば、範囲の終りは探索しない。

    @return
    処理が成功すれば、mtext_prop_range () は $KEY プロパティの値の数を
    返す。そうでなければ-1 を返し、 外部変数 #merror_code にエラーコー
    ドを設定する。

    @latexonly \IPAlabel{mtext_prop_range} @endlatexonly  */

/***
    @errors
    @c MERROR_RANGE, @c MERROR_SYMBOL

    @seealso
    mtext_put_prop (), mtext_put_prop_values (),
    mtext_get_prop (), mtext_get_prop_values (), 
    mtext_pop_prop (), mtext_push_prop ()  */

int
mtext_prop_range (MText *mt, MSymbol key, int pos,
		  int *from, int *to, int deeper)
{
  MTextPlist *plist;
  MInterval *interval, *temp;
  void *val;
  int nprops;

  M_CHECK_POS (mt, pos, -1);

  plist = get_plist_create (mt, key, 0);
  if (! plist)
    {
      if (from) *from = 0;
      if (to) *to = mtext_nchars (mt);
      return 0;
    }

  interval = find_interval (plist, pos);
  nprops = interval->nprops;
  if (deeper || ! nprops)
    {
      if (from) *from = interval->start;
      if (to) *to = interval->end;
      return interval->nprops;
    }

  val = nprops ? interval->stack[nprops - 1] : NULL;

  if (from)
    {
      for (temp = interval;
	   temp->prev
	     && (temp->prev->nprops
		 ? (nprops
		    && (val == temp->prev->stack[temp->prev->nprops - 1]))
		 : ! nprops);
	   temp = temp->prev);
      *from = temp->start;
    }

  if (to)
    {
      for (temp = interval;
	   temp->next
	     && (temp->next->nprops
		 ? (nprops
		    && val == temp->next->stack[temp->next->nprops - 1])
		 : ! nprops);
	   temp = temp->next);
      *to = temp->end;
    }

  return nprops;
}

/***en
    @brief Create a text property.

    The mtext_property () function returns a newly allocated text
    property whose key is $KEY and value is $VAL.  The created text 
    property is not attached to any M-text, i.e. it is detached.

    $CONTROL_BITS must be 0 or logical OR of @c enum @c
    MTextPropertyControl.  */

/***ja
    @brief テキストプロパティを生成する.

    関数 mtext_property () は $KEY をキー、$VAL を値とする新しく割り当
    てられたテキストプロパティを返す。生成したテキストプロパティはいか
    なる M-text にも付加されていない、すなわち分離して (detached) いる。

    $CONTROL_BITS は 0 であるか @c enum @c MTextPropertyControl の論理 
    OR でなくてはならない。  */

MTextProperty *
mtext_property (MSymbol key, void *val, int control_bits)
{
  return new_text_property (NULL, 0, 0, key, val, control_bits);
}

/***en
    @brief Return the M-text of a text property.

    The mtext_property_mtext () function returns the M-text to which
    text property $PROP is attached.  If $PROP is currently detached,
    NULL is returned.  */

/***ja
    @brief あるテキストプロパティを持つ M-text を返す.

    関数 mtext_property_mtext () は、テキストプロパティ$PROP が付加さ
    れている M-text を返す。その時点で $PROP が分離していれば NULL を
    返す。  */

MText *
mtext_property_mtext (MTextProperty *prop)
{
  return prop->mt;
}

/***en
    @brief Return the key of a text property.

    The mtext_property_key () function returns the key (symbol) of
    text property $PROP.  */

/***ja
    @brief テキストプロパティのキーを返す.

    関数 mtext_property_key () は、テキストプロパティ $PROP のキー（シ
    ンボル）を返す。  */

MSymbol
mtext_property_key (MTextProperty *prop)
{
  return prop->key;
}

/***en
    @brief Return the value of a text property.

    The mtext_property_value () function returns the value of text
    property $PROP.  */

/***ja
    @brief テキストプロパティの値を返す.

    関数 mtext_property_value () は、テキストプロパティ $PROP の値を返
    す。  */

void *
mtext_property_value (MTextProperty *prop)
{
  return prop->val;
}

/***en
    @brief Return the start position of a text property.

    The mtext_property_start () function returns the start position of
    text property $PROP.  The start position is a character position
    of an M-text where $PROP begins.  If $PROP is detached, it returns
    -1.  */

/***ja
    @brief テキストプロパティの開始位置を返す.

    関数 mtext_property_start () は、テキストプロパティ $PROP の開始位
    置を返す。開始位置とは M-text 中で $PROP が始まる文字位置である。
    $PROP が分離されていれば、-1 を返す。  */

int
mtext_property_start (MTextProperty *prop)
{
  return (prop->mt ? prop->start : -1);
}

/***en
    @brief Return the end position of a text property.

    The mtext_property_end () function returns the end position of
    text property $PROP.  The end position is a character position of
    an M-text where $PROP ends.  If $PROP is detached, it returns
    -1.  */

/***ja
    @brief テキストプロパティの終了位置を返す.

    関数 mtext_property_end () は、テキストプロパティ $PROP の終了位置
    を返す。終了位置とは M-text 中で $PROP が終る文字位置である。$PROP 
    が分離されていれば、-1 を返す。  */

int
mtext_property_end (MTextProperty *prop)
{
  return (prop->mt ? prop->end : -1);
}

/***en
    @brief Get the topmost text property.

    The mtext_get_property () function searches the character at
    position $POS in M-text $MT for a text property whose key is $KEY.

    @return
    If a text property is found, mtext_get_property () returns it.  If
    there are multiple text properties, it returns the topmost one.
    If no such property is found, it returns @c NULL without changing
    the external variable #merror_code.

    If an error is detected, mtext_get_property () returns @c NULL and
    assigns an error code to the external variable #merror_code.  */

/***ja
    @brief 一番上のテキストプロパティを得る.

    関数 mtext_get_property () は M-text $MT の位置 $POS の文字がキー
    が $KEY であるテキストプロパティを持つかどうかを調べる。

    @return 
    テキストプロパティが見つかれば、mtext_get_property () はそれを返す。
    複数ある場合には、一番上のものを返す。見つからなければ、外部変数 
    #merror_code を変えることなく @c NULL を返す。

    エラーが検出された場合 mtext_get_property () は @c NULL を返し、外
    部変数 #merror_code にエラーコードを設定する。    */

MTextProperty *
mtext_get_property (MText *mt, int pos, MSymbol key)
{
  MTextPlist *plist;
  MInterval *interval;

  M_CHECK_POS (mt, pos, NULL);

  plist = get_plist_create (mt, key, 0);
  if (! plist)
    return NULL;

  interval = find_interval (plist, pos);
  if (! interval->nprops)
    return NULL;
  return interval->stack[interval->nprops - 1];
}

/***en
    @brief Get multiple text properties.

    The mtext_get_properties () function searches the character at
    $POS in M-text $MT for properties whose key is $KEY.  If such
    properties are found, they are stored in the memory area pointed
    to by $PROPS.  $NUM limits the maximum number of stored
    properties.

    @return
    If the operation was successful, mtext_get_properties () returns
    the number of actually stored properties.  If the character at
    $POS does not have a property whose key is $KEY, the return value
    is 0. If an error is detected, mtext_get_properties () returns -1
    and assigns an error code to the external variable #merror_code.  */

/***ja
    @brief 複数のテキストプロパティを得る.

    関数 mtext_get_properties () は M-text $MT の位置 $POS の文字がキー
    が $KEY であるテキストプロパティを持つかどうかを調べる。そのような
    プロパティがみつかれば、$PROPS が指すメモリ領域に保存する。$NUM は
    保存されるプロパティの数の上限である。

    @return 
    処理が成功すれば、mtext_get_properties () は実際に保存したプロパティ
    の数を返す。$POS の位置の文字がキーが $KEY であるプロパティを持た
    なければ、0 が返る。エラーが検出された場合には、
    mtext_get_properties () は -1 を返し、外部変数 #merror_code にエラー
    コードを設定する。  */

int
mtext_get_properties (MText *mt, int pos, MSymbol key,
		      MTextProperty **props, int num)
{
  MTextPlist *plist;
  MInterval *interval;
  int nprops;
  int i;
  int offset;

  M_CHECK_POS (mt, pos, -1);

  plist = get_plist_create (mt, key, 0);
  if (! plist)
    return 0;

  interval = find_interval (plist, pos);
  /* It is assured that INTERVAL is not NULL.  */
  nprops = interval->nprops;
  if (nprops == 0 || num <= 0)
    return 0;
  if (nprops == 1 || num == 1)
    {
      props[0] = interval->stack[nprops - 1];
      return 1;
    }

  if (nprops <= num)
    num = nprops, offset = 0;
  else
    offset = nprops - num;
  for (i = 0; i < num; i++)
    props[i] = interval->stack[offset + i];
  return num;
}

/***en
    @brief Attach a text property to an M-text.

    The mtext_attach_property () function attaches text property $PROP
    to the range between $FROM and $TO in M-text $MT.  If $PROP is
    already attached to an M-text, it is detached before attached to
    $MT.

    @return
    If the operation was successful, mtext_attach_property () returns
    0.  Otherwise it returns -1 and assigns an error code to the
    external variable #merror_code.  */

/***ja
    @brief  M-textにテキストプロパティを付加する.

    関数 mtext_attach_property () は、M-text $MT の $FROM から $TO ま
    での領域にテキストプロパティ $PROP を付加する。もし $PROP が既に
    M-text に付加されていれば、$MT に付加する前に分離される。

    @return 
    処理に成功すれば、mtext_attach_property () は 0 を返す。そうでなけ
    れば -1 を返して外部変数#merror_code にエラーコードを設定する。      */


int
mtext_attach_property (MText *mt, int from, int to, MTextProperty *prop)
{     
  MTextPlist *plist;
  MInterval *interval;

  M_CHECK_RANGE (mt, from, to, -1, 0);

  M17N_OBJECT_REF (prop);
  if (prop->mt)
    mtext_detach_property (prop);
  prepare_to_modify (mt, from, to, prop->key, 0);
  plist = get_plist_create (mt, prop->key, 1);
  xassert (check_plist (plist, 0) == 0);
  interval = pop_all_properties (plist, from, to);
  xassert (check_plist (plist, 0) == 0);
  prop->mt = mt;
  prop->start = from;
  prop->end = to;
  PUSH_PROP (interval, prop);
  M17N_OBJECT_UNREF (prop);
  xassert (check_plist (plist, 0) == 0);
  if (interval->next)
    maybe_merge_interval (plist, interval);
  if (interval->prev)
    maybe_merge_interval (plist, interval->prev);
  xassert (check_plist (plist, 0) == 0);
  return 0;
}

/***en
    @brief Detach a text property from an M-text.

    The mtext_detach_property () function makes text property $PROP
    detached.

    @return
    This function always returns 0.  */

/***ja
    @brief  M-text からテキストプロパティを分離する.

    関数 mtext_detach_property () はテキストプロパティ $PROP を分離する。

    @return
    この関数は常に 0 を返す。  */

int
mtext_detach_property (MTextProperty *prop)
{
  MTextPlist *plist;
  int start = prop->start, end = prop->end;

  if (! prop->mt)
    return 0;
  prepare_to_modify (prop->mt, start, end, prop->key, 0);
  plist = get_plist_create (prop->mt, prop->key, 0);
  xassert (plist);
  detach_property (plist, prop, NULL);
  return 0;
}

/***en
    @brief Push a text property onto an M-text.

    The mtext_push_property () function pushes text property $PROP to
    the characters between $FROM (inclusive) and $TO (exclusive) in
    M-text $MT.

    @return
    If the operation was successful, mtext_push_property () returns
    0.  Otherwise it returns -1 and assigns an error code to the
    external variable #merror_code.  */

/***ja
    @brief M-text にテキストプロパティをプッシュする.

    関数 mtext_push_property () は、テキストプロパティ $PROP を、
    M-text $MT 中の $FROM （含まれる）から $TO （含まれない）の範囲の
    文字にプッシュする。

    @return
    処理に成功すれば、mtext_push_property () は 0 を返す。そうでなけ
    れば -1 を返して外部変数#merror_code にエラーコードを設定する。      */


int
mtext_push_property (MText *mt, int from, int to, MTextProperty *prop)
{
  MTextPlist *plist;
  MInterval *head, *tail, *interval;
  int check_head, check_tail;

  M_CHECK_RANGE (mt, from, to, -1, 0);

  M17N_OBJECT_REF (prop);
  if (prop->mt)
    mtext_detach_property (prop);
  prepare_to_modify (mt, from, to, prop->key, 0);
  plist = get_plist_create (mt, prop->key, 1);
  prop->mt = mt;
  prop->start = from;
  prop->end = to;

  /* Find an interval that covers the position FROM.  */
  head = find_interval (plist, from);

  /* If the found interval starts before FROM, divide it at FROM.  */
  if (head->start < from)
    {
      divide_interval (plist, head, from);
      head = head->next;
      check_head = 0;
    }
  else
    check_head = 1;

  /* Find an interval that ends at TO.  If TO is not at the end of an
     interval, make one that ends at TO.  */
  if (head->end == to)
    {
      tail = head;
      check_tail = 1;
    }
  else if (head->end > to)
    {
      divide_interval (plist, head, to);
      tail = head;
      check_tail = 0;
    }
  else
    {
      tail = find_interval (plist, to);
      if (! tail)
	{
	  tail = plist->tail;
	  check_tail = 0;
	}
      else if (tail->start == to)
	{
	  tail = tail->prev;
	  check_tail = 1;
	}
      else
	{
	  divide_interval (plist, tail, to);
	  check_tail = 0;
	}
    }

  /* Push PROP to the current values of intervals between HEAD and TAIL
     (both inclusive).  */
  for (interval = head; ; interval = interval->next)
    {
      PUSH_PROP (interval, prop);
      if (interval == tail)
	break;
    }

  /* If there is a possibility that TAIL now has the same value as the
     next one, check it and concatenate them if necessary.  */
  if (tail->next && check_tail)
    maybe_merge_interval (plist, tail);

  /* If there is a possibility that HEAD now has the same value as the
     previous one, check it and concatenate them if necessary.  */
  if (head->prev && check_head)
    maybe_merge_interval (plist, head->prev);

  M17N_OBJECT_UNREF (prop);
  xassert (check_plist (plist, 0) == 0);
  return 0;
}

/***en
    @brief Symbol for specifying serializer functions.

    To serialize a text property, the user must supply a serializer
    function for that text property.  This is done by giving a symbol
    property whose key is #Mtext_prop_serializer and value is a
    pointer to an appropriate serializer function.

    @seealso
    mtext_serialize (), #MTextPropSerializeFunc
  */

/***ja
    @brief シリアライザ関数を指定するシンボル.

    テキストプロパティをシリアライズするためには、そのテキストプロパ
    ティ用のシリアライザ関数を与えなくてはならない。具体的には、
    #Mtext_prop_serializer をキーとし、適切なシリアライズ関数へのポイ
    ンタを値とするシンボルプロパティを指定する。

    @seealso
    mtext_serialize (), #MTextPropSerializeFunc
  */
MSymbol Mtext_prop_serializer;

/***en
    @brief Symbol for specifying deserializer functions.

    To deserialize a text property, the user must supply a deserializer
    function for that text property.  This is done by giving a symbol
    property whose key is #Mtext_prop_deserializer and value is a
    pointer to an appropriate deserializer function.

    @seealso
    mtext_deserialize (), #MTextPropSerializeFunc
  */

/***ja
    @brief デシリアライザ関数を指定するシンボル.

    テキストプロパティをデシリアライズするためには、そのテキストプロ
    パティ用のデシリアライザ関数を与えなくてはならない。具体的には、
    #Mtext_prop_deserializer をキーとし、適切なデシリアライズ関数への
    ポインタを値とするシンボルプロパティを指定する。

    @seealso
    mtext_deserialize (), #MTextPropSerializeFunc
  */
MSymbol Mtext_prop_deserializer;

/***en
    @brief Serialize text properties in an M-text.

    The mtext_serialize () function serializes the text between $FROM
    and $TO in M-text $MT.  The serialized result is an M-text in a
    form of XML.  $PROPERTY_LIST limits the text properties to be
    serialized. Only those text properties whose key 

    @li appears as the value of an element in $PROPERTY_LIST, and
    @li has the symbol property #Mtext_prop_serializer

    are serialized as a "property" element in the resulting XML
    representation.

    The DTD of the generated XML is as follows:

@verbatim
<!DOCTYPE mtext [
  <!ELEMENT mtext (property*,body+)>
  <!ELEMENT property EMPTY>
  <!ELEMENT body (#PCDATA)>
  <!ATTLIST property key CDATA #REQUIRED>
  <!ATTLIST property value CDATA #REQUIRED>
  <!ATTLIST property from CDATA #REQUIRED>
  <!ATTLIST property to CDATA #REQUIRED>
  <!ATTLIST property control CDATA #REQUIRED>
 ]>
@endverbatim

    This function depends on the libxml2 library.  If the m17n library
    is configured without libxml2, this function always fails.

    @return
    If the operation was successful, mtext_serialize () returns an
    M-text in the form of XML.  Otherwise it returns @c NULL and assigns an
    error code to the external variable #merror_code.

    @seealso
    mtext_deserialize (), #Mtext_prop_serializer  */

/***ja
    @brief M-text 中のテキストプロパティをシリアライズする.

    関数 mtext_serialize () は M-text $MT の $FROM から $TO までのテキ
    ストをシリアライズする。シリアライズした結果は XML 形式の M-text で
    ある。 $PROPERTY_LIST はシリアライズされるテキストプロパティを限定
    する。対象となるテキストプロパティは、そのキーが

    @li $PROPERTY_LIST の要素の値として現われ、かつ
    @li シンボルプロパティ #Mtext_prop_serializer を持つ
    
    もののみである。この条件を満たすテキストプロパティは、生成される 
    XML 表現中で "property" 要素にシリアライズされる。

    生成される XML の DTD は以下の通り:

@verbatim
<!DOCTYPE mtext [
  <!ELEMENT mtext (property*,body+)>
  <!ELEMENT property EMPTY>
  <!ELEMENT body (#PCDATA)>
  <!ATTLIST property key CDATA #REQUIRED>
  <!ATTLIST property value CDATA #REQUIRED>
  <!ATTLIST property from CDATA #REQUIRED>
  <!ATTLIST property to CDATA #REQUIRED>
  <!ATTLIST property control CDATA #REQUIRED>
 ]>
@endverbatim

    この関数は libxml2 ライブラリに依存する。m17n ライブラリがlibxml2 
    無しに設定されている場合、この関数は常に失敗する。

    @return 
    処理に成功すれば、mtext_serialize () は XML 形式で M-text を返す。
    そうでなければ @c NULL を返して外部変数#merror_code にエラーコード
    を設定する。

    @seealso
    mtext_deserialize (), #Mtext_prop_serializer  */

MText *
mtext_serialize (MText *mt, int from, int to, MPlist *property_list)
{
#ifdef HAVE_XML2
  MPlist *plist, *pl;
  MTextPropSerializeFunc func;
  MText *work;
  xmlDocPtr doc;
  xmlNodePtr node;
  unsigned char *ptr;
  int n;

  M_CHECK_RANGE (mt, from, to, NULL, NULL);
  if (mt->format != MTEXT_FORMAT_US_ASCII
      && mt->format != MTEXT_FORMAT_UTF_8)
    mtext__adjust_format (mt, MTEXT_FORMAT_UTF_8);
  if (MTEXT_DATA (mt)[mtext_nbytes (mt)] != 0)
    MTEXT_DATA (mt)[mtext_nbytes (mt)] = 0;
  doc = xmlParseMemory (XML_TEMPLATE, strlen (XML_TEMPLATE) + 1);
  node = xmlDocGetRootElement (doc);

  plist = mplist ();
  MPLIST_DO (pl, property_list)
    {
      MSymbol key = MPLIST_VAL (pl);

      func = ((MTextPropSerializeFunc)
	      msymbol_get_func (key, Mtext_prop_serializer));
      if (func)
	extract_text_properties (mt, from, to, key, plist);
    }

  work = mtext ();
  MPLIST_DO (pl, plist)
    {
      MTextProperty *prop = MPLIST_VAL (pl);
      char buf[256];
      MPlist *serialized_plist;
      xmlNodePtr child;

      func = ((MTextPropSerializeFunc)
	      msymbol_get_func (prop->key, Mtext_prop_serializer));
      serialized_plist = (func) (prop->val);
      if (! serialized_plist)
	continue;
      mtext_reset (work);
      mplist__serialize (work, serialized_plist, 0);
      child = xmlNewChild (node, NULL, (xmlChar *) "property", NULL);
      xmlSetProp (child, (xmlChar *) "key",
		  (xmlChar *) MSYMBOL_NAME (prop->key));
      xmlSetProp (child, (xmlChar *) "value", (xmlChar *) MTEXT_DATA (work));
      sprintf (buf, "%d", prop->start - from);
      xmlSetProp (child, (xmlChar *) "from", (xmlChar *) buf);
      sprintf (buf, "%d", prop->end - from);
      xmlSetProp (child, (xmlChar *) "to", (xmlChar *) buf);
      sprintf (buf, "%d", prop->control.flag);
      xmlSetProp (child, (xmlChar *) "control", (xmlChar *) buf);
      xmlAddChild (node, xmlNewText ((xmlChar *) "\n"));

      M17N_OBJECT_UNREF (serialized_plist);
    }
  M17N_OBJECT_UNREF (plist);

  if (from > 0 || to < mtext_nchars (mt))
    mtext_copy (work, 0, mt, from, to);
  else
    {
      M17N_OBJECT_UNREF (work);
      work = mt;
    }
  for (from = 0, to = mtext_nchars (mt); from <= to; from++)
    {
      ptr = MTEXT_DATA (mt) + POS_CHAR_TO_BYTE (mt, from);
      xmlNewTextChild (node, NULL, (xmlChar *) "body", (xmlChar *) ptr);
      from = mtext_character (mt, from, to, 0);
      if (from < 0)
	from = to;
    }

  xmlDocDumpMemoryEnc (doc, (xmlChar **) &ptr, &n, "UTF-8");
  if (work == mt)
    work = mtext ();
  mtext__cat_data (work, ptr, n, MTEXT_FORMAT_UTF_8);
  return work;
#else  /* not HAVE_XML2 */
  MERROR (MERROR_TEXTPROP, NULL);
#endif	/* not HAVE_XML2 */
}

/***en
    @brief Deserialize text properties in an M-text.

    The mtext_deserialize () function deserializes M-text $MT.  $MT
    must be an XML having the following DTD.

@verbatim
<!DOCTYPE mtext [
  <!ELEMENT mtext (property*,body+)>
  <!ELEMENT property EMPTY>
  <!ELEMENT body (#PCDATA)>
  <!ATTLIST property key CDATA #REQUIRED>
  <!ATTLIST property value CDATA #REQUIRED>
  <!ATTLIST property from CDATA #REQUIRED>
  <!ATTLIST property to CDATA #REQUIRED>
  <!ATTLIST property control CDATA #REQUIRED>
 ]>
@endverbatim

    This function depends on the libxml2 library.  If the m17n library
    is configured without libxml2, this function always fail.

    @return
    If the operation was successful, mtext_deserialize () returns the
    resulting M-text.  Otherwise it returns @c NULL and assigns an error
    code to the external variable #merror_code.

    @seealso
    mtext_serialize (), #Mtext_prop_deserializer  */

/***ja
    @brief M-text 中のテキストプロパティをデシリアライズする.

    関数 mtext_deserialize () は M-text $MT をデシリアライズする。$MT
    は次の DTD を持つ XML でなくてはならない。
 
@verbatim
<!DOCTYPE mtext [
  <!ELEMENT mtext (property*,body+)>
  <!ELEMENT property EMPTY>
  <!ELEMENT body (#PCDATA)>
  <!ATTLIST property key CDATA #REQUIRED>
  <!ATTLIST property value CDATA #REQUIRED>
  <!ATTLIST property from CDATA #REQUIRED>
  <!ATTLIST property to CDATA #REQUIRED>
  <!ATTLIST property control CDATA #REQUIRED>
 ]>
@endverbatim

    この関数は libxml2 ライブラリに依存する。m17n ライブラリがlibxml2 
    無しに設定されている場合、この関数は常に失敗する。

    @return 
    処理に成功すれば、mtext_serialize () は得られた M-text を
    返す。そうでなければ @c NULL を返して外部変数 #merror_code にエラー
    コードを設定する。

    @seealso
    mtext_serialize (), #Mtext_prop_deserializer  */

MText *
mtext_deserialize (MText *mt)
{
#ifdef HAVE_XML2
  xmlDocPtr doc;
  xmlNodePtr node;
  xmlXPathContextPtr context;
  xmlXPathObjectPtr result;
  xmlChar *body_str, *key_str, *val_str, *from_str, *to_str, *ctl_str;
  int i;

  if (mt->format > MTEXT_FORMAT_UTF_8)
    MERROR (MERROR_TEXTPROP, NULL);
  doc = xmlParseMemory ((char *) MTEXT_DATA (mt), mtext_nbytes (mt));
  if (! doc)
    MERROR (MERROR_TEXTPROP, NULL);
  node = xmlDocGetRootElement (doc);
  if (! node)
    {
      xmlFreeDoc (doc);
      MERROR (MERROR_TEXTPROP, NULL);
    }
  if (xmlStrcmp (node->name, (xmlChar *) "mtext"))
    {
      xmlFreeDoc (doc);
      MERROR (MERROR_TEXTPROP, NULL);
    }

  context = xmlXPathNewContext (doc);
  result = xmlXPathEvalExpression ((xmlChar *) "//body", context);
  if (xmlXPathNodeSetIsEmpty (result->nodesetval))
    {
      xmlFreeDoc (doc);
      MERROR (MERROR_TEXTPROP, NULL);
    }
  for (i = 0, mt = mtext (); i < result->nodesetval->nodeNr; i++)
    {
      if (i > 0)
	mtext_cat_char (mt, 0);
      node = (xmlNodePtr) result->nodesetval->nodeTab[i];
      body_str = xmlNodeListGetString (doc, node->xmlChildrenNode, 1);
      if (body_str)
	{
	  mtext__cat_data (mt, body_str, strlen ((char *) body_str),
			   MTEXT_FORMAT_UTF_8);
	  xmlFree (body_str);
	}
    }

  result = xmlXPathEvalExpression ((xmlChar *) "//property", context);
  if (! xmlXPathNodeSetIsEmpty (result->nodesetval))
    for (i = 0; i < result->nodesetval->nodeNr; i++)
      {
	MSymbol key;
	MTextPropDeserializeFunc func;
	MTextProperty *prop;
	MPlist *plist;
	int from, to, control;
	void *val;

	key_str = xmlGetProp (result->nodesetval->nodeTab[i],
			      (xmlChar *) "key");
	val_str = xmlGetProp (result->nodesetval->nodeTab[i],
			      (xmlChar *) "value");
	from_str = xmlGetProp (result->nodesetval->nodeTab[i],
			       (xmlChar *) "from");
	to_str = xmlGetProp (result->nodesetval->nodeTab[i],
			     (xmlChar *) "to");
	ctl_str = xmlGetProp (result->nodesetval->nodeTab[i],
			      (xmlChar *) "control");

	key = msymbol ((char *) key_str);
	func = ((MTextPropDeserializeFunc)
		msymbol_get_func (key, Mtext_prop_deserializer));
	if (! func)
	  continue;
	plist = mplist__from_string (val_str, strlen ((char *) val_str));
	if (! plist)
	  continue;
	if (sscanf ((char *) from_str, "%d", &from) != 1
	    || from < 0 || from >= mtext_nchars (mt))
	  continue;
	if (sscanf ((char *) to_str, "%d", &to) != 1
	    || to <= from || to > mtext_nchars (mt))
	  continue;
	if (sscanf ((char *) ctl_str, "%d", &control) != 1
	    || control < 0 || control > MTEXTPROP_CONTROL_MAX)
	  continue;
	val = (func) (plist);
	M17N_OBJECT_UNREF (plist);
	prop = mtext_property (key, val, control);
	if (key->managing_key)
	  M17N_OBJECT_UNREF (val);
	mtext_push_property (mt, from, to, prop);
	M17N_OBJECT_UNREF (prop);

	xmlFree (key_str);
	xmlFree (val_str);
	xmlFree (from_str);
	xmlFree (to_str);
	xmlFree (ctl_str);
      }
  xmlXPathFreeContext (context);
  xmlFreeDoc (doc);
  return mt;
#else  /* not HAVE_XML2 */
  MERROR (MERROR_TEXTPROP, NULL);
#endif	/* not HAVE_XML2 */
}

/*** @} */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
