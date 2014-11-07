/*
** pic32-options.h
**
** Copyright (c) 2008 Microchip Technology, Inc.
**
** This file contains data declarations and
** function prototypes related to pic30-specifc
** options processing for the linker.
**
** It is included by the following target-specific
** emulation files:
**
**   ld/emultmpl/pic32_elf32.em
**
*/

#ifndef _PIC32_OPTIONS_H
#define _PIC32_OPTIONS_H

/*
** Externals declared in bfd *.c
*/

extern bfd_boolean pic32_report_mem;

static void elf32pic32mx_list_options 
   PARAMS ((FILE *)); 
static int elf32pic32mx_parse_args 
   PARAMS ((int, char**));

/* Emulation-specific Options */
const char * shortopts = "-Dp:"; /* note: leading "-" is important */

/* Codes used for the long options with no short synonyms.  1000 isn't
   special; it's just an arbitrary non-ASCII char value that is
   different from the range used in lexsup.c  */
enum elfpic32mx_options {
  REPORT_MEM_OPTION = 1000,
};

static struct option longopts[] = 
{ 
  { "report-mem", no_argument, NULL, REPORT_MEM_OPTION },
  { NULL,        no_argument,       NULL, 0                } 
}; 
#endif

