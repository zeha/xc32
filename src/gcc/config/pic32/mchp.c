
/* Subroutines used for PIC32mx support
   Copyright (C) 1989, 1990, 1991, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.
   Contributed by J. Grosbach, james.grosbach@microchip.com, and
   T. Kuhrt, tracy.kuhrt@microchip.com
 
   This file is part of GCC.
 
   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
 
   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
 
   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING.  If not, write to
   the Free Software Foundation, 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifdef __MINGW32__
void *alloca(size_t);
#else
#include <alloca.h>
#endif
#include <stdio.h>
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "cpplib.h"
#include "c-pragma.h"
#include "toplev.h"
#include "tree.h"
#include "output.h"
#include "mchp-protos.h"

const char *    mchp_processor_string = NULL;
const char *    mchp_config_data_dir = NULL;
const char *    mchp_it_transport = NULL;

#define MCHP_CONFIGURATION_DATA_FILENAME "configuration.data"
#define MCHP_CONFIGURATION_HEADER_MARKER \
  "Daytona Configuration Word Definitions: "
#define MCHP_CONFIGURATION_HEADER_VERSION "0001"
#define MCHP_CONFIGURATION_HEADER_SIZE \
  (sizeof (MCHP_CONFIGURATION_HEADER_MARKER) + 5)

#define MCHP_WORD_MARKER        "CWORD:"
#define MCHP_SETTING_MARKER     "CSETTING:"
#define MCHP_VALUE_MARKER       "CVALUE:"
#define MCHP_WORD_MARKER_LEN    (sizeof (MCHP_WORD_MARKER) - 1)
#define MCHP_SETTING_MARKER_LEN (sizeof (MCHP_SETTING_MARKER) - 1)
#define MCHP_VALUE_MARKER_LEN   (sizeof (MCHP_VALUE_MARKER) - 1)

/* Pretty much arbitrary max line length for the config data file.
   Anything longer than this is either absurd or a bogus file. */
#define MCHP_MAX_CONFIG_LINE_LENGTH 1024

struct mchp_config_value {
  char *name;
  char *description;
  unsigned value;
  struct mchp_config_value *next;
};
struct mchp_config_setting {
  char *name;
  char *description;
  unsigned mask;
  struct mchp_config_value *values;
  struct mchp_config_setting *next;
};
struct mchp_config_word {
  unsigned address;
  unsigned mask;
  unsigned default_value;
  struct mchp_config_setting *settings;
};


struct mchp_config_specification {
  struct mchp_config_word *word; /* the definition of the word this value
                                    is referencing */
  unsigned value;           /* the value of the word to put to the device */
  unsigned referenced_bits; /* the bits which have been explicitly specified
                                i.e., have had a setting = value pair in a
                                config pragma */
  struct mchp_config_specification *next;
};

static struct mchp_config_specification *mchp_configuration_values;


/* get a line, and remove any line-ending \n or \r\n */
static char *
get_line (char *buf, size_t n, FILE *fptr)
{
  if (fgets (buf, n, fptr) == NULL)
    return NULL;
  while (buf [strlen (buf) - 1] == '\n'
      || buf [strlen (buf) - 1] == '\r')
    buf [strlen (buf) - 1] = '\0';
  return buf;
}

/* Verify the header record for the configuration data file
 */
static int
verify_configuration_header_record (FILE *fptr)
{
  char header_record[MCHP_CONFIGURATION_HEADER_SIZE + 1];
  /* the first record of the file is a string identifying
     file and its format version number. */
  if (get_line (header_record, MCHP_CONFIGURATION_HEADER_SIZE + 1, fptr) 
      == NULL)
    {
      warning ("malformed configuration word definition file.");
      return 1;
    }
  /* verify that this file is a daytona configuration word file */
  if (strncmp (header_record, MCHP_CONFIGURATION_HEADER_MARKER,
        sizeof (MCHP_CONFIGURATION_HEADER_MARKER) - 1) != 0)
    {
      warning ("malformed configuration word definition file.");
      return 1;
    }
  /* verify that the version number is one we can deal with */
  if (strncmp (header_record + sizeof (MCHP_CONFIGURATION_HEADER_MARKER) - 1,
               MCHP_CONFIGURATION_HEADER_VERSION,
               sizeof (MCHP_CONFIGURATION_HEADER_VERSION) - 1))
    {
      warning ("configuration word definition file version mismatch.");
      return 1;
    }
  return 0;
}

/* Load the configuration word definitions from the data file
 */
static int
load_configuration_definition (const char *fname)
{
  int retval = 0;
  FILE *fptr;
  char line [MCHP_MAX_CONFIG_LINE_LENGTH];

  if ((fptr = fopen (fname, "rb")) == NULL)
    return 1;

  if (verify_configuration_header_record (fptr))
    return 1;

  while (get_line (line, sizeof (line), fptr) != NULL)
    {
      /* parsing the file is very straightforward. We check the record
         type and transition our state based on it:

          CWORD       Add a new word to the word list and make it the
                        current word
          SETTING     If there is no current word, diagnostic and abort
                      Add a new setting to the current word and make
                        it the current setting
          VALUE       If there is no current setting, diagnostic and abort
                      Add a new value to the current setting
          other       Diagnostic and abort
        */
      if (!strncmp (MCHP_WORD_MARKER, line, MCHP_WORD_MARKER_LEN))
        {
          struct mchp_config_specification *spec;

          /* This is a fixed length record. we validate the following:
              - total record length
              - delimiters in the expected locations */
          if (strlen (line) != (MCHP_WORD_MARKER_LEN
                + 24   /* two 8-byte hex value fields */
                + 2)   /* two ':' delimiters */
              || line [MCHP_WORD_MARKER_LEN + 8] != ':'
              || line [MCHP_WORD_MARKER_LEN + 17] != ':')
            {
              warning ("malformed configuration word definition file. bad config word record");
              break;
            }

          spec = xmalloc (sizeof (struct mchp_config_specification));
          spec->next = mchp_configuration_values;

          spec->word = xcalloc (sizeof (struct mchp_config_word), 1);
          spec->word->address = strtoul (line + MCHP_WORD_MARKER_LEN, NULL, 16);
          spec->word->mask = strtoul (line + MCHP_WORD_MARKER_LEN + 9, NULL, 16);
          spec->word->default_value = 
            strtoul (line + MCHP_WORD_MARKER_LEN + 18, NULL, 16);

          /* initialize the value to the default with no bits referenced */
          spec->value = spec->word->default_value;
          spec->referenced_bits = 0;

          mchp_configuration_values = spec;
        }
      else if (!strncmp (MCHP_SETTING_MARKER, line, MCHP_SETTING_MARKER_LEN))
        {
          struct mchp_config_setting *setting;
          size_t len;

          if (!mchp_configuration_values)
            {
              warning ("malformed configuration word definition file. setting record without preceding word record");
              break;
            }

          /* Validate the fixed length portion of the record by checking
             that the record length is >= the size of the minimum valid
             record (empty description and one character name) and that the
             ':' delimiter following the mask is present. */
          if (strlen (line) < (MCHP_SETTING_MARKER_LEN
                + 8     /* 8-byte hex mask field */
                + 2     /* two ':' delimiters */
                + 1)    /* non-empty setting name */
              || line [MCHP_SETTING_MARKER_LEN + 8] != ':')
            {
              warning ("malformed configuration word definition file. bad setting record");
              break;
            }

          setting = xcalloc (sizeof (struct mchp_config_setting), 1);
          setting->next = mchp_configuration_values->word->settings;

          setting->mask = strtoul (line + MCHP_SETTING_MARKER_LEN, NULL, 16);
          len = strcspn (line + MCHP_SETTING_MARKER_LEN + 9, ":");
          /* Validate that the name is not empty */
          if (len == 0)
            {
              warning ("malformed configuration word definition file. bad setting record");
              break;
            }
          setting->name = xmalloc (len + 1);
          strncpy (setting->name, line + MCHP_SETTING_MARKER_LEN + 9, len);
          setting->name [len] = '\0';
          setting->description = 
            xmalloc (strlen (line + MCHP_SETTING_MARKER_LEN + len + 10) + 2);
          strcpy (setting->description, 
              line + MCHP_SETTING_MARKER_LEN + len + 10);

          mchp_configuration_values->word->settings = setting;
        }
      else if (!strncmp (MCHP_VALUE_MARKER, line, MCHP_VALUE_MARKER_LEN))
        {
          struct mchp_config_value *value;
          size_t len;
          if (!mchp_configuration_values
              || !mchp_configuration_values->word->settings)
            {
              warning ("malformed configuration word definition file.");
              break;
            }
          /* Validate the fixed length portion of the record by checking
             that the record length is >= the size of the minimum valid
             record (empty description and one character name) and that the
             ':' delimiter following the mask is present. */
          if (strlen (line) < (MCHP_VALUE_MARKER_LEN
                + 8     /* 8-byte hex mask field */
                + 2     /* two ':' delimiters */
                + 1)    /* non-empty setting name */
              || line [MCHP_VALUE_MARKER_LEN + 8] != ':')
            {
              warning ("malformed configuration word definition file. bad value record");
              break;
            }

          value = xcalloc (sizeof (struct mchp_config_value), 1);
          value->next = mchp_configuration_values->word->settings->values;

          value->value = strtoul (line + MCHP_VALUE_MARKER_LEN, NULL, 16);
          len = strcspn (line + MCHP_VALUE_MARKER_LEN + 9, ":");
          /* Validate that the name is not empty */
          if (len == 0)
            {
              warning ("malformed configuration word definition file. bad setting record");
              break;
            }
          value->name = xmalloc (len + 1);
          strncpy (value->name, line + MCHP_VALUE_MARKER_LEN + 9, len);
          value->name [len] = '\0';
          value->description = 
            xmalloc (strlen (line + MCHP_VALUE_MARKER_LEN + len + 10) + 2);
          strcpy (value->description, 
              line + MCHP_VALUE_MARKER_LEN + len + 10);

          mchp_configuration_values->word->settings->values = value;
        }
      else
        {
          warning ("malformed configuration word definition file.");
          break;
        }
    }
  /* if we didn't exit the loop because of end of file, we have an
     error of some sort. */
  if (!feof (fptr))
    {
      warning ("malformed configuration word definition file.");
      retval = 1;
    }


  fclose (fptr);
  return retval;
}


static void
handle_configuration_setting (const char *name, const char *value_name)
{
  struct mchp_config_specification *spec;

  /* Look up setting in the definitions for the configuration words */
  for (spec = mchp_configuration_values ; spec ; spec = spec->next)
    {
      struct mchp_config_setting *setting;
      for (setting = spec->word->settings ; setting ; setting = setting->next)
        {
          if (strcmp (setting->name, name) == 0)
            {
              struct mchp_config_value *value;

              /* If we've already specified this setting, that's an
                 error, even if the new value and the old value match */
              if (spec->referenced_bits & setting->mask)
                {
                  error ("multiple definitions for configuration setting '%s'",
                      name);
                  return;
                }

              /* look up the value */
              for (value = setting->values ;
                  value ;
                  value = value->next)
                {
                  if (strcmp (value->name, value_name) == 0)
                    {
                      /* mark this setting as having been specified */
                      spec->referenced_bits |= setting->mask;
                      /* update the value of the word with the value
                         indicated */
                      spec->value = (spec->value & ~setting->mask) 
                        | value->value;
                      return;
                    }
                }
              /* If we got here, we didn't match the value name */
              error ("unknown value for configuration setting '%s': '%s'",
                  name, value_name);
              return;
            }
        }
    }
  /* if we got here, we didn't find the setting, which is an error */
  error ("unknown configuration setting: '%s'", name);
}

#define CLEAR_REST_OF_INPUT_LINE() do{int t;tree tv;do{t=c_lex(&tv);}while(t!=CPP_EOF);}while(0);

/* handler function for the config pragma */
void 
mchp_handle_config_pragma (struct cpp_reader *pfile)
{
  int tok;
  tree tok_value;
  static int shown_no_config_warning = 0;

  /* If we're compiling for the default device, we don't process
     configuration words */
  if (!mchp_processor_string)
    {
      error ("#pragma config directive not available for generic device");
      CLEAR_REST_OF_INPUT_LINE();
      return;
    }

  /* the first time we see a config pragma, we need to load the
     configuration word data from the definition file. */
  if (!mchp_configuration_values)
    {
      /* alloc space for the filename: directory + '/' + "configuration.data"
       */
      char *fname = alloca (strlen (mchp_config_data_dir) + 1 +
          strlen (MCHP_CONFIGURATION_DATA_FILENAME));
      strcpy (fname, mchp_config_data_dir);
      if (fname [strlen (fname) - 1] != '/'
          && fname [strlen (fname) - 1] != '\\')
        strcat (fname, "/");
      strcat (fname, MCHP_CONFIGURATION_DATA_FILENAME);
      
      if (load_configuration_definition (fname))
        {
          if (!shown_no_config_warning)
            {
              shown_no_config_warning = 1;
              warning ("configuration word information not available for "
                  "this processor. #pragma config is ignored.");
            }
          CLEAR_REST_OF_INPUT_LINE();
          return;
        }
    }

  /* The payload for the config pragma is a comma delimited list of
     "setting = value" pairs. Both the setting and the value must
     be valid C identifiers. */
  tok = c_lex (&tok_value);
  while (1)
    {
      const cpp_token *raw_token;
      const char *setting_name, *value_name;

      /* the current token should be the setting name */
      if (tok != CPP_NAME)
        {
          error ("configuration setting name expected in configuration pragma");
          break;
        }

      setting_name = IDENTIFIER_POINTER (tok_value);
      /* the next token should be the '=' */
      tok = c_lex (&tok_value);
      if (tok != CPP_EQ)
        {
          error ("'=' expected in configuration pragma");
          break;
        }
      /* now we have the value name. We don't use c_lex() to get this one
         since we don't want the additional interpretation going on there.
         i.e., converting integers from the string. */
      raw_token = cpp_get_token (pfile);

      if (raw_token->type == CPP_NAME)
        {
          value_name = IDENTIFIER_POINTER (
              HT_IDENT_TO_GCC_IDENT (HT_NODE (raw_token->val.node)));

        }
      else if (raw_token->type == CPP_NUMBER)
        {
          value_name = raw_token->val.str.text;
        }
      else
        {
          error ("configuration value name expected in configuration pragma");
          break;
        }

      handle_configuration_setting (setting_name, value_name);

      /* if the next token is ',' then we have another setting. */
      tok = c_lex (&tok_value);
      if (tok == CPP_COMMA)
        tok = c_lex (&tok_value);
      /* if it's EOF, we're done */
      else if (tok == CPP_EOF)
        break;
      /* otherwise, we have spurious input */
      else
        {
          error ("',' or end of line expected in configuration pragma");
          break;
        }
    }
  /* if we ended for any reason other than end of line, we have an error.
     Any needed diagnostic should have already been issued, so just
     clear the rest of the data on the line. */
  if (tok != CPP_EOF)
    CLEAR_REST_OF_INPUT_LINE();
}

void
mchp_output_configuration_words (void)
{
  struct mchp_config_specification *spec;

  for (spec = mchp_configuration_values ; spec ; spec = spec->next)
    {
      /* if there are referenced bits in the word, output its value */
      if (spec->referenced_bits)
        {
          fprintf (asm_out_file, "# Configuration word @ 0x%08x\n", spec->word->address);
          fprintf (asm_out_file, "\t.section\t.config_%08X, \"a\",@progbits\n",
              spec->word->address);
          fprintf (asm_out_file, "\t.type\t__config_%08X, @object\n",
              spec->word->address);
          fprintf (asm_out_file, "\t.size\t__config_%08X, 4\n",
              spec->word->address);
          fprintf (asm_out_file, "__config_%08X:\n", spec->word->address);
          fprintf (asm_out_file, "\t.word\t%u\n", spec->value);
        }
    }
}


