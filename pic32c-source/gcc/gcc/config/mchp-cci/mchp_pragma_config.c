#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include "diagnostic.h"
#include "tree.h"
#include "stringpool.h"
#include "cgraph.h"

#include "mchp_pragma_config.h"

/*  Configuration specificiation data used by cci */
struct mchp_config_specification *mchp_configuration_values = NULL;

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
  while (buf[strlen (buf) - 1] == '\n' || buf[strlen (buf) - 1] == '\r')
    buf[strlen (buf) - 1] = '\0';
  return buf;
}
#define GET_LINE get_line

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
      warning (0, "Malformed configuration word definition file.");
      return 1;
    }
  /* verify that this file is a daytona configuration word file */
  if (strncmp (header_record, MCHP_CONFIGURATION_HEADER_MARKER,
               sizeof (MCHP_CONFIGURATION_HEADER_MARKER) - 1)
      != 0)
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
int
mchp_load_configuration_definition (const char *fname)
{
  int retval = 0;
  FILE *fptr;
  char line[MCHP_MAX_CONFIG_LINE_LENGTH];

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
          if (strlen (line)
                  != (MCHP_WORD_MARKER_LEN
                      + 24 /* two 8-byte hex value fields */
                      + 2) /* two ':' delimiters */
              || line[MCHP_WORD_MARKER_LEN + 8] != ':'
              || line[MCHP_WORD_MARKER_LEN + 17] != ':')
            {
              warning (0, "Malformed configuration word definition file. "
                          "Bad config word record");
              break;
            }

          spec = (struct mchp_config_specification *)xmalloc (
              sizeof (struct mchp_config_specification));
          spec->next = mchp_configuration_values;

          spec->word = (struct mchp_config_word *)xcalloc (
              sizeof (struct mchp_config_word), 1);
          spec->word->address
              = strtoul (line + MCHP_WORD_MARKER_LEN, NULL, 16);
          spec->word->mask
              = strtoul (line + MCHP_WORD_MARKER_LEN + 9, NULL, 16);
          spec->word->default_value
              = strtoul (line + MCHP_WORD_MARKER_LEN + 18, NULL, 16);

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
          if (strlen (line)
                  < (MCHP_SETTING_MARKER_LEN + 8 /* 8-byte hex mask field */
                     + 2                         /* two ':' delimiters */
                     + 1)                        /* non-empty setting name */
              || line[MCHP_SETTING_MARKER_LEN + 8] != ':')
            {
              warning (0, "Malformed configuration word definition file. "
                          "Bad setting record");
              break;
            }

          setting = (struct mchp_config_setting *)xcalloc (
              sizeof (struct mchp_config_setting), 1);
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
          setting->name = (char *)xmalloc (len + 1);
          strncpy (setting->name, line + MCHP_SETTING_MARKER_LEN + 9, len);
          setting->name[len] = '\0';
          setting->description = (char *)xmalloc (
              strlen (line + MCHP_SETTING_MARKER_LEN + len + 10) + 2);
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
          if (strlen (line)
                  < (MCHP_VALUE_MARKER_LEN + 8 /* 8-byte hex mask field */
                     + 2                       /* two ':' delimiters */
                     + 1)                      /* non-empty setting name */
              || line[MCHP_VALUE_MARKER_LEN + 8] != ':')
            {
              warning (0, "Malformed configuration word definition file. "
                          "Bad value record");
              break;
            }

          value = (struct mchp_config_value *)xcalloc (
              sizeof (struct mchp_config_value), 1);
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
          value->name = (char *)xmalloc (len + 1);
          strncpy (value->name, line + MCHP_VALUE_MARKER_LEN + 9, len);
          value->name[len] = '\0';
          value->description = (char *)xmalloc (
              strlen (line + MCHP_VALUE_MARKER_LEN + len + 10) + 2);
          strcpy (value->description, line + MCHP_VALUE_MARKER_LEN + len + 10);

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
  for (spec = mchp_configuration_values; spec; spec = spec->next)
    {
      struct mchp_config_setting *setting;
      struct mchp_config_setting *setting_prev;
      for (setting = spec->word->settings; setting; setting = setting->next)
        {
          if (setting->values)
            {
              setting_prev = setting;
            }
          else if (setting_prev)
            {
              setting->values = setting_prev->values;
            }
        }
    }

  fclose (fptr);
  return retval;
}

void
mchp_handle_configuration_setting (const char *name,
                                   const unsigned char *value_name)
{
  struct mchp_config_specification *spec;

  /* Look up setting in the definitions for the configuration words */
  for (spec = mchp_configuration_values; spec; spec = spec->next)
    {
      struct mchp_config_setting *setting;
      for (setting = spec->word->settings; setting; setting = setting->next)
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
              for (value = setting->values; value; value = value->next)
                {
                  if (strcmp (value->name, (const char *)value_name) == 0)
                    {
                      /* mark this setting as having been specified */
                      spec->referenced_bits |= setting->mask;
                      /* update the value of the word with the value
                         indicated */
                      spec->value
                          = (spec->value & ~setting->mask) | value->value;
                      return;
                    }
                }

              /* Handle SET/CLEAR -> 1/0 */
              if (strcmp ("SET", (const char *)value_name) == 0
                  || strcmp ("CLEAR", (const char *)value_name) == 0)
                {
                  int shift;
                  unsigned int mask;
                  unsigned int converted_value;
                  mask = setting->mask;
                  shift = 0;
                  while ((0x1 & mask) != 1)
                    {
                      shift++;
                      mask >>= 1;
                    }
                  converted_value = (value_name[0] == 'S') ? mask : 0;
                  /* mark this setting as having been specified */
                  spec->referenced_bits |= setting->mask;
                  /* update the value of the word with the value
                     indicated */
                  spec->value
                      = (spec->value & ~setting->mask)
                        | ((setting->mask) & (converted_value << shift));
                  return;
                }
#if defined(MCHP_ALLOW_INTEGER_CONFIGVALUE)
              const unsigned char *current = NULL;
              bool all_digits = TRUE;
              all_digits = TRUE;
              current = value_name;
              do
                {
                  if (!ISDIGIT (*current))
                    all_digits = FALSE;
                  current++;
                }
              while (*current);
              if (all_digits)
                {
                  int shift;
                  unsigned int mask;
                  unsigned int converted_value;
                  mask = setting->mask;
                  shift = 0;
                  while ((0x1 & mask) != 1)
                    {
                      shift++;
                      mask >>= 1;
                    }
                  converted_value
                      = strtoul ((const char *)value_name, NULL, 10);
                  if (((setting->mask) >> shift & converted_value)
                      != converted_value)
                    warning (0,
                             "Configuration value 0x%x masked to 0x%x for "
                             "setting %qs",
                             converted_value, (setting->mask) >> shift, name);
                  /* mark this setting as having been specified */
                  spec->referenced_bits |= setting->mask;
                  /* update the value of the word with the value
                     indicated */
                  spec->value
                      = (spec->value & ~setting->mask)
                        | ((setting->mask)
                           & (strtoul ((const char *)value_name, NULL, 10)
                              << shift));
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

/* Verify that all the settings in a configuration word have been
   specified. */
static bool
pic32c_verify_configuration_word (struct mchp_config_word *word,
                                  unsigned referenced_bits)
{
  struct mchp_config_setting *setting;
  unsigned mask = 0;

  for (setting = word->settings; setting != NULL; setting = setting->next)
    {
      mask |= setting->mask;
    }

  return (mask == referenced_bits);
}

/* Check that the aggregration of settings for a configuration word is
   valid. */
static void
pic32c_check_configuration_word (struct mchp_config_word *word,
                                 unsigned referenced_bits)
{
#if defined(TARGET_IS_PIC32C)
  if (mchp_allow_partial_config_words)
    return;
#else
  return;
#endif

  if (!pic32c_verify_configuration_word (word, referenced_bits))
    {
      /* TODO: Can we make the location information more useful, or
         ignore line/column information? */
      error ("config word for address 0x%x not fully specified",
             word->address);
    }
}

static void
pragma_config2var_decl (unsigned int address, unsigned int value)
{

  static char buff[64];

  sprintf (buff, "__config_%08X", address);
  tree pconfig_var = build_decl (UNKNOWN_LOCATION, VAR_DECL,
                                 get_identifier (buff), uint32_type_node);

  sprintf (buff, ".config_%08X", address);
  set_decl_section_name (pconfig_var, buff);

  DECL_ARTIFICIAL (pconfig_var) = 1;
  DECL_PRESERVE_P (pconfig_var) = 1;
  DECL_USER_ALIGN (pconfig_var) = 1;
  DECL_IGNORED_P (pconfig_var) = 1;
  TREE_READONLY (pconfig_var) = 1;
  TREE_PUBLIC (pconfig_var) = 1;
  TREE_STATIC (pconfig_var) = 1;
  TREE_USED (pconfig_var) = 1;
  SET_DECL_ALIGN (pconfig_var, BITS_PER_UNIT);

  DECL_INITIAL (pconfig_var) = build_int_cst (uint32_type_node, value);

  tree address_attrib = build_tree_list (
      get_identifier ("address"),
      build_tree_list (NULL, build_int_cst (uint32_type_node, address)));
  tree keep_attrib = build_tree_list (get_identifier ("keep"), NULL_TREE);
  tree attrib = chainon (address_attrib, keep_attrib);
  DECL_ATTRIBUTES (pconfig_var) = attrib;

  varpool_node::finalize_decl (pconfig_var);
}

void
mchp_output_configuration_words (void)
{
  struct mchp_config_specification *spec;

  if (mchp_configuration_values)
    {
      for (spec = mchp_configuration_values; spec; spec = spec->next)
        {
          /* if there are referenced bits in the word, output its value */
          if (spec->referenced_bits)
            {
              pic32c_check_configuration_word (spec->word,
                                               spec->referenced_bits);
              pragma_config2var_decl (spec->word->address, spec->value);
            }
        }
    }
}
