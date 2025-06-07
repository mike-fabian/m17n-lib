/* minputtest.c -- Input method tester.			-*- coding: utf-8; -*-
   Copyright (C) 2025 David Mandelberg

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
   Boston, MA 02110-1301 USA.  */

/***en
    @enpage m17n-input-test test an input method

    @section m17n-input-test-synopsis SYNOPSIS

    m17n-input-test-synopsis [ OPTION ... ]

    @section m17n-input-test-description DESCRIPTION

    Feed keys to an input method and assert that it does what's expected. Run
    with --help to see the options.
*/

#ifndef FOR_DOXYGEN

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <m17n.h>

#define ARRAY_SIZE 4096
#define TEXT_BUF_SIZE 4096

static void
array_append (const char *array[ARRAY_SIZE], size_t *length, const char *value)
{
  if (*length >= ARRAY_SIZE)
    {
      fprintf (
          stderr,
          "Array too long, consider increasing ARRAY_SIZE in minputtest.c\n");
      exit (1);
    }
  array[*length] = value;
  (*length)++;
}

static void
text_to_buf (MText *text, char buf[TEXT_BUF_SIZE])
{
  int written = mconv_encode_buffer (Mcoding_utf_8, text, buf, TEXT_BUF_SIZE);
  if (written < 0)
    {
      fprintf (stderr, "mconv_encode_buffer failed.\n");
      exit (1);
    }
  if (written >= TEXT_BUF_SIZE)
    {
      fprintf (stderr, "Text too long, consider increasing TEXT_BUF_SIZE in "
                       "minputtest.c.\n");
      exit (1);
    }
  buf[written] = '\x00';
  if (strlen (buf) != written)
    {
      fprintf (stderr, "Text has NULL byte, which is not yet supported.\n");
      exit (1);
    }
}

static const char *
program_name (const char *arg0)
{
  const char *name = arg0;
  const char *p = arg0;
  while (*p)
    {
      if (*p++ == '/')
        {
          name = p;
        }
    }
  return name;
}

/* CLI arguments.
   char pointers are either to static strings or to parts of argv.
*/
typedef struct
{
  /* Which input method to test. */
  const char *language;
  const char *name;

  /* Input keysyms. */
  const char *keys[ARRAY_SIZE];
  size_t keys_length;

  /* Expected results. */
  const char *commit;
  bool candidates_show;
  /* NULL indicates the boundary between candidate groups. */
  const char *candidates[ARRAY_SIZE];
  size_t candidates_length;
  const char *preedit;
} Args;

static void
help_exit (const char *arg0, int exit_code)
{
  FILE *stream = exit_code ? stderr : stdout;
  fprintf (stream, "Usage: %s [ OPTION ... ]\n", program_name (arg0));
  fprintf (stream, "Test an input method.\n");
  fprintf (stream, "The following OPTIONs are available.\n");
  fprintf (stream, "  %-13s %s", "--language", "Input method language.\n");
  fprintf (stream, "  %-13s %s", "--name", "Input method name.\n");
  fprintf (stream, "  %-13s %s", "-k",
           "Key symbol to feed to the IM, can be repeated.\n");
  fprintf (stream, "  %-13s %s", "--commit",
           "Text that the IM should commit.\n");
  fprintf (stream, "  %-13s %s", "--candidates-show",
           "If the candidate list should be shown.\n");
  fprintf (stream, "  %-13s %s", "-c",
           "An expected candidate, can be repeated.\n");
  fprintf (stream, "  %-13s %s", "--next-group",
           "Divider between candidate groups.\n");
  fprintf (stream, "  %-13s %s", "--preedit", "Expected preedit.\n");
  fprintf (stream, "  %-13s %s", "--version", "Print version number.\n");
  fprintf (stream, "  %-13s %s", "-h, --help", "Print this message.\n");
  exit (exit_code);
}

static Args
parse_args (int argc, char *argv[])
{
  Args args = {
    .language = NULL,
    .name = NULL,
    .keys_length = 0,
    .commit = "",
    .candidates_show = false,
    .candidates_length = 0,
    .preedit = "",
  };

  for (size_t i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "--language") && i + 1 < argc)
        {
          args.language = argv[++i];
        }
      else if (!strcmp (argv[i], "--name") && i + 1 < argc)
        {
          args.name = argv[++i];
        }
      else if (!strcmp (argv[i], "-k") && i + 1 < argc)
        {
          array_append (args.keys, &args.keys_length, argv[++i]);
        }
      else if (!strcmp (argv[i], "--commit") && i + 1 < argc)
        {
          args.commit = argv[++i];
        }
      else if (!strcmp (argv[i], "--candidates-show"))
        {
          args.candidates_show = true;
        }
      else if (!strcmp (argv[i], "-c") && i + 1 < argc)
        {
          array_append (args.candidates, &args.candidates_length, argv[++i]);
        }
      else if (!strcmp (argv[i], "--next-group"))
        {
          array_append (args.candidates, &args.candidates_length, NULL);
        }
      else if (!strcmp (argv[i], "--preedit") && i + 1 < argc)
        {
          args.preedit = argv[++i];
        }
      else if (!strcmp (argv[i], "--version"))
        {
          printf ("%s (m17n library) %s\n", program_name (argv[0]),
                  M17NLIB_VERSION_NAME);
          exit (0);
        }
      else if (!strcmp (argv[i], "-h") || !strcmp (argv[i], "--help"))
        {
          help_exit (argv[0], 0);
        }
      else
        {
          help_exit (argv[0], 1);
        }
    }

  if (!args.language)
    {
      fprintf (stderr, "Missing argument: --language language\n");
      help_exit (argv[0], 1);
    }
  if (!args.name)
    {
      fprintf (stderr, "Missing argument: --name name\n");
      help_exit (argv[0], 1);
    }

  return args;
}

static bool
expect_mtext_equal (const char *field_name, MText *actual,
                    const char *expected)
{
  char buf[TEXT_BUF_SIZE];
  text_to_buf (actual, buf);
  if (strcmp (buf, expected))
    {
      fprintf (stderr, "%s does not match. Expected '%s', got '%s'.\n",
               field_name, expected, buf);
      return false;
    }
  else
    {
      return true;
    }
}

static bool
candidate_lists_equal (const char *list1[ARRAY_SIZE], size_t length1,
                       const char *list2[ARRAY_SIZE], size_t length2)
{
  if (length1 != length2)
    {
      return false;
    }
  for (size_t i = 0; i < length1; i++)
    {
      if (list1[i] && list2[i] && !strcmp (list1[i], list2[i]))
        {
          continue;
        }
      else if (!list1[i] && !list2[i])
        {
          continue;
        }
      else
        {
          return false;
        }
    }
  return true;
}

static bool
print_candidate_list (const char **list, size_t length)
{
  fprintf (stderr, "  (\n");
  for (size_t i = 0; i < length; i++)
    {
      if (list[i])
        {
          fprintf (stderr, "    '%s'\n", list[i]);
        }
      else
        {
          fprintf (stderr, "  )\n");
          fprintf (stderr, "  (\n");
        }
    }
  fprintf (stderr, "  )\n");
}

static bool
expect_candidate_list_equal (MPlist *actual, const char *expected[ARRAY_SIZE],
                             size_t expected_length)
{
  const char *actual_strings[ARRAY_SIZE];
  size_t actual_strings_length = 0;
  for (MPlist *group_head = actual;
       group_head && mplist_key (group_head) != Mnil;
       group_head = mplist_next (group_head))
    {
      if (mplist_key (group_head) == Mplist)
        {
          for (MPlist *candidate_head = mplist_value (group_head);
               mplist_key (candidate_head) != Mnil;
               candidate_head = mplist_next (candidate_head))
            {
              if (mplist_key (candidate_head) != Mtext)
                {
                  fprintf (stderr, "Candidate should be text but is '%s'.\n",
                           msymbol_name (mplist_key (candidate_head)));
                  exit (1);
                }
              char buf[TEXT_BUF_SIZE];
              text_to_buf (mplist_value (candidate_head), buf);
              array_append (actual_strings, &actual_strings_length,
                            strdup (buf));
            }
        }
      else if (mplist_key (group_head) == Mtext)
        {
          MText *group_value = mplist_value (group_head);
          for (int i = 0; i < mtext_len (group_value); i++)
            {
              MText *candidate = mtext ();
              if (!mtext_cat_char (candidate, mtext_ref_char (group_value, i)))
                {
                  fprintf (stderr, "mtext_cat_char failed.\n");
                  exit (1);
                }
              char buf[TEXT_BUF_SIZE];
              text_to_buf (candidate, buf);
              array_append (actual_strings, &actual_strings_length,
                            strdup (buf));
              m17n_object_unref (candidate);
            }
        }
      else
        {
          fprintf (stderr,
                   "Candidate list should have only lists or texts, but has "
                   "'%s'.\n",
                   msymbol_name (mplist_key (group_head)));
          exit (1);
        }
      if (mplist_key (mplist_next (group_head)) != Mnil)
        {
          array_append (actual_strings, &actual_strings_length, NULL);
        }
    }

  bool equal = candidate_lists_equal (actual_strings, actual_strings_length,
                                      expected, expected_length);
  if (!equal)
    {
      fprintf (stderr, "Candidate list does not match. Expected:\n");
      print_candidate_list (expected, expected_length);
      fprintf (stderr, "Actual:\n");
      print_candidate_list (actual_strings, actual_strings_length);
    }

  for (size_t i = 0; i < actual_strings_length; i++)
    {
      free ((void *)actual_strings[i]);
    }

  return equal;
}

int
main (int argc, char *argv[])
{
  Args args = parse_args (argc, argv);

  int retval = 0;
  MInputMethod *im = NULL;
  MInputContext *ic = NULL;

  M17N_INIT ();

  MText *committed = mtext ();

  im = minput_open_im (msymbol (args.language), msymbol (args.name), NULL);
  if (!im)
    {
      fprintf (stderr, "minput_open_im failed.\n");
      retval = 1;
      goto done;
    }
  ic = minput_create_ic (im, NULL);
  if (!ic)
    {
      fprintf (stderr, "minput_create_ic failed.\n");
      retval = 1;
      goto done;
    }

  for (size_t i = 0; i < args.keys_length; i++)
    {
      MSymbol key = msymbol (args.keys[i]);
      if (minput_filter (ic, key, NULL) != 0)
        {
          continue;
        }
      if (minput_lookup (ic, key, NULL, committed) != 0)
        {
          /* Key wasn't handled, so it would be commited or forwarded. */
          for (size_t j = 0; args.keys[i][j]; j++)
            {
              mtext_cat_char (committed, args.keys[i][j]);
            }
        }
    }

  if (!expect_mtext_equal ("committed", committed, args.commit))
    {
      retval = 1;
    }
  if ((bool)ic->candidate_show != args.candidates_show)
    {
      fprintf (stderr, "Error: candidates %s shown.\n",
               ic->candidate_show ? "were" : "were not");
      retval = 1;
    }
  if (!expect_candidate_list_equal (ic->candidate_list, args.candidates,
                                    args.candidates_length))
    {
      retval = 1;
    }
  if (!expect_mtext_equal ("preedit", ic->preedit, args.preedit))
    {
      retval = 1;
    }

done:
  if (ic)
    {
      minput_destroy_ic (ic);
    }
  if (im)
    {
      minput_close_im (im);
    }
  m17n_object_unref (committed);
  M17N_FINI ();

  if (retval)
    {
      fprintf (
          stderr,
          "Running with MDEBUG_INPUT=1 might help debug test failures.\n");
    }

  return retval;
}

#endif /* not FOR_DOXYGEN */
