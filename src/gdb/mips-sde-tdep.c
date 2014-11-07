/* Target-dependent code for GNU/Linux on MIPS processors.

   Copyright 2001, 2002, 2004 Free Software Foundation, Inc.

   This file is part of GDB.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include "defs.h"
#include "osabi.h"
#include "gdb_string.h"
#include "elf-bfd.h"
#include "block.h"
#include "symtab.h"

#include "frame.h"
#include "frame-unwind.h"
#include "frame-base.h"
#include "trad-frame.h"

#include "mips-tdep.h"

static struct trad_frame_cache *
mips_sde_frame_cache (struct frame_info *next_frame, void **this_cache)
{
  CORE_ADDR pc;
  CORE_ADDR xcpt_frame;
  CORE_ADDR start_addr;
  CORE_ADDR stack_addr;
  struct trad_frame_cache *this_trad_cache;
  const struct mips_regnum *regs = mips_regnum (current_gdbarch);
  int i;
  const int sizeof_reg_t = mips_abi_regsize (current_gdbarch); /* FIXME: Is this correct? */
  enum mips_abi abi = mips_abi (current_gdbarch);

  if ((*this_cache) != NULL)
    return (*this_cache);
  this_trad_cache = trad_frame_cache_zalloc (next_frame);
  (*this_cache) = this_trad_cache;

  /* The previous registers are held in struct xcptcontext
     which is at $sp+offs

     struct xcptcontext {
       reg_t	sr;
       reg_t	icr;
       reg_t	cr;
       reg_t	epc;
       reg_t	vaddr;
       reg_t	regs[32];
       reg_t	mdlo;
       reg_t	mdhi;
       reg_t	mdex;
       ...
     };
  */

  stack_addr = frame_unwind_register_signed (next_frame, SP_REGNUM);
  switch (abi)
    {
    case MIPS_ABI_O32:
      /* 40: XCPTCONTEXT
	 24: xcpt_gen() argspace		(16 bytes)
	 16: _xcptcall() saved ra, rounded up	( 8 bytes)
	 00: _xcptcall() argspace 		(16 bytes) */
      xcpt_frame = stack_addr + 40; 
      break;
    case MIPS_ABI_N32:
    case MIPS_ABI_N64:
    default:			/* wild guess */
      /* 16: XCPTCONTEXT
	 16: xcpt_gen() argspace 		( 0 bytes)
	 00: _xcptcall() saved ra, rounded up	(16 bytes) */
      xcpt_frame = stack_addr + 16;
      break;
    }

  trad_frame_set_reg_addr (this_trad_cache, PS_REGNUM + NUM_REGS,
			   xcpt_frame + 0*sizeof_reg_t);
  trad_frame_set_reg_addr (this_trad_cache, regs->cause + NUM_REGS,
			   xcpt_frame + 1*sizeof_reg_t);
  trad_frame_set_reg_addr (this_trad_cache, regs->pc + NUM_REGS,
			   xcpt_frame + 2*sizeof_reg_t);
  trad_frame_set_reg_addr (this_trad_cache, regs->badvaddr + NUM_REGS,
			   xcpt_frame + 3*sizeof_reg_t);
  for (i = 0; i < MIPS_NUMREGS; i++) {
    trad_frame_set_reg_addr (this_trad_cache, i + ZERO_REGNUM + NUM_REGS,
			     xcpt_frame + (4+i)*sizeof_reg_t);
  }
  trad_frame_set_reg_addr (this_trad_cache, regs->lo + NUM_REGS,
			   xcpt_frame + 36*sizeof_reg_t);
  trad_frame_set_reg_addr (this_trad_cache, regs->hi + NUM_REGS,       
			   xcpt_frame + 37*sizeof_reg_t);

  pc = frame_pc_unwind (next_frame);
  find_pc_partial_function (pc, NULL, &start_addr, NULL);
  trad_frame_set_id (this_trad_cache, frame_id_build (start_addr, stack_addr));

  return this_trad_cache;
}

static void
mips_sde_frame_this_id (struct frame_info *next_frame, void **this_cache,
			 struct frame_id *this_id)
{
  struct trad_frame_cache *this_trad_cache
    = mips_sde_frame_cache (next_frame, this_cache);
  trad_frame_get_id (this_trad_cache, this_id);
}

static void
mips_sde_frame_prev_register (struct frame_info *next_frame,
				 void **this_cache,
				 int regnum, int *optimizedp,
				 enum lval_type *lvalp, CORE_ADDR *addrp,
				 int *realnump, void *valuep)
{
  struct trad_frame_cache *this_trad_cache
    = mips_sde_frame_cache (next_frame, this_cache);
  trad_frame_get_register (this_trad_cache, next_frame, regnum, optimizedp,
			   lvalp, addrp, realnump, valuep);
}

static const struct frame_unwind mips_sde_frame_unwind =
{
  SIGTRAMP_FRAME,
  mips_sde_frame_this_id,
  mips_sde_frame_prev_register
};

static const struct frame_unwind *
mips_sde_frame_sniffer (struct frame_info *next_frame)
{
  CORE_ADDR pc = frame_pc_unwind (next_frame);
  char *name;

  find_pc_partial_function (pc, &name, NULL, NULL);
  if (name
      && (strcmp (name, "_xcptcall") == 0
	  || strcmp (name, "_sigtramp") == 0))
    return &mips_sde_frame_unwind;
  else
    return NULL;
}

static CORE_ADDR
mips_sde_frame_base_address (struct frame_info *next_frame,
			      void **this_cache)
{
  struct trad_frame_cache *this_trad_cache
    = mips_sde_frame_cache (next_frame, this_cache);
  return trad_frame_get_this_base (this_trad_cache);
}

static const struct frame_base mips_sde_frame_base =
{
  &mips_sde_frame_unwind,
  mips_sde_frame_base_address,
  mips_sde_frame_base_address,
  mips_sde_frame_base_address
};

static const struct frame_base *
mips_sde_frame_base_sniffer (struct frame_info *next_frame)
{
  if (mips_sde_frame_sniffer (next_frame) != NULL)
    return &mips_sde_frame_base;
  else
    return NULL;
}

static void
mips_sde_elf_osabi_sniff_abi_tag_sections (bfd *abfd, asection *sect,
					   void *obj)
{
  enum gdb_osabi *os_ident_ptr = obj;
  const char *name;

  name = bfd_get_section_name (abfd, sect);

  if (strncmp (name, ".sde", 4) == 0)
    {
      /* The presence of a section with a ".sde" prefix is
         indicative of an SDE binary  */
      *os_ident_ptr = GDB_OSABI_SDE;
    }
}

static enum gdb_osabi
mips_sde_elf_osabi_sniffer (bfd *abfd)
{
  unsigned int elfosabi;
  enum gdb_osabi osabi = GDB_OSABI_UNKNOWN;

  /* If the generic sniffer gets a hit, return and let other sniffers
     get a crack at it.  */
  bfd_map_over_sections (abfd,
			 generic_elf_osabi_sniff_abi_tag_sections,
			 &osabi);
  if (osabi != GDB_OSABI_UNKNOWN)
    return GDB_OSABI_UNKNOWN;

  elfosabi = elf_elfheader (abfd)->e_ident[EI_OSABI];

  if (elfosabi == ELFOSABI_NONE)
    {
      /* When elfosabi is ELFOSABI_NONE (0), then the ELF structures in the
	 file are conforming to the base specification for that machine 
	 (there are no OS-specific extensions).  In order to determine the 
	 real OS in use we must look for OS notes that have been added.  
	 
	 For IRIX, we simply look for sections named with .sde. as
	 prefixes.  */
      bfd_map_over_sections (abfd,
			     mips_sde_elf_osabi_sniff_abi_tag_sections, 
			     &osabi);
    }
  return osabi;
}

static void
mips_sde_init_abi (struct gdbarch_info info,
                    struct gdbarch *gdbarch)
{
  frame_unwind_append_sniffer (gdbarch, mips_sde_frame_sniffer);
  frame_base_append_sniffer (gdbarch, mips_sde_frame_base_sniffer);
}

void
_initialize_mips_sde_tdep (void)
{
  const struct bfd_arch_info *arch_info;

  gdbarch_register_osabi_sniffer (bfd_arch_mips, 
				  bfd_target_elf_flavour,
				  mips_sde_elf_osabi_sniffer);
				      
  gdbarch_register_osabi (bfd_arch_mips, 0, GDB_OSABI_SDE,
			  mips_sde_init_abi);
}


