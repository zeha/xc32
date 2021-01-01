/* Subroutines used for Microchip PIC32C code generation.
   Copyright (C) 1989, 1990, 1991, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
   Free Software Foundation, Inc.
   Contributed by Rathish C, rathish.chamukuttan@microchip.com

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "c-family/c-common.h"
#include "hash-table.h"
#include "tm.h"
#include "rtl.h"
#include "hash-set.h"
#include "machmode.h"
#include "vec.h"
#include "double-int.h"
#include "input.h"
#include "alias.h"
#include "symtab.h"
#include "wide-int.h"
#include "inchash.h"
#include "tree.h"
#include "attribs.h"
#include "memmodel.h"
#include "gimple.h"
#include "fold-const.h"
#include "stringpool.h"
#include "stor-layout.h"
#include "calls.h"
#include "varasm.h"
#include "obstack.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "insn-config.h"
#include "conditions.h"
#include "output.h"
#include "insn-attr.h"
#include "flags.h"
#include "reload.h"
#include "function.h"
#include "hashtab.h"
#include "statistics.h"
#include "real.h"
#include "fixed-value.h"
#include "expmed.h"
#include "dojump.h"
#include "explow.h"
#include "emit-rtl.h"
#include "stmt.h"
#include "expr.h"
#include "insn-codes.h"
#include "optabs.h"
#include "diagnostic-core.h"
#include "recog.h"
#include "predict.h"
#include "dominance.h"
#include "cfg.h"
#include "cfgrtl.h"
#include "cfganal.h"
#include "lcm.h"
#include "cfgbuild.h"
#include "cfgcleanup.h"
#include "basic-block.h"
#include "hash-map.h"
#include "is-a.h"
#include "plugin-api.h"
#include "ipa-ref.h"
#include "cgraph.h"
#include "ggc.h"
#include "except.h"
#include "tm_p.h"
#include "target.h"
// #include "target-def.h"
#include "debug.h"
#include "langhooks.h"
#include "bitmap.h"
#include "df.h"
#include "intl.h"
#include "libfuncs.h"
#include "params.h"
#include "opts.h"
#include "dumpfile.h"
#include "gimple-expr.h"
#include "builtins.h"
#include "tm-constrs.h"
#include "rtl-iter.h"
#include "sched-int.h"
#include "gimplify.h"
#include "version.h"
#include "incpath.h"
#include "cpplib.h"
#include "gimple-iterator.h"
#include "gimplify.h"
#include "builtins.h"
#include "gimple-fold.h"

#include "dumpfile.h"
#include "tree-pretty-print.h"
#include "xc-coverage.h"

#include "pic32c-protos.h"
#include "pic32c.h"
#include "config/mchp-cci/cci-pragmas.h"

/* This file should be included last.  */
#include "target-def.h"

#define DUMP_DEBUG_TCM 0

#if DUMP_DEBUG_TCM
#define DEBUG_PIC32C(...)                                                      \
  do                                                                           \
    {                                                                          \
      fprintf (stderr, "--> (%s , %d) : ", __func__, __LINE__);                \
      fprintf (stderr, __VA_ARGS__);                                           \
    }                                                                          \
  while (0)
#define DEBUG_ENTER_FUNC                                                       \
  do                                                                           \
    {                                                                          \
      DEBUG_PIC32C ("Enter func\n");                                           \
    }                                                                          \
  while (0)
#else
#define DEBUG_PIC32C(...)
#define DEBUG_ENTER_FUNC
#endif

/* PIC32C builtins decls table */
static GTY (()) tree pic32c_builtin_decls[PIC32C_BUILTIN_MAX];

#ifdef __MINGW32__
void *alloca (size_t);
#else
#include <alloca.h>
#endif

#define xstr(s) str (s)
#define str(s) #s

#define const_section readonly_data_section
#define dconst_section readonly_data_section

SECTION_FLAGS_INT mchp_text_flags = SECTION_CODE;

#define JOIN2(X, Y) (X##Y)
#define JOIN(X, Y) JOIN2 (X, Y)
#define MCHP_ULL(X) JOIN2 (X, ULL)

/* Return a nonzero value if DECL has a section attribute.  */
#ifndef IN_NAMED_SECTION
#define IN_NAMED_SECTION(DECL)                                                 \
  ((TREE_CODE (DECL) == FUNCTION_DECL || TREE_CODE (DECL) == VAR_DECL)         \
   && DECL_SECTION_NAME (DECL) != NULL)
#endif

/* 0x100000 */
#define SECTION_READ_ONLY (MCHP_ULL (SECTION_MACH_DEP))
#define SECTION_CONST     (MCHP_ULL (SECTION_MACH_DEP))
#define SECTION_RAMFUNC   (MCHP_ULL (SECTION_MACH_DEP) << 1ull)
#define SECTION_NEAR      (MCHP_ULL (SECTION_MACH_DEP) << 2ull)
#define SECTION_PERSIST   (MCHP_ULL (SECTION_MACH_DEP) << 3ull)
#define SECTION_NOLOAD    (MCHP_ULL (SECTION_MACH_DEP) << 4ull)
#define SECTION_INFO      (MCHP_ULL (SECTION_MACH_DEP) << 5ull)
#define SECTION_ADDRESS   (MCHP_ULL (SECTION_MACH_DEP) << 6ull)
#define SECTION_ALIGN     (MCHP_ULL (SECTION_MACH_DEP) << 7ull)
#define SECTION_KEEP      (MCHP_ULL (SECTION_MACH_DEP) << 8ull)
#define SECTION_COHERENT  (MCHP_ULL (SECTION_MACH_DEP) << 9ull)
#define SECTION_REGION    (MCHP_ULL (SECTION_MACH_DEP) << 10ull)
#define SECTION_SERIALMEM (MCHP_ULL (SECTION_MACH_DEP) << 11ull)
#define SECTION_ITCM      (MCHP_ULL (SECTION_MACH_DEP) << 12ull)
#define SECTION_DTCM      (MCHP_ULL (SECTION_MACH_DEP) << 13ull)
#define SECTION_NOPA      (MCHP_ULL (SECTION_MACH_DEP) << 14ull)

/* the attribute names from the assemblers point of view */
#define SECTION_ATTR_ADDRESS   "address"
#define SECTION_ATTR_ALIGN     "align"
#define SECTION_ATTR_BSS       "bss"
#define SECTION_ATTR_CODE      "code"
#define SECTION_ATTR_CONST     "code"
#define SECTION_ATTR_COHERENT  "coherent"
#define SECTION_ATTR_DATA      "data"
#define SECTION_ATTR_DTCM      "dtcm"
#define SECTION_ATTR_INFO      "info"
#define SECTION_ATTR_ITCM      "itcm"
#define SECTION_ATTR_KEEP      "keep"
#define SECTION_ATTR_NEAR      "near"
#define SECTION_ATTR_NOLOAD    "noload"
#define SECTION_ATTR_PERSIST   "persist"
#define SECTION_ATTR_RAMFUNC   "ramfunc"
#define SECTION_ATTR_DEFAULT   "unused"
#define SECTION_ATTR_REGION    "memory"
#define SECTION_ATTR_CO_SHARED "shared"
#define SECTION_ATTR_SERIALMEM "serial_mem"
#define SECTION_ATTR_NOPA      "nopa"

#define SECTION_NAME_BSS        ".bss"
#define SECTION_NAME_NBSS       ".sbss"
#define SECTION_NAME_SBSS       ".sbss"
#define SECTION_NAME_DATA       ".data"
#define SECTION_NAME_NDATA      ".sdata"
#define SECTION_NAME_SDATA      ".sdata"
#define SECTION_NAME_CONST      ".rodata"
#define SECTION_NAME_RAMFUNC    ".ramfunc"
#define SECTION_NAME_PBSS       ".pbss"
#define SECTION_NAME_PERSIST    ".persist"
#define SECTION_NAME_INIT       ".init"
#define SECTION_NAME_FINI       ".fini"
#define SECTION_NAME_CTORS      ".ctors"
#define SECTION_NAME_DTORS      ".dtors"
#define SECTION_NAME_INIT_ARRAY ".init_array"
#define SECTION_NAME_FINI_ARRAY ".fini_array"
#define SECTION_NAME_SERIALMEM  ".serial_mem"
#define SECTION_NAME_ITCM       ".text_itcm"


struct valid_section_flags_
{
  const char *flag_name;
  char single_letter_equiv;
  unsigned long long flag_mask;
  unsigned long long incompatible_with;
} valid_section_flags[]
  = {{SECTION_ATTR_ADDRESS, 0, SECTION_ADDRESS, SECTION_ALIGN | SECTION_INFO},
      
     {SECTION_ATTR_ALIGN, 0, SECTION_ALIGN, SECTION_ADDRESS | SECTION_INFO},
      
     {SECTION_ATTR_BSS, 'b', SECTION_BSS,
      SECTION_CODE | SECTION_WRITE | SECTION_PERSIST | SECTION_READ_ONLY},
      
     {SECTION_ATTR_CODE, 'x', SECTION_CODE,
      SECTION_WRITE | SECTION_BSS | SECTION_NEAR | SECTION_PERSIST
	| SECTION_READ_ONLY | SECTION_DTCM},
      
     {SECTION_ATTR_CONST, 'r', SECTION_READ_ONLY,
      SECTION_CODE | SECTION_WRITE | SECTION_BSS | SECTION_NEAR | SECTION_INFO
	| SECTION_DTCM},
      
     {SECTION_ATTR_COHERENT, 0, SECTION_COHERENT, 0},
      
     {SECTION_ATTR_DATA, 'd', SECTION_WRITE,
      SECTION_BSS | SECTION_PERSIST | SECTION_READ_ONLY | SECTION_ITCM},
      
     {SECTION_ATTR_DTCM, 0, SECTION_DTCM,
      SECTION_CODE | SECTION_PERSIST | SECTION_READ_ONLY | SECTION_ITCM
	| SECTION_NEAR},
      
     {SECTION_ATTR_INFO, 0, SECTION_INFO,
      SECTION_PERSIST | SECTION_ADDRESS | SECTION_NEAR | SECTION_ALIGN
	| SECTION_NOLOAD | SECTION_MERGE | SECTION_READ_ONLY | SECTION_ITCM
	| SECTION_DTCM},
      
     {SECTION_ATTR_ITCM, 0, SECTION_ITCM,
      SECTION_WRITE | SECTION_BSS | SECTION_NEAR | SECTION_PERSIST
	| SECTION_RAMFUNC | SECTION_DTCM},
      
     {SECTION_ATTR_KEEP, 0, SECTION_KEEP, 0},
      
     {SECTION_ATTR_NOLOAD, 0, SECTION_NOLOAD, SECTION_MERGE | SECTION_INFO},
      
     {SECTION_ATTR_PERSIST, 'b', SECTION_PERSIST,
      SECTION_CODE | SECTION_WRITE | SECTION_BSS | SECTION_MERGE | SECTION_INFO
	| SECTION_READ_ONLY | SECTION_ITCM | SECTION_DTCM},
      
     {SECTION_ATTR_RAMFUNC, 0, SECTION_RAMFUNC,
      SECTION_WRITE | SECTION_BSS | SECTION_NEAR | SECTION_PERSIST
	| SECTION_ITCM | SECTION_DTCM},
      
     {SECTION_ATTR_REGION, 0, SECTION_REGION, 0},
     {SECTION_ATTR_SERIALMEM, 'r', SECTION_READ_ONLY,
      SECTION_CODE | SECTION_WRITE | SECTION_BSS | SECTION_NEAR | SECTION_INFO
	| SECTION_ITCM | SECTION_DTCM},
     {SECTION_ATTR_NOPA, 0, SECTION_NOPA, 0},
     {0, 0, 0, 0}};
static const int num_vsf
  = (sizeof (valid_section_flags) / sizeof (struct valid_section_flags_)) - 1;
static struct valid_section_flags_ key_vsf = {0, 0, 0, 0};

struct reserved_section_names_
{
  const char *section_name;
  SECTION_FLAGS_INT mask;
} reserved_section_names[] = {
  {".bss", SECTION_BSS},
  {".rodata", SECTION_READ_ONLY},
  {".rodata1", SECTION_READ_ONLY},
  {".data", SECTION_WRITE},
  {".data_dtcm", SECTION_WRITE | SECTION_DTCM},
  {".bss_dtcm", SECTION_BSS | SECTION_DTCM},
  {".dconst", SECTION_WRITE},
  {".sbss", SECTION_BSS | SECTION_NEAR},
  {".sdata", SECTION_WRITE | SECTION_NEAR},
  {".sdconst", SECTION_WRITE | SECTION_NEAR},
  {".pbss", SECTION_PERSIST},
  {".text", SECTION_CODE},
  {".text_itcm", SECTION_ITCM | SECTION_CODE},
  {".ramfunc", SECTION_RAMFUNC | SECTION_CODE},
  {".codecov_info", SECTION_INFO},
  {".gnu.linkonce.d", SECTION_WRITE},
  {".gnu.linkonce.t", SECTION_CODE},
  {".gnu.linkonce.r", SECTION_READ_ONLY},
  {".gnu.linkonce.s2", SECTION_READ_ONLY},
  {".gnu.linkonce.sb2", SECTION_READ_ONLY},
  {".got", SECTION_WRITE},
  {".got.plt", SECTION_WRITE},
  {".gnu.linkonce.s", SECTION_WRITE | SECTION_NEAR},
  {".gnu.linkonce.sb", SECTION_BSS | SECTION_NEAR},
  {".dynsbss", SECTION_BSS | SECTION_NEAR},
  {".scommon", SECTION_BSS | SECTION_NEAR},
  {".dynbss", SECTION_BSS},
  {0, 0},
};
static const int num_rsn
  = (sizeof (reserved_section_names) / sizeof (struct reserved_section_names_))
    - 1;
static struct reserved_section_names_ key_rsn = {0, 0};

/*
 *  Static Variables
 */
static const char *mchp_default_section = "*";
static char this_default_name[sizeof ("*_012345670123456701234567")];
static time_t current_time = 0;

/*
 *  Section stack to process the section names.
 */
enum
{
  ss_pushed = 0,    /* section stack was pushed */
  ss_set = 1,       /* section stack was set */
  ss_should_pop = 2 /* popped, but wait til we see what is pushed next */
};

typedef struct sectionStack_
{
  const char *pszName;
  unsigned int pszFlag;
  struct sectionStack_ *pop;
  SECTION_FLAGS_INT flags;
} sectionStack;

enum css
{
  css_pop = 0,
  css_push = 1,
  css_activate = 2,
  css_tos = 3
};

static sectionStack default_section
  = {".text.default_section", SECTION_CODE, 0x0, 0x0};

static sectionStack *lSectionStack = &default_section;
static sectionStack *freeSectionStack;

/*
 *  Table mapping smart-io builtin ids to function name/decl.
 *  Used by pic32c_expand_smartio_builtin.
 *  Note order must match order of (contiguous) smart-io builtin ids
 *  from pic32c.h.
 */
typedef struct smartio_fndesc_
{
  const char *base_name;
  enum built_in_function fcode;
} smartio_fndesc;

static smartio_fndesc smartio_fn_info[PIC32C_BUILTIN_SMARTIO_N] = {
  {"fprintf", BUILT_IN_FPRINTF},     /* PIC32C_BUILTIN_FPRINTF_S */
  {"sprintf", BUILT_IN_SPRINTF},     /* PIC32C_BUILTIN_SPRINTF_S */
  {"snprintf", BUILT_IN_SNPRINTF},   /* PIC32C_BUILTIN_SNPRINTF_S */
  {"vfprintf", BUILT_IN_VFPRINTF},   /* PIC32C_BUILTIN_VFPRINTF_S */
  {"vsprintf", BUILT_IN_VSPRINTF},   /* PIC32C_BUILTIN_VSPRINTF_S */
  {"vsnprintf", BUILT_IN_VSNPRINTF}, /* PIC32C_BUILTIN_VSNPRINTF_S */
  {"fscanf", BUILT_IN_FSCANF},       /* PIC32C_BUILTIN_FSCANF_S */
  {"sscanf", BUILT_IN_SSCANF},       /* PIC32C_BUILTIN_SSCANF_S */
  {"vfscanf", BUILT_IN_VFSCANF},     /* PIC32C_BUILTIN_VFSCANF_S */
  {"vsscanf", BUILT_IN_VSSCANF},     /* PIC32C_BUILTIN_VSSCANF_S */
  {"printf", BUILT_IN_PRINTF},       /* PIC32C_BUILTIN_PRINTF_S */
  {"vprintf", BUILT_IN_VPRINTF},     /* PIC32C_BUILTIN_VPRINTF_S */
  {"scanf", BUILT_IN_SCANF},	 /* PIC32C_BUILTIN_SCANF_S */
  {"vscanf", BUILT_IN_VSCANF}	/* PIC32C_BUILTIN_VSCANF_S */
};
#define SMARTIO_MAX_NAME                                                       \
  64 /* note this must have room for longest base name + suffix */

/* Map a smart-io builtin in the PIC32C builtin enum space to a standard IO
   builtin fcode. No error checking is done - must be in range
   PIC32C_BUILTIN_SMARTIO_FIRST to PIC32C_BUILTIN_SMARTIO_LAST. */
#define SMARTIO_TO_STDIO(fc) (smartio_fn_info[PIC32C_SMARTIO_INDEX (fc)].fcode)

/*  Configuration specificiation data used by cci */
struct mchp_config_specification *mchp_configuration_values;

/* mchp_pragma_nocodecov is != 0 when #pragma nocodecov is in effect */
int mchp_pragma_nocodecov = 0;


/*
 *  Static function prototypes.
 */
static int ignore_attribute (const char *attribute, const char *attached_to,
			     tree node);

static tree get_mchp_absolute_address (tree decl);

static int mchp_persistent_p (tree func);

static int mchp_keep_p (tree decl);

static int mchp_nopa_p (tree decl);

static const char *default_section_name (tree decl, SECTION_FLAGS_INT flags);

void mchp_push_section_name (const char *pszSectionName,
			     SECTION_FLAGS_INT pszSectionFlag);

static tree mchp_push_pop_constant_section (tree decl, enum css push,
					    const char **section_name);

static void mchp_pop_section_name (void);

static const char *mchp_strip_name_encoding_helper (const char *symbol_name);

static void mchp_merged_asm_named_section (const char *name,
					   SECTION_FLAGS_INT flags);

static char *mchp_get_named_section_flags (const char *pszSectionName,
					   SECTION_FLAGS_INT flags);

static SECTION_FLAGS_INT validate_identifier_flags (const char *id, tree decl);

static SECTION_FLAGS_INT validate_section_flags (const char *name,
						 SECTION_FLAGS_INT attr_flags);

static int mchp_vsf_compare (const void *va, const void *vb);

static int mchp_rsn_compare (const void *va, const void *vb);

static SECTION_FLAGS_INT section_flags_from_decl (tree decl);

static struct reserved_section_names_ *pic32c_find_in_rsn (const char *name);

static valid_section_flags_ *pic32c_find_in_vsf (const char *name);

extern bool bss_initializer_p (const_tree decl);


/* Attributes processing */

static int
ignore_attribute (const char *attribute, const char *attached_to, tree node)
{
  tree scope;

  if (TREE_CODE (node) == PARM_DECL)
    {
      warning (0, "ignoring %s attribute applied to parameter %s", attribute,
	       attached_to);
      return 1;
    }
  else if (TREE_CODE (node) == FIELD_DECL)
    {
      warning (0, "ignoring %s attribute applied to member %s", attribute,
	       attached_to);
      return 1;
    }

  /* DECL_CONTEXT is not set up by the time we process the attributes */
  scope = current_function_decl;

  if (scope && (TREE_CODE (scope) == FUNCTION_DECL) && (!TREE_STATIC (node))
      && ((TREE_CODE (node) != VAR_DECL) || !TREE_PUBLIC (node)))
    {
      warning (0, "ignoring %s attribute applied to automatic %s", attribute,
	       attached_to);
      return 1;
    }
  return 0;
}

/*
 *  "address()" attribute.
 */

tree
pic32c_address_attribute (tree *decl, tree identifier ATTRIBUTE_UNUSED,
			  tree args, int flags ATTRIBUTE_UNUSED,
			  bool *no_add_attrs)
{
  const char *attached_to = 0;
  tree address = NULL_TREE;

  if (args == NULL)
    {
      return NULL_TREE;
    }

  if (flag_pic || flag_pie)
    {
      warning (0, "'address' attribute cannot be specified with -fpic, -fPIC "
		  "or -fpie option");
      *no_add_attrs = 1;
      return NULL_TREE;
    }

  if (DECL_P (*decl))
    {
      attached_to = IDENTIFIER_POINTER (DECL_NAME (*decl));
    }

  /* We can assert one argument since that should be enforced by
     the parser from the attribute table
  */

  gcc_assert (TREE_CHAIN (args) == NULL);

  if (DECL_P (*decl))
    {
      if (ignore_attribute ("address", attached_to, *decl))
	{
	  *no_add_attrs = 1;
	  return NULL_TREE;
	}

      address = TREE_VALUE (args);

      if (TREE_CODE (address) != INTEGER_CST)
	{
	  error ("invalid address argument for '%s'", attached_to);
	  *no_add_attrs = 1;
	}
      else
	{
	  /* currently the assembler will not accept an unaligned address
	    TODO: test this works as expected */
	  if (TREE_INT_CST_LOW (address) % 4)
	    {
	      warning (0, "invalid address argument for '%s'", attached_to);
	      warning (0, "unaligned addresses are not yet permitted, ignoring "
			  "attribute");
	      *no_add_attrs = 1;
	    }
	}

      unsigned long long address_val;
      address_val = (unsigned long long) TREE_INT_CST_LOW (address);

#if 0 /* TODO: update address-range checking for Cortex-Mx cores */
    if ((address_val &0xFFFFFFFFul) < 0x20000000ul)
    {
      error ("Physical-memory address used with address attribute. Use a "
         "virtual-memory address for '%s'",
         attached_to);
      *no_add_attrs = 1;
    }
#endif

      if (address_val > 0xFFFFFFFFul)
	{
	  error ("Address of '%s' is outside of the 32-bit address range",
		 attached_to);
	  *no_add_attrs = 1;
	}
    }

  /* Address attribute implies noinline and noclone */
  if (TREE_CODE (*decl) == FUNCTION_DECL)
    {
      tree attrib_noinline, attrib_noclone;
      //    long unsigned int address_val;
      //    address_val = (long unsigned int)TREE_INT_CST_LOW(address) &
      //    0xFFFFFFFFul;

      if (lookup_attribute ("noinline", DECL_ATTRIBUTES (*decl)) == NULL)
	{
	  attrib_noinline
	    = build_tree_list (get_identifier ("noinline"), NULL_TREE);
	  attrib_noinline = chainon (DECL_ATTRIBUTES (*decl), attrib_noinline);
	  decl_attributes (decl, attrib_noinline, 0);
	}

      if (lookup_attribute ("noclone", DECL_ATTRIBUTES (*decl)) == NULL)
	{
	  attrib_noclone
	    = build_tree_list (get_identifier ("noclone"), NULL_TREE);
	  attrib_noclone = chainon (DECL_ATTRIBUTES (*decl), attrib_noclone);
	  decl_attributes (decl, attrib_noclone, 0);
	}
    }

  /* For support purposes, emit a warning for all uses of the address()
  attribute */

  if (mchp_warn_address_attribute_use)
    {
      warning (OPT_Waddress_attribute_use, "use of address() attribute on %qs",
	       attached_to);
    }

  return NULL_TREE;
}

/*
 *  "target_error" attribute.
 */
tree pic32c_target_error_attribute(tree *node, tree identifier ATTRIBUTE_UNUSED,
                                   tree args, int flags ATTRIBUTE_UNUSED,
                                   bool *no_add_attrs)
{
  if (!DECL_P (*node)) {
    error("'target_error' attribute may be applied only to declarations");
    return NULL_TREE;
  }
  const char *attached_to = IDENTIFIER_POINTER(DECL_NAME(*node));
  if (TREE_CODE(TREE_VALUE(args)) != STRING_CST) {
    error("invalid argument to 'target_error' attribute applied to '%s',"
          " literal string expected", attached_to);
    return NULL_TREE;
  }
  TREE_DEPRECATED(*node) = 1;
  return NULL_TREE;
}

/*
 *  "unsupported" attribute.
 */
tree
pic32c_unsupported_attribute (tree *node, tree identifier ATTRIBUTE_UNUSED,
			      tree args, int flags ATTRIBUTE_UNUSED,
			      bool *no_add_attrs)
{
  tree type = NULL_TREE;
  int warn = 0;
  tree what = NULL_TREE;
  const char *attached_to = 0;

  if (DECL_P (*node))
    {
      attached_to = IDENTIFIER_POINTER (DECL_NAME (*node));
    }

  if (!args)
    *no_add_attrs = true;
  else if (TREE_CODE (TREE_VALUE (args)) != STRING_CST)
    {
      error ("invalid argument to 'unsupported' attribute applied to '%s',"
	     " literal string expected",
	     attached_to);
      *no_add_attrs = true;
    }

  if (DECL_P (*node))
    {
      tree decl = *node;
      type = TREE_TYPE (decl);

      if (TREE_CODE (decl) == TYPE_DECL || TREE_CODE (decl) == PARM_DECL
	  || TREE_CODE (decl) == VAR_DECL || TREE_CODE (decl) == FUNCTION_DECL
	  || TREE_CODE (decl) == FIELD_DECL)
	TREE_DEPRECATED (decl) = 1;
      else
	warn = 1;
    }
  else if (TYPE_P (*node))
    {
      if (!(flags & (int) ATTR_FLAG_TYPE_IN_PLACE))
	*node = build_variant_type_copy (*node);
      TREE_DEPRECATED (*node) = 1;
      type = *node;
    }
  else
    warn = 1;

  if (warn)
    {
      *no_add_attrs = true;
      if (type && TYPE_NAME (type))
	{
	  if (TREE_CODE (TYPE_NAME (type)) == IDENTIFIER_NODE)
	    what = TYPE_NAME (*node);
	  else if (TREE_CODE (TYPE_NAME (type)) == TYPE_DECL
		   && DECL_NAME (TYPE_NAME (type)))
	    what = DECL_NAME (TYPE_NAME (type));
	}
      if (what)
	warning (OPT_Wattributes, "%qE attribute ignored for %qE", identifier,
		 what);
      else
	warning (OPT_Wattributes, "%qE attribute ignored", identifier);
    }

  return NULL_TREE;
}

/*
** Return nonzero if IDENTIFIER is a valid noload attribute.
*/
tree
pic32c_noload_attribute (tree *decl, tree identifier ATTRIBUTE_UNUSED,
			 tree args, int flags ATTRIBUTE_UNUSED,
			 bool *no_add_attrs)
{
  const char *attached_to = 0;

  if (DECL_P (*decl))
    attached_to = IDENTIFIER_POINTER (DECL_NAME (*decl));

  if (ignore_attribute ("noload", attached_to, *decl))
    {
      *no_add_attrs = 1;
      return NULL_TREE;
    }

  /* If the persistent attribute is specified, noload is implied, and
     we can simply handle this as any other persistent data (i.e.
     placed into noload section .pbss. */
  if (mchp_persistent_p (*decl))
    {
      return NULL_TREE;
    }

  if (TREE_CODE (*decl) != FUNCTION_DECL && DECL_INITIAL (*decl))
    {
      location_t loc = DECL_SOURCE_LOCATION (*decl);

      /* This odd warning is for consistency with xc16. It's been moved
         to attribute checking to avoid repeatedly issuing it in build_prefix. */
      if (attached_to)
	warning_at (loc, 0, "%D Noload variable '%s' will not be initialized",
		    *decl, attached_to);
      else
	warning_at (loc, 0, "%D Noload variable will not be initialized",
		    *decl);
    }


  /* Like 'keep' we require unique sections for noload at present. The
     alternative (which seems preferable) would be to create noload variants
     of all sections and map accordingly.
     Like 'keep', we do not imply the unqiue_section attribute or any checks
     directly. */
  DECL_COMMON (*decl) = 0;
  DECL_UNIQUE_SECTION (*decl) = 1;

  return NULL_TREE;
}

/*
** Return nonzero if IDENTIFIER is a valid space attribute.
*/
tree
pic32c_space_attribute (tree *decl, tree identifier ATTRIBUTE_UNUSED, tree args,
			int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  const char *attached_to = 0;
  const char *ident = NULL;
  if (args == NULL)
    {
      return NULL_TREE;
    }

  if (DECL_P (*decl))
    {
      attached_to = IDENTIFIER_POINTER (DECL_NAME (*decl));
    }

  /* we can assert one argument since that should be enforced by
     the parser from the attribute table */

  gcc_assert (TREE_CHAIN (args) == NULL);

  if (NULL == DECL_NAME (*decl))
    {
      warning (OPT_Wattributes, "Ignoring space attribute due to missing "
				"declaration. Check placement of attribute.");
      return NULL_TREE;
    }

  ident = IDENTIFIER_POINTER (DECL_NAME (*decl));

  if (DECL_P (*decl))
    {
      tree space;

      if (ignore_attribute ("space", attached_to, *decl))
	{
	  *no_add_attrs = 1;
	  return NULL_TREE;
	}
      space = lookup_attribute ("space", DECL_ATTRIBUTES (*decl));

      if (space)
	{
	  warning (OPT_Wattributes, "ignoring previous space attribute");
	}

      /*
	 space(data) attribute can be specified for functions.
	 It will be treated as ramfunc.
	 If region() attribute is specified, space(data) can be
	 used to qualify the function to be copied to external
	 data memory.
	*/


  if (TREE_VALUE (args) == get_identifier ("prog"))
	{
	  return space;
	}
  else if (TREE_VALUE (args) == get_identifier ("data"))
	{
    /* TODO add long_call attributes to functions with space(data) attribute,
    otherwise linker needs -mlong-calls to link these functions */
	  return space;
	}
#if 0 /* not supported yet */
      else if (TREE_VALUE(args) == get_identifier("serial_mem"))
        {
          if (FUNCTION_DECL == TREE_CODE(*decl))
            {
              warning (OPT_Wattributes, "Ignoring space(serial_mem) attribute for function %qs. Suggest space(prog) for functions.", ident);
            }
          else if (VAR_DECL == TREE_CODE(*decl))
            {
                DECL_COMMON (*decl) = 0;
                /* Serial mem is flash so the variable should be const qualified */
                if (!TREE_READONLY(*decl))
                  error ("%Jspace(\"serial_mem\") variable '%D' should be const qualified", *decl, *decl);
            }
          return space;
        }
#endif
      else if ((TREE_VALUE (args) == get_identifier ("psv"))
	       || (TREE_VALUE (args) == get_identifier ("auto_psv")))
	{
	  /* Accept for C30 compatibility */
	  warning (OPT_Wattributes,
		   "Ignoring unavailable space(psv) attribute for '%s'. ",
		   ident);
	  inform (input_location,
		  "Suggest space(prog) for this device instead.");
	  TREE_VALUE (args) = get_identifier ("prog");
	  return NULL_TREE;
	}
    }
  if (TYPE_P (*decl))
    {
      if (TREE_VALUE (args) == get_identifier ("prog"))
	{
	  return NULL_TREE;
	}
      else if (TREE_VALUE (args) == get_identifier ("data"))
	{
	  return NULL_TREE;
	}
#if 0
      else if (TREE_VALUE(args) == get_identifier("serial_mem"))
        {
          if (FUNCTION_DECL == TREE_CODE(*decl))
            {
              warning (OPT_Wattributes, "Ignoring space(serial_mem) attribute for function %qs. Suggest space(prog) for functions.", ident);
            }
          return NULL_TREE;
        }
#endif
      else if ((TREE_VALUE (args) == get_identifier ("psv"))
	       || (TREE_VALUE (args) == get_identifier ("auto_psv")))
	{
	  /* Accept for C30 compatibility */
	  warning (OPT_Wattributes,
		   "Ignoring unavailable space(psv) attribute for '%s'. ",
		   ident);
	  inform (input_location,
		  "Suggest space(prog) for this device instead.");
	  TREE_VALUE (args) = get_identifier ("prog");
	  return NULL_TREE;
	}
    }
  warning (OPT_Wattributes,
	   "invalid space argument for '%s', ignoring attribute", attached_to);
  *no_add_attrs = 1;
  return NULL_TREE;
}

/* tcm attribute */
tree
pic32c_tcm_attribute (tree *decl, tree identifier ATTRIBUTE_UNUSED, tree args,
		      int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  const char *attached_to = 0;

  if (DECL_P (*decl))
    {
      attached_to = IDENTIFIER_POINTER (DECL_NAME (*decl));
    }

  if (NULL == DECL_NAME (*decl))
    {
      warning (OPT_Wattributes, "Ignoring tcm attribute due to missing "
				"declaration. Check placement of attribute.");
      return NULL_TREE;
    }

  if (ignore_attribute ("tcm", attached_to, *decl))
    {
      *no_add_attrs = 1;
      return NULL_TREE;
    }

  if ((TREE_CODE (*decl) == FUNCTION_DECL) && !(TARGET_MCHP_ITCM || TARGET_MCHP_TCM)) {
    warning (OPT_Wattributes, "Ignoring tcm attribute. "
              "Pass -mitcm or -mtcm as appropriate for your target device to enable TCM usage.");
    return NULL_TREE;
  }
  else if (!(TARGET_MCHP_DTCM || TARGET_MCHP_TCM)) {
    warning (OPT_Wattributes, "Ignoring tcm attribute. "
              "Pass -mdtcm or -mtcm as appropriate for your target device to enable TCM usage.");
    return NULL_TREE;
  }

  /* tcm attribute on a function implies noinline and noclone */
  if (TREE_CODE (*decl) == FUNCTION_DECL)
    {
      tree attrib_noinline, attrib_noclone;
      if (lookup_attribute ("noinline", DECL_ATTRIBUTES (*decl)) == NULL)
      {
        attrib_noinline
          = build_tree_list (get_identifier ("noinline"), NULL_TREE);
        attrib_noinline = chainon (DECL_ATTRIBUTES (*decl), attrib_noinline);
        decl_attributes (decl, attrib_noinline, 0);
      }
      if (lookup_attribute ("noclone", DECL_ATTRIBUTES (*decl)) == NULL)
      {
        attrib_noclone
          = build_tree_list (get_identifier ("noclone"), NULL_TREE);
        attrib_noclone = chainon (DECL_ATTRIBUTES (*decl), attrib_noclone);
        decl_attributes (decl, attrib_noclone, 0);
      }
    }

  return NULL_TREE;
}

/* Handle a "unique_section" attribute; arguments as in
   struct attribute_spec.handler.
*/
tree
pic32c_unique_section_attribute (tree *node, tree name,
                                 tree args ATTRIBUTE_UNUSED,
                                 int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree decl = *node;

  if ((TREE_CODE (decl) == FUNCTION_DECL
        || TREE_CODE (decl) == VAR_DECL))
    {
      if (TREE_CODE (decl) == VAR_DECL
          && current_function_decl != NULL_TREE
          && ! TREE_STATIC (decl))
        {
          error ("%qE attribute cannot be specified for "
                  "local variable '%D'", name, decl);
          *no_add_attrs = true;
        }

        DECL_UNIQUE_SECTION (decl) = 1;
        DECL_COMMON (decl) = 0;
    }
  else
    {
      error ("%qE attribute not allowed for '%D'", name, decl);
      *no_add_attrs = true;
    }

  return NULL_TREE;
}

/* keep attribute to prevent removal by linker's --gc-sections feature */
tree
pic32c_keep_attribute (tree *decl, tree identifier ATTRIBUTE_UNUSED, tree args,
		       int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  DECL_COMMON (*decl) = 0;
  DECL_UNIQUE_SECTION (*decl) = 1;
  return NULL_TREE;
}

/* nopa attribute to prevent PA on a specific section */
tree
pic32c_nopa_attribute (tree *decl, tree identifier ATTRIBUTE_UNUSED, tree args,
		       int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  DECL_COMMON (*decl) = 0;
  DECL_UNIQUE_SECTION (*decl) = 1;
  return NULL_TREE;
}

tree pic32c_persistent_attribute(tree *node, tree identifier ATTRIBUTE_UNUSED,
                                 tree args, int flags ATTRIBUTE_UNUSED,
                                 bool *no_add_attrs)
{
  const char *attached_to = 0;

  DECL_UNIQUE_SECTION (*node) = 1;

  if (flag_pic || flag_pie)
    {
      warning (0, "'persistent' attribute cannot be specified with -fpic, -fPIC or -fpie option");
      *no_add_attrs = 1;
      return NULL_TREE;
    }

  if (DECL_P(*node))
    {
      attached_to = IDENTIFIER_POINTER(DECL_NAME(*node));
    }
    // TODO: "coherent" is N/A on PIC32C
#if 0
  /* The persistent attribute implies coherent for variables. */
    // Don't worry about the initialization value specified for persistent,
    // it will be et to NULL.
    // TODO: The .pbss section will not be created now. Is it required?

  if ((TREE_CODE(*node) == VAR_DECL) &&
//       !DECL_INITIAL(*node) &&
      (lookup_attribute ("coherent", DECL_ATTRIBUTES(*node)) == NULL) )
    {
      tree attrib_coherent = build_tree_list (get_identifier ("coherent"), NULL_TREE);
      attrib_coherent = chainon (DECL_ATTRIBUTES(*node), attrib_coherent);
      decl_attributes (node, attrib_coherent, 0);
      DECL_COMMON (*node) = 0;
    }
#endif
  return NULL_TREE;
}

/* PIC32C target specific builtin function registration */

/* Get the smartio (_S) target builtin declaration from the given
   standard library builtin index. */
tree
mchp_get_smartio_fndecl (int bx)
{
  switch (bx)
    {
    case BUILT_IN_FPRINTF: 
    case BUILT_IN_FPRINTF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_FPRINTF_S];
    case BUILT_IN_SPRINTF: 
    case BUILT_IN_SPRINTF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_SPRINTF_S];
    case BUILT_IN_SNPRINTF: 
    case BUILT_IN_SNPRINTF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_SNPRINTF_S];
    case BUILT_IN_VFPRINTF: 
    case BUILT_IN_VFPRINTF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_VFPRINTF_S];
    case BUILT_IN_VSPRINTF: 
    case BUILT_IN_VSPRINTF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_VSPRINTF_S];
    case BUILT_IN_VSNPRINTF: 
    case BUILT_IN_VSNPRINTF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_VSNPRINTF_S];
    case BUILT_IN_FSCANF: 
    case BUILT_IN_FSCANF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_FSCANF_S];
    case BUILT_IN_SSCANF: 
    case BUILT_IN_SSCANF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_SSCANF_S];
    case BUILT_IN_VFSCANF: 
    case BUILT_IN_VFSCANF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_VFSCANF_S];
    case BUILT_IN_VSSCANF: 
    case BUILT_IN_VSSCANF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_VSSCANF_S];
    case BUILT_IN_PRINTF: 
    case BUILT_IN_PRINTF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_PRINTF_S];
    case BUILT_IN_VPRINTF: 
    case BUILT_IN_VPRINTF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_VPRINTF_S];
    case BUILT_IN_SCANF: 
    case BUILT_IN_SCANF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_SCANF_S];
    case BUILT_IN_VSCANF: 
    case BUILT_IN_VSCANF_INTEGER:
      return pic32c_builtin_decls[PIC32C_BUILTIN_VSCANF_S];
    default:
      gcc_unreachable ();
    }
  return NULL_TREE;
}

void
pic32c_subtarget_init_builtins ()
{
  extern void mchp_print_builtin_function (tree);
  tree const_str_type = build_pointer_type (
    build_qualified_type (char_type_node, TYPE_QUAL_CONST));
  tree str_type = build_pointer_type (char_type_node);
  tree printf_s_ftype
    = build_varargs_function_type_list (intSI_type_node, const_str_type,
					const_str_type, NULL_TREE);
  tree fprintf_s_ftype
    = build_varargs_function_type_list (intSI_type_node, const_str_type,
					fileptr_type_node, const_str_type,
					NULL_TREE);
  tree sprintf_s_ftype
    = build_varargs_function_type_list (intSI_type_node, const_str_type,
					str_type, const_str_type, NULL_TREE);
  tree snprintf_s_ftype
    = build_varargs_function_type_list (intSI_type_node, const_str_type,
					str_type, size_type_node,
					const_str_type, NULL_TREE);
  tree vprintf_s_ftype
    = build_function_type_list (intSI_type_node, const_str_type, const_str_type,
				va_list_type_node, NULL_TREE);
  tree vfprintf_s_ftype
    = build_function_type_list (intSI_type_node, const_str_type,
				fileptr_type_node, const_str_type,
				va_list_type_node, NULL_TREE);
  tree vsprintf_s_ftype
    = build_function_type_list (intSI_type_node, const_str_type, str_type,
				const_str_type, va_list_type_node, NULL_TREE);
  tree vsnprintf_s_ftype
    = build_function_type_list (intSI_type_node, const_str_type, str_type,
				size_type_node, const_str_type,
				va_list_type_node, NULL_TREE);
  tree void_ftype = build_function_type_list (void_type_node, NULL_TREE);

  pic32c_builtin_decls[PIC32C_BUILTIN_NOP]
    = add_builtin_function ("__builtin_nop", void_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (PIC32C_BUILTIN_NOP),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_SOFTWARE_BREAKPOINT]
    = add_builtin_function ("__builtin_software_breakpoint", void_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_SOFTWARE_BREAKPOINT),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_FPRINTF_S]
    = add_builtin_function ("__builtin_fprintf_s", fprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_FPRINTF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_SPRINTF_S]
    = add_builtin_function ("__builtin_sprintf_s", sprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_SPRINTF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_SNPRINTF_S]
    = add_builtin_function ("__builtin_snprintf_s", snprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_SNPRINTF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_VFPRINTF_S]
    = add_builtin_function ("__builtin_vfprintf_s", vfprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_VFPRINTF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_VSPRINTF_S]
    = add_builtin_function ("__builtin_vsprintf_s", vsprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_VSPRINTF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_VSNPRINTF_S]
    = add_builtin_function ("__builtin_vsnprintf_s", vsnprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_VSNPRINTF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_FSCANF_S]
    = add_builtin_function ("__builtin_fscanf_s", fprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_FSCANF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_SSCANF_S]
    = add_builtin_function ("__builtin_sscanf_s", sprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_SSCANF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_VFSCANF_S]
    = add_builtin_function ("__builtin_vfscanf_s", vfprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_VFSCANF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_VSSCANF_S]
    = add_builtin_function ("__builtin_vsscanf_s", vsprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_VSSCANF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_PRINTF_S]
    = add_builtin_function ("__builtin_printf_s", printf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_PRINTF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_VPRINTF_S]
    = add_builtin_function ("__builtin_vprintf_s", vprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_VPRINTF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_SCANF_S]
    = add_builtin_function ("__builtin_scanf_s", printf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (PIC32C_BUILTIN_SCANF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);
  pic32c_builtin_decls[PIC32C_BUILTIN_VSCANF_S]
    = add_builtin_function ("__builtin_vscanf_s", vprintf_s_ftype,
			    BUILTIN_CODE_PIC32C_TO_ARM (
			      PIC32C_BUILTIN_VSCANF_S),
			    BUILT_IN_MD, NULL, NULL_TREE);

  if (TARGET_PRINT_BUILTINS || TARGET_WRITE_BUILTINS_H)
    {
      unsigned int i;
      for (i = 0; i != (unsigned int) PIC32C_BUILTIN_MAX; ++i)
	{
	  /* skip non-public smart-io builtins */
	  if (PIC32C_BUILTIN_SMARTIO_FIRST <= i
	      && PIC32C_BUILTIN_SMARTIO_LAST >= i)
	    continue;
	  mchp_print_builtin_function (pic32c_builtin_decls[i]);
	}
    }
}

tree
pic32c_subtarget_builtin_decl (unsigned code,
			       bool initialize_p ATTRIBUTE_UNUSED)
{
  unsigned int pic32c_code = BUILTIN_CODE_ARM_TO_PIC32C (code);

  if (pic32c_code >= PIC32C_BUILTIN_MAX)
    return error_mark_node;

  return pic32c_builtin_decls[pic32c_code];
}

static rtx
pic32c_expand_smartio_builtin (unsigned int pic32c_fcode, tree exp, rtx target)
{
  int nargs = call_expr_nargs (exp) - 1;
  int i;
  int sx;
  tree *buffer = XALLOCAVEC (tree, nargs);
  tree *argp = CALL_EXPR_ARGP (exp);
  tree fndecl;
  tree conv_spec, offset_node;
  char namebuf[SMARTIO_MAX_NAME] = {'\0'};

  /* get function information from table */
  gcc_assert (PIC32C_BUILTIN_SMARTIO_FIRST <= pic32c_fcode
	      && PIC32C_BUILTIN_SMARTIO_LAST >= pic32c_fcode);
  sx = PIC32C_SMARTIO_INDEX (pic32c_fcode);

  /* form name from base and suffix from conversion spec list. */
  sprintf (namebuf, "__%s_%s", smartio_fn_info[sx].base_name,
	   c_getstr (argp[0]));

  /* get fndecl for corresponding standard builtin */
  fndecl = builtin_decl_implicit (smartio_fn_info[sx].fcode);
  if (!fndecl)
    fndecl = builtin_decl_explicit (smartio_fn_info[sx].fcode);
  gcc_assert (fndecl);
  fndecl = build_fn_decl (namebuf, TREE_TYPE (fndecl));

  /* note nargs is the count for the final call, less the conversion spec info
   */
  for (i = 1; i <= nargs; ++i)
    buffer[i - 1] = argp[i];

  return expand_call (build_call_expr_loc_array (EXPR_LOCATION (exp), fndecl,
						 nargs, buffer),
		      target, target == const0_rtx);
}

rtx
pic32c_subtarget_expand_builtin (unsigned int fcode, tree exp, rtx target)
{
  unsigned int pic32c_fcode = BUILTIN_CODE_ARM_TO_PIC32C (fcode);

  gcc_assert (pic32c_fcode < PIC32C_BUILTIN_MAX);

  switch (pic32c_fcode)
    {
    case PIC32C_BUILTIN_NOP:
      emit_insn (gen_nop ());
      return target;

    case PIC32C_BUILTIN_SOFTWARE_BREAKPOINT:
      if (TARGET_THUMB1 || TARGET_THUMB2)
	{
	  emit_insn (gen_blockage ());
	  emit_insn (gen_pic32c_bkpt ());
	  emit_insn (gen_blockage ());
	}
      else
	{
	  warning (0,
		   "__builtin_software_breakpoint() supported only for Thumb1 or Thumb2");
	}
      return target;

    case PIC32C_BUILTIN_FPRINTF_S:
    case PIC32C_BUILTIN_SPRINTF_S:
    case PIC32C_BUILTIN_SNPRINTF_S:
    case PIC32C_BUILTIN_VFPRINTF_S:
    case PIC32C_BUILTIN_VSPRINTF_S:
    case PIC32C_BUILTIN_VSNPRINTF_S:
    case PIC32C_BUILTIN_FSCANF_S:
    case PIC32C_BUILTIN_SSCANF_S:
    case PIC32C_BUILTIN_VFSCANF_S:
    case PIC32C_BUILTIN_VSSCANF_S:
    case PIC32C_BUILTIN_PRINTF_S:
    case PIC32C_BUILTIN_VPRINTF_S:
    case PIC32C_BUILTIN_SCANF_S:
    case PIC32C_BUILTIN_VSCANF_S:
      return pic32c_expand_smartio_builtin (pic32c_fcode, exp, target);

    default:
      break;
    }

  return NULL_RTX;
}

/* This target hook implements the folding from gimple_fold_builtin for
   smart-io variants, which is a matter of adjusting the parameter positions
   to account for the additional smart-io format parameter. */
bool
pic32c_subtarget_gimple_fold_builtin (gimple_stmt_iterator *gsi)
{
  gimple *stmt = gsi_stmt (*gsi);
  tree call = gimple_call_fn (stmt);
  bool changed = false;

  if (call)
    {
      tree fndecl = gimple_call_fndecl (stmt);
      int fcode = BUILTIN_CODE_ARM_TO_PIC32C (DECL_FUNCTION_CODE (fndecl));
      unsigned n = gimple_call_num_args (stmt);

      switch (fcode)
	{
	case PIC32C_BUILTIN_SPRINTF_S:
	  return gimple_fold_builtin_sprintf (gsi, 1);

	case PIC32C_BUILTIN_SNPRINTF_S:
	  return gimple_fold_builtin_snprintf (gsi, 1);

	case PIC32C_BUILTIN_VFPRINTF_S:
	case PIC32C_BUILTIN_FPRINTF_S:
	  if (n == 3 || n == 4)
	    return gimple_fold_builtin_fprintf (gsi, gimple_call_arg (stmt, 1),
						gimple_call_arg (stmt, 2),
						n == 4
						  ? gimple_call_arg (stmt, 3)
						  : NULL_TREE,
						SMARTIO_TO_STDIO (fcode));
	  break;

	case PIC32C_BUILTIN_VPRINTF_S:
	case PIC32C_BUILTIN_PRINTF_S:
	  if (n == 2 || n == 3)
	    return gimple_fold_builtin_printf (gsi, gimple_call_arg (stmt, 1),
					       n == 3
						 ? gimple_call_arg (stmt, 2)
						 : NULL_TREE,
					       SMARTIO_TO_STDIO (fcode));
	  break;

	default:
	  break;
	}
    }
  return changed;
}

/*
 * Print the list of include-file search directories
 *  TARGET_PRINT_MCHP_SEARCH_DIRS is enabled by
 *  -mprint-mchp-search-dirs
 */
void
pic32c_final_include_paths (struct cpp_dir *quote, struct cpp_dir *bracket)
{
  struct cpp_dir *p;
  if (TARGET_PRINT_MCHP_SEARCH_DIRS)
    {
        for (p = quote;; p = p->next)
        {
            if (!p)
                break;
            fprintf (stderr, "compiler:%s\n", p->name);
        }
    }
}

static int
mchp_keep_p (tree decl)
{
  tree a;
  a = lookup_attribute ("keep", DECL_ATTRIBUTES (decl));
  return (a != NULL_TREE);
}

static int
mchp_persistent_p (tree decl)
{
  tree a;
  const char *sname = NULL;

  a = lookup_attribute ("persistent", DECL_ATTRIBUTES (decl));
  if (a == NULL_TREE)
  {
    sname = DECL_SECTION_NAME(decl);
    /* test user-specified ".pbss" or ".persist" attributes */
    if (sname &&
  (!strcmp (sname, SECTION_NAME_PERSIST) || !strcmp (sname, SECTION_NAME_PBSS)))
      return 1;
  }

  return a != NULL_TREE;
}

static int mchp_nopa_p (tree decl)
{
  tree a;
  a = lookup_attribute ("nopa", DECL_ATTRIBUTES (decl));
  return (a != NULL_TREE);
}

static tree
get_mchp_absolute_address (tree decl)
{
  tree retval;
  retval = lookup_attribute ("address", DECL_ATTRIBUTES (decl));

  return retval;
}

static tree
get_mchp_space_attribute (tree decl)
{
  return lookup_attribute ("space", DECL_ATTRIBUTES (decl));
}

static tree
get_pic32c_tcm_attribute (tree decl)
{
  if (!(TARGET_MCHP_ITCM || TARGET_MCHP_TCM))
    return NULL_TREE;

  return lookup_attribute ("tcm", DECL_ATTRIBUTES (decl));
}

static tree
get_pic32c_noload_attribute (tree decl)
{
  return lookup_attribute ("noload", DECL_ATTRIBUTES (decl));
}

static const char *
mchp_strip_name_encoding_helper (const char *symbol_name)
{
  const char *var;
  int sz = 0;

  var = symbol_name;
  while ((sz = ENCODED_NAME_P (var)))
    {
      var = var + ENCODED_NAME_P (symbol_name);
      var += (*var == '*');
    }
  return var;
}

bool
mchp_ramfunc_type_p (tree decl)
{
  bool retval = FALSE;
  tree space = 0;
  // tree region = 0;
  if (TREE_CODE (decl) == FUNCTION_DECL)
    {
      if (lookup_attribute ("ramfunc", TYPE_ATTRIBUTES (TREE_TYPE (decl)))
	  != NULL)
	{
	  retval = TRUE;
	}

      // region = lookup_attribute("region", DECL_ATTRIBUTES(decl));
      space = lookup_attribute ("space", DECL_ATTRIBUTES (decl));
      // if (region)
      // {
      // retval = FALSE ;
      // }
      if (space)
	{
	  if (get_identifier ("data") == (TREE_VALUE (TREE_VALUE (space))))
	    retval = TRUE;
	}
    }
  return retval;
}

/***********************************************************************
 * Section name processing.
 ***********************************************************************/

/*
 * Write a symbol name to a file, removing any special MCHP flags
 * encoded in the name.
 */
void
mchp_write_encoded_name (FILE *file, const char *symbol_name)
{
  const char *p = symbol_name;

  asm_fprintf(file, "%U");

  while ((*p != '\0') && (*p != MCHP_EXTENDED_FLAG[0]))
    ++p;

  fprintf(file, "%.*s", (int)(p - symbol_name), symbol_name);

  if (*p == '\0')
    return;

  const char *real_name = mchp_strip_name_encoding(p);
  fprintf(file, "%s", real_name);
}

const char *
mchp_strip_name_encoding (const char *symbol_name)
{
  //
  const char *var;
  //  mchp_interesting_fn *match;
  var = mchp_strip_name_encoding_helper (symbol_name);

/* PIC32C CHANGES */
#if 0     
  if (mchp_io_size_val > 0)
    {
      match = mchp_match_conversion_fn(var);
      while (match)
        {
          if (match->function_convertable)
            {

#define CCS_ADD_FLAG(FLAG)                                                     \
  if (match->conv_flags & JOIN (conv_, FLAG))                                  \
    {                                                                          \
      *f++ = #FLAG[0];                                                         \
      added = (mchp_conversion_status) (added | JOIN (conv_, FLAG));           \
    }

#define CCS_ADD_FLAG_ALT(FLAG, ALT)                                            \
  if ((match->conv_flags & JOIN (conv_, FLAG))                                 \
      && ((added & JOIN (conv_, ALT)) == 0))                                   \
    {                                                                          \
      *f++ = #ALT[0];                                                          \
      added = (mchp_conversion_status) (                                       \
	added | (mchp_conversion_status) JOIN (conv_, ALT));                   \
    }

              {
                char extra_flags[sizeof("_aAcdeEfFgGnopsuxX0")] = "_";
                char *f = &extra_flags[1];
                mchp_conversion_status added;
                /*
                 * order is important here
                 *  add new flags alphabetically with lower case preceding uppercase
                 *    ie _aAcdEfgG not
                 *       _acdfgAEG
                 */

                added = (mchp_conversion_status)0;
                /*
                 * we don't implement all 131K unique combinations, only
                 * a subset...
                */

                /* a | A -> aA */
                CCS_ADD_FLAG(a);
                CCS_ADD_FLAG_ALT(A,a);
                CCS_ADD_FLAG(A);
                CCS_ADD_FLAG_ALT(a,A);

                /* c | d | n | o | p | u | x | X -> cdnopuxX */
                CCS_ADD_FLAG(c);
                CCS_ADD_FLAG_ALT(d,c);
                CCS_ADD_FLAG_ALT(n,c);
                CCS_ADD_FLAG_ALT(o,c);
                CCS_ADD_FLAG_ALT(p,c);
                CCS_ADD_FLAG_ALT(u,c);
                CCS_ADD_FLAG_ALT(x,c);
                CCS_ADD_FLAG_ALT(X,c);

                /* c | d | n | o | p | u | x | X -> cdnopuxX */
                CCS_ADD_FLAG(d);
                CCS_ADD_FLAG_ALT(c,d);
                CCS_ADD_FLAG_ALT(n,d);
                CCS_ADD_FLAG_ALT(o,d);
                CCS_ADD_FLAG_ALT(p,d);
                CCS_ADD_FLAG_ALT(u,d);
                CCS_ADD_FLAG_ALT(x,d);
                CCS_ADD_FLAG_ALT(X,d);

                /* e | E -> eE */
                CCS_ADD_FLAG(e);
                CCS_ADD_FLAG_ALT(E,e);
                CCS_ADD_FLAG(E);
                CCS_ADD_FLAG_ALT(e,E);

                /* f | F -> fF */
                CCS_ADD_FLAG(f);
                CCS_ADD_FLAG_ALT(F,f);
                CCS_ADD_FLAG(F);
                CCS_ADD_FLAG_ALT(f,F);

                /* g | G -> gG */
                CCS_ADD_FLAG(g);
                CCS_ADD_FLAG_ALT(G,g);
                CCS_ADD_FLAG(G);
                CCS_ADD_FLAG_ALT(g,G);

                /* c | d | n | o | p | u | x | X -> cdnopuxX */
                CCS_ADD_FLAG(n);
                CCS_ADD_FLAG_ALT(c,n);
                CCS_ADD_FLAG_ALT(d,n);
                CCS_ADD_FLAG_ALT(n,n);
                CCS_ADD_FLAG_ALT(o,n);
                CCS_ADD_FLAG_ALT(p,n);
                CCS_ADD_FLAG_ALT(u,n);
                CCS_ADD_FLAG_ALT(x,n);
                CCS_ADD_FLAG_ALT(X,n);

                /* c | d | n | o | p | u | x | X -> cdnopuxX */
                CCS_ADD_FLAG(o);
                CCS_ADD_FLAG_ALT(c,o);
                CCS_ADD_FLAG_ALT(d,o);
                CCS_ADD_FLAG_ALT(n,o);
                CCS_ADD_FLAG_ALT(o,o);
                CCS_ADD_FLAG_ALT(p,o);
                CCS_ADD_FLAG_ALT(u,o);
                CCS_ADD_FLAG_ALT(x,o);
                CCS_ADD_FLAG_ALT(X,o);

                CCS_ADD_FLAG(p);
                CCS_ADD_FLAG_ALT(c,p);
                CCS_ADD_FLAG_ALT(d,p);
                CCS_ADD_FLAG_ALT(n,p);
                CCS_ADD_FLAG_ALT(o,p);
                CCS_ADD_FLAG_ALT(p,p);
                CCS_ADD_FLAG_ALT(u,p);
                CCS_ADD_FLAG_ALT(x,p);
                CCS_ADD_FLAG_ALT(X,p);

                CCS_ADD_FLAG(s);

                CCS_ADD_FLAG(u);
                CCS_ADD_FLAG_ALT(c,u);
                CCS_ADD_FLAG_ALT(d,u);
                CCS_ADD_FLAG_ALT(n,u);
                CCS_ADD_FLAG_ALT(o,u);
                CCS_ADD_FLAG_ALT(p,u);
                CCS_ADD_FLAG_ALT(u,u);
                CCS_ADD_FLAG_ALT(x,u);
                CCS_ADD_FLAG_ALT(X,u);

                CCS_ADD_FLAG(x);
                CCS_ADD_FLAG_ALT(c,x);
                CCS_ADD_FLAG_ALT(d,x);
                CCS_ADD_FLAG_ALT(n,x);
                CCS_ADD_FLAG_ALT(o,x);
                CCS_ADD_FLAG_ALT(p,x);
                CCS_ADD_FLAG_ALT(u,x);
                CCS_ADD_FLAG_ALT(x,x);
                CCS_ADD_FLAG_ALT(X,x);

                CCS_ADD_FLAG(X);
                CCS_ADD_FLAG_ALT(c,X);
                CCS_ADD_FLAG_ALT(d,X);
                CCS_ADD_FLAG_ALT(n,X);
                CCS_ADD_FLAG_ALT(o,X);
                CCS_ADD_FLAG_ALT(p,X);
                CCS_ADD_FLAG_ALT(u,X);
                CCS_ADD_FLAG_ALT(x,X);
                CCS_ADD_FLAG_ALT(X,X);
                *f++=0;

                if (strlen(extra_flags) > 1)
                  {
                    if (match->encoded_name = NULL)
                      free(match->encoded_name);
                    match->encoded_name = (char*)xmalloc(strlen(match->map_to) +
                    strlen(extra_flags) + 1);
                    sprintf(match->encoded_name,"%s%s", match->map_to, extra_flags);
                  }
                else
                  {
                    /* we have no flags */
                    match->encoded_name = (char*)xmalloc(strlen(match->map_to) + 3);
                    sprintf(match->encoded_name,"%s_0", match->map_to);
                  }
              }
              if (match->encoded_name) return match->encoded_name;
            }
          if (match[1].name &&
              (strcmp(match[1].name,var) == 0)) match++;
          else match = 0;
        }
    }
#endif
  return var;
}

static int
mchp_build_prefix (tree decl, int fnear, char *prefix)
{
  char *f = prefix;

  tree address_attr = 0;
  tree space_attr = 0;
  tree tcm_attr = 0;
  tree noload_attr = 0;

  SECTION_FLAGS_INT flags = 0;
  const char *ident;
  int section_type_set = 0;

  location_t loc = DECL_SOURCE_LOCATION(decl);

  if (fnear == -1)
    {
      section_type_set = 1;
      fnear = 0;
    }

  address_attr = get_mchp_absolute_address (decl);
  space_attr = get_mchp_space_attribute (decl);
  tcm_attr = get_pic32c_tcm_attribute (decl);
  noload_attr = get_pic32c_noload_attribute (decl);

  if (DECL_SECTION_NAME (decl))
    {
      const char *name = DECL_SECTION_NAME (decl);

      flags = mchp_section_type_flags (0, name, 1);

      if (((flags & SECTION_WRITE) && (!DECL_INITIAL (decl)))
	  || ((flags & SECTION_WRITE) && bss_initializer_p (decl)))
	{
	  flags &= ~(SECTION_FLAGS_INT) SECTION_WRITE;
	  flags |= (SECTION_FLAGS_INT) SECTION_BSS;
	}
    }
  else
    {
      if (address_attr)
	{
	  set_decl_section_name (decl, "*");
	  if (TREE_CODE (decl) == VAR_DECL && DECL_SECTION_NAME (decl) != NULL
	      && DECL_INITIAL (decl) == NULL_TREE && DECL_COMMON (decl))
	    DECL_COMMON (decl) = 0;
	}

      if (TREE_CODE (decl) == STRING_CST && !tcm_attr)
	flags |= SECTION_READ_ONLY;
    }

  if (TREE_CODE (decl) == VAR_DECL)
    {
      if (!space_attr && !tcm_attr && TREE_READONLY (decl)
	  && (DECL_INITIAL (decl) || (DECL_EXTERNAL (decl))))
	{
	  flags |= SECTION_READ_ONLY;
	}

      if ((flags & SECTION_PERSIST)  || (mchp_persistent_p(decl)))
        {
          f += sprintf(f, MCHP_PRST_FLAG);
          section_type_set = 1;
          DECL_COMMON (decl) = 0;
          if (DECL_INITIAL(decl))
            {
                if (DECL_NAME(decl) != NULL_TREE)
                  {
                    ident = IDENTIFIER_POINTER(DECL_NAME(decl));
                    warning(0, "Persistent variable '%s' will not be initialized",
                            ident);
                  }
                else
                  {
                    warning(0, "Persistent variable will not be initialized");
                  }
                /* Persistent variables will not be initialized. So, make them as bss sections. */
                DECL_INITIAL(decl) = NULL_TREE;
            }
        }
    }

  if (address_attr)
    {
      if (flags & SECTION_ADDRESS)
	{
	  if (DECL_NAME (decl) != NULL_TREE)
	    {
	      ident = IDENTIFIER_POINTER (DECL_NAME (decl));
	      warning (
		0,
		"address attribute conflicts with section attribute for '%s'",
		ident);
	    }
	  else
	    {
	      warning (0, "address attribute conflicts with section attribute");
	    }
	}
      else
	f += sprintf (f, MCHP_ADDR_FLAG);
    }

  if (noload_attr)
    {
      f += sprintf (f, MCHP_NOLOAD_FLAG);
    }

  if (tcm_attr)
    {
      if (TREE_CODE (decl) == VAR_DECL)
	{
	  if (!DECL_EXTERNAL (decl))
	    {
	      if (!DECL_INITIAL (decl) || bss_initializer_p (decl))
		f += sprintf (f, MCHP_BSS_FLAG);
	      else
		f += sprintf (f, MCHP_DATA_FLAG);
	    }

	  f += sprintf (f, MCHP_DTCM_FLAG);
	}
      else if (TREE_CODE (decl) == FUNCTION_DECL)
	{
	  f += sprintf (f, MCHP_ITCM_FLAG);
	}

      fnear = 0;
      DECL_COMMON (decl) = 0;
      section_type_set = 1;
    }


  if ((flags & SECTION_KEEP) || mchp_keep_p (decl))
    {
      DECL_COMMON (decl) = 0;
      f += sprintf (f, MCHP_KEEP_FLAG);
    }

  if ((flags & SECTION_NOPA) || mchp_nopa_p(decl)) {
    DECL_COMMON(decl) = 0;
    f += sprintf(f, MCHP_NOPA_FLAG);
  }

  fnear |= (flags & SECTION_NEAR);
  if ((flags & SECTION_CODE)
      || (space_attr
	  && (get_identifier ("prog")
	      == (TREE_VALUE (TREE_VALUE (space_attr))))))
    {
      if (TREE_CODE (decl) == VAR_DECL)
	f += sprintf (f, MCHP_CONST_FLAG);
      else
	f += sprintf (f, MCHP_PROG_FLAG);

      fnear = 0;
      DECL_COMMON (decl) = 0;
      section_type_set = 1;
    }

  if ((flags & SECTION_WRITE)
      || (space_attr
	  && (get_identifier ("data")
	      == (TREE_VALUE (TREE_VALUE (space_attr))))))
    {
      if (TREE_CODE (decl) == FUNCTION_DECL)
	{
	  f += sprintf (f, MCHP_RAMFUNC_FLAG);
	}
      else
	{
	  f += sprintf (f, MCHP_DATA_FLAG);
	}
      section_type_set = 1;
    }

  /* we can't ask for a BSS section apart from by using the old naming
    convention or flags in a section directive - but if it is there, we
    should honour it */
  if (flags & SECTION_BSS)
    {
      f += sprintf (f, MCHP_BSS_FLAG);
      section_type_set = 1;
      flags &= ~(SECTION_FLAGS_INT) SECTION_WRITE;
    }
  if (flags & SECTION_READ_ONLY)
    {
      f += sprintf (f, MCHP_CONST_FLAG);
      fnear = 0;
      section_type_set = 1;
    }

  if ((!section_type_set) && (!DECL_EXTERNAL (decl)))
    {
      if (!DECL_INITIAL (decl) || bss_initializer_p (decl))
	f += sprintf (f, MCHP_BSS_FLAG);
      else
	f += sprintf (f, MCHP_DATA_FLAG);
    }
  if ((fnear) && !(flags & (SECTION_INFO)))
    {
      f += sprintf (f, MCHP_NEAR_FLAG);
    }
  if (mchp_keep_p (decl))
    {
      f += sprintf (f, MCHP_KEEP_FLAG);
    }
  if (mchp_nopa_p (decl))
    {
      f += sprintf (f, MCHP_NOPA_FLAG);
    }

    // TODO: "coherent" is N/A on PIC32C
#if 0
  if (mchp_coherent_p (decl)
    || (flags & SECTION_PERSIST) || mchp_persistent_p(decl))
     /* add implicit coherent flag when persistent is specified */
  {
    f += sprintf(f, MCHP_COHERENT_FLAG);
  }
#endif
  return fnear;
}

static SECTION_FLAGS_INT
section_flags_from_decl (tree decl)
{
  char prefix[120] = {0};
  int fNear;
  const char *fn_name;
  char *f = prefix;
  SECTION_FLAGS_INT flags = 0;

  /* TODO should this be here */
  switch (TREE_CODE (decl))
    {
    case FUNCTION_DECL:
      fNear = 0;
      fn_name = IDENTIFIER_POINTER (DECL_NAME (decl));

      f += sprintf (f, MCHP_FCNN_FLAG);
      mchp_build_prefix (decl, -1, f);
      break;

    case VAR_DECL:
      fNear = 0;
      if (TREE_READONLY (decl))
	{
	  /*
	  ** If this is a constant declaration,
	  ** and constants are located in code space,
	  ** then it cannot be a near declaration.
	  */
	  fNear = 0;
	}

      mchp_build_prefix (decl, fNear, prefix);
      break;

    default:
      // Issue XC32-752: if 'decl' is not really a DECL, there would be no flags to be
      // extracted from it and calling validate_identifier_flags() would result in an ICE
      return 0;
    }

  if ((prefix[0] != 0) == 0)
    fNear = 0;

  flags = validate_identifier_flags (prefix, decl);

  return flags;
}

/* validate prefix before an identifier */
static SECTION_FLAGS_INT
validate_identifier_flags (const char *prefix, tree decl)
{
  const char *f = prefix;
  SECTION_FLAGS_INT flags = 0;
  int add_section_code_flag = 0;
  struct valid_section_flags_ *v_flags = 0;
  const char *id = NULL;

  gcc_assert (sizeof (SECTION_FLAGS_INT) >= sizeof (uint64_t));

  if (DECL_NAME (decl) != NULL_TREE)
    id = IDENTIFIER_POINTER (DECL_NAME (decl));

  while (f && *f == MCHP_EXTENDED_FLAG[0])
    {
      if (strncmp (f, MCHP_PROG_FLAG, sizeof (MCHP_PROG_FLAG) - 1) == 0)
	{
	  flags |= SECTION_CODE;
	  f += sizeof (MCHP_PROG_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_RAMFUNC_FLAG, sizeof (MCHP_RAMFUNC_FLAG) - 1)
	       == 0)
	{
	  flags |= SECTION_RAMFUNC;
	  f += sizeof (MCHP_RAMFUNC_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_FCNN_FLAG, sizeof (MCHP_FCNN_FLAG) - 1) == 0)
	{
	  add_section_code_flag++;
	  f += sizeof (MCHP_FCNN_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_FCNS_FLAG, sizeof (MCHP_FCNS_FLAG) - 1) == 0)
	{
	  add_section_code_flag++;
	  f += sizeof (MCHP_FCNS_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_DATA_FLAG, sizeof (MCHP_DATA_FLAG) - 1) == 0)
	{
	  flags |= SECTION_WRITE;
	  f += sizeof (MCHP_DATA_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_CONST_FLAG, sizeof (MCHP_CONST_FLAG) - 1) == 0)
	{
	  flags |= SECTION_READ_ONLY;
	  f += sizeof (MCHP_CONST_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_SERIALMEM_FLAG,
			sizeof (MCHP_SERIALMEM_FLAG) - 1)
	       == 0)
	{
	  flags |= SECTION_SERIALMEM;
	  f += sizeof (MCHP_SERIALMEM_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_PRST_FLAG, sizeof (MCHP_PRST_FLAG) - 1) == 0)
	{
	  flags |= SECTION_PERSIST;
	  f += sizeof (MCHP_PRST_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_NEAR_FLAG, sizeof (MCHP_NEAR_FLAG) - 1) == 0)
	{
	  flags |= SECTION_NEAR;
	  f += sizeof (MCHP_NEAR_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_ADDR_FLAG, sizeof (MCHP_ADDR_FLAG) - 1) == 0)
	{
	  flags |= SECTION_ADDRESS;
	  f += sizeof (MCHP_ADDR_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_NOLOAD_FLAG, sizeof (MCHP_NOLOAD_FLAG) - 1)
	       == 0)
	{
	  flags |= SECTION_NOLOAD;
	  f += sizeof (MCHP_NOLOAD_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_BSS_FLAG, sizeof (MCHP_BSS_FLAG) - 1) == 0)
	{
	  flags |= SECTION_BSS;
	  f += sizeof (MCHP_BSS_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_KEEP_FLAG, sizeof (MCHP_KEEP_FLAG) - 1) == 0)
	{
	  flags |= SECTION_KEEP;
	  f += sizeof (MCHP_KEEP_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_COHERENT_FLAG, sizeof (MCHP_COHERENT_FLAG) - 1)
	       == 0)
	{
	  flags |= SECTION_COHERENT;
	  f += sizeof (MCHP_COHERENT_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_REGION_FLAG, sizeof (MCHP_REGION_FLAG) - 1)
	       == 0)
	{
	  flags |= SECTION_REGION;
	  f += sizeof (MCHP_REGION_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_ITCM_FLAG, sizeof (MCHP_ITCM_FLAG) - 1) == 0)
	{
	  flags |= SECTION_ITCM;
	  f += sizeof (MCHP_ITCM_FLAG) - 1;
	}
      else if (strncmp (f, MCHP_DTCM_FLAG, sizeof (MCHP_DTCM_FLAG) - 1) == 0)
	{
	  flags |= SECTION_DTCM;
	  f += sizeof (MCHP_DTCM_FLAG) - 1;
	}
       else if (strncmp (f, MCHP_NOPA_FLAG, sizeof (MCHP_NOPA_FLAG) - 1) == 0)
	{
	  flags |= SECTION_NOPA;
	  f += sizeof (MCHP_NOPA_FLAG) - 1;
	}
       else
	{
	  if (id)
	    {
	      error ("Could not determine flags for identifier: '%s'", id);
	    }
	  else
	    {
	      error ("Could not determine identifier flags");
	    }

	  return flags;
	}
    }
  if (add_section_code_flag && !(flags & SECTION_RAMFUNC)
      && !(flags & SECTION_REGION))
    flags |= SECTION_CODE;

  /* check flags are valid */
  for (v_flags = valid_section_flags; v_flags->flag_name; v_flags++)
    {
      if ((flags & v_flags->flag_mask) && (flags & v_flags->incompatible_with))
	{
	  if (id)
	    {
	      error ("Incompatible section flags for identifier '%s'", id);
	    }
	  else if (IN_NAMED_SECTION (decl))
	    {
	      error ("Incompatible section flags for identifier in section '%s",
		     DECL_SECTION_NAME (decl));
	    }
	  else
	    {
	      error ("Incompatible section flags");
	    }

	  return flags;
	}
    }

  return flags;
}

static int
mchp_rsn_compare (const void *va, const void *vb)
{
  const struct reserved_section_names_ *m
    = (const struct reserved_section_names_ *) va;
  const struct reserved_section_names_ *n
    = (const struct reserved_section_names_ *) vb;

  if (m != NULL && n != NULL)
    return strcmp (m->section_name, n->section_name);

  return 0;
}

static int
mchp_vsf_compare (const void *va, const void *vb)
{
  const struct valid_section_flags_ *f
    = (const struct valid_section_flags_ *) va;
  const struct valid_section_flags_ *g
    = (const struct valid_section_flags_ *) vb;

  if (f != NULL && g != NULL)
    return strncmp (f->flag_name, g->flag_name, strlen (g->flag_name));

  return 0;
}

static struct reserved_section_names_ *
pic32c_find_in_rsn (const char *name)
{
  struct reserved_section_names_ *r_section = NULL;

  for (r_section = reserved_section_names; r_section && r_section->section_name;
       r_section++)
    {
      if (strcmp (r_section->section_name, name) == 0)
	return r_section;
    }

  return NULL;
}

static struct valid_section_flags_ *
pic32c_find_in_vsf (const char *name)
{
  struct valid_section_flags_ *v_flags = NULL;

  for (v_flags = valid_section_flags; v_flags && v_flags->flag_name; v_flags++)
    {
      if (strcmp (v_flags->flag_name, name) == 0)
	return v_flags;
    }

  return NULL;
}

/* validates a section declaration based on its name and any flags */
static SECTION_FLAGS_INT
validate_section_flags (const char *name, SECTION_FLAGS_INT attr_flags)
{
  SECTION_FLAGS_INT set_flags = attr_flags;
  struct reserved_section_names_ *r_section = 0;
  struct valid_section_flags_ *v_flags = 0;
  char *f, *fe, comma = 0;
  char *flags = NULL;
  int first_flag = 1;
  int i;
  static bool sorted = FALSE;

  f = 0;
  flags = (char *) strchr (name, ',');
  if (flags)
    {
      *flags = 0;
      f = flags + 1;
      comma = ',';
    }

  /* sort before doing bsearch */
  if (!sorted)
    {
      qsort (reserved_section_names, num_rsn, sizeof (reserved_section_names_),
	     mchp_rsn_compare);

      qsort (valid_section_flags, num_vsf, sizeof (struct valid_section_flags_),
	     mchp_vsf_compare);

      sorted = TRUE;
    }

  /* FIXME: is this bsearch called on arrays in ascending order??? */
  if (name)
    {
      r_section = NULL;
      key_rsn.section_name = name;
      r_section = (struct reserved_section_names_ *)
	bsearch (&key_rsn, reserved_section_names, num_rsn,
		 sizeof (struct reserved_section_names_), mchp_rsn_compare);
    }

  if (r_section)
    {
      set_flags |= r_section->mask;
    }

  if (f)
    do
      {
	fe = strchr (f, ',');
	if (fe)
	  {
	    *fe = 0;
	  }
	/* nasty safe-ctype.h means that we can't use isspace */
	while (*f && ISSPACE (*f))
	  f++;
	if (*f)
	  {
	    /* FIXME: is this bsearch called on arrays in ascending order??? */
	    key_vsf.flag_name = f;
	    v_flags = (struct valid_section_flags_ *)
	      bsearch (&key_vsf, valid_section_flags, num_vsf,
		       sizeof (struct valid_section_flags_), mchp_vsf_compare);

	    if (!v_flags)
	      {
		if (first_flag)
		  {
		    char *s;

		    for (s = f; *s; s++)
		      {
			for (v_flags = valid_section_flags; v_flags->flag_name;
			     v_flags++)
			  {
			    if (*s == v_flags->single_letter_equiv)
			      {
				if (v_flags->single_letter_equiv == 'b')
				  {
				    /* this may be .pbss,b for persistent */
				    if ((strncmp (name, SECTION_NAME_PBSS, 5)
					 == 0)
					&& ((v_flags->flag_mask
					     & SECTION_PERSIST)
					    == 0))
				      continue;
				  }
				else if (v_flags->single_letter_equiv == 'r')
				  {
				    /* 'r' used to be used for .eedata - don't
				       set READ_ONLY for .eedata section */
				    if (strncmp (name, ".eedata", 7) == 0)
				      break;
				  }
				set_flags |= v_flags->flag_mask;
				break;
			      }
			  }
			if (!v_flags->flag_name)
			  {
			    warning (
			      0, "'%c': unrecognized old-style section flag",
			      *s);
			    break;
			  }
			*s = ' ';
			comma = ' ';
		      }
		    first_flag = 0;
		  }
		else
		  warning (0, "'%s': unrecognized section flag", f);
	      }
	    else
	      {
		set_flags |= v_flags->flag_mask;
	      }
	    if (fe)
	      {
		*fe = ',';
		f = fe + 1;
	      }
	    else
	      break;
	  }
	else
	  break;
      }
    while (1);
  if (flags)
    *flags = comma;
  for (v_flags = valid_section_flags; v_flags->flag_name; v_flags++)
    {
      if ((set_flags & v_flags->flag_mask)
	  && (set_flags & v_flags->incompatible_with))
	{
	  error ("incompatible section flags for section '%s'", name);
	  return set_flags;
	}
    }
  return set_flags;
}

/*
** unsigned int
** TARGET_SECTION_TYPE_FLAGS(tree decl, const char *name, int reloc);
**
** A target hook to choose a set of section attributes for use by
** TARGET_ASM_NAMED_SECTION based on a variable or function decl,
** a section name, and whether or not the declaration's initializer
** may contain runtime relocations.
**
** <decl> is either a FUNCTION_DECL, a VAR_DECL or NULL_TREE.
** If <decl> is null, read-write data should be assumed.
** <reloc> indicates whether the initial value of exp requires
** link-time relocations.
*/
SECTION_FLAGS_INT
mchp_section_type_flags (tree decl, const char *name,
			 int reloc ATTRIBUTE_UNUSED)
{

  SECTION_FLAGS_INT flag = 0;
  rtx rtl;

  if (decl)
    {
      flag = section_flags_from_decl (decl);
    }

  if (name)
    flag = validate_section_flags (name, flag);

  return flag;
}

void
pic32c_subtarget_encode_section_info (tree decl, rtx rtl,
				      int first_seen ATTRIBUTE_UNUSED)
{
  char prefix[120] = {0};
  int fNear;
  const char *fn_name;
  char *f = prefix;

  switch (TREE_CODE (decl))
    {
    case FUNCTION_DECL:
      fNear = 0;
      fn_name = IDENTIFIER_POINTER (DECL_NAME (decl));
      f += sprintf (f, MCHP_FCNN_FLAG);
      mchp_build_prefix (decl, -1, f);
      break;

    case VAR_DECL:
      fNear = 0;
      if (TREE_READONLY (decl))
	{
	  /*
	  ** If this is a constant declaration,
	  ** and constants are located in code space,
	  ** then it cannot be a near declaration.
	  */
	  fNear = 0;
	}
      SYMBOL_REF_FLAG (XEXP (rtl, 0)) = mchp_build_prefix (decl, fNear, prefix);
      break;

    default:
      break;
    }

  if ((prefix[0] != 0) == 0)
    fNear = 0;
  {
    const char *str = XSTR (XEXP (rtl, 0), 0);
    int len = strlen (str);
    const char *stripped_str = mchp_strip_name_encoding_helper (str);
    char *newstr = (char *) xmalloc (len + strlen (prefix) + 1);
    sprintf (newstr, "%s%s", prefix, stripped_str);
    XSTR (XEXP (rtl, 0), 0) = newstr;

    /* previously allocated ? */
    if (stripped_str != str)
      free ((void *) str);
  }
}

/* Return a nonzero value if DECL has a section attribute.  */
#ifndef IN_NAMED_SECTION
#define IN_NAMED_SECTION(DECL)                                                 \
  ((TREE_CODE (DECL) == FUNCTION_DECL || TREE_CODE (DECL) == VAR_DECL)         \
   && DECL_SECTION_NAME (DECL) != NULL)
#endif

/*
** A C statement or statements to switch to the appropriate
** section for output of DECL.  DECL is either a `VAR_DECL' node
** or a constant of some sort.  RELOC indicates whether forming
** the initial value of DECL requires link-time relocations.
*/
section *
mchp_select_section (tree decl, int reloc,
		     unsigned HOST_WIDE_INT align ATTRIBUTE_UNUSED)
{

  extern section *mergeable_string_section (
    tree decl ATTRIBUTE_UNUSED, unsigned HOST_WIDE_INT align ATTRIBUTE_UNUSED,
    unsigned int flags ATTRIBUTE_UNUSED);
  const char *sname;
  const char *ident = 0; /* compiler can't tell that this is intialized */

  rtx rtl;
  SECTION_FLAGS_INT flags = 0;

  if ((TREE_CODE (decl) == FUNCTION_DECL) || (TREE_CODE (decl) == VAR_DECL))
    {
      if (IN_NAMED_SECTION (decl) || get_mchp_absolute_address (decl)
	  || get_mchp_space_attribute (decl) || get_pic32c_tcm_attribute (decl)
          || mchp_persistent_p (decl))
	{
	  flags = section_flags_from_decl (decl);

	  sname = default_section_name (decl, flags);
	  if (flags)
	    {
	      if (sname)
		{
		  if (!DECL_P (decl))
		    decl = NULL_TREE;
		  return get_named_section (decl, sname, reloc);
		}
	    }
	}
    }

  switch (categorize_decl_for_section (decl, reloc))
    {
    case SECCAT_TEXT:
      return text_section;
    case SECCAT_RODATA:
      return readonly_data_section;
    case SECCAT_RODATA_MERGE_STR:
      return mergeable_string_section (decl, align, 0);
    case SECCAT_RODATA_MERGE_STR_INIT:
      return mergeable_string_section (DECL_INITIAL (decl), align, 0);
    case SECCAT_RODATA_MERGE_CONST:
      return mergeable_constant_section (DECL_MODE (decl), align, 0);
    case SECCAT_SRODATA:
      sname = ".sdata2";
      break;
    case SECCAT_DATA:
      return data_section;
    case SECCAT_DATA_REL:
      sname = ".data.rel";
      break;
    case SECCAT_DATA_REL_LOCAL:
      sname = ".data.rel.local";
      break;
    case SECCAT_DATA_REL_RO:
      sname = ".data.rel.ro";
      break;
    case SECCAT_DATA_REL_RO_LOCAL:
      sname = ".data.rel.ro.local";
      break;
    case SECCAT_SDATA:
      sname = ".sdata";
      break;
    case SECCAT_TDATA:
      sname = ".tdata";
      break;
    case SECCAT_BSS:
      if (bss_section)
	return bss_section;
      sname = ".bss";
      break;
    case SECCAT_SBSS:
      sname = ".sbss";
      break;
    case SECCAT_TBSS:
      sname = ".tbss";
      break;
    default:
      gcc_unreachable ();
    }

  if (!DECL_P (decl))
    decl = NULL_TREE;
  return get_named_section (decl, sname, reloc);
}

static const char *
default_section_name (tree decl, SECTION_FLAGS_INT flags)
{

  static char result[1024];
  char *f;
  int i;
  tree addr_attr = NULL;
  tree is_aligned = NULL;
  tree space_attr = NULL;
  tree tcm_attr = NULL;
  tree region, memory = 0;
  tree frp = 0;

  const char *pszSectionName = 0;
  int is_default = 0;
  int len_this_default_name;

  if (current_time == 0)
    current_time = time (0);
  /*
   * if we have a decl, use its address as the unique section name
   */
  if (decl)
    {
      len_this_default_name = sprintf (this_default_name, "*_%8.8lx%lx",
				       (unsigned long) decl, current_time);
    }
  else
    {
      len_this_default_name = 1;
      this_default_name[0] = '*';
      this_default_name[1] = 0;
    }

  f = result;
  *f = 0;

  if (decl)
    {
      addr_attr = get_mchp_absolute_address (decl);
      space_attr = get_mchp_space_attribute (decl);
      tcm_attr = get_pic32c_tcm_attribute (decl);

      if (DECL_SECTION_NAME (decl))
	{
	  pszSectionName = DECL_SECTION_NAME (decl);
	}
#if defined(PIC32_SUPPORT_CRYPTO_ATTRIBUTE) && 0
      if (mchp_crypto_p (decl))
	{
	  if (addr_attr)
	    f += sprintf (result, ".$XC32$_crypto.%s,%s(0x%lx)",
			  IDENTIFIER_POINTER (DECL_NAME (decl)),
			  SECTION_ATTR_ADDRESS,
			  (long unsigned int) TREE_INT_CST_LOW (
			    TREE_VALUE (TREE_VALUE (addr_attr))));
	  else
	    f += sprintf (result, ".$XC32$_crypto.%s",
			  IDENTIFIER_POINTER (DECL_NAME (decl)));
	}
      else
#endif
	if (addr_attr)
	{
	  if (!pszSectionName
	      || (strcmp (pszSectionName, mchp_default_section) == 0))
	    f += sprintf (result, "%s,%s(0x%lx)", this_default_name,
			  SECTION_ATTR_ADDRESS,
			  (long unsigned int) TREE_INT_CST_LOW (
			    TREE_VALUE (TREE_VALUE (addr_attr))));
	  else
	    {
	      if (((TREE_CODE (decl) == VAR_DECL) && flag_data_sections)
		  || ((TREE_CODE (decl) == FUNCTION_DECL)
		      && flag_function_sections))
		f += sprintf (result, "%s,%s(0x%lx)", pszSectionName,
			      SECTION_ATTR_ADDRESS,
			      (long unsigned int) TREE_INT_CST_LOW (
				TREE_VALUE (TREE_VALUE (addr_attr))));
	      else
		f += sprintf (result, "%s,%s(0x%lx)", pszSectionName,
			      SECTION_ATTR_ADDRESS,
			      (long unsigned int) TREE_INT_CST_LOW (
				TREE_VALUE (TREE_VALUE (addr_attr))));
	    }
	}
      else if (TREE_CODE (decl) == VAR_DECL)
	{
	  if (mchp_persistent_p(decl))
	    {
	      if (!pszSectionName || strcmp(pszSectionName, SECTION_NAME_PERSIST) != 0)
		pszSectionName = SECTION_NAME_PBSS;
	    }
	  if (pszSectionName
	      && !(tcm_attr && flag_data_sections)) /* FIX ME: fix
						       -fdata-sections on tcm
						       consts */
	    {
	      f += sprintf (result, "%s", pszSectionName);
	    }
	  else
	    {
	      if (!is_aligned)
		is_default = 1;
	      f += sprintf (result, "%s", this_default_name);
	    }
	}
      else if (TREE_CODE (decl) == FUNCTION_DECL)
	{
	  if (pszSectionName)
	    {
	      f += sprintf (result, "%s", pszSectionName);
	    }
	  else
	    {
	      if (!is_aligned)
		is_default = 1;
	      f += sprintf (result, "%s", this_default_name);
	    }
	}
      else
	{
	  if (!is_aligned)
	    is_default = 1;
	  f += sprintf (result, "%s", this_default_name);
	}

#if 0 // Not added Yet
         if (region)
         {
            if (TREE_CODE(TREE_VALUE(TREE_VALUE(region))) == STRING_CST)
            {
                 memory = TREE_VALUE(TREE_VALUE(region));
            }
         }

         if (memory)
         {
           pic32_external_memory *m;
           is_default = 0;

           for (m = pic32_external_memory_head; m; m = m->next) {
              if (strcmp (m->name, TREE_STRING_POINTER (memory)) == 0) break;
           }

           if (m == 0) error("invalid external memory attribute");

           f+= sprintf(f,",memory(_%s)", m->name);
#if 0
           if (!(flags & SECTION_NOLOAD))
           {
              f+= sprintf(f, "," SECTION_ATTR_NOLOAD);
           }
#endif
         }
#endif // Not added Yet

      if ((!is_default)
	  || (strncmp (result, this_default_name, len_this_default_name)))
	return result;
    }

  if (flags)
    {
      i = 0;
      if ((flags & SECTION_READ_ONLY) /*&& (!TARGET_EMBEDDED_DATA)*/)
	{
	  if ((TREE_CODE (decl) == STRING_CST)
	      || (DECL_INITIAL (decl) && TREE_CONSTANT (DECL_INITIAL (decl))))
	    {
	      const char *name;
#if defined(PIC32_SUPPORT_CRYPTO_ATTRIBUTE) && 0
	      if (mchp_crypto_p (decl)) /* crypto */
		{
		  name = ".$XC32$_crypto";
		}
	      else
		name = SECTION_NAME_CONST;
#else
	      name = SECTION_NAME_CONST;
#endif
	      while (reserved_section_names[i].section_name)
		{
		  if (((flags ^ reserved_section_names[i].mask) == 0)
		      && strstr (name, reserved_section_names[i].section_name)
			   == 0)
		    return name;
		  i++;
		}
	    }
	}
#if 1 /* TODO Not added Yet  */
      else if (flags & SECTION_RAMFUNC)
	{
	  if (TREE_CODE (decl) == FUNCTION_DECL)
	    {
	      const char *name;
	      name = SECTION_NAME_RAMFUNC;
	      while (reserved_section_names[i].section_name)
		{
		  if (((flags ^ reserved_section_names[i].mask) == 0)
		      && strstr (name, reserved_section_names[i].section_name)
			   == 0)
		    return name;
		  i++;
		}
	    }
	}
#endif
      i = 0;
      while (reserved_section_names[i].section_name)
	{
	  if ((flags ^ reserved_section_names[i].mask) == 0)
	    {
	      if (strcmp (reserved_section_names[i].section_name, ".text") == 0)
		{
		  const char *txt = mchp_text_scn;

		  if (!txt)
		    txt = reserved_section_names[i].section_name;
		  if (!flag_function_sections || !decl)
		    return txt;
#if 1
		  else
		    {
		      char *retval;
		      retval = (char *) xmalloc (
			strlen (txt) + 3
			+ strlen (IDENTIFIER_POINTER (DECL_NAME (decl))));
		      /* if we are prepending .isr use that instead of the
			 default name of .text until keep attribute is
			 implemented */
		      f += sprintf (retval, "%s.%s", txt,
				    IDENTIFIER_POINTER (DECL_NAME (decl)));
		      return retval;
		    }
#endif
		}
	      else
		return reserved_section_names[i].section_name;
	    }
	  i++;
	}
    }

  return this_default_name;
}

/*
** mchp_text_section_asm_op()
** A C expression whose value is a string, including spacing, containing
** the assembler operation that should precede instructions and read-only
** data. Normally "\t.text" is right.
*/

const char *
mchp_text_section_asm_op (void)
{

  static char *pszSection = NULL;
  static unsigned int pszSection_len;
  const char *section_name;
  const char *code_attr_name;
  tree space_attr = NULL;
  tree tcm_attr = NULL;
  bool is_ramfunc = FALSE;

  /* TODO */
  if (current_function_decl)
    {
      tcm_attr = get_pic32c_tcm_attribute (current_function_decl);
      /* TODO: at the moment do not allow tcm and space attributes together */
      if (!tcm_attr)
	{
	  space_attr = get_mchp_space_attribute (current_function_decl);
	  if (space_attr)
	    {
	      is_ramfunc = (get_identifier ("data")
			    == (TREE_VALUE (TREE_VALUE (space_attr))));
	    }
	}
      section_name
	= default_section_name (current_function_decl,
				is_ramfunc ? SECTION_RAMFUNC : SECTION_CODE);
      set_decl_section_name (current_function_decl, section_name);
    }
  else
    {
      section_name = mchp_text_scn ? mchp_text_scn : ".text";
    }

  //  lfInExecutableSection = TRUE;

  if (pszSection == NULL)
    {
      pszSection_len = strlen (section_name);
      pszSection = (char *) xmalloc (pszSection_len + 100);
    }
  else if (strlen (section_name) > pszSection_len)
    {
      pszSection_len = strlen (section_name);
      free (pszSection);
      pszSection = (char *) xmalloc (pszSection_len + 100);
    }

  if (tcm_attr != NULL)
    {
      code_attr_name = SECTION_ATTR_ITCM;
    }
  else if (is_ramfunc)
    code_attr_name = SECTION_ATTR_RAMFUNC;
  else
    code_attr_name = SECTION_ATTR_CODE;

  sprintf (pszSection, "\t.section\t%s,%s", section_name, code_attr_name);

  return (const char *) pszSection;
}

/************************************************************************/
/*
** mchp_data_section_asm_op()
** A C expression whose value is a string, including spacing, containing
** the assembler operation to identify the following data as writable
** initialized data. Normally "\t.data" is right.
*/
/************************************************************************/

const char *
mchp_data_section_asm_op (void)
{
  return ((const char *) "\t.section\t.data,data");
}

/************************************************************************/
/*
** mchp_rdata_section_asm_op()
**
** Constants go in the code/data window, hence the .const section
** is marked as executable or data (depending on the command-line),
** so that the assembler knows the word width.
*/
/************************************************************************/

const char *
mchp_rdata_section_asm_op (void)
{

  static char szSection[32];
  sprintf (szSection, "\t.section\t%s,%s", SECTION_NAME_CONST,
	   SECTION_ATTR_CODE);

  return (szSection);
}

const char *
mchp_bss_section_asm_op (void)
{
  return ((const char *) "\t.section\t.bss,bss");
}

const char *
mchp_sbss_section_asm_op (void)
{
  return ((const char *) "\t.section\t.sbss,bss,near");
}

const char *
mchp_sdata_section_asm_op (void)
{
  return ((const char *) "\t.section\t.sdata,data,near");
}

static char *
mchp_get_named_section_flags (const char *pszSectionName,
			      SECTION_FLAGS_INT flags)
{

  char pszSectionFlag[100] = " @ Invalid Section Attributes";
  char *f;

  if (pszSectionName == 0)
    return NULL;

  if (strcmp ("*", pszSectionName) == 0)
    return NULL;

  f = pszSectionFlag;

  if (flags & SECTION_BSS)
    {
      f += sprintf (f, "," SECTION_ATTR_BSS);
    }
  if (flags & SECTION_WRITE)
    {
      f += sprintf (f, "," SECTION_ATTR_DATA);
    }
  if (flags & SECTION_CODE)
    {
      f += sprintf (f, "," SECTION_ATTR_CODE);
    }
  if (flags & SECTION_READ_ONLY)
    {
      f += sprintf (f, "," SECTION_ATTR_CONST);
    }
  if (flags & SECTION_SERIALMEM)
    {
      f += sprintf (f, "," SECTION_ATTR_SERIALMEM);
    }
  if (flags & SECTION_RAMFUNC)
    {
      f += sprintf (f, "," SECTION_ATTR_RAMFUNC);
    }
  if (flags & SECTION_NEAR)
    {
      f += sprintf (f, "," SECTION_ATTR_NEAR);
    }
  if (flags & SECTION_PERSIST)
    {
      f += sprintf (f, "," SECTION_ATTR_PERSIST);
    }
  if (flags & SECTION_NOLOAD)
    {
      f += sprintf (f, "," SECTION_ATTR_NOLOAD);
    }
  if ((flags & SECTION_DEBUG) || (flags & SECTION_INFO))
    {
      f += sprintf (f, "," SECTION_ATTR_INFO);
    }
  if (flags & SECTION_KEEP)
    {
      f += sprintf (f, "," SECTION_ATTR_KEEP);
    }
  if (flags & SECTION_COHERENT)
    {
      f += sprintf (f, "," SECTION_ATTR_COHERENT);
    }
  if (flags & SECTION_ITCM)
    {
      f += sprintf (f, "," SECTION_ATTR_ITCM);
    }
  if (flags & SECTION_DTCM)
    {
      f += sprintf (f, "," SECTION_ATTR_DTCM);
    }
  if (flags & SECTION_NOPA)
    {
      f += sprintf (f, "," SECTION_ATTR_NOPA);
    }

  return xstrdup (pszSectionFlag);
}

static void
mchp_merged_asm_named_section (const char *pszSectionName,
			       SECTION_FLAGS_INT flags)
{

  char *pszSectionFlag;
  pszSectionFlag = mchp_get_named_section_flags (pszSectionName, flags);
#if 1
  fprintf (asm_out_file, "\t.section\t%s%s\n", pszSectionName, pszSectionFlag);
#endif
}

/************************************************************************/
/* TARGET_ASM_NAMED_SECTION target hook.                */
/* Output assembly directives to switch to section pszSectionName.    */
/* The section name will have any user-specifed flags appended.        */
/* The section should have attributes as specified by flags, which is a    */
/* bit mask of the SECTION_* flags defined in output.h.         */
/************************************************************************/
void
mchp_asm_named_section (const char *pszSectionName, SECTION_FLAGS_INT flags,
			tree decl)
{
  flags |= mchp_section_type_flags (decl, pszSectionName, 0);
  if (decl)
    {
      if (((flags & SECTION_WRITE) && (!DECL_INITIAL (decl)))
	  || ((flags & SECTION_WRITE) && bss_initializer_p (decl)))
	{
	  flags &= ~(SECTION_FLAGS_INT) SECTION_WRITE;
	  flags |= (SECTION_FLAGS_INT) SECTION_BSS;
	}
    }

  mchp_merged_asm_named_section (pszSectionName, flags);
}

/* Returns TRUE iff the target attribute indicated by ATTR_ID takes a plain
   identifier as an argument, so the front end shouldn't look it up.  */
bool
pic32c_attribute_takes_identifier_p (const_tree attr_id)
{
  if (is_attribute_p ("space", attr_id))
    return true;
  return false;
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
  if (mchp_allow_partial_config_words)
    return;

  if (!pic32c_verify_configuration_word (word, referenced_bits))
    {
      /* TODO: Can we make the location information more useful, or
         ignore line/column information? */
      error ("config word for address 0x%x not fully specified",
             word->address);
    }
}

static void
pic32c_output_configuration_words (void)
{
  struct mchp_config_specification *spec;

  if (mchp_configuration_values)
    {
      fputs ("@ Begin Microchip Technology PIC32 MCU configuration words\n",
	     asm_out_file);

      for (spec = mchp_configuration_values; spec; spec = spec->next)
	{
	  /* if there are referenced bits in the word, output its value */
	  if (spec->referenced_bits)
	    {
              pic32c_check_configuration_word (spec->word,
                                               spec->referenced_bits);
	      fprintf (asm_out_file, "@ Configuration word at address 0x%08x\n",
		       spec->word->address);
	      fprintf (
		asm_out_file,
		"\t.section\t.config_%08X, code, keep, address(0x%08X)\n",
		spec->word->address, spec->word->address);
	      fprintf (asm_out_file, "\t.type\t__config_%08X, %%object\n",
		       spec->word->address);
	      fprintf (asm_out_file, "\t.size\t__config_%08X, 4\n",
		       spec->word->address);
	      fprintf (asm_out_file, "__config_%08X:\n", spec->word->address);
	      fprintf (asm_out_file, "\t.word\t0x%08X\n", spec->value);
	    }
	}

      fputs ("@ End Microchip Technology PIC32 MCU configuration words\n",
	     asm_out_file);
    }
}

void
pic32c_file_end (void)
{
  pic32c_output_configuration_words ();
}

/* The following hooks were added for Code Coverage but they
 * may prove useful for other purposes as well */
void
pic32c_expand_function_start (tree decl)
{
  pic32c_start_function (decl);
}

void
pic32c_asm_code_end (void)
{
  xccov_code_end ();
}

void
pic32c_cond_reg_usage (void)
{
  pic32c_reserve_registers ();
}

/* if 'mchp_pragma_nocodecov' is set, adds 'nocodecov' attribute to function types */
void
pic32c_set_default_type_attributes (tree type)
{
  if (mchp_pragma_nocodecov
      && (TREE_CODE (type) == FUNCTION_TYPE || TREE_CODE (type) == METHOD_TYPE))
    {
      tree type_attr_list = TYPE_ATTRIBUTES (type);
      tree attr_name = get_identifier ("nocodecov");

      type_attr_list = tree_cons (attr_name, NULL_TREE, type_attr_list);
      TYPE_ATTRIBUTES (type) = type_attr_list;
    }
}

void
pic32c_emit_cc_section (const char *name)
{
  gcc_assert (name);

  SECTION_FLAGS_INT flags = 0;

  if (!strcmp (name, CODECOV_SECTION))
    {
      flags = SECTION_BSS;
    }
  else if (!strcmp (name, CODECOV_INFO_HDR) || !strcmp (name, CODECOV_INFO))
    {
      flags = SECTION_INFO | SECTION_KEEP;
    }
  else
    {
      gcc_unreachable ();
    }

  switch_to_section (get_section (name, flags, NULL));
}

/* --- end of Code Coverage-related hooks */

/* Build a TREE_LIST node encoding a format string and conversion specifier
   set for smart-io. */
tree
build_smartio_format (tree fmt, tree spec)
{
  tree result = tree_cons (get_identifier ("smartio-spec"), spec, NULL);
  result = tree_cons (get_identifier ("smartio-fmt"), fmt, result);
  TREE_TYPE (result) = TREE_TYPE (fmt);
  TREE_CONSTANT (result) = 1;
  return result;
}

/* Handle MDBG(str) debug flags.
   This checks the -mdbg= string for the given option, memoizing the
   result. On first call the string will be parsed and recorded for
   quicker lookup on subsequent calls.
   */

#if _BUILD_MCHP_DEVEL_

typedef simple_hashmap_traits<nofree_string_hash, int> str_hash_map;
typedef hash_map<const char*, int, str_hash_map> dbg_hash_map;

/* Add entries from the given debug string to a hash table. */
static int
init_dbg_hash (dbg_hash_map *tab, const char *str)
{
#define DBG_HASH_MAX_LEN 15
#define DBG_HASH_MAX_ENTRIES 64
#define DBG_HASH_BUFSIZE ((DBG_HASH_MAX_LEN+1) * DBG_HASH_MAX_ENTRIES)
#define DBG_HASH_DELIM ','

  /* DEVEL_FIXME: need to store all our debug strings statically so the hash
     map can compare them. The alternative seems to be building a traits
     type from scratch that will store our strings. */
  static char buf[DBG_HASH_BUFSIZE] = {'\0'};
  const char *p = str;
  char *bp = buf;
  int found = 0;

  if (!tab)
    return 0;

  while (*p != '\0')
    {
      unsigned int i = 0;
      const char *q = p;

      /* Copy at most DBG_HASH_MAX_LEN characters of the string into
         our buffer. */
      while (*q != '\0' && *q != DBG_HASH_DELIM)
        {
          if (i < DBG_HASH_MAX_LEN)
            bp[i++] = *q;
          ++q;
        }
      bp[i] = '\0';

      /* Add to map with value 1 indicating present. In the future
         we can support other values in the map. */
      found += (i != 0);
      if (i != 0) 
        tab->put (bp, 1);
      bp += (i + 1);

      /* DEVEL_TODO: consider warning (or growing) if we exceed the
         max number of debug options. */
      if (*q == DBG_HASH_DELIM && found < DBG_HASH_MAX_ENTRIES)
        p = ++q;
      else
        break;
    }

  return found != 0;
#undef DBG_HASH_MAX_LEN
#undef DBG_HASH_MAX_ENTRIES
#undef DBG_HASH_BUFSIZE
#undef DBG_HASH_DELIM
}

int
pic32c_get_debug_option(const char *str)
{
  cl_deferred_option *opt;
  vec<cl_deferred_option> v;
  static dbg_hash_map *dbg_hash = NULL;
  static int init = 0, avail = 0;
  unsigned int i;

  if (!init)
    {
      if (mchp_deferred_debug)
        {
          v = *((vec<cl_deferred_option> *) mchp_deferred_debug);
          dbg_hash = new dbg_hash_map (16, 1);
        }
      else
        v = vNULL;

      FOR_EACH_VEC_ELT (v, i, opt)
        {
          switch (opt->opt_index)
            {
            case OPT_mdbg_:
              if (dbg_hash)
                avail = init_dbg_hash (dbg_hash, opt->arg);
              break;

            default:
              gcc_unreachable ();
            }
        }

      init = 1;
    }

  if (avail && str)
    {
      int *p = dbg_hash->get (str);
      return p ? *p : 0;
    }
  return 0;
}

#endif /* _BUILD_MCHP_DEVEL_ */

/*  stuff for writing pic32c_builtins.h           */

void decode_qualifier(tree type, char **buffer) {
  int qualifiers;

  qualifiers = TYPE_QUALS(type);
  if (qualifiers & TYPE_QUAL_CONST)
    *buffer += sprintf(*buffer,"const ");
  if (qualifiers & TYPE_QUAL_VOLATILE)
    *buffer += sprintf(*buffer,"volatile ");
  if (qualifiers & TYPE_QUAL_RESTRICT)
    *buffer += sprintf(*buffer, "%s ",
                       flag_isoc99 ? "restrict" : "__restrict__");
}


bool decode_type(tree type, char **buffer) {
/*  returms true unless an unrepresentable type is found */
  int strune=0;
  int type_precision = -1;
  int simd_width_in_bits;
  int precision;
  bool retval = true;

  decode_qualifier(type, buffer);
  switch TREE_CODE(type) {
    case POINTER_TYPE:
      retval = decode_type(TREE_TYPE(type),buffer);
      *buffer += sprintf(*buffer, " *");
      return (retval);

    case VOID_TYPE:
      *buffer += sprintf(*buffer,"void");
      return (true);
      
    case VECTOR_TYPE:
	simd_width_in_bits = 8*int_size_in_bytes(type);
	*buffer += sprintf(*buffer,"__simd%d_",simd_width_in_bits);
	if (TREE_CODE(TREE_TYPE(type)) == REAL_TYPE) {
	    // vectors of real types need to be handled specially because we don't have the base types float32_t available
	    // we do have __simdxx_float16_t and simdxx_float32_t though.
	    type_precision = TYPE_PRECISION(TREE_TYPE(type));
	    if (type_precision <= 8) {
		type_precision = 8;
	    } else if (type_precision <= 16) {
		type_precision = 16;
	    } else if (type_precision <= 32) {
		type_precision = 32;
	    } else if (type_precision <= 64) {
		type_precision = 64;
	    } else type_precision = 0;
	    
	    *buffer += sprintf(*buffer,"float%d_t", type_precision);
          
	}
	else {
	    return (decode_type(TREE_TYPE(type),buffer));
	}
	return (true);
	
    case BOOLEAN_TYPE:
    case INTEGER_TYPE:
      type_precision = TYPE_PRECISION(type);
      if (type_precision <= 8) {
        type_precision = 8;
      } else if (type_precision <= 16) {
        type_precision = 16;
      } else if (type_precision <= 32) {
        type_precision = 32;
      } else if (type_precision <= 64) {
        type_precision = 64;
      } else type_precision = 0;

      if (type_precision) {
        if (TYPE_UNSIGNED(type)) {
          char *b = *buffer;
          *b++ = 'u';
          *buffer = b;
        }
        *buffer += sprintf(*buffer,"int%d_t", type_precision);
        return (true);
      }
      /* FALLSTHROUGH */
    case REAL_TYPE:
      if (type_precision == -1) {
        type_precision = TYPE_PRECISION(type);
        switch (type_precision) {
	case 16:    return(false); // not representable!
          case 32:  *buffer += sprintf(*buffer, "float");
                    return (true);
          case 64:  *buffer += sprintf(*buffer, "long double");
                    return (true);
        }
      }
      /* FALLSTHROUGH */
    case FIXED_POINT_TYPE:
      if (type_precision == -1) {

	  type_precision = TYPE_PRECISION(type);
        if (TYPE_UNSIGNED(type)) {
          *buffer += sprintf(*buffer,"unsigned ");
        }
        if (type_precision == 16) {
          *buffer += sprintf(*buffer, "__Fract");
          return (true);
        } else if (type_precision == 40) {
          *buffer += sprintf(*buffer, "__Accum");
          return (true);
        }
      }
      // oh - oh, we didn't find a type 
      if (TYPE_NAME(type)) {
        int prec = TYPE_PRECISION (type);
        retval = decode_type(TYPE_NAME(type),buffer);
        *buffer += sprintf(*buffer,":%d", prec);
      } else {
        int prec = TYPE_PRECISION (type);

        if (TYPE_NAME (type)) {
          retval = decode_type(TYPE_NAME(type),buffer);
          if (TYPE_PRECISION (type) != prec)
          {
            *buffer += sprintf(*buffer,":%d", prec);
          }
        } else {
 	    return false;    
         }
      }
      return (retval);

    case TYPE_DECL:
	if (DECL_NAME(type)) {
	    if (strstr( IDENTIFIER_POINTER(DECL_NAME(type)), "builtin_neon_") != NULL) return (false);
	    *buffer += sprintf(*buffer,"%s",
                           IDENTIFIER_POINTER(DECL_NAME(type)));
      } else {
	    return false;    
      }
      break;

    case UNION_TYPE:
      *buffer += sprintf(*buffer,"union ");
      strune=1;
      /* FALLSTHROUGH */ 
    case RECORD_TYPE:
      if (strune == 0) {
        *buffer += sprintf(*buffer,"struct ");
        strune=1;
      }
      //FALLSTHROUGH 
    case ENUMERAL_TYPE:
      if (strune == 0) {
        *buffer += sprintf(*buffer,"enum ");
        strune=1;
      }
      if (TYPE_NAME(type)) {
	  return(decode_type(TYPE_NAME(type),buffer));
      } else {
	  return(false);
      }
   }
}

void pretty_tree_with_prototype(tree fndecl_or_type) {
  static FILE *builtins_h = 0;
  static bool initialised = 0;
  tree type,arg = 0;
  int i = 0;
  char name_buffer[1024];
  char result_buffer[1024];
  char args_buffer[1024];
  char *nbp = name_buffer;
  char *rbp = result_buffer;
  char *abp = args_buffer;
  bool print_it = true;
  int comma=0;
  tree fndecl;

  if (!initialised) {
    initialised=1;
    if (access("pic32c_builtins.h", F_OK) != -1)
        builtins_h = fopen("pic32c_builtins.h","a");
    else
        builtins_h = fopen("pic32c_builtins.h","w");

    if (builtins_h == NULL) {
        warning(0,"Cannot open file for writing");
    }
    fprintf(builtins_h,"#include <stdint.h>\n\n");
  }

    
  if (TYPE_P(fndecl_or_type)) {
    fndecl = 0;
    type = fndecl_or_type;
  }
  else {
    type = TREE_TYPE(fndecl_or_type);
    fndecl = fndecl_or_type;
    gcc_assert(TREE_CODE(fndecl) == FUNCTION_DECL);
  }

  

  print_it &= decode_type(TREE_TYPE(type),&rbp);
  
  nbp += sprintf(nbp," %s",
     fndecl && DECL_NAME(fndecl) ?
	  IDENTIFIER_POINTER(DECL_NAME(fndecl)) : "!!unknown");

  
  arg = TYPE_ARG_TYPES(type);
  if (arg == NULL)
      abp += sprintf(abp,"void");
  else
      abp += sprintf(abp,"        ");
  
  while (arg) {

    print_it &= decode_type(TREE_VALUE(arg),&abp);
    arg = TREE_CHAIN(arg);
    if ((arg != NULL) && (arg != void_list_node)) abp += sprintf(abp,",\n        ");
    if (arg == void_list_node) break;
  }
  if (print_it) fprintf(builtins_h,"%s %s(\n%s);\n",result_buffer,name_buffer,args_buffer);

}









