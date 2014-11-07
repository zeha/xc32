/* Microchip PIC32-specific support for 32-bit ELF
   Copyright 1993, 1995, 1999 Free Software Foundation, Inc.

This file is part of BFD, the Binary File Descriptor library.

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _ELF32_PIC32_H
#define _ELF32_PIC32_H

#include "bfd.h"
#include "sysdep.h"
#include "elf-bfd.h"

/* Variables set by pic32 linker options */
bfd_boolean pic32_debug = 0;
bfd_boolean pic32_report_mem = 0;
bfd_boolean pic32_smart_io = TRUE; /* Enabled by default */
bfd_boolean pic32_has_smart_io_option = 0;

#endif

