OUTPUT_FORMAT("elf32-tradbigmips", "elf32-tradbigmips", "elf32-tradlittlemips")
OUTPUT_ARCH(mips)
ENTRY(__start)
STARTUP(crt0.o)
GROUP(-lm -lc -lmdi -le -lgcc)
SEARCH_DIR(.);

SECTIONS
{
  PROVIDE (_ebase  =  0x80000000);
  PROVIDE (_smtc  =  _smtc_disable);
  PROVIDE (_vpe  =  _vpe_disable);
  /* Something (YAMON?) must have initialised the memory
     controller before we try to load to RAM. */
  .text 0x80100000: {
     _ftext = . ;
    PROVIDE (eprol  =  .);
    *(.text)
    *(.text.*)
    *(.gnu.linkonce.t*)
    *(.mips16.fn.*)
    *(.mips16.call.*)
    _ecode = .;
  }
  .init : {
    KEEP(*(.init))
  }
  .fini : {
    KEEP(*(.fini))
  }
  PROVIDE (etext  =  .);
  _etext  =  .;

  .eh_frame_hdr : { *(.eh_frame_hdr) }
  .eh_frame : { KEEP (*(.eh_frame)) }
  .gcc_except_table : { *(.gcc_except_table) }
  .jcr : { KEEP (*(.jcr)) }
  .ctors    :
  {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */

    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbeginS.o(.ctors))

    /* We don't want to include the .ctor section from
       from the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */

    KEEP (*(EXCLUDE_FILE (*crtend.o *crtendS.o *crtn.o) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  }

  .dtors    :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbeginS.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtendS.o *crtn.o) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  }

  . = .;
  .rodata : {
    *(.rdata)
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r*)
  }
  .sdeosabi : {
     /* SDE OSABI indication for GDB */
     *(.sdeosabi)
  }
  . = ALIGN(16);
  .data : {
    _fdata = .;
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d*)
  }
  .data1   : { *(.data1) }
  . = ALIGN(8);
  _gp = . + 0x7FF0;
  __global = _gp;
  .lit8 : {
    *(.lit8)
  }
  .lit4 : {
    *(.lit4)
  }
  .sdata : {
    *(.sdata)
    *(.sdata1) 
    *(.sdata.*)
    *(.gnu.linkonce.s*)
  }
  . = ALIGN(8);
  PROVIDE (edata  =  .);
  _edata  =  .;
  _fbss = .;
  .sbss : {
    __sbss_start = .;
    *(.sbss)
    *(.sbss.*)
    *(.scommon)
    __sbss_end = .;
  }
  .bss : {
    _bss_start = . ;
    *(.bss)
   *(.bss.*)
    *(COMMON)
  }

  . = ALIGN(32);
  PROVIDE (end = .);
  _end = .;

  /* DWARF debug sections.
     Symbols in the DWARF debugging sections are relative to
     the beginning of the section so we begin them at 0.  */

  /* DWARF 1 */
  .debug          0 : { *(.debug) }
  .line           0 : { *(.line) }

  /* GNU DWARF 1 extensions */
  .debug_srcinfo  0 : { *(.debug_srcinfo) }
  .debug_sfnames  0 : { *(.debug_sfnames) }

  /* DWARF 1.1 and DWARF 2 */
  .debug_aranges  0 : { *(.debug_aranges) }
  .debug_pubnames 0 : { *(.debug_pubnames) }

  /* DWARF 2 */
  .debug_info     0 : { *(.debug_info) }
  .debug_abbrev   0 : { *(.debug_abbrev) }
  .debug_line     0 : { *(.debug_line) }
  .debug_frame    0 : { *(.debug_frame) }
  .debug_str      0 : { *(.debug_str) }
  .debug_loc      0 : { *(.debug_loc) }
  .debug_macinfo  0 : { *(.debug_macinfo) }

  /* SGI/MIPS DWARF 2 extensions */
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
}
