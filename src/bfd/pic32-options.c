
/*
** pic32-options.c
**
** Copyright (c) 2008 Microchip Technology, Inc.
**
** This file contains pic32-specifc options
** processing for the linker.
**
** It is included by the following target-specific
** emulation files:
**
**   ld/emultmpl/pic32_elf32.em
**
*/

#include <errno.h>

/*
** list_options()
**
** This function is called by the --help option.
*/
static void 
elf32pic32mx_list_options (file) 
     FILE * file; 
{ 
  fprintf (file, _("  --report-mem         Report memory usage to console\n")); 
} /* static void elf32pic32mx_list_options () */

/*
** parse_args()
**
** This function is called for each command-line option.
*/
static int 
elf32pic32mx_parse_args (argc, argv) 
     int     argc; 
     char ** argv; 
{ 
  int        longind; 
  int        optc; 
  int        prevoptind = optind; 
  int        prevopterr = opterr; 
  int        wanterror; 
  static int lastoptind = -1; 

  if (lastoptind != optind) 
    opterr = 0; 
  
  wanterror  = opterr; 
  lastoptind = optind; 
  optc   = getopt_long_only (argc, argv, shortopts, longopts, & longind); 
  opterr = prevopterr; 
  switch (optc) 
    { 
    default: 
      if (wanterror) 
        xexit (1); 
      optind =  prevoptind; 
      return 0;
    case REPORT_MEM_OPTION:
      pic32_report_mem = TRUE;
      break;
    } 
  
  return 1; 
} /* static int elf32pic32mx_parse_args ()*/

