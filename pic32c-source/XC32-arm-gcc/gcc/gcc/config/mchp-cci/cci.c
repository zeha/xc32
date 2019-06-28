/* Subroutines used for Microchip PIC32 MCU support
   Copyright (C) 2017
   Microchip Technology, Inc.

This file is part of Microchip XC compiler tool-chain.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */

/*
 *  Common CCI definitions for Microchip Compilers
 */

#ifdef _BUILD_MCHP_

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "target.h"
#include "varasm.h"
#include "output.h"
#include "tree-core.h"
#include "tree.h"
#include "memmodel.h"
#include "gimple.h"
#include "cfghooks.h"
#include "df.h"
#include "tm_p.h"
#include "stringpool.h"
#include "cgraph.h"
#include "alias.h"
#include "fold-const.h"
#include "stor-layout.h"
#include "calls.h"
#include "explow.h"
#include "libfuncs.h"
#include "common/common-target.h"
#include "langhooks.h"
#include "cfgrtl.h"
#include "cfganal.h"
#include "sched-int.h"
#include "gimplify.h"
#include "tree-pass.h"
#include "context.h"
#include "target-globals.h"
#include "builtins.h"
#include "c-family/c-common.h"
#include "c-family/c-pragma.h"
#include "c/c-tree.h"
#include "diagnostic.h"

#define CCI_H "config/mchp-cci/cci.h"
#include CCI_H

void mchp_print_builtin_function (const_tree t)
{
  if (t && DECL_P(t))
    puts (IDENTIFIER_POINTER(DECL_NAME(t)));
}

#endif /* _BUILD_MCHP_ */
