/* Subroutines used for MIPS code generation.
   Copyright (C) 1989, 1990, 1991, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002, 2003, 2004 Free Software Foundation, Inc.
   Contributed by A. Lichnewsky, lich@inria.inria.fr.
   Changes by Michael Meissner, meissner@osf.org.
   64 bit r4000 support by Ian Lance Taylor, ian@cygnus.com, and
   Brendan Eich, brendan@microunity.com.

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
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tm.h"
#include <signal.h>
#include "rtl.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "real.h"
#include "insn-config.h"
#include "conditions.h"
#include "insn-attr.h"
#include "recog.h"
#include "toplev.h"
#include "output.h"
#include "tree.h"
#include "function.h"
#include "expr.h"
#include "optabs.h"
#include "flags.h"
#include "reload.h"
#include "tm_p.h"
#include "ggc.h"
#include "gstab.h"
#include "hashtab.h"
#include "debug.h"
#include "target.h"
#include "target-def.h"
#include "integrate.h"
#include "langhooks.h"
#include "cfglayout.h"
#include "cgraph.h"
#include "cpplib.h"
#include "c-pragma.h"
#include <assert.h>

/* Enumeration for all of the relational tests, so that we can build
   arrays indexed by the test type, and not worry about the order
   of EQ, NE, etc.  */

enum internal_test {
  ITEST_EQ,
  ITEST_NE,
  ITEST_GT,
  ITEST_GE,
  ITEST_LT,
  ITEST_LE,
  ITEST_GTU,
  ITEST_GEU,
  ITEST_LTU,
  ITEST_LEU,
  ITEST_MAX
};

/* Return true if it is likely that the given mode will be accessed
   using only a single instruction.  */
#define SINGLE_WORD_MODE_P(MODE) \
  ((MODE) != BLKmode && GET_MODE_SIZE (MODE) <= UNITS_PER_WORD)

/* True if X is an unspec wrapper around a SYMBOL_REF or LABEL_REF.  */
#define UNSPEC_ADDRESS_P(X)                                     \
  (GET_CODE (X) == UNSPEC                                       \
   && XINT (X, 1) >= UNSPEC_ADDRESS_FIRST                       \
   && XINT (X, 1) < UNSPEC_ADDRESS_FIRST + NUM_SYMBOL_TYPES)

/* Extract the symbol or label from UNSPEC wrapper X.  */
#define UNSPEC_ADDRESS(X) \
  XVECEXP (X, 0, 0)

/* Extract the symbol type from UNSPEC wrapper X.  */
#define UNSPEC_ADDRESS_TYPE(X) \
  ((enum mips_symbol_type) (XINT (X, 1) - UNSPEC_ADDRESS_FIRST))

/* True if X is (const (unspec [(const_int 0)] UNSPEC_GP)).  This is used
   to initialize the mips16 gp pseudo register.  */
#define CONST_GP_P(X) \
  (GET_CODE (X) == CONST                        \
   && GET_CODE (XEXP (X, 0)) == UNSPEC          \
   && XINT (XEXP (X, 0), 1) == UNSPEC_GP)

/* The maximum distance between the top of the stack frame and the
   value $sp has when we save & restore registers.

   Use a maximum gap of 0x80 in the mips16 case.  We can then use
   unextended instructions to save and restore registers, and to
   allocate and deallocate the top part of the frame.

   The value in the !mips16 case must be a SMALL_OPERAND and must
   preserve the maximum stack alignment.  It could really be 0x7ff0,
   but SGI's assemblers implement daddiu $sp,$sp,-0x7ff0 as a
   multi-instruction addu sequence.  Use 0x7fe0 to work around this. */
#define MIPS_MAX_FIRST_STACK_STEP \
  (TARGET_M16E_SAVE ? 2040 \
   : TARGET_MIPS16 && frame_pointer_needed ? 0x80 \
     : 0x7fe0)

/* Classifies a SYMBOL_REF, LABEL_REF or UNSPEC address.

   SYMBOL_GENERAL
       Used when none of the below apply.

   SYMBOL_SMALL_DATA
       The symbol refers to something in a small data section.

   SYMBOL_CONSTANT_POOL
       The symbol refers to something in the mips16 constant pool.

   SYMBOL_GOT_LOCAL
       The symbol refers to local data that will be found using
       the global offset table.

   SYMBOL_GOT_GLOBAL
       Likewise non-local data.

   SYMBOL_GOTOFF_PAGE
       An UNSPEC wrapper around a SYMBOL_GOT_LOCAL.  It represents the
       offset from _gp of a GOT page entry.

   SYMBOL_GOTOFF_GLOBAL
       An UNSPEC wrapper around a SYMBOL_GOT_GLOBAL.  It represents the
       the offset from _gp of the symbol's GOT entry.

   SYMBOL_GOTOFF_CALL
       Like SYMBOL_GOTOFF_GLOBAL, but used when calling a global function.
       The GOT entry is allowed to point to a stub rather than to the
       function itself.

   SYMBOL_GOTOFF_LOADGP
       An UNSPEC wrapper around a function's address.  It represents the
       offset of _gp from the start of the function.  */
enum mips_symbol_type {
  SYMBOL_GENERAL,
  SYMBOL_LONGCALL,
  SYMBOL_SMALL_DATA,
  SYMBOL_CONSTANT_POOL,
  SYMBOL_SECTION_RELATIVE,
  SYMBOL_GOT_LOCAL,
  SYMBOL_GOT_GLOBAL,
  SYMBOL_GOTOFF_PAGE,
  SYMBOL_GOTOFF_GLOBAL,
  SYMBOL_GOTOFF_CALL,
  SYMBOL_GOTOFF_LOADGP
};
#define NUM_SYMBOL_TYPES (SYMBOL_GOTOFF_LOADGP + 1)


/* Classifies an address.

   ADDRESS_REG
       A natural register + offset address.  The register satisfies
       mips_valid_base_register_p and the offset is a const_arith_operand.

   ADDRESS_LO_SUM
       A LO_SUM rtx.  The first operand is a valid base register and
       the second operand is a symbolic address.

   ADDRESS_CONST_INT
       A signed 16-bit constant address.

   ADDRESS_SYMBOLIC:
       A constant symbolic address (equivalent to CONSTANT_SYMBOLIC).  */
enum mips_address_type {
  ADDRESS_REG,
  ADDRESS_LO_SUM,
  ADDRESS_CONST_INT,
  ADDRESS_SYMBOLIC
};

/* A function to save or store a register.  The first argument is the
   register and the second is the stack slot.  */
typedef void (*mips_save_restore_fn) (rtx, rtx);

struct constant;
struct mips_arg_info;
struct mips_address_info;
struct mips_integer_op;

static enum mips_symbol_type mips_classify_symbol (rtx);
static void mips_split_const (rtx, rtx *, HOST_WIDE_INT *);
static bool mips_offset_within_object_p (rtx, HOST_WIDE_INT);
static bool mips_symbolic_constant_p (rtx, enum mips_symbol_type *);
static bool mips_valid_base_register_p (rtx, enum machine_mode, int);
static bool mips_symbolic_address_p (enum mips_symbol_type, enum machine_mode);
static bool mips_classify_address (struct mips_address_info *, rtx,
                                   enum machine_mode, int);
static int mips_symbol_insns (enum mips_symbol_type);
static bool mips16_unextended_reference_p (enum machine_mode mode, rtx, rtx);
static rtx mips_force_temporary (rtx, rtx);
static rtx mips_split_symbol (rtx, rtx, enum mips_symbol_type);
static rtx mips_unspec_address (rtx, enum mips_symbol_type);
static rtx mips_unspec_offset_high (rtx, rtx, rtx, enum mips_symbol_type);
static rtx mips_load_got (rtx, rtx, enum mips_symbol_type);
static rtx mips_add_offset (rtx, rtx, HOST_WIDE_INT);
static unsigned int mips_build_shift (struct mips_integer_op *, HOST_WIDE_INT);
static unsigned int mips_build_lower (struct mips_integer_op *,
                                      unsigned HOST_WIDE_INT);
static unsigned int mips_build_integer (struct mips_integer_op *,
                                        unsigned HOST_WIDE_INT);
static void mips_move_integer (rtx, unsigned HOST_WIDE_INT);
static void mips_legitimize_const_move (enum machine_mode, rtx, rtx);
static int m16_check_op (rtx, int, int, int);
static bool mips_rtx_costs (rtx, int, int, int *);
static int mips_address_cost (rtx);
static enum internal_test map_test_to_internal_test (enum rtx_code);
static void get_float_compare_codes (enum rtx_code, enum rtx_code *,
                                     enum rtx_code *);
static void mips_load_call_address (rtx, rtx, int);
static bool mips_function_ok_for_sibcall (tree, tree);
static void mips_block_move_straight (rtx, rtx, HOST_WIDE_INT);
static void mips_adjust_block_mem (rtx, HOST_WIDE_INT, rtx *, rtx *);
static void mips_block_move_loop (rtx, rtx, HOST_WIDE_INT);
static void mips_arg_info (const CUMULATIVE_ARGS *, enum machine_mode,
                           tree, int, struct mips_arg_info *);
static bool mips_get_unaligned_mem (rtx *, unsigned int, int, rtx *, rtx *);
static void mips_set_architecture (const struct mips_cpu_info *);
static void mips_set_tune (const struct mips_cpu_info *);
static struct machine_function *mips_init_machine_status (void);
static void print_operand_reloc (FILE *, rtx, const char **);
static bool mips_assemble_integer (rtx, unsigned int, int);
static void mips_file_start (void);
static void mips_file_end (void);
static bool mips_rewrite_small_data_p (rtx);
static int small_data_pattern_1 (rtx *, void *);
static int mips_rewrite_small_data_1 (rtx *, void *);
static bool mips_function_has_gp_insn (void);
static unsigned int mips_global_pointer (void);
static bool mips_save_reg_p (unsigned int);
static void mips_save_restore_reg (enum machine_mode, int, HOST_WIDE_INT,
                                   mips_save_restore_fn);
static void mips_for_each_saved_reg (HOST_WIDE_INT, mips_save_restore_fn);
static void mips_output_cplocal (void);
static void mips_emit_loadgp (void);
static void mips_output_function_prologue (FILE *, HOST_WIDE_INT);
static void mips_set_frame_expr (rtx);
static rtx mips_frame_set (rtx, rtx);
static void mips_save_reg (rtx, rtx);
static void mips_output_function_epilogue (FILE *, HOST_WIDE_INT);
static void mips_restore_reg (rtx, rtx);
static void mips_output_mi_thunk (FILE *, tree, HOST_WIDE_INT,
                                  HOST_WIDE_INT, tree);
static int symbolic_expression_p (rtx, rtx*);
static int mips_comp_type_attributes (tree, tree);

static void mips_select_rtx_section (enum machine_mode, rtx,
                                     unsigned HOST_WIDE_INT);
static void mips_select_section (tree, int, unsigned HOST_WIDE_INT)
                                  ATTRIBUTE_UNUSED;
static bool mips_in_small_data_p (tree);
static bool mips_cannot_force_const_mem (rtx);
static void mips_encode_section_info (tree, rtx, int);
static int mips_fpr_return_fields (tree, tree *);
static bool mips_return_in_msb (tree);
static rtx mips_return_fpr_pair (enum machine_mode mode,
                                 enum machine_mode mode1, HOST_WIDE_INT,
                                 enum machine_mode mode2, HOST_WIDE_INT);
static rtx mips16_gp_pseudo_reg (void);
static void mips16_fp_args (FILE *, int, int);
static void build_mips16_function_stub (FILE *, tree, int);
static rtx add_constant (struct constant **, rtx, enum machine_mode);
static void dump_constants (struct constant *, rtx);
static rtx *mips_find_symbol (rtx *);
static void mips16_lay_out_constants (void);
static void mips_avoid_hazard (rtx, rtx, int *, rtx *, rtx);
static void mips_avoid_hazards (void);
static void mips_reorg (void);
static bool mips_strict_matching_cpu_name_p (const char *, const char *);
static bool mips_matching_cpu_name_p (const char *, const char *);
static const struct mips_cpu_info *mips_parse_cpu (const char *, const char *);
static const struct mips_cpu_info *mips_cpu_info_from_isa (int);
static int mips_adjust_cost (rtx, rtx, rtx, int);
static bool mips_is_costly_dependence (rtx, rtx, rtx, int, int);
static int mips_sched_reorder (FILE *, int, rtx *, int *, int);
static int mips_variable_issue (FILE *, int, rtx, int);
static int mips_dfa_new_cycle (FILE *, int, rtx, int, int, int *sort_p);
static int mips_issue_rate (void);
static int mips_use_dfa_pipeline_interface (void);
static bool mips_sched_regno_clobber_dependencies (int);
static void mips_init_libfuncs (void);
static tree mips_build_builtin_va_list (void);
static void mips_set_mips16_mode (int);
static void mips_prepare_function_start (tree);
static const char *mips_pch_valid_p  (const void *, size_t);
static void mips_web_mark_register (rtx, char *);
static bool mips_spill_auto_register (tree, bool);

#if TARGET_IRIX
static void irix_asm_named_section_1 (const char *, unsigned int,
                                      unsigned int);
static void irix_asm_named_section (const char *, unsigned int);
static int irix_section_align_entry_eq (const void *, const void *);
static hashval_t irix_section_align_entry_hash (const void *);
static void irix_file_start (void);
static int irix_section_align_1 (void **, void *);
static void copy_file_data (FILE *, FILE *);
static void irix_file_end (void);
static unsigned int irix_section_type_flags (tree, const char *, int);
#endif

static void mips_init_builtins (void);
static rtx mips_expand_builtin (tree, rtx, rtx, enum machine_mode, int);
static rtx mips_expand_compare_builtin (rtx, unsigned int, tree);
static rtx mips_expand_ps_compare_builtin (enum mips_cmp_choice, rtx,
                                           unsigned int, tree);
static rtx mips_expand_4s_compare_builtin (enum mips_cmp_choice, rtx,
                                           unsigned int, tree);
static rtx mips_expand_ps_cond_move_builtin (enum mips_cmp_choice, rtx,
                                             unsigned int, tree);
static rtx mips_expand_bposge (rtx, int);
static rtx mips_expand_two_operands_with_immediate (rtx, unsigned int, tree,
                                                   int, int);
static tree mchp_function_interrupt_p (tree decl);
static int mchp_function_naked_p (tree func);
static void mchp_target_insert_attributes (tree node, tree *attr_ptr);
static tree mchp_interrupt_attribute (tree *decl, tree identifier, tree args,
    int flags, bool *no_add_attrs);
static void mchp_add_vector_dispatch_entry (const char *target_name,
    int vector_number);
static tree mchp_at_vector_attribute (tree *decl, tree identifier,
    tree args, int flags, bool *no_add_attrs);
static tree mchp_vector_attribute (tree *decl, tree identifier, tree args,
    int flags, bool *no_add_attrs);
static int mchp_vector_attribute_compound_expr (tree *node, tree expr, 
    bool *no_add_attrs);
static int mchp_register_interrupt_context_p (unsigned regno);

/* Structure to be filled in by compute_frame_size with register
   save masks, and offsets for the current function.  */

struct mips_frame_info GTY(())
{
  HOST_WIDE_INT total_size;     /* # bytes that the entire frame takes up */
  HOST_WIDE_INT var_size;       /* # bytes that variables take up */
  HOST_WIDE_INT args_size;      /* # bytes that outgoing arguments take up */
  HOST_WIDE_INT cprestore_size; /* # bytes that the .cprestore slot takes up */
  HOST_WIDE_INT gp_reg_size;    /* # bytes needed to store gp regs */
  HOST_WIDE_INT fp_reg_size;    /* # bytes needed to store fp regs */
  HOST_WIDE_INT int_reg_size;   /* # bytes needed to store interrupt regs */
  unsigned int mask;            /* mask of saved gp registers */
  unsigned int fmask;           /* mask of saved fp registers */
  HOST_WIDE_INT gp_save_offset; /* offset from vfp to store gp registers */
  HOST_WIDE_INT fp_save_offset; /* offset from vfp to store fp registers */
  HOST_WIDE_INT int_save_offset;/* offset from vfp to store interrupt regs */
  HOST_WIDE_INT gp_sp_offset;   /* offset from new sp to store gp registers */
  HOST_WIDE_INT fp_sp_offset;   /* offset from new sp to store fp registers */
  HOST_WIDE_INT int_sp_offset;  /* offset from new sp to store interrupt regs */
  bool initialized;             /* true if frame size already calculated */
  int num_gp;                   /* number of gp registers saved */
  int num_fp;                   /* number of fp registers saved */
  bool has_interrupt_context;   /* true if interrupt context is saved */
  bool has_hilo_context;        /* true if hi and lo registers are saved */
                                /*   will only be true for interrupts */
};

struct machine_function GTY(()) {
  /* Pseudo-reg holding the address of the current function when
     generating embedded PIC code.  */
  rtx embedded_pic_fnaddr_rtx;

  /* Pseudo-reg holding the value of $28 in a mips16 function which
     refers to GP relative global variables.  */
  rtx mips16_gp_pseudo_rtx;

  /* Current frame information, calculated by compute_frame_size.  */
  struct mips_frame_info frame;

  /* Length of instructions in function; mips16 only.  */
  long insns_len;

  /* The register to use as the global pointer within this function.  */
  unsigned int global_pointer;

  /* True if mips_adjust_insn_length should ignore an instruction's
     hazard attribute.  */
  bool ignore_hazard_length_p;

  /* True if the whole function is suitable for .set noreorder and
     .set nomacro.  */
  bool all_noreorder_p;

  /* True if the function is known to have an instruction that needs $gp.  */
  bool has_gp_insn_p;

  /* When doing section relative addressing, it holds the rtx of the label
     if there are accesses to symbols in the .data section.  */
  rtx sectrel_base_data_rtx;

  /* When doing section relative addressing, it holds the rtx of the label
     if there are accesses to symbols in the .bss section.  */
  rtx sectrel_base_bss_rtx;

  /* The length of all strings seen when compiling for the mips16.  This
     is used to tell how many strings are in the constant pool, so that
     we can see if we may have an overflow.  This is reset each time the
     constant pool is output.  */
  int mips_string_length;

  /* When generating mips16 code, a list of all strings that are to be
     output after the current function.  */
  rtx mips16_strings;
};

/* Information about a single argument.  */
struct mips_arg_info
{
  /* True if the argument is passed in a floating-point register, or
     would have been if we hadn't run out of registers.  */
  bool fpr_p;

  /* The argument's size, in bytes.  */
  unsigned int num_bytes;

  /* The number of words passed in registers, rounded up.  */
  unsigned int reg_words;

  /* The offset of the first register from GP_ARG_FIRST or FP_ARG_FIRST,
     or MAX_ARGS_IN_REGISTERS if the argument is passed entirely
     on the stack.  */
  unsigned int reg_offset;

  /* The number of words that must be passed on the stack, rounded up.  */
  unsigned int stack_words;

  /* The offset from the start of the stack overflow area of the argument's
     first stack word.  Only meaningful when STACK_WORDS is nonzero.  */
  unsigned int stack_offset;
};


/* Information about an address described by mips_address_type.

   ADDRESS_CONST_INT
       No fields are used.

   ADDRESS_REG
       REG is the base register and OFFSET is the constant offset.

   ADDRESS_LO_SUM
       REG is the register that contains the high part of the address,
       OFFSET is the symbolic address being referenced and SYMBOL_TYPE
       is the type of OFFSET's symbol.

   ADDRESS_SYMBOLIC
       SYMBOL_TYPE is the type of symbol being referenced.  */

struct mips_address_info
{
  enum mips_address_type type;
  rtx reg;
  rtx offset;
  enum mips_symbol_type symbol_type;
};


/* One stage in a constant building sequence.  These sequences have
   the form:

        A = VALUE[0]
        A = A CODE[1] VALUE[1]
        A = A CODE[2] VALUE[2]
        ...

   where A is an accumulator, each CODE[i] is a binary rtl operation
   and each VALUE[i] is a constant integer.  */
struct mips_integer_op {
  enum rtx_code code;
  unsigned HOST_WIDE_INT value;
};


/* The largest number of operations needed to load an integer constant.
   The worst accepted case for 64-bit constants is LUI,ORI,SLL,ORI,SLL,ORI.
   When the lowest bit is clear, we can try, but reject a sequence with
   an extra SLL at the end.  */
#define MIPS_MAX_INTEGER_OPS 7


/* Global variables for machine-dependent things.  */

/* Threshold for data being put into the small data/bss area, instead
   of the normal data area.  */
int mips_section_threshold = -1;

/* Count the number of .file directives, so that .loc is up to date.  */
int num_source_filenames = 0;

/* Count the number of sdb related labels are generated (to find block
   start and end boundaries).  */
int sdb_label_count = 0;

/* Next label # for each statement for Silicon Graphics IRIS systems.  */
int sym_lineno = 0;

/* Linked list of all externals that are to be emitted when optimizing
   for the global pointer if they haven't been declared by the end of
   the program with an appropriate .comm or initialization.  */

struct extern_list GTY (())
{
  struct extern_list *next;     /* next external */
  const char *name;             /* name of the external */
  int size;                     /* size in bytes */
};

static GTY (()) struct extern_list *extern_head = 0;

/* Generates references for sections such that it can be reference in
   section relative addressing */
static void mips_output_section_bases (void);

/* Return symbol_ref which points to base of .bss section. */
static rtx mips_base_bss_rtx (void);

/* Return symbol_ref which points to base of .data section. */
static rtx mips_base_data_rtx (void);

static rtx mips_legitimize_section_relative_address (rtx, rtx, tree);

/* Name of the file containing the current function.  */
const char *current_function_file = "";

/* Number of nested .set noreorder, noat, nomacro, and volatile requests.  */
int set_noreorder;
int set_noat;
int set_nomacro;
int set_volatile;

/* The next branch instruction is a branch likely, not branch normal.  */
int mips_branch_likely;

/* Cached operands, and operator to compare for use in set/branch/trap
   on condition codes.  */
rtx branch_cmp[2];

/* what type of branch to use */
enum cmp_type branch_type;

/* The target cpu for code generation.  */
enum processor_type mips_arch;
const struct mips_cpu_info *mips_arch_info;

/* The target cpu for optimization and scheduling.  */
enum processor_type mips_tune;
const struct mips_cpu_info *mips_tune_info;

/* Which instruction set architecture to use.  */
int mips_isa;

/* Which ABI to use.  */
int mips_abi;

/* Strings to hold which cpu and instruction set architecture to use.  */
const char *mips_arch_string;   /* for -march=<xxx> */
const char *mips_tune_string;   /* for -mtune=<xxx> */
const char *mips_isa_string;    /* for -mips{1,2,3,4} */
const char *mips_abi_string;    /* for -mabi={32,n32,64,eabi} */

const char *mips_brlikely_string; /* -mbranch-likely={no,yes,predict} */
enum mips_brlikely mips_brlikely;
enum mips_brlikely mips_base_brlikely;

const char *mips_branch_cost_string; /* -mbranch-cost=N */
int mips_branch_cost;
int mips_base_branch_cost;

const char *mips_no_data_in_code_string; /* -mno-data-in-code */

const char *mips_code_xonly_string; /* -mcode-xonly */

const char *mips_flip_mips16_string; /* -mflip-mips16 */
static GTY(()) int mips16_flipper;

/* Whether we are generating mips16 hard float code.  In mips16 mode
   we always set TARGET_SOFT_FLOAT; this variable is nonzero if
   -msoft-float was not specified by the user, which means that we
   should arrange to call mips32 hard floating point code.  */
int mips16_hard_float;

/* Remember the ambient target flags, excluding mips16. */
static int mips_base_target_flags;

/* The mips16 command-line target flags only. */
static int mips_base_mips16;

const char *mips_cache_flush_func = CACHE_FLUSH_FUNC;

/* If TRUE, we split addresses into their high and low parts in the RTL.  */
int mips_split_addresses;

int mips_base_split_addresses;

/* Mode used for saving/restoring general purpose registers.  */
static enum machine_mode gpr_mode;

/* Array giving truth value on whether or not a given hard register
   can support a given mode.  */
char mips_hard_regno_mode_ok[(int)MAX_MACHINE_MODE][FIRST_PSEUDO_REGISTER];

/* In mips16, this tags the label_ref if it now moved to the inlined
   constant table by mips16_lay_out_constants */
#define MIPS16_INLINE_CONSTANT_TABLE_LABEL_P(RTX)                   \
  (RTL_FLAG_CHECK1("MIPS16_INLINE_CONSTANT_TABLE_LABEL_P", (RTX),   \
                   LABEL_REF)->unchanging)

/* True if this function return value will get returned in a
   floating-point register. */
#define MIPS_RETURN_FP_REG_P(mode)                              \
      ((GET_MODE_CLASS (mode) == MODE_FLOAT                     \
        || GET_MODE_CLASS (mode) == MODE_COMPLEX_FLOAT)         \
       && GET_MODE_UNIT_SIZE (mode) <= UNITS_PER_FPVALUE)

/* Returns a two character string representing a function floating
   point return mode, used to name MIPS16 function stubs. */
#define MIPS_RETURN_FP_REG_TYPE(mode)   \
      ((mode) == SFmode ? "sf"          \
       : (mode) == DFmode ? "df"        \
       : (mode) == SCmode ? "sc"        \
       : "dc")

/* List of all MIPS punctuation characters used by print_operand.  */
char mips_print_operand_punct[256];

/* Map GCC register number to debugger register number.  */
int mips_dbx_regno[FIRST_PSEUDO_REGISTER];

/* An alias set for the GOT.  */
static GTY(()) int mips_got_alias_set;

/* A copy of the original flag_delayed_branch: see override_options.  */
static int mips_flag_delayed_branch;

/* Similar copies of option settings */
static int mips_base_schedule_insns; /* flag_schedule_insns */
static int mips_base_align_loops; /* align_loops */
static int mips_base_align_jumps; /* align_jumps */
static int mips_base_align_functions; /* align_functions */

static GTY (()) int mips_output_filename_first_time = 1;

/* mips_split_p[X] is true if symbols of type X can be split by
   mips_split_symbol().  */
static bool mips_split_p[NUM_SYMBOL_TYPES];

/* mips_lo_relocs[X] is the relocation to use when a symbol of type X
   appears in a LO_SUM.  It can be null if such LO_SUMs aren't valid or
   if they are matched by a special .md file pattern.  */
static const char *mips_lo_relocs[NUM_SYMBOL_TYPES];

/* Likewise for HIGHs.  */
static const char *mips_hi_relocs[NUM_SYMBOL_TYPES];

/* Hardware names for the registers.  If -mrnames is used, this
   will be overwritten with mips_sw_reg_names.  */

char mips_reg_names[][8] =
{
 "$0",   "$1",   "$2",   "$3",   "$4",   "$5",   "$6",   "$7",
 "$8",   "$9",   "$10",  "$11",  "$12",  "$13",  "$14",  "$15",
 "$16",  "$17",  "$18",  "$19",  "$20",  "$21",  "$22",  "$23",
 "$24",  "$25",  "$26",  "$27",  "$28",  "$sp",  "$fp",  "$31",
 "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
 "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
 "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
 "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31",
 "hi",   "lo",   "",     "$fcc0","$fcc1","$fcc2","$fcc3","$fcc4",
 "$fcc5","$fcc6","$fcc7","", "", "$arg", "$frame", "$fakec",
 "$c0r0", "$c0r1", "$c0r2", "$c0r3", "$c0r4", "$c0r5", "$c0r6", "$c0r7",
 "$c0r8", "$c0r9", "$c0r10","$c0r11","$c0r12","$c0r13","$c0r14","$c0r15",
 "$c0r16","$c0r17","$c0r18","$c0r19","$c0r20","$c0r21","$c0r22","$c0r23",
 "$c0r24","$c0r25","$c0r26","$c0r27","$c0r28","$c0r29","$c0r30","$c0r31",
 "$c2r0", "$c2r1", "$c2r2", "$c2r3", "$c2r4", "$c2r5", "$c2r6", "$c2r7",
 "$c2r8", "$c2r9", "$c2r10","$c2r11","$c2r12","$c2r13","$c2r14","$c2r15",
 "$c2r16","$c2r17","$c2r18","$c2r19","$c2r20","$c2r21","$c2r22","$c2r23",
 "$c2r24","$c2r25","$c2r26","$c2r27","$c2r28","$c2r29","$c2r30","$c2r31",
 "$c3r0", "$c3r1", "$c3r2", "$c3r3", "$c3r4", "$c3r5", "$c3r6", "$c3r7",
 "$c3r8", "$c3r9", "$c3r10","$c3r11","$c3r12","$c3r13","$c3r14","$c3r15",
 "$c3r16","$c3r17","$c3r18","$c3r19","$c3r20","$c3r21","$c3r22","$c3r23",
 "$c3r24","$c3r25","$c3r26","$c3r27","$c3r28","$c3r29","$c3r30","$c3r31",
 "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo", "$dsp_po",
 "$dsp_sc", "$dsp_ca", "$dsp_ou", "$dsp_cc", "$dsp_ef"
};

/* Mips software names for the registers, used to overwrite the
   mips_reg_names array.  */

char mips_sw_reg_names[][8] =
{
  "$zero","$at",  "$v0",  "$v1",  "$a0",  "$a1",  "$a2",  "$a3",
  "$t0",  "$t1",  "$t2",  "$t3",  "$t4",  "$t5",  "$t6",  "$t7",
  "$s0",  "$s1",  "$s2",  "$s3",  "$s4",  "$s5",  "$s6",  "$s7",
  "$t8",  "$t9",  "$k0",  "$k1",  "$gp",  "$sp",  "$fp",  "$ra",
  "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
  "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
  "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
  "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31",
  "hi",   "lo",   "",     "$fcc0","$fcc1","$fcc2","$fcc3","$fcc4",
  "$fcc5","$fcc6","$fcc7","$rap", "", "$arg", "$frame", "$fakec",
  "$c0r0", "$c0r1", "$c0r2", "$c0r3", "$c0r4", "$c0r5", "$c0r6", "$c0r7",
  "$c0r8", "$c0r9", "$c0r10","$c0r11","$c0r12","$c0r13","$c0r14","$c0r15",
  "$c0r16","$c0r17","$c0r18","$c0r19","$c0r20","$c0r21","$c0r22","$c0r23",
  "$c0r24","$c0r25","$c0r26","$c0r27","$c0r28","$c0r29","$c0r30","$c0r31",
  "$c2r0", "$c2r1", "$c2r2", "$c2r3", "$c2r4", "$c2r5", "$c2r6", "$c2r7",
  "$c2r8", "$c2r9", "$c2r10","$c2r11","$c2r12","$c2r13","$c2r14","$c2r15",
  "$c2r16","$c2r17","$c2r18","$c2r19","$c2r20","$c2r21","$c2r22","$c2r23",
  "$c2r24","$c2r25","$c2r26","$c2r27","$c2r28","$c2r29","$c2r30","$c2r31",
  "$c3r0", "$c3r1", "$c3r2", "$c3r3", "$c3r4", "$c3r5", "$c3r6", "$c3r7",
  "$c3r8", "$c3r9", "$c3r10","$c3r11","$c3r12","$c3r13","$c3r14","$c3r15",
  "$c3r16","$c3r17","$c3r18","$c3r19","$c3r20","$c3r21","$c3r22","$c3r23",
  "$c3r24","$c3r25","$c3r26","$c3r27","$c3r28","$c3r29","$c3r30","$c3r31",
  "$ac1hi", "$ac1lo", "$ac2hi", "$ac2lo", "$ac3hi", "$ac3lo", "$dsp_po",
  "$dsp_sc", "$dsp_ca", "$dsp_ou", "$dsp_cc", "$dsp_ef"
};

/* Map hard register number to register class */
const enum reg_class mips_regno_to_class[] =
{
  LEA_REGS,     LEA_REGS,       M16_NA_REGS,    M16_NA_REGS,
  M16_REGS,     M16_REGS,       M16_REGS,       M16_REGS,
  LEA_REGS,     LEA_REGS,       LEA_REGS,       LEA_REGS,
  LEA_REGS,     LEA_REGS,       LEA_REGS,       LEA_REGS,
  M16_NA_REGS,  M16_NA_REGS,    LEA_REGS,       LEA_REGS,
  LEA_REGS,     LEA_REGS,       LEA_REGS,       LEA_REGS,
  T_REG,        PIC_FN_ADDR_REG, LEA_REGS,      LEA_REGS,
  LEA_REGS,     SP_REG,         LEA_REGS,       LEA_REGS,
  FP_REGS,      FP_REGS,        FP_REGS,        FP_REGS,
  FP_REGS,      FP_REGS,        FP_REGS,        FP_REGS,
  FP_REGS,      FP_REGS,        FP_REGS,        FP_REGS,
  FP_REGS,      FP_REGS,        FP_REGS,        FP_REGS,
  FP_REGS,      FP_REGS,        FP_REGS,        FP_REGS,
  FP_REGS,      FP_REGS,        FP_REGS,        FP_REGS,
  FP_REGS,      FP_REGS,        FP_REGS,        FP_REGS,
  FP_REGS,      FP_REGS,        FP_REGS,        FP_REGS,
  MD0_REG,      MD1_REG,        NO_REGS,        ST_REGS,
  ST_REGS,      ST_REGS,        ST_REGS,        ST_REGS,
  ST_REGS,      ST_REGS,        ST_REGS,        NO_REGS,
  NO_REGS,      ALL_REGS,       ALL_REGS,       NO_REGS,
  COP0_REGS,    COP0_REGS,      COP0_REGS,      COP0_REGS,
  COP0_REGS,    COP0_REGS,      COP0_REGS,      COP0_REGS,
  COP0_REGS,    COP0_REGS,      COP0_REGS,      COP0_REGS,
  COP0_REGS,    COP0_REGS,      COP0_REGS,      COP0_REGS,
  COP0_REGS,    COP0_REGS,      COP0_REGS,      COP0_REGS,
  COP0_REGS,    COP0_REGS,      COP0_REGS,      COP0_REGS,
  COP0_REGS,    COP0_REGS,      COP0_REGS,      COP0_REGS,
  COP0_REGS,    COP0_REGS,      COP0_REGS,      COP0_REGS,
  COP2_REGS,    COP2_REGS,      COP2_REGS,      COP2_REGS,
  COP2_REGS,    COP2_REGS,      COP2_REGS,      COP2_REGS,
  COP2_REGS,    COP2_REGS,      COP2_REGS,      COP2_REGS,
  COP2_REGS,    COP2_REGS,      COP2_REGS,      COP2_REGS,
  COP2_REGS,    COP2_REGS,      COP2_REGS,      COP2_REGS,
  COP2_REGS,    COP2_REGS,      COP2_REGS,      COP2_REGS,
  COP2_REGS,    COP2_REGS,      COP2_REGS,      COP2_REGS,
  COP2_REGS,    COP2_REGS,      COP2_REGS,      COP2_REGS,
  COP3_REGS,    COP3_REGS,      COP3_REGS,      COP3_REGS,
  COP3_REGS,    COP3_REGS,      COP3_REGS,      COP3_REGS,
  COP3_REGS,    COP3_REGS,      COP3_REGS,      COP3_REGS,
  COP3_REGS,    COP3_REGS,      COP3_REGS,      COP3_REGS,
  COP3_REGS,    COP3_REGS,      COP3_REGS,      COP3_REGS,
  COP3_REGS,    COP3_REGS,      COP3_REGS,      COP3_REGS,
  COP3_REGS,    COP3_REGS,      COP3_REGS,      COP3_REGS,
  COP3_REGS,    COP3_REGS,      COP3_REGS,      COP3_REGS,
  DSP_ACC_REGS, DSP_ACC_REGS,   DSP_ACC_REGS,   DSP_ACC_REGS,
  DSP_ACC_REGS, DSP_ACC_REGS,   DSP_CNTL_REG,   DSP_CNTL_REG,
  DSP_CNTL_REG, DSP_CNTL_REG,   DSP_CNTL_REG,   DSP_CNTL_REG
};

/* Map register constraint character to register class.  */
enum reg_class mips_char_to_class[256];

/* A table describing all the processors gcc knows about.  Names are
   matched in the order listed.  The first mention of an ISA level is
   taken as the canonical name for that ISA.

   To ease comparison, please keep this table in the same order as
   gas's mips_cpu_info_table[].  */
const struct mips_cpu_info mips_cpu_info_table[] = {
  /* Entries for generic ISAs */
  { "mips1", PROCESSOR_R3000, 1,
    0,
    12, 0, 4, 0 },
  { "mips2", PROCESSOR_R6000, 2,
    0,
    17, 0, 0, 0 },
  { "mips3", PROCESSOR_R4000, 3,
    0,
    10, 20, 0, 0 },
  { "mips4", PROCESSOR_R8000, 4,
    0,
    17, 0, 0, 0 },
  { "mips32", PROCESSOR_4KC, 32,
    0,
    4, 0, 0, 0 },
  { "mips32r2", PROCESSOR_24K, 33,
    MIPS_OPT_FETCH64,
    5, 0, 8, 10 },
  { "mips64", PROCESSOR_5KC, 64,
    0,
    4, 11, 4, 5 },
  { "mips64r2", PROCESSOR_20KC, 65,
    0,
    4, 7, 4, 5 },

  /* MIPS I */
  { "r3000", PROCESSOR_R3000, 1,
    0,
    12, 0, 4, 5 },
  { "r2000", PROCESSOR_R3000, 1, /* = r3000 */
    0,
    12, 0, 4, 5 },
  { "r3900", PROCESSOR_R3900, 1,
    0,
    12, 0, 4, 5 },

  /* MIPS II */
  { "r6000", PROCESSOR_R6000, 2,
    0,
    17, 0, 5, 6 },

  /* MIPS III */
  { "r4000", PROCESSOR_R4000, 3,
    0,
    10, 20, 0, 0 },
  { "vr4100", PROCESSOR_R4100, 3,
    0,
    1, 4, 0, 0 },
  { "vr4111", PROCESSOR_R4111, 3,
    0,
    1, 4, 0, 0 },
  { "vr4120", PROCESSOR_R4120, 3,
    0,
    1, 4, 0, 0 },
  { "vr4300", PROCESSOR_R4300, 3,
    0,
    5, 8, 0, 0 },
  { "r4400", PROCESSOR_R4000, 3, /* = r4000 */
    0,
    10, 20, 0, 0 },
  { "r4600", PROCESSOR_R4600, 3,
    0,
    10, 12, 0, 0 },
  { "orion", PROCESSOR_R4600, 3, /* = r4600 */
    0,
    10, 12, 0, 0 },
  { "r4650", PROCESSOR_R4650, 3,
    0,
    4, 6, 0, 0 },

  /* MIPS IV */
  { "r8000", PROCESSOR_R8000, 4,
    0,
    17, 0, 0, 0 },
  { "vr5000", PROCESSOR_R5000, 4,
    0,
    5, 6, 4, 5 },
  { "vr5400", PROCESSOR_R5400, 4,
    0,
    3, 4, 5, 6 },
  { "vr5500", PROCESSOR_R5500, 4,
    0,
    3, 4, 5, 6 },
  { "rm7000", PROCESSOR_R7000, 4,
    0,
    5, 9, 6, 9 },
  { "rm9000", PROCESSOR_R9000, 4,
    0,
    3, 8, 6, 9 },

  /* MIPS32 */

  /* 4K family */
  { "4kc", PROCESSOR_4KC, 32,
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT,
    4, 0, 0, 0 },
  { "4km", PROCESSOR_4KC, 32,
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT,
    4, 0, 0, 0 },
  { "4kp", PROCESSOR_4KP, 32,		/* 4K + slow multiplier */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT,
    32, 0, 0, 0 },

  { "4ksc", PROCESSOR_4KC, 32,
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT | MIPS_OPT_SMARTMIPS,
    4, 0, 0, 0 },

  /* MIPS32 Release 2 */

  /* M4K family */
  { "m4k", PROCESSOR_4KC, 33,
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT,
    4, 0, 0, 0 },
  { "m4kp", PROCESSOR_4KP, 33,		/* M4K with slow multiplier */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT,
    32, 0, 0, 0 },

  /* 4KE family */
  { "4kec", PROCESSOR_4KC, 33,
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT,
    4, 0, 0, 0 },
  { "4kem", PROCESSOR_4KC, 33,
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT,
    4, 0, 0, 0 },
  { "4kep", PROCESSOR_4KP, 33,		/* 4KE with slow multiplier */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT,
    32, 0, 0, 0 },

  { "4ksd", PROCESSOR_4KC, 33,
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT | MIPS_OPT_SMARTMIPS,
    4, 0, 0, 0 },

  /* 24K family */
  { "24k", PROCESSOR_24K, 33, 		/* Alias to 24kf2_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64,
    5, 0, 8, 10 },
  { "24kc", PROCESSOR_24K, 33, 		/* 24K no FPU */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_SOFTFLOAT,
    5, 0, 8, 10 },
  { "24kf2_1", PROCESSOR_24K, 33,	/* 24K 2:1 core/fpu ratio */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64,
    5, 0, 8, 10 },
  { "24kf", PROCESSOR_24K, 33, 		/* Alias to 24kf2_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64,
    5, 0, 8, 10 },
  { "24kf1_1", PROCESSOR_24KX, 33,	/* 24K 1:1 core/fpu ratio */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64,
    5, 0, 4, 5 },
  { "24kfx", PROCESSOR_24KX, 33, 	/* Alias to 24kf1_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64,
    5, 0, 4, 5 },
  { "24kx", PROCESSOR_24KX, 33, 	/* Alias to 24kf1_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64,
    5, 0, 4, 5 },

  /* 24ke family - identical to 24k for the compiler */
  { "24ke", PROCESSOR_24K, 33,		/* Alias to 24kef2_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 8, 10 },
  { "24kec", PROCESSOR_24K, 33, 	/* 24KE no FPU */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_SOFTFLOAT | MIPS_OPT_DSP,
    5, 0, 8, 10 },
  { "24kef2_1", PROCESSOR_24K, 33,	/* 24KE 2:1 core/fpu ratio */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 8, 10 },
  { "24kef", PROCESSOR_24K, 33,		/* Alias to 24kef2_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 8, 10 },
  { "24kef1_1", PROCESSOR_24KX, 33,	/* 24KE 1:1 FPU */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 4, 5 },
  { "24kefx", PROCESSOR_24KX, 33,	/* Alias to 24kef1_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 4, 5 },
  { "24kex", PROCESSOR_24KX, 33,	/* Alias to 24kef1_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 4, 5 },

  /* 34K family - identical pipeline to 24ke, currently. */
  { "34k", PROCESSOR_24K, 33,		/* Alias to 34kf2_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 8, 10 },
  { "34kc", PROCESSOR_24K, 33,		/* 34K no FPU */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_SOFTFLOAT | MIPS_OPT_DSP,
    5, 0, 8, 10 },
  { "34kf2_1", PROCESSOR_24K, 33,	/* 34K 2:1 core/fpu ratio */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 8, 10 },
  { "34kf", PROCESSOR_24K, 33,		/* Alias to 34kf2_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 8, 10 },
  { "34kf1_1", PROCESSOR_24KX, 33,	/* 34K 1:1 core/fpu ratio */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 4, 5 },
  { "34kfx", PROCESSOR_24KX, 33,	/* Alias to 34kf1_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 4, 5 },
  { "34kx", PROCESSOR_24KX, 33,	        /* Alias to 34kf1_1 */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    5, 0, 4, 5 },

  /* 74K family */
  { "74kc", PROCESSOR_74K, 33,          /* 74K no FPU */
    MIPS_OPT_FETCH64 | MIPS_OPT_DSP | MIPS_OPT_SOFTFLOAT,
    4, 0, 0, 0 },
  { "74kf2_1", PROCESSOR_74K, 33,	/* 74K 2:1 core/fpu ratio */
    MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    4, 0, 10, 10 },
  { "74kf", PROCESSOR_74K, 33,		/* Alias to 74kf2_1 */
    MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    4, 0, 10, 10 },
  { "74kf1_1", PROCESSOR_74KX, 33,	/* 74K 1:1 core/fpu ratio */
    MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    4, 0, 5, 5 },
  { "74kfx", PROCESSOR_74KX, 33,	/* Alias to 74kf1_1 */
    MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    4, 0, 5, 5 },
  { "74kx", PROCESSOR_74KX, 33,	        /* Alias to 74kf1_1 */
    MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    4, 0, 5, 5 },
  { "74kf3_2", PROCESSOR_74KZ, 33,      /* 74k 3:2 core/fpu ratio */
    MIPS_OPT_FETCH64 | MIPS_OPT_DSP,
    4, 0, 7, 7 },

  /* MIPS64 */

  /* 5K family */
  { "5kc", PROCESSOR_5KC, 64,		/* 5K no FPU */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64 | MIPS_OPT_SOFTFLOAT,
    4, 11, 4, 5 },
  { "5kf", PROCESSOR_5KC, 64,		/* 5K + FPU */
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_FETCH64,
    4, 11, 4, 5 },

  /* 20Kc family */
  { "20kc", PROCESSOR_20KC, 64, 	/* 20Kc + FPU */
    MIPS_OPT_PREDICTLIKELY | MIPS_OPT_FETCH64,
    4, 7, 4, 5 },
  { "25kf", PROCESSOR_20KC, 64, 	/* 25Kf + FPU */
    MIPS_OPT_PREDICTLIKELY | MIPS_OPT_FETCH64,
    4, 7, 4, 5 },

  { "sb1", PROCESSOR_SB1, 64,
    0,
    4, 7, 0, 0 },		/* guess! */

  /* Microchip PIC32MX - Copy of m4k */
  { "pic32mx", PROCESSOR_4KC, 33,
    MIPS_OPT_BRANCHLIKELY | MIPS_OPT_SOFTFLOAT,
    4, 0, 0, 0 },

  { "sr71000", PROCESSOR_SR71000, 64,
    0,
    4, 6, 8, 16 },

  /* End marker */
  { 0, 0, 0,
    0,
    0, 0, 0, 0 }
};

static const struct attribute_spec mips_attribute_table[] =
{
  /* { name, min_len, max_len, decl_req, type_req, fn_type_req, handler } */
  /* Function calls made to this symbol must be done indirectly, because
     it may lie outside of the 28 bit addressing range of a normal function
     call.  */
  { "longcall",         0, 0, false, true, true, NULL },
  { "far",              0, 0, false, true, true,  NULL },
  { "near",             0, 0, false, true, true,  NULL },
  /* Switch MIPS16 ASE on and off per-function. */
  { "mips16",           0, 0, false, true, true, NULL },
  { "nomips16",         0, 0, false, true, true, NULL },
  /* Microchip: allow functions to be specified as interrupt handlers */
  { "interrupt",        1, 1, true, true, true, mchp_interrupt_attribute },
  { "vector",           1, 64, true, false, false, mchp_vector_attribute },
  { "at_vector",        1, 1, true, false, false, mchp_at_vector_attribute },
  /* also allow functions to be created without prologue/epilogue code */
  { "naked",            0, 0, true, false, false, NULL },
  { NULL,               0, 0, false, false, false, NULL }
};

/* Microchip devices allow an interrupt specification via pragma.
   We keep a list of the functions specified via pragma and apply the
   interrupt attribute to them when we see a declaration later. */
struct interrupt_pragma_spec
{
  const char *name;     /* name of the interrupt function */
  tree ipl;             /* IPL */
  unsigned irq_number;  /* If placing the interrup at the actual
                           vector, which one? Zero means don't place the
                           handler at the vector. */
  struct interrupt_pragma_spec *next;
};
struct interrupt_pragma_spec *interrupt_pragma_list_head;

/* Microchip devices also allow interrupt vector dispactch functions
   to be defined via a pragma or an attribute (of the interrupt handler).
   We keep a list of the dispatch functions needed and emit them at
   the end of processing the translation unit. */
struct vector_dispatch_spec
{
  const char *target; /* target function name */
  int vector_number;  /* exception vector table number */
  struct vector_dispatch_spec *next;
};
struct vector_dispatch_spec *vector_dispatch_list_head;

/* Is the current function an interrupt? If so, how is context handled? */
enum function_type_tag { NON_INTERRUPT, SOFTWARE_CONTEXT_SAVE, SRS_CONTEXT_SAVE};
enum function_type_tag current_function_type;

/* Nonzero if -march should decide the default value of MASK_SOFT_FLOAT.  */
#ifndef MIPS_MARCH_CONTROLS_SOFT_FLOAT
#define MIPS_MARCH_CONTROLS_SOFT_FLOAT 0
#endif

/* Initialize the GCC target structure.  */
#undef TARGET_ASM_ALIGNED_HI_OP
#define TARGET_ASM_ALIGNED_HI_OP "\t.half\t"
#undef TARGET_ASM_ALIGNED_SI_OP
#define TARGET_ASM_ALIGNED_SI_OP "\t.word\t"
#undef TARGET_ASM_INTEGER
#define TARGET_ASM_INTEGER mips_assemble_integer

#undef TARGET_ASM_FUNCTION_PROLOGUE
#define TARGET_ASM_FUNCTION_PROLOGUE mips_output_function_prologue
#undef TARGET_ASM_FUNCTION_EPILOGUE
#define TARGET_ASM_FUNCTION_EPILOGUE mips_output_function_epilogue
#undef TARGET_ASM_SELECT_RTX_SECTION
#define TARGET_ASM_SELECT_RTX_SECTION mips_select_rtx_section

#undef TARGET_SCHED_ADJUST_COST
#define TARGET_SCHED_ADJUST_COST mips_adjust_cost
#undef TARGET_SCHED_IS_COSTLY_DEPENDENCE      
#define TARGET_SCHED_IS_COSTLY_DEPENDENCE mips_is_costly_dependence
#undef TARGET_SCHED_VARIABLE_ISSUE
#define TARGET_SCHED_VARIABLE_ISSUE mips_variable_issue
#undef  TARGET_SCHED_REORDER
#define TARGET_SCHED_REORDER mips_sched_reorder
#undef TARGET_SCHED_DFA_NEW_CYCLE
#define TARGET_SCHED_DFA_NEW_CYCLE mips_dfa_new_cycle
#undef TARGET_SCHED_ISSUE_RATE
#define TARGET_SCHED_ISSUE_RATE mips_issue_rate
#undef TARGET_SCHED_USE_DFA_PIPELINE_INTERFACE
#define TARGET_SCHED_USE_DFA_PIPELINE_INTERFACE mips_use_dfa_pipeline_interface
#undef TARGET_SCHED_REGNO_CLOBBER_DEPENDENCIES
#define TARGET_SCHED_REGNO_CLOBBER_DEPENDENCIES \
  mips_sched_regno_clobber_dependencies

#undef TARGET_FUNCTION_OK_FOR_SIBCALL
#define TARGET_FUNCTION_OK_FOR_SIBCALL mips_function_ok_for_sibcall

#undef TARGET_PREPARE_FUNCTION_START
#define TARGET_PREPARE_FUNCTION_START mips_prepare_function_start

#undef TARGET_VALID_POINTER_MODE
#define TARGET_VALID_POINTER_MODE mips_valid_pointer_mode
#undef TARGET_RTX_COSTS
#define TARGET_RTX_COSTS mips_rtx_costs
#undef TARGET_ADDRESS_COST
#define TARGET_ADDRESS_COST mips_address_cost

#undef TARGET_ENCODE_SECTION_INFO
#define TARGET_ENCODE_SECTION_INFO mips_encode_section_info
#undef TARGET_IN_SMALL_DATA_P
#define TARGET_IN_SMALL_DATA_P mips_in_small_data_p

#undef TARGET_CANNOT_FORCE_CONST_MEM
#define TARGET_CANNOT_FORCE_CONST_MEM mips_cannot_force_const_mem

#undef TARGET_MACHINE_DEPENDENT_REORG
#define TARGET_MACHINE_DEPENDENT_REORG mips_reorg

#undef TARGET_ATTRIBUTE_TABLE
#define TARGET_ATTRIBUTE_TABLE mips_attribute_table

#undef TARGET_ASM_FILE_START
#undef TARGET_ASM_FILE_END
#if TARGET_IRIX
#define TARGET_ASM_FILE_START irix_file_start
#define TARGET_ASM_FILE_END irix_file_end
#else
#define TARGET_ASM_FILE_START mips_file_start
#define TARGET_ASM_FILE_END mips_file_end
#endif
#undef TARGET_ASM_FILE_START_FILE_DIRECTIVE
#define TARGET_ASM_FILE_START_FILE_DIRECTIVE true

#if TARGET_IRIX
#undef TARGET_SECTION_TYPE_FLAGS
#define TARGET_SECTION_TYPE_FLAGS irix_section_type_flags
#endif

#undef TARGET_INIT_LIBFUNCS
#define TARGET_INIT_LIBFUNCS mips_init_libfuncs

/* For MIPS builtin functions */
#undef TARGET_INIT_BUILTINS
#define TARGET_INIT_BUILTINS mips_init_builtins

#undef TARGET_EXPAND_BUILTIN
#define TARGET_EXPAND_BUILTIN mips_expand_builtin

#undef TARGET_BUILD_BUILTIN_VA_LIST
#define TARGET_BUILD_BUILTIN_VA_LIST mips_build_builtin_va_list
#undef TARGET_RETURN_IN_MSB
#define TARGET_RETURN_IN_MSB mips_return_in_msb

#undef TARGET_ASM_OUTPUT_MI_THUNK
#define TARGET_ASM_OUTPUT_MI_THUNK mips_output_mi_thunk
#undef TARGET_ASM_CAN_OUTPUT_MI_THUNK
#define TARGET_ASM_CAN_OUTPUT_MI_THUNK hook_bool_tree_hwi_hwi_tree_true

#undef  TARGET_COMP_TYPE_ATTRIBUTES
#define TARGET_COMP_TYPE_ATTRIBUTES mips_comp_type_attributes

#undef  TARGET_PCH_VALID_P
#define TARGET_PCH_VALID_P mips_pch_valid_p

#undef TARGET_WEB_MARK_REGISTER
#define TARGET_WEB_MARK_REGISTER mips_web_mark_register

#undef TARGET_SPILL_AUTO_REGISTER
#define TARGET_SPILL_AUTO_REGISTER mips_spill_auto_register

/* Microchip PIC32 specific stuff */
#undef TARGET_INSERT_ATTRIBUTES
#define TARGET_INSERT_ATTRIBUTES mchp_target_insert_attributes

struct gcc_target targetm = TARGET_INITIALIZER;

#ifdef TARGET_MCHP_PIC32MX
static int pic32_license_valid = 1;
#endif

/* Return 0 if the attributes for two types are incompatible, 1 if they
   are compatible, and 2 if they are nearly compatible (which causes a
   warning to be generated).  */

static int
mips_comp_type_attributes (tree type1, tree type2)
{
  int l1, l2, s1, s2;

  /* Check for mismatch of non-default calling convention.  */
  if (TREE_CODE (type1) != FUNCTION_TYPE)
    return 1;

  /* Check for mismatched call attributes.  */
  l1 = (lookup_attribute ("longcall", TYPE_ATTRIBUTES (type1)) != NULL
        || lookup_attribute ("far", TYPE_ATTRIBUTES (type1)) != NULL);
  l2 = (lookup_attribute ("longcall", TYPE_ATTRIBUTES (type2)) != NULL
        || lookup_attribute ("far", TYPE_ATTRIBUTES (type2)) != NULL);
  s1 = lookup_attribute ("near", TYPE_ATTRIBUTES (type1)) != NULL;
  s2 = lookup_attribute ("near", TYPE_ATTRIBUTES (type2)) != NULL;

  /* Disallow mixed near/far attributes.  */
  if ((l1 & s2) || (l2 & s1))
    return 0;

  l1 = lookup_attribute ("nomips16", TYPE_ATTRIBUTES (type1)) != NULL;
  l2 = lookup_attribute ("nomips16", TYPE_ATTRIBUTES (type2)) != NULL;
  s1 = lookup_attribute ("mips16", TYPE_ATTRIBUTES (type1)) != NULL;
  s2 = lookup_attribute ("mips16", TYPE_ATTRIBUTES (type2)) != NULL;

  /* Only bother to check if an attribute is defined.  */
  if (l1 | l2 | s1 | s2)
    {
      /* Disallow mixed mips16/nomips16 attributes.  */
      if ((l1 & s2) || (l2 & s1))
        return 0;

      /* If one type has an attribute, the other must have the same attribute.  */
      if ((l1 != l2) || (s1 != s2))
        return 0;
    }

  return 1;
}


/* Classify symbol X, which must be a SYMBOL_REF or a LABEL_REF.  */

static enum mips_symbol_type
mips_classify_symbol (rtx x)
{
  if (GET_CODE (x) == LABEL_REF)
    {
      if (MIPS16_INLINE_CONSTANT_TABLE_LABEL_P (x) && TARGET_MIPS16)
        return SYMBOL_CONSTANT_POOL;
      return (TARGET_ABICALLS ? SYMBOL_GOT_LOCAL : SYMBOL_GENERAL);
    }

  if (GET_CODE (x) != SYMBOL_REF)
    abort ();

  if (CONSTANT_POOL_ADDRESS_P (x))
    {
      if (TARGET_MIPS16)
        {
	  /* Use gp-relative addressing if allowed, otherwise return 
	     SYMBOL_CONSTANT_POOL which will be put into .rodata if
	     TARGET_EMBEDDED_DATA, access is split into hi/lo pair.  */
	  if (TARGET_NO_DATA_IN_CODE
	      && GET_MODE_SIZE (get_pool_mode (x)) <= mips_section_threshold)
	    return SYMBOL_SMALL_DATA;
	  else
	    return SYMBOL_CONSTANT_POOL;
        }

      if (TARGET_ABICALLS)
        return SYMBOL_GOT_LOCAL;

      if (! TARGET_EMBEDDED_DATA
          && GET_MODE_SIZE (get_pool_mode (x)) <= mips_section_threshold)
        return SYMBOL_SMALL_DATA;

      return SYMBOL_GENERAL;
    }

  /* Determine if section relative addressing can be use, we make it like
     SYMBOL_CONSTANT_POOL. This has priority over gp-relative addressing. */
  if (flag_section_relative_addressing && TARGET_MIPS16
      && SYMBOL_REF_DECL (x)
      && !TREE_CONSTANT_POOL_ADDRESS_P (x))
    {
      struct cgraph_varpool_node *node =
        cgraph_varpool_node_for_identifier (DECL_ASSEMBLER_NAME (SYMBOL_REF_DECL (x)));
      if (node && node->needed && node->sectrel_node)
        return SYMBOL_SECTION_RELATIVE;
    }

  if (SYMBOL_REF_SMALL_P (x) && TARGET_GP_OPT)
    return SYMBOL_SMALL_DATA;

  /* When generating mips16 code, SYMBOL_REF_FLAG indicates a string
     in the current function's constant pool.  */
  if (TARGET_MIPS16 && SYMBOL_REF_FLAG (x))
    return SYMBOL_CONSTANT_POOL;

  if (TARGET_ABICALLS)
    {
      if (SYMBOL_REF_DECL (x) == 0)
        return SYMBOL_REF_LOCAL_P (x) ? SYMBOL_GOT_LOCAL : SYMBOL_GOT_GLOBAL;

      /* There are three cases to consider:

            - o32 PIC (either with or without explicit relocs)
            - n32/n64 PIC without explicit relocs
            - n32/n64 PIC with explicit relocs

         In the first case, both local and global accesses will use an
         R_MIPS_GOT16 relocation.  We must correctly predict which of
         the two semantics (local or global) the assembler and linker
         will apply.  The choice doesn't depend on the symbol's
         visibility, so we deliberately ignore decl_visibility and
         binds_local_p here.

         In the second case, the assembler will not use R_MIPS_GOT16
         relocations, but it chooses between local and global accesses
         in the same way as for o32 PIC.

         In the third case we have more freedom since both forms of
         access will work for any kind of symbol.  However, there seems
         little point in doing things differently.  */
      if (DECL_P (SYMBOL_REF_DECL (x)) && (TREE_PUBLIC (SYMBOL_REF_DECL (x))
                                           || DECL_WEAK (SYMBOL_REF_DECL (x))
                                           || DECL_ONE_ONLY (SYMBOL_REF_DECL (x))))
        return SYMBOL_GOT_GLOBAL;

      return SYMBOL_GOT_LOCAL;
    }

  if (SYMBOL_REF_LONGCALL_P (x))
    return SYMBOL_LONGCALL;

  return SYMBOL_GENERAL;
}


/* Split X into a base and a constant offset, storing them in *BASE
   and *OFFSET respectively.  */

static void
mips_split_const (rtx x, rtx *base, HOST_WIDE_INT *offset)
{
  *offset = 0;

  if (GET_CODE (x) == CONST)
    x = XEXP (x, 0);

  if (GET_CODE (x) == PLUS && GET_CODE (XEXP (x, 1)) == CONST_INT)
    {
      *offset += INTVAL (XEXP (x, 1));
      x = XEXP (x, 0);
    }
  *base = x;
}


/* Return true if SYMBOL is a SYMBOL_REF and OFFSET + SYMBOL points
   to the same object as SYMBOL.  */

static bool
mips_offset_within_object_p (rtx symbol, HOST_WIDE_INT offset)
{
  if (GET_CODE (symbol) == LABEL_REF)
    /* A MIPS16 constant pool entry which has been hoisted inline into
       the code has a LABEL_REF, not a SYMBOL_REF, so we no have
       access to the underlying constant pool data. We'll just have to
       assume that the offset is still valid - at this late stage in
       the compile process it should have been checked before. */
    return TARGET_MIPS16 && MIPS16_INLINE_CONSTANT_TABLE_LABEL_P (symbol)
      && offset >= 0 && offset < MAX_FIXED_MODE_SIZE / BITS_PER_UNIT;

  if (GET_CODE (symbol) != SYMBOL_REF)
    return false;

  if (CONSTANT_POOL_ADDRESS_P (symbol)
      && offset >= 0
      && offset < (int) GET_MODE_SIZE (get_pool_mode (symbol)))
    return true;

  if (SYMBOL_REF_DECL (symbol) != 0
      && offset >= 0
      && offset < int_size_in_bytes (TREE_TYPE (SYMBOL_REF_DECL (symbol))))
    return true;

  return false;
}


/* Return true if X is a symbolic constant that can be calculated in
   the same way as a bare symbol.  If it is, store the type of the
   symbol in *SYMBOL_TYPE.  */

static bool
mips_symbolic_constant_p (rtx x, enum mips_symbol_type *symbol_type)
{
  HOST_WIDE_INT offset;

  mips_split_const (x, &x, &offset);
  if (UNSPEC_ADDRESS_P (x))
    *symbol_type = UNSPEC_ADDRESS_TYPE (x);
  else if (GET_CODE (x) == SYMBOL_REF || GET_CODE (x) == LABEL_REF)
    *symbol_type = mips_classify_symbol (x);
  else
    return false;

  if (offset == 0)
    return true;

  /* Check whether a nonzero offset is valid for the underlying
     relocations.  */
  switch (*symbol_type)
    {
    case SYMBOL_GENERAL:
    case SYMBOL_LONGCALL:
      /* If the target has 64-bit pointers and the object file only
         supports 32-bit symbols, the values of those symbols will be
         sign-extended.  In this case we can't allow an arbitrary offset
         in case the 32-bit value X + OFFSET has a different sign from X.  */
      if (Pmode == DImode && !ABI_HAS_64BIT_SYMBOLS)
        return mips_offset_within_object_p (x, offset);

      /* In other cases the relocations can handle any offset.  */
      return true;

    case SYMBOL_SMALL_DATA:
    case SYMBOL_CONSTANT_POOL:
      /* Make sure that the offset refers to something within the
         underlying object.  This should guarantee that the final
         PC- or GP-relative offset is within the 16-bit limit.  */
      return mips_offset_within_object_p (x, offset);

    case SYMBOL_GOT_LOCAL:
    case SYMBOL_GOTOFF_PAGE:
      /* The linker should provide enough local GOT entries for a
         16-bit offset.  Larger offsets may lead to GOT overflow.  */
      return SMALL_OPERAND (offset);

    case SYMBOL_SECTION_RELATIVE:
    case SYMBOL_GOT_GLOBAL:
    case SYMBOL_GOTOFF_GLOBAL:
    case SYMBOL_GOTOFF_CALL:
    case SYMBOL_GOTOFF_LOADGP:
      return false;
    }
  abort ();
}


/* This function is used to implement REG_MODE_OK_FOR_BASE_P.  */

int
mips_regno_mode_ok_for_base_p (int regno, enum machine_mode mode, int strict)
{
  if (regno >= FIRST_PSEUDO_REGISTER)
    {
      if (!strict)
        return true;
      regno = reg_renumber[regno];
    }

  /* These fake registers will be eliminated to either the stack or
     hard frame pointer, both of which are usually valid base registers.
     Reload deals with the cases where the eliminated form isn't valid.  */
  if (regno == ARG_POINTER_REGNUM || regno == FRAME_POINTER_REGNUM)
    return true;

  /* In mips16 mode, the stack pointer can only address word and doubleword
     values, nothing smaller.  There are two problems here:

       (a) Instantiating virtual registers can introduce new uses of the
           stack pointer.  If these virtual registers are valid addresses,
           the stack pointer should be too.

       (b) Most uses of the stack pointer are not made explicit until
           FRAME_POINTER_REGNUM and ARG_POINTER_REGNUM have been eliminated.
           We don't know until that stage whether we'll be eliminating to the
           stack pointer (which needs the restriction) or the hard frame
           pointer (which doesn't).

     All in all, it seems more consitent to only enforce this restriction
     during and after reload.  */
  if (TARGET_MIPS16 && regno == STACK_POINTER_REGNUM)
    return !strict || GET_MODE_SIZE (mode) == 4 || GET_MODE_SIZE (mode) == 8;

  return TARGET_MIPS16 ? M16_REG_P (regno) : GP_REG_P (regno);
}


/* Return true if X is a valid base register for the given mode.
   Allow only hard registers if STRICT.  */

static bool
mips_valid_base_register_p (rtx x, enum machine_mode mode, int strict)
{
  if (!strict && GET_CODE (x) == SUBREG)
    x = SUBREG_REG (x);

  return (GET_CODE (x) == REG
          && mips_regno_mode_ok_for_base_p (REGNO (x), mode, strict));
}


/* Return true if symbols of type SYMBOL_TYPE can directly address a value
   with mode MODE.  This is used for both symbolic and LO_SUM addresses.  */

static bool
mips_symbolic_address_p (enum mips_symbol_type symbol_type,
                         enum machine_mode mode)
{
  switch (symbol_type)
    {
    case SYMBOL_GENERAL:
      /* For this case, we will split address to HIGH and LO_SUM. */
      if (TARGET_MIPS16 && TARGET_NO_DATA_IN_CODE && !TARGET_GP_OPT)
        return true;
    case SYMBOL_LONGCALL:
      return !TARGET_MIPS16;

    case SYMBOL_SMALL_DATA:
      return true;

    case SYMBOL_CONSTANT_POOL:
    case SYMBOL_SECTION_RELATIVE:
      /* PC-relative addressing is only available for lw, sw, ld and sd.  */
      return GET_MODE_SIZE (mode) == 4 || GET_MODE_SIZE (mode) == 8;

    case SYMBOL_GOT_LOCAL:
      return true;

    case SYMBOL_GOT_GLOBAL:
      /* The address will have to be loaded from the GOT first.  */
      return false;

    case SYMBOL_GOTOFF_PAGE:
    case SYMBOL_GOTOFF_GLOBAL:
    case SYMBOL_GOTOFF_CALL:
    case SYMBOL_GOTOFF_LOADGP:
      return true;
    }
  abort ();
}


/* Return true if X is a valid address for machine mode MODE.  If it is,
   fill in INFO appropriately.  STRICT is true if we should only accept
   hard base registers.  */

static bool
mips_classify_address (struct mips_address_info *info, rtx x,
                       enum machine_mode mode, int strict)
{
  switch (GET_CODE (x))
    {
    case REG:
    case SUBREG:
      info->type = ADDRESS_REG;
      info->reg = x;
      info->offset = const0_rtx;
      return mips_valid_base_register_p (info->reg, mode, strict);

    case PLUS:
      info->type = ADDRESS_REG;
      info->reg = XEXP (x, 0);
      info->offset = XEXP (x, 1);
      return (mips_valid_base_register_p (info->reg, mode, strict)
              && const_arith_operand (info->offset, VOIDmode));

    case LO_SUM:
      info->type = ADDRESS_LO_SUM;
      info->reg = XEXP (x, 0);
      info->offset = XEXP (x, 1);
      return (mips_valid_base_register_p (info->reg, mode, strict)
              && mips_symbolic_constant_p (info->offset, &info->symbol_type)
              && mips_symbolic_address_p (info->symbol_type, mode)
              && mips_lo_relocs[info->symbol_type] != 0);

    case CONST_INT:
      /* Small-integer addresses don't occur very often, but they
         are legitimate if $0 is a valid base register.  */
      info->type = ADDRESS_CONST_INT;
      return !TARGET_MIPS16 && SMALL_INT (x);

    case CONST:
      if (flag_section_relative_addressing && TARGET_MIPS16)
        {
          /* We want to split (const (plus symbol offset)) such that it can
             be optimised using section-relative address instead of leaving it
             in the constant pool. */
          rtx y = XEXP (x, 0);
          if (GET_CODE (y) == PLUS && GET_CODE (XEXP (y, 0)) == SYMBOL_REF
              && GET_CODE (XEXP (y, 1)) == CONST_INT
              && mips_classify_symbol (XEXP (y, 0)) == SYMBOL_SECTION_RELATIVE)
            return false;
        }

    case LABEL_REF:
    case SYMBOL_REF:
      info->type = ADDRESS_SYMBOLIC;
      return (mips_symbolic_constant_p (x, &info->symbol_type)
              && mips_symbolic_address_p (info->symbol_type, mode)
              && !mips_split_p[info->symbol_type]);

    default:
      return false;
    }
}

/* Return the number of instructions needed to load a symbol of the
   given type into a register.  If valid in an address, the same number
   of instructions are needed for loads and stores.  Treat extended
   mips16 instructions as two instructions.  */

static int
mips_symbol_insns (enum mips_symbol_type type)
{
  switch (type)
    {
    case SYMBOL_GENERAL:
      /* For this case, we will split address to HIGH and LO_SUM. */
      if (TARGET_MIPS16 && TARGET_NO_DATA_IN_CODE && !TARGET_GP_OPT)
        return 2;
    case SYMBOL_LONGCALL:
      /* In mips16 code, general symbols must be fetched from the
         constant pool.  */
      if (TARGET_MIPS16)
        return 0;

      /* When using 64-bit symbols, we need 5 preparatory instructions,
         such as:

             lui     $at,%highest(symbol)
             daddiu  $at,$at,%higher(symbol)
             dsll    $at,$at,16
             daddiu  $at,$at,%hi(symbol)
             dsll    $at,$at,16

         The final address is then $at + %lo(symbol).  With 32-bit
         symbols we just need a preparatory lui.  */
      return (ABI_HAS_64BIT_SYMBOLS ? 6 : 2);

    case SYMBOL_SMALL_DATA:
      return 1;

    case SYMBOL_CONSTANT_POOL:
      /* This case is for mips16 only.  We may need an extended instruction
         if it is out of range, but that should be avoided by laying out
         the internal constant table. */
#if 0
      /* Until we fix layout constant table to remove any non-extended load,
         we cannot guarantee that an extended version will not occur.
         We do not want delay branch filling to move an extended insn into
         the delay slot. */

      if (SYMBOL_REF_FLAG (x))
        return 2;
      else
        return 1;
#endif

    case SYMBOL_SECTION_RELATIVE:
      /* MIPS16, Assume extended instruction is needed. */
      return 2;

    case SYMBOL_GOT_LOCAL:
    case SYMBOL_GOT_GLOBAL:
      /* Unless -funit-at-a-time is in effect, we can't be sure whether
         the local/global classification is accurate.  See override_options
         for details.

         The worst cases are:

         (1) For local symbols when generating o32 or o64 code.  The assembler
             will use:

                 lw           $at,%got(symbol)
                 nop

             ...and the final address will be $at + %lo(symbol).

         (2) For global symbols when -mxgot.  The assembler will use:

                 lui     $at,%got_hi(symbol)
                 (d)addu $at,$at,$gp

             ...and the final address will be $at + %got_lo(symbol).  */
      return 3;

    case SYMBOL_GOTOFF_PAGE:
    case SYMBOL_GOTOFF_GLOBAL:
    case SYMBOL_GOTOFF_CALL:
    case SYMBOL_GOTOFF_LOADGP:
      /* Check whether the offset is a 16- or 32-bit value.  */
      return mips_split_p[type] ? 2 : 1;
    }
  abort ();
}


/* Return true if a value at OFFSET bytes from BASE can be accessed
   using an unextended mips16 instruction.  MODE is the mode of the
   value.

   Usually the offset in an unextended instruction is a 5-bit field.
   The offset is unsigned and shifted left once for HIs, twice
   for SIs, and so on.  An exception is SImode accesses off the
   stack pointer, which have an 8-bit immediate field.  */

static bool
mips16_unextended_reference_p (enum machine_mode mode, rtx base, rtx offset)
{
  if (TARGET_MIPS16
      && GET_CODE (offset) == CONST_INT
      && INTVAL (offset) >= 0
      && (INTVAL (offset) & (GET_MODE_SIZE (mode) - 1)) == 0)
    {
      if (GET_MODE_SIZE (mode) == 4 && base == stack_pointer_rtx)
        return INTVAL (offset) < 256 * GET_MODE_SIZE (mode);
      return INTVAL (offset) < 32 * GET_MODE_SIZE (mode);
    }
  return false;
}


/* Return the number of instructions needed to load or store a value
   of mode MODE at X.  Return 0 if X isn't valid for MODE.

   For mips16 code, count extended instructions as two instructions.  */

int
mips_address_insns (rtx x, enum machine_mode mode)
{
  struct mips_address_info addr;
  int factor;

  if (mode == BLKmode)
    /* BLKmode is used for single unaligned loads and stores.  */
    factor = 1;
  else
    /* Each word of a multi-word value will be accessed individually.  */
    factor = (GET_MODE_SIZE (mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD;

  if (mips_classify_address (&addr, x, mode, false))
    switch (addr.type)
      {
      case ADDRESS_REG:
        if (TARGET_MIPS16
            && !mips16_unextended_reference_p (mode, addr.reg, addr.offset))
          return factor * 2;
        return factor;

      case ADDRESS_LO_SUM:
        return (TARGET_MIPS16 ? factor * 2 : factor);

      case ADDRESS_CONST_INT:
        return factor;

      case ADDRESS_SYMBOLIC:
        return factor * mips_symbol_insns (addr.symbol_type);
      }
  return 0;
}


/* Likewise for constant X.  */

int
mips_const_insns (rtx x)
{
  struct mips_integer_op codes[MIPS_MAX_INTEGER_OPS];
  enum mips_symbol_type symbol_type;
  HOST_WIDE_INT offset;

  switch (GET_CODE (x))
    {
    case CONSTANT_P_RTX:
      return 1;

    case HIGH:
      if (TARGET_MIPS16
          || !mips_symbolic_constant_p (XEXP (x, 0), &symbol_type)
          || !mips_split_p[symbol_type])
        return 0;

      return 1;

    case CONST_INT:
      if (TARGET_MIPS16)
        /* Unsigned 8-bit constants can be loaded using an unextended
           LI instruction.  Unsigned 16-bit constants can be loaded
           using an extended LI.  Negative constants must be loaded
           using LI and then negated.  */
        return (INTVAL (x) >= 0 && INTVAL (x) < 256 ? 1
                : SMALL_OPERAND_UNSIGNED (INTVAL (x)) ? 2
                : INTVAL (x) > -256 && INTVAL (x) < 0 ? 2
                : SMALL_OPERAND_UNSIGNED (-INTVAL (x)) ? 3
                /* For this case, we will build a 32-bit integer.  */
                : TARGET_NO_DATA_IN_CODE && !TARGET_GP_OPT &&
                  (BIG_OPERAND_UNSIGNED (INTVAL (x)) ||
                   BIG_OPERAND (INTVAL (x))) ? 10
                /* XXX This is a kludge when reloading an out-of-range
                   address offset -- forcing the offset into the const
                   pool causes another reload to be required, which
                   screws up. NEED TO FIX RELOAD. */
                : TARGET_NO_DATA_IN_CODE && reload_in_progress ? 10
                : 0);

      return mips_build_integer (codes, INTVAL (x));

    case CONST_VECTOR:
    case CONST_DOUBLE:
      return (!TARGET_MIPS16 && x == CONST0_RTX (GET_MODE (x)) ? 1 : 0);

    case CONST:
      if (CONST_GP_P (x))
        return 1;

      /* See if we can refer to X directly.  */
      if (mips_symbolic_constant_p (x, &symbol_type))
        return mips_symbol_insns (symbol_type);

      /* Otherwise try splitting the constant into a base and offset.
         16-bit offsets can be added using an extra addiu.  Larger offsets
         must be calculated separately and then added to the base.  */
      mips_split_const (x, &x, &offset);
      if (offset != 0)
        {
          int n = mips_const_insns (x);
          if (n != 0)
            {
              if (SMALL_OPERAND (offset))
                return n + 1;
              else
                return n + 1 + mips_build_integer (codes, offset);
            }
        }
      return 0;

    case SYMBOL_REF:
    case LABEL_REF:
      return mips_symbol_insns (mips_classify_symbol (x));

    default:
      return 0;
    }
}


/* Return the number of instructions needed for memory reference X.
   Count extended mips16 instructions as two instructions.  */

int
mips_fetch_insns (rtx x)
{
  if (GET_CODE (x) != MEM)
    abort ();

  return mips_address_insns (XEXP (x, 0), GET_MODE (x));
}


/* Return truth value of whether OP can be used as an operands
   where a register or 16 bit unsigned integer is needed.  */

int
uns_arith_operand (rtx op, enum machine_mode mode)
{
  if (GET_CODE (op) == CONST_INT && SMALL_INT_UNSIGNED (op))
    return 1;

  return register_operand (op, mode);
}


/* True if OP can be treated as a signed 16-bit constant.  */

int
const_arith_operand (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return GET_CODE (op) == CONST_INT && SMALL_INT (op);
}


/* Return true if OP is a register operand or a signed 16-bit constant.  */

int
arith_operand (rtx op, enum machine_mode mode)
{
  return const_arith_operand (op, mode) || register_operand (op, mode);
}

/* Return true if OP is a register operand or a unsigned 10-bit constant.  */
int
reg_simm10_operand (rtx op, enum machine_mode mode)
{
  return (register_operand (op, mode)
          || (GET_CODE (op) == CONST_INT
              && (unsigned HOST_WIDE_INT) (INTVAL (op)) + 0x200 < 0x400));
}

/* Return truth value of whether OP is an integer which fits in 16 bits.  */

int
small_int (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return (GET_CODE (op) == CONST_INT && SMALL_INT (op));
}

/* Return truth value of whether OP is a register or the constant 0.
   Do not accept 0 in mips16 mode since $0 is not one of the core 8
   registers.  */

int
reg_or_0_operand (rtx op, enum machine_mode mode)
{
  switch (GET_CODE (op))
    {
    case CONST_INT:
      if (TARGET_MIPS16)
        return 0;
      return INTVAL (op) == 0;

    case CONST_DOUBLE:
    case CONST_VECTOR:
      if (TARGET_MIPS16)
        return 0;
      return op == CONST0_RTX (mode);

    default:
      return register_operand (op, mode);
    }
}

/* Accept a register or the floating point constant 1 in the appropriate mode.  */

int
reg_or_const_float_1_operand (rtx op, enum machine_mode mode)
{
  REAL_VALUE_TYPE d;

  switch (GET_CODE (op))
    {
    case CONST_DOUBLE:
      if (mode != GET_MODE (op)
          || (mode != DFmode && mode != SFmode))
        return 0;

      REAL_VALUE_FROM_CONST_DOUBLE (d, op);
      return REAL_VALUES_EQUAL (d, dconst1);

    default:
      return register_operand (op, mode);
    }
}

/* Accept the floating point constant 1 in the appropriate mode.  */

int
const_float_1_operand (rtx op, enum machine_mode mode)
{
  REAL_VALUE_TYPE d;

  if (GET_CODE (op) != CONST_DOUBLE
      || mode != GET_MODE (op)
      || (mode != DFmode && mode != SFmode))
    return 0;

  REAL_VALUE_FROM_CONST_DOUBLE (d, op);

  return REAL_VALUES_EQUAL (d, dconst1);
}

/* Return true if OP is either the HI or LO register.  */

int
hilo_operand (rtx op, enum machine_mode mode)
{
  return ((mode == VOIDmode || mode == GET_MODE (op))
          && REG_P (op) && MD_REG_P (REGNO (op)));
}

/* Return true if OP is an extension operator.  */

int
extend_operator (rtx op, enum machine_mode mode)
{
  return ((mode == VOIDmode || mode == GET_MODE (op))
          && (GET_CODE (op) == ZERO_EXTEND || GET_CODE (op) == SIGN_EXTEND));
}

/* Return nonzero if the code of this rtx pattern is EQ or NE.  */

int
equality_op (rtx op, enum machine_mode mode)
{
  if (mode != GET_MODE (op))
    return 0;

  return GET_CODE (op) == EQ || GET_CODE (op) == NE;
}

/* Return nonzero if the code is a relational operations (EQ, LE, etc.) */

int
cmp_op (rtx op, enum machine_mode mode)
{
  if (mode != GET_MODE (op))
    return 0;

  return GET_RTX_CLASS (GET_CODE (op)) == '<';
}

/* Return nonzero if the code is a relational operation suitable for a
   conditional trap instruction (only EQ, NE, LT, LTU, GE, GEU).
   We need this in the insn that expands `trap_if' in order to prevent
   combine from erroneously altering the condition.  */

int
trap_cmp_op (rtx op, enum machine_mode mode)
{
  if (mode != GET_MODE (op))
    return 0;

  switch (GET_CODE (op))
    {
    case EQ:
    case NE:
    case LT:
    case LTU:
    case GE:
    case GEU:
      return 1;

    default:
      return 0;
    }
}

/* Return nonzero if the operand is either the PC or a label_ref.  */

int
pc_or_label_operand (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  if (op == pc_rtx)
    return 1;

  if (GET_CODE (op) == LABEL_REF)
    return 1;

  return 0;
}

/* Test for a valid call address.  */

int
call_insn_operand (rtx op, enum machine_mode mode)
{
  enum mips_symbol_type symbol_type;

  if (mips_symbolic_constant_p (op, &symbol_type))
    switch (symbol_type)
      {
      case SYMBOL_GENERAL:
        return true;

      case SYMBOL_GOT_GLOBAL:
        /* Without explicit relocs, there is no special syntax for
           loading the address of a call destination into a register.
           Using "la $25,foo; jal $25" would prevent the lazy binding
           of "foo", so keep the address of global symbols with the
           jal macro.  */
        return !TARGET_EXPLICIT_RELOCS;

      default:
        return false;
      }
  return register_operand (op, mode);
}


/* Return nonzero if OP is valid as a source operand for a move
   instruction.  */

int
move_operand (rtx op, enum machine_mode mode)
{
  enum mips_symbol_type symbol_type;

  if (!general_operand (op, mode))
    return false;

  switch (GET_CODE (op))
    {
    case CONST_INT:
      /* When generating mips16 code, LEGITIMATE_CONSTANT_P rejects
         CONST_INTs that can't be loaded using simple insns.  */
      if (TARGET_MIPS16)
        {
          /* For this case, we need to synthesize the integer. */
          if (TARGET_NO_DATA_IN_CODE
              && (!TARGET_GP_OPT || reload_in_progress)
              && !(SMALL_INT (op) || SMALL_INT_UNSIGNED (op)))
            return false;
          else
            return true;
        }

      /* Otherwise check whether the constant can be loaded in a single
         instruction.  */
      return LUI_INT (op) || SMALL_INT (op) || SMALL_INT_UNSIGNED (op);

    case LABEL_REF:
      if (TARGET_MIPS16 && (TARGET_NO_DATA_IN_CODE || TARGET_CODE_XONLY)
	  && LABEL_REF_JUMP_TABLE_P (op))
	return false;
      /* fall through..  */
    case SYMBOL_REF:
      if (TARGET_MIPS16 && TARGET_NO_DATA_IN_CODE 
	  && CONSTANT_POOL_ADDRESS_P (op))
	return false;
      /* fall through..  */
    case CONST:
      if (CONST_GP_P (op))
        return true;

      return (mips_symbolic_constant_p (op, &symbol_type)
              && !mips_split_p[symbol_type]);

    default:
      return true;
    }
}


/* Accept any operand that can appear in a mips16 constant table
   instruction.  We can't use any of the standard operand functions
   because for these instructions we accept values that are not
   accepted by LEGITIMATE_CONSTANT, such as arbitrary SYMBOL_REFs.  */

int
consttable_operand (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return CONSTANT_P (op);
}

/* Return 1 if OP is a symbolic operand, i.e. a symbol_ref or a label_ref,
   possibly with an offset.  */

int
symbolic_operand (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  enum mips_symbol_type symbol_type;

  return mips_symbolic_constant_p (op, &symbol_type);
}


/* Return true if we're generating PIC and OP is a global symbol.  */

int
global_got_operand (rtx op, enum machine_mode mode)
{
  enum mips_symbol_type symbol_type;

  return ((mode == VOIDmode || mode == GET_MODE (op))
          && mips_symbolic_constant_p (op, &symbol_type)
          && symbol_type == SYMBOL_GOT_GLOBAL);
}


/* Likewise for local symbols.  */

int
local_got_operand (rtx op, enum machine_mode mode)
{
  enum mips_symbol_type symbol_type;

  return ((mode == VOIDmode || mode == GET_MODE (op))
          && mips_symbolic_constant_p (op, &symbol_type)
          && symbol_type == SYMBOL_GOT_LOCAL);
}


/* Return true if OP is a memory reference that uses the stack pointer
   as a base register.  */

int
stack_operand (rtx op, enum machine_mode mode)
{
  struct mips_address_info addr;

  return ((mode == VOIDmode || mode == GET_MODE (op))
          && GET_CODE (op) == MEM
          && mips_classify_address (&addr, XEXP (op, 0), GET_MODE (op), false)
          && addr.type == ADDRESS_REG
          && addr.reg == stack_pointer_rtx);
}


/* This function is used to implement GO_IF_LEGITIMATE_ADDRESS.  It
   returns a nonzero value if X is a legitimate address for a memory
   operand of the indicated MODE.  STRICT is nonzero if this function
   is called during reload.  */

bool
mips_legitimate_address_p (enum machine_mode mode, rtx x, int strict)
{
  struct mips_address_info addr;

  return mips_classify_address (&addr, x, mode, strict);
}


/* Copy VALUE to a register and return that register.  If new psuedos
   are allowed, copy it into a new register, otherwise use DEST.  */

static rtx
mips_force_temporary (rtx dest, rtx value)
{
  if (!no_new_pseudos)
    return force_reg (Pmode, value);
  else
    {
      emit_move_insn (copy_rtx (dest), value);
      return dest;
    }
}


/* Return a LO_SUM expression for ADDR.  TEMP is as for mips_force_temporary
   and is used to load the high part into a register.  */

static rtx
mips_split_symbol (rtx temp, rtx addr, enum mips_symbol_type symbol_type)
{
  rtx high;

  if (TARGET_MIPS16)
    {
      /* For this case, we will split address to HIGH and LO_SUM. */
      if (TARGET_NO_DATA_IN_CODE && ! TARGET_GP_OPT
          && symbol_type != SYMBOL_SECTION_RELATIVE)
        {
          if (temp == 0 ||
              (! no_new_pseudos && ! reload_in_progress && ! reload_completed))
            temp = gen_reg_rtx (Pmode);

          emit_insn (gen_rtx_SET (VOIDmode, temp,
                                  gen_rtx_HIGH (Pmode, copy_rtx (addr))));
          return gen_rtx_LO_SUM (Pmode, temp, addr);
        }
      else if (TARGET_NO_DATA_IN_CODE && TARGET_GP_OPT)
        {
          rtx unspec;
          if (temp == 0 ||
              (! no_new_pseudos && ! reload_in_progress && ! reload_completed))
             temp = gen_reg_rtx (Pmode);

          /* We avoid using mips16_gp_pseudo_reg (), because it may not be
             ready yet.  (The fix for debug info moves the initialization
             of mips16 gp rtx after NOTE_INSN_BLOCK_BEGIN.)  */
          unspec = gen_rtx_UNSPEC (VOIDmode, gen_rtvec (1, const0_rtx),
                                   UNSPEC_GP);
          emit_move_insn (temp, gen_rtx_CONST (Pmode, unspec));

          return gen_rtx_LO_SUM (Pmode, temp, addr);
        }
      else if (flag_section_relative_addressing)
        {
          /* The caller is responsible to check if we have a section relative
             address symbol before deciding to split. */
          tree decl;
          rtx sym, sectrel_address;
          /* addr can possibly be of form const (plus symbol_ref const_int),
             in which case, we need to extract the correct tree from the
             right symbol. */
          if (!symbolic_expression_p (addr, &sym))
            abort ();
          decl = SYMBOL_REF_DECL (sym);

          sectrel_address =
            mips_legitimize_section_relative_address (temp, addr, decl);
          if (sectrel_address)
            return sectrel_address;
          else
            abort();
        }
      else
        high = mips16_gp_pseudo_reg ();
    }
  else
    high = mips_force_temporary (temp, gen_rtx_HIGH (Pmode, copy_rtx (addr)));
  return gen_rtx_LO_SUM (Pmode, high, addr);
}


/* Return an UNSPEC address with underlying address ADDRESS and symbol
   type SYMBOL_TYPE.  */

static rtx
mips_unspec_address (rtx address, enum mips_symbol_type symbol_type)
{
  rtx base;
  HOST_WIDE_INT offset;

  mips_split_const (address, &base, &offset);
  base = gen_rtx_UNSPEC (Pmode, gen_rtvec (1, base),
                         UNSPEC_ADDRESS_FIRST + symbol_type);
  return plus_constant (gen_rtx_CONST (Pmode, base), offset);
}


/* If mips_unspec_address (ADDR, SYMBOL_TYPE) is a 32-bit value, add the
   high part to BASE and return the result.  Just return BASE otherwise.
   TEMP is available as a temporary register if needed.

   The returned expression can be used as the first operand to a LO_SUM.  */

static rtx
mips_unspec_offset_high (rtx temp, rtx base, rtx addr,
                         enum mips_symbol_type symbol_type)
{
  if (mips_split_p[symbol_type])
    {
      addr = gen_rtx_HIGH (Pmode, mips_unspec_address (addr, symbol_type));
      addr = mips_force_temporary (temp, addr);
      return mips_force_temporary (temp, gen_rtx_PLUS (Pmode, addr, base));
    }
  return base;
}


/* Return a memory reference for the GOT slot whose offset is given by
   mips_unspec_address (ADDR, SYMBOL_TYPE).  Register BASE contains the
   high part of the offset plus $gp.  */

static rtx
mips_load_got (rtx base, rtx addr, enum mips_symbol_type symbol_type)
{
  rtx mem, offset;

  offset = mips_unspec_address (addr, symbol_type);
  mem = gen_rtx_MEM (ptr_mode, gen_rtx_LO_SUM (Pmode, base, offset));
  set_mem_alias_set (mem, mips_got_alias_set);

  /* GOT entries are constant and references to them can't trap.  */
  RTX_UNCHANGING_P (mem) = 1;
  MEM_NOTRAP_P (mem) = 1;

  return mem;
}


/* Return the offset of ADDR's GOT entry from _gp.  ADDR is a
   global_got_operand.  */

rtx
mips_gotoff_global (rtx addr)
{
  return mips_unspec_address (addr, SYMBOL_GOTOFF_GLOBAL);
}


/* Fetch the high part of local_got_operand ADDR from the GOT.  */

rtx
mips_load_got_page (rtx addr)
{
  return mips_load_got (pic_offset_table_rtx, addr, SYMBOL_GOTOFF_PAGE);
}


/* Fetch the address of global_got_operand ADDR from the GOT.  BASE is a
   register that holds the address _gp + %got_hi(ADDR).  */

rtx
mips_load_got_global (rtx base, rtx addr)
{
  return mips_load_got (base, addr, SYMBOL_GOTOFF_GLOBAL);
}


/* Return a legitimate address for REG + OFFSET.  TEMP is as for
   mips_force_temporary; it is only needed when OFFSET is not a
   SMALL_OPERAND.  */

static rtx
mips_add_offset (rtx temp, rtx reg, HOST_WIDE_INT offset)
{
  if (!SMALL_OPERAND (offset))
    {
      rtx high;
      if (TARGET_MIPS16)
        {
          /* Load the full offset into a register so that we can use
             an unextended instruction for the address itself.  */
          high = GEN_INT (offset);
          offset = 0;
        }
      else
        {
          /* Leave OFFSET as a 16-bit offset and put the excess in HIGH.  */
          high = GEN_INT (CONST_HIGH_PART (offset));
          offset = CONST_LOW_PART (offset);
        }
      high = mips_force_temporary (temp, high);
      reg = mips_force_temporary (temp, gen_rtx_PLUS (Pmode, high, reg));
    }
  return plus_constant (reg, offset);
}

#include "cgraph.h"

/* This function is used to implement LEGITIMIZE_ADDRESS.  If *XLOC can
   be legitimized in a way that the generic machinery might not expect,
   put the new address in *XLOC and return true.  MODE is the mode of
   the memory being accessed.  */

bool
mips_legitimize_address (rtx *xloc, enum machine_mode mode)
{
  enum mips_symbol_type symbol_type;

  /* See if the address can split into a high part and a LO_SUM.  */
  if (mips_symbolic_constant_p (*xloc, &symbol_type)
      && mips_symbolic_address_p (symbol_type, mode)
      && mips_split_p[symbol_type])
    {
      *xloc = mips_split_symbol (0, *xloc, symbol_type);
      return true;
    }

  if (GET_CODE (*xloc) == PLUS && GET_CODE (XEXP (*xloc, 1)) == CONST_INT)
    {
      /* Handle REG + CONSTANT using mips_add_offset.  */
      rtx reg;

      reg = XEXP (*xloc, 0);
      if (!mips_valid_base_register_p (reg, mode, 0))
        reg = copy_to_mode_reg (Pmode, reg);
      *xloc = mips_add_offset (0, reg, INTVAL (XEXP (*xloc, 1)));
      return true;
    }

  /* Split (const (plus symbol offset)) for section-relative addressing. */
  if (TARGET_MIPS16 && flag_section_relative_addressing && GET_CODE (*xloc) == CONST)
    {
      rtx src = XEXP (*xloc, 0);

      if (GET_CODE (src) == PLUS && GET_CODE (XEXP (src, 0)) == SYMBOL_REF
          && GET_CODE (XEXP (src, 1)) == CONST_INT
          && mips_classify_symbol (XEXP (src, 0)) == SYMBOL_SECTION_RELATIVE)
        {
          rtx base, sectrel_addr;
          HOST_WIDE_INT offset;

          mips_split_const (src, &base, &offset);
          sectrel_addr =
            mips_legitimize_section_relative_address (0, *xloc,
						      SYMBOL_REF_DECL (base));
          if (sectrel_addr)
            {
              *xloc = mips_add_offset (0, sectrel_addr, offset);
              return true;
            }
        }
    }
  return false;
}


/* Subroutine of mips_build_integer (with the same interface).
   Assume that the final action in the sequence should be a left shift.  */

static unsigned int
mips_build_shift (struct mips_integer_op *codes, HOST_WIDE_INT value)
{
  unsigned int i, shift;

  /* Shift VALUE right until its lowest bit is set.  Shift arithmetically
     since signed numbers are easier to load than unsigned ones.  */
  shift = 0;
  while ((value & 1) == 0)
    value /= 2, shift++;

  i = mips_build_integer (codes, value);
  codes[i].code = ASHIFT;
  codes[i].value = shift;
  return i + 1;
}


/* As for mips_build_shift, but assume that the final action will be
   an IOR or PLUS operation.  */

static unsigned int
mips_build_lower (struct mips_integer_op *codes, unsigned HOST_WIDE_INT value)
{
  unsigned HOST_WIDE_INT high;
  unsigned int i;

  high = value & ~(unsigned HOST_WIDE_INT) 0xffff;
  if (!LUI_OPERAND (high) && (value & 0x18000) == 0x18000)
    {
      /* The constant is too complex to load with a simple lui/ori pair
         so our goal is to clear as many trailing zeros as possible.
         In this case, we know bit 16 is set and that the low 16 bits
         form a negative number.  If we subtract that number from VALUE,
         we will clear at least the lowest 17 bits, maybe more.  */
      i = mips_build_integer (codes, CONST_HIGH_PART (value));
      codes[i].code = PLUS;
      codes[i].value = CONST_LOW_PART (value);
    }
  else
    {
      i = mips_build_integer (codes, high);
      codes[i].code = IOR;
      codes[i].value = value & 0xffff;
    }
  return i + 1;
}


/* Fill CODES with a sequence of rtl operations to load VALUE.
   Return the number of operations needed.  */

static unsigned int
mips_build_integer (struct mips_integer_op *codes,
                    unsigned HOST_WIDE_INT value)
{
  if (SMALL_OPERAND (value)
      || SMALL_OPERAND_UNSIGNED (value)
      || LUI_OPERAND (value))
    {
      /* The value can be loaded with a single instruction.  */
      codes[0].code = NIL;
      codes[0].value = value;
      return 1;
    }
  else if ((value & 1) != 0 || LUI_OPERAND (CONST_HIGH_PART (value)))
    {
      /* Either the constant is a simple LUI/ORI combination or its
         lowest bit is set.  We don't want to shift in this case.  */
      return mips_build_lower (codes, value);
    }
  else if ((value & 0xffff) == 0)
    {
      /* The constant will need at least three actions.  The lowest
         16 bits are clear, so the final action will be a shift.  */
      return mips_build_shift (codes, value);
    }
  else
    {
      /* The final action could be a shift, add or inclusive OR.
         Rather than use a complex condition to select the best
         approach, try both mips_build_shift and mips_build_lower
         and pick the one that gives the shortest sequence.
         Note that this case is only used once per constant.  */
      struct mips_integer_op alt_codes[MIPS_MAX_INTEGER_OPS];
      unsigned int cost, alt_cost;

      cost = mips_build_shift (codes, value);
      alt_cost = mips_build_lower (alt_codes, value);
      if (alt_cost < cost)
        {
          memcpy (codes, alt_codes, alt_cost * sizeof (codes[0]));
          cost = alt_cost;
        }
      return cost;
    }
}


/* Move VALUE into register DEST.  */

static void
mips_move_integer (rtx dest, unsigned HOST_WIDE_INT value)
{
  struct mips_integer_op codes[MIPS_MAX_INTEGER_OPS];
  enum machine_mode mode;
  unsigned int i, cost;
  rtx x;

  mode = GET_MODE (dest);
  cost = mips_build_integer (codes, value);

  /* Apply each binary operation to X.  Invariant: X is a legitimate
     source operand for a SET pattern.  */
  x = GEN_INT (codes[0].value);
  for (i = 1; i < cost; i++)
    {
      if (no_new_pseudos)
        emit_move_insn (dest, x), x = dest;
      else
        x = force_reg (mode, x);
      x = gen_rtx_fmt_ee (codes[i].code, mode, x, GEN_INT (codes[i].value));
    }

  emit_insn (gen_rtx_SET (VOIDmode, dest, x));
}


/* Subroutine of mips_legitimize_move.  Move constant SRC into register
   DEST given that SRC satisfies immediate_operand but doesn't satisfy
   move_operand.  */

static void
mips_legitimize_const_move (enum machine_mode mode, rtx dest, rtx src)
{
  rtx base;
  HOST_WIDE_INT offset;
  enum mips_symbol_type symbol_type;

  /* For this case, we generate multi instructions for 32-bit integer.  */
  if (TARGET_MIPS16 && TARGET_NO_DATA_IN_CODE
      && (GET_MODE (dest) == SImode || GET_MODE (dest) == DImode)
      && GET_CODE (src) == CONST_INT
      && (!TARGET_GP_OPT || reload_in_progress)
      && !(SMALL_INT(src) || SMALL_INT_UNSIGNED (src)))
    {
      HOST_WIDE_INT ival = INTVAL (src);
      HOST_WIDE_INT lo16 = (short)(ival & 0xffff);
      HOST_WIDE_INT hi16 = (ival >> 16) + (lo16 < 0);
      rtx hireg;
      rtx temp;

      if (GET_MODE (dest) == SImode)
        hireg = no_new_pseudos ? dest : gen_reg_rtx (SImode);
      else
        hireg = no_new_pseudos ? gen_lowpart (SImode, dest)
                               : gen_reg_rtx (SImode);

      if (no_new_pseudos)
        emit_move_insn (temp = hireg, GEN_INT (hi16));
      else
        temp = force_reg (SImode, GEN_INT (hi16));
      emit_insn (gen_ashlsi3 (hireg, temp, GEN_INT (16)));

      if (GET_MODE (dest) == SImode)
        {
          if (lo16 != 0 && register_operand (dest, SImode))
            emit_insn (gen_addsi3 (dest, hireg, GEN_INT (lo16)));
          else if (lo16 != 0)
            {
              emit_insn (gen_addsi3 (hireg, hireg, GEN_INT (lo16)));
              emit_move_insn (dest, hireg);
            }
          else if (hireg != dest)
            emit_move_insn (dest, hireg);
          return;
        }
      else
        {
          if (lo16 != 0)
            {
              if (! no_new_pseudos)
                temp = gen_reg_rtx (SImode);
              emit_insn (gen_addsi3 (temp, hireg, GEN_INT (lo16)));
            }
          else
            temp = hireg;

          if (! TARGET_64BIT)
            {
              if (! no_new_pseudos)
                emit_move_insn (gen_lowpart (SImode, dest), temp);
              emit_move_insn (gen_highpart (SImode, dest),
                              GEN_INT (ival < 0 ? -1 : 0));
              /* We add this insn, because REG_EQUAL will be attached to it.  */
              emit_move_insn (dest, dest);
            }
          else if (! no_new_pseudos)
            emit_insn (gen_extendsidi2 (dest, temp));
          return;
        }
    }

  /* Split moves of big integers into smaller pieces.  In mips16 code,
     it's better to force the constant into memory instead.  */
  if (GET_CODE (src) == CONST_INT && !TARGET_MIPS16)
    {
      mips_move_integer (dest, INTVAL (src));
      return;
    }

  /* See if the symbol can be split.  For mips16, this is often worse than
     forcing it in the constant pool since it needs the single-register form
     of addiu or daddiu.  */
  if ((!TARGET_MIPS16 || TARGET_NO_DATA_IN_CODE)
      && mips_symbolic_constant_p (src, &symbol_type)
      && mips_split_p[symbol_type])
    {
      emit_move_insn (dest, mips_split_symbol (dest, src, symbol_type));
      return;
    }

  /* If we have (const (plus symbol offset)), load the symbol first
     and then add in the offset.  This is usually better than forcing
     the constant into memory, at least in non-mips16 code.  */
  mips_split_const (src, &base, &offset);
  if ((!TARGET_MIPS16 || flag_section_relative_addressing ||
       TARGET_NO_DATA_IN_CODE)
      && offset != 0
      && (!no_new_pseudos || SMALL_OPERAND (offset)))
    {
      base = mips_force_temporary (dest, base);
      emit_move_insn (dest, mips_add_offset (0, base, offset));
      return;
    }

  /* Now split any symbol_refs if flag_section_relative_addressing. */
  if (TARGET_MIPS16 && flag_section_relative_addressing
      && mips_symbolic_constant_p (src, &symbol_type)
      && mips_split_p[symbol_type])
    {
      emit_move_insn (dest, mips_split_symbol (dest, src, symbol_type));
      return;
    }

  src = force_const_mem (mode, src);

  /* When using explicit relocs, constant pool references are sometimes
     not legitimate addresses.  */
  if (!memory_operand (src, VOIDmode))
    src = replace_equiv_address (src, mips_split_symbol (dest, XEXP (src, 0),
                                                         SYMBOL_GENERAL));
  emit_move_insn (dest, src);
}


/* If (set DEST SRC) is not a valid instruction, emit an equivalent
   sequence that is valid.  */

bool
mips_legitimize_move (enum machine_mode mode, rtx dest, rtx src)
{
  if (!register_operand (dest, mode) && !reg_or_0_operand (src, mode))
    {
      emit_move_insn (dest, force_reg (mode, src));
      return true;
    }

  /* The source of an SImode move must be a move_operand.  Likewise
     DImode moves on 64-bit targets.  We need to deal with constants
     that would be legitimate immediate_operands but not legitimate
     move_operands.  */
  if ((GET_MODE_SIZE (mode) <= UNITS_PER_WORD
       || (TARGET_MIPS16 && TARGET_NO_DATA_IN_CODE && ! TARGET_GP_OPT))
      && CONSTANT_P (src)
      && !move_operand (src, mode))
    {
      struct cgraph_varpool_node *node;
      mips_legitimize_const_move (mode, dest, src);
      node = section_relative_symbol (src);
      if (node)
        {
          rtx base;
          int offset;
          if (node->bss_off != -1)
            {
              base = mips_base_bss_rtx ();
              offset = node->bss_off;
            }
          else
            {
              base = mips_base_data_rtx ();
              offset = node->data_off;
            }
          set_unique_reg_note (get_last_insn (),
                               REG_EQUAL,
                               plus_constant (base, offset));
        }
      else
        set_unique_reg_note (get_last_insn (), REG_EQUAL, copy_rtx (src));
      return true;
    }
  return false;
}

/* We need a lot of little routines to check constant values on the
   mips16.  These are used to figure out how long the instruction will
   be.  It would be much better to do this using constraints, but
   there aren't nearly enough letters available.  */

static int
m16_check_op (rtx op, int low, int high, int mask)
{
  return (GET_CODE (op) == CONST_INT
          && INTVAL (op) >= low
          && INTVAL (op) <= high
          && (INTVAL (op) & mask) == 0);
}

int
m16_uimm3_b (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, 0x1, 0x8, 0);
}

int
m16_simm4_1 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, - 0x8, 0x7, 0);
}

int
m16_nsimm4_1 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, - 0x7, 0x8, 0);
}

int
m16_simm5_1 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, - 0x10, 0xf, 0);
}

int
m16_nsimm5_1 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, - 0xf, 0x10, 0);
}

int
m16_uimm5_4 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, (- 0x10) << 2, 0xf << 2, 3);
}

int
m16_nuimm5_4 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, (- 0xf) << 2, 0x10 << 2, 3);
}

int
m16_simm8_1 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, - 0x80, 0x7f, 0);
}

int
m16_nsimm8_1 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, - 0x7f, 0x80, 0);
}

int
m16_uimm8_1 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, 0x0, 0xff, 0);
}

int
m16_nuimm8_1 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, - 0xff, 0x0, 0);
}

int
m16_uimm8_m1_1 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, - 0x1, 0xfe, 0);
}

int
m16_uimm8_4 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, 0x0, 0xff << 2, 3);
}

int
m16_nuimm8_4 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, (- 0xff) << 2, 0x0, 3);
}

int
m16_simm8_8 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, (- 0x80) << 3, 0x7f << 3, 7);
}

int
m16_nsimm8_8 (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return m16_check_op (op, (- 0x7f) << 3, 0x80 << 3, 7);
}

static bool
mips_rtx_costs (rtx x, int code, int outer_code, int *total)
{
  enum machine_mode mode = GET_MODE (x);

  switch (code)
    {
    case CONST_INT:
      if (!TARGET_MIPS16)
        {
          if (INTVAL (x) == 0)
            {
              /* Zero can always be used at no cost. */
              *total = 0;
              return true;
            }
          switch (outer_code)
            {
            case AND:
            case IOR:
            case XOR:
            case EQ:
            case NE:
              /* Operand is zero-extended. */
              if (SMALL_OPERAND_UNSIGNED (INTVAL (x)))
                {
                  *total = 0;
                  return true;
                }
              break;
            case PLUS:
            case MINUS:
            case LT:
            case LE:
            case GT:
            case GE:
            case LTU:
            case LEU:
            case GTU:
            case GEU:
              /* Operand is sign-extended. */
              if (SMALL_OPERAND (INTVAL (x)))
                {
                  *total = 0;
                  return true;
                }
              break;
            case ASHIFT:
            case ASHIFTRT:
            case LSHIFTRT:
            case ROTATE:
            case ROTATERT:
              /* Assume this is the shift amount, value to be shifted
                 is handled below. */
              *total = 0;
              return true;
            case SET:
              /* SET has already counted 1 insn, add in any extra */
              *total = COSTS_N_INSNS (mips_const_insns (x) - 1);
              return true;
            }
          /* extra insns needed to load constant into register */
          *total = COSTS_N_INSNS (mips_const_insns (x));
          return true;
        }

      /* MIPS16 */
      switch (outer_code)
        {
        case ASHIFT:
        case ASHIFTRT:
        case LSHIFTRT:
          /* A number between 1 and 8 inclusive is efficient for a shift.
             Otherwise, we will need an extended instruction.  */
          {
            if (INTVAL (x) >= 1 && INTVAL (x) <= 8)
              *total = 0;
            else
              *total = COSTS_N_INSNS (1);
            return true;
          }
          break;

          /* We can use cmpi for an xor with an unsigned 16 bit value.  */
        case XOR:
          if (INTVAL (x) >= 0 && INTVAL (x) < 0x10000)
            {
              *total = 0;
              return true;
            }
          break;

        case PLUS:
          if (INTVAL (x) >= -0x80 && INTVAL (x) <= 0x7f)
            *total = 0;
          else
            /* Assume the constant is always valid (in range), then we
               need an extended instruction. */
            *total = COSTS_N_INSNS (1);
          return true;

          /* We may be able to use slt or sltu for a comparison with a
             signed 16 bit value.  (The boundary conditions aren't quite
             right, but this is just a heuristic anyhow.)  */
        case LT:
        case LE:
        case GE:
        case GT:
        case LTU:
        case LEU:
        case GEU:
        case GTU:
        case COMPARE:
          if (INTVAL (x) >= 0 && INTVAL (x) < 0x100)
            {
              *total = 0;
              return true;
            }
          else if (INTVAL (x) >= -0x8000 && INTVAL (x) < 0x8000)
            {
              *total = COSTS_N_INSNS (1);
              return true;
            }
          break;

          /* Equality comparisons with 0 are cheap.  */
        case EQ:
        case NE:
          if (INTVAL (x) == 0)
            {
              *total = 0;
              return true;
            }
          break;

        default:
          /* Otherwise fall through to the handling below.  */
          break;
        }

    case CONST:
    case SYMBOL_REF:
    case LABEL_REF:
      if (LEGITIMATE_CONSTANT_P (x))
        {
          int length = mips_const_insns (x);
          if (length)
            *total = COSTS_N_INSNS (length);
          else
            abort();
          return true;
        }
      else
        {
          /* The value will need to be fetched from the constant pool.  */
          *total = CONSTANT_POOL_COST;
          return true;
        }

    case CONST_DOUBLE:
      if (LEGITIMATE_CONSTANT_P (x))
        {
          if (mode == DFmode && mips_isa >= 2)
            *total = COSTS_N_INSNS (3);
          else
            *total = COSTS_N_INSNS (4);
          return true;
        }
      else
        {
          /* The value will need to be fetched from the constant pool.  */
          int addrcost = GET_MODE_SIZE (mode) > mips_section_threshold ? 1 : 0;
          if (mode == SFmode || (mode == DFmode && mips_isa >= 2))
            *total = COSTS_N_INSNS (1 + addrcost);
          else if (mode == DFmode)
            *total = COSTS_N_INSNS (2 + addrcost);
          else
            *total = CONSTANT_POOL_COST;
          return true;
        }

    case MEM:
      {
        /* If the address is legitimate, return the number of
           instructions it needs, otherwise use the default handling.  */
        int n = mips_address_insns (XEXP (x, 0), GET_MODE (x));
        if (n > 0)
          {
            if (TARGET_MIPS16 && !mips_section_threshold)
              /* Take into account the cost of storing into the constant
                 pool. */
              *total = COSTS_N_INSNS (2 + n);
            else
              *total = COSTS_N_INSNS (1 + n);
            return true;
          }
        return false;
      }

    case CTZ:
    case FFS:
      *total = COSTS_N_INSNS (6);
      return false;

    case NOT:
      *total = COSTS_N_INSNS ((mode == DImode && !TARGET_64BIT) ? 2 : 1);
      return false;

    case AND:
    case IOR:
    case XOR:
      if (mode == DImode && !TARGET_64BIT)
        {
          *total = COSTS_N_INSNS (2);
          return false;
        }
      return false;

    case ASHIFT:
    case ASHIFTRT:
    case LSHIFTRT:
    case ROTATE:
    case ROTATERT:
      if (mode == DImode && !TARGET_64BIT)
        {
          *total = COSTS_N_INSNS ((GET_CODE (XEXP (x, 1)) == CONST_INT)
                                  ? 4 : 12);
          return false;
        }
      if (GET_CODE (XEXP (x, 0)) == CONST_INT)
        /* CONST_INT case above treats const shift operands as zero cost,
           but we have to load the value to be shifted. */
        *total += COSTS_N_INSNS (mips_const_insns (XEXP (x, 0)));
      return false;

    case ABS:
      if (mode == SFmode || mode == DFmode)
        *total = COSTS_N_INSNS (1);
      else
        *total = COSTS_N_INSNS (4);
      return false;

    case LO_SUM:
      *total = COSTS_N_INSNS (1);
      return true;

    case PLUS:
    case MINUS:
      if (mode == SFmode || mode == DFmode)
        {
          if (TUNE_MIPS3000 || TUNE_MIPS3900)
            *total = COSTS_N_INSNS (2);
          else if (TUNE_MIPS6000)
            *total = COSTS_N_INSNS (3);
          else
            *total = COSTS_N_INSNS (6);
          return false;
        }
      if (mode == DImode && !TARGET_64BIT)
        {
          *total = COSTS_N_INSNS (4);
          return false;
        }
      /* MINUS is not commutative, so first operand must be loaded
         into a register if it is a CONST_INT, overriding
         CONST_INT handling above. */
      if (code == MINUS && SCALAR_INT_MODE_P (mode)
          && GET_CODE (XEXP (x, 0)) == CONST_INT)
        *total += COSTS_N_INSNS (mips_const_insns (XEXP (x, 0)));
      return false;

    case NEG:
      if (mode == DImode && !TARGET_64BIT)
        {
          *total = 4;
          return false;
        }
      return false;

    case MULT:
      {
        int mult3_p;
        if (mode == SImode)
          mult3_p = GENERATE_MULT3_SI || TARGET_MAD;
        else
          mult3_p = GENERATE_MULT3_DI;
        if (optimize_size && SCALAR_INT_MODE_P (mode))
          {
            /* Reflect number of insns, not the execution time. */
            if (mode == DImode && ! TARGET_64BIT)
              /* Synthesized muldi. */
              *total = COSTS_N_INSNS (9);
            else
              *total = COSTS_N_INSNS (mult3_p ? 1 : 2);
          }
        else
          {
            *total = 0;
            if (mips_tune_info)
              {
                if (mode == SImode)
                  *total = COSTS_N_INSNS (mips_tune_info->mulsi_cost
                                          + (mult3_p ? 0 : 1));
                else if (mode == DImode)
                  {
                    if (TARGET_64BIT)
                      *total = COSTS_N_INSNS (mips_tune_info->muldi_cost
                                              + (mult3_p ? 0 : 1));
                    else
                      /* Synthesized muldi. */
                      *total = COSTS_N_INSNS (mips_tune_info->mulsi_cost
                                              * 3 + 6);
                  }
                else if (mode == SFmode && mips_tune_info->mulsf_cost)
                  *total = COSTS_N_INSNS (mips_tune_info->mulsf_cost);
                else if (mode == DFmode && mips_tune_info->mulsf_cost)
                  *total = COSTS_N_INSNS (mips_tune_info->muldf_cost);
              }
            if (*total == 0)
              {
                /* Old defaults. */
                if (mode == SFmode)
                  *total = COSTS_N_INSNS (7);
                else if (mode == DFmode)
                  *total = COSTS_N_INSNS (8);
                else
                  *total = COSTS_N_INSNS (10);
              }
          }
      }
      return false;

    case DIV:
    case MOD:
      if (mode == SFmode)
        {
          if (TUNE_MIPS3000
              || TUNE_MIPS3900)
            *total = COSTS_N_INSNS (12);
          else if (TUNE_MIPS6000)
            *total = COSTS_N_INSNS (15);
          else if (TUNE_MIPS5400 || TUNE_MIPS5500)
            *total = COSTS_N_INSNS (30);
          else
            *total = COSTS_N_INSNS (23);
          return false;
        }

      if (mode == DFmode)
        {
          if (TUNE_MIPS3000
              || TUNE_MIPS3900)
            *total = COSTS_N_INSNS (19);
          else if (TUNE_MIPS5400 || TUNE_MIPS5500)
            *total = COSTS_N_INSNS (59);
          else if (TUNE_MIPS6000)
            *total = COSTS_N_INSNS (16);
          else
            *total = COSTS_N_INSNS (36);
          return false;
        }
      /* Fall through.  */

    case UDIV:
    case UMOD:
      if (optimize_size && SCALAR_INT_MODE_P (mode))
        {
          /* Reflect number of insns, not the execution time. */
          if (TARGET_CHECK_ZERO_DIV)
            *total = COSTS_N_INSNS (5);
          else
            *total = COSTS_N_INSNS (2);
          return false;
        }
      if (TUNE_MIPS3000
          || TUNE_MIPS3900)
        *total = COSTS_N_INSNS (35);
      else if (TUNE_MIPS6000)
        *total = COSTS_N_INSNS (38);
      else if (TUNE_MIPS5000)
        *total = COSTS_N_INSNS (36);
      else if (TUNE_MIPS5400 || TUNE_MIPS5500)
        *total = COSTS_N_INSNS ((mode == SImode) ? 42 : 74);
      else
        *total = COSTS_N_INSNS (69);
      return false;

    case SIGN_EXTEND:
      /* A sign extend from SImode to DImode in 64 bit mode is often
         zero instructions, because the result can often be used
         directly by another instruction; we'll call it one.  */
      if (TARGET_64BIT && mode == DImode
          && GET_MODE (XEXP (x, 0)) == SImode)
        *total = COSTS_N_INSNS (1);
      else if (ISA_HAS_SEB_SEH && (GET_MODE (XEXP (x, 0)) == QImode
                                   || GET_MODE (XEXP (x, 0)) == HImode))
        *total = COSTS_N_INSNS (1);
      else if (memory_operand (XEXP (x, 0), GET_MODE (XEXP (x, 0))))
        *total = COSTS_N_INSNS (1);
      else
        *total = COSTS_N_INSNS (2);
      return true;

    case ZERO_EXTEND:
      if (TARGET_64BIT && mode == DImode
          && GET_MODE (XEXP (x, 0)) == SImode
          && register_operand (XEXP (x, 0), SImode))
        *total = COSTS_N_INSNS (2);
      else
        *total = COSTS_N_INSNS (1);
      return true;

    default:
      return false;
    }
}

/* Provide the costs of an addressing mode that contains ADDR.
   If ADDR is not a valid address, its cost is irrelevant.  */

static int
mips_address_cost (rtx addr)
{
  return mips_address_insns (addr, SImode);
}

/* Return a pseudo that points to the address of the current function.
   The first time it is called for a function, an initializer for the
   pseudo is emitted in the beginning of the function.  */

rtx
embedded_pic_fnaddr_reg (void)
{
  if (cfun->machine->embedded_pic_fnaddr_rtx == NULL)
    {
      rtx seq;

      cfun->machine->embedded_pic_fnaddr_rtx = gen_reg_rtx (Pmode);

      /* Output code at function start to initialize the pseudo-reg.  */
      /* ??? We used to do this in FINALIZE_PIC, but that does not work for
         inline functions, because it is called after RTL for the function
         has been copied.  The pseudo-reg in embedded_pic_fnaddr_rtx however
         does not get copied, and ends up not matching the rest of the RTL.
         This solution works, but means that we get unnecessary code to
         initialize this value every time a function is inlined into another
         function.  */
      start_sequence ();
      emit_insn (gen_get_fnaddr (cfun->machine->embedded_pic_fnaddr_rtx,
                                 XEXP (DECL_RTL (current_function_decl), 0)));
      seq = get_insns ();
      end_sequence ();
      push_topmost_sequence ();
      emit_insn_after (seq, get_insns ());
      pop_topmost_sequence ();
    }

  return cfun->machine->embedded_pic_fnaddr_rtx;
}

/* Return RTL for the offset from the current function to the argument.
   X is the symbol whose offset from the current function we want.  */

rtx
embedded_pic_offset (rtx x)
{
  /* Make sure it is emitted.  */
  embedded_pic_fnaddr_reg ();

  return
    gen_rtx_CONST (Pmode,
                   gen_rtx_MINUS (Pmode, x,
                                  XEXP (DECL_RTL (current_function_decl), 0)));
}

/* Return one word of double-word value OP, taking into account the fixed
   endianness of certain registers.  HIGH_P is true to select the high part,
   false to select the low part.  */

rtx
mips_subword (rtx op, int high_p)
{
  unsigned int byte;
  enum machine_mode mode;

  mode = GET_MODE (op);
  if (mode == VOIDmode)
    mode = DImode;

  if (TARGET_BIG_ENDIAN ? !high_p : high_p)
    byte = UNITS_PER_WORD;
  else
    byte = 0;

  if (GET_CODE (op) == REG)
    {
      if (FP_REG_P (REGNO (op)))
        return gen_rtx_REG (word_mode, high_p ? REGNO (op) + 1 : REGNO (op));
    }

  if (GET_CODE (op) == MEM)
    return mips_rewrite_small_data (adjust_address (op, word_mode, byte));

  return simplify_gen_subreg (word_mode, op, mode, byte);
}


/* Return true if a 64-bit move from SRC to DEST should be split into two.  */

bool
mips_split_64bit_move_p (rtx dest, rtx src)
{
  if (TARGET_64BIT)
    return false;

  /* FP->FP moves can be done in a single instruction.  */
  if (FP_REG_RTX_P (src) && FP_REG_RTX_P (dest))
    return false;

  /* Check for floating-point loads and stores.  They can be done using
     ldc1 and sdc1 on MIPS II and above.  */
  if (mips_isa > 1)
    {
      if (FP_REG_RTX_P (dest) && GET_CODE (src) == MEM)
        return false;
      if (FP_REG_RTX_P (src) && GET_CODE (dest) == MEM)
        return false;
    }
  return true;
}


/* Split a 64-bit move from SRC to DEST assuming that
   mips_split_64bit_move_p holds.

   Moves into and out of FPRs cause some difficulty here.  Such moves
   will always be DFmode, since paired FPRs are not allowed to store
   DImode values.  The most natural representation would be two separate
   32-bit moves, such as:

        (set (reg:SI $f0) (mem:SI ...))
        (set (reg:SI $f1) (mem:SI ...))

   However, the second insn is invalid because odd-numbered FPRs are
   not allowed to store independent values.  Use the patterns load_df_low,
   load_df_high and store_df_high instead.  */

void
mips_split_64bit_move (rtx dest, rtx src)
{
  if (FP_REG_RTX_P (dest))
    {
      /* Loading an FPR from memory or from GPRs.  */
      emit_insn (gen_load_df_low (copy_rtx (dest), mips_subword (src, 0)));
      if (TARGET_FLOAT64 && ISA_HAS_MXHC1 && GET_CODE (src) != MEM)
        emit_insn (gen_mthc1 (dest, mips_subword (src, 1),
                              copy_rtx (dest)));
      else
        emit_insn (gen_load_df_high (dest, mips_subword (src, 1),
                                     copy_rtx (dest)));
    }
  else if (FP_REG_RTX_P (src))
    {
      /* Storing an FPR into memory or GPRs.  */
      emit_move_insn (mips_subword (dest, 0), mips_subword (src, 0));
      if (TARGET_FLOAT64 && ISA_HAS_MXHC1 && GET_CODE (dest) != MEM)
        emit_insn (gen_mfhc1 (mips_subword (dest, 1), src));
      else
        emit_insn (gen_store_df_high (mips_subword (dest, 1), src));
    }
  else
    {
      /* The operation can be split into two normal moves.  Decide in
         which order to do them.  */
      rtx low_dest;

      low_dest = mips_subword (dest, 0);
      if (GET_CODE (low_dest) == REG
          && reg_overlap_mentioned_p (low_dest, src))
        {
          emit_move_insn (mips_subword (dest, 1), mips_subword (src, 1));
          emit_move_insn (low_dest, mips_subword (src, 0));
        }
      else
        {
          emit_move_insn (low_dest, mips_subword (src, 0));
          emit_move_insn (mips_subword (dest, 1), mips_subword (src, 1));
        }
    }
}

/* Return the appropriate instructions to move SRC into DEST.  Assume
   that SRC is operand 1 and DEST is operand 0.  */

const char *
mips_output_move (rtx dest, rtx src)
{
  enum rtx_code dest_code, src_code;
  bool dbl_p;

  dest_code = GET_CODE (dest);
  src_code = GET_CODE (src);
  dbl_p = (GET_MODE_SIZE (GET_MODE (dest)) == 8);

  if (dbl_p && mips_split_64bit_move_p (dest, src))
    return "#";

  if ((src_code == REG && GP_REG_P (REGNO (src)))
      || (!TARGET_MIPS16 && src == CONST0_RTX (GET_MODE (dest))))
    {
      if (dest_code == REG)
        {
          if (GP_REG_P (REGNO (dest)))
            return "move\t%0,%z1";

          if (MD_REG_P (REGNO (dest)))
            return "mt%0\t%z1";

          if (DSP_ACC_REG_P (REGNO (dest)))
            {
              static char retval[] = "mt__\t%z1,%q0";
              retval[2] = mips_reg_names[REGNO (dest)][4];
              retval[3] = mips_reg_names[REGNO (dest)][5];
              return retval;
            }

          if (FP_REG_P (REGNO (dest)))
            return (dbl_p ? "dmtc1\t%z1,%0" : "mtc1\t%z1,%0");

          if (ALL_COP_REG_P (REGNO (dest)))
            {
              static char retval[] = "dmtc_\t%z1,%0";

              retval[4] = COPNUM_AS_CHAR_FROM_REGNUM (REGNO (dest));
              return (dbl_p ? retval : retval + 1);
            }
        }
      if (dest_code == MEM)
        return (dbl_p ? "sd\t%z1,%0" : "sw\t%z1,%0");
    }
  if (dest_code == REG && GP_REG_P (REGNO (dest)))
    {
      if (src_code == REG)
        {
          if (MD_REG_P (REGNO (src)))
            return "mf%1\t%0";

          if (DSP_ACC_REG_P (REGNO (src)))
            {
              static char retval[] = "mf__\t%0,%q1";
              retval[2] = mips_reg_names[REGNO (src)][4];
              retval[3] = mips_reg_names[REGNO (src)][5];
              return retval;
            }

          if (ST_REG_P (REGNO (src)) && ISA_HAS_8CC)
            return "lui\t%0,0x3f80\n\tmovf\t%0,%.,%1";

          if (FP_REG_P (REGNO (src)))
            return (dbl_p ? "dmfc1\t%0,%1" : "mfc1\t%0,%1");

          if (ALL_COP_REG_P (REGNO (src)))
            {
              static char retval[] = "dmfc_\t%0,%1";

              retval[4] = COPNUM_AS_CHAR_FROM_REGNUM (REGNO (src));
              return (dbl_p ? retval : retval + 1);
            }
        }

      if (src_code == MEM)
        return (dbl_p ? "ld\t%0,%1" : "lw\t%0,%1");

      if (src_code == CONST_INT)
        {
          /* Don't use the X format, because that will give out of
             range numbers for 64 bit hosts and 32 bit targets.  */
          if (!TARGET_MIPS16)
            return "li\t%0,%1\t\t\t# %X1";

          if (INTVAL (src) >= 0 && INTVAL (src) <= 0xffff)
            return "li\t%0,%1";

          if (INTVAL (src) < 0 && INTVAL (src) >= -0xffff)
            return "li\t%0,%n1\n\tneg\t%0";
        }

      if (src_code == HIGH)
        return "lui\t%0,%h1";

      if (CONST_GP_P (src))
        return "move\t%0,%1";

      if (symbolic_operand (src, VOIDmode))
        return (dbl_p ? "dla\t%0,%1" : "la\t%0,%1");
    }
  if (src_code == REG && FP_REG_P (REGNO (src)))
    {
      if (dest_code == REG && FP_REG_P (REGNO (dest)))
        {
          if (GET_MODE(dest) == V2SFmode)
            return "mov.ps\t%0,%1";
          else
            return (dbl_p ? "mov.d\t%0,%1" : "mov.s\t%0,%1");
        }

      if (dest_code == MEM)
        return (dbl_p ? "sdc1\t%1,%0" : "swc1\t%1,%0");
    }
  if (dest_code == REG && FP_REG_P (REGNO (dest)))
    {
      if (src_code == MEM)
        return (dbl_p ? "ldc1\t%0,%1" : "lwc1\t%0,%1");
    }
  if (dest_code == REG && ALL_COP_REG_P (REGNO (dest)) && src_code == MEM)
    {
      static char retval[] = "l_c_\t%0,%1";

      retval[1] = (dbl_p ? 'd' : 'w');
      retval[3] = COPNUM_AS_CHAR_FROM_REGNUM (REGNO (dest));
      return retval;
    }
  if (dest_code == MEM && src_code == REG && ALL_COP_REG_P (REGNO (src)))
    {
      static char retval[] = "s_c_\t%1,%0";

      retval[1] = (dbl_p ? 'd' : 'w');
      retval[3] = COPNUM_AS_CHAR_FROM_REGNUM (REGNO (src));
      return retval;
    }
  abort ();
}

/* Restore $gp from its save slot.  Valid only when using o32 or
   o64 abicalls.  */

void
mips_restore_gp (void)
{
  rtx address, slot;

  if (!TARGET_ABICALLS || !(mips_abi == ABI_32 || mips_abi == ABI_O64))
    abort ();

  address = mips_add_offset (pic_offset_table_rtx,
                             frame_pointer_needed
                             ? hard_frame_pointer_rtx
                             : stack_pointer_rtx,
                             current_function_outgoing_args_size);
  slot = gen_rtx_MEM (Pmode, address);
  RTX_UNCHANGING_P (slot) = 1;

  emit_move_insn (pic_offset_table_rtx, slot);
  if (!TARGET_EXPLICIT_RELOCS)
    emit_insn (gen_blockage ());
}


/* Make normal rtx_code into something we can index from an array */

static enum internal_test
map_test_to_internal_test (enum rtx_code test_code)
{
  enum internal_test test = ITEST_MAX;

  switch (test_code)
    {
    case EQ:  test = ITEST_EQ;  break;
    case NE:  test = ITEST_NE;  break;
    case GT:  test = ITEST_GT;  break;
    case GE:  test = ITEST_GE;  break;
    case LT:  test = ITEST_LT;  break;
    case LE:  test = ITEST_LE;  break;
    case GTU: test = ITEST_GTU; break;
    case GEU: test = ITEST_GEU; break;
    case LTU: test = ITEST_LTU; break;
    case LEU: test = ITEST_LEU; break;
    default:                    break;
    }

  return test;
}


/* Generate the code to compare two integer values.  The return value is:
   (reg:SI xx)          The pseudo register the comparison is in
   0                    No register, generate a simple branch.

   ??? This is called with result nonzero by the Scond patterns in
   mips.md.  These patterns are called with a target in the mode of
   the Scond instruction pattern.  Since this must be a constant, we
   must use SImode.  This means that if RESULT is nonzero, it will
   always be an SImode register, even if TARGET_64BIT is true.  We
   cope with this by calling convert_move rather than emit_move_insn.
   This will sometimes lead to an unnecessary extension of the result;
   for example:

   long long
   foo (long long i)
   {
     return i < 5;
   }

   TEST_CODE is the rtx code for the comparison.
   CMP0 and CMP1 are the two operands to compare.
   RESULT is the register in which the result should be stored (null for
     branches).
   For branches, P_INVERT points to an integer that is nonzero on return
     if the branch should be inverted.  */

rtx
gen_int_relational (enum rtx_code test_code, rtx result, rtx cmp0,
                    rtx cmp1, int *p_invert)
{
  struct cmp_info
  {
    enum rtx_code test_code;    /* code to use in instruction (LT vs. LTU) */
    int const_low;              /* low bound of constant we can accept */
    int const_high;             /* high bound of constant we can accept */
    int const_add;              /* constant to add (convert LE -> LT) */
    int reverse_regs;           /* reverse registers in test */
    int invert_const;           /* != 0 if invert value if cmp1 is constant */
    int invert_reg;             /* != 0 if invert value if cmp1 is register */
    int unsignedp;              /* != 0 for unsigned comparisons.  */
  };

  static const struct cmp_info info[ (int)ITEST_MAX ] = {

    { XOR,       0,  65535,  0,  0,  0,  0, 0 },        /* EQ  */
    { XOR,       0,  65535,  0,  0,  1,  1, 0 },        /* NE  */
    { LT,   -32769,  32766,  1,  1,  1,  0, 0 },        /* GT  */
    { LT,   -32768,  32767,  0,  0,  1,  1, 0 },        /* GE  */
    { LT,   -32768,  32767,  0,  0,  0,  0, 0 },        /* LT  */
    { LT,   -32769,  32766,  1,  1,  0,  1, 0 },        /* LE  */
    { LTU,  -32769,  32766,  1,  1,  1,  0, 1 },        /* GTU */
    { LTU,  -32768,  32767,  0,  0,  1,  1, 1 },        /* GEU */
    { LTU,  -32768,  32767,  0,  0,  0,  0, 1 },        /* LTU */
    { LTU,  -32769,  32766,  1,  1,  0,  1, 1 },        /* LEU */
  };

  enum internal_test test;
  enum machine_mode mode;
  const struct cmp_info *p_info;
  int branch_p;
  int eqne_p;
  int invert;
  rtx reg;
  rtx reg2;

  test = map_test_to_internal_test (test_code);
  if (test == ITEST_MAX)
    abort ();

  p_info = &info[(int) test];
  eqne_p = (p_info->test_code == XOR);

  mode = GET_MODE (cmp0);
  if (mode == VOIDmode)
    mode = GET_MODE (cmp1);

  /* Eliminate simple branches */
  branch_p = (result == 0);
  if (branch_p)
    {
      if (GET_CODE (cmp0) == REG || GET_CODE (cmp0) == SUBREG)
        {
          /* Comparisons against zero are simple branches */
          if (GET_CODE (cmp1) == CONST_INT && INTVAL (cmp1) == 0
              && (! TARGET_MIPS16 || eqne_p))
            return 0;

          /* Test for beq/bne.  */
          if (eqne_p && ! TARGET_MIPS16)
            return 0;
        }

      /* Allocate a pseudo to calculate the value in.  */
      result = gen_reg_rtx (mode);
    }

  /* Make sure we can handle any constants given to us.  */
  if (GET_CODE (cmp0) == CONST_INT)
    cmp0 = force_reg (mode, cmp0);

  if (GET_CODE (cmp1) == CONST_INT)
    {
      HOST_WIDE_INT value = INTVAL (cmp1);

      if (value < p_info->const_low
          || value > p_info->const_high
          /* ??? Why?  And why wasn't the similar code below modified too?  */
          || (TARGET_64BIT
              && HOST_BITS_PER_WIDE_INT < 64
              && p_info->const_add != 0
              && ((p_info->unsignedp
                   ? ((unsigned HOST_WIDE_INT) (value + p_info->const_add)
                      > (unsigned HOST_WIDE_INT) INTVAL (cmp1))
                   : (value + p_info->const_add) > INTVAL (cmp1))
                  != (p_info->const_add > 0))))
        cmp1 = force_reg (mode, cmp1);
    }

  /* See if we need to invert the result.  */
  invert = (GET_CODE (cmp1) == CONST_INT
            ? p_info->invert_const : p_info->invert_reg);

  if (p_invert != (int *)0)
    {
      *p_invert = invert;
      invert = 0;
    }

  /* Comparison to constants, may involve adding 1 to change a LT into LE.
     Comparison between two registers, may involve switching operands.  */
  if (GET_CODE (cmp1) == CONST_INT)
    {
      if (p_info->const_add != 0)
        {
          HOST_WIDE_INT new = INTVAL (cmp1) + p_info->const_add;

          /* If modification of cmp1 caused overflow,
             we would get the wrong answer if we follow the usual path;
             thus, x > 0xffffffffU would turn into x > 0U.  */
          if ((p_info->unsignedp
               ? (unsigned HOST_WIDE_INT) new >
               (unsigned HOST_WIDE_INT) INTVAL (cmp1)
               : new > INTVAL (cmp1))
              != (p_info->const_add > 0))
            {
              /* This test is always true, but if INVERT is true then
                 the result of the test needs to be inverted so 0 should
                 be returned instead.  */
              emit_move_insn (result, invert ? const0_rtx : const_true_rtx);
              return result;
            }
          else
            cmp1 = GEN_INT (new);
        }
    }

  else if (p_info->reverse_regs)
    {
      rtx temp = cmp0;
      cmp0 = cmp1;
      cmp1 = temp;
    }

  if (test == ITEST_NE && GET_CODE (cmp1) == CONST_INT && INTVAL (cmp1) == 0)
    reg = cmp0;
  else
    {
      reg = (invert || eqne_p) ? gen_reg_rtx (mode) : result;
      convert_move (reg, gen_rtx (p_info->test_code, mode, cmp0, cmp1), 0);
    }

  if (test == ITEST_NE)
    {
      if (! TARGET_MIPS16)
        {
          convert_move (result, gen_rtx (GTU, mode, reg, const0_rtx), 0);
          if (p_invert != NULL)
            *p_invert = 0;
          invert = 0;
        }
      else
        {
          reg2 = invert ? gen_reg_rtx (mode) : result;
          convert_move (reg2, gen_rtx (LTU, mode, reg, const1_rtx), 0);
          reg = reg2;
        }
    }

  else if (test == ITEST_EQ)
    {
      reg2 = invert ? gen_reg_rtx (mode) : result;
      convert_move (reg2, gen_rtx_LTU (mode, reg, const1_rtx), 0);
      reg = reg2;
    }

  if (invert)
    {
      rtx one;

      if (! TARGET_MIPS16)
        one = const1_rtx;
      else
        {
          /* The value is in $24.  Copy it to another register, so
             that reload doesn't think it needs to store the $24 and
             the input to the XOR in the same location.  */
          reg2 = gen_reg_rtx (mode);
          emit_move_insn (reg2, reg);
          reg = reg2;
          one = force_reg (mode, const1_rtx);
        }
      convert_move (result, gen_rtx (XOR, mode, reg, one), 0);
    }

  return result;
}

/* Work out how to check a floating-point condition.  We need a
   separate comparison instruction (C.cond.fmt), followed by a
   branch or conditional move.  Given that IN_CODE is the
   required condition, set *CMP_CODE to the C.cond.fmt code
   and *action_code to the branch or move code.  */

static void
get_float_compare_codes (enum rtx_code in_code, enum rtx_code *cmp_code,
                         enum rtx_code *action_code)
{
  switch (in_code)
    {
    case NE:
    case UNGE:
    case UNGT:
    case LTGT:
    case ORDERED:
      *cmp_code = reverse_condition_maybe_unordered (in_code);
      *action_code = EQ;
      break;

    default:
      *cmp_code = in_code;
      *action_code = NE;
      break;
    }
}

/* Emit the common code for doing conditional branches.
   operand[0] is the label to jump to.
   The comparison operands are saved away by cmp{si,di,sf,df}.  */

void
gen_conditional_branch (rtx *operands, enum rtx_code test_code)
{
  enum cmp_type type = branch_type;
  rtx cmp0 = branch_cmp[0];
  rtx cmp1 = branch_cmp[1];
  enum machine_mode mode;
  enum rtx_code cmp_code;
  rtx reg;
  int invert;
  rtx label1, label2;

  switch (type)
    {
    case CMP_SI:
    case CMP_DI:
      mode = type == CMP_SI ? SImode : DImode;
      invert = 0;
      reg = gen_int_relational (test_code, NULL_RTX, cmp0, cmp1, &invert);

      if (reg)
        {
          cmp0 = reg;
          cmp1 = const0_rtx;
          test_code = NE;
          if (invert)
            {
              test_code = EQ;
              invert = 0;
            }
        }
      else if (GET_CODE (cmp1) == CONST_INT && INTVAL (cmp1) != 0)
        /* We don't want to build a comparison against a nonzero
           constant.  */
        cmp1 = force_reg (mode, cmp1);

      break;

    case CMP_SF:
    case CMP_DF:
      if (! ISA_HAS_8CC)
        reg = gen_rtx_REG (CCmode, FPSW_REGNUM);
      else
        reg = gen_reg_rtx (CCmode);

      get_float_compare_codes (test_code, &cmp_code, &test_code);
      emit_insn (gen_rtx_SET (VOIDmode, reg,
                              gen_rtx (cmp_code, CCmode, cmp0, cmp1)));

      mode = CCmode;
      cmp0 = reg;
      cmp1 = const0_rtx;
      invert = 0;
      break;

    default:
      fatal_insn ("bad test", gen_rtx (test_code, VOIDmode, cmp0, cmp1));
    }

  /* Generate the branch.  */

  label1 = gen_rtx_LABEL_REF (VOIDmode, operands[0]);
  label2 = pc_rtx;

  if (invert)
    {
      fatal_insn ("MIPS: inverting branch not expected",
                  gen_rtx (test_code, VOIDmode, cmp0, cmp1));
      abort();

#if 0
      label2 = label1;
      label1 = pc_rtx;
#endif

    }

  emit_jump_insn (gen_rtx_SET (VOIDmode, pc_rtx,
                               gen_rtx_IF_THEN_ELSE (VOIDmode,
                                                     gen_rtx (test_code, mode,
                                                              cmp0, cmp1),
                                                     label1, label2)));
}

/* Emit the common code for conditional moves.  OPERANDS is the array
   of operands passed to the conditional move define_expand.  */

void
gen_conditional_move (rtx *operands)
{
  rtx op0 = branch_cmp[0];
  rtx op1 = branch_cmp[1];
  enum machine_mode mode = GET_MODE (branch_cmp[0]);
  enum rtx_code cmp_code = GET_CODE (operands[1]);
  enum rtx_code move_code = NE;
  enum machine_mode op_mode = GET_MODE (operands[0]);
  enum machine_mode cmp_mode;
  rtx cmp_reg;

  if (GET_MODE_CLASS (mode) != MODE_FLOAT)
    {
      switch (cmp_code)
        {
        case EQ:
          cmp_code = XOR;
          move_code = EQ;
          break;
        case NE:
          cmp_code = XOR;
          break;
        case LT:
          break;
        case GE:
          cmp_code = LT;
          move_code = EQ;
          break;
        case GT:
          cmp_code = LT;
          op0 = force_reg (mode, branch_cmp[1]);
          op1 = branch_cmp[0];
          break;
        case LE:
          cmp_code = LT;
          op0 = force_reg (mode, branch_cmp[1]);
          op1 = branch_cmp[0];
          move_code = EQ;
          break;
        case LTU:
          break;
        case GEU:
          cmp_code = LTU;
          move_code = EQ;
          break;
        case GTU:
          cmp_code = LTU;
          op0 = force_reg (mode, branch_cmp[1]);
          op1 = branch_cmp[0];
          break;
        case LEU:
          cmp_code = LTU;
          op0 = force_reg (mode, branch_cmp[1]);
          op1 = branch_cmp[0];
          move_code = EQ;
          break;
        default:
          abort ();
        }
    }
  else
    get_float_compare_codes (cmp_code, &cmp_code, &move_code);

  if (mode == SImode || mode == DImode)
    cmp_mode = mode;
  else if (mode == SFmode || mode == DFmode)
    cmp_mode = CCmode;
  else
    abort ();

  cmp_reg = gen_reg_rtx (cmp_mode);
  emit_insn (gen_rtx_SET (cmp_mode, cmp_reg,
                          gen_rtx (cmp_code, cmp_mode, op0, op1)));

  emit_insn (gen_rtx_SET (op_mode, operands[0],
                          gen_rtx_IF_THEN_ELSE (op_mode,
                                                gen_rtx (move_code, VOIDmode,
                                                         cmp_reg,
                                                         CONST0_RTX (SImode)),
                                                operands[2], operands[3])));
}

/* Emit a conditional trap.  OPERANDS is the array of operands passed to
   the conditional_trap expander.  */

void
mips_gen_conditional_trap (rtx *operands)
{
  rtx op0, op1;
  enum rtx_code cmp_code = GET_CODE (operands[0]);
  enum machine_mode mode = GET_MODE (branch_cmp[0]);

  /* MIPS conditional trap machine instructions don't have GT or LE
     flavors, so we must invert the comparison and convert to LT and
     GE, respectively.  */
  switch (cmp_code)
    {
    case GT: cmp_code = LT; break;
    case LE: cmp_code = GE; break;
    case GTU: cmp_code = LTU; break;
    case LEU: cmp_code = GEU; break;
    default: break;
    }
  if (cmp_code == GET_CODE (operands[0]))
    {
      op0 = force_reg (mode, branch_cmp[0]);
      op1 = branch_cmp[1];
    }
  else
    {
      op0 = force_reg (mode, branch_cmp[1]);
      op1 = branch_cmp[0];
    }
  if (GET_CODE (op1) == CONST_INT && ! SMALL_INT (op1))
    op1 = force_reg (mode, op1);

  emit_insn (gen_rtx_TRAP_IF (VOIDmode,
                              gen_rtx (cmp_code, GET_MODE (operands[0]), op0, op1),
                              operands[1]));
}

/* Load function address ADDR into register DEST.  SIBCALL_P is true
   if the address is needed for a sibling call.  */

static void
mips_load_call_address (rtx dest, rtx addr, int sibcall_p)
{
  /* If we're generating PIC, and this call is to a global function,
     try to allow its address to be resolved lazily.  This isn't
     possible for NewABI sibcalls since the value of $gp on entry
     to the stub would be our caller's gp, not ours.  */
  if (TARGET_EXPLICIT_RELOCS
      && !(sibcall_p && TARGET_NEWABI)
      && global_got_operand (addr, VOIDmode))
    {
      rtx high, lo_sum_symbol;

      high = mips_unspec_offset_high (dest, pic_offset_table_rtx,
                                      addr, SYMBOL_GOTOFF_CALL);
      lo_sum_symbol = mips_unspec_address (addr, SYMBOL_GOTOFF_CALL);
      if (Pmode == SImode)
        emit_insn (gen_load_callsi (dest, high, lo_sum_symbol));
      else
        emit_insn (gen_load_calldi (dest, high, lo_sum_symbol));
    }
  else if (TARGET_MIPS16 && TARGET_NO_DATA_IN_CODE
           && (reload_in_progress || reload_completed))
    {
      /* We must fetch addr from constant pool manually.  */
      rtx constmem = force_const_mem (Pmode, addr);
      emit_move_insn (dest, XEXP (constmem, 0));
      emit_move_insn (dest, gen_rtx_MEM (Pmode, dest));
    }
  else
    emit_move_insn (dest, addr);
}


/* Expand a call or call_value instruction.  RESULT is where the
   result will go (null for calls), ADDR is the address of the
   function, ARGS_SIZE is the size of the arguments and AUX is
   the value passed to us by mips_function_arg.  SIBCALL_P is true
   if we are expanding a sibling call, false if we're expanding
   a normal call.  */

void
mips_expand_call (rtx result, rtx addr, rtx args_size, rtx aux, int sibcall_p)
{
  rtx orig_addr, pattern, insn;

  orig_addr = addr;
  if (!call_insn_operand (addr, VOIDmode))
    {
      addr = gen_reg_rtx (Pmode);
      mips_load_call_address (addr, orig_addr, sibcall_p);
    }

  if (TARGET_MIPS16
      && mips16_hard_float
      && build_mips16_call_stub (result, addr, args_size,
                                 aux == 0 ? 0 : (int) GET_MODE (aux)))
    return;

  /* If we are generating non-mips16 code for this function and we pass
     -mips16 on the command line, we need to generate a fn_stub for any
     builtin functions that we call.  */
  if (!TARGET_MIPS16
      && mips_base_mips16
      && mips16_hard_float
      && GET_CODE (addr) == SYMBOL_REF
      && SYMBOL_REF_DECL (addr)
      && TREE_TYPE (SYMBOL_REF_DECL (addr))
      && TREE_TYPE (SYMBOL_REF_DECL (addr)) != error_mark_node
      && DECL_BUILT_IN (SYMBOL_REF_DECL (addr)))
    build_mips16_function_stub (asm_out_file, SYMBOL_REF_DECL (addr),
                                aux == 0 ? 0 : (int) GET_MODE (aux));

  if (result == 0)
    pattern = (sibcall_p
               ? gen_sibcall_internal (addr, args_size)
               : gen_call_internal (addr, args_size));
  else if (GET_CODE (result) == PARALLEL && XVECLEN (result, 0) == 2)
    {
      rtx reg1, reg2;

      reg1 = XEXP (XVECEXP (result, 0, 0), 0);
      reg2 = XEXP (XVECEXP (result, 0, 1), 0);
      pattern =
        (sibcall_p
         ? gen_sibcall_value_multiple_internal (reg1, addr, args_size, reg2)
         : gen_call_value_multiple_internal (reg1, addr, args_size, reg2));
    }
  else
    pattern = (sibcall_p
               ? gen_sibcall_value_internal (result, addr, args_size)
               : gen_call_value_internal (result, addr, args_size));

  insn = emit_call_insn (pattern);

  /* Lazy-binding stubs require $gp to be valid on entry.  */
  if (global_got_operand (orig_addr, VOIDmode))
    use_reg (&CALL_INSN_FUNCTION_USAGE (insn), pic_offset_table_rtx);
}


/* We can handle any sibcall except when TARGET_SIBCALLS is true, or
   when the called function is a MIPS16 function, since there is no
   direct "jx" instruction equivalent to "jalx" to switch the ISA
   mode.
   
   MCHP: cannot handle sibcalls from interrupt handler functions since
   we need extra epilogue code.  */

static bool
mips_function_ok_for_sibcall (tree decl,
                              tree exp ATTRIBUTE_UNUSED)
{
  if (!TARGET_SIBCALLS)
    return 0;
  if (decl && SYMBOL_REF_MIPS16_P (XEXP (DECL_RTL (decl), 0)))
    return 0;
  if (mchp_function_interrupt_p (current_function_decl))
    return 0;
  return 1;
}

/* Return true if operand OP is a condition code register.
   Only for use during or after reload.  */

int
fcc_register_operand (rtx op, enum machine_mode mode)
{
  return ((mode == VOIDmode || mode == GET_MODE (op))
          && (reload_in_progress || reload_completed)
          && (GET_CODE (op) == REG || GET_CODE (op) == SUBREG)
          && ST_REG_P (true_regnum (op)));
}

/* Emit code to move general operand SRC into condition-code
   register DEST.  SCRATCH is a scratch TFmode float register.
   The sequence is:

        FP1 = SRC
        FP2 = 0.0f
        DEST = FP2 < FP1

   where FP1 and FP2 are single-precision float registers
   taken from SCRATCH.  */

void
mips_emit_fcc_reload (rtx dest, rtx src, rtx scratch)
{
  rtx fp1, fp2;

  /* Change the source to SFmode.  */
  if (GET_CODE (src) == MEM)
    src = adjust_address (src, SFmode, 0);
  else if (GET_CODE (src) == REG || GET_CODE (src) == SUBREG)
    src = gen_rtx_REG (SFmode, true_regnum (src));

  fp1 = gen_rtx_REG (SFmode, REGNO (scratch));
  fp2 = gen_rtx_REG (SFmode, REGNO (scratch) + FP_INC);

  emit_move_insn (copy_rtx (fp1), src);
  emit_move_insn (copy_rtx (fp2), CONST0_RTX (SFmode));
  emit_insn (gen_slt_sf (dest, fp2, fp1));
}

/* Emit code to change the current function's return address to
   ADDRESS.  SCRATCH is available as a scratch register, if needed.
   ADDRESS and SCRATCH are both word-mode GPRs.  */

void
mips_set_return_address (rtx address, rtx scratch)
{
  rtx slot_address;

  compute_frame_size (get_frame_size ());
  if (((cfun->machine->frame.mask >> 31) & 1) == 0)
    abort ();
  slot_address = mips_add_offset (scratch, stack_pointer_rtx,
                                  cfun->machine->frame.gp_sp_offset);

  emit_move_insn (gen_rtx_MEM (GET_MODE (address), slot_address), address);
}

/* Emit straight-line code to move LENGTH bytes from SRC to DEST.
   Assume that the areas do not overlap.  */

static void
mips_block_move_straight (rtx dest, rtx src, HOST_WIDE_INT length)
{
  HOST_WIDE_INT offset, delta;
  unsigned HOST_WIDE_INT bits;
  int i;
  enum machine_mode mode;
  rtx *regs;

  /* Work out how many bits to move at a time.  If both operands have
     half-word alignment, it is usually better to move in half words.
     For instance, lh/lh/sh/sh is usually better than lwl/lwr/swl/swr
     and lw/lw/sw/sw is usually better than ldl/ldr/sdl/sdr.
     Otherwise move word-sized chunks.  */
  if (MEM_ALIGN (src) == BITS_PER_WORD / 2
      && MEM_ALIGN (dest) == BITS_PER_WORD / 2)
    bits = BITS_PER_WORD / 2;
  else
    bits = BITS_PER_WORD;

  mode = mode_for_size (bits, MODE_INT, 0);
  delta = bits / BITS_PER_UNIT;

  /* Allocate a buffer for the temporary registers.  */
  regs = alloca (sizeof (rtx) * length / delta);

  /* Load as many BITS-sized chunks as possible.  Use a normal load if
     the source has enough alignment, otherwise use left/right pairs.  */
  for (offset = 0, i = 0; offset + delta <= length; offset += delta, i++)
    {
      regs[i] = gen_reg_rtx (mode);
      if (MEM_ALIGN (src) >= bits)
        emit_move_insn (regs[i], adjust_address (src, mode, offset));
      else
        {
          rtx part = adjust_address (src, BLKmode, offset);
          if (!mips_expand_unaligned_load (regs[i], part, bits, 0))
            abort ();
        }
    }

  /* Copy the chunks to the destination.  */
  for (offset = 0, i = 0; offset + delta <= length; offset += delta, i++)
    if (MEM_ALIGN (dest) >= bits)
      emit_move_insn (adjust_address (dest, mode, offset), regs[i]);
    else
      {
        rtx part = adjust_address (dest, BLKmode, offset);
        if (!mips_expand_unaligned_store (part, regs[i], bits, 0))
          abort ();
      }

  /* Mop up any left-over bytes.  */
  if (offset < length)
    {
      src = adjust_address (src, BLKmode, offset);
      dest = adjust_address (dest, BLKmode, offset);
      move_by_pieces (dest, src, length - offset,
                      MIN (MEM_ALIGN (src), MEM_ALIGN (dest)), 0);
    }
}

#define MAX_MOVE_REGS 4
#define MAX_MOVE_BYTES (MAX_MOVE_REGS * UNITS_PER_WORD)


/* Helper function for doing a loop-based block operation on memory
   reference MEM.  Each iteration of the loop will operate on LENGTH
   bytes of MEM.

   Create a new base register for use within the loop and point it to
   the start of MEM.  Create a new memory reference that uses this
   register.  Store them in *LOOP_REG and *LOOP_MEM respectively.  */

static void
mips_adjust_block_mem (rtx mem, HOST_WIDE_INT length,
                       rtx *loop_reg, rtx *loop_mem)
{
  *loop_reg = copy_addr_to_reg (XEXP (mem, 0));

  /* Although the new mem does not refer to a known location,
     it does keep up to LENGTH bytes of alignment.  */
  *loop_mem = change_address (mem, BLKmode, *loop_reg);
  set_mem_align (*loop_mem, MIN (MEM_ALIGN (mem), length * BITS_PER_UNIT));
}


/* Move LENGTH bytes from SRC to DEST using a loop that moves MAX_MOVE_BYTES
   per iteration.  LENGTH must be at least MAX_MOVE_BYTES.  Assume that the
   memory regions do not overlap.  */

static void
mips_block_move_loop (rtx dest, rtx src, HOST_WIDE_INT length)
{
  rtx label, src_reg, dest_reg, final_src;
  HOST_WIDE_INT leftover;

  leftover = length % MAX_MOVE_BYTES;
  length -= leftover;

  /* Create registers and memory references for use within the loop.  */
  mips_adjust_block_mem (src, MAX_MOVE_BYTES, &src_reg, &src);
  mips_adjust_block_mem (dest, MAX_MOVE_BYTES, &dest_reg, &dest);

  /* Calculate the value that SRC_REG should have after the last iteration
     of the loop.  */
  final_src = expand_simple_binop (Pmode, PLUS, src_reg, GEN_INT (length),
                                   0, 0, OPTAB_WIDEN);

  /* Emit the start of the loop.  */
  label = gen_label_rtx ();
  emit_label (label);

  /* Emit the loop body.  */
  mips_block_move_straight (dest, src, MAX_MOVE_BYTES);

  /* Move on to the next block.  */
  emit_move_insn (src_reg, plus_constant (src_reg, MAX_MOVE_BYTES));
  emit_move_insn (dest_reg, plus_constant (dest_reg, MAX_MOVE_BYTES));

  /* Emit the loop condition.  */
  if (Pmode == DImode)
    emit_insn (gen_cmpdi (src_reg, final_src));
  else
    emit_insn (gen_cmpsi (src_reg, final_src));
  emit_jump_insn (gen_bne (label));

  /* Mop up any left-over bytes.  */
  if (leftover)
    mips_block_move_straight (dest, src, leftover);
}

/* Expand a movstrsi instruction.  */

bool
mips_expand_block_move (rtx dest, rtx src, rtx length)
{
  if (GET_CODE (length) == CONST_INT)
    {
      if (INTVAL (length) <= 2 * MAX_MOVE_BYTES)
        {
          mips_block_move_straight (dest, src, INTVAL (length));
          return true;
        }
      else if (optimize)
        {
          mips_block_move_loop (dest, src, INTVAL (length));
          return true;
        }
    }
  return false;
}

/* Argument support functions.  */

/* Initialize CUMULATIVE_ARGS for a function.  */

void
init_cumulative_args (CUMULATIVE_ARGS *cum, tree fntype,
                      rtx libname ATTRIBUTE_UNUSED)
{
  static CUMULATIVE_ARGS zero_cum;
  tree param, next_param;

  if (TARGET_DEBUG_E_MODE)
    {
      fprintf (stderr,
               "\ninit_cumulative_args, fntype = 0x%.8lx", (long)fntype);

      if (!fntype)
        fputc ('\n', stderr);

      else
        {
          tree ret_type = TREE_TYPE (fntype);
          fprintf (stderr, ", fntype code = %s, ret code = %s\n",
                   tree_code_name[(int)TREE_CODE (fntype)],
                   tree_code_name[(int)TREE_CODE (ret_type)]);
        }
    }

  *cum = zero_cum;
  cum->prototype = (fntype && TYPE_ARG_TYPES (fntype));

  /* Determine if this function has variable arguments.  This is
     indicated by the last argument being 'void_type_mode' if there
     are no variable arguments.  The standard MIPS calling sequence
     passes all arguments in the general purpose registers in this case.  */

  for (param = fntype ? TYPE_ARG_TYPES (fntype) : 0;
       param != 0; param = next_param)
    {
      next_param = TREE_CHAIN (param);
      if (next_param == 0 && TREE_VALUE (param) != void_type_node)
        cum->gp_reg_found = 1;
    }
}


/* Fill INFO with information about a single argument.  CUM is the
   cumulative state for earlier arguments.  MODE is the mode of this
   argument and TYPE is its type (if known).  NAMED is true if this
   is a named (fixed) argument rather than a variable one.  */

static void
mips_arg_info (const CUMULATIVE_ARGS *cum, enum machine_mode mode,
               tree type, int named, struct mips_arg_info *info)
{
  bool doubleword_aligned_p;
  unsigned int num_bytes, num_words, max_regs;

  /* Decide whether this argument should go in a floating-point register,
     assuming one is free.  Later code checks for availability.  */

  info->fpr_p = ((GET_MODE_CLASS (mode) == MODE_FLOAT
                  || (GET_MODE_CLASS (mode) == MODE_VECTOR_FLOAT
                      && TARGET_PAIRED_SINGLE_FLOAT))
                  && GET_MODE_SIZE (mode) <= UNITS_PER_FPVALUE);

  if (info->fpr_p)
    switch (mips_abi)
      {
      case ABI_32:
      case ABI_O64:
        info->fpr_p = (!cum->gp_reg_found
                       && cum->arg_number < 2
                       && (type == 0 || FLOAT_TYPE_P (type)
                           || (TYPE_MODE (type) == V2SFmode
                               && TARGET_PAIRED_SINGLE_FLOAT)));
        break;

      case ABI_N32:
      case ABI_64:
        info->fpr_p = (named && (type == 0 || FLOAT_TYPE_P (type)
                                 || (TYPE_MODE (type) == V2SFmode
                                     && TARGET_PAIRED_SINGLE_FLOAT)));
        break;
      }

  /* See whether the argument has doubleword alignment.  */
  doubleword_aligned_p = FUNCTION_ARG_BOUNDARY (mode, type) > BITS_PER_WORD;

  if (mips_abi != ABI_EABI && MUST_PASS_IN_STACK (mode, type))
    /* This argument must be passed on the stack.  Eat up all the
       remaining registers.  */
    info->reg_offset = MAX_ARGS_IN_REGISTERS;
  else
    {
      /* Set REG_OFFSET to the register count we're interested in.
         The EABI allocates the floating-point registers separately,
         but the other ABIs allocate them like integer registers.  */
      info->reg_offset = (mips_abi == ABI_EABI && info->fpr_p
                          ? cum->num_fprs
                          : cum->num_gprs);

      /* Advance to an even register if the argument is doubleword-aligned.  */
      if (doubleword_aligned_p)
        info->reg_offset += info->reg_offset & 1;
    }

  /* The alignment applied to registers is also applied to stack arguments.  */
  info->stack_offset = cum->stack_words;
  if (doubleword_aligned_p)
    info->stack_offset += info->stack_offset & 1;

  if (mode == BLKmode)
    info->num_bytes = int_size_in_bytes (type);
  else
    info->num_bytes = GET_MODE_SIZE (mode);

  /* Work out the size of the argument.  */
  num_bytes = type ? int_size_in_bytes (type) : GET_MODE_SIZE (mode);
  num_words = (num_bytes + UNITS_PER_WORD - 1) / UNITS_PER_WORD;
  max_regs = MAX_ARGS_IN_REGISTERS - info->reg_offset;

  /* Partition the argument between registers and stack.  */
  info->reg_words = MIN (num_words, max_regs);
  info->stack_words = num_words - info->reg_words;
}


/* Implement FUNCTION_ARG_ADVANCE.  */

void
function_arg_advance (CUMULATIVE_ARGS *cum, enum machine_mode mode,
                      tree type, int named)
{
  struct mips_arg_info info;

  mips_arg_info (cum, mode, type, named, &info);

  if (!info.fpr_p)
    cum->gp_reg_found = true;

  /* See the comment above the cumulative args structure in mips.h
     for an explanation of what this code does.  It assumes the O32
     ABI, which passes at most 2 arguments in float registers.  */
  if (cum->arg_number < 2 && info.fpr_p)
    cum->fp_code += (mode == SFmode ? 1 : 2) << (cum->arg_number * 2);

  if (mips_abi != ABI_EABI || !info.fpr_p)
    cum->num_gprs = info.reg_offset + info.reg_words;
  else if (info.reg_words > 0)
    cum->num_fprs += FP_INC;

  if (info.stack_words > 0)
    cum->stack_words = info.stack_offset + info.stack_words;

  cum->arg_number++;
}

/* Implement FUNCTION_ARG.  */

struct rtx_def *
function_arg (const CUMULATIVE_ARGS *cum, enum machine_mode mode,
              tree type, int named)
{
  struct mips_arg_info info;

  /* We will be called with a mode of VOIDmode after the last argument
     has been seen.  Whatever we return will be passed to the call
     insn.  If we need a mips16 fp_code, return a REG with the code
     stored as the mode.  */
  if (mode == VOIDmode)
    {
      if ((TARGET_MIPS16
           || (!TARGET_MIPS16 && mips_base_mips16 && mips16_hard_float))
          && cum->fp_code != 0)
        return gen_rtx_REG ((enum machine_mode) cum->fp_code, 0);

      else
        return 0;
    }

  mips_arg_info (cum, mode, type, named, &info);

  /* Return straight away if the whole argument is passed on the stack.  */
  if (info.reg_offset == MAX_ARGS_IN_REGISTERS)
    return 0;

  if (type != 0
      && TREE_CODE (type) == RECORD_TYPE
      && (mips_abi == ABI_N32 || mips_abi == ABI_64)
      && TYPE_SIZE_UNIT (type)
      && host_integerp (TYPE_SIZE_UNIT (type), 1)
      && named)
    {
      /* The Irix 6 n32/n64 ABIs say that if any 64 bit chunk of the
         structure contains a double in its entirety, then that 64 bit
         chunk is passed in a floating point register.  */
      tree field;

      /* First check to see if there is any such field.  */
      for (field = TYPE_FIELDS (type); field; field = TREE_CHAIN (field))
        if (TREE_CODE (field) == FIELD_DECL
            && TREE_CODE (TREE_TYPE (field)) == REAL_TYPE
            && TYPE_PRECISION (TREE_TYPE (field)) == BITS_PER_WORD
            && host_integerp (bit_position (field), 0)
            && int_bit_position (field) % BITS_PER_WORD == 0)
          break;

      if (field != 0)
        {
          /* Now handle the special case by returning a PARALLEL
             indicating where each 64 bit chunk goes.  INFO.REG_WORDS
             chunks are passed in registers.  */
          unsigned int i;
          HOST_WIDE_INT bitpos;
          rtx ret;

          /* assign_parms checks the mode of ENTRY_PARM, so we must
             use the actual mode here.  */
          ret = gen_rtx_PARALLEL (mode, rtvec_alloc (info.reg_words));

          bitpos = 0;
          field = TYPE_FIELDS (type);
          for (i = 0; i < info.reg_words; i++)
            {
              rtx reg;

              for (; field; field = TREE_CHAIN (field))
                if (TREE_CODE (field) == FIELD_DECL
                    && int_bit_position (field) >= bitpos)
                  break;

              if (field
                  && int_bit_position (field) == bitpos
                  && TREE_CODE (TREE_TYPE (field)) == REAL_TYPE
                  && !TARGET_SOFT_FLOAT
                  && TYPE_PRECISION (TREE_TYPE (field)) == BITS_PER_WORD)
                reg = gen_rtx_REG (DFmode, FP_ARG_FIRST + info.reg_offset + i);
              else
                reg = gen_rtx_REG (DImode, GP_ARG_FIRST + info.reg_offset + i);

              XVECEXP (ret, 0, i)
                = gen_rtx_EXPR_LIST (VOIDmode, reg,
                                     GEN_INT (bitpos / BITS_PER_UNIT));

              bitpos += BITS_PER_WORD;
            }
          return ret;
        }
    }

  if (!info.fpr_p || TARGET_MIPS16)
    return gen_rtx_REG (mode, GP_ARG_FIRST + info.reg_offset);
  else if (info.reg_offset == 1 && mips_abi == ABI_32)
    /* This code handles the special o32 case in which the second word
       of the argument structure is passed in floating-point registers.  */
    return gen_rtx_REG (mode, FP_ARG_FIRST + 2);
  else
    return gen_rtx_REG (mode, FP_ARG_FIRST + info.reg_offset);
}


/* Implement FUNCTION_ARG_PARTIAL_NREGS.  */

int
function_arg_partial_nregs (const CUMULATIVE_ARGS *cum,
                            enum machine_mode mode, tree type, int named)
{
  struct mips_arg_info info;

  mips_arg_info (cum, mode, type, named, &info);
  return info.stack_words > 0 ? info.reg_words : 0;
}


/* Return true if FUNCTION_ARG_PADDING (MODE, TYPE) should return
   upward rather than downward.  In other words, return true if the
   first byte of the stack slot has useful data, false if the last
   byte does.  */

bool
mips_pad_arg_upward (enum machine_mode mode, tree type)
{
  /* On little-endian targets, the first byte of every stack argument
     is passed in the first byte of the stack slot.  */
  if (!BYTES_BIG_ENDIAN)
    return true;

  /* Otherwise, integral types are padded downward: the last byte of a
     stack argument is passed in the last byte of the stack slot.  */
  if (type != 0
      ? INTEGRAL_TYPE_P (type) || POINTER_TYPE_P (type)
      : GET_MODE_CLASS (mode) == MODE_INT)
    return false;

  /* Big-endian o64 pads floating-point arguments downward.  */
  if (mips_abi == ABI_O64)
    if (type != 0 ? FLOAT_TYPE_P (type) : GET_MODE_CLASS (mode) == MODE_FLOAT)
      return false;

  /* Other types are padded upward for o32, o64, n32 and n64.  */
  if (mips_abi != ABI_EABI)
    return true;

  /* Arguments smaller than a stack slot are padded downward.  */
  if (mode != BLKmode)
    return (GET_MODE_BITSIZE (mode) >= PARM_BOUNDARY);
  else
    return (int_size_in_bytes (type) >= (PARM_BOUNDARY / BITS_PER_UNIT));
}


/* Likewise BLOCK_REG_PADDING (MODE, TYPE, ...).  Return !BYTES_BIG_ENDIAN
   if the least significant byte of the register has useful data.  Return
   the opposite if the most significant byte does.  */

bool
mips_pad_reg_upward (enum machine_mode mode, tree type)
{
  /* No shifting is required for floating-point arguments.  */
  if (type != 0 ? FLOAT_TYPE_P (type) : GET_MODE_CLASS (mode) == MODE_FLOAT)
    return !BYTES_BIG_ENDIAN;

  /* Otherwise, apply the same padding to register arguments as we do
     to stack arguments.  */
  return mips_pad_arg_upward (mode, type);
}

int
mips_setup_incoming_varargs (const CUMULATIVE_ARGS *cum,
                             enum machine_mode mode, tree type, int no_rtl)
{
  CUMULATIVE_ARGS local_cum;
  int gp_saved, fp_saved;

  /* The caller has advanced CUM up to, but not beyond, the last named
     argument.  Advance a local copy of CUM past the last "real" named
     argument, to find out how many registers are left over.  */

  local_cum = *cum;
  FUNCTION_ARG_ADVANCE (local_cum, mode, type, 1);

  /* Found out how many registers we need to save.  */
  gp_saved = MAX_ARGS_IN_REGISTERS - local_cum.num_gprs;
  fp_saved = (EABI_FLOAT_VARARGS_P
              ? MAX_ARGS_IN_REGISTERS - local_cum.num_fprs
              : 0);

  if (!no_rtl)
    {
      if (gp_saved > 0)
        {
          rtx ptr, mem;

          ptr = virtual_incoming_args_rtx;
          switch (mips_abi)
            {
            case ABI_32:
            case ABI_O64:
              ptr = plus_constant (ptr, local_cum.num_gprs * UNITS_PER_WORD);
              break;

            case ABI_EABI:
              ptr = plus_constant (ptr, -gp_saved * UNITS_PER_WORD);
              break;
            }
          mem = gen_rtx_MEM (BLKmode, ptr);
          set_mem_alias_set (mem, get_varargs_alias_set ());

          move_block_from_reg (local_cum.num_gprs + GP_ARG_FIRST,
                               mem, gp_saved);
        }
      if (fp_saved > 0)
        {
          /* We can't use move_block_from_reg, because it will use
             the wrong mode.  */
          enum machine_mode mode;
          int off, i;

          /* Set OFF to the offset from virtual_incoming_args_rtx of
             the first float register.   The FP save area lies below
             the integer one, and is aligned to UNITS_PER_FPVALUE bytes.  */
          off = -gp_saved * UNITS_PER_WORD;
          off &= ~(UNITS_PER_FPVALUE - 1);
          off -= fp_saved * UNITS_PER_FPREG;

          mode = TARGET_SINGLE_FLOAT ? SFmode : DFmode;

          for (i = local_cum.num_fprs; i < MAX_ARGS_IN_REGISTERS; i += FP_INC)
            {
              rtx ptr, mem;

              ptr = plus_constant (virtual_incoming_args_rtx, off);
              mem = gen_rtx_MEM (mode, ptr);
              set_mem_alias_set (mem, get_varargs_alias_set ());
              emit_move_insn (mem, gen_rtx_REG (mode, FP_ARG_FIRST + i));
              off += UNITS_PER_HWFPVALUE;
            }
        }
    }
  if (mips_abi == ABI_32 || mips_abi == ABI_O64)
    /* No need for pretend arguments: the register parameter area was
       allocated by the caller.  */
    return 0;
  return (gp_saved * UNITS_PER_WORD) + (fp_saved * UNITS_PER_FPREG);
}

/* Create the va_list data type.
   We keep 3 pointers, and two offsets.
   Two pointers are to the overflow area, which starts at the CFA.
     One of these is constant, for addressing into the GPR save area below it.
     The other is advanced up the stack through the overflow region.
   The third pointer is to the GPR save area.  Since the FPR save area
     is just below it, we can address FPR slots off this pointer.
   We also keep two one-byte offsets, which are to be subtracted from the
     constant pointers to yield addresses in the GPR and FPR save areas.
     These are downcounted as float or non-float arguments are used,
     and when they get to zero, the argument must be obtained from the
     overflow region.
   If !EABI_FLOAT_VARARGS_P, then no FPR save area exists, and a single
     pointer is enough.  It's started at the GPR save area, and is
     advanced, period.
   Note that the GPR save area is not constant size, due to optimization
     in the prologue.  Hence, we can't use a design with two pointers
     and two offsets, although we could have designed this with two pointers
     and three offsets.  */

static tree
mips_build_builtin_va_list (void)
{
  if (EABI_FLOAT_VARARGS_P)
    {
      tree f_ovfl, f_gtop, f_ftop, f_goff, f_foff, f_res, record;
      tree array, index;

      record = (*lang_hooks.types.make_type) (RECORD_TYPE);

      f_ovfl = build_decl (FIELD_DECL, get_identifier ("__overflow_argptr"),
                          ptr_type_node);
      f_gtop = build_decl (FIELD_DECL, get_identifier ("__gpr_top"),
                          ptr_type_node);
      f_ftop = build_decl (FIELD_DECL, get_identifier ("__fpr_top"),
                          ptr_type_node);
      f_goff = build_decl (FIELD_DECL, get_identifier ("__gpr_offset"),
                          unsigned_char_type_node);
      f_foff = build_decl (FIELD_DECL, get_identifier ("__fpr_offset"),
                          unsigned_char_type_node);
      /* Explicitly pad to the size of a pointer, so that -Wpadded won't
         warn on every user file.  */
      index = build_int_2 (GET_MODE_SIZE (ptr_mode) - 2 - 1, 0);
      array = build_array_type (unsigned_char_type_node,
                                build_index_type (index));
      f_res = build_decl (FIELD_DECL, get_identifier ("__reserved"), array);

      DECL_FIELD_CONTEXT (f_ovfl) = record;
      DECL_FIELD_CONTEXT (f_gtop) = record;
      DECL_FIELD_CONTEXT (f_ftop) = record;
      DECL_FIELD_CONTEXT (f_goff) = record;
      DECL_FIELD_CONTEXT (f_foff) = record;
      DECL_FIELD_CONTEXT (f_res) = record;

      TYPE_FIELDS (record) = f_ovfl;
      TREE_CHAIN (f_ovfl) = f_gtop;
      TREE_CHAIN (f_gtop) = f_ftop;
      TREE_CHAIN (f_ftop) = f_goff;
      TREE_CHAIN (f_goff) = f_foff;
      TREE_CHAIN (f_foff) = f_res;

      layout_type (record);
      return record;
    }
  else if (TARGET_IRIX && !TARGET_IRIX5)
    /* On IRIX 6, this type is 'char *'.  */
    return build_pointer_type (char_type_node);
  else
    /* Otherwise, we use 'void *'.  */
    return ptr_type_node;
}

/* Implement va_start.  */

void
mips_va_start (tree valist, rtx nextarg)
{
  const CUMULATIVE_ARGS *cum = &current_function_args_info;

  /* ARG_POINTER_REGNUM is initialized to STACK_POINTER_BOUNDARY, but
     since the stack is aligned for a pair of argument-passing slots,
     and the beginning of a variable argument list may be an odd slot,
     we have to decrease its alignment.  */
  if (cfun && cfun->emit->regno_pointer_align)
    while (((current_function_pretend_args_size * BITS_PER_UNIT)
            & (REGNO_POINTER_ALIGN (ARG_POINTER_REGNUM) - 1)) != 0)
      REGNO_POINTER_ALIGN (ARG_POINTER_REGNUM) /= 2;

  if (mips_abi == ABI_EABI)
    {
      int gpr_save_area_size;

      gpr_save_area_size
        = (MAX_ARGS_IN_REGISTERS - cum->num_gprs) * UNITS_PER_WORD;

      if (EABI_FLOAT_VARARGS_P)
        {
          tree f_ovfl, f_gtop, f_ftop, f_goff, f_foff;
          tree ovfl, gtop, ftop, goff, foff;
          tree t;
          int fpr_offset;
          int fpr_save_area_size;

          f_ovfl = TYPE_FIELDS (va_list_type_node);
          f_gtop = TREE_CHAIN (f_ovfl);
          f_ftop = TREE_CHAIN (f_gtop);
          f_goff = TREE_CHAIN (f_ftop);
          f_foff = TREE_CHAIN (f_goff);

          ovfl = build (COMPONENT_REF, TREE_TYPE (f_ovfl), valist, f_ovfl);
          gtop = build (COMPONENT_REF, TREE_TYPE (f_gtop), valist, f_gtop);
          ftop = build (COMPONENT_REF, TREE_TYPE (f_ftop), valist, f_ftop);
          goff = build (COMPONENT_REF, TREE_TYPE (f_goff), valist, f_goff);
          foff = build (COMPONENT_REF, TREE_TYPE (f_foff), valist, f_foff);

          /* Emit code to initialize OVFL, which points to the next varargs
             stack argument.  CUM->STACK_WORDS gives the number of stack
             words used by named arguments.  */
          t = make_tree (TREE_TYPE (ovfl), virtual_incoming_args_rtx);
          if (cum->stack_words > 0)
            t = build (PLUS_EXPR, TREE_TYPE (ovfl), t,
                       build_int_2 (cum->stack_words * UNITS_PER_WORD, 0));
          t = build (MODIFY_EXPR, TREE_TYPE (ovfl), ovfl, t);
          expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);

          /* Emit code to initialize GTOP, the top of the GPR save area.  */
          t = make_tree (TREE_TYPE (gtop), virtual_incoming_args_rtx);
          t = build (MODIFY_EXPR, TREE_TYPE (gtop), gtop, t);
          expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);

          /* Emit code to initialize FTOP, the top of the FPR save area.
             This address is gpr_save_area_bytes below GTOP, rounded
             down to the next fp-aligned boundary.  */
          t = make_tree (TREE_TYPE (ftop), virtual_incoming_args_rtx);
          fpr_offset = gpr_save_area_size + UNITS_PER_FPVALUE - 1;
          fpr_offset &= ~(UNITS_PER_FPVALUE - 1);
          if (fpr_offset)
            t = build (PLUS_EXPR, TREE_TYPE (ftop), t,
                       build_int_2 (-fpr_offset, -1));
          t = build (MODIFY_EXPR, TREE_TYPE (ftop), ftop, t);
          expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);

          /* Emit code to initialize GOFF, the offset from GTOP of the
             next GPR argument.  */
          t = build (MODIFY_EXPR, TREE_TYPE (goff), goff,
                     build_int_2 (gpr_save_area_size, 0));
          expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);

          /* Likewise emit code to initialize FOFF, the offset from FTOP
             of the next FPR argument.  */
          fpr_save_area_size
            = (MAX_ARGS_IN_REGISTERS - cum->num_fprs) * UNITS_PER_FPREG;
          t = build (MODIFY_EXPR, TREE_TYPE (foff), foff,
                     build_int_2 (fpr_save_area_size, 0));
          expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);
        }
      else
        {
          /* Everything is in the GPR save area, or in the overflow
             area which is contiguous with it.  */
          nextarg = plus_constant (nextarg, -gpr_save_area_size);
          std_expand_builtin_va_start (valist, nextarg);
        }
    }
  else
    std_expand_builtin_va_start (valist, nextarg);
}

/* Implement va_arg.  */

rtx
mips_va_arg (tree valist, tree type)
{
  HOST_WIDE_INT size, rsize;
  rtx addr_rtx;
  tree t;

  size = int_size_in_bytes (type);
  rsize = (size + UNITS_PER_WORD - 1) & -UNITS_PER_WORD;

  if (mips_abi == ABI_EABI)
    {
      bool indirect;
      rtx r;

      indirect
        = function_arg_pass_by_reference (NULL, TYPE_MODE (type), type, 0);

      if (indirect)
        {
          size = POINTER_SIZE / BITS_PER_UNIT;
          rsize = UNITS_PER_WORD;
        }

      addr_rtx = gen_reg_rtx (Pmode);

      if (!EABI_FLOAT_VARARGS_P)
        {
          /* Case of all args in a merged stack.  No need to check bounds,
             just advance valist along the stack.  */

          tree gpr = valist;
          if (!indirect
              && !TARGET_64BIT
              && TYPE_ALIGN (type) > (unsigned) BITS_PER_WORD)
            {
              /* Align the pointer using: ap = (ap + align - 1) & -align,
                 where align is 2 * UNITS_PER_WORD.  */
              t = build (PLUS_EXPR, TREE_TYPE (gpr), gpr,
                         build_int_2 (2 * UNITS_PER_WORD - 1, 0));
              t = build (BIT_AND_EXPR, TREE_TYPE (t), t,
                         build_int_2 (-2 * UNITS_PER_WORD, -1));
              t = build (MODIFY_EXPR, TREE_TYPE (gpr), gpr, t);
              expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);
            }

          /* Emit code to set addr_rtx to the valist, and postincrement
             the valist by the size of the argument, rounded up to the
             next word.  */
          t = build (POSTINCREMENT_EXPR, TREE_TYPE (gpr), gpr,
                     size_int (rsize));
          r = expand_expr (t, addr_rtx, Pmode, EXPAND_NORMAL);
          if (r != addr_rtx)
            emit_move_insn (addr_rtx, r);

          /* Flush the POSTINCREMENT.  */
          emit_queue();
        }
      else
        {
          /* Not a simple merged stack.  */

          tree f_ovfl, f_gtop, f_ftop, f_goff, f_foff;
          tree ovfl, top, off;
          rtx lab_over = NULL_RTX, lab_false;
          HOST_WIDE_INT osize;

          f_ovfl = TYPE_FIELDS (va_list_type_node);
          f_gtop = TREE_CHAIN (f_ovfl);
          f_ftop = TREE_CHAIN (f_gtop);
          f_goff = TREE_CHAIN (f_ftop);
          f_foff = TREE_CHAIN (f_goff);

          /* We maintain separate pointers and offsets for floating-point
             and integer arguments, but we need similar code in both cases.
             Let:

                 TOP be the top of the register save area;
                 OFF be the offset from TOP of the next register;
                 ADDR_RTX be the address of the argument;
                 RSIZE be the number of bytes used to store the argument
                   when it's in the register save area;
                 OSIZE be the number of bytes used to store it when it's
                   in the stack overflow area; and
                 PADDING be (BYTES_BIG_ENDIAN ? OSIZE - RSIZE : 0)

             The code we want is:

                  1: off &= -rsize;       // round down
                  2: if (off != 0)
                  3:   {
                  4:     addr_rtx = top - off;
                  5:     off -= rsize;
                  6:   }
                  7: else
                  8:   {
                  9:     ovfl += ((intptr_t) ovfl + osize - 1) & -osize;
                 10:     addr_rtx = ovfl + PADDING;
                 11:     ovfl += osize;
                 14:   }

             [1] and [9] can sometimes be optimized away.  */

          lab_false = gen_label_rtx ();
          lab_over = gen_label_rtx ();

          ovfl = build (COMPONENT_REF, TREE_TYPE (f_ovfl), valist, f_ovfl);
          if (GET_MODE_CLASS (TYPE_MODE (type)) == MODE_FLOAT
              && GET_MODE_SIZE (TYPE_MODE (type)) <= UNITS_PER_FPVALUE)
            {
              top = build (COMPONENT_REF, TREE_TYPE (f_ftop), valist, f_ftop);
              off = build (COMPONENT_REF, TREE_TYPE (f_foff), valist, f_foff);

              /* When floating-point registers are saved to the stack,
                 each one will take up UNITS_PER_HWFPVALUE bytes, regardless
                 of the float's precision.  */
              rsize = UNITS_PER_HWFPVALUE;
            }
          else
            {
              top = build (COMPONENT_REF, TREE_TYPE (f_gtop), valist, f_gtop);
              off = build (COMPONENT_REF, TREE_TYPE (f_goff), valist, f_goff);
              if (rsize > UNITS_PER_WORD)
                {
                  /* [1] Emit code for: off &= -rsize.  */
                  t = build (BIT_AND_EXPR, TREE_TYPE (off), off,
                             build_int_2 (-rsize, -1));
                  t = build (MODIFY_EXPR, TREE_TYPE (off), off, t);
                  expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);
                }
            }
          /* Every overflow argument must take up at least UNITS_PER_WORD
             bytes (= PARM_BOUNDARY bits).  RSIZE can sometimes be smaller
             than that, such as in the combination -mgp64 -msingle-float
             -fshort-double.  Doubles passed in registers will then take
             up UNITS_PER_HWFPVALUE bytes, but those passed on the stack
             take up UNITS_PER_WORD bytes.  */
          osize = MAX (rsize, UNITS_PER_WORD);

          /* [2] Emit code to branch if off == 0.  */
          r = expand_expr (off, NULL_RTX, TYPE_MODE (TREE_TYPE (off)),
                           EXPAND_NORMAL);
          emit_cmp_and_jump_insns (r, const0_rtx, EQ, const1_rtx, GET_MODE (r),
                                   1, lab_false);

          /* [4] Emit code for: addr_rtx = top - off.  */
          t = build (MINUS_EXPR, TREE_TYPE (top), top, off);
          r = expand_expr (t, addr_rtx, Pmode, EXPAND_NORMAL);
          if (r != addr_rtx)
            emit_move_insn (addr_rtx, r);

          /* [5] Emit code for: off -= rsize.  */
          t = build (MINUS_EXPR, TREE_TYPE (off), off, build_int_2 (rsize, 0));
          t = build (MODIFY_EXPR, TREE_TYPE (off), off, t);
          expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);

          /* [7] Emit code to jump over the else clause, then the label
             that starts it.  */
          emit_queue();
          emit_jump (lab_over);
          emit_barrier ();
          emit_label (lab_false);

          if (osize > UNITS_PER_WORD)
            {
              /* [9] Emit: ovfl += ((intptr_t) ovfl + osize - 1) & -osize.  */
              t = build (PLUS_EXPR, TREE_TYPE (ovfl), ovfl,
                         build_int_2 (osize - 1, 0));
              t = build (BIT_AND_EXPR, TREE_TYPE (ovfl), t,
                         build_int_2 (-osize, -1));
              t = build (MODIFY_EXPR, TREE_TYPE (ovfl), ovfl, t);
              expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);
            }

          /* [10, 11].  Emit code to store ovfl in addr_rtx, then
             post-increment ovfl by osize.  On big-endian machines,
             the argument has OSIZE - RSIZE bytes of leading padding.  */
          t = build (POSTINCREMENT_EXPR, TREE_TYPE (ovfl), ovfl,
                     size_int (osize));
          if (BYTES_BIG_ENDIAN && osize > rsize)
            t = build (PLUS_EXPR, TREE_TYPE (t), t,
                       build_int_2 (osize - rsize, 0));
          r = expand_expr (t, addr_rtx, Pmode, EXPAND_NORMAL);
          if (r != addr_rtx)
            emit_move_insn (addr_rtx, r);

          emit_queue();
          emit_label (lab_over);
        }
      if (BYTES_BIG_ENDIAN && rsize != size)
        addr_rtx = plus_constant (addr_rtx, rsize - size);
      if (indirect)
        {
          addr_rtx = force_reg (Pmode, addr_rtx);
          r = gen_rtx_MEM (Pmode, addr_rtx);
          set_mem_alias_set (r, get_varargs_alias_set ());
          emit_move_insn (addr_rtx, r);
        }
      return addr_rtx;
    }
  else
    {
      /* Not EABI.  */
      int align;
      HOST_WIDE_INT min_offset;

      /* ??? The original va-mips.h did always align, despite the fact
         that alignments <= UNITS_PER_WORD are preserved by the va_arg
         increment mechanism.  */

      if ((mips_abi == ABI_N32 || mips_abi == ABI_64)
          && TYPE_ALIGN (type) > 64)
        align = 16;
      else if (TARGET_64BIT)
        align = 8;
      else if (TYPE_ALIGN (type) > 32)
        align = 8;
      else
        align = 4;

      t = build (PLUS_EXPR, TREE_TYPE (valist), valist,
                 build_int_2 (align - 1, 0));
      t = build (BIT_AND_EXPR, TREE_TYPE (t), t, build_int_2 (-align, -1));

      /* If arguments of type TYPE must be passed on the stack,
         set MIN_OFFSET to the offset of the first stack parameter.  */
      if (!MUST_PASS_IN_STACK (TYPE_MODE (type), type))
        min_offset = 0;
      else if (TARGET_NEWABI)
        min_offset = current_function_pretend_args_size;
      else
        min_offset = REG_PARM_STACK_SPACE (current_function_decl);

      /* Make sure the new address is at least MIN_OFFSET bytes from
         the incoming argument pointer.  */
      if (min_offset > 0)
        t = build (MAX_EXPR, TREE_TYPE (valist), t,
                   make_tree (TREE_TYPE (valist),
                              plus_constant (virtual_incoming_args_rtx,
                                             min_offset)));

      t = build (MODIFY_EXPR, TREE_TYPE (valist), valist, t);
      expand_expr (t, const0_rtx, VOIDmode, EXPAND_NORMAL);

      /* Everything past the alignment is standard.  */
      return std_expand_builtin_va_arg (valist, type);
    }
}

/* Return true if it is possible to use left/right accesses for a
   bitfield of WIDTH bits starting BITPOS bits into *OP.  When
   returning true, update *OP, *LEFT and *RIGHT as follows:

   *OP is a BLKmode reference to the whole field.

   *LEFT is a QImode reference to the first byte if big endian or
   the last byte if little endian.  This address can be used in the
   left-side instructions (lwl, swl, ldl, sdl).

   *RIGHT is a QImode reference to the opposite end of the field and
   can be used in the parterning right-side instruction.  */

static bool
mips_get_unaligned_mem (rtx *op, unsigned int width, int bitpos,
                        rtx *left, rtx *right)
{
  rtx first, last;

  /* Check that the operand really is a MEM.  Not all the extv and
     extzv predicates are checked.  */
  if (GET_CODE (*op) != MEM)
    return false;

  /* Check that the size is valid.  */
  if (width != 32 && (!TARGET_64BIT || width != 64))
    return false;

  /* We can only access byte-aligned values.  Since we are always passed
     a reference to the first byte of the field, it is not necessary to
     do anything with BITPOS after this check.  */
  if (bitpos % BITS_PER_UNIT != 0)
    return false;

  /* Reject aligned bitfields: we want to use a normal load or store
     instead of a left/right pair.  */
  if (MEM_ALIGN (*op) >= width)
    return false;

  /* Adjust *OP to refer to the whole field.  This also has the effect
     of legitimizing *OP's address for BLKmode, possibly simplifying it.  */
  *op = adjust_address (*op, BLKmode, 0);
  set_mem_size (*op, GEN_INT (width / BITS_PER_UNIT));

  /* Get references to both ends of the field.  We deliberately don't
     use the original QImode *OP for FIRST since the new BLKmode one
     might have a simpler address.  */
  first = adjust_address (*op, QImode, 0);
  last = adjust_address (*op, QImode, width / BITS_PER_UNIT - 1);

  /* Allocate to LEFT and RIGHT according to endiannes.  LEFT should
     be the upper word and RIGHT the lower word.  */
  if (TARGET_BIG_ENDIAN)
    *left = first, *right = last;
  else
    *left = last, *right = first;

  return true;
}


/* Try to emit the equivalent of (set DEST (zero_extract SRC WIDTH BITPOS)).
   Return true on success.  We only handle cases where zero_extract is
   equivalent to sign_extract.  */

bool
mips_expand_unaligned_load (rtx dest, rtx src, unsigned int width, int bitpos)
{
  rtx left, right, temp;

  /* If TARGET_64BIT, the destination of a 32-bit load will be a
     paradoxical word_mode subreg.  This is the only case in which
     we allow the destination to be larger than the source.  */
  if (GET_CODE (dest) == SUBREG
      && GET_MODE (dest) == DImode
      && SUBREG_BYTE (dest) == 0
      && GET_MODE (SUBREG_REG (dest)) == SImode)
    dest = SUBREG_REG (dest);

  /* After the above adjustment, the destination must be the same
     width as the source.  */
  if (GET_MODE_BITSIZE (GET_MODE (dest)) != width)
    return false;

  if (!mips_get_unaligned_mem (&src, width, bitpos, &left, &right))
    return false;

  temp = gen_reg_rtx (GET_MODE (dest));
  if (GET_MODE (dest) == DImode)
    {
      emit_insn (gen_mov_ldl (temp, src, left));
      emit_insn (gen_mov_ldr (dest, copy_rtx (src), right, temp));
    }
  else
    {
      emit_insn (gen_mov_lwl (temp, src, left));
      emit_insn (gen_mov_lwr (dest, copy_rtx (src), right, temp));
    }
  return true;
}


/* Try to expand (set (zero_extract DEST WIDTH BITPOS) SRC).  Return
   true on success.  */

bool
mips_expand_unaligned_store (rtx dest, rtx src, unsigned int width, int bitpos)
{
  rtx left, right;

  if (!mips_get_unaligned_mem (&dest, width, bitpos, &left, &right))
    return false;

  src = gen_lowpart (mode_for_size (width, MODE_INT, 0), src);

  if (GET_MODE (src) == DImode)
    {
      emit_insn (gen_mov_sdl (dest, src, left));
      emit_insn (gen_mov_sdr (copy_rtx (dest), copy_rtx (src), right));
    }
  else
    {
      emit_insn (gen_mov_swl (dest, src, left));
      emit_insn (gen_mov_swr (copy_rtx (dest), copy_rtx (src), right));
    }
  return true;
}

/* Set up globals to generate code for the ISA or processor
   described by INFO.  */

static void
mips_set_architecture (const struct mips_cpu_info *info)
{
  if (info != 0)
    {
      mips_arch_info = info;
      mips_arch = info->cpu;
      mips_isa = info->isa;
    }
}


/* Likewise for tuning.  */

static void
mips_set_tune (const struct mips_cpu_info *info)
{
  if (info != 0)
    {
      mips_tune_info = info;
      mips_tune = info->cpu;
    }
}


static void
mips_init_tables (void)
{
  enum machine_mode mode;
  int regno;

  mips_char_to_class['d'] = TARGET_MIPS16 ? M16_REGS : GR_REGS;
  mips_char_to_class['e'] = M16_NA_REGS;
  mips_char_to_class['t'] = T_REG;
  mips_char_to_class['f'] = (TARGET_HARD_FLOAT ? FP_REGS : NO_REGS);
  mips_char_to_class['h'] = (TARGET_BIG_ENDIAN ? MD0_REG : MD1_REG);
  mips_char_to_class['l'] = (TARGET_BIG_ENDIAN ? MD1_REG : MD0_REG);
  mips_char_to_class['x'] = MD_REGS;
  mips_char_to_class['b'] = ALL_REGS;
  mips_char_to_class['c'] = (TARGET_ABICALLS ? PIC_FN_ADDR_REG :
                             TARGET_MIPS16 ? M16_NA_REGS :
                             GR_REGS);
  mips_char_to_class['e'] = LEA_REGS;
  mips_char_to_class['j'] = PIC_FN_ADDR_REG;
  mips_char_to_class['y'] = GR_REGS;
  mips_char_to_class['z'] = ST_REGS;
  mips_char_to_class['B'] = COP0_REGS;
  mips_char_to_class['C'] = COP2_REGS;
  mips_char_to_class['D'] = COP3_REGS;
  mips_char_to_class['a'] = (TARGET_MIPS_DSP32 ? MD_AND_DSP_ACC_REGS : NO_REGS);
  mips_char_to_class['v'] = SP_REG;

  /* Set up array giving whether a given register can hold a given mode.  */

  for (mode = VOIDmode;
       mode != MAX_MACHINE_MODE;
       mode = (enum machine_mode) ((int)mode + 1))
    {
      register int size              = GET_MODE_SIZE (mode);
      register enum mode_class class = GET_MODE_CLASS (mode);

      for (regno = 0; regno < FIRST_PSEUDO_REGISTER; regno++)
        {
          register int temp;

          if (mode == CCV2mode)
            {
              if (! ISA_HAS_8CC)
                temp = 0;
              else
                temp = (ST_REG_P (regno)) && ((regno - ST_REG_FIRST) % 2 == 0);
            }

          else if (mode == CCV4mode)
            {
              if (! ISA_HAS_8CC)
                temp = 0;
              else
                temp = (ST_REG_P (regno)) && ((regno - ST_REG_FIRST) % 4 == 0);
            }

          else if (mode == CCmode)
            {
              if (! ISA_HAS_8CC)
                temp = (regno == FPSW_REGNUM);
              else
                temp = (ST_REG_P (regno) || GP_REG_P (regno)
                        || FP_REG_P (regno));
            }

          else if (GP_REG_P (regno))
            temp = ((regno & 1) == 0 || size <= UNITS_PER_WORD);

          else if (FP_REG_P (regno))
            temp = (((regno % FP_INC) == 0)
                     /* MIPS32 allows SFmode in odd registers */
                    || (ISA_HAS_ODD_SF_REG && size <= 4))
                    && TARGET_HARD_FLOAT
                    && (((class == MODE_FLOAT || class == MODE_COMPLEX_FLOAT
                          || (class == MODE_VECTOR_FLOAT
                              && TARGET_PAIRED_SINGLE_FLOAT))
                         && size <= UNITS_PER_FPVALUE)
                        /* Allow integer modes that fit into a single
                           register.  We need to put integers into FPRs
                           when using instructions like cvt and trunc.  */
                        || (class == MODE_INT && size <= UNITS_PER_FPREG)
                        /* Allow TFmode for CCmode reloads.  */
                        || (ISA_HAS_8CC && mode == TFmode));

          else if (MD_REG_P (regno))
            temp = (class == MODE_INT
                    && (size <= UNITS_PER_WORD
                        || (regno == MD_REG_FIRST
                            && size == 2 * UNITS_PER_WORD)));

          else if (DSP_ACC_REG_P (regno))
            temp = (class == MODE_INT
                    && TARGET_MIPS_DSP32 && !TARGET_MIPS16
                    && (size <= UNITS_PER_WORD
                        || ((regno - DSP_ACC_REG_FIRST) % 2 == 0
                            && size == 2 * UNITS_PER_WORD)));

          else if (ALL_COP_REG_P (regno))
            temp = (class == MODE_INT
                    && !TARGET_MIPS16
                    && size <= UNITS_PER_WORD);
          else
            temp = 0;

          mips_hard_regno_mode_ok[(int)mode][regno] = temp;
        }
    }

  memset (mips_split_p, '\0', sizeof (mips_split_p));
  memset (mips_hi_relocs, '\0', sizeof (mips_hi_relocs));
  memset (mips_lo_relocs, '\0', sizeof (mips_lo_relocs));

  if (TARGET_EXPLICIT_RELOCS || mips_split_addresses)
    {
      mips_split_p[SYMBOL_GENERAL] = true;
      mips_hi_relocs[SYMBOL_GENERAL] = "%hi(";
      mips_lo_relocs[SYMBOL_GENERAL] = "%lo(";
      mips_split_p[SYMBOL_LONGCALL] = true;
      mips_hi_relocs[SYMBOL_LONGCALL] = "%hi(";
      mips_lo_relocs[SYMBOL_LONGCALL] = "%lo(";
    }

  if (TARGET_MIPS16)
    {
      /* The high part is provided by a pseudo copy of $gp.  */
      mips_split_p[SYMBOL_SMALL_DATA] = true;
      mips_lo_relocs[SYMBOL_SMALL_DATA] = "%gprel(";

      /* For this case, we need to split symbols.  */
      if (TARGET_NO_DATA_IN_CODE && !TARGET_GP_OPT)
        {
          mips_split_p[SYMBOL_CONSTANT_POOL] = true;
          mips_hi_relocs[SYMBOL_CONSTANT_POOL] = "%hi(";
          mips_lo_relocs[SYMBOL_CONSTANT_POOL] = "%lo(";

          mips_split_p[SYMBOL_GENERAL] = true;
          mips_hi_relocs[SYMBOL_GENERAL] = "%hi(";
          mips_lo_relocs[SYMBOL_GENERAL] = "%lo(";
        }

      if (flag_section_relative_addressing)
        {
          mips_split_p[SYMBOL_SECTION_RELATIVE] = true;
          mips_split_p[SYMBOL_SMALL_DATA] = false;
        }
    }

  if (TARGET_EXPLICIT_RELOCS)
    {
      /* Small data constants are kept whole until after reload,
         then lowered by mips_rewrite_small_data.  */
      mips_lo_relocs[SYMBOL_SMALL_DATA] = "%gp_rel(";

      mips_split_p[SYMBOL_GOT_LOCAL] = true;
      if (TARGET_NEWABI)
        {
          mips_lo_relocs[SYMBOL_GOTOFF_PAGE] = "%got_page(";
          mips_lo_relocs[SYMBOL_GOT_LOCAL] = "%got_ofst(";
        }
      else
        {
          mips_lo_relocs[SYMBOL_GOTOFF_PAGE] = "%got(";
          mips_lo_relocs[SYMBOL_GOT_LOCAL] = "%lo(";
        }

      if (TARGET_XGOT)
        {
          /* The HIGH and LO_SUM are matched by special .md patterns.  */
          mips_split_p[SYMBOL_GOT_GLOBAL] = true;

          mips_split_p[SYMBOL_GOTOFF_GLOBAL] = true;
          mips_hi_relocs[SYMBOL_GOTOFF_GLOBAL] = "%got_hi(";
          mips_lo_relocs[SYMBOL_GOTOFF_GLOBAL] = "%got_lo(";

          mips_split_p[SYMBOL_GOTOFF_CALL] = true;
          mips_hi_relocs[SYMBOL_GOTOFF_CALL] = "%call_hi(";
          mips_lo_relocs[SYMBOL_GOTOFF_CALL] = "%call_lo(";
        }
      else
        {
          if (TARGET_NEWABI)
            mips_lo_relocs[SYMBOL_GOTOFF_GLOBAL] = "%got_disp(";
          else
            mips_lo_relocs[SYMBOL_GOTOFF_GLOBAL] = "%got(";
          mips_lo_relocs[SYMBOL_GOTOFF_CALL] = "%call16(";
        }
    }

  if (TARGET_NEWABI)
    {
      mips_split_p[SYMBOL_GOTOFF_LOADGP] = true;
      mips_hi_relocs[SYMBOL_GOTOFF_LOADGP] = "%hi(%neg(%gp_rel(";
      mips_lo_relocs[SYMBOL_GOTOFF_LOADGP] = "%lo(%neg(%gp_rel(";
    }

}


static int mips_force_reinit = 1;


static const char *
mips_pch_valid_p  (const void *data_p, size_t len)
{
  mips_force_reinit = 1;
  return default_pch_valid_p (data_p, len);
}


static void
mips_set_mips16_mode (int mips16_p)
{
  static int first = 1;
  int was_mips16_p = !!TARGET_MIPS16;

  if (!mips_force_reinit && mips16_p == was_mips16_p)
    return;

  target_flags = mips_base_target_flags;
  if (mips16_p)
    {
      /* Select mips16 instruction set. */
      target_flags |= MASK_MIPS16 | mips_base_mips16;
      if (ISA_HAS_MIPS16E)
        /* Automatically use MIPS16e if available. */
        target_flags |= MASK_MIPS16E;
      /* When compiling for the mips16, we can not use floating point.  We
         have recorded the original hard float value in mips16_hard_float.  */
      target_flags |= MASK_SOFT_FLOAT;
      /* Silently disable -mexplicit-relocs since it doesn't apply
         to mips16 code.  Even so, it would overly pedantic to warn
         about "-mips16 -mexplicit-relocs", especially given that
         we use a %gprel() operator.  */
      target_flags &= ~MASK_EXPLICIT_RELOCS;
      mips_split_addresses = 0;
      /* By default, don't check for divide-by-zero in MIPS16 code
         when optimizing. */
      if (!(target_flags_explicit & MASK_NO_CHECK_ZERO_DIV)
          && optimize)
        target_flags |= MASK_NO_CHECK_ZERO_DIV;
      /* Pretend branches are cheap. */
      mips_branch_cost = optimize > 1 ? 1 : 0;
      /* Branch likely is not available in mips16. */
      mips_brlikely = BRLIKELY_NO;
      /* Don't run the scheduler before reload, since it tends to
         increase register pressure.  */
      flag_schedule_insns = 0;
      /* Use default default delayed_branch option. */
      flag_delayed_branch = mips_flag_delayed_branch;
      /* Force no alignment of mips16 code. */
      /* XXX would 32-bit alignment be an acceptable compromise? */
      align_loops = align_jumps = align_functions = 1;
    }
  else
    {
      /* Reset to select base non-mips16 ISA */
      target_flags &= ~(MASK_MIPS16 | MASK_MIPS16E);
      mips_branch_cost = mips_base_branch_cost;
      mips_brlikely = mips_base_brlikely;
      mips_split_addresses = mips_base_split_addresses;
      flag_schedule_insns = mips_base_schedule_insns;
      /* When using explicit relocs, we call dbr_schedule from within
         mips_reorg.  */
      if (TARGET_EXPLICIT_RELOCS)
        flag_delayed_branch = 0;
      else
        flag_delayed_branch = mips_flag_delayed_branch;
      if (mips_base_align_loops > 0) align_loops = mips_base_align_loops;
      if (mips_base_align_jumps > 0) align_jumps = mips_base_align_jumps;
      if (mips_base_align_functions > 0) align_functions = mips_base_align_functions;
    }

  align_loops_log = floor_log2 (align_loops * 2 - 1);
  align_jumps_log = floor_log2 (align_jumps * 2 - 1);
  align_functions_log = floor_log2 (align_functions * 2 - 1);

  /* HARD_FRAME_POINTER_REGNUM changes between MIPS16 and non-MIPS16. */
  if (hard_frame_pointer_rtx)
    REG_POINTER (hard_frame_pointer_rtx) = 0;
  hard_frame_pointer_rtx
    = gen_rtx_raw_REG (Pmode, HARD_FRAME_POINTER_REGNUM);

  /* (Re)initialise mips target internals for new ISA. */
  mips_init_tables ();

  if (!first)
    /* Reinitialise RTL backend. */
    backend_reinit ();

  mips_force_reinit = first = 0;
}


static void
mips_prepare_function_start (fndecl)
     tree fndecl;
{
  tree x;

  if (!fndecl)
    return;

  /* Get the DECL of the top-most enclosing function. */
  /* XXX do we still need to do this? */
  while ((x = decl_function_context (fndecl)))
    fndecl = x;

  mips_set_mips16_mode (SYMBOL_REF_MIPS16_P (XEXP (DECL_RTL (fndecl), 0)));

#ifdef TARGET_MCHP_PIC32MX
  /* If we're in MIPS16[e] mode for an interrupt function, that's an error */
  if (mchp_function_interrupt_p (fndecl) && TARGET_MIPS16)
    {
      warning ("interrupt handler functions cannot be mips16, "
               "assuming nomips16 attribute");
      mips_set_mips16_mode (0);
    }
#endif
}

/* Set up the threshold for data to go into the small data area, instead
   of the normal data area, and detect any conflicts in the switches.  */

void
override_options (void)
{
  int i, start;

#ifdef TARGET_MCHP_PIC32MX
  if (profile_flag == 1) 
    {
      warning ("-p not supported, ignored");
      profile_flag = 0;
    }

  /* PIC32MX cannot use Position Independent Code yet.  */
  if (flag_pie || flag_pic || TARGET_ABICALLS)
    {
      error ("%s is not supported on PIC32 targets",
               (flag_pic > 1 ? "-fPIC" :
                flag_pic ? "-fpic" :
                "-mabicalls"));
      target_flags &= ~MASK_ABICALLS;
      flag_pic = flag_pie = flag_shlib = 0;
    }

#endif

  /* Make sure that the user didn't turn off MIPS16 support when
     MIPS16E support is requested.  */
  if (TARGET_MIPS16E
      && (target_flags_explicit & MASK_MIPS16)
      && !TARGET_MIPS16)
    {
      warning ("-mips16e used with -mno-mips16");
      target_flags &= ~MASK_MIPS16E;
    }

  /* If TARGET_MIPS16E, enable MASK_MIPS16 */
  if (TARGET_MIPS16E)
    target_flags |= MASK_MIPS16;

  /* Save the command-line MIPS16 setting, then disable. It will be
     set again on a per-function basis by mips_set_mips16_mode(). */
  mips_base_mips16 = target_flags & (MASK_MIPS16 | MASK_MIPS16E);
  target_flags ^= mips_base_mips16;

  /* Make sure that the user didn't turn off paired single support when
     MIPS-3D support is requested.  */
  if (TARGET_MIPS3D && (target_flags_explicit & MASK_PAIRED_SINGLE)
      && !TARGET_PAIRED_SINGLE_FLOAT)
    warning ("-mips3d requires -mpaired-single");

  /* If TARGET_MIPS3D, enable MASK_PAIRED_SINGLE */
  if (TARGET_MIPS3D)
    target_flags |= MASK_PAIRED_SINGLE;

  mips_section_threshold = g_switch_set ?
                             g_switch_value :
                             MIPS_DEFAULT_GVALUE;
  /* Interpret -mabi.  */
  mips_abi = MIPS_ABI_DEFAULT;
  if (mips_abi_string != 0)
    {
      if (strcmp (mips_abi_string, "32") == 0)
        mips_abi = ABI_32;
      else if (strcmp (mips_abi_string, "o64") == 0)
        mips_abi = ABI_O64;
      else if (strcmp (mips_abi_string, "n32") == 0)
        mips_abi = ABI_N32;
      else if (strcmp (mips_abi_string, "64") == 0)
        mips_abi = ABI_64;
      else if (strcmp (mips_abi_string, "eabi") == 0)
        mips_abi = ABI_EABI;
      else
        fatal_error ("bad value (%s) for -mabi= switch", mips_abi_string);
    }

  /* The following code determines the architecture and register size.
     Similar code was added to GAS 2.14 (see tc-mips.c:md_after_parse_args()).
     The GAS and GCC code should be kept in sync as much as possible.  */

  /* If we're building for a Microchip PIC32MX target, the arch and tune
     stuff is easy since it's fixed and the -march/-mtune options are not
     available. */

#ifdef TARGET_MCHP_PIC32MX
    mips_set_architecture (mips_parse_cpu ("default CPU",
                                           MIPS_CPU_STRING_DEFAULT));
    mips_set_tune (mips_arch_info);
#else
  if (mips_arch_string != 0)
    mips_set_architecture (mips_parse_cpu ("-march", mips_arch_string));

  if (mips_isa_string != 0)
    {
      /* Handle -mipsN.  */
      char *whole_isa_str = concat ("mips", mips_isa_string, NULL);
      const struct mips_cpu_info *isa_info;

      isa_info = mips_parse_cpu ("-mips option", whole_isa_str);
      free (whole_isa_str);

      /* -march takes precedence over -mipsN, since it is more descriptive.
         There's no harm in specifying both as long as the ISA levels
         are the same.  */
      if (mips_arch_info != 0 && mips_isa != isa_info->isa)
	error ("-mips%s conflicts with the other architecture options, "
	       "which specify a %s processor",
	       mips_isa_string, mips_cpu_info_from_isa (mips_isa)->name);

      /* Set architecture based on the given option.  */
      mips_set_architecture (isa_info);
    }

  if (mips_arch_info == 0)
    {
      if (TARGET_MIPS_DSP32)
        /* -mdsp implies -mips32r2 */
        mips_set_architecture (mips_parse_cpu ("-mdsp option", "mips32r2"));
      else if (TARGET_SMARTMIPS)
        /* -msmartmips implies -mips32 */
        mips_set_architecture (mips_parse_cpu ("-msmartmips option", "mips32"));
      else if (mips_base_mips16 & MASK_MIPS16E)
        /* -mips16e implies -mips32r2 */
        mips_set_architecture (mips_parse_cpu ("-mips16e option", "mips32r2"));
    }

  if (mips_arch_info == 0)
    {
#ifdef MIPS_CPU_STRING_DEFAULT
      mips_set_architecture (mips_parse_cpu ("default CPU",
                                             MIPS_CPU_STRING_DEFAULT));
#else
      mips_set_architecture (mips_cpu_info_from_isa (MIPS_ISA_DEFAULT));
#endif
    }

#ifdef MIPS_ISA_CONTROLS_ABI
  mips_abi = MIPS_ISA_CONTROLS_ABI ();
#endif

  if (ABI_NEEDS_64BIT_REGS && !ISA_HAS_64BIT_REGS)
    error ("-march=%s is not compatible with the selected ABI",
           mips_arch_info->name);

  /* Optimize for mips_arch, unless -mtune selects a different processor.  */
  if (mips_tune_string != 0)
    mips_set_tune (mips_parse_cpu ("-mtune", mips_tune_string));

  if (mips_tune_info == 0)
    mips_set_tune (mips_arch_info);

#endif /* end #ifdef TARGET_MCHP_PIC32MX */

  if ((target_flags_explicit & MASK_64BIT) != 0)
    {
      /* The user specified the size of the integer registers.  Make sure
         it agrees with the ABI and ISA.  */
      if (TARGET_64BIT && !ISA_HAS_64BIT_REGS)
        error ("-mgp64 used with a 32-bit processor");
      else if (!TARGET_64BIT && ABI_NEEDS_64BIT_REGS)
        error ("-mgp32 used with a 64-bit ABI");
      else if (TARGET_64BIT && ABI_NEEDS_32BIT_REGS)
        error ("-mgp64 used with a 32-bit ABI");
    }
  else
    {
      /* Infer the integer register size from the ABI and processor.
         Restrict ourselves to 32-bit registers if that's all the
         processor has, or if the ABI cannot handle 64-bit registers.  */
      if (ABI_NEEDS_32BIT_REGS || !ISA_HAS_64BIT_REGS)
        target_flags &= ~MASK_64BIT;
      else
        target_flags |= MASK_64BIT;
    }

  if ((target_flags_explicit & MASK_FLOAT64) != 0)
    {
      /* Really, -mfp32 and -mfp64 are ornamental options.  There's
         only one right answer here.  */
      if (TARGET_64BIT && TARGET_DOUBLE_FLOAT && !TARGET_FLOAT64)
        error ("unsupported combination: %s", "-mgp64 -mfp32 -mdouble-float");
      else if (!TARGET_64BIT && TARGET_FLOAT64 && !ISA_HAS_MXHC1)
        error ("unsupported combination: %s", "-mgp32 -mfp64");
      else if (TARGET_SINGLE_FLOAT && TARGET_FLOAT64)
        error ("unsupported combination: %s", "-mfp64 -msingle-float");
    }
  else
    {
      /* -msingle-float selects 32-bit float registers.  Otherwise the
         float registers should be the same size as the integer ones.  */
      if (TARGET_64BIT && TARGET_DOUBLE_FLOAT)
        target_flags |= MASK_FLOAT64;
      else
        target_flags &= ~MASK_FLOAT64;
    }

  /* End of code shared with GAS.  */


  if ((target_flags_explicit & MASK_LONG64) == 0)
    {
      /* If no type size setting options (-mlong64,-mint64,-mlong32)
         were used, then set the type sizes.  In the EABI in 64 bit mode,
         longs and pointers are 64 bits.  Likewise for the SGI Irix6 N64
         ABI.  */
      if ((mips_abi == ABI_EABI && TARGET_64BIT) || mips_abi == ABI_64)
        target_flags |= MASK_LONG64;
      else
        target_flags &= ~MASK_LONG64;
    }

  if (MIPS_MARCH_CONTROLS_SOFT_FLOAT
      && (target_flags_explicit & MASK_SOFT_FLOAT) == 0)
    {
      /* For some configurations, it is useful to have -march control
         the default setting of MASK_SOFT_FLOAT.  */
      switch ((int) mips_arch)
        {
        case PROCESSOR_R4100:
        case PROCESSOR_R4111:
        case PROCESSOR_R4120:
          target_flags |= MASK_SOFT_FLOAT;
          break;

        default:
          target_flags &= ~MASK_SOFT_FLOAT;
          break;
        }
      if (mips_arch_info->options & MIPS_OPT_SOFTFLOAT)
        target_flags |= MASK_SOFT_FLOAT;
    }

  if (mips_abi != ABI_32 && mips_abi != ABI_O64)
    flag_pcc_struct_return = 0;

#if defined(USE_COLLECT2)
  /* For IRIX 5 or IRIX 6 with integrated O32 ABI support, USE_COLLECT2 is
     always defined when GNU as is not in use, but collect2 is only used
     for the O32 ABI, so override the toplev.c and target-def.h defaults
     for flag_gnu_linker, TARGET_ASM_{CONSTRUCTOR, DESTRUCTOR} and
     TARGET_HAVE_CTORS_DTORS.

     Since the IRIX 5 and IRIX 6 O32 assemblers cannot handle named
     sections, constructor/destructor handling depends on the ABI in use.

     Since USE_COLLECT2 is defined, we only need to restore the non-collect2
     defaults for the N32/N64 ABIs.  */
  if (TARGET_IRIX && !TARGET_SGI_O32_AS)
    {
      targetm.have_ctors_dtors = true;
      targetm.asm_out.constructor = default_named_section_asm_out_constructor;
      targetm.asm_out.destructor = default_named_section_asm_out_destructor;
    }
#endif

  /* Handle some quirks of the IRIX 5 and IRIX 6 O32 assemblers.  */

  if (TARGET_SGI_O32_AS)
    {
      /* They don't recognize `.[248]byte'.  */
      targetm.asm_out.unaligned_op.hi = "\t.align 0\n\t.half\t";
      targetm.asm_out.unaligned_op.si = "\t.align 0\n\t.word\t";
      /* The IRIX 6 O32 assembler gives an error for `align 0; .dword',
         contrary to the documentation, so disable it.  */
      targetm.asm_out.unaligned_op.di = NULL;

      /* They cannot handle named sections.  */
      targetm.have_named_sections = false;
      /* Therefore, EH_FRAME_SECTION_NAME isn't defined and we must use
         collect2.  */
      targetm.terminate_dw2_eh_frame_info = true;
      targetm.asm_out.eh_frame_section = collect2_eh_frame_section;

      /* They cannot handle debug information.  */
      if (write_symbols != NO_DEBUG)
        {
          /* Adapt wording to IRIX version: IRIX 5 only had a single ABI,
             so -mabi=32 isn't usually specified.  */
          if (TARGET_IRIX5)
            inform ("-g is only supported using GNU as,");
          else
            inform ("-g is only supported using GNU as with -mabi=32,");
          inform ("-g option disabled");
          write_symbols = NO_DEBUG;
        }
    }

  if (! mips_brlikely_string)
    {
      /* If neither -mbranch-likely nor -mno-branch-likely was given
         on the command line, set mips_brlikely based on the target
         architecture and ISA.

         By default, we enable use of Branch Likely instructions on
         all architectures which support them except for MIPS32 and MIPS64
         (i.e., the generic MIPS32 and MIPS64 ISAs, and processors which
         implement them).

         The MIPS32 and MIPS64 architecture specifications say "Software
         is strongly encouraged to avoid use of Branch Likely
         instructions, as they will be removed from a future revision
         of the [MIPS32 and MIPS64] architecture."  Therefore, we do not
         issue those instructions unless instructed to do so by
         -mbranch-likely.  */
      if (TARGET_SR71K)
        mips_brlikely = BRLIKELY_NO;
      else if (mips_tune_info->options & MIPS_OPT_PREDICTLIKELY)
        /* only use branch likely only if predicted "very likely" on 20Kc */
        mips_brlikely = BRLIKELY_PREDICT;
      else if (mips_tune_info->options & MIPS_OPT_BRANCHLIKELY)
        /* branch likely has no penalty for these pipelines */
        mips_brlikely = BRLIKELY_YES;
      else if (ISA_HAS_BRANCHLIKELY
               && !(ISA_MIPS32 || ISA_MIPS32R2 || ISA_MIPS64 || ISA_MIPS64R2))
        mips_brlikely = BRLIKELY_YES;
      else
        mips_brlikely = BRLIKELY_NO;
    }
  else
    {
      if (*mips_brlikely_string == '\0')
        mips_brlikely = BRLIKELY_YES;
      else if (strcmp (mips_brlikely_string, "=yes") == 0)
        mips_brlikely = BRLIKELY_YES;
      else if (strcmp (mips_brlikely_string, "=no") == 0)
        mips_brlikely = BRLIKELY_NO;
      else if (strcmp (mips_brlikely_string, "=predict") == 0)
        mips_brlikely = BRLIKELY_PREDICT;
      else
        error ("Invalid option `-mbranch-likely%s'",
               mips_brlikely_string);
    }

  if (mips_brlikely != BRLIKELY_NO && !ISA_HAS_BRANCHLIKELY)
    warning ("generation of Branch Likely instructions enabled, but not supported by architecture");

  if (mips_no_data_in_code_string)
    {
      if (mips_no_data_in_code_string[0] != '\0')
        error ("Invalid option `-mno-data-in-code%s'",
                mips_no_data_in_code_string);

      if (TARGET_EMBEDDED_DATA)
        {
          warning ("-mno-data-in-code requires -mno-embedded-data");
          target_flags &= ~MASK_EMBEDDED_DATA;
        }
    }

  if (mips_code_xonly_string)
    {
      if (mips_code_xonly_string[0] != '\0')
        error ("Invalid option `-mcode-xonly%s'", mips_code_xonly_string);
    }

  if (mips_flip_mips16_string)
    {
      if (mips_flip_mips16_string[0] != '\0')
        error ("Invalid option `-mflip-mips16%s'", mips_flip_mips16_string);
    }

  if (! mips_branch_cost_string)
    {
      /* Branch is very expensive on super-scalar cpus, and on most of
	 these we can use conditional move.  Branch is also more
	 expensive on the super- or long-pipelined cpus (r4000,
	 r6000), but branch cost 2 seems to generate best code for
	 simple short pipeline, single-issue cpus. */
      switch (mips_tune)
	{
	case PROCESSOR_R4000:
	case PROCESSOR_R6000:
	case PROCESSOR_5KC:
	  mips_branch_cost = 3;
	  break;
	case PROCESSOR_20KC:
	case PROCESSOR_R5400:
	case PROCESSOR_R7000:
	case PROCESSOR_R9000:
	  mips_branch_cost = 4;
	  break;
	case PROCESSOR_74K:
	case PROCESSOR_74KX:
	case PROCESSOR_74KZ:
	  /* Although the 74k has a fairly long pipe and is dual-issued.
	     It ALU & AGEN are relatively short overall and a cost of 2 seem
	     to generate good code.  */
	  mips_branch_cost = 2;
	  break;
	default:
	  mips_branch_cost = 2;
	  break;
	}
    }
  else
    {
      mips_branch_cost = atoi (mips_branch_cost_string);
      if (mips_branch_cost < 0 || mips_branch_cost > 10)
        error ("-mbranch-cost=%s is invalid or out of range",
               mips_branch_cost_string);
    }

  /* The effect of -mabicalls isn't defined for the EABI.  */
  if (mips_abi == ABI_EABI && TARGET_ABICALLS)
    {
      error ("unsupported combination: %s", "-mabicalls -mabi=eabi");
      target_flags &= ~MASK_ABICALLS;
    }

  /* Switch on ABICALLS mode if -fpic or -fpie were used, and the
     user hasn't explicitly disabled these modes. */
  if ((flag_pic || flag_pie) && ! TARGET_EMBEDDED_PIC && ! TARGET_ABICALLS
      && ! ((target_flags_explicit & MASK_ABICALLS))
      && mips_abi != ABI_EABI)
    target_flags |= MASK_ABICALLS;

  /* MIPS16 cannot generate PIC yet.  */
  if (mips_base_mips16 && (flag_pic || TARGET_ABICALLS))
    {
      warning ("%s is not supported with -mips16, ignored",
               (flag_pic > 1 ? "-fPIC" :
                flag_pic ? "-fpic" :
                "-mabicalls"));
      target_flags &= ~MASK_ABICALLS;
      flag_pic = flag_pie = flag_shlib = 0;
    }

  /* -fpic (-KPIC) is the default when TARGET_ABICALLS is defined.  We need
     to set flag_pic so that the LEGITIMATE_PIC_OPERAND_P macro will work.  */
  /* ??? -non_shared turns off pic code generation, but this is not
     implemented.  */
  if (TARGET_ABICALLS)
    {
      flag_pic = 1;
      if (mips_section_threshold > 0)
        {
          if (g_switch_set)
            {
              if (target_flags_explicit & MASK_ABICALLS)
                warning ("-G is incompatible with -mabicalls");
              else
                warning ("-G is incompatible with PIC code which is the default");
            }
          mips_section_threshold = 0;
        }
    }

  /* The MIPS and SGI o32 assemblers expect small-data variables to
     be declared before they are used.  Although we once had code to
     do this, it was very invasive and fragile.  It no longer seems
     worth the effort.  */
  if (!TARGET_EXPLICIT_RELOCS && !TARGET_GAS)
    mips_section_threshold = 0;

  /* We switch to small data sections using ".section", which the native
     o32 irix assemblers don't understand.  Disable -G accordingly.
     We must do this regardless of command-line options since otherwise
     the compiler would abort.  */
  if (!targetm.have_named_sections)
    mips_section_threshold = 0;

  /* -membedded-pic is a form of PIC code suitable for embedded
     systems.  All calls are made using PC relative addressing, and
     all data is addressed using the $gp register.  This requires gas,
     which does most of the work, and GNU ld, which automatically
     expands PC relative calls which are out of range into a longer
     instruction sequence.  All gcc really does differently is
     generate a different sequence for a switch.  */
  if (TARGET_EMBEDDED_PIC)
    {
      flag_pic = 1;
      if (TARGET_ABICALLS)
        warning ("-membedded-pic and -mabicalls are incompatible");

      if (g_switch_set)
        warning ("-G and -membedded-pic are incompatible");

      /* Setting mips_section_threshold is not required, because gas
         will force everything to be GP addressable anyhow, but
         setting it will cause gcc to make better estimates of the
         number of instructions required to access a particular data
         item.  */
      mips_section_threshold = 0x7fffffff;
    }

  /* mips_split_addresses is a half-way house between explicit
     relocations and the traditional assembler macros.  It can
     split absolute 32-bit symbolic constants into a high/lo_sum
     pair but uses macros for other sorts of access.

     Like explicit relocation support for REL targets, it relies
     on GNU extensions in the assembler and the linker.

     Although this code should work for -O0, it has traditionally
     been treated as an optimization.  */
  if (TARGET_GAS && TARGET_SPLIT_ADDRESSES
      && optimize && !flag_pic
      && !ABI_HAS_64BIT_SYMBOLS)
    mips_split_addresses = 1;
  else
    mips_split_addresses = 0;

  /* -mexplicit-relocs doesn't yet support non-PIC n64.  We don't know
     how to generate %highest/%higher/%hi/%lo sequences.  */
  if (mips_abi == ABI_64 && !TARGET_ABICALLS)
    {
      if ((target_flags_explicit & target_flags & MASK_EXPLICIT_RELOCS) != 0)
        sorry ("non-PIC n64 with explicit relocations");
      target_flags &= ~MASK_EXPLICIT_RELOCS;
    }

  /* Explicit relocations for "old" ABIs are a GNU extension.  Unless
     the user has said otherwise, assume that they are not available
     with assemblers other than gas.  */
  if (!TARGET_NEWABI && !TARGET_GAS
      && (target_flags_explicit & MASK_EXPLICIT_RELOCS) == 0)
    target_flags &= ~MASK_EXPLICIT_RELOCS;

  /* Make -mabicalls -fno-unit-at-a-time imply -mno-explicit-relocs
     unless the user says otherwise.

     There are two problems here:

       (1) The value of an R_MIPS_GOT16 relocation depends on whether
           the symbol is local or global.  We therefore need to know
           a symbol's binding before refering to it using %got().

       (2) R_MIPS_CALL16 can only be applied to global symbols.

     When not using -funit-at-a-time, a symbol's binding may change
     after it has been used.  For example, the C++ front-end will
     initially assume that the typeinfo for an incomplete type will be
     comdat, on the basis that the type could be completed later in the
     file.  But if the type never is completed, the typeinfo will become
     local instead.  */
  if (!flag_unit_at_a_time
      && TARGET_ABICALLS
      && (target_flags_explicit & MASK_EXPLICIT_RELOCS) == 0)
    target_flags &= ~MASK_EXPLICIT_RELOCS;

  /* -mrnames says to use the MIPS software convention for register
     names instead of the hardware names (ie, $a0 instead of $4).
     We do this by switching the names in mips_reg_names, which the
     reg_names points into via the REGISTER_NAMES macro.  */

  if (TARGET_NAME_REGS)
    memcpy (mips_reg_names, mips_sw_reg_names, sizeof (mips_reg_names));

  /* The muldiv hi/lo reg numbers change based on endianness, so that
     when using them as a register pair we access the correct register
     when we get the correct sub word. */
  strcpy (mips_reg_names[HI_REGNUM], "hi");
  strcpy (mips_reg_names[LO_REGNUM], "lo");

  if (!WORDS_BIG_ENDIAN)
    {
      for (i = DSP_ACC_REG_FIRST; i <= DSP_ACC_REG_LAST; i += 2)
        {
          strcpy (&mips_reg_names[i][4], "lo");
          strcpy (&mips_reg_names[i+1][4], "hi");
        }
    }

  /* When compiling for the mips16, we can not use floating point.  We
     record the original hard float value in mips16_hard_float.  */
  mips16_hard_float = TARGET_HARD_FLOAT;

  /* When using explicit relocs, we call dbr_schedule from within
     mips_reorg.  */
  mips_flag_delayed_branch = flag_delayed_branch;
  if (TARGET_EXPLICIT_RELOCS)
      flag_delayed_branch = 0;

  /* Prefer a call to memcpy over inline code when optimizing for size,
     though see MOVE_RATIO in mips.h. */
  if (optimize_size && (target_flags_explicit & MASK_MEMCPY) == 0)
    target_flags |= MASK_MEMCPY;

#ifdef MIPS_TFMODE_FORMAT
  REAL_MODE_FORMAT (TFmode) = &MIPS_TFMODE_FORMAT;
#endif

  /* Make sure that when TARGET_MIPS3D or TARGET_PAIRED_SINGLE_FLOAT is true,
     TARGET_FLOAT64 and TARGET_HARD_FLOAT are both true */
  if ((TARGET_MIPS3D || TARGET_PAIRED_SINGLE_FLOAT)
      && ! (TARGET_FLOAT64 && TARGET_HARD_FLOAT))
    {
      warning ("-mips3d and -mpaired-single require 64-bit h/w floating point");
      target_flags &= ~(MASK_MIPS3D | MASK_PAIRED_SINGLE);
    }

  /* Enable DSP ASE by default if architecture supports it. */
  if ((mips_arch_info->options & MIPS_OPT_DSP)
      && (target_flags_explicit & MASK_DSP) == 0)
    target_flags |= MASK_DSP;

  /* Enable DSPR2 by default for the target is 74K or 74KX.  */
  if (TARGET_MIPS_DSP32 && TARGET_MIPS74K)
    target_flags |= MASK_DSPR2;

  /* Enable SmartMIPS ASE by default if architecture supports it. */ 
  if ((mips_arch_info->options & MIPS_OPT_SMARTMIPS)
      && (target_flags_explicit & MASK_SMARTMIPS) == 0)
    target_flags |= MASK_SMARTMIPS;

  /* Warn if -mdsp used without -mips64r3 or -mips32r2. */
  if (TARGET_MIPS_DSP64 && !ISA_MIPS64R2)
    warning ("-mdsp requires -mips64r2");
  else if (TARGET_MIPS_DSP32 && !ISA_MIPS32R2)
    warning ("-mdsp requires -mips32r2");

  if (!flag_unit_at_a_time)
    /* For section-relative addressing we need full information about
       each variable, which requires unit-at-a-time mode. */
    flag_section_relative_addressing = 0;
  else if (flag_data_sections)
    /* Section-relative addressing is pointless if each variable is in
       its own section. */
    flag_section_relative_addressing = 0;

  if (flag_section_relative_addressing
      && !g_switch_set && TARGET_DATA_IN_CODE)
    /* When -fsection-relative-addressing is requested default to -G0;
       unless -mno-data-in-code also used, in which case use the
       MIPS_DEFAULT_GVALUE set above. */
    mips_section_threshold = 0;
      
  if (TARGET_NO_DATA_IN_CODE
      || mips_section_threshold != 0)
    /* When forced to use gp-relative addressing, disable
       section-relative addressing, since with additional small data
       sections there will be too many active section bases.  */
    flag_section_relative_addressing = 0;

  mips_print_operand_punct['?'] = 1;
  mips_print_operand_punct['#'] = 1;
  mips_print_operand_punct['/'] = 1;
  mips_print_operand_punct['&'] = 1;
  mips_print_operand_punct['!'] = 1;
  mips_print_operand_punct['*'] = 1;
  mips_print_operand_punct['@'] = 1;
  mips_print_operand_punct['.'] = 1;
  mips_print_operand_punct['('] = 1;
  mips_print_operand_punct[')'] = 1;
  mips_print_operand_punct['['] = 1;
  mips_print_operand_punct[']'] = 1;
  mips_print_operand_punct['<'] = 1;
  mips_print_operand_punct['>'] = 1;
  mips_print_operand_punct['{'] = 1;
  mips_print_operand_punct['}'] = 1;
  mips_print_operand_punct['^'] = 1;
  mips_print_operand_punct['$'] = 1;
  mips_print_operand_punct['+'] = 1;
  mips_print_operand_punct['~'] = 1;

  /* Set up array to map GCC register number to debug register number.
     Ignore the special purpose register numbers.  */

  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    mips_dbx_regno[i] = -1;

  start = GP_DBX_FIRST - GP_REG_FIRST;
  for (i = GP_REG_FIRST; i <= GP_REG_LAST; i++)
    mips_dbx_regno[i] = i + start;

  start = FP_DBX_FIRST - FP_REG_FIRST;
  for (i = FP_REG_FIRST; i <= FP_REG_LAST; i++)
    mips_dbx_regno[i] = i + start;

  /* Fixed big-endian ordering of hi and lo registers in debug data. */
  mips_dbx_regno[HI_REGNUM] = MD_DBX_FIRST + 0;
  mips_dbx_regno[LO_REGNUM] = MD_DBX_FIRST + 1;

  start = DSP_ACC_DBX_FIRST - DSP_ACC_REG_FIRST;
  for (i = DSP_ACC_REG_FIRST; i <= DSP_ACC_REG_LAST; i++)
    mips_dbx_regno[i] = i + start;

  /* Save GPR registers in word_mode sized hunks.  word_mode hasn't been
     initialized yet, so we can't use that here.  */
  gpr_mode = TARGET_64BIT ? DImode : SImode;

  /* Provide default values for align_* for 64-bit targets.  */
  if (TARGET_64BIT || (mips_tune_info->options & MIPS_OPT_FETCH64))
    {
      if (align_loops == 0)
        align_loops = 8;
      if (align_jumps == 0)
        align_jumps = 8;
      if (align_functions == 0)
        align_functions = 8;
    }

  /* Function to allocate machine-dependent function status.  */
  init_machine_status = &mips_init_machine_status;

  /* Create a unique alias set for GOT references.  */
  mips_got_alias_set = new_alias_set ();

  /* Save base state of options for 32-bit ISA. */
  mips_base_target_flags = target_flags;
  mips_base_branch_cost = mips_branch_cost;
  mips_base_brlikely = mips_brlikely;
  mips_base_split_addresses = mips_split_addresses;
  mips_base_schedule_insns = flag_schedule_insns;
  mips_base_align_loops = align_loops;
  mips_base_align_jumps = align_jumps;
  mips_base_align_functions = align_functions;

  /*
   *  On systems where we have a licence manager, call it
   */
#ifdef TARGET_MCHP_PIC32MX
#define PIC32_EXPIRED_LICENSE -243
#define PIC32_ACADEMIC_LICENSE -100
#ifdef __MINGW32__
#ifndef SKIP_LICENSE_MANAGER
  { char *path;
    char *exec;
    char *args[] = { 0, "-k", 0 };
    char *c;
    char *err_msg, *err_arg;
    int pid;
    int status;
    extern char **save_argv;

    pic32_license_valid = 0;
    /* pic32-lm.exe must reside in the same directory as cc1.exe */
    path = make_relative_prefix(save_argv[0], "/pic32mx/bin/gcc/pic32mx/3.4.4",
                                              "/bin");

    if (!path) fatal_error("Could not locate `%s`\n", save_argv[0]);
    exec = alloca(strlen(path)+sizeof("pic32-lm.exe") + 1);
    sprintf(exec, "%s/pic32-lm.exe", path);
    args[0] = exec;
    pid = pexecute(exec, args, "MPLAB C Compiler for PIC32", 0, &err_msg, &err_arg,
                   PEXECUTE_FIRST | PEXECUTE_LAST);
    pid = pwait(pid, &status, 0);
    if (pid < 0) {
      /* Set academic edition if the license manager isn't available. */
      /* The academic edition disables optimization options without an eval period. */
      pic32_license_valid=PIC32_ACADEMIC_LICENSE;
    }
    else {
       if (WIFEXITED(status) && (WEXITSTATUS(status) == 0)) {
         pic32_license_valid=1;
       } else if (WIFEXITED(status)) {
         pic32_license_valid=WEXITSTATUS(status) - 256;
       }
    }
  }
#endif /* SKIP_LICENSE_MANAGER */

  if (pic32_license_valid != 1) {
    static int message_displayed = 0;
    static int message_purchase_displayed = 0;
    char *invalid = (char*) "an invalid license";

    #define NULLIFY(X) \
    if ((X) && (message_displayed++ == 0)) {\
      warning("Compiler option ignored due to %s", invalid); \
      if (message_purchase_displayed++ == 0) { \
        warning ("Disable the option or visit http://www.microchip.com/c32 " \
                 "to purchase a full license."); \
      } \
    } \
    X

    if (pic32_license_valid == PIC32_EXPIRED_LICENSE) {
      invalid = "an expired evaluation period";
    } else if (pic32_license_valid == PIC32_ACADEMIC_LICENSE) {
      invalid = "academic-edition limitations";
      /* Suppress the purchase suggestion for the academic edition. */
      message_purchase_displayed++;
    }

    /* Disable -O2 optimizations */
    NULLIFY(flag_crossjumping) = 0;
    NULLIFY(flag_optimize_sibling_calls) = 0;
    NULLIFY(flag_cse_follow_jumps) = 0;
    NULLIFY(flag_cse_skip_blocks) = 0;
    NULLIFY(flag_gcse) = 0;
    NULLIFY(flag_expensive_optimizations) = 0;
    NULLIFY(flag_strength_reduce) = 0;
    NULLIFY(flag_rerun_cse_after_loop) = 0;
    NULLIFY(flag_rerun_loop_opt) = 0;
    NULLIFY(flag_caller_saves) = 0;
    NULLIFY(flag_force_mem) = 0;
    NULLIFY(flag_peephole2) = 0;
    NULLIFY(flag_schedule_insns) = 0;
    NULLIFY(flag_schedule_insns_after_reload) = 0;
    NULLIFY(flag_regmove) = 0;
    NULLIFY(flag_strict_aliasing) = 0;
    NULLIFY(flag_delete_null_pointer_checks) = 0;
    NULLIFY(flag_reorder_blocks) = 0;
    NULLIFY(flag_reorder_functions) = 0;
    NULLIFY(flag_unit_at_a_time) = 0;
    NULLIFY(flag_web) = 0;
    NULLIFY(flag_rename_registers) = 0;

    /* Disable -O3 optimizations */
    NULLIFY(flag_inline_functions) = 0;
    NULLIFY(flag_unswitch_loops) = 0;

    /* Disable -Os optimization(s) */
    /* flag_web and flag_inline_functions already disabled */

    /* Disable -mips16 and -mips16e */
    NULLIFY(mips_base_mips16) = 0;

    #undef NULLIFY
  }
#endif /* __MINGW32__ */
#undef PIC32_EXPIRED_LICENSE
#undef PIC32_ACADEMIC_LICENSE
#endif /* TARGET_MCHP_PIC32MX */

  /* Now select the mips16 or 32-bit instruction set, as requested. */
  mips_set_mips16_mode (mips_base_mips16 != 0);
}

/* Implement CONDITIONAL_REGISTER_USAGE.  */

void
mips_conditional_register_usage (void)
{
  if (!TARGET_HARD_FLOAT)
    {
      int regno;

      for (regno = FP_REG_FIRST; regno <= FP_REG_LAST; regno++)
        fixed_regs[regno] = call_used_regs[regno] = 1;
      for (regno = ST_REG_FIRST; regno <= ST_REG_LAST; regno++)
        fixed_regs[regno] = call_used_regs[regno] = 1;
    }
  else if (! ISA_HAS_8CC)
    {
      int regno;

      /* We only have a single condition code register.  We
         implement this by hiding all the condition code registers,
         and generating RTL that refers directly to ST_REG_FIRST.  */
      for (regno = ST_REG_FIRST; regno <= ST_REG_LAST; regno++)
        fixed_regs[regno] = call_used_regs[regno] = 1;
    }
  /* In mips16 mode, we permit the $t temporary registers to be used
     for reload.  We prohibit the unused $s registers, since they
     are caller saved, and saving them via a mips16 register would
     probably waste more time than just reloading the value.  */
  if (TARGET_MIPS16)
    {
      fixed_regs[8] = call_used_regs[8] = 1;
      fixed_regs[9] = call_used_regs[9] = 1;
      fixed_regs[10] = call_used_regs[10] = 1;
      fixed_regs[11] = call_used_regs[11] = 1;
      fixed_regs[12] = call_used_regs[12] = 1;
      fixed_regs[13] = call_used_regs[13] = 1;
      fixed_regs[14] = call_used_regs[14] = 1;
      fixed_regs[15] = call_used_regs[15] = 1;
      fixed_regs[18] = call_used_regs[18] = 1;
      fixed_regs[19] = call_used_regs[19] = 1;
      fixed_regs[20] = call_used_regs[20] = 1;
      fixed_regs[21] = call_used_regs[21] = 1;
      fixed_regs[22] = call_used_regs[22] = 1;
      fixed_regs[23] = call_used_regs[23] = 1;
      fixed_regs[30] = call_used_regs[30] = 1;
    }

  /* These DSP control register fields are global.  */
  if (TARGET_MIPS_DSP32)
    {
      global_regs[DSP_CONTROL_SC_REGNUM] = 1;
      global_regs[DSP_CONTROL_PO_REGNUM] = 1;
    }

  /*  The DSP registers are not available unless the DSP ASE is enabled. */
  if (!TARGET_MIPS_DSP32 || TARGET_MIPS16)
    {
      int regno;
      for (regno = DSP_ACC_REG_FIRST; regno <= DSP_ACC_REG_LAST; regno++)
        fixed_regs[regno] = call_used_regs[regno] = 1;
      for (regno = DSP_CONTROL_PO_REGNUM; regno <= DSP_CONTROL_EF_REGNUM; regno++)
        fixed_regs[regno] = call_used_regs[regno] = 1;
    }
  /* fp20-23 are now caller saved.  */
  if (mips_abi == ABI_64)
    {
      int regno;
      for (regno = FP_REG_FIRST + 20; regno < FP_REG_FIRST + 24; regno++)
        call_really_used_regs[regno] = call_used_regs[regno] = 1;
    }
  /* Odd registers from fp21 to fp31 are now caller saved.  */
  if (mips_abi == ABI_N32)
    {
      int regno;
      for (regno = FP_REG_FIRST + 21; regno <= FP_REG_FIRST + 31; regno+=2)
        call_really_used_regs[regno] = call_used_regs[regno] = 1;
    }
}

/* Allocate a chunk of memory for per-function machine-dependent data.  */
static struct machine_function *
mips_init_machine_status (void)
{
  return ((struct machine_function *)
          ggc_alloc_cleared (sizeof (struct machine_function)));
}

/* On the mips16, we want to allocate $24 (T_REG) before other
   registers for instructions for which it is possible.  This helps
   avoid shuffling registers around in order to set up for an xor,
   encouraging the compiler to use a cmp instead.  */

void
mips_order_regs_for_local_alloc (void)
{
  register int i;

  for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
    reg_alloc_order[i] = i;

  if (TARGET_MIPS16)
    {
      /* It really doesn't matter where we put register 0, since it is
         a fixed register anyhow.  */
      reg_alloc_order[0] = 24;
      reg_alloc_order[24] = 0;
    }
}


/* The MIPS debug format wants all automatic variables and arguments
   to be in terms of the virtual frame pointer (stack pointer before
   any adjustment in the function), while the MIPS 3.0 linker wants
   the frame pointer to be the stack pointer after the initial
   adjustment.  So, we do the adjustment here.  The arg pointer (which
   is eliminated) points to the virtual frame pointer, while the frame
   pointer (which may be eliminated) points to the stack pointer after
   the initial adjustments.  */

HOST_WIDE_INT
mips_debugger_offset (rtx addr, HOST_WIDE_INT offset)
{
  rtx offset2 = const0_rtx;
  rtx reg = eliminate_constant_term (addr, &offset2);

  if (offset == 0)
    offset = INTVAL (offset2);

  if (reg == stack_pointer_rtx || reg == frame_pointer_rtx
      || reg == hard_frame_pointer_rtx)
    {
      HOST_WIDE_INT frame_size = (!cfun->machine->frame.initialized)
                                  ? compute_frame_size (get_frame_size ())
                                  : cfun->machine->frame.total_size;

      /* MIPS16 frame is smaller */
      if (frame_pointer_needed && TARGET_MIPS16)
        frame_size -= cfun->machine->frame.args_size;

      offset = offset - frame_size;
    }

  /* sdbout_parms does not want this to crash for unrecognized cases.  */
#if 0
  else if (reg != arg_pointer_rtx)
    fatal_insn ("mips_debugger_offset called with non stack/frame/arg pointer",
                addr);
#endif

  return offset;
}

/* Implement the PRINT_OPERAND macro.  The MIPS-specific operand codes are:

   'X'  OP is CONST_INT, prints 32 bits in hexadecimal format = "0x%08x",
   'x'  OP is CONST_INT, prints 16 bits in hexadecimal format = "0x%04x",
   'h'  OP is HIGH, prints %hi(X),
   'd'  output integer constant in decimal,
   'z'  if the operand is 0, use $0 instead of normal operand.
   'D'  print second part of double-word register or memory operand.
   'L'  print low-order register of double-word register operand.
   'M'  print high-order register of double-word register operand.
   'C'  print part of opcode for a branch condition.
   'F'  print part of opcode for a floating-point branch condition.
   'N'  print part of opcode for a branch condition, inverted.
   'W'  print part of opcode for a floating-point branch condition, inverted.
   'S'  OP is CODE_LABEL, print with prefix of "LS" (for embedded switch).
   'B'  print 'z' for EQ, 'n' for NE
   'b'  print 'n' for EQ, 'z' for NE
   'T'  print 'f' for EQ, 't' for NE
   't'  print 't' for EQ, 'f' for NE
   'Z'  print register and a comma, but print nothing for $fcc0
   'R'  print the reloc associated with LO_SUM
   'Y'  print fcc register plus 1, also check if register number is even
   'y'  print fcc register, also check if register number is even
   'V'  print fcc register plus 2, also check if register number is modulo 4.
   'v'  print fcc register, also check if register number is modulo 4.
   'Q'  print fcc register.
   'q'  print dsp acc register $ac0 (original hi/lo), $ac1, $ac2, $ac3.

   The punctuation characters are:

   '('  Turn on .set noreorder
   ')'  Turn on .set reorder
   '['  Turn on .set noat
   ']'  Turn on .set at
   '<'  Turn on .set nomacro
   '>'  Turn on .set macro
   '{'  Turn on .set volatile (not GAS)
   '}'  Turn on .set novolatile (not GAS)
   '&'  Turn on .set noreorder if filling delay slots
   '*'  Turn on both .set noreorder and .set nomacro if filling delay slots
   '!'  Turn on .set nomacro if filling delay slots
   '#'  Print nop if in a .set noreorder section.
   '/'  Like '#', but does nothing within a delayed branch sequence
   '?'  Print 'l' if we are to use a branch likely instead of normal branch.
   '@'  Print the name of the assembler temporary register (at or $1).
   '.'  Print the name of the register with a hard-wired zero (zero or $0).
   '^'  Print the name of the pic call-through register (t9 or $25).
   '$'  Print the name of the stack pointer register (sp or $29).
   '+'  Print the name of the gp register (usually gp or $28).
   '~'  Output a branch alignment to LABEL_ALIGN(NULL).  */

void
print_operand (FILE *file, rtx op, int letter)
{
  register enum rtx_code code;

  if (PRINT_OPERAND_PUNCT_VALID_P (letter))
    {
      switch (letter)
        {
        case '?':
          if (mips_branch_likely)
            putc ('l', file);
          break;

        case '@':
          fputs (reg_names [GP_REG_FIRST + 1], file);
          break;

        case '^':
          fputs (reg_names [PIC_FUNCTION_ADDR_REGNUM], file);
          break;

        case '.':
          fputs (reg_names [GP_REG_FIRST + 0], file);
          break;

        case '$':
          fputs (reg_names[STACK_POINTER_REGNUM], file);
          break;

        case '+':
          fputs (reg_names[PIC_OFFSET_TABLE_REGNUM], file);
          break;

        case '&':
          if (final_sequence != 0 && set_noreorder++ == 0)
            fputs (".set\tnoreorder\n\t", file);
          break;

        case '*':
          if (final_sequence != 0)
            {
              if (set_noreorder++ == 0)
                fputs (".set\tnoreorder\n\t", file);

              if (set_nomacro++ == 0)
                fputs (".set\tnomacro\n\t", file);
            }
          break;

        case '!':
          if (final_sequence != 0 && set_nomacro++ == 0)
            fputs ("\n\t.set\tnomacro", file);
          break;

        case '#':
          if (set_noreorder != 0)
            fputs ("\n\tnop", file);
          break;

        case '/':
          /* Print an extra newline so that the delayed insn is separated
             from the following ones.  This looks neater and is consistent
             with non-nop delayed sequences.  */
          if (set_noreorder != 0 && final_sequence == 0)
            fputs ("\n\tnop\n", file);
          break;

        case '(':
          if (set_noreorder++ == 0)
            fputs (".set\tnoreorder\n\t", file);
          break;

        case ')':
          if (set_noreorder == 0)
            error ("internal error: %%) found without a %%( in assembler pattern");

          else if (--set_noreorder == 0)
            fputs ("\n\t.set\treorder", file);

          break;

        case '[':
          if (set_noat++ == 0)
            fputs (".set\tnoat\n\t", file);
          break;

        case ']':
          if (set_noat == 0)
            error ("internal error: %%] found without a %%[ in assembler pattern");
          else if (--set_noat == 0)
            fputs ("\n\t.set\tat", file);

          break;

        case '<':
          if (set_nomacro++ == 0)
            fputs (".set\tnomacro\n\t", file);
          break;

        case '>':
          if (set_nomacro == 0)
            error ("internal error: %%> found without a %%< in assembler pattern");
          else if (--set_nomacro == 0)
            fputs ("\n\t.set\tmacro", file);

          break;

        case '{':
          if (set_volatile++ == 0)
            fprintf (file, "%s.set\tvolatile\n\t", TARGET_MIPS_AS ? "" : "#");
          break;

        case '}':
          if (set_volatile == 0)
            error ("internal error: %%} found without a %%{ in assembler pattern");
          else if (--set_volatile == 0)
            fprintf (file, "\n\t%s.set\tnovolatile", (TARGET_MIPS_AS) ? "" : "#");

          break;

        case '~':
          {
            if (align_labels_log > 0)
              ASM_OUTPUT_ALIGN (file, align_labels_log);
          }
        break;

        default:
          error ("PRINT_OPERAND: unknown punctuation '%c'", letter);
          break;
        }

      return;
    }

  if (! op)
    {
      error ("PRINT_OPERAND null pointer");
      return;
    }

  code = GET_CODE (op);

  if (letter == 'C')
    switch (code)
      {
      case EQ:  fputs ("eq",  file); break;
      case NE:  fputs ("ne",  file); break;
      case GT:  fputs ("gt",  file); break;
      case GE:  fputs ("ge",  file); break;
      case LT:  fputs ("lt",  file); break;
      case LE:  fputs ("le",  file); break;
      case GTU: fputs ("gtu", file); break;
      case GEU: fputs ("geu", file); break;
      case LTU: fputs ("ltu", file); break;
      case LEU: fputs ("leu", file); break;
      default:
        fatal_insn ("PRINT_OPERAND, invalid insn for %%C", op);
      }

  else if (letter == 'N')
    switch (code)
      {
      case EQ:  fputs ("ne",  file); break;
      case NE:  fputs ("eq",  file); break;
      case GT:  fputs ("le",  file); break;
      case GE:  fputs ("lt",  file); break;
      case LT:  fputs ("ge",  file); break;
      case LE:  fputs ("gt",  file); break;
      case GTU: fputs ("leu", file); break;
      case GEU: fputs ("ltu", file); break;
      case LTU: fputs ("geu", file); break;
      case LEU: fputs ("gtu", file); break;
      default:
        fatal_insn ("PRINT_OPERAND, invalid insn for %%N", op);
      }

  else if (letter == 'F')
    switch (code)
      {
      case EQ: fputs ("c1f", file); break;
      case NE: fputs ("c1t", file); break;
      default:
        fatal_insn ("PRINT_OPERAND, invalid insn for %%F", op);
      }

  else if (letter == 'W')
    switch (code)
      {
      case EQ: fputs ("c1t", file); break;
      case NE: fputs ("c1f", file); break;
      default:
        fatal_insn ("PRINT_OPERAND, invalid insn for %%W", op);
      }

  else if (letter == 'h')
    {
      if (GET_CODE (op) == HIGH)
        op = XEXP (op, 0);

      print_operand_reloc (file, op, mips_hi_relocs);
    }

  else if (letter == 'R')
    print_operand_reloc (file, op, mips_lo_relocs);

  else if (letter == 'S')
    {
      char buffer[100];

      ASM_GENERATE_INTERNAL_LABEL (buffer, "LS", CODE_LABEL_NUMBER (op));
      assemble_name (file, buffer);
    }

  else if (letter == 'Z')
    {
      register int regnum;

      if (code != REG)
        abort ();

      regnum = REGNO (op);
      if (! ST_REG_P (regnum))
        abort ();

      if (regnum != ST_REG_FIRST)
        fprintf (file, "%s,", reg_names[regnum]);
    }

  else if (letter == 'Q')
    {
      register int regnum;

      if (code != REG)
        abort ();

      regnum = REGNO (op);
      if (! ST_REG_P (regnum))
        abort ();

      fprintf (file, "%s", reg_names[regnum]);
    }

  else if (letter == 'V')
    {
      register int regnum;

      if (code != REG)
        abort ();

      regnum = REGNO (op);
      if (! ST_REG_P (regnum)
          || (regnum - ST_REG_FIRST) % 4 != 0)
        abort ();

      fprintf (file, "%s", reg_names[regnum + 2]);
    }

  else if (letter == 'v')
    {
      register int regnum;

      if (code != REG)
        abort ();

      regnum = REGNO (op);
      if (! ST_REG_P (regnum)
          || (regnum - ST_REG_FIRST) % 4 != 0)
        abort ();

      fprintf (file, "%s", reg_names[regnum]);
    }

  else if (letter == 'Y')
    {
      register int regnum;

      if (code != REG)
        abort ();

      regnum = REGNO (op);
      if (! ST_REG_P (regnum)
          || (regnum - ST_REG_FIRST) % 2 != 0)
        abort ();

      fprintf (file, "%s", reg_names[regnum + 1]);
    }

  else if (letter == 'y')
    {
      register int regnum;

      if (code != REG)
        abort ();

      regnum = REGNO (op);
      if (! ST_REG_P (regnum)
          || (regnum - ST_REG_FIRST) % 2 != 0)
        abort ();

      fprintf (file, "%s", reg_names[regnum]);
    }

  else if (letter == 'q')
    {
      register int regnum;

      if (code != REG)
        abort ();

      regnum = REGNO (op);
      if (MD_REG_P (regnum))
        fprintf (file, "$ac0");
      else if (DSP_ACC_REG_P (regnum))
        fprintf (file, "$ac%d", (regnum + 2 - DSP_ACC_REG_FIRST) >> 1);
      else
        abort ();
    }

  else if (code == REG || code == SUBREG)
    {
      register int regnum;

      if (code == REG)
        regnum = REGNO (op);
      else
        regnum = true_regnum (op);

      if ((letter == 'M' && ! WORDS_BIG_ENDIAN)
          || (letter == 'L' && WORDS_BIG_ENDIAN)
          || letter == 'D')
        regnum++;

      fprintf (file, "%s", reg_names[regnum]);
    }

  else if (code == MEM)
    {
      if (letter == 'D')
        output_address (plus_constant (XEXP (op, 0), 4));
      else
        output_address (XEXP (op, 0));
    }

  else if (letter == 'x' && GET_CODE (op) == CONST_INT)
    fprintf (file, HOST_WIDE_INT_PRINT_HEX, 0xffff & INTVAL(op));

  else if (letter == 'X' && GET_CODE(op) == CONST_INT)
    fprintf (file, HOST_WIDE_INT_PRINT_HEX, INTVAL (op));

  else if (letter == 'd' && GET_CODE(op) == CONST_INT)
    fprintf (file, HOST_WIDE_INT_PRINT_DEC, (INTVAL(op)));

  else if (letter == 'z' && op == CONST0_RTX (GET_MODE (op)))
    fputs (reg_names[GP_REG_FIRST], file);

  else if (letter == 'd' || letter == 'x' || letter == 'X')
    output_operand_lossage ("invalid use of %%d, %%x, or %%X");

  else if (letter == 'B')
    fputs (code == EQ ? "z" : "n", file);
  else if (letter == 'b')
    fputs (code == EQ ? "n" : "z", file);
  else if (letter == 'T')
    fputs (code == EQ ? "f" : "t", file);
  else if (letter == 't')
    fputs (code == EQ ? "t" : "f", file);

  else if (CONST_GP_P (op))
    fputs (reg_names[GLOBAL_POINTER_REGNUM], file);

  else
    output_addr_const (file, op);
}


/* Print symbolic operand OP, which is part of a HIGH or LO_SUM.
   RELOCS is the array of relocations to use.  */

static void
print_operand_reloc (FILE *file, rtx op, const char **relocs)
{
  enum mips_symbol_type symbol_type;
  const char *p;
  rtx base;
  HOST_WIDE_INT offset;

  if (!mips_symbolic_constant_p (op, &symbol_type) || relocs[symbol_type] == 0)
    fatal_insn ("PRINT_OPERAND, invalid operand for relocation", op);

  /* If OP uses an UNSPEC address, we want to print the inner symbol.  */
  mips_split_const (op, &base, &offset);
  if (UNSPEC_ADDRESS_P (base))
    op = plus_constant (UNSPEC_ADDRESS (base), offset);

  fputs (relocs[symbol_type], file);
  output_addr_const (file, op);
  for (p = relocs[symbol_type]; *p != 0; p++)
    if (*p == '(')
      fputc (')', file);
}

/* Output address operand X to FILE.  */

void
print_operand_address (FILE *file, rtx x)
{
  struct mips_address_info addr;

  if (mips_classify_address (&addr, x, word_mode, true))
    switch (addr.type)
      {
      case ADDRESS_REG:
        print_operand (file, addr.offset, 0);
        fprintf (file, "(%s)", reg_names[REGNO (addr.reg)]);
        return;

      case ADDRESS_LO_SUM:
        print_operand (file, addr.offset, 'R');
        fprintf (file, "(%s)", reg_names[REGNO (addr.reg)]);
        return;

      case ADDRESS_CONST_INT:
      case ADDRESS_SYMBOLIC:
        output_addr_const (file, x);
        return;
      }
  abort ();
}

/* Target hook for assembling integer objects.  It appears that the Irix
   6 assembler can't handle 64-bit decimal integers, so avoid printing
   such an integer here.  */

static bool
mips_assemble_integer (rtx x, unsigned int size, int aligned_p)
{
  if ((TARGET_64BIT || TARGET_GAS) && size == 8 && aligned_p)
    {
      fputs ("\t.dword\t", asm_out_file);
      if (HOST_BITS_PER_WIDE_INT < 64 || GET_CODE (x) != CONST_INT)
        output_addr_const (asm_out_file, x);
      else
        print_operand (asm_out_file, x, 'X');
      fputc ('\n', asm_out_file);
      return true;
    }
  return default_assemble_integer (x, size, aligned_p);
}

/* When using assembler macros, keep track of all of small-data externs
   so that mips_file_end can emit the appropriate declarations for them.

   In most cases it would be safe (though pointless) to emit .externs
   for other symbols too.  One exception is when an object is within
   the -G limit but declared by the user to be in a section other
   than .sbss or .sdata.  */

int
mips_output_external (FILE *file ATTRIBUTE_UNUSED, tree decl, const char *name)
{
  register struct extern_list *p;

  if (!TARGET_EXPLICIT_RELOCS && mips_in_small_data_p (decl))
    {
      p = (struct extern_list *) ggc_alloc (sizeof (struct extern_list));
      p->next = extern_head;
      p->name = name;
      p->size = int_size_in_bytes (TREE_TYPE (decl));
      extern_head = p;
    }

  if (TARGET_IRIX && mips_abi == ABI_32 && TREE_CODE (decl) == FUNCTION_DECL)
    {
      p = (struct extern_list *) ggc_alloc (sizeof (struct extern_list));
      p->next = extern_head;
      p->name = name;
      p->size = -1;
      extern_head = p;
    }

  return 0;
}

#if TARGET_IRIX
void
irix_output_external_libcall (rtx fun)
{
  register struct extern_list *p;

  if (mips_abi == ABI_32)
    {
      p = (struct extern_list *) ggc_alloc (sizeof (struct extern_list));
      p->next = extern_head;
      p->name = XSTR (fun, 0);
      p->size = -1;
      extern_head = p;
    }
}
#endif

/* Emit a new filename to a stream.  If we are smuggling stabs, try to
   put out a MIPS ECOFF file and a stab.  */

void
mips_output_filename (FILE *stream, const char *name)
{
  char ltext_label_name[100];

  /* If we are emitting DWARF-2, let dwarf2out handle the ".file"
     directives.  */
  if (write_symbols == DWARF2_DEBUG)
    return;
  else if (mips_output_filename_first_time)
    {
      mips_output_filename_first_time = 0;
      SET_FILE_NUMBER ();
      current_function_file = name;
      ASM_OUTPUT_FILENAME (stream, num_source_filenames, name);
      /* This tells mips-tfile that stabs will follow.  */
      if (!TARGET_GAS && write_symbols == DBX_DEBUG)
        fprintf (stream, "\t#@stabs\n");
    }

  else if (write_symbols == DBX_DEBUG)
    {
      ASM_GENERATE_INTERNAL_LABEL (ltext_label_name, "Ltext", 0);
      fprintf (stream, "%s", ASM_STABS_OP);
      output_quoted_string (stream, name);
      fprintf (stream, ",%d,0,0,%s\n", N_SOL, &ltext_label_name[1]);
    }

  else if (name != current_function_file
           && strcmp (name, current_function_file) != 0)
    {
      SET_FILE_NUMBER ();
      current_function_file = name;
      ASM_OUTPUT_FILENAME (stream, num_source_filenames, name);
    }
}

/* Emit a linenumber.  For encapsulated stabs, we need to put out a stab
   as well as a .loc, since it is possible that MIPS ECOFF might not be
   able to represent the location for inlines that come from a different
   file.  */

void
mips_output_lineno (FILE *stream, int line)
{
  if (write_symbols == DBX_DEBUG)
    {
      ++sym_lineno;
      fprintf (stream, "%sLM%d:\n%s%d,0,%d,%sLM%d\n",
               LOCAL_LABEL_PREFIX, sym_lineno, ASM_STABN_OP, N_SLINE, line,
               LOCAL_LABEL_PREFIX, sym_lineno);
    }
  else
    {
      fprintf (stream, "\n\t.loc\t%d %d\n", num_source_filenames, line);
      LABEL_AFTER_LOC (stream);
    }
}

/* Output an ASCII string, in a space-saving way.  PREFIX is the string
   that should be written before the opening quote, such as "\t.ascii\t"
   for real string data or "\t# " for a comment.  */

void
mips_output_ascii (FILE *stream, const char *string_param, size_t len,
                   const char *prefix)
{
  size_t i;
  int cur_pos = 17;
  register const unsigned char *string =
    (const unsigned char *)string_param;

  fprintf (stream, "%s\"", prefix);
  for (i = 0; i < len; i++)
    {
      register int c = string[i];

      switch (c)
        {
        case '\"':
        case '\\':
          putc ('\\', stream);
          putc (c, stream);
          cur_pos += 2;
          break;

        case TARGET_NEWLINE:
          fputs ("\\n", stream);
          if (i+1 < len
              && (((c = string[i+1]) >= '\040' && c <= '~')
                  || c == TARGET_TAB))
            cur_pos = 32767;            /* break right here */
          else
            cur_pos += 2;
          break;

        case TARGET_TAB:
          fputs ("\\t", stream);
          cur_pos += 2;
          break;

        case TARGET_FF:
          fputs ("\\f", stream);
          cur_pos += 2;
          break;

        case TARGET_BS:
          fputs ("\\b", stream);
          cur_pos += 2;
          break;

        case TARGET_CR:
          fputs ("\\r", stream);
          cur_pos += 2;
          break;

        default:
          if (c >= ' ' && c < 0177)
            {
              putc (c, stream);
              cur_pos++;
            }
          else
            {
              fprintf (stream, "\\%03o", c);
              cur_pos += 4;
            }
        }

      if (cur_pos > 72 && i+1 < len)
        {
          cur_pos = 17;
          fprintf (stream, "\"\n%s\"", prefix);
        }
    }
  fprintf (stream, "\"\n");
}

/* Implement TARGET_ASM_FILE_START.  */

static void
mips_file_start (void)
{
  default_file_start ();

  /* Versions of the MIPS assembler before 2.20 generate errors if a branch
     inside of a .set noreorder section jumps to a label outside of the .set
     noreorder section.  Revision 2.20 just set nobopt silently rather than
     fixing the bug.  */

  if (TARGET_MIPS_AS && optimize && flag_delayed_branch)
    fprintf (asm_out_file, "\t.set\tnobopt\n");

  if (TARGET_GAS)
    {
#if defined(OBJECT_FORMAT_ELF) && !TARGET_IRIX
      /* Generate a special section to describe the ABI switches used to
         produce the resultant binary.  This used to be done by the assembler
         setting bits in the ELF header's flags field, but we have run out of
         bits.  GDB needs this information in order to be able to correctly
         debug these binaries.  See the function mips_gdbarch_init() in
         gdb/mips-tdep.c.  This is unnecessary for the IRIX 5/6 ABIs and
         causes unnecessary IRIX 6 ld warnings.  */
      const char * abi_string = NULL;

      switch (mips_abi)
        {
        case ABI_32:   abi_string = "abi32"; break;
        case ABI_N32:  abi_string = "abiN32"; break;
        case ABI_64:   abi_string = "abi64"; break;
        case ABI_O64:  abi_string = "abiO64"; break;
        case ABI_EABI: abi_string = TARGET_64BIT ? "eabi64" : "eabi32"; break;
        default:
          abort ();
        }
      /* Note - we use fprintf directly rather than called named_section()
         because in this way we can avoid creating an allocated section.  We
         do not want this section to take up any space in the running
         executable.  */
      fprintf (asm_out_file, "\t.section .mdebug.%s\n", abi_string);

      /* Restore the default section.  */
      fprintf (asm_out_file, "\t.previous\n");
#endif
    }

  /* Generate the pseudo ops that System V.4 wants.  */
#ifndef ABICALLS_ASM_OP
#define ABICALLS_ASM_OP "\t.abicalls"
#endif
  if (TARGET_ABICALLS)
    /* ??? but do not want this (or want pic0) if -non-shared? */
    fprintf (asm_out_file, "%s\n", ABICALLS_ASM_OP);

  if (flag_verbose_asm)
    fprintf (asm_out_file, "\n%s -G value = %d, Arch = %s, ISA = %d\n",
             ASM_COMMENT_START,
             mips_section_threshold, mips_arch_info->name, mips_isa);
}

#ifdef BSS_SECTION_ASM_OP
/* Implement ASM_OUTPUT_ALIGNED_BSS.  This differs from the default only
   in the use of sbss.  */

void
mips_output_aligned_bss (FILE *stream, tree decl, const char *name,
                         unsigned HOST_WIDE_INT size, int align)
{
  extern tree last_assemble_variable_decl;

  if (decl != NULL && DECL_SECTION_NAME (decl) != NULL_TREE)
    named_section (decl, NULL, 0);
  else if (mips_in_small_data_p (decl))
    named_section (0, ".sbss", 0);
  else
    bss_section ();
  ASM_OUTPUT_ALIGN (stream, floor_log2 (align / BITS_PER_UNIT));
  last_assemble_variable_decl = decl;
  ASM_DECLARE_OBJECT_NAME (stream, name, decl);
  ASM_OUTPUT_SKIP (stream, size != 0 ? size : 1);
}
#endif


/* Utility function to add an entry to the vector dispatch list */
static void
mchp_add_vector_dispatch_entry (const char *target_name, int vector_number)
{
  struct vector_dispatch_spec *dispatch_entry;

  /* add the vector to the list of dispatch functions to emit */
  dispatch_entry = ggc_alloc (sizeof (struct vector_dispatch_spec));
  dispatch_entry->next = vector_dispatch_list_head;
  dispatch_entry->target = target_name;
  dispatch_entry->vector_number = vector_number;
  vector_dispatch_list_head = dispatch_entry;
}



/* Implement TARGET_ASM_FILE_END.  When using assembler macros, emit
   .externs for any small-data variables that turned out to be external.  */

static void
mips_file_end (void)
{
  tree name_tree;
  struct extern_list *p;
  struct vector_dispatch_spec *dispatch_entry;

  fputs ("# MCHP vector dispatch table\n", asm_out_file);

  /* output the vector dispatch functions specified in this translation
     unit, if any */
  for (dispatch_entry = vector_dispatch_list_head ;
      dispatch_entry ;
      dispatch_entry = dispatch_entry->next)
    {
      fprintf (asm_out_file, "\t.section\t.vector_%d,\"ax\",@progbits\n",
          dispatch_entry->vector_number);
      fprintf (asm_out_file, "\t.align\t2\n");
      fprintf (asm_out_file, "\t.set\tnomips16\n");
      fprintf (asm_out_file, "\t.ent\t__vector_dispatch_%d\n",
          dispatch_entry->vector_number);
      fprintf (asm_out_file, "__vector_dispatch_%d:\n",
          dispatch_entry->vector_number);
      fprintf (asm_out_file, "\tj\t%s\n", dispatch_entry->target);
      fprintf (asm_out_file, "\t.end\t__vector_dispatch_%d\n",
          dispatch_entry->vector_number);
      fprintf (asm_out_file, "\t.size\t__vector_dispatch_%d, .-"
          "__vector_dispatch_%d\n",
          dispatch_entry->vector_number,
          dispatch_entry->vector_number);
    }

  fputs ("# MCHP configuration words\n", asm_out_file);

  mchp_output_configuration_words ();

  if (extern_head)
    {
      fputs ("\n", asm_out_file);

      for (p = extern_head; p != 0; p = p->next)
        {
          name_tree = get_identifier (p->name);

          /* Positively ensure only one .extern for any given symbol.  */
          if (!TREE_ASM_WRITTEN (name_tree)
              && TREE_SYMBOL_REFERENCED (name_tree))
            {
              TREE_ASM_WRITTEN (name_tree) = 1;
              /* In IRIX 5 or IRIX 6 for the O32 ABI, we must output a
                 `.global name .text' directive for every used but
                 undefined function.  If we don't, the linker may perform
                 an optimization (skipping over the insns that set $gp)
                 when it is unsafe.  */
              if (TARGET_IRIX && mips_abi == ABI_32 && p->size == -1)
                {
                  fputs ("\t.globl ", asm_out_file);
                  assemble_name (asm_out_file, p->name);
                  fputs (" .text\n", asm_out_file);
                }
              else
                {
                  fputs ("\t.extern\t", asm_out_file);
                  assemble_name (asm_out_file, p->name);
                  fprintf (asm_out_file, ", %d\n", p->size);
                }
            }
        }
    }

  if (flag_section_relative_addressing)
    mips_output_section_bases ();

}

/* Implement ASM_OUTPUT_ALIGNED_DECL_COMMON.  This is usually the same as
   the elfos.h version, but we also need to handle -muninit-const-in-rodata
   and the limitations of the SGI o32 assembler.  */

void
mips_output_aligned_decl_common (FILE *stream, tree decl, const char *name,
                                 unsigned HOST_WIDE_INT size,
                                 unsigned int align)
{
  /* If the target wants uninitialized const declarations in
     .rdata then don't put them in .comm.   */
  if (TARGET_EMBEDDED_DATA && TARGET_UNINIT_CONST_IN_RODATA
      && TREE_CODE (decl) == VAR_DECL && TREE_READONLY (decl)
      && (DECL_INITIAL (decl) == 0 || DECL_INITIAL (decl) == error_mark_node))
    {
      if (TREE_PUBLIC (decl) && DECL_NAME (decl))
        targetm.asm_out.globalize_label (stream, name);

      readonly_data_section ();
      ASM_OUTPUT_ALIGN (stream, floor_log2 (align / BITS_PER_UNIT));
      mips_declare_object (stream, name, "",
                           ":\n\t.space\t" HOST_WIDE_INT_PRINT_UNSIGNED "\n",
                           size);
    }
  else
    /* The SGI o32 assembler doesn't accept an alignment.  */
    mips_declare_common_object (stream, name, "\n\t.comm\t",
                                size, align, !TARGET_SGI_O32_AS);
}

/* Declare a common object of SIZE bytes using asm directive INIT_STRING.
   NAME is the name of the object and ALIGN is the required alignment
   in bytes.  TAKES_ALIGNMENT_P is true if the directive takes a third
   alignment argument.  */

void
mips_declare_common_object (FILE *stream, const char *name,
                            const char *init_string,
                            unsigned HOST_WIDE_INT size,
                            unsigned int align, bool takes_alignment_p)
{
  if (!takes_alignment_p)
    {
      size += (align / BITS_PER_UNIT) - 1;
      size -= size % (align / BITS_PER_UNIT);
      mips_declare_object (stream, name, init_string,
                           "," HOST_WIDE_INT_PRINT_UNSIGNED "\n", size);
    }
  else
    mips_declare_object (stream, name, init_string,
                         "," HOST_WIDE_INT_PRINT_UNSIGNED ",%u\n",
                         size, align / BITS_PER_UNIT);
}

/* Implement ASM_OUTPUT_ALIGNED_DECL_LOCAL.  */

void
mips_output_aligned_decl_local (FILE *stream, tree decl, const char *name,
                                unsigned HOST_WIDE_INT size,
                                unsigned int align)
{
#if defined(ASM_OUTPUT_ALIGNED_BSS) && defined(BSS_SECTION_ASM_OP)
  if (! TARGET_SGI_O32_AS)
    {
      mips_output_aligned_bss (stream, decl, name, size, align);
      return;
    }
#endif

  /* The SGI o32 assembler doesn't accept an alignment, so round up
     the size instead.  */
  /* XXX Could we use mips_output_aligned_bss for Irix too? */
  size += (align / BITS_PER_UNIT) - 1;
  size -= size % (align / BITS_PER_UNIT);
  mips_declare_object (stream, name, "\n\t.lcomm\t",
                       "," HOST_WIDE_INT_PRINT_UNSIGNED "\n", size);
}

/* Emit either a label, .comm, or .lcomm directive.  When using assembler
   macros, mark the symbol as written so that mips_file_end won't emit an
   .extern for it.  STREAM is the output file, NAME is the name of the
   symbol, INIT_STRING is the string that should be written before the
   symbol and FINAL_STRING is the string that shoulbe written after it.
   FINAL_STRING is a printf() format that consumes the remaining arguments.  */

void
mips_declare_object (FILE *stream, const char *name, const char *init_string,
                     const char *final_string, ...)
{
  va_list ap;

  fputs (init_string, stream);
  assemble_name (stream, name);
  va_start (ap, final_string);
  vfprintf (stream, final_string, ap);
  va_end (ap);

  if (!TARGET_EXPLICIT_RELOCS)
    {
      tree name_tree = get_identifier (name);
      TREE_ASM_WRITTEN (name_tree) = 1;
    }
}

#ifdef ASM_OUTPUT_SIZE_DIRECTIVE
extern int size_directive_output;

/* Implement ASM_DECLARE_OBJECT_NAME.  This is like most of the standard ELF
   definitions except that it uses mips_declare_object() to emit the label.  */

void
mips_declare_object_name (FILE *stream, const char *name,
                          tree decl)
{
  if (!TARGET_SGI_O32_AS)
    {
#ifdef ASM_OUTPUT_TYPE_DIRECTIVE
      ASM_OUTPUT_TYPE_DIRECTIVE (stream, name, "object");
#endif

      size_directive_output = 0;
      if (!flag_inhibit_size_directive && DECL_SIZE (decl))
        {
          HOST_WIDE_INT size;

          size_directive_output = 1;
          size = int_size_in_bytes (TREE_TYPE (decl));
          ASM_OUTPUT_SIZE_DIRECTIVE (stream, name, size);
        }
    }

  if (TREE_CODE (decl) == VAR_DECL
      && ! TREE_PUBLIC (decl) && DECL_ONE_ONLY (decl) && DECL_WEAK (decl))
    {
      /* Weak symbols in link-once sections must be flagged as weak in
         the assembler file, even when they're not public, so that
         they are relocated correctly by the linker. */
      ASM_WEAKEN_LABEL (stream, name);
    }

  mips_declare_object (stream, name, "", ":\n", 0);
}

/* Implement ASM_FINISH_DECLARE_OBJECT.  This is generic ELF stuff.  */

void
mips_finish_declare_object (FILE *stream, tree decl, int top_level, int at_end)
{
  const char *name;

  name = XSTR (XEXP (DECL_RTL (decl), 0), 0);
  if (!TARGET_SGI_O32_AS
      && !flag_inhibit_size_directive
      && DECL_SIZE (decl) != 0
      && !at_end && top_level
      && DECL_INITIAL (decl) == error_mark_node
      && !size_directive_output)
    {
      HOST_WIDE_INT size;

      size_directive_output = 1;
      size = int_size_in_bytes (TREE_TYPE (decl));
      ASM_OUTPUT_SIZE_DIRECTIVE (stream, name, size);
    }
}
#endif

/* Return true if X is a small data address that can be rewritten
   as a LO_SUM.  */

static bool
mips_rewrite_small_data_p (rtx x)
{
  enum mips_symbol_type symbol_type;

  return (TARGET_EXPLICIT_RELOCS
          && mips_symbolic_constant_p (x, &symbol_type)
          && symbol_type == SYMBOL_SMALL_DATA);
}


/* A for_each_rtx callback for small_data_pattern.  */

static int
small_data_pattern_1 (rtx *loc, void *data ATTRIBUTE_UNUSED)
{
  if (GET_CODE (*loc) == LO_SUM)
    return -1;

  return mips_rewrite_small_data_p (*loc);
}

/* Return true if OP refers to small data symbols directly, not through
   a LO_SUM.  */

int
small_data_pattern (rtx op, enum machine_mode mode ATTRIBUTE_UNUSED)
{
  return (GET_CODE (op) != SEQUENCE
          && for_each_rtx (&op, small_data_pattern_1, 0));
}

/* A for_each_rtx callback, used by mips_rewrite_small_data.  */

static int
mips_rewrite_small_data_1 (rtx *loc, void *data ATTRIBUTE_UNUSED)
{
  if (mips_rewrite_small_data_p (*loc))
    *loc = gen_rtx_LO_SUM (Pmode, pic_offset_table_rtx, *loc);

  if (GET_CODE (*loc) == LO_SUM)
    return -1;

  return 0;
}

/* If possible, rewrite OP so that it refers to small data using
   explicit relocations.  */

rtx
mips_rewrite_small_data (rtx op)
{
  op = copy_insn (op);
  for_each_rtx (&op, mips_rewrite_small_data_1, 0);
  return op;
}

/* Return true if the current function has an insn that implicitly
   refers to $gp.  */

static bool
mips_function_has_gp_insn (void)
{
  /* Don't bother rechecking if we found one last time.  */
  if (!cfun->machine->has_gp_insn_p)
    {
      rtx insn;

      push_topmost_sequence ();
      for (insn = get_insns (); insn; insn = NEXT_INSN (insn))
        if (INSN_P (insn)
            && GET_CODE (PATTERN (insn)) != USE
            && GET_CODE (PATTERN (insn)) != CLOBBER
            && (get_attr_got (insn) != GOT_UNSET
                || small_data_pattern (PATTERN (insn), VOIDmode)))
          break;
      pop_topmost_sequence ();

      cfun->machine->has_gp_insn_p = (insn != 0);
    }
  return cfun->machine->has_gp_insn_p;
}


/* Return the register that should be used as the global pointer
   within this function.  Return 0 if the function doesn't need
   a global pointer.  */

static unsigned int
mips_global_pointer (void)
{
  unsigned int regno;

  /* $gp is always available in non-abicalls code.  */
  if (!TARGET_ABICALLS)
    return GLOBAL_POINTER_REGNUM;

  /* We must always provide $gp when it is used implicitly.  */
  if (!TARGET_EXPLICIT_RELOCS)
    return GLOBAL_POINTER_REGNUM;

  /* FUNCTION_PROFILER includes a jal macro, so we need to give it
     a valid gp.  */
  if (current_function_profile)
    return GLOBAL_POINTER_REGNUM;

  /* If the function has a nonlocal goto, $gp must hold the correct
     global pointer for the target function.  */
  if (current_function_has_nonlocal_goto)
    return GLOBAL_POINTER_REGNUM;

  /* If the gp is never referenced, there's no need to initialize it.
     Note that reload can sometimes introduce constant pool references
     into a function that otherwise didn't need them.  For example,
     suppose we have an instruction like:

          (set (reg:DF R1) (float:DF (reg:SI R2)))

     If R2 turns out to be constant such as 1, the instruction may have a
     REG_EQUAL note saying that R1 == 1.0.  Reload then has the option of
     using this constant if R2 doesn't get allocated to a register.

     In cases like these, reload will have added the constant to the pool
     but no instruction will yet refer to it.  */
  if (!regs_ever_live[GLOBAL_POINTER_REGNUM]
      && !current_function_uses_const_pool
      && !mips_function_has_gp_insn ())
    return 0;

  /* We need a global pointer, but perhaps we can use a call-clobbered
     register instead of $gp.  */
  if (TARGET_NEWABI && current_function_is_leaf)
    for (regno = GP_REG_FIRST; regno <= GP_REG_LAST; regno++)
      if (!regs_ever_live[regno]
          && call_used_regs[regno]
          && !fixed_regs[regno]
          && regno != PIC_FUNCTION_ADDR_REGNUM)
        return regno;

  return GLOBAL_POINTER_REGNUM;
}


/* Return true if the current function must save REGNO.  */

static bool
mips_save_reg_p (unsigned int regno)
{
  /* We only need to save $gp for NewABI PIC.  */
  if (regno == GLOBAL_POINTER_REGNUM)
    return (TARGET_ABICALLS && TARGET_NEWABI
            && cfun->machine->global_pointer == regno);

  /* Check call-saved registers. */
  if (current_function_type == SOFTWARE_CONTEXT_SAVE)
    {
      /* If this is a leaf function, we can use our analysis of the code
         to determine which registers are necessary to save and restore.
         If it's not a leaf function, we have to make conservative
         assumptions that the called function(s) will tromp all over all of
         the call used registers, so we need to save those regardless. */
      if (current_function_is_leaf)
        {
          return (regs_ever_live [regno] && (!call_used_regs [regno]
                || mchp_register_interrupt_context_p (regno)));
//    /* FIXME: do more saving than we really need to so that we can test
//       things out with lots of active save/restore */
//      return ((regs_ever_live[regno] && !call_used_regs[regno])
//            || mchp_register_interrupt_context_p (regno));
        }
      else
        {
          /* for non-leaf functions, we save everything we do for normal
             functions plus all of the interrupt context. */
          return ((regs_ever_live[regno] && !call_used_regs[regno])
            || mchp_register_interrupt_context_p (regno));
        }
    }
  else if (current_function_type == SRS_CONTEXT_SAVE)
    {
      /* If we're using a shadow set, we don't need to save any GPR */
      if (GP_REG_P (regno))
        return false;
      else if (regs_ever_live[regno] && !call_used_regs[regno])
        return true;
    }
  else
    {
      /* The non-interrupt case is more straightforward. We just need
         to save those registers which are both call saved and used by
         this function. */
      if (regs_ever_live[regno] && !call_used_regs[regno])
        return true;
    }

  /* Save whole floating-point reg pair if either half is used - for
     MIPS32 which can use even or odd half for SFmode. */
  if (FP_REG_P (regno) && FP_INC == 2
      && regs_ever_live[regno ^ 1] && !call_used_regs[regno ^ 1])
    return true;

  /* We need to save the old frame pointer before setting up a new one.  */
  if (regno == HARD_FRAME_POINTER_REGNUM && frame_pointer_needed)
    return true;

  /* We need to save the incoming return address if it is ever clobbered
     within the function.  */
  if (regno == GP_REG_FIRST + 31 && regs_ever_live[regno])
    return true;

  /* The return address will be clobbered when profiling. */
  if (regno == GP_REG_FIRST + 31 && current_function_profile)
    return true;

  if (TARGET_MIPS16)
    {
      tree return_type;

      return_type = DECL_RESULT (current_function_decl);

      /* $18 is a special case in mips16 code.  It may be used to call
         a function which returns a floating point value, but it is
         marked in call_used_regs.  */
      if (regno == GP_REG_FIRST + 18 && regs_ever_live[regno])
        return true;

      /* $31 is also a special case.  It will be used to copy a return
         value into the floating point registers if the return value is
         floating point.  */
      if (regno == GP_REG_FIRST + 31
          && mips16_hard_float
          && !aggregate_value_p (return_type, current_function_decl)
          && MIPS_RETURN_FP_REG_P (DECL_MODE (return_type)))
        return true;

      /* In a MIPS16 function which calls __builtin_unwind_init also
         save the 32-bit frame pointer, in case it is needed to unwind
         a 32-bit function higher up in the call stack. */
      if (current_function_has_nonlocal_label
          && regno == GP_REG_FIRST + 30)
        return true;
    }

  return false;
}


/* Return the bytes needed to compute the frame pointer from the current
   stack pointer.  SIZE is the size (in bytes) of the local variables.

   Mips stack frames look like:

             Before call                        After call
        +-----------------------+       +-----------------------+
   high |                       |       |                       |
   mem. |                       |       |                       |
        |  caller's temps.      |       |  caller's temps.      |
        |                       |       |                       |
        +-----------------------+       +-----------------------+
        |                       |       |                       |
        |  arguments on stack.  |       |  arguments on stack.  |
        |                       |       |                       |
        +-----------------------+       +-----------------------+
        |  4 words to save      |       |  4 words to save      |
        |  arguments passed     |       |  arguments passed     |
        |  in registers, even   |       |  in registers, even   |
    SP->|  if not passed.       |  VFP->|  if not passed.       |
        +-----------------------+       +-----------------------+
                                        |                       |
                                        |  fp register save     |
                                        |                       |
                                        +-----------------------+
                                        |                       |
                                        |  gp register save     |
                                        |                       |
                                        +-----------------------+
                                        |                       |
                                        |  local variables      |
                                        |                       |
                                        +-----------------------+
                                        |                       |
                                        |  alloca allocations   |
                                        |                       |
                                        +-----------------------+
                                        |                       |
                                        |  GP save for V.4 abi  |
                                        |                       |
                                        +-----------------------+
                                        |                       |
                                        |  arguments on stack   |
                                        |                       |
                                        +-----------------------+
                                        |  4 words to save      |
                                        |  arguments passed     |
                                        |  in registers, even   |
   low                              SP->|  if not passed.       |
   memory                               +-----------------------+

*/

#define BITSET_P(VALUE, BIT) (((VALUE) & (1L << (BIT))) != 0)

HOST_WIDE_INT
compute_frame_size (HOST_WIDE_INT size)
{
  unsigned int regno;
  HOST_WIDE_INT total_size;     /* # bytes that the entire frame takes up */
  HOST_WIDE_INT var_size;       /* # bytes that variables take up */
  HOST_WIDE_INT args_size;      /* # bytes that outgoing arguments take up */
  HOST_WIDE_INT cprestore_size; /* # bytes that the cprestore slot takes up */
  HOST_WIDE_INT gp_reg_rounded; /* # bytes needed to store gp after rounding */
  HOST_WIDE_INT gp_reg_size;    /* # bytes needed to store gp regs */
  HOST_WIDE_INT fp_reg_size;    /* # bytes needed to store fp regs */
  HOST_WIDE_INT int_reg_size;   /* # bytes needed for interrupt context regs */
  unsigned int mask;            /* mask of saved gp registers */
  unsigned int fmask;           /* mask of saved fp registers */
  bool has_interrupt_context;   /* true if interrupt context is saved */
  bool has_hilo_context;        /* true if hi and lo registers are saved */
                                /*   will only be true for interrupts */

  cfun->machine->global_pointer = mips_global_pointer ();

  /* has_interrupt_context tells us whether we're saving any interrupt
     specific data */
  has_interrupt_context = (current_function_type != NON_INTERRUPT);
  /* has_hilo_context is true if we need to push/pop HI and LO */
  has_hilo_context = 0;

  gp_reg_size = 0;
  fp_reg_size = 0;
  int_reg_size = 0;
  mask = 0;
  fmask = 0;
  var_size = MIPS_STACK_ALIGN (size);
  args_size = current_function_outgoing_args_size;
  cprestore_size = MIPS_STACK_ALIGN (STARTING_FRAME_OFFSET) - args_size;

  /* The space set aside by STARTING_FRAME_OFFSET isn't needed in leaf
     functions.  If the function has local variables, we're committed
     to allocating it anyway.  Otherwise reclaim it here.  */
  if (var_size == 0 && current_function_is_leaf)
    cprestore_size = args_size = 0;

  /* The MIPS 3.0 linker does not like functions that dynamically
     allocate the stack and have 0 for STACK_DYNAMIC_OFFSET, since it
     looks like we are trying to create a second frame pointer to the
     function, so allocate some stack space to make it happy.  */

  if (args_size == 0 && current_function_calls_alloca)
    args_size = 4 * UNITS_PER_WORD;

  total_size = var_size + args_size + cprestore_size;

  /* Calculate space needed for gp registers.  */
  for (regno = GP_REG_FIRST; regno <= GP_REG_LAST; regno++)
    if (mips_save_reg_p (regno))
      {
        gp_reg_size += GET_MODE_SIZE (gpr_mode);
        mask |= 1 << (regno - GP_REG_FIRST);
      }

  /* We need to restore these for the handler.  */
  if (current_function_calls_eh_return)
    {
      unsigned int i;
      for (i = 0; ; ++i)
        {
          regno = EH_RETURN_DATA_REGNO (i);
          if (regno == INVALID_REGNUM)
            break;
          gp_reg_size += GET_MODE_SIZE (gpr_mode);
          mask |= 1 << (regno - GP_REG_FIRST);
        }
    }

  /* For MIPS16E, handle $s2-$s8 specially.  If one of $s2-$s8 must be
     saved, then all lower registers must be saved also. The same is
     true in reverse for saved "arg" registers $a3-$a0. */
  if (TARGET_M16E_SAVE)
    {
      int i;
      static const char s_list[] = {30, 23, 22, 21, 20, 19, 18, 0};
      static const char as_list[] = {4, 5, 6, 7, 0};
      for (i = 0; (regno = s_list[i]) != 0; i++)
        if (BITSET_P (mask, regno))
          {
            i++;
            for (; (regno = s_list[i]) != 0; i++)
              if (!BITSET_P (mask, regno))
                {
                  gp_reg_size += GET_MODE_SIZE (gpr_mode);
                  mask |= 1 << regno;
                }

            break;
          }
      /* Similarly for any "arg" registers which must be saved. */
      for (i = 0; (regno = as_list[i]) != 0; i++)
        if (BITSET_P (mask, regno))
          {
            while ((regno = as_list[++i]) != 0)
              if (!BITSET_P (mask, regno))
                {
                  gp_reg_size += GET_MODE_SIZE (gpr_mode);
                  mask |= 1 << regno;
                }

            break;
          }
    }

  /* This loop must iterate over the same space as its companion in
     save_restore_insns.  */
  for (regno = (FP_REG_LAST - FP_INC + 1);
       regno >= FP_REG_FIRST;
       regno -= FP_INC)
    {
      if (mips_save_reg_p (regno))
        {
          fp_reg_size += FP_INC * UNITS_PER_FPREG;
          fmask |= ((1 << FP_INC) - 1) << (regno - FP_REG_FIRST);
        }
    }

  gp_reg_rounded = MIPS_STACK_ALIGN (gp_reg_size);
  total_size += gp_reg_rounded + MIPS_STACK_ALIGN (fp_reg_size);

  /* add in space for the interrupt context information */
  if (has_interrupt_context)
    {
      /* all interrupt context functions need space
         to preserve STATUS */
      int_reg_size += GET_MODE_SIZE (SImode);
      /* all software interrupt context functions need space
         to preserve EPC also */
      if (current_function_type == SOFTWARE_CONTEXT_SAVE)
        int_reg_size += GET_MODE_SIZE (SImode);
      /* If HI/LO is defined in this function, we need to save them too.
         If the function is not a leaf function, we assume that the
         called function uses them. */
      if (regs_ever_live [LO_REGNUM] || regs_ever_live [HI_REGNUM]
          || !current_function_is_leaf)
        {
          int_reg_size += GET_MODE_SIZE (SImode) * 2;
          has_hilo_context = 1;
        }
    }
  total_size += int_reg_size;

  /* Add in space reserved on the stack by the callee for storing arguments
     passed in registers.  */
  if (mips_abi != ABI_32 && mips_abi != ABI_O64)
    total_size += MIPS_STACK_ALIGN (current_function_pretend_args_size);

  /* Save other computed information.  */
  cfun->machine->frame.total_size = total_size;
  cfun->machine->frame.var_size = var_size;
  cfun->machine->frame.args_size = args_size;
  cfun->machine->frame.cprestore_size = cprestore_size;
  cfun->machine->frame.gp_reg_size = gp_reg_size;
  cfun->machine->frame.fp_reg_size = fp_reg_size;
  cfun->machine->frame.int_reg_size = int_reg_size;
  cfun->machine->frame.mask = mask;
  cfun->machine->frame.fmask = fmask;
  cfun->machine->frame.initialized = reload_completed;
  cfun->machine->frame.num_gp = gp_reg_size / UNITS_PER_WORD;
  cfun->machine->frame.num_fp = fp_reg_size / (FP_INC * UNITS_PER_FPREG);
  cfun->machine->frame.has_interrupt_context = has_interrupt_context;
  cfun->machine->frame.has_hilo_context = has_hilo_context;

  if (mask)
    {
      HOST_WIDE_INT offset;

      /* MIPS16E saves registers from the top */
      if (TARGET_M16E_SAVE)
        offset = total_size - GET_MODE_SIZE (gpr_mode);
      else
        offset = (args_size + cprestore_size + var_size
                  + gp_reg_size - GET_MODE_SIZE (gpr_mode));
      cfun->machine->frame.gp_sp_offset = offset;
      cfun->machine->frame.gp_save_offset = offset - total_size;
    }
  else
    {
      cfun->machine->frame.gp_sp_offset = 0;
      cfun->machine->frame.gp_save_offset = 0;
    }

  if (fmask)
    {
      HOST_WIDE_INT offset;

      offset = (args_size + cprestore_size + var_size
                + gp_reg_rounded + fp_reg_size
                - FP_INC * UNITS_PER_FPREG);
      cfun->machine->frame.fp_sp_offset = offset;
      cfun->machine->frame.fp_save_offset = offset - total_size;
    }
  else
    {
      cfun->machine->frame.fp_sp_offset = 0;
      cfun->machine->frame.fp_save_offset = 0;
    }

  if (has_interrupt_context)
    {
      HOST_WIDE_INT offset;

      offset = (args_size + cprestore_size + var_size
                + gp_reg_rounded + fp_reg_size
                + int_reg_size - GET_MODE_SIZE (SImode));
      cfun->machine->frame.int_sp_offset = offset;
      cfun->machine->frame.int_save_offset = offset - total_size;
    }

  /* Ok, we're done.  */
  return total_size;
}

/* Implement INITIAL_ELIMINATION_OFFSET.  FROM is either the frame
   pointer or argument pointer.  TO is either the stack pointer or
   hard frame pointer.  */

HOST_WIDE_INT
mips_initial_elimination_offset (int from, int to)
{
  HOST_WIDE_INT offset;

  compute_frame_size (get_frame_size ());

  /* Set OFFSET to the offset from the stack pointer.  */
  switch (from)
    {
    case FRAME_POINTER_REGNUM:
      offset = 0;
      break;

    case ARG_POINTER_REGNUM:
      offset = cfun->machine->frame.total_size;
      if (mips_abi == ABI_N32 || mips_abi == ABI_64)
        offset -= current_function_pretend_args_size;
      break;

    default:
      abort ();
    }

  if (TARGET_MIPS16 && to == HARD_FRAME_POINTER_REGNUM)
    offset -= cfun->machine->frame.args_size;

  return offset;
}

/* Implement RETURN_ADDR_RTX.  Note, we do not support moving
   back to a previous frame.  */
rtx
mips_return_addr (int count, rtx frame ATTRIBUTE_UNUSED)
{
  if (count != 0)
    return const0_rtx;

  return get_hard_reg_initial_val (Pmode, GP_REG_FIRST + 31);
}

/* Use FN to save or restore register REGNO.  MODE is the register's
   mode and OFFSET is the offset of its save slot from the current
   stack pointer.  */

static void
mips_save_restore_reg (enum machine_mode mode, int regno,
                       HOST_WIDE_INT offset, mips_save_restore_fn fn)
{
  rtx mem;

  mem = gen_rtx_MEM (mode, plus_constant (stack_pointer_rtx, offset));

  if (!current_function_calls_eh_return)
    RTX_UNCHANGING_P (mem) = 1;

  fn (gen_rtx_REG (mode, regno), mem);

}


/* Call FN for each register that is saved by the current function.
   SP_OFFSET is the offset of the current stack pointer from the start
   of the frame.  */

static void
mips_for_each_saved_reg (HOST_WIDE_INT sp_offset, mips_save_restore_fn fn)
{
  enum machine_mode fpr_mode;
  HOST_WIDE_INT offset;
  int regno;

  /* Save registers starting from high to low.  The debuggers prefer at least
     the return register be stored at func+4, and also it allows us not to
     need a nop in the epilog if at least one register is reloaded in
     addition to return address.  */
  offset = cfun->machine->frame.gp_sp_offset - sp_offset;
  for (regno = GP_REG_LAST; regno >= GP_REG_FIRST; regno--)
    if (BITSET_P (cfun->machine->frame.mask, regno - GP_REG_FIRST))
      {
        mips_save_restore_reg (gpr_mode, regno, offset, fn);
        offset -= GET_MODE_SIZE (gpr_mode);
      }

  /* This loop must iterate over the same space as its companion in
     compute_frame_size.  */
  offset = cfun->machine->frame.fp_sp_offset - sp_offset;
  fpr_mode = (TARGET_SINGLE_FLOAT ? SFmode : DFmode);
  for (regno = (FP_REG_LAST - FP_INC + 1);
       regno >= FP_REG_FIRST;
       regno -= FP_INC)
    if (BITSET_P (cfun->machine->frame.fmask, regno - FP_REG_FIRST))
      {
        mips_save_restore_reg (fpr_mode, regno, offset, fn);
        offset -= GET_MODE_SIZE (fpr_mode);
      }
}

/* If we're generating n32 or n64 abicalls, and the current function
   does not use $28 as its global pointer, emit a cplocal directive.
   Use pic_offset_table_rtx as the argument to the directive.  */

static void
mips_output_cplocal (void)
{
  if (!TARGET_EXPLICIT_RELOCS
      && cfun->machine->global_pointer > 0
      && cfun->machine->global_pointer != GLOBAL_POINTER_REGNUM)
    output_asm_insn (".cplocal %+", 0);
}

/* If we're generating n32 or n64 abicalls, emit instructions
   to set up the global pointer.  */

static void
mips_emit_loadgp (void)
{
  if (TARGET_ABICALLS && TARGET_NEWABI && cfun->machine->global_pointer > 0)
    {
      rtx addr, offset, incoming_address;

      addr = XEXP (DECL_RTL (current_function_decl), 0);
      offset = mips_unspec_address (addr, SYMBOL_GOTOFF_LOADGP);
      incoming_address = gen_rtx_REG (Pmode, PIC_FUNCTION_ADDR_REGNUM);
      emit_insn (gen_loadgp (offset, incoming_address));
      if (!TARGET_EXPLICIT_RELOCS)
        emit_insn (gen_loadgp_blockage ());
    }
}

/* Set up the stack and frame (if desired) for the function.  */

static void
mips_output_function_prologue (FILE *file, HOST_WIDE_INT size ATTRIBUTE_UNUSED)
{
  const char *fnname;
  HOST_WIDE_INT tsize = cfun->machine->frame.total_size;

  /* ??? When is this really needed?  At least the GNU assembler does not
     need the source filename more than once in the file, beyond what is
     emitted by the debug information.  */
  if (!TARGET_GAS)
    ASM_OUTPUT_SOURCE_FILENAME (file, DECL_SOURCE_FILE (current_function_decl));

#ifdef SDB_DEBUGGING_INFO
  if (debug_info_level != DINFO_LEVEL_TERSE && write_symbols == SDB_DEBUG)
    ASM_OUTPUT_SOURCE_LINE (file, DECL_SOURCE_LINE (current_function_decl), 0);
#endif

  /* Select the mips16 mode for this function. */
  if (TARGET_MIPS16E)
    fprintf (file, "\t.set\tmips16e\n");
  else if (TARGET_MIPS16)
    fprintf (file, "\t.set\tmips16\n");
  else
    fprintf (file, "\t.set\tnomips16\n");

  /* Required to context preserve $at in software ISR context saving */
  if (set_noat == 0)
    {
      fprintf (file, "\t.set\tnoat\n");
      set_noat++;
    }

  if (!FUNCTION_NAME_ALREADY_DECLARED)
    {
      /* Get the function name the same way that toplev.c does before calling
         assemble_start_function.  This is needed so that the name used here
         exactly matches the name used in ASM_DECLARE_FUNCTION_NAME.  */
      fnname = XSTR (XEXP (DECL_RTL (current_function_decl), 0), 0);

      if (!flag_inhibit_size_directive)
        {
          fputs ("\t.ent\t", file);
          assemble_name (file, fnname);
          fputs ("\n", file);
        }

      assemble_name (file, fnname);
      fputs (":\n", file);
    }

  if (!flag_inhibit_size_directive)
    {
      /* .frame FRAMEREG, FRAMESIZE, RETREG */
      fprintf (file,
               "\t.frame\t%s," HOST_WIDE_INT_PRINT_DEC ",%s\t\t"
               "# vars= " HOST_WIDE_INT_PRINT_DEC ", regs= %d/%d"
               ", args= " HOST_WIDE_INT_PRINT_DEC
               ", gp= " HOST_WIDE_INT_PRINT_DEC "\n",
               (reg_names[(frame_pointer_needed)
                          ? HARD_FRAME_POINTER_REGNUM : STACK_POINTER_REGNUM]),
               ((frame_pointer_needed && TARGET_MIPS16)
                ? tsize - cfun->machine->frame.args_size
                : tsize),
               reg_names[GP_REG_FIRST + 31],
               cfun->machine->frame.var_size,
               cfun->machine->frame.num_gp,
               cfun->machine->frame.num_fp,
               cfun->machine->frame.args_size,
               cfun->machine->frame.cprestore_size);

      /* .mask MASK, GPOFFSET; .fmask FPOFFSET */
      fprintf (file, "\t.mask\t0x%08x," HOST_WIDE_INT_PRINT_DEC "\n",
               cfun->machine->frame.mask,
               cfun->machine->frame.gp_save_offset);
      fprintf (file, "\t.fmask\t0x%08x," HOST_WIDE_INT_PRINT_DEC "\n",
               cfun->machine->frame.fmask,
               cfun->machine->frame.fp_save_offset);

      /* Require:
         OLD_SP == *FRAMEREG + FRAMESIZE => can find old_sp from nominated FP reg.
         HIGHEST_GP_SAVED == *FRAMEREG + FRAMESIZE + GPOFFSET => can find saved regs.  */
    }

  if (TARGET_ABICALLS && !TARGET_NEWABI && cfun->machine->global_pointer > 0)
    {
      /* Handle the initialization of $gp for SVR4 PIC.  */
      if (!cfun->machine->all_noreorder_p)
        output_asm_insn ("%(.cpload\t%^%)", 0);
      else
        output_asm_insn ("%(.cpload\t%^\n\t%<", 0);
    }
  else if (cfun->machine->all_noreorder_p)
    output_asm_insn ("%(%<", 0);

  /* Tell the assembler which register we're using as the global
     pointer.  This is needed for thunks, since they can use either
     explicit relocs or assembler macros.  */
  mips_output_cplocal ();

  /* If this is an interrupt function, we need to save that
     context first. We don't want to use the generic
     mips_for_each_saved_reg() function because we want to
     leave defined values in K0 and K1. */
  if (cfun->machine->frame.has_interrupt_context)
    {
      HOST_WIDE_INT step1;
      HOST_WIDE_INT offset;

      step1 = MIN (tsize, MIPS_MAX_FIRST_STACK_STEP);

      /* if this interrupt is using a shadow register set, we need to
         get the stack pointer from the previous register set */
      /* Trumping that concern, at least for the time being, is that we
         want the first four instructions of the interrupt handler to be
         the same for all handler functions. This lets there be cache lines
         locked to those instructions, lowering the latency. */
//      if (current_function_type == SRS_CONTEXT_SAVE)
//        {
          fprintf (file, "\trdpgpr\t$sp, $sp\n");
//        }

      /* Start at the uppermost location for saving */
      offset = cfun->machine->frame.int_sp_offset;

      /* we want the first four instructions at the top of the interrupt
         to be common across all software save handlers so an application
         can lock the cache lines to those instructions and reduce the
         latency */
      if (current_function_type == SOFTWARE_CONTEXT_SAVE)
        {
          /* This is for non-SRS interrupts. We need to differentiate and
             generate different code for highest priority handlers.
             We don't need to move the Cause IPL into the SR for the
             highest priority interrupt since we're not allowing nested
             interrupts at that level. All interrupts are disabled in that
             case (see below loading of SR), so the IPL value in SR is not
             relevent. */

          /* Load the Cause RIPL into k0 */
          fprintf (file, "\tmfc0\t$k0, $13\n");

          /* Load EPC into k1 */
          fprintf (file, "\tmfc0\t$k1, $14\n");

        }


      /* allocate the stack space to save the registers. If more space
         needs to be allocated, the expand_prologue() function handled
         it. */
      fprintf (file, "\taddiu\t$sp, $sp, " HOST_WIDE_INT_PRINT_DEC "\n",
          -step1);

      if (current_function_type == SOFTWARE_CONTEXT_SAVE)
        {
          /* Push EPC into its stack slot */
          fprintf (file, "\tsw\t$k1, " HOST_WIDE_INT_PRINT_DEC "($sp)\n", offset);
          offset -= GET_MODE_SIZE (SImode);
        }

      /* Push status into its stack slot */
      fprintf (file, "\tmfc0\t$k1, $12\n");
      if (current_function_type == SOFTWARE_CONTEXT_SAVE) {
          /* right justify the RIPL in k0               */
	  /* and avoid Read after Write interlock on k1 */ 
          /* C32-119                                    */
          fprintf (file, "\tsrl\t$k0, $k0, %d\n", CAUSE_IPL);
      }
      fprintf (file, "\tsw\t$k1, " HOST_WIDE_INT_PRINT_DEC "($sp)\n", offset);
      offset -= GET_MODE_SIZE (SImode);

      if (current_function_type == SOFTWARE_CONTEXT_SAVE)
        {
          /* Insert the RIPL into our copy of SR (k1) as the new IPL */
          fprintf (file, "\tins\t$k1, $k0, %d, 6\n", SR_IPL);
        }

      /* we may need to also preserve HI/LO. We want to do this as late
         as possible in the prologue sequence so that if we interrupted
         during a mul/div operation, we give it as much time to complete
         as we can before accessing HI/LO and potentially stalling the
         pipeline. */
      if (cfun->machine->frame.has_hilo_context)
        {
          fprintf (file, "\tmflo\t$k0\n");
          fprintf (file, "\tsw\t$k0, " HOST_WIDE_INT_PRINT_DEC "($sp)\n",
              offset);
          offset -= GET_MODE_SIZE (SImode);
          fprintf (file, "\tmfhi\t$k0\n");
          fprintf (file, "\tsw\t$k0, " HOST_WIDE_INT_PRINT_DEC "($sp)\n",
              offset);
          offset -= GET_MODE_SIZE (SImode);
        }

      /* Enable interrupts and clear the UM, ERL and EXL bits.
         IE is already the correct value, so we don't have to do
         anything explicit there for software context save. For SRS,
         we're in the highest priority alreadys, so we don't want
         to reenable interrupts at all, and should thus also clear IE. */
      if (current_function_type == SOFTWARE_CONTEXT_SAVE)
        fprintf (file, "\tins\t$k1, $0, %d, %d\n", SR_EXL, 4);
      else {
        fprintf (file, "\tins\t$k1, $0, %d, %d\n", SR_IE, 5);
        /*
         In current PIC32 IPL7 is the ONLY one with a SRS
         so we can set the IPL in Status directly IPL=bits 15-10
         */
          fprintf (file, "\tori\t$k1, $k1, 0x1c00\n");  
        }
      fprintf (file, "\tmtc0\t$k1, $12\n");
    }
}

/* Make the last instruction frame related and note that it performs
   the operation described by FRAME_PATTERN.  */

static void
mips_set_frame_expr (rtx frame_pattern)
{
  rtx insn;

  insn = get_last_insn ();
  RTX_FRAME_RELATED_P (insn) = 1;
  REG_NOTES (insn) = alloc_EXPR_LIST (REG_FRAME_RELATED_EXPR,
                                      frame_pattern,
                                      REG_NOTES (insn));
}


/* Return a frame-related rtx that stores REG at MEM.
   REG must be a single register.  */

static rtx
mips_frame_set (rtx mem, rtx reg)
{
  rtx set = gen_rtx_SET (VOIDmode, mem, reg);
  RTX_FRAME_RELATED_P (set) = 1;
  return set;
}


/* Save register REG to MEM.  Make the instruction frame-related.  */

static void
mips_save_reg (rtx reg, rtx mem)
{
  if (GET_MODE (reg) == DFmode && !TARGET_FLOAT64)
    {
      rtx x1, x2;

      if (mips_split_64bit_move_p (mem, reg))
        mips_split_64bit_move (mem, reg);
      else
        {
          emit_move_insn (mem, reg);
        }            

      x1 = mips_frame_set (mips_subword (mem, 0), mips_subword (reg, 0));
      x2 = mips_frame_set (mips_subword (mem, 1), mips_subword (reg, 1));
      mips_set_frame_expr (gen_rtx_PARALLEL (VOIDmode, gen_rtvec (2, x1, x2)));
    }
  else
    {
      if (TARGET_MIPS16
          && REGNO (reg) != GP_REG_FIRST + 31
          && !M16_REG_P (REGNO (reg)))
        {
          /* Save a non-mips16 register by moving it through a temporary.
             We don't need to do this for $31 since there's a special
             instruction for it.  */
          emit_move_insn (MIPS_PROLOGUE_TEMP (GET_MODE (reg)), reg);
          emit_move_insn (mem, MIPS_PROLOGUE_TEMP (GET_MODE (reg)));
        }
      else
        {
            emit_move_insn (mem, reg);
        }

      mips_set_frame_expr (mips_frame_set (mem, reg));
    }
}


/* Expand the prologue into a bunch of separate insns.  */

void
mips_expand_prologue (void)
{
  HOST_WIDE_INT size;
  bool mips16e_save_frame = false;
  tree ipl_tree;
  unsigned interrupt_priority;

  if ((ipl_tree = mchp_function_interrupt_p (current_function_decl)) != NULL)
    {
      /* the priority can be either "single" or "ipl[0..7]"
         When the interrupt handler is for single interrupt mode, we
         treat it as a software context save handler. */
      if (strcmp (IDENTIFIER_POINTER (TREE_VALUE (ipl_tree)), "single") == 0)
        interrupt_priority = 0;
      else
        interrupt_priority = IDENTIFIER_POINTER (TREE_VALUE (ipl_tree))[3] - '0';
      if (interrupt_priority == 7)
        current_function_type = SRS_CONTEXT_SAVE;
      else
        current_function_type = SOFTWARE_CONTEXT_SAVE;
      if (mchp_function_naked_p (current_function_decl))
        error ("interrupt handler functions cannot also be naked functions");
    }
  else
    current_function_type = NON_INTERRUPT;

  if (cfun->machine->global_pointer > 0)
    REGNO (pic_offset_table_rtx) = cfun->machine->global_pointer;

  size = compute_frame_size (get_frame_size ());

  /* if this function is specified as a naked function, just return without
     expanding any code */
  if (mchp_function_naked_p (current_function_decl))
    return;

  /* Save the registers.  Allocate up to MIPS_MXA_FIRST_STACK_STEP
     bytes beforehand; this is enough to cover the register save area
     without going out of range.  */
  if ((cfun->machine->frame.mask | cfun->machine->frame.fmask) != 0
      || cfun->machine->frame.has_interrupt_context)
    {
      HOST_WIDE_INT step1;

      step1 = MIN (size, MIPS_MAX_FIRST_STACK_STEP);

      if (TARGET_M16E_SAVE)
        {
          int i, regno;
          int nreg = 0, orig_nreg;
          HOST_WIDE_INT offset;
          rtx dwarf;
          static const char reglist_save[] = {31, 30, 23, 22, 21, 20, 19,
                                              18, 17, 16, 7, 6, 5, 4, 0};
          unsigned int mask = cfun->machine->frame.mask;

          if (frame_pointer_needed && size > MIPS_MAX_FIRST_STACK_STEP
              && ((size - cfun->machine->frame.args_size)
                  <= MIPS_MAX_FIRST_STACK_STEP)
              && SMALL_OPERAND (-cfun->machine->frame.args_size))
            {
              /* Reduce the stack to that of the frame pointer. */
              step1 = size - cfun->machine->frame.args_size;
              mips16e_save_frame = true;
            }

          for (i = 0, nreg = 0; (regno = reglist_save[i]) != 0; i++)
            if (BITSET_P (mask, regno))
              {
                mask &= ~(1 << regno);
                nreg++;
              }

          orig_nreg = nreg;

          if (dwarf2out_do_frame ())
            {
              rtx tmp;
              int fsize = 0;

              /* Compute frame size remaining after pushing regs */
              fsize = step1 - nreg * UNITS_PER_WORD;
              if (fsize > 0)
                nreg++;

              dwarf = gen_rtx_PARALLEL (VOIDmode, rtvec_alloc (nreg));
              RTX_FRAME_RELATED_P (dwarf) = 1;

              /* Now tell Dwarf to "push" each register */
              for (i = 0, nreg = 0; (regno = reglist_save[i]) != 0; i++)
                if (BITSET_P (cfun->machine->frame.mask, regno))
                  {
                    tmp = gen_rtx_SET (VOIDmode,
                                       gen_rtx_MEM (SImode,
                                                    gen_rtx_PRE_DEC (BLKmode,
                                                      stack_pointer_rtx)),
                                                    gen_rtx_REG (SImode,
                                                      GP_REG_FIRST + regno));
                    RTX_FRAME_RELATED_P (tmp) = 1;
                    XVECEXP (dwarf, 0, nreg) = tmp;
                    nreg++;
                  }

              /* Adjust stack pointer by remaining frame size */
              if (fsize > 0)
                {
                  tmp = gen_rtx_SET (VOIDmode, stack_pointer_rtx,
                                     gen_rtx_MINUS (SImode, stack_pointer_rtx,
                                                    GEN_INT (fsize)));
                  RTX_FRAME_RELATED_P (tmp) = 1;
                  XVECEXP (dwarf, 0, nreg) = tmp;
                }
            }

          emit_insn (gen_save_mips16e (GEN_INT (step1)));

          /* Attach to save instruction */
          if (dwarf2out_do_frame ())
            mips_set_frame_expr (dwarf);

          if (mips16e_save_frame)
            RTX_FRAME_RELATED_P
              (emit_move_insn (hard_frame_pointer_rtx,
                               stack_pointer_rtx)) = 1;

          size -= step1;

          /* Check if we need to save other registers */
          offset = step1 - GET_MODE_SIZE (gpr_mode) * (orig_nreg + 1);
          for (regno = GP_REG_LAST; regno >= GP_REG_FIRST; regno--)
            if (BITSET_P (mask, regno - GP_REG_FIRST))
              {
                mips_save_restore_reg (gpr_mode, regno, offset, mips_save_reg);
                offset -= GET_MODE_SIZE (gpr_mode);
              }
        }
      else
        {
          /* if this is an interrupt function, this first stack allocation
             will be performed by the static frame code and doesn't need
             to be done here. */
          if (current_function_type == NON_INTERRUPT)
            RTX_FRAME_RELATED_P (emit_insn (gen_add3_insn (stack_pointer_rtx,
                    stack_pointer_rtx,
                    GEN_INT (-step1)))) = 1;

          size -= step1;

          /* save the general purpose registers which need to be preserved */
          mips_for_each_saved_reg (size, mips_save_reg);
        }

    }

  /* Allocate the rest of the frame.  */
  if (size > 0)
    {
      if (SMALL_OPERAND (-size))
        RTX_FRAME_RELATED_P (emit_insn (gen_add3_insn (stack_pointer_rtx,
                                                       stack_pointer_rtx,
                                                       GEN_INT (-size)))) = 1;
      else
        {
          if (TARGET_MIPS16 && TARGET_NO_DATA_IN_CODE)
            {
              /* We must fetch the size from constant pool manually,
                 as emit_move_insn won't handle splitting the constant
                 pool address. */
              rtx constmem = force_const_mem (Pmode, GEN_INT (size));
              emit_move_insn (MIPS_PROLOGUE_TEMP (Pmode), XEXP (constmem, 0));
              emit_move_insn (MIPS_PROLOGUE_TEMP (Pmode),
                              gen_rtx_MEM (Pmode, MIPS_PROLOGUE_TEMP (Pmode)));
            }
          else
            emit_move_insn (MIPS_PROLOGUE_TEMP (Pmode), GEN_INT (size));

          if (TARGET_MIPS16)
            {
              /* There are no instructions to add or subtract registers
                 from the stack pointer, so use the frame pointer as a
                 temporary.  We should always be using a frame pointer
                 in this case anyway.  */
              if (!frame_pointer_needed)
                abort ();

              emit_move_insn (hard_frame_pointer_rtx, stack_pointer_rtx);
              emit_insn (gen_sub3_insn (hard_frame_pointer_rtx,
                                        hard_frame_pointer_rtx,
                                        MIPS_PROLOGUE_TEMP (Pmode)));
              emit_move_insn (stack_pointer_rtx, hard_frame_pointer_rtx);
            }
          else
            emit_insn (gen_sub3_insn (stack_pointer_rtx,
                                      stack_pointer_rtx,
                                      MIPS_PROLOGUE_TEMP (Pmode)));

          /* Describe the combined effect of the previous instructions.  */
          mips_set_frame_expr
            (gen_rtx_SET (VOIDmode, stack_pointer_rtx,
                          plus_constant (stack_pointer_rtx, -size)));
        }
    }

  /* Set up the frame pointer, if we're using one.  In mips16 code,
     we point the frame pointer ahead of the outgoing argument area.
     This should allow more variables & incoming arguments to be
     accessed with unextended instructions.  */
  if (frame_pointer_needed && !mips16e_save_frame)
    {
      if (TARGET_MIPS16 && cfun->machine->frame.args_size != 0)
        {
          rtx offset = GEN_INT (cfun->machine->frame.args_size);
          if (SMALL_OPERAND (cfun->machine->frame.args_size))
            RTX_FRAME_RELATED_P
              (emit_insn (gen_add3_insn (hard_frame_pointer_rtx,
                                         stack_pointer_rtx,
                                         offset))) = 1;
          else
            {
              if (TARGET_NO_DATA_IN_CODE)
                {
                  /* We must fetch the size from constant pool manually,
                     as emit_move_insn won't handle splitting the constant
                     pool address. */
                  rtx constmem = force_const_mem (Pmode, offset);
                  emit_move_insn (MIPS_PROLOGUE_TEMP (Pmode),
                                  XEXP (constmem, 0));
                  emit_move_insn (MIPS_PROLOGUE_TEMP (Pmode),
                                  gen_rtx_MEM (Pmode,
                                               MIPS_PROLOGUE_TEMP (Pmode)));
                }
              else
                emit_move_insn (MIPS_PROLOGUE_TEMP (Pmode), offset);

              emit_move_insn (hard_frame_pointer_rtx, stack_pointer_rtx);
              emit_insn (gen_add3_insn (hard_frame_pointer_rtx,
                                        hard_frame_pointer_rtx,
                                        MIPS_PROLOGUE_TEMP (Pmode)));
              mips_set_frame_expr
                (gen_rtx_SET (VOIDmode, hard_frame_pointer_rtx,
                              plus_constant (stack_pointer_rtx,
                                             cfun->machine->frame.args_size)));
            }
        }
      else
        RTX_FRAME_RELATED_P (emit_move_insn (hard_frame_pointer_rtx,
                                             stack_pointer_rtx)) = 1;
    }

  /* If generating o32/o64 abicalls, save $gp on the stack.  */
  if (TARGET_ABICALLS && !TARGET_NEWABI && !current_function_is_leaf)
    emit_insn (gen_cprestore (GEN_INT (current_function_outgoing_args_size)));

  mips_emit_loadgp ();

  /* If we are profiling, make sure no instructions are scheduled before
     the call to mcount.  */

  if (current_function_profile)
    emit_insn (gen_blockage ());
    
}

/* Do any necessary cleanup after a function to restore stack, frame,
   and regs.  */

#define RA_MASK BITMASK_HIGH    /* 1 << 31 */
#define PIC_OFFSET_TABLE_MASK (1 << (PIC_OFFSET_TABLE_REGNUM - GP_REG_FIRST))

static void
mips_output_function_epilogue (FILE *file ATTRIBUTE_UNUSED,
                               HOST_WIDE_INT size ATTRIBUTE_UNUSED)
{
  /* If this is an interrupt handler function, we need to process
     the additional interrupt context as well */
  if (cfun->machine->frame.has_interrupt_context)
    {
      HOST_WIDE_INT offset;
      HOST_WIDE_INT tsize = cfun->machine->frame.total_size;
      HOST_WIDE_INT step1 = MIN (tsize, MIPS_MAX_FIRST_STACK_STEP);

      /* Disable interrupts */
      if (current_function_type == SOFTWARE_CONTEXT_SAVE){
        fprintf (file, "\tdi\n");
        fprintf (file, "\tehb\n");
      }
      /* Restore the original HI/LO if required */
      if (cfun->machine->frame.has_hilo_context)
        {
          offset = cfun->machine->frame.int_sp_offset -
            GET_MODE_SIZE (gpr_mode);
          if (current_function_type == SOFTWARE_CONTEXT_SAVE)
            offset -= GET_MODE_SIZE (gpr_mode);
          fprintf (file, "\tlw\t$k0, " HOST_WIDE_INT_PRINT_DEC "($sp)\n",
              offset);
          fprintf (file, "\tmtlo\t$k0\n");
          offset -= GET_MODE_SIZE (gpr_mode);
          fprintf (file, "\tlw\t$k0, " HOST_WIDE_INT_PRINT_DEC "($sp)\n",
              offset);
          fprintf (file, "\tmthi\t$k0\n");
        }

      offset = cfun->machine->frame.int_sp_offset;
      if (current_function_type == SOFTWARE_CONTEXT_SAVE)
        {
          /* Restore the original EPC */
          fprintf (file, "\tlw\t$k0, " HOST_WIDE_INT_PRINT_DEC "($sp)\n", offset);
          fprintf (file, "\tmtc0\t$k0, $14\n");
          offset -= GET_MODE_SIZE (gpr_mode);

        }

      /* Get the original Status into K0 */
      fprintf (file, "\tlw\t$k0, " HOST_WIDE_INT_PRINT_DEC "($sp)\n", offset);

      /* Deallocate the stack space */
      if (current_function_type == SOFTWARE_CONTEXT_SAVE)
        fprintf (file, "\taddiu\t$sp, $sp, " HOST_WIDE_INT_PRINT_DEC "\n",
            step1);

      /* Restore the original status */
      fprintf (file, "\tmtc0\t$k0, $12\n");

      /* return */
      fprintf (file, "\teret\n");
   }
  /* Reinstate the normal $gp.  */
  REGNO (pic_offset_table_rtx) = GLOBAL_POINTER_REGNUM;
  mips_output_cplocal ();

  if (cfun->machine->all_noreorder_p)
    {
      /* Avoid using %>%) since it adds excess whitespace.  */
      output_asm_insn (".set\tmacro", 0);
      output_asm_insn (".set\treorder", 0);
      set_noreorder = set_nomacro = 0;
    }
  while (set_noat > 0)
  {
    output_asm_insn (".set\tat", 0);
    set_noat--;
  }
      
  if (!FUNCTION_NAME_ALREADY_DECLARED && !flag_inhibit_size_directive)
    {
      const char *fnname;

      /* Get the function name the same way that toplev.c does before calling
         assemble_start_function.  This is needed so that the name used here
         exactly matches the name used in ASM_DECLARE_FUNCTION_NAME.  */
      fnname = XSTR (XEXP (DECL_RTL (current_function_decl), 0), 0);
      fputs ("\t.end\t", file);
      assemble_name (file, fnname);
      fputs ("\n", file);
    }

  /* In mips16 mode, we may need to generate a 32 bit to handle
     floating point arguments.  The linker will arrange for any 32 bit
     functions to call this stub, which will then jump to the 16 bit
     function proper.  */
  if (TARGET_MIPS16 && mips16_hard_float
      && current_function_args_info.fp_code != 0)
    build_mips16_function_stub (file, current_function_decl,
                                current_function_args_info.fp_code);

    current_function_type = NON_INTERRUPT;
}

void
mips_output_pool_epilogue ()
{
  rtx string;


  /* If any following function uses the same strings as this one, force
     them to refer those strings indirectly.  Nearby functions could
     refer them using pc-relative addressing, but it isn't safe in
     general.  For instance, some functions may be placed in sections
     other than .text, and we don't know whether they be close enough
     to this one.  In large files, even other .text functions can be
     too far away.  */
  for (string = cfun->machine->mips16_strings;
       string != 0; string = XEXP (string, 1))
    SYMBOL_REF_FLAG (XEXP (string, 0)) = 0;
  free_EXPR_LIST_list (&cfun->machine->mips16_strings);
  cfun->machine->mips_string_length = 0;
}


/* Emit instructions to restore register REG from slot MEM.  */

static void
mips_restore_reg (rtx reg, rtx mem)
{
  /* There's no mips16 instruction to load $31 directly.  Load into
     $7 instead and adjust the return insn appropriately.  */
  if (TARGET_MIPS16 && REGNO (reg) == GP_REG_FIRST + 31)
    reg = gen_rtx_REG (GET_MODE (reg), 7);

  if (TARGET_MIPS16 && !M16_REG_P (REGNO (reg)))
    {
      /* Can't restore directly; move through a temporary.  */
      emit_move_insn (MIPS_EPILOGUE_TEMP (GET_MODE (reg)), mem);
      emit_move_insn (reg, MIPS_EPILOGUE_TEMP (GET_MODE (reg)));
    }
  else
    emit_move_insn (reg, mem);
}


/* Expand the epilogue into a bunch of separate insns.  SIBCALL_P is true
   if this epilogue precedes a sibling call, false if it is for a normal
   "epilogue" pattern.  */

void
mips_expand_epilogue (int sibcall_p)
{
  HOST_WIDE_INT step1, step2;
  rtx base, target;
  tree return_type;
  enum machine_mode return_mode;

  if (!sibcall_p && mips_can_use_return_insn ())
    {
      emit_jump_insn (gen_return ());
      return;
    }

  /* if this function is specified as a naked function, we don't want
     any actual epilogue code. We still need to have something for
     the "return" statement branches to target. */
  if (mchp_function_naked_p (current_function_decl))
    {
      emit_jump_insn (gen_return_from_epilogue ());
      return;
    }


  /* In mips16 mode, if the return value should go into a floating
     point register, we need to call a helper routine to copy it
     over. */
  return_type = DECL_RESULT (current_function_decl);
  return_mode = DECL_MODE (return_type);
  if (TARGET_MIPS16
      && mips16_hard_float
      && !aggregate_value_p (return_type, current_function_decl)
      && MIPS_RETURN_FP_REG_P (return_mode))
    {
      char name[17];
      rtx func;
      rtx insn;
      tree id;

      sprintf (name, "__mips16_ret_%s", MIPS_RETURN_FP_REG_TYPE (return_mode));
      id = get_identifier (name);
      func = gen_rtx (SYMBOL_REF, Pmode, IDENTIFIER_POINTER (id));
      insn = emit_call_insn (gen_call_internal (func, const0_rtx));
      CALL_INSN_FUNCTION_USAGE (insn) =
        gen_rtx (EXPR_LIST, VOIDmode,
                 gen_rtx (USE, VOIDmode, gen_rtx (REG, return_mode, GP_RETURN)),
                 CALL_INSN_FUNCTION_USAGE (insn));
    }

  /* Split the frame into two.  STEP1 is the amount of stack we should
     deallocate before restoring the registers.  STEP2 is the amount we
     should deallocate afterwards.

     Start off by assuming that no registers need to be restored.  */
  step1 = cfun->machine->frame.total_size;
  step2 = 0;

  /* Work out which register holds the frame address.  Account for the
     frame pointer offset used by mips16 code.  */
  if (!frame_pointer_needed)
    base = stack_pointer_rtx;
  else
    {
      base = hard_frame_pointer_rtx;
      if (TARGET_MIPS16)
        step1 -= cfun->machine->frame.args_size;
    }

  /* If we need to restore registers, deallocate as much stack as
     possible in the second step without going out of range.  */
  if ((cfun->machine->frame.mask | cfun->machine->frame.fmask) != 0
      || cfun->machine->frame.has_interrupt_context)
    {
      step2 = MIN (step1, MIPS_MAX_FIRST_STACK_STEP);
      step1 -= step2;
    }

  /* Set TARGET to BASE + STEP1.  */
  target = base;
  if (step1 > 0)
    {
      rtx adjust;

      /* Get an rtx for STEP1 that we can add to BASE.  */
      adjust = GEN_INT (step1);
      if (!SMALL_OPERAND (step1))
        {
          if (TARGET_MIPS16 && TARGET_NO_DATA_IN_CODE)
            {
              /* We must fetch the size from constant pool manually,
                 as emit_move_insn won't handle splitting the constant
                 pool address. */
              rtx constmem = force_const_mem (Pmode, adjust);
              emit_move_insn (MIPS_EPILOGUE_TEMP (Pmode), XEXP (constmem, 0));
              emit_move_insn (MIPS_EPILOGUE_TEMP (Pmode),
                              gen_rtx_MEM (Pmode, MIPS_EPILOGUE_TEMP (Pmode)));
            }
          else
            emit_move_insn (MIPS_EPILOGUE_TEMP (Pmode), adjust);
          adjust = MIPS_EPILOGUE_TEMP (Pmode);
        }

      /* Normal mode code can copy the result straight into $sp.  */
      if (!TARGET_MIPS16)
        target = stack_pointer_rtx;

      emit_insn (gen_add3_insn (target, base, adjust));
    }

  /* Copy TARGET into the stack pointer.  */
  if (target != stack_pointer_rtx)
    emit_move_insn (stack_pointer_rtx, target);

  /* If we're using addressing macros for n32/n64 abicalls, $gp is
     implicitly used by all SYMBOL_REFs.  We must emit a blockage
     insn before restoring it.  */
  if (TARGET_ABICALLS && TARGET_NEWABI && !TARGET_EXPLICIT_RELOCS)
    emit_insn (gen_blockage ());

  if (TARGET_M16E_SAVE && cfun->machine->frame.mask != 0)
    {
      int i, regno;
      int nreg;
      HOST_WIDE_INT offset;
      static const char reglist_save[] = {31, 30, 23, 22, 21, 20, 19,
                                          18, 17, 16, 7, 6, 5, 4, 0};
      unsigned int mask = cfun->machine->frame.mask;

      /* Clear the bits in mask which will be handled by the restore insn. */
      for (i = 0, nreg = 0; (regno = reglist_save[i]) != 0; i++)
        if (BITSET_P (mask, regno))
          {
            mask &= ~(1 << regno);
            nreg++;
          }

      /* Restore any other registers manually. */
      offset = cfun->machine->frame.total_size - step2;
      for (regno = GP_REG_LAST; regno >= GP_REG_FIRST; regno--)
        if (BITSET_P (mask, regno - GP_REG_FIRST))
          {
            mips_save_restore_reg (gpr_mode, regno, offset, mips_restore_reg);
            offset -= GET_MODE_SIZE (gpr_mode);
          }

      /* Restore registers and deallocate final bit of frame. */
      emit_insn (gen_restore_mips16e (GEN_INT (step2)));
    }
  else
    {
      /* Restore the registers.  */
      mips_for_each_saved_reg (cfun->machine->frame.total_size - step2,
                               mips_restore_reg);



      /* Deallocate the final bit of the frame for non-interrupts.  */
      if (!cfun->machine->frame.has_interrupt_context && step2 > 0)
        {
          emit_insn (gen_add3_insn (stack_pointer_rtx,
                stack_pointer_rtx,
                GEN_INT (step2)));
        }
    }

  /* Add in the __builtin_eh_return stack adjustment.   We need to
     use a temporary in mips16 code.  */
  if (current_function_calls_eh_return)
    {
      if (TARGET_MIPS16)
        {
          emit_move_insn (MIPS_EPILOGUE_TEMP (Pmode), stack_pointer_rtx);
          emit_insn (gen_add3_insn (MIPS_EPILOGUE_TEMP (Pmode),
                                    MIPS_EPILOGUE_TEMP (Pmode),
                                    EH_RETURN_STACKADJ_RTX));
          emit_move_insn (stack_pointer_rtx, MIPS_EPILOGUE_TEMP (Pmode));
        }
      else
        emit_insn (gen_add3_insn (stack_pointer_rtx,
                                  stack_pointer_rtx,
                                  EH_RETURN_STACKADJ_RTX));
    }

  if (!sibcall_p)
    {
      /* The mips16 loads the return address into $7, not $31.  */
      /* But, mips16e can load to $31 */
      if (TARGET_MIPS16 && (cfun->machine->frame.mask & RA_MASK) != 0
          && !TARGET_M16E_SAVE)
        emit_jump_insn (gen_return_internal (gen_rtx (REG, Pmode,
                                                      GP_REG_FIRST + 7)));
      else
        {
          /* non-interrupt functions generate a return instruction here */
          if (current_function_type == NON_INTERRUPT)
            emit_jump_insn (gen_return_internal (gen_rtx (REG, Pmode,
                    GP_REG_FIRST + 31)));
          /* interrupt functions need to emit a placeholder instruction
             so that this pattern will return a non-empty expansion.
             we don't want anything below this (there shouldn't be any
             RTL following this, anyway) moved above it, so we'll use a
             blockage. */
          else
            emit_insn (gen_blockage ());
        }
    }
}

/* Return nonzero if this function is known to have a null epilogue.
   This allows the optimizer to omit jumps to jumps if no stack
   was created.  */

int
mips_can_use_return_insn (void)
{
  tree return_type;

  /* interrupt handlers need to go through the epilogue */
  if (current_function_type != NON_INTERRUPT)
    return 0;

  /* naked functions don't emit a return instruction directly.
     since the prologue/epilogue is user-generated, we can't make
     assumptions about what optimizations might be OK. */
  if (mchp_function_naked_p (current_function_decl))
    return 0;

  if (!reload_completed)
    return 0;

  if (regs_ever_live[31] || current_function_profile)
    return 0;

  return_type = DECL_RESULT (current_function_decl);

  /* In mips16 mode, a function which returns a floating point value
     needs to arrange to copy the return value into the floating point
     registers.  */
  if (TARGET_MIPS16
      && mips16_hard_float
      && ! aggregate_value_p (return_type, current_function_decl)
      && MIPS_RETURN_FP_REG_P (DECL_MODE (return_type)))
    return 0;

  if (cfun->machine->frame.initialized)
    return cfun->machine->frame.total_size == 0;

  return compute_frame_size (get_frame_size ()) == 0;
}

/* Implement TARGET_ASM_OUTPUT_MI_THUNK.  Generate rtl rather than asm text
   in order to avoid duplicating too much logic from elsewhere.  */

static void
mips_output_mi_thunk (FILE *file, tree thunk_fndecl ATTRIBUTE_UNUSED,
                      HOST_WIDE_INT delta, HOST_WIDE_INT vcall_offset,
                      tree function)
{
  rtx this, temp1, temp2, insn, fnaddr;

  /* Pretend to be a post-reload pass while generating rtl.  */
  no_new_pseudos = 1;
  reload_completed = 1;

  /* Pick a global pointer for -mabicalls.  Use $15 rather than $28
     for TARGET_NEWABI since the latter is a call-saved register.  */
  if (TARGET_ABICALLS)
    cfun->machine->global_pointer
      = REGNO (pic_offset_table_rtx)
      = TARGET_NEWABI ? 15 : GLOBAL_POINTER_REGNUM;

  /* Set up the global pointer for n32 or n64 abicalls.  */
  mips_emit_loadgp ();

  /* We need two temporary registers in some cases.  */
  temp1 = gen_rtx_REG (Pmode, 2);
  temp2 = gen_rtx_REG (Pmode, 3);

  /* Find out which register contains the "this" pointer.  */
  if (aggregate_value_p (TREE_TYPE (TREE_TYPE (function)), function))
    this = gen_rtx_REG (Pmode, GP_ARG_FIRST + 1);
  else
    this = gen_rtx_REG (Pmode, GP_ARG_FIRST);

  /* Add DELTA to THIS.  */
  if (delta != 0)
    {
      rtx offset = GEN_INT (delta);
      if (!SMALL_OPERAND (delta))
        {
          emit_move_insn (temp1, offset);
          offset = temp1;
        }
      emit_insn (gen_add3_insn (this, this, offset));
    }

  /* If needed, add *(*THIS + VCALL_OFFSET) to THIS.  */
  if (vcall_offset != 0)
    {
      rtx addr;

      /* Set TEMP1 to *THIS.  */
      emit_move_insn (temp1, gen_rtx_MEM (Pmode, this));

      /* Set ADDR to a legitimate address for *THIS + VCALL_OFFSET.  */
      addr = mips_add_offset (temp2, temp1, vcall_offset);

      /* Load the offset and add it to THIS.  */
      emit_move_insn (temp1, gen_rtx_MEM (Pmode, addr));
      emit_insn (gen_add3_insn (this, this, temp1));
    }

  /* Jump to the target function.  Use a sibcall if direct jumps are
     allowed, otherwise load the address into a register first.  */
  fnaddr = XEXP (DECL_RTL (function), 0);
  if (!TARGET_SIBCALLS || TARGET_ABICALLS
      || !flag_optimize_sibling_calls
      || SYMBOL_REF_MIPS16_P (fnaddr)
      || SYMBOL_REF_LONGCALL_P (fnaddr))
    {
      /* This is messy.  gas treats "la $25,foo" as part of a call
         sequence and may allow a global "foo" to be lazily bound.
         The general move patterns therefore reject this combination.

         In this context, lazy binding would actually be OK for o32 and o64,
         but it's still wrong for n32 and n64; see mips_load_call_address.
         We must therefore load the address via a temporary register if
         mips_dangerous_for_la25_p.

         If we jump to the temporary register rather than $25, the assembler
         can use the move insn to fill the jump's delay slot.  */
      if (TARGET_ABICALLS && !mips_dangerous_for_la25_p (fnaddr))
        temp1 = gen_rtx_REG (Pmode, PIC_FUNCTION_ADDR_REGNUM);
      mips_load_call_address (temp1, fnaddr, true);

      if (TARGET_ABICALLS && REGNO (temp1) != PIC_FUNCTION_ADDR_REGNUM)
        emit_move_insn (gen_rtx_REG (Pmode, PIC_FUNCTION_ADDR_REGNUM), temp1);
      emit_jump_insn (gen_indirect_jump (temp1));
    }
  else
    {
      insn = emit_call_insn (gen_sibcall_internal (fnaddr, const0_rtx));
      SIBLING_CALL_P (insn) = 1;
    }

  /* Run just enough of rest_of_compilation.  This sequence was
     "borrowed" from alpha.c.  */
  insn = get_insns ();
  insn_locators_initialize ();
  split_all_insns_noflow ();
  shorten_branches (insn);
  final_start_function (insn, file, 1);
  final (insn, file, 1, 0);
  final_end_function ();

  /* Clean up the vars set above.  Note that final_end_function resets
     the global pointer for us.  */
  reload_completed = 0;
  no_new_pseudos = 0;
}

/* Returns nonzero if X contains a SYMBOL_REF.  */

static int
symbolic_expression_p (rtx x, rtx *sym)
{
  if (GET_CODE (x) == SYMBOL_REF)
    {
      *sym = x;
      return 1;
    }

  if (GET_CODE (x) == CONST)
    return symbolic_expression_p (XEXP (x, 0), sym);

  if (GET_RTX_CLASS (GET_CODE (x)) == '1')
    return symbolic_expression_p (XEXP (x, 0), sym);

  if (GET_RTX_CLASS (GET_CODE (x)) == 'c'
      || GET_RTX_CLASS (GET_CODE (x)) == '2')
    return (symbolic_expression_p (XEXP (x, 0), sym)
            || symbolic_expression_p (XEXP (x, 1), sym));

  return 0;
}

/* Choose the section to use for the constant rtx expression X that has
   mode MODE.  */

static void
mips_select_rtx_section (enum machine_mode mode, rtx x,
                         unsigned HOST_WIDE_INT align)
{
  /* For mips16 code, we make sure data in code is true.  */
  if (TARGET_MIPS16 && TARGET_DATA_IN_CODE)
    {
      /* In mips16 mode, the constant table always goes in the same section
         as the function, so that constants can be loaded using PC relative
         addressing.  */
      function_section (current_function_decl);
    }
  else if (TARGET_EMBEDDED_DATA)
    {
      /* For embedded applications, always put constants in read-only data,
         in order to reduce RAM usage.  */
      mergeable_constant_section (mode, align, 0);
    }
  else
    {
      /* For hosted applications, always put constants in small data if
         possible, as this gives the best performance.  */
      /* ??? Consider using mergeable small data sections.  */
      rtx sym;

      /* If no data in code and gp optimization, we will use .sdata.  */
      if ((GET_MODE_SIZE (mode) <= (unsigned) mips_section_threshold
           || TARGET_NO_DATA_IN_CODE)
          && mips_section_threshold > 0)
        named_section (0, ".sdata", 0);
      else if (flag_pic && symbolic_expression_p (x, &sym))
        {
          if (targetm.have_named_sections)
            named_section (0, ".data.rel.ro", 3);
          else
            data_section ();
        }
      else
        mergeable_constant_section (mode, align, 0);
    }
}

/* Choose the section to use for DECL.  RELOC is true if its value contains
   any relocatable expression.  */

static void
mips_select_section (tree decl, int reloc,
                     unsigned HOST_WIDE_INT align ATTRIBUTE_UNUSED)
{
  if ((TARGET_EMBEDDED_PIC
       || (TARGET_MIPS16 && TARGET_DATA_IN_CODE && ! TARGET_CODE_XONLY
           && current_function_decl))
      && TREE_CODE (decl) == STRING_CST
      && !flag_writable_strings)
    /* For embedded position independent code, put constant strings in the
       text section, because the data section is limited to 64K in size.
       For mips16 code, put strings in the text section so that a PC
       relative load instruction can be used to get their address.
       Note that for mips16 code, we make sure data in code is true and
       execute-only code is false.  */
    if (current_function_decl)
      function_section (current_function_decl);
    else
      text_section ();
  else if (targetm.have_named_sections)
    default_elf_select_section (decl, reloc, align);
  else
    /* The native irix o32 assembler doesn't support named sections.  */
    default_select_section (decl, reloc, align);
}


/* Implement TARGET_IN_SMALL_DATA_P.  Return true if it would be safe to
   access DECL using %gp_rel(...)($gp).  */

static bool
mips_in_small_data_p (tree decl)
{
  HOST_WIDE_INT size;

  if (TREE_CODE (decl) == STRING_CST || TREE_CODE (decl) == FUNCTION_DECL)
    return false;

  /* We don't yet generate small-data references for -mabicalls.  See related
     -G handling in override_options.  */
  if (TARGET_ABICALLS)
    return false;

  if (TREE_CODE (decl) == VAR_DECL && DECL_SECTION_NAME (decl) != 0)
    {
      const char *name;

      /* Reject anything that isn't in a known small-data section.  */
      name = TREE_STRING_POINTER (DECL_SECTION_NAME (decl));
      if (strcmp (name, ".sdata") != 0 && strcmp (name, ".sbss") != 0)
        return false;

      /* If a symbol is defined externally, the assembler will use the
         usual -G rules when deciding how to implement macros.  */
      if (TARGET_EXPLICIT_RELOCS || !DECL_EXTERNAL (decl))
        return true;
    }
  else if (TARGET_EMBEDDED_DATA)
    {
      /* Don't put constants into the small data section: we want them
         to be in ROM rather than RAM.  */
      if (TREE_CODE (decl) != VAR_DECL)
        return false;

      if (TREE_READONLY (decl)
          && !TREE_SIDE_EFFECTS (decl)
          && (!DECL_INITIAL (decl)
              || DECL_INITIAL (decl) == error_mark_node
              || TREE_CONSTANT (DECL_INITIAL (decl))))
        return false;
    }

  if (DECL_EXTERNAL (decl)
      && TREE_READONLY (decl))
    return false;

  if (DECL_ARTIFICIAL (decl)
      || (TREE_CODE (decl) == VAR_DECL
	  && DECL_CONTEXT (decl)
	  && TREE_CODE (DECL_CONTEXT (decl)) == NAMESPACE_DECL
	  && strcmp (DECL_SOURCE_FILE (DECL_CONTEXT (decl)), "<internal>") == 0))
    return false;

  if (TARGET_MIPS16 && TARGET_NO_DATA_IN_CODE)
    {
      if (TREE_READONLY (decl)
          && !TREE_SIDE_EFFECTS (decl)
          && (!DECL_INITIAL (decl)
              || DECL_INITIAL (decl) == error_mark_node
              || TREE_CONSTANT (DECL_INITIAL (decl))))
        return false;

      /* We don't put external weak or link once symbols in small data area.  */
      if (DECL_EXTERNAL (decl) && (DECL_WEAK (decl) || DECL_ONE_ONLY (decl)))
        return false;
    }

  size = int_size_in_bytes (TREE_TYPE (decl));
  return (size > 0 && size <= mips_section_threshold);
}


/* When generating embedded PIC code, SYMBOL_REF_FLAG is set for
   symbols which are not in the .text section.

   When generating mips16 code, SYMBOL_REF_FLAG is set for string
   constants which are put in the .text section.  We also record the
   total length of all such strings; this total is used to decide
   whether we need to split the constant table, and need not be
   precisely correct.  */

static void
mips_encode_section_info (tree decl, rtx rtl, int first)
{
  rtx symbol;

  if (GET_CODE (rtl) != MEM)
    return;

  symbol = XEXP (rtl, 0);

  if (GET_CODE (symbol) != SYMBOL_REF)
    return;

  /* Now that we've forced global string literals to .rodata, we will
     always force the string addresses in the constant pool through
     indirection. */
  if (TARGET_MIPS16 && TARGET_DATA_IN_CODE && !TARGET_CODE_XONLY)
    {
      if (first && TREE_CODE (decl) == STRING_CST
          && ! flag_writable_strings
          /* If this string is from a function, and the function will
             go in a gnu linkonce section, then we can't directly
             access the string.  This gets an assembler error
             "unsupported PC relative reference to different section".
             If we modify SELECT_SECTION to put it in function_section
             instead of text_section, it still fails because
             DECL_SECTION_NAME isn't set until assemble_start_function.
             If we fix that, it still fails because strings are shared
             among multiple functions, and we have cross section
             references again.  We force it to work by putting string
             addresses in the constant pool and indirecting.  */
          && (current_function_decl
              && ! DECL_ONE_ONLY (current_function_decl)))
        {
          cfun->machine->mips16_strings =
            alloc_EXPR_LIST (0, symbol, cfun->machine->mips16_strings);
          cfun->machine->mips_string_length += TREE_STRING_LENGTH (decl);
          SYMBOL_REF_FLAG (symbol) = 1;
        }
    }

  if (TARGET_EMBEDDED_PIC)
    {
      if (TREE_CODE (decl) == VAR_DECL)
        SYMBOL_REF_FLAG (symbol) = 1;
      else if (TREE_CODE (decl) == FUNCTION_DECL)
        SYMBOL_REF_FLAG (symbol) = 0;
      else if (TREE_CODE (decl) == STRING_CST
               && ! flag_writable_strings)
        SYMBOL_REF_FLAG (symbol) = 0;
      else
        SYMBOL_REF_FLAG (symbol) = 1;
    }

  default_encode_section_info (decl, rtl, first);

  if (TREE_CODE (decl) == FUNCTION_DECL)
    {
      tree func_attr = TYPE_ATTRIBUTES (TREE_TYPE (decl));
      int is_longcall = TARGET_LONG_CALLS;
      int is_mips16 = mips_base_mips16;

      if (lookup_attribute ("far", func_attr) != NULL_TREE
          || lookup_attribute ("longcall", func_attr) != NULL_TREE)
        is_longcall = 1;
      else if (lookup_attribute ("near", func_attr) != NULL_TREE)
        is_longcall = 0;
      if (is_longcall)
        SYMBOL_REF_FLAGS (symbol) |= SYMBOL_FLAG_LONGCALL;

      if (lookup_attribute ("mips16", func_attr) != NULL_TREE)
        is_mips16 = 1;
      else if (lookup_attribute ("nomips16", func_attr) != NULL_TREE)
        is_mips16 = 0;
      else if (mips_flip_mips16_string && !DECL_BUILT_IN (decl)
               && !DECL_ARTIFICIAL (decl))
        {
          if (first)
            {
              /* Debug: flip MIPS16 on each function. */
              mips16_flipper = ! mips16_flipper;
              if (mips16_flipper)
                is_mips16 = ! is_mips16;
            }
          else
            /* Don't flip after first again. */
            is_mips16 = SYMBOL_REF_MIPS16_P (symbol);
        }

      /* If there was an explicit -mno-mips16, then prevent MIPS16
         selection, even when an explicit attribute is given. */
      if ((target_flags_explicit & MASK_MIPS16)
          && mips_base_mips16 == 0)
        is_mips16 = 0;

      if (is_mips16 && (flag_pic || TARGET_ABICALLS))
        {
          warning ("%J%s is not supported with mips16"
                   " attribute on '%F', attribute ignored",
                   decl,
                   (flag_pic > 1 ? "-fPIC"
                    : flag_pic ? "-fpic"
                    : "-mabicalls"),
                   decl);
          is_mips16 = 0;
        }
#ifdef TARGET_MCHP_PIC32MX
      /* Prevent MIPS16 selection via explicit attribute without a valid
         license. */
      if (is_mips16 && (pic32_license_valid < 0))
        {
          if (-100 == pic32_license_valid) {
            warning ("The academic edition does not support the mips16"
                     " attribute on  '%F', attribute ignored",
                     decl);
          } else {
            warning ("The current limited compiler license does not support the mips16"
                     " attribute on '%F', attribute ignored",
                     decl);
          }
          is_mips16 = 0;
        }
#endif /* TARGET_MCHP_PIC32MX */

      if (is_mips16)
        SYMBOL_REF_FLAGS (symbol) |= SYMBOL_FLAG_MIPS16;
    }
}

/* See whether VALTYPE is a record whose fields should be returned in
   floating-point registers.  If so, return the number of fields and
   list them in FIELDS (which should have two elements).  Return 0
   otherwise.

   For n32 & n64, a structure with one or two fields is returned in
   floating-point registers as long as every field has a floating-point
   type.  */

static int
mips_fpr_return_fields (tree valtype, tree *fields)
{
  tree field;
  int i;

  if (!TARGET_NEWABI)
    return 0;

  if (TREE_CODE (valtype) != RECORD_TYPE)
    return 0;

  i = 0;
  for (field = TYPE_FIELDS (valtype); field != 0; field = TREE_CHAIN (field))
    {
      if (TREE_CODE (field) != FIELD_DECL)
        continue;

      if (TREE_CODE (TREE_TYPE (field)) != REAL_TYPE)
        return 0;

      if (i == 2)
        return 0;

      fields[i++] = field;
    }
  return i;
}


/* Implement TARGET_RETURN_IN_MSB.  For n32 & n64, we should return
   a value in the most significant part of $2/$3 if:

      - the target is big-endian;

      - the value has a structure or union type (we generalize this to
        cover aggregates from other languages too); and

      - the structure is not returned in floating-point registers.  */

static bool
mips_return_in_msb (tree valtype)
{
  tree fields[2];

  return (TARGET_NEWABI
          && TARGET_BIG_ENDIAN
          && AGGREGATE_TYPE_P (valtype)
          && mips_fpr_return_fields (valtype, fields) == 0);
}


/* Return a composite value in a pair of floating-point registers.
   MODE1 and OFFSET1 are the mode and byte offset for the first value,
   likewise MODE2 and OFFSET2 for the second.  MODE is the mode of the
   complete value.

   For n32 & n64, $f0 always holds the first value and $f2 the second.
   Otherwise the values are packed together as closely as possible.  */

static rtx
mips_return_fpr_pair (enum machine_mode mode,
                      enum machine_mode mode1, HOST_WIDE_INT offset1,
                      enum machine_mode mode2, HOST_WIDE_INT offset2)
{
  int inc;

  inc = (TARGET_NEWABI || ABI_32 || ABI_O64 ? 2 : FP_INC);
  return gen_rtx_PARALLEL
    (mode,
     gen_rtvec (2,
                gen_rtx_EXPR_LIST (VOIDmode,
                                   gen_rtx_REG (mode1, FP_RETURN),
                                   GEN_INT (offset1)),
                gen_rtx_EXPR_LIST (VOIDmode,
                                   gen_rtx_REG (mode2, FP_RETURN + inc),
                                   GEN_INT (offset2))));

}


/* Implement FUNCTION_VALUE and LIBCALL_VALUE.  For normal calls,
   VALTYPE is the return type and MODE is VOIDmode.  For libcalls,
   VALTYPE is null and MODE is the mode of the return value.  */

rtx
mips_function_value (tree valtype, tree func ATTRIBUTE_UNUSED,
                     enum machine_mode mode)
{
  if (valtype)
    {
      tree fields[2];
      int unsignedp;

      mode = TYPE_MODE (valtype);
      unsignedp = TREE_UNSIGNED (valtype);

      /* Since we define PROMOTE_FUNCTION_RETURN, we must promote
         the mode just as PROMOTE_MODE does.  */
      mode = promote_mode (valtype, mode, &unsignedp, 1);

      /* Handle structures whose fields are returned in $f0/$f2.  */
      switch (mips_fpr_return_fields (valtype, fields))
        {
        case 1:
          return gen_rtx_REG (mode, FP_RETURN);

        case 2:
          return mips_return_fpr_pair (mode,
                                       TYPE_MODE (TREE_TYPE (fields[0])),
                                       int_byte_position (fields[0]),
                                       TYPE_MODE (TREE_TYPE (fields[1])),
                                       int_byte_position (fields[1]));
        }

      /* If a value is passed in the most significant part of a register, see
         whether we have to round the mode up to a whole number of words.  */
      if (mips_return_in_msb (valtype))
        {
          HOST_WIDE_INT size = int_size_in_bytes (valtype);
          if (size % UNITS_PER_WORD != 0)
            {
              size += UNITS_PER_WORD - size % UNITS_PER_WORD;
              mode = mode_for_size (size * BITS_PER_UNIT, MODE_INT, 0);
            }
        }
    }

  if ((GET_MODE_CLASS (mode) == MODE_FLOAT
       || (GET_MODE_CLASS (mode) == MODE_VECTOR_FLOAT
           && TARGET_PAIRED_SINGLE_FLOAT))
      && GET_MODE_SIZE (mode) <= UNITS_PER_HWFPVALUE)
    return gen_rtx_REG (mode, FP_RETURN);

  /* Handle long doubles for n32 & n64.  */
  if (mode == TFmode)
    return mips_return_fpr_pair (mode,
                                 DImode, 0,
                                 DImode, GET_MODE_SIZE (mode) / 2);

  if (GET_MODE_CLASS (mode) == MODE_COMPLEX_FLOAT
      && GET_MODE_SIZE (mode) <= UNITS_PER_HWFPVALUE * 2)
    return mips_return_fpr_pair (mode,
                                 GET_MODE_INNER (mode), 0,
                                 GET_MODE_INNER (mode),
                                 GET_MODE_SIZE (mode) / 2);

  return gen_rtx_REG (mode, GP_RETURN);
}

/* The implementation of FUNCTION_ARG_PASS_BY_REFERENCE.  Return
   nonzero when an argument must be passed by reference.  */

int
function_arg_pass_by_reference (const CUMULATIVE_ARGS *cum ATTRIBUTE_UNUSED,
                                enum machine_mode mode, tree type,
                                int named ATTRIBUTE_UNUSED)
{
  int size;

  /* The EABI is the only one to pass args by reference.  */
  if (mips_abi != ABI_EABI)
    return 0;

  /* ??? How should SCmode be handled?  */
  if (type == NULL_TREE || mode == DImode || mode == DFmode)
    return 0;

  size = int_size_in_bytes (type);
  return size == -1 || size > UNITS_PER_WORD;
}

/* Return the class of registers for which a mode change from FROM to TO
   is invalid.

   In little-endian mode, the hi-lo registers are numbered backwards,
   so (subreg:SI (reg:DI hi) 0) gets the high word instead of the low
   word as intended.

   Similarly, when using paired floating-point registers, the first
   register holds the low word, regardless of endianness.  So in big
   endian mode, (subreg:SI (reg:DF $f0) 0) does not get the high word
   as intended.

   Also, loading a 32-bit value into a 64-bit floating-point register
   will not sign-extend the value, despite what LOAD_EXTEND_OP says.
   We can't allow 64-bit float registers to change from a 32-bit
   mode to a 64-bit mode.  */

bool
mips_cannot_change_mode_class (enum machine_mode from,
                               enum machine_mode to, enum reg_class class)
{
  if (GET_MODE_SIZE (from) != GET_MODE_SIZE (to))
    {
      if (FLOAT_MODE_P (to))
        return true;
      if (TARGET_BIG_ENDIAN || TARGET_FLOAT64)
        return reg_classes_intersect_p (FP_REGS, class);
    }
  return false;
}

/* Return true if X should not be moved directly into register $25.
   We need this because many versions of GAS will treat "la $25,foo" as
   part of a call sequence and so allow a global "foo" to be lazily bound.  */

bool
mips_dangerous_for_la25_p (rtx x)
{
  HOST_WIDE_INT offset;

  if (TARGET_EXPLICIT_RELOCS)
    return false;

  mips_split_const (x, &x, &offset);
  return global_got_operand (x, VOIDmode);
}

/* Implement PREFERRED_RELOAD_CLASS.  */

enum reg_class
mips_preferred_reload_class (rtx x, enum reg_class class)
{
  if (mips_dangerous_for_la25_p (x) && reg_class_subset_p (LEA_REGS, class))
    return LEA_REGS;

  if (TARGET_HARD_FLOAT
      && FLOAT_MODE_P (GET_MODE (x))
      && reg_class_subset_p (FP_REGS, class))
    return FP_REGS;

  if (reg_class_subset_p (GR_REGS, class))
    class = GR_REGS;

  if (TARGET_MIPS16 && reg_class_subset_p (M16_REGS, class))
    class = M16_REGS;

  return class;
}

/* This function returns the register class required for a secondary
   register when copying between one of the registers in CLASS, and X,
   using MODE.  If IN_P is nonzero, the copy is going from X to the
   register, otherwise the register is the source.  A return value of
   NO_REGS means that no secondary register is required.  */

enum reg_class
mips_secondary_reload_class (enum reg_class class,
                             enum machine_mode mode, rtx x, int in_p)
{
  enum reg_class gr_regs = TARGET_MIPS16 ? M16_REGS : GR_REGS;
  int regno = -1;
  int gp_reg_p;

  if (GET_CODE (x) == REG || GET_CODE (x) == SUBREG)
    regno = true_regnum (x);

  gp_reg_p = TARGET_MIPS16 ? M16_REG_P (regno) : GP_REG_P (regno);

  if (mips_dangerous_for_la25_p (x))
    {
      gr_regs = LEA_REGS;
      if (TEST_HARD_REG_BIT (reg_class_contents[(int) class], 25))
        return gr_regs;
    }

  /* Copying from HI or LO to anywhere other than a general register
     requires a general register.  */
  if (class == MD0_REG || class == MD1_REG || class == MD_REGS ||
      class == DSP_ACC_REGS || class == MD_AND_DSP_ACC_REGS)
    {
      if (TARGET_MIPS16 && in_p)
        {
          /* We can't really copy to HI or LO at all in mips16 mode.  */
          return M16_REGS;
        }
      return gp_reg_p ? NO_REGS : gr_regs;
    }
  if (MD_REG_P (regno) || DSP_ACC_REG_P (regno))
    {
      if (TARGET_MIPS16 && ! in_p)
        {
          /* We can't really copy to HI or LO at all in mips16 mode.  */
          return M16_REGS;
        }
      return class == gr_regs ? NO_REGS : gr_regs;
    }

  /* We can only copy a value to a condition code register from a
     floating point register, and even then we require a scratch
     floating point register.  We can only copy a value out of a
     condition code register into a general register.  */
  if (class == ST_REGS)
    {
      if (in_p)
        return FP_REGS;
      return gp_reg_p ? NO_REGS : gr_regs;
    }
  if (ST_REG_P (regno))
    {
      if (! in_p)
        return FP_REGS;
      return class == gr_regs ? NO_REGS : gr_regs;
    }

  if (class == FP_REGS)
    {
      if (GET_CODE (x) == MEM)
        {
          /* In this case we can use lwc1, swc1, ldc1 or sdc1.  */
          return NO_REGS;
        }
      else if (CONSTANT_P (x) && GET_MODE_CLASS (mode) == MODE_FLOAT)
        {
          /* We can use the l.s and l.d macros to load floating-point
             constants.  ??? For l.s, we could probably get better
             code by returning GR_REGS here.  */
          return NO_REGS;
        }
      else if (gp_reg_p || x == CONST0_RTX (mode))
        {
          /* In this case we can use mtc1, mfc1, dmtc1 or dmfc1.  */
          return NO_REGS;
        }
      else if (FP_REG_P (regno))
        {
          /* In this case we can use mov.s or mov.d.  */
          return NO_REGS;
        }
      else
        {
          /* Otherwise, we need to reload through an integer register.  */
          return gr_regs;
        }
    }

  /* In mips16 mode, going between memory and anything but M16_REGS
     requires an M16_REG.  */
  if (TARGET_MIPS16)
    {
      if (class != M16_REGS && class != M16_NA_REGS)
        {
          if (gp_reg_p)
            return NO_REGS;
          return M16_REGS;
        }
      if (! gp_reg_p)
        {
          if (class == M16_REGS || class == M16_NA_REGS)
            return NO_REGS;
          return M16_REGS;
        }
    }

  return NO_REGS;
}

/* Implement CLASS_MAX_NREGS.

   Usually all registers are word-sized.  The only supported exception
   is -mgp64 -msingle-float, which has 64-bit words but 32-bit float
   registers.  A word-based calculation is correct even in that case,
   since -msingle-float disallows multi-FPR values.  */

int
mips_class_max_nregs (enum reg_class class ATTRIBUTE_UNUSED,
                      enum machine_mode mode)
{
  if (mode == CCV2mode)
    return 2;
  else if (mode == CCV4mode)
    return 4;
  else
    return (GET_MODE_SIZE (mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD;
}

bool
mips_valid_pointer_mode (enum machine_mode mode)
{
  return (mode == SImode || (TARGET_64BIT && mode == DImode));
}


/* If we can access small data directly (using gp-relative relocation
   operators) return the small data pointer, otherwise return null.

   For each mips16 function which refers to GP relative symbols, we
   use a pseudo register, initialized at the start of the function, to
   hold the $gp value.  */

static rtx
mips16_gp_pseudo_reg (void)
{
  if (cfun->machine->mips16_gp_pseudo_rtx == NULL_RTX)
    {
      rtx unspec;
      rtx insn, scan;
      rtx line_number_note = NULL_RTX;
      rtx note;

      cfun->machine->mips16_gp_pseudo_rtx = gen_reg_rtx (Pmode);
      RTX_UNCHANGING_P (cfun->machine->mips16_gp_pseudo_rtx) = 1;

      /* We want to initialize this to a value which gcc will believe
         is constant.  */
      start_sequence ();
      unspec = gen_rtx_UNSPEC (VOIDmode, gen_rtvec (1, const0_rtx), UNSPEC_GP);
      emit_move_insn (cfun->machine->mips16_gp_pseudo_rtx,
                      gen_rtx_CONST (Pmode, unspec));
      insn = get_insns ();
      end_sequence ();

      push_topmost_sequence ();
      /* We need to emit the initialization after the first
         NOTE_INSN_BLOCK_BEG note (instead of after NOTE_INSN_FUNCTION_BEG),
         so that it will be integrated.  */
      for (scan = get_insns (); scan != NULL_RTX; scan = NEXT_INSN (scan))
        if (GET_CODE (scan) == NOTE
            && NOTE_LINE_NUMBER (scan) == NOTE_INSN_BLOCK_BEG)
          break;

      /* Search for any line number note between NOTE_INSN_BLOCK_BEG and the
         first real instruction.  We want to know that we are not inserting
         before this.  There could be line number notes before
         NOTE_INSN_FUNCTION_BEG or the first NOTE_INSN_BLOCK_BEG, but we ignore
         these and use the nearest line number to the first actual instruction.
      */
      for (note = scan; note != NULL_RTX; note = NEXT_INSN (note))
        {
          /* Jump out if we reach an actual instruction. */
          if (GET_CODE (note) != NOTE)
            break;
          else
            if (NOTE_LINE_NUMBER (note) > 0)
              line_number_note = note;
        }
      /* If we still haven't found any line number notes, keep searching until
         we find the first one. */
      if (line_number_note == NULL_RTX)
        for (; note != NULL_RTX; note = NEXT_INSN (note))
          if (GET_CODE (note) == NOTE && NOTE_LINE_NUMBER (note) > 0)
            {
              line_number_note = note;
              break;
            }

      if (scan == NULL_RTX)
        scan = get_insns ();
      insn = emit_insn_after (insn, scan);
      if (line_number_note != NULL_RTX)
        emit_note_copy_after (line_number_note, scan);
      pop_topmost_sequence ();
    }

  return cfun->machine->mips16_gp_pseudo_rtx;
}

/* Write out code to move floating point arguments in or out of
   general registers.  Output the instructions to FILE.  FP_CODE is
   the code describing which arguments are present (see the comment at
   the definition of CUMULATIVE_ARGS in mips.h).  FROM_FP_P is nonzero if
   we are copying from the floating point registers.  */

static void
mips16_fp_args (FILE *file, int fp_code, int from_fp_p)
{
  char dir;
  int gparg, fparg;
  unsigned int f;

  /* This code only works for the original 32 bit ABI and the O64 ABI.  */
  if (mips_abi != ABI_32 && mips_abi != ABI_O64)
    abort ();

  if (from_fp_p)
    dir = 'f';
  else
    dir = 't';
  gparg = GP_ARG_FIRST;
  fparg = FP_ARG_FIRST;
  for (f = (unsigned int) fp_code; f != 0; f >>= 2)
    {
      if ((f & 3) == 1) /* SFmode */
        {
          fprintf (file, "\tm%cc1\t%s,%s\n", dir,
                   reg_names[gparg], reg_names[fparg]);
          fparg += 2;
          gparg += 1;
        }
      else if ((f & 3) == 2) /* DFmode */
        {
          if (TARGET_64BIT)
            fprintf (file, "\tdm%cc1\t%s,%s\n", dir,
                     reg_names[gparg], reg_names[fparg]);
          else
            {
              if (gparg % 2 != 0)
                ++gparg;
              if (TARGET_FLOAT64 && ISA_HAS_MXHC1)
                {
                  /* -mips32r2 -mfp64 */
                  fprintf (file, "\tm%cc1\t%s,%s\n\tm%chc1\t%s,%s\n",
                           dir,
                           reg_names[gparg + (WORDS_BIG_ENDIAN ? 1 : 0)],
                           reg_names[fparg],
                           dir,
                           reg_names[gparg + (WORDS_BIG_ENDIAN ? 0 : 1)],
                           reg_names[fparg]);
;
                }
              else if (TARGET_BIG_ENDIAN)
                fprintf (file, "\tm%cc1\t%s,%s\n\tm%cc1\t%s,%s\n", dir,
                         reg_names[gparg], reg_names[fparg + 1], dir,
                         reg_names[gparg + 1], reg_names[fparg]);
              else
                fprintf (file, "\tm%cc1\t%s,%s\n\tm%cc1\t%s,%s\n", dir,
                         reg_names[gparg], reg_names[fparg], dir,
                         reg_names[gparg + 1], reg_names[fparg + 1]);
            }
          fparg += 2;
          gparg += GET_MODE_SIZE (DFmode) / UNITS_PER_WORD;
        }
      else
        abort ();
    }
}


/* We keep 2 list of functions for which we have already built fn_stubs
   and call_stubs.  */

struct mips16_stub
{
  struct mips16_stub *next;
  char *name;
  int fpret;
};

static struct mips16_stub *mips16_fn_stubs;
static struct mips16_stub *mips16_call_stubs;

/* Build a mips16 function stub.  This is used for functions which
   take arguments in the floating point registers.  It is 32 bit code
   that moves the floating point args into the general registers, and
   then jumps to the 16 bit code.  */

static void
build_mips16_function_stub (FILE *file, tree fn_decl, int fp_code)
{
  const char *fnname;
  char *secname, *stubname;
  tree stubid, stubdecl;
  int need_comma;
  unsigned int f;
  struct mips16_stub *l;

  if (DECL_ASSEMBLER_NAME_SET_P (fn_decl))
    {
      fnname = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (fn_decl));
      /* Check for user assembler names.  */
      if (fnname[0] == '*')
        {
          fnname = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (fn_decl)) + 1;
          /* Do we care about prefix such as -fleading-underscore?
             If so, handle user_label_prefix.  */
        }
    }
  else
    fnname = XSTR (XEXP (DECL_RTL (fn_decl), 0), 0);
  secname = (char *) alloca (strlen (fnname) + 20);
  sprintf (secname, ".mips16.fn.%s", fnname);
  stubname = (char *) alloca (strlen (fnname) + 20);
  sprintf (stubname, "__fn_stub_%s", fnname);
  stubid = get_identifier (stubname);

  for (l = mips16_fn_stubs; l != NULL; l = l->next)
    if (strcmp (l->name, fnname) == 0)
      break;

  if (l != NULL)
    return;

  stubdecl = build_decl (FUNCTION_DECL, stubid,
                         build_function_type (void_type_node, NULL_TREE));
  DECL_SECTION_NAME (stubdecl) = build_string (strlen (secname), secname);
  DECL_RESULT (stubdecl) = build_decl (RESULT_DECL, NULL_TREE, void_type_node);
  DECL_CONTEXT (DECL_RESULT (stubdecl)) = stubdecl;
  DECL_UNINLINABLE (stubdecl) = 1;

  fprintf (file, "\t# Stub function for %s (",
           lang_hooks.decl_printable_name (fn_decl, 2));
  need_comma = 0;
  for (f = (unsigned int) fp_code; f != 0; f >>= 2)
    {
      fprintf (file, "%s%s",
               need_comma ? ", " : "",
               (f & 3) == 1 ? "float" : "double");
      need_comma = 1;
    }
  fprintf (file, ")\n");

  fprintf (file, "\t.set\tnomips16\n");
  function_section (stubdecl);
  ASM_OUTPUT_ALIGN (file, floor_log2 (FUNCTION_BOUNDARY / BITS_PER_UNIT));

  /* ??? If FUNCTION_NAME_ALREADY_DECLARED is defined, then we are
     within a .ent, and we can not emit another .ent.  */
  if (!FUNCTION_NAME_ALREADY_DECLARED)
    {
      fputs ("\t.ent\t", file);
      assemble_name (file, stubname);
      fputs ("\n", file);
    }

  assemble_name (file, stubname);
  fputs (":\n", file);

  /* We don't want the assembler to insert any nops here.  */
  fprintf (file, "\t.set\tnoreorder\n");

  mips16_fp_args (file, fp_code, 1);

  fprintf (asm_out_file, "\t.set\tnoat\n");
  fprintf (asm_out_file, "\tla\t%s,", reg_names[GP_REG_FIRST + 1]);
  assemble_name (file, fnname);
  fprintf (file, "\n");
  fprintf (asm_out_file, "\tjr\t%s\n", reg_names[GP_REG_FIRST + 1]);
  fprintf (asm_out_file, "\t.set\tat\n");

  /* Unfortunately, we can't fill the jump delay slot.  We can't fill
     with one of the mfc1 instructions, because the result is not
     available for one instruction, so if the very first instruction
     in the function refers to the register, it will see the wrong
     value.  */
  fprintf (file, "\tnop\n");

  fprintf (file, "\t.set\treorder\n");

  if (!FUNCTION_NAME_ALREADY_DECLARED)
    {
      fputs ("\t.end\t", file);
      assemble_name (file, stubname);
      fputs ("\n", file);
    }

  function_section (current_function_decl);

  /* Record function stub.  */
  l = (struct mips16_stub *) xmalloc (sizeof *l);
  l->name = xstrdup (fnname);
  l->fpret = -1;
  l->next = mips16_fn_stubs;
  mips16_fn_stubs = l;
}


static void
mips16_fpret_double (int gpreg, int fpreg)
{
  if (TARGET_64BIT && TARGET_FLOAT64)
    fprintf (asm_out_file, "\tdmfc1\t%s,%s\n",
             reg_names[gpreg], reg_names[fpreg]);
  else if (TARGET_FLOAT64 && ISA_HAS_MXHC1)
    {
      fprintf (asm_out_file, "\tmfc1\t%s,%s\n",
               reg_names[gpreg + WORDS_BIG_ENDIAN],
               reg_names[fpreg]);
      fprintf (asm_out_file, "\tmfhc1\t%s,%s\n",
               reg_names[gpreg + ! WORDS_BIG_ENDIAN],
               reg_names[fpreg]);
    }
  else
    {
      if (TARGET_BIG_ENDIAN)
        {
          fprintf (asm_out_file, "\tmfc1\t%s,%s\n",
                   reg_names[gpreg + 0],
                   reg_names[fpreg + 1]);
          fprintf (asm_out_file, "\tmfc1\t%s,%s\n",
                   reg_names[gpreg + 1],
                   reg_names[fpreg + 0]);
        }
      else
        {
          fprintf (asm_out_file, "\tmfc1\t%s,%s\n",
                   reg_names[gpreg + 0],
                   reg_names[fpreg + 0]);
          fprintf (asm_out_file, "\tmfc1\t%s,%s\n",
                   reg_names[gpreg + 1],
                   reg_names[fpreg + 1]);
        }
    }
}


/* Build a call stub for a mips16 call.  A stub is needed if we are
   passing any floating point values which should go into the floating
   point registers.  If we are, and the call turns out to be to a 32
   bit function, the stub will be used to move the values into the
   floating point registers before calling the 32 bit function.  The
   linker will magically adjust the function call to either the 16 bit
   function or the 32 bit stub, depending upon where the function call
   is actually defined.

   Similarly, we need a stub if the return value might come back in a
   floating point register.

   RETVAL is the location of the return value, or null if this is
   a call rather than a call_value.  FN is the address of the
   function and ARG_SIZE is the size of the arguments.  FP_CODE
   is the code built by function_arg.  This function returns a nonzero
   value if it builds the call instruction itself.  */

int
build_mips16_call_stub (rtx retval, rtx fn, rtx arg_size, int fp_code)
{
  int fpret = 0;
  const char *fnname;
  char *secname, *stubname;
  struct mips16_stub *l;
  tree stubid, stubdecl;
  int need_comma;
  unsigned int f;

  /* We don't need to do anything if we aren't in mips16 mode, or if
     we were invoked with the -msoft-float option.  */
  if (! TARGET_MIPS16 || ! mips16_hard_float)
    return 0;

  /* Figure out whether the value might come back in a floating point
     register.  */
  if (retval)
    fpret = MIPS_RETURN_FP_REG_P (GET_MODE (retval));

  /* We don't need to do anything if there were no floating point
     arguments and the value will not be returned in a floating point
     register.  */
  if (fp_code == 0 && ! fpret)
    return 0;

  /* We don't need to do anything if this is a call to a special
     mips16 support function.  */
  if (GET_CODE (fn) == SYMBOL_REF
      && strncmp (XSTR (fn, 0), "__mips16_", 9) == 0)
    return 0;

  /* This code will only work for o32 and o64 abis.  The other ABI's
     require more sophisticated support.  */
  if (mips_abi != ABI_32 && mips_abi != ABI_O64)
    abort ();

  /* We can only handle SFmode, DFmode, SCmode and DCmode floating point return
     values.  */
  if (fpret && GET_MODE (retval) != SFmode && GET_MODE (retval) != DFmode
      && GET_MODE (retval) != SCmode && GET_MODE (retval) != DCmode)
    abort ();

  /* If we're calling via a function pointer, then we must always call
     via a stub.  There are magic stubs provided in libgcc.a for each
     of the required cases.  Each of them expects the function address
     to arrive in register $2.  */

  if (GET_CODE (fn) != SYMBOL_REF)
    {
      char buf[30];
      tree id;
      rtx stub_fn, insn;

      /* ??? If this code is modified to support other ABI's, we need
         to handle PARALLEL return values here.  */

      if (fpret)
        sprintf (buf, "__mips16_call_stub_%s_%d",
                 MIPS_RETURN_FP_REG_TYPE (GET_MODE (retval)),
                 fp_code);
      else
        sprintf (buf, "__mips16_call_stub_%d",
                 fp_code);
      id = get_identifier (buf);
      stub_fn = gen_rtx (SYMBOL_REF, Pmode, IDENTIFIER_POINTER (id));

      emit_move_insn (gen_rtx (REG, Pmode, 2), fn);

      if (retval == NULL_RTX)
        insn = gen_call_internal (stub_fn, arg_size);
      else
        insn = gen_call_value_internal (retval, stub_fn, arg_size);
      insn = emit_call_insn (insn);

      /* Put the register usage information on the CALL.  */
      if (GET_CODE (insn) != CALL_INSN)
        abort ();
      CALL_INSN_FUNCTION_USAGE (insn) =
        gen_rtx (EXPR_LIST, VOIDmode,
                 gen_rtx (USE, VOIDmode, gen_rtx (REG, Pmode, 2)),
                 CALL_INSN_FUNCTION_USAGE (insn));

      /* If we are handling a floating point return value, we need to
         save $18 in the function prologue.  Putting a note on the
         call will mean that regs_ever_live[$18] will be true if the
         call is not eliminated, and we can check that in the prologue
         code.  */
      if (fpret)
        CALL_INSN_FUNCTION_USAGE (insn) =
          gen_rtx (EXPR_LIST, VOIDmode,
                   gen_rtx (USE, VOIDmode, gen_rtx (REG, word_mode, 18)),
                   CALL_INSN_FUNCTION_USAGE (insn));

      /* Return 1 to tell the caller that we've generated the call
         insn.  */
      return 1;
    }

  /* We know the function we are going to call.  If we have already
     built a stub, we don't need to do anything further.  */

  fnname = XSTR (fn, 0);
  for (l = mips16_call_stubs; l != NULL; l = l->next)
    if (strcmp (l->name, fnname) == 0)
      break;

  if (l == NULL)
    {
      /* Build a special purpose stub.  When the linker sees a
         function call in mips16 code, it will check where the target
         is defined.  If the target is a 32 bit call, the linker will
         search for the section defined here.  It can tell which
         symbol this section is associated with by looking at the
         relocation information (the name is unreliable, since this
         might be a static function).  If such a section is found, the
         linker will redirect the call to the start of the magic
         section.

         If the function does not return a floating point value, the
         special stub section is named
             .mips16.call.FNNAME

         If the function does return a floating point value, the stub
         section is named
             .mips16.call.fp.FNNAME
         */

      secname = (char *) alloca (strlen (fnname) + 40);
      sprintf (secname, ".mips16.call.%s%s",
               fpret ? "fp." : "",
               fnname);
      stubname = (char *) alloca (strlen (fnname) + 20);
      sprintf (stubname, "__call_stub_%s%s",
               fpret ? "fp_" : "",
               fnname);
      stubid = get_identifier (stubname);
      stubdecl = build_decl (FUNCTION_DECL, stubid,
                             build_function_type (void_type_node, NULL_TREE));
      DECL_SECTION_NAME (stubdecl) = build_string (strlen (secname), secname);
      DECL_RESULT (stubdecl) = build_decl (RESULT_DECL, NULL_TREE, void_type_node);
      DECL_CONTEXT (DECL_RESULT (stubdecl)) = stubdecl;
      DECL_UNINLINABLE (stubdecl) = 1;

      fprintf (asm_out_file, "\t# Stub function to call %s%s (",
               (fpret
                ? (GET_MODE (retval) == SFmode ? "float " : "double ")
                : ""),
               fnname);
      need_comma = 0;
      for (f = (unsigned int) fp_code; f != 0; f >>= 2)
        {
          fprintf (asm_out_file, "%s%s",
                   need_comma ? ", " : "",
                   (f & 3) == 1 ? "float" : "double");
          need_comma = 1;
        }
      fprintf (asm_out_file, ")\n");

      fprintf (asm_out_file, "\t.set\tnomips16\n");
      assemble_start_function (stubdecl, stubname);

      if (!FUNCTION_NAME_ALREADY_DECLARED)
        {
          fputs ("\t.ent\t", asm_out_file);
          assemble_name (asm_out_file, stubname);
          fputs ("\n", asm_out_file);

          assemble_name (asm_out_file, stubname);
          fputs (":\n", asm_out_file);
        }

      /* We build the stub code by hand.  That's the only way we can
         do it, since we can't generate 32 bit code during a 16 bit
         compilation.  */

      /* We don't want the assembler to insert any nops here.  */
      fprintf (asm_out_file, "\t.set\tnoreorder\n");

      mips16_fp_args (asm_out_file, fp_code, 0);

      if (! fpret)
        {
          fprintf (asm_out_file, "\t.set\tnoat\n");
          fprintf (asm_out_file, "\tla\t%s,%s\n", reg_names[GP_REG_FIRST + 1],
                   fnname);
          fprintf (asm_out_file, "\tjr\t%s\n", reg_names[GP_REG_FIRST + 1]);
          fprintf (asm_out_file, "\t.set\tat\n");
          /* Unfortunately, we can't fill the jump delay slot.  We
             can't fill with one of the mtc1 instructions, because the
             result is not available for one instruction, so if the
             very first instruction in the function refers to the
             register, it will see the wrong value.  */
          fprintf (asm_out_file, "\tnop\n");
        }
      else
        {
          fprintf (asm_out_file, "\tmove\t%s,%s\n",
                   reg_names[GP_REG_FIRST + 18], reg_names[GP_REG_FIRST + 31]);
          fprintf (asm_out_file, "\tjal\t%s\n", fnname);
          /* As above, we can't fill the delay slot.  */
          fprintf (asm_out_file, "\tnop\n");
          if (GET_MODE (retval) == SFmode)
            fprintf (asm_out_file, "\tmfc1\t%s,%s\n",
                     reg_names[GP_REG_FIRST + 2], reg_names[FP_REG_FIRST + 0]);
          else if (GET_MODE (retval) == SCmode)
            {
              fprintf (asm_out_file, "\tmfc1\t%s,%s\n",
                       reg_names[GP_REG_FIRST + 2], reg_names[FP_REG_FIRST + 0]);
              fprintf (asm_out_file, "\tmfc1\t%s,%s\n",
                       reg_names[GP_REG_FIRST + 3], reg_names[FP_REG_FIRST + 2]);
            }
          else if (GET_MODE (retval) == DFmode)
            {
              mips16_fpret_double (GP_REG_FIRST + 2, FP_REG_FIRST + 0);
            }
          else if (GET_MODE (retval) == DCmode)
            {
              mips16_fpret_double (GP_REG_FIRST + 2, FP_REG_FIRST + 0);
              mips16_fpret_double (GP_REG_FIRST + 4, FP_REG_FIRST + 2);
            }
          fprintf (asm_out_file, "\tj\t%s\n", reg_names[GP_REG_FIRST + 18]);
          /* As above, we can't fill the delay slot.  */
          fprintf (asm_out_file, "\tnop\n");
        }

      fprintf (asm_out_file, "\t.set\treorder\n");

#ifdef ASM_DECLARE_FUNCTION_SIZE
      ASM_DECLARE_FUNCTION_SIZE (asm_out_file, stubname, stubdecl);
#endif

      if (!FUNCTION_NAME_ALREADY_DECLARED)
        {
          fputs ("\t.end\t", asm_out_file);
          assemble_name (asm_out_file, stubname);
          fputs ("\n", asm_out_file);
        }

      /* Record this stub.  */
      l = (struct mips16_stub *) xmalloc (sizeof *l);
      l->name = xstrdup (fnname);
      l->fpret = fpret;
      l->next = mips16_call_stubs;
      mips16_call_stubs = l;
    }

  /* If we expect a floating point return value, but we've built a
     stub which does not expect one, then we're in trouble.  We can't
     use the existing stub, because it won't handle the floating point
     value.  We can't build a new stub, because the linker won't know
     which stub to use for the various calls in this object file.
     Fortunately, this case is illegal, since it means that a function
     was declared in two different ways in a single compilation.  */
  if (fpret && ! l->fpret)
    error ("can not handle inconsistent calls to `%s'", fnname);

  /* If we are calling a stub which handles a floating point return
     value, we need to arrange to save $18 in the prologue.  We do
     this by marking the function call as using the register.  The
     prologue will later see that it is used, and emit code to save
     it.  */

  if (l->fpret)
    {
      rtx insn;

      if (retval == NULL_RTX)
        insn = gen_call_internal (fn, arg_size);
      else
        insn = gen_call_value_internal (retval, fn, arg_size);
      insn = emit_call_insn (insn);

      if (GET_CODE (insn) != CALL_INSN)
        abort ();

      CALL_INSN_FUNCTION_USAGE (insn) =
        gen_rtx (EXPR_LIST, VOIDmode,
                 gen_rtx (USE, VOIDmode, gen_rtx (REG, word_mode, 18)),
                 CALL_INSN_FUNCTION_USAGE (insn));

      /* Return 1 to tell the caller that we've generated the call
         insn.  */
      return 1;
    }

  /* Return 0 to let the caller generate the call insn.  */
  return 0;
}

/* We keep a list of constants we which we have to add to internal
   constant tables in the middle of large functions.  */

struct constant
{
  struct constant *next;
  rtx value;
  rtx label;
  enum machine_mode mode;
};

/* Add a constant to the list in *PCONSTANTS.  */

static rtx
add_constant (struct constant **pconstants, rtx val, enum machine_mode mode)
{
  struct constant *c;

  for (c = *pconstants; c != NULL; c = c->next)
    if (mode == c->mode && rtx_equal_p (val, c->value))
      return c->label;

  c = (struct constant *) xmalloc (sizeof *c);
  c->value = val;
  c->mode = mode;
  c->label = gen_label_rtx ();
  c->next = *pconstants;
  *pconstants = c;
  return c->label;
}

/* Dump out the constants in CONSTANTS after INSN.  */

static void
dump_constants (struct constant *constants, rtx insn)
{
  struct constant *c;
  int align;

  c = constants;
  align = 0;
  while (c != NULL)
    {
      rtx r;
      struct constant *next;

      switch (GET_MODE_SIZE (c->mode))
        {
        case 1:
          align = 0;
          break;
        case 2:
          if (align < 1)
            insn = emit_insn_after (gen_align_2 (), insn);
          align = 1;
          break;
        case 4:
          if (align < 2)
            insn = emit_insn_after (gen_align_4 (), insn);
          align = 2;
          break;
        default:
          if (align < 3)
            insn = emit_insn_after (gen_align_8 (), insn);
          align = 3;
          break;
        }

      insn = emit_label_after (c->label, insn);

      switch (c->mode)
        {
        case QImode:
          r = gen_consttable_qi (c->value);
          break;
        case HImode:
          r = gen_consttable_hi (c->value);
          break;
        case SImode:
          r = gen_consttable_si (c->value);
          break;
        case SFmode:
          r = gen_consttable_sf (c->value);
          break;
        case DImode:
          r = gen_consttable_di (c->value);
          break;
        case DFmode:
          r = gen_consttable_df (c->value);
          break;
        default:
          abort ();
        }

      insn = emit_insn_after (r, insn);

      next = c->next;
      free (c);
      c = next;
    }

  emit_barrier_after (insn);
}

/* Find the symbol in an address expression.  */

static rtx *
mips_find_symbol (rtx *p)
{
  rtx *tem;
  enum rtx_code code = GET_CODE (*p);

  if (code == SYMBOL_REF)
    return p;

  if (code == MEM || code == CONST)
    return mips_find_symbol (&XEXP (*p, 0));

  if (code != PLUS)
    return 0;

  if (XEXP (*p, 0) != 0)
    {
      tem = mips_find_symbol (&XEXP (*p, 0));
      if (tem != 0)
        return tem;
    }

  if (XEXP (*p, 1) != 0)
    {
      tem = mips_find_symbol (&XEXP (*p, 1));
      if (tem != 0)
        return tem;
    }

  return 0;
}

/* In mips16 mode, we need to look through the function to check for
   PC relative loads that are out of range.  */

static void
mips16_lay_out_constants (void)
{
  int insns_len, max_internal_pool_size, pool_size, len, addr, first_constant_ref;
  rtx first, insn;
  struct constant *constants;

  first = get_insns ();

  /* Scan the function looking for PC relative loads which may be out
     of range.  All such loads will either be from the constant table,
     or be getting the address of a constant string.  If the size of
     the function plus the size of the constant table is less than
     0x8000, then all loads are in range.  */

  /* Let shorten_branches compute branch instruction lengths and
     handle alignment issues. */
  shorten_branches (first);
  insns_len = insn_current_address;

  /* Store the original value of insns_len in cfun->machine, so
     that simple_memory_operand can look at it.  */
  cfun->machine->insns_len = insns_len;

  pool_size = get_pool_size ();
  if (insns_len + pool_size
      + cfun->machine->mips_string_length < 0x8000)
    return;

  /* Loop over the insns and figure out what the maximum internal pool
     size could be.  */
  max_internal_pool_size = 0;
  for (insn = first; insn; insn = NEXT_INSN (insn))
    {
      if (GET_CODE (insn) == INSN
          && GET_CODE (PATTERN (insn)) == SET)
        {
          rtx *srcp, src;

          srcp = mips_find_symbol (&SET_SRC (PATTERN (insn)));
          if (!srcp)
            continue;
          src = *srcp;
          if (CONSTANT_POOL_ADDRESS_P (src))
            max_internal_pool_size += GET_MODE_SIZE (get_pool_mode (src));
          else if (SYMBOL_REF_FLAG (src))
            max_internal_pool_size += GET_MODE_SIZE (Pmode);
        }
    }

  constants = NULL;
  addr = 0;
  first_constant_ref = -1;

  for (insn = first; insn; insn = NEXT_INSN (insn))
    {
      if (GET_CODE (insn) == INSN
          && GET_CODE (PATTERN (insn)) == SET)
        {
          rtx val, *srcp, src;
          enum machine_mode mode = VOIDmode;

          if (srcp = mips_find_symbol (&SET_SRC (PATTERN (insn))))
            {
              src = *srcp;
              val = NULL_RTX;
              if (CONSTANT_POOL_ADDRESS_P (src))
                {
                  /* ??? This is very conservative, which means that we
                     will generate too many copies of the constant table.
                     The only solution would seem to be some form of
                     relaxing.  */
                  if (((insns_len - addr)
                       + max_internal_pool_size
                       + get_pool_offset (src)
                       + cfun->machine->mips_string_length)
                      >= 0x8000)
                    {
                      val = get_pool_constant (src);
                      mode = get_pool_mode (src);
                    }
                  max_internal_pool_size -= GET_MODE_SIZE (get_pool_mode (src));
                }
              else if (SYMBOL_REF_FLAG (src))
                {
                  /* Including all of mips_string_length is conservative,
                     and so is including all of max_internal_pool_size.  */
                  if (((insns_len - addr)
                       + max_internal_pool_size
                       + pool_size
                       + cfun->machine->mips_string_length)
                      >= 0x8000)
                    {
                      val = src;
                      mode = Pmode;
                    }
                  max_internal_pool_size -= Pmode;
                }

              if (val != NULL_RTX)
                {
                  rtx lab, lab_ref;

                  /* This PC relative load is out of range.  ??? In the
                     case of a string constant, we are only guessing that
                     it is range, since we don't know the offset of a
                     particular string constant.  */

                  lab = add_constant (&constants, val, mode);
                  lab_ref = gen_rtx_LABEL_REF (Pmode, lab);
                  MIPS16_INLINE_CONSTANT_TABLE_LABEL_P (lab_ref) = 1;

                  if (CONSTANT_POOL_ADDRESS_P (src))
                    /* Simply replace constpool symbol by new label. */
                    *srcp = lab_ref;
                  else
                    /* Load address of string from new pointer. */
                    *srcp = gen_rtx_MEM (Pmode, lab_ref);

                  INSN_CODE (insn) = -1;

                  if (first_constant_ref < 0)
                    first_constant_ref = addr;
                }
            }
        }

      addr += (len = get_attr_length (insn));

      /* ??? We put switch tables in .text, but we don't define
         JUMP_TABLES_IN_TEXT_SECTION, so get_attr_length will not
         compute their lengths correctly.  */
      if (GET_CODE (insn) == JUMP_INSN && len == 0)
        {
          rtx body;

          body = PATTERN (insn);
          if (GET_CODE (body) == ADDR_VEC || GET_CODE (body) == ADDR_DIFF_VEC)
            addr += (XVECLEN (body, GET_CODE (body) == ADDR_DIFF_VEC)
                          * GET_MODE_SIZE (GET_MODE (body)));
          addr += GET_MODE_SIZE (GET_MODE (body)) - 1;
        }

      if (GET_CODE (insn) == BARRIER)
        {
          /* Output any constants we have accumulated.  Note that we
             don't need to change ADDR, since its only use is
             subtraction from INSNS_LEN, and both would be changed by
             the same amount.
             ??? If the instructions up to the next barrier reuse a
             constant, it would often be better to continue
             accumulating.  */
          if (constants != NULL)
            dump_constants (constants, insn);
          constants = NULL;
          first_constant_ref = -1;
        }

      if (constants != NULL
               && (NEXT_INSN (insn) == NULL
                   || (first_constant_ref >= 0
                       && (((addr - first_constant_ref)
                            + 2 /* for alignment */
                            + 2 /* for a short jump insn */
                            + pool_size)
                           >= 0x8000))))
        {
          /* If we haven't had a barrier within 0x8000 bytes of a
             constant reference or we are at the end of the function,
             emit a barrier now.  */

          rtx label, jump, barrier;

          label = gen_label_rtx ();
          jump = emit_jump_insn_after (gen_jump (label), insn);
          JUMP_LABEL (jump) = label;
          LABEL_NUSES (label) = 1;
          barrier = emit_barrier_after (jump);
          emit_label_after (label, barrier);
          first_constant_ref = -1;
        }
     }

  /* Instruction length information is no longer valid due to the new
     instructions that have been generated.  */
  init_insn_lengths ();
}


/* Subroutine of mips_reorg.  If there is a hazard between INSN
   and a previous instruction, avoid it by inserting nops after
   instruction AFTER.

   *DELAYED_REG and *HILO_DELAY describe the hazards that apply at
   this point.  If *DELAYED_REG is non-null, INSN must wait a cycle
   before using the value of that register.  *HILO_DELAY counts the
   number of instructions since the last hilo hazard (that is,
   the number of instructions since the last mflo or mfhi).

   After inserting nops for INSN, update *DELAYED_REG and *HILO_DELAY
   for the next instruction.

   LO_REG is an rtx for the LO register, used in dependence checking.  */

static void
mips_avoid_hazard (rtx after, rtx insn, int *hilo_delay,
                   rtx *delayed_reg, rtx lo_reg)
{
  rtx pattern, set;
  int nops, ninsns;

  if (!INSN_P (insn))
    return;

  pattern = PATTERN (insn);

  /* Do not put the whole function in .set noreorder if it contains
     an asm statement.  We don't know whether there will be hazards
     between the asm statement and the gcc-generated code.  */
  if (GET_CODE (pattern) == ASM_INPUT || asm_noperands (pattern) >= 0)
    cfun->machine->all_noreorder_p = false;

  /* Ignore zero-length instructions (barriers and the like).  */
  ninsns = get_attr_length (insn) / 4;
  if (ninsns == 0)
    return;

  /* Work out how many nops are needed.  Note that we only care about
     registers that are explicitly mentioned in the instruction's pattern.
     It doesn't matter that calls use the argument registers or that they
     clobber hi and lo.  */
  if (*hilo_delay < 2 && reg_set_p (lo_reg, pattern))
    nops = 2 - *hilo_delay;
  else if (*delayed_reg != 0 && reg_referenced_p (*delayed_reg, pattern))
    nops = 1;
  else
    nops = 0;

  /* Insert the nops between this instruction and the previous one.
     Each new nop takes us further from the last hilo hazard.  */
  *hilo_delay += nops;
  while (nops-- > 0)
    emit_insn_after (gen_hazard_nop (), after);

  /* Set up the state for the next instruction.  */
  *hilo_delay += ninsns;
  *delayed_reg = 0;
  if (INSN_CODE (insn) >= 0)
    switch (get_attr_hazard (insn))
      {
      case HAZARD_NONE:
        break;

      case HAZARD_HILO:
        *hilo_delay = 0;
        break;

      case HAZARD_DELAY:
        set = single_set (insn);
        if (set == 0)
          abort ();
        *delayed_reg = SET_DEST (set);
        break;
      }
}


/* Go through the instruction stream and insert nops where necessary.
   See if the whole function can then be put into .set noreorder &
   .set nomacro.  */

static void
mips_avoid_hazards (void)
{
  rtx insn, last_insn, lo_reg, delayed_reg;
  int hilo_delay, i;

  /* Force all instructions to be split into their final form.  */
  split_all_insns_noflow ();

  /* Recalculate instruction lengths without taking nops into account.  */
  cfun->machine->ignore_hazard_length_p = true;
  shorten_branches (get_insns ());

  /* The profiler code uses assembler macros.  */
  cfun->machine->all_noreorder_p = !current_function_profile;

  if (TARGET_FIX_24K_E48)
    /* Disable generation of noreorder code.  */
    cfun->machine->all_noreorder_p = false;

  last_insn = 0;
  hilo_delay = 2;
  delayed_reg = 0;
  lo_reg = gen_rtx_REG (SImode, LO_REGNUM);

  for (insn = get_insns (); insn != 0; insn = NEXT_INSN (insn))
    if (INSN_P (insn))
      {
        if (GET_CODE (PATTERN (insn)) == SEQUENCE)
          for (i = 0; i < XVECLEN (PATTERN (insn), 0); i++)
            mips_avoid_hazard (last_insn, XVECEXP (PATTERN (insn), 0, i),
                               &hilo_delay, &delayed_reg, lo_reg);
        else
          mips_avoid_hazard (last_insn, insn, &hilo_delay,
                             &delayed_reg, lo_reg);

        last_insn = insn;
      }
}


/* Implement TARGET_MACHINE_DEPENDENT_REORG.  */

static void
mips_reorg (void)
{
  if (TARGET_MIPS16)
    mips16_lay_out_constants ();
  else if (TARGET_EXPLICIT_RELOCS)
    {
      if (mips_flag_delayed_branch)
        dbr_schedule (get_insns (), rtl_dump_file);
      mips_avoid_hazards ();
    }
}

/* We need to use a special set of functions to handle hard floating
   point code in mips16 mode.  Also, allow for --enable-gofast.  */

#include "config/gofast.h"

static void
mips_init_libfuncs (void)
{
  if (TARGET_MIPS16 && mips16_hard_float)
    {
      set_optab_libfunc (add_optab, SFmode, "__mips16_addsf3");
      set_optab_libfunc (sub_optab, SFmode, "__mips16_subsf3");
      set_optab_libfunc (smul_optab, SFmode, "__mips16_mulsf3");
      set_optab_libfunc (sdiv_optab, SFmode, "__mips16_divsf3");

      set_optab_libfunc (eq_optab, SFmode, "__mips16_eqsf2");
      set_optab_libfunc (ne_optab, SFmode, "__mips16_nesf2");
      set_optab_libfunc (gt_optab, SFmode, "__mips16_gtsf2");
      set_optab_libfunc (ge_optab, SFmode, "__mips16_gesf2");
      set_optab_libfunc (lt_optab, SFmode, "__mips16_ltsf2");
      set_optab_libfunc (le_optab, SFmode, "__mips16_lesf2");
      set_optab_libfunc (unord_optab, SFmode, "__mips16_unordsf2");

      set_conv_libfunc (sfix_optab, SImode, SFmode, "__mips16_fixsfsi");
      set_conv_libfunc (sfloat_optab, SFmode, SImode, "__mips16_floatsisf");

      if (TARGET_DOUBLE_FLOAT)
        {
          set_optab_libfunc (add_optab, DFmode, "__mips16_adddf3");
          set_optab_libfunc (sub_optab, DFmode, "__mips16_subdf3");
          set_optab_libfunc (smul_optab, DFmode, "__mips16_muldf3");
          set_optab_libfunc (sdiv_optab, DFmode, "__mips16_divdf3");

          set_optab_libfunc (eq_optab, DFmode, "__mips16_eqdf2");
          set_optab_libfunc (ne_optab, DFmode, "__mips16_nedf2");
          set_optab_libfunc (gt_optab, DFmode, "__mips16_gtdf2");
          set_optab_libfunc (ge_optab, DFmode, "__mips16_gedf2");
          set_optab_libfunc (lt_optab, DFmode, "__mips16_ltdf2");
          set_optab_libfunc (le_optab, DFmode, "__mips16_ledf2");
	  set_optab_libfunc (unord_optab, DFmode, "__mips16_unorddf2");

          set_conv_libfunc (sext_optab, DFmode, SFmode, "__mips16_extendsfdf2");
          set_conv_libfunc (trunc_optab, SFmode, DFmode, "__mips16_truncdfsf2");

          set_conv_libfunc (sfix_optab, SImode, DFmode, "__mips16_fixdfsi");
          set_conv_libfunc (sfloat_optab, DFmode, SImode, "__mips16_floatsidf");
        }
    }
#if (defined(TARGET_MCHP_PIC32MX) && !defined(PIC32_NO_FAST_MATH))
  else
    gofast_maybe_init_libfuncs ();
#endif
}

/* Return a number assessing the cost of moving a register in class
   FROM to class TO.  The classes are expressed using the enumeration
   values such as `GENERAL_REGS'.  A value of 2 is the default; other
   values are interpreted relative to that.

   It is not required that the cost always equal 2 when FROM is the
   same as TO; on some machines it is expensive to move between
   registers if they are not general registers.

   If reload sees an insn consisting of a single `set' between two
   hard registers, and if `REGISTER_MOVE_COST' applied to their
   classes returns a value of 2, reload does not check to ensure that
   the constraints of the insn are met.  Setting a cost of other than
   2 will allow reload to verify that the constraints are met.  You
   should do this if the `movM' pattern's constraints do not allow
   such copying.

   ??? We make the cost of moving from HI/LO into general
   registers the same as for one of moving general registers to
   HI/LO for TARGET_MIPS16 in order to prevent allocating a
   pseudo to HI/LO.  This might hurt optimizations though, it
   isn't clear if it is wise.  And it might not work in all cases.  We
   could solve the DImode LO reg problem by using a multiply, just
   like reload_{in,out}si.  We could solve the SImode/HImode HI reg
   problem by using divide instructions.  divu puts the remainder in
   the HI reg, so doing a divide by -1 will move the value in the HI
   reg for all values except -1.  We could handle that case by using a
   signed divide, e.g.  -1 / 2 (or maybe 1 / -2?).  We'd have to emit
   a compare/branch to test the input value to see which instruction
   we need to use.  This gets pretty messy, but it is feasible.  */

int
mips_register_move_cost (enum machine_mode mode ATTRIBUTE_UNUSED,
                         enum reg_class to, enum reg_class from)
{
  if (from == M16_REGS && GR_REG_CLASS_P (to))
    return 2;
  else if (from == M16_NA_REGS && GR_REG_CLASS_P (to))
    return 2;
  else if (GR_REG_CLASS_P (from))
    {
      if (to == M16_REGS)
        return 2;
      else if (to == M16_NA_REGS)
        return 2;
      else if (GR_REG_CLASS_P (to))
        {
          if (TARGET_MIPS16)
            return 4;
          else
            return 2;
        }
      else if (to == FP_REGS)
        return 4;
      else if (to == MD0_REG || to == MD1_REG || to == MD_REGS
               || to == DSP_ACC_REGS || to == MD_AND_DSP_ACC_REGS)
        {
          if (TARGET_MIPS16)
            return 12;
          else
            return 6;
        }
      else if (COP_REG_CLASS_P (to))
        {
          return 5;
        }
    }  /* GR_REG_CLASS_P (from) */
  else if (from == FP_REGS)
    {
      if (GR_REG_CLASS_P (to))
        return 4;
      else if (to == FP_REGS)
        return 2;
      else if (to == ST_REGS)
        return 8;
    }  /* from == FP_REGS */
  else if (from == MD0_REG || from == MD1_REG || from == MD_REGS
           || from == DSP_ACC_REGS || from == MD_AND_DSP_ACC_REGS)
    {
      if (GR_REG_CLASS_P (to))
        {
          if (TARGET_MIPS16)
            return 12;
          else
            return 6;
        }
    }  /* from == MD0_REG, etc.  */
  else if (from == ST_REGS && GR_REG_CLASS_P (to))
    return 4;
  else if (COP_REG_CLASS_P (from))
    {
      return 5;
    }  /* COP_REG_CLASS_P (from) */

  /* Fall through.  */

  return 12;
}

/* Return the length of INSN.  LENGTH is the initial length computed by
   attributes in the machine-description file.  */

int
mips_adjust_insn_length (rtx insn, int length)
{
  /* A unconditional jump has an unfilled delay slot if it is not part
     of a sequence.  A conditional jump normally has a delay slot, but
     does not on MIPS16.  */
  if (GET_CODE (insn) == CALL_INSN
      || (TARGET_MIPS16 ? simplejump_p (insn) : JUMP_P (insn)))
    length += 4;

  /* See how many nops might be needed to avoid hardware hazards.  */
  if (!cfun->machine->ignore_hazard_length_p && INSN_CODE (insn) >= 0)
    switch (get_attr_hazard (insn))
      {
      case HAZARD_NONE:
        break;

      case HAZARD_DELAY:
        length += 4;
        break;

      case HAZARD_HILO:
        length += 8;
        break;
      }

  /* All MIPS16 instructions are a measly two bytes.  */
  if (TARGET_MIPS16)
    length /= 2;

  return length;
}


/* Return an asm sequence to start a noat block and load the address
   of a label into $1.  */

const char *
mips_output_load_label (void)
{
  if (TARGET_EXPLICIT_RELOCS)
    switch (mips_abi)
      {
      case ABI_N32:
        return "%[lw\t%@,%%got_page(%0)(%+)\n\taddiu\t%@,%@,%%got_ofst(%0)";

      case ABI_64:
        return "%[ld\t%@,%%got_page(%0)(%+)\n\tdaddiu\t%@,%@,%%got_ofst(%0)";

      default:
        if (ISA_HAS_LOAD_DELAY)
          return "%[lw\t%@,%%got(%0)(%+)%#\n\taddiu\t%@,%@,%%lo(%0)";
        return "%[lw\t%@,%%got(%0)(%+)\n\taddiu\t%@,%@,%%lo(%0)";
      }
  else
    {
      if (Pmode == DImode)
        return "%[dla\t%@,%0";
      else
        return "%[la\t%@,%0";
    }
}


/* Output assembly instructions to peform a conditional branch.

   INSN is the branch instruction.  OPERANDS[0] is the condition.
   OPERANDS[1] is the target of the branch.  OPERANDS[2] is the target
   of the first operand to the condition.  If TWO_OPERANDS_P is
   nonzero the comparison takes two operands; OPERANDS[3] will be the
   second operand.

   If INVERTED_P is nonzero we are to branch if the condition does
   not hold.  If FLOAT_P is nonzero this is a floating-point comparison.

   LENGTH is the length (in bytes) of the sequence we are to generate.
   That tells us whether to generate a simple conditional branch, or a
   reversed conditional branch around a `jr' instruction.  */
const char *
mips_output_conditional_branch (rtx insn, rtx *operands, int two_operands_p,
                                int float_p, int inverted_p, int length)
{
  static char buffer[200];
  /* The kind of comparison we are doing.  */
  enum rtx_code code = GET_CODE (operands[0]);
  /* Nonzero if the opcode for the comparison needs a `z' indicating
     that it is a comparison against zero.  */
  int need_z_p;
  /* A string to use in the assembly output to represent the first
     operand.  */
  const char *op1 = "%z2";
  /* A string to use in the assembly output to represent the second
     operand.  Use the hard-wired zero register if there's no second
     operand.  */
  const char *op2 = (two_operands_p ? ",%z3" : ",%.");
  /* The operand-printing string for the comparison.  */
  const char *const comp = (float_p ? "%F0" : "%C0");
  /* The operand-printing string for the inverted comparison.  */
  const char *const inverted_comp = (float_p ? "%W0" : "%N0");

  /* The MIPS processors (for levels of the ISA at least two), have
     "likely" variants of each branch instruction.  These instructions
     annul the instruction in the delay slot if the branch is not
     taken.  */
  mips_branch_likely = (final_sequence && INSN_ANNULLED_BRANCH_P (insn));

  if (!two_operands_p)
    {
      /* To compute whether than A > B, for example, we normally
         subtract B from A and then look at the sign bit.  But, if we
         are doing an unsigned comparison, and B is zero, we don't
         have to do the subtraction.  Instead, we can just check to
         see if A is nonzero.  Thus, we change the CODE here to
         reflect the simpler comparison operation.  */
      switch (code)
        {
        case GTU:
          code = NE;
          break;

        case LEU:
          code = EQ;
          break;

        case GEU:
          /* A condition which will always be true.  */
          code = EQ;
          op1 = "%.";
          break;

        case LTU:
          /* A condition which will always be false.  */
          code = NE;
          op1 = "%.";
          break;

        default:
          /* Not a special case.  */
          break;
        }
    }

  /* Relative comparisons are always done against zero.  But
     equality comparisons are done between two operands, and therefore
     do not require a `z' in the assembly language output.  */
  need_z_p = (!float_p && code != EQ && code != NE);
  /* For comparisons against zero, the zero is not provided
     explicitly.  */
  if (need_z_p)
    op2 = "";

  /* Begin by terminating the buffer.  That way we can always use
     strcat to add to it.  */
  buffer[0] = '\0';

  switch (length)
    {
    case 4:
    case 8:
      /* Just a simple conditional branch.  */
      if (float_p)
        sprintf (buffer, "%%*b%s%%?\t%%Z2%%1%%/",
                 inverted_p ? inverted_comp : comp);
      else
        sprintf (buffer, "%%*b%s%s%%?\t%s%s,%%1%%/",
                 inverted_p ? inverted_comp : comp,
                 need_z_p ? "z" : "",
                 op1,
                 op2);
      return buffer;

    case 12:
    case 16:
    case 24:
    case 28:
      {
        /* Generate a reversed conditional branch around ` j'
           instruction:

                .set noreorder
                .set nomacro
                bc    l
                delay_slot or #nop
                j     target
                #nop
             l:
                .set macro
                .set reorder

           If the original branch was a likely branch, the delay slot
           must be executed only if the branch is taken, so generate:

                .set noreorder
                .set nomacro
                bc    l
                #nop
                j     target
                delay slot or #nop
             l:
                .set macro
                .set reorder

           When generating non-embedded PIC, instead of:

                j     target

           we emit:

                .set noat
                la    $at, target
                jr    $at
                .set at
        */

        rtx orig_target;
        rtx target = gen_label_rtx ();

        orig_target = operands[1];
        operands[1] = target;
        /* Generate the reversed comparison.  This takes four
           bytes.  */
        if (float_p)
          sprintf (buffer, "%%*b%s\t%%Z2%%1",
                   inverted_p ? comp : inverted_comp);
        else
          sprintf (buffer, "%%*b%s%s\t%s%s,%%1",
                   inverted_p ? comp : inverted_comp,
                   need_z_p ? "z" : "",
                   op1,
                   op2);
        output_asm_insn (buffer, operands);

        if (length != 16 && length != 28 && ! mips_branch_likely)
          {
            /* Output delay slot instruction.  */
            rtx insn = final_sequence;
            final_scan_insn (XVECEXP (insn, 0, 1), asm_out_file,
                             optimize, 0, 1, NULL);
            INSN_DELETED_P (XVECEXP (insn, 0, 1)) = 1;
          }
        else
          output_asm_insn ("%#", 0);

        if (length <= 16)
          output_asm_insn ("j\t%0", &orig_target);
        else
          {
            output_asm_insn (mips_output_load_label (), &orig_target);
            output_asm_insn ("jr\t%@%]", 0);
          }

        if (length != 16 && length != 28 && mips_branch_likely)
          {
            /* Output delay slot instruction.  */
            rtx insn = final_sequence;
            final_scan_insn (XVECEXP (insn, 0, 1), asm_out_file,
                             optimize, 0, 1, NULL);
            INSN_DELETED_P (XVECEXP (insn, 0, 1)) = 1;
          }
        else
          output_asm_insn ("%#", 0);

        (*targetm.asm_out.internal_label) (asm_out_file, "L",
                                   CODE_LABEL_NUMBER (target));

        return "";
      }

    default:
      abort ();
    }

  /* NOTREACHED */
  return 0;
}

/* Used to output div or ddiv instruction DIVISION, which has the
   operands given by OPERANDS.  If we need a divide-by-zero check,
   output the instruction and return an asm string that traps if
   operand 2 is zero.  Otherwise just return DIVISION itself.  */

const char *
mips_output_division (const char *division, rtx *operands)
{
  if (TARGET_CHECK_ZERO_DIV)
    {
      output_asm_insn (division, operands);

      if (TARGET_MIPS16)
        return "bnez\t%2,1f\n\tbreak\t7\n1:";
#ifdef TARGET_MIPS_SDE
      /* XXX use another target flag to enable this */
      else if (ISA_HAS_COND_TRAP)
        return "teq\t%2,%.,7";
#endif
      else
        return "bne\t%2,%.,1f%#\n\tbreak\t7\n1:";
    }
  return division;
}

/* Return true if GIVEN is the same as CANONICAL, or if it is CANONICAL
   with a final "000" replaced by "k".  Ignore case.

   Note: this function is shared between GCC and GAS.  */

static bool
mips_strict_matching_cpu_name_p (const char *canonical, const char *given)
{
  while (*given != 0 && TOLOWER (*given) == TOLOWER (*canonical))
    given++, canonical++;

  return ((*given == 0 && *canonical == 0)
          || (strcmp (canonical, "000") == 0 && strcasecmp (given, "k") == 0));
}


/* Return true if GIVEN matches CANONICAL, where GIVEN is a user-supplied
   CPU name.  We've traditionally allowed a lot of variation here.

   Note: this function is shared between GCC and GAS.  */

static bool
mips_matching_cpu_name_p (const char *canonical, const char *given)
{
  /* First see if the name matches exactly, or with a final "000"
     turned into "k".  */
  if (mips_strict_matching_cpu_name_p (canonical, given))
    return true;

  /* If not, try comparing based on numerical designation alone.
     See if GIVEN is an unadorned number, or 'r' followed by a number.  */
  if (TOLOWER (*given) == 'r')
    given++;
  if (!ISDIGIT (*given))
    return false;

  /* Skip over some well-known prefixes in the canonical name,
     hoping to find a number there too.  */
  if (TOLOWER (canonical[0]) == 'v' && TOLOWER (canonical[1]) == 'r')
    canonical += 2;
  else if (TOLOWER (canonical[0]) == 'r' && TOLOWER (canonical[1]) == 'm')
    canonical += 2;
  else if (TOLOWER (canonical[0]) == 'r')
    canonical += 1;

  return mips_strict_matching_cpu_name_p (canonical, given);
}


/* Parse an option that takes the name of a processor as its argument.
   OPTION is the name of the option and CPU_STRING is the argument.
   Return the corresponding processor enumeration if the CPU_STRING is
   recognized, otherwise report an error and return null.

   A similar function exists in GAS.  */

static const struct mips_cpu_info *
mips_parse_cpu (const char *option, const char *cpu_string)
{
  const struct mips_cpu_info *p;
  const char *s;

  /* In the past, we allowed upper-case CPU names, but it doesn't
     work well with the multilib machinery.  */
  for (s = cpu_string; *s != 0; s++)
    if (ISUPPER (*s))
      {
        warning ("the cpu name must be lower case");
        break;
      }

  /* 'from-abi' selects the most compatible architecture for the given
     ABI: MIPS I for 32-bit ABIs and MIPS III for 64-bit ABIs.  For the
     EABIs, we have to decide whether we're using the 32-bit or 64-bit
     version.  Look first at the -mgp options, if given, otherwise base
     the choice on MASK_64BIT in TARGET_DEFAULT.  */
  if (strcasecmp (cpu_string, "from-abi") == 0)
    return mips_cpu_info_from_isa (ABI_NEEDS_32BIT_REGS ? 1
                                   : ABI_NEEDS_64BIT_REGS ? 3
                                   : (TARGET_64BIT ? 3 : 1));

  /* 'default' has traditionally been a no-op.  Probably not very useful.  */
  if (strcasecmp (cpu_string, "default") == 0)
    return 0;

  for (p = mips_cpu_info_table; p->name != 0; p++)
    if (mips_matching_cpu_name_p (p->name, cpu_string))
      return p;

  error ("bad value (%s) for %s", cpu_string, option);
  return 0;
}


/* Return the processor associated with the given ISA level, or null
   if the ISA isn't valid.  */

static const struct mips_cpu_info *
mips_cpu_info_from_isa (int isa)
{
  const struct mips_cpu_info *p;

  for (p = mips_cpu_info_table; p->name != 0; p++)
    if (p->isa == isa)
      return p;

  return 0;
}

/* Adjust the cost of INSN based on the relationship between INSN that
   is dependent on DEP_INSN through the dependence LINK.  The default
   is to make no adjustment to COST.

   On the MIPS, ignore the cost of anti- and output-dependencies.
   Except on the 20Kc where output-dependence is treated like
   input-dependence. */

static int
mips_adjust_cost (rtx insn ATTRIBUTE_UNUSED, rtx link,
                  rtx dep ATTRIBUTE_UNUSED, int cost)
{
  if (REG_NOTE_KIND (link) == REG_DEP_OUTPUT
      && TUNE_MIPS20KC)
    return cost;
  if (REG_NOTE_KIND (link) != 0)
    return 0;   /* Anti or output dependence.  */
  return cost;
}


static bool
mips_is_costly_dependence (rtx insn, rtx next, rtx link, int cost, int distance)
{
  if (TUNE_MIPS74K
      && insn && INSN_P (insn) && GET_CODE (PATTERN (insn)) == SET 
      && get_attr_type (insn) == TYPE_STORE
      && next && INSN_P (next) && GET_CODE (PATTERN (next)) == SET
      && get_attr_type (next) == TYPE_LOAD)
    {
      return true;
    }

  return false;
}

/* Record whether last insn is a load (value 1) or store (value 2).  */
static int last_agen_insn = -1;

/* Reorder the schedulers priority list if needed */
#include "sched-int.h"

static int
mips_sched_reorder (FILE * stream ATTRIBUTE_UNUSED,
		    int verbose ATTRIBUTE_UNUSED,
		    rtx * ready, int *n_readyp,
		    int clock ATTRIBUTE_UNUSED)
{
  if (TUNE_MIPS74K)
    {
      int i;
      int store_pos = -1;
      int load_pos = -1;

      for (i = *n_readyp-1; i >= 0; i--)
	{
	  rtx insn = ready[i];
  	  if (GET_CODE (PATTERN (insn)) == SET
	      && get_attr_type (insn) == TYPE_STORE
	      && store_pos == -1)
	    store_pos = i;
  	  else if (GET_CODE (PATTERN (insn)) == SET
		   && get_attr_type (insn) == TYPE_LOAD
		   && load_pos == -1)
	    load_pos = i;
	}

      if (load_pos == -1 || store_pos == -1)
	return mips_issue_rate ();
	
      switch (last_agen_insn)
	{
	case -1:
	  /* Prefer to schedule loads.  */
	case 1:
	  /* Swap loads to the front of the queue.  */
	  if (INSN_PRIORITY (ready[load_pos]) + 4 
	       >= INSN_PRIORITY (ready[store_pos])
	      && load_pos < store_pos)
	    {
	      rtx temp;
	      INSN_PRIORITY (ready[load_pos]) 
		= INSN_PRIORITY (ready[store_pos]);
	      temp = ready[load_pos];
	      ready[load_pos] = ready[store_pos];
	      ready[store_pos] = temp;
	    }
	  break;
	case 2:
	  /* Swap stores to the front of the queue.  */
	  if (INSN_PRIORITY (ready[store_pos]) + 4 
	       >= INSN_PRIORITY (ready[load_pos])
	      && store_pos < load_pos)
	    {
	      rtx temp;
	      INSN_PRIORITY (ready[store_pos]) 
		= INSN_PRIORITY (ready[load_pos]);
	      temp = ready[store_pos];
	      ready[store_pos] = ready[load_pos];
	      ready[load_pos] = temp;
	    }
	  break;
	default:
	  break;
	}
      /* At this point the ready queue may no longer sorted, but thats ok
	 since 74k can't schedule concurrent ld/st on the same cycle.  */
    }

  return mips_issue_rate ();
}

static int
mips_variable_issue (FILE *dump, int sched_verbose, rtx insn,
                     int can_issue_more)
{
  if (TUNE_MIPS74K)
    {
      switch (GET_CODE (insn))
	{
	case INSN:
	  if (GET_CODE (PATTERN (insn)) == SET)
	    {
	      if (get_attr_type (insn) == TYPE_LOAD)
		last_agen_insn = 1;
	      else if (get_attr_type (insn) == TYPE_STORE)
		last_agen_insn = 2;
	    }
	  break;
	case JUMP_INSN:
	  last_agen_insn = -1;
	  break;
	case CALL_INSN:
	  last_agen_insn = -1;
	  return 0;
	default:
	  break;
	}
    }

  if (GET_CODE (PATTERN (insn)) == USE
      || GET_CODE (PATTERN (insn)) == CLOBBER)
    return can_issue_more;
  else 
    return can_issue_more - 1;
}


#define LAPSE_CYCLES 1

static int
mips_dfa_new_cycle (FILE *sched_dump,
		    int sched_verbose,
		    rtx insn,
		    int last_clock_var,
		    int clock_var,
		    int *sort_p)
{
  if (TUNE_MIPS74K)
    {
      if (reload_completed && flag_sched2_use_superblocks)
	return 0;

      /* We reach the end of the block (at the branch insn), and if there
	 are some spare cycles, then pull in insns from following block by
	 advancing cycle by LAPSE_CYCLES.  Hence readies insns from the
	 queue that stalls in LAPSE_CYCLES, effectively moving it to the
	 ready list.  */
     if (GET_CODE (insn) == JUMP_INSN)
	{
	  if (clock_var - last_clock_var != 0
	      && clock_var - last_clock_var <= LAPSE_CYCLES)
	    {
	      if (sched_verbose >= 5)
		fprintf (sched_dump, ";;\t\t*** Lapse cycle *** \n");
	      return 1;
	    }
  	}
    }
  
  return 0;
}


/* Implement TARGET_WEB_MARK_REGISTER.
   For instructions that read and write accumulators, we mark the register
   for no renamiing in the web phase.  */

static void
mips_web_mark_register (rtx insn, char *use_addressof)
{
  if (GET_CODE (PATTERN (insn)) == PARALLEL
      && GET_CODE (XVECEXP (PATTERN (insn), 0, 0)) == SET)
    {
      rtx dest = SET_DEST (XVECEXP (PATTERN (insn), 0, 0));
      rtx src = SET_SRC (XVECEXP (PATTERN (insn), 0, 0));
      if (GET_CODE (dest) == REG && GET_CODE (src) == UNSPEC)
	{
	  rtx op0 = XVECEXP (src, 0, 0);
	  int unspec_num = XINT (src, 1);
	  if (GET_CODE (op0) == REG &&  GET_MODE (op0) == GET_MODE (dest)
	      && REGNO (op0) == REGNO (dest)
	      && (unspec_num == UNSPEC_DPAQ_S_W_PH
		  || unspec_num == UNSPEC_DPAQ_S_W_PH_64
		  || unspec_num == UNSPEC_DPAQ_W_QH
		  || unspec_num == UNSPEC_DPAQ_S_W_QH
		  || unspec_num == UNSPEC_DPSQ_S_W_PH
		  || unspec_num == UNSPEC_DPSQ_S_W_PH_64
		  || unspec_num == UNSPEC_DPSQ_W_QH
		  || unspec_num == UNSPEC_DPSQ_S_W_QH
		  || unspec_num == UNSPEC_MULSAQ_S_W_PH
		  || unspec_num == UNSPEC_MULSAQ_S_W_PH_64
		  || unspec_num == UNSPEC_MULSAQ_W_QH
		  || unspec_num == UNSPEC_MULSAQ_S_W_QH
		  || unspec_num == UNSPEC_MULSAQ_L_PW
		  || unspec_num == UNSPEC_MULSAQ_S_L_PW
		  || unspec_num == UNSPEC_DPAQ_SA_L_W
		  || unspec_num == UNSPEC_DPAQ_SA_L_W_64
		  || unspec_num == UNSPEC_DPAQ_L_PW
		  || unspec_num == UNSPEC_DPAQ_SA_L_PW
		  || unspec_num == UNSPEC_DPSQ_SA_L_W
		  || unspec_num == UNSPEC_DPSQ_SA_L_W_64
		  || unspec_num == UNSPEC_DPSQ_L_PW
		  || unspec_num == UNSPEC_DPSQ_SA_L_PW
		  || unspec_num == UNSPEC_MAQ_S_W_PHL
		  || unspec_num == UNSPEC_MAQ_S_W_PHR
		  || unspec_num == UNSPEC_MAQ_S_W_PHL_64
		  || unspec_num == UNSPEC_MAQ_S_W_PHR_64
		  || unspec_num == UNSPEC_MAQ_W_QHLL
		  || unspec_num == UNSPEC_MAQ_W_QHLR
		  || unspec_num == UNSPEC_MAQ_W_QHRL
		  || unspec_num == UNSPEC_MAQ_W_QHRR
		  || unspec_num == UNSPEC_MAQ_S_W_QHLL
		  || unspec_num == UNSPEC_MAQ_S_W_QHLR
		  || unspec_num == UNSPEC_MAQ_S_W_QHRL
		  || unspec_num == UNSPEC_MAQ_S_W_QHRR
		  || unspec_num == UNSPEC_MAQ_L_PWL
		  || unspec_num == UNSPEC_MAQ_L_PWR
		  || unspec_num == UNSPEC_MAQ_S_L_PWL
		  || unspec_num == UNSPEC_MAQ_S_L_PWR
		  || unspec_num == UNSPEC_MAQ_SA_W_PHL
		  || unspec_num == UNSPEC_MAQ_SA_W_PHR
		  || unspec_num == UNSPEC_MAQ_SA_W_PHL_64
		  || unspec_num == UNSPEC_MAQ_SA_W_PHR_64
		  || unspec_num == UNSPEC_MTHLIP
		  || unspec_num == UNSPEC_MTHLIP_64
		  || unspec_num == UNSPEC_DMTHLIP
		  || unspec_num == UNSPEC_DPAQX_S_W_PH
		  || unspec_num == UNSPEC_DPAQX_SA_W_PH
		  || unspec_num == UNSPEC_DPSQX_S_W_PH
		  || unspec_num == UNSPEC_DPSQX_SA_W_PH
		  || unspec_num == UNSPEC_DPAQX_S_W_PH_64
		  || unspec_num == UNSPEC_DPAQX_SA_W_PH_64
		  || unspec_num == UNSPEC_DPSQX_S_W_PH_64
		  || unspec_num == UNSPEC_DPSQX_SA_W_PH_64))
	    use_addressof[REGNO (dest)] = 1;
	}
    }
  else if (GET_CODE (PATTERN (insn)) == SET)
    {
      rtx dest = SET_DEST (PATTERN (insn));
      rtx src = SET_SRC (PATTERN (insn));
      if (GET_CODE (dest) == REG && GET_CODE (src) == UNSPEC)
	{
	  rtx op0 = XVECEXP (src, 0, 0);
	  int unspec_num = XINT (src, 1);
	  if (GET_CODE (op0) == REG &&  GET_MODE (op0) == GET_MODE (dest)
	      && REGNO (op0) == REGNO (dest)
	      && (unspec_num == UNSPEC_DPAU_H_QBL
		  || unspec_num == UNSPEC_DPAU_H_QBR
		  || unspec_num == UNSPEC_DPAU_H_QBL_64
		  || unspec_num == UNSPEC_DPAU_H_QBR_64
		  || unspec_num == UNSPEC_DPSU_H_QBL
		  || unspec_num == UNSPEC_DPSU_H_QBR
		  || unspec_num == UNSPEC_DPSU_H_QBL_64
		  || unspec_num == UNSPEC_DPSU_H_QBR_64
		  || unspec_num == UNSPEC_DMADD
		  || unspec_num == UNSPEC_DMADDU
		  || unspec_num == UNSPEC_DMSUB
		  || unspec_num == UNSPEC_DMSUBU
		  || unspec_num == UNSPEC_SHILO
		  || unspec_num == UNSPEC_SHILO_64
		  || unspec_num == UNSPEC_DSHILO
		  || unspec_num == UNSPEC_DPA_W_PH
		  || unspec_num == UNSPEC_DPA_W_PH_64
		  || unspec_num == UNSPEC_DPS_W_PH
		  || unspec_num == UNSPEC_DPS_W_PH_64
		  || unspec_num == UNSPEC_MULSA_W_PH
		  || unspec_num == UNSPEC_MULSA_W_PH_64
		  || unspec_num == UNSPEC_DPAX_W_PH
		  || unspec_num == UNSPEC_DPSX_W_PH
		  || unspec_num == UNSPEC_DPAX_W_PH_64
		  || unspec_num == UNSPEC_DPSX_W_PH_64))
	    use_addressof[REGNO (dest)] = 1;
	}
    }
}

/* Implement HARD_REGNO_NREGS.  The size of FP registers are controlled
   by UNITS_PER_FPREG.  All other registers are word sized.  */

unsigned int
mips_hard_regno_nregs (int regno, enum machine_mode mode)
{
  if (! FP_REG_P (regno))
    {
      if (mode == CCV2mode)
        return 2;
      else if (mode == CCV4mode)
        return 4;
      else
        return ((GET_MODE_SIZE (mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD);
    }
  else
    return ((GET_MODE_SIZE (mode) + UNITS_PER_FPREG - 1) / UNITS_PER_FPREG);
}

/* Implement RETURN_IN_MEMORY.  Under the old (i.e., 32 and O64 ABIs)
   all BLKmode objects are returned in memory.  Under the new (N32 and
   64-bit MIPS ABIs) small structures are returned in a register.
   Objects with varying size must still be returned in memory, of
   course.  */

int
mips_return_in_memory (tree type)
{
  if (mips_abi == ABI_32 || mips_abi == ABI_O64)
    return (TYPE_MODE (type) == BLKmode);
  else
    return ((int_size_in_bytes (type) > (2 * UNITS_PER_WORD))
            || (int_size_in_bytes (type) == -1));
}

static int
mips_issue_rate (void)
{
  switch (mips_tune)
    {
    case PROCESSOR_74K:
    case PROCESSOR_74KX:
    case PROCESSOR_74KZ:
      /* The 74k is not strictly quad-issue cpu, but can be seen as one
	 by the scheduler.  It can issue 1 ALU, 1 AGEN, 2 FPU insns, but
	 in reality only max of 3 insn can be issued as the floating point
	 load/stores also requires a slot in the AGEN pipe.  */
     return 4;

    case PROCESSOR_20KC:
    case PROCESSOR_R5400:
    case PROCESSOR_R5500:
    case PROCESSOR_R7000:
    case PROCESSOR_R9000:
      return 2;

    default:
      return 1;
    }

  abort ();

}

/* Implements TARGET_SCHED_USE_DFA_PIPELINE_INTERFACE.  Return true for
   processors that have a DFA pipeline description.  */

static int
mips_use_dfa_pipeline_interface (void)
{
  switch (mips_tune)
    {
    case PROCESSOR_20KC:
    case PROCESSOR_24K:
    case PROCESSOR_24KX:
    case PROCESSOR_74K:
    case PROCESSOR_74KX:
    case PROCESSOR_74KZ:
    case PROCESSOR_4KC:
    case PROCESSOR_4KP:
    case PROCESSOR_5KC:
    case PROCESSOR_R5400:
    case PROCESSOR_R5500:
    case PROCESSOR_R7000:
    case PROCESSOR_R9000:
    case PROCESSOR_SR71000:
      return true;

    default:
      return false;
    }
}


/* Implement TARGET_SCHED_REGNO_CLOBBER_DEPENDENCIES.  */
static bool
mips_sched_regno_clobber_dependencies (int regno)
{
  return (regno == DSP_CONTROL_OU_REGNUM);
}


const char *
mips_emit_prefetch (rtx *operands)
{
  int write = INTVAL (operands[1]);
  int locality = INTVAL (operands[2]);
  int indexed = GET_CODE (operands[3]) == REG;
  int code;
  char buffer[30];

  if (locality <= 0)
    code = (write ? 5 : 4);     /* store_streamed / load_streamed.  */
  else if (locality <= 2)
    code = (write ? 1 : 0);     /* store / load.  */
  else
    code = (write ? 7 : 6);     /* store_retained / load_retained.  */

  sprintf (buffer, "%s\t%d,%%3(%%0)", indexed ? "prefx" : "pref", code);
  output_asm_insn (buffer, operands);
  return "";
}



#if TARGET_IRIX
/* Output assembly to switch to section NAME with attribute FLAGS.  */

static void
irix_asm_named_section_1 (const char *name, unsigned int flags,
                           unsigned int align)
{
  unsigned int sh_type, sh_flags, sh_entsize;

  sh_flags = 0;
  if (!(flags & SECTION_DEBUG))
    sh_flags |= 2; /* SHF_ALLOC */
  if (flags & SECTION_WRITE)
    sh_flags |= 1; /* SHF_WRITE */
  if (flags & SECTION_CODE)
    sh_flags |= 4; /* SHF_EXECINSTR */
  if (flags & SECTION_SMALL)
    sh_flags |= 0x10000000; /* SHF_MIPS_GPREL */
  if (strcmp (name, ".debug_frame") == 0)
    sh_flags |= 0x08000000; /* SHF_MIPS_NOSTRIP */
  if (flags & SECTION_DEBUG)
    sh_type = 0x7000001e; /* SHT_MIPS_DWARF */
  else if (flags & SECTION_BSS)
    sh_type = 8; /* SHT_NOBITS */
  else
    sh_type = 1; /* SHT_PROGBITS */

  if (flags & SECTION_CODE)
    sh_entsize = 4;
  else
    sh_entsize = 0;

  fprintf (asm_out_file, "\t.section %s,%#x,%#x,%u,%u\n",
           name, sh_type, sh_flags, sh_entsize, align);
}

static void
irix_asm_named_section (const char *name, unsigned int flags)
{
  if (TARGET_SGI_O32_AS)
    default_no_named_section (name, flags);
  else if (mips_abi == ABI_32 && TARGET_GAS)
    default_elf_asm_named_section (name, flags);
  else
    irix_asm_named_section_1 (name, flags, 0);
}

/* In addition to emitting a .align directive, record the maximum
   alignment requested for the current section.  */

struct GTY (()) irix_section_align_entry
{
  const char *name;
  unsigned int log;
  unsigned int flags;
};

static htab_t irix_section_align_htab;
static FILE *irix_orig_asm_out_file;

static int
irix_section_align_entry_eq (const void *p1, const void *p2)
{
  const struct irix_section_align_entry *old = p1;
  const char *new = p2;

  return strcmp (old->name, new) == 0;
}

static hashval_t
irix_section_align_entry_hash (const void *p)
{
  const struct irix_section_align_entry *old = p;
  return htab_hash_string (old->name);
}

void
irix_asm_output_align (FILE *file, unsigned int log)
{
  const char *section = current_section_name ();
  struct irix_section_align_entry **slot, *entry;

  if (mips_abi != ABI_32)
    {
      if (! section)
        abort ();

      slot = (struct irix_section_align_entry **)
        htab_find_slot_with_hash (irix_section_align_htab, section,
                                  htab_hash_string (section), INSERT);
      entry = *slot;
      if (! entry)
        {
          entry = (struct irix_section_align_entry *)
            xmalloc (sizeof (struct irix_section_align_entry));
          *slot = entry;
          entry->name = section;
          entry->log = log;
          entry->flags = current_section_flags ();
        }
      else if (entry->log < log)
        entry->log = log;
    }

  fprintf (file, "\t.align\t%u\n", log);
}

/* The IRIX assembler does not record alignment from .align directives,
   but takes it from the first .section directive seen.  Play file
   switching games so that we can emit a .section directive at the
   beginning of the file with the proper alignment attached.  */

static void
irix_file_start (void)
{
  mips_file_start ();

  if (mips_abi == ABI_32)
    return;

  irix_orig_asm_out_file = asm_out_file;
  asm_out_file = tmpfile ();

  irix_section_align_htab = htab_create (31, irix_section_align_entry_hash,
                                         irix_section_align_entry_eq, NULL);
}

static int
irix_section_align_1 (void **slot, void *data ATTRIBUTE_UNUSED)
{
  const struct irix_section_align_entry *entry
    = *(const struct irix_section_align_entry **) slot;

  irix_asm_named_section_1 (entry->name, entry->flags, 1 << entry->log);
  return 1;
}

static void
copy_file_data (FILE *to, FILE *from)
{
  char buffer[8192];
  size_t len;
  rewind (from);
  if (ferror (from))
    fatal_error ("can't rewind temp file: %m");

  while ((len = fread (buffer, 1, sizeof (buffer), from)) > 0)
    if (fwrite (buffer, 1, len, to) != len)
      fatal_error ("can't write to output file: %m");

  if (ferror (from))
    fatal_error ("can't read from temp file: %m");

  if (fclose (from))
    fatal_error ("can't close temp file: %m");
}

static void
irix_file_end (void)
{
  if (mips_abi != ABI_32)
    {
      /* Emit section directives with the proper alignment at the top of the
         real output file.  */
      FILE *temp = asm_out_file;
      asm_out_file = irix_orig_asm_out_file;
      htab_traverse (irix_section_align_htab, irix_section_align_1, NULL);

      /* Copy the data emitted to the temp file to the real output file.  */
      copy_file_data (asm_out_file, temp);
    }

  mips_file_end ();
}


/* Implement TARGET_SECTION_TYPE_FLAGS.  Make sure that .sdata and
   .sbss sections get the SECTION_SMALL flag: this isn't set by the
   default code.  */

static unsigned int
irix_section_type_flags (tree decl, const char *section, int relocs_p)
{
  unsigned int flags;

  flags = default_section_type_flags (decl, section, relocs_p);

  if (strcmp (section, ".sdata") == 0
      || strcmp (section, ".sbss") == 0
      || strncmp (section, ".gnu.linkonce.s.", 16) == 0
      || strncmp (section, ".gnu.linkonce.sb.", 17) == 0)
    flags |= SECTION_SMALL;

  return flags;
}

#endif /* TARGET_IRIX */


/*
 * MIPS builtin functions
 */
struct builtin_description
{
  enum insn_code icode;          /* MIPS instruction code */
  const char *name;              /* MIPS builtin function name */
  enum mips_builtins code;       /* MIPS builtin code */
  enum mips_function_type ftype; /* function type */
  int target_flags;              /* target flags required for
                                    this builtin function */
};

/* NOTE: The order of mips_bdesc[] must be the same as the order of
   enum mips_builtins{} in mips.h */
static const struct builtin_description mips_bdesc[] =
{
  { CODE_FOR_mips_pll_ps, "__builtin_mips_pll_ps", MIPS_BUILTIN_PLL_PS, MIPS_V2SF_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_pul_ps, "__builtin_mips_pul_ps", MIPS_BUILTIN_PUL_PS, MIPS_V2SF_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_plu_ps, "__builtin_mips_plu_ps", MIPS_BUILTIN_PLU_PS, MIPS_V2SF_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_puu_ps, "__builtin_mips_puu_ps", MIPS_BUILTIN_PUU_PS, MIPS_V2SF_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_cvt_ps_s, "__builtin_mips_cvt_ps_s", MIPS_BUILTIN_CVT_PS_S, MIPS_V2SF_FTYPE_SF_SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_cvt_s_pl, "__builtin_mips_cvt_s_pl", MIPS_BUILTIN_CVT_S_PL, MIPS_SF_FTYPE_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_cvt_s_pu, "__builtin_mips_cvt_s_pu", MIPS_BUILTIN_CVT_S_PU, MIPS_SF_FTYPE_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_absv2sf2, "__builtin_mips_abs_ps", MIPS_BUILTIN_ABS_PS, MIPS_V2SF_FTYPE_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_alnv_ps, "__builtin_mips_alnv_ps", MIPS_BUILTIN_ALNV_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_INT, MASK_PAIRED_SINGLE },

  { CODE_FOR_mips_addr_ps, "__builtin_mips_addr_ps", MIPS_BUILTIN_ADDR_PS, MIPS_V2SF_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_mulr_ps, "__builtin_mips_mulr_ps", MIPS_BUILTIN_MULR_PS, MIPS_V2SF_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cvt_pw_ps, "__builtin_mips_cvt_pw_ps", MIPS_BUILTIN_CVT_PW_PS, MIPS_V2SF_FTYPE_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cvt_ps_pw, "__builtin_mips_cvt_ps_pw", MIPS_BUILTIN_CVT_PS_PW, MIPS_V2SF_FTYPE_V2SF, MASK_MIPS3D },

  { CODE_FOR_mips_recip1_s, "__builtin_mips_recip1_s", MIPS_BUILTIN_RECIP1_S, MIPS_SF_FTYPE_SF, MASK_MIPS3D },
  { CODE_FOR_mips_recip1_d, "__builtin_mips_recip1_d", MIPS_BUILTIN_RECIP1_D, MIPS_DF_FTYPE_DF, MASK_MIPS3D },
  { CODE_FOR_mips_recip1_ps, "__builtin_mips_recip1_ps", MIPS_BUILTIN_RECIP1_PS, MIPS_V2SF_FTYPE_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_recip2_s, "__builtin_mips_recip2_s", MIPS_BUILTIN_RECIP2_S, MIPS_SF_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_recip2_d, "__builtin_mips_recip2_d", MIPS_BUILTIN_RECIP2_D, MIPS_DF_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_recip2_ps, "__builtin_mips_recip2_ps", MIPS_BUILTIN_RECIP2_PS, MIPS_V2SF_FTYPE_V2SF_V2SF, MASK_MIPS3D },

  { CODE_FOR_mips_rsqrt1_s, "__builtin_mips_rsqrt1_s", MIPS_BUILTIN_RSQRT1_S, MIPS_SF_FTYPE_SF, MASK_MIPS3D },
  { CODE_FOR_mips_rsqrt1_d, "__builtin_mips_rsqrt1_d", MIPS_BUILTIN_RSQRT1_D, MIPS_DF_FTYPE_DF, MASK_MIPS3D },
  { CODE_FOR_mips_rsqrt1_ps, "__builtin_mips_rsqrt1_ps", MIPS_BUILTIN_RSQRT1_PS, MIPS_V2SF_FTYPE_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_rsqrt2_s, "__builtin_mips_rsqrt2_s", MIPS_BUILTIN_RSQRT2_S, MIPS_SF_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_rsqrt2_d, "__builtin_mips_rsqrt2_d", MIPS_BUILTIN_RSQRT2_D, MIPS_DF_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_rsqrt2_ps, "__builtin_mips_rsqrt2_ps", MIPS_BUILTIN_RSQRT2_PS, MIPS_V2SF_FTYPE_V2SF_V2SF, MASK_MIPS3D },

  { CODE_FOR_mips_c_f_ps, "__builtin_mips_any_c_f_ps", MIPS_BUILTIN_ANY_C_F_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_f_ps, "__builtin_mips_upper_c_f_ps", MIPS_BUILTIN_UPPER_C_F_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_f_ps, "__builtin_mips_lower_c_f_ps", MIPS_BUILTIN_LOWER_C_F_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_f_ps, "__builtin_mips_all_c_f_ps", MIPS_BUILTIN_ALL_C_F_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_un_ps, "__builtin_mips_any_c_un_ps", MIPS_BUILTIN_ANY_C_UN_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_un_ps, "__builtin_mips_upper_c_un_ps", MIPS_BUILTIN_UPPER_C_UN_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_un_ps, "__builtin_mips_lower_c_un_ps", MIPS_BUILTIN_LOWER_C_UN_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_un_ps, "__builtin_mips_all_c_un_ps", MIPS_BUILTIN_ALL_C_UN_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_eq_ps, "__builtin_mips_any_c_eq_ps", MIPS_BUILTIN_ANY_C_EQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_eq_ps, "__builtin_mips_upper_c_eq_ps", MIPS_BUILTIN_UPPER_C_EQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_eq_ps, "__builtin_mips_lower_c_eq_ps", MIPS_BUILTIN_LOWER_C_EQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_eq_ps, "__builtin_mips_all_c_eq_ps", MIPS_BUILTIN_ALL_C_EQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ueq_ps, "__builtin_mips_any_c_ueq_ps", MIPS_BUILTIN_ANY_C_UEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ueq_ps, "__builtin_mips_upper_c_ueq_ps", MIPS_BUILTIN_UPPER_C_UEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ueq_ps, "__builtin_mips_lower_c_ueq_ps", MIPS_BUILTIN_LOWER_C_UEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ueq_ps, "__builtin_mips_all_c_ueq_ps", MIPS_BUILTIN_ALL_C_UEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_olt_ps, "__builtin_mips_any_c_olt_ps", MIPS_BUILTIN_ANY_C_OLT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_olt_ps, "__builtin_mips_upper_c_olt_ps", MIPS_BUILTIN_UPPER_C_OLT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_olt_ps, "__builtin_mips_lower_c_olt_ps", MIPS_BUILTIN_LOWER_C_OLT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_olt_ps, "__builtin_mips_all_c_olt_ps", MIPS_BUILTIN_ALL_C_OLT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ult_ps, "__builtin_mips_any_c_ult_ps", MIPS_BUILTIN_ANY_C_ULT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ult_ps, "__builtin_mips_upper_c_ult_ps", MIPS_BUILTIN_UPPER_C_ULT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ult_ps, "__builtin_mips_lower_c_ult_ps", MIPS_BUILTIN_LOWER_C_ULT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ult_ps, "__builtin_mips_all_c_ult_ps", MIPS_BUILTIN_ALL_C_ULT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ole_ps, "__builtin_mips_any_c_ole_ps", MIPS_BUILTIN_ANY_C_OLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ole_ps, "__builtin_mips_upper_c_ole_ps", MIPS_BUILTIN_UPPER_C_OLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ole_ps, "__builtin_mips_lower_c_ole_ps", MIPS_BUILTIN_LOWER_C_OLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ole_ps, "__builtin_mips_all_c_ole_ps", MIPS_BUILTIN_ALL_C_OLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ule_ps, "__builtin_mips_any_c_ule_ps", MIPS_BUILTIN_ANY_C_ULE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ule_ps, "__builtin_mips_upper_c_ule_ps", MIPS_BUILTIN_UPPER_C_ULE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ule_ps, "__builtin_mips_lower_c_ule_ps", MIPS_BUILTIN_LOWER_C_ULE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ule_ps, "__builtin_mips_all_c_ule_ps", MIPS_BUILTIN_ALL_C_ULE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_sf_ps, "__builtin_mips_any_c_sf_ps", MIPS_BUILTIN_ANY_C_SF_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_sf_ps, "__builtin_mips_upper_c_sf_ps", MIPS_BUILTIN_UPPER_C_SF_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_sf_ps, "__builtin_mips_lower_c_sf_ps", MIPS_BUILTIN_LOWER_C_SF_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_sf_ps, "__builtin_mips_all_c_sf_ps", MIPS_BUILTIN_ALL_C_SF_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngle_ps, "__builtin_mips_any_c_ngle_ps", MIPS_BUILTIN_ANY_C_NGLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngle_ps, "__builtin_mips_upper_c_ngle_ps", MIPS_BUILTIN_UPPER_C_NGLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngle_ps, "__builtin_mips_lower_c_ngle_ps", MIPS_BUILTIN_LOWER_C_NGLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngle_ps, "__builtin_mips_all_c_ngle_ps", MIPS_BUILTIN_ALL_C_NGLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_seq_ps, "__builtin_mips_any_c_seq_ps", MIPS_BUILTIN_ANY_C_SEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_seq_ps, "__builtin_mips_upper_c_seq_ps", MIPS_BUILTIN_UPPER_C_SEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_seq_ps, "__builtin_mips_lower_c_seq_ps", MIPS_BUILTIN_LOWER_C_SEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_seq_ps, "__builtin_mips_all_c_seq_ps", MIPS_BUILTIN_ALL_C_SEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngl_ps, "__builtin_mips_any_c_ngl_ps", MIPS_BUILTIN_ANY_C_NGL_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngl_ps, "__builtin_mips_upper_c_ngl_ps", MIPS_BUILTIN_UPPER_C_NGL_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngl_ps, "__builtin_mips_lower_c_ngl_ps", MIPS_BUILTIN_LOWER_C_NGL_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngl_ps, "__builtin_mips_all_c_ngl_ps", MIPS_BUILTIN_ALL_C_NGL_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_lt_ps, "__builtin_mips_any_c_lt_ps", MIPS_BUILTIN_ANY_C_LT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_lt_ps, "__builtin_mips_upper_c_lt_ps", MIPS_BUILTIN_UPPER_C_LT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_lt_ps, "__builtin_mips_lower_c_lt_ps", MIPS_BUILTIN_LOWER_C_LT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_lt_ps, "__builtin_mips_all_c_lt_ps", MIPS_BUILTIN_ALL_C_LT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_nge_ps, "__builtin_mips_any_c_nge_ps", MIPS_BUILTIN_ANY_C_NGE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_nge_ps, "__builtin_mips_upper_c_nge_ps", MIPS_BUILTIN_UPPER_C_NGE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_nge_ps, "__builtin_mips_lower_c_nge_ps", MIPS_BUILTIN_LOWER_C_NGE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_nge_ps, "__builtin_mips_all_c_nge_ps", MIPS_BUILTIN_ALL_C_NGE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_le_ps, "__builtin_mips_any_c_le_ps", MIPS_BUILTIN_ANY_C_LE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_le_ps, "__builtin_mips_upper_c_le_ps", MIPS_BUILTIN_UPPER_C_LE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_le_ps, "__builtin_mips_lower_c_le_ps", MIPS_BUILTIN_LOWER_C_LE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_le_ps, "__builtin_mips_all_c_le_ps", MIPS_BUILTIN_ALL_C_LE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngt_ps, "__builtin_mips_any_c_ngt_ps", MIPS_BUILTIN_ANY_C_NGT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngt_ps, "__builtin_mips_upper_c_ngt_ps", MIPS_BUILTIN_UPPER_C_NGT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngt_ps, "__builtin_mips_lower_c_ngt_ps", MIPS_BUILTIN_LOWER_C_NGT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngt_ps, "__builtin_mips_all_c_ngt_ps", MIPS_BUILTIN_ALL_C_NGT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },

  { CODE_FOR_mips_cabs_f_ps, "__builtin_mips_any_cabs_f_ps", MIPS_BUILTIN_ANY_CABS_F_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_f_ps, "__builtin_mips_upper_cabs_f_ps", MIPS_BUILTIN_UPPER_CABS_F_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_f_ps, "__builtin_mips_lower_cabs_f_ps", MIPS_BUILTIN_LOWER_CABS_F_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_f_ps, "__builtin_mips_all_cabs_f_ps", MIPS_BUILTIN_ALL_CABS_F_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_un_ps, "__builtin_mips_any_cabs_un_ps", MIPS_BUILTIN_ANY_CABS_UN_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_un_ps, "__builtin_mips_upper_cabs_un_ps", MIPS_BUILTIN_UPPER_CABS_UN_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_un_ps, "__builtin_mips_lower_cabs_un_ps", MIPS_BUILTIN_LOWER_CABS_UN_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_un_ps, "__builtin_mips_all_cabs_un_ps", MIPS_BUILTIN_ALL_CABS_UN_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_eq_ps, "__builtin_mips_any_cabs_eq_ps", MIPS_BUILTIN_ANY_CABS_EQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_eq_ps, "__builtin_mips_upper_cabs_eq_ps", MIPS_BUILTIN_UPPER_CABS_EQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_eq_ps, "__builtin_mips_lower_cabs_eq_ps", MIPS_BUILTIN_LOWER_CABS_EQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_eq_ps, "__builtin_mips_all_cabs_eq_ps", MIPS_BUILTIN_ALL_CABS_EQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ueq_ps, "__builtin_mips_any_cabs_ueq_ps", MIPS_BUILTIN_ANY_CABS_UEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ueq_ps, "__builtin_mips_upper_cabs_ueq_ps", MIPS_BUILTIN_UPPER_CABS_UEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ueq_ps, "__builtin_mips_lower_cabs_ueq_ps", MIPS_BUILTIN_LOWER_CABS_UEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ueq_ps, "__builtin_mips_all_cabs_ueq_ps", MIPS_BUILTIN_ALL_CABS_UEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_olt_ps, "__builtin_mips_any_cabs_olt_ps", MIPS_BUILTIN_ANY_CABS_OLT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_olt_ps, "__builtin_mips_upper_cabs_olt_ps", MIPS_BUILTIN_UPPER_CABS_OLT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_olt_ps, "__builtin_mips_lower_cabs_olt_ps", MIPS_BUILTIN_LOWER_CABS_OLT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_olt_ps, "__builtin_mips_all_cabs_olt_ps", MIPS_BUILTIN_ALL_CABS_OLT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ult_ps, "__builtin_mips_any_cabs_ult_ps", MIPS_BUILTIN_ANY_CABS_ULT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ult_ps, "__builtin_mips_upper_cabs_ult_ps", MIPS_BUILTIN_UPPER_CABS_ULT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ult_ps, "__builtin_mips_lower_cabs_ult_ps", MIPS_BUILTIN_LOWER_CABS_ULT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ult_ps, "__builtin_mips_all_cabs_ult_ps", MIPS_BUILTIN_ALL_CABS_ULT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ole_ps, "__builtin_mips_any_cabs_ole_ps", MIPS_BUILTIN_ANY_CABS_OLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ole_ps, "__builtin_mips_upper_cabs_ole_ps", MIPS_BUILTIN_UPPER_CABS_OLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ole_ps, "__builtin_mips_lower_cabs_ole_ps", MIPS_BUILTIN_LOWER_CABS_OLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ole_ps, "__builtin_mips_all_cabs_ole_ps", MIPS_BUILTIN_ALL_CABS_OLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ule_ps, "__builtin_mips_any_cabs_ule_ps", MIPS_BUILTIN_ANY_CABS_ULE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ule_ps, "__builtin_mips_upper_cabs_ule_ps", MIPS_BUILTIN_UPPER_CABS_ULE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ule_ps, "__builtin_mips_lower_cabs_ule_ps", MIPS_BUILTIN_LOWER_CABS_ULE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ule_ps, "__builtin_mips_all_cabs_ule_ps", MIPS_BUILTIN_ALL_CABS_ULE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_sf_ps, "__builtin_mips_any_cabs_sf_ps", MIPS_BUILTIN_ANY_CABS_SF_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_sf_ps, "__builtin_mips_upper_cabs_sf_ps", MIPS_BUILTIN_UPPER_CABS_SF_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_sf_ps, "__builtin_mips_lower_cabs_sf_ps", MIPS_BUILTIN_LOWER_CABS_SF_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_sf_ps, "__builtin_mips_all_cabs_sf_ps", MIPS_BUILTIN_ALL_CABS_SF_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngle_ps, "__builtin_mips_any_cabs_ngle_ps", MIPS_BUILTIN_ANY_CABS_NGLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngle_ps, "__builtin_mips_upper_cabs_ngle_ps", MIPS_BUILTIN_UPPER_CABS_NGLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngle_ps, "__builtin_mips_lower_cabs_ngle_ps", MIPS_BUILTIN_LOWER_CABS_NGLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngle_ps, "__builtin_mips_all_cabs_ngle_ps", MIPS_BUILTIN_ALL_CABS_NGLE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_seq_ps, "__builtin_mips_any_cabs_seq_ps", MIPS_BUILTIN_ANY_CABS_SEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_seq_ps, "__builtin_mips_upper_cabs_seq_ps", MIPS_BUILTIN_UPPER_CABS_SEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_seq_ps, "__builtin_mips_lower_cabs_seq_ps", MIPS_BUILTIN_LOWER_CABS_SEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_seq_ps, "__builtin_mips_all_cabs_seq_ps", MIPS_BUILTIN_ALL_CABS_SEQ_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngl_ps, "__builtin_mips_any_cabs_ngl_ps", MIPS_BUILTIN_ANY_CABS_NGL_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngl_ps, "__builtin_mips_upper_cabs_ngl_ps", MIPS_BUILTIN_UPPER_CABS_NGL_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngl_ps, "__builtin_mips_lower_cabs_ngl_ps", MIPS_BUILTIN_LOWER_CABS_NGL_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngl_ps, "__builtin_mips_all_cabs_ngl_ps", MIPS_BUILTIN_ALL_CABS_NGL_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_lt_ps, "__builtin_mips_any_cabs_lt_ps", MIPS_BUILTIN_ANY_CABS_LT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_lt_ps, "__builtin_mips_upper_cabs_lt_ps", MIPS_BUILTIN_UPPER_CABS_LT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_lt_ps, "__builtin_mips_lower_cabs_lt_ps", MIPS_BUILTIN_LOWER_CABS_LT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_lt_ps, "__builtin_mips_all_cabs_lt_ps", MIPS_BUILTIN_ALL_CABS_LT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_nge_ps, "__builtin_mips_any_cabs_nge_ps", MIPS_BUILTIN_ANY_CABS_NGE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_nge_ps, "__builtin_mips_upper_cabs_nge_ps", MIPS_BUILTIN_UPPER_CABS_NGE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_nge_ps, "__builtin_mips_lower_cabs_nge_ps", MIPS_BUILTIN_LOWER_CABS_NGE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_nge_ps, "__builtin_mips_all_cabs_nge_ps", MIPS_BUILTIN_ALL_CABS_NGE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_le_ps, "__builtin_mips_any_cabs_le_ps", MIPS_BUILTIN_ANY_CABS_LE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_le_ps, "__builtin_mips_upper_cabs_le_ps", MIPS_BUILTIN_UPPER_CABS_LE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_le_ps, "__builtin_mips_lower_cabs_le_ps", MIPS_BUILTIN_LOWER_CABS_LE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_le_ps, "__builtin_mips_all_cabs_le_ps", MIPS_BUILTIN_ALL_CABS_LE_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngt_ps, "__builtin_mips_any_cabs_ngt_ps", MIPS_BUILTIN_ANY_CABS_NGT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngt_ps, "__builtin_mips_upper_cabs_ngt_ps", MIPS_BUILTIN_UPPER_CABS_NGT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngt_ps, "__builtin_mips_lower_cabs_ngt_ps", MIPS_BUILTIN_LOWER_CABS_NGT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngt_ps, "__builtin_mips_all_cabs_ngt_ps", MIPS_BUILTIN_ALL_CABS_NGT_PS, MIPS_INT_FTYPE_V2SF_V2SF, MASK_MIPS3D },


  { CODE_FOR_mips_c_f_4s, "__builtin_mips_any_c_f_4s", MIPS_BUILTIN_ANY_C_F_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_f_4s, "__builtin_mips_all_c_f_4s", MIPS_BUILTIN_ALL_C_F_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_un_4s, "__builtin_mips_any_c_un_4s", MIPS_BUILTIN_ANY_C_UN_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_un_4s, "__builtin_mips_all_c_un_4s", MIPS_BUILTIN_ALL_C_UN_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_eq_4s, "__builtin_mips_any_c_eq_4s", MIPS_BUILTIN_ANY_C_EQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_eq_4s, "__builtin_mips_all_c_eq_4s", MIPS_BUILTIN_ALL_C_EQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ueq_4s, "__builtin_mips_any_c_ueq_4s", MIPS_BUILTIN_ANY_C_UEQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ueq_4s, "__builtin_mips_all_c_ueq_4s", MIPS_BUILTIN_ALL_C_UEQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_olt_4s, "__builtin_mips_any_c_olt_4s", MIPS_BUILTIN_ANY_C_OLT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_olt_4s, "__builtin_mips_all_c_olt_4s", MIPS_BUILTIN_ALL_C_OLT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ult_4s, "__builtin_mips_any_c_ult_4s", MIPS_BUILTIN_ANY_C_ULT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ult_4s, "__builtin_mips_all_c_ult_4s", MIPS_BUILTIN_ALL_C_ULT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ole_4s, "__builtin_mips_any_c_ole_4s", MIPS_BUILTIN_ANY_C_OLE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ole_4s, "__builtin_mips_all_c_ole_4s", MIPS_BUILTIN_ALL_C_OLE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ule_4s, "__builtin_mips_any_c_ule_4s", MIPS_BUILTIN_ANY_C_ULE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ule_4s, "__builtin_mips_all_c_ule_4s", MIPS_BUILTIN_ALL_C_ULE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_sf_4s, "__builtin_mips_any_c_sf_4s", MIPS_BUILTIN_ANY_C_SF_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_sf_4s, "__builtin_mips_all_c_sf_4s", MIPS_BUILTIN_ALL_C_SF_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngle_4s, "__builtin_mips_any_c_ngle_4s", MIPS_BUILTIN_ANY_C_NGLE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngle_4s, "__builtin_mips_all_c_ngle_4s", MIPS_BUILTIN_ALL_C_NGLE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_seq_4s, "__builtin_mips_any_c_seq_4s", MIPS_BUILTIN_ANY_C_SEQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_seq_4s, "__builtin_mips_all_c_seq_4s", MIPS_BUILTIN_ALL_C_SEQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngl_4s, "__builtin_mips_any_c_ngl_4s", MIPS_BUILTIN_ANY_C_NGL_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngl_4s, "__builtin_mips_all_c_ngl_4s", MIPS_BUILTIN_ALL_C_NGL_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_lt_4s, "__builtin_mips_any_c_lt_4s", MIPS_BUILTIN_ANY_C_LT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_lt_4s, "__builtin_mips_all_c_lt_4s", MIPS_BUILTIN_ALL_C_LT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_nge_4s, "__builtin_mips_any_c_nge_4s", MIPS_BUILTIN_ANY_C_NGE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_nge_4s, "__builtin_mips_all_c_nge_4s", MIPS_BUILTIN_ALL_C_NGE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_le_4s, "__builtin_mips_any_c_le_4s", MIPS_BUILTIN_ANY_C_LE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_le_4s, "__builtin_mips_all_c_le_4s", MIPS_BUILTIN_ALL_C_LE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngt_4s, "__builtin_mips_any_c_ngt_4s", MIPS_BUILTIN_ANY_C_NGT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_ngt_4s, "__builtin_mips_all_c_ngt_4s", MIPS_BUILTIN_ALL_C_NGT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },

  { CODE_FOR_mips_cabs_f_4s, "__builtin_mips_any_cabs_f_4s", MIPS_BUILTIN_ANY_CABS_F_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_f_4s, "__builtin_mips_all_cabs_f_4s", MIPS_BUILTIN_ALL_CABS_F_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_un_4s, "__builtin_mips_any_cabs_un_4s", MIPS_BUILTIN_ANY_CABS_UN_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_un_4s, "__builtin_mips_all_cabs_un_4s", MIPS_BUILTIN_ALL_CABS_UN_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_eq_4s, "__builtin_mips_any_cabs_eq_4s", MIPS_BUILTIN_ANY_CABS_EQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_eq_4s, "__builtin_mips_all_cabs_eq_4s", MIPS_BUILTIN_ALL_CABS_EQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ueq_4s, "__builtin_mips_any_cabs_ueq_4s", MIPS_BUILTIN_ANY_CABS_UEQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ueq_4s, "__builtin_mips_all_cabs_ueq_4s", MIPS_BUILTIN_ALL_CABS_UEQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_olt_4s, "__builtin_mips_any_cabs_olt_4s", MIPS_BUILTIN_ANY_CABS_OLT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_olt_4s, "__builtin_mips_all_cabs_olt_4s", MIPS_BUILTIN_ALL_CABS_OLT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ult_4s, "__builtin_mips_any_cabs_ult_4s", MIPS_BUILTIN_ANY_CABS_ULT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ult_4s, "__builtin_mips_all_cabs_ult_4s", MIPS_BUILTIN_ALL_CABS_ULT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ole_4s, "__builtin_mips_any_cabs_ole_4s", MIPS_BUILTIN_ANY_CABS_OLE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ole_4s, "__builtin_mips_all_cabs_ole_4s", MIPS_BUILTIN_ALL_CABS_OLE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ule_4s, "__builtin_mips_any_cabs_ule_4s", MIPS_BUILTIN_ANY_CABS_ULE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ule_4s, "__builtin_mips_all_cabs_ule_4s", MIPS_BUILTIN_ALL_CABS_ULE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_sf_4s, "__builtin_mips_any_cabs_sf_4s", MIPS_BUILTIN_ANY_CABS_SF_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_sf_4s, "__builtin_mips_all_cabs_sf_4s", MIPS_BUILTIN_ALL_CABS_SF_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngle_4s, "__builtin_mips_any_cabs_ngle_4s", MIPS_BUILTIN_ANY_CABS_NGLE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngle_4s, "__builtin_mips_all_cabs_ngle_4s", MIPS_BUILTIN_ALL_CABS_NGLE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_seq_4s, "__builtin_mips_any_cabs_seq_4s", MIPS_BUILTIN_ANY_CABS_SEQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_seq_4s, "__builtin_mips_all_cabs_seq_4s", MIPS_BUILTIN_ALL_CABS_SEQ_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngl_4s, "__builtin_mips_any_cabs_ngl_4s", MIPS_BUILTIN_ANY_CABS_NGL_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngl_4s, "__builtin_mips_all_cabs_ngl_4s", MIPS_BUILTIN_ALL_CABS_NGL_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_lt_4s, "__builtin_mips_any_cabs_lt_4s", MIPS_BUILTIN_ANY_CABS_LT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_lt_4s, "__builtin_mips_all_cabs_lt_4s", MIPS_BUILTIN_ALL_CABS_LT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_nge_4s, "__builtin_mips_any_cabs_nge_4s", MIPS_BUILTIN_ANY_CABS_NGE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_nge_4s, "__builtin_mips_all_cabs_nge_4s", MIPS_BUILTIN_ALL_CABS_NGE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_le_4s, "__builtin_mips_any_cabs_le_4s", MIPS_BUILTIN_ANY_CABS_LE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_le_4s, "__builtin_mips_all_cabs_le_4s", MIPS_BUILTIN_ALL_CABS_LE_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngt_4s, "__builtin_mips_any_cabs_ngt_4s", MIPS_BUILTIN_ANY_CABS_NGT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngt_4s, "__builtin_mips_all_cabs_ngt_4s", MIPS_BUILTIN_ALL_CABS_NGT_4S, MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },

  { CODE_FOR_mips_cabs_f_s, "__builtin_mips_cabs_f_s", MIPS_BUILTIN_CABS_F_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_un_s, "__builtin_mips_cabs_un_s", MIPS_BUILTIN_CABS_UN_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_eq_s, "__builtin_mips_cabs_eq_s", MIPS_BUILTIN_CABS_EQ_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ueq_s, "__builtin_mips_cabs_ueq_s", MIPS_BUILTIN_CABS_UEQ_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_olt_s, "__builtin_mips_cabs_olt_s", MIPS_BUILTIN_CABS_OLT_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ult_s, "__builtin_mips_cabs_ult_s", MIPS_BUILTIN_CABS_ULT_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ole_s, "__builtin_mips_cabs_ole_s", MIPS_BUILTIN_CABS_OLE_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ule_s, "__builtin_mips_cabs_ule_s", MIPS_BUILTIN_CABS_ULE_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_sf_s, "__builtin_mips_cabs_sf_s", MIPS_BUILTIN_CABS_SF_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngle_s, "__builtin_mips_cabs_ngle_s", MIPS_BUILTIN_CABS_NGLE_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_seq_s, "__builtin_mips_cabs_seq_s", MIPS_BUILTIN_CABS_SEQ_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngl_s, "__builtin_mips_cabs_ngl_s", MIPS_BUILTIN_CABS_NGL_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_lt_s, "__builtin_mips_cabs_lt_s", MIPS_BUILTIN_CABS_LT_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_nge_s, "__builtin_mips_cabs_nge_s", MIPS_BUILTIN_CABS_NGE_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_le_s, "__builtin_mips_cabs_le_s", MIPS_BUILTIN_CABS_LE_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngt_s, "__builtin_mips_cabs_ngt_s", MIPS_BUILTIN_CABS_NGT_S, MIPS_INT_FTYPE_SF_SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_f_d, "__builtin_mips_cabs_f_d", MIPS_BUILTIN_CABS_F_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_un_d, "__builtin_mips_cabs_un_d", MIPS_BUILTIN_CABS_UN_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_eq_d, "__builtin_mips_cabs_eq_d", MIPS_BUILTIN_CABS_EQ_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ueq_d, "__builtin_mips_cabs_ueq_d", MIPS_BUILTIN_CABS_UEQ_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_olt_d, "__builtin_mips_cabs_olt_d", MIPS_BUILTIN_CABS_OLT_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ult_d, "__builtin_mips_cabs_ult_d", MIPS_BUILTIN_CABS_ULT_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ole_d, "__builtin_mips_cabs_ole_d", MIPS_BUILTIN_CABS_OLE_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ule_d, "__builtin_mips_cabs_ule_d", MIPS_BUILTIN_CABS_ULE_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_sf_d, "__builtin_mips_cabs_sf_d", MIPS_BUILTIN_CABS_SF_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngle_d, "__builtin_mips_cabs_ngle_d", MIPS_BUILTIN_CABS_NGLE_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_seq_d, "__builtin_mips_cabs_seq_d", MIPS_BUILTIN_CABS_SEQ_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngl_d, "__builtin_mips_cabs_ngl_d", MIPS_BUILTIN_CABS_NGL_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_lt_d, "__builtin_mips_cabs_lt_d", MIPS_BUILTIN_CABS_LT_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_nge_d, "__builtin_mips_cabs_nge_d", MIPS_BUILTIN_CABS_NGE_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_le_d, "__builtin_mips_cabs_le_d", MIPS_BUILTIN_CABS_LE_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngt_d, "__builtin_mips_cabs_ngt_d", MIPS_BUILTIN_CABS_NGT_D, MIPS_INT_FTYPE_DF_DF, MASK_MIPS3D },

  { CODE_FOR_mips_c_f_ps, "__builtin_mips_movt_c_f_ps", MIPS_BUILTIN_MOVT_C_F_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_un_ps, "__builtin_mips_movt_c_un_ps", MIPS_BUILTIN_MOVT_C_UN_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_eq_ps, "__builtin_mips_movt_c_eq_ps", MIPS_BUILTIN_MOVT_C_EQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ueq_ps, "__builtin_mips_movt_c_ueq_ps", MIPS_BUILTIN_MOVT_C_UEQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_olt_ps, "__builtin_mips_movt_c_olt_ps", MIPS_BUILTIN_MOVT_C_OLT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ult_ps, "__builtin_mips_movt_c_ult_ps", MIPS_BUILTIN_MOVT_C_ULT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ole_ps, "__builtin_mips_movt_c_ole_ps", MIPS_BUILTIN_MOVT_C_OLE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ule_ps, "__builtin_mips_movt_c_ule_ps", MIPS_BUILTIN_MOVT_C_ULE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_sf_ps, "__builtin_mips_movt_c_sf_ps", MIPS_BUILTIN_MOVT_C_SF_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngle_ps, "__builtin_mips_movt_c_ngle_ps", MIPS_BUILTIN_MOVT_C_NGLE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_seq_ps, "__builtin_mips_movt_c_seq_ps", MIPS_BUILTIN_MOVT_C_SEQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngl_ps, "__builtin_mips_movt_c_ngl_ps", MIPS_BUILTIN_MOVT_C_NGL_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_lt_ps, "__builtin_mips_movt_c_lt_ps", MIPS_BUILTIN_MOVT_C_LT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_nge_ps, "__builtin_mips_movt_c_nge_ps", MIPS_BUILTIN_MOVT_C_NGE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_le_ps, "__builtin_mips_movt_c_le_ps", MIPS_BUILTIN_MOVT_C_LE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngt_ps, "__builtin_mips_movt_c_ngt_ps", MIPS_BUILTIN_MOVT_C_NGT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_cabs_f_ps, "__builtin_mips_movt_cabs_f_ps", MIPS_BUILTIN_MOVT_CABS_F_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_un_ps, "__builtin_mips_movt_cabs_un_ps", MIPS_BUILTIN_MOVT_CABS_UN_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_eq_ps, "__builtin_mips_movt_cabs_eq_ps", MIPS_BUILTIN_MOVT_CABS_EQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ueq_ps, "__builtin_mips_movt_cabs_ueq_ps", MIPS_BUILTIN_MOVT_CABS_UEQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_olt_ps, "__builtin_mips_movt_cabs_olt_ps", MIPS_BUILTIN_MOVT_CABS_OLT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ult_ps, "__builtin_mips_movt_cabs_ult_ps", MIPS_BUILTIN_MOVT_CABS_ULT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ole_ps, "__builtin_mips_movt_cabs_ole_ps", MIPS_BUILTIN_MOVT_CABS_OLE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ule_ps, "__builtin_mips_movt_cabs_ule_ps", MIPS_BUILTIN_MOVT_CABS_ULE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_sf_ps, "__builtin_mips_movt_cabs_sf_ps", MIPS_BUILTIN_MOVT_CABS_SF_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngle_ps, "__builtin_mips_movt_cabs_ngle_ps", MIPS_BUILTIN_MOVT_CABS_NGLE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_seq_ps, "__builtin_mips_movt_cabs_seq_ps", MIPS_BUILTIN_MOVT_CABS_SEQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngl_ps, "__builtin_mips_movt_cabs_ngl_ps", MIPS_BUILTIN_MOVT_CABS_NGL_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_lt_ps, "__builtin_mips_movt_cabs_lt_ps", MIPS_BUILTIN_MOVT_CABS_LT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_nge_ps, "__builtin_mips_movt_cabs_nge_ps", MIPS_BUILTIN_MOVT_CABS_NGE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_le_ps, "__builtin_mips_movt_cabs_le_ps", MIPS_BUILTIN_MOVT_CABS_LE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngt_ps, "__builtin_mips_movt_cabs_ngt_ps", MIPS_BUILTIN_MOVT_CABS_NGT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_c_f_ps, "__builtin_mips_movf_c_f_ps", MIPS_BUILTIN_MOVF_C_F_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_un_ps, "__builtin_mips_movf_c_un_ps", MIPS_BUILTIN_MOVF_C_UN_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_eq_ps, "__builtin_mips_movf_c_eq_ps", MIPS_BUILTIN_MOVF_C_EQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ueq_ps, "__builtin_mips_movf_c_ueq_ps", MIPS_BUILTIN_MOVF_C_UEQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_olt_ps, "__builtin_mips_movf_c_olt_ps", MIPS_BUILTIN_MOVF_C_OLT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ult_ps, "__builtin_mips_movf_c_ult_ps", MIPS_BUILTIN_MOVF_C_ULT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ole_ps, "__builtin_mips_movf_c_ole_ps", MIPS_BUILTIN_MOVF_C_OLE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ule_ps, "__builtin_mips_movf_c_ule_ps", MIPS_BUILTIN_MOVF_C_ULE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_sf_ps, "__builtin_mips_movf_c_sf_ps", MIPS_BUILTIN_MOVF_C_SF_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngle_ps, "__builtin_mips_movf_c_ngle_ps", MIPS_BUILTIN_MOVF_C_NGLE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_seq_ps, "__builtin_mips_movf_c_seq_ps", MIPS_BUILTIN_MOVF_C_SEQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngl_ps, "__builtin_mips_movf_c_ngl_ps", MIPS_BUILTIN_MOVF_C_NGL_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_lt_ps, "__builtin_mips_movf_c_lt_ps", MIPS_BUILTIN_MOVF_C_LT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_nge_ps, "__builtin_mips_movf_c_nge_ps", MIPS_BUILTIN_MOVF_C_NGE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_le_ps, "__builtin_mips_movf_c_le_ps", MIPS_BUILTIN_MOVF_C_LE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_c_ngt_ps, "__builtin_mips_movf_c_ngt_ps", MIPS_BUILTIN_MOVF_C_NGT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_PAIRED_SINGLE },
  { CODE_FOR_mips_cabs_f_ps, "__builtin_mips_movf_cabs_f_ps", MIPS_BUILTIN_MOVF_CABS_F_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_un_ps, "__builtin_mips_movf_cabs_un_ps", MIPS_BUILTIN_MOVF_CABS_UN_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_eq_ps, "__builtin_mips_movf_cabs_eq_ps", MIPS_BUILTIN_MOVF_CABS_EQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ueq_ps, "__builtin_mips_movf_cabs_ueq_ps", MIPS_BUILTIN_MOVF_CABS_UEQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_olt_ps, "__builtin_mips_movf_cabs_olt_ps", MIPS_BUILTIN_MOVF_CABS_OLT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ult_ps, "__builtin_mips_movf_cabs_ult_ps", MIPS_BUILTIN_MOVF_CABS_ULT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ole_ps, "__builtin_mips_movf_cabs_ole_ps", MIPS_BUILTIN_MOVF_CABS_OLE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ule_ps, "__builtin_mips_movf_cabs_ule_ps", MIPS_BUILTIN_MOVF_CABS_ULE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_sf_ps, "__builtin_mips_movf_cabs_sf_ps", MIPS_BUILTIN_MOVF_CABS_SF_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngle_ps, "__builtin_mips_movf_cabs_ngle_ps", MIPS_BUILTIN_MOVF_CABS_NGLE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_seq_ps, "__builtin_mips_movf_cabs_seq_ps", MIPS_BUILTIN_MOVF_CABS_SEQ_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngl_ps, "__builtin_mips_movf_cabs_ngl_ps", MIPS_BUILTIN_MOVF_CABS_NGL_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_lt_ps, "__builtin_mips_movf_cabs_lt_ps", MIPS_BUILTIN_MOVF_CABS_LT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_nge_ps, "__builtin_mips_movf_cabs_nge_ps", MIPS_BUILTIN_MOVF_CABS_NGE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_le_ps, "__builtin_mips_movf_cabs_le_ps", MIPS_BUILTIN_MOVF_CABS_LE_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },
  { CODE_FOR_mips_cabs_ngt_ps, "__builtin_mips_movf_cabs_ngt_ps", MIPS_BUILTIN_MOVF_CABS_NGT_PS, MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF, MASK_MIPS3D },

  /* MIPS DSP ASE Revision 0.98 3/24/2005 */
  /* Table 2-1. MIPS DSP ASE Instructions: Arithmetic */
  /* ADDQ* */
  { CODE_FOR_addv2hi3, "__builtin_mips_addq_ph", MIPS_BUILTIN_ADDQ_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_addq_s_ph, "__builtin_mips_addq_s_ph", MIPS_BUILTIN_ADDQ_S_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_addv4hi3, "__builtin_mips_addq_qh", MIPS_BUILTIN_ADDQ_QH, MIPS_V4HI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_addq_s_qh, "__builtin_mips_addq_s_qh", MIPS_BUILTIN_ADDQ_S_QH, MIPS_V4HI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_addq_s_w, "__builtin_mips_addq_s_w", MIPS_BUILTIN_ADDQ_S_W, MIPS_SI_FTYPE_SI_SI, MASK_DSP },
  { CODE_FOR_addv2si3, "__builtin_mips_addq_pw", MIPS_BUILTIN_ADDQ_PW, MIPS_V2SI_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_addq_s_pw, "__builtin_mips_addq_s_pw", MIPS_BUILTIN_ADDQ_S_PW, MIPS_V2SI_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  /* ADDU* */
  { CODE_FOR_addv4qi3, "__builtin_mips_addu_qb", MIPS_BUILTIN_ADDU_QB, MIPS_V4QI_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_addu_s_qb, "__builtin_mips_addu_s_qb", MIPS_BUILTIN_ADDU_S_QB, MIPS_V4QI_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_addv8qi3, "__builtin_mips_addu_ob", MIPS_BUILTIN_ADDU_OB, MIPS_V8QI_FTYPE_V8QI_V8QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_addu_s_ob, "__builtin_mips_addu_s_ob", MIPS_BUILTIN_ADDU_S_OB, MIPS_V8QI_FTYPE_V8QI_V8QI, MASK_DSP|MASK_64BIT },
  /* SUBQ* */
  { CODE_FOR_subv2hi3, "__builtin_mips_subq_ph", MIPS_BUILTIN_SUBQ_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_subq_s_ph, "__builtin_mips_subq_s_ph", MIPS_BUILTIN_SUBQ_S_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_subv4hi3, "__builtin_mips_subq_qh", MIPS_BUILTIN_SUBQ_QH, MIPS_V4HI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_subq_s_qh, "__builtin_mips_subq_s_qh", MIPS_BUILTIN_SUBQ_S_QH, MIPS_V4HI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_subq_s_w, "__builtin_mips_subq_s_w", MIPS_BUILTIN_SUBQ_S_W, MIPS_SI_FTYPE_SI_SI, MASK_DSP },
  { CODE_FOR_subv2si3, "__builtin_mips_subq_pw", MIPS_BUILTIN_SUBQ_PW, MIPS_V2SI_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_subq_s_pw, "__builtin_mips_subq_s_pw", MIPS_BUILTIN_SUBQ_S_PW, MIPS_V2SI_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  /* SUBU*  */
  { CODE_FOR_subv4qi3, "__builtin_mips_subu_qb", MIPS_BUILTIN_SUBU_QB, MIPS_V4QI_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_subu_s_qb, "__builtin_mips_subu_s_qb", MIPS_BUILTIN_SUBU_S_QB, MIPS_V4QI_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_subv8qi3, "__builtin_mips_subu_ob", MIPS_BUILTIN_SUBU_OB, MIPS_V8QI_FTYPE_V8QI_V8QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_subu_s_ob, "__builtin_mips_subu_s_ob", MIPS_BUILTIN_SUBU_S_OB, MIPS_V8QI_FTYPE_V8QI_V8QI, MASK_DSP|MASK_64BIT },
  /* ADDSC */
  { CODE_FOR_mips_addsc, "__builtin_mips_addsc", MIPS_BUILTIN_ADDSC, MIPS_SI_FTYPE_SI_SI, MASK_DSP },
  /* ADDWC */
  { CODE_FOR_mips_addwc, "__builtin_mips_addwc", MIPS_BUILTIN_ADDWC, MIPS_SI_FTYPE_SI_SI, MASK_DSP },
  /* MODSUB  */
  { CODE_FOR_mips_modsub, "__builtin_mips_modsub", MIPS_BUILTIN_MODSUB, MIPS_SI_FTYPE_SI_SI, MASK_DSP },
  /* RADDU* */
  { CODE_FOR_mips_raddu_w_qb, "__builtin_mips_raddu_w_qb", MIPS_BUILTIN_RADDU_W_QB, MIPS_SI_FTYPE_V4QI, MASK_DSP },
  { CODE_FOR_mips_raddu_l_ob, "__builtin_mips_raddu_l_ob", MIPS_BUILTIN_RADDU_L_OB, MIPS_DI_FTYPE_V8QI, MASK_DSP|MASK_64BIT },
  /* ABSQ* */
  { CODE_FOR_mips_absq_s_ph, "__builtin_mips_absq_s_ph", MIPS_BUILTIN_ABSQ_S_PH, MIPS_V2HI_FTYPE_V2HI, MASK_DSP },
  { CODE_FOR_mips_absq_s_w, "__builtin_mips_absq_s_w", MIPS_BUILTIN_ABSQ_S_W, MIPS_SI_FTYPE_SI, MASK_DSP },
  { CODE_FOR_mips_absq_qh, "__builtin_mips_absq_qh", MIPS_BUILTIN_ABSQ_QH, MIPS_V4HI_FTYPE_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_absq_s_qh, "__builtin_mips_absq_s_qh", MIPS_BUILTIN_ABSQ_S_QH, MIPS_V4HI_FTYPE_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_absq_pw, "__builtin_mips_absq_pw", MIPS_BUILTIN_ABSQ_PW, MIPS_V2SI_FTYPE_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_absq_s_pw, "__builtin_mips_absq_s_pw", MIPS_BUILTIN_ABSQ_S_PW, MIPS_V2SI_FTYPE_V2SI, MASK_DSP|MASK_64BIT },
  /* PRECRQ* */
  { CODE_FOR_mips_precrq_qb_ph, "__builtin_mips_precrq_qb_ph", MIPS_BUILTIN_PRECRQ_QB_PH, MIPS_V4QI_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_precrq_ph_w, "__builtin_mips_precrq_ph_w", MIPS_BUILTIN_PRECRQ_PH_W, MIPS_V2HI_FTYPE_SI_SI, MASK_DSP },
  { CODE_FOR_mips_precrq_rs_ph_w, "__builtin_mips_precrq_rs_ph_w", MIPS_BUILTIN_PRECRQ_RS_PH_W, MIPS_V2HI_FTYPE_SI_SI, MASK_DSP },
  { CODE_FOR_mips_precrq_ob_qh, "__builtin_mips_precrq_ob_qh", MIPS_BUILTIN_PRECRQ_OB_QH, MIPS_V8QI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_precrq_qh_pw, "__builtin_mips_precrq_qh_pw", MIPS_BUILTIN_PRECRQ_QH_PW, MIPS_V4HI_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_precrq_rs_qh_pw, "__builtin_mips_precrq_rs_qh_pw", MIPS_BUILTIN_PRECRQ_RS_QH_PW, MIPS_V4HI_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_precrq_pw_l, "__builtin_mips_precrq_pw_l", MIPS_BUILTIN_PRECRQ_PW_L, MIPS_V2SI_FTYPE_DI_DI, MASK_DSP|MASK_64BIT },
  /* PRECRQU* */
  { CODE_FOR_mips_precrqu_s_qb_ph, "__builtin_mips_precrqu_s_qb_ph", MIPS_BUILTIN_PRECRQU_S_QB_PH, MIPS_V4QI_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_precrqu_s_ob_qh, "__builtin_mips_precrqu_s_ob_qh", MIPS_BUILTIN_PRECRQU_S_OB_QH, MIPS_V8QI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_precrqu_s_qh_pw, "__builtin_mips_precrqu_s_qh_pw", MIPS_BUILTIN_PRECRQU_S_QH_PW, MIPS_V4HI_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_precrqu_s_pw_l, "__builtin_mips_precrqu_s_pw_l", MIPS_BUILTIN_PRECRQU_S_PW_L, MIPS_V2SI_FTYPE_DI_DI, MASK_DSP|MASK_64BIT },
  /* PRECEQ* */
  { CODE_FOR_mips_preceq_w_phl, "__builtin_mips_preceq_w_phl", MIPS_BUILTIN_PRECEQ_W_PHL, MIPS_SI_FTYPE_V2HI, MASK_DSP },
  { CODE_FOR_mips_preceq_w_phr, "__builtin_mips_preceq_w_phr", MIPS_BUILTIN_PRECEQ_W_PHR, MIPS_SI_FTYPE_V2HI, MASK_DSP },
  { CODE_FOR_mips_preceq_pw_qhl, "__builtin_mips_preceq_pw_qhl", MIPS_BUILTIN_PRECEQ_PW_QHL, MIPS_V2SI_FTYPE_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_preceq_pw_qhr, "__builtin_mips_preceq_pw_qhr", MIPS_BUILTIN_PRECEQ_PW_QHR, MIPS_V2SI_FTYPE_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_preceq_pw_qhla, "__builtin_mips_preceq_pw_qhla", MIPS_BUILTIN_PRECEQ_PW_QHLA, MIPS_V2SI_FTYPE_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_preceq_pw_qhra, "__builtin_mips_preceq_pw_qhra", MIPS_BUILTIN_PRECEQ_PW_QHRA, MIPS_V2SI_FTYPE_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_preceq_l_pwl, "__builtin_mips_preceq_l_pwl", MIPS_BUILTIN_PRECEQ_L_PWL, MIPS_DI_FTYPE_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_preceq_l_pwr, "__builtin_mips_preceq_l_pwr", MIPS_BUILTIN_PRECEQ_L_PWR, MIPS_DI_FTYPE_V2SI, MASK_DSP|MASK_64BIT },
  /* PRECEQU* */
  { CODE_FOR_mips_precequ_ph_qbl, "__builtin_mips_precequ_ph_qbl", MIPS_BUILTIN_PRECEQU_PH_QBL, MIPS_V2HI_FTYPE_V4QI, MASK_DSP },
  { CODE_FOR_mips_precequ_ph_qbr, "__builtin_mips_precequ_ph_qbr", MIPS_BUILTIN_PRECEQU_PH_QBR, MIPS_V2HI_FTYPE_V4QI, MASK_DSP },
  { CODE_FOR_mips_precequ_ph_qbla, "__builtin_mips_precequ_ph_qbla", MIPS_BUILTIN_PRECEQU_PH_QBLA, MIPS_V2HI_FTYPE_V4QI, MASK_DSP },
  { CODE_FOR_mips_precequ_ph_qbra, "__builtin_mips_precequ_ph_qbra", MIPS_BUILTIN_PRECEQU_PH_QBRA, MIPS_V2HI_FTYPE_V4QI, MASK_DSP },
  { CODE_FOR_mips_precequ_qh_obl, "__builtin_mips_precequ_qh_obl", MIPS_BUILTIN_PRECEQU_QH_OBL, MIPS_V4HI_FTYPE_V8QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_precequ_qh_obr, "__builtin_mips_precequ_qh_obr", MIPS_BUILTIN_PRECEQU_QH_OBR, MIPS_V4HI_FTYPE_V8QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_precequ_qh_obla, "__builtin_mips_precequ_qh_obla", MIPS_BUILTIN_PRECEQU_QH_OBLA, MIPS_V4HI_FTYPE_V8QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_precequ_qh_obra, "__builtin_mips_precequ_qh_obra", MIPS_BUILTIN_PRECEQU_QH_OBRA, MIPS_V4HI_FTYPE_V8QI, MASK_DSP|MASK_64BIT },
  /* PRECEU* */
  { CODE_FOR_mips_preceu_ph_qbl, "__builtin_mips_preceu_ph_qbl", MIPS_BUILTIN_PRECEU_PH_QBL, MIPS_V2HI_FTYPE_V4QI, MASK_DSP },
  { CODE_FOR_mips_preceu_ph_qbr, "__builtin_mips_preceu_ph_qbr", MIPS_BUILTIN_PRECEU_PH_QBR, MIPS_V2HI_FTYPE_V4QI, MASK_DSP },
  { CODE_FOR_mips_preceu_ph_qbla, "__builtin_mips_preceu_ph_qbla", MIPS_BUILTIN_PRECEU_PH_QBLA, MIPS_V2HI_FTYPE_V4QI, MASK_DSP },
  { CODE_FOR_mips_preceu_ph_qbra, "__builtin_mips_preceu_ph_qbra", MIPS_BUILTIN_PRECEU_PH_QBRA, MIPS_V2HI_FTYPE_V4QI, MASK_DSP },
  /* Table 2-2. MIPS DSP ASE Instructions: Shift */
  /* SHLL* */
  { CODE_FOR_mips_shll_qb, "__builtin_mips_shll_qb", MIPS_BUILTIN_SHLL_QB, MIPS_V4QI_FTYPE_V4QI_SI, MASK_DSP },
  { CODE_FOR_mips_shll_ob, "__builtin_mips_shll_ob", MIPS_BUILTIN_SHLL_OB, MIPS_V8QI_FTYPE_V8QI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_shll_ph, "__builtin_mips_shll_ph", MIPS_BUILTIN_SHLL_PH, MIPS_V2HI_FTYPE_V2HI_SI, MASK_DSP },
  { CODE_FOR_mips_shll_s_ph, "__builtin_mips_shll_s_ph", MIPS_BUILTIN_SHLL_S_PH, MIPS_V2HI_FTYPE_V2HI_SI, MASK_DSP },
  { CODE_FOR_mips_shll_qh, "__builtin_mips_shll_qh", MIPS_BUILTIN_SHLL_QH, MIPS_V4HI_FTYPE_V4HI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_shll_s_qh, "__builtin_mips_shll_s_qh", MIPS_BUILTIN_SHLL_S_QH, MIPS_V4HI_FTYPE_V4HI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_shll_s_w, "__builtin_mips_shll_s_w", MIPS_BUILTIN_SHLL_S_W, MIPS_SI_FTYPE_SI_SI, MASK_DSP },
  { CODE_FOR_mips_shll_pw, "__builtin_mips_shll_pw", MIPS_BUILTIN_SHLL_PW, MIPS_V2SI_FTYPE_V2SI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_shll_s_pw, "__builtin_mips_shll_s_pw", MIPS_BUILTIN_SHLL_S_PW, MIPS_V2SI_FTYPE_V2SI_DI, MASK_DSP|MASK_64BIT },
  /* SHRL* */
  { CODE_FOR_mips_shrl_qb, "__builtin_mips_shrl_qb", MIPS_BUILTIN_SHRL_QB, MIPS_V4QI_FTYPE_V4QI_SI, MASK_DSP },
  { CODE_FOR_mips_shrl_ob, "__builtin_mips_shrl_ob", MIPS_BUILTIN_SHRL_OB, MIPS_V8QI_FTYPE_V8QI_DI, MASK_DSP|MASK_64BIT },
  /* SHRA* */
  { CODE_FOR_mips_shra_ph, "__builtin_mips_shra_ph", MIPS_BUILTIN_SHRA_PH, MIPS_V2HI_FTYPE_V2HI_SI, MASK_DSP },
  { CODE_FOR_mips_shra_r_ph, "__builtin_mips_shra_r_ph", MIPS_BUILTIN_SHRA_R_PH, MIPS_V2HI_FTYPE_V2HI_SI, MASK_DSP },
  { CODE_FOR_mips_shra_r_w, "__builtin_mips_shra_r_w", MIPS_BUILTIN_SHRA_R_W, MIPS_SI_FTYPE_SI_SI, MASK_DSP },
  { CODE_FOR_mips_shra_qh, "__builtin_mips_shra_qh", MIPS_BUILTIN_SHRA_QH, MIPS_V4HI_FTYPE_V4HI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_shra_r_qh, "__builtin_mips_shra_r_qh", MIPS_BUILTIN_SHRA_R_QH, MIPS_V4HI_FTYPE_V4HI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_shra_pw, "__builtin_mips_shra_pw", MIPS_BUILTIN_SHRA_PW, MIPS_V2SI_FTYPE_V2SI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_shra_r_pw, "__builtin_mips_shra_r_pw", MIPS_BUILTIN_SHRA_R_PW, MIPS_V2SI_FTYPE_V2SI_DI, MASK_DSP|MASK_64BIT },
  /* Table 2-3. MIPS DSP ASE Instructions: Multiply */
  /* MULEU* */
  { CODE_FOR_mips_muleu_s_ph_qbl, "__builtin_mips_muleu_s_ph_qbl", MIPS_BUILTIN_MULEU_S_PH_QBL, MIPS_V2HI_FTYPE_V4QI_V2HI, MASK_DSP },
  { CODE_FOR_mips_muleu_s_ph_qbr, "__builtin_mips_muleu_s_ph_qbr", MIPS_BUILTIN_MULEU_S_PH_QBR, MIPS_V2HI_FTYPE_V4QI_V2HI, MASK_DSP },
  /* MULQ* */
  { CODE_FOR_mips_mulq_rs_ph, "__builtin_mips_mulq_rs_ph", MIPS_BUILTIN_MULQ_RS_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_mulq_qh, "__builtin_mips_mulq_qh", MIPS_BUILTIN_MULQ_QH, MIPS_V4HI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_mulq_rs_qh, "__builtin_mips_mulq_rs_qh", MIPS_BUILTIN_MULQ_RS_QH, MIPS_V4HI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  /* MULEQ* */
  { CODE_FOR_mips_muleq_s_w_phl, "__builtin_mips_muleq_s_w_phl", MIPS_BUILTIN_MULEQ_S_W_PHL, MIPS_SI_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_muleq_s_w_phr, "__builtin_mips_muleq_s_w_phr", MIPS_BUILTIN_MULEQ_S_W_PHR, MIPS_SI_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_muleq_pw_qhl, "__builtin_mips_muleq_pw_qhl", MIPS_BUILTIN_MULEQ_PW_QHL, MIPS_V2SI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_muleq_pw_qhr, "__builtin_mips_muleq_pw_qhr", MIPS_BUILTIN_MULEQ_PW_QHR, MIPS_V2SI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_muleq_s_pw_qhl, "__builtin_mips_muleq_s_pw_qhl", MIPS_BUILTIN_MULEQ_S_PW_QHL, MIPS_V2SI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_muleq_s_pw_qhr, "__builtin_mips_muleq_s_pw_qhr", MIPS_BUILTIN_MULEQ_S_PW_QHR, MIPS_V2SI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  /* DPAU* */
  { CODE_FOR_mips_dpau_h_qbl, "__builtin_mips_dpau_h_qbl", MIPS_BUILTIN_DPAU_H_QBL, MIPS_DI_FTYPE_DI_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_dpau_h_qbr, "__builtin_mips_dpau_h_qbr", MIPS_BUILTIN_DPAU_H_QBR, MIPS_DI_FTYPE_DI_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_dpau_h_qbl_64, "__builtin_mips_dpau_h_qbl_64", MIPS_BUILTIN_DPAU_H_QBL_64, MIPS_TI_FTYPE_TI_V4QI_V4QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dpau_h_qbr_64, "__builtin_mips_dpau_h_qbr_64", MIPS_BUILTIN_DPAU_H_QBR_64, MIPS_TI_FTYPE_TI_V4QI_V4QI, MASK_DSP|MASK_64BIT },
  /* DPSU* */
  { CODE_FOR_mips_dpsu_h_qbl, "__builtin_mips_dpsu_h_qbl", MIPS_BUILTIN_DPSU_H_QBL, MIPS_DI_FTYPE_DI_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_dpsu_h_qbr, "__builtin_mips_dpsu_h_qbr", MIPS_BUILTIN_DPSU_H_QBR, MIPS_DI_FTYPE_DI_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_dpsu_h_qbl_64, "__builtin_mips_dpsu_h_qbl_64", MIPS_BUILTIN_DPSU_H_QBL_64, MIPS_TI_FTYPE_TI_V4QI_V4QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dpsu_h_qbr_64, "__builtin_mips_dpsu_h_qbr_64", MIPS_BUILTIN_DPSU_H_QBR_64, MIPS_TI_FTYPE_TI_V4QI_V4QI, MASK_DSP|MASK_64BIT },
  /* DPAQ* */
  { CODE_FOR_mips_dpaq_s_w_ph, "__builtin_mips_dpaq_s_w_ph", MIPS_BUILTIN_DPAQ_S_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_dpaq_s_w_ph_64, "__builtin_mips_dpaq_s_w_ph_64", MIPS_BUILTIN_DPAQ_S_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dpaq_w_qh, "__builtin_mips_dpaq_w_qh", MIPS_BUILTIN_DPAQ_W_QH, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dpaq_s_w_qh, "__builtin_mips_dpaq_s_w_qh", MIPS_BUILTIN_DPAQ_S_W_QH, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  /* DPSQ* */
  { CODE_FOR_mips_dpsq_s_w_ph, "__builtin_mips_dpsq_s_w_ph", MIPS_BUILTIN_DPSQ_S_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_dpsq_s_w_ph_64, "__builtin_mips_dpsq_s_w_ph_64", MIPS_BUILTIN_DPSQ_S_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dpsq_w_qh, "__builtin_mips_dpsq_w_qh", MIPS_BUILTIN_DPSQ_W_QH, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dpsq_s_w_qh, "__builtin_mips_dpsq_s_w_qh", MIPS_BUILTIN_DPSQ_S_W_QH, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  /* MULSAQ* */
  { CODE_FOR_mips_mulsaq_s_w_ph, "__builtin_mips_mulsaq_s_w_ph", MIPS_BUILTIN_MULSAQ_S_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_mulsaq_s_w_ph_64, "__builtin_mips_mulsaq_s_w_ph_64", MIPS_BUILTIN_MULSAQ_S_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_mulsaq_w_qh, "__builtin_mips_mulsaq_w_qh", MIPS_BUILTIN_MULSAQ_W_QH, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_mulsaq_s_w_qh, "__builtin_mips_mulsaq_s_w_qh", MIPS_BUILTIN_MULSAQ_S_W_QH, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_mulsaq_l_pw, "__builtin_mips_mulsaq_l_pw", MIPS_BUILTIN_MULSAQ_L_PW, MIPS_TI_FTYPE_TI_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_mulsaq_s_l_pw, "__builtin_mips_mulsaq_s_l_pw", MIPS_BUILTIN_MULSAQ_S_L_PW, MIPS_TI_FTYPE_TI_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  /* DPAQ* */
  { CODE_FOR_mips_dpaq_sa_l_w, "__builtin_mips_dpaq_sa_l_w", MIPS_BUILTIN_DPAQ_SA_L_W, MIPS_DI_FTYPE_DI_SI_SI, MASK_DSP },
  { CODE_FOR_mips_dpaq_sa_l_w_64, "__builtin_mips_dpaq_sa_l_w_64", MIPS_BUILTIN_DPAQ_SA_L_W_64, MIPS_TI_FTYPE_TI_SI_SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dpaq_l_pw, "__builtin_mips_dpaq_l_pw", MIPS_BUILTIN_DPAQ_L_PW, MIPS_TI_FTYPE_TI_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dpaq_sa_l_pw, "__builtin_mips_dpaq_sa_l_pw", MIPS_BUILTIN_DPAQ_SA_L_PW, MIPS_TI_FTYPE_TI_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  /* DPSQ* */
  { CODE_FOR_mips_dpsq_sa_l_w, "__builtin_mips_dpsq_sa_l_w", MIPS_BUILTIN_DPSQ_SA_L_W, MIPS_DI_FTYPE_DI_SI_SI, MASK_DSP },
  { CODE_FOR_mips_dpsq_sa_l_w_64, "__builtin_mips_dpsq_sa_l_w_64", MIPS_BUILTIN_DPSQ_SA_L_W_64, MIPS_TI_FTYPE_TI_SI_SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dpsq_l_pw, "__builtin_mips_dpsq_l_pw", MIPS_BUILTIN_DPSQ_L_PW, MIPS_TI_FTYPE_TI_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dpsq_sa_l_pw, "__builtin_mips_dpsq_sa_l_pw", MIPS_BUILTIN_DPSQ_SA_L_PW, MIPS_TI_FTYPE_TI_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  /* MAQ* */
  { CODE_FOR_mips_maq_s_w_phl, "__builtin_mips_maq_s_w_phl", MIPS_BUILTIN_MAQ_S_W_PHL, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_maq_s_w_phr, "__builtin_mips_maq_s_w_phr", MIPS_BUILTIN_MAQ_S_W_PHR, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_maq_s_w_phl_64, "__builtin_mips_maq_s_w_phl_64", MIPS_BUILTIN_MAQ_S_W_PHL_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_s_w_phr_64, "__builtin_mips_maq_s_w_phr_64", MIPS_BUILTIN_MAQ_S_W_PHR_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_w_qhll, "__builtin_mips_maq_w_qhll", MIPS_BUILTIN_MAQ_W_QHLL, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_w_qhlr, "__builtin_mips_maq_w_qhlr", MIPS_BUILTIN_MAQ_W_QHLR, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_w_qhrl, "__builtin_mips_maq_w_qhrl", MIPS_BUILTIN_MAQ_W_QHRL, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_w_qhrr, "__builtin_mips_maq_w_qhrr", MIPS_BUILTIN_MAQ_W_QHRR, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_s_w_qhll, "__builtin_mips_maq_s_w_qhll", MIPS_BUILTIN_MAQ_S_W_QHLL, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_s_w_qhlr, "__builtin_mips_maq_s_w_qhlr", MIPS_BUILTIN_MAQ_S_W_QHLR, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_s_w_qhrl, "__builtin_mips_maq_s_w_qhrl", MIPS_BUILTIN_MAQ_S_W_QHRL, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_s_w_qhrr, "__builtin_mips_maq_s_w_qhrr", MIPS_BUILTIN_MAQ_S_W_QHRR, MIPS_TI_FTYPE_TI_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_l_pwl, "__builtin_mips_maq_l_pwl", MIPS_BUILTIN_MAQ_L_PWL, MIPS_TI_FTYPE_TI_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_l_pwr, "__builtin_mips_maq_l_pwr", MIPS_BUILTIN_MAQ_L_PWR, MIPS_TI_FTYPE_TI_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_s_l_pwl, "__builtin_mips_maq_s_l_pwl", MIPS_BUILTIN_MAQ_S_L_PWL, MIPS_TI_FTYPE_TI_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_s_l_pwr, "__builtin_mips_maq_s_l_pwr", MIPS_BUILTIN_MAQ_S_L_PWR, MIPS_TI_FTYPE_TI_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  /* MAQ_SA* */
  { CODE_FOR_mips_maq_sa_w_phl, "__builtin_mips_maq_sa_w_phl", MIPS_BUILTIN_MAQ_SA_W_PHL, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_maq_sa_w_phr, "__builtin_mips_maq_sa_w_phr", MIPS_BUILTIN_MAQ_SA_W_PHR, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_maq_sa_w_phl_64, "__builtin_mips_maq_sa_w_phl_64", MIPS_BUILTIN_MAQ_SA_W_PHL_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_maq_sa_w_phr_64, "__builtin_mips_maq_sa_w_phr_64", MIPS_BUILTIN_MAQ_SA_W_PHR_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSP|MASK_64BIT },
  /* DMADD* */
  { CODE_FOR_mips_dmadd, "__builtin_mips_dmadd", MIPS_BUILTIN_DMADD, MIPS_TI_FTYPE_TI_DI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dmaddu, "__builtin_mips_dmaddu", MIPS_BUILTIN_DMADDU, MIPS_TI_FTYPE_TI_UDI_UDI, MASK_DSP|MASK_64BIT },
  /* DMSUB* */
  { CODE_FOR_mips_dmsub, "__builtin_mips_dmsub", MIPS_BUILTIN_DMSUB, MIPS_TI_FTYPE_TI_DI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dmsubu, "__builtin_mips_dmsubu", MIPS_BUILTIN_DMSUBU, MIPS_TI_FTYPE_TI_UDI_UDI, MASK_DSP|MASK_64BIT },
  /* Table 2-4. MIPS DSP ASE Instructions: General Bit/Manipulation */
  /* BITREV */
  { CODE_FOR_mips_bitrev, "__builtin_mips_bitrev", MIPS_BUILTIN_BITREV, MIPS_SI_FTYPE_SI, MASK_DSP },
  /* INSV */
  { CODE_FOR_mips_insv, "__builtin_mips_insv", MIPS_BUILTIN_INSV, MIPS_SI_FTYPE_SI_SI, MASK_DSP },
  { CODE_FOR_mips_dinsv, "__builtin_mips_dinsv", MIPS_BUILTIN_DINSV, MIPS_DI_FTYPE_DI_DI, MASK_DSP|MASK_64BIT },
  /* REPL* */
  { CODE_FOR_mips_repl_qb, "__builtin_mips_repl_qb", MIPS_BUILTIN_REPL_QB, MIPS_V4QI_FTYPE_SI, MASK_DSP },
  { CODE_FOR_mips_repl_ob, "__builtin_mips_repl_ob", MIPS_BUILTIN_REPL_OB, MIPS_V8QI_FTYPE_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_repl_ph, "__builtin_mips_repl_ph", MIPS_BUILTIN_REPL_PH, MIPS_V2HI_FTYPE_SI, MASK_DSP },
  { CODE_FOR_mips_repl_qh, "__builtin_mips_repl_qh", MIPS_BUILTIN_REPL_QH, MIPS_V4HI_FTYPE_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_repl_pw, "__builtin_mips_repl_pw", MIPS_BUILTIN_REPL_PW, MIPS_V2SI_FTYPE_DI, MASK_DSP|MASK_64BIT },
  /* Table 2-5. MIPS DSP ASE Instructions: Compare-Pick */
  /* CMPU.* CMP.* */
  { CODE_FOR_mips_cmpu_eq_qb, "__builtin_mips_cmpu_eq_qb", MIPS_BUILTIN_CMPU_EQ_QB, MIPS_VOID_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_cmpu_lt_qb, "__builtin_mips_cmpu_lt_qb", MIPS_BUILTIN_CMPU_LT_QB, MIPS_VOID_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_cmpu_le_qb, "__builtin_mips_cmpu_le_qb", MIPS_BUILTIN_CMPU_LE_QB, MIPS_VOID_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_cmpgu_eq_qb, "__builtin_mips_cmpgu_eq_qb", MIPS_BUILTIN_CMPGU_EQ_QB, MIPS_SI_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_cmpgu_lt_qb, "__builtin_mips_cmpgu_lt_qb", MIPS_BUILTIN_CMPGU_LT_QB, MIPS_SI_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_cmpgu_le_qb, "__builtin_mips_cmpgu_le_qb", MIPS_BUILTIN_CMPGU_LE_QB, MIPS_SI_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_cmp_eq_ph, "__builtin_mips_cmp_eq_ph", MIPS_BUILTIN_CMP_EQ_PH, MIPS_VOID_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_cmp_lt_ph, "__builtin_mips_cmp_lt_ph", MIPS_BUILTIN_CMP_LT_PH, MIPS_VOID_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_cmp_le_ph, "__builtin_mips_cmp_le_ph", MIPS_BUILTIN_CMP_LE_PH, MIPS_VOID_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_cmpu_eq_ob, "__builtin_mips_cmpu_eq_ob", MIPS_BUILTIN_CMPU_EQ_OB, MIPS_VOID_FTYPE_V8QI_V8QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_cmpu_lt_ob, "__builtin_mips_cmpu_lt_ob", MIPS_BUILTIN_CMPU_LT_OB, MIPS_VOID_FTYPE_V8QI_V8QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_cmpu_le_ob, "__builtin_mips_cmpu_le_ob", MIPS_BUILTIN_CMPU_LE_OB, MIPS_VOID_FTYPE_V8QI_V8QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_cmp_eq_qh, "__builtin_mips_cmp_eq_qh", MIPS_BUILTIN_CMP_EQ_QH, MIPS_VOID_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_cmp_lt_qh, "__builtin_mips_cmp_lt_qh", MIPS_BUILTIN_CMP_LT_QH, MIPS_VOID_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_cmp_le_qh, "__builtin_mips_cmp_le_qh", MIPS_BUILTIN_CMP_LE_QH, MIPS_VOID_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_cmp_eq_pw, "__builtin_mips_cmp_eq_pw", MIPS_BUILTIN_CMP_EQ_PW, MIPS_VOID_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_cmp_lt_pw, "__builtin_mips_cmp_lt_pw", MIPS_BUILTIN_CMP_LT_PW, MIPS_VOID_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_cmp_le_pw, "__builtin_mips_cmp_le_pw", MIPS_BUILTIN_CMP_LE_PW, MIPS_VOID_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  /* PICK* */
  { CODE_FOR_mips_pick_qb, "__builtin_mips_pick_qb", MIPS_BUILTIN_PICK_QB, MIPS_V4QI_FTYPE_V4QI_V4QI, MASK_DSP },
  { CODE_FOR_mips_pick_ph, "__builtin_mips_pick_ph", MIPS_BUILTIN_PICK_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSP },
  { CODE_FOR_mips_pick_ob, "__builtin_mips_pick_ob", MIPS_BUILTIN_PICK_OB, MIPS_V8QI_FTYPE_V8QI_V8QI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_pick_qh, "__builtin_mips_pick_qh", MIPS_BUILTIN_PICK_QH, MIPS_V4HI_FTYPE_V4HI_V4HI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_pick_pw, "__builtin_mips_pick_pw", MIPS_BUILTIN_PICK_PW, MIPS_V2SI_FTYPE_V2SI_V2SI, MASK_DSP|MASK_64BIT },
  /* PACKRL* */
  { CODE_FOR_mips_packrl_ph, "__builtin_mips_packrl_ph", MIPS_BUILTIN_PACKRL_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSP },
  /* Table 2-6. MIPS DSP ASE Instructions: Accumulator and DSPControl Access */
  /* EXTR* */
  { CODE_FOR_mips_extr_w, "__builtin_mips_extr_w", MIPS_BUILTIN_EXTR_W, MIPS_SI_FTYPE_DI_SI, MASK_DSP },
  { CODE_FOR_mips_extr_r_w, "__builtin_mips_extr_r_w", MIPS_BUILTIN_EXTR_R_W, MIPS_SI_FTYPE_DI_SI, MASK_DSP },
  { CODE_FOR_mips_extr_rs_w, "__builtin_mips_extr_rs_w", MIPS_BUILTIN_EXTR_RS_W, MIPS_SI_FTYPE_DI_SI, MASK_DSP },
  { CODE_FOR_mips_extr_w_64, "__builtin_mips_extr_w_64", MIPS_BUILTIN_EXTR_W_64, MIPS_SI_FTYPE_TI_SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_extr_r_w_64, "__builtin_mips_extr_r_w_64", MIPS_BUILTIN_EXTR_R_W_64, MIPS_SI_FTYPE_TI_SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_extr_rs_w_64, "__builtin_mips_extr_rs_w_64", MIPS_BUILTIN_EXTR_RS_W_64, MIPS_SI_FTYPE_TI_SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextr_w, "__builtin_mips_dextr_w", MIPS_BUILTIN_DEXTR_W, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextr_r_w, "__builtin_mips_dextr_r_w", MIPS_BUILTIN_DEXTR_R_W, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextr_rs_w, "__builtin_mips_dextr_rs_w", MIPS_BUILTIN_DEXTR_RS_W, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextr_l, "__builtin_mips_dextr_l", MIPS_BUILTIN_DEXTR_L, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextr_r_l, "__builtin_mips_dextr_r_l", MIPS_BUILTIN_DEXTR_R_L, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextr_rs_l, "__builtin_mips_dextr_rs_l", MIPS_BUILTIN_DEXTR_RS_L, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  /* EXTR*_S.H */
  { CODE_FOR_mips_extr_s_h, "__builtin_mips_extr_s_h", MIPS_BUILTIN_EXTR_S_H, MIPS_SI_FTYPE_DI_SI, MASK_DSP },
  { CODE_FOR_mips_extr_s_h_64, "__builtin_mips_extr_s_h_64", MIPS_BUILTIN_EXTR_S_H_64, MIPS_SI_FTYPE_TI_SI, MASK_DSP|MASK_64BIT },
  /* EXTL* */
  { CODE_FOR_mips_dextl_w, "__builtin_mips_dextl_w", MIPS_BUILTIN_DEXTL_W, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextl_s_w, "__builtin_mips_dextl_s_w", MIPS_BUILTIN_DEXTL_S_W, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextl_l, "__builtin_mips_dextl_l", MIPS_BUILTIN_DEXTL_L, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextl_s_l, "__builtin_mips_dextl_s_l", MIPS_BUILTIN_DEXTL_S_L, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  /* EXTP* */
  { CODE_FOR_mips_extp, "__builtin_mips_extp", MIPS_BUILTIN_EXTP, MIPS_SI_FTYPE_DI_SI, MASK_DSP },
  { CODE_FOR_mips_extpdp, "__builtin_mips_extpdp", MIPS_BUILTIN_EXTPDP, MIPS_SI_FTYPE_DI_SI, MASK_DSP },
  { CODE_FOR_mips_extp_64, "__builtin_mips_extp_64", MIPS_BUILTIN_EXTP_64, MIPS_SI_FTYPE_TI_SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_extpdp_64, "__builtin_mips_extpdp_64", MIPS_BUILTIN_EXTPDP_64, MIPS_SI_FTYPE_TI_SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextp, "__builtin_mips_dextp", MIPS_BUILTIN_DEXTP, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dextpdp, "__builtin_mips_dextpdp", MIPS_BUILTIN_DEXTPDP, MIPS_DI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  /* SHILO* */
  { CODE_FOR_mips_shilo, "__builtin_mips_shilo", MIPS_BUILTIN_SHILO, MIPS_DI_FTYPE_DI_SI, MASK_DSP },
  { CODE_FOR_mips_shilo_64, "__builtin_mips_shilo_64", MIPS_BUILTIN_SHILO_64, MIPS_TI_FTYPE_TI_SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dshilo, "__builtin_mips_dshilo", MIPS_BUILTIN_DSHILO, MIPS_TI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  /* MTHLIP* */
  { CODE_FOR_mips_mthlip, "__builtin_mips_mthlip", MIPS_BUILTIN_MTHLIP, MIPS_DI_FTYPE_DI_SI, MASK_DSP },
  { CODE_FOR_mips_mthlip_64, "__builtin_mips_mthlip_64", MIPS_BUILTIN_MTHLIP_64, MIPS_TI_FTYPE_TI_SI, MASK_DSP|MASK_64BIT },
  { CODE_FOR_mips_dmthlip, "__builtin_mips_dmthlip", MIPS_BUILTIN_DMTHLIP, MIPS_TI_FTYPE_TI_DI, MASK_DSP|MASK_64BIT },
  /* WRDSP */
  { CODE_FOR_mips_wrdsp, "__builtin_mips_wrdsp", MIPS_BUILTIN_WRDSP, MIPS_VOID_FTYPE_SI_SI, MASK_DSP },
  /* RDDSP */
  { CODE_FOR_mips_rddsp, "__builtin_mips_rddsp", MIPS_BUILTIN_RDDSP, MIPS_SI_FTYPE_SI, MASK_DSP },
  /* Table 2-7. MIPS DSP ASE Instructions: Indexed-Load */
  /* L*X */
  { CODE_FOR_mips_lbux, "__builtin_mips_lbux", MIPS_BUILTIN_LBUX, MIPS_SI_FTYPE_PTR_SI, MASK_DSP },
  { CODE_FOR_mips_lhx, "__builtin_mips_lhx", MIPS_BUILTIN_LHX, MIPS_SI_FTYPE_PTR_SI, MASK_DSP },
  { CODE_FOR_mips_lwx, "__builtin_mips_lwx", MIPS_BUILTIN_LWX, MIPS_SI_FTYPE_PTR_SI, MASK_DSP },
  { CODE_FOR_mips_ldx, "__builtin_mips_ldx", MIPS_BUILTIN_LDX, MIPS_DI_FTYPE_PTR_DI, MASK_DSP|MASK_64BIT },
  /* Table 2-8. MIPS DSP ASE Instructions: Branch */
  /* BPOSGE32 */
  { CODE_FOR_mips_bposge, "__builtin_mips_bposge32", MIPS_BUILTIN_BPOSGE32, MIPS_SI_FTYPE_VOID, MASK_DSP },
  { CODE_FOR_mips_bposge, "__builtin_mips_bposge64", MIPS_BUILTIN_BPOSGE64, MIPS_SI_FTYPE_VOID, MASK_DSP|MASK_64BIT },

  /* MIPS DSP ASE Rev2 Revision 0.02 5/19/2006 */
  { CODE_FOR_mips_absq_s_qb, "__builtin_mips_absq_s_qb", MIPS_BUILTIN_ABSQ_S_QB, MIPS_V4QI_FTYPE_V4QI, MASK_DSPR2 },
  { CODE_FOR_mips_addu_ph, "__builtin_mips_addu_ph", MIPS_BUILTIN_ADDU_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_addu_s_ph, "__builtin_mips_addu_s_ph", MIPS_BUILTIN_ADDU_S_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_adduh_qb, "__builtin_mips_adduh_qb", MIPS_BUILTIN_ADDUH_QB, MIPS_V4QI_FTYPE_V4QI_V4QI, MASK_DSPR2 },
  { CODE_FOR_mips_adduh_r_qb, "__builtin_mips_adduh_r_qb", MIPS_BUILTIN_ADDUH_R_QB, MIPS_V4QI_FTYPE_V4QI_V4QI, MASK_DSPR2 },
  { CODE_FOR_mips_append, "__builtin_mips_append", MIPS_BUILTIN_APPEND, MIPS_SI_FTYPE_SI_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_balign, "__builtin_mips_balign", MIPS_BUILTIN_BALIGN, MIPS_SI_FTYPE_SI_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_cmpgdu_eq_qb, "__builtin_mips_cmpgdu_eq_qb", MIPS_BUILTIN_CMPGDU_EQ_QB, MIPS_SI_FTYPE_V4QI_V4QI, MASK_DSPR2 },
  { CODE_FOR_mips_cmpgdu_lt_qb, "__builtin_mips_cmpgdu_lt_qb", MIPS_BUILTIN_CMPGDU_LT_QB, MIPS_SI_FTYPE_V4QI_V4QI, MASK_DSPR2 },
  { CODE_FOR_mips_cmpgdu_le_qb, "__builtin_mips_cmpgdu_le_qb", MIPS_BUILTIN_CMPGDU_LE_QB, MIPS_SI_FTYPE_V4QI_V4QI, MASK_DSPR2 },
  { CODE_FOR_mips_dpa_w_ph, "__builtin_mips_dpa_w_ph", MIPS_BUILTIN_DPA_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_dpa_w_ph_64, "__builtin_mips_dpa_w_ph_64", MIPS_BUILTIN_DPA_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSPR2|MASK_64BIT },
  { CODE_FOR_mips_dps_w_ph, "__builtin_mips_dps_w_ph", MIPS_BUILTIN_DPS_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_dps_w_ph_64, "__builtin_mips_dps_w_ph_64", MIPS_BUILTIN_DPS_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSPR2|MASK_64BIT },
  { CODE_FOR_mips_madd, "__builtin_mips_madd", MIPS_BUILTIN_MADD, MIPS_DI_FTYPE_DI_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_maddu, "__builtin_mips_maddu", MIPS_BUILTIN_MADDU, MIPS_DI_FTYPE_DI_USI_USI, MASK_DSPR2 },
  { CODE_FOR_mips_msub, "__builtin_mips_msub", MIPS_BUILTIN_MSUB, MIPS_DI_FTYPE_DI_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_msubu, "__builtin_mips_msubu", MIPS_BUILTIN_MSUBU, MIPS_DI_FTYPE_DI_USI_USI, MASK_DSPR2 },
  { CODE_FOR_mulv2hi3, "__builtin_mips_mul_ph", MIPS_BUILTIN_MUL_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_mul_s_ph, "__builtin_mips_mul_s_ph", MIPS_BUILTIN_MUL_S_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_mulq_rs_w, "__builtin_mips_mulq_rs_w", MIPS_BUILTIN_MULQ_RS_W, MIPS_SI_FTYPE_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_mulq_s_ph, "__builtin_mips_mulq_s_ph", MIPS_BUILTIN_MULQ_S_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_mulq_s_w, "__builtin_mips_mulq_s_w", MIPS_BUILTIN_MULQ_S_W, MIPS_SI_FTYPE_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_mulsa_w_ph, "__builtin_mips_mulsa_w_ph", MIPS_BUILTIN_MULSA_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_mulsa_w_ph_64, "__builtin_mips_mulsa_w_ph_64", MIPS_BUILTIN_MULSA_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSPR2|MASK_64BIT },
  { CODE_FOR_mips_mult, "__builtin_mips_mult", MIPS_BUILTIN_MULT, MIPS_DI_FTYPE_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_multu, "__builtin_mips_multu", MIPS_BUILTIN_MULTU, MIPS_DI_FTYPE_USI_USI, MASK_DSPR2 },
  { CODE_FOR_mips_precr_qb_ph, "__builtin_mips_precr_qb_ph", MIPS_BUILTIN_PRECR_QB_PH, MIPS_V4QI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_precr_sra_ph_w, "__builtin_mips_precr_sra_ph_w", MIPS_BUILTIN_PRECR_SRA_PH_W, MIPS_V2HI_FTYPE_SI_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_precr_sra_r_ph_w, "__builtin_mips_precr_sra_r_ph_w", MIPS_BUILTIN_PRECR_SRA_R_PH_W, MIPS_V2HI_FTYPE_SI_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_prepend, "__builtin_mips_prepend", MIPS_BUILTIN_PREPEND, MIPS_SI_FTYPE_SI_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_shra_qb, "__builtin_mips_shra_qb", MIPS_BUILTIN_SHRA_QB, MIPS_V4QI_FTYPE_V4QI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_shra_r_qb, "__builtin_mips_shra_r_qb", MIPS_BUILTIN_SHRA_R_QB, MIPS_V4QI_FTYPE_V4QI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_shrl_ph, "__builtin_mips_shrl_ph", MIPS_BUILTIN_SHRL_PH, MIPS_V2HI_FTYPE_V2HI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_subu_ph, "__builtin_mips_subu_ph", MIPS_BUILTIN_SUBU_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_subu_s_ph, "__builtin_mips_subu_s_ph", MIPS_BUILTIN_SUBU_S_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_subuh_qb, "__builtin_mips_subuh_qb", MIPS_BUILTIN_SUBUH_QB, MIPS_V4QI_FTYPE_V4QI_V4QI, MASK_DSPR2 },
  { CODE_FOR_mips_subuh_r_qb, "__builtin_mips_subuh_r_qb", MIPS_BUILTIN_SUBUH_R_QB, MIPS_V4QI_FTYPE_V4QI_V4QI, MASK_DSPR2 },
  { CODE_FOR_mips_addqh_ph, "__builtin_mips_addqh_ph", MIPS_BUILTIN_ADDQH_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_addqh_r_ph, "__builtin_mips_addqh_r_ph", MIPS_BUILTIN_ADDQH_R_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_addqh_w, "__builtin_mips_addqh_w", MIPS_BUILTIN_ADDQH_W, MIPS_SI_FTYPE_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_addqh_r_w, "__builtin_mips_addqh_r_w", MIPS_BUILTIN_ADDQH_R_W, MIPS_SI_FTYPE_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_subqh_ph, "__builtin_mips_subqh_ph", MIPS_BUILTIN_SUBQH_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_subqh_r_ph, "__builtin_mips_subqh_r_ph", MIPS_BUILTIN_SUBQH_R_PH, MIPS_V2HI_FTYPE_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_subqh_w, "__builtin_mips_subqh_w", MIPS_BUILTIN_SUBQH_W, MIPS_SI_FTYPE_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_subqh_r_w, "__builtin_mips_subqh_r_w", MIPS_BUILTIN_SUBQH_R_W, MIPS_SI_FTYPE_SI_SI, MASK_DSPR2 },
  { CODE_FOR_mips_dpax_w_ph, "__builtin_mips_dpax_w_ph", MIPS_BUILTIN_DPAX_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_dpsx_w_ph, "__builtin_mips_dpsx_w_ph", MIPS_BUILTIN_DPSX_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_dpaqx_s_w_ph, "__builtin_mips_dpaqx_s_w_ph", MIPS_BUILTIN_DPAQX_S_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_dpaqx_sa_w_ph, "__builtin_mips_dpaqx_sa_w_ph", MIPS_BUILTIN_DPAQX_SA_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_dpsqx_s_w_ph, "__builtin_mips_dpsqx_s_w_ph", MIPS_BUILTIN_DPSQX_S_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_dpsqx_sa_w_ph, "__builtin_mips_dpsqx_sa_w_ph", MIPS_BUILTIN_DPSQX_SA_W_PH, MIPS_DI_FTYPE_DI_V2HI_V2HI, MASK_DSPR2 },
  { CODE_FOR_mips_dpax_w_ph_64, "__builtin_mips_dpax_w_ph_64", MIPS_BUILTIN_DPAX_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSPR2|MASK_64BIT },
  { CODE_FOR_mips_dpsx_w_ph_64, "__builtin_mips_dpsx_w_ph_64", MIPS_BUILTIN_DPSX_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSPR2|MASK_64BIT },
  { CODE_FOR_mips_dpaqx_s_w_ph_64, "__builtin_mips_dpaqx_s_w_ph_64", MIPS_BUILTIN_DPAQX_S_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSPR2|MASK_64BIT },
  { CODE_FOR_mips_dpaqx_sa_w_ph_64, "__builtin_mips_dpaqx_sa_w_ph_64", MIPS_BUILTIN_DPAQX_SA_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSPR2|MASK_64BIT },
  { CODE_FOR_mips_dpsqx_s_w_ph_64, "__builtin_mips_dpsqx_s_w_ph_64", MIPS_BUILTIN_DPSQX_S_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSPR2|MASK_64BIT },
  { CODE_FOR_mips_dpsqx_sa_w_ph_64, "__builtin_mips_dpsqx_sa_w_ph_64", MIPS_BUILTIN_DPSQX_SA_W_PH_64, MIPS_TI_FTYPE_TI_V2HI_V2HI, MASK_DSPR2|MASK_64BIT },

};

/* Expand conditional move builtin functions */
static rtx mips_expand_ps_cond_move_builtin (enum mips_cmp_choice cmp_choice,
                                             rtx target, unsigned int fcode,
                                             tree arglist)
{
  rtx pat;
  enum insn_code icode;
  tree arg0;
  tree arg1;
  tree arg2;
  tree arg3;
  rtx op0;
  rtx op1;
  rtx op2;
  rtx op3;
  enum machine_mode tmode;
  enum machine_mode mode0;
  enum machine_mode mode1;
  rtx temp_target;

  arg0 = TREE_VALUE (arglist);
  arg1 = TREE_VALUE (TREE_CHAIN (arglist));
  arg2 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
  arg3 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (TREE_CHAIN (arglist))));
  op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
  op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
  op2 = expand_expr (arg2, NULL_RTX, VOIDmode, 0);
  op3 = expand_expr (arg3, NULL_RTX, VOIDmode, 0);

  icode = mips_bdesc[fcode].icode;
  tmode = insn_data[icode].operand[0].mode;
  mode0 = insn_data[icode].operand[1].mode;
  mode1 = insn_data[icode].operand[2].mode;

  if (VECTOR_MODE_P (mode0) && op0 == const0_rtx)
    op0 = gen_reg_rtx (mode0);

  if (VECTOR_MODE_P (mode1) && op1 == const0_rtx)
    op1 = gen_reg_rtx (mode1);

  if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
    op0 = copy_to_mode_reg (mode0, op0);

  if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
    op1 = copy_to_mode_reg (mode1, op1);

  /* temp_target is the result of the comparison. */
  temp_target = gen_reg_rtx (tmode);

  pat = GEN_FCN (icode) (temp_target, op0, op1);
  if (! pat)
    return 0;

  emit_insn (pat);

  icode = CODE_FOR_mips_cond_move_tf_ps;
  tmode = insn_data[icode].operand[0].mode;

  if (target == 0
      || GET_MODE (target) != tmode
      || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
    target = gen_reg_rtx (tmode);

  if (VECTOR_MODE_P (tmode) && op2 == const0_rtx)
     op2 = gen_reg_rtx (tmode);

  if (VECTOR_MODE_P (tmode) && op3 == const0_rtx)
     op3 = gen_reg_rtx (tmode);

  /* Let op2 be the same as the tmode */
  if (! (*insn_data[icode].operand[0].predicate) (op2, tmode))
    op2 = copy_to_mode_reg (tmode, op2);

  /* Let op3 be the same as the tmode */
  if (! (*insn_data[icode].operand[0].predicate) (op3, tmode))
    op3 = copy_to_mode_reg (tmode, op3);

  /* Copy op2 to target */
  emit_insn (gen_rtx_SET (tmode, target, op2));

  if (cmp_choice == MIPS_CMP_MOVT)
    emit_insn (gen_mips_cond_move_tf_ps (target, op3, target, temp_target));
  else if (cmp_choice == MIPS_CMP_MOVF)
    emit_insn (gen_mips_cond_move_tf_ps (target, target, op3, temp_target));
  else
    return 0;

  return target;
}

/* Expand comparing four single builtin functions */
rtx
mips_expand_4s_compare_builtin (enum mips_cmp_choice cmp_choice, rtx target,
                                unsigned int fcode, tree arglist)
{
  rtx pat;
  enum insn_code icode;
  tree arg0;
  tree arg1;
  tree arg2;
  tree arg3;
  rtx op0;
  rtx op1;
  rtx op2;
  rtx op3;
  enum machine_mode tmode;
  enum machine_mode mode0;
  enum machine_mode mode1;
  enum machine_mode mode2;
  enum machine_mode mode3;
  rtx temp_target;
  rtx label1;
  rtx label2;
  rtx if_then_else;
  enum rtx_code test_code;
  int compare_value;

  if (target == 0 || GET_MODE (target) != SImode)
    target = gen_reg_rtx (SImode);

  icode = mips_bdesc[fcode].icode;
  arg0 = TREE_VALUE (arglist);
  arg1 = TREE_VALUE (TREE_CHAIN (arglist));
  arg2 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
  arg3 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (TREE_CHAIN (arglist))));
  op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
  op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
  op2 = expand_expr (arg2, NULL_RTX, VOIDmode, 0);
  op3 = expand_expr (arg3, NULL_RTX, VOIDmode, 0);
  tmode = insn_data[icode].operand[0].mode;
  mode0 = insn_data[icode].operand[1].mode;
  mode1 = insn_data[icode].operand[2].mode;
  mode2 = insn_data[icode].operand[3].mode;
  mode3 = insn_data[icode].operand[4].mode;

  if (VECTOR_MODE_P (mode0) && op0 == const0_rtx)
     op0 = gen_reg_rtx (mode0);

  if (VECTOR_MODE_P (mode1) && op1 == const0_rtx)
     op1 = gen_reg_rtx (mode1);

  if (VECTOR_MODE_P (mode2) && op2 == const0_rtx)
     op2 = gen_reg_rtx (mode2);

  if (VECTOR_MODE_P (mode3) && op3 == const0_rtx)
     op3 = gen_reg_rtx (mode3);

  temp_target = gen_reg_rtx (tmode);

  if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
    op0 = copy_to_mode_reg (mode0, op0);

  if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
    op1 = copy_to_mode_reg (mode1, op1);

  if (! (*insn_data[icode].operand[3].predicate) (op2, mode2))
    op2 = copy_to_mode_reg (mode2, op2);

  if (! (*insn_data[icode].operand[4].predicate) (op3, mode3))
    op3 = copy_to_mode_reg (mode3, op3);

  pat = GEN_FCN (icode) (temp_target, op0, op1, op2, op3);
  if (! pat)
    return 0;

  /*
    We fake the value of CCV4 to be
    0, if ANY is true    <-->  NOT 0, if ALL is false
    1, if ALL is true    <-->  NOT 1, if ANY is false

    Thus, we can map "enum mips_cmp_choice" to RTL comparison operators:
    MIPS_CMP_ANY ->   (EQ 0)
    MIPS_CMP_ALL ->   (EQ 1)

    However, because MIPS doesn't have "branch_all" instructions,
    for MIPS_CMP_ALL, we will use (NE 1) and reverse the assignment of
    the target to 1 fisrt and then 0.
  */
  switch (cmp_choice)
    {
    case MIPS_CMP_ANY:
      test_code = EQ;
      compare_value = 0;
      break;

    case MIPS_CMP_ALL:
      test_code = NE;
      compare_value = 1;
      break;

    default:
      return 0;
    }

  if (cmp_choice == MIPS_CMP_ALL)
    emit_move_insn (target, const1_rtx);
  else
    emit_move_insn (target, const0_rtx);

  emit_insn (pat);

  label1 = gen_label_rtx ();
  label2 = gen_label_rtx ();
  if_then_else
    = gen_rtx_IF_THEN_ELSE (VOIDmode,
                            gen_rtx (test_code, CCV4mode, temp_target,
                                     GEN_INT (compare_value)),
                            gen_rtx_LABEL_REF (VOIDmode, label1), pc_rtx);

  emit_jump_insn (gen_rtx_SET (VOIDmode, pc_rtx, if_then_else));
  emit_jump_insn (gen_rtx_SET (VOIDmode, pc_rtx,
                               gen_rtx_LABEL_REF (VOIDmode, label2)));

  emit_barrier ();
  emit_label (label1);

  if (cmp_choice == MIPS_CMP_ALL)
    emit_move_insn (target, const0_rtx);
  else
    emit_move_insn (target, const1_rtx);

  emit_label (label2);

  return target;
}

/* Expand comparing single/double builtin functions */
rtx
mips_expand_compare_builtin (rtx target, unsigned int fcode, tree arglist)
{
  rtx pat;
  enum insn_code icode;
  tree arg0;
  tree arg1;
  rtx op0;
  rtx op1;
  enum machine_mode tmode;
  enum machine_mode mode0;
  enum machine_mode mode1;
  rtx temp_target;
  rtx label1;
  rtx label2;
  rtx if_then_else;
  enum rtx_code test_code;

  if (target == 0 || GET_MODE (target) != SImode)
    target = gen_reg_rtx (SImode);

  icode = mips_bdesc[fcode].icode;
  arg0 = TREE_VALUE (arglist);
  arg1 = TREE_VALUE (TREE_CHAIN (arglist));
  op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
  op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
  tmode = insn_data[icode].operand[0].mode;
  mode0 = insn_data[icode].operand[1].mode;
  mode1 = insn_data[icode].operand[2].mode;

  temp_target = gen_reg_rtx (tmode);

  if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
    op0 = copy_to_mode_reg (mode0, op0);

  if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
    op1 = copy_to_mode_reg (mode1, op1);

  pat = GEN_FCN (icode) (temp_target, op0, op1);
  if (! pat)
    return 0;

  emit_move_insn (target, const0_rtx);
  emit_insn (pat);

  label1 = gen_label_rtx ();
  label2 = gen_label_rtx ();

  test_code =  NE;
  if_then_else
    = gen_rtx_IF_THEN_ELSE (VOIDmode,
                            gen_rtx (test_code, CCmode,
                                     temp_target, const0_rtx),
                            gen_rtx_LABEL_REF (VOIDmode, label1), pc_rtx);

  emit_jump_insn (gen_rtx_SET (VOIDmode, pc_rtx, if_then_else));
  emit_jump_insn (gen_rtx_SET (VOIDmode, pc_rtx,
                               gen_rtx_LABEL_REF (VOIDmode, label2)));

  emit_barrier ();
  emit_label (label1);
  emit_move_insn (target, const1_rtx);
  emit_label (label2);

  return target;
}

/* Expand comparing paired single builtin functions */
rtx
mips_expand_ps_compare_builtin (enum mips_cmp_choice cmp_choice, rtx target,
                                unsigned int fcode, tree arglist)
{
  rtx pat;
  enum insn_code icode;
  tree arg0;
  tree arg1;
  rtx op0;
  rtx op1;
  enum machine_mode tmode;
  enum machine_mode mode0;
  enum machine_mode mode1;
  rtx temp_target;
  rtx label1;
  rtx label2;
  rtx if_then_else;
  enum rtx_code test_code;
  int compare_value;

  if (target == 0 || GET_MODE (target) != SImode)
    target = gen_reg_rtx (SImode);

  icode = mips_bdesc[fcode].icode;
  arg0 = TREE_VALUE (arglist);
  arg1 = TREE_VALUE (TREE_CHAIN (arglist));
  op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
  op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
  tmode = insn_data[icode].operand[0].mode;
  mode0 = insn_data[icode].operand[1].mode;
  mode1 = insn_data[icode].operand[2].mode;

  if (VECTOR_MODE_P (mode0) && op0 == const0_rtx)
     op0 = gen_reg_rtx (mode0);

  if (VECTOR_MODE_P (mode1) && op1 == const0_rtx)
     op1 = gen_reg_rtx (mode1);

  temp_target = gen_reg_rtx (tmode);

  if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
    op0 = copy_to_mode_reg (mode0, op0);

  if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
    op1 = copy_to_mode_reg (mode1, op1);

  pat = GEN_FCN (icode) (temp_target, op0, op1);
  if (! pat)
    return 0;

  /*
    We fake the value of CCV2 to be
    0, if ANY is true    <-->  NOT 0, if ALL is false
    1, if UPPER is true  <-->  NOT 1, if UPPER is false
    2, if LOWER is true  <-->  NOT 2, if LOWER is false
    3, if ALL is true    <-->  NOT 3, if ANY is false

    Thus, we can map "enum mips_cmp_choice" to RTL comparison operators:
    MIPS_CMP_ANY ->   (EQ 0)
    MIPS_CMP_UPPER -> (EQ 1)
    MIPS_CMP_LOWER -> (EQ 2)
    MIPS_CMP_ALL ->   (EQ 3)

    However, because MIPS doesn't have "branch_all" instructions,
    for MIPS_CMP_ALL, we will use (NE 3) and reverse the assignment of
    the target to 1 fisrt and then 0.
  */
  switch (cmp_choice)
    {
    case MIPS_CMP_ANY:
      test_code = EQ;
      compare_value = 0;
      break;

    case MIPS_CMP_UPPER:
      test_code = EQ;
      compare_value = 1;
      break;

    case MIPS_CMP_LOWER:
      test_code = EQ;
      compare_value = 2;
      break;

    case MIPS_CMP_ALL:
      test_code = NE;
      compare_value = 3;
      break;

    default:
      return 0;
    }

  if (cmp_choice == MIPS_CMP_ALL)
    emit_move_insn (target, const1_rtx);
  else
    emit_move_insn (target, const0_rtx);

  emit_insn (pat);

  label1 = gen_label_rtx ();
  label2 = gen_label_rtx ();

  if_then_else
    = gen_rtx_IF_THEN_ELSE (VOIDmode,
                            gen_rtx (test_code, CCV2mode, temp_target,
                                     GEN_INT (compare_value)),
                            gen_rtx_LABEL_REF (VOIDmode, label1), pc_rtx);

  emit_jump_insn (gen_rtx_SET (VOIDmode, pc_rtx, if_then_else));
  emit_jump_insn (gen_rtx_SET (VOIDmode, pc_rtx,
                               gen_rtx_LABEL_REF (VOIDmode, label2)));

  emit_barrier ();
  emit_label (label1);

  if (cmp_choice == MIPS_CMP_ALL)
    emit_move_insn (target, const0_rtx);
  else
    emit_move_insn (target, const1_rtx);

  emit_label (label2);

  return target;
}

/* Expand bposge32/bposge64.  We will generate the following code
Ex:
    mov      target, 0
    bposge32 label1
    nop
    j        label2
label1:
    mov      target, 1
label2:

*/
rtx
mips_expand_bposge (rtx target, int compare_value)
{
  rtx label1;
  rtx label2;
  rtx if_then_else;

  if (target == 0 || GET_MODE (target) != SImode)
    target = gen_reg_rtx (SImode);

  /* Move 0 to target */
  emit_move_insn (target, const0_rtx);

  /* Generate two labels */
  label1 = gen_label_rtx ();
  label2 = gen_label_rtx ();

  /* Generate if_then_else */
  if_then_else
    = gen_rtx_IF_THEN_ELSE (VOIDmode,
                            gen_rtx (GE, CCDSPmode,
                                     gen_rtx_REG (CCDSPmode,
                                                  DSP_CONTROL_PO_REGNUM),
                                     GEN_INT (compare_value)),
                            gen_rtx_LABEL_REF (VOIDmode, label1), pc_rtx);

  emit_jump_insn (gen_rtx_SET (VOIDmode, pc_rtx, if_then_else));
  emit_jump_insn (gen_rtx_SET (VOIDmode, pc_rtx,
                               gen_rtx_LABEL_REF (VOIDmode, label2)));
  emit_barrier ();
  emit_label (label1);
  emit_move_insn (target, const1_rtx);
  emit_label (label2);
  return target;
}

/* The second operand may be (signed/unsigned) immediate with the number
   of bits.  */

rtx
mips_expand_two_operands_with_immediate (rtx target, unsigned int fcode,
                                         tree arglist, int bits, int is_signed)
{
  rtx pat;
  enum insn_code icode;
  tree arg0;
  tree arg1;
  enum machine_mode tmode;
  enum machine_mode mode0;
  enum machine_mode mode1;
  rtx op0;
  rtx op1;
  unsigned HOST_WIDE_INT value;

  icode = mips_bdesc[fcode].icode;
  arg0 = TREE_VALUE (arglist);
  arg1 = TREE_VALUE (TREE_CHAIN (arglist));
  op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
  op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
  tmode = insn_data[icode].operand[0].mode;
  mode0 = insn_data[icode].operand[1].mode;
  mode1 = insn_data[icode].operand[2].mode;

  if (TREE_CODE (arg1) == INTEGER_CST)
  {
    if (is_signed == 1) /* signed */
      {
        if ((TREE_INT_CST_LOW (arg1) + (1 << (bits - 1))) >
            (unsigned HOST_WIDE_INT) ((1 << bits) - 1))
          {
            warning ("argument 2 of `%s' not in the range %d to %d.",
                     mips_bdesc[fcode].name,
                     - (1 << (bits - 1)), (1 << (bits - 1)) - 1);
            value = INTVAL (op1) & ((1 << bits) - 1);
            if (value & (unsigned HOST_WIDE_INT) (1 << (bits - 1)))
              value |= ~ (unsigned HOST_WIDE_INT) ((1 << bits) - 1);
            op1 = GEN_INT (value);
          }
      }
    else /* unsigned */
      {
        if (TREE_INT_CST_LOW (arg1) >
            (unsigned HOST_WIDE_INT) ((1 << bits) - 1))
          {
            warning ("argument 2 of `%s' not in the range 0 to %d.",
                     mips_bdesc[fcode].name, (1 << bits) - 1);
            op1 = GEN_INT (INTVAL (op1) & ((1 << bits) - 1));
          }
      }
  }

  if (VECTOR_MODE_P (mode0) && op0 == const0_rtx)
    op0 = gen_reg_rtx (mode0);
  if (VECTOR_MODE_P (mode1) && op1 == const0_rtx)
    op1 = gen_reg_rtx (mode1);

  if (target == 0
      || GET_MODE (target) != tmode
      || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
    target = gen_reg_rtx (tmode);

  if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
    op0 = copy_to_mode_reg (mode0, op0);
  if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
    op1 = copy_to_mode_reg (mode1, op1);

  pat = GEN_FCN (icode) (target, op0, op1);
  if (! pat)
    return 0;

  emit_insn (pat);
  return target;
}

/* Expand builtin functions */
rtx
mips_expand_builtin (tree exp, rtx target, rtx subtarget ATTRIBUTE_UNUSED,
                     enum machine_mode mode ATTRIBUTE_UNUSED,
                     int ignore ATTRIBUTE_UNUSED)
{
  rtx pat;
  enum insn_code icode;
  tree fndecl = TREE_OPERAND (TREE_OPERAND (exp, 0), 0);
  tree arglist = TREE_OPERAND (exp, 1);
  unsigned int fcode = DECL_FUNCTION_CODE (fndecl);
  tree arg0;
  tree arg1;
  tree arg2;
  enum machine_mode tmode;
  enum machine_mode mode0;
  enum machine_mode mode1;
  enum machine_mode mode2;
  rtx op0;
  rtx op1;
  rtx op2;

  if (TARGET_MIPS16)
    {
      error ("built-in function `%s' not supported for MIPS16",
             IDENTIFIER_POINTER (DECL_NAME (fndecl)));
      return const0_rtx;
    }

  if (TARGET_64BIT)
    {
      switch (fcode)
        {
	case MIPS_BUILTIN_DPAU_H_QBL:
	case MIPS_BUILTIN_DPAU_H_QBR:
	case MIPS_BUILTIN_DPSU_H_QBL:
	case MIPS_BUILTIN_DPSU_H_QBR:
	case MIPS_BUILTIN_DPAQ_S_W_PH:
	case MIPS_BUILTIN_DPSQ_S_W_PH:
	case MIPS_BUILTIN_MULSAQ_S_W_PH:
	case MIPS_BUILTIN_DPAQ_SA_L_W:
	case MIPS_BUILTIN_DPSQ_SA_L_W:
	case MIPS_BUILTIN_MAQ_S_W_PHL:
	case MIPS_BUILTIN_MAQ_S_W_PHR:
	case MIPS_BUILTIN_MAQ_SA_W_PHL:
	case MIPS_BUILTIN_MAQ_SA_W_PHR:
	case MIPS_BUILTIN_EXTR_W:
	case MIPS_BUILTIN_EXTR_R_W:
	case MIPS_BUILTIN_EXTR_RS_W:
	case MIPS_BUILTIN_EXTR_S_H:
	case MIPS_BUILTIN_EXTP:
	case MIPS_BUILTIN_EXTPDP:
	case MIPS_BUILTIN_SHILO:
	case MIPS_BUILTIN_MTHLIP:
	case MIPS_BUILTIN_DPA_W_PH:
	case MIPS_BUILTIN_DPS_W_PH:
	case MIPS_BUILTIN_MADD:
	case MIPS_BUILTIN_MADDU:
	case MIPS_BUILTIN_MSUB:
	case MIPS_BUILTIN_MSUBU:
	case MIPS_BUILTIN_MULSA_W_PH:
	case MIPS_BUILTIN_MULT:
	case MIPS_BUILTIN_MULTU:
	case MIPS_BUILTIN_DPAX_W_PH:
	case MIPS_BUILTIN_DPSX_W_PH:
	case MIPS_BUILTIN_DPAQX_S_W_PH:
	case MIPS_BUILTIN_DPAQX_SA_W_PH:
	case MIPS_BUILTIN_DPSQX_S_W_PH:
	case MIPS_BUILTIN_DPSQX_SA_W_PH:
	  error ("This function `%s' cannot be used in MIPS64.",
		  mips_bdesc[fcode].name);
	  return 0;
 
	default:
	  break;
        }
    }

  switch (fcode)
    {
    /* No Operand */
    /* BPOSGE32 */
    case MIPS_BUILTIN_BPOSGE32:
      return mips_expand_bposge (target, 32);

    /* No Operand */
    /* BPOSGE64 */
    case MIPS_BUILTIN_BPOSGE64:
      return mips_expand_bposge (target, 64);

    /* Two Operands */
    case MIPS_BUILTIN_PLL_PS:
    case MIPS_BUILTIN_PUL_PS:
    case MIPS_BUILTIN_PLU_PS:
    case MIPS_BUILTIN_PUU_PS:
    case MIPS_BUILTIN_CVT_PS_S:
    case MIPS_BUILTIN_ADDR_PS:
    case MIPS_BUILTIN_MULR_PS:
    case MIPS_BUILTIN_RECIP2_S:
    case MIPS_BUILTIN_RECIP2_D:
    case MIPS_BUILTIN_RECIP2_PS:
    case MIPS_BUILTIN_RSQRT2_S:
    case MIPS_BUILTIN_RSQRT2_D:
    case MIPS_BUILTIN_RSQRT2_PS:
    /* ADDQ* */
    case MIPS_BUILTIN_ADDQ_PH:
    case MIPS_BUILTIN_ADDQ_S_PH:
    case MIPS_BUILTIN_ADDQ_QH:
    case MIPS_BUILTIN_ADDQ_S_QH:
    case MIPS_BUILTIN_ADDQ_S_W:
    case MIPS_BUILTIN_ADDQ_PW:
    case MIPS_BUILTIN_ADDQ_S_PW:
    /* ADDU* */
    case MIPS_BUILTIN_ADDU_QB:
    case MIPS_BUILTIN_ADDU_S_QB:
    case MIPS_BUILTIN_ADDU_OB:
    case MIPS_BUILTIN_ADDU_S_OB:
    /* SUBQ* */
    case MIPS_BUILTIN_SUBQ_PH:
    case MIPS_BUILTIN_SUBQ_S_PH:
    case MIPS_BUILTIN_SUBQ_QH:
    case MIPS_BUILTIN_SUBQ_S_QH:
    case MIPS_BUILTIN_SUBQ_S_W:
    case MIPS_BUILTIN_SUBQ_PW:
    case MIPS_BUILTIN_SUBQ_S_PW:
    /* SUBU*  */
    case MIPS_BUILTIN_SUBU_QB:
    case MIPS_BUILTIN_SUBU_S_QB:
    case MIPS_BUILTIN_SUBU_OB:
    case MIPS_BUILTIN_SUBU_S_OB:
    /* ADDSC */
    case MIPS_BUILTIN_ADDSC:
    /* ADDWC */
    case MIPS_BUILTIN_ADDWC:
    /* MODSUB  */
    case MIPS_BUILTIN_MODSUB:
    /* MULEU* */
    case MIPS_BUILTIN_MULEU_S_PH_QBL:
    case MIPS_BUILTIN_MULEU_S_PH_QBR:
    /* MULQ* */
    case MIPS_BUILTIN_MULQ_RS_PH:
    case MIPS_BUILTIN_MULQ_QH:
    case MIPS_BUILTIN_MULQ_RS_QH:
    /* MULEQ* */
    case MIPS_BUILTIN_MULEQ_S_W_PHL:
    case MIPS_BUILTIN_MULEQ_S_W_PHR:
    case MIPS_BUILTIN_MULEQ_PW_QHL:
    case MIPS_BUILTIN_MULEQ_PW_QHR:
    case MIPS_BUILTIN_MULEQ_S_PW_QHL:
    case MIPS_BUILTIN_MULEQ_S_PW_QHR:
    /* INSV */
    case MIPS_BUILTIN_INSV:
    case MIPS_BUILTIN_DINSV:
    /* CMPGU.* */
    case MIPS_BUILTIN_CMPGU_EQ_QB:
    case MIPS_BUILTIN_CMPGU_LT_QB:
    case MIPS_BUILTIN_CMPGU_LE_QB:
    /* PICK* */
    case MIPS_BUILTIN_PICK_QB:
    case MIPS_BUILTIN_PICK_PH:
    case MIPS_BUILTIN_PICK_OB:
    case MIPS_BUILTIN_PICK_QH:
    case MIPS_BUILTIN_PICK_PW:
    /* PACKRL* */
    case MIPS_BUILTIN_PACKRL_PH:
    /* PRECRQ* */
    case MIPS_BUILTIN_PRECRQ_QB_PH:
    case MIPS_BUILTIN_PRECRQ_PH_W:
    case MIPS_BUILTIN_PRECRQ_RS_PH_W:
    case MIPS_BUILTIN_PRECRQ_OB_QH:
    case MIPS_BUILTIN_PRECRQ_QH_PW:
    case MIPS_BUILTIN_PRECRQ_RS_QH_PW:
    case MIPS_BUILTIN_PRECRQ_PW_L:
    /* PRECRQU* */
    case MIPS_BUILTIN_PRECRQU_S_QB_PH:
    case MIPS_BUILTIN_PRECRQU_S_OB_QH:
    case MIPS_BUILTIN_PRECRQU_S_QH_PW:
    case MIPS_BUILTIN_PRECRQU_S_PW_L:
    /* MTHLIP* */
    case MIPS_BUILTIN_MTHLIP:
    case MIPS_BUILTIN_MTHLIP_64:
    case MIPS_BUILTIN_DMTHLIP:
    /* L*X */
    case MIPS_BUILTIN_LBUX:
    case MIPS_BUILTIN_LHX:
    case MIPS_BUILTIN_LWX:
    case MIPS_BUILTIN_LDX:
    /* DSP Rev2 */
    case MIPS_BUILTIN_ADDU_PH:
    case MIPS_BUILTIN_ADDU_S_PH:
    case MIPS_BUILTIN_ADDUH_QB:
    case MIPS_BUILTIN_ADDUH_R_QB:
    case MIPS_BUILTIN_CMPGDU_EQ_QB:
    case MIPS_BUILTIN_CMPGDU_LT_QB:
    case MIPS_BUILTIN_CMPGDU_LE_QB:
    case MIPS_BUILTIN_MUL_PH:
    case MIPS_BUILTIN_MUL_S_PH:
    case MIPS_BUILTIN_MULQ_RS_W:
    case MIPS_BUILTIN_MULQ_S_PH:
    case MIPS_BUILTIN_MULQ_S_W:
    case MIPS_BUILTIN_MULT:
    case MIPS_BUILTIN_MULTU:
    case MIPS_BUILTIN_PRECR_QB_PH:
    case MIPS_BUILTIN_SUBU_PH:
    case MIPS_BUILTIN_SUBU_S_PH:
    case MIPS_BUILTIN_SUBUH_QB:
    case MIPS_BUILTIN_SUBUH_R_QB:
    case MIPS_BUILTIN_ADDQH_PH:
    case MIPS_BUILTIN_ADDQH_R_PH:
    case MIPS_BUILTIN_ADDQH_W:
    case MIPS_BUILTIN_ADDQH_R_W:
    case MIPS_BUILTIN_SUBQH_PH:
    case MIPS_BUILTIN_SUBQH_R_PH:
    case MIPS_BUILTIN_SUBQH_W:
    case MIPS_BUILTIN_SUBQH_R_W:
      icode = mips_bdesc[fcode].icode;
      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
      op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;
      mode1 = insn_data[icode].operand[2].mode;

      if (VECTOR_MODE_P (mode0) && op0 == const0_rtx)
        op0 = gen_reg_rtx (mode0);

      if (VECTOR_MODE_P (mode1) && op1 == const0_rtx)
        op1 = gen_reg_rtx (mode1);

      if (target == 0
          || GET_MODE (target) != tmode
          || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
        target = gen_reg_rtx (tmode);

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
        op0 = copy_to_mode_reg (mode0, op0);
      if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
        op1 = copy_to_mode_reg (mode1, op1);

      pat = GEN_FCN (icode) (target, op0, op1);
      if (! pat)
        return 0;

      emit_insn (pat);
      return target;

    /* Two Operands.  The 2nd operand may be a 3-bit unsigned immediate */
    /* SHLL* */
    case MIPS_BUILTIN_SHLL_QB:
    case MIPS_BUILTIN_SHLL_OB:
    /* SHRL* */
    case MIPS_BUILTIN_SHRL_QB:
    case MIPS_BUILTIN_SHRL_OB:
    /* DSP Rev2 */
    case MIPS_BUILTIN_SHRA_QB:
    case MIPS_BUILTIN_SHRA_R_QB:
      return mips_expand_two_operands_with_immediate (target, fcode, arglist, 
                                                      3, 0);

    /* Two Operands.  The 2nd operand may be a 4-bit unsigned immediate */
    /* SHLL* */
    case MIPS_BUILTIN_SHLL_PH:
    case MIPS_BUILTIN_SHLL_S_PH:
    case MIPS_BUILTIN_SHLL_QH:
    case MIPS_BUILTIN_SHLL_S_QH:
    /* SHRA* */
    case MIPS_BUILTIN_SHRA_PH:
    case MIPS_BUILTIN_SHRA_R_PH:
    case MIPS_BUILTIN_SHRA_QH:
    case MIPS_BUILTIN_SHRA_R_QH:
    /* DSP Rev2 */
    case MIPS_BUILTIN_SHRL_PH:
      return mips_expand_two_operands_with_immediate (target, fcode, arglist, 
                                                      4, 0);

    /* Two Operands.  The 2nd operand may be a 5-bit unsigned immediate */
    /* SHLL* */
    case MIPS_BUILTIN_SHLL_S_W:
    case MIPS_BUILTIN_SHLL_PW:
    case MIPS_BUILTIN_SHLL_S_PW:
    /* SHRA* */
    case MIPS_BUILTIN_SHRA_R_W:
    case MIPS_BUILTIN_SHRA_PW:
    case MIPS_BUILTIN_SHRA_R_PW:
    /* EXTR*_S.H */
    case MIPS_BUILTIN_EXTR_S_H:
    case MIPS_BUILTIN_EXTR_S_H_64:
    /* EXTR* */
    case MIPS_BUILTIN_EXTR_W:
    case MIPS_BUILTIN_EXTR_R_W:
    case MIPS_BUILTIN_EXTR_RS_W:
    case MIPS_BUILTIN_EXTR_W_64:
    case MIPS_BUILTIN_EXTR_R_W_64:
    case MIPS_BUILTIN_EXTR_RS_W_64:
    case MIPS_BUILTIN_DEXTR_W:
    case MIPS_BUILTIN_DEXTR_R_W:
    case MIPS_BUILTIN_DEXTR_RS_W:
    case MIPS_BUILTIN_DEXTR_L:
    case MIPS_BUILTIN_DEXTR_R_L:
    case MIPS_BUILTIN_DEXTR_RS_L:
    /* EXTL* */
    case MIPS_BUILTIN_DEXTL_W:
    case MIPS_BUILTIN_DEXTL_S_W:
    case MIPS_BUILTIN_DEXTL_L:
    case MIPS_BUILTIN_DEXTL_S_L:
    /* EXTP* */
    case MIPS_BUILTIN_EXTP:
    case MIPS_BUILTIN_EXTPDP:
    case MIPS_BUILTIN_EXTP_64:
    case MIPS_BUILTIN_EXTPDP_64:
    case MIPS_BUILTIN_DEXTP:
    case MIPS_BUILTIN_DEXTPDP:
      return mips_expand_two_operands_with_immediate (target, fcode, arglist,
                                                      5, 0);

    /* Two Operands.  The 2nd operand may be a 6-bit signed immediate */
    /* SHILO* */
    case MIPS_BUILTIN_SHILO:
    case MIPS_BUILTIN_SHILO_64:
      return mips_expand_two_operands_with_immediate (target, fcode, arglist,
                                                      6, 1);

    /* Two Operands.  The 2nd operand may be a 7-bit signed immediate */
    case MIPS_BUILTIN_DSHILO:
      return mips_expand_two_operands_with_immediate (target, fcode, arglist,
                                                      7, 1);

    /* One Operand */
    case MIPS_BUILTIN_CVT_S_PL:
    case MIPS_BUILTIN_CVT_S_PU:
    case MIPS_BUILTIN_ABS_PS:
    case MIPS_BUILTIN_CVT_PW_PS:
    case MIPS_BUILTIN_CVT_PS_PW:
    case MIPS_BUILTIN_RECIP1_S:
    case MIPS_BUILTIN_RECIP1_D:
    case MIPS_BUILTIN_RECIP1_PS:
    case MIPS_BUILTIN_RSQRT1_S:
    case MIPS_BUILTIN_RSQRT1_D:
    case MIPS_BUILTIN_RSQRT1_PS:
    /* RADDU* */
    case MIPS_BUILTIN_RADDU_W_QB:
    case MIPS_BUILTIN_RADDU_L_OB:
    /* ABSQ* */
    case MIPS_BUILTIN_ABSQ_S_PH:
    case MIPS_BUILTIN_ABSQ_S_W:
    case MIPS_BUILTIN_ABSQ_QH:
    case MIPS_BUILTIN_ABSQ_S_QH:
    case MIPS_BUILTIN_ABSQ_PW:
    case MIPS_BUILTIN_ABSQ_S_PW:
    /* PRECEQ* */
    case MIPS_BUILTIN_PRECEQ_W_PHL:
    case MIPS_BUILTIN_PRECEQ_W_PHR:
    case MIPS_BUILTIN_PRECEQ_PW_QHL:
    case MIPS_BUILTIN_PRECEQ_PW_QHR:
    case MIPS_BUILTIN_PRECEQ_PW_QHLA:
    case MIPS_BUILTIN_PRECEQ_PW_QHRA:
    case MIPS_BUILTIN_PRECEQ_L_PWL:
    case MIPS_BUILTIN_PRECEQ_L_PWR:
    /* PRECEQU* */
    case MIPS_BUILTIN_PRECEQU_PH_QBL:
    case MIPS_BUILTIN_PRECEQU_PH_QBR:
    case MIPS_BUILTIN_PRECEQU_PH_QBLA:
    case MIPS_BUILTIN_PRECEQU_PH_QBRA:
    case MIPS_BUILTIN_PRECEQU_QH_OBL:
    case MIPS_BUILTIN_PRECEQU_QH_OBR:
    case MIPS_BUILTIN_PRECEQU_QH_OBLA:
    case MIPS_BUILTIN_PRECEQU_QH_OBRA:
    /* BITREV */
    case MIPS_BUILTIN_BITREV:
    /* PRECEU* */
    case MIPS_BUILTIN_PRECEU_PH_QBL:
    case MIPS_BUILTIN_PRECEU_PH_QBR:
    case MIPS_BUILTIN_PRECEU_PH_QBLA:
    case MIPS_BUILTIN_PRECEU_PH_QBRA:
    /* DSP Rev2 */
    case MIPS_BUILTIN_ABSQ_S_QB:
      icode = mips_bdesc[fcode].icode;
      arg0 = TREE_VALUE (arglist);
      op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;

      if (VECTOR_MODE_P (mode0) && op0 == const0_rtx)
        op0 = gen_reg_rtx (mode0);

      if (target == 0
          || GET_MODE (target) != tmode
          || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
        target = gen_reg_rtx (tmode);

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
        op0 = copy_to_mode_reg (mode0, op0);

      pat = GEN_FCN (icode) (target, op0);
      if (! pat)
        return 0;

      emit_insn (pat);
      return target;

    /* Two Operands & no target.  The 2nd operand is a 6-bit unsigned
       immediate. */
    /* WRDSP */
    case MIPS_BUILTIN_WRDSP:
      icode = mips_bdesc[fcode].icode;
      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
      op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
      mode0 = insn_data[icode].operand[0].mode;

      if (TREE_CODE (arg1) != INTEGER_CST
          || TREE_INT_CST_LOW (arg1) > 63 )
        {
          warning ("argument 2 of `%s' not in the range 0 to 63.",
                   mips_bdesc[fcode].name);
          op1 = GEN_INT (INTVAL (op1) & 0x3f);
        }

      if (! (*insn_data[icode].operand[0].predicate) (op0, mode0))
        op0 = copy_to_mode_reg (mode0, op0);

      pat = GEN_FCN (icode) (op0, op1);
      if (! pat)
        return 0;

      emit_insn (pat);
      return 0; /* No Target */

    /* One Operand.  The 1st operand is a 6-bit unsigned literal. */
    /* RDDSP */
    case MIPS_BUILTIN_RDDSP:
      icode = mips_bdesc[fcode].icode;
      arg0 = TREE_VALUE (arglist);
      op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
      tmode = insn_data[icode].operand[0].mode;

      if (TREE_CODE (arg0) != INTEGER_CST
          || TREE_INT_CST_LOW (arg0) > 63 )
        {
          warning ("argument 1 of `%s' not in the range 0 to 63.",
                   mips_bdesc[fcode].name);
          op0 = GEN_INT (INTVAL (op0) & 0x3f);
        }

      if (target == 0
          || GET_MODE (target) != tmode
          || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
        target = gen_reg_rtx (tmode);

      pat = GEN_FCN (icode) (target, op0);
      if (! pat)
        return 0;

      emit_insn (pat);
      return target;

    /* One Operand - The 1st operand may be an 8-bit unsigned literal. */
    /* REPL* */
    case MIPS_BUILTIN_REPL_QB:
    case MIPS_BUILTIN_REPL_OB:
      icode = mips_bdesc[fcode].icode;
      arg0 = TREE_VALUE (arglist);
      op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;

      if (TREE_CODE (arg0) == INTEGER_CST && TREE_INT_CST_LOW (arg0) > 255 )
        {
          warning ("argument 1 of `%s' not in the range 0 to 255.",
                   mips_bdesc[fcode].name);
          op0 = GEN_INT (INTVAL (op0) & 0xff);
        }

      if (target == 0
          || GET_MODE (target) != tmode
          || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
        target = gen_reg_rtx (tmode);

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
        op0 = copy_to_mode_reg (mode0, op0);

      pat = GEN_FCN (icode) (target, op0);
      if (! pat)
        return 0;

      emit_insn (pat);
      return target;

    /* One Operand - The 1st operand may be a 16-bit signed literal. */
    /* REPL_PH, REPL_QH, REPH_PW */
    /* The immediate version of instructions can accept -512 to 511, but
       the variable version of instructions can deal with -32768 to 32767.
       If the immediate operand is not in -32768 to 32767, GCC will give
       warnings and mask out the number to 16 bits.
       NOTE: GCC can generate the immedaite version when the number is -512 to
       511, or the variable version otherwise.  */
    case MIPS_BUILTIN_REPL_PH:
    case MIPS_BUILTIN_REPL_QH:
    case MIPS_BUILTIN_REPL_PW:
      icode = mips_bdesc[fcode].icode;
      arg0 = TREE_VALUE (arglist);
      op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;

      if (TREE_CODE (arg0) == INTEGER_CST && ! SMALL_INT (op0))
        {
          warning ("argument 1 of `%s' not in the range -32768 to 32767.",
                   mips_bdesc[fcode].name);
          op0 = GEN_INT (INTVAL (op0) & 0xffff);
        }

      if (target == 0
          || GET_MODE (target) != tmode
          || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
        target = gen_reg_rtx (tmode);

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
        op0 = copy_to_mode_reg (mode0, op0);

      pat = GEN_FCN (icode) (target, op0);
      if (! pat)
        return 0;

      emit_insn (pat);
      return target;

    /* Two Operands.  No Target */
    /* CMP.* */
    case MIPS_BUILTIN_CMPU_EQ_QB:
    case MIPS_BUILTIN_CMPU_LT_QB:
    case MIPS_BUILTIN_CMPU_LE_QB:
    case MIPS_BUILTIN_CMP_EQ_PH:
    case MIPS_BUILTIN_CMP_LT_PH:
    case MIPS_BUILTIN_CMP_LE_PH:
    case MIPS_BUILTIN_CMPU_EQ_OB:
    case MIPS_BUILTIN_CMPU_LT_OB:
    case MIPS_BUILTIN_CMPU_LE_OB:
    case MIPS_BUILTIN_CMP_EQ_QH:
    case MIPS_BUILTIN_CMP_LT_QH:
    case MIPS_BUILTIN_CMP_LE_QH:
    case MIPS_BUILTIN_CMP_EQ_PW:
    case MIPS_BUILTIN_CMP_LT_PW:
    case MIPS_BUILTIN_CMP_LE_PW:
      icode = mips_bdesc[fcode].icode;
      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
      op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
      mode0 = insn_data[icode].operand[0].mode;
      mode1 = insn_data[icode].operand[1].mode;

      if (! (*insn_data[icode].operand[0].predicate) (op0, mode0))
        op0 = copy_to_mode_reg (mode0, op0);
      if (! (*insn_data[icode].operand[1].predicate) (op1, mode1))
        op1 = copy_to_mode_reg (mode1, op1);

      pat = GEN_FCN (icode) (op0, op1);
      if (! pat)
        return 0;

      emit_insn (pat);
      return 0; /* No target */

    /* Three Operands */
    case MIPS_BUILTIN_ALNV_PS:
    /* DPAU* */
    case MIPS_BUILTIN_DPAU_H_QBL:
    case MIPS_BUILTIN_DPAU_H_QBR:
    case MIPS_BUILTIN_DPAU_H_QBL_64:
    case MIPS_BUILTIN_DPAU_H_QBR_64:
    /* DPSU* */
    case MIPS_BUILTIN_DPSU_H_QBL:
    case MIPS_BUILTIN_DPSU_H_QBR:
    case MIPS_BUILTIN_DPSU_H_QBL_64:
    case MIPS_BUILTIN_DPSU_H_QBR_64:
    /* DPAQ* */
    case MIPS_BUILTIN_DPAQ_S_W_PH:
    case MIPS_BUILTIN_DPAQ_S_W_PH_64:
    case MIPS_BUILTIN_DPAQ_W_QH:
    case MIPS_BUILTIN_DPAQ_S_W_QH:
    /* DPSQ* */
    case MIPS_BUILTIN_DPSQ_S_W_PH:
    case MIPS_BUILTIN_DPSQ_S_W_PH_64:
    case MIPS_BUILTIN_DPSQ_W_QH:
    case MIPS_BUILTIN_DPSQ_S_W_QH:
    /* MULSAQ* */
    case MIPS_BUILTIN_MULSAQ_S_W_PH:
    case MIPS_BUILTIN_MULSAQ_S_W_PH_64:
    case MIPS_BUILTIN_MULSAQ_W_QH:
    case MIPS_BUILTIN_MULSAQ_S_W_QH:
    /* MULSAQ* */
    case MIPS_BUILTIN_MULSAQ_L_PW:
    case MIPS_BUILTIN_MULSAQ_S_L_PW:
    /* DPAQ* */
    case MIPS_BUILTIN_DPAQ_SA_L_W:
    case MIPS_BUILTIN_DPAQ_SA_L_W_64:
    case MIPS_BUILTIN_DPAQ_L_PW:
    case MIPS_BUILTIN_DPAQ_SA_L_PW:
    /* DPSQ* */
    case MIPS_BUILTIN_DPSQ_SA_L_W:
    case MIPS_BUILTIN_DPSQ_SA_L_W_64:
    case MIPS_BUILTIN_DPSQ_L_PW:
    case MIPS_BUILTIN_DPSQ_SA_L_PW:
    /* MAQ* */
    case MIPS_BUILTIN_MAQ_S_W_PHL:
    case MIPS_BUILTIN_MAQ_S_W_PHR:
    case MIPS_BUILTIN_MAQ_S_W_PHL_64:
    case MIPS_BUILTIN_MAQ_S_W_PHR_64:
    case MIPS_BUILTIN_MAQ_W_QHLL:
    case MIPS_BUILTIN_MAQ_W_QHLR:
    case MIPS_BUILTIN_MAQ_W_QHRL:
    case MIPS_BUILTIN_MAQ_W_QHRR:
    case MIPS_BUILTIN_MAQ_S_W_QHLL:
    case MIPS_BUILTIN_MAQ_S_W_QHLR:
    case MIPS_BUILTIN_MAQ_S_W_QHRL:
    case MIPS_BUILTIN_MAQ_S_W_QHRR:
    case MIPS_BUILTIN_MAQ_L_PWL:
    case MIPS_BUILTIN_MAQ_L_PWR:
    case MIPS_BUILTIN_MAQ_S_L_PWL:
    case MIPS_BUILTIN_MAQ_S_L_PWR:
    /* MAQ_SA* */
    case MIPS_BUILTIN_MAQ_SA_W_PHL:
    case MIPS_BUILTIN_MAQ_SA_W_PHR:
    case MIPS_BUILTIN_MAQ_SA_W_PHL_64:
    case MIPS_BUILTIN_MAQ_SA_W_PHR_64:
    /* DMADD* */
    case MIPS_BUILTIN_DMADD:
    case MIPS_BUILTIN_DMADDU:
    /* DMSUB* */
    case MIPS_BUILTIN_DMSUB:
    case MIPS_BUILTIN_DMSUBU:
    /* DSP Rev2 */
    case MIPS_BUILTIN_DPA_W_PH:
    case MIPS_BUILTIN_DPA_W_PH_64:
    case MIPS_BUILTIN_DPS_W_PH:
    case MIPS_BUILTIN_DPS_W_PH_64:
    case MIPS_BUILTIN_MADD:
    case MIPS_BUILTIN_MADDU:
    case MIPS_BUILTIN_MSUB:
    case MIPS_BUILTIN_MSUBU:
    case MIPS_BUILTIN_MULSA_W_PH:
    case MIPS_BUILTIN_MULSA_W_PH_64:
    case MIPS_BUILTIN_DPAX_W_PH:
    case MIPS_BUILTIN_DPSX_W_PH:
    case MIPS_BUILTIN_DPAQX_S_W_PH:
    case MIPS_BUILTIN_DPAQX_SA_W_PH:
    case MIPS_BUILTIN_DPSQX_S_W_PH:
    case MIPS_BUILTIN_DPSQX_SA_W_PH:
    case MIPS_BUILTIN_DPAX_W_PH_64:
    case MIPS_BUILTIN_DPSX_W_PH_64:
    case MIPS_BUILTIN_DPAQX_S_W_PH_64:
    case MIPS_BUILTIN_DPAQX_SA_W_PH_64:
    case MIPS_BUILTIN_DPSQX_S_W_PH_64:
    case MIPS_BUILTIN_DPSQX_SA_W_PH_64:
      icode = mips_bdesc[fcode].icode;
      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      arg2 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
      op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
      op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
      op2 = expand_expr (arg2, NULL_RTX, VOIDmode, 0);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;
      mode1 = insn_data[icode].operand[2].mode;
      mode2 = insn_data[icode].operand[3].mode;

      if (VECTOR_MODE_P (mode0) && op0 == const0_rtx)
        op0 = gen_reg_rtx (mode0);

      if (VECTOR_MODE_P (mode1) && op1 == const0_rtx)
        op1 = gen_reg_rtx (mode1);

      if (VECTOR_MODE_P (mode2) && op2 == const0_rtx)
        op2 = gen_reg_rtx (mode2);

      if (target == 0
          || GET_MODE (target) != tmode
          || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
        target = gen_reg_rtx (tmode);

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
        op0 = copy_to_mode_reg (mode0, op0);

      if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
        op1 = copy_to_mode_reg (mode1, op1);

      if (! (*insn_data[icode].operand[3].predicate) (op2, mode2))
        op2 = copy_to_mode_reg (mode2, op2);

      pat = GEN_FCN (icode) (target, op0, op1, op2);
      if (! pat)
        return 0;

      emit_insn (pat);
      return target;

    /* Three Operands.  The 3nd operand is a 2-bit unsigned immediate. */
    /* DSP Rev2 */
    case MIPS_BUILTIN_BALIGN:
      icode = mips_bdesc[fcode].icode;
      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      arg2 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
      op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
      op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
      op2 = expand_expr (arg2, NULL_RTX, VOIDmode, 0);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;
      mode1 = insn_data[icode].operand[2].mode;
      mode2 = insn_data[icode].operand[3].mode;

      if (VECTOR_MODE_P (mode0) && op0 == const0_rtx)
	op0 = gen_reg_rtx (mode0);

      if (VECTOR_MODE_P (mode1) && op1 == const0_rtx)
	op1 = gen_reg_rtx (mode1);

      if (TREE_CODE (arg2) != INTEGER_CST || TREE_INT_CST_LOW (arg2) > 4 )
        {
	  warning ("argument 3 of `%s' not in the range 0 to 3.",
		   mips_bdesc[fcode].name);
	  op2 = GEN_INT (INTVAL (op2) & 0x1f);
        }

      if (target == 0
	  || GET_MODE (target) != tmode
	  || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
        target = gen_reg_rtx (tmode);

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
        op0 = copy_to_mode_reg (mode0, op0);

      if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
        op1 = copy_to_mode_reg (mode1, op1);

      pat = GEN_FCN (icode) (target, op0, op1, op2);
      if (! pat)
        return 0;
      
      emit_insn (pat);
      return target;

    /* Three Operands.  The 3nd operand is a 5-bit unsigned immediate. */
    /* DSP Rev2 */
    case MIPS_BUILTIN_APPEND:
    case MIPS_BUILTIN_PRECR_SRA_PH_W:
    case MIPS_BUILTIN_PRECR_SRA_R_PH_W:
    case MIPS_BUILTIN_PREPEND:
      icode = mips_bdesc[fcode].icode;
      arg0 = TREE_VALUE (arglist);
      arg1 = TREE_VALUE (TREE_CHAIN (arglist));
      arg2 = TREE_VALUE (TREE_CHAIN (TREE_CHAIN (arglist)));
      op0 = expand_expr (arg0, NULL_RTX, VOIDmode, 0);
      op1 = expand_expr (arg1, NULL_RTX, VOIDmode, 0);
      op2 = expand_expr (arg2, NULL_RTX, VOIDmode, 0);
      tmode = insn_data[icode].operand[0].mode;
      mode0 = insn_data[icode].operand[1].mode;
      mode1 = insn_data[icode].operand[2].mode;
      mode2 = insn_data[icode].operand[3].mode;

      if (VECTOR_MODE_P (mode0) && op0 == const0_rtx)
	op0 = gen_reg_rtx (mode0);

      if (VECTOR_MODE_P (mode1) && op1 == const0_rtx)
	op1 = gen_reg_rtx (mode1);

      if (TREE_CODE (arg2) != INTEGER_CST || TREE_INT_CST_LOW (arg2) > 31 )
        {
	  warning ("argument 3 of `%s' not in the range 0 to 31.",
		   mips_bdesc[fcode].name);
	  op2 = GEN_INT (INTVAL (op2) & 0x1f);
        }

      if (target == 0
	  || GET_MODE (target) != tmode
	  || ! (*insn_data[icode].operand[0].predicate) (target, tmode))
        target = gen_reg_rtx (tmode);

      if (! (*insn_data[icode].operand[1].predicate) (op0, mode0))
        op0 = copy_to_mode_reg (mode0, op0);

      if (! (*insn_data[icode].operand[2].predicate) (op1, mode1))
        op1 = copy_to_mode_reg (mode1, op1);

      pat = GEN_FCN (icode) (target, op0, op1, op2);
      if (! pat)
        return 0;
      
      emit_insn (pat);
      return target;

    /* Paired Single Comparison */
    case MIPS_BUILTIN_ANY_C_F_PS:
    case MIPS_BUILTIN_ANY_C_UN_PS:
    case MIPS_BUILTIN_ANY_C_EQ_PS:
    case MIPS_BUILTIN_ANY_C_UEQ_PS:
    case MIPS_BUILTIN_ANY_C_OLT_PS:
    case MIPS_BUILTIN_ANY_C_ULT_PS:
    case MIPS_BUILTIN_ANY_C_OLE_PS:
    case MIPS_BUILTIN_ANY_C_ULE_PS:
    case MIPS_BUILTIN_ANY_C_SF_PS:
    case MIPS_BUILTIN_ANY_C_NGLE_PS:
    case MIPS_BUILTIN_ANY_C_SEQ_PS:
    case MIPS_BUILTIN_ANY_C_NGL_PS:
    case MIPS_BUILTIN_ANY_C_LT_PS:
    case MIPS_BUILTIN_ANY_C_NGE_PS:
    case MIPS_BUILTIN_ANY_C_LE_PS:
    case MIPS_BUILTIN_ANY_C_NGT_PS:
    case MIPS_BUILTIN_ANY_CABS_F_PS:
    case MIPS_BUILTIN_ANY_CABS_UN_PS:
    case MIPS_BUILTIN_ANY_CABS_EQ_PS:
    case MIPS_BUILTIN_ANY_CABS_UEQ_PS:
    case MIPS_BUILTIN_ANY_CABS_OLT_PS:
    case MIPS_BUILTIN_ANY_CABS_ULT_PS:
    case MIPS_BUILTIN_ANY_CABS_OLE_PS:
    case MIPS_BUILTIN_ANY_CABS_ULE_PS:
    case MIPS_BUILTIN_ANY_CABS_SF_PS:
    case MIPS_BUILTIN_ANY_CABS_NGLE_PS:
    case MIPS_BUILTIN_ANY_CABS_SEQ_PS:
    case MIPS_BUILTIN_ANY_CABS_NGL_PS:
    case MIPS_BUILTIN_ANY_CABS_LT_PS:
    case MIPS_BUILTIN_ANY_CABS_NGE_PS:
    case MIPS_BUILTIN_ANY_CABS_LE_PS:
    case MIPS_BUILTIN_ANY_CABS_NGT_PS:
      return mips_expand_ps_compare_builtin (MIPS_CMP_ANY, target,
                                             fcode, arglist);

    /* Paired Single Comparison */
    case MIPS_BUILTIN_UPPER_C_F_PS:
    case MIPS_BUILTIN_UPPER_C_UN_PS:
    case MIPS_BUILTIN_UPPER_C_EQ_PS:
    case MIPS_BUILTIN_UPPER_C_UEQ_PS:
    case MIPS_BUILTIN_UPPER_C_OLT_PS:
    case MIPS_BUILTIN_UPPER_C_ULT_PS:
    case MIPS_BUILTIN_UPPER_C_OLE_PS:
    case MIPS_BUILTIN_UPPER_C_ULE_PS:
    case MIPS_BUILTIN_UPPER_C_SF_PS:
    case MIPS_BUILTIN_UPPER_C_NGLE_PS:
    case MIPS_BUILTIN_UPPER_C_SEQ_PS:
    case MIPS_BUILTIN_UPPER_C_NGL_PS:
    case MIPS_BUILTIN_UPPER_C_LT_PS:
    case MIPS_BUILTIN_UPPER_C_NGE_PS:
    case MIPS_BUILTIN_UPPER_C_LE_PS:
    case MIPS_BUILTIN_UPPER_C_NGT_PS:
    case MIPS_BUILTIN_UPPER_CABS_F_PS:
    case MIPS_BUILTIN_UPPER_CABS_UN_PS:
    case MIPS_BUILTIN_UPPER_CABS_EQ_PS:
    case MIPS_BUILTIN_UPPER_CABS_UEQ_PS:
    case MIPS_BUILTIN_UPPER_CABS_OLT_PS:
    case MIPS_BUILTIN_UPPER_CABS_ULT_PS:
    case MIPS_BUILTIN_UPPER_CABS_OLE_PS:
    case MIPS_BUILTIN_UPPER_CABS_ULE_PS:
    case MIPS_BUILTIN_UPPER_CABS_SF_PS:
    case MIPS_BUILTIN_UPPER_CABS_NGLE_PS:
    case MIPS_BUILTIN_UPPER_CABS_SEQ_PS:
    case MIPS_BUILTIN_UPPER_CABS_NGL_PS:
    case MIPS_BUILTIN_UPPER_CABS_LT_PS:
    case MIPS_BUILTIN_UPPER_CABS_NGE_PS:
    case MIPS_BUILTIN_UPPER_CABS_LE_PS:
    case MIPS_BUILTIN_UPPER_CABS_NGT_PS:
      return mips_expand_ps_compare_builtin (MIPS_CMP_UPPER, target,
                                             fcode, arglist);

    /* Paired Single Comparison */
    case MIPS_BUILTIN_LOWER_C_F_PS:
    case MIPS_BUILTIN_LOWER_C_UN_PS:
    case MIPS_BUILTIN_LOWER_C_EQ_PS:
    case MIPS_BUILTIN_LOWER_C_UEQ_PS:
    case MIPS_BUILTIN_LOWER_C_OLT_PS:
    case MIPS_BUILTIN_LOWER_C_ULT_PS:
    case MIPS_BUILTIN_LOWER_C_OLE_PS:
    case MIPS_BUILTIN_LOWER_C_ULE_PS:
    case MIPS_BUILTIN_LOWER_C_SF_PS:
    case MIPS_BUILTIN_LOWER_C_NGLE_PS:
    case MIPS_BUILTIN_LOWER_C_SEQ_PS:
    case MIPS_BUILTIN_LOWER_C_NGL_PS:
    case MIPS_BUILTIN_LOWER_C_LT_PS:
    case MIPS_BUILTIN_LOWER_C_NGE_PS:
    case MIPS_BUILTIN_LOWER_C_LE_PS:
    case MIPS_BUILTIN_LOWER_C_NGT_PS:
    case MIPS_BUILTIN_LOWER_CABS_F_PS:
    case MIPS_BUILTIN_LOWER_CABS_UN_PS:
    case MIPS_BUILTIN_LOWER_CABS_EQ_PS:
    case MIPS_BUILTIN_LOWER_CABS_UEQ_PS:
    case MIPS_BUILTIN_LOWER_CABS_OLT_PS:
    case MIPS_BUILTIN_LOWER_CABS_ULT_PS:
    case MIPS_BUILTIN_LOWER_CABS_OLE_PS:
    case MIPS_BUILTIN_LOWER_CABS_ULE_PS:
    case MIPS_BUILTIN_LOWER_CABS_SF_PS:
    case MIPS_BUILTIN_LOWER_CABS_NGLE_PS:
    case MIPS_BUILTIN_LOWER_CABS_SEQ_PS:
    case MIPS_BUILTIN_LOWER_CABS_NGL_PS:
    case MIPS_BUILTIN_LOWER_CABS_LT_PS:
    case MIPS_BUILTIN_LOWER_CABS_NGE_PS:
    case MIPS_BUILTIN_LOWER_CABS_LE_PS:
    case MIPS_BUILTIN_LOWER_CABS_NGT_PS:
      return mips_expand_ps_compare_builtin (MIPS_CMP_LOWER, target,
                                             fcode, arglist);

    /* Paired Single Comparison */
    case MIPS_BUILTIN_ALL_C_F_PS:
    case MIPS_BUILTIN_ALL_C_UN_PS:
    case MIPS_BUILTIN_ALL_C_EQ_PS:
    case MIPS_BUILTIN_ALL_C_UEQ_PS:
    case MIPS_BUILTIN_ALL_C_OLT_PS:
    case MIPS_BUILTIN_ALL_C_ULT_PS:
    case MIPS_BUILTIN_ALL_C_OLE_PS:
    case MIPS_BUILTIN_ALL_C_ULE_PS:
    case MIPS_BUILTIN_ALL_C_SF_PS:
    case MIPS_BUILTIN_ALL_C_NGLE_PS:
    case MIPS_BUILTIN_ALL_C_SEQ_PS:
    case MIPS_BUILTIN_ALL_C_NGL_PS:
    case MIPS_BUILTIN_ALL_C_LT_PS:
    case MIPS_BUILTIN_ALL_C_NGE_PS:
    case MIPS_BUILTIN_ALL_C_LE_PS:
    case MIPS_BUILTIN_ALL_C_NGT_PS:
    case MIPS_BUILTIN_ALL_CABS_F_PS:
    case MIPS_BUILTIN_ALL_CABS_UN_PS:
    case MIPS_BUILTIN_ALL_CABS_EQ_PS:
    case MIPS_BUILTIN_ALL_CABS_UEQ_PS:
    case MIPS_BUILTIN_ALL_CABS_OLT_PS:
    case MIPS_BUILTIN_ALL_CABS_ULT_PS:
    case MIPS_BUILTIN_ALL_CABS_OLE_PS:
    case MIPS_BUILTIN_ALL_CABS_ULE_PS:
    case MIPS_BUILTIN_ALL_CABS_SF_PS:
    case MIPS_BUILTIN_ALL_CABS_NGLE_PS:
    case MIPS_BUILTIN_ALL_CABS_SEQ_PS:
    case MIPS_BUILTIN_ALL_CABS_NGL_PS:
    case MIPS_BUILTIN_ALL_CABS_LT_PS:
    case MIPS_BUILTIN_ALL_CABS_NGE_PS:
    case MIPS_BUILTIN_ALL_CABS_LE_PS:
    case MIPS_BUILTIN_ALL_CABS_NGT_PS:
      return mips_expand_ps_compare_builtin (MIPS_CMP_ALL, target,
                                             fcode, arglist);

    /* Four Single Comparison */
    case MIPS_BUILTIN_ANY_C_F_4S:
    case MIPS_BUILTIN_ANY_C_UN_4S:
    case MIPS_BUILTIN_ANY_C_EQ_4S:
    case MIPS_BUILTIN_ANY_C_UEQ_4S:
    case MIPS_BUILTIN_ANY_C_OLT_4S:
    case MIPS_BUILTIN_ANY_C_ULT_4S:
    case MIPS_BUILTIN_ANY_C_OLE_4S:
    case MIPS_BUILTIN_ANY_C_ULE_4S:
    case MIPS_BUILTIN_ANY_C_SF_4S:
    case MIPS_BUILTIN_ANY_C_NGLE_4S:
    case MIPS_BUILTIN_ANY_C_SEQ_4S:
    case MIPS_BUILTIN_ANY_C_NGL_4S:
    case MIPS_BUILTIN_ANY_C_LT_4S:
    case MIPS_BUILTIN_ANY_C_NGE_4S:
    case MIPS_BUILTIN_ANY_C_LE_4S:
    case MIPS_BUILTIN_ANY_C_NGT_4S:
    case MIPS_BUILTIN_ANY_CABS_F_4S:
    case MIPS_BUILTIN_ANY_CABS_UN_4S:
    case MIPS_BUILTIN_ANY_CABS_EQ_4S:
    case MIPS_BUILTIN_ANY_CABS_UEQ_4S:
    case MIPS_BUILTIN_ANY_CABS_OLT_4S:
    case MIPS_BUILTIN_ANY_CABS_ULT_4S:
    case MIPS_BUILTIN_ANY_CABS_OLE_4S:
    case MIPS_BUILTIN_ANY_CABS_ULE_4S:
    case MIPS_BUILTIN_ANY_CABS_SF_4S:
    case MIPS_BUILTIN_ANY_CABS_NGLE_4S:
    case MIPS_BUILTIN_ANY_CABS_SEQ_4S:
    case MIPS_BUILTIN_ANY_CABS_NGL_4S:
    case MIPS_BUILTIN_ANY_CABS_LT_4S:
    case MIPS_BUILTIN_ANY_CABS_NGE_4S:
    case MIPS_BUILTIN_ANY_CABS_LE_4S:
    case MIPS_BUILTIN_ANY_CABS_NGT_4S:
      return mips_expand_4s_compare_builtin (MIPS_CMP_ANY, target,
                                             fcode, arglist);

    /* Four Single Comparison */
    case MIPS_BUILTIN_ALL_C_F_4S:
    case MIPS_BUILTIN_ALL_C_UN_4S:
    case MIPS_BUILTIN_ALL_C_EQ_4S:
    case MIPS_BUILTIN_ALL_C_UEQ_4S:
    case MIPS_BUILTIN_ALL_C_OLT_4S:
    case MIPS_BUILTIN_ALL_C_ULT_4S:
    case MIPS_BUILTIN_ALL_C_OLE_4S:
    case MIPS_BUILTIN_ALL_C_ULE_4S:
    case MIPS_BUILTIN_ALL_C_SF_4S:
    case MIPS_BUILTIN_ALL_C_NGLE_4S:
    case MIPS_BUILTIN_ALL_C_SEQ_4S:
    case MIPS_BUILTIN_ALL_C_NGL_4S:
    case MIPS_BUILTIN_ALL_C_LT_4S:
    case MIPS_BUILTIN_ALL_C_NGE_4S:
    case MIPS_BUILTIN_ALL_C_LE_4S:
    case MIPS_BUILTIN_ALL_C_NGT_4S:
    case MIPS_BUILTIN_ALL_CABS_F_4S:
    case MIPS_BUILTIN_ALL_CABS_UN_4S:
    case MIPS_BUILTIN_ALL_CABS_EQ_4S:
    case MIPS_BUILTIN_ALL_CABS_UEQ_4S:
    case MIPS_BUILTIN_ALL_CABS_OLT_4S:
    case MIPS_BUILTIN_ALL_CABS_ULT_4S:
    case MIPS_BUILTIN_ALL_CABS_OLE_4S:
    case MIPS_BUILTIN_ALL_CABS_ULE_4S:
    case MIPS_BUILTIN_ALL_CABS_SF_4S:
    case MIPS_BUILTIN_ALL_CABS_NGLE_4S:
    case MIPS_BUILTIN_ALL_CABS_SEQ_4S:
    case MIPS_BUILTIN_ALL_CABS_NGL_4S:
    case MIPS_BUILTIN_ALL_CABS_LT_4S:
    case MIPS_BUILTIN_ALL_CABS_NGE_4S:
    case MIPS_BUILTIN_ALL_CABS_LE_4S:
    case MIPS_BUILTIN_ALL_CABS_NGT_4S:
      return mips_expand_4s_compare_builtin (MIPS_CMP_ALL, target,
                                             fcode, arglist);

    /* Single/Double Compare Absolute */
    case MIPS_BUILTIN_CABS_F_S:
    case MIPS_BUILTIN_CABS_UN_S:
    case MIPS_BUILTIN_CABS_EQ_S:
    case MIPS_BUILTIN_CABS_UEQ_S:
    case MIPS_BUILTIN_CABS_OLT_S:
    case MIPS_BUILTIN_CABS_ULT_S:
    case MIPS_BUILTIN_CABS_OLE_S:
    case MIPS_BUILTIN_CABS_ULE_S:
    case MIPS_BUILTIN_CABS_SF_S:
    case MIPS_BUILTIN_CABS_NGLE_S:
    case MIPS_BUILTIN_CABS_SEQ_S:
    case MIPS_BUILTIN_CABS_NGL_S:
    case MIPS_BUILTIN_CABS_LT_S:
    case MIPS_BUILTIN_CABS_NGE_S:
    case MIPS_BUILTIN_CABS_LE_S:
    case MIPS_BUILTIN_CABS_NGT_S:
    case MIPS_BUILTIN_CABS_F_D:
    case MIPS_BUILTIN_CABS_UN_D:
    case MIPS_BUILTIN_CABS_EQ_D:
    case MIPS_BUILTIN_CABS_UEQ_D:
    case MIPS_BUILTIN_CABS_OLT_D:
    case MIPS_BUILTIN_CABS_ULT_D:
    case MIPS_BUILTIN_CABS_OLE_D:
    case MIPS_BUILTIN_CABS_ULE_D:
    case MIPS_BUILTIN_CABS_SF_D:
    case MIPS_BUILTIN_CABS_NGLE_D:
    case MIPS_BUILTIN_CABS_SEQ_D:
    case MIPS_BUILTIN_CABS_NGL_D:
    case MIPS_BUILTIN_CABS_LT_D:
    case MIPS_BUILTIN_CABS_NGE_D:
    case MIPS_BUILTIN_CABS_LE_D:
    case MIPS_BUILTIN_CABS_NGT_D:
      return mips_expand_compare_builtin (target, fcode, arglist);

    /* Conditional Move on True */
    case MIPS_BUILTIN_MOVT_C_F_PS:
    case MIPS_BUILTIN_MOVT_C_UN_PS:
    case MIPS_BUILTIN_MOVT_C_EQ_PS:
    case MIPS_BUILTIN_MOVT_C_UEQ_PS:
    case MIPS_BUILTIN_MOVT_C_OLT_PS:
    case MIPS_BUILTIN_MOVT_C_ULT_PS:
    case MIPS_BUILTIN_MOVT_C_OLE_PS:
    case MIPS_BUILTIN_MOVT_C_ULE_PS:
    case MIPS_BUILTIN_MOVT_C_SF_PS:
    case MIPS_BUILTIN_MOVT_C_NGLE_PS:
    case MIPS_BUILTIN_MOVT_C_SEQ_PS:
    case MIPS_BUILTIN_MOVT_C_NGL_PS:
    case MIPS_BUILTIN_MOVT_C_LT_PS:
    case MIPS_BUILTIN_MOVT_C_NGE_PS:
    case MIPS_BUILTIN_MOVT_C_LE_PS:
    case MIPS_BUILTIN_MOVT_C_NGT_PS:
    case MIPS_BUILTIN_MOVT_CABS_F_PS:
    case MIPS_BUILTIN_MOVT_CABS_UN_PS:
    case MIPS_BUILTIN_MOVT_CABS_EQ_PS:
    case MIPS_BUILTIN_MOVT_CABS_UEQ_PS:
    case MIPS_BUILTIN_MOVT_CABS_OLT_PS:
    case MIPS_BUILTIN_MOVT_CABS_ULT_PS:
    case MIPS_BUILTIN_MOVT_CABS_OLE_PS:
    case MIPS_BUILTIN_MOVT_CABS_ULE_PS:
    case MIPS_BUILTIN_MOVT_CABS_SF_PS:
    case MIPS_BUILTIN_MOVT_CABS_NGLE_PS:
    case MIPS_BUILTIN_MOVT_CABS_SEQ_PS:
    case MIPS_BUILTIN_MOVT_CABS_NGL_PS:
    case MIPS_BUILTIN_MOVT_CABS_LT_PS:
    case MIPS_BUILTIN_MOVT_CABS_NGE_PS:
    case MIPS_BUILTIN_MOVT_CABS_LE_PS:
    case MIPS_BUILTIN_MOVT_CABS_NGT_PS:
      return mips_expand_ps_cond_move_builtin (MIPS_CMP_MOVT, target,
                                               fcode, arglist);

    /* Conditional Move on False */
    case MIPS_BUILTIN_MOVF_C_F_PS:
    case MIPS_BUILTIN_MOVF_C_UN_PS:
    case MIPS_BUILTIN_MOVF_C_EQ_PS:
    case MIPS_BUILTIN_MOVF_C_UEQ_PS:
    case MIPS_BUILTIN_MOVF_C_OLT_PS:
    case MIPS_BUILTIN_MOVF_C_ULT_PS:
    case MIPS_BUILTIN_MOVF_C_OLE_PS:
    case MIPS_BUILTIN_MOVF_C_ULE_PS:
    case MIPS_BUILTIN_MOVF_C_SF_PS:
    case MIPS_BUILTIN_MOVF_C_NGLE_PS:
    case MIPS_BUILTIN_MOVF_C_SEQ_PS:
    case MIPS_BUILTIN_MOVF_C_NGL_PS:
    case MIPS_BUILTIN_MOVF_C_LT_PS:
    case MIPS_BUILTIN_MOVF_C_NGE_PS:
    case MIPS_BUILTIN_MOVF_C_LE_PS:
    case MIPS_BUILTIN_MOVF_C_NGT_PS:
    case MIPS_BUILTIN_MOVF_CABS_F_PS:
    case MIPS_BUILTIN_MOVF_CABS_UN_PS:
    case MIPS_BUILTIN_MOVF_CABS_EQ_PS:
    case MIPS_BUILTIN_MOVF_CABS_UEQ_PS:
    case MIPS_BUILTIN_MOVF_CABS_OLT_PS:
    case MIPS_BUILTIN_MOVF_CABS_ULT_PS:
    case MIPS_BUILTIN_MOVF_CABS_OLE_PS:
    case MIPS_BUILTIN_MOVF_CABS_ULE_PS:
    case MIPS_BUILTIN_MOVF_CABS_SF_PS:
    case MIPS_BUILTIN_MOVF_CABS_NGLE_PS:
    case MIPS_BUILTIN_MOVF_CABS_SEQ_PS:
    case MIPS_BUILTIN_MOVF_CABS_NGL_PS:
    case MIPS_BUILTIN_MOVF_CABS_LT_PS:
    case MIPS_BUILTIN_MOVF_CABS_NGE_PS:
    case MIPS_BUILTIN_MOVF_CABS_LE_PS:
    case MIPS_BUILTIN_MOVF_CABS_NGT_PS:
      return mips_expand_ps_cond_move_builtin (MIPS_CMP_MOVF, target,
                                               fcode, arglist);

    default:
      break;
    }

  return 0;
}

/* Init builtin functions */
void
mips_init_builtins (void)
{
  const struct builtin_description *d;
  size_t i;
  tree v2sf_ftype_v2sf_v2sf;
  tree v2sf_ftype_sf_sf;
  tree sf_ftype_v2sf;
  tree v2sf_ftype_v2sf;
  tree int_ftype_v2sf_v2sf;
  tree int_ftype_v2sf_v2sf_v2sf_v2sf;
  tree v2sf_ftype_v2sf_v2sf_int;
  tree int_ftype_sf_sf;
  tree int_ftype_df_df;
  tree sf_ftype_sf;
  tree df_ftype_df;
  tree sf_ftype_sf_sf;
  tree df_ftype_df_df;
  tree v2sf_ftype_v2sf_v2sf_v2sf_v2sf;

  /* For MIPS DSP ASE */
  tree v2hi_ftype_v2hi_v2hi;
  tree v4hi_ftype_v4hi_v4hi;
  tree si_ftype_si_si;
  tree v2si_ftype_v2si_v2si;
  tree v4qi_ftype_v4qi_v4qi;
  tree v8qi_ftype_v8qi_v8qi;
  tree si_ftype_v4qi;
  tree di_ftype_v8qi;
  tree v2hi_ftype_v2hi;
  tree si_ftype_si;
  tree v4hi_ftype_v4hi;
  tree v2si_ftype_v2si;
  tree v4qi_ftype_v2hi_v2hi;
  tree v2hi_ftype_si_si;
  tree v8qi_ftype_v4hi_v4hi;
  tree v4hi_ftype_v2si_v2si;
  tree v2si_ftype_di_di;
  tree si_ftype_v2hi;
  tree v2si_ftype_v4hi;
  tree di_ftype_v2si;
  tree v2hi_ftype_v4qi;
  tree v4hi_ftype_v8qi;
  tree v4qi_ftype_v4qi_si;
  tree v8qi_ftype_v8qi_di;
  tree v2hi_ftype_v2hi_si;
  tree v4hi_ftype_v4hi_di;
  tree v2si_ftype_v2si_di;
  tree v2hi_ftype_v4qi_v2hi;
  tree si_ftype_v2hi_v2hi;
  tree v2si_ftype_v4hi_v4hi;
  tree di_ftype_di_v4qi_v4qi;
  tree ti_ftype_ti_v4qi_v4qi;
  tree di_ftype_di_v2hi_v2hi;
  tree ti_ftype_ti_v2hi_v2hi;
  tree ti_ftype_ti_v4hi_v4hi;
  tree ti_ftype_ti_v2si_v2si;
  tree di_ftype_di_si_si;
  tree ti_ftype_ti_si_si;
  tree ti_ftype_ti_di_di;
  tree ti_ftype_ti_udi_udi;
  tree di_ftype_di_di;
  tree v4qi_ftype_si;
  tree v8qi_ftype_di;
  tree v2hi_ftype_si;
  tree v4hi_ftype_di;
  tree v2si_ftype_di;
  tree void_ftype_v4qi_v4qi;
  tree si_ftype_v4qi_v4qi;
  tree void_ftype_v2hi_v2hi;
  tree void_ftype_v8qi_v8qi;
  tree void_ftype_v4hi_v4hi;
  tree void_ftype_v2si_v2si;
  tree si_ftype_di_si;
  tree si_ftype_ti_si;
  tree di_ftype_ti_di;
  tree di_ftype_di_si;
  tree ti_ftype_ti_si;
  tree ti_ftype_ti_di;
  tree void_ftype_si_si;
  tree si_ftype_ptr_si;
  tree di_ftype_ptr_di;
  tree si_ftype_void;

  tree v4qi_ftype_v4qi;
  tree si_ftype_si_si_si;
  tree di_ftype_si_si;
  tree v2hi_ftype_si_si_si;
  tree di_ftype_di_usi_usi;
  tree di_ftype_usi_usi;

  if (TARGET_PAIRED_SINGLE_FLOAT)
    {
      int_ftype_sf_sf
        = build_function_type_list (integer_type_node,
                                    float_type_node, float_type_node,
                                    NULL_TREE);

      int_ftype_df_df
        = build_function_type_list (integer_type_node,
                                    double_type_node, double_type_node,
                                    NULL_TREE);

      v2sf_ftype_v2sf_v2sf
        = build_function_type_list (V2SF_type_node,
                                    V2SF_type_node, V2SF_type_node, NULL_TREE);

      v2sf_ftype_sf_sf
        = build_function_type_list (V2SF_type_node,
                                    float_type_node, float_type_node,
                                    NULL_TREE);

      sf_ftype_v2sf
        = build_function_type_list (float_type_node, V2SF_type_node, NULL_TREE);

      v2sf_ftype_v2sf
        = build_function_type_list (V2SF_type_node, V2SF_type_node, NULL_TREE);

      int_ftype_v2sf_v2sf
        = build_function_type_list (integer_type_node,
                                    V2SF_type_node, V2SF_type_node, NULL_TREE);

      int_ftype_v2sf_v2sf_v2sf_v2sf
        = build_function_type_list (integer_type_node,
                                    V2SF_type_node, V2SF_type_node,
                                    V2SF_type_node, V2SF_type_node, NULL_TREE);

      v2sf_ftype_v2sf_v2sf_v2sf_v2sf
        = build_function_type_list (V2SF_type_node,
                                    V2SF_type_node, V2SF_type_node,
                                    V2SF_type_node, V2SF_type_node, NULL_TREE);

      v2sf_ftype_v2sf_v2sf_int
        = build_function_type_list (V2SF_type_node,
                                    V2SF_type_node, V2SF_type_node,
                                    integer_type_node, NULL_TREE);

      sf_ftype_sf
        = build_function_type_list (float_type_node,
                                    float_type_node, NULL_TREE);

      df_ftype_df
        = build_function_type_list (double_type_node,
                                    double_type_node, NULL_TREE);

      sf_ftype_sf_sf
        = build_function_type_list (float_type_node,
                                    float_type_node, float_type_node,
                                    NULL_TREE);

      df_ftype_df_df
        = build_function_type_list (double_type_node,
                                    double_type_node, double_type_node,
                                    NULL_TREE);
    }

  if (TARGET_MIPS_DSP32)
    {
      v2hi_ftype_v2hi_v2hi
        = build_function_type_list (V2HI_type_node,
                                    V2HI_type_node, V2HI_type_node,
                                    NULL_TREE);

      si_ftype_si_si
        = build_function_type_list (intSI_type_node,
                                    intSI_type_node, intSI_type_node,
                                    NULL_TREE);

      v4qi_ftype_v4qi_v4qi
        = build_function_type_list (V4QI_type_node,
                                    V4QI_type_node, V4QI_type_node,
                                    NULL_TREE);

      si_ftype_v4qi
        = build_function_type_list (intSI_type_node,
                                    V4QI_type_node,
                                    NULL_TREE);

      v2hi_ftype_v2hi
        = build_function_type_list (V2HI_type_node,
                                    V2HI_type_node,
                                    NULL_TREE);

      si_ftype_si
        = build_function_type_list (intSI_type_node,
                                    intSI_type_node,
                                    NULL_TREE);

      v4qi_ftype_v2hi_v2hi
        = build_function_type_list (V4QI_type_node,
                                    V2HI_type_node, V2HI_type_node,
                                    NULL_TREE);

      v2hi_ftype_si_si
        = build_function_type_list (V2HI_type_node,
                                    intSI_type_node, intSI_type_node,
                                    NULL_TREE);

      si_ftype_v2hi
        = build_function_type_list (intSI_type_node,
                                    V2HI_type_node,
                                    NULL_TREE);

      v2hi_ftype_v4qi
        = build_function_type_list (V2HI_type_node,
                                    V4QI_type_node,
                                    NULL_TREE);

      v4qi_ftype_v4qi_si
        = build_function_type_list (V4QI_type_node,
                                    V4QI_type_node, intSI_type_node,
                                    NULL_TREE);

      v2hi_ftype_v2hi_si
        = build_function_type_list (V2HI_type_node,
                                    V2HI_type_node, intSI_type_node,
                                    NULL_TREE);

      v2hi_ftype_v4qi_v2hi
        = build_function_type_list (V2HI_type_node,
                                    V4QI_type_node, V2HI_type_node,
                                    NULL_TREE);

      si_ftype_v2hi_v2hi
        = build_function_type_list (intSI_type_node,
                                    V2HI_type_node, V2HI_type_node,
                                    NULL_TREE);

      di_ftype_di_v4qi_v4qi
        = build_function_type_list (intDI_type_node,
                                    intDI_type_node, V4QI_type_node, V4QI_type_node,
                                    NULL_TREE);

      di_ftype_di_v2hi_v2hi
        = build_function_type_list (intDI_type_node,
                                    intDI_type_node, V2HI_type_node, V2HI_type_node,
                                    NULL_TREE);

      di_ftype_di_si_si
        = build_function_type_list (intDI_type_node,
                                    intDI_type_node, intSI_type_node, intSI_type_node,
                                    NULL_TREE);

      v4qi_ftype_si
        = build_function_type_list (V4QI_type_node,
                                    intSI_type_node,
                                    NULL_TREE);

      v2hi_ftype_si
        = build_function_type_list (V2HI_type_node,
                                    intSI_type_node,
                                    NULL_TREE);

      void_ftype_v4qi_v4qi
        = build_function_type_list (void_type_node,
                                    V4QI_type_node, V4QI_type_node,
                                    NULL_TREE);

      si_ftype_v4qi_v4qi
        = build_function_type_list (intSI_type_node,
                                    V4QI_type_node, V4QI_type_node,
                                    NULL_TREE);

      void_ftype_v2hi_v2hi
        = build_function_type_list (void_type_node,
                                    V2HI_type_node, V2HI_type_node,
                                    NULL_TREE);

      si_ftype_di_si
        = build_function_type_list (intSI_type_node,
                                    intDI_type_node, intSI_type_node,
                                    NULL_TREE);

      di_ftype_di_si
        = build_function_type_list (intDI_type_node,
                                    intDI_type_node, intSI_type_node,
                                    NULL_TREE);

      void_ftype_si_si
        = build_function_type_list (void_type_node,
                                    intSI_type_node, intSI_type_node,
                                    NULL_TREE);

      si_ftype_ptr_si
        = build_function_type_list (intSI_type_node,
                                    ptr_type_node, intSI_type_node,
                                    NULL_TREE);

      si_ftype_void
        = build_function_type (intSI_type_node, void_list_node);
    }

  if (TARGET_MIPS_DSP32R2)
    {
      v4qi_ftype_v4qi
	= build_function_type_list (V4QI_type_node,
				    V4QI_type_node,
				    NULL_TREE);

      si_ftype_si_si_si
	= build_function_type_list (intSI_type_node,
				    intSI_type_node, intSI_type_node, intSI_type_node,
				    NULL_TREE);

      di_ftype_si_si
	= build_function_type_list (intDI_type_node,
				    intSI_type_node, intSI_type_node,
				    NULL_TREE);

      v2hi_ftype_si_si_si
	= build_function_type_list (V2HI_type_node,
				    intSI_type_node, intSI_type_node, intSI_type_node,
				    NULL_TREE);

      di_ftype_di_usi_usi
	= build_function_type_list (intDI_type_node,
				    intDI_type_node, unsigned_intSI_type_node, unsigned_intSI_type_node,
				    NULL_TREE);

      di_ftype_usi_usi
	= build_function_type_list (intDI_type_node,
				    unsigned_intSI_type_node, unsigned_intSI_type_node,
				    NULL_TREE);
    }

  if (TARGET_MIPS_DSP64)
    {
      v4hi_ftype_v4hi_v4hi
        = build_function_type_list (V4HI_type_node,
                                    V4HI_type_node, V4HI_type_node,
                                    NULL_TREE);

      v2si_ftype_v2si_v2si
        = build_function_type_list (V2SI_type_node,
                                    V2SI_type_node, V2SI_type_node,
                                    NULL_TREE);

      v8qi_ftype_v8qi_v8qi
        = build_function_type_list (V8QI_type_node,
                                    V8QI_type_node, V8QI_type_node,
                                    NULL_TREE);

      di_ftype_v8qi
        = build_function_type_list (intDI_type_node,
                                    V8QI_type_node,
                                    NULL_TREE);

      v4hi_ftype_v4hi
        = build_function_type_list (V4HI_type_node,
                                    V4HI_type_node,
                                    NULL_TREE);

      v2si_ftype_v2si
        = build_function_type_list (V2SI_type_node,
                                    V2SI_type_node,
                                    NULL_TREE);

      v8qi_ftype_v4hi_v4hi
        = build_function_type_list (V8QI_type_node,
                                    V4HI_type_node, V4HI_type_node,
                                    NULL_TREE);

      v4hi_ftype_v2si_v2si
        = build_function_type_list (V4HI_type_node,
                                    V2SI_type_node, V2SI_type_node,
                                    NULL_TREE);

      v2si_ftype_di_di
        = build_function_type_list (V2SI_type_node,
                                    intDI_type_node, intDI_type_node,
                                    NULL_TREE);

      v2si_ftype_v4hi
        = build_function_type_list (V2SI_type_node,
                                    V4HI_type_node,
                                    NULL_TREE);

      di_ftype_v2si
        = build_function_type_list (intDI_type_node,
                                    V2SI_type_node,
                                    NULL_TREE);

      v4hi_ftype_v8qi
        = build_function_type_list (V4HI_type_node,
                                    V8QI_type_node,
                                    NULL_TREE);

      v8qi_ftype_v8qi_di
        = build_function_type_list (V8QI_type_node,
                                    V8QI_type_node, intDI_type_node,
                                    NULL_TREE);

      v4hi_ftype_v4hi_di
        = build_function_type_list (V4HI_type_node,
                                    V4HI_type_node, intDI_type_node,
                                    NULL_TREE);

      v2si_ftype_v2si_di
        = build_function_type_list (V2SI_type_node,
                                    V2SI_type_node, intDI_type_node,
                                    NULL_TREE);

      v2si_ftype_v4hi_v4hi
        = build_function_type_list (V2SI_type_node,
                                    V4HI_type_node, V4HI_type_node,
                                    NULL_TREE);

      ti_ftype_ti_v4qi_v4qi
        = build_function_type_list (intTI_type_node,
                                    intTI_type_node, V4QI_type_node, V4QI_type_node,
                                    NULL_TREE);

      ti_ftype_ti_v2hi_v2hi
        = build_function_type_list (intTI_type_node,
                                    intTI_type_node, V2HI_type_node, V2HI_type_node,
                                    NULL_TREE);

      ti_ftype_ti_v4hi_v4hi
        = build_function_type_list (intTI_type_node,
                                    intTI_type_node, V4HI_type_node, V4HI_type_node,
                                    NULL_TREE);

      ti_ftype_ti_v2si_v2si
        = build_function_type_list (intTI_type_node,
                                    intTI_type_node, V2SI_type_node, V2SI_type_node,
                                    NULL_TREE);

      ti_ftype_ti_si_si
        = build_function_type_list (intTI_type_node,
                                    intTI_type_node, intSI_type_node, intSI_type_node,
                                    NULL_TREE);

      ti_ftype_ti_di_di
        = build_function_type_list (intTI_type_node,
                                    intTI_type_node, intDI_type_node, intDI_type_node,
                                    NULL_TREE);

      ti_ftype_ti_udi_udi
	= build_function_type_list (intTI_type_node,
				    intTI_type_node, unsigned_intDI_type_node, unsigned_intDI_type_node,
				    NULL_TREE);

      di_ftype_di_di
        = build_function_type_list (intDI_type_node,
                                    intDI_type_node, intDI_type_node,
                                    NULL_TREE);

      v8qi_ftype_di
        = build_function_type_list (V8QI_type_node,
                                    intDI_type_node,
                                    NULL_TREE);

      v4hi_ftype_di
        = build_function_type_list (V4HI_type_node,
                                    intDI_type_node,
                                    NULL_TREE);

      v2si_ftype_di
        = build_function_type_list (V2SI_type_node,
                                    intDI_type_node,
                                    NULL_TREE);

      void_ftype_v8qi_v8qi
        = build_function_type_list (void_type_node,
                                    V8QI_type_node, V8QI_type_node,
                                    NULL_TREE);

      void_ftype_v4hi_v4hi
        = build_function_type_list (void_type_node,
                                    V4HI_type_node, V4HI_type_node,
                                    NULL_TREE);

      void_ftype_v2si_v2si
        = build_function_type_list (void_type_node,
                                    V2SI_type_node, V2SI_type_node,
                                    NULL_TREE);

      si_ftype_ti_si
        = build_function_type_list (intSI_type_node,
                                    intTI_type_node, intSI_type_node,
                                    NULL_TREE);

      di_ftype_ti_di
        = build_function_type_list (intDI_type_node,
                                    intTI_type_node, intDI_type_node,
                                    NULL_TREE);

      ti_ftype_ti_si
        = build_function_type_list (intTI_type_node,
                                    intTI_type_node, intSI_type_node,
                                    NULL_TREE);

      ti_ftype_ti_di
        = build_function_type_list (intTI_type_node,
                                    intTI_type_node, intDI_type_node,
                                    NULL_TREE);

      di_ftype_ptr_di
        = build_function_type_list (intDI_type_node,
                                    ptr_type_node, intDI_type_node,
                                    NULL_TREE);

    }

  for (i = 0, d = mips_bdesc; i < ARRAY_SIZE (mips_bdesc) ; i++, d++)
    {
      if ((d->target_flags & MASK_PAIRED_SINGLE)
          && ! TARGET_PAIRED_SINGLE_FLOAT)
        continue;

      if ((d->target_flags & MASK_MIPS3D)
          && ! TARGET_MIPS3D)
        continue;

      if ((d->target_flags & MASK_DSP)
          && ! TARGET_MIPS_DSP32)
        continue;

      if ((d->target_flags & MASK_DSPR2)
	  && ! TARGET_MIPS_DSP32R2)
        continue;

      if ((d->target_flags & MASK_DSP) && (d->target_flags & MASK_64BIT)
          && ! TARGET_MIPS_DSP64)
        continue;

      if ((d->target_flags & MASK_DSPR2) && (d->target_flags & MASK_64BIT)
	  && ! TARGET_MIPS_DSP64R2)
        continue;

    switch (d->ftype)
      {
      case MIPS_V2SF_FTYPE_V2SF_V2SF:
        builtin_function (d->name, v2sf_ftype_v2sf_v2sf,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SF_FTYPE_SF_SF:
        builtin_function (d->name, v2sf_ftype_sf_sf,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SF_FTYPE_V2SF:
        builtin_function (d->name, sf_ftype_v2sf,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SF_FTYPE_V2SF:
        builtin_function (d->name, v2sf_ftype_v2sf,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_INT_FTYPE_V2SF_V2SF:
        builtin_function (d->name, int_ftype_v2sf_v2sf,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_INT_FTYPE_V2SF_V2SF_V2SF_V2SF:
        builtin_function (d->name, int_ftype_v2sf_v2sf_v2sf_v2sf,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SF_FTYPE_V2SF_V2SF_INT:
        builtin_function (d->name, v2sf_ftype_v2sf_v2sf_int,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SF_FTYPE_V2SF_V2SF_V2SF_V2SF:
        builtin_function (d->name, v2sf_ftype_v2sf_v2sf_v2sf_v2sf,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SF_FTYPE_SF:
        builtin_function (d->name, sf_ftype_sf,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_DF_FTYPE_DF:
        builtin_function (d->name, df_ftype_df,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_INT_FTYPE_SF_SF:
        builtin_function (d->name, int_ftype_sf_sf,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_INT_FTYPE_DF_DF:
        builtin_function (d->name, int_ftype_df_df,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SF_FTYPE_SF_SF:
        builtin_function (d->name, sf_ftype_sf_sf,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_DF_FTYPE_DF_DF:
        builtin_function (d->name, df_ftype_df_df,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      /* For MIPS DSP ASE */
      case MIPS_V2HI_FTYPE_V2HI_V2HI:
        builtin_function (d->name, v2hi_ftype_v2hi_v2hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V4HI_FTYPE_V4HI_V4HI:
        builtin_function (d->name, v4hi_ftype_v4hi_v4hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SI_FTYPE_SI_SI:
        builtin_function (d->name, si_ftype_si_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SI_FTYPE_V2SI_V2SI:
        builtin_function (d->name, v2si_ftype_v2si_v2si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V4QI_FTYPE_V4QI_V4QI:
        builtin_function (d->name, v4qi_ftype_v4qi_v4qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V8QI_FTYPE_V8QI_V8QI:
        builtin_function (d->name, v8qi_ftype_v8qi_v8qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SI_FTYPE_V4QI:
        builtin_function (d->name, si_ftype_v4qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_DI_FTYPE_V8QI:
        builtin_function (d->name, di_ftype_v8qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2HI_FTYPE_V2HI:
        builtin_function (d->name, v2hi_ftype_v2hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SI_FTYPE_SI:
        builtin_function (d->name, si_ftype_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V4HI_FTYPE_V4HI:
        builtin_function (d->name, v4hi_ftype_v4hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SI_FTYPE_V2SI:
        builtin_function (d->name, v2si_ftype_v2si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V4QI_FTYPE_V2HI_V2HI:
        builtin_function (d->name, v4qi_ftype_v2hi_v2hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2HI_FTYPE_SI_SI:
        builtin_function (d->name, v2hi_ftype_si_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V8QI_FTYPE_V4HI_V4HI:
        builtin_function (d->name, v8qi_ftype_v4hi_v4hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V4HI_FTYPE_V2SI_V2SI:
        builtin_function (d->name, v4hi_ftype_v2si_v2si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SI_FTYPE_DI_DI:
        builtin_function (d->name, v2si_ftype_di_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SI_FTYPE_V2HI:
        builtin_function (d->name, si_ftype_v2hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SI_FTYPE_V4HI:
        builtin_function (d->name, v2si_ftype_v4hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_DI_FTYPE_V2SI:
        builtin_function (d->name, di_ftype_v2si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2HI_FTYPE_V4QI:
        builtin_function (d->name, v2hi_ftype_v4qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V4HI_FTYPE_V8QI:
        builtin_function (d->name, v4hi_ftype_v8qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V4QI_FTYPE_V4QI_SI:
        builtin_function (d->name, v4qi_ftype_v4qi_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V8QI_FTYPE_V8QI_DI:
        builtin_function (d->name, v8qi_ftype_v8qi_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2HI_FTYPE_V2HI_SI:
        builtin_function (d->name, v2hi_ftype_v2hi_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V4HI_FTYPE_V4HI_DI:
        builtin_function (d->name, v4hi_ftype_v4hi_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SI_FTYPE_V2SI_DI:
        builtin_function (d->name, v2si_ftype_v2si_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2HI_FTYPE_V4QI_V2HI:
        builtin_function (d->name, v2hi_ftype_v4qi_v2hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SI_FTYPE_V2HI_V2HI:
        builtin_function (d->name, si_ftype_v2hi_v2hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SI_FTYPE_V4HI_V4HI:
        builtin_function (d->name, v2si_ftype_v4hi_v4hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_DI_FTYPE_DI_V4QI_V4QI:
        builtin_function (d->name, di_ftype_di_v4qi_v4qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_TI_FTYPE_TI_V4QI_V4QI:
        builtin_function (d->name, ti_ftype_ti_v4qi_v4qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_DI_FTYPE_DI_V2HI_V2HI:
        builtin_function (d->name, di_ftype_di_v2hi_v2hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_TI_FTYPE_TI_V2HI_V2HI:
        builtin_function (d->name, ti_ftype_ti_v2hi_v2hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_TI_FTYPE_TI_V4HI_V4HI:
        builtin_function (d->name, ti_ftype_ti_v4hi_v4hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_TI_FTYPE_TI_V2SI_V2SI:
        builtin_function (d->name, ti_ftype_ti_v2si_v2si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_DI_FTYPE_DI_SI_SI:
        builtin_function (d->name, di_ftype_di_si_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_TI_FTYPE_TI_SI_SI:
        builtin_function (d->name, ti_ftype_ti_si_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_TI_FTYPE_TI_DI_DI:
        builtin_function (d->name, ti_ftype_ti_di_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_TI_FTYPE_TI_UDI_UDI:
	builtin_function (d->name, ti_ftype_ti_udi_udi,
			  d->code, BUILT_IN_MD, NULL, NULL_TREE);
	break;

      case MIPS_DI_FTYPE_DI_DI:
        builtin_function (d->name, di_ftype_di_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V4QI_FTYPE_SI:
        builtin_function (d->name, v4qi_ftype_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V8QI_FTYPE_DI:
        builtin_function (d->name, v8qi_ftype_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2HI_FTYPE_SI:
        builtin_function (d->name, v2hi_ftype_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V4HI_FTYPE_DI:
        builtin_function (d->name, v4hi_ftype_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_V2SI_FTYPE_DI:
        builtin_function (d->name, v2si_ftype_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_VOID_FTYPE_V4QI_V4QI:
        builtin_function (d->name, void_ftype_v4qi_v4qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SI_FTYPE_V4QI_V4QI:
        builtin_function (d->name, si_ftype_v4qi_v4qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_VOID_FTYPE_V2HI_V2HI:
        builtin_function (d->name, void_ftype_v2hi_v2hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_VOID_FTYPE_V8QI_V8QI:
        builtin_function (d->name, void_ftype_v8qi_v8qi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_VOID_FTYPE_V4HI_V4HI:
        builtin_function (d->name, void_ftype_v4hi_v4hi,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_VOID_FTYPE_V2SI_V2SI:
        builtin_function (d->name, void_ftype_v2si_v2si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SI_FTYPE_DI_SI:
        builtin_function (d->name, si_ftype_di_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SI_FTYPE_TI_SI:
        builtin_function (d->name, si_ftype_ti_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_DI_FTYPE_TI_DI:
        builtin_function (d->name, di_ftype_ti_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_DI_FTYPE_DI_SI:
        builtin_function (d->name, di_ftype_di_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_TI_FTYPE_TI_SI:
        builtin_function (d->name, ti_ftype_ti_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_TI_FTYPE_TI_DI:
        builtin_function (d->name, ti_ftype_ti_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_VOID_FTYPE_SI_SI:
        builtin_function (d->name, void_ftype_si_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SI_FTYPE_PTR_SI:
        builtin_function (d->name, si_ftype_ptr_si,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_DI_FTYPE_PTR_DI:
        builtin_function (d->name, di_ftype_ptr_di,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;

      case MIPS_SI_FTYPE_VOID:
        builtin_function (d->name, si_ftype_void,
                          d->code, BUILT_IN_MD, NULL, NULL_TREE);
        break;


      case MIPS_V4QI_FTYPE_V4QI:
	builtin_function (d->name, v4qi_ftype_v4qi,
			  d->code, BUILT_IN_MD, NULL, NULL_TREE);
	break;

      case MIPS_SI_FTYPE_SI_SI_SI:
	builtin_function (d->name, si_ftype_si_si_si,
			  d->code, BUILT_IN_MD, NULL, NULL_TREE);
	break;

      case MIPS_DI_FTYPE_SI_SI:
	builtin_function (d->name, di_ftype_si_si,
			  d->code, BUILT_IN_MD, NULL, NULL_TREE);
	break;

      case MIPS_V2HI_FTYPE_SI_SI_SI:
	builtin_function (d->name, v2hi_ftype_si_si_si,
			  d->code, BUILT_IN_MD, NULL, NULL_TREE);
	break;

      case MIPS_DI_FTYPE_DI_USI_USI:
	builtin_function (d->name, di_ftype_di_usi_usi,
			  d->code, BUILT_IN_MD, NULL, NULL_TREE);
	break;

      case MIPS_DI_FTYPE_USI_USI:
	builtin_function (d->name, di_ftype_usi_usi,
			  d->code, BUILT_IN_MD, NULL, NULL_TREE);
	break;

      default:
        break;
      }
    }
}

const char *
mips16e_output_save (rtx *operands)
{
  int i;
  int regno;
  rtx insn;
  static char buffer[300];
  char *s = buffer;
  unsigned int size = INTVAL (operands[0]);
  static const char reglist_save[] = {30, 23, 22, 21, 20, 19, 18, 0};
  static const char reglist_asave[] = {4, 5, 6, 7, 0};

  HOST_WIDE_INT tsize = cfun->machine->frame.total_size;
  int savearg = 0;

  /* Look through the initial insns to see if any of them store
     the function parameters into the incoming parameter storage
     area.  If they do, we delete the insn, and save the register
     using the entry pseudo-instruction instead.  We don't try to
     look past a label, jump, or call.  */
  for (insn = get_insns (); insn != NULL_RTX; insn = NEXT_INSN (insn))
    {
      rtx note, set, src, dest, base, offset;
      int hireg, nr;

      if (GET_CODE (insn) == CODE_LABEL
          || GET_CODE (insn) == JUMP_INSN
          || GET_CODE (insn) == CALL_INSN)
        break;

      set = single_set (insn);
      if (!set)
        continue;

      /* An insn storing a function parameter will still have a
         REG_EQUIV note on it mentioning the argument pointer.  */
      note = find_reg_note (insn, REG_EQUIV, NULL_RTX);
      if (note == NULL_RTX)
        continue;
      if (! reg_mentioned_p (arg_pointer_rtx, XEXP (note, 0)))
        continue;

      src = SET_SRC (set);
      if (GET_CODE (src) != REG)
        continue;

      regno = REGNO (src);
      if (regno < GP_ARG_FIRST || regno > GP_ARG_LAST)
        continue;

      dest = SET_DEST (set);
      if (GET_CODE (dest) != MEM)
        continue;
      if (GET_MODE_SIZE (GET_MODE (dest)) == UNITS_PER_WORD)
        ;
      else if (GET_MODE_SIZE (GET_MODE (dest)) == 2 * UNITS_PER_WORD
               && regno < GP_ARG_LAST)
        ;
      else
        continue;

      /* Cannot save a register twice. */
      for (nr = HARD_REGNO_NREGS (regno, GET_MODE (dest)); nr != 0; nr--)
        if (BITSET_P (cfun->machine->frame.mask, regno + nr - 1))
          break;
      if (nr != 0)
        continue;

      offset = const0_rtx;
      base = eliminate_constant_term (XEXP (dest, 0), &offset);
      if (GET_CODE (base) != REG
          || GET_CODE (offset) != CONST_INT)
        continue;
      if (REGNO (base) == STACK_POINTER_REGNUM
          && INTVAL (offset) == tsize + (regno - GP_ARG_FIRST) * UNITS_PER_WORD)
        ;
      else if (REGNO (base) == HARD_FRAME_POINTER_REGNUM
               && frame_pointer_needed
               && (INTVAL (offset)
                   == (tsize
                       + (regno - GP_ARG_FIRST) * UNITS_PER_WORD
                       - current_function_outgoing_args_size)))
        ;
      else
        continue;

      /* This insn stores a parameter onto the stack, in the same
         location where the entry pseudo-instruction will put it.
         Delete the insn, and arrange to tell the entry
         instruction to save the register.  */
      PUT_CODE (insn, NOTE);
      NOTE_LINE_NUMBER (insn) = NOTE_INSN_DELETED;
      NOTE_SOURCE_FILE (insn) = 0;

      hireg = (regno
               + HARD_REGNO_NREGS (regno, GET_MODE (dest))
               - 1);
      if (hireg > savearg)
        savearg = hireg;
    }

  /* If this is a varargs function, we need to save all the
     registers onto the stack anyhow.  */
  /* XXX not strictly true, but what the hell */
  if (current_function_stdarg)
    savearg = GP_ARG_LAST;

  /* Save */
  sprintf (s, "save\t");
  s += strlen(s);

  /* Save $4 - $7 */
  if (savearg > 0)
    {
      if (savearg == GP_ARG_FIRST)
        sprintf (s, "%s,", reg_names[savearg]);
      else
        sprintf (s, "%s-%s,", reg_names[GP_ARG_FIRST], reg_names[savearg]);
      s += strlen (s);
    }

  /* Size */
  sprintf (s, "%d", size);
  s += strlen (s);

  /* Save $16 */
  if (BITSET_P (cfun->machine->frame.mask, 16))
    {
      sprintf (s, ",%s", reg_names[GP_REG_FIRST + 16]);
      s += strlen(s);
    }

  /* Save $17 */
  if (BITSET_P (cfun->machine->frame.mask, 17))
    {
      sprintf (s, ",%s", reg_names[GP_REG_FIRST + 17]);
      s += strlen(s);
    }

  /* Save $s2-$s8 */
  for (i = 0; (regno = reglist_save[i]) != 0; i++)
    if (BITSET_P (cfun->machine->frame.mask, regno))
      {
        if (regno > 18)
          sprintf (s, ",%s-%s", reg_names[GP_REG_FIRST + 18],
                                reg_names[GP_REG_FIRST + regno]);
        else
          sprintf (s, ",%s", reg_names[GP_REG_FIRST + regno]);

        s += strlen(s);
        break;
      }

  /* Save $a0-$a3 */
  for (i = 0; (regno = reglist_asave[i]) != 0; i++)
    if (BITSET_P (cfun->machine->frame.mask, regno))
      {
        if (regno < 7)
          sprintf (s, ",%s-%s", reg_names[GP_REG_FIRST + regno],
                                reg_names[GP_REG_FIRST + 7]);
        else
          sprintf (s, ",%s", reg_names[GP_REG_FIRST + 7]);
        s += strlen(s);
        break;
      }

  /* Save $31 */
  if (BITSET_P (cfun->machine->frame.mask, 31))
    {
      sprintf (s, ",%s", reg_names[GP_REG_FIRST + 31]);
      s += strlen(s);
    }

  return buffer;
}

const char *
mips16e_output_restore (rtx *operands)
{
  int i;
  int regno;
  static char buffer[300];
  char *s = buffer;
  unsigned int size = INTVAL (operands[0]);
  static const char reglist_save[] = {30, 23, 22, 21, 20, 19, 18, 0};
  static const char reglist_asave[] = {4, 5, 6, 7, 0};

  /* Size */
  sprintf (s, "restore\t%d", size);
  s += strlen(s);

  /* Restore $16 */
  if (BITSET_P (cfun->machine->frame.mask, 16))
    {
      sprintf (s, ",%s", reg_names[GP_REG_FIRST + 16]);
      s += strlen(s);
    }

  /* Restore $17 */
  if (BITSET_P (cfun->machine->frame.mask, 17))
    {
      sprintf (s, ",%s", reg_names[GP_REG_FIRST + 17]);
      s += strlen(s);
    }

  /* Restore $s2-$s8 */
  for (i = 0; (regno = reglist_save[i]) != 0; i++)
    if (BITSET_P (cfun->machine->frame.mask, regno))
      {
        if (regno > 18)
          sprintf (s, ",%s-%s", reg_names[GP_REG_FIRST + 18],
                                reg_names[GP_REG_FIRST + regno]);
        else
          sprintf (s, ",%s", reg_names[GP_REG_FIRST + regno]);

        s += strlen(s);
        break;
      }

  /* Restore $a0-$a3 */
  for (i = 0; (regno = reglist_asave[i]) != 0; i++)
    if (BITSET_P (cfun->machine->frame.mask, regno))
      {
        if (regno < 7)
          sprintf (s, ",%s-%s", reg_names[GP_REG_FIRST + regno],
                                reg_names[GP_REG_FIRST + 7]);
        else
          sprintf (s, ",%s", reg_names[GP_REG_FIRST + 7]);
        s += strlen(s);
        break;
      }

  /* Restore $31 */
  if (BITSET_P (cfun->machine->frame.mask, 31))
    {
      sprintf (s, ",%s", reg_names[GP_REG_FIRST + 31]);
      s += strlen(s);
    }

  return buffer;
}

static void
mips_output_section_bases ()
{
  if (cgraph_varpool_data_list)
    {
      fprintf (asm_out_file, "\t.data\n");
      fprintf (asm_out_file, "\t.align\t%d\n",
               floor_log2 (cgraph_varpool_data_align / BITS_PER_UNIT));
      fprintf (asm_out_file, "%sLsr_base_data:\n", LOCAL_LABEL_PREFIX);

      /* Output the values and spaces for the data section */
      cgraph_varpool_output_statics (cgraph_varpool_data_list);
    }

  if (cgraph_varpool_udata_list)
    {
      fprintf (asm_out_file, "\t.section\t.bss\n");
      fprintf (asm_out_file, "\t.align\t%d\n",
               floor_log2 (cgraph_varpool_udata_align / BITS_PER_UNIT));
      fprintf (asm_out_file, "%sLsr_base_bss:\n", LOCAL_LABEL_PREFIX);

      /* Output the values and spaces for the bss section */
      cgraph_varpool_output_statics (cgraph_varpool_udata_list);
    }
}

static rtx
mips_base_bss_rtx (void)
{
  rtx base = cfun->machine->sectrel_base_bss_rtx;
  if (!base)
    {
      char name[32];
      tree id;
      sprintf (name, "%sLsr_base_bss", LOCAL_LABEL_PREFIX);
      id = get_identifier (name);
      base = gen_rtx_SYMBOL_REF (Pmode, IDENTIFIER_POINTER (id));
      SYMBOL_REF_FLAGS (base) |= SYMBOL_FLAG_SECTION_RELATIVE;
      cfun->machine->sectrel_base_bss_rtx = base;
    }
  return base;
}


static rtx
mips_base_data_rtx (void)
{
  rtx base = cfun->machine->sectrel_base_data_rtx;
  if (!base)
    {
      char name[32];
      tree id;
      sprintf (name, "%sLsr_base_data", LOCAL_LABEL_PREFIX);
      id = get_identifier (name);
      base = gen_rtx_SYMBOL_REF (Pmode, IDENTIFIER_POINTER (id));
      SYMBOL_REF_FLAGS (base) |= SYMBOL_FLAG_SECTION_RELATIVE;
      cfun->machine->sectrel_base_data_rtx = base;
    }
  return base;
}

static rtx
mips_legitimize_section_relative_address (rtx temp, rtx addr, tree decl)
{
  struct cgraph_varpool_node *node;

  node = cgraph_varpool_node_for_identifier (DECL_ASSEMBLER_NAME (decl));
  if (node && node->sectrel_node && node->needed)
    {
      rtx sectrel_address;

      if (temp == 0 && no_new_pseudos)
        abort();  /* Shouldn't be called. */

      if (!no_new_pseudos)
        temp = gen_reg_rtx (Pmode);

      if (node->bss_off != -1)
        {
          /* This goes to .bss section */
          emit_move_insn (temp, mips_base_bss_rtx ());
          sectrel_address = mips_add_offset (0, temp, node->bss_off);
        }
      else if (node->data_off != -1)
        {
          /* This goes to .data section */
          emit_move_insn (temp, mips_base_data_rtx ());
          sectrel_address = mips_add_offset (0, temp, node->data_off);
        }
      else
        abort ();
      return sectrel_address;
    }

  return NULL_RTX;
}

/* Implement TARGET_CANNOT_FORCE_CONST_MEM.  Return true if x is of a form that
   cannot (or should not) be spilled to the constant pool.  */

static bool
mips_cannot_force_const_mem (rtx x)
{
  return (TARGET_MIPS16 && TARGET_NO_DATA_IN_CODE && !TARGET_GP_OPT
          && GET_CODE (x) == HIGH);
}

/* DSP ALU can bypass data with 0 delays for the following pairs. */

enum insn_code dspalu_bypass_table[][2] =
{
  {CODE_FOR_mips_addsc, CODE_FOR_mips_addwc},
  {CODE_FOR_mips_cmpu_eq_qb, CODE_FOR_mips_pick_qb},
  {CODE_FOR_mips_cmpu_lt_qb, CODE_FOR_mips_pick_qb},
  {CODE_FOR_mips_cmpu_le_qb, CODE_FOR_mips_pick_qb},
  {CODE_FOR_mips_cmp_eq_ph, CODE_FOR_mips_pick_ph},
  {CODE_FOR_mips_cmp_lt_ph, CODE_FOR_mips_pick_ph},
  {CODE_FOR_mips_cmp_le_ph, CODE_FOR_mips_pick_ph},
  {CODE_FOR_mips_cmpu_eq_ob, CODE_FOR_mips_pick_ob},
  {CODE_FOR_mips_cmpu_lt_ob, CODE_FOR_mips_pick_ob},
  {CODE_FOR_mips_cmpu_le_ob, CODE_FOR_mips_pick_ob},
  {CODE_FOR_mips_cmp_eq_qh, CODE_FOR_mips_pick_qh},
  {CODE_FOR_mips_cmp_lt_qh, CODE_FOR_mips_pick_qh},
  {CODE_FOR_mips_cmp_le_qh, CODE_FOR_mips_pick_qh},
  {CODE_FOR_mips_cmp_eq_pw, CODE_FOR_mips_pick_pw},
  {CODE_FOR_mips_cmp_lt_pw, CODE_FOR_mips_pick_pw},
  {CODE_FOR_mips_cmp_le_pw, CODE_FOR_mips_pick_pw},
  {CODE_FOR_mips_wrdsp, CODE_FOR_mips_insv},
  {CODE_FOR_mips_wrdsp, CODE_FOR_mips_dinsv}
};

bool
dspalu_bypass_p (rtx out_insn, rtx in_insn)
{
  int i;
  int num_bypass = sizeof (dspalu_bypass_table)/(sizeof (enum insn_code) << 1);
  enum insn_code out_icode = INSN_CODE (out_insn);
  enum insn_code in_icode = INSN_CODE (in_insn);

  for (i = 0; i < num_bypass; i++)
    {
      if (out_icode == dspalu_bypass_table[i][0] &&
          in_icode == dspalu_bypass_table[i][1])
        return true;
    }

  return false;
}

/* For automatic variables that have been identified as suitable for
   allocation in a pseudo register, we make additional checks such
   that if it's a structure type, and all of its fields are smaller
   than 32-bits and correctly aligned, then its better to push it onto
   the stack instead of synthesising bit insert or extract operations
   in a register. 
   SPILL holds the state of fields alreday looked into so far.  
   Normally false is passes from the top level.  */

static bool
mips_spill_auto_register (tree decl, bool spill)
{
  tree type = TREE_TYPE (decl);
  tree field;

  /* Don't spill for 64-bit targets - seems to be broken. */
  if (TARGET_64BIT)
    return false;

  /* Don't spill if ISA is MIPS32R2 or MIPS64R2, since they have
     real register ins/ext instructions. */
  if (!TARGET_MIPS16
      && (ISA_MIPS32R2 || ISA_MIPS64R2))
    return false;

  switch (TREE_CODE (type))
    {
    case UNION_TYPE:
    case RECORD_TYPE:
      /* Must be a struct/union to be a candidate for spilling. */
      spill = false;
      for (field = TYPE_FIELDS (type); field != 0; field = TREE_CHAIN (field))
	{
	  if (TREE_CODE (field) == FIELD_DECL)
	    {
	      if (TREE_CODE (TREE_TYPE (field)) == UNION_TYPE
		  || TREE_CODE (TREE_TYPE (field)) == RECORD_TYPE
		  || TREE_CODE (TREE_TYPE (field)) == ARRAY_TYPE)
		{
		  /* Sub records or array base must conform.  */
		  if (mips_spill_auto_register (field, spill))
		    spill |= true;
		  else
		    return false;
		}
	      else if (DECL_SIZE (field) 
		       && host_integerp (DECL_SIZE (field), 1))
		{
		  unsigned int bits = tree_low_cst (DECL_SIZE (field), 1);
		  if (bits == 8
		      || bits == 16
		      || (bits == 32 && TARGET_64BIT))
		    /* Only worth spilling if there are correctly
		       aligned partial words. */
		    spill |= true;
		  else if (bits == 32 || bits == 64)
		    /* Allow word and dword fields, but they
		       aren't enough to force spilling. */
		    spill &= true;
		  else
		    return false;
		  /* Cannot cope with fields whose positions are
		     variable. */
		  if (! host_integerp (bit_position (field), 0))
		    return false;
		  if (int_bit_position (field) % bits != 0)
		    /* Don't spill if the fields are not aligned. */
		    return false;
		}
	    }
	  else
	    return false;
	}
      return spill;

    case ARRAY_TYPE:
      if (TREE_CODE (TREE_TYPE (type)) == UNION_TYPE
	  || TREE_CODE (TREE_TYPE (type)) == RECORD_TYPE
  	  || TREE_CODE (TREE_TYPE (type)) == ARRAY_TYPE)
	return mips_spill_auto_register (type, spill);

      if (TREE_CODE (TREE_TYPE (type)) == INTEGER_TYPE)
	{
	  /* Any size that's modulo of 8 is good.  */
	  switch (tree_low_cst (TYPE_SIZE (TREE_TYPE (type)), 1))
	    {
	    case 8:
	    case 16:
	      spill |= true;
	      break;
	    case 32:
	    case 64:
	      /* Keep spill unchanged.  */
	      break;    
	    default:
	      return false;
	    }
	}
      return spill;

    default:
      return false;
    }
}

int
mips_addr_vec_align (rtx addr_vec)
{
  int align = GET_MODE_SIZE (GET_MODE (PATTERN (addr_vec)));

  if (TARGET_MIPS16 && JUMP_TABLES_IN_TEXT_SECTION && align < 4)
    /* We'll be using an LA / ADDIUPC instruction to take the address
       of an addr diff vector, then it must be 4 byte aligned to avoid
       using an extended instruction. */
    align = 4;

  if (align > BIGGEST_ALIGNMENT / BITS_PER_UNIT)
    align = BIGGEST_ALIGNMENT / BITS_PER_UNIT;
  return exact_log2 (align);

}

/* Check if the interrupt attribute is set for a function. If it is, return
   the IPL identifier, else NULL */

static tree
mchp_function_interrupt_p (tree decl)
{
  tree attrlist = TYPE_ATTRIBUTES(TREE_TYPE(decl));
  tree attr;
  attr = lookup_attribute ("interrupt", attrlist);
  if (attr)
    attr = TREE_VALUE (attr);
  return attr;
}

/* parse the vector pragma */
void
mchp_handle_vector_pragma (struct cpp_reader *pfile ATTRIBUTE_UNUSED)
{
  int tok;
  tree tok_value;
  const char *fname;

  /* syntax:
     vector-pragma: # pragma vector target-name irq-num [ , irq-num ]...
   */

  /* Recognize the syntax. */
  /* First we have the name of the function which is an interrupt handler */
  tok = c_lex (&tok_value);
  if (tok != CPP_NAME)
    {
      error ("function name not found in vector #pragma");
      return;
    }
  fname = IDENTIFIER_POINTER (tok_value);
  assert (fname);

  /* The first address is mandatory since we have a vector clause */
  tok = c_lex (&tok_value);
  if (tok != CPP_NUMBER ||
      ((int)TREE_INT_CST_LOW (tok_value) < 0 ||
       (int)TREE_INT_CST_LOW (tok_value) > 63))
    {
      error ("IRQ number must be an integer between 0 and 63");
      return;
    }
  /* add the vector to the list of dispatch functions to emit */
  mchp_add_vector_dispatch_entry (fname, (int)TREE_INT_CST_LOW (tok_value));

  /* There are optionally more addresses, comma separated */
  tok = c_lex (&tok_value);
  while (tok == CPP_COMMA)
    {
      tok = c_lex (&tok_value);
      if (tok != CPP_NUMBER)
        {
          error ("address constant expected for vector function specifier");
          return;
        }
      /* add the vector to the list of dispatch functions to emit */
      mchp_add_vector_dispatch_entry (fname,
          (int)TREE_INT_CST_LOW (tok_value));

      tok = c_lex (&tok_value);
    }

  /* No further input is valid. We should have end of line here. */
  if (tok != CPP_EOF)
    {
      error ("extraneous data at end of line of #pragma vector");
    }
}

/* parse the interrupt pragma */
void
mchp_handle_interrupt_pragma (struct cpp_reader *pfile ATTRIBUTE_UNUSED)
{
  struct interrupt_pragma_spec *p;
  const cpp_token *raw_tok;
  int tok;
  tree tok_value;
  const char *fname;
  tree ipl;

  /* Make sure our pragma is specified at file scope */
  if (!global_bindings_p ())
    {
      error ("interrupt pragma must have file scope");
      return;
    }
  /* syntax:
     interrupt-pragma: # pragma interrupt function-name ipl-specifier [ vector [[@]irq-num [ , irq-num ]... ] ]

     ipl-specifier: ipl0 ... ipl7 |
                    single
     */

  /* Recognize the syntax. */
  /* First we have the name of the function which is an interrupt handler */
  tok = c_lex (&tok_value);
  if (tok != CPP_NAME)
    {
      error ("function name not found in interrupt #pragma");
      return;
    }
  fname = IDENTIFIER_POINTER (tok_value);
  assert (fname);
  /* Second we have the IPL */
  tok = c_lex (&tok_value);
  if (tok != CPP_NAME
      || (strcmp ("single", IDENTIFIER_POINTER (tok_value)) != 0
        && ((strncmp ("ipl", IDENTIFIER_POINTER (tok_value), 3) != 0
            && strncmp ("IPL", IDENTIFIER_POINTER (tok_value), 3) != 0)
          || IDENTIFIER_POINTER (tok_value)[3] > '7'
          || IDENTIFIER_POINTER (tok_value)[3] < '0')))
    {
      error ("priority level specifier not found for interrupt #pragma %s",
          fname);
      return;
    }
  ipl = tok_value;

  /* add the interrupt designation to the list of interrupt pragmas */
  p = ggc_alloc (sizeof (struct interrupt_pragma_spec));
  p->next = interrupt_pragma_list_head;
  p->name = fname;
  p->ipl = ipl;
  p->irq_number = 0;
  interrupt_pragma_list_head = p;

  /* We can also have a comma delimited list of vector functions to add
     targetting this handler. Each of these vectors is functionally equivalent
     to a separate #pragma vector targetting this function. Allowing the
     specification here is syntactic convenience. */
  tok = c_lex (&tok_value);
  /* the specifier is optional, so check for end of line explicitly */
  if (tok == CPP_EOF)
    return;

  /* We have some non-EOF token, so check if it's "vector" */
  if (tok != CPP_NAME || strcmp ("vector", IDENTIFIER_POINTER (tok_value)))
    {
      error ("'vector' expected in #pragma interrupt specification");
      return;
    }
  /* The first address is mandatory since we have a vector clause */
  /* We need to recognize the '@' sign here, but the c_lex function
     will issue an error if it sees it ('@' is only a valid token in
     Objective-C. So we need to use the lower-level functions to get
     this token in order to be able to see the '@' sign if it's there. */
  do
    raw_tok = cpp_get_token (parse_in);
  while (raw_tok->type == CPP_PADDING);
  if (raw_tok->type == CPP_ATSIGN)
    {
      /* '@' found */
      tok = c_lex (&tok_value);

      /* To place the handler function at the vector address, we
         put the function in the special section located at that
         address. */
      if (tok != CPP_NUMBER ||
          ((int)TREE_INT_CST_LOW (tok_value) < 0 ||
           (int)TREE_INT_CST_LOW (tok_value) > 63))
        {
          error ("IRQ number must be an integer between 0 and 63");
          return;
        }
      /* if this is a "single" handler, the only valid vector is zero */
      if (strcmp ("single", IDENTIFIER_POINTER (ipl)) == 0
          && (int)TREE_INT_CST_LOW (tok_value) != 0)
        {
          error ("'single' interrupt handlers only work with vector 0");
          return;
        }
      
      p->irq_number = (unsigned)TREE_INT_CST_LOW (tok_value);
    }
  else
    {
      /* it wasn't an '@' sign, so it's either a number or an error.
         handle that by backing up a token and re-lexing it with the
         normal c_lex() function. */
      _cpp_backup_tokens (parse_in, 1);
      tok = c_lex (&tok_value);

      if (tok != CPP_NUMBER ||
          ((int)TREE_INT_CST_LOW (tok_value) < 0 ||
           (int)TREE_INT_CST_LOW (tok_value) > 63))
        {
          error ("IRQ number must be an integer between 0 and 63");
          return;
        }
      /* if this is a "single" handler, the only valid vector is zero */
      if (strcmp ("single", IDENTIFIER_POINTER (ipl)) == 0
          && (int)TREE_INT_CST_LOW (tok_value) != 0)
        {
          error ("'single' interrupt handlers require vector 0");
          return;
        }
      /* add the vector to the list of dispatch functions to emit */
      mchp_add_vector_dispatch_entry (fname,
          (int)TREE_INT_CST_LOW (tok_value));
    }

  /* There are optionally more addresses, comma separated */
  tok = c_lex (&tok_value);
  while (tok == CPP_COMMA)
    {
      tok = c_lex (&tok_value);
      if (tok != CPP_NUMBER ||
          ((int)TREE_INT_CST_LOW (tok_value) < 0 ||
           (int)TREE_INT_CST_LOW (tok_value) > 63))
        {
          error ("IRQ number must be an integer between 0 and 63");
          return;
        }
      /* add the vector to the list of dispatch functions to emit */
      mchp_add_vector_dispatch_entry (fname,
          (int)TREE_INT_CST_LOW (tok_value));
      tok = c_lex (&tok_value);
    }

  /* No further input is valid. We should have end of line here. */
  if (tok != CPP_EOF)
    {
      error ("extraneous data at end of line of #pragma interrupt");
    }
}

/* Called when a decl is being created.

   This allows us to add attributes based on pragmas or other target
   specific stuff. In this case, we want to apply the interrupt attribute
   to decls of functions specified as interrupts via #pragma. If the
   handler function was specified to reside at the vector address, we
   will also apply a section attribute to the function to do that. */
static void
mchp_target_insert_attributes (tree decl, tree *attr_ptr)
{
  if (TREE_CODE (decl) == FUNCTION_DECL)
    {
      const char *fname;
      struct interrupt_pragma_spec *p;
      /* Check if an interrupt pragma is around for this function */
      fname = IDENTIFIER_POINTER (DECL_NAME (decl));
      assert (fname);
      for (p = interrupt_pragma_list_head ;
          p && strcmp (p->name, fname) ;
          p = p->next)
        ;
      /* if p != NULL, we have a match */
      if (p)
        {
          /* Apply the interrupt attribute to the decl.

             The attribute list is a TREE_LIST. We need to create a new
             TREE_LIST node with the existing attribute list as its
             TREE_CHAIN entry.

             The attribute itself has the purpose field being the name
             of the attribute and the parameters of the attribute being
             another TREE_LIST as the value of the attribute tree.  */
          *attr_ptr = tree_cons (get_identifier ("interrupt"),
              build_tree_list (NULL_TREE, p->ipl),
              *attr_ptr);
          /* If the interrupt goes directly at the vector address,
             add a section attribute to the function to place it in the
             dedicated vector section */
          if (p->irq_number)
            {
              char scn_name[11];
              assert (p->irq_number < 64);
              sprintf (scn_name, ".vector_%d", p->irq_number);
              *attr_ptr = tree_cons (get_identifier ("section"),
                  build_tree_list (NULL_TREE,
                    build_string (strlen (scn_name), scn_name)),
                  *attr_ptr);
            }
        }
    }
}

/* Callback when an interrupt attribute is recognized. 'node' points
   to the node to which the attribute is applied (which may not be a DECL).
   If it is a decl, we modify in place, otherwise we need to create a copy
   for any changes. 'name' is the identifier node specifying the attribute.
   'args' is a TREE_LIST of the arguments to the attribute. 'flags' gives
   us context information. We use 'no_add_attrs' to indicate whether the
   attribute should be added to the decl/type or not; in this case, on error.

   Return is NULL_TREE since we're not modifying the attribute for other
   decls/types */
static tree
mchp_interrupt_attribute (tree *node ATTRIBUTE_UNUSED,
    tree name ATTRIBUTE_UNUSED, tree args,
    int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  /* we want to validate that the argument isn't bogus. There should
     be one and only one argument in the args TREE_LIST and it should
     be an identifier of the form "ipl[0-7]". */

  /* we can assert one argument since that should be enforced by
     the parser from the attribute table */
  assert (TREE_CHAIN (args) == NULL);
  if (TREE_CODE (TREE_VALUE (args)) != IDENTIFIER_NODE
      || (strcmp ("single", IDENTIFIER_POINTER (TREE_VALUE (args))) != 0
          && ((strncmp ("ipl", IDENTIFIER_POINTER (TREE_VALUE (args)), 3) != 0
	       && strncmp ("IPL", IDENTIFIER_POINTER (TREE_VALUE(args)),3)!= 0)
            || IDENTIFIER_POINTER (TREE_VALUE (args))[3] > '7'
            || IDENTIFIER_POINTER (TREE_VALUE (args))[3] < '0')))
    {
      error ("Interrupt priority must be specified as 'ipl0' - 'ipl7'");
      *no_add_attrs = 1;
      return NULL_TREE;
    }

  return NULL_TREE;
}

/* Callback when a vector attribute is recognized. 'node' points
   to the node to which the attribute is applied (which may not be a DECL).
   If it is a decl, we modify in place, otherwise we need to create a copy
   for any changes. 'name' is the identifier node specifying the attribute.
   'args' is a TREE_LIST of the arguments to the attribute. 'flags' gives
   us context information. We use 'no_add_attrs' to indicate whether the
   attribute should be added to the decl/type or not; in this case, we
   never want to keep the attribute since it's just an indicator to us to
   add the vector.

   In effect, this is just a section attribute with a pre-defined section
   name.
   */
static tree
mchp_at_vector_attribute (tree *node, tree name ATTRIBUTE_UNUSED, tree args,
    int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree decl = *node;
  char scn_name[11];

  /* If this attribute isn't on the actual function declaration, we
     ignore it */
  if (TREE_CODE (decl) != FUNCTION_DECL)
    return NULL_TREE;

  if (DECL_SECTION_NAME (decl) != NULL_TREE)
    {
      error ("the 'at_vector' attribute cannot be used with the 'section' attribute");
      *no_add_attrs = true;
      return NULL_TREE;
    }
  /* The argument must be an integer constant between 0 and 63 */
  if (TREE_CODE (TREE_VALUE (args)) != INTEGER_CST ||
      (int)TREE_INT_CST_LOW (TREE_VALUE (args)) < 0 ||
      (int)TREE_INT_CST_LOW (TREE_VALUE (args)) > 63)
    {
    printf ("\nTree arg is %s\n", (char*)TREE_STRING_POINTER (TREE_VALUE (args)));
    *no_add_attrs = 1;

    error ("IRQ number must be an integer between 0 and 63");
    return NULL_TREE;
    }
  /* now mark the decl as going into the section for the indicated vector */
  sprintf (scn_name, ".vector_%d", (int)TREE_INT_CST_LOW (TREE_VALUE (args)));
  DECL_SECTION_NAME (decl) = build_string (strlen (scn_name), scn_name);

  return NULL_TREE;
}

/* Callback when a vector attribute is recognized. 'node' points
   to the node to which the attribute is applied (which may not be a DECL).
   If it is a decl, we modify in place, otherwise we need to create a copy
   for any changes. 'name' is the identifier node specifying the attribute.
   'args' is a TREE_LIST of the arguments to the attribute. 'flags' gives
   us context information. We use 'no_add_attrs' to indicate whether the
   attribute should be added to the decl/type or not; in this case, we
   never want to keep the attribute since it's just an indicator to us to
   add the vector.

   Return is NULL_TREE since we're not modifying the attribute for other
   decls/types */
static tree
mchp_vector_attribute (tree *node, tree name ATTRIBUTE_UNUSED, tree args,
    int flags ATTRIBUTE_UNUSED, bool *no_add_attrs)
{
  tree decl = *node;
    unsigned int subexpr;
  /* If this attribute isn't on the actual function declaration, we
     ignore it */
  if (TREE_CODE (decl) != FUNCTION_DECL)
    return NULL_TREE;

  /* The vector attribute has a comma delimited list of IRQ #'s as
     arguments. At least one must be present. */
  assert (args);
  while (args)
    {
      if (TREE_CODE (TREE_VALUE (args)) == INTEGER_CST)
        {
            /* The argument must be an integer constant between 0 and 63 */
            if ((int)TREE_INT_CST_LOW (TREE_VALUE (args)) < 0 ||
              (int)TREE_INT_CST_LOW (TREE_VALUE (args)) > 63)
              {
                *no_add_attrs = 1;

                error ("Vector number must be an integer between 0 and 63");
                return NULL_TREE;
              }
            /* add the vector to the list of dispatch functions to emit */
            mchp_add_vector_dispatch_entry (
                IDENTIFIER_POINTER (DECL_NAME (*node)),
                (int)TREE_INT_CST_LOW (TREE_VALUE (args)));
        }
      else
        {
          if (TREE_CODE (TREE_VALUE (args)) == COMPOUND_EXPR)
            {
              mchp_vector_attribute_compound_expr (node, TREE_VALUE(args),
                                                   no_add_attrs);
            }
        }
      args = TREE_CHAIN (args);
    }

  return NULL_TREE;
}

/* Handles compound expressions within the vector attribute list. 
   Called from mchp_vector_attribute(). */
static int
mchp_vector_attribute_compound_expr (tree *node, tree expr, 
                                     bool *no_add_attrs)
{
  int len = 0;

  if (expr == NULL_TREE)
    return 0;
  for (; TREE_CODE (expr) == COMPOUND_EXPR; expr = TREE_OPERAND (expr, 1))
    {
      len += mchp_vector_attribute_compound_expr (node, TREE_OPERAND (expr, 0), 
                                                  no_add_attrs);
    }
  if (TREE_CODE (expr) == INTEGER_CST)
    {
      /* The argument must be an integer constant between 0 and 63 */
      if ((int)TREE_INT_CST_LOW (expr) < 0 ||
          (int)TREE_INT_CST_LOW (expr) > 63)
        {
          *no_add_attrs = 1;

          error ("Vector number must be an integer between 0 and 63");
          return 0;
        }
      /* add the vector to the list of dispatch functions to emit */
      mchp_add_vector_dispatch_entry (
        IDENTIFIER_POINTER (DECL_NAME (*node)),
        (int)TREE_INT_CST_LOW (expr));
    }
  ++len;
  return len;
}

/* Returns true if regno is a register ordinarilly not callee saved which
   must nevertheless be preserved by an interrupt handler function. */
static int
mchp_register_interrupt_context_p (unsigned regno)
{
  /* return true for v0, v1, a0-a3, t0-t9 and ra */
  if ((regno >= 1 && regno <= 15) /* v0, v1, a0-a3, t0-t7 */
      || regno == 24 /* t8 */
      || regno == 25 /* t9 */
      || regno == 31) /* ra */
    return 1;
  return 0;
}

int
mchp_epilogue_uses (unsigned regno)
{
  /* $31 is the return address */
  if (regno == 31)
    return 1;
  /* if the register is part of the GPRs that's saved for interrupt context,
     we need to mark it as used by the epilogue. */
  if (current_function_type == SOFTWARE_CONTEXT_SAVE)
    return (mchp_register_interrupt_context_p (regno));
  return 0;
}

static int
mchp_function_naked_p (tree func)
{
  tree a;
  /* according to the specification of the attribute, we shouldn't ever
     have it on a non-function declaration */
  if (TREE_CODE (func) != FUNCTION_DECL)
    abort();

  a = lookup_attribute ("naked", DECL_ATTRIBUTES (func));
  return a != NULL_TREE;
}

#include "gt-mips.h"

