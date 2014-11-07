/* Definitions of target machine for GNU compiler.  
   MIPS SDE version.
   Copyright (C) 2003, 2004
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


#define TARGET_MIPS_SDE	1

/* Let the -march option set soft-float. */

#define MIPS_MARCH_CONTROLS_SOFT_FLOAT 1

/* Select default ABI based on whether ISA supports 64-bits */

#define MIPS_ISA_CONTROLS_ABI() \
  (mips_abi_string \
   ? mips_abi			/* explicit -mabi option */ \
   : ((ISA_HAS_64BIT_REGS	/* 64-bit ISA without explicit -mgp32 */ \
       && (TARGET_64BIT || !(target_flags_explicit & MASK_64BIT))) \
      ? ABI_N32 \
      : ABI_32))

#undef ASM_ABI_DEFAULT_SPEC
#define ASM_ABI_DEFAULT_SPEC "%{mgp32:-32;mips64:-n32;mips64r2:-n32;mips3:-n32;mips4:-n32;mgp64:-n32;:-32}"

/* Use trap rather than break for all but MIPS I ISA. Force -no-mips16, so that
   MIPS16 assembler code requires an explicit ".set mips16". */

#undef SUBTARGET_ASM_SPEC
#define SUBTARGET_ASM_SPEC "\
%{!mips1:--trap} \
%{fPIC|fpic|fpie:%{!mips16*:-KPIC}} \
%{mips16:-no-mips16} %{mips16e:-no-mips16e}"

#undef LINK_SPEC
#define LINK_SPEC "\
%(endian_spec) \
%{G*} %{mips1} %{mips2} %{mips3} %{mips4} %{mips32} %{mips32r2} %{mips64} \
%{bestGnum} \
%{shared} %{non_shared}  %{call_shared} \
%{shared|call_shared: \
  %{mabi=n32:-melf32%{EB:b}%{EL:l}tsmipn32} \
  %{mabi=64:-melf64%{EB:b}%{EL:l}tsmip} \
  %{mabi=32:-melf32%{EB:b}%{EL:l}tsmip} \
  %{!mabi:%{!mips64:%{!mips3:%{!mips4:-melf32%{EB:b}%{EL:l}tsmip}}}} \
  %{!mabi:%{mips64*|mips3|mips4:-melf32%{EB:b}%{EL:l}tsmipn32}}} \
%{!shared:%{!call_shared: \
  %{mabi=n32:-melf32mipssden32} \
  %{mabi=64:-melf64mipssde} \
  %{!mabi:%{!mgp32:%{mips64*|mips3|mips4:-melf32mipssden32}}}}}"

/* Define this as 1 if `char' should by default be signed; else as 0.  */

#undef DEFAULT_SIGNED_CHAR
#define DEFAULT_SIGNED_CHAR 1

/* SDE-MIPS won't ever support SDB or MIPS debugging info. */

#undef SDB_DEBUGGING_INFO
#undef MIPS_DEBUGGING_INFO

/* But we will allow DWARF-1 for backwards compatibility. */

#define DWARF_DEBUGGING_INFO

#undef SUBTARGET_TARGET_SWITCHES
#define SUBTARGET_TARGET_SWITCHES					\
  {"no-float",	  MASK_SOFT_FLOAT | MASK_NO_FLOAT,			\
     N_("Don't use floating point libraries")},				\

#define TARGET_OS_CPP_BUILTINS()			\
    do {						\
      builtin_assert ("machine=mips");			\
      builtin_assert ("system=sde");			\
      builtin_assert ("system=posix");			\
      builtin_define ("__SDE_MIPS__");			\
							\
      if (TARGET_ABICALLS || flag_pic)			\
        {						\
	  builtin_define ("__PIC__");			\
	  builtin_define ("__pic__");			\
        }						\
							\
      if (ISA_MIPS32R2 || ISA_MIPS64R2)			\
	/* deprecated - use __mips_isa_rev >= 2 */	\
        builtin_define ("__mipsr2");			\
							\
      if (mips_abi == ABI_N32)				\
        {						\
	  builtin_assert ("abi=n32");			\
          builtin_define ("_ABIN32=2");			\
          builtin_define ("_MIPS_SIM=_ABIN32");		\
        }						\
      else if (mips_abi == ABI_64)			\
        {						\
	  builtin_assert ("abi=n64");			\
          builtin_define ("_ABI64=3");			\
          builtin_define ("_MIPS_SIM=_ABI64");		\
        }						\
      else if (mips_abi == ABI_O64)			\
        {						\
	  builtin_assert ("abi=o64");			\
          builtin_define ("_ABIO64=4");			\
          builtin_define ("_MIPS_SIM=_ABIO64");		\
        }						\
      else if (mips_abi == ABI_32)			\
        {						\
	  builtin_assert ("abi=32");			\
	  builtin_define ("_ABIO32=1");			\
	  builtin_define ("_MIPS_SIM=_ABIO32");		\
        }						\
      else if (mips_abi == ABI_EABI)			\
        {						\
	  builtin_assert ("abi=eabi");			\
          builtin_define ("_ABIEABI=5");		\
          builtin_define ("_MIPS_SIM=_ABIEABI");	\
        }						\
							\
     if (TARGET_INT64)					\
        builtin_define ("_MIPS_SZINT=64");		\
     else						\
        builtin_define ("_MIPS_SZINT=32");		\
							\
     if (TARGET_LONG64)					\
        builtin_define ("_MIPS_SZLONG=64");		\
     else						\
        builtin_define ("_MIPS_SZLONG=32");		\
							\
     if (POINTER_SIZE == 64)				\
        builtin_define ("_MIPS_SZPTR=64");		\
     else						\
        builtin_define ("_MIPS_SZPTR=32");		\
							\
      if (TARGET_FLOAT64)				\
	{						\
          builtin_define ("_MIPS_FPSET=32");		\
	  /* deprecated - use __mips_fpr == 64 */	\
	  builtin_define ("__mipsfp64");		\
	}						\
     else						\
	{						\
          builtin_define ("_MIPS_FPSET=16");		\
	}						\
							\
      if (TARGET_64BIT)					\
	builtin_define ("__mips64");			\
							\
      if (TARGET_NO_FLOAT) 				\
	{						\
	  builtin_define ("__NO_FLOAT");		\
	  builtin_define ("__mips_no_float");		\
	}						\
      else if (TARGET_SOFT_FLOAT && !mips16_hard_float)	\
	{						\
	  builtin_define ("__SOFT_FLOAT");		\
	}						\
      else if (TARGET_SINGLE_FLOAT)			\
	{						\
	  builtin_define ("__SINGLE_FLOAT");		\
	}						\
							\
      if (TARGET_BIG_ENDIAN)				\
        {						\
	  builtin_assert ("endian=big");		\
	  builtin_assert ("cpu=mipseb");		\
	}						\
      else						\
        {						\
	  builtin_assert ("endian=little");		\
	  builtin_assert ("cpu=mipsel");		\
	}						\
    } while (0)


/* Using long will always be right for the size_t and ptrdiff_t, since
   sizeof(long) must equal sizeof(void *), following from the setting of
   the -mlong64 option */

#undef SIZE_TYPE
#define SIZE_TYPE	"long unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE	"long int"

#if 0
/* We now have an indirect errno pointer, and might want to support
   matherr at some point, so we can't do this any more. */

/* This is the value of the error code EDOM for this machine,
   used by the sqrt instruction.  */

/* For an IEEE-compliant sqrt we must define SDE's value of EDOM */
#define TARGET_EDOM	33

/* This is how to refer to the variable errno. 
   Take account of the size of an int. */

#define GEN_ERRNO_RTX \
  gen_rtx_MEM (TARGET_INT64 ? DImode : SImode, \
	       gen_rtx_SYMBOL_REF (Pmode, "errno"))
#endif

/* Tell calls.c about SDE-MIPS standard library functions that look
   like setjmp and longjmp. */

#define FNNAME_LIKE_SETJMP(NAME) \
     ((NAME)[0] == 'x' && (NAME)[1] == 'c' \
      && ! strcmp (NAME, "xcptsetjmp"))

#define FNNAME_LIKE_LONGJMP(NAME) \
     ((NAME)[0] == 'x' && (NAME)[1] == 'c' \
      && (! strcmp (NAME, "xcptlongjmp") || ! strcmp (NAME, "xcptrestore")))


/* Enable parsing of #pragma pack(push,<n>) and #pragma pack(pop).  */

#define HANDLE_PRAGMA_PACK_PUSH_POP 1

/* Use standard ELF-style local labels (not '$' as on early Irix) */

#undef LOCAL_LABEL_PREFIX
#define LOCAL_LABEL_PREFIX "."

/* Use periods rather than dollar signs in special g++ assembler names.  */

#define NO_DOLLAR_IN_LABEL

/* Attach a special .ident directive to the end of the file to identify
   the version of GCC which compiled this code.  */

#undef IDENT_ASM_OP
#define IDENT_ASM_OP 	"\t.ident\t"

/* Output #ident string into the ELF .comment section, so it doesn't
   form part of the load image, and which can be stripped. */

#undef ASM_OUTPUT_IDENT
#define ASM_OUTPUT_IDENT(STREAM, STRING)				\
  fprintf (STREAM, "%s\"%s\"\n", IDENT_ASM_OP, STRING);

/* This is how to declare the size of a function.  */
#undef ASM_DECLARE_FUNCTION_SIZE
#define ASM_DECLARE_FUNCTION_SIZE(FILE, FNAME, DECL)		\
  do								\
    {								\
      if (!flag_inhibit_size_directive)				\
	ASM_OUTPUT_MEASURED_SIZE (FILE, FNAME);			\
    }								\
  while (0)

/* Currently we don't support 128bit long doubles, so for now we force
   N32 to be 64bit. */
#undef LONG_DOUBLE_TYPE_SIZE
#define LONG_DOUBLE_TYPE_SIZE 64

#ifdef IN_LIBGCC2
#undef  LIBGCC2_LONG_DOUBLE_TYPE_SIZE
#define LIBGCC2_LONG_DOUBLE_TYPE_SIZE 64
#endif

/* Describe how we implement __builtin_eh_return.  */

/* As far as I can see, nothing uses more than 2 EH data registers. So
   return the registers $6 & $7 in reverse order. Must not clash with
   return register ($2), or EH_RETURN_STACKADJ ($3), or
   MIPS_EPILOGUE_TEMP ($5). */
#undef EH_RETURN_DATA_REGNO
#define EH_RETURN_DATA_REGNO(N) \
  ((N) < 2 ? 7 - (N) : INVALID_REGNUM)

/* Use $5 as a temporary for both MIPS16 and non-MIPS16. */
#undef MIPS_EPILOGUE_TEMP_REGNUM
#define MIPS_EPILOGUE_TEMP_REGNUM (GP_REG_FIRST + 5)

/* For __clear_cache in libgcc2.c */
#undef CLEAR_INSN_CACHE
#define CLEAR_INSN_CACHE(beg, end) \
	mips_sync_icache (beg, end - beg)

/* For mips_cache_flush_func in mips.h */
#undef CACHE_FLUSH_FUNC
#define CACHE_FLUSH_FUNC	"mips_sync_icache"

/* For mips.h -  */
/* For inline code which needs to sync the icache and dcache,
   noting that the SDE library takes arguments (address, size). */
#undef MIPS_ICACHE_SYNC
#define MIPS_ICACHE_SYNC(ADDR, SIZE)					\
  if (mips_cache_flush_func && mips_cache_flush_func[0])		\
    emit_library_call (gen_rtx_SYMBOL_REF (Pmode, mips_cache_flush_func),\
		       0, VOIDmode, 2, ADDR, Pmode,			\
		       GEN_INT (SIZE), TYPE_MODE (sizetype))

#undef FUNCTION_PROFILER
#define FUNCTION_PROFILER(FILE, LABELNO)				\
{									\
  fprintf (FILE, "\t.set\tnoat\n");					\
  if (TARGET_MIPS16) 							\
    fprintf (FILE, "\tmove\t%s,%s\n",					\
	     reg_names[V0_REGNUM+1], reg_names[RA_REGNUM]);		\
  else									\
    fprintf (FILE, "\tmove\t%s,%s\n",					\
	     reg_names[AT_REGNUM], reg_names[RA_REGNUM]);		\
  if (TARGET_ABICALLS)							\
    {									\
      fprintf (FILE, "\tla\t%s,_mcount\n", reg_names[PIC_FUNCTION_ADDR_REGNUM]); \
      fprintf (FILE, "\tjal\t%s,%s\n", 					\
	       reg_names[GP_REG_FIRST + 31],				\
	       reg_names[PIC_FUNCTION_ADDR_REGNUM]); 			\
    }									\
  else									\
    fprintf (FILE, "\tjal\t_mcount\n");					\
  fprintf (FILE, "\t.set\tat\n");					\
}

/* Force all .init and .fini entries to be 32-bit, not mips16, so that
   in a mixed environment they are all the same mode. The crti.asm and
   crtn.asm files will also be compiled as 32-bit due to the
   -no-mips16 flag in SUBTARGET_ASM_SPEC above. */
#undef CRT_CALL_STATIC_FUNCTION
#define CRT_CALL_STATIC_FUNCTION(SECTION_OP, FUNC)	\
   asm (SECTION_OP "\n\
	.set push\n\
	.set nomips16\n\
	jal " USER_LABEL_PREFIX #FUNC "\n\
	.set pop\n\
	" TEXT_SECTION_ASM_OP);

/* Force the call stack unwinders in unwind.inc not to be MIPS16 code
   when compiled with hardware floating point. This is because MIPS16
   cannot save and restore the floating point registers, which will be
   important if in a mixed MIPS16/non-MIPS16 environment. */

#ifdef IN_LIBGCC2
#if __mips_hard_float
#define UNWIND_ATTRIBUTE __attribute__((__nomips16__))
#endif
#endif /* IN_LIBGCC2 */
