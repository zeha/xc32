/* Target-dependent definitions for MIPS SDE.
   Copyright 2004 Free Software Foundation, Inc.
   Contributed by MIPS Technologies, Inc.

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

#ifndef TM_SDE_H
#define TM_SDE_H

#include "mips/tm-mips.h"

/* The original O32 ABI explicitly states that the naked char type is
   unsigned, and the other ABI specs don't contradict it. We stick to
   this for compatibility with previous releases of SDE, and for
   better performance. But this is different from other MIPS
   configurations. */

#define MIPS_CHAR_SIGNED 		0

/* On embedded MIPS we can't indicate a virtual method offset using
   bit 31, because is always set for the unmapped kernel address
   spaces, where embedded programs usually run. We'll use bit 30
   instead and hope that programs never get linked to run in KSEG2! */

#undef METHOD_PTR_IS_VIRTUAL
#undef METHOD_PTR_FROM_VOFFSET
#undef METHOD_PTR_TO_VOFFSET
#define METHOD_PTR_IS_VIRTUAL(ADDR)  ((ADDR) & 0x40000000)
#define METHOD_PTR_FROM_VOFFSET(OFFSET) (0x40000000 | (OFFSET))
#define METHOD_PTR_TO_VOFFSET(ADDR) (~0x40000000 & (ADDR))

#endif /* TM_SDE_H */
