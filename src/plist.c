/* plist.c -- plist module.
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
   Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   02111-1307, USA.  */

/***en
    @addtogroup m17nPlist

    @brief Property List objects and API for them.

    A @e property @e list (or @e plist for short) is a list of zero or
    more properties.  A property consists of a @e key and a @e value,
    where key is a symbol and value is anything that can be cast to
    <tt>(void *)</tt>.

    If the key of a property is a @e managing @e key, its @e value is
    a @e managed @e object.  A property list itself is a managed
    objects.

    If each key of a plist is one of #Msymbol, #Mtext, #Minteger, and
    #Mplist, the plist is called as @e well-formed and represented by
    the following notation in the documentation.

@verbatim
      PLIST ::= '(' ELEMENT * ')'

      ELEMENT ::= INTEGER | SYMBOL | M-TEXT | PLIST

      M-TEXT ::= '"' text data ... '"'
@endverbatim

    For instance, if a plist has four elements; integer -20, symbol of
    name "sym", M-text of contents "abc", and plist of integer 10 and
    symbol of name "another-symbol", it is represented as this:

      (-20 sym "abc" (10 another-symbol))

  */
/***ja
    @addtogroup m17nPlist

    @brief プロパティリストオブジェクトとそれに関する API.

    @e プロパティリスト (または @e plist) は 0 
    個以上のプロパティのリストである。プロパティは @e キー と @e 値 
    からなる。キーはシンボルであり、値は <tt>(void *)</tt> 
    にキャストできるものならば何でも良い。

    あるプロパティのキーが @e 管理キー ならば、その @e 値 は @e 管理下
    @e オブジェクト 
    である。プロパティリスト自体も管理下オブジェクトである。  */

/*=*/

#if !defined (FOR_DOXYGEN) || defined (DOXYGEN_INTERNAL_MODULE)
/*** @addtogroup m17nInternal
     @{ */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "m17n.h"
#include "m17n-misc.h"
#include "internal.h"
#include "character.h"
#include "mtext.h"
#include "symbol.h"
#include "plist.h"

static M17NObjectArray plist_table;

/** Set PLIST to a newly allocated plist object.  */

#define MPLIST_NEW(plist)				\
  do {							\
    M17N_OBJECT (plist, free_plist, MERROR_PLIST);	\
    M17N_OBJECT_REGISTER (plist_table, plist);		\
  } while (0)


/** Set the element of PLIST to KEY and VAL.  If PLIST is an anchor,
    append a new anchor.  */

#define MPLIST_SET(plist, key, val)	\
  do {					\
    MPLIST_KEY (plist) = (key);		\
    MPLIST_VAL (plist) = (val);		\
    if (! (plist)->next)		\
      MPLIST_NEW ((plist)->next);	\
  } while (0)


/** Set the element of PLIST to KEY and VAL.  PLIST must be an anchor.
    Append a new anchor and set PLIST to that anchor.  */

#define MPLIST_SET_ADVANCE(plist, key, val)	\
  do {						\
    MPLIST_KEY (plist) = (key);			\
    MPLIST_VAL (plist) = (val);			\
    MPLIST_NEW ((plist)->next);			\
    plist = (plist)->next;			\
  } while (0)


static void
free_plist (void *object)
{
  MPlist *plist = (MPlist *) object;

  do {
    MPlist *next = plist->next;

    if (MPLIST_KEY (plist) != Mnil && MPLIST_KEY (plist)->managing_key)
      M17N_OBJECT_UNREF (MPLIST_VAL (plist));
    M17N_OBJECT_UNREGISTER (plist_table, plist);
    free (plist);
    plist = next;
  } while (plist && plist->control.ref_count == 1);
  M17N_OBJECT_UNREF (plist);
}



/* Load a plist from a string.  */

#define READ_CHUNK 0x10000

typedef struct
{
  /* File pointer if the stream is associated with a file.  Otherwise
     NULL.  */
  FILE *fp;
  int eof;
  unsigned char buffer[READ_CHUNK];
  unsigned char *p, *pend;
} MStream;

static int
get_byte (MStream *st)
{
  int n;

  if (! st->fp || st->eof)
    return EOF;
  n = fread (st->buffer, 1, READ_CHUNK, st->fp);
  if (n <= 0)
    {
      st->eof = 1;
      return EOF;
    }
  st->p = st->buffer + 1;
  st->pend = st->buffer + n;
  return st->buffer[0];
}

#define GETC(st) ((st)->p < (st)->pend ? *(st)->p++ : get_byte (st))

#define UNGETC(c, st) (--((st)->p))

/** Mapping table for reading a number.  Hexadecimal chars
    (0..9,A..F,a..F) are mapped to the corresponding numbers.
    Apostrophe (code 39) is mapped to 254.  All the other bytes are
    mapped to 255.  */
unsigned char hex_mnemonic[256];

/** Mapping table for escaped characters.  Mnemonic characters (e, b,
    f, n, r, or t) that follows '\' are mapped to the corresponding
    character code.  All the other bytes are mapped to themselves.  */
unsigned char escape_mnemonic[256];


/** Read an integer from the stream ST.  It is assumed that we have
    already read one character C.  */

static int
read_decimal (MStream *st, int c)
{
  int num = 0;

  while (c >= '0' && c <= '9')
    {
      num = (num * 10) + (c - '0');
      c = GETC (st);
    }

  if (c != EOF)
    UNGETC (c, st);
  return num;
}

/** Read an unsigned from the stream ST.  */

static unsigned
read_hexadesimal (MStream *st)
{
  int c;
  unsigned num = 0, n;

  while ((c = GETC (st)) != EOF
	 && (n = hex_mnemonic[c]) < 16)
    num = (num << 4) | n;
  if (c != EOF)
    UNGETC (c, st);
  return num;
}


/** Read an M-text element from ST, and add it to LIST.  Return a list
    for the next element.  */

#define READ_MTEXT_BUF_SIZE 256

static MPlist *
read_mtext_element (MPlist *plist, MStream *st, int skip)
{
  union {
    int chars[READ_MTEXT_BUF_SIZE];
    unsigned char bytes[sizeof (int) * READ_MTEXT_BUF_SIZE];
  } buffer;
  unsigned char *bytes = buffer.bytes;
  int nbytes = sizeof (int) * READ_MTEXT_BUF_SIZE;
  int *chars = NULL;
  int nchars = 0;
  int c, i, j;

  i = 0;
  while ((c = GETC (st)) != EOF && c != '"')
    {
      int is_char = 0;

      if (c == '\\')
	{
	  c = GETC (st);
	  if (c == EOF)
	    break;
	  if (c == '\n')
	    continue;
	  if (c == 'x' || c == 'u')
	    {
	      int next_c;

	      c = read_hexadesimal (st);
	      next_c = GETC (st);
	      if (next_c != ' ')
		UNGETC (next_c, st);
	      if (c >= 0x80)
		is_char = 1;
	    }
	  else
	    c = escape_mnemonic[c];
	}

      if (! skip)
	{
	  if (is_char && ! chars)
	    {
	      chars = buffer.chars;
	      for (j = i - 1; j >= 0; j--)
		chars[j] = bytes[j];
	      nchars = READ_MTEXT_BUF_SIZE;
	      if (bytes != buffer.bytes)
		free (bytes);
	    }

	  if (chars)
	    {
	      if (i + 1 >= nchars)
		{
		  nchars *= 2;
		  if (chars == buffer.chars)
		    {
		      MTABLE_MALLOC (chars, nchars, MERROR_PLIST);
		      memcpy (chars, buffer.chars, sizeof (int) * i);
		    }
		  else
		    MTABLE_REALLOC (chars, nchars, MERROR_PLIST);
		}
	      chars[i++] = c;
	    }
	  else
	    {
	      if (i + MAX_UTF8_CHAR_BYTES >= nbytes)
		{
		  nbytes *= 2;
		  if (bytes == buffer.bytes)
		    {
		      MTABLE_MALLOC (bytes, nbytes, MERROR_PLIST);
		      memcpy (bytes, buffer.bytes, i);
		    }
		  else
		    MTABLE_REALLOC (bytes, nbytes, MERROR_PLIST);
		}
	      bytes[i++] = c;
	    }
	}
    }

  if (! skip)
    {
      MText *mt;

      if (chars)
	{
	  mt = mtext__from_data (chars, i, MTEXT_FORMAT_UTF_32, 1);
	  if (chars != buffer.chars)
	    free (chars);
	}	  
      else
	{
	  mt = mtext__from_data (bytes, i, MTEXT_FORMAT_UTF_8, 1);
	  if (bytes != buffer.bytes)
	    free (bytes);
	}
      MPLIST_SET_ADVANCE (plist, Mtext, mt);
    }
  return plist;
}

static int
read_character (MStream *st, int c)
{
  unsigned char buf[MAX_UTF8_CHAR_BYTES + 1];
  int len = CHAR_BYTES_BY_HEAD (c);
  int i;

  buf[0] = c;
  for (i = 1; i < len; i++)
    {
      c = GETC (st);
      if (c == EOF
	  || (c & 0xC0) != 0x80)
	break;
      buf[i] = c;
    }
  if (i == len)
    c = STRING_CHAR_UTF8 (buf);
  else
    c = buf[0];
  return c;
}


/** Read a symbol element from ST, and add it to LIST.  Return a list
    for the next element.  */

static MPlist *
read_symbol_element (MPlist *plist, MStream *st, int c, int skip)
{
  unsigned char buffer[1024];
  int bufsize = 1024;
  unsigned char *buf = buffer;
  int i;

  i = 0;
  while (c != EOF
	 && c > ' '
	 && c != ')' && c != '(' && c != '"')
    {
      if (i >= bufsize)
	{
	  bufsize *= 2;
	  if (buf == buffer)
	    {
	      MTABLE_MALLOC (buf, bufsize, MERROR_PLIST);
	      memcpy (buf, buffer, i);
	    }
	  else
	    MTABLE_REALLOC (buf, bufsize, MERROR_PLIST);
	}
      if (c == '\\')
	{
	  c = GETC (st);
	  if (c == EOF)
	    break;
	  c = escape_mnemonic[c];
	}
      if (! skip)
	buf[i++] = c;
      c = GETC (st);
    }

  if (c > ' ')
    UNGETC (c, st);
  if (! skip)
    {
      buf[i] = 0;
      MPLIST_SET_ADVANCE (plist, Msymbol, msymbol ((char *) buf));
      if (buf != buffer)
	free (buf);
    }
  return plist;
}

/** Read an integer element from ST, and add it to LIST.  Return a
    list for the next element.  It is assumed that we have already
    read the character C. */

static MPlist *
read_integer_element (MPlist *plist, MStream *st, int c, int skip)
{
  int num;

  if (c == '#')
    {
      c = GETC (st);
      if (c != 'x')
	{
	  UNGETC (c, st);
	  return read_symbol_element (plist, st, '#', skip);
	}
      num = read_hexadesimal (st);
    }
  else if (c == '0')
    {
      c = GETC (st);
      num = (c == 'x' ? read_hexadesimal (st) : read_decimal (st, c));
    }
  else if (c == '?')
    {
      c = GETC (st);
      if (c == EOF)
	num = 0;
      else if (c != '\\')
	{
	  if (c < 128 || ! CHAR_UNITS_BY_HEAD_UTF8 (c))
	    num = c;
	  else
	    num = read_character (st, c);
	}
      else
	{
	  c = GETC (st);
	  if (c == EOF)
	    num = '\\';
	  else if (c < 128 || ! CHAR_UNITS_BY_HEAD_UTF8 (c))
	    num = escape_mnemonic[c];
	  else
	    num = read_character (st, c);
	}
    }
  else if (c == '-')
    {
      c = GETC (st);
      if (c < '0' || c > '9')
	{
	  UNGETC (c, st);
	  return read_symbol_element (plist, st, '-', skip);
	}
      num = - read_decimal (st, c);
    }
  else
    num = read_decimal (st, c);

  if (! skip)
    MPLIST_SET_ADVANCE (plist, Minteger, (void *) num);
  return plist;
}

/* Read an element of various type from stream ST, and add it to LIST.
   Return a list for the next element.  The element type is decided by
   the first token character found as below:
	'(': plist
	'"': mtext
	'0'..'9', '-': integer
	'?': integer representing character code
	the other ASCII letters: symbol

   If KEYS is not NULL, it is a plist contains target keys and stop
   keys.  In this caes, read only a plist whose key has value 1 in
   KEYS, and return NULL when we encounter a plist whose key has value
   0 in KEYS while skipping any other elements.  */

static MPlist *
read_element (MPlist *plist, MStream *st, MPlist *keys)
{
  int c;

  /* Skip separators and comments.  */
  while (1)
    {
      while ((c = GETC (st)) != EOF && c <= ' ');
      if (c != ';')
	break;
      while ((c = GETC (st)) != EOF && c != '\n');
      if (c == EOF)
	break;
    }

  if (c == '(')
    {
      MPlist *pl, *p;

      MPLIST_NEW (pl);
      p = pl;
      p = read_element (p, st, NULL);
      if (keys && p && MPLIST_SYMBOL_P (pl))
	{
	  if (MPLIST_TAIL_P (keys))
	    {
	      while ((p = read_element (p, st, NULL)));
	      MPLIST_SET_ADVANCE (plist, Mplist, pl);
	      return NULL;
	    }
	  else
	    {
	      MPlist *p0 = keys;

	      MPLIST_FIND (p0, MPLIST_SYMBOL (pl));
	      if (! MPLIST_TAIL_P (p0) && ! MPLIST_VAL (p0))
		{
		  M17N_OBJECT_UNREF (pl);
		  return NULL;
		}
	      while ((p = read_element (p, st, NULL)));
	      if (! MPLIST_TAIL_P (p0))
		{
		  MPLIST_SET_ADVANCE (plist, Mplist, pl);
		  return NULL;
		}
	      else
		M17N_OBJECT_UNREF (pl);
	    }
	}
      else
	{
	  if (p)
	    while ((p = read_element (p, st, NULL)));
	  MPLIST_SET_ADVANCE (plist, Mplist, pl);
	}
      return plist;
    }
  if (c == '"')
    return (read_mtext_element (plist, st, keys ? 1 : 0));
  if ((c >= '0' && c <= '9') || c == '-' || c == '?' || c == '#')
    return (read_integer_element (plist, st, c, keys ? 1 : 0));
  if (c == EOF || c == ')')
    return NULL;
  return (read_symbol_element (plist, st, c, keys ? 1 : 0));
}

#define PUTC(MT, C)			\
  do {					\
    if (MT)				\
      mtext_cat_char ((MT), (C));	\
    else				\
      putc ((C), stderr);		\
  } while (0);

#define PUTS(MT, STR)			\
  do {					\
    if (MT)				\
      MTEXT_CAT_ASCII ((MT), (STR));	\
    else				\
      fputs ((STR), stderr);		\
  } while (0)


static void 
write_symbol (MText *mt, MSymbol sym)
{
  if (sym == Mnil)
    {
      PUTS (mt, "nil");
    }
  else
    {
      char *name = MSYMBOL_NAME (sym);

      if (isdigit (*name))
	PUTC (mt, '\\');
      while (*name)
	{
	  if (*name <= ' ' || *name == '\\' || *name == '"'
	      || *name == '(' || *name == ')')
	    PUTC (mt, '\\');
	  PUTC (mt, *name); 
	  name++;
	}
    }
}

static void
write_element (MText *mt, MPlist *plist, int indent)
{
  if (MPLIST_SYMBOL_P (plist))
    {
      write_symbol (mt, MPLIST_SYMBOL (plist));
    }
  else if (MPLIST_INTEGER_P (plist))
    {
      int num = MPLIST_INTEGER (plist);
      char buf[128];

      sprintf (buf, "%d", num);
      PUTS (mt, buf);
    }
  else if (MPLIST_PLIST_P (plist))
    {
      MPlist *pl;
      int newline = 0;

      plist = MPLIST_PLIST (plist);
      PUTC (mt, '(');
      if (indent >= 0)
	indent++;
      MPLIST_DO (pl, plist)
	{
	  if (pl != plist)
	    {
	      if (indent > 0 && (MPLIST_PLIST_P (pl) || MPLIST_MTEXT_P (pl)))
		newline = 1;
	      if (newline)
		{
		  int i;

		  PUTC (mt, '\n');
		  for (i = 1; i < indent; i++)
		    PUTC (mt, ' ');
		}
	      PUTC (mt, ' ');
	    }
	  write_element (mt, pl, indent);
	  if (indent >= 0)
	    newline = (MPLIST_PLIST_P (pl) || MPLIST_MTEXT_P (pl));
	}
      PUTC (mt, ')');
    }
  else if (MPLIST_MTEXT_P (plist))
    {
      MText *this_mt = MPLIST_MTEXT (plist);
      int from = 0, to = mtext_nchars (this_mt);
      int stop1 = 0, stop2 = 0;

      if (! mt && this_mt->format > MTEXT_FORMAT_UTF_8)
	{
	  this_mt = mtext_dup (this_mt);
	  mtext__adjust_format (this_mt, MTEXT_FORMAT_UTF_8);
	}

      PUTC (mt, '"');
      while (1)
	{
	  int stop, escaped;

	  if (from == stop1)
	    {
	      if ((stop1 = mtext_character (this_mt, from, to, '"')) < 0)
		stop1 = to;
	    }
	  if (from == stop2)
	    {
	      if ((stop2 = mtext_character (this_mt, from, to, '\\')) < 0)
		stop2 = to;
	    }
	  if (stop1 < stop2)
	    stop = stop1++, escaped = '"';
	  else
	    stop = stop2++, escaped = '\\';
	  if (mt)
	    mtext_copy (mt, mtext_nchars (mt), this_mt, from, stop);
	  else
	    {
	      unsigned char *data = MTEXT_DATA (this_mt);
	      unsigned char *beg = data + mtext__char_to_byte (this_mt, from);
	      unsigned char *end = data + mtext__char_to_byte (this_mt, stop);

	      while (beg < end)
		putc (*beg, stderr), beg++;
	    }
	  if (stop == to)
	    break;
	  PUTC (mt, '\\');
	  PUTC (mt, escaped);
	  from = stop + 1;
	}
      PUTC (mt, '"');
      if (this_mt != MPLIST_MTEXT (plist))
	M17N_OBJECT_UNREF (this_mt);
    }
  else if (MPLIST_STRING_P (plist))
    {
      char *str = MPLIST_STRING (plist);

      if (mt)
	{
	  MText *this_mt = mtext__from_data (str, strlen (str),
					     MTEXT_FORMAT_UTF_8, 0);

	  mtext_copy (mt, mtext_nchars (mt),
		      this_mt, 0, mtext_nchars (this_mt));
	  M17N_OBJECT_UNREF (this_mt);
	}
      else
	fprintf (stderr, "%s", str);
    }
  else 
    {
      write_symbol (mt, MPLIST_KEY (plist));
      PUTC (mt, ':');
      if (MPLIST_NESTED_P (plist))
	write_element (mt, plist, indent + 1);
      else
	{
	  char buf[128];

	  sprintf (buf, "%04X", (unsigned) MPLIST_VAL (plist));
	  PUTS (mt, buf);
	}
    }
}


/* Internal API */
int
mplist__init ()
{
  int i;

  M17N_OBJECT_ADD_ARRAY (plist_table, "Plist");

  Minteger = msymbol ("integer");
  Mplist = msymbol_as_managing_key ("plist");
  Mtext = msymbol_as_managing_key ("mtext");

  for (i = 0; i < 256; i++)
    hex_mnemonic[i] = 255;
  for (i = '0'; i <= '9'; i++)
    hex_mnemonic[i] = i - '0';
  for (i = 'A'; i <= 'F'; i++)
    hex_mnemonic[i] = i - 'A' + 10;
  for (i = 'a'; i <= 'f'; i++)
    hex_mnemonic[i] = i - 'a' + 10;
  for (i = 0; i < 256; i++)
    escape_mnemonic[i] = i;
  escape_mnemonic['e'] = 27;
  escape_mnemonic['b'] = '\b';
  escape_mnemonic['f'] = '\f';
  escape_mnemonic['n'] = '\n';
  escape_mnemonic['r'] = '\r';
  escape_mnemonic['t'] = '\t';
  escape_mnemonic['\\'] = '\\';

  return 0;
}

void
mplist__fini (void)
{
}


/* Parse this form of PLIST:
      (symbol:KEY1 TYPE1:VAL1 symbol:KEY2 TYPE2:VAL2 ...)
   and return a newly created plist of this form:
      (KEY1:VAL1 KEY2:VAL2 ...)  */

MPlist *
mplist__from_plist (MPlist *plist)
{
  MPlist *pl, *p;

  MPLIST_NEW (pl);
  p = pl;
  while (! MPLIST_TAIL_P (plist))
    {
      MSymbol key, type;

      if (! MPLIST_SYMBOL_P (plist))
	MERROR (MERROR_PLIST, NULL);
      key = MPLIST_SYMBOL (plist);
      plist = MPLIST_NEXT (plist);
      type = MPLIST_KEY (plist);
      if (type->managing_key && MPLIST_VAL (plist))
	M17N_OBJECT_REF (MPLIST_VAL (plist));
      MPLIST_SET_ADVANCE (p, key, MPLIST_VAL (plist));
      plist = MPLIST_NEXT (plist);
    }
  return pl;
}

/** Parse this form of PLIST:
      ((symbol:KEY1 ANY:VAL1 ... ) (symbol:KEY2 ANY:VAL2 ...) ...)
    and return a newly created plist of this form:
      (KEY1:(ANY:VAL1 ...) KEY2:(ANY:VAL2 ...) ...)
    ANY can be any type.  */

MPlist *
mplist__from_alist (MPlist *plist)
{
  MPlist *pl, *p;

  MPLIST_NEW (pl);
  p = pl;
  MPLIST_DO (plist, plist)
    {
      MPlist *elt;

      if (! MPLIST_PLIST_P (plist))
	MERROR (MERROR_PLIST, NULL);
      elt = MPLIST_PLIST (plist);
      if (! MPLIST_SYMBOL_P (elt))
	MERROR (MERROR_PLIST, NULL);
      MPLIST_SET_ADVANCE (p, MPLIST_SYMBOL (elt), MPLIST_NEXT (elt));
      M17N_OBJECT_REF (MPLIST_NEXT (elt));
    }
  return pl;
}


MPlist *
mplist__from_file (FILE *fp, MPlist *keys)
{
  MPlist *plist, *pl;
  MStream st;

  st.fp = fp;
  st.eof = 0;
  st.p = st.pend = st.buffer;
  MPLIST_NEW (plist);
  pl = plist;
  while ((pl = read_element (pl, &st, keys)));
  return plist;
}


/** Parse $STR of $N bytes and return a property list object.  $FORMAT
    must be either @c MTEXT_FORMAT_US_ASCII or @c MTEXT_FORMAT_UTF_8,
    and controls how to produce @c STRING or @c M-TEXT in the
    following definition.

    The syntax of $STR is as follows.

    PLIST ::= '(' ELEMENT * ')'

    ELEMENT ::= SYMBOL | INTEGER | UNSIGNED | STRING | M-TEXT | PLIST

    SYMBOL ::= ascii-character-sequence

    INTEGER ::= '-' ? [ '0' | .. | '9' ]+

    UNSIGNED ::= '0x' [ '0' | .. | '9' | 'A' | .. | 'F' | 'a' | .. | 'f' ]+

    M-TEXT ::= '"' byte-sequence '"'

    Each kind of @c ELEMENT is assigned one of these keys:
	@c Msymbol, @c Mint, @c Munsigned, @c Mtext, @c Mplist

    In an ascii-character-sequence, a backslush (\) is used as the escape
    character, which means that, for instance, <tt>"abc\ def"</tt>
    produces a symbol whose name is of length seven with the fourth
    character being a space.

    In a byte-sequence, "\r", "\n", "\e", and "\t" are replaced by CR,
    NL, ESC, and TAB character respectively, "\xXX" are replaced by
    byte 0xXX.  After this replacement, the byte-sequence is decoded
    into M-TEXT by $CODING.  */

MPlist *
mplist__from_string (unsigned char *str, int n)
{
  MPlist *plist, *pl;
  MStream st;

  st.fp = NULL;
  st.eof = 0;
  st.p = str;
  st.pend = str + n;
  MPLIST_NEW (plist);
  pl = plist;
  while ((pl = read_element (pl, &st, NULL)));
  return plist;
}

int
mplist__serialize (MText *mt, MPlist *plist, int pretty)
{
  MPlist *pl;
  int separator = pretty ? '\n' : ' ';

  MPLIST_DO (pl, plist)
    {
      if (pl != plist)
	mtext_cat_char (mt, separator);
      write_element (mt, pl, pretty ? 0 : -1);
    }
  if (pretty)
    mtext_cat_char (mt, separator);
  return 0;
}

/**en
    @brief Concatenate two plists.

    The mplist__conc () function concatenates plist $TAIL at the end of
    plist $PLIST and return $PLIST.  If $TAIL is empty, return $PLIST
    without modifying it.  */

MPlist *
mplist__conc (MPlist *plist, MPlist *tail)
{
  MPlist *pl;

  if (MPLIST_TAIL_P (tail))
    return plist;
  MPLIST_DO (pl, plist);
  MPLIST_KEY (pl) = MPLIST_KEY (tail);
  MPLIST_VAL (pl) = MPLIST_VAL (tail);
  if (MPLIST_KEY (pl)->managing_key)
    M17N_OBJECT_REF (MPLIST_VAL (pl));
  tail = MPLIST_NEXT (tail);
  MPLIST_NEXT (pl) = tail;
  M17N_OBJECT_REF (tail);
  return plist;
}

/*=*/
/**en
    @brief Discard a property at the beginning of a property list.

    The mplist__pop_unref () function removes a property at the
    beginning of property list $PLIST, and if the property value is a
    managed object, unref it.  As a result, the second key and value
    of the original $PLIST become the first of those of the new
    $PLIST.  */

void
mplist__pop_unref (MPlist *plist)
{
  MSymbol key;
  void *val;

  if (MPLIST_TAIL_P (plist))
    return;
  key = MPLIST_KEY (plist);
  val = mplist_pop (plist);
  if (key->managing_key)
    M17N_OBJECT_UNREF (val);
}

/**en
    @brief Search for an element of an alist represented by a plist.

    The mplist__assq () function treats $PLIST as an association list
    (elements are plists (key is #Mplist) whose first element is a
    symbol (key is #Msymbol)), and find an element whose first element
    has key #Msymbol and value $KEY.

    Non-plist elements of $PLIST are ignored.

    @return
    This function returns a found element or NULL if no element
    matches with $KEY.  */

MPlist *
mplist__assq (MPlist *plist, MSymbol key)
{
  MPLIST_DO (plist, plist)
    if (MPLIST_PLIST_P (plist))
      {
	MPlist *pl = MPLIST_PLIST (plist);

	if (MPLIST_SYMBOL_P (pl) && MPLIST_SYMBOL (pl) == key)
	  return plist;
      }
  return NULL;
}

/*** @} */
#endif /* !FOR_DOXYGEN || DOXYGEN_INTERNAL_MODULE */


/* External API */

/*** @addtogroup m17nPlist */
/*** @{ */
/*=*/

/***en
    @brief Symbol whose name is "integer".

    The symbol @c Minteger has the name <tt>"integer"</tt>.  The value
    of a property whose key is @c Minteger must be an integer.  */
/***ja
    @brief "integer" を名前として持つシンボル.

    シンボル @c Minteger は <tt>"integer"</tt> という名前を持つ。キーが
    @c Minteger であるプロパティの値は整数値でなくてはならない。  */

MSymbol Minteger;
/*=*/

/***en
    @brief Symbol whose name is "plist".

    The symbol @c Mplist has the name <tt>"plist"</tt>.  It is a
    managing key.  A value of a property whose key is @c Mplist must
    be a plist.  */
/***ja
    @brief "plist" を名前として持つシンボル.

    シンボル @c Mplist は <tt>"plist"</tt> 
    という名前を持つ。これは管理キーである。キーが @c Mplist 
    であるプロパティの値は plist でなくてはならない。  */

MSymbol Mplist;
/*=*/

/***en
    @brief Symbol whose name is "mtext".

    The symbol @c Mtext has the name <tt>"mtext"</tt>.  It is a
    managing key.  A value of a property whose key is @c Mtext must be an
    M-text.  */

/***ja
    @brief "mtext" を名前として持つシンボル.

    シンボル @c Mtext は <tt>"mtext"</tt>
    という名前を持つ管理キーである。キーが @c Mtext 
    であるプロパティの値は M-text でなくてはならない。      */

MSymbol Mtext;

/*=*/
/***en
    @brief Create a property list object.

    The mplist () function returns a newly created property list
    object of length zero.

    @returns
    This function returns a newly created property list.

    @errors
    This function never fails.  */
/***ja
    @brief プロパティリストオブジェクトを作る.

    関数 mplist () は長さ 0 のプロパティリストオブジェクトを新しく作って返す。

    @returns
    この関数は新しく作られたプロパティリストオブジェクトを返す。

    @errors
    この関数は決して失敗しない。     */

MPlist *
mplist (void)
{
  MPlist *plist;

  MPLIST_NEW (plist);
  return plist;
}  

/*=*/
/***en
    @brief Copy a property list.

    The mplist_copy () function copies property list $PLIST.  In the
    copy, the values are the same as those of $PLIST.

    @return
    This function returns a newly created plist which is a copy of
    $PLIST.  

    @errors
    This function never fails.  */ 
/***ja
    @brief プロパティリストをコピーする.

    関数 mplist_copy () はプロパティリスト $PLIST 
    をコピーする。コピーのすべての値はコピー元 $PLIST の値と同じである。

    @return
    この関数は新しく作られた、$PLIST のコピーであるプロパティリストを返す。    

    @errors
    この関数は決して失敗しない。     */

MPlist *
mplist_copy (MPlist *plist)
{
  MPlist *copy = mplist (), *pl = copy;

  MPLIST_DO (plist, plist)
    pl = mplist_add (pl, MPLIST_KEY (plist), MPLIST_VAL (plist));
  return copy;
}

/*=*/

/***en
    @brief Set the value of a property in a property list.

    The mplist_put () function searches property list $PLIST
    from the beginning for a property whose key is $KEY.  If such a
    property is found, its value is changed to $VALUE.  Otherwise, a
    new property whose key is $KEY and value is $VALUE is appended at
    the end of $PLIST.  See the documentation of mplist_add () for
    the restriction on $KEY and $VAL.

    If $KEY is a managing key, $VAL must be a managed object.  In this
    case, the reference count of the old value, if not @c NULL, is
    decremented by one, and that of $VAL is incremented by one.

    @return
    If the operation was successful, mplist_put () returns a sublist of
    $PLIST whose first element is the just modified or added one.
    Otherwise, it returns @c NULL.  */
/***ja
    @brief プロパティリスト中のプロパティの値を設定する.

    関数 mplist_put () はプロパティリスト $PLIST を始めから探して、キーが
    $KEY であるプロパティを見つける。見つかれば、その値を $VALUE 
    に変更する。見つからなければ、キーが $KEY で値が $VALUE 
    である新しいプロパティが $PLIST の末尾に追加される。$KEY と $VAL
    に対する制限については、mplist_add () の説明を参照。

    $KEY が管理キーならば、
    $VAL は管理下オブジェクトでなくてはならない。この場合、古い値の参照数は 
    @c NULL でなければ 1 減らされ、$VAL の参照数は 1 増やされる。

    @return 
    処理が成功すれば mplist_put () は変更されたか追加された要素から始まる
    $PLIST の部分リストを返す。そうでなければ @c NULL を返す。   */

MPlist *
mplist_put (MPlist *plist, MSymbol key, void *val)
{
  if (key == Mnil)
    MERROR (MERROR_PLIST, NULL);
  MPLIST_FIND (plist, key);
  if (key->managing_key)
    {
      if (! MPLIST_TAIL_P (plist))
	M17N_OBJECT_UNREF (MPLIST_VAL (plist));
      if (val)
	M17N_OBJECT_REF (val);
    }
  MPLIST_SET (plist, key, val);
  return plist;
}

/*=*/

/***en
    @brief Get the value of a property in a property list.

    The mplist_get () function searches property list $PLIST from the
    beginning for a property whose key is $KEY.  If such a property is
    found, its value is returned as the type of <tt>(void *)</tt>.  If
    not found, @c NULL is returned.

    When @c NULL is returned, there are two possibilities: one is the
    case where no property is found (see above); the other is the case
    where a property is found and its value is @c NULL.  In case that
    these two cases must be distinguished, use the mplist_find_by_key ()
    function.  */
/***ja
    @brief プロパティリスト中のプロパティの値を得る.

    関数 mplist_get () は、プロパティリスト $PLIST を始めから探して、キー
    が $KEY であるプロパティを見つける。見つかれば、その値を
    <tt>(void *)</tt> 型で返す。見つからなければ @c NULL を返す。

    @c NULL が返った際には二つの可能性がある: 
    上記のようにプロパティが見つからなかった場合と、プロパティが見つかり、その値が
    @c NULL である場合である。これらを区別する必要がある場合には関数 
    mplist_find_by_key () を使うこと。  */

/***
    @seealso
    mplist_find_by_key () */

void *
mplist_get (MPlist *plist, MSymbol key)
{
  MPLIST_FIND (plist, key);
  return (MPLIST_TAIL_P (plist) ? NULL : MPLIST_VAL (plist));
}

/*=*/

/***en
    @brief Set the value (function pointer) of a property in a property list.

    The mplist_put_func () function is like mplist_put () but for
    settting function pointer $FUNC in property list $PLIST for key
    $KEY.  */

/***
    @seealso
    mplist_put (), M17N_FUNC ()  */

MPlist *
mplist_put_func (MPlist *plist, MSymbol key, M17NFunc func)
{
  if (key == Mnil)
    MERROR (MERROR_PLIST, NULL);
  do {
    MPLIST_FIND (plist, key);
  } while (! MPLIST_TAIL_P (plist) && ! MPLIST_VAL_FUNC_P (plist));

  MPLIST_KEY (plist) = (key);
  MPLIST_FUNC (plist) = func;
  if (! plist->next)
    MPLIST_NEW ((plist)->next);
  return plist;
}

/*=*/

/***en
    @brief Get the value (function pointer) of a property in a property list.

    The mplist_get_func () funciont is like mplist_get () but for
    getting a function pointer from property list $PLIST by key $KEY.  */

/***
    @seealso
    mplist_get () */
M17NFunc
mplist_get_func (MPlist *plist, MSymbol key)
{
  do {
    MPLIST_FIND (plist, key);
  } while (! MPLIST_TAIL_P (plist) && ! MPLIST_VAL_FUNC_P (plist));
  return (MPLIST_TAIL_P (plist) ? NULL : MPLIST_FUNC (plist));
}

/*=*/

/***en
    @brief Add a property at the end of a property list.

    The mplist_add () function appends at the end of property list
    $PLIST a property whose key is $KEY and value is $VAL.  $KEY can
    be any symbol other than @c Mnil.

    If $KEY is a managing key, $VAL must be a managed object.  In this
    case, the reference count of $VAL is incremented by one.

    @return
    If the operation was successful, mplist_add () returns a sublist of
    $PLIST whose first element is the just added one.  Otherwise, it
    returns @c NULL.  */
/***ja
    @brief プロパティリスト末尾にプロパティを追加する.

    関数 mplist_add () は、プロパティリスト $PLIST の末尾にキーが $KEY 
    で値が $VAL であるプロパティを追加する。$KEY は、@c Mnil 以外の任意のシンボルでよい。

    $KEY が管理キーならば、$VAL は管理下オブジェクトでなくてはならない。この場合、
    $VAL の参照数は 1 増やされる。

    @return
    処理が成功すれば mplist_add () は追加された要素から始まる $PLIST 
    の部分リストを返す。そうでなければ @c NULL を返す。  */

MPlist *
mplist_add (MPlist *plist, MSymbol key, void *val)
{
  if (key == Mnil)
    MERROR (MERROR_PLIST, NULL);
  MPLIST_FIND (plist, Mnil);
  if (val && key->managing_key)
    M17N_OBJECT_REF (val);
  MPLIST_KEY (plist) = key;
  MPLIST_VAL (plist) = val;
  MPLIST_NEW (plist->next);
  return plist;
}

/*=*/

/***en
    @brief Add a property at the beginning of a property list.

    The mplist_push () function inserts at the beginning of property
    list $PLIST a property whose key is $KEY and value is $VAL.

    If $KEY is a managing key, $VAL must be a managed object.  In this
    case, the reference count of $VAL is incremented by one.

    @return
    If the operation was successful, this function returns $PLIST.
    Otherwise, it returns @c NULL.  */
/***ja
    @brief プロパティリストの先頭にプロパティを挿入する.

    関数 mplist_push () はプロパティリスト $PLIST の先頭にキーが $KEY 
    で値が $VAL であるオブジェクトを挿入する。

    $KEY が管理キーならば、$VAL は管理下オブジェクトでなくてはならない。この場合、
    $VAL の参照数は 1 増やされる。

    @return
    処理が成功すればこの関数は $PLIST を返し、そうでなければ@c NULL 
    を返す。  */

MPlist *
mplist_push (MPlist *plist, MSymbol key, void *val)
{
  MPlist *pl;

  if (key == Mnil)
    MERROR (MERROR_PLIST, NULL);
  MPLIST_NEW (pl);
  MPLIST_KEY (pl) = MPLIST_KEY (plist);
  MPLIST_VAL (pl) = MPLIST_VAL (plist);
  MPLIST_NEXT (pl) = MPLIST_NEXT (plist);
  plist->next = pl;
  if (val && key->managing_key)
    M17N_OBJECT_REF (val);
  MPLIST_KEY (plist) = key;
  MPLIST_VAL (plist) = val;
  return plist;
}

/*=*/

/***en
    @brief Remove a property at the beginning of a property list.

    The mplist_pop () function removes a property at the beginning of
    property list $PLIST.  As a result, the second key and value of
    the $PLIST become the first ones.

    @return
    If the operation was successful, this function return the value of
    the just popped property.  Otherwise, it returns @c NULL.  */
/***ja
    @brief プロパティリストの先頭からプロパティを削除する.

    関数 mplist_pop () はプロパティリスト $PLIST の先頭のプロパティを削
    除する。結果として、元の2番目のキーと値が先頭のキーと値になる。

    @return 
    処理に成功すれば、この関数は削除されたプロパティの値を返す。そうでなければ
    @c NULL を返す。  */

void *
mplist_pop (MPlist *plist)
{
  void *val;
  MPlist *next;

  if (MPLIST_TAIL_P (plist))
    return NULL;
  val = MPLIST_VAL (plist);
  next = MPLIST_NEXT (plist);
  MPLIST_KEY (plist) = MPLIST_KEY (next);
  MPLIST_VAL (plist) = MPLIST_VAL (next);
  if (MPLIST_KEY (plist) != Mnil
      && MPLIST_KEY (plist)->managing_key
      && MPLIST_VAL (plist))
    M17N_OBJECT_REF (MPLIST_VAL (plist));
  MPLIST_NEXT (plist) = MPLIST_NEXT (next);
  if (plist->next)
    M17N_OBJECT_REF (plist->next);
  M17N_OBJECT_UNREF (next);
  return val;
}

/*=*/
/***en
    @brief Find a property of a specific key in a property list.

    The mplist_find_by_key () function searches property list
    $PLIST from the beginning for a property whose key is $KEY.  If
    such a property is found, a sublist of $PLIST whose first element
    is the found one is returned.  Otherwise, @c NULL is returned.

    If $KEY is @c Mnil, it returns a sublist of $PLIST whose
    first element is the last one of $PLIST.  */
/***ja
    @brief プロパティリスト中から指定のキーを持つプロパティを探す.

    関数 mplist_find_by_key () はプロパティリスト $PLIST 
    を始めから探 して、キーが $KEY 
    であるプロパティを見つける。見つかれば、そのプロパティから始まる
    $PLIST の部分リストを返す。そうでなければ @c NULL を返す。

    $KEY が @c Mnil ならば、$PLIST の最後の要素から始まる部分リストを返す。  */

MPlist *
mplist_find_by_key (MPlist *plist, MSymbol key)
{
  MPLIST_FIND (plist, key);
  return (MPLIST_TAIL_P (plist)
	  ? (key == Mnil ? plist : NULL)
	  : plist);
}

/*=*/
/***en
    @brief Find a property of a specific value in a property list.

    The mplist_find_by_value () function searches property list $PLIST
    from the beginning for a property whose value is $VAL.  If such a
    property is found, a sublist of $PLIST whose first element is the
    found one is returned.  Otherwise, @c NULL is returned.  */
/***ja
    @brief プロパティリスト中から指定の値を持つプロパティを探す.

    関数 mplist_find_by_value () はプロパティリスト $PLIST 
    を始めから探して、値が $VAL 
    であるプロパティを見つける。見つかれば、そのプロパティから始まる
    $PLIST の部分リストを返す。そうでなければ @c NULL を返す。 */

MPlist *
mplist_find_by_value (MPlist *plist, void *val)
{
  MPLIST_DO (plist, plist)
    {
      if (MPLIST_VAL (plist) == val)
	return plist;
    }
  return NULL;
}

/*=*/

/***en
    @brief Return the next sublist of a property list.

    The mplist_next () function returns a pointer to the sublist of
    property list $PLIST, which begins at the second element in $PLIST.  If the
    length of $PLIST is zero, it returns @c NULL.  */
/***ja
    @brief プロパティリストの次の部分リストを返す.

    関数 mplist_next () はプロパティリスト $PLIST の 2 
    番目の要素から始まる部分リストへのポインタを返す。$PLIST の長さが 0 
    ならば @c NULL を返す。  */

MPlist *
mplist_next (MPlist *plist)
{
  return (MPLIST_TAIL_P (plist) ? NULL : plist->next);
}

/*=*/

/***en
    @brief Set the first property in a property list.

    The mplist_set () function sets the key and the value of the first
    property in property list $PLIST to $KEY and $VALUE, respectively.
    See the documentation of mplist_add () for the restriction on $KEY
    and $VAL.

    @return
    If the operation was successful, mplist_set () returns $PLIST.
    Otherwise, it returns @c NULL.  */
/***ja
    @brief プロパティリストの最初のプロパティを設定する.

    関数 mplist_set () はプロパティリスト $PLIST 
    の最初のプロパティのキーと値をそれぞれ $KEY と $VALUE に設定する。
    $KEY と $VAL に対する制限については、mplist_add () の説明を参照。

    @return
    処理に成功すれば mplist_set () は $PLIST を返す。そうでなければ @c NULL を返す。  */

MPlist *
mplist_set (MPlist *plist, MSymbol key, void * val)
{
  if (key == Mnil)
    {
      if (! MPLIST_TAIL_P (plist))
	{
	  key = MPLIST_KEY (plist);
	  M17N_OBJECT_UNREF (MPLIST_NEXT (plist));
	  MPLIST_KEY (plist) = Mnil;
	  if (key->managing_key)
	    M17N_OBJECT_UNREF (MPLIST_VAL (plist));
	  plist->next = NULL;
	}
    }
  else
    {
      if (val && key->managing_key)
	M17N_OBJECT_REF (val);
      if (! MPLIST_TAIL_P (plist)
	  && MPLIST_KEY (plist)->managing_key)
	M17N_OBJECT_UNREF (MPLIST_VAL (plist));
      MPLIST_SET (plist, key, val);
    }
  return plist;
}

/*=*/

/***en
    @brief Return the length of a property list.

    The mplist_length () function returns the number of properties in
    property list  $PLIST.  */
/***ja
    @brief プロパティリストの長さを返す.

    関数 mplist_length () はプロパティリスト $PLIST 中のプロパティの数を返す。  */

int
mplist_length (MPlist *plist)
{
  int n;

  for (n = 0; ! (MPLIST_TAIL_P (plist)); n++, plist = plist->next);
  return n;
}

/*=*/

/***en
    @brief Return the key of the first property in a property list.

    The mplist_key () function returns the key of the first property
    in property list $PLIST.  If the length of $PLIST is zero,
    it returns @c Mnil.  */
/***ja
    @brief プロパティリスト中の最初のプロパティのキーを返す.

    関数 mplist_key () は、プロパティリスト $PLIST 
    中の最初のプロパティのキーを返す。$PLIST の長さが 0 ならば、 @c Mnil 
    を返す。  */

MSymbol
mplist_key (MPlist *plist)
{
  return MPLIST_KEY (plist);
}

/*=*/

/***en
    @brief Return the value of the first property in a property list.

    The mplist_value () function returns the value of the first
    property in property list  $PLIST.  If the length of $PLIST
    is zero, it returns @c NULL.  */
/***ja
    @brief プロパティリスト中の最初のプロパティの値を返す.

    関数 mplist_value () は、プロパティリスト $PLIST 中の最初のプロパティの値を返す。
    $PLIST の長さが 0 ならば、 @c Mnil を返す。  */

void *
mplist_value (MPlist *plist)
{
  return MPLIST_VAL (plist);
}

/***en
    @brief Generate a property list by deserializing an M-text.

    The mplist_deserialize () function parses M-text $MT and returns a
    property list.

    The syntax of $MT is as follows.

    MT ::= '(' ELEMENT * ')'

    ELEMENT ::= SYMBOL | INTEGER | M-TEXT | PLIST

    SYMBOL ::= ascii-character-sequence

    INTEGER ::= '-' ? [ '0' | .. | '9' ]+
		| '0x' [ '0' | .. | '9' | 'A' | .. | 'F' | 'a' | .. | 'f' ]+

    M-TEXT ::= '"' character-sequence '"'

    Each alternatives of @c ELEMENT is assigned one of these keys: @c
    Msymbol, @c Minteger, @c Mtext, @c Mplist

    In an ascii-character-sequence, a backslash (\) is used as the escape
    character, which means that, for instance, <tt>"abc\ def"</tt>
    produces a symbol whose name is of length seven with the fourth
    character being a space.  */
/***ja
    @brief M-text をデシリアライズしてプロパティリストを作る.

    関数 mplist_deserialize () は M-text $MT を解析してプロパティリストを返す。

    $MT のシンタックスは以下の通り。

    MT ::= '(' ELEMENT * ')'

    ELEMENT ::= SYMBOL | INTEGER | M-TEXT | PLIST

    SYMBOL ::= アスキー文字列

    INTEGER ::= '-' ? [ '0' | .. | '9' ]+
		| '0x' [ '0' | .. | '9' | 'A' | .. | 'F' | 'a' | .. | 'f' ]+

    M-TEXT ::= '"' character-sequence '"'

    @c ELEMENT の各選択肢はキー：@c Msymbol, @c Minteger, @c Mtext,
    @c Mplist のいずれかを割り当てられている。

    アスキー文字列内では、バックスラッシュ (\) がエスケープ文字として用いられる。たとえば
    <tt>"abc\ def"</tt> は 4 文字目が空白文字であり長さが 7 
    である持つ名前を持つシンボルを生成する。   */

MPlist *
mplist_deserialize (MText *mt)
{
  MPlist *plist;
  MText *tmp = NULL;

  if (mt->format > MTEXT_FORMAT_UTF_8)
    {
      if (MTEXT_READ_ONLY_P (mt))
	mt = tmp = mtext_cpy (mtext (), mt);
      else
	mtext__adjust_format (mt, MTEXT_FORMAT_UTF_8);
    }
  plist = mplist__from_string (MTEXT_DATA (mt), mtext_nbytes (mt));
  if (tmp)
    M17N_OBJECT_UNREF (tmp);
  return plist;
}

/*** @}  */

/*** @addtogroup m17nDebug */
/*=*/
/*** @{  */

/***en
    @brief Dump a property list.

    The mdebug_dump_plist () function prints a property list $PLIST in
    a human readable way to the stderr.  $INDENT specifies how many
    columns to indent the lines but the first one.

    @return
    This function returns $PLIST.  */
/***ja
    @brief プロパティリストをダンプする.

    関数 mdebug_dump_plist () はプロパティリスト $PLIST を stderr 
    に人間に可読な形で印刷する。 $INDENT は２行目以降のインデントを指定する。

    @return
    この関数は $PLIST を返す。  */
MPlist *
mdebug_dump_plist (MPlist *plist, int indent)
{
  char *prefix = (char *) alloca (indent + 1);
  MPlist *pl;

  memset (prefix, 32, indent);
  prefix[indent] = 0;

  fprintf (stderr, "(");
  MPLIST_DO (pl, plist)
    {
      if (pl != plist)
	fprintf (stderr, "\n%s ", prefix);
      write_element (NULL, pl, indent + 1);
    }
  fprintf (stderr, ")");
  return plist;
}

/*** @} */

/*
  Local Variables:
  coding: euc-japan
  End:
*/
