#include <m17n-gui.h>

#include <config.h>

#ifdef HAVE_WORDCUT

#define THAI_BEG 0x0E00
#define THAI_END 0x0E6F

static int wordcut_initiazlied = 0;

#include <wordcut/wordcut.h>

static Wordcut wordcut;

static MSymbol Mwordcut_wordbeg, Miso_8859_11;

static void
init_th_wordcut ()
{
  Mwordcut_wordbeg = msymbol (" wordcut-wordseg");
  Miso_8859_11 = msymbol ("iso-8859-11");
}

int
thai_line_break (MText *mt, int pos, int from, int to)
{
  WordcutResult result;
  MTextProperty *prop;
  int pos1, pos2, c, i;
  unsigned char *str;

  if (wordcut_initiazlied < 0)
    return pos;
  if (! wordcut_initiazlied)
    {
      if (wordcut_init (&wordcut, WORDCUT_TDICT) != 0)
	{
	  wordcut_initiazlied = -1;
	  return pos;
	}
      init_th_wordcut ();
      wordcut_initiazlied = 1;
    }
  prop = mtext_get_property (mt, pos, Mwordcut_wordbeg);
  if (prop)
    {
      pos1 = mtext_property_start (prop);
      if (pos1 == from)
	pos1 = pos;
    return pos1;
    }
  str = alloca (to - from);
  pos1 = pos - 1;
  while (pos1 >= from
	 && (c = mtext_ref_char (mt, pos1)) >= THAI_BEG && c <= THAI_END)
    str[pos1-- - from] = mchar_encode (Miso_8859_11, c);
  pos1++;
  pos2 = pos;
  while (pos2 < to
	 && (c = mtext_ref_char (mt, pos2)) >= 0x0E00 && c <= 0x0E6F)
    str[pos2++ - from] = mchar_encode (Miso_8859_11, c);
  str[pos2 - from] = 0;
  wordcut_cut (&wordcut, (char *) (str + pos1 - from), &result);
  for (i = 0; i < result.count; i++)
    {
      int start = pos1 + result.start[i];

      prop = mtext_property (Mwordcut_wordbeg, Mt,
			     MTEXTPROP_VOLATILE_WEAK | MTEXTPROP_NO_MERGE);
      mtext_attach_property (mt, start, start + result.offset[i], prop);
      m17n_object_unref (prop);
    }
  prop = mtext_get_property (mt, pos, Mwordcut_wordbeg);
  pos1 = mtext_property_start (prop);
  if (pos1 == from)
    pos1 = pos;
  return pos1;
}

#define CHECK_THAI_LINE_BREAK(c, mt, pos, from, to)		\
  do {								\
    if ((c) >= THAI_BEG && (c) <= THAI_END)			\
      return thai_line_break ((mt), (pos), (from), (to));	\
  } while (0)

#else  /* not HAVE_WORDCUT */

#define CHECK_THAI_LINE_BREAK(c, mt, pos, from, to) (void) 0

#endif	/* not HAVE_WORDCUT */

int
line_break (MText *mt, int pos, int from, int to, int line, int y)
{
  int c = mtext_ref_char (mt, pos);
  int orig_pos = pos;

  if (c == ' ' || c == '\t' || c == '\n')
    {
      for (pos++; pos < to; pos++)
	if ((c = mtext_ref_char (mt, pos)) != ' ' && c != '\t' && c != '\n')
	  break;
    }
  else
    {
      while (pos > from)
	{
	  if (c == ' ' || c == '\t')
	    break;
	  CHECK_THAI_LINE_BREAK (c, mt, pos, from, to);
	  pos--;
	  c = mtext_ref_char (mt, pos);
	}
      if (pos == from)
	pos = orig_pos;
      else
	pos++;
    }
  return pos;
}
