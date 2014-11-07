/* Microchip CCI Backend
   Copyright (C) 1987, 1989, 1992, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009,
   2010
   Free Software Foundation, Inc.

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 3, or (at your option) any later
version.

GCC is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/*
 *  Common CCI Backend definitions for Microchip Compilers
 */

#include <stdio.h>
#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "tree.h"

#define CCI_BACKEND_H "config/mchp-cci/cci-backend.h"
#include CCI_BACKEND_H

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

void mchp_print_builtin_function (const_tree t)
{
  if (DECL_P(t))
    puts (IDENTIFIER_POINTER(DECL_NAME(t)));
}
