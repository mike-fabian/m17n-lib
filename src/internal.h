/* internal.h -- common header file for the internal CORE and SHELL APIs.
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

#ifndef _M17N_INTERNAL_H_
#define _M17N_INTERNAL_H_

/** @file internal.h
    @brief a documentation for internal.h

    longer version of internal.h description 
*/

extern int m17n__core_initialized;
extern int m17n__shell_initialized;
extern int m17n__gui_initialized;

extern int mdebug_hook ();

/** Return with code RET while setting merror_code to ERR.  */

#define MERROR(err, ret)	\
  do {				\
    merror_code = (err);	\
    mdebug_hook ();		\
    return (ret);		\
  } while (0)


#define MERROR_GOTO(err, label)	\
  do {				\
    if ((err))			\
      merror_code = (err);	\
    mdebug_hook ();		\
    goto label;			\
  } while (0)


#define MWARNING(err)	\
  do {			\
    mdebug_hook ();	\
    goto warning;	\
  } while (0)


#define M_CHECK_CHAR(c, ret)		\
  if ((c) < 0 || (c) > MCHAR_MAX)	\
    MERROR (MERROR_CHAR, (ret));	\
  else


/** Memory allocation stuffs.  */

/* Call a handler function for memory full situation with argument
   ERR.  ERR must be one of enum MErrorCode.  By default, the
   handler function just calls exit () with argument ERR.  */

#define MEMORY_FULL(err)		\
  do {					\
    (*m17n_memory_full_handler) (err);	\
    exit (err);				\
  } while (0)


/** The macro MTABLE_MALLOC () allocates memory (by malloc) for an
    array of SIZE objects.  The size of each object is determined by
    the type of P.  Then, it sets P to the allocated memory.  ERR must
    be one of enum MErrorCode.  If the allocation fails, the macro
    MEMORY_FULL () is called with argument ERR.  */

#define MTABLE_MALLOC(p, size, err)		\
  do {						\
    int bytes = sizeof (*(p)) * (size);		\
    if (! ((p) = (void *) malloc (bytes)))	\
      MEMORY_FULL (err);			\
  } while (0)


/** The macro MTABLE_CALLOC() is like the macro MTABLE_MALLOC but use
    calloc instead of malloc, thus the allocated memory are zero
    cleared.  */

#define MTABLE_CALLOC(p, size, err)				\
  do {								\
    if (! ((p) = (void *) calloc (sizeof (*(p)), size)))	\
      MEMORY_FULL (err);					\
  } while (0)


/** The macro MTABLE_REALLOC () changes the size of memory block
    pointed to by P to a size suitable for an array of SIZE objects.
    The size of each object is determined by the type of P.  ERR must
    be one of enum MErrorCode.  If the allocation fails, the macro
    MEMORY_FULL () is called with argument ERR.  */

#define MTABLE_REALLOC(p, size, err)					\
  do {									\
    if (! ((p) = (void *) realloc ((p), sizeof (*(p)) * (size))))	\
      MEMORY_FULL (err);						\
  } while (0)


/** The macro MTABLE_ALLOCA () allocates memory (by alloca) for an
    array of SIZE objects.  The size of each object is determined by
    the type of P.  Then, it sets P to the allocated memory.  ERR must
    be one of enum MErrorCode.  If the allocation fails, the macro
    MEMORY_FULL () is called with argument ERR.  */

#define MTABLE_ALLOCA(p, size, err)		\
  do {						\
    int bytes = sizeof (*(p)) * (size);		\
    if (! ((p) = (void *) alloca (bytes)))	\
      MEMORY_FULL (err);			\
    memset ((p), 0, bytes);			\
  } while (0)


/** short description of MSTRUCT_MALLOC */
/** The macro MSTRUCT_MALLOC () allocates memory (by malloc) for an
    object whose size is determined by the type of P, and sets P to
    the allocated memory.  ERR must be one of enum MErrorCode.  If
    the allocation fails, the macro MEMORY_FULL () is called with
    argument ERR.  */

#define MSTRUCT_MALLOC(p, err)				\
  do {							\
    if (! ((p) = (void *) malloc (sizeof (*(p)))))	\
      MEMORY_FULL (err);				\
  } while (0)


#define MSTRUCT_CALLOC(p, err) MTABLE_CALLOC ((p), 1, (err))


/** Extendable array.  */

#define MLIST_RESET(list)	\
  ((list)->used = 0)


#define MLIST_INIT1(list, mem, increment)	\
  do {						\
    (list)->size = (list)->used = 0;		\
    (list)->inc = (increment);			\
    (list)->mem = NULL;				\
  } while (0)


#define MLIST_APPEND1(list, mem, elt, err)			\
  do {								\
    if ((list)->inc <= 0)					\
      mdebug_hook ();						\
    if ((list)->size == (list)->used)				\
      {								\
	(list)->size += (list)->inc;				\
	MTABLE_REALLOC ((list)->mem, (list)->size, (err));	\
      }								\
    (list)->mem[(list)->used++] = (elt);			\
  } while (0)


#define MLIST_PREPEND1(list, mem, elt, err)			\
  do {								\
    if ((list)->inc <= 0)					\
      mdebug_hook ();						\
    if ((list)->size == (list)->used)				\
      {								\
	(list)->size += (list)->inc;				\
	MTABLE_REALLOC ((list)->mem, (list)->size, (err));	\
      }								\
    memmove ((list)->mem + 1, (list)->mem,			\
	     sizeof *((list)->mem) * ((list)->used));		\
    (list)->mem[0] = (elt);					\
    (list)->used++;						\
  } while (0)


#define MLIST_INSERT1(list, mem, idx, len, err)				\
  do {									\
    while ((list)->used + (len) > (list)->size)				\
      {									\
	(list)->size += (list)->inc;					\
	MTABLE_REALLOC ((list)->mem, (list)->size, (err));		\
      }									\
    memmove ((list)->mem + ((idx) + (len)), (list)->mem + (idx),	\
	     (sizeof *((list)->mem)) * ((list)->used - (idx)));		\
    (list)->used += (len);						\
  } while (0)


#define MLIST_DELETE1(list, mem, idx, len)				\
  do {									\
    memmove ((list)->mem + (idx), (list)->mem + (idx) + (len),		\
	     (sizeof *((list)->mem)) * ((list)->used - (idx) - (len)));	\
    (list)->used -= (len);						\
  } while (0)


#define MLIST_COPY1(list0, list1, mem, err)			\
  do {								\
    (list0)->size = (list0)->used = (list1)->used;		\
    (list0)->inc = 1;						\
    MTABLE_MALLOC ((list0)->mem, (list0)->used, (err));		\
    memcpy ((list0)->mem, (list1)->mem,				\
	    (sizeof (list0)->mem) * (list0)->used);		\
  } while (0)


#define MLIST_FREE1(list, mem)		\
  if ((list)->size)			\
    {					\
      free ((list)->mem);		\
      (list)->mem = NULL;		\
      (list)->size = (list)->used = 0;	\
    }					\
  else



typedef struct
{
  void (*freer) (void *);
  int size, inc, used;
  unsigned *counts;
} M17NObjectRecord;

typedef struct
{
  /**en Reference count of the object.  */
  unsigned ref_count : 16;

  unsigned ref_count_extended : 1;

  /**en A flag bit used for various perpose.  */
  unsigned flag : 15;

  union {
    /**en If <ref_count_extended> is zero, a function to free the
       object.  */
    void (*freer) (void *);
    /**en If <ref_count_extended> is nonzero, a pointer to the
       struct M17NObjectRecord.  */
    M17NObjectRecord *record;
  } u;
} M17NObject;


/** Allocate a managed object OBJECT which has freer FREE_FUNC.  */

#define M17N_OBJECT(object, free_func, err)		\
  do {							\
    MSTRUCT_CALLOC ((object), (err));			\
    ((M17NObject *) (object))->ref_count = 1;		\
    ((M17NObject *) (object))->u.freer = free_func;	\
  } while (0)


/**en Increment the reference count of OBJECT if the count is not
   0.  */

#define M17N_OBJECT_REF(object)				\
  do {							\
    if (((M17NObject *) (object))->ref_count_extended)	\
      m17n_object_ref (object);				\
    else if (((M17NObject *) (object))->ref_count > 0)	\
      {							\
	((M17NObject *) (object))->ref_count++;		\
	if (! ((M17NObject *) (object))->ref_count)	\
	  {						\
	    ((M17NObject *) (object))->ref_count--;	\
	    m17n_object_ref (object);			\
	  }						\
      }							\
  } while (0)


#define M17N_OBJECT_REF_NTIMES(object, n)				\
  do {									\
    int i;								\
									\
    if (((M17NObject *) (object))->ref_count_extended)			\
      for (i = 0; i < n; i++)						\
	m17n_object_ref (object);					\
    else if (((M17NObject *) (object))->ref_count > 0)			\
      {									\
	int orig_ref_count = ((M17NObject *) (object))->ref_count;	\
									\
	for (i = 0; i < n; i++)						\
	  if (! ++((M17NObject *) (object))->ref_count)			\
	    {								\
	      ((M17NObject *) (object))->ref_count = orig_ref_count;	\
	      for (i = 0; i < n; i++)					\
		m17n_object_ref (object);				\
	    }								\
      }									\
  } while (0)


/**en Decrement the reference count of OBJECT if the count is greater
    than 0.  In that case, if the count becomes 0, free OBJECT.  */

#define M17N_OBJECT_UNREF(object)				\
  do {								\
    if (object)							\
      {								\
	if (((M17NObject *) (object))->ref_count_extended)	\
	  m17n_object_unref (object);				\
	else if (((M17NObject *) (object))->ref_count == 0)	\
	  break;						\
	else if (((M17NObject *) (object))->ref_count > 1)	\
	  ((M17NObject *) (object))->ref_count--;		\
	else							\
	  {							\
	    if (((M17NObject *) (object))->u.freer)		\
	      (((M17NObject *) (object))->u.freer) (object);	\
	    else						\
	      free (object);					\
	  }							\
      }								\
  } while (0)


typedef struct
{
  int count;
  int size, inc, used;
  void **objects;
} M17NObjectArray;

extern void mdebug__register_object (M17NObjectArray *array, void *object);

#define M17N_OBJECT_REGISTER(array, object)	\
  if (mdebug__flag & MDEBUG_FINI)		\
    mdebug__register_object (&array, object);	\
  else

extern void mdebug__unregister_object (M17NObjectArray *array, void *object);

#define M17N_OBJECT_UNREGISTER(array, object)	\
  if (mdebug__flag & MDEBUG_FINI)		\
    mdebug__unregister_object (&array, object);	\
  else

extern void mdebug__report_object (char *name, M17NObjectArray *array);



struct MTextPlist;

struct MText
{
  M17NObject control;

  enum MTextFormat format;

  /**en Number of characters in the M-text */
  /**ja M-text 中の文字数 */
  int nchars;

  /**en Number of bytes used to represent the characters in the M-text. */
  /**ja M-text 中の文字を表わすために用いられるバイト数 */
  int nbytes;

  /**en Character sequence of the M-text. */
  /**ja M-text 中の文字列 */
  unsigned char *data;

  /**en Number of bytes allocated for the @c data member. */
  /**ja メンバ @c data に割り当てられたバイト数 */
  int allocated;

  /**en Pointer to the property list of the M-text. */
  /**ja M-text のプロパティリストへのポインタ */
  struct MTextPlist *plist;

  /**en Caches of the character position and the corresponding byte position. */
  /**ja 文字位置および対応するバイト位置のキャッシュ */
  int cache_char_pos, cache_byte_pos;
};

/** short description of M_CHECK_POS */
/** longer description of M_CHECK_POS */

#define M_CHECK_POS(mt, pos, ret)		\
  do {						\
    if ((pos) < 0 || (pos) >= (mt)->nchars)	\
      MERROR (MERROR_RANGE, (ret));		\
  } while (0)


/** short description of M_CHECK_POS_X */
/** longer description of M_CHECK_POS_X */

#define M_CHECK_POS_X(mt, pos, ret)		\
  do {						\
    if ((pos) < 0 || (pos) > (mt)->nchars)	\
      MERROR (MERROR_RANGE, (ret));		\
  } while (0)


/** short description of M_CHECK_RANGE */
/** longer description of M_CHECK_RANGE */

#define M_CHECK_RANGE(mt, from, to, ret, ret2)			\
  do {								\
    if ((from) < 0 || (to) < (from) || (to) > (mt)->nchars)	\
      MERROR (MERROR_RANGE, (ret));				\
    if ((from) == (to))						\
      return (ret2);						\
  } while (0)

#define M_CHECK_RANGE_X(mt, from, to, ret)			\
  do {								\
    if ((from) < 0 || (to) < (from) || (to) > (mt)->nchars)	\
      MERROR (MERROR_RANGE, (ret));				\
  } while (0)


#define M_CHECK_POS_NCHARS(mt, pos, nchars, ret, ret2)	\
  do {							\
    int to = (pos) + (nchars);			\
							\
    M_CHECK_RANGE ((mt), (pos), (to), (ret), (ret2));	\
  } while (0)


#define MTEXT_READ_ONLY_P(mt) ((mt)->allocated < 0)

#define M_CHECK_READONLY(mt, ret)	\
  do {					\
    if ((mt)->allocated < 0)		\
      MERROR (MERROR_MTEXT, (ret));	\
  } while (0)

#define mtext_nchars(mt) ((mt)->nchars)

#define mtext_nbytes(mt) ((mt)->nbytes)

#define mtext_allocated(mt) ((mt)->allocated)

#define mtext_reset(mt) (mtext_del ((mt), 0, (mt)->nchars))



enum MDebugMaskBit
  {
    MDEBUG_INIT =	0x01,
    MDEBUG_FINI =	0x02,
    MDEBUG_CHARSET =	0x04,
    MDEBUG_CODING =	0x08,
    MDEBUG_DATABASE =	0x10,
    MDEBUG_FONT =	0x0100,
    MDEBUG_FONT_FLT =	0x0200,
    MDEBUG_FONT_OTF =	0x0400,
    MDEBUG_INPUT = 	0x0800,
    MDEBUG_MAX
  };

extern int mdebug__flag;
extern void mdebug__push_time ();
extern void mdebug__pop_time ();
extern void mdebug__print_time ();

#define MDEBUG_PRINT(msg)		\
  do {					\
    if (mdebug__flag & mdebug_mask)	\
      fprintf (stderr, (msg));		\
  } while (0)

#define MDEBUG_PRINT1(fmt, arg)		\
  do {					\
    if (mdebug__flag & mdebug_mask)	\
      fprintf (stderr, (fmt), (arg));	\
  } while (0)

#define MDEBUG_PRINT2(fmt, arg1, arg2)		\
  do {						\
    if (mdebug__flag & mdebug_mask)		\
      fprintf (stderr, (fmt), (arg1), (arg2));	\
  } while (0)

#define MDEBUG_PRINT3(fmt, arg1, arg2, arg3)		\
  do {							\
    if (mdebug__flag & mdebug_mask)			\
      fprintf (stderr, (fmt), (arg1), (arg2), (arg3));	\
  } while (0)

#define MDEBUG_PRINT4(fmt, arg1, arg2, arg3, arg4)		\
  do {								\
    if (mdebug__flag & mdebug_mask)				\
      fprintf (stderr, (fmt), (arg1), (arg2), (arg3), (arg4));	\
  } while (0)

#define MDEBUG_PRINT5(fmt, arg1, arg2, arg3, arg4, arg5)		\
  do {									\
    if (mdebug__flag & mdebug_mask)					\
      fprintf (stderr, (fmt), (arg1), (arg2), (arg3), (arg4), (arg5));	\
  } while (0)


#define MDEBUG_PUSH_TIME()		\
  do {					\
    if (mdebug__flag & mdebug_mask)	\
      mdebug__push_time ();		\
  } while (0)


#define MDEBUG_POP_TIME()		\
  do {					\
    if (mdebug__flag & mdebug_mask)	\
      mdebug__pop_time ();		\
  } while (0)


#define MDEBUG_PRINT_TIME(tag, ARG_LIST)	\
  do {						\
    if (mdebug__flag & mdebug_mask)		\
      {						\
	fprintf (stderr, " [%s] ", tag);	\
	mdebug__print_time ();			\
	fprintf ARG_LIST;			\
	fprintf (stderr, "\n");			\
      }						\
  } while (0)


#define SWAP_16(c) (((c) >> 8) | (((c) & 0xFF) << 8))

#define SWAP_32(c)			\
  (((c) >> 24) | (((c) >> 8) & 0xFF00)	\
   | (((c) & 0xFF00) << 8) | (((c) & 0xFF) << 24))


extern void *(*mdatabase__finder) (MSymbol tag1, MSymbol tag2,
				   MSymbol tag3, MSymbol tag4);
extern void *(*mdatabase__loader) (void *);

/* Initialize/finalize function.  */

extern int msymbol__init ();
extern void msymbol__fini ();

extern int mplist__init ();
extern void mplist__fini ();

extern int mtext__init ();
extern void mtext__fini ();

extern int mtext__prop_init ();
extern void mtext__prop_fini ();

extern int mchartable__init ();
extern void mchartable__fini ();

extern int mcharset__init ();
extern void mcharset__fini ();

extern int mcoding__init ();
extern void mcoding__fini ();

extern int mdatabase__init (void);
extern void mdatabase__fini (void);

extern int mchar__init ();
extern void mchar__fini ();

extern int mlang__init ();
extern void mlang__fini ();

extern int mlocale__init ();
extern void mlocale__fini ();

extern int minput__init ();
extern void minput__fini ();

#endif /* _M17N_INTERNAL_H_ */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
