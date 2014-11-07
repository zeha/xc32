/* Native-dependent code for GNU/Linux on MIPS processors.

   Copyright 2001, 2002, 2003 Free Software Foundation, Inc.

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
#include "gdbcore.h"
#include "inferior.h"

#include "gdb_assert.h"
#include "gdb_ptrace.h"

#include "mips-tdep.h"

/* Pseudo registers can not be read.  ptrace does not provide a way to
   read (or set) PS_REGNUM, and there's no point in reading or setting
   ZERO_REGNUM.  We also can not set BADVADDR, CAUSE, or FCRIR via
   ptrace().  */

static int
mips_linux_cannot_fetch_register (int regno)
{
  if (regno > ZERO_REGNUM && regno < ZERO_REGNUM + 32)
    return 0;
  else if (regno >= mips_regnum (current_gdbarch)->fp0
	   && regno <= mips_regnum (current_gdbarch)->fp0 + 32)
    return 0;
  else if (regno >= mips_regnum (current_gdbarch)->dspacc
	   && regno <= mips_regnum (current_gdbarch)->dspacc + 6)
    return 0;
  else if (regno == mips_regnum (current_gdbarch)->lo
	   || regno == mips_regnum (current_gdbarch)->hi
	   || regno == mips_regnum (current_gdbarch)->badvaddr
	   || regno == mips_regnum (current_gdbarch)->cause
	   || regno == mips_regnum (current_gdbarch)->pc
	   || regno == mips_regnum (current_gdbarch)->fp_control_status
	   || regno == mips_regnum (current_gdbarch)->fp_implementation_revision
	   || regno == mips_regnum (current_gdbarch)->dspctl)
    return 0;
  else
    return 1;
}

static int
mips_linux_cannot_store_register (int regno)
{
  if (regno > ZERO_REGNUM && regno < ZERO_REGNUM + 32)
    return 0;
  else if (regno >= mips_regnum (current_gdbarch)->fp0
	   && regno <= mips_regnum (current_gdbarch)->fp0 + 32)
    return 0;
  else if (regno >= mips_regnum (current_gdbarch)->dspacc
	   && regno <= mips_regnum (current_gdbarch)->dspacc + 6)
    return 0;
  else if (regno == mips_regnum (current_gdbarch)->lo
	   || regno == mips_regnum (current_gdbarch)->hi
	   || regno == mips_regnum (current_gdbarch)->pc
	   || regno == mips_regnum (current_gdbarch)->fp_control_status
	   || regno == mips_regnum (current_gdbarch)->dspctl)
    return 0;
  else
    return 1;
}

/* Fetch register REGNUM from the inferior.  */

static void
fetch_register (int regnum)
{
  CORE_ADDR addr;
  size_t size;
  PTRACE_TYPE_RET *buf;
  int tid, i;

  if (mips_linux_cannot_fetch_register (regnum))
    {
      regcache_raw_supply (current_regcache, regnum, NULL);
      return;
    }

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = TIDGET (inferior_ptid);
  if (tid == 0)
    tid = PIDGET (inferior_ptid); /* Not a threaded program.  */

  /* This isn't really an address.  But ptrace thinks of it as one.  */
  addr = register_addr (regnum, U_REGS_OFFSET);
  size = register_size (current_gdbarch, regnum);

  gdb_assert ((size % sizeof (PTRACE_TYPE_RET)) == 0);
  buf = alloca (size);

  /* Read the register contents from the inferior a chuck at the time.  */
  for (i = 0; i < size / sizeof (PTRACE_TYPE_RET); i++)
    {
      errno = 0;
      buf[i] = ptrace (PT_READ_U, tid, (PTRACE_TYPE_ARG3) addr, 0);
      if (errno == EIO)
	{
	  set_register_cached (regnum, -1);
	  break;
	}
      else if (errno != 0)
	error ("Couldn't read register %s (#%d): %s.", REGISTER_NAME (regnum),
	       regnum, safe_strerror (errno));

      addr += sizeof (PTRACE_TYPE_RET);
    }
  regcache_raw_supply (current_regcache, regnum, buf);
}

/* Fetch register REGNUM from the inferior.  If REGNUM is -1, do this
   for all registers.  */

void
fetch_inferior_registers (int regnum)
{
  if (regnum == -1)
    for (regnum = 0; regnum < NUM_REGS; regnum++)
      fetch_register (regnum);
  else
    fetch_register (regnum);
}

/* Store register REGNUM into the inferior.  */

static void
store_register (int regnum)
{
  CORE_ADDR addr;
  size_t size;
  PTRACE_TYPE_RET *buf;
  int tid, i;

  if (mips_linux_cannot_store_register (regnum))
    return;

  /* GNU/Linux LWP ID's are process ID's.  */
  tid = TIDGET (inferior_ptid);
  if (tid == 0)
    tid = PIDGET (inferior_ptid); /* Not a threaded program.  */

  /* This isn't really an address.  But ptrace thinks of it as one.  */
  addr = register_addr (regnum, U_REGS_OFFSET);
  size = register_size (current_gdbarch, regnum);

  gdb_assert ((size % sizeof (PTRACE_TYPE_RET)) == 0);
  buf = alloca (size);

  /* Write the register contents into the inferior a chunk at the time.  */
  regcache_raw_collect (current_regcache, regnum, buf);
  for (i = 0; i < size / sizeof (PTRACE_TYPE_RET); i++)
    {
      errno = 0;
      ptrace (PT_WRITE_U, tid, (PTRACE_TYPE_ARG3) addr, buf[i]);
      if (errno == EIO)
	break;
      else if (errno != 0)
	error ("Couldn't write register %s (#%d): %s.", REGISTER_NAME (regnum),
	       regnum, safe_strerror (errno));

      addr += sizeof (PTRACE_TYPE_RET);
    }
}

/* Store register REGNUM back into the inferior.  If REGNUM is -1, do
   this for all registers (including the floating point registers).  */

void
store_inferior_registers (int regnum)
{
  if (regnum == -1)
    for (regnum = 0; regnum < NUM_REGS; regnum++)
      store_register (regnum);
  else
    store_register (regnum);
}
