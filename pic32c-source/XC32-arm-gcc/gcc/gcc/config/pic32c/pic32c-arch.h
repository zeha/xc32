/* Definitions of types that are used to store PIC32C architecture and
   device information.
   Copyright (C) 2017 Free Software Foundation, Inc.
   Contributed by LG (lavinia.ghica@microchip.com)

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

#ifndef PIC32C_ARCH_H
#define PIC32C_ARCH_H

#define PIC32C_MMCU_DEFAULT "pic32c"

#ifndef NULL
#define     NULL    0
#endif

#ifndef     bool
#define     bool    unsigned char
#define     true    1
#define     false   0
#endif


typedef enum pic32c_spec_id
{
    LIB_STARTUP,
    DEVICE_LIB,
    LIB,
    INST_SET,
    DEVICE_MACRO,
    DEVICE_FPU,
    CPP,
    CC1_DEBUG,
    CC1_SAVE_TEMPS,
    ASM_ARCH,
    ASM_FLOAT,
    ENDIANNESS,
    LINKER_SCRIPT,
    CC1_SMARTIO,
    CC1_CONFIG_DATA,
    LINKER_SMARTIO,
    /* End of list.  */
    PIC32C_SPECS_END
}pic32c_spec_id;

typedef struct pic32c_spec_t
{
    pic32c_spec_id      spec_id;
    const char* const   spec_name;
    const char* const   spec_string;
}pic32c_spec_t;


/*
 * MCHP - PIC32C device specific features.
 */
typedef enum pic32c_arch_id
{
    CORTEX_M0PLUS   = 0x1,
    CORTEX_M3       = 0x2,
    CORTEX_M4       = 0x3,
    CORTEX_M7       = 0x4,
    PIC32C_UNKNOWN_ARCH
}pic32c_arch_id;



/* Architecture-specific properties.  */

typedef struct
{
    enum pic32c_arch_id arch_id;

    ///\ arm IS revision
    unsigned char arm_rev;

    ///\ Architecture name
    const char *const core_name;
    
    ///\ Architecture id to built-in define __PIC32C_ARCH__ (NULL -> no macro)
//    const char *const macro;
    unsigned long long    flags;

} pic32c_arch_t;


/* Device-specific properties.  */

typedef struct
{
    /* Device name.  */
    const char *const name;

    const char *const arch_name;
  
    /* Index in avr_arch_types[].  */
    enum pic32c_arch_id arch_id;


    /* device macro, NULL == no macro.  */
    const char *const macro;

    /* type of floting point unit, NULL == no fpu  */
    const char *const fpu;

} pic32c_mcu_t;


/* Preprocessor macros to define depending on MCU type.  */

extern pic32c_arch_t pic32c_arch_types[];
extern pic32c_arch_t *pic32c_arch;

extern pic32c_mcu_t pic32c_mcu_types[];

extern pic32c_spec_t pic32c_specs[];

#endif /* PIC32C_ARCH_H */
