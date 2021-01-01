#!/usr/bin/env python
'''
Generates a resource file (currently for XC32 only).
'''

from optparse import OptionParser
import sys
import os
import bm
import fdom
import decimal
import xmlindent
import re
import edc
import cmdlinetools
import domtools
import langcommon
import optionhelp

# Initialize the OptionParser.
_optionParser = OptionParser(
  usage='Usage: pic2res.py picfile1 picfile2 picfile3...\n\n%s' % (
    __doc__.strip()))
_optionParser.add_option(
'-f', '--no-flatten', dest='NoFlatten', action='store_true',
default=False, help='Skip the flattening step to save time.')
_optionParser.add_option(
'-v', '--version',  dest='g_compilerVersion', action='store',
default="0.00", help='Compiler version')
_optionParser.add_option(
'-r', '--revision', dest='g_partsRevision', action='store',
default="#", help='Part-support revision')

# Add a summary of command-line options to the module docstring for
# Sphinx.
__doc__ += optionhelp.makeOptionSummary(_optionParser)

g_bFlatten = True
g_compilerVersion="0.00"
g_partsRevision="#"
global outfile
global xcoutfile
global html_list
global gfirst_time
global gconfigdevlist

class HTMLConfigInfo(object):
  def __init__(self, name,  datasheet):
    self.name = name
    self.datasheet = datasheet

def compareNames(a,b):
  if a.name > b.name:
    return 1
  if a.name < b.name:
    return -1
  return 0

gconfigdevlist = []
gfirst_time = 1

def parseOptions():
  global g_bFlatten
  global g_compilerVersion
  global g_partsRevision

  options, args = _optionParser.parse_args()
  cmdlinetools.make_input_filename_list(args, _optionParser)

  if options.NoFlatten:
    g_bFlatten = False

  #Make sure mandatory options appearerd
  if options.g_compilerVersion is "0.00" or options.g_partsRevision is "#":
    print "Mandatory option misssing\n"
    _optionParser.print_help()
    exit(-1)

  g_compilerVersion =  options.g_compilerVersion
  g_partsRevision = options.g_partsRevision

def processSingleInputFile():
  inputFileName = cmdlinetools.long_filename
  print "."
  doc = domtools.parse(inputFileName)
  try:
    pic = edc.PIC(doc, flatten=g_bFlatten)
    buildresourcefile(pic)
  finally:
    domtools.unlink(doc)

  genxcmainheader()

def genxcmainheader():
  cwd=os.getcwd()
  path=os.path.join(cwd,'32Family')
  if not os.path.exists(path):
      os.mkdir(path)
  xcmainoutfile=open(os.path.join(path,"xc.h"), 'w')
  langcommon.print32HfileStartingInfo("XC", "XC", xcmainoutfile, "pic32")

  xcmainoutfile.write ("#if defined(__PIC32C)\n")
  xcmainoutfile.write ("#include <xc-pic32c.h>\n")
  xcmainoutfile.write ("#else /* defined(__PIC32M) */\n")
  xcmainoutfile.write ("#include <xc-pic32m.h>\n")
  xcmainoutfile.write ("#endif\n\n")

  xcmainoutfile.write ("#endif /* __XC_H */\n\n")
  xcmainoutfile.close ()

def buildresourcefile(pic):

  global gfirst_time
  global g_compilerVersion
  global g_partsRevision

# strip any leading "PIC" from the device name
  if pic.name[0:3] == "PIC":
    name = pic.name[3:]
  else :
    name = pic.name

  if gfirst_time:

    langcommon.print32HfileStartingInfo("XC", "XC_PIC32M", xcoutfile, "pic32")

    if g_compilerVersion != "0.00":
      xcoutfile.write("#ifndef __XC32_PART_SUPPORT_VERSION\n")
      xcoutfile.write("#define __XC32_PART_SUPPORT_VERSION %s\n" % str(int(1000 * decimal.Decimal(g_compilerVersion))))
      xcoutfile.write("#endif\n")
    if g_partsRevision != "#":
      xcoutfile.write("#ifndef __XC32_PART_SUPPORT_UPDATE\n")
      xcoutfile.write("#define __XC32_PART_SUPPORT_UPDATE %s\n" % g_partsRevision)
      xcoutfile.write("#endif\n\n")

    xcoutfile.write ("#if defined(__%s__)\n" % (name.upper()))
    xcoutfile.write ("#  include <proc/p%s.h>\n" % (name.lower()))

    # Create the generic device that looks like a PIC32MX795F512L
    outfile.write ("{\"32MXGENERIC\", IS_DEVICE_ID | P32MX | HAS_MIPS32R2 |  HAS_MIPS16, 2, 0x01AB}\n")

    gfirst_time = 0
  else:
    xcoutfile.write ("#elif defined(__%s__)\n" % (name.upper()))
    xcoutfile.write ("#  include <proc/p%s.h>\n" % (name.lower()))

  cinfo = HTMLConfigInfo(name, edc.get(pic.node, 'dsid'))
  gconfigdevlist.append (cinfo)
  if not g_bFlatten:
    return
  outfile.write ("{\"%s\"," % name.upper())

#  get processor id
  procid = edc.get(pic.node, 'procid')
  if not procid:
    procid="0x00"

  outfile.write (" IS_DEVICE_ID |")

#  get family type from name
#  For now, if the name does not start with "32",
#  then assume "32MZ" (for CPG's Ipswich)
  if (name[0:2] == "32"):
    familytype = name[0:4]
  else:
    familytype = "32MZ"

  outfile.write (" P%s" % familytype)

#  currently all devices have this
  if (pic.instructionSet.hasMIPS32r2 or re.search("32MX", familytype.upper())):
    outfile.write (" | HAS_MIPS32R2")

#  if has_mips16 (like 32MX)
  if (pic.instructionSet.hasMIPS16 or re.search("32MX", familytype.upper())):
    outfile.write (" | HAS_MIPS16")

  if pic.instructionSet.hasMicroMIPS :
    outfile.write (" | HAS_MICROMIPS")

  if pic.instructionSet.hasDSPr2:
    outfile.write (" | HAS_DSPR2")

  if pic.instructionSet.hasMCUASE:
    outfile.write (" | HAS_MCU")

#  if device uses L1Cache (like 32MZ)
  if pic.hasL1Cache :
    outfile.write (" | HAS_L1CACHE")

#  if device has vector offsets
  if pic.interrupts.hasVariableOffsets :
    outfile.write (" | HAS_VECTOROFFSETS")

  for nmmrid, SFRDef_node in sorted(pic.registerMap.idOntoCoreNMMR.items(), key=lambda pair:pair[0]):
    name = SFRDef_node.getAttribute("edc:cname")
    has_floating_point = False
    if name == "f0":
      has_floating_point = True
      break

  if has_floating_point :
    outfile.write (" | HAS_FLTPT")

#  if external memory bus
    #outfile.write (" | HAS_EXTMEM")

  # The dinit section should be allocated to serial mem and should contain code
  if re.search("32WK", familytype.upper()):
    outfile.write (" | HAS_DINIT_SERIALMEM")

#  get number of register sets [1,8]
  outfile.write(", %s" % pic.interrupts.shadowSetCount)

#  write processor id
  outfile.write (", %s" %  procid)
  outfile.write ("}\n")

def getlangboilerplate():
    xcoutfile.write ("#elif defined(__32M4KCORE__)\n")
    xcoutfile.write ("#  include <proc/p32m4kcore.h>\n")
    xcoutfile.write ("#elif defined(__32MXGENERIC__)\n")
    xcoutfile.write ("#  include <proc/p32mxgeneric.h>\n")
    xcoutfile.write ("#else\n")
    xcoutfile.write ("#  error Unknown processor!\n")
    xcoutfile.write ("#endif\n\n")

    xcoutfile.write ("#ifdef __LANGUAGE_ASSEMBLY__\n")
    xcoutfile.write ("/*\n")
    xcoutfile.write (" * Conventional register names.\n")
    xcoutfile.write (" */\n\n")
    xcoutfile.write ("/* Zero register:  Always returns 0 */\n")
    xcoutfile.write ("#define zero               $0\n\n")
    xcoutfile.write ("/* Assembler temporary register:  Reserved for use by the assembler */\n")
    xcoutfile.write ("#define at_reg             $1\n")
    xcoutfile.write ("#define AT_reg             $1\n\n")
    xcoutfile.write ("/* Return value registers:  Value returned by subroutine */\n")
    xcoutfile.write ("#define v0                 $2\n")
    xcoutfile.write ("#define v1                 $3\n\n")
    xcoutfile.write ("/* Argument registers:  First few parameters for a subroutine */\n")
    xcoutfile.write ("#define a0                 $4\n")
    xcoutfile.write ("#define a1                 $5\n")
    xcoutfile.write ("#define a2                 $6\n")
    xcoutfile.write ("#define a3                 $7\n\n")
    xcoutfile.write ("/* Subroutine register temporaries:  Registers that subroutines can use\n")
    xcoutfile.write (" * without saving\n")
    xcoutfile.write (" */\n")
    xcoutfile.write ("#define t0                 $8\n")
    xcoutfile.write ("#define t1                 $9\n")
    xcoutfile.write ("#define t2                 $10\n")
    xcoutfile.write ("#define t3                 $11\n")
    xcoutfile.write ("#define t4                 $12\n")
    xcoutfile.write ("#define t5                 $13\n")
    xcoutfile.write ("#define t6                 $14\n")
    xcoutfile.write ("#define t7                 $15\n")
    xcoutfile.write ("#define t8                 $24\n")
    xcoutfile.write ("#define t9                 $25\n\n")
    xcoutfile.write ("/* Subroutine register variables:  A subroutine that writes one of these\n")
    xcoutfile.write (" * must save the old value and restore it before it exists, so the calling\n")
    xcoutfile.write (" * routine sees the values preserved\n")
    xcoutfile.write (" */\n")
    xcoutfile.write ("#define s0                 $16\n")
    xcoutfile.write ("#define s1                 $17\n")
    xcoutfile.write ("#define s2                 $18\n")
    xcoutfile.write ("#define s3                 $19\n")
    xcoutfile.write ("#define s4                 $20\n")
    xcoutfile.write ("#define s5                 $21\n")
    xcoutfile.write ("#define s6                 $22\n")
    xcoutfile.write ("#define s7                 $23\n\n")
    xcoutfile.write ("/* Ninth subroutine register variable:  Can either be used as above or\n")
    xcoutfile.write (" * as the frame pointer for functions that require one\n")
    xcoutfile.write (" */\n")
    xcoutfile.write ("#define s8                 $30\n")
    xcoutfile.write ("#define fp                 $30\n\n")
    xcoutfile.write ("/* Interrupt/Trap handler registers:  Reserved for use by interrupt/trap\n")
    xcoutfile.write (" * handler.  May change under your feet!\n")
    xcoutfile.write (" */\n")
    xcoutfile.write ("#define k0                 $26\n")
    xcoutfile.write ("#define k1                 $27\n\n")
    xcoutfile.write ("/* Global pointer register:  Gives easy access to (some) `static' or `extern'\n")
    xcoutfile.write (" * variables\n")
    xcoutfile.write (" */\n")
    xcoutfile.write ("#define gp                 $28\n\n")
    xcoutfile.write ("/* Stack pointer register:  Stack pointer */\n")
    xcoutfile.write ("#define sp                 $29\n\n")
    xcoutfile.write ("/* Return address register:  Contains the return address for the subroutine */\n")
    xcoutfile.write ("#define ra                 $31\n")
    xcoutfile.write ("#endif\n\n")
    xcoutfile.write ("#if defined (__LANGUAGE_C__) || defined (__LANGUAGE_C_PLUS_PLUS__)\n\n")
    xcoutfile.write ("#if defined (__XC32_VERSION) && (__XC32_VERSION >= 2490)\n")
    xcoutfile.write ("#include <pic32m_builtins.h>\n")
    xcoutfile.write ("#endif\n\n")
    xcoutfile.write ("/*\n")
    xcoutfile.write (" * __XC_UART selects the default UART port that read() and write() will use.\n")
    xcoutfile.write (" * read() is called by fscanf and family, while write() is called by printf\n")
    xcoutfile.write (" * and family.\n")
    xcoutfile.write (" */\n")
    xcoutfile.write ("extern int __XC_UART;\n\n")
    xcoutfile.write ("/* \n")
    xcoutfile.write (" * When __DEBUG is defined, __debug_break() resolves to an MPLAB XC32 builtin \n")
    xcoutfile.write (" * function, which creates a MIPS32 software debug-breakpoint (sddbp 0) \n")
    xcoutfile.write (" * instruction.\n")
    xcoutfile.write (" */\n")
    xcoutfile.write ("/* #if defined(__DEBUG)\n")
    xcoutfile.write (" *  #define __debug_break() __builtin_software_breakpoint()\n")
    xcoutfile.write (" * #else\n")
    xcoutfile.write (" *  #define __debug_break() ((void)0)\n")
    xcoutfile.write (" * #endif\n")
    xcoutfile.write (" */\n\n")
    xcoutfile.write ("typedef unsigned long _reg_t;\n\n")
    xcoutfile.write ("/*\n")
    xcoutfile.write (" * Inline assembly macros\n")
    xcoutfile.write (" */\n\n")
    xcoutfile.write ("#ifndef _nop\n")
    xcoutfile.write ("#ifdef __mips16\n")
    xcoutfile.write ("#define _nop()  __asm__ __volatile__ (\" move $0,$16\" : :) \n")
    xcoutfile.write ("#else\n")
    xcoutfile.write ("#define _nop()  __asm__ __volatile__ (\"%(ssnop%)\" : :) \n")
    xcoutfile.write ("#endif\n")
    xcoutfile.write ("#endif\n\n")
    xcoutfile.write ("#ifndef Nop\n")
    xcoutfile.write ("/* Consider using _nop rather than Nop */\n")
    xcoutfile.write ("#define Nop()  _nop()\n")
    xcoutfile.write ("#endif\n\n")
    xcoutfile.write ("#ifndef _ehb\n")
    xcoutfile.write ("#define _ehb()  __asm__ __volatile__ (\"%(ehb%)\" : :) \n")
    xcoutfile.write ("#endif\n\n")
    xcoutfile.write ("#ifdef __mips16\n")
    xcoutfile.write ("extern void __attribute__((__nomips16__)) _sync(void);\n")
    xcoutfile.write ("#else\n")
    xcoutfile.write ("#define _sync() __asm__ __volatile__ (\"sync\" : : : \"memory\")\n")
    xcoutfile.write ("#endif\n\n")
    xcoutfile.write ("#define _wait() __asm__ __volatile__ (\"wait\")\n\n")

    xcoutfile.write ("#if defined(__GCC_HAVE_BUILTIN_MIPS_CACHE) && defined(__PIC32_HAS_L1CACHE)\n")
    xcoutfile.write ("#  define _cache(op,addr)      __builtin_mips_cache(op,addr)\n")
    xcoutfile.write ("#else /* _cache() */\n")
    xcoutfile.write ("#  define _cache(op,addr)      _Pragma(\"message (\\\"Warning - Using _cache(op,addr) on an uncached target device\\\")\")\n")
    xcoutfile.write ("#endif /* _cache */\n\n")

    xcoutfile.write ("#if defined(__PIC32_HAS_L1CACHE)\n")
    xcoutfile.write ("#  define _synci(x) __extension__({ \\\n")
    xcoutfile.write ("       __asm__ (\"synci %a0\" :: \"d\" (x)); \\\n")
    xcoutfile.write ("   })\n")
    xcoutfile.write ("#else /* _synci */\n")
    xcoutfile.write ("#  define _synci(addr)         _Pragma(\"message (\\\"Warning - Using _synci(addr) on an uncached target device\\\")\")\n")
    xcoutfile.write ("#endif /* _synci */\n\n")

    xcoutfile.write ("#if defined(__PIC32_HAS_L1CACHE)\n")
    xcoutfile.write ("#  define _prefetch(hint,x) __extension__({ \\\n")
    xcoutfile.write ("      __asm__ (\"pref %0, %a1\" :: \"JYA\" (hint), \"d\" (x) : \"memory\"); \\\n")
    xcoutfile.write ("   })\n")
    xcoutfile.write ("#else /* _prefetch */\n")
    xcoutfile.write ("#  define _prefetch(hint,addr) _Pragma(\"message (\\\"Warning - Using _prefetch(hint,addr) on an uncached target device\\\")\")\n")
    xcoutfile.write ("#endif /* _prefetch */\n\n")

    xcoutfile.write ("#define _mfc0(rn, sel)            __builtin_mfc0(rn,sel)\n")
    xcoutfile.write ("#define _mtc0(rn, sel, v)         __builtin_mtc0(rn,sel,v)\n")
    xcoutfile.write ("/* exchange (swap) VAL and CP0 register REG,SEL */\n")
    xcoutfile.write ("#define _mxc0(rn, sel, v)         __builtin_mxc0(rn,sel,v)\n")
    xcoutfile.write ("/* bit clear non-zero bits from CLR in CP0 register REG,SEL */\n")
    xcoutfile.write ("#define _bcc0(rn, sel, clr)       __builtin_bcc0(rn,sel,clr)\n")
    xcoutfile.write ("/* bit set non-zero bits from SET in CP0 register REG,SEL */\n")
    xcoutfile.write ("#define _bsc0(rn, sel, set)       __builtin_bsc0(rn,sel,set)\n")
    xcoutfile.write ("/* bit clear non-zero bits from CLR and set non-zero bits from SET in REG,SEL */\n")
    xcoutfile.write ("#define _bcsc0(rn, sel, clr, set) __builtin_bcsc0(rn,sel,clr,set)\n\n")

    xcoutfile.write ("/* count leading zeroes */\n")
    xcoutfile.write ("#define _clz(x) __builtin_clz (x)\n\n")
    xcoutfile.write ("/* count trailing zeroes */\n")
    xcoutfile.write ("#define _ctz(x) __builtin_ctz (x)\n\n")

    xcoutfile.write ("#if ! __mips16\n")
    xcoutfile.write ("/* count leading ones */\n")
    xcoutfile.write ("#define _clo(x) __extension__({ \\\n")
    xcoutfile.write ("    unsigned int __x = (x); \\\n")
    xcoutfile.write ("    unsigned int __v; \\\n")
    xcoutfile.write ("    __asm__ (\"clo %0,%1\" : \"=d\" (__v) : \"d\" (__x)); \\\n")
    xcoutfile.write ("    __v; \\\n")
    xcoutfile.write ("})\n\n")
    xcoutfile.write ("/* Simulate 64-bit count leading zeroes */\n")
    xcoutfile.write ("#define _dclz(x) __extension__({ \\\n")
    xcoutfile.write ("    unsigned long long __x = (x); \\\n")
    xcoutfile.write ("    unsigned int __hx = (__x >> 32); \\\n")
    xcoutfile.write ("    __hx ? _clz(__hx) : 32 + _clz(__x); \\\n")
    xcoutfile.write ("})\n\n")
    xcoutfile.write ("/* Simulate 64-bit count leading ones */\n")
    xcoutfile.write ("#define _dclo(x) __extension__({ \\\n")
    xcoutfile.write ("    unsigned long long __llx = (x); \\\n")
    xcoutfile.write ("    unsigned int __hx = (__llx >> 32); \\\n")
    xcoutfile.write ("    (~__hx) ? _clo(__hx) : 32 + _clo(__llx); \\\n")
    xcoutfile.write ("})\n\n")
    xcoutfile.write ("/* Simulate 64-bit count trailing zeroes */\n")
    xcoutfile.write ("#define _dctz(x) __extension__({ \\\n")
    xcoutfile.write ("    unsigned long long __dx = (x); \\\n")
    xcoutfile.write ("    unsigned int __ldx = __dx; \\\n")
    xcoutfile.write ("    unsigned int __hdx = __dx >> 32; \\\n")
    xcoutfile.write ("    __ldx ? _ctz(__ldx) : (63 ^ _clz(__hdx & -__hdx)); \\\n")
    xcoutfile.write ("   })\n\n")
    xcoutfile.write ("/* MIPS32r2 wsbh opcode */\n")
    xcoutfile.write ("#define _wsbh(x) __extension__({ \\\n")
    xcoutfile.write ("    unsigned int __x = (x), __v; \\\n")
    xcoutfile.write ("    __asm__ (\"wsbh %0,%1\" \\\n")
    xcoutfile.write ("             : \"=d\" (__v) \\\n")
    xcoutfile.write ("             : \"d\" (__x)); \\\n")
    xcoutfile.write ("    __v; \\\n")
    xcoutfile.write ("})\n\n")
    xcoutfile.write ("/* MIPS32r2 byte-swap word */\n")
    xcoutfile.write ("#define _bswapw(x) __extension__({ \\\n")
    xcoutfile.write ("    unsigned int __x = (x), __v; \\\n")
    xcoutfile.write ("    __asm__ (\"wsbh %0,%1; rotr %0,16\" \\\n")
    xcoutfile.write ("             : \"=d\" (__v) \\\n")
    xcoutfile.write ("             : \"d\" (__x)); \\\n")
    xcoutfile.write ("    __v; \\\n")
    xcoutfile.write ("})\n\n")
    xcoutfile.write ("/* MIPS32r2 insert bits */\n")
    xcoutfile.write ("#define _ins(tgt,val,pos,sz) __extension__({ \\\n")
    xcoutfile.write ("    unsigned int __t = (tgt), __v = (val); \\\n")
    xcoutfile.write ("    __asm__ (\"ins %0,%z1,%2,%3\" \\\n")
    xcoutfile.write ("             : \"+d\" (__t) \\\n")
    xcoutfile.write ("             : \"dJ\" (__v), \"I\" (pos), \"I\" (sz)); \\\n")
    xcoutfile.write ("    __t; \\\n")
    xcoutfile.write ("})\n\n")
    xcoutfile.write ("/* MIPS32r2 extract bits */\n")
    xcoutfile.write ("#define _ext(x,pos,sz) __extension__({ \\\n")
    xcoutfile.write ("    unsigned int __x = (x), __v; \\\n")
    xcoutfile.write ("    __asm__ (\"ext %0,%z1,%2,%3\" \\\n")
    xcoutfile.write ("             : \"=d\" (__v) \\\n")
    xcoutfile.write ("             : \"dJ\" (__x), \"I\" (pos), \"I\" (sz)); \\\n")
    xcoutfile.write ("    __v; \\\n")
    xcoutfile.write ("})\n\n")
    xcoutfile.write ("/* MIPS32r2 jr.hb */\n")
    xcoutfile.write ("#define _jr_hb() __asm__ __volatile__( \\\n")
    xcoutfile.write ("       \"bltzal  $0,0f\\n\"               \\\n")
    xcoutfile.write ("\"0:     addiu   $31,1f-0b\\n\"           \\\n")
    xcoutfile.write ("\"       jr.hb   $31\\n\"                 \\\n")
    xcoutfile.write ("\"1:\"                                   \\\n")
    xcoutfile.write ("        : : : \"$31\")\n\n")
    xcoutfile.write ("/* MIPS32r2 write previous gpr */\n")
    xcoutfile.write ("#define _wrpgpr(regno, val) \\\n")
    xcoutfile.write ("do { \\\n")
    xcoutfile.write ("    __asm __volatile__ (\"wrpgpr $%0,%z1\" \\\n")
    xcoutfile.write ("                        : /* no outputs */ \\\n")
    xcoutfile.write ("                        : \"JK\" (regno), \"dJ\" (val)); \\\n")
    xcoutfile.write ("} while (0)\n\n")
    xcoutfile.write ("/* MIPS32r2 read previous gpr */\n")
    xcoutfile.write ("#define _rdpgpr(regno) \\\n")
    xcoutfile.write ("__extension__({ \\\n")
    xcoutfile.write ("    _reg_t __val; \\\n")
    xcoutfile.write ("    __asm __volatile__ (\"rdpgpr %0,$%1\" \\\n")
    xcoutfile.write ("                        : \"=d\" (__val) \\\n")
    xcoutfile.write ("                        : \"JK\" (regno)); \\\n")
    xcoutfile.write ("    __val; \\\n")
    xcoutfile.write ("})\n\n")
    xcoutfile.write ("/* MIPS32r2 set SRSCtl.PSS (previous shadow set), returning old value */\n")
    xcoutfile.write ("extern unsigned int _xchsrspss (unsigned int);\n")
    xcoutfile.write ("#endif\n\n")
    xcoutfile.write ("#define _get_byte(addr, errp)   (*(volatile unsigned char *)(addr))\n")
    xcoutfile.write ("#define _get_half(addr, errp)   (*(volatile unsigned short *)(addr))\n")
    xcoutfile.write ("#define _get_word(addr, errp)   (*(volatile unsigned int *)(addr))\n")
    xcoutfile.write ("#define _get_dword(addr, errp)  (*(volatile unsigned long long *)(addr))\n\n")
    xcoutfile.write ("#define _put_byte(addr, v)      (*(volatile unsigned char *)(addr)=(v), 0)\n")
    xcoutfile.write ("#define _put_half(addr, v)      (*(volatile unsigned short *)(addr)=(v), 0)\n")
    xcoutfile.write ("#define _put_word(addr, v)      (*(volatile unsigned int *)(addr)=(v), 0)\n")
    xcoutfile.write ("#define _put_dword(addr, v)     (*(volatile unsigned long long *)(addr)=(v), 0)\n")
    xcoutfile.write ("#endif\n\n")
    xcoutfile.write ("#include <cp0defs.h>\n\n")
    xcoutfile.write ("#if defined (__LANGUAGE_C__) || defined (__LANGUAGE_C_PLUS_PLUS__)\n")
    xcoutfile.write ("#  include <sys/appio.h>\n")
    xcoutfile.write ("#  include <sys/l1cache.h>\n")
    xcoutfile.write ("#endif\n\n")
    xcoutfile.write ("#endif\n")

def processnames():
  gconfigdevlist.sort(compareNames)
  htmlfile.write("<tr>  ")
  ct = 0
  columns = 6
  DS_Dict={'61185':'PIC32MX330/350/370/430/450/470', '61143':'PIC32MX320/340/360/420/440/460', \
    '61156':'PIC32MX5XX/6XX/7XX', '61168':'PIC32MX1XX/2XX'}

  # Print devices grouped by family
  uniqueds = list(set([n.name[:4] for n in gconfigdevlist]))
  for family in uniqueds:
    if family.startswith('32'):
      htmlfile.write ("<h3>The following devices from the <b>%s</b> family are supported:</h3>\n" % family)
    else:
      htmlfile.write ("<h3>The following devices from the <b>%s</b> family are supported:</h3>\n" % family[:3])
    htmlfile.write ("      <div class=\"SupportedDevices\">\n");
    htmlfile.write ("<table summary=\"Supported %s devices\">\n" % family)
    htmlfile.write("<tr>  ")
    ct = 0
    for n in gconfigdevlist:
      if not (family == n.name[:4]):
        continue
      ct = ct + 1
      htmlfile.write("<td><A href=\"./config_docs/%s.html\">  %s</A></td>" % (n.name.lower(), n.name))
      if not (ct % columns):
        htmlfile.write("</tr>\n")
    if ct > columns:
      for i in range(0, columns - (ct % columns)):
        htmlfile.write("<td></td>")
    htmlfile.write("</tr>\n")
    htmlfile.write("</table></div>\n<br>\n")

def supportlist():
  gconfigdevlist.sort(compareNames)
  ct = 0
  columns = 6
  DS_Dict={'61185':'PIC32MX330/350/370/430/450/470', '61143':'PIC32MX320/340/360/420/440/460', \
    '61156':'PIC32MX5XX/6XX/7XX', '61168':'PIC32MX1XX/2XX'}

  # Print devices grouped by family
  uniqueds = list(set([n.name[:4] for n in gconfigdevlist]))

  htmlfile.write("\n\n<!-- Hidden Table with links to online data sheet. Useful for inclusing in README.html \n")
  for family in uniqueds:
    if family.startswith('32'):
      htmlfile.write ("<h3>The following devices from the <b>%s</b> family are supported:</h3>\n" % family)
    else:
      htmlfile.write ("<h3>The following devices from the <b>%s</b> family are supported:</h3>\n" % family[:3])
    htmlfile.write ("      <div class=\"SupportedDevices\">\n");
    htmlfile.write ("      <table summary=\"Supported %s devices\">\n" % family)
    htmlfile.write("<tr>  ")
    ct = 0
    for n in gconfigdevlist:
      if not (family == n.name[:4]):
    	continue
      ct = ct + 1
      if family.startswith('32'):
         htmlfile.write("<td><A href=\"http://www.microchip.com/wwwproducts/Devices.aspx?Product=PIC%s\">  %s</A></td>" % (n.name.lower(), n.name))
      else:
         htmlfile.write("<td><A href=\"http://www.microchip.com/wwwproducts/Devices.aspx?Product=%s\">  %s</A></td>" % (n.name.lower(), n.name))
      if not (ct % columns):
    	htmlfile.write("</tr>\n")
    if ct > columns:
      for i in range(0, columns - (ct % columns)):
    	htmlfile.write("<td></td>")
    htmlfile.write("</tr>\n")
    htmlfile.write("</table></div>\n<br>\n")
  htmlfile.write("<--\n\n\n")


def OpenResFile() :

  global outfile
  global xcoutfile
  global htmlfile

  cwd=os.getcwd()
  path=os.path.join(cwd,'32Family')
  if not os.path.exists(path):
     os.mkdir(path)

  outfile=open(os.path.join(path,"xc32_DVSdevices.res"), 'w')
  xcoutfile=open(os.path.join(path,"xc-pic32m.h"), 'w')
  htmlfile=open(os.path.join(path,"PIC32ConfigSet.html"), 'w')

  htmlfile.write ("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
  htmlfile.write ("<html>\n<head>\n");

  htmlfile.write ("    <style type=\"text/css\">\n");
  htmlfile.write (".SupportedDevices {\n");
  htmlfile.write ("        margin:0px;padding:0px;\n");
  htmlfile.write ("        width:90%;\n");
  htmlfile.write ("        box-shadow: 10px 10px 5px #888888;\n");
  htmlfile.write ("        border:1px solid #000000;\n");
  htmlfile.write ("        \n");
  htmlfile.write ("        -moz-border-radius-bottomleft:1px;\n");
  htmlfile.write ("        -webkit-border-bottom-left-radius:1px;\n");
  htmlfile.write ("        border-bottom-left-radius:1px;\n");
  htmlfile.write ("        \n");
  htmlfile.write ("        -moz-border-radius-bottomright:1px;\n");
  htmlfile.write ("        -webkit-border-bottom-right-radius:1px;\n");
  htmlfile.write ("        border-bottom-right-radius:1px;\n");
  htmlfile.write ("        \n");
  htmlfile.write ("        -moz-border-radius-topright:1px;\n");
  htmlfile.write ("        -webkit-border-top-right-radius:1px;\n");
  htmlfile.write ("        border-top-right-radius:1px;\n");
  htmlfile.write ("        \n");
  htmlfile.write ("        -moz-border-radius-topleft:1px;\n");
  htmlfile.write ("        -webkit-border-top-left-radius:1px;\n");
  htmlfile.write ("        border-top-left-radius:1px;\n");
  htmlfile.write ("  }.SupportedDevices table{\n");
  htmlfile.write ("        width:100%;\n");
  htmlfile.write ("        \n");
  htmlfile.write ("        margin:0px;padding:0px;\n");
  htmlfile.write ("  }.SupportedDevices tr:last-child td:last-child {\n");
  htmlfile.write ("        -moz-border-radius-bottomright:1px;\n");
  htmlfile.write ("        -webkit-border-bottom-right-radius:1px;\n");
  htmlfile.write ("        border-bottom-right-radius:1px;\n");
  htmlfile.write ("  }\n");
  htmlfile.write ("  .SupportedDevices table tr:first-child td:first-child {\n");
  htmlfile.write ("        -moz-border-radius-topleft:1px;\n");
  htmlfile.write ("        -webkit-border-top-left-radius:1px;\n");
  htmlfile.write ("        border-top-left-radius:1px;\n");
  htmlfile.write ("  }\n");
  htmlfile.write ("  .SupportedDevices table tr:first-child td:last-child {\n");
  htmlfile.write ("        -moz-border-radius-topright:1px;\n");
  htmlfile.write ("        -webkit-border-top-right-radius:1px;\n");
  htmlfile.write ("        border-top-right-radius:1px;\n");
  htmlfile.write ("  }.SupportedDevices tr:last-child td:first-child{\n");
  htmlfile.write ("        -moz-border-radius-bottomleft:1px;\n");
  htmlfile.write ("        -webkit-border-bottom-left-radius:1px;\n");
  htmlfile.write ("        border-bottom-left-radius:1px;\n");
  htmlfile.write ("  }.SupportedDevices tr:hover td{\n");
  htmlfile.write ("        \n");
  htmlfile.write ("  }\n");
  htmlfile.write ("  .SupportedDevices tr:nth-child(odd){ background-color:#e5e5e5; }\n");
  htmlfile.write ("  .SupportedDevices tr:nth-child(even)    { background-color:#ffffff; }.SupportedDevices td{\n");
  htmlfile.write ("        vertical-align:middle;\n");
  htmlfile.write ("        \n");
  htmlfile.write ("        border:1px solid #000000;\n");
  htmlfile.write ("        border-width:0px 1px 1px 0px;\n");
  htmlfile.write ("        text-align:left;\n");
  htmlfile.write ("        padding:3px;\n");
  htmlfile.write ("        font-size:14px;\n");
  htmlfile.write ("        font-family:Courier;\n");
  htmlfile.write ("        font-weight:normal;\n");
  htmlfile.write ("        color:#000000;\n");
  htmlfile.write ("  }.SupportedDevices tr:last-child td{\n");
  htmlfile.write ("        border-width:0px 1px 0px 0px;\n");
  htmlfile.write ("  }.SupportedDevices tr td:last-child{\n");
  htmlfile.write ("        border-width:0px 0px 1px 0px;\n");
  htmlfile.write ("  }.SupportedDevices tr:last-child td:last-child{\n");
  htmlfile.write ("        border-width:0px 0px 0px 0px;\n");
  htmlfile.write ("  }\n");
  htmlfile.write ("  .SupportedDevices tr:first-child td{\n");
  htmlfile.write ("\n");
  htmlfile.write ("        border:1px solid #000000;\n");
  htmlfile.write ("        border-width:0px 1px 1px 0px;\n");
  htmlfile.write ("        text-align:left;\n");
  htmlfile.write ("        padding:3px;\n");
  htmlfile.write ("        font-size:14px;\n");
  htmlfile.write ("        font-family:Courier;\n");
  htmlfile.write ("        font-weight:normal;\n");
  htmlfile.write ("        color:#000000;\n");
  htmlfile.write ("  }\n");
  htmlfile.write ("  .SupportedDevices tr:first-child:hover td{\n");
  htmlfile.write ("  }\n");
  htmlfile.write ("  .SupportedDevices tr:first-child td:first-child{\n");
  htmlfile.write ("        border-width:0px 0px 1px 0px;\n");
  htmlfile.write ("  }\n");
  htmlfile.write ("  .SupportedDevices tr:first-child td:last-child{\n");
  htmlfile.write ("        border-width:0px 0px 1px 1px;\n");
  htmlfile.write ("  }\n");
  htmlfile.write ("  </style>\n");

  htmlfile.write ("<title>Device Index for Config-Bit Setting</title>")
  htmlfile.write ("</head>\n");
  htmlfile.write ("<body bgcolor=\"#FFFFE0\" style=\"font-family: sans-serif\">\n")
  htmlfile.write ("<p>[ <a href=\"./XC32MasterIndex.htm\">Documentation Index</a> ]</p>\n")
  htmlfile.write ("<h1>MPLAB<font size=\"-3\"><sup>&reg;</sup></font> XC32 Supported devices</h1>\n\n")

def CloseResFile() :

  outfile.close ()
  getlangboilerplate()
  xcoutfile.close ()
  processnames()
  supportlist()

  htmlfile.write ("</body>\n")
  htmlfile.write ("</html>\n\n")

  htmlfile.close ()


if __name__ == '__main__':
  cmdlinetools.main(
    parseOptions, processSingleInputFile,
    preprocess=OpenResFile, postprocess=CloseResFile)
