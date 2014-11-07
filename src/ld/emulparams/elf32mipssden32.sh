EMBEDDED=yes

# Default to non-demand paged, non-dynamic link
DEMAND_PAGED=FALSE
DYNAMIC_LINK=FALSE

DATA_PLT=
INIT_AFTER_TEXT=

# This is an ELF platform.
# Handle both big- and little-ended 32-bit MIPS objects.
ARCH=mips
MACHINE=
SCRIPT_NAME=elf
ELFSIZE=32
TEMPLATE_NAME=elf32
OUTPUT_FORMAT="elf32-ntradbigmips"
BIG_OUTPUT_FORMAT="elf32-ntradbigmips"
LITTLE_OUTPUT_FORMAT="elf32-ntradlittlemips"

# SDE kit expects to be started at __start, not _start
ENTRY=__start

# Default load address of KSEG0 + 128K (suitable for PMON)
TEXT_START_ADDR='/*TEXT_ADDR*/ 0x80020000'
NONPAGED_TEXT_START_ADDR='/*TEXT_ADDR*/ 0x80020000'
DATA_ADDR='/*DATA_ADDR*/ ALIGN(8)'

# Shared library options
GENERATE_SHLIB_SCRIPT=
SHLIB_TEXT_START_ADDR='0x80000000'
MAXPAGESIZE=0x40000
COMMONPAGESIZE=0x1000
TEXT_DYNAMIC=

# Force .reset section to start of text (i.e. rom), followed by 
# relocatable boot code that needs to be linked within +/-128K
# (max branch distance)
TEXT_START_SYMBOLS='
  _ftext = . ;
  *(.reset)
  *(.boot)
'

OTHER_TEXT_SECTIONS='*(.mips16.fn.*) *(.mips16.call.*)'

# Use _ecode at end of executable code
TEXT_END_SYMBOLS='
  __ecode = .;
  _ecode = .;
  PROVIDE (ecode = .);
'

# Use _etext at end of code and read-only data
READONLY_END_SYMBOLS='
  __etext = .;
  _etext = .;
  PROVIDE (etext = .);
'

# Define _fdata at start of read-write data, not at start of .data
# XXX Need to sort out how .tdata etc get positioned first
DATA_START_SYMBOLS='_fdata = . ;'
#READWRITE_START_SYMBOLS='
#  _fdata = . ;
#'

# GOT-related settings.  
OTHER_GOT_SYMBOLS='
  _gp = ALIGN(16) + 0x7ff0;
'

# Realign after .lit4 so that _edata and .bss are always 8 byte aligned,
# so that clearing bss can be fast on 64-bit CPUs.
OTHER_SDATA_SECTIONS="
  .lit8         ${RELOCATING-0} : { *(.lit8) }
  .lit4         ${RELOCATING-0} : { *(.lit4) }
  .srdata       ${RELOCATING-0} : { *(.srdata) }
  . = ALIGN (8);
"

# Add __edata and __end symbols
OTHER_BSS_SYMBOLS='PROVIDE (__edata = .);
  _fbss = .;'
OTHER_BSS_END_SYMBOLS='PROVIDE (__end = .);'

# XXX Should we do this stuff differently now, perhaps
# using sorted ctor entries.
# Retain for legacy code.
OTHER_READONLY_SECTIONS='
  .sdeinit        : { KEEP (*(.sdeinit)) }
  .sdefini        : { KEEP (*(.sdefini)) }
  .sdeosabi       : { KEEP (*(.sdeosabi)) }
'

OTHER_SECTIONS='
  .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }
  .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }
  .MIPS.options : { *(.MIPS.options) }
  /DISCARD/     : { *(.MIPS.content*) *(.MIPS.events*) }
'

# Do we still need to explicitly add .pdr?
#OTHER_SECTIONS='
#  .pdr 0 : { *(.pdr) }
#'

# Link crt0.o ctor/dtor sections before crtbegin.o
CTOR_START='KEEP (*crt0.o(.ctors))'
DTOR_START='KEEP (*crt0.o(.dtors))'

# Exclude crtn.o ctor/dtor sections from sort - must be last
OTHER_EXCLUDE_FILES='*crtn.o'

