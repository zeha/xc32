/*
 *  Common CCI definitions for Microchip Compilers
 */

#ifdef _BUILD_MCHP_

#include "c-common.h"

#define CCI_H "config/mchp-cci/cci.h"
#include CCI_H

#ifdef _BUILD_C30_
#define TARGET_C30 1
#else
#define TARGET_C30 0
#endif
#ifdef _BUILD_C32_
#define TARGET_C32 1
#else
#define TARGET_C32 0
#endif
#define TARGET_MCHP 1

static void cci_define(void *pfile_v, const char *keyword, const char *target) {
  struct cpp_reader *pfile = (struct cpp_reader *)pfile_v;
  char *buffer;

  if (target) 
    {
      buffer = xmalloc(strlen(keyword) + strlen(target) + 6);
      sprintf(buffer,"%s=\"%s\"", keyword, target);
    }
  else
    {
      buffer = xmalloc(strlen(keyword) + strlen("=") + 6);
      sprintf(buffer,"%s=", keyword);
    }
  cpp_define(pfile, buffer);
  return;
}

static void cci_attribute(void *pfile_v,const char *keyword, const char *target,
                   int varargs, int n) {
  struct cpp_reader *pfile = (struct cpp_reader *)pfile_v;
  int params_specified = 0;
  char *buffer,*c;
  int size;
  int i;

  if (n) {
    for (c = target; *c; c++) {
      if ((*c == 'P') && (c[1] >= '0') && (c[1] <= '9')) {
        params_specified=1;
        break;
      }
    }
  }
  size = strlen(keyword)+sizeof("__attribute__(())")+strlen(target)+1;
  if (n) {
    if (params_specified == 0) {
      size += 8 * n;  /* up to 99 params: Pnn, */
    }
  }
  if (varargs) size += strlen("=()(),...__VA_ARGS__");
  buffer = xmalloc(size);
  c = buffer;
  c += sprintf(c,"%s",keyword);
  if (n || varargs) {
    *c++ = '(';
    for (i = 1; i <= n; i++) {
      c += sprintf(c, "P%d", i);
      if (i < n) *c += ',';
    }
    if (varargs) {
      if (n) *c++=',';
      c += sprintf(c, "...");
    }
    *c++ = ')';
  }
  c += sprintf(c, "=__attribute__((%s", target);
  if ((n || varargs) && !params_specified) {
    *c++ = '(';
    for (i = 1; i <= n; i++) {
      c += sprintf(c, "P%d", i);
      if (i < n) *c += ',';
    }
    if (varargs) {
      if (n) *c++=',';
      c += sprintf(c, "__VA_ARGS__");
    }
    *c++ = ')';
  }
  *c++ = ')';
  *c++ = ')';
  *c++ = 0;

  cpp_define(pfile, buffer);
  /* can we free buffer? */
#if 0
  free(buffer);
#endif
}

/*
 * call from TARGET_CPU_CPP_BUILTINS
 */

void mchp_init_cci(void *pfile_v) {
  struct cpp_reader *pfile = (struct cpp_reader *)pfile_v;

#define CCI(TARGET, CCI_KIND, CCI_KEYWORD, TGT_KEYWORD, N) \
  if (TARGET && CCI_KIND == CCI_define) \
    cci_define(pfile_v, CCI_KEYWORD, TGT_KEYWORD);
#include CCI_H
#ifdef CCI
#error CCI is still defined...
#endif

#define CCI(TARGET, CCI_KIND, CCI_KEYWORD, TGT_KEYWORD, N) \
  if (TARGET && CCI_KIND == CCI_attribute) \
    cci_attribute(pfile_v, CCI_KEYWORD, TGT_KEYWORD, 0, 0);
#include CCI_H

#define CCI(TARGET, CCI_KIND, CCI_KEYWORD, TGT_KEYWORD, N) \
  if (TARGET && CCI_KIND == CCI_attribute_n) \
    cci_attribute(pfile_v, CCI_KEYWORD, TGT_KEYWORD, 0, N);
#include CCI_H

#define CCI(TARGET, CCI_KIND, CCI_KEYWORD, TGT_KEYWORD, N) \
  if (TARGET && CCI_KIND == CCI_attribute_v) \
    cci_attribute(pfile_v, CCI_KEYWORD, TGT_KEYWORD, 1, 0);
#include CCI_H

#define CCI(TARGET, CCI_KIND, CCI_KEYWORD, TGT_KEYWORD, N) \
  if (TARGET && CCI_KIND == CCI_attribute_nv) \
    cci_attribute(pfile_v, CCI_KEYWORD, TGT_KEYWORD, 1, N);
#include CCI_H

#define CCI(TARGET, CCI_KIND, CCI_KEYWORD, TGT_KEYWORD, N) \
  if (TARGET && CCI_KIND == CCI_set_value) \
    TGT_KEYWORD = N;
}

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
      warning (0, "Malformed configuration word definition file.");
      return 1;
    }
  /* verify that this file is a daytona configuration word file */
  if (strncmp (header_record, MCHP_CONFIGURATION_HEADER_MARKER,
               sizeof (MCHP_CONFIGURATION_HEADER_MARKER) - 1) != 0)
    {
      warning (0, "Malformed configuration word definition file.");
      return 1;
    }
  
  /* verify that the version number is one we can deal with */
  if (strncmp (header_record + sizeof (MCHP_CONFIGURATION_HEADER_MARKER) - 1,
               MCHP_CONFIGURATION_HEADER_VERSION,
               sizeof (MCHP_CONFIGURATION_HEADER_VERSION) - 1))
    {
      warning (0, "Configuration word definition file version mismatch.");
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

  /* If we're compiling for the default device, we don't process
     configuration words */
  if (!mchp_processor_string)
    {
      error ("#pragma config directive not available for the "
             "default generic device, %s", mchp_processor_string);
      CLEAR_REST_OF_INPUT_LINE();
      return;
    }

  if (!mchp_config_data_dir)
    {
      error ("Configuration-word data directory not specified "
             "but required for #pragma config directive");
      CLEAR_REST_OF_INPUT_LINE();
      return;
    }

  /* the first time we see a config pragma, we need to load the
     configuration word data from the definition file. */
  if (!mchp_configuration_values)
    {
      /* alloc space for the filename: directory + '/' + "configuration.data"
       */
      char *fname = (char*)alloca (strlen (mchp_config_data_dir) + 1 +
                            strlen (MCHP_CONFIGURATION_DATA_FILENAME));
      strcpy (fname, mchp_config_data_dir);
      if (fname [strlen (fname) - 1] != '/'
          && fname [strlen (fname) - 1] != '\\')
        strcat (fname, "/");
      strcat (fname, MCHP_CONFIGURATION_DATA_FILENAME);

      if (mchp_load_configuration_definition (fname))
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
        value_name = IDENTIFIER_POINTER (tok_value);
      else if (tok == CPP_NUMBER)
        {
          if (host_integerp (tok_value, 1 /* positive only */ ))
          {
            #define MAX_VALUE_NAME_LENGTH 22
            HOST_WIDE_INT i;
            i = tree_low_cst (tok_value, 1 /* positive only */ );
            value_name = (unsigned char*)xcalloc(MAX_VALUE_NAME_LENGTH,1);
            snprintf(value_name, MAX_VALUE_NAME_LENGTH, "%d", i);
            #undef MAX_VALUE_NAME_LENGTH
          }
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

#endif


