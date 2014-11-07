/*
 *  CCI support for Microchip Compilers
 */

#ifndef CCI
/*
 * General defines....
 *
 */

enum CCI_KIND {
  CCI_unknown,     /* should never happen, I don't like 0 being a valid enum */
  CCI_set_value,   /* execute the following:
                    *
                    *  KEYWORD = N;
                    *
                    * Great for setting flags or options...
                    */
  CCI_define,      /* straight define:   
                    *
                    * #define KEYWORD=CCI_VAL
                    */
  CCI_attribute,   /* attribute define:  
                    *
                    * #define KEYWORD=__attribute__((CCI_VAL)))
                    */
  CCI_attribute_n, /* attribute define:  
                    *
                    * #define KEYWORD(P1,P2,...Pn)= \
                    *   __attribute__((CCI_VAL(P1,P2,...Pn)))
                    *
                    * if P1 (all or none) is specified in the conversion, then
                    *   that will be used otherwise the compiler will generate
                    *   the parenthetical expression after CCI_VAL
                    */
  CCI_attribute_v, /* attribute define:  
                    *
                    * #define KEYWORD(...)=__attribute__((CCI_VAL(__VA_ARGS__)))
                    */
  CCI_attribute_nv, /* attribute define:  
                    *
                    * #define KEYWORD(P1,P2,...Pn,...)= \
                    *   __attribute__((CCI_VAL(P1,P2,...Pn,__VA_ARGS__)))
                    *
                    * if P1 (all or none) is specified in the conversion, then
                    *   that will be used otherwise the compiler will generate
                    *   the parenthetical expression after CCI_VAL
                    */
};

extern void mchp_init_cci(void *pfile_v);
extern void mchp_handle_config_pragma(struct cpp_reader *pfile);

/*
 * #pragma config support
 */

#define MCHP_WORD_MARKER        "CWORD:"
#define MCHP_SETTING_MARKER     "CSETTING:"
#define MCHP_VALUE_MARKER       "CVALUE:"
#define MCHP_WORD_MARKER_LEN    (sizeof (MCHP_WORD_MARKER) - 1)
#define MCHP_SETTING_MARKER_LEN (sizeof (MCHP_SETTING_MARKER) - 1)
#define MCHP_VALUE_MARKER_LEN   (sizeof (MCHP_VALUE_MARKER) - 1)

/* Pretty much arbitrary max line length for the config data file.
   Anything longer than this is either absurd or a bogus file. */
#define MCHP_MAX_CONFIG_LINE_LENGTH 1024

struct mchp_config_value
  {
    char *name;
    char *description;
    unsigned value;
    struct mchp_config_value *next;
  };

struct mchp_config_setting
  {
    char *name;
    char *description;
    unsigned mask;
    struct mchp_config_value *values;
    struct mchp_config_setting *next;
  };

struct mchp_config_word
  {
    unsigned address;
    unsigned mask;
    unsigned default_value;
    struct mchp_config_setting *settings;
  };

struct mchp_config_specification
  {
    struct mchp_config_word *word; /* the definition of the word this value
                                    is referencing */
    unsigned value;           /* the value of the word to put to the device */
    unsigned referenced_bits; /* the bits which have been explicitly specified
                                i.e., have had a setting = value pair in a
                                config pragma */
    struct mchp_config_specification *next;
  };

extern struct mchp_config_specification *mchp_configuration_values;


#else

/* CCI mapping */

/* for use as NULL define */
#define EMPTY 0

/*  TARGET,        CCI_KIND          CCI Keyword,    CCI for C30,     n      */ 
/*  -------------------------------------------------------------------------*/
CCI(TARGET_C30,    CCI_attribute_v,  "__interrupt",  "interrupt",     0)
CCI(TARGET_C32,    CCI_attribute_nv, "__interrupt", \
    "vector(P1), interrupt(__VA_ARGS__), nomips16",                   1) 

CCI(TARGET_MCHP,   CCI_attribute,    "__persistent", "persistent",    0)

CCI(TARGET_C30,    CCI_attribute,    "__near",       "near",          0)
CCI(TARGET_C32,    CCI_define,       "__near",       EMPTY,           0)

CCI(TARGET_MCHP,   CCI_attribute,    "__far",        "far",           0)

CCI(TARGET_MCHP,   CCI_attribute_n,  "__at",         "address",       1)

CCI(TARGET_C30,    CCI_attribute,    "__eeprom",     "space(eedata)", 0)
CCI(TARGET_C32,    CCI_attribute,    "__eeprom",     
    "unsupported(\"__eeprom is unsupported on this platform\")",      0)

CCI(TARGET_C30,    CCI_attribute,    "__xdata",      "space(xmemory)",0)
CCI(TARGET_C30,    CCI_attribute,    "__ydata",      "space(ymemory)",0)
CCI(TARGET_C32,    CCI_define,       "__xdata",      EMPTY,           0)
CCI(TARGET_C32,    CCI_define,       "__ydata",      EMPTY,           0)
CCI(TARGET_MCHP,   CCI_define,       "__bank(X)",    EMPTY,           0)

CCI(TARGET_C30,    CCI_define,       "__abi(...)",   EMPTY,           0)
CCI(TARGET_C32,    CCI_attribute_n,  "__abi",        "P1",            1)

CCI(TARGET_MCHP,   CCI_attribute_n,  "__align",      "aligned",       1)

CCI(TARGET_MCHP,   CCI_attribute,    "__deprecate",  "deprecated",    0)

CCI(TARGET_MCHP,   CCI_attribute,    "__pack",       "packed",        0)

/* __packed is already defined for C30 */
CCI(TARGET_C32,    CCI_define,       "__packed",     EMPTY,           0)

CCI(TARGET_MCHP,   CCI_attribute_n,  "__section",    "section",       1)

CCI(TARGET_MCHP,   CCI_define,       "__CCI__",      "1",             0)

#undef CCI
#endif
