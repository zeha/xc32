/* Definitions of target machine PIC32 for GNU compiler.
   Copyright (C) 1994, 1997, 1999, 2000, 2002, 2003, 2004
   Free Software Foundation, Inc.

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


/* Macro for conditional compilation of PIC32 only stuff */
#define TARGET_MCHP_PIC32MX 1
#define MCHP_DEBUG 1
extern const char *mchp_io_size;
extern       int   mchp_io_size_val;
extern const char *mchp_legacy_libc;
extern       int   mchp_legacy_libc_val;
extern const char *mchp_it_transport;

#undef DEFAULT_SIGNED_CHAR
#define DEFAULT_SIGNED_CHAR 1

#undef TARGET_SHORT_DOUBLE
#define TARGET_SHORT_DOUBLE  1

/*
** This is how to output a reference to a user-level label named NAME.
** `assemble_name' uses this.
*/
#if 0
#undef ASM_OUTPUT_LABELREF
#define ASM_OUTPUT_LABELREF(FILE, NAME) \
do {                     \
  char * real_name;               \
  real_name = mchp_strip_name_encoding ((NAME));   \
  asm_fprintf (FILE, "%U%s", real_name);      \
} while (0)
#endif

/* Put at the end of the command given to the linker if -nodefaultlibs or
 * -nostdlib is not specified on the command line. This includes all the
 * standard libraries, the peripheral libraries if the -mno-peripheral-libs
 * option is not specified on the command line, and the processor-specific
 * peripheral library if -mno-peripheral-libs option is not specified, but
 * the -mprocessor option is specified.
 */
#undef  LIB_SPEC
#define LIB_SPEC "--start-group -ldebug \
  %{mlegacy-libc:%{!mno-legacy-libc:-llega-c}} %{mno-legacy-libc:-lc} \
  %{!mlegacy-libc:-lc} \
 -lm -le -ldsp %{!mno-peripheral-libs:-lmchp_peripheral %{mprocessor=*:-lmchp_peripheral_%*}} --end-group"


/* Don't set.  This defaults to crt0.o if not specified. */
#undef  STARTFILE_SPEC

/* Don't set.  This defaults to an empty string if not specified. */
#undef  ENDFILE_SPEC

/* Added on the linker command line after all user-specified -L options are
 * included.  This will add all the standard -L search paths, the
 * processor-specific library search path, and define _DEBUGGER if the
 * -mdebugger command-line option was specified.
 */
#define LINK_LIBGCC_SPEC "%D -L %s%{mprocessor=*:./proc/%*; :./proc/32MXGENERIC} %{mdebugger:--defsym _DEBUGGER=1}"

/* These are MIPS-specific specs that we do not utilize.  Undefine them
 * and define them as an empty string.
 */
#undef ENDIAN_SPEC
#define ENDIAN_SPEC ""

#undef ASM_ABI_DEFAULT_SPEC
#define ASM_ABI_DEFAULT_SPEC ""

/* Supports configure-time default options (i.e., '--with' options) in the
 * driver.  We don't have any options that are configurable at this time.
 */
#undef OPTION_DEFAULT_SPECS

/* The default MIPS CPU is pic32mx. */
#undef MIPS_CPU_STRING_DEFAULT
#define MIPS_CPU_STRING_DEFAULT "pic32mx"

/* Override the LINK_SPEC specified in mips.h since we removed a number
 * of the options utilized in that spec.
 */
#undef LINK_SPEC
#define LINK_SPEC "\
%{G*} %{bestGnum} %{shared} %{non_shared} \
%{mno-smart-io:--no-smart-io} %{msmart-io=0:--no-smart-io}"

/* Override the GAS_ASM_SPEC specified in mips.h since we removed the mtune
 * option utilized in that spec.
 */
#undef GAS_ASM_SPEC
#define GAS_ASM_SPEC "%{v}"

/* Override the ASM_SPEC specified in mips.h since we removed a number
 * of the options utilized in that spec.
 */
#undef ASM_SPEC
#define ASM_SPEC "\
%{G*} \
%{mips16:%{!mno-mips16:-mips16}} %{mno-mips16:-no-mips16} \
%{mips16e} \
%(subtarget_asm_optimizing_spec) \
%(subtarget_asm_debugging_spec) \
%{mxgot:-xgot} \
%(target_asm_spec) \
%(subtarget_asm_spec)"

/* SUBTARGET_CC1_SPEC is passed to the compiler proper.  It may be
   overridden by subtargets.  */
#ifndef SUBTARGET_CC1_SPEC
#define SUBTARGET_CC1_SPEC ""
#endif

/* CC1_SPEC is the set of arguments to pass to the compiler proper.  This
 * was copied from the one in mips.h, but that one had some problems and
 * contained the endian-selection options.
 */
#undef CC1_SPEC
#define CC1_SPEC "\
%{mips16*:%{!fno-section-relative-addressing:-fsection-relative-addressing -fno-common}} \
%{mips16*:-mcode-xonly} \
%{fsection-relative-addressing:-fno-common} \
%{G*} \
-mconfig-data-dir= %J%s%{mprocessor=*:./proc/%*; :./proc/32MXGENERIC} \
%{mno-float:-fno-builtin-fabs -fno-builtin-fabsf} \
%{mno-smart-io:-msmart-io=0} \
%{msmart-io:-msmart-io=1} \
%{mno-legacy-libc:-mlegacy-libc=0} \
%{mlegacy-libc:-mlegacy-libc=1} \
%{!Wno-implicit-function-declaration:-Wimplicit-function-declaration} \
%(subtarget_cc1_spec)"

/* Preprocessor specs.  */

/* SUBTARGET_CPP_SPEC is passed to the preprocessor.  It may be
   overridden by subtargets.  */
#ifndef SUBTARGET_CPP_SPEC
#define SUBTARGET_CPP_SPEC ""
#endif

#undef CPP_SPEC
#define CPP_SPEC "%(subtarget_cpp_spec)\
%{mappio-debug:-D__APPIO_DEBUG} \
"

/* None of the OPTIONS specified in MULTILIB_OPTIONS are set by default. */
#undef MULTILIB_DEFAULTS


/* define PATH to be used if C_INCLUDE_PATH is not declared
   (and CPLUS_INCLUDE_PATH for C++, &c).  The directories are all relative
   to the current executable's directory */
extern char * mchp_default_include_path(void) __attribute__((weak));
#define DEFAULT_INCLUDE_PATH (mchp_default_include_path)
#ifndef TARGET_EXTRA_INCLUDES
extern void mchp_system_include_paths(const char *root, const char *system,
                                      int nostdinc);

#define TARGET_EXTRA_INCLUDES mchp_system_include_paths
#endif

/* By default, the GCC_EXEC_PREFIX_ENV prefix is "GCC_EXEC_PREFIX", however
   in a cross compiler, another environment variable might want to be used
   to avoid conflicts with the host any host GCC_EXEC_PREFIX */
#ifndef GCC_EXEC_PREFIX_ENV
#define GCC_EXEC_PREFIX_ENV "PIC32_EXEC_PREFIX"
#endif

/* Used for option processing */
extern const char *mchp_processor_string;
extern const char *mchp_config_data_dir;

/* Since these switches are only used by the specs, do not need to assign a
 * unique mask
 */
#define MASK_LINK_PERIPHERAL_LIBS 0
#define MASK_DEBUG_EXEC           0
#define MASK_APPIO_DEBUG          0

/* Put small constants in .rodata, not .sdata. */
#undef TARGET_DEFAULT
#define TARGET_DEFAULT		MASK_EMBEDDED_DATA

#ifndef SUBTARGET_TARGET_SWITCHES
#define SUBTARGET_TARGET_SWITCHES
#endif

/* Many of these were taken from mips.h, but we needed to remove the options
 * that did not make sense for the Microchip port.  SWITCHES are command-line
 * options that set and clear bits in 'target_flags'.
 */
#undef TARGET_SWITCHES
#define TARGET_SWITCHES							\
{									\
  SUBTARGET_TARGET_SWITCHES                                             \
  { "debugger",           MASK_DEBUG_EXEC,                              \
      N_("Allocate room for debugger executive") },                     \
  { "appio-debug",        MASK_APPIO_DEBUG,                             \
      N_("Enable APPIO debug-support library functions") },             \
  { "no-peripheral-libs", MASK_LINK_PERIPHERAL_LIBS,                    \
      N_("Do not link peripheral libraries") },                         \
  {"int64",		  MASK_INT64 | MASK_LONG64,			\
     N_("Use 64-bit int type")},					\
  {"long64",		  MASK_LONG64,					\
     N_("Use 64-bit long type")},					\
  {"long32",		 -(MASK_LONG64 | MASK_INT64),			\
     N_("Use 32-bit long type")},					\
  {"memcpy",		  MASK_MEMCPY,					\
     N_("Don't optimize block moves")},					\
  {"no-memcpy",		 -MASK_MEMCPY,					\
     N_("Optimize block moves")},					\
  {"abicalls",		  MASK_ABICALLS,				\
     N_("Use Irix PIC")},						\
  {"no-abicalls",	 -MASK_ABICALLS,				\
     N_("Don't use Irix PIC")},						\
  {"long-calls",	  MASK_LONG_CALLS,				\
     N_("Use indirect calls")},						\
  {"no-long-calls",	 -MASK_LONG_CALLS,				\
     N_("Don't use indirect calls")},					\
  {"embedded-pic",	  MASK_EMBEDDED_PIC,				\
     N_("Use embedded PIC")},						\
  {"no-embedded-pic",	 -MASK_EMBEDDED_PIC,				\
     N_("Don't use embedded PIC")},					\
  {"embedded-data",	  MASK_EMBEDDED_DATA,				\
     N_("Use ROM instead of RAM")},					\
  {"no-embedded-data",	 -MASK_EMBEDDED_DATA,				\
     N_("Don't use ROM instead of RAM")},				\
  {"uninit-const-in-rodata", MASK_UNINIT_CONST_IN_RODATA,		\
     N_("Put uninitialized constants in ROM (needs -membedded-data)")},	\
  {"no-uninit-const-in-rodata", -MASK_UNINIT_CONST_IN_RODATA,		\
     N_("Don't put uninitialized constants in ROM")},			\
  {"check-zero-division",-MASK_NO_CHECK_ZERO_DIV,			\
     N_("Trap on integer divide by zero")},				\
  {"no-check-zero-division", MASK_NO_CHECK_ZERO_DIV,			\
     N_("Don't trap on integer divide by zero")},			\
  {"ips16",		  MASK_MIPS16,					\
     N_("Generate mips16 code") },					\
  {"no-mips16",		 -MASK_MIPS16,					\
     N_("Generate normal-mode code") },					\
  {"ips16e",		  MASK_MIPS16E,					\
     N_("Generate mips16e code") },					\
  {"xgot",		  MASK_XGOT,					\
     N_("Lift restrictions on GOT size") },				\
  {"no-xgot",		 -MASK_XGOT,					\
     N_("Do not lift restrictions on GOT size") },			\
  {"smart-io",   0,							\
     N_("Enable smart-IO library call forwarding level 1") }, 		\
  {"no-smart-io",   0,							\
     N_("Disable smart-IO library call forwarding") }, 			\
  {"legacy-libc",   0,							\
     N_("Use legacy lib/include directory") }, 				\
  {"",			  (TARGET_DEFAULT				\
			   | TARGET_CPU_DEFAULT				\
			   | TARGET_ENDIAN_DEFAULT),			\
     NULL},								\
}

#ifndef SUBTARGET_TARGET_OPTIONS
#define SUBTARGET_TARGET_OPTIONS
#endif

/* Many of these were taken from mips.h, but we needed to remove the options
 * that did not make sense for the Microchip port. OPTIONS define command-line
 * options that take values.
 */
#undef TARGET_OPTIONS
#define TARGET_OPTIONS							\
{									\
  SUBTARGET_TARGET_OPTIONS						\
  { "no-flush-func", &mips_cache_flush_func,				\
      N_("Don't call any cache flush functions"), 0},			\
  { "flush-func=", &mips_cache_flush_func,				\
      N_("Specify cache flush function"), 0},				\
  { "branch-likely", &mips_brlikely_string,				\
      N_("Specify when to use branch likely instructions"), 0},		\
  { "no-branch-likely", &mips_brlikely_string,				\
      N_("Alias for -mbranch-likely=no"), "=no"},			\
  { "branch-cost=", &mips_branch_cost_string,				\
      N_("Specify cost of branch instructions"), 0},			\
  { "no-data-in-code", &mips_no_data_in_code_string,			\
      N_("Don't place MIPS16 constant data in the code section"), 0},	\
  { "code-xonly", &mips_code_xonly_string,				\
      N_("Execute-only MIPS16 code, strings and jump tables in data seg"), 0},\
  { "flip-mips16", &mips_flip_mips16_string,				\
      0, 0},                                                            \
  { "processor=",    &mchp_processor_string,                            \
      N_("Specify processor"), 0},                                      \
  { "config-data-dir=", &mchp_config_data_dir,                          \
      0, 0},                                                            \
  {"smart-io=", &mchp_io_size,                                          \
      N_("Set smart IO library call forwarding level"), 0},             \
  {"legacy-libc=", &mchp_legacy_libc,                                   \
      N_("Enable/Disable legacy libc library/include"), 0},             \
  {"it=", &mchp_it_transport,                                           \
      N_("Enable instrumented trace for MPLAB REAL ICE"), 0},           \
}

/* We want to change the default pre-defined macros. Many of these
   are the same as presented in sde.h, but not all */
#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS()                            \
  do {                                                      \
    if (TARGET_ABICALLS || flag_pic)                        \
      {                                                     \
        builtin_define ("__PIC__");                         \
        builtin_define ("__pic__");                         \
      }                                                     \
                                                            \
    if (mips_abi != ABI_32)                                 \
      fatal_error ("internal error: mips_abi != ABI_32");   \
    builtin_define ("_ABIO32=1");                           \
    builtin_define ("_MIPS_SIM=_ABIO32");                   \
    builtin_define_std ("__C32__");                         \
    if (TARGET_INT64)                                       \
      {                                                     \
        builtin_define ("_MIPS_SZINT=64");                  \
        builtin_define ("_MCHP_SZINT=64");                  \
      }                                                     \
    else                                                    \
      {                                                     \
        builtin_define ("_MIPS_SZINT=32");                  \
        builtin_define ("_MCHP_SZINT=32");                  \
      }                                                     \
                                                            \
    if (TARGET_LONG64)                                      \
      {                                                     \
        builtin_define ("_MIPS_SZLONG=64");                 \
        builtin_define ("_MCHP_SZLONG=64");                 \
      }                                                     \
    else                                                    \
      {                                                     \
        builtin_define ("_MIPS_SZLONG=32");                 \
        builtin_define ("_MCHP_SZLONG=32");                 \
      }                                                     \
                                                            \
    builtin_define ("_MIPS_SZPTR=32");                      \
    builtin_define ("_MCHP_SZPTR=32");                      \
                                                            \
    if (TARGET_NO_FLOAT)                                    \
      {                                                     \
        builtin_define ("__NO_FLOAT");                      \
        builtin_define ("__mips_no_float");                 \
        builtin_define ("__mchp_no_float");                 \
      }                                                     \
    else if (TARGET_SOFT_FLOAT && !mips16_hard_float)       \
      {                                                     \
        builtin_define ("__SOFT_FLOAT");                    \
      }                                                     \
    else if (TARGET_SINGLE_FLOAT)                           \
      {                                                     \
        builtin_define ("__SINGLE_FLOAT");                  \
      }                                                     \
                                                            \
    builtin_define ("__BUILTIN_ITTYPE");                    \
                                                            \
    builtin_define_std ("PIC32MX");                         \
    if (mchp_processor_string && *mchp_processor_string)    \
      {                                                     \
        char *proc, *p;                                     \
        int setnum, memsize;                                \
        char *pinset;                                       \
        pinset = alloca(2);                                 \
        pinset[1] = NULL;                                   \
        proc = alloca (strlen (mchp_processor_string) + 5); \
        sprintf (proc, "__%s__", mchp_processor_string);    \
        for (p = proc ; *p ; p++)                           \
          *p = toupper (*p);                                \
        builtin_define (proc);                              \
                                                            \
        if (strchr(proc,'F') != NULL) {                     \
          sscanf (proc, "__32MX%6dF%6d%1c__", &setnum,      \
                  &memsize, &pinset[0]);                    \
          builtin_define_with_int_value                     \
             ("__PIC32_FEATURE_SET__",                      \
              setnum);                                      \
          builtin_define_with_int_value                     \
             ("__PIC32_MEMORY_SIZE__",                      \
              memsize);                                     \
          builtin_define_with_value                         \
             ("__PIC32_PIN_SET__",                          \
              pinset, 1);                                   \
        }                                                   \
      }                                                     \
    else                                                    \
      {                                                     \
        builtin_define ("__32MXGENERIC__");                 \
      }                                                     \
      if (1) \
      { \
    if (mchp_it_transport && *mchp_it_transport)            \
      {                                                     \
        char *mchp_it_define = NULL;                        \
        if (strlen (mchp_it_transport) > 100)               \
          {                                                     \
            warning("-mit=%s invalid; defaulting to -mit=dc",   \
                    mchp_it_transport);                         \
            mchp_it_transport = "dc";                           \
          }                                                     \
          {                                                 \
            char *mchp_it_option = NULL;                                           \
            char *mchp_it_option_arg = NULL;                                       \
            mchp_it_option = alloca(strlen(mchp_it_transport));                    \
             if (strchr(mchp_it_transport, '(') < strchr(mchp_it_transport, ')'))  \
             { \
              mchp_it_option_arg = alloca(strlen(mchp_it_transport)); \
              sscanf (mchp_it_transport, "%10[^(](%90[^)])",          \
                mchp_it_option, mchp_it_option_arg);                  \
              }                                                       \
              else                                                    \
               {                                                      \
                 mchp_it_option = mchp_it_transport;                  \
               }                                                      \
            mchp_it_define =                                          \
              alloca (strlen ("__IT_TRANSPORT=xx") +            \
                      strlen(mchp_it_option) + 1);                    \
            snprintf (mchp_it_define, strlen ("__IT_TRANSPORT= ") +     \
                      strlen(mchp_it_option) + 1, "__IT_TRANSPORT=%s",  \
                      mchp_it_option);                                         \
            builtin_define (mchp_it_define);                          \
            if (mchp_it_option_arg != NULL) {                         \
              int i=1;                                                \
              char *mchp_it_option_define = NULL;                     \
              char *s,*c;                                             \
              c = mchp_it_option_arg;                                 \
              do {                                                    \
                s = c;                                                \
                for (; *c && *c != ','; c++);                         \
                if (*c) *c++ = 0;                                     \
                mchp_it_option_define =                               \
                  alloca(strlen("__IT_TRANSPORT_OPTION%d=%s")+        \
                  strlen(s));                                         \
                snprintf(mchp_it_option_define,                       \
                  strlen("__IT_TRANSPORT_OPTION%d=%s")+               \
                  strlen(s), "__IT_TRANSPORT_OPTION%d=%s",i++,s);     \
                builtin_define(mchp_it_option_define);                \
               } while (*c);                                          \
              }                                                       \
          }                                                           \
      }                                                               \
      }                                                               \
    if (version_string && *version_string)                  \
      {                                                     \
        char *Microchip;                                    \
        int pic32_compiler_version;                         \
        Microchip = strrchr (version_string, 'v');          \
        if (Microchip)                                      \
          {                                                 \
            int major =0, minor=0;                          \
            while ((*Microchip) &&                          \
                   ((*Microchip < '0') ||                   \
                    (*Microchip > '9')))                    \
              { Microchip++; }                              \
            if (*Microchip)                                 \
              {                                             \
                major = strtol (Microchip, &Microchip, 0);  \
              }                                             \
            if ((*Microchip) &&                             \
               ((*Microchip=='_') || (*Microchip=='.')))    \
               {                                            \
                 Microchip++;                               \
                 minor = strtol(Microchip, &Microchip, 0);  \
               }                                            \
             pic32_compiler_version = (major*100) + minor;  \
          }                                                 \
        else                                                \
          {                                                 \
            fatal_error ("internal error: version_string == NULL");  \
            builtin_define_with_int_value ("__C32_VERSION__", -1);   \
          }                                                 \
        builtin_define_with_int_value ("__C32_VERSION__", pic32_compiler_version);  \
      }                                                     \
    builtin_define_with_int_value ("__MCHP_SMARTIO_VALUE__", mchp_io_size_val); \
  } while (0);

/* The Microchip port has a few pragmas to define as well */
#define REGISTER_TARGET_PRAGMAS() { \
  c_register_pragma(0, "interrupt", mchp_handle_interrupt_pragma); \
  c_register_pragma(0, "vector", mchp_handle_vector_pragma); \
  c_register_pragma(0, "config", mchp_handle_config_pragma); }

/* There are no additional prefixes to try after STANDARD_EXEC_PREFIX. */
#undef MD_EXEC_PREFIX

/* There are no additional prefixes to try after STANDARD_STARTFILE_PREFIX. */
#undef MD_STARTFILE_PREFIX
#if 0
#undef TARGET_STRIP_NAME_ENCODING
#define TARGET_STRIP_NAME_ENCODING mchp_strip_name_encoding
#endif

