/* Subroutines used for Microchip PIC32 MCU support
   Copyright (C) 2017
   Microchip Technology, Inc.

This file is part of Microchip XC compiler tool-chain.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/*
 *  Common CCI definitions for Microchip Compilers
 */

#ifdef _BUILD_MCHP_

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "varasm.h"
#include "output.h"
#include "tree-core.h"
#include "tree.h"
#include "memmodel.h"
#include "gimple.h"
#include "cfghooks.h"
#include "df.h"
#include "tm_p.h"
#include "stringpool.h"
#include "cgraph.h"
#include "alias.h"
#include "fold-const.h"
#include "stor-layout.h"
#include "calls.h"
#include "explow.h"
#include "libfuncs.h"
#include "common/common-target.h"
#include "langhooks.h"
#include "cfgrtl.h"
#include "cfganal.h"
#include "sched-int.h"
#include "gimplify.h"
#include "tree-pass.h"
#include "context.h"
#include "target-globals.h"
#include "builtins.h"
#include "c-family/c-common.h"
#include "c-family/c-pragma.h"
#include "c/c-tree.h"
#include "diagnostic.h"

#define CCI_H "config/mchp-cci/cci.h"
#include CCI_H
#include "config/mchp-cci/cci-pragmas.h"

/*
 * #pragma config stuff
 */

#ifndef MCHP_CONFIGURATION_HEADER_SIZE
#error Please define MCHP_CONFIGURATION_HEADER_SIZE
#endif

#ifndef MCHP_CONFIGURATION_HEADER_MARKER
#error Please define MCHP_CONFIGURATION_HEADER_MARKER
#endif

#ifndef MCHP_CONFIGURATION_HEADER_VERSION
#error Please define MCHP_CONFIGURATION_HEADER_VERSION
#endif

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
#define GET_LINE get_line

#ifndef CLEAR_REST_OF_INPUT_LINE
#define CLEAR_REST_OF_INPUT_LINE() \
  do {                             \
       int t;                      \
       tree tv;                    \
       do {                        \
         t=pragma_lex(&tv);        \
       } while (t!=CPP_EOF);       \
      } while(0);
#endif

/* Verify the header record for the configuration data file
 */
static int
verify_configuration_header_record(FILE *fptr)
{
  char header_record[MCHP_CONFIGURATION_HEADER_SIZE + 1];
  /* the first record of the file is a string identifying
     file and its format version number. */
  if (get_line (header_record, MCHP_CONFIGURATION_HEADER_SIZE + 1, fptr)
      == NULL)
    {
      warning (0, "Malformed configuration word definition file (header).");
      return 1;
    }
  /* verify that this file is a daytona configuration word file */
  if (strncmp (header_record, MCHP_CONFIGURATION_HEADER_MARKER,
               sizeof (MCHP_CONFIGURATION_HEADER_MARKER) - 1) != 0)
    {
      warning (0, "Malformed configuration word definition file (marker).");
      return 1;
    }

  /* verify that the version number is one we can deal with */
  if (strncmp (header_record + sizeof (MCHP_CONFIGURATION_HEADER_MARKER) - 1,
               MCHP_CONFIGURATION_HEADER_VERSION,
               sizeof (MCHP_CONFIGURATION_HEADER_VERSION) - 1))
    {
      warning (0, "Configuration word definition file version mismatch (version).");
      return 1;
    }
  return 0;
}

/* Load the configuration word definitions from the data file
 */
static int
mchp_load_configuration_definition(const char *fname)
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
              warning (0, "Malformed configuration word definition file. "
                          "Bad config word record");
              break;
            }

          spec = (struct mchp_config_specification *)
                 xmalloc (sizeof (struct mchp_config_specification));
          spec->next = mchp_configuration_values;

          spec->word = (struct mchp_config_word *)
                       xcalloc (sizeof (struct mchp_config_word), 1);
          spec->word->address = strtoul (line + MCHP_WORD_MARKER_LEN, NULL, 16);
          spec->word->mask = strtoul(line + MCHP_WORD_MARKER_LEN + 9, NULL, 16);
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
              warning (0, "Malformed configuration word definition file. "
                          "Setting record without preceding word record");
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
              warning (0, "Malformed configuration word definition file. "
                          "Bad setting record");
              break;
            }

          setting = (struct mchp_config_setting *)
                    xcalloc (sizeof (struct mchp_config_setting), 1);
          setting->next = mchp_configuration_values->word->settings;

          setting->mask = strtoul (line + MCHP_SETTING_MARKER_LEN, NULL, 16);
          len = strcspn (line + MCHP_SETTING_MARKER_LEN + 9, ":");
          /* Validate that the name is not empty */
          if (len == 0)
            {
              warning (0, "Malformed configuration word definition file. "
                          "Bad setting record");
              break;
            }
          setting->name = (char*)xmalloc (len + 1);
          strncpy (setting->name, line + MCHP_SETTING_MARKER_LEN + 9, len);
          setting->name [len] = '\0';
          setting->description =
            (char*)xmalloc(strlen(line + MCHP_SETTING_MARKER_LEN + len + 10)+2);
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
              warning (0, "Malformed configuration word definition file.");
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
              warning (0, "Malformed configuration word definition file. "
                          "Bad value record");
              break;
            }

          value = (struct mchp_config_value *)
                  xcalloc (sizeof (struct mchp_config_value), 1);
          value->next = mchp_configuration_values->word->settings->values;

          value->value = strtoul (line + MCHP_VALUE_MARKER_LEN, NULL, 16);
          len = strcspn (line + MCHP_VALUE_MARKER_LEN + 9, ":");
          /* Validate that the name is not empty */
          if (len == 0)
            {
              warning (0, "Malformed configuration word definition file. "
                          "Bad setting record");
              break;
            }
          value->name = (char*)xmalloc (len + 1);
          strncpy (value->name, line + MCHP_VALUE_MARKER_LEN + 9, len);
          value->name [len] = '\0';
          value->description =
            (char*)xmalloc (strlen (line + MCHP_VALUE_MARKER_LEN + len + 10)+2);
          strcpy (value->description,
                  line + MCHP_VALUE_MARKER_LEN + len + 10);

          mchp_configuration_values->word->settings->values = value;
        }
      else
        {
          warning (0, "Malformed configuration word definition file.");
          break;
        }
    }
  /* if we didn't exit the loop because of end of file, we have an
     error of some sort. */
  if (!feof (fptr))
    {
      warning (0, "Malformed configuration word definition file.");
      retval = 1;
    }

  /* This is the bit for handling aliasing. The linked list is stored in
     reverse order so we have to keep track of the previous setting since the 
     alias are after the original in the linked list.*/

  struct mchp_config_specification *spec;
  for (spec = mchp_configuration_values; spec ; spec = spec ->next) {
    struct mchp_config_setting *setting;  
    struct mchp_config_setting *setting_prev;  
    for (setting = spec->word->settings; setting; setting = setting->next) {
      if (setting->values) {
         setting_prev = setting;
      } else if (setting_prev) {
         setting->values = setting_prev->values;
      }
    }   
  }

  fclose (fptr);
  return retval;
}

static void
mchp_handle_configuration_setting (const char *name,
                                   const unsigned char *value_name)
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
                                    if (strcmp (value->name, (const char*)value_name) == 0)
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
                            
                            /* Handle SET/CLEAR -> 1/0 */
                            if (strcmp ("SET", (const char*)value_name) == 0 ||
                                strcmp ("CLEAR", (const char*)value_name) == 0)
                              {
                                int shift;
                                unsigned int mask;
                                unsigned int converted_value;
                                mask = setting->mask;
                                shift = 0;
                                while((0x1 & mask) != 1)
                                  {
                                    shift++;
                                    mask >>= 1;
                                  }
                                converted_value = (value_name[0] == 'S') ? mask : 0;
                                /* mark this setting as having been specified */
                                spec->referenced_bits |= setting->mask;
                                /* update the value of the word with the value
                                   indicated */
                                spec->value = (spec->value & ~setting->mask)
                                              | ((setting->mask) & (converted_value << shift));
                                return;
                              }
#if defined(MCHP_ALLOW_INTEGER_CONFIGVALUE)
                            char* current = NULL;
                            bool all_digits = TRUE;
                            all_digits = TRUE;
                            current = (char *)value_name;
                            do
                                {
                                    if (!ISDIGIT(*current))
                                        all_digits = FALSE;
                                    current++;
                                }
                            while(*current!=NULL);
                            if (all_digits)
                                {
                                    int shift;
                                    unsigned int mask;
                                    unsigned int converted_value;
                                    mask = setting->mask;
                                    shift = 0;
                                    while((0x1 & mask) != 1)
                                        {
                                            shift++;
                                            mask >>= 1;
                                        }
                                    converted_value = strtoul((char *)value_name, NULL, 10);
                                    if (((setting->mask)>>shift & converted_value) != converted_value)
                                        warning (0, "Configuration value 0x%x masked to 0x%x for setting %qs",
                                                 converted_value, (setting->mask)>>shift, name);
                                    /* mark this setting as having been specified */
                                    spec->referenced_bits |= setting->mask;
                                    /* update the value of the word with the value
                                       indicated */
                                    spec->value = (spec->value & ~setting->mask)
                                                  | (setting->mask) & (strtoul((char *)value_name, NULL, 10) << shift);
                                    return;
                                }
#endif /* MCHP_ALLOW_INTEGER_CONFIGVALUE */
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


/* handler function for the config pragma */
void
mchp_handle_config_pragma (struct cpp_reader *pfile)
{
  enum cpp_ttype tok;
  tree tok_value;
  static int shown_no_config_warning = 0;
  
  if (flag_generate_lto)
    {
      warning (0, "#pragma config not supported with -flto option, this file will not participate in LTO");
      flag_generate_lto = 0;
    }


  if (!mchp_config_data_file)
    {
      error ("Configuration-word data file not specified "
             "but required for #pragma config directive; has a valid device "
             "been selected?");
      CLEAR_REST_OF_INPUT_LINE();
      return;
    }

  /* the first time we see a config pragma, we need to load the
     configuration word data from the definition file. */
  if (!mchp_configuration_values)
    {
      if (mchp_load_configuration_definition (mchp_config_data_file))
        {
          if (!shown_no_config_warning)
            {
              shown_no_config_warning = 1;
              warning (0, "Configuration word information not available for "
                       "this processor. #pragma config is ignored.");
            }
          CLEAR_REST_OF_INPUT_LINE();
          return;
        }
    }

  /* The payload for the config pragma is a comma delimited list of
     "setting = value" pairs. Both the setting and the value must
     be valid C identifiers. */
  tok = pragma_lex (&tok_value);
  while (1)
    {
      const cpp_token *raw_token;
      const char *setting_name;
      unsigned char *value_name;

      /* the current token should be the setting name */
      if (tok != CPP_NAME)
        {
          error ("configuration setting name expected in configuration pragma");
          break;
        }

      setting_name = IDENTIFIER_POINTER (tok_value);
      /* the next token should be the '=' */
      tok = pragma_lex (&tok_value);
      if (tok != CPP_EQ)
        {
          error ("'=' expected in configuration pragma");
          break;
        }
      /* now we have the value name. We don't use pragma_lex() to get this one
         since we don't want the additional interpretation going on there.
         i.e., converting integers from the string. */
      tok = pragma_lex (&tok_value);
      if (tok == CPP_NAME)
        value_name = (unsigned char *)IDENTIFIER_POINTER (tok_value);
      else if (tok == CPP_NUMBER)
        {
          if (tree_fits_uhwi_p (tok_value))
          {
            #define MAX_VALUE_NAME_LENGTH 22
            HOST_WIDE_INT i;
            i = tree_to_uhwi (tok_value);
            value_name = (unsigned char*)xcalloc(MAX_VALUE_NAME_LENGTH,1);
            snprintf((char *)value_name, MAX_VALUE_NAME_LENGTH, "%u", i);
            #undef MAX_VALUE_NAME_LENGTH
          }
          else
            {
              error ("config-setting value must be a valid integer constant");
              break;
            }
        }
      else
        {
          error ("config-setting value must be a name or a constant integer");
          break;
        }
      mchp_handle_configuration_setting (setting_name, value_name);

      /* if the next token is ',' then we have another setting. */
      tok = pragma_lex (&tok_value);
      if (tok == CPP_COMMA)
        tok = pragma_lex (&tok_value);
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

#endif /* _BUILD_MCHP_ */
