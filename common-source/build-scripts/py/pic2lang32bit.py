#!/usr/bin/env python

'''
'pic2lang32bit.py' generates language tool part support for 32-bit devices.

'''

from optparse import OptionParser

import sys
import os
import fdom
import showdom
import langcommon
import edc
import cmdlinetools
import domtools
import re
import time
import optionhelp

# Following required for some windows machines
import codecs
codecs.register(lambda name: codecs.lookup('utf-8') if name == 'cp65001' else None)

# Initialize the OptionParser.
_optionParser = OptionParser(
    usage='Usage: pic2lang32bit.py picfile1 picfile2 picfile3...\n\n%s' % (
        __doc__.strip()))

# Add a summary of command-line options to the module docstring for
# Sphinx.
__doc__ += optionhelp.makeOptionSummary(_optionParser)

def countVectors(pic):
    '''Return the highest IRQ number.'''

    maxVectorNumber = 0
    InterruptList_node = fdom.first(pic.node, 'InterruptList')
    if len(fdom.children(InterruptList_node, 'Interrupt')) > 0:
        for each in fdom.children(InterruptList_node, 'Interrupt'):
            irq=fdom.get2(each, 'irq')
            if (int(irq) > maxVectorNumber):
                maxVectorNumber = int(irq)
    return maxVectorNumber

class Bunch:
    def __init__(self, **kwds):
        self.__dict__.update(kwds)

DefaultMemRegions = ""
namewithoutpic = ""
bootksegsize = 0
bootiswritable = "true"
progsize = 0
_BEV_EXCPT_ADDR = ""
gen_ldforpic = False
g_FlattenPIC = True

def parseOptions():
    '''Parse the command-line options (callback function).'''
    global gen_ldforpic
    global g_FlattenPIC
    gen_ldforpic = False
    g_FlattenPIC = True

    _optionParser.add_option(
        '-p', '--pic', dest='ldforpic', action='store_true', default=False,
        help='Emit linker script file for PIE.')
    _optionParser.add_option(
        '-f', '--no-flatten', dest='NoFlatten', action='store_true',
        default=False, help='Skip the flattening step to save time.')

    options, args = _optionParser.parse_args()
    if options.ldforpic :
        gen_ldforpic = True
    if options.NoFlatten:
        g_FlattenPIC = False
    cmdlinetools.make_input_filename_list(args, _optionParser)

def processSingleInputFile():
    '''Process a single input \*.PIC file (callback function).'''

    translatePICFile(cmdlinetools.long_filename)

def translatePICFile(path):
    '''Generate XC32 compiler artifacts from a path to a \*.PIC file.'''

    doc = None
    try:
        pic = edc.PIC(path,flatten=g_FlattenPIC)
        doc = pic.doc
        translatePIC(pic)
    finally:
        domtools.unlink(doc)

def translatePIC(pic):
    '''Generate XC32 compiler artifacts from an edc.PIC object.'''

    global DefaultMemRegions
    global namewithoutpic
    global bootksegsize
    global bootiswritable
    global progsize
    global _BEV_EXCPT_ADDR
    global progmemories
    global datamemories
    try:
        printThings(pic)
        printSupportFiles(pic)
    finally:
        DefaultMemRegions = ""
        namewithoutpic = ""
        bootksegsize = 0
        progsize = 0
        _BEV_EXCPT_ADDR = ""
        progmemories = []
        datamemories = []

        # If we don't reset these objects they (or some of them)
        # continue to grow with each call to this function.
        langcommon.alias = []
        langcommon.cnamesList = {}
        langcommon.orderCnames = []
        langcommon.count = 0
        langcommon.bitsList = {}
        langcommon.RegandBitnames = {}
        langcommon.orderList = []
        langcommon.cnameDict = {}
        langcommon.trackAddrs = {}
        langcommon.widthGreaterthanOne = {}
        langcommon.modesDict = {}
        langcommon.base_addresses = {}
        langcommon.bit_ListOrder = []


def printThings(pic):
    del langcommon.orderCnames[:]
    shadowSFRS={}
    allSFRS={}
    cnameList=[]
    modeList=[]
    allCnames={}
    picname=pic.name
    family=pic.arch
    hasAppMemRegions=False
    global namewithoutpic
    global DefaultMemRegions
    namewithoutpic=picname.replace("PIC32", "32").upper()
    original_namewithoutpic=namewithoutpic
    cwd=os.getcwd()
    path=os.path.join(cwd,'32Family', namewithoutpic)
    if not os.path.exists(path):
        os.makedirs(path)

    # Some devices, such as the WFI32*, are modules that use another device
    Properties_node = fdom.first(pic.node, 'Properties')
    for each in fdom.children(Properties_node, 'Property'):
        if fdom.get2(each, 'propkey') == "device.is.copy.of":
            namewithoutpic=fdom.get2(each, 'propvalue').replace("PIC32", "32").upper()

    # For PIC32WK (Imola), the application entry point is in RAM rather than boot
    # flash.
    if "32WK" in namewithoutpic :
        DefaultMemRegions = Bunch(data="kseg0_data_mem", reset="kseg0_program_mem", startup="kseg0_program_mem", debug="kseg0_program_mem")
    else :
        DefaultMemRegions = Bunch(data="kseg1_data_mem", reset="kseg1_boot_mem", startup="kseg0_boot_mem", debug="debug_exec_mem")

    getmemregions (pic)

    with open(os.path.join(path,'p'+original_namewithoutpic.lower()+'.h'),"w") as outfile:
        print namewithoutpic
        langcommon.print32HfileStartingInfo(picname, namewithoutpic, outfile,family)
        # This part of the header file is for C/C++ only.
        outfile.write ("#if defined (__LANGUAGE_C__) || defined (__LANGUAGE_C_PLUS_PLUS)\n\n")
        outfile.write ("#include <stdint.h>\n\n")
        outfile.write ("#ifdef __cplusplus\n")
        outfile.write ("extern \"C\" {\n")
        outfile.write ("#endif\n\n")
        
        # Application-specific memory regions
        for memory in datamemories :
            if (memory.name.upper().find("CRYPTO") == 0) or (memory.name.upper().find("WCM") == 0):
                hasAppMemRegions=True
                continue
        if hasAppMemRegions:
            outfile.write ("/* Application-Defined Memory Regions */\n")
            for memory in datamemories :
                if (memory.name.upper().find("CRYPTO") == 0) or (memory.name.upper().find("WCM") == 0):
                    outfile.write ("#pragma region name=\"%s\" origin=0x%-8X size=0x%X\n" % (memory.name, memory.origin, memory.length))
            outfile.write ("\n")

        # Special Function Registers
        shadowSFRS,allSFRS=langcommon.allSFRS(pic,shadowSFRS, allSFRS)
        type="SFR"
        entry="first"
        for addr,SFRDef_node in sorted(allSFRS.items(), key=lambda pair:pair[0]):
            cname=fdom.get2(SFRDef_node, 'cname')
            if cname in ['OSCCON','C1EC','C2EC','CRCDAT32']:
                allCnames,registerSize=langcommon.addCnames(cname,SFRDef_node,addr)
                if allCnames is not None:
                    modeList,SFRModeList_node=langcommon.countModes(SFRDef_node,registerSize,allCnames,outfile,cname,type,family)
                    langcommon.printStructUnion(modeList,SFRModeList_node,outfile,0,registerSize,allCnames,type,family,entry)
                    if cname=='C1EC':
                        outfile.write("extern volatile unsigned char %s __attribute__((__sfr__));\n" % ( 'C1RERRCNT'))
                        outfile.write("extern volatile unsigned char %s __attribute__((__sfr__));\n" % ( 'C1TERRCNT'))
                    if cname=='C2EC':
                        outfile.write("extern volatile unsigned char %s __attribute__((__sfr__));\n" % ( 'C2RERRCNT'))
                        outfile.write("extern volatile unsigned char %s __attribute__((__sfr__));\n" % ( 'C2TERRCNT'))
            else:
                is_muxed = False
                for addr_muxed, MuxedSFRDef_list in pic.mainPartition.registerMap.addrOntoMuxedSFRs.iteritems():
                    if addr_muxed == addr:
                        is_muxed = True
                        break
                if not is_muxed:
                  allCnames, registerSize = langcommon.addCnames(cname,SFRDef_node,addr)
                  if allCnames is not None:
                      modeList, SFRModeList_node = langcommon.countModes(SFRDef_node,registerSize,allCnames,outfile,cname,type,family)
                      langcommon.printStructUnion(modeList,SFRModeList_node,outfile,0,registerSize,allCnames,type,family,entry)
            entry=""
            del cnameList[:]
        for addr, MuxedSFRDef_list in pic.mainPartition.registerMap.addrOntoMuxedSFRs.iteritems():
          for MuxedSFRDef_node in MuxedSFRDef_list:
              # allSFRS.update({addr:MuxedSFRDef_node})
              cname=fdom.get2(MuxedSFRDef_node, 'cname')
              allCnames, registerSize = langcommon.addCnames(cname,MuxedSFRDef_node,addr)
              if allCnames is not None:
                  modeList, SFRModeList_node = langcommon.countModes(MuxedSFRDef_node,registerSize,allCnames,outfile,cname,type,family)
                  langcommon.printStructUnion(modeList,SFRModeList_node,outfile,0,registerSize,allCnames,type,family,entry)
              entry=""
              del cnameList[:]
        type="DCR"
        print "DCRs ..."
        for addr, DCRDef_node in sorted(pic.addrOntoDCR.iteritems(), key=lambda pair:pair[0]):
            cname = fdom.get2(DCRDef_node, 'cname')
            allCnames,registerSize=langcommon.addCnames(cname,DCRDef_node,addr)
            if allCnames is not None:
                modeList,SFRModeList_node=langcommon.countModes(DCRDef_node,registerSize,allCnames,outfile,cname,type,family)
                langcommon.printStructUnion(modeList,SFRModeList_node,outfile,0,registerSize,allCnames,type,family,"")
        for addr, DCRDef_node in sorted(pic.addrOntoAltDCR.iteritems(), key=lambda pair:pair[0]):
            cname = fdom.get2(DCRDef_node, 'cname')
            allCnames,registerSize=langcommon.addCnames(cname,DCRDef_node,addr)
            if allCnames is not None:
                modeList,SFRModeList_node=langcommon.countModes(DCRDef_node,registerSize,allCnames,outfile,cname,type,family)
                langcommon.printStructUnion(modeList,SFRModeList_node,outfile,0,registerSize,allCnames,type,family,"")

        outfile.write ("#ifdef __cplusplus\n")
        outfile.write ("}\n")
        outfile.write ("#endif\n\n")
        print "Writing Externs. This may take a bit..."
        langcommon.printExterns(outfile)
        if family=="32xxxx":
            print "Writing vectors"
            langcommon.printVectors(pic,outfile)
            outfile.write ("\n")

            outfile.write ("/* Default Memory-region macros */\n")
            flash_size_kb = 0
            for memory in progmemories :
                if (memory.length > 4) :
                    basename = "__%s_BASE" % memory.name.upper().split()[0]
                    lengthname = "__%s_LENGTH" % memory.name.upper().split()[0]
                    outfile.write ("#define %-40s 0x%-8X\n" % (basename, memory.origin))
                    outfile.write ("#define %-40s 0x%X\n" % (lengthname, memory.length))
                    if (memory.name.upper().split()[0] == "KSEG0_PROGRAM_MEM") :
                        flash_size_kb = memory.length / 1024
            for memory in datamemories :
                if (memory.length > 4) :
                    basename = "__%s_BASE" % memory.name.upper().split()[0]
                    lengthname = "__%s_LENGTH" % memory.name.upper().split()[0]
                    outfile.write ("#define %-40s 0x%-8X\n" % (basename, memory.origin))
                    outfile.write ("#define %-40s 0x%X\n" % (lengthname, memory.length))

            outfile.write ("\n/*  The following device macros are predefined by the MPLAB XC32\n")
            outfile.write (" *  compiler when compiling with the -mprocessor=<device> option.\n")
            outfile.write (" *  We also define them here to help the MPLAB X editor evaluate\n")
            outfile.write (" *  them correctly.\n")
            outfile.write (" */\n")

            outfile.write ("#ifndef __%s\n" % namewithoutpic.upper())
            outfile.write ("#  define __%s 1\n" % namewithoutpic.upper())
            outfile.write ("#endif\n")
            outfile.write ("#ifndef __%s__\n" % namewithoutpic.upper())
            outfile.write ("#  define __%s__ 1\n" % namewithoutpic.upper())
            outfile.write ("#endif\n")
            outfile.write ("#ifndef __XC__\n")
            outfile.write ("#  define __XC__ 1\n")
            outfile.write ("#endif\n")
            outfile.write ("#ifndef __XC\n")
            outfile.write ("#  define __XC 1\n")
            outfile.write ("#endif\n")
            outfile.write ("#ifndef __XC32__\n")
            outfile.write ("#  define __XC32__ 1\n")
            outfile.write ("#endif\n")
            outfile.write ("#ifndef __XC32\n")
            outfile.write ("#  define __XC32 1\n")
            outfile.write ("#endif\n")


            if ("32MZ" in namewithoutpic):
                outfile.write ("#ifndef __PIC32MZ\n")
                outfile.write ("#  define __PIC32MZ 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __PIC32MZ__\n")
                outfile.write ("#  define __PIC32MZ__ 1\n")
                outfile.write ("#endif\n")
                macro = re.match(r"32MZ(?P<zeros1>[0]*)(?P<flash_size>[0-9]+)(?P<feature_set>[A-Z]{2,})(?P<product_group>[A-Z]{1})(?P<zeros2>[0]*)(?P<pin_count>[0-9]+)\Z", namewithoutpic.upper())
                if macro :
                    if ((flash_size_kb > 0) and (flash_size_kb != (int)(macro.group('flash_size')))) :
                        outfile.write ("#undef __PIC32_FLASH_SIZE\n")
                        outfile.write ("#define __PIC32_FLASH_SIZE %s\n" % flash_size_kb)
                    else :
                        outfile.write ("#ifndef __PIC32_FLASH_SIZE\n")
                        outfile.write ("#  define __PIC32_FLASH_SIZE %d\n" % (int(macro.group('flash_size'))))
                        outfile.write ("#endif\n")
                    feature_set = macro.group('feature_set')
                    outfile.write ("#ifndef __PIC32_FEATURE_SET\n")
                    outfile.write ("#  define __PIC32_FEATURE_SET \"%s\"\n" % (feature_set))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_FEATURE_SET__\n")
                    outfile.write ("#  define __PIC32_FEATURE_SET__ \"%s\"\n" % (feature_set))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_FEATURE_SET0\n")
                    outfile.write ("#  define __PIC32_FEATURE_SET0 '%s'\n" % (feature_set[0]))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_FEATURE_SET1\n")
                    outfile.write ("#  define __PIC32_FEATURE_SET1 '%s'\n" % (feature_set[1]))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_PRODUCT_GROUP\n")
                    outfile.write ("#  define __PIC32_PRODUCT_GROUP '%s'\n" % (macro.group('product_group')))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_PIN_COUNT\n")
                    outfile.write ("#  define __PIC32_PIN_COUNT %d\n" % (int(macro.group('pin_count'))))
                    outfile.write ("#endif\n\n")
                else:
                    # e.g. 32MZ1025W104132
                    macro = re.match(r"32MZ(?P<zeros1>[0]*)(?P<flash_size>[0-9]+)(?P<feature_set>[A-Z]{1,})(?P<zeros2>[0]*)(?P<pin_count>[0-9]+)\Z", namewithoutpic.upper())
                    if macro:
                        outfile.write ("#ifndef __PIC32_FLASH_SIZE\n")
                        outfile.write ("#  define __PIC32_FLASH_SIZE %d\n" % (int(macro.group('flash_size'))))
                        outfile.write ("#endif\n")
                        feature_set = macro.group('feature_set')
                        outfile.write ("#ifndef __PIC32_FEATURE_SET\n")
                        outfile.write ("#  define __PIC32_FEATURE_SET \"%s\"\n" % (feature_set))
                        outfile.write ("#endif\n")
                        outfile.write ("#ifndef __PIC32_FEATURE_SET__\n")
                        outfile.write ("#  define __PIC32_FEATURE_SET__ \"%s\"\n" % (feature_set))
                        outfile.write ("#endif\n")
                        outfile.write ("#ifndef __PIC32_FEATURE_SET0\n")
                        outfile.write ("#  define __PIC32_FEATURE_SET0 '%s'\n" % (feature_set[0]))
                        outfile.write ("#endif\n")

            elif "32MX" in namewithoutpic :
                outfile.write ("#ifndef __PIC32MX\n")
                outfile.write ("#  define __PIC32MX 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __PIC32MX__\n")
                outfile.write ("#  define __PIC32MX__ 1\n")
                outfile.write ("#endif\n")
                macro = re.match(r"32MX(?P<feature_set>[0-9]{3,})F(?P<zeros1>[0]*)(?P<memory_size>[0-9]{1,})(?P<pin_set>[A-Z]{1})\Z", namewithoutpic.upper())
                if macro :
                    if (flash_size_kb > 0) :
                        outfile.write ("#ifndef __PIC32_FLASH_SIZE\n")
                        outfile.write ("#  define __PIC32_FLASH_SIZE %s\n" % flash_size_kb)
                        outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_FEATURE_SET\n")
                    outfile.write ("#  define __PIC32_FEATURE_SET %s\n" % (macro.group('feature_set')))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_FEATURE_SET__\n")
                    outfile.write ("#  define __PIC32_FEATURE_SET__ %s\n" % (macro.group('feature_set')))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_MEMORY_SIZE\n")
                    outfile.write ("#  define __PIC32_MEMORY_SIZE %d\n" % (int(macro.group('memory_size'))))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_MEMORY_SIZE__\n")
                    outfile.write ("#  define __PIC32_MEMORY_SIZE__ %d\n" % (int(macro.group('memory_size'))))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_PIN_SET\n")
                    outfile.write ("#  define __PIC32_PIN_SET '%s'\n" % (macro.group('pin_set')))
                    outfile.write ("#endif\n\n")
                    outfile.write ("#ifndef __PIC32_PIN_SET__\n")
                    outfile.write ("#  define __PIC32_PIN_SET__ '%s'\n" % (macro.group('pin_set')))
                    outfile.write ("#endif\n\n")

            elif "32MM" in namewithoutpic :
                outfile.write ("#ifndef __PIC32MM\n")
                outfile.write ("#  define __PIC32MM 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __PIC32MM__\n")
                outfile.write ("#  define __PIC32MM__ 1\n")
                outfile.write ("#endif\n")
                macro = re.match(r"32MM(?P<zeros1>[0]+)(?P<flash_size>[0-9]+)(?P<feature_set>[A-Z]{2,})(?P<product_group>[A-Z]{1})(?P<zeros2>[0]+)(?P<pin_count>[0-9]+)\Z", namewithoutpic.upper())
                if macro :
                    if ((flash_size_kb > 0) and (flash_size_kb != (int)(macro.group('flash_size')))) :
                        outfile.write ("#undef __PIC32_FLASH_SIZE\n")
                        outfile.write ("#define __PIC32_FLASH_SIZE %s\n" % flash_size_kb)
                    else :
                        outfile.write ("#ifndef __PIC32_FLASH_SIZE\n")
                        outfile.write ("#  define __PIC32_FLASH_SIZE %d\n" % (int(macro.group('flash_size'))))
                        outfile.write ("#endif\n")
                    feature_set = macro.group('feature_set')
                    outfile.write ("#ifndef __PIC32_FEATURE_SET\n")
                    outfile.write ("#  define __PIC32_FEATURE_SET \"%s\"\n" % (feature_set))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_FEATURE_SET__\n")
                    outfile.write ("#  define __PIC32_FEATURE_SET__ \"%s\"\n" % (feature_set))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_FEATURE_SET0\n")
                    outfile.write ("#  define __PIC32_FEATURE_SET0 '%s'\n" % (feature_set[0]))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_FEATURE_SET1\n")
                    outfile.write ("#  define __PIC32_FEATURE_SET1 '%s'\n" % (feature_set[1]))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_PRODUCT_GROUP\n")
                    outfile.write ("#  define __PIC32_PRODUCT_GROUP '%s'\n" % (macro.group('product_group')))
                    outfile.write ("#endif\n")
                    outfile.write ("#ifndef __PIC32_PIN_COUNT\n")
                    outfile.write ("#  define __PIC32_PIN_COUNT %d\n" % (int(macro.group('pin_count'))))
                    outfile.write ("#endif\n\n")


            elif "32MK" in namewithoutpic :
                if (flash_size_kb > 0) :
                    outfile.write ("#ifndef __PIC32_FLASH_SIZE\n")
                    outfile.write ("#  define __PIC32_FLASH_SIZE %s\n" % flash_size_kb)
                    outfile.write ("#endif\n")
                outfile.write ("#ifndef __PIC32MK\n")
                outfile.write ("#  define __PIC32MK 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __PIC32MK__\n")
                outfile.write ("#  define __PIC32MK__ 1\n")
                outfile.write ("#endif\n")

            elif "32WK" in namewithoutpic :
                outfile.write ("#ifndef __PIC32WK\n")
                outfile.write ("#  define __PIC32WK 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __PIC32WK__\n")
                outfile.write ("#  define __PIC32WK__ 1\n")
                outfile.write ("#endif\n")

            elif "MEC14" in namewithoutpic :
                outfile.write ("#ifndef __MEC\n")
                outfile.write ("#  define __MEC 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __MEC__\n")
                outfile.write ("#  define __MEC__ 1\n")
                outfile.write ("#endif\n")

            elif "MGC" in namewithoutpic :
                outfile.write ("#ifndef __MGC\n")
                outfile.write ("#  define __MGC 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __MGC__\n")
                outfile.write ("#  define __MGC__ 1\n")
                outfile.write ("#endif\n")

            elif "MTCH" in namewithoutpic :
                outfile.write ("#ifndef __MTCH\n")
                outfile.write ("#  define __MTCH 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __MTCH__\n")
                outfile.write ("#  define __MTCH__ 1\n")
                outfile.write ("#endif\n")

            elif "USB49" in namewithoutpic :
                outfile.write ("#ifndef __USB49\n")
                outfile.write ("#  define __USB49 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __USB49__\n")
                outfile.write ("#  define __USB49__ 1\n")
                outfile.write ("#endif\n")
            elif "USB70" in namewithoutpic :
                outfile.write ("#ifndef __USB70\n")
                outfile.write ("#  define __USB70 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __USB70__\n")
                outfile.write ("#  define __USB70__ 1\n")
                outfile.write ("#endif\n")
            elif "USB72" in namewithoutpic :
                outfile.write ("#ifndef __USB72\n")
                outfile.write ("#  define __USB72 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __USB72__\n")
                outfile.write ("#  define __USB72__ 1\n")
                outfile.write ("#endif\n")                
            elif "IS22" in namewithoutpic :
                outfile.write ("#ifndef __IS22\n")
                outfile.write ("#  define __IS22 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __IS22__\n")
                outfile.write ("#  define __IS22__ 1\n")
                outfile.write ("#endif\n")
            elif "BT55" in namewithoutpic :
                outfile.write ("#ifndef __BT55\n")
                outfile.write ("#  define __BT55 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __BT55__\n")
                outfile.write ("#  define __BT55__ 1\n")
                outfile.write ("#endif\n")
            elif "USB249" in namewithoutpic :
                outfile.write ("#ifndef __USB249\n")
                outfile.write ("#  define __USB249 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __USB249__\n")
                outfile.write ("#  define __USB249__ 1\n")
                outfile.write ("#endif\n")

            # the WFI32 is a module that uses another device
            if "WFI32" in original_namewithoutpic:
                outfile.write ("#ifndef __WFI32\n")
                outfile.write ("#  define __WFI32 1\n")
                outfile.write ("#endif\n")
                outfile.write ("#ifndef __WFI32__\n")
                outfile.write("#  define __WFI32__ 1\n")
                outfile.write ("#endif\n")

            # Check all devices
            outfile.write ("/*  The following device macros indicate which core features are\n")
            outfile.write (" *  available on this device.\n")
            outfile.write (" */\n")
            if pic.hasL1Cache:
                outfile.write ("#ifndef __PIC32_HAS_L1CACHE\n")
                outfile.write ("# define __PIC32_HAS_L1CACHE 1\n")
                outfile.write ("#endif\n")
            if (pic.instructionSet.hasMIPS32r2 or "32MX" in namewithoutpic.upper()):
                outfile.write ("#ifndef __PIC32_HAS_MIPS32R2\n")
                outfile.write ("# define __PIC32_HAS_MIPS32R2 1\n")
                outfile.write ("#endif\n")
            if (pic.instructionSet.hasMIPS16 or "32MX" in namewithoutpic.upper()):
                outfile.write ("#ifndef __PIC32_HAS_MIPS16\n")
                outfile.write ("# define __PIC32_HAS_MIPS16 1\n")
                outfile.write ("#endif\n")
            if pic.instructionSet.hasMicroMIPS:
                outfile.write ("#ifndef __PIC32_HAS_MICROMIPS\n")
                outfile.write ("# define __PIC32_HAS_MICROMIPS 1\n")
                outfile.write ("#endif\n")
            if pic.instructionSet.hasDSPr2:
                outfile.write ("#ifndef __PIC32_HAS_DSPR2\n")
                outfile.write ("# define __PIC32_HAS_DSPR2 1\n")
                outfile.write ("#endif\n")
            if pic.instructionSet.hasMCUASE:
                outfile.write ("#ifndef __PIC32_HAS_MCUASE\n")
                outfile.write ("# define __PIC32_HAS_MCUASE 1\n")
                outfile.write ("#endif\n")
            if (fdom.first(pic.node, 'FPU') is not None):
                outfile.write ("#ifndef __PIC32_HAS_FPU64\n")
                outfile.write ("# define __PIC32_HAS_FPU64 1\n")
                outfile.write ("#endif\n")
            if (namewithoutpic.upper().find("32MX") != 0):
                outfile.write ("#ifndef __PIC32_HAS_SSX\n")
                outfile.write ("# define __PIC32_HAS_SSX 1\n")
                outfile.write ("#endif\n")
            if (namewithoutpic.upper().find("32MZ") == 0):
                outfile.write ("#ifndef __PIC32_HAS_MMU_MZ_FIXED\n")
                outfile.write ("# define __PIC32_HAS_MMU_MZ_FIXED 1\n")
                outfile.write ("#endif\n")
            if (namewithoutpic.upper().find("32MM") == 0) :
                outfile.write ("#ifndef __PIC32_HAS_INTCONVS\n")
                outfile.write ("# define __PIC32_HAS_INTCONVS 1\n")
                outfile.write ("#endif\n")
            if (namewithoutpic.upper().find("32WK") != 0):
                outfile.write ("#ifndef __PIC32_HAS_INIT_DATA\n")
                outfile.write ("# define __PIC32_HAS_INIT_DATA 1\n")
                outfile.write ("#endif\n")

            outfile.write ("#ifndef __PIC32_SRS_SET_COUNT\n")
            outfile.write ("# define __PIC32_SRS_SET_COUNT %d\n" % (pic.interrupts.shadowSetCount))
            outfile.write ("#endif\n")

            if "32MX" in namewithoutpic :
                outfile.write ("\n/* include generic header file for backwards compatibility with old C32 v1.xx code */\n")
                outfile.write ("/* WARNING: Macros from this file are deprecated and should not be used in new     */\n")
                outfile.write ("/*          source code.                                                           */\n")
                devfamily="pic32mx"
                outfile.write ("#include \"proc/p%s.h\"\n\n" % devfamily)
                outfile.write ("\n")
            outfile.write ("\n#endif\n")

    namewithoutpic=original_namewithoutpic

def printOffsetSfile(pic,outfile):

    outfile.write ("\n")
    firstsection = 1

    ct = 0
    vectorlist = []
    for addr,SFRDef_node in sorted(pic.addrOntoSFR.items(), key=lambda pair:pair[0]):
        name = SFRDef_node.getAttribute("edc:cname")
        if name[:3] == "OFF":
            if len(name) == 6:
                vector_index = name[3:]
                kseg1 = fdom.get2(SFRDef_node.parentNode, "kseg1", edc.str2int)
                if int(vector_index) == ct:
                    if firstsection:
                        outfile.write ("  .section .__vector_offset_%X_data,data,keep,address(0x%X)\n" % (long(addr)+kseg1, long(addr)+kseg1))
                        firstsection = 0
                    outfile.write ("  .word __vector_offset_%s\n" % str(int(vector_index)))
                else:
                    while ct < int(vector_index):
                        ct = ct + 1
                    outfile.write ("  .section .__vector_offset_%X_data,data,keep,address(0x%X)\n" % (long(addr)+kseg1, long(addr)+kseg1))
                    outfile.write ("  .word __vector_offset_%s\n" % str(int(vector_index)))
                ct = ct + 1


def getmemequates (pic, outfile):
    global _BEV_EXCPT_ADDR
    bootconfigsize = 0
    outfile.write ("\n")
    outfile.write ("/*************************************************************************\n")
    outfile.write (" * Memory Address Equates\n")
    outfile.write (" * _RESET_ADDR                    -- Reset Vector or entry point\n")
    if ("PIC32" in pic.name.upper() or "MGC3" in pic.name.upper() or "BT55" in pic.name.upper() or "WFI" in pic.name.upper() or "MTCH" in pic.name.upper()):
        outfile.write (" * _BEV_EXCPT_ADDR                -- Boot exception Vector\n")
        outfile.write (" * _DBG_EXCPT_ADDR                -- In-circuit Debugging Exception Vector\n")
        if ((False == pic.interrupts.hasVariableOffsets) or "BT55" in pic.name.upper()) :
            outfile.write (" * _DBG_CODE_ADDR                 -- In-circuit Debug Executive address\n")
            outfile.write (" * _DBG_CODE_SIZE                 -- In-circuit Debug Executive size\n")
        if pic.instructionSet.hasMicroMIPS:
            outfile.write (" * _SIMPLE_TLB_REFILL_EXCPT_ADDR  -- Simple TLB-Refill Exception Vector\n")
        if pic.hasL1Cache:
            outfile.write (" * _CACHE_ERR_EXCPT_ADDR          -- Cache-error Exception Vector\n")

    outfile.write (" * _GEN_EXCPT_ADDR                -- General Exception Vector\n")
    outfile.write (" *************************************************************************/\n")
    if ("PIC32M" in pic.name.upper() or "MGC3" in pic.name.upper() or "BT55" in pic.name.upper() or "WFI" in pic.name.upper() or "MTCH" in pic.name.upper()):
        outfile.write ("_RESET_ADDR                    = 0xBFC00000;\n")
        outfile.write ("_BEV_EXCPT_ADDR                = 0xBFC00380;\n")
        _BEV_EXCPT_ADDR = 0xBFC00380
        outfile.write ("_DBG_EXCPT_ADDR                = 0xBFC00480;\n")

        if pic.interrupts.hasVariableOffsets:
            vectorinprogmemsize = 0
            vectorinprogmemstart = 0
        else:
            for region_id, region_node in pic.idOntoRegion.iteritems():
                if fdom.hasLocalName(region_node, 'CodeSector') and region_id == "code":
                    codestart = fdom.get2(region_node,"beginaddr", edc.str2int)
                    codesize = fdom.get2(region_node,"endaddr", edc.str2int) - fdom.get2(region_node,"beginaddr", edc.str2int)
                if fdom.hasLocalName(region_node, 'BootConfigSector') and region_id == "bootconfig":
                    if region_id == "bootconfig":
                        bootconfigsize = fdom.get2(region_node,"endaddr", edc.str2int) - fdom.get2(region_node,"beginaddr", edc.str2int)
            # Is this Mansfield or Fontana?
            if bootconfigsize < 0x2FF0 :
                if ((codesize < 0x10000) or (namewithoutpic.upper().find("32MM") == 0)):
                    vectorinprogmemsize = 0x200+((countVectors(pic)+1)*0x20)
                    vectorinprogmemstart = codestart
                else :
                    vectorinprogmemsize = 0x1000
                    vectorinprogmemstart = 0
            else :
                vectorinprogmemsize = 0
                vectorinprogmemstart = 0
        if vectorinprogmemsize == 0:
            exceptmem = 0
        else:
            exceptmem = 0x2C01000

        if ((vectorinprogmemsize > 0) and (exceptmem > 0)):
            outfile.write ("_DBG_CODE_ADDR                 = 0x9FC00490;\n")
            outfile.write ("_DBG_CODE_SIZE                 = 0x760;\n")
        elif (False == pic.interrupts.hasVariableOffsets) :
            outfile.write ("_DBG_CODE_ADDR                 = 0xBFC02000;\n")
            outfile.write ("_DBG_CODE_SIZE                 = 0xFF0;\n")

        if pic.instructionSet.hasMicroMIPS:
            outfile.write ("_SIMPLE_TLB_REFILL_EXCPT_ADDR  = _ebase_address + 0;\n")

        if pic.hasL1Cache:
            outfile.write ("_CACHE_ERR_EXCPT_ADDR          = _ebase_address + 0x100;\n")

    elif (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
        outfile.write ("_RESET_ADDR                    = 0xBFD00000;\n")

    elif "32WK" in pic.name.upper() :
        outfile.write ("PROVIDE(_RESET_ADDR            = 0x80000000);\n")

    outfile.write ("_GEN_EXCPT_ADDR                = _ebase_address + 0x180;\n")
    outfile.write ("\n")


def getmemequates_pic (pic, outfile):
    global _BEV_EXCPT_ADDR
    bootconfigsize = 0
    if ("PIC32" in pic.name.upper() or "MGC3" in pic.name.upper() or "BT55" in pic.name.upper() or "WFI" in pic.name.upper() or "MTCH" in pic.name.upper()):
        if ((False == pic.interrupts.hasVariableOffsets) or "BT55" in pic.name.upper()) :
	    outfile.write ("\n")
	    outfile.write ("/*************************************************************************\n")
	    outfile.write (" * Memory Address Equates\n")
            outfile.write (" * _DBG_CODE_SIZE                 -- In-circuit Debug Executive size\n")
	    outfile.write (" *************************************************************************/\n")

    if ("PIC32M" in pic.name.upper() or "MGC3" in pic.name.upper() or "BT55" in pic.name.upper()) :

        if pic.interrupts.hasVariableOffsets:
            vectorinprogmemsize = 0
            vectorinprogmemstart = 0
        else:
            for region_id, region_node in pic.idOntoRegion.iteritems():
                if fdom.hasLocalName(region_node, 'CodeSector') and region_id == "code":
                    codestart = fdom.get2(region_node,"beginaddr", edc.str2int)
                    codesize = fdom.get2(region_node,"endaddr", edc.str2int) - fdom.get2(region_node,"beginaddr", edc.str2int)
                if fdom.hasLocalName(region_node, 'BootConfigSector') and region_id == "bootconfig":
                    if region_id == "bootconfig":
                        bootconfigsize = fdom.get2(region_node,"endaddr", edc.str2int) - fdom.get2(region_node,"beginaddr", edc.str2int)
            # Is this Mansfield or Fontana?
            if bootconfigsize < 0x2FF0 :
                if ((codesize < 0x10000) or (namewithoutpic.upper().find("32MM") == 0)):
                    vectorinprogmemsize = 0x200+((countVectors(pic)+1)*0x20)
                    vectorinprogmemstart = codestart
                else :
                    vectorinprogmemsize = 0x1000
                    vectorinprogmemstart = 0
            else :
                vectorinprogmemsize = 0
                vectorinprogmemstart = 0
        if vectorinprogmemsize == 0:
            exceptmem = 0
        else:
            exceptmem = 0x2C01000

        if ((vectorinprogmemsize > 0) and (exceptmem > 0)):
            outfile.write ("_DBG_CODE_SIZE                 = 0x760;\n")
        elif (False == pic.interrupts.hasVariableOffsets) :
            outfile.write ("_DBG_CODE_SIZE                 = 0xFF0;\n")

    outfile.write ("\n")


def getobjfile (pic, outfile):
    outfile.write ("/*************************************************************************\n")
    outfile.write (" * Legacy processor-specific object file.  Contains SFR definitions.\n")
    outfile.write (" * The SFR definitions are now provided in a processor-specific *.S\n")
    outfile.write (" * assembly source file rather than the processor.o file. Use the new\n")
    outfile.write (" * .S file rather than this processor.o file for new projects. MPLAB XC32\n")
    outfile.write (" * v2.10 and later will automatically link the new .S file.\n")
    outfile.write (" *************************************************************************/\n")
    # procdefs.ld is not preprocessed
    # DFPs use the full linker script, not procdefs.ld
    if "procdefs.ld" not in outfile.name :
      outfile.write ("#if defined(__XC32_VERSION__) && (__XC32_VERSION__ < 2100)\n")
      outfile.write ("OPTIONAL(\"processor.o\")\n")
      outfile.write ("#endif")
    else :
      outfile.write ("OPTIONAL(\"processor.o\")\n")

    if ("PIC32MX" in pic.name.upper() or "MGC3" in pic.name.upper()):
        outfile.write ("\n/*************************************************************************\n")
        outfile.write (" * Processor-specific legacy peripheral libraries are optional.\n")
        outfile.write (" * These legacy plibs are available as a separate download from\n")
        outfile.write (" *   http://www.microchip.com/mplabxc\n")
        outfile.write (" * Use MPLAB Harmony for new projects.\n")
        outfile.write (" *   http://www.microchip.com/mplabharmony\n")
        outfile.write (" *************************************************************************/\n")
        outfile.write ("OPTIONAL(\"libmchp_peripheral.a\")\n")
        outfile.write ("OPTIONAL(\"libmchp_peripheral_%s.a\")\n" % namewithoutpic)
    outfile.write ("\n")

def getvectorinfo (pic):
    exceptmem = 0
    ksegoffset = 0
    codestart = 0
    codesize = 0
    bootconfigstart = 0
    bootconfigsize = 0

    for region_id, region_node in pic.idOntoRegion.iteritems():
        if fdom.hasLocalName(region_node, 'CodeSector') and region_id == "code":
            ksegoffset = fdom.get2(region_node,"kseg0", edc.str2int)
            if (ksegoffset == ""):
                cmdlinetools.error('The edc:kseg0 attribute is required, but missing: %s' % showdom.elem2str(region_node))
            codestart = fdom.get2(region_node,"beginaddr", edc.str2int)
            codesize = fdom.get2(region_node,"endaddr", edc.str2int) - fdom.get2(region_node,"beginaddr", edc.str2int)
            if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
                ksegkey = fdom.get2(region_node, "ksegdef", edc.str2int)
                if ksegkey == 2:
                    ksegoffset = fdom.get2(region_node, "kseg1", edc.str2int)
                else:
                    ksegoffset = fdom.get2(region_node, "kseg0", edc.str2int)
        if (fdom.hasLocalName(region_node, 'BootConfigSector') and (region_id == "bootconfig")):
            bootconfigstart = fdom.get2(region_node,"beginaddr", edc.str2int)
            bootconfigsize = fdom.get2(region_node,"endaddr", edc.str2int) - fdom.get2(region_node,"beginaddr", edc.str2int)

    # Is this Mansfield or Fontana?
    if (bootconfigsize < 0x2FF0) :
        if ((codesize < 0x10000) or (namewithoutpic.upper().find("32MM") == 0)) :
            vectorinprogmemsize = 0x200+((countVectors(pic)+1)*0x20)
            vectorinprogmemstart = codestart
        else :
            vectorinprogmemsize = 0x1000
            vectorinprogmemstart = 0
    else :
        vectorinprogmemsize = 0
        vectorinprogmemstart = 0
        exceptmem = 0x2C01000

    if "32WK" in pic.name.upper() :
        vectorinprogmemsize = 0
        vectorinprogmemstart = 1024*20
    elif (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
        vectorinprogmemsize = 0
        vectorinprogmemstart = 0
    # Is this Michigan?
    elif pic.interrupts.hasVariableOffsets:
        vectorinprogmemsize = 0
        vectorinprogmemstart = 0

    if (vectorinprogmemstart >= codestart) :
        savevectorinprogmemstart = vectorinprogmemstart + ksegoffset + exceptmem
        savecodestart = savevectorinprogmemstart + vectorinprogmemsize
        savecodesize = codesize - vectorinprogmemsize
    elif vectorinprogmemsize == 0 :
        savecodestart = codestart + ksegoffset + exceptmem
        savecodesize = codesize - vectorinprogmemsize
        savevectorinprogmemstart = savecodestart
    else :
        savecodestart = codestart + ksegoffset
        savecodesize = codesize - vectorinprogmemsize
        savevectorinprogmemstart = codestart + ksegoffset + codesize - 0x1000

    return (savevectorinprogmemstart, vectorinprogmemsize, savecodestart, savecodesize)

def getintvector_pic (pic, outfile):
    outfile.write ("/*************************************************************************\n")
    outfile.write (" * Symbols used for interrupt-vector table generation\n")
    if pic.interrupts.hasVariableOffsets:
        outfile.write (" * To override the defaults, define the _ebase_address symbol using\n")
        outfile.write (" * the --defsym linker opt as shown in this example:\n")
        outfile.write (" *   xc32-gcc src.c -Wl,--defsym=_ebase_address=0x9D001000\n")
    else :
        outfile.write (" * To override the defaults, define the _vector_spacing & _ebase_address\n")
        outfile.write (" * symbols using the --defsym linker opt as shown in these examples:\n")
        outfile.write (" *   xc32-gcc src.c -Wl,--defsym=_vector_spacing=2\n")
        outfile.write (" *   xc32-gcc src.c -Wl,--defsym=_ebase_address=0x9D001000\n")
    outfile.write (" *************************************************************************/\n")

    ebase, n1, n2, n3 = getvectorinfo (pic)

    if "32WK" in namewithoutpic and "procdefs.ld" not in outfile.name :
        outfile.write ("\n#ifndef VECTOR_SPACING\n")
        outfile.write ("#  define VECTOR_SPACING 0x%X\n" % 1)
        outfile.write ("#endif\n")
        outfile.write ("PROVIDE(_vector_spacing = VECTOR_SPACING);\n")

        outfile.write ("\n#ifndef EBASE_ADDRESS\n")
        if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
            outfile.write ("#  define EBASE_ADDRESS .\n")
        # Is this Michigan?
        elif pic.interrupts.hasVariableOffsets:
            outfile.write ("#  define EBASE_ADDRESS .\n")
        else:
            outfile.write ("#  define EBASE_ADDRESS .\n")
        outfile.write ("#endif\n")
        # CPG SRAM device
        if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
            outfile.write ("PROVIDE(_ebase_address = ABSOLUTE(EBASE_ADDRESS));\n")
        else:
            outfile.write ("PROVIDE(_ebase_address = ABSOLUTE(EBASE_ADDRESS));\n")
    else :
        outfile.write ("PROVIDE(_vector_spacing = 0x0001);\n")
        # CPG SRAM device
        if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
            outfile.write ("PROVIDE(_ebase_address = ABSOLUTE(.));\n")
        else:
            outfile.write ("PROVIDE(_ebase_address = ABSOLUTE(.));\n")

def getintvector (pic, outfile):
    outfile.write ("/*************************************************************************\n")
    outfile.write (" * Symbols used for interrupt-vector table generation\n")
    if pic.interrupts.hasVariableOffsets:
        outfile.write (" * To override the defaults, define the _ebase_address symbol using\n")
        outfile.write (" * the --defsym linker opt as shown in this example:\n")
        outfile.write (" *   xc32-gcc src.c -Wl,--defsym=_ebase_address=0x9D001000\n")
    else :
        outfile.write (" * To override the defaults, define the _vector_spacing & _ebase_address\n")
        outfile.write (" * symbols using the --defsym linker opt as shown in these examples:\n")
        outfile.write (" *   xc32-gcc src.c -Wl,--defsym=_vector_spacing=2\n")
        outfile.write (" *   xc32-gcc src.c -Wl,--defsym=_ebase_address=0x9D001000\n")
    outfile.write (" *************************************************************************/\n")

    ebase, n1, n2, n3 = getvectorinfo (pic)

    if "32WK" in namewithoutpic and "procdefs.ld" not in outfile.name :
        outfile.write ("\n#ifndef VECTOR_SPACING\n")
        outfile.write ("#  define VECTOR_SPACING 0x%X\n" % 1)
        outfile.write ("#endif\n")
        outfile.write ("PROVIDE(_vector_spacing = VECTOR_SPACING);\n")
        outfile.write ("\n#ifndef EBASE_ADDRESS\n")
        if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
            outfile.write ("#  define EBASE_ADDRESS 0x%-8X\n" % 0xBFD00000)
        # Is this Michigan?
        elif pic.interrupts.hasVariableOffsets:
            outfile.write ("#  define EBASE_ADDRESS 0x%-8X\n" % ebase)
        else:
            outfile.write ("#  define EBASE_ADDRESS  0x%-8X\n" % ebase)
        outfile.write ("#endif\n")
        # CPG SRAM device
        if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
            outfile.write ("PROVIDE(_ebase_address = EBASE_ADDRESS);\n")
        else:
            outfile.write ("PROVIDE(_ebase_address = EBASE_ADDRESS);\n")
    else :
        outfile.write ("PROVIDE(_vector_spacing = 0x0001);\n")
        # CPG SRAM device
        if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
            outfile.write ("PROVIDE(_ebase_address = 0x%-8X);\n" % 0xBFD00000)
        else:
            outfile.write ("PROVIDE(_ebase_address = 0x%-8X);\n" % ebase)

progmemories = []
datamemories = []
def getmemregions (pic):
    global progsize
    global bootksegsize
    global bootiswritable

    bootksegsize = 0
    bootksegstart = 0
    bootkseg0offset = 0
    bootkseg1offset = 0
    spiflashspace_node = 0

    class MemoryEntry(object):
        def __init__(self, name,  origin, length):
            self.name = name
            self.origin = origin
            self.length = length

    firstsfrloop = 1
    ksegoffset = 0

    for region_id, region_node in pic.idOntoRegion.iteritems():
        begin=fdom.get2(region_node,"beginaddr", edc.str2int, defaultValue="")
        end=fdom.get2(region_node,"endaddr", edc.str2int, defaultValue="")

        if fdom.has(region_node, "ksegdef"):
            ksegkey = fdom.get2(region_node, "ksegdef", edc.str2int)
            if ksegkey == 2:
                ksegoffset = fdom.get2(region_node, "kseg1", edc.str2int)
#          print "At 1", ksegoffset
            else:
                ksegoffset = fdom.get2(region_node, "kseg0", edc.str2int)
#          print "At 2", ksegoffset
        if fdom.hasLocalName(region_node, 'CodeSector') and region_id == "code":
            ksegkey = fdom.get2(region_node, "ksegdef", edc.str2int, "")
            if ksegkey == 1:
                ksegoffset=fdom.get2(region_node,"kseg0", edc.str2int)
#          print "At 3", ksegoffset
            elif ksegkey == 2:
                ksegoffset=fdom.get2(region_node,"kseg1", edc.str2int)
#          print "At 4", ksegoffset
            elif ksegkey == 3:
                ksegoffset=fdom.get2(region_node,"kseg2", edc.str2int)
#          print "At 5", ksegoffset
            else :
                ksegoffset=fdom.get2(region_node,"kseg0", edc.str2int)
#          print "At 6", ksegoffset
            progstart = begin + ksegoffset
            progsize = end - begin
            progid = region_id
        # and (re.search("otp", region_id) == False)
        elif (fdom.hasLocalName(region_node, 'BootConfigSector')) :
            if (fdom.get2(region_node, "isotp", defaultValue="") == "true") :
                continue
            if fdom.has(region_node, 'kseg0'):
                bootkseg0offset=fdom.get2(region_node,"kseg0", edc.str2int)
            else :
                cmdlinetools.error('The edc:kseg0 attribute is required, but missing: %s' % showdom.elem2str(region_node))
                bootkseg0offset=0x80000000
            if fdom.has(region_node, 'kseg1'):
                bootkseg1offset=fdom.get2(region_node,"kseg1", edc.str2int)
            else :
                cmdlinetools.error('The edc:kseg1 attribute is required, but missing: %s' % showdom.elem2str(region_node))
                bootkseg1offset=0xa0000000
            bootksegstart = begin
            bootksegsize = end - begin
            bootiswritable = fdom.get2(region_node, "iswritable", defaultValue="true")

            debugstart = begin + bootkseg1offset + 0x2000
            debugsize = 0xFF0

            exceptstart = begin + bootkseg0offset + 0x1000
            exceptsize = 0x1000

            # Michigan devices have multiple BootConfigSectors and ConfigFuseSectors
            # so they will be processed inside this for loop.
            if pic.hasL1Cache or namewithoutpic.upper().find("32MK") == 0:
                if region_id == 'lowerbootalias':
                    mem = MemoryEntry("kseg1_boot_mem", bootksegstart + bootkseg1offset, 0x480)
                    progmemories.append(mem)
                    # There is a hole for the debug vector between 0x9FC00480 and 0x9FC004B0
                    mem = MemoryEntry("kseg0_boot_mem", bootksegstart + bootkseg0offset + 0x4B0, 0)
                    progmemories.append(mem)
                    mem = MemoryEntry("kseg1_boot_mem_4B0", bootksegstart + bootkseg1offset + 0x4B0, bootksegsize - 0x4B0)
                    progmemories.append(mem)
                else:
                    mem = MemoryEntry("%s" % region_id, bootksegstart + bootkseg1offset, bootksegsize)
                    progmemories.append(mem)
        elif fdom.hasLocalName(region_node, 'ConfigFuseSector'):
            kseg1offset=fdom.get2(region_node,"kseg1", edc.str2int)
            # get number of configwords
            configwords = len(fdom.children(region_node,"DCRDef"))

            if "PIC32MX" in pic.name.upper():
                mem = MemoryEntry("configsfrs", begin + kseg1offset, configwords * 4)
            else:
                configstring = "configsfrs_%X" % (begin + kseg1offset)
                mem = MemoryEntry(configstring, begin + kseg1offset, configwords * 4)

            datamemories.append(mem)
        elif fdom.hasLocalName(region_node, 'AltConfigFuseSector'):
            kseg1offset=fdom.get2(region_node,"kseg1", edc.str2int)
            # get number of configwords
            configwords = len(fdom.children(region_node,"DCRDef"))

            configstring = "configsfrs_%X" % (begin + kseg1offset)
            mem = MemoryEntry(configstring, begin + kseg1offset, configwords * 4)
            datamemories.append(mem)
        elif fdom.hasLocalName(region_node, 'SFRDataSector'):
            sfrkseg1offset=fdom.get2(region_node,"kseg1", edc.str2int)
            if firstsfrloop :
                sfrstart = begin
                firstsfrloop = 0
            else :
                if begin < sfrstart :
                    sfrstart = begin

        elif fdom.hasLocalName(region_node, ['GPRDataSector', 'EBIDataSector', 'SQIDataSector']):
            if region_id == 'kseg0_data_mem':
                gprksegoffset=fdom.get2(region_node,"kseg0", edc.str2int)
                mem = MemoryEntry("%-20s (w!x)" % region_id, begin + gprksegoffset, end - begin)
                datamemories.append(mem)
                DefaultMemRegions.data = region_id
            elif region_id == 'kseg1_data_mem':
                gprksegoffset=fdom.get2(region_node,"kseg1", edc.str2int)
                if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
                    mem = MemoryEntry("%-20s (rwx)" % region_id, begin + gprksegoffset, end - begin)
                else :
                    mem = MemoryEntry("%-20s (w!x)" % region_id, begin + gprksegoffset, end - begin)
                datamemories.append(mem)
                DefaultMemRegions.data = region_id
            else:
                gprksegoffset=fdom.get2(region_node,"kseg1", edc.str2int, defaultValue="")
                if (gprksegoffset):
                    mem = MemoryEntry("%s" % region_id, begin + gprksegoffset, end - begin)
                    datamemories.append(mem)
                if not(gprksegoffset):
                    gprksegoffset=fdom.get2(region_node,"kseg2", edc.str2int)
                    if (gprksegoffset):
                        mem = MemoryEntry("kseg2_%s" % region_id, begin + gprksegoffset, end - begin)
                        datamemories.append(mem)
                    gprksegoffset=fdom.get2(region_node,"kseg3", edc.str2int, defaultValue="")
                    if (gprksegoffset):
                        mem = MemoryEntry("kseg3_%s" % region_id, begin + gprksegoffset, end - begin)
                        datamemories.append(mem)

        elif (fdom.hasLocalName(region_node, 'SPIFlashSector')):
            spiflashspace_node = region_node
            begin = fdom.get2(region_node, 'beginaddr', edc.str2int, 0)
            end = fdom.get2(region_node, 'endaddr', edc.str2int, 0)
            mem = MemoryEntry("serial_mem", begin + 0x80000000, end-begin)
            progmemories.append(mem)

    if fdom.first(pic.node, 'SPIFlashSpace') and (spiflashspace_node == 0):
        spiflashspace_node = fdom.first(pic.node, 'SPIFlashSpace')
        begin = fdom.get2(spiflashspace_node, 'beginaddr', edc.str2int, 0)
        end = fdom.get2(spiflashspace_node, 'endaddr', edc.str2int, 0)
        mem = MemoryEntry("serial_mem", begin + 0x80000000, end-begin)
        progmemories.append(mem)

    if "PIC32MX" in pic.name.upper():
        for addr, DCRDef_node in sorted(pic.addrOntoDCR.iteritems(), key=lambda pair:pair[0]):
            cname = fdom.get2(DCRDef_node, 'cname')
            name = "config" + cname[len(cname)-1]
            kseg1 = fdom.get2(DCRDef_node.parentNode, "kseg1", edc.str2int)
            mem = MemoryEntry("%s" % name, (long(addr)+kseg1), 0x4)
            progmemories.append(mem)
    else:
        for addr, DCRDef_node in pic.addrOntoDCR.iteritems():
            kseg = fdom.get2(DCRDef_node.parentNode, "kseg1", edc.str2int)
            hexstring = "_%X" % (long(addr)+kseg)
            mem = MemoryEntry("config%s" % hexstring, (long(addr)+kseg), 0x4)
            progmemories.append(mem)
        for addr, DCRDef_node in pic.addrOntoAltDCR.iteritems():
            kseg = fdom.get2(DCRDef_node.parentNode, "kseg1", edc.str2int)
            hexstring = "_%X" % (long(addr)+kseg)
            mem = MemoryEntry("config%s" % hexstring, (long(addr)+kseg), 0x4)
            progmemories.append(mem)

    if not pic.interrupts.hasVariableOffsets:
        vectorinmem = 0
        if (bootksegsize == 0 or bootiswritable == "false") :
            # Ipswich (MEC14*) project
            mem = MemoryEntry("kseg0_program_mem     (rx)", progstart, progsize)
            progmemories.append(mem)
            DefaultMemRegions.reset = "kseg0_program_mem"
            vectorinmem = 1
        elif (bootksegsize < 0x2FF0) :
            if (namewithoutpic.upper().find("32MM") == 0):
                mem = MemoryEntry("kseg0_program_mem     (rx)", progstart, progsize)
                progmemories.append(mem)
            elif (progsize < 0x10000):
                exception_mem_size = 0x200+((countVectors(pic)+1)*0x20)
                # exception at beginning of program space
                mem = MemoryEntry("exception_mem", progstart, exception_mem_size)
                progmemories.append(mem)
                mem = MemoryEntry("kseg0_program_mem     (rx)", progstart + exception_mem_size, progsize - exception_mem_size)
                progmemories.append(mem)
            else :
                mem = MemoryEntry("exception_mem", progstart + progsize - 0x1000, 0x1000)
                progmemories.append(mem)
                mem = MemoryEntry("kseg0_program_mem     (rx)", progstart, progsize - 0x1000)
                progmemories.append(mem)
                # exception at end of program space
            mem = MemoryEntry("debug_exec_mem", bootksegstart + bootkseg0offset + 0x490, 0x760)
            progmemories.append(mem)
            mem = MemoryEntry("kseg0_boot_mem", bootksegstart + bootkseg0offset + 0x490, 0)
            progmemories.append(mem)

            vectorinmem = 1
        if vectorinmem == 0 :
            mem = MemoryEntry("exception_mem", exceptstart, exceptsize)
            progmemories.append(mem)
            mem = MemoryEntry("kseg0_program_mem     (rx)", progstart, progsize)
            progmemories.append(mem)
            mem = MemoryEntry("debug_exec_mem", bootksegstart + bootkseg1offset + 0x2000, 0xFF0)
            progmemories.append(mem)
            mem = MemoryEntry("kseg0_boot_mem", bootksegstart + bootkseg0offset + 0x490, 0x970)
            progmemories.append(mem)

        mem = MemoryEntry("sfrs", sfrstart + sfrkseg1offset, 0x100000)
        datamemories.append(mem)
        if ((bootksegstart + bootkseg1offset) > 0):
            if (bootiswritable == "true") :
                mem = MemoryEntry("kseg1_boot_mem", bootksegstart + bootkseg1offset, 0x490)
            else :
                mem = MemoryEntry("kseg1_boot_mem", bootksegstart + bootkseg1offset, bootksegsize)
            progmemories.append(mem)

    elif (pic.interrupts.hasVariableOffsets and not pic.hasL1Cache):
        # PIC32MM
        if "32MK" not in namewithoutpic.upper():
          if ((bootksegstart + bootkseg1offset) > 0):
              mem = MemoryEntry("kseg1_boot_mem", bootksegstart + bootkseg1offset, 0x490)
              progmemories.append(mem)

        #  Reserve memory for the debug exec if the BootConfigSector is large
        #  enough
        if "32WK" not in namewithoutpic :
            if (bootksegsize <= 0x490+8):
                mem = MemoryEntry("debug_exec_mem", bootksegstart + bootkseg0offset + 0x490, 0)
                progmemories.append(mem)
            else:
                mem = MemoryEntry("debug_exec_mem", bootksegstart + bootkseg0offset + 0x490, bootksegsize-0x490)
                progmemories.append(mem)
            mem = MemoryEntry("kseg0_boot_mem", bootksegstart + bootkseg0offset + 0x490, 0)
            progmemories.append(mem)

        vectorinmem = 1
        mem = MemoryEntry("kseg0_program_mem     (rx)", progstart, progsize)
        progmemories.append(mem)
        mem = MemoryEntry("sfrs", sfrstart + sfrkseg1offset, 0x100000)
        datamemories.append(mem)

    else:
        mem = MemoryEntry("kseg0_program_mem     (rx)", progstart, progsize)
        progmemories.append(mem)
        mem = MemoryEntry("sfrs", sfrstart + sfrkseg1offset, 0x100000)
        datamemories.append(mem)

def printmemregions (pic, outfile):
    def compareMemories(a,b):
        if a.origin > b.origin:
            return 1
        if a.origin < b.origin:
            return -1
        return 0
    progmemories.sort(compareMemories)
    outfile.write ("/*************************************************************************\n")
    outfile.write (" * Memory Regions\n")
    outfile.write (" *\n")
    outfile.write (" * Memory regions without attributes cannot be used for orphaned sections.\n")
    outfile.write (" * Only sections specifically assigned to these regions can be allocated\n")
    outfile.write (" * into these regions.\n")

    if "PIC32" in pic.name.upper() or "MGC3" in pic.name.upper() or "BT55" in pic.name.upper() or "WFI" in pic.name.upper():
        outfile.write (" *\n")
        outfile.write (" * The Debug exception vector is located at 0x9FC00480.\n")

    if (len(pic.addrOntoDCR) > 0) :
        outfile.write (" *\n")
        outfile.write (" * The config_<address> sections are used to locate the config words at\n")
        outfile.write (" * their absolute addresses.\n")

    outfile.write (" *************************************************************************/\n\n")
    # For PIC32WK, use preprocessor symbols for memory origin and length
    if "32WK" in namewithoutpic and "procdefs.ld" not in outfile.name :
        program_mem_origin_name = ""
        program_mem_length_name = ""
        program_mem_origin = 0
        program_mem_length = 0
        for memory in progmemories :
            if (memory.name.upper().find("PROGRAM_MEM") > 0) :
                program_mem_origin_name = "{!s}_ORIGIN".format(memory.name.upper().split()[0])
                program_mem_length_name = "{!s}_LENGTH".format(memory.name.upper().split()[0])
                program_mem_origin = memory.origin
                program_mem_length = memory.length
                break
        for memory in datamemories :
            if (memory.name.upper().find("DATA_MEM") > 0) :
                data_mem_origin_name = "{!s}_ORIGIN".format(memory.name.upper().split()[0])
                data_mem_length_name = "{!s}_LENGTH".format(memory.name.upper().split()[0])
                data_mem_origin = memory.origin
                data_mem_length = memory.length
                break

        for memory in progmemories :
            if (memory.name.upper().find("PROGRAM_MEM") > 0) :
                outfile.write ("#ifndef %s_ORIGIN\n" % (memory.name.upper().split()[0]))
                outfile.write ("#  define %s_ORIGIN 0x%-8X\n" % (memory.name.upper().split()[0], memory.origin))
                outfile.write ("#endif\n")
                outfile.write ("#ifndef %s_LENGTH\n" % (memory.name.upper().split()[0]))
                outfile.write ("#  define %s_LENGTH 0x%X\n" % (memory.name.upper().split()[0], memory.length))
                outfile.write ("#elif (%s_LENGTH > 0x%X)\n" % (memory.name.upper().split()[0], (int(memory.length)+int(data_mem_length))))
                outfile.write ("#  error %s_LENGTH is greater than the max size of 0x%X\n" % (memory.name.upper().split()[0], (int(memory.length)+int(data_mem_length))))
                outfile.write ("#endif\n")
            elif (memory.length > 4) :
                outfile.write ("#ifndef %s_ORIGIN\n" % (memory.name.upper().split()[0]))
                outfile.write ("#  define %s_ORIGIN 0x%-8X\n" % (memory.name.upper().split()[0], memory.origin))
                outfile.write ("#endif\n")
                outfile.write ("#ifndef %s_LENGTH\n" % (memory.name.upper().split()[0]))
                outfile.write ("#  define %s_LENGTH 0x%X\n" % (memory.name.upper().split()[0], memory.length))
                outfile.write ("#elif (%s_LENGTH > 0x%X)\n" % (memory.name.upper().split()[0], memory.length))
                outfile.write ("#  error %s_LENGTH is greater than the max size of 0x%X\n" % (memory.name.upper().split()[0], memory.length))
                outfile.write ("#endif\n")

        datamemories.sort(compareMemories)
        for memory in datamemories :
            if (memory.name.upper().find("DATA_MEM") > 0) and (program_mem_origin+program_mem_length == memory.origin):
                outfile.write ("#ifndef %s_ORIGIN\n" % (memory.name.upper().split()[0]))
                outfile.write ("#  define %s_ORIGIN (%s+%s)\n" % (memory.name.upper().split()[0], program_mem_origin_name, program_mem_length_name))
                outfile.write ("#endif\n")
                outfile.write ("#ifndef %s_LENGTH\n" % (memory.name.upper().split()[0]))
                outfile.write ("#  define %s_LENGTH (0x%X-%s)\n" % (memory.name.upper().split()[0], (int(memory.length)+int(program_mem_length)), program_mem_length_name))
                outfile.write ("#endif\n")
            elif (memory.name.upper().find("SFR") == -1) :
                outfile.write ("#ifndef %s_ORIGIN\n" % (memory.name.upper().split()[0]))
                outfile.write ("#  define %s_ORIGIN 0x%-8X\n" % (memory.name.upper().split()[0], memory.origin))
                outfile.write ("#endif\n")
                outfile.write ("#ifndef %s_LENGTH\n" % (memory.name.upper().split()[0]))
                outfile.write ("#  define %s_LENGTH 0x%X\n" % (memory.name.upper().split()[0], memory.length))
                outfile.write ("#endif\n")

        outfile.write ("\nMEMORY\n")
        outfile.write ("{\n")

        for memory in progmemories :
            if (memory.length > 4) :
                outfile.write ("  %-28s: ORIGIN = %s_ORIGIN, LENGTH = %s_LENGTH\n" % (memory.name, memory.name.upper().split()[0], memory.name.upper().split()[0]))
            else :
                outfile.write ("  %-28s: ORIGIN = 0x%-8X, LENGTH = 0x%X\n" % (memory.name, memory.origin, memory.length))
        datamemories.sort(compareMemories)
        for memory in datamemories :
            if (memory.name.upper().find("SFR") == -1) :
                outfile.write ("  %-28s: ORIGIN = %s_ORIGIN, LENGTH = %s_LENGTH\n" % (memory.name, memory.name.upper().split()[0], memory.name.upper().split()[0]))
            else :
                outfile.write ("  %-28s: ORIGIN = 0x%-8X, LENGTH = 0x%X\n" % (memory.name, memory.origin, memory.length))
        outfile.write ("}\n")
    else :
        outfile.write ("\nMEMORY\n")
        outfile.write ("{\n")
        progmemories.sort(compareMemories)
        for memory in progmemories :
            outfile.write ("  %-28s: ORIGIN = 0x%-8X, LENGTH = 0x%X\n" % (memory.name, memory.origin, memory.length))
        datamemories.sort(compareMemories)
        for memory in datamemories :
            if (memory.name.upper().find("CRYPTO") == -1) and (memory.name.upper().find("WCM") == -1):
                outfile.write ("  %-28s: ORIGIN = 0x%-8X, LENGTH = 0x%X\n" % (memory.name, memory.origin, memory.length))
        outfile.write ("}\n")

def getconfigwords (pic, outfile):
    configlist = []

    if (len(pic.addrOntoDCR) == 0) :
        return

    if "PIC32MX" in pic.name.upper():
        outfile.write ("\n/*************************************************************************\n")
        outfile.write (" * Configuration-word sections. Map the config-pragma input sections to\n")
        outfile.write (" * absolute-address output sections.\n")
        outfile.write (" *************************************************************************/\n")
        # write a SECTIONS specification to locate the configuration word
        # sections
        outfile.write ("SECTIONS\n")
        outfile.write ("{\n")
        for addr, DCRDef_node in sorted(pic.addrOntoDCR.iteritems(), key=lambda pair:pair[0]):
            cname = fdom.get2(DCRDef_node, 'cname')
            name = "config" + cname[len(cname)-1]
            kseg1 = fdom.get2(DCRDef_node.parentNode, "kseg1", edc.str2int)
            outfile.write ("  .config_%08X : {\n" % (long(addr)+kseg1))
            outfile.write ("    KEEP(*(.config_%08X))\n" % (long(addr)+kseg1))
            outfile.write ("  } > %s\n" % name)
        outfile.write ("}\n")
    # No need to map config bits, use absolute sections instead
    else:
        outfile.write ("\n/*************************************************************************\n")
        outfile.write (" * Configuration-word sections. Map the config-pragma input sections to\n")
        outfile.write (" * absolute-address output sections.\n")
        outfile.write (" *************************************************************************/\n")
        # write a SECTIONS specification to locate the configuration word
        # sections
        outfile.write ("SECTIONS\n")
        outfile.write ("{\n")
        # Config bits
        for addr, DCRDef_node in pic.addrOntoDCR.iteritems():
            kseg = fdom.get2(DCRDef_node.parentNode, "kseg1", edc.str2int)
            configwords = len(fdom.children(DCRDef_node.parentNode,"DCRDef"))
            configlist.append("config_%08X" % (long(addr)+kseg))
        # Alternate config bits
        for addr, DCRDef_node in pic.addrOntoAltDCR.iteritems():
            kseg = fdom.get2(DCRDef_node.parentNode, "kseg1", edc.str2int)
            configwords = len(fdom.children(DCRDef_node.parentNode,"DCRDef"))
            configlist.append("config_%08X" % (long(addr)+kseg))

        for configlabel in sorted(configlist):
            outfile.write ("  .%s : {\n" % (configlabel))
            outfile.write ("    KEEP(*(.%s))\n" % (configlabel))
            outfile.write ("  } > %s\n" % (configlabel))
        outfile.write ("}\n")

def getsections (pic, outfile):
    global progsize
    global bootksegsize
    global bootiswritable
    global _BEV_EXCPT_ADDR

    if (progsize == 0) :
        cmdlinetools.error('Call getsections() after first calling getmemregions() to initialize progsize')
        exit(-1)

    #if pic.hasL1Cache :
    #  DefaultMemRegions.data = "kseg0_data_mem"

    outfile.write ("SECTIONS\n")
    outfile.write ("{\n")

    if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
        outfile.write ("  .reset _RESET_ADDR :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP(*(.reset))\n")
        outfile.write ("  } > %s\n" % DefaultMemRegions.reset)
    elif "PIC32WK" not in pic.name.upper() :
        outfile.write ("  /* Boot Sections */\n")
        outfile.write ("  .reset _RESET_ADDR :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP(*(.reset))\n")
        outfile.write ("    KEEP(*(.reset.startup))\n")
        outfile.write ("  } > %s\n" % DefaultMemRegions.reset)
        if (_BEV_EXCPT_ADDR != ""):
            outfile.write ("  .bev_excpt _BEV_EXCPT_ADDR :\n")
            outfile.write ("  {\n")
            outfile.write ("    KEEP(*(.bev_handler))\n")
            outfile.write ("  } > %s\n" % DefaultMemRegions.reset)
    if "PIC32WK" in pic.name.upper() :
        outfile.write ("  /* Do not populate the default debug-exception handler */\n")
        outfile.write ("  /DISCARD/ : { *(._debug_exception) }\n\n")
        outfile.write ("  /* .dinit at beginning of serial_mem by default */\n")
        outfile.write ("#ifndef DINIT_ADDRESS\n")
        outfile.write ("#define DINIT_ADDRESS ORIGIN(serial_mem)\n")
        outfile.write ("#endif /* DINIT ADDRESS */\n")
        outfile.write ("  .dinit DINIT_ADDRESS :\n")
        outfile.write ("  {\n")
        outfile.write ("    _dinit_begin = .;\n")
        outfile.write ("    KEEP(*(.dinit))\n")
        outfile.write ("    . = ALIGN(4) ;\n")
        outfile.write ("    _dinit_end = .;\n")
        outfile.write ("  } > serial_mem\n")

    if (True == pic.hasL1Cache) :
        outfile.write ("  .cache_init :\n")
        outfile.write ("  {\n")
        outfile.write ("    *(.cache_init)\n")
        outfile.write ("    *(.cache_init.*)\n")
        outfile.write ("  } > kseg1_boot_mem_4B0\n")


    if ((False == pic.interrupts.hasVariableOffsets) and (DefaultMemRegions.reset == "kseg1_boot_mem")) :
        outfile.write ("  /* Debug exception vector */\n")
        outfile.write ("  .dbg_excpt _DBG_EXCPT_ADDR (NOLOAD) :\n")
        outfile.write ("  {\n")
        outfile.write ("    . += (DEFINED (_DEBUGGER) ? 0x8 : 0x0);\n")
        outfile.write ("  } > %s\n" % DefaultMemRegions.reset)

        outfile.write ("  /* Space reserved for the debug executive */\n")
        outfile.write ("  .dbg_code _DBG_CODE_ADDR (NOLOAD) :\n")
        outfile.write ("  {\n")
        outfile.write ("    . += (DEFINED (_DEBUGGER) ? _DBG_CODE_SIZE : 0x0);\n")
        outfile.write ("  } > debug_exec_mem\n")

    outfile.write ("\n")

    if (namewithoutpic.upper().find("32MZ") == 0) or (namewithoutpic.upper().find("BT55") == 0) or (namewithoutpic.upper().find("32BL") == 0) or (namewithoutpic.upper().find("WFI") == 0):
        outfile.write ("  .simple_tlb_refill_excpt _SIMPLE_TLB_REFILL_EXCPT_ADDR :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP(*(.simple_tlb_refill_vector))\n")
        if pic.interrupts.hasVariableOffsets :
            outfile.write ("  } > kseg0_program_mem\n")
        else :
            outfile.write ("  } > exception_mem\n")

    if pic.hasL1Cache:
        outfile.write ("  .cache_err_excpt _CACHE_ERR_EXCPT_ADDR :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP(*(.cache_err_vector))\n")
        if pic.interrupts.hasVariableOffsets :
            outfile.write ("  } > kseg0_program_mem\n")
        else :
            outfile.write ("  } > exception_mem\n")

    if (namewithoutpic.upper().find("32") == 0 or True):
        outfile.write ("  .app_excpt _GEN_EXCPT_ADDR :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP(*(.gen_handler))\n")
        if (pic.interrupts.hasVariableOffsets or (namewithoutpic.upper().find("MEC14") == 0) or (namewithoutpic.upper().find("USB") == 0) or (namewithoutpic.upper().find("32MM") == 0)) :
            outfile.write ("  } > kseg0_program_mem\n\n")
        else :
            outfile.write ("  } > exception_mem\n\n")

    #print "pic.interrupts.hasVariableOffsets = %d" % pic.interrupts.hasVariableOffsets
    #print "pic.interrupts.hasComputedOffsets = %d" % pic.interrupts.hasComputedOffsets
    #print "pic.interrupts.hasNewComputedOffsets = %d" % pic.interrupts.hasNewComputedOffsets

    if (pic.interrupts.hasVariableOffsets and ((namewithoutpic.upper().find("32M") == 0) or (namewithoutpic.upper().find("32WK") == 0) or (namewithoutpic.upper().find("WFI") == 0) or (namewithoutpic.upper().find("MGC3") == 0) or (namewithoutpic.upper().find("BT55") == 0)) and not (namewithoutpic.upper().find("32MM") == 0)):
        # Generate PIC32MZ vector sections here
        outfile.write ("  /* Interrupt vector table with vector offsets */\n")
        outfile.write ("  .vectors _ebase_address + 0x200 :\n")
        outfile.write ("  {\n")
        outfile.write ("    /*  Symbol __vector_offset_n points to .vector_n if it exists, \n")
        outfile.write ("     *  otherwise points to the default handler. The\n")
        outfile.write ("     *  vector_offset_init.o module then provides a .data section\n")
        outfile.write ("     *  containing values used to initialize the vector-offset SFRs\n")
        outfile.write ("     *  in the crt0 startup code.\n")
        outfile.write ("     */\n")
        for vector_index in range (0,256) :
            outfile.write ("    . = ALIGN(4) ;\n")
            outfile.write ("    __vector_offset_%d = (DEFINED(__vector_dispatch_%d) ? (. - _ebase_address) : __vector_offset_default);\n" % (vector_index, vector_index))
            outfile.write ("    KEEP(*(.vector_%d))\n" % vector_index)
        outfile.write ("    /* Default interrupt handler */\n")
        outfile.write ("    . = ALIGN(4) ;\n")
        outfile.write ("    __vector_offset_default = . - _ebase_address;\n")
        outfile.write ("    KEEP(*(.vector_default))\n")
        outfile.write ("  } > kseg0_program_mem\n")
    elif ((namewithoutpic.upper().find("32") == 0) or (namewithoutpic.upper().find("MGC3") == 0) or (namewithoutpic.upper().find("BT55") == 0)):
        # For small devices with the vector table at the beginning of program flash, limit the number of vector sections to the number
        # of actually implemented vectors.

        # TODO: Determine this by using INTCON.VS
        if (namewithoutpic.upper().find("32MM") == 0) :
            for vector_index in range (0,countVectors(pic)+1) :
                outfile.write ("  .vector_%d _ebase_address + 0x200 + ((_vector_spacing << 3) * %d) :\n" % (vector_index, vector_index))
                outfile.write ("  {\n")
                outfile.write ("     KEEP(*(.vector_%d))\n" % (vector_index))
                outfile.write ("  } > kseg0_program_mem\n")
                outfile.write ("  ASSERT (_vector_spacing == 0 || SIZEOF(.vector_%d) <= (_vector_spacing << 3), \"function at exception vector %d too large\")\n" % (vector_index, vector_index))
        elif ((bootksegsize < 0x2FF0) and (progsize < 0x10000)):
            for vector_index in range (0,countVectors(pic)+1) :
                outfile.write ("  .vector_%d _ebase_address + 0x200 + ((_vector_spacing << 5) * %d) :\n" % (vector_index, vector_index))
                outfile.write ("  {\n")
                outfile.write ("     KEEP(*(.vector_%d))\n" % (vector_index))
                outfile.write ("  } > exception_mem\n")
                outfile.write ("  ASSERT (_vector_spacing == 0 || SIZEOF(.vector_%d) <= (_vector_spacing << 5), \"function at exception vector %d too large\")\n" % (vector_index, vector_index))
        else :
            for vector_index in range (0,64) :
                outfile.write ("  .vector_%d _ebase_address + 0x200 + ((_vector_spacing << 5) * %d) :\n" % (vector_index, vector_index))
                outfile.write ("  {\n")
                outfile.write ("     KEEP(*(.vector_%d))\n" % (vector_index))
                outfile.write ("  } > exception_mem\n")
                outfile.write ("  ASSERT (_vector_spacing == 0 || SIZEOF(.vector_%d) <= (_vector_spacing << 5), \"function at exception vector %d too large\")\n"     % (vector_index, vector_index))
    elif (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
        outfile.write ("  /* Jump Table Vectored Interrupt Controller (JTVIC) has 19 aggregated \n")
        outfile.write ("   * interrupt sources in its power-on-reset configuration. The \n")
        outfile.write ("   * data sheet uses nomenclature GIRQ00 - GIRQ18.\n")
        outfile.write ("   * The MIPS core was built to support EIC Mode 2 only. The JTVIC \n")
        outfile.write ("   * supplies a 17-bit offset to the MIPS core. The MIPS core combines this \n")
        outfile.write ("   * 17-bit offset with EBASE to produce the final vector location.\n")
        outfile.write ("   * Similar to the PIC32MZ EVIC, the JTVIC has a register for each \n")
        outfile.write ("   * GIRQx containing the 17-bit value supplied to the MIPS core.\n")
        outfile.write ("   * This means ISR's can be located anywhere above EBASE except for \n")
        outfile.write ("   * the fixed location of the General Exception at EBASE + 0x180.\n")
        outfile.write ("   * All JTVIC ISR's should be put into a named sections .vector_0, \n")
        outfile.write ("   * .vector_1, ..., .vector_18\n")
        outfile.write ("   */\n")
        outfile.write ("  .vectors _ebase_address + 0x200 :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP (*(.vector_0))\n")
        outfile.write ("    KEEP (*(.vector_1))\n")
        outfile.write ("    KEEP (*(.vector_2))\n")
        outfile.write ("    KEEP (*(.vector_3))\n")
        outfile.write ("    KEEP (*(.vector_4))\n")
        outfile.write ("    KEEP (*(.vector_5))\n")
        outfile.write ("    KEEP (*(.vector_6))\n")
        outfile.write ("    KEEP (*(.vector_7))\n")
        outfile.write ("    KEEP (*(.vector_8))\n")
        outfile.write ("    KEEP (*(.vector_9))\n")
        outfile.write ("    KEEP (*(.vector_10))\n")
        outfile.write ("    KEEP (*(.vector_11))\n")
        outfile.write ("    KEEP (*(.vector_12))\n")
        outfile.write ("    KEEP (*(.vector_13))\n")
        outfile.write ("    KEEP (*(.vector_14))\n")
        outfile.write ("    KEEP (*(.vector_15))\n")
        outfile.write ("    KEEP (*(.vector_16))\n")
        outfile.write ("    KEEP (*(.vector_17))\n")
        outfile.write ("    KEEP (*(.vector_18))\n")
        outfile.write ("    KEEP (*(.vec_default))\n")
        outfile.write ("  } > kseg0_program_mem\n")

    if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
        outfile.write ("\n  /*  The startup code is in the .reset.startup section.\n")
        outfile.write ("  *  SRAM Application startup code does NOT need to be \n")
        outfile.write ("  *  located at the beginning of CODE SRAM. A processor/chip\n")
        outfile.write ("  *  reset will go to the BootROM reset/startup code and \n")
        outfile.write ("  *  begin the BootROM SPI application load sequence.\n")
        outfile.write ("  *  Upon a successful SPI load and verification, BootROM \n")
        outfile.write ("  *  will jump into the Application. We expect the jump address \n")
        outfile.write ("  *  to be .startup(reset handler) of the application because \n")
        outfile.write ("  *  .startup runs the XC32 startup code and calls C main.\n")
        outfile.write ("  *  Since application .startup is never entered on a real HW \n")
        outfile.write ("  *  reset/nmi/soft-reset it can be located anywhere in SRAM \n")
        outfile.write ("  *  CODE space.\n")
        outfile.write ("  */\n")
    else :
        outfile.write ("\n  /*  The startup code is in the .reset.startup section.\n")
        outfile.write ("   *  Keep this here for backwards compatibility with older\n")
        outfile.write ("   *  C32 v1.xx releases.\n")
        outfile.write ("   */\n")

    if (((namewithoutpic.upper().find("32") == 0) or    (namewithoutpic.upper().find("MGC3") == 0) or (namewithoutpic.upper().find("BT55") == 0) or (namewithoutpic.upper().find("WFI") == 0)) and
      (DefaultMemRegions.startup == "kseg0_boot_mem")):
        outfile.write ("  .startup ORIGIN(kseg0_boot_mem) :\n")
    else:
        outfile.write ("  .startup :\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP(*(.startup))\n")
    if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
        outfile.write ("    KEEP(*(.reset.startup))\n")
    if ((namewithoutpic.upper().find("32") == 0) or (namewithoutpic.upper().find("MGC3") == 0) or (namewithoutpic.upper().find("BT55") == 0) or (namewithoutpic.upper().find("WFI") == 0)):
        outfile.write ("  } > %s\n" % DefaultMemRegions.startup)
    else:
        outfile.write ("  } > kseg0_program_mem\n")

    outfile.write ("\n  /* Code Sections - Note that input sections *(.text) and *(.text.*)\n")
    outfile.write ("   * are not mapped here. The best-fit allocator locates them,\n")
    outfile.write ("   * so that .text may flow around absolute sections as needed.\n")
    outfile.write ("   */\n")
    outfile.write ("  .text :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.stub .gnu.linkonce.t.*)\n")
    outfile.write ("    KEEP (*(.text.*personality*))\n")
    outfile.write ("    *(.mips16.fn.*)\n")
    outfile.write ("    *(.mips16.call.*)\n")
    outfile.write ("    *(.gnu.warning)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  /* Global-namespace object initialization */\n")
    outfile.write ("  .init   :\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*crti.o(.init))\n")
    outfile.write ("    KEEP (*crtbegin.o(.init))\n")
    outfile.write ("    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o *crtn.o ).init))\n")
    outfile.write ("    KEEP (*crtend.o(.init))\n")
    outfile.write ("    KEEP (*crtn.o(.init))\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  .fini   :\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*(.fini))\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  .preinit_array   :\n")
    outfile.write ("  {\n")
    outfile.write ("    PROVIDE_HIDDEN (__preinit_array_start = .);\n")
    outfile.write ("    KEEP (*(.preinit_array))\n")
    outfile.write ("    PROVIDE_HIDDEN (__preinit_array_end = .);\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  .init_array   :\n")
    outfile.write ("  {\n")
    outfile.write ("    PROVIDE_HIDDEN (__init_array_start = .);\n")
    outfile.write ("    KEEP (*(SORT(.init_array.*)))\n")
    outfile.write ("    KEEP (*(.init_array))\n")
    outfile.write ("    PROVIDE_HIDDEN (__init_array_end = .);\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  .fini_array   :\n")
    outfile.write ("  {\n")
    outfile.write ("    PROVIDE_HIDDEN (__fini_array_start = .);\n")
    outfile.write ("    KEEP (*(SORT(.fini_array.*)))\n")
    outfile.write ("    KEEP (*(.fini_array))\n")
    outfile.write ("    PROVIDE_HIDDEN (__fini_array_end = .);\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  .ctors   :\n")
    outfile.write ("  {\n")
    outfile.write ("    /* XC32 uses crtbegin.o to find the start of\n")
    outfile.write ("       the constructors, so we make sure it is\n")
    outfile.write ("       first.  Because this is a wildcard, it\n")
    outfile.write ("       doesn't matter if the user does not\n")
    outfile.write ("       actually link against crtbegin.o; the\n")
    outfile.write ("       linker won't look for a file to match a\n")
    outfile.write ("       wildcard.  The wildcard also means that it\n")
    outfile.write ("       doesn't matter which directory crtbegin.o\n")
    outfile.write ("       is in.  */\n")
    outfile.write ("    KEEP (*crtbegin.o(.ctors))\n")
    outfile.write ("    KEEP (*crtbegin?.o(.ctors))\n")
    outfile.write ("    /* We don't want to include the .ctor section from\n")
    outfile.write ("       the crtend.o file until after the sorted ctors.\n")
    outfile.write ("       The .ctor section from the crtend file contains the\n")
    outfile.write ("       end of ctors marker and it must be last */\n")
    outfile.write ("    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))\n")
    outfile.write ("    KEEP (*(SORT(.ctors.*)))\n")
    outfile.write ("    KEEP (*(.ctors))\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  .dtors   :\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*crtbegin.o(.dtors))\n")
    outfile.write ("    KEEP (*crtbegin?.o(.dtors))\n")
    outfile.write ("    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))\n")
    outfile.write ("    KEEP (*(SORT(.dtors.*)))\n")
    outfile.write ("    KEEP (*(.dtors))\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  /* Read-only sections */\n")
    outfile.write ("  .rodata   :\n")
    outfile.write ("  {\n")
    outfile.write ("    *( .gnu.linkonce.r.*)\n")
    outfile.write ("    *(.rodata1)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  /*\n")
    outfile.write ("   * Small initialized constant global and static data can be placed in the\n")
    outfile.write ("   * .sdata2 section.  This is different from .sdata, which contains small\n")
    outfile.write ("   * initialized non-constant global and static data.\n")
    outfile.write ("   */\n")
    outfile.write ("  .sdata2 ALIGN(4) :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  /*\n")
    outfile.write ("   * Uninitialized constant global and static data (i.e., variables which will\n")
    outfile.write ("   * always be zero).  Again, this is different from .sbss, which contains\n")
    outfile.write ("   * small non-initialized, non-constant global and static data.\n")
    outfile.write ("   */\n")
    outfile.write ("  .sbss2 ALIGN(4) :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("  .eh_frame_hdr   :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.eh_frame_hdr)\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  .eh_frame   : ONLY_IF_RO\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*(.eh_frame))\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  .gcc_except_table   : ONLY_IF_RO\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.gcc_except_table .gcc_except_table.*)\n")
    outfile.write ("  } >kseg0_program_mem\n")
    outfile.write ("    . = ALIGN(4) ;\n")

    if "PIC32" in pic.name.upper() or "MGC3" in pic.name.upper() or "BT55" in pic.name.upper() or "WFI" in pic.name.upper():
        outfile.write ("  .dbg_data (NOLOAD) :\n")
        outfile.write ("  {\n")
        outfile.write ("    . += (DEFINED (_DEBUGGER) ? 0x200 : 0x0);\n")
        if pic.instructionSet.hasDSPr2:
            outfile.write ("    /* Additional data memory required for DSPr2 registers */\n")
            outfile.write ("    . += (DEFINED (_DEBUGGER) ? 0x80 : 0x0);\n")
        if (fdom.first(pic.node, 'FPU') is not None):
            outfile.write ("    /* Additional data memory required for FPU64 registers */\n")
            outfile.write ("    . += (DEFINED (_DEBUGGER) ? 0x100 : 0x0);\n")
        outfile.write ("  } >%s\n" % DefaultMemRegions.data)

    outfile.write ("  .jcr   :\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*(.jcr))\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >%s\n" % DefaultMemRegions.data)
    outfile.write ("  .eh_frame    : ONLY_IF_RW\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*(.eh_frame))\n")
    outfile.write ("  } >%s\n" % DefaultMemRegions.data)
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  .gcc_except_table    : ONLY_IF_RW\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.gcc_except_table .gcc_except_table.*)\n")
    outfile.write ("  } >%s\n" % DefaultMemRegions.data)
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  /* Persistent data - Use the new C 'persistent' attribute instead. */\n")
    outfile.write ("  .persist   :\n")
    outfile.write ("  {\n")
    outfile.write ("    _persist_begin = .;\n")
    outfile.write ("    *(.persist .persist.*)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("    _persist_end = .;\n")
    outfile.write ("  } >%s\n" % DefaultMemRegions.data)
    outfile.write ("  /*\n")
    outfile.write ("   *  Note that input sections named .data* are not mapped here.\n")
    outfile.write ("   *  The best-fit allocator locates them, so that they may flow\n")
    outfile.write ("   *  around absolute sections as needed.\n")
    outfile.write ("   */\n")
    outfile.write ("  .data   :\n")
    outfile.write ("  {\n")
    outfile.write ("    *( .gnu.linkonce.d.*)\n")
    outfile.write ("    SORT(CONSTRUCTORS)\n")
    outfile.write ("    *(.data1)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >%s\n" % DefaultMemRegions.data)
    outfile.write ("  . = .;\n")
    outfile.write ("  _gp = ALIGN(16) + 0x7ff0;\n")
    outfile.write ("  .got ALIGN(4) :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.got.plt) *(.got)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >%s /* AT>kseg0_program_mem */\n" % DefaultMemRegions.data)
    outfile.write ("  /*\n")
    outfile.write ("   * Note that 'small' data sections are still mapped in the linker\n")
    outfile.write ("   * script. This ensures that they are grouped together for\n")
    outfile.write ("   * gp-relative addressing. Absolute sections are allocated after\n")
    outfile.write ("   * the 'small' data sections so small data cannot flow around them.\n")
    outfile.write ("   */\n")
    outfile.write ("  /*\n")
    outfile.write ("   * We want the small data sections together, so single-instruction offsets\n")
    outfile.write ("   * can access them all, and initialized data all before uninitialized, so\n")
    outfile.write ("   * we can shorten the on-disk segment size.\n")
    outfile.write ("   */\n")
    outfile.write ("  .sdata ALIGN(4) :\n")
    outfile.write ("  {\n")
    outfile.write ("    _sdata_begin = . ;\n")
    outfile.write ("    *(.sdata .sdata.* .gnu.linkonce.s.*)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("    _sdata_end = . ;\n")
    outfile.write ("  } >%s\n" % DefaultMemRegions.data)
    outfile.write ("  .lit8           :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.lit8)\n")
    outfile.write ("  } >%s\n" % DefaultMemRegions.data)
    outfile.write ("  .lit4           :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.lit4)\n")
    outfile.write ("  } >%s\n" % DefaultMemRegions.data)
    outfile.write ("  . = ALIGN (4) ;\n")
    outfile.write ("  _data_end = . ;\n")
    outfile.write ("  _bss_begin = . ;\n")
    outfile.write ("  .sbss ALIGN(4) :\n")
    outfile.write ("  {\n")
    outfile.write ("    _sbss_begin = . ;\n")
    outfile.write ("    *(.dynsbss)\n")
    outfile.write ("    *(.sbss .sbss.* .gnu.linkonce.sb.*)\n")
    outfile.write ("    *(.scommon)\n")
    outfile.write ("    _sbss_end = . ;\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  } >%s\n" % DefaultMemRegions.data)
    outfile.write ("  /*\n")
    outfile.write ("   *  Align here to ensure that the .bss section occupies space up to\n")
    outfile.write ("   *  _end.  Align after .bss to ensure correct alignment even if the\n")
    outfile.write ("   *  .bss section disappears because there are no input sections.\n")
    outfile.write ("   *\n")
    outfile.write ("   *  Note that input sections named .bss* are no longer mapped here.\n")
    outfile.write ("   *  The best-fit allocator locates them, so that they may flow\n")
    outfile.write ("   *  around absolute sections as needed.\n")
    outfile.write ("   *\n")
    outfile.write ("   */\n")
    outfile.write ("  .bss     :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.dynbss)\n")
    outfile.write ("    *(COMMON)\n")
    outfile.write ("   /* Align here to ensure that the .bss section occupies space up to\n")
    outfile.write ("      _end.  Align after .bss to ensure correct alignment even if the\n")
    outfile.write ("      .bss section disappears because there are no input sections. */\n")
    outfile.write ("   . = ALIGN(. != 0 ? 4 : 1);\n")
    outfile.write ("  } >%s\n" % DefaultMemRegions.data)
    outfile.write ("  . = ALIGN(4) ;\n")
    outfile.write ("  _end = . ;\n")
    outfile.write ("  _bss_end = . ;\n")
    outfile.write ("  /*\n")
    outfile.write ("   *  The heap and stack are best-fit allocated by the linker after other\n")
    outfile.write ("   *  data and bss sections have been allocated.\n")
    outfile.write ("   */\n")
    outfile.write ("  /*\n")
    outfile.write ("   * RAM functions go at the end of our stack and heap allocation.\n")
    outfile.write ("   * Alignment of 2K required by the boundary register (BMXDKPBA).\n")
    outfile.write ("   *\n")
    outfile.write ("   * RAM functions are now allocated by the linker. The linker generates\n")
    outfile.write ("   * _ramfunc_begin and _bmxdkpba_address symbols depending on the\n")
    outfile.write ("   * location of RAM functions.\n")
    outfile.write ("   */\n")
    outfile.write ("  _bmxdudba_address = LENGTH(%s) ;\n" % DefaultMemRegions.data)
    outfile.write ("  _bmxdupba_address = LENGTH(%s) ;\n" % DefaultMemRegions.data)
    outfile.write ("    /* The .pdr section belongs in the absolute section */\n")
    outfile.write ("    /DISCARD/ : { *(.pdr) }\n")
    outfile.write ("  .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }\n")
    outfile.write ("  .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }\n")
    outfile.write ("  .mdebug.abi32 0 : { KEEP(*(.mdebug.abi32)) }\n")
    outfile.write ("  .mdebug.abiN32 0 : { KEEP(*(.mdebug.abiN32)) }\n")
    outfile.write ("  .mdebug.abi64 0 : { KEEP(*(.mdebug.abi64)) }\n")
    outfile.write ("  .mdebug.abiO64 0 : { KEEP(*(.mdebug.abiO64)) }\n")
    outfile.write ("  .mdebug.eabi32 0 : { KEEP(*(.mdebug.eabi32)) }\n")
    outfile.write ("  .mdebug.eabi64 0 : { KEEP(*(.mdebug.eabi64)) }\n")
    outfile.write ("  .gcc_compiled_long32 : { KEEP(*(.gcc_compiled_long32)) }\n")
    outfile.write ("  .gcc_compiled_long64 : { KEEP(*(.gcc_compiled_long64)) }\n")
    outfile.write ("  /* Stabs debugging sections.  */\n")
    outfile.write ("  .stab          0 : { *(.stab) }\n")
    outfile.write ("  .stabstr       0 : { *(.stabstr) }\n")
    outfile.write ("  .stab.excl     0 : { *(.stab.excl) }\n")
    outfile.write ("  .stab.exclstr  0 : { *(.stab.exclstr) }\n")
    outfile.write ("  .stab.index    0 : { *(.stab.index) }\n")
    outfile.write ("  .stab.indexstr 0 : { *(.stab.indexstr) }\n")
    outfile.write ("  .comment       0 : { *(.comment) }\n")
    outfile.write ("  /* DWARF debug sections used by MPLAB X for source-level debugging. \n")
    outfile.write ("     Symbols in the DWARF debugging sections are relative to the beginning\n")
    outfile.write ("     of the section so we begin them at 0.  */\n")
    outfile.write ("  /* DWARF 1 */\n")
    outfile.write ("  .debug          0 : { *.elf(.debug) *(.debug) }\n")
    outfile.write ("  .line           0 : { *.elf(.line) *(.line) }\n")
    outfile.write ("  /* GNU DWARF 1 extensions */\n")
    outfile.write ("  .debug_srcinfo  0 : { *.elf(.debug_srcinfo) *(.debug_srcinfo) }\n")
    outfile.write ("  .debug_sfnames  0 : { *.elf(.debug_sfnames) *(.debug_sfnames) }\n")
    outfile.write ("  /* DWARF 1.1 and DWARF 2 */\n")
    outfile.write ("  .debug_aranges  0 : { *.elf(.debug_aranges) *(.debug_aranges) }\n")
    outfile.write ("  .debug_pubnames 0 : { *.elf(.debug_pubnames) *(.debug_pubnames) }\n")
    outfile.write ("  /* DWARF 2 */\n")
    outfile.write ("  .debug_info     0 : { *.elf(.debug_info .gnu.linkonce.wi.*) *(.debug_info .gnu.linkonce.wi.*) }\n")
    outfile.write ("  .debug_abbrev   0 : { *.elf(.debug_abbrev) *(.debug_abbrev) }\n")
    outfile.write ("  .debug_line     0 : { *.elf(.debug_line) *(.debug_line) }\n")
    outfile.write ("  .debug_frame    0 : { *.elf(.debug_frame) *(.debug_frame) }\n")
    outfile.write ("  .debug_str      0 : { *.elf(.debug_str) *(.debug_str) }\n")
    outfile.write ("  .debug_loc      0 : { *.elf(.debug_loc) *(.debug_loc) }\n")
    outfile.write ("  .debug_macinfo  0 : { *.elf(.debug_macinfo) *(.debug_macinfo) }\n")
    outfile.write ("  /* SGI/MIPS DWARF 2 extensions */\n")
    outfile.write ("  .debug_weaknames 0 : { *.elf(.debug_weaknames) *(.debug_weaknames) }\n")
    outfile.write ("  .debug_funcnames 0 : { *.elf(.debug_funcnames) *(.debug_funcnames) }\n")
    outfile.write ("  .debug_typenames 0 : { *.elf(.debug_typenames) *(.debug_typenames) }\n")
    outfile.write ("  .debug_varnames  0 : { *.elf(.debug_varnames) *(.debug_varnames) }\n")
    outfile.write ("  .debug_pubtypes 0 : { *.elf(.debug_pubtypes) *(.debug_pubtypes) }\n")
    outfile.write ("  .debug_ranges   0 : { *.elf(.debug_ranges) *(.debug_ranges) }\n")
    outfile.write ("  /DISCARD/ : { *(.rel.dyn) }\n")
    outfile.write ("  .gnu.attributes 0 : { KEEP (*(.gnu.attributes)) }\n")
    outfile.write ("  /DISCARD/ : { *(.note.GNU-stack) }\n")
    outfile.write ("  /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) *(.discard) }\n")
    outfile.write ("}\n\n")

def getsections_pic (pic, outfile):
    global progsize
    global bootksegsize
    global bootiswritable
    global _BEV_EXCPT_ADDR

#    if (progsize == 0) :
#        cmdlinetools.error('Call getsections() after first calling getmemregions() to initialize progsize')
#        exit(-1)

    #if pic.hasL1Cache :
    #  DefaultMemRegions.data = "kseg0_data_mem"

    outfile.write ("SECTIONS\n")
    outfile.write ("{\n")

    if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
        outfile.write ("  .reset :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP(*(.reset))\n")
        outfile.write ("  }\n")
    elif "PIC32WK" not in pic.name.upper() :
        outfile.write ("  /* Boot Sections */\n")
        outfile.write ("  .reset :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP(*(.reset))\n")
        outfile.write ("    KEEP(*(.reset.startup))\n")
        outfile.write ("  }\n")
        if (_BEV_EXCPT_ADDR != ""):
            outfile.write ("  .bev_excpt :\n")
            outfile.write ("  {\n")
            outfile.write ("    KEEP(*(.bev_handler))\n")
            outfile.write ("  }\n")
    if "PIC32WK" in pic.name.upper() :
        outfile.write ("  /* Do not populate the default debug-exception handler */\n")
        outfile.write ("  /DISCARD/ : { *(._debug_exception) }\n\n")
        outfile.write ("  /* .dinit at beginning of serial_mem by default */\n")
        outfile.write ("  /* .dinit at beginning of serial_mem by default */\n")
#        outfile.write ("#ifndef DINIT_ADDRESS\n")
#        outfile.write ("#define DINIT_ADDRESS ORIGIN(serial_mem)\n")
#        outfile.write ("#endif /* DINIT ADDRESS */\n")
        outfile.write ("  .dinit :\n")
        outfile.write ("  {\n")
        outfile.write ("    _dinit_begin = .;\n")
        outfile.write ("    KEEP(*(.dinit))\n")
        outfile.write ("    . = ALIGN(4) ;\n")
        outfile.write ("    _dinit_end = .;\n")
        outfile.write ("  }\n")

    if (True == pic.hasL1Cache) :
        outfile.write ("  .cache_init :\n")
        outfile.write ("  {\n")
        outfile.write ("    *(.cache_init)\n")
        outfile.write ("    *(.cache_init.*)\n")
        outfile.write ("  }\n")


    if ((False == pic.interrupts.hasVariableOffsets) and (DefaultMemRegions.reset == "kseg1_boot_mem")) :
        outfile.write ("  /* Debug exception vector */\n")
        outfile.write ("  .dbg_excpt (NOLOAD) :\n")
        outfile.write ("  {\n")
        outfile.write ("    . += (DEFINED (_DEBUGGER) ? 0x8 : 0x0);\n")
        outfile.write ("  }\n")

        outfile.write ("  /* Space reserved for the debug executive */\n")
        outfile.write ("  .dbg_code (NOLOAD) :\n")
        outfile.write ("  {\n")
        outfile.write ("    . += (DEFINED (_DEBUGGER) ? _DBG_CODE_SIZE : 0x0);\n")
        outfile.write ("  }\n")

    outfile.write ("\n")

    if (namewithoutpic.upper().find("32MZ") == 0) or (namewithoutpic.upper().find("BT55") == 0) or (namewithoutpic.upper().find("WFI") == 0) or (namewithoutpic.upper().find("32BL") == 0):
        outfile.write ("  .simple_tlb_refill_excpt :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP(*(.simple_tlb_refill_vector))\n")
        if pic.interrupts.hasVariableOffsets :
            outfile.write ("  }\n")
        else :
            outfile.write ("  }\n")

    if pic.hasL1Cache:
        outfile.write ("  .cache_err_excpt :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP(*(.cache_err_vector))\n")
        if pic.interrupts.hasVariableOffsets :
            outfile.write ("  }\n")
        else :
            outfile.write ("  }\n")

    if (namewithoutpic.upper().find("32") == 0 or True):
        outfile.write ("  .app_excpt :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP(*(.gen_handler))\n")
        if (pic.interrupts.hasVariableOffsets or (namewithoutpic.upper().find("MEC14") == 0) or (namewithoutpic.upper().find("USB") == 0) or (namewithoutpic.upper().find("32MM") == 0)) :
            outfile.write ("  }\n\n")
        else :
            outfile.write ("  }\n\n")

    outfile.write (".plt            : { *(.plt) }\n")
    outfile.write (".iplt           : { *(.iplt) }\n\n")

    getintvector_pic (pic, outfile)
    outfile.write ("\n\n")

    if (pic.interrupts.hasVariableOffsets and ((namewithoutpic.upper().find("32") == 0) or (namewithoutpic.upper().find("WFI") == 0) or (namewithoutpic.upper().find("MGC3") == 0) or (namewithoutpic.upper().find("BT55") == 0) )) :
        # Generate PIC32MZ vector sections here
        outfile.write ("  /* Interrupt vector table with vector offsets */\n")
        outfile.write ("  .vectors _ebase_address + 0x200 :\n")
        outfile.write ("  {\n")
        outfile.write ("    /*  Symbol __vector_offset_n points to .vector_n if it exists, \n")
        outfile.write ("     *  otherwise points to the default handler. The\n")
        outfile.write ("     *  vector_offset_init.o module then provides a .data section\n")
        outfile.write ("     *  containing values used to initialize the vector-offset SFRs\n")
        outfile.write ("     *  in the crt0 startup code.\n")
        outfile.write ("     */\n")
        for vector_index in range (0,256) :
            outfile.write ("    __vector_offset_%d = (DEFINED(__vector_dispatch_%d) ? (. - _ebase_address) : __vector_offset_default);\n" % (vector_index, vector_index))
            outfile.write ("    KEEP(*(.vector_%d))\n" % vector_index)
        outfile.write ("    /* Default interrupt handler */\n")
        outfile.write ("    __vector_offset_default = . - _ebase_address;\n")
        outfile.write ("    KEEP(*(.vector_default))\n")
        outfile.write ("  }\n")
    elif ((namewithoutpic.upper().find("32") == 0) or (namewithoutpic.upper().find("MGC3") == 0) or (namewithoutpic.upper().find("BT55") == 0)):
        # For small devices with the vector table at the beginning of program flash, limit the number of vector sections to the number
        # of actually implemented vectors.

        # TODO: Determine this by using INTCON.VS
        if (namewithoutpic.upper().find("32MM") == 0) :
            for vector_index in range (0,countVectors(pic)+1) :
                outfile.write ("  .vector_%d _ebase_address + 0x200 + ((_vector_spacing << 5) * %d) :\n" % (vector_index, vector_index))
                outfile.write ("  {\n")
                outfile.write ("     KEEP(*(.vector_%d))\n" % (vector_index))
                outfile.write ("  }\n")
                outfile.write ("  ASSERT (_vector_spacing == 0 || SIZEOF(.vector_%d) <= (_vector_spacing << 5), \"function at exception vector %d too large\")\n" % (vector_index, vector_index))
        elif ((bootksegsize < 0x2FF0) and (progsize < 0x10000)):
            for vector_index in range (0,countVectors(pic)+1) :
                outfile.write ("  .vector_%d _ebase_address + 0x200 + ((_vector_spacing << 5) * %d) :\n" % (vector_index, vector_index))
                outfile.write ("  {\n")
                outfile.write ("     KEEP(*(.vector_%d))\n" % (vector_index))
                outfile.write ("  }\n")
                outfile.write ("  ASSERT (_vector_spacing == 0 || SIZEOF(.vector_%d) <= (_vector_spacing << 5), \"function at exception vector %d too large\")\n" % (vector_index, vector_index))
        else :
            for vector_index in range (0,64) :
                outfile.write ("  .vector_%d _ebase_address + 0x200 + ((_vector_spacing << 5) * %d) :\n" % (vector_index, vector_index))
                outfile.write ("  {\n")
                outfile.write ("     KEEP(*(.vector_%d))\n" % (vector_index))
                outfile.write ("  }\n")
                outfile.write ("  ASSERT (_vector_spacing == 0 || SIZEOF(.vector_%d) <= (_vector_spacing << 5), \"function at exception vector %d too large\")\n"     % (vector_index, vector_index))
    elif (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
        outfile.write ("  /* MEC14x4 JTVIC has 19 aggregated interrupt sources in its \n")
        outfile.write ("   * power-on-reset configuration. MEC14xx data sheet uses nomenclature \n")
        outfile.write ("   * GIRQ00 - GIRQ18.\n")
        outfile.write ("   * MEC14x4 M14K core was built to support EIC Mode 2 only. The JTVIC \n")
        outfile.write ("   * supplies a 17-bit offset to the M14K core. The M14K combines this \n")
        outfile.write ("   * 17-bit offset with EBASE to produce the final vector location.\n")
        outfile.write ("   * Similar to the PIC32MZ EVIC, the JTVIC has a register for each \n")
        outfile.write ("   * GIRQx containing the 17-bit value supplied to the M14K core.\n")
        outfile.write ("   * This means ISR's can be located anywhere above EBASE except for \n")
        outfile.write ("   * the fixed location of the General Exception at EBASE + 0x180.\n")
        outfile.write ("   * All JTVIC ISR's should be put into a named sections .vector_0, \n")
        outfile.write ("   * .vector_1, ..., .vector_18\n")
        outfile.write ("   */\n")
        outfile.write ("  .vectors _ebase_address + 0x200 :\n")
        outfile.write ("  {\n")
        outfile.write ("    KEEP (*(.vector_0))\n")
        outfile.write ("    KEEP (*(.vector_1))\n")
        outfile.write ("    KEEP (*(.vector_2))\n")
        outfile.write ("    KEEP (*(.vector_3))\n")
        outfile.write ("    KEEP (*(.vector_4))\n")
        outfile.write ("    KEEP (*(.vector_5))\n")
        outfile.write ("    KEEP (*(.vector_6))\n")
        outfile.write ("    KEEP (*(.vector_7))\n")
        outfile.write ("    KEEP (*(.vector_8))\n")
        outfile.write ("    KEEP (*(.vector_9))\n")
        outfile.write ("    KEEP (*(.vector_10))\n")
        outfile.write ("    KEEP (*(.vector_11))\n")
        outfile.write ("    KEEP (*(.vector_12))\n")
        outfile.write ("    KEEP (*(.vector_13))\n")
        outfile.write ("    KEEP (*(.vector_14))\n")
        outfile.write ("    KEEP (*(.vector_15))\n")
        outfile.write ("    KEEP (*(.vector_16))\n")
        outfile.write ("    KEEP (*(.vector_17))\n")
        outfile.write ("    KEEP (*(.vector_18))\n")
        outfile.write ("    KEEP (*(.vec_default))\n")
        outfile.write ("  }\n")

    if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
        outfile.write ("\n  /*  The startup code is in the .reset.startup section.\n")
        outfile.write ("  *  SRAM Application startup code does NOT need to be \n")
        outfile.write ("  *  located at the beginning of CODE SRAM. A processor/chip\n")
        outfile.write ("  *  reset will go to the BootROM reset/startup code and \n")
        outfile.write ("  *  begin the BootROM SPI application load sequence.\n")
        outfile.write ("  *  Upon a successful SPI load and verification, BootROM \n")
        outfile.write ("  *  will jump into the Application. We expect the jump address \n")
        outfile.write ("  *  to be .startup(reset handler) of the application because \n")
        outfile.write ("  *  .startup runs the XC32 startup code and calls C main.\n")
        outfile.write ("  *  Since application .startup is never entered on a real HW \n")
        outfile.write ("  *  reset/nmi/soft-reset it can be located anywhere in SRAM \n")
        outfile.write ("  *  CODE space.\n")
        outfile.write ("  */\n")
    else :
        outfile.write ("\n  /*  The startup code is in the .reset.startup section.\n")
        outfile.write ("   *  Keep this here for backwards compatibility with older\n")
        outfile.write ("   *  C32 v1.xx releases.\n")
        outfile.write ("   */\n")

#    if (((namewithoutpic.upper().find("32") == 0) or    (namewithoutpic.upper().find("MGC3") == 0) or (namewithoutpic.upper().find("BT55") == 0)) and
#      (DefaultMemRegions.startup == "kseg0_boot_mem")):
#        outfile.write ("  .startup ORIGIN(kseg0_boot_mem) :\n")
#    else:
#        outfile.write ("  .startup :\n")
    outfile.write ("  .startup :\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP(*(.startup))\n")
    if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())) :
        outfile.write ("    KEEP(*(.reset.startup))\n")
#    if ((namewithoutpic.upper().find("32") == 0) or (namewithoutpic.upper().find("MGC3") == 0) or (namewithoutpic.upper().find("BT55") == 0)):
#        outfile.write ("  } > %s\n" % DefaultMemRegions.startup)
#    else:
#        outfile.write ("  } > kseg0_program_mem\n")
    outfile.write ("  }\n")

#    outfile.write ("\n  /* Code Sections - Note that input sections *(.text) and *(.text.*)\n")
#    outfile.write ("   * are not mapped here. The best-fit allocator locates them,\n")
#    outfile.write ("   * so that .text may flow around absolute sections as needed.\n")
#    outfile.write ("   */\n")
    outfile.write ("  .text :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.text.unlikely .text.*_unlikely .text.unlikely.*)\n")
    outfile.write ("    *(.text.exit .text.exit.*)\n")
    outfile.write ("    *(.text.hot .text.hot.*)\n")
    outfile.write ("    *(.text .stub .text.* .gnu.linkonce.t.*)\n")
    outfile.write ("    KEEP (*(.text.*personality*))\n")
    outfile.write ("    *(.mips16.fn.*)\n")
    outfile.write ("    *(.mips16.call.*)\n")
    outfile.write ("    *(.gnu.warning)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  /* Global-namespace object initialization */\n")
    outfile.write ("  .init   :\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*crti.o(.init))\n")
    outfile.write ("    KEEP (*crtbegin.o(.init))\n")
    outfile.write ("    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o *crtn.o ).init))\n")
    outfile.write ("    KEEP (*crtend.o(.init))\n")
    outfile.write ("    KEEP (*crtn.o(.init))\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .fini   :\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*(.fini))\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .preinit_array   :\n")
    outfile.write ("  {\n")
    outfile.write ("    PROVIDE_HIDDEN (__preinit_array_start = .);\n")
    outfile.write ("    KEEP (*(.preinit_array))\n")
    outfile.write ("    PROVIDE_HIDDEN (__preinit_array_end = .);\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .init_array   :\n")
    outfile.write ("  {\n")
    outfile.write ("    PROVIDE_HIDDEN (__init_array_start = .);\n")
    outfile.write ("    KEEP (*(SORT(.init_array.*)))\n")
    outfile.write ("    KEEP (*(.init_array))\n")
    outfile.write ("    PROVIDE_HIDDEN (__init_array_end = .);\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .fini_array   :\n")
    outfile.write ("  {\n")
    outfile.write ("    PROVIDE_HIDDEN (__fini_array_start = .);\n")
    outfile.write ("    KEEP (*(SORT(.fini_array.*)))\n")
    outfile.write ("    KEEP (*(.fini_array))\n")
    outfile.write ("    PROVIDE_HIDDEN (__fini_array_end = .);\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .ctors   :\n")
    outfile.write ("  {\n")
    outfile.write ("    /* XC32 uses crtbegin.o to find the start of\n")
    outfile.write ("       the constructors, so we make sure it is\n")
    outfile.write ("       first.  Because this is a wildcard, it\n")
    outfile.write ("       doesn't matter if the user does not\n")
    outfile.write ("       actually link against crtbegin.o; the\n")
    outfile.write ("       linker won't look for a file to match a\n")
    outfile.write ("       wildcard.  The wildcard also means that it\n")
    outfile.write ("       doesn't matter which directory crtbegin.o\n")
    outfile.write ("       is in.  */\n")
    outfile.write ("    KEEP (*crtbegin.o(.ctors))\n")
    outfile.write ("    KEEP (*crtbegin?.o(.ctors))\n")
    outfile.write ("    /* We don't want to include the .ctor section from\n")
    outfile.write ("       the crtend.o file until after the sorted ctors.\n")
    outfile.write ("       The .ctor section from the crtend file contains the\n")
    outfile.write ("       end of ctors marker and it must be last */\n")
    outfile.write ("    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))\n")
    outfile.write ("    KEEP (*(SORT(.ctors.*)))\n")
    outfile.write ("    KEEP (*(.ctors))\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .dtors   :\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*crtbegin.o(.dtors))\n")
    outfile.write ("    KEEP (*crtbegin?.o(.dtors))\n")
    outfile.write ("    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))\n")
    outfile.write ("    KEEP (*(SORT(.dtors.*)))\n")
    outfile.write ("    KEEP (*(.dtors))\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  /* Read-only sections */\n")
    outfile.write ("  .rodata   :\n")
    outfile.write ("  {\n")
    outfile.write ("    *( .gnu.linkonce.r.*)\n")
    outfile.write ("    *(.rodata1)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  /*\n")
    outfile.write ("   * Small initialized constant global and static data can be placed in the\n")
    outfile.write ("   * .sdata2 section.  This is different from .sdata, which contains small\n")
    outfile.write ("   * initialized non-constant global and static data.\n")
    outfile.write ("   */\n")
    outfile.write ("  .sdata2 ALIGN(4) :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  /*\n")
    outfile.write ("   * Uninitialized constant global and static data (i.e., variables which will\n")
    outfile.write ("   * always be zero).  Again, this is different from .sbss, which contains\n")
    outfile.write ("   * small non-initialized, non-constant global and static data.\n")
    outfile.write ("   */\n")
    outfile.write ("  .sbss2 ALIGN(4) :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .eh_frame_hdr   :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.eh_frame_hdr)\n")
    outfile.write ("  }\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  .eh_frame   : ONLY_IF_RO\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*(.eh_frame))\n")
    outfile.write ("  }\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  .gcc_except_table   : ONLY_IF_RO\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.gcc_except_table .gcc_except_table.*)\n")
    outfile.write ("  }\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  .gnu_extab   : ONLY_IF_RO\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.gnu_extab*)\n")
    outfile.write ("  }\n")
    outfile.write ("    . = ALIGN(4) ;\n")

    if "PIC32" in pic.name.upper() or "MGC3" in pic.name.upper() or "BT55" in pic.name.upper() or "WFI" in pic.name.upper():
        outfile.write ("  .dbg_data (NOLOAD) :\n")
        outfile.write ("  {\n")
        outfile.write ("    . += (DEFINED (_DEBUGGER) ? 0x200 : 0x0);\n")
        if pic.instructionSet.hasDSPr2:
            outfile.write ("    /* Additional data memory required for DSPr2 registers */\n")
            outfile.write ("    . += (DEFINED (_DEBUGGER) ? 0x80 : 0x0);\n")
        if (fdom.first(pic.node, 'FPU') is not None):
            outfile.write ("    /* Additional data memory required for FPU64 registers */\n")
            outfile.write ("    . += (DEFINED (_DEBUGGER) ? 0x100 : 0x0);\n")
        outfile.write ("  }\n")

    outfile.write ("  . = .;\n")
    outfile.write ("  _gp = ALIGN(4) ;\n")
    outfile.write ("  .got ALIGN(4) :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.got)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .jcr   :\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*(.jcr))\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .eh_frame    : ONLY_IF_RW\n")
    outfile.write ("  {\n")
    outfile.write ("    KEEP (*(.eh_frame))\n")
    outfile.write ("    KEEP (*(.eh_frame.*))\n")
    outfile.write ("  }\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  .gcc_except_table    : ONLY_IF_RW\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.gcc_except_table .gcc_except_table.*)\n")
    outfile.write ("  }\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  .gnu_extab      : ONLY_IF_RW { *(.gnu_extab) }\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  .exception_ranges   : ONLY_IF_RW { *(.exception_ranges .exception_ranges*) }\n")
    outfile.write ("    . = ALIGN(4) ;\n")

    outfile.write ("  /* Persistent data - Use the new C 'persistent' attribute instead. */\n")
    outfile.write ("  .persist   :\n")
    outfile.write ("  {\n")
    outfile.write ("    _persist_begin = .;\n")
    outfile.write ("    *(.persist .persist.*)\n")
    outfile.write ("    *(.pbss .pbss.*)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("    _persist_end = .;\n")
    outfile.write ("  }\n")
    outfile.write ("  .data.rel.ro : { *(.data.rel.ro.local* .gnu.linkonce.d.rel.ro.local.*) *(.data.rel.ro .data.rel.ro.* .gnu.linkonce.d.rel.ro.*) }\n")
    outfile.write ("  .got.plt        : { *(.got.plt) }\n")
    outfile.write ("  .data   :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.data .data.* .gnu.linkonce.d.*)\n")
    outfile.write ("    SORT(CONSTRUCTORS)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .data1          : { *(.data1) }\n")
    outfile.write ("  . = .;\n")
    outfile.write ("  /*\n")
    outfile.write ("   * Note that 'small' data sections are still mapped in the linker\n")
    outfile.write ("   * script. This ensures that they are grouped together for\n")
    outfile.write ("   * gp-relative addressing. Absolute sections are allocated after\n")
    outfile.write ("   * the 'small' data sections so small data cannot flow around them.\n")
    outfile.write ("   */\n")
    outfile.write ("  /*\n")
    outfile.write ("   * We want the small data sections together, so single-instruction offsets\n")
    outfile.write ("   * can access them all, and initialized data all before uninitialized, so\n")
    outfile.write ("   * we can shorten the on-disk segment size.\n")
    outfile.write ("   */\n")
    outfile.write ("  .sdata ALIGN(4) :\n")
    outfile.write ("  {\n")
    outfile.write ("    _sdata_begin = . ;\n")
    outfile.write ("    *(.sdata .sdata.* .gnu.linkonce.s.*)\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("    _sdata_end = . ;\n")
    outfile.write ("  }\n")
    outfile.write ("  .lit8           :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.lit8)\n")
    outfile.write ("  }\n")
    outfile.write ("  .lit4           :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.lit4)\n")
    outfile.write ("  }\n")
    outfile.write ("  . = ALIGN (4) ;\n")
    outfile.write ("  _data_end = . ;\n")
    outfile.write ("  _bss_begin = . ;\n")
    outfile.write ("  .sbss ALIGN(4) :\n")
    outfile.write ("  {\n")
    outfile.write ("    _sbss_begin = . ;\n")
    outfile.write ("    *(.dynsbss)\n")
    outfile.write ("    *(.sbss .sbss.* .gnu.linkonce.sb.*)\n")
    outfile.write ("    *(.scommon)\n")
    outfile.write ("    _sbss_end = . ;\n")
    outfile.write ("    . = ALIGN(4) ;\n")
    outfile.write ("  }\n")
    outfile.write ("  /*\n")
    outfile.write ("   *  Align here to ensure that the .bss section occupies space up to\n")
    outfile.write ("   *  _end.  Align after .bss to ensure correct alignment even if the\n")
    outfile.write ("   *  .bss section disappears because there are no input sections.\n")
    outfile.write ("   *\n")
    outfile.write ("   *  Note that input sections named .bss* are no longer mapped here.\n")
    outfile.write ("   *  The best-fit allocator locates them, so that they may flow\n")
    outfile.write ("   *  around absolute sections as needed.\n")
    outfile.write ("   *\n")
    outfile.write ("   */\n")
    outfile.write ("  .bss     :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.dynbss)\n")
    outfile.write ("    *(.bss .bss.* .gnu.linkonce.b.*)\n")
    outfile.write ("    *(COMMON)\n")
    outfile.write ("   /* Align here to ensure that the .bss section occupies space up to\n")
    outfile.write ("      _end.  Align after .bss to ensure correct alignment even if the\n")
    outfile.write ("      .bss section disappears because there are no input sections. */\n")
    outfile.write ("   . = ALIGN(. != 0 ? 4 : 1);\n")
    outfile.write ("  }\n")
    outfile.write ("  . = ALIGN(4) ;\n")
    outfile.write ("  _end = . ;\n")
    outfile.write ("  _bss_end = . ;\n")
#    outfile.write ("  . = DATA_SEGMENT_END (.);\n")
#    outfile.write ("  /*\n")
#    outfile.write ("   *  The heap and stack are best-fit allocated by the linker after other\n")
#    outfile.write ("   *  data and bss sections have been allocated.\n")
#    outfile.write ("   */\n")
#    outfile.write ("  /*\n")
#    outfile.write ("   * RAM functions go at the end of our stack and heap allocation.\n")
#    outfile.write ("   * Alignment of 2K required by the boundary register (BMXDKPBA).\n")
#    outfile.write ("   *\n")
#    outfile.write ("   * RAM functions are now allocated by the linker. The linker generates\n")
#    outfile.write ("   * _ramfunc_begin and _bmxdkpba_address symbols depending on the\n")
#    outfile.write ("   * location of RAM functions.\n")
#    outfile.write ("   */\n")
#    outfile.write ("  _bmxdudba_address = LENGTH(%s) ;\n" % DefaultMemRegions.data)
#    outfile.write ("  _bmxdupba_address = LENGTH(%s) ;\n" % DefaultMemRegions.data)
    outfile.write ("    /* The .pdr section belongs in the absolute section */\n")
    outfile.write ("    /DISCARD/ : { *(.pdr) }\n")
    outfile.write ("  .gptab.sdata : { *(.gptab.data) *(.gptab.sdata) }\n")
    outfile.write ("  .gptab.sbss : { *(.gptab.bss) *(.gptab.sbss) }\n")
    outfile.write ("  .mdebug.abi32 : { KEEP(*(.mdebug.abi32)) }\n")
    outfile.write ("  .mdebug.abiN32 : { KEEP(*(.mdebug.abiN32)) }\n")
    outfile.write ("  .mdebug.abi64 : { KEEP(*(.mdebug.abi64)) }\n")
    outfile.write ("  .mdebug.abiO64 : { KEEP(*(.mdebug.abiO64)) }\n")
    outfile.write ("  .mdebug.eabi32 : { KEEP(*(.mdebug.eabi32)) }\n")
    outfile.write ("  .mdebug.eabi64 : { KEEP(*(.mdebug.eabi64)) }\n")
    outfile.write ("  .gcc_compiled_long32 : { KEEP(*(.gcc_compiled_long32)) }\n")
    outfile.write ("  .gcc_compiled_long64 : { KEEP(*(.gcc_compiled_long64)) }\n")
    outfile.write ("  /* Stabs debugging sections.  */\n")
    outfile.write ("  .stab          0 : { *(.stab) }\n")
    outfile.write ("  .stabstr       0 : { *(.stabstr) }\n")
    outfile.write ("  .stab.excl     0 : { *(.stab.excl) }\n")
    outfile.write ("  .stab.exclstr  0 : { *(.stab.exclstr) }\n")
    outfile.write ("  .stab.index    0 : { *(.stab.index) }\n")
    outfile.write ("  .stab.indexstr 0 : { *(.stab.indexstr) }\n")
    outfile.write ("  .comment       0 : { *(.comment) }\n")
    outfile.write ("  /* DWARF debug sections used by MPLAB X for source-level debugging. \n")
    outfile.write ("     Symbols in the DWARF debugging sections are relative to the beginning\n")
    outfile.write ("     of the section so we begin them at 0.  */\n")
    outfile.write ("  /* DWARF 1 */\n")
    outfile.write ("  .debug          0 : { *(.debug) }\n")
    outfile.write ("  .line           0 : { *(.line) }\n")
    outfile.write ("  /* GNU DWARF 1 extensions */\n")
    outfile.write ("  .debug_srcinfo  0 : { *(.debug_srcinfo) }\n")
    outfile.write ("  .debug_sfnames  0 : { *(.debug_sfnames) }\n")
    outfile.write ("  /* DWARF 1.1 and DWARF 2 */\n")
    outfile.write ("  .debug_aranges  0 : { *(.debug_aranges) }\n")
    outfile.write ("  .debug_pubnames 0 : { *(.debug_pubnames) }\n")
    outfile.write ("  /* DWARF 2 */\n")
    outfile.write ("  .debug_info     0 : { *(.debug_info .gnu.linkonce.wi.*) }\n")
    outfile.write ("  .debug_abbrev   0 : { *(.debug_abbrev) }\n")
    outfile.write ("  .debug_line     0 : { *(.debug_line) }\n")
    outfile.write ("  .debug_frame    0 : { *(.debug_frame) }\n")
    outfile.write ("  .debug_str      0 : { *(.debug_str) }\n")
    outfile.write ("  .debug_loc      0 : { *(.debug_loc) }\n")
    outfile.write ("  .debug_macinfo  0 : { *(.debug_macinfo) }\n")
    outfile.write ("  /* SGI/MIPS DWARF 2 extensions */\n")
    outfile.write ("  .debug_weaknames 0 : { *(.debug_weaknames) }\n")
    outfile.write ("  .debug_funcnames 0 : { *(.debug_funcnames) }\n")
    outfile.write ("  .debug_typenames 0 : { *(.debug_typenames) }\n")
    outfile.write ("  .debug_varnames  0 : { *(.debug_varnames) }\n")
    outfile.write ("  .debug_pubtypes 0 : { *(.debug_pubtypes) }\n")
    outfile.write ("  .debug_ranges   0 : { *(.debug_ranges) }\n")
#    outfile.write ("  /DISCARD/ : { *(.rel.dyn) }\n")
    outfile.write ("  .gnu.attributes 0 : { KEEP (*(.gnu.attributes)) }\n")
    outfile.write ("  /DISCARD/ : { *(.note.GNU-stack) }\n")
    outfile.write ("  /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) *(.discard) }\n")
    outfile.write ("}\n\n")


def getrelsections_pic (pic, outfile):

    outfile.write ("\nSECTIONS\n")
    outfile.write ("{\n")
    outfile.write ("  /* Read-only sections, merged into text segment: */\n")
    outfile.write ("  . = SEGMENT_START(\"text-segment\", 0) + SIZEOF_HEADERS;\n")
    outfile.write ("  .reginfo        : { *(.reginfo) }\n")
    outfile.write ("  .note.gnu.build-id : { *(.note.gnu.build-id) }\n")
    outfile.write ("  .dynamic        : { *(.dynamic) }\n")
    outfile.write ("  .hash           : { *(.hash) }\n")
    outfile.write ("  .gnu.hash       : { *(.gnu.hash) }\n")
    outfile.write ("  .dynsym         : { *(.dynsym) }\n")
    outfile.write ("  .dynstr         : { *(.dynstr) }\n")
    outfile.write ("  .gnu.version    : { *(.gnu.version) }\n")
    outfile.write ("  .gnu.version_d  : { *(.gnu.version_d) }\n")
    outfile.write ("  .gnu.version_r  : { *(.gnu.version_r) }\n")
    outfile.write ("  .rel.dyn        :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.rel.init)\n")
    outfile.write ("    *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*)\n")
    outfile.write ("    *(.rel.fini)\n")
    outfile.write ("    *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)\n")
    outfile.write ("    *(.rel.data.rel.ro .rel.data.rel.ro.* .rel.gnu.linkonce.d.rel.ro.*)\n")
    outfile.write ("    *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)\n")
    outfile.write ("    *(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*)\n")
    outfile.write ("    *(.rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*)\n")
    outfile.write ("    *(.rel.ctors)\n")
    outfile.write ("    *(.rel.dtors)\n")
    outfile.write ("    *(.rel.got)\n")
    outfile.write ("    *(.rel.dyn)\n")
    outfile.write ("    *(.rel.sdata .rel.sdata.* .rel.gnu.linkonce.s.*)\n")
    outfile.write ("    *(.rel.sbss .rel.sbss.* .rel.gnu.linkonce.sb.*)\n")
    outfile.write ("    *(.rel.sdata2 .rel.sdata2.* .rel.gnu.linkonce.s2.*)\n")
    outfile.write ("    *(.rel.sbss2 .rel.sbss2.* .rel.gnu.linkonce.sb2.*)\n")
    outfile.write ("    *(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)\n")
    outfile.write ("    *(.rel.iplt)\n")
    outfile.write ("  }\n")

    outfile.write ("  .rela.dyn       :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.rela.init)\n")
    outfile.write ("    *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)\n")
    outfile.write ("    *(.rela.fini)\n")
    outfile.write ("    *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)\n")
    outfile.write ("    *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)\n")
    outfile.write ("    *(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)\n")
    outfile.write ("    *(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*)\n")
    outfile.write ("    *(.rela.ctors)\n")
    outfile.write ("    *(.rela.dtors)\n")
    outfile.write ("    *(.rela.got)\n")
    outfile.write ("    *(.rela.sdata .rela.sdata.* .rela.gnu.linkonce.s.*)\n")
    outfile.write ("    *(.rela.sbss .rela.sbss.* .rela.gnu.linkonce.sb.*)\n")
    outfile.write ("    *(.rela.sdata2 .rela.sdata2.* .rela.gnu.linkonce.s2.*)\n")
    outfile.write ("    *(.rela.sbss2 .rela.sbss2.* .rela.gnu.linkonce.sb2.*)\n")
    outfile.write ("    *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)\n")
    outfile.write ("    *(.rela.iplt)\n")
    outfile.write ("  }\n")

    outfile.write ("  .rel.plt        :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.rel.plt)\n")
    outfile.write ("  }\n")
    outfile.write ("  .rela.plt       :\n")
    outfile.write ("  {\n")
    outfile.write ("    *(.rela.plt)\n")
    outfile.write ("  }\n")
    outfile.write ("}\n")


def getinitsymbols_cache(pic, lfile):
    if not (pic.hasL1Cache):
        return;

    lfile.write ("/*************************************************************************\n")
    lfile.write (" *                    L1 Cache initialization symbols\n")
    lfile.write (" *************************************************************************/\n")
    # Symbols used for L1 cache initialization in startup code
    if pic.hasL1Cache:
        dcache_base_addr = 0
        icache_base_addr = 0
        for region_id, region_node in pic.idOntoRegion.iteritems():
            begin=fdom.get2(region_node, "beginaddr", edc.str2int, defaultValue="")
            end=fdom.get2(region_node, "endaddr", edc.str2int, defaultValue="")
            if fdom.hasLocalName(region_node,'GPRDataSector'):
                if region_id == 'kseg0_data_mem':
                    gprksegoffset=fdom.get2(region_node,"kseg0", edc.str2int)
                    if (gprksegoffset):
                        dcache_base_addr = begin + gprksegoffset
            if fdom.hasLocalName(region_node,'CodeSector'):
                if region_id == 'code':
                    gprksegoffset=fdom.get2(region_node,"kseg0", edc.str2int)
                    if (gprksegoffset):
                        icache_base_addr = begin + gprksegoffset
        ##--------------------------------------------------------------
        lfile.write ("/*\n")
        lfile.write (" *  Provide virtual addresses for cache initialization\n")
        lfile.write (" *  These symbols are used by the pic32_init_cache.o module to set up\n")
        lfile.write (" *  the cache at startup.\n")
        lfile.write (" */\n")
        lfile.write("EXTERN  (__pic32_init_cache_program_base_addr)\n")
        lfile.write("PROVIDE (__pic32_init_cache_program_base_addr = 0x" + str("%08X" % icache_base_addr) + ") ;\n")
        lfile.write("EXTERN  (__pic32_init_cache_data_base_addr)\n")
        lfile.write("PROVIDE (__pic32_init_cache_data_base_addr = 0x" + str("%08X" % dcache_base_addr) + ") ;\n\n")
        ##--------------------------------------------------------------------

def getinitsymbols_tlb(pic, lfile):
    tlb_init_entrynum = 0

    #ZAA - additional output
    # if there is SQI block...
    sqi_kseg2_virtual_base_addr = 0x0
    for region_id, region_node in pic.idOntoRegion.iteritems():
        begin=fdom.get2(region_node,"beginaddr", edc.str2int, defaultValue="")
        end=fdom.get2(region_node,"endaddr", edc.str2int, defaultValue="")
        if fdom.hasLocalName(region_node,'SQIDataSector'):
            # cached mapping for SQI
            #ns_uri, ns_prefix, attr = 'http://crownking/edc', 'edc', 'kseg2'
            #if region_node.hasAttributeNS(ns_uri, attr):
                #gprksegoffset = edc.str2int(region_node.getAttributeNS(ns_uri, attr))
            gprksegoffset=fdom.get2(region_node,"kseg2", edc.str2int)
            if (gprksegoffset):
                sqi_kseg2_virtual_base_addr = begin + gprksegoffset
                ##--------------------------------------------------------------
                lfile.write ("/*************************************************************************\n")
                lfile.write (" *   TLB-Based MMU Initialization section for EBI/SQI memory regions\n")
                lfile.write (" *************************************************************************/\n")
                lfile.write ("/*\n")
                lfile.write (" *  Provide virtual and physical addresses of the SQI for TLB initialization\n")
                lfile.write (" *  These symbols are used below to create a table, which is in turn used to\n")
                lfile.write (" *  initialize the TLB at startup.\n")
                lfile.write (" */\n")
                lfile.write("EXTERN  (__pic32_init_sqi_kseg2_virtual_base_addr)\n")
                lfile.write("PROVIDE (__pic32_init_sqi_kseg2_virtual_base_addr = 0x" + str("%08X" % sqi_kseg2_virtual_base_addr) + ") ;\n")
                lfile.write("EXTERN  (__pic32_init_sqi_kseg2_physical_base_addr)\n")
                lfile.write("PROVIDE (__pic32_init_sqi_kseg2_physical_base_addr = 0x" + str("%08X" % begin) + ") ;\n")
                lfile.write("EXTERN  (__pic32_init_sqi_kseg2_entrylo0_bitset)\n")
                lfile.write("PROVIDE (__pic32_init_sqi_kseg2_entrylo0_bitset = 0x1F) ;\n")
                lfile.write("\n/*  ENTRYHI = VPN2<31:13> VPN2X<12:11> 0<10:8> ASID<7:0> */\n")
                lfile.write("__pic32_tlb_%d_entryhi  = __pic32_init_sqi_kseg2_virtual_base_addr ;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  Create one page table entry consisting of two 16MB physical pages. \n")
                lfile.write(" *  ENTRYLO0 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> \n")
                lfile.write(" *  Shift __pic32_init_sqi_kseg3_physical_base_addr to PFN. \n")
                lfile.write(" *  Bitwise-OR the other bits of ENTRYLO0.\n")
                lfile.write(" */\n")
                lfile.write("__pic32_tlb_%d_entrylo0 = (__pic32_init_sqi_kseg2_physical_base_addr >> 6) | __pic32_init_sqi_kseg2_entrylo0_bitset ;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  ENTRYLO1 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> */\n")
                lfile.write("__pic32_tlb_%d_entrylo1 = __pic32_tlb_%d_entrylo0 | ((__pic32_init_sqi_kseg2_physical_base_addr + 0x1000000) >> 6) ;\n\n" %  (tlb_init_entrynum, tlb_init_entrynum))
                tlb_init_entrynum = tlb_init_entrynum+1
                lfile.write("\n/*  ENTRYHI = VPN2<31:13> VPN2X<12:11> 0<10:8> ASID<7:0> */\n")
                lfile.write("__pic32_tlb_%d_entryhi  = __pic32_init_sqi_kseg2_virtual_base_addr + 0x2000000;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  Create one page table entry consisting of two 16MB physical pages. \n")
                lfile.write(" *  ENTRYLO0 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> \n")
                lfile.write(" *  Shift __pic32_init_sqi_kseg3_physical_base_addr to PFN. \n")
                lfile.write(" *  Bitwise-OR the other bits of ENTRYLO0.\n")
                lfile.write(" */\n")
                lfile.write("__pic32_tlb_%d_entrylo0 = ((__pic32_init_sqi_kseg2_physical_base_addr + 0x2000000) >> 6) | __pic32_init_sqi_kseg2_entrylo0_bitset;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  ENTRYLO1 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> */\n")
                lfile.write("__pic32_tlb_%d_entrylo1 = __pic32_tlb_%d_entrylo0 | 0x00040000 ;\n\n" % (tlb_init_entrynum, tlb_init_entrynum))
                tlb_init_entrynum = tlb_init_entrynum+1
                ##--------------------------------------------------------------------
            # uncached mapping for SQI
            #ns_uri, ns_prefix, attr = 'http://crownking/edc', 'edc', 'kseg3'
            #if region_node.hasAttributeNS(ns_uri, attr):
            #gprksegoffset = edc.str2int(region_node.getAttributeNS(ns_uri, attr))
            gprksegoffset=fdom.get2(region_node,"kseg3", edc.str2int)
            if (gprksegoffset):
                sqi_kseg2_virtual_base_addr = begin + gprksegoffset
                ##--------------------------------------------------------------
                lfile.write("EXTERN  (__pic32_init_sqi_kseg3_virtual_base_addr)\n")
                lfile.write("PROVIDE (__pic32_init_sqi_kseg3_virtual_base_addr = 0x" + str("%08X" % sqi_kseg2_virtual_base_addr) + ") ;\n")
                lfile.write("EXTERN  (__pic32_init_sqi_kseg3_physical_base_addr)\n")
                lfile.write("PROVIDE (__pic32_init_sqi_kseg3_physical_base_addr = 0x" + str("%08X" % begin) + ") ;\n")
                lfile.write("EXTERN  (__pic32_init_sqi_kseg3_entrylo0_bitset)\n")
                lfile.write("PROVIDE (__pic32_init_sqi_kseg3_entrylo0_bitset = 0x17) ;\n")
                lfile.write("\n/*  ENTRYHI = VPN2<31:13> VPN2X<12:11> 0<10:8> ASID<7:0> */\n")
                lfile.write("__pic32_tlb_%d_entryhi  = __pic32_init_sqi_kseg3_virtual_base_addr ;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  Create one page table entry consisting of two 16MB physical pages. \n")
                lfile.write(" *  ENTRYLO0 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> \n")
                lfile.write(" *  Shift __pic32_init_sqi_kseg3_physical_base_addr to PFN. \n")
                lfile.write(" *  Bitwise-OR the other bits of ENTRYLO0. \n")
                lfile.write(" */\n")
                lfile.write("__pic32_tlb_%d_entrylo0 = (__pic32_init_sqi_kseg3_physical_base_addr >> 6) | __pic32_init_sqi_kseg3_entrylo0_bitset ;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  ENTRYLO1 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> */\n")
                lfile.write("__pic32_tlb_%d_entrylo1 = __pic32_tlb_%d_entrylo0 | ((__pic32_init_sqi_kseg3_physical_base_addr + 0x1000000) >> 6) ;\n\n" %  (tlb_init_entrynum, tlb_init_entrynum))
                tlb_init_entrynum = tlb_init_entrynum+1
                lfile.write("\n/*  ENTRYHI = VPN2<31:13> VPN2X<12:11> 0<10:8> ASID<7:0> */\n")
                lfile.write("__pic32_tlb_%d_entryhi  = __pic32_init_sqi_kseg3_virtual_base_addr + 0x2000000;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  Create one page table entry consisting of two 16MB physical pages. \n")
                lfile.write(" *  ENTRYLO0 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> \n")
                lfile.write(" *  Shift __pic32_init_sqi_kseg3_physical_base_addr to PFN. \n")
                lfile.write(" *  Bitwise-OR the other bits of ENTRYLO0.\n")
                lfile.write(" */\n")
                lfile.write("__pic32_tlb_%d_entrylo0 = ((__pic32_init_sqi_kseg3_physical_base_addr + 0x2000000) >> 6) | __pic32_init_sqi_kseg3_entrylo0_bitset;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  ENTRYLO1 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> */\n")
                lfile.write("__pic32_tlb_%d_entrylo1 = __pic32_tlb_%d_entrylo0 | 0x00040000 ;\n\n" % (tlb_init_entrynum, tlb_init_entrynum))
                tlb_init_entrynum = tlb_init_entrynum+1
                ##--------------------------------------------------------------------

    #ZAA - additional output
    # if there is EBI block...
    ebi_kseg2_virtual_base_addr = 0x0
    for region_id, region_node in pic.idOntoRegion.iteritems():
        begin=fdom.get2(region_node,"beginaddr", edc.str2int, defaultValue="")
        end=fdom.get2(region_node,"endaddr", edc.str2int, defaultValue="")
        if fdom.hasLocalName(region_node,'EBIDataSector'):
            # cached mapping for EBI
            #ns_uri, ns_prefix, attr = 'http://crownking/edc', 'edc', 'kseg2'
            #if region_node.hasAttributeNS(ns_uri, attr):
            #gprksegoffset = edc.str2int(region_node.getAttributeNS(ns_uri, attr))
            gprksegoffset=fdom.get2(region_node,"kseg2", edc.str2int)
            if (gprksegoffset):
                ebi_kseg2_virtual_base_addr = begin + gprksegoffset
                ##--------------------------------------------------------------
                lfile.write ("/*\n")
                lfile.write (" *  Provide virtual and physical addresses of the EBI for TLB initialization\n")
                lfile.write (" *  These symbols are used below to create a table, which is in turn used to\n")
                lfile.write (" *  initialize the TLB at startup.\n")
                lfile.write (" */\n")
                lfile.write("EXTERN  (__pic32_init_ebi_kseg2_virtual_base_addr)\n")
                lfile.write("PROVIDE (__pic32_init_ebi_kseg2_virtual_base_addr = 0x" + str("%08X" % ebi_kseg2_virtual_base_addr) + ") ;\n")
                lfile.write("EXTERN  (__pic32_init_ebi_kseg2_physical_base_addr)\n")
                lfile.write("PROVIDE (__pic32_init_ebi_kseg2_physical_base_addr = 0x" + str("%08X" % begin) + ") ;\n")
                lfile.write("EXTERN  (__pic32_init_ebi_kseg2_entrylo0_bitset)\n")
                lfile.write("PROVIDE (__pic32_init_ebi_kseg2_entrylo0_bitset = 0x1F) ;\n")
                lfile.write("\n/*  ENTRYHI = VPN2<31:13> VPN2X<12:11> 0<10:8> ASID<7:0> */\n")
                lfile.write("__pic32_tlb_%d_entryhi  = __pic32_init_ebi_kseg2_virtual_base_addr ;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  Create one page table entry consisting of two 16MB physical pages. \n")
                lfile.write(" *  ENTRYLO0 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> \n")
                lfile.write(" *  Shift __pic32_init_sqi_kseg3_physical_base_addr to PFN. \n")
                lfile.write(" *  Bitwise-OR the other bits of ENTRYLO0.\n")
                lfile.write(" */\n")
                lfile.write("__pic32_tlb_%d_entrylo0 = (__pic32_init_ebi_kseg2_physical_base_addr >> 6) | __pic32_init_ebi_kseg2_entrylo0_bitset ;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  ENTRYLO1 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> */\n")
                lfile.write("__pic32_tlb_%d_entrylo1 = __pic32_tlb_%d_entrylo0 | ((__pic32_init_ebi_kseg2_physical_base_addr + 0x1000000) >> 6) ;\n\n" %  (tlb_init_entrynum, tlb_init_entrynum))
                tlb_init_entrynum = tlb_init_entrynum+1
                lfile.write("\n/*  ENTRYHI = VPN2<31:13> VPN2X<12:11> 0<10:8> ASID<7:0> */\n")
                lfile.write("__pic32_tlb_%d_entryhi  = __pic32_init_ebi_kseg2_virtual_base_addr + 0x2000000;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  Create one page table entry consisting of two 16MB physical pages. \n")
                lfile.write(" *  ENTRYLO0 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> \n")
                lfile.write(" *  Shift __pic32_init_sqi_kseg3_physical_base_addr to PFN. \n")
                lfile.write(" *  Bitwise-OR the other bits of ENTRYLO0.\n")
                lfile.write(" */\n")
                lfile.write("__pic32_tlb_%d_entrylo0 = ((__pic32_init_ebi_kseg2_physical_base_addr + 0x2000000) >> 6) | __pic32_init_ebi_kseg2_entrylo0_bitset;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  ENTRYLO1 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> */\n")
                lfile.write("__pic32_tlb_%d_entrylo1 = __pic32_tlb_%d_entrylo0 | 0x00040000 ;\n\n" % (tlb_init_entrynum, tlb_init_entrynum))
                tlb_init_entrynum = tlb_init_entrynum+1
                ##--------------------------------------------------------------------
            # uncached mapping for EBI
            #ns_uri, ns_prefix, attr = 'http://crownking/edc', 'edc', 'kseg3'
            #if region_node.hasAttributeNS(ns_uri, attr):
            #gprksegoffset = edc.str2int(region_node.getAttributeNS(ns_uri, attr))
            gprksegoffset=fdom.get2(region_node,"kseg3", edc.str2int)
            if (gprksegoffset):
                ebi_kseg2_virtual_base_addr = begin + gprksegoffset
                ##--------------------------------------------------------------
                lfile.write("EXTERN  (__pic32_init_ebi_kseg3_virtual_base_addr)\n")
                lfile.write("PROVIDE (__pic32_init_ebi_kseg3_virtual_base_addr = 0x" + str("%08X" % ebi_kseg2_virtual_base_addr) + ") ;\n")
                lfile.write("EXTERN  (__pic32_init_ebi_kseg3_physical_base_addr)\n")
                lfile.write("PROVIDE (__pic32_init_ebi_kseg3_physical_base_addr = 0x" + str("%08X" % begin) + ") ;\n")
                lfile.write("EXTERN  (__pic32_init_ebi_kseg3_entrylo0_bitset)\n")
                lfile.write("PROVIDE (__pic32_init_ebi_kseg3_entrylo0_bitset = 0x17) ;\n")
                lfile.write("\n/*  ENTRYHI = VPN2<31:13> VPN2X<12:11> 0<10:8> ASID<7:0> */\n")
                lfile.write("__pic32_tlb_%d_entryhi  = __pic32_init_ebi_kseg3_virtual_base_addr ;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  Create one page table entry consisting of two 16MB physical pages. \n")
                lfile.write(" *  ENTRYLO0 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> \n")
                lfile.write(" *  Shift __pic32_init_sqi_kseg3_physical_base_addr to PFN. \n")
                lfile.write(" *  Bitwise-OR the other bits of ENTRYLO0.\n")
                lfile.write(" */\n")
                lfile.write("__pic32_tlb_%d_entrylo0 = (__pic32_init_ebi_kseg3_physical_base_addr >> 6) | __pic32_init_ebi_kseg3_entrylo0_bitset ;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  ENTRYLO1 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> */\n")
                lfile.write("__pic32_tlb_%d_entrylo1 = __pic32_tlb_%d_entrylo0 | ((__pic32_init_ebi_kseg3_physical_base_addr + 0x1000000) >> 6) ;\n\n" %  (tlb_init_entrynum, tlb_init_entrynum))
                tlb_init_entrynum = tlb_init_entrynum+1
                lfile.write("\n/*  ENTRYHI = VPN2<31:13> VPN2X<12:11> 0<10:8> ASID<7:0> */\n")
                lfile.write("__pic32_tlb_%d_entryhi  = __pic32_init_ebi_kseg3_virtual_base_addr + 0x2000000;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  Create one page table entry consisting of two 16MB physical pages. \n")
                lfile.write(" *  ENTRYLO0 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> \n")
                lfile.write(" *  Shift __pic32_init_sqi_kseg3_physical_base_addr to PFN. \n")
                lfile.write(" *  Bitwise-OR the other bits of ENTRYLO0.\n")
                lfile.write(" */\n")
                lfile.write("__pic32_tlb_%d_entrylo0 = ((__pic32_init_ebi_kseg3_physical_base_addr + 0x2000000) >> 6) | __pic32_init_ebi_kseg3_entrylo0_bitset;\n\n" % tlb_init_entrynum)
                lfile.write("\n/*  ENTRYLO1 = RI<31> XI<30> 0<29:26> PFN<25:6> C<5:3> D<2> V<1> G<0> */\n")
                lfile.write("__pic32_tlb_%d_entrylo1 = __pic32_tlb_%d_entrylo0 | 0x00040000 ;\n\n" % (tlb_init_entrynum, tlb_init_entrynum))
                tlb_init_entrynum = tlb_init_entrynum+1
                ##--------------------------------------------------------------------
    if (tlb_init_entrynum > 0):
        lfile.write("/*\n")
        lfile.write(" *  The default pic32_init_tlb_ebi_sqi.o module, which is\n");
        lfile.write(" *  called by the default C startup code (crt0_<isa>.o),\n")
        lfile.write(" *  uses the table created in the .tlb_init_values output\n")
        lfile.write(" *  section to initialize the Translation Lookaside buffer (TLB)\n")
        lfile.write(" *  of the Memory Management Unit (MMU).\n")
        lfile.write(" */\n")
        lfile.write("SECTIONS\n")
        lfile.write("{\n")
        lfile.write("  .tlb_init_values :\n")
        lfile.write("  {\n")
        lfile.write("    . = ALIGN(4) ;\n")
        lfile.write("    __pic32_tlb_init_values_begin = ABSOLUTE(.);\n")
        tlb_entry_index = 0
        while (tlb_entry_index < tlb_init_entrynum):
            lfile.write("    LONG(__pic32_tlb_%d_entryhi) ;\n" % tlb_entry_index)
            lfile.write("    LONG(__pic32_tlb_%d_entrylo0) ;\n" % tlb_entry_index)
            lfile.write("    LONG(__pic32_tlb_%d_entrylo1) ;\n" % tlb_entry_index)
            tlb_entry_index += 1
        lfile.write("    __pic32_tlb_init_values_end = ABSOLUTE(.);\n")
        lfile.write("    __pic32_tlb_init_values_count = %d ;\n" % tlb_entry_index)
        lfile.write("  } > kseg1_boot_mem_4B0\n")
        lfile.write("}\n\n")

def writeLinker(pic, lfile):
    lfile.write ("/*--------------------------------------------------------------------------\n")
    lfile.write (" * MPLAB XC Compiler -  %s linker script\n" % pic.name.upper())
    lfile.write (" * Build date : %s\n" % time.strftime("%b %d %Y"))
    lfile.write (" * \n")
    lfile.write (" * Copyright (c) %s, Microchip Technology Inc. and its subsidiaries (\"Microchip\")\n" %time.strftime("%Y"))
    lfile.write (" * All rights reserved.\n")
    lfile.write (" * \n")
    langcommon.printBoilerplate1Info(lfile)

    lfile.write ("/* Default linker script, for normal executables */\n\n")

    if pic.interrupts.hasVariableOffsets :
        lfile.write ("/*  NOTE: This single-file linker script replaces the two-file system used\n")
        lfile.write (" *  for older PIC32 devices. \n")
        lfile.write (" */\n\n")
    elif "PIC32MX" in pic.name.upper():
        lfile.write ("/*  IMPORTANT: PIC32MX MCUs use two files for the default linker script: \n")
        lfile.write (" *    1) pic32mx/lib/ldscripts/elf32pic32.x (main linker script)\n")
        lfile.write (" *    2) pic32mx/lib/proc/%s/procdefs.ld (variant-specific fragment)\n" % namewithoutpic)
        lfile.write (" *\n")
        lfile.write (" *  This file is provided only as a convenience when adding a custom linker script\n")
        lfile.write (" *  to your application.\n")
        lfile.write (" */\n\n")

    lfile.write ("OUTPUT_FORMAT(\"elf32-tradlittlemips\")\n")
    lfile.write ("OUTPUT_ARCH(pic32mx)\n")
    lfile.write ("ENTRY(_reset)\n")
    lfile.write ("/*\n")
    lfile.write (" * Provide for a minimum stack and heap size\n")
    lfile.write (" * - _min_stack_size - represents the minimum space that must be made\n")
    lfile.write (" *                     available for the stack.  Can be overridden from\n")
    lfile.write (" *                     the command line using the linker's --defsym option.\n")
    lfile.write (" * - _min_heap_size  - represents the minimum space that must be made\n")
    lfile.write (" *                     available for the heap.  Must be specified on\n")
    lfile.write (" *                     the command line using the linker's --defsym option.\n")
    lfile.write (" */\n")
    lfile.write ("EXTERN (_min_stack_size _min_heap_size)\n")
    lfile.write ("PROVIDE(_min_stack_size = 0x400) ;\n")

    if (("MEC14" in pic.name.upper()) or ("USB" in pic.name.upper())):
        lfile.write ("PROVIDE(_min_heap_size = 0x400) ;\n")
    lfile.write ("\n");

    getobjfile (pic,lfile)
    if (pic.interrupts.hasVariableOffsets) :
        lfile.write ("\n/*************************************************************************\n")
        lfile.write (" * Vector-offset initialization\n")
        lfile.write (" *************************************************************************/\n")
        lfile.write ("OPTIONAL(\"vector_offset_init.o\")\n")
        lfile.write ("\n")
    print "  Generating int vectors"
    getintvector (pic, lfile)
    print "  Generating mem equates"
    getmemequates (pic, lfile)
    print "  Generating mem regions"
    printmemregions (pic, lfile)
    print "  Generating config words"
    getconfigwords (pic, lfile)
    print "  Generating sections"
    getsections (pic, lfile)
    print "  Generating cache init symbols"
    getinitsymbols_cache (pic, lfile)
    print "  Generating tlb init symbols"
    getinitsymbols_tlb (pic, lfile)

def writeLinkerForPIC(pic, lfile):
    lfile.write ("/*--------------------------------------------------------------------------\n")
    lfile.write (" * MPLAB XC Compiler -  %s linker script\n" % pic.name.upper())
    lfile.write (" * Build date : %s\n" % time.strftime("%b %d %Y"))
    lfile.write (" * \n")
    lfile.write (" * Copyright (c) %s, Microchip Technology Inc. and its subsidiaries (\"Microchip\")\n" %time.strftime("%Y"))
    lfile.write (" * All rights reserved.\n")
    lfile.write (" * \n")
    langcommon.printBoilerplate1Info(lfile)

    lfile.write ("/* Default linker script, for position independent executables */\n\n")

    if pic.interrupts.hasVariableOffsets :
        lfile.write ("/*  NOTE: This single-file linker script replaces the two-file system used\n")
        lfile.write (" *  for older PIC32 devices. \n")
        lfile.write (" */\n\n")
    elif "PIC32MX" in pic.name.upper():
        lfile.write ("/*  IMPORTANT: PIC32MX MCUs use two files for the default linker script: \n")
        lfile.write (" *    1) pic32mx/lib/ldscripts/elf32pic32.x (main linker script)\n")
        lfile.write (" *    2) pic32mx/lib/proc/%s/procdefs.ld (variant-specific fragment)\n" % namewithoutpic)
        lfile.write (" *\n")
        lfile.write (" *  This file is provided only as a convenience when adding a custom linker script\n")
        lfile.write (" *  to your application.\n")
        lfile.write (" */\n\n")

    lfile.write ("OUTPUT_FORMAT(\"elf32-tradlittlemips\")\n")
    lfile.write ("OUTPUT_ARCH(pic32mx)\n")
    lfile.write ("ENTRY(_reset)\n")
#    lfile.write ("/*\n")
#    lfile.write (" * Provide for a minimum stack and heap size\n")
#    lfile.write (" * - _min_stack_size - represents the minimum space that must be made\n")
#    lfile.write (" *                     available for the stack.  Can be overridden from\n")
#    lfile.write (" *                     the command line using the linker's --defsym option.\n")
#    lfile.write (" * - _min_heap_size  - represents the minimum space that must be made\n")
#    lfile.write (" *                     available for the heap.  Must be specified on\n")
#    lfile.write (" *                     the command line using the linker's --defsym option.\n")
#    lfile.write (" */\n")
#    lfile.write ("EXTERN (_min_stack_size _min_heap_size)\n")
#    lfile.write ("PROVIDE(_min_stack_size = 0x400) ;\n")

    getobjfile (pic,lfile)
    getrelsections_pic(pic, lfile)

#    if "MEC14" in pic.name.upper():
#        lfile.write ("PROVIDE(_min_heap_size = 0x400) ;\n")
    lfile.write ("\n");

    if (pic.interrupts.hasVariableOffsets) :
        lfile.write ("\n/*************************************************************************\n")
        lfile.write (" * Vector-offset initialization\n")
        lfile.write (" *************************************************************************/\n")
        lfile.write ("OPTIONAL(\"vector_offset_init_pic.o\")\n")
        lfile.write ("\n")
#    print "  Generating int vectors"
#    getintvector (pic, lfile)
#    print "  Generating mem equates"
    getmemequates_pic (pic, lfile)
#    print "  Generating mem regions"
#    printmemregions (pic, lfile)
#    print "  Generating config words"
#    getconfigwords (pic, lfile)
#    print "  Generating sections"
    getsections_pic (pic, lfile)
#    print "  Generating cache init symbols"
#    getinitsymbols_cache (pic, lfile)
#    print "  Generating tlb init symbols"
#    getinitsymbols_tlb (pic, lfile)
def writeProcdef(pic, procdef):
    print "  Generating objfiles"
    getobjfile (pic, procdef)
    print "  Generating int vectors"
    getintvector (pic, procdef)
    print "  Generating mem equates"
    getmemequates (pic, procdef)
    print "  Generating mem regions"
    printmemregions (pic, procdef)
    print "  Generating config words"
    getconfigwords (pic, procdef)

def printSupportFiles(pic):
    picname=pic.name
    family=pic.arch
    namewithoutpic=picname.replace("PIC32", "32").upper()
    path=os.getcwd()
    configpath=os.path.join(path,'32Family', 'config_docs')
    if not os.path.exists(configpath):
        os.makedirs(configpath)
    path=os.path.join(path,'32Family', namewithoutpic)
    if not os.path.exists(path):
        os.makedirs(path)

    # Generate architecture file
    arch_file = open(os.path.join(path, "pic32m"), 'w')
    arch_file.close()

    MZOffset = ""
    "Build a Makefile to build the processor-specific register definitions"
    print "Generating Makefile for %s" % namewithoutpic
    with open(os.path.join(path,"Makefile"), 'w') as mfile:

        for nmmrid, SFRDef_node in sorted(pic.registerMap.idOntoCoreNMMR.items(), key=lambda pair:pair[0]):
            name = SFRDef_node.getAttribute("edc:cname")
            device_has_fpu = False
            if name == "f0":
                device_has_fpu = True
                break

        mfile.write ("ROOT := $(shell pwd)\n")
        mfile.write ("include $(ROOT)/../../defines.mk\n\n")
        mfile.write ("PREFIX=xc32\n")
        mfile.write ("TARGET=pic32mx\n")
        mfile.write ("AR	=$(PREFIX)-ar\n")
        mfile.write ("CC	=$(PREFIX)-gcc\n")
        mfile.write ("STRIP   =$(PREFIX)-strip\n")
        mfile.write ("OUTPUTFILE=processor.o\n\n")

        if pic.interrupts.hasVariableOffsets:
			if gen_ldforpic:
				mfile.write ("ISROFFSETFILE=vector_offset_init.o\n\n")
				mfile.write ("ISROFFSETFILE_PIC=vector_offset_init_pic.o\n\n")
				MZOffset = " $(ISROFFSETFILE) $(ISROFFSETFILE_PIC)"
			else :
				mfile.write ("ISROFFSETFILE=vector_offset_init.o\n\n")
				MZOffset = " $(ISROFFSETFILE)"

        mfile.write ("ISRFILE=default_isr_vectors.o\n\n")
        if gen_ldforpic:
           mfile.write ("ISRFILE_PIC=default_isr_vectors_pic.o\n\n")

        mfile.write ("STARTUP_OPTIONS = -D__LIBBUILD__\n")
        if (namewithoutpic.upper().find("32MX") == 0):
            mfile.write ("STARTUP_OPTIONS += -DPIC32MX\n")
        else:
            mfile.write ("STARTUP_OPTIONS += -DINIT_SSX\n")
        if (namewithoutpic.upper().find("32MZ") == 0) or (namewithoutpic.upper().find("WFI") == 0):
            mfile.write ("STARTUP_OPTIONS += -DPIC32MZ\n")
            # TODO: Determine a better way to decide to use fixed ebi/sqi mapping
            # Do not init the MMU for fixed MZ mapping on "DA" devices
            #if (namewithoutpic.upper().find("DA", 7,10) == -1):
            mfile.write ("STARTUP_OPTIONS += -DINIT_MMU_MZ_FIXED\n")
        if (namewithoutpic.upper().find("32WK") == 0):
            mfile.write ("STARTUP_OPTIONS += -DPIC32WK\n")
        if (namewithoutpic.upper().find("32MX") == 0):
            mfile.write ("STARTUP_OPTIONS += -DPIC32MX\n")
        if (namewithoutpic.upper().find("32MM") == 0):
            mfile.write ("STARTUP_OPTIONS += -DPIC32MM\n")
        if (namewithoutpic.upper().find("32MK") == 0):
            mfile.write ("STARTUP_OPTIONS += -DPIC32MK\n")
        if (namewithoutpic.upper().find("BT55") == 0):
            mfile.write ("STARTUP_OPTIONS += -DBT55\n")
        if pic.hasL1Cache:
            mfile.write ("STARTUP_OPTIONS += -DINIT_L1_CACHE\n")
        if pic.instructionSet.hasDSPr2:
            mfile.write ("STARTUP_OPTIONS += -DINIT_DSPR2\n")
        # Enable the FPU for devices with a hardware floating-point unit
        if device_has_fpu:
            mfile.write ("STARTUP_OPTIONS += -mhard-float -mfp64 -DINIT_FPU64\n")
        if pic.instructionSet.hasMCUASE:
            mfile.write ("STARTUP_OPTIONS += -DINIT_MCUASE\n")
        if pic.instructionSet.hasMicroMIPS :
            mfile.write ("STARTUP_OPTIONS += -D__PIC32_HAS_MICROMIPS\n")
        if pic.instructionSet.hasMIPS32r2 :
            mfile.write ("STARTUP_OPTIONS += -D__PIC32_HAS_MIPS32R2\n")
        if pic.instructionSet.hasMicroMIPS and not(pic.instructionSet.hasMIPS32r2):
            mfile.write ("STARTUP_OPTIONS += -mmicromips -D__mips_micromips\n")
        if (namewithoutpic.upper().find("32MM") == 0) :
            mfile.write ("STARTUP_OPTIONS += -DINIT_INTCONVS\n")
        # dinit not in serial mem
        if (namewithoutpic.upper().find("32WK") != 0):
            mfile.write ("STARTUP_OPTIONS += -DINIT_DATA\n")

        # Number of Register sets to initialize
        mfile.write ("STARTUP_OPTIONS += -DPIC32_SRS_SET_COUNT=%d\n" % (pic.interrupts.shadowSetCount))

        mfile.write("\n")

        if pic.instructionSet.hasMIPS32r2:
	    if gen_ldforpic:
                mfile.write("STARTUPOBJ += crt0_mips32r2.o mcrt0_mips32r2.o crti_mips32r2.o crtn_mips32r2.o cpprt0_mips32r2.o crt0_mips32r2_pic.o cpprt0_mips32r2_pic.o\n\n")
	    else:
	       mfile.write("STARTUPOBJ += crt0_mips32r2.o mcrt0_mips32r2.o crti_mips32r2.o crtn_mips32r2.o cpprt0_mips32r2.o\n\n")
        if pic.instructionSet.hasMicroMIPS:
	    if gen_ldforpic:
            	mfile.write("STARTUPOBJ += crt0_micromips.o mcrt0_micromips.o crti_micromips.o crtn_micromips.o cpprt0_micromips.o crt0_micromips_pic.o cpprt0_micromips_pic.o\n\n")
	    else:
	       	mfile.write("STARTUPOBJ += crt0_micromips.o mcrt0_micromips.o crti_micromips.o crtn_micromips.o cpprt0_micromips.o\n\n")
        if pic.interrupts.hasVariableOffsets:
	    if gen_ldforpic:
                mfile.write("all: $(OUTPUTFILE) $(ISRFILE) $(ISROFFSETFILE) $(ISRFILE_PIC) $(ISROFFSETFILE_PIC) $(STARTUPOBJ)\n\n")
	    else:
	        mfile.write("all: $(OUTPUTFILE) $(ISRFILE) $(ISROFFSETFILE) $(STARTUPOBJ)\n\n")
        else:
            if gen_ldforpic:
		mfile.write ("all: $(OUTPUTFILE) $(ISRFILE) $(ISRFILE_PIC) $(STARTUPOBJ)\n\n")
	    else:
		mfile.write ("all: $(OUTPUTFILE) $(ISRFILE) $(STARTUPOBJ)\n\n")

        if gen_ldforpic:
		mfile.write ("install: $(OUTPUTFILE) $(ISRFILE) $(ISRFILE_PIC)%s $(STARTUPOBJ)\n\n" % MZOffset)
	        mfile.write ("install-proc: $(OUTPUTFILE) $(ISRFILE) $(ISRFILE_PIC)%s $(STARTUPOBJ)\n\n" % MZOffset)
	else:
		mfile.write ("install: $(OUTPUTFILE) $(ISRFILE)%s $(STARTUPOBJ)\n\n" % MZOffset)
		mfile.write ("install-proc: $(OUTPUTFILE) $(ISRFILE)%s $(STARTUPOBJ)\n\n" % MZOffset)
        mfile.write ("install-baseline:\n\n")
        mfile.write ("install-src:\n\n")
        mfile.write ("clean:\n")
	if gen_ldforpic:
        	mfile.write ("\trm -rf $(OUTPUTFILE) $(ISRFILE) $(ISRFILE_PIC) $(STARTUPOBJ)")
	else:
		mfile.write ("\trm -rf $(OUTPUTFILE) $(ISRFILE) $(STARTUPOBJ)")
        if pic.interrupts.hasVariableOffsets :
	    if gen_ldforpic:
                mfile.write (" $(ISROFFSETFILE) $(ISROFFSETFILE_PIC)")
	    else:
		mfile.write (" $(ISROFFSETFILE)")
        mfile.write ("\n\n")

        if pic.interrupts.hasVariableOffsets :
            mfile.write ("$(ISROFFSETFILE): p%s_voff_init.S\n" % namewithoutpic.upper())
            mfile.write ("\t$(CC) -c -o $@ $(SIZE_OPTION) $?\n\n")
            if gen_ldforpic:
            	mfile.write ("$(ISROFFSETFILE_PIC): p%s_voff_init.S\n" % namewithoutpic.upper())
    	        mfile.write ("\t$(CC) -fPIC -G0 -c -o $@ $(SIZE_OPTION) $?\n\n")
        mfile.write ("$(OUTPUTFILE): p%s.S\n" % namewithoutpic.upper())
        mfile.write ("\t$(CC) -c -o $@ $(SIZE_OPTION) $?\n\n")
        mfile.write ("$(ISRFILE): p%s_div.S\n" % namewithoutpic.upper())
        mfile.write ("\t$(CC) -c -o $@ $(SIZE_OPTION) $?\n\n")
        if gen_ldforpic:
	   mfile.write ("$(ISRFILE_PIC): p%s_div.S\n" % namewithoutpic.upper())
    	   mfile.write ("\t$(CC) -fPIC -G0 -c -o $@ $(SIZE_OPTION) $?\n\n")

        if pic.instructionSet.hasMIPS32r2:
            mfile.write ("mcrt0_mips32r2.o: ../../libpic32/startup/crt0.S\n")
            mfile.write ("\t$(CC) $(ASFLAGS) -mips32r2 -mno-mips16 -mno-micromips -D__%s__ -c -DMCRT0 -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n"
              % namewithoutpic.upper())
            mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

            mfile.write ("crt0_mips32r2.o: ../../libpic32/startup/crt0.S\n")
            mfile.write ("\t$(CC) $(ASFLAGS) -mips32r2 -mno-mips16 -mno-micromips -D__%s__ -c -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n"
              % namewithoutpic.upper())
            mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

            mfile.write ("cpprt0_mips32r2.o: ../../libpic32/startup/crt0.S\n")
            mfile.write ("\t$(CC) $(ASFLAGS) -mips32r2 -mno-mips16 -mno-micromips -D__%s__ -c -DCPP_INIT -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n"
              % namewithoutpic.upper())
            mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

            mfile.write ("crti_mips32r2.o: ../../libpic32/startup/crti.s\n")
            mfile.write ("\t$(CC) $(ASFLAGS) -mips32r2 -mno-mips16 -mno-micromips -D__%s__ -c -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n"
              % namewithoutpic.upper())
            mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

            mfile.write ("crtn_mips32r2.o: ../../libpic32/startup/crtn.s\n")
            mfile.write ("\t$(CC) $(ASFLAGS) -mips32r2 -mno-mips16 -mno-micromips -D__%s__ -c -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n"
              % namewithoutpic.upper())
            mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")
            if gen_ldforpic:
                mfile.write ("crt0_mips32r2_pic.o: ../../libpic32/startup/crt0_pic.S\n")
                mfile.write ("\t$(CC)  -fPIC -G0 $(ASFLAGS) -mips32r2 -mno-mips16 -mno-micromips -D__%s__ -c -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n"
                  % namewithoutpic.upper())
                mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")
                mfile.write ("cpprt0_mips32r2_pic.o: ../../libpic32/startup/crt0_pic.S\n")
                mfile.write ("\t$(CC)  -fPIC -G0 $(ASFLAGS) -mips32r2 -mno-mips16 -mno-micromips -D__%s__ -c -DCPP_INIT -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n"
                  % namewithoutpic.upper())
                mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

        if pic.instructionSet.hasMicroMIPS:
            mfile.write ("mcrt0_micromips.o: ../../libpic32/startup/crt0.S\n")
            mfile.write ("\t$(CC) $(ASFLAGS) -D__%s__ -c -DMCRT0 -mno-mips16 -mmicromips -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n" % namewithoutpic.upper())
            mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

            mfile.write ("crt0_micromips.o: ../../libpic32/startup/crt0.S\n")
            mfile.write ("\t$(CC) $(ASFLAGS) -D__%s__ -c -mno-mips16 -mmicromips -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n" % namewithoutpic.upper())
            mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

            mfile.write ("cpprt0_micromips.o: ../../libpic32/startup/crt0.S\n")
            mfile.write ("\t$(CC) $(ASFLAGS) -D__%s__ -c -DCPP_INIT -mno-mips16 -mmicromips -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n" % namewithoutpic.upper())
            mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

            mfile.write ("crti_micromips.o: ../../libpic32/startup/crti.s\n")
            mfile.write ("\t$(CC) $(ASFLAGS) -D__%s__ -c -mno-mips16 -mmicromips -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n" % namewithoutpic.upper())
            mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

            mfile.write ("crtn_micromips.o: ../../libpic32/startup/crtn.s\n")
            mfile.write ("\t$(CC) $(ASFLAGS) -D__%s__ -c -mno-mips16 -mmicromips -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n" % namewithoutpic.upper())
            mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")
            if gen_ldforpic:
                mfile.write ("crt0_micromips_pic.o: ../../libpic32/startup/crt0_pic.S\n")
                mfile.write ("\t$(CC)  -fPIC -G0 $(ASFLAGS) -D__%s__ -c -mno-mips16 -mmicromips -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n" % namewithoutpic.upper())
                mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

                mfile.write ("cpprt0_micromips_pic.o: ../../libpic32/startup/crt0_pic.S\n")
                mfile.write ("\t$(CC)  -fPIC -G0 $(ASFLAGS) -D__%s__ -c -DCPP_INIT -mno-mips16 -mmicromips -I$(ROOT)/../../include $(STARTUP_OPTIONS) $< -o $@\n" % namewithoutpic.upper())
                mfile.write ("\t$(STRIP) $(STRIPFLAGS) $@\n\n")

    "Build default isr vectors for pic32 device"
    print "Writing _div.S"
    with open(os.path.join(path,"p"+namewithoutpic.upper()+"_div.S"),"w") as vfile:
        # Output the header information
        vfile.write (" #-------------------------------------------------------------------------\n")
        vfile.write (" # MPLAB XC Compiler -  %s processor Default ISR Vectors module\n" % picname)
        vfile.write (" # \n")
        vfile.write (" # Copyright (c) %s, Microchip Technology Inc. and its subsidiaries (\"Microchip\")\n" %time.strftime("%Y"))
        vfile.write (" # All rights reserved.\n")
        vfile.write (" # \n")

        langcommon.printBoilerplate2Info(vfile)
        vfile.write (" #-------------------------------------------------------------------------\n")

        vfile.write ("\n")

        vfile.write ("#if defined(__XC32) && !defined(NO_DEFAULT_ISR_VECTORS)\n")
        vfile.write ("  .section .discard, info\n")

        if pic.interrupts.hasVariableOffsets:
            vfile.write ("  .word _DefaultInterrupt\n");
        else:
            for x in range (0,countVectors(pic)+1) :
                vfile.write ("  .word __vector_dispatch_%d\n" % (x));

        vfile.write ("#endif\n")
        vfile.write ("\n")

    print "Writing .S file"
    with open(os.path.join(path,"p"+namewithoutpic.upper()+".S"),"w") as sfile:
        sfile.write(' #-------------------------------------------------------------------------\n')
        sfile.write(" # MPLAB XC Compiler -  "+picname+" processor definition module\n")
        sfile.write(" # \n")
        sfile.write(" # Copyright (c) %s, Microchip Technology Inc. and its subsidiaries (\"Microchip\")\n" %time.strftime("%Y"))
        sfile.write(" # All rights reserved.\n")
        sfile.write(" # \n")

        langcommon.printBoilerplate2Info(sfile)
        sfile.write(' #-------------------------------------------------------------------------\n')

        sfile.write("\n")
        langcommon.printSfile(sfile)
        sfile.write("\n")
        sfile.write("\n")

    if pic.interrupts.hasVariableOffsets :
        print "Writing _voff_init.S file"
        with open(os.path.join(path,"p"+namewithoutpic.upper()+"_voff_init.S"),"w") as sfile:
            sfile.write(' #-------------------------------------------------------------------------\n')
            sfile.write(" # MPLAB XC Compiler -  "+picname+" vector-offset initialization module\n")
            sfile.write(" # \n")
            sfile.write(" # Copyright (c) %s, Microchip Technology Inc. and its subsidiaries (\"Microchip\")\n" %time.strftime("%Y"))
            sfile.write(" # All rights reserved.\n")
            sfile.write(" # \n")

            langcommon.printBoilerplate2Info(sfile)
            sfile.write(' #-------------------------------------------------------------------------\n')

            sfile.write("\n")
            printOffsetSfile(pic, sfile)
            sfile.write("\n")
            sfile.write("\n")

    with open(os.path.join(configpath,namewithoutpic.lower()+".html"),"w") as htmlfile:
        htmlfile.write ("<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
        htmlfile.write ("<html>\n<head>\n<title>");
        htmlfile.write ("Config-Bit Settings for %s" % pic.name)
        htmlfile.write ("</title>\n</head>\n");
        htmlfile.write ("<body bgcolor=\"#FFFFE0\" style=\"font-family: sans-serif\">\n")
        htmlfile.write ("<p>[ <a href=\"../XC32MasterIndex.htm\">Documentation Index</a> | <a href=\"../PIC32ConfigSet.html\">Device index</a> ]</p>\n")
        htmlfile.write ("<h1>Configuration-Bit Settings for %s</h1>" % pic.name)
        htmlfile.write ("<p>(Data Sheet # <a href=\"http://www.microchip.com/wwwproducts/Devices.aspx?Product=%s&amp;ds=1\" target=\"Datasheet\">DS-%s</a>)</p>\n" % (pic.name, edc.get(pic.node, 'dsid')))

        htmlfile.write ("<p>Usage:<br><strong>#pragma config</strong> <em>SETTING</em> = <em>VALUE</em><br>\n")

        if ("32MZ" in pic.name.upper()) or ("WFI" in pic.name.upper()):
            htmlfile.write ("<strong>#pragma config_alt</strong> <em>SETTING</em> = <em>VALUE</em><br>\n")
            htmlfile.write ("<strong>#pragma config_bf1</strong> <em>SETTING</em> = <em>VALUE</em><br>\n")
            htmlfile.write ("<strong>#pragma config_abf1</strong> <em>SETTING</em> = <em>VALUE</em><br>\n")
            htmlfile.write ("<strong>#pragma config_bf2</strong> <em>SETTING</em> = <em>VALUE</em><br>\n")
            htmlfile.write ("<strong>#pragma config_abf2</strong> <em>SETTING</em> = <em>VALUE</em>")
        htmlfile.write ("</p>\n")

        with open(os.path.join(path,"configuration.data"),"w") as configfile:
            configfile.write ("Daytona Configuration Word Definitions: ")
            langcommon.configDataFile(pic, configfile, htmlfile)

        htmlfile.write ("<p>[ <a href=\"../XC32MasterIndex.htm\">Documentation Index</a> | <a href=\"../PIC32ConfigSet.html\">Device index</a> ]</p>\n")
        htmlfile.write ("</body>\n</html>\n\n")

    print "Generating device-specific .ld"
    with open(os.path.join(path,"p"+namewithoutpic.upper()+".ld"),"w") as lfile:
        writeLinker(pic, lfile)
        print "  Closing device-specific .ld"

    if gen_ldforpic:
        print "Generating device-specific .ld for PIC"
        with open(os.path.join(path,"p"+namewithoutpic.upper()+"_pic.ld"),"w") as lfile:
            writeLinkerForPIC(pic, lfile)
        print "  Closing device-specific .ld for PIC"

    if (namewithoutpic.upper().find("32MX") == 0):
        print "Generating procdefs.ld"
        with open(os.path.join(path,"procdefs.ld"),"w") as procdef:
            writeProcdef(pic, procdef)
            print "  Closing procdefs.ld"

if __name__ == '__main__':

    cmdlinetools.main(parseOptions, processSingleInputFile)
