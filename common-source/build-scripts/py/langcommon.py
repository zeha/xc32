
from collections import OrderedDict
import time
import fdom
import edc
import re
import ltx
import pic2prm
import operator

alias=[]
cnamesList={}
orderCnames=[]
count=0
bitsList={}
RegandBitnames={}
orderList=[]
cnameDict={}
trackAddrs={}
widthGreaterthanOne={}
modesDict={}
base_addresses={}
bit_ListOrder=[]

class ConfigEntry:
    def __init__(self, field1, field2, field3, field4, alt_field4 = None):
        self.field1 = field1
        self.field2 = field2
        self.field3 = field3
        self.field4 = field4
        self.alt_field4 = alt_field4

def printHfileStartingInfo(picname, withoutpic,outfile,family):

  # Output the header information
  outfile.write ("/*-------------------------------------------------------------------------\n")
  outfile.write (" * MPLAB-Cxx  %s processor header\n" % picname)
  outfile.write (" *\n")
  outfile.write (" * (c) Copyright 1999-%s Microchip Technology, All rights reserved\n" % time.localtime().tm_year)
  outfile.write (" *-------------------------------------------------------------------------*/\n\n")

  outfile.write ("#ifndef __%s_H\n" % withoutpic)
  outfile.write ("#define __%s_H\n" % withoutpic)
  outfile.write ("\n")

# print language tool poiler plate with passed comment marker
def printBoilerplateGeneric(outfile, open_comment):
  if ( open_comment == "/*" ):
     close_comment="*/"
     comment = "*"
  elif ( open_comment == ";"):
     close_comment=";"
     comment=";"
  elif ( open_comment == "#"):
     close_comment="#"
     comment="#"
  else:
     raise "Unkown 'open_comment style"

  outfile.write (" %s This software is developed by Microchip Technology Inc. and its\n"% comment)
  outfile.write (" %s subsidiaries (\"Microchip\").\n" % comment)
  outfile.write (" %s \n"% comment);
  outfile.write (" %s Redistribution and use in source and binary forms, with or without\n"% comment)
  outfile.write (" %s modification, are permitted provided that the following conditions are \n"% comment)
  outfile.write (" %s met:\n"% comment)
  outfile.write (" %s \n"% comment);
  outfile.write (" %s 1.      Redistributions of source code must retain the above copyright\n"% comment)
  outfile.write (" %s         notice, this list of conditions and the following disclaimer.\n"% comment)
  outfile.write (" %s 2.      Redistributions in binary form must reproduce the above \n"% comment)
  outfile.write (" %s         copyright notice, this list of conditions and the following \n"% comment)
  outfile.write (" %s         disclaimer in the documentation and/or other materials provided \n"% comment)
  outfile.write (" %s         with the distribution. Publication is not required when this file \n"% comment)
  outfile.write (" %s         is used in an embedded application.\n"% comment)
  outfile.write (" %s 3.      Microchip's name may not be used to endorse or promote products\n"% comment)
  outfile.write (" %s         derived from this software without specific prior written \n"% comment)
  outfile.write (" %s         permission.\n"% comment)
  outfile.write (" %s \n"% comment);
  outfile.write (" %s THIS SOFTWARE IS PROVIDED BY MICROCHIP \"AS IS\" AND ANY EXPRESS OR IMPLIED\n"% comment)
  outfile.write (" %s WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n"% comment)
  outfile.write (" %s MERCHANTABILITY AND FITNESS FOR PURPOSE ARE DISCLAIMED. IN NO EVENT \n"% comment)
  outfile.write (" %s SHALL MICROCHIP BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n"% comment)
  outfile.write (" %s EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING BUT NOT LIMITED TO\n"% comment)
  outfile.write (" %s PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA OR PROFITS;\n"% comment)
  outfile.write (" %s OR BUSINESS INTERRUPTION) HOWSOEVER CAUSED AND ON ANY THEORY OF LIABILITY,\n" % comment)
  outfile.write (" %s WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR \n"% comment)
  outfile.write (" %s OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF\n" %comment)
  outfile.write (" %s ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"% comment)
  outfile.write (" %s \n" % comment);
  outfile.write (" %s\n\n" % close_comment)

def printBoilerplate1Info (outfile):
  printBoilerplateGeneric(outfile,"/*");

def printBoilerplate2Info (outfile):
  printBoilerplateGeneric(outfile,"#");

def print32HfileStartingInfo(picname, withoutpic, outfile,family):
  outfile.write ("/*-------------------------------------------------------------------------\n")
  outfile.write (" * %s processor header\n" % picname)
  outfile.write (" * Build date : %s\n" % time.strftime("%b %d %Y"))
  outfile.write (" *\n")
  outfile.write (" * Copyright (c) %s, Microchip Technology Inc. and its subsidiaries (\"Microchip\")\n" %time.strftime("%Y"))
  outfile.write (" * All rights reserved.\n")
  outfile.write (" * \n")
  printBoilerplate1Info(outfile)
  outfile.write ("#pragma once\n")
  outfile.write ("#ifndef __%s_H\n" % withoutpic)
  outfile.write ("#define __%s_H\n" % withoutpic)
  outfile.write ("\n")

def print32HfilesEndingInfo(outfile):
  outfile.write ("#ifdef __cplusplus\n")
  outfile.write ("} /* extern \"C\" */\n")
  outfile.write ("#endif\n\n")


def allSFRS(pic,shadowSFRS,allSFRS):
  for x,y in pic.addrOntoShadowSFR.items():
      shadowSFRS.update({x:y})

  for a,b in pic.addrOntoSFR.items():
      allSFRS.update({a:b})

  for a,b in pic.mainPartition.registerMap.addrOntoJoinedSFR.iteritems():
      allSFRS.update({a:b})

  for addr, MuxedSFRDef_list in pic.mainPartition.registerMap.addrOntoMuxedSFRs.iteritems():
      for MuxedSFRDef_node in MuxedSFRDef_list:
          allSFRS.update({addr:MuxedSFRDef_node})

  for p, q in shadowSFRS.items():
      if p not in allSFRS.keys() :
          allSFRS.update({p:q})

  return shadowSFRS,allSFRS

def addrInShadowSFRS(shadowSFRS,SFRDef_node,addr):
  sortlist=[]
  shadowSFRValue=shadowSFRS.get(addr)
  sortlist.append(fdom.get2(SFRDef_node, 'cname'))
  sortlist.append(fdom.get2(shadowSFRValue, 'cname'))
  sortlist=sorted(sortlist)
  return sortlist

def addCnames(cname,SFRDef_node,addr):
  allCnames={}
  portals=""
  langhidden = fdom.get2(SFRDef_node, 'islanghidden', defaultValue="")
  hidden = fdom.get2(SFRDef_node, 'ishidden', defaultValue="")
  if langhidden == "true" or hidden == "true":
    return None, None
  nzlangwidth=fdom.get2(SFRDef_node, 'nzlangwidth', edc.str2int, defaultValue="")
  nzwidth = fdom.get2(SFRDef_node, 'nzwidth', edc.str2int, defaultValue="")

  registerSize = 'uint32_t '
  if nzlangwidth=="":
    nzlangwidth=nzwidth
  if nzlangwidth <= 8  :
    registerSize = 'uint8_t'
  migAlias=[]
  legacyAlias=[]
  tempSort=[]
  portals=fdom.get2(SFRDef_node,'portals', defaultValue="")
  SFRAliasList_node = fdom.first(SFRDef_node, 'AliasList')

  if SFRAliasList_node:
    migAlias=fdom.children(SFRAliasList_node, 'MigrationAlias')
    legacyAlias=fdom.children(SFRAliasList_node, 'LegacyAlias')

    for each in migAlias:
      aliasname = fdom.get2(each,'cname')
      if (aliasname.find("99zz") > 0 or aliasname.find("98zy") > 0):
        print "Skipping alias name:", aliasname
        cnamesList.update({(cname):addr})
        orderCnames.append(cname)
        if "CLR" in portals:
          cnamesList.update({cname+"CLR":addr+4})
          orderCnames.append(cname+"CLR")
        if "SET" in portals:
          cnamesList.update({cname+"SET":addr+8})
          orderCnames.append(cname+"SET")
        if "INV" in portals:
          cnamesList.update({cname+"INV":addr+12})
          orderCnames.append(cname+"INV")

        continue
      allCnames.update({fdom.get2(each,'cname'):SFRDef_node})

      cnamesList.update({(cname):addr})
      cnamesList.update({fdom.get2(each,'cname'):addr})
      orderCnames.append(cname)
      orderCnames.append(fdom.get2(each,'cname'))
      if "CLR" in portals:

        cnamesList.update({cname+"CLR":addr+4})
        cnamesList.update({fdom.get2(each,'cname')+"CLR":addr+4})
        orderCnames.append(cname+"CLR")
        orderCnames.append(fdom.get2(each,'cname')+"CLR")
      if "SET" in portals:

        cnamesList.update({cname+"SET":addr+8})
        cnamesList.update({fdom.get2(each,'cname')+"SET":addr+8})
        orderCnames.append(cname+"SET")
        orderCnames.append(fdom.get2(each,'cname')+"SET")

      if "INV" in portals:

        cnamesList.update({cname+"INV":addr+12})
        cnamesList.update({fdom.get2(each,'cname')+"INV":addr+12})
        orderCnames.append(cname+"INV")
        orderCnames.append(fdom.get2(each,'cname')+"INV")
      alias.append(fdom.get2(each,'cname'))

    if len(legacyAlias)>0:
      for item in  legacyAlias:
        aliasname = fdom.get2(item,'cname')
        if (aliasname.find("99zz") > 0 or aliasname.find("98zy") > 0):
          print "Skipping alias name:", aliasname
          cnamesList.update({(cname):addr})
          orderCnames.append(cname)
          if portals:
            if "CLR" in portals:
              cnamesList.update({cname+"CLR":addr+4})
              orderCnames.append(cname+"CLR")
              del tempSort[:]
            if "SET" in portals:
              cnamesList.update({cname+"SET":addr+8})
              orderCnames.append(cname+"SET")
            if "INV" in portals:
              cnamesList.update({cname+"INV":addr+12})
              orderCnames.append(cname+"INV")
          continue
        allCnames.update({fdom.get2(item,'cname'):SFRDef_node})
        cnamesList.update({(cname):addr})
        cnamesList.update({fdom.get2(item,'cname'):addr})
        orderCnames.append(cname)
        orderCnames.append(fdom.get2(item,'cname'))
        if portals:
          if "CLR" in portals:
            cnamesList.update({cname+"CLR":addr+4})
            cnamesList.update({fdom.get2(item,'cname')+"CLR":addr+4})
            orderCnames.append(cname+"CLR")
            orderCnames.append(fdom.get2(item,'cname')+"CLR")
            del tempSort[:]
          if "SET" in portals:

            cnamesList.update({cname+"SET":addr+8})
            cnamesList.update({fdom.get2(item,'cname')+"SET":addr+8})
            orderCnames.append(cname+"SET")
            orderCnames.append(fdom.get2(item,'cname')+"SET")
          if "INV" in portals:

            cnamesList.update({cname+"INV":addr+12})
            cnamesList.update({fdom.get2(item,'cname')+"INV":addr+12})
            orderCnames.append(cname+"INV")
            orderCnames.append(fdom.get2(item,'cname')+"INV")
        alias.append(fdom.get2(item,'cname'))

  allCnames.update({(cname):SFRDef_node})
  if len(migAlias)<1 and len(legacyAlias)<1:
    cnamesList.update({(cname):addr})
    orderCnames.append(cname)
    if portals:
      if "CLR" in portals:
        cnamesList.update({cname+"CLR":addr+4})
        orderCnames.append(cname+"CLR")
      if "SET" in portals:
        cnamesList.update({cname+"SET":addr+8})
        orderCnames.append(cname+"SET")
      if "INV" in portals:
        cnamesList.update({cname+"INV":addr+12})
        orderCnames.append(cname+"INV")

  return allCnames,registerSize


def countModes(SFRDef_node,registerSize,allCnames,outfile,cname,type,family):

    SFRModeList_node = fdom.first(SFRDef_node, type+'ModeList')
    if SFRModeList_node :
      for item in sorted(allCnames.keys()):
        if item not in ( "WREG0", "WREG1", "WREG2", "WREG3", "WREG4", "WREG5", "WREG6", "WREG7", "WREG8", "WREG9", "WREG10", "WREG11", "WREG12", "WREG13", "WREG14", "WREG15" ):

           all_offset=""
           modeList=fdom.children(SFRModeList_node, type+'Mode')
           for each in modeList:
            modeChildren= fdom.children(each,type+"FieldDef")
            adjPoint= fdom.children(each,"AdjustPoint")
            if modeChildren==[] and len(adjPoint)>0:
              pass
            else:
             all_offset="True"

      modeList=[]
      modeList=fdom.children(SFRModeList_node, type+'Mode')
      for each in modeList[:]:
        if fdom.hasMatch(each, 'id', edc.ds_or_lt_modeid_pattern):
          langhidden=fdom.get2(each, 'islanghidden', defaultValue="")
          hidden=fdom.get2(each, 'ishidden', defaultValue="")
          if langhidden=="" or langhidden=="false" or hidden=="" or hidden=="false":
            modeChildren= fdom.children(each,type+"FieldDef")
            adjPoint= fdom.children(each,"AdjustPoint")
            if modeChildren==[] and len(adjPoint)>0:
                modeList.remove(each)
            for one in modeChildren[:] :
              langhidden=fdom.get2(one, 'islanghidden', defaultValue="")
              hidden=fdom.get2(one, 'ishidden', defaultValue="")
              if langhidden=="true" or hidden=="true":
                modeChildren.remove(one)
                if modeChildren==[] :
                  if adjPoint!=[] and len(adjPoint)==1:
                    modeList.remove(each)
                  else :
                    modeList.remove(each)
          else:
            modeList.remove(each)
        else:
          modeList.remove(each)
      modeCount=len(modeList) # Number of Modes

      return modeList,SFRModeList_node



    return modeList
def printStructUnion(modeList,SFRModeList_node,outfile,c,registerSize,allCnames,type,family,entry):
 if entry=="first":
    del orderList[:]
    bitsList.clear()
    RegandBitnames.clear()
    trackAddrs.clear()
    cnameDict.clear()
    modesDict.clear()
    widthGreaterthanOne.clear()
    del bit_ListOrder[:]
    base_addresses.clear()

 hasDebug = ""
 dpos = 0
 dmask = 0
 dlen = 0
 aliasExist=""
 innerCname=""
 for each in sorted(allCnames.keys()):


  c=0
  cname=""
  cname=str(each)
  cnamewidth=fdom.get2(allCnames.get(each), 'nzwidth')
  addr=cnamesList.get(cname)

  if family=="32xxxx":
    outfile.write("#define %s %s\n" % (cname, cname))
    outfile.write('extern volatile %s  %s __attribute__((section("sfrs"), address(0x%08X)));\n' % (registerSize, cname, addr+0XA0000000))
  else :
    if outfile!="":
      if cname in ( "WREG0", "WREG1", "WREG2", "WREG3", "WREG4", "WREG5", "WREG6", "WREG7", "WREG8", "WREG9", "WREG10", "WREG11", "WREG12", "WREG13", "WREG14", "WREG15" ):
        outfile.write("#define %s %s\n" % (cname, cname))
        outfile.write("extern volatile unsigned %s %s __attribute__((__sfr__,__deprecated__,__unsafe__));\n" % (registerSize, cname))
        cnameDict.update({cname:cname})
        orderList.append(cname)


      else:
        outfile.write("#define %s %s\n" % (cname, cname))
        outfile.write("extern volatile unsigned %s %s __attribute__((__sfr__));\n" % (registerSize, cname))
        cnameDict.update({cname:cname})
        orderList.append(cname)

  if modeList==[]:
    modeCount=0
  else:
    modeCount=len(modeList)
  for SFRMode_node in modeList:

   id=fdom.get2(SFRMode_node,"id")
   if fdom.hasMatch(SFRMode_node, 'id', edc.ds_or_lt_modeid_pattern):
    length=  len(fdom.children(SFRMode_node,type+'FieldDef') )
    for field_node, place, width in edc.visit_fields(SFRMode_node):
      langhidden=fdom.get2(field_node, 'islanghidden', defaultValue="")
      hidden=fdom.get2(field_node, 'ishidden', defaultValue="")
      if langhidden=="true" or hidden=="true":
         length=length-1
         if length==0:
           continue

      innerCname=fdom.get2(field_node,'cname', defaultValue="")
      widthnz=fdom.get2(field_node,'nzwidth', defaultValue="")
      #if(innerCname==cname) and widthnz=="16" or widthnz=="0x10" or width==16:
      #  length=length-1
      #  continue


    if type=="DCR":
      modeCount=modeCount+1;

    if fdom.hasMatch(SFRMode_node, 'id', edc.ds_or_lt_modeid_pattern):

      if c==1 and len(fdom.children(SFRMode_node,type+'FieldDef') )>=1 and modeCount>1 and outfile!="":
        if family=="32xxxx":
          outfile.write("  };\n")
          outfile.write("  struct {\n")
        else:
          outfile.write("    };\n")
          outfile.write("    struct {\n")
      if len(fdom.children(SFRMode_node,type+'FieldDef'))>=1 and modeCount>1 and c==0:
        if outfile!="":
          if family=="32xxxx":
            outfile.write("typedef union {\n")
            outfile.write("  struct {\n")

          else:
            outfile.write("__extension__ typedef struct tag"+cname+"BITS {\n")
            outfile.write("  union {\n")
            outfile.write("    struct {\n")
        c=1

      if c==0 and len(fdom.children(SFRMode_node,type+'FieldDef') )>=1 and modeCount==1 and length>=1:
        if outfile!="":
          if family=="32xxxx":
            outfile.write("typedef struct {\n" )
          else:
             outfile.write("typedef struct tag"+cname+"BITS {\n" )
        c=2
      cnt=0

      for field_node, place, width in edc.visit_fields(SFRMode_node):
        fwidth=width
        adjBits="Junk"
        lang_hidden=fdom.get2(field_node, 'islanghidden', defaultValue="")
        hidden=fdom.get2(field_node, 'ishidden', defaultValue="")
        if lang_hidden=="true" or hidden=="true":
          width=fdom.get2(field_node,'nzwidth', defaultValue="")
          width=int(width,16)
          adjBits=""
        if c==1 or c==2 :
          if adjBits==""   :
            bits=adjBits
          else:
            bits = fdom.get2(field_node, 'cname', defaultValue="")

          if bits.startswith('n'):
            bits = 'NOT_%s' % bits[1:]
          if bits=="":
            if cnt>=1:
              if cnt==1:
                cwidth=width+prev
                cnt=cnt+1
              else :
                precwidth=cwidth+width
                cwidth=precwidth
                cnt=cnt+1
            else :
              cnt=cnt+1;
              prev=width
              prevbits=bits
          else :
            if((innerCname==cname) and (widthnz=="16" or widthnz=="0x10" or width==16 or cnamewidth=="16")) or fwidth>1:
                widthGreaterthanOne.update({cname+bits:cname})
            if type == "DCR":
              if bits == "DEBUG":
                dpos = place
                dmask = (pow (2, width) - 1) << place
                dlen = width
                hasDebug = cname
#            if cname=="RPOR0":
#              print innerCname
#              print cname
#              print fwidth
#              print widthnz
#              print cnamewidth
            if cnt==0:
              if outfile!="" :
                if c==1:
                  if family=="32xxxx":
                    outfile.write("    uint32_t %s:%d;\n" %(bits,width))
                  else:
                    outfile.write("      unsigned %s:%d;\n" %(bits,width))

                else :
                  outfile.write("  uint32_t %s:%d;\n" %(bits,width))

              bitsList.update({"_"+cname+"_"+bits+"_POSITION": (place)})
              bitsList.update({"_"+cname+"_"+bits+"_MASK": ((pow (2, width) - 1) << place)})
              bitsList.update({"_"+cname+"_"+bits+"_LENGTH": (width)})
              bit_ListOrder.append("_"+cname+"_"+bits+"_POSITION")
              bit_ListOrder.append("_"+cname+"_"+bits+"_MASK")
              bit_ListOrder.append("_"+cname+"_"+bits+"_LENGTH")

              RegandBitnames.update({cname+bits: bits})
              trackAddrs.update({cname+bits: "0x%04X" %place})
              cnameDict.update({cname+bits:cname})
              modesDict.update({cname+bits:SFRMode_node})
              orderList.append(cname+bits)

            if cnt>2:
              if outfile!="":
                if c==1:
                  if family=="32xxxx":
                    outfile.write("    uint32_t %s:%d;\n"  % (prevbits,cwidth))
                    outfile.write("    uint32_t %s:%d;\n"  % (bits,width))
                  else:
                    outfile.write("      unsigned %s:%d;\n"  % (prevbits,cwidth))
                    outfile.write("      unsigned %s:%d;\n"  % (bits,width))


                else:
                  outfile.write("  uint32_t %s:%d;\n"  % (prevbits,cwidth))
                  outfile.write("  uint32_t %s:%d;\n"  % (bits,width))

              bitsList.update({"_"+cname+"_"+bits+"_POSITION": (place)})
              bitsList.update({"_"+cname+"_"+bits+"_MASK": ((pow (2, width) - 1) << place)})
              bitsList.update({"_"+cname+"_"+bits+"_LENGTH": (width)})
              bit_ListOrder.append("_"+cname+"_"+bits+"_POSITION")
              bit_ListOrder.append("_"+cname+"_"+bits+"_MASK")
              bit_ListOrder.append("_"+cname+"_"+bits+"_LENGTH")
              RegandBitnames.update({cname+bits: bits})
              trackAddrs.update({cname+bits: "0x%04X" %place})
              cnameDict.update({cname+bits:cname})
              modesDict.update({cname+bits:SFRMode_node})
              orderList.append(cname+bits)

            if cnt==1:
              if outfile!="":
                if c==1:
                  if family=="32xxxx":
                    outfile.write("    uint32_t %s:%d;\n"  % (prevbits,prev))
                    outfile.write("    uint32_t %s:%d;\n"  % (bits,width))
                  else:
                    outfile.write("      unsigned %s:%d;\n"  % (prevbits,prev))
                    outfile.write("      unsigned %s:%d;\n"  % (bits,width))

                else:
                  outfile.write("  uint32_t %s:%d;\n"  % (prevbits,prev))
                  outfile.write("  uint32_t %s:%d;\n"  % (bits,width))

              bitsList.update({"_"+cname+"_"+bits+"_POSITION": (place)})
              bitsList.update({"_"+cname+"_"+bits+"_MASK": ((pow (2, width) - 1) << place)})
              bitsList.update({"_"+cname+"_"+bits+"_LENGTH": (width)})
              bit_ListOrder.append("_"+cname+"_"+bits+"_POSITION")
              bit_ListOrder.append("_"+cname+"_"+bits+"_MASK")
              bit_ListOrder.append("_"+cname+"_"+bits+"_LENGTH")
              RegandBitnames.update({cname+bits: bits})
              trackAddrs.update({cname+bits:"0x%04X" %place})
              cnameDict.update({cname+bits:cname})
              modesDict.update({cname+bits:SFRMode_node})
              orderList.append(cname+bits)
            if cnt==2 :
              if outfile!="":
                if c==1:
                  if family=="32xxxx":
                    outfile.write("    uint32_t %s:%d;\n"  % (prevbits,cwidth))
                    outfile.write("    uint32_t %s:%d;\n"  % (bits,width))
                  else:
                    outfile.write("      unsigned %s:%d;\n"  % (prevbits,cwidth))
                    outfile.write("      unsigned %s:%d;\n"  % (bits,width))

                else:
                  outfile.write("  uint32_t %s:%d;\n"  % ("",cwidth))
                  outfile.write("  uint32_t %s:%d;\n"  % (bits,width))

              bitsList.update({"_"+cname+"_"+bits+"_POSITION": (place)})
              bitsList.update({"_"+cname+"_"+bits+"_MASK": ((pow (2, width) - 1) << place)})
              bitsList.update({"_"+cname+"_"+bits+"_LENGTH": (width)})
              bit_ListOrder.append("_"+cname+"_"+bits+"_POSITION")
              bit_ListOrder.append("_"+cname+"_"+bits+"_MASK")
              bit_ListOrder.append("_"+cname+"_"+bits+"_LENGTH")
              RegandBitnames.update({cname+bits: bits})
              trackAddrs.update({cname+bits: "0x%04X" %place})
              cnameDict.update({cname+bits:cname})
              modesDict.update({cname+bits:SFRMode_node})
              orderList.append(cname+bits)
            cnt=0
            le=len(fdom.children(SFRMode_node,type+'FieldDef' ))
    if length>=1  and  modeCount==1 and c==2 and outfile!="" :
      if family=="32xxxx" :
        outfile.write("} " +"__"+cname+"bits_t;\n")
        addr = cnamesList.get(cname)
        outfile.write("extern volatile "+'__'+cname+"bits_t "+cname+"bits"+' __asm__ ("'+cname+'") __attribute__((section("sfrs"), address(0x%08X)));\n' % (addr+0XA0000000))
      else:
        outfile.write("} " +cname+"BITS;\n")
        outfile.write("#define %s %s\n" % (cname+"bits", cname+"bits"))
        outfile.write("extern volatile "+cname+"BITS "+cname+"bits __attribute__((__sfr__));\n")
        outfile.write("\n")

  if  modeCount>1 and c==1 and outfile!="" :
        if family=="32xxxx"  :
          outfile.write("  };\n")
          if type=="DCR":
            if hasDebug and cname==hasDebug:
              outfile.write("  struct {\n")
              outfile.write("    uint32_t FDEBUG:2;\n");
              outfile.write("  };\n");
              bitsList.update({"_"+cname+"_FDEBUG_POSITION": (dpos)})
              bitsList.update({"_"+cname+"_FDEBUG_MASK": (dmask)})
              bitsList.update({"_"+cname+"_FDEBUG_LENGTH": (dlen)})
              bit_ListOrder.append("_"+cname+"_FDEBUG_POSITION")
              bit_ListOrder.append("_"+cname+"_FDEBUG_MASK")
              bit_ListOrder.append("_"+cname+"_FDEBUG_LENGTH")
              #outfile.write("  };\n")
              hasDebug=""
            outfile.write("  struct {\n")
            outfile.write("    uint32_t w:32;\n");
            outfile.write("  };\n");
            bitsList.update({"_"+cname+"_w_POSITION": (0x0)})
            bitsList.update({"_"+cname+"_w_MASK": (0xFFFFFFFF)})
            bitsList.update({"_"+cname+"_w_LENGTH": (0x20)})
            bit_ListOrder.append("_"+cname+"_w_POSITION")
            bit_ListOrder.append("_"+cname+"_w_MASK")
            bit_ListOrder.append("_"+cname+"_w_LENGTH")
          outfile.write("} "+"__"+cname+"bits_t;\n")
          addr = cnamesList.get(cname)
          outfile.write("extern volatile "+'__'+cname+"bits_t "+cname+"bits"+' __asm__ ("'+cname+'") __attribute__((section("sfrs"), address(0x%08X)));\n' % (addr+0XA0000000))

        else:
          outfile.write("    };\n")
          outfile.write("  };\n")
          outfile.write("} " +cname+"BITS;\n")
          outfile.write("#define %s %s\n" % (cname+"bits", cname+"bits"))
          outfile.write("extern volatile "+cname+"BITS "+cname+"bits __attribute__((__sfr__));\n")
          outfile.write("\n")


  portals=fdom.get2(allCnames.get(each),'portals', defaultValue="")
  if cname not in alias and len(allCnames.keys())==1 and family=="32xxxx" and outfile!="":

    if "CLR" in portals:
      addr=cnamesList.get(cname + 'CLR')
      outfile.write('extern volatile %s       %sCLR __attribute__((section("sfrs"),address(0x%08X)));\n' % (registerSize, cname, addr+0XA0000000))
    if  "SET" in portals:
      addr=cnamesList.get(cname + 'SET')
      outfile.write('extern volatile %s       %sSET __attribute__((section("sfrs"),address(0x%08X)));\n' % (registerSize, cname, addr+0XA0000000))
    if "INV"  in portals:
      addr=cnamesList.get(cname + 'INV')
      outfile.write('extern volatile %s       %sINV __attribute__((section("sfrs"),address(0x%08X)));\n' % (registerSize, cname, addr+0XA0000000))
  if aliasExist!=""  and len(allCnames.keys())>1 and family=="32xxxx":
    addr=cnamesList.get(cname + 'CLR')
    if "CLR" in portals:
      outfile.write('extern volatile %s       %sCLR __attribute__((section("sfrs"),address(0x%08X)));\n' % (registerSize, aliasExist, addr+0XA0000000))
    if  "CLR" in portals:
      outfile.write('extern volatile %s       %sCLR __attribute__((section("sfrs"),address(0x%08X)));\n' % (registerSize, cname, addr+0XA0000000))

    addr=cnamesList.get(cname + 'SET')
    if  "SET" in portals:
      outfile.write('extern volatile %s       %sSET __attribute__((section("sfrs"),address(0x%08X)));\n' % (registerSize, aliasExist, addr+0XA0000000))
    if "SET"  in portals:
      outfile.write('extern volatile %s       %sSET __attribute__((section("sfrs"),address(0x%08X)));\n' % (registerSize, cname, addr+0XA0000000))

    addr=cnamesList.get(cname + 'INV')
    if "INV"  in portals:
      outfile.write('extern volatile %s       %sINV __attribute__((section("sfrs"),address(0x%08X)));\n' % (registerSize, aliasExist, addr+0XA0000000))
    if "INV" in portals:
      outfile.write('extern volatile %s       %sINV __attribute__((section("sfrs"),address(0x%08X)));\n' % (registerSize, cname, addr+0XA0000000))


  if len(allCnames.keys())>1:
    aliasExist=cname


def printExterns(outfile):

  outfile.write ("#elif defined (__LANGUAGE_ASSEMBLY__)\n")
  for cname in orderCnames:
    addr=cnamesList.get(cname)

    #TODO hardcoded offset value for virtual memory
    outfile.write("  .extern %-16s /* 0x%08X */\n"  % (cname,(addr+0XA0000000)))
  outfile.write ("#else\n")
  outfile.write ("#error Unknown language!\n")
  outfile.write ("#endif\n")
  outfile.write ("\n")
  printDefines(outfile)

def printDefines(outfile):
  i=0
  for each in bit_ListOrder:
    someValue = bitsList.get(each, None)
    if someValue is not None:
      i=i+1
      outfile.write("#define %-40s 0x%08X\n" % (each, someValue))
      if i%3==0:
        outfile.write("\n")

def printVectors(pic,outfile):

 InterruptList_node = fdom.first(pic.node, 'InterruptList')
 if len(fdom.children(InterruptList_node, 'Interrupt')) > 0:
   outfile.write ("/* Vector Numbers */\n")
   for each in fdom.children(InterruptList_node, 'Interrupt'):

     cname=fdom.get2(each, 'cname')
     irq=fdom.get2(each, 'irq')
     outfile.write ("#define %-40s %s\n" % ("_"+cname+"_VECTOR", irq))
 if len(fdom.children(InterruptList_node, 'InterruptRequest')) > 0:
   outfile.write ("\n/* IRQ Numbers */\n")
   for each in fdom.children(InterruptList_node, 'InterruptRequest'):
     cname=fdom.get2(each, 'cname')
     irq=fdom.get2(each, 'irq')
     outfile.write ("#define %-40s %s\n" % ("_"+cname+"_IRQ", irq))

 outfile.write ("\n")
 outfile.write ("/* Device Peripherals */\n")

 # Collect all of the interesting data.
 device_peripherals = []
 for addr,SFRDef_node in sorted(pic.addrOntoSFR.items(), key=lambda pair:pair[0]):
   baseofperipheral = SFRDef_node.getAttribute("ltx:baseofperipheral")
   if (baseofperipheral != "") :
     macro_names = baseofperipheral.split()
     for macro_name in macro_names:
       # Don't allow USB in these families for now. TODO
       if ((pic.name.upper()[:10] == "PIC32MX110") or (pic.name.upper()[:10] == "PIC32MX120")
         or (pic.name.upper()[:10] == "PIC32MX130") or (pic.name.upper()[:10] == "PIC32MX140")
         or (pic.name.upper()[:10] == "PIC32MX150") or (pic.name.upper()[:10] == "PIC32MX170")) :
         if (macro_name == "USB") :
           break;
       if (re.search(r"[a-z]+", macro_name)) :
         break;
       device_peripherals.append(macro_name)
   # Added for backwards compatibility
   if SFRDef_node.getAttribute("edc:cname") == "DMACON":
     device_peripherals.append("DMAC")
 device_peripherals = list(set(device_peripherals))
 device_peripherals.sort()
 for macro_name in device_peripherals:
   outfile.write('#define _%s\n' % (macro_name))

 outfile.write ("\n")
 outfile.write ("/* Base Addresses for Peripherals */\n")

 macro_defs = {}
 for addr,SFRDef_node in sorted(pic.addrOntoSFR.items(), key=lambda pair:pair[0]):
   baseofperipheral = SFRDef_node.getAttribute("ltx:baseofperipheral")
   if (baseofperipheral != "") :
     macro_names = baseofperipheral.split()
     for macro_name in macro_names:
       # Don't allow USB in these families for now. TODO
       if ((pic.name.upper()[:10] == "PIC32MX110") or (pic.name.upper()[:10] == "PIC32MX120")
         or (pic.name.upper()[:10] == "PIC32MX130") or (pic.name.upper()[:10] == "PIC32MX140")
         or (pic.name.upper()[:10] == "PIC32MX150") or (pic.name.upper()[:10] == "PIC32MX170")) :
         if (macro_name == "USB") :
           break;
       if (re.search(r"[a-z]+", macro_name)) :
         break;
       macro_defs["_%s_BASE_ADDRESS" % (re.sub("\W.*","", macro_name))] = (addr | 0xB0000000)
   else :
      # Added for backwards compatibility
      if SFRDef_node.getAttribute("edc:cname") == "DMACON":
         macro_defs["_DMAC_BASE_ADDRESS"] = (addr | 0xB0000000)

 sorted_macro_defs = sorted(macro_defs.items(), key=operator.itemgetter(0))
 for name,addr in sorted_macro_defs :
    outfile.write("#define %-40s 0x%8X\n" % (name, addr))

def printSfile(sfile):
  for each in orderCnames:
    addr=cnamesList.get(each)
  #TODO hardcoded offset value for virtual memory
    sfile.write("%s = 0x%08X\n"  % (each,(addr+0XA0000000)))
    sfile.write ("  .global %s\n" % each)


### This is popcount, the fastest method to count highbits
# http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
# http://www.valuedlessons.com/2009/01/popcount-in-python-with-benchmarks.html

POPCOUNT_TABLE16 = [0] * 2**16
for index in xrange(len(POPCOUNT_TABLE16)):
    POPCOUNT_TABLE16[index] = (index & 1) + POPCOUNT_TABLE16[index >> 1]

def popcount32_table16(v):
    return (POPCOUNT_TABLE16[ v        & 0xffff] +
            POPCOUNT_TABLE16[(v >> 16) & 0xffff])

def getconfigData(pic, addr, DCRDef_node, offset = None, version = 1):
    config_lines = []
    region_node = DCRDef_node.parentNode

    if pic.instructionSet.hasMicroMIPS:
      if fdom.has(region_node, "registerprefix"):
        preSetting = ("%s_" % fdom.get2(region_node, 'registerprefix'))
      else:
        preSetting = ""
    else:
      preSetting = ""
    cname = fdom.get2(DCRDef_node, 'cname')
    try:
      default = fdom.get2(DCRDef_node, 'default', edc.str2int)
    except:
      default = 0

    # if an offset is not specified, we attempt to grab a value
    # from the PIC, which for MIPS is specified by the ksegdef and
    # ksegN offset values on the parent node.
    # if kseg1 exists (i.e. the previous default), it will be used
    # unless ksegdef exists and is set to '1' indicating kseg0 should
    # be used.
    if offset is None:
        kseg = "kseg1"
        if fdom.has(region_node, "ksegdef"):
            ksegdef = int(fdom.get2(region_node, "ksegdef"))
            if ksegdef == 1 and fdom.has(region_node, "kseg0"):
                kseg = "kseg0"
        if fdom.has(region_node, kseg):
            offset = int(fdom.get2(region_node, kseg), 16)
        if offset is None:
            offset = 0

    if version == 1:
        config_lines.append(ConfigEntry(
            "CWORD",
            "%08X" % (addr+offset),
            "%08X" % (default),
            "%08X" % (default)))
    elif version == 2:
        bit_width = int(fdom.get2(DCRDef_node, ('nzwidth'), defaultValue="0x20"), 16)
        mask = "0xFFFFFFFF" if bit_width == 32 else "0x000000FF"
        config_lines.append(ConfigEntry(
            "CINT%d" % (bit_width),
            "%08X"   % (addr+offset),
            "%s"     % (mask),
            "%08X"   % (default)))

    if cname=="Reserved" :
        return config_lines
    langhidden=fdom.get2(DCRDef_node, 'islanghidden', defaultValue="")
    hidden=fdom.get2(DCRDef_node, 'ishidden', defaultValue="")
    if langhidden=="true" or hidden=="true":
         return config_lines
    temp=0
    cnames=[]
    val=[]
    field_alias=[]
    option_alias=[]

    DCRModeList_node = fdom.first(DCRDef_node, 'DCRModeList')
    if DCRModeList_node :
      for DCRMode_node in fdom.children(DCRModeList_node, 'DCRMode'):
        langhidden=fdom.get2(DCRMode_node, 'islanghidden', defaultValue="")
        hidden=fdom.get2(DCRMode_node, 'ishidden', defaultValue="")
        if langhidden=="true" or hidden=="true":
         continue

        for field_node, place, width in edc.visit_fields(DCRMode_node):

          langhidden=fdom.get2(field_node, 'islanghidden', defaultValue="")
          hidden=fdom.get2(field_node, 'ishidden', defaultValue="")

          if langhidden=="true" or hidden=="true":
              continue
          fname, desc = fdom.get2(field_node, ('cname', 'desc'), defaultValue="") # DCRFieldDef
          # Get register name
          ancestor_node = fdom.ancestor(field_node) # DCRMode
          ancestor_node = fdom.ancestor(ancestor_node) # DCRModeList
          ancestor_node = fdom.ancestor(ancestor_node) # DCRDef (Register node)
          fname2, desc2 = fdom.get2(ancestor_node, ('cname', 'desc'), defaultValue="")
          ancestor_node = fdom.ancestor(ancestor_node) # ConfigFuseSector
          fname3 = fdom.get2(ancestor_node, ('regionid'), defaultValue="")
          if fname!="" and fname2 != "" and fname3 != "":
            del field_alias[:]
            field_alias.append(fname3 + "_" + fname2 + "_" + fname)
            field_alias.append(fname2 + "_" + fname)
            for alias_node in fdom.children(field_node, 'AliasList'):
              for legacy_node in fdom.children(alias_node, 'LegacyAlias'):
                field_alias.append(fdom.get2(legacy_node, 'cname'))

            for aname in field_alias:
              config_lines.append(ConfigEntry(
                  "CSETTING",
                  "%08X" % ((pow (2, width) - 1) << place),
                  "%s" % (preSetting + aname),
                  "%s" % ("Alias for " + fname),
                  "%s" % (desc)))

            config_lines.append(ConfigEntry(
                "CSETTING",
                "%08X" % ((pow (2, width) - 1) << place),
                "%s" % (preSetting + fname),
                "%s" % (desc)))

          have_cvalue = 0
          for option_node in fdom.children(field_node, 'DCRFieldSemantic'):
            langhidden_FieldSemantic=fdom.get2(option_node, 'islanghidden', defaultValue="")
            hidden_FieldSemantic=fdom.get2(option_node, 'ishidden', defaultValue="")
            if langhidden_FieldSemantic=="true" or hidden_FieldSemantic=="true":
              return config_lines

            desc = ""
            if fdom.hasAttr(option_node, 'desc'):
              desc = fdom.attr(option_node, 'desc')
            cname1, when = fdom.get2(option_node, ('cname', 'when'))

            mask, value = edc.parse_when(when)
            del option_alias[:]
            for alias_node in fdom.children(option_node, 'AliasList'):
              for legacy_node in fdom.children(alias_node, 'LegacyAlias'):
                option_alias.append(fdom.get2(legacy_node, 'cname'))

            for aname in option_alias:
              config_lines.append(ConfigEntry(
                  "CVALUE",
                  "%08X" % (value << place),
                  "%s" % (aname),
                  "%s" % ("Alias for " + cname1),
                  "%s" % (desc)))
              have_cvalue = 1

            config_lines.append(ConfigEntry(
                "CVALUE",
                "%08X" % (value << place),
                "%s" % (cname1),
                "%s" % (desc)))
            have_cvalue = 1

          if not have_cvalue and (fname!=""):
            mask = edc.bitfields.maskOfBitfield(field_node)
            max = hex((1 << popcount32_table16(mask))-1)
            config_lines.append(ConfigEntry(
                "CVALUE",
                "00000000",
                "00000000",
                "Range is from 0 to %s" % max))

    return config_lines

def configDataFile(pic,configfile, htmlfile, offset = None):
  config_lines = []
  tmp = None

  # Determine if the DFP has any configuration registers with 8-bit lengths,
  # adjust version accordingly
  version = 1
  for _, DCRDef_node in pic.addrOntoDCR.iteritems():
    configFuseSector_node = DCRDef_node.parentNode
    bit_width = int(fdom.get2(DCRDef_node, ('nzwidth'), defaultValue='0x20'), 16)
    sector_name = fdom.get2(configFuseSector_node, ('regionid'), defaultValue='No Name Specified')

    if configFuseSector_node.nodeName == "edc:ConfigFuseSector":
      if bit_width not in {32, 8}:
          raise Exception("Unsupported bit width " + str(bit_width) + " for ConfigFuseSector: " + sector_name)
      elif bit_width is 8:
          version = 2

  # Write the version to file
  configfile.write(("%04d\n" % version))

  for addr, DCRDef_node in sorted(pic.addrOntoDCR.iteritems(), key=lambda pair:pair[0]):
    tmp = getconfigData(pic, addr, DCRDef_node, offset, version)
    if tmp:
        config_lines += tmp

  for addr, AltDCRDef_node in sorted(pic.addrOntoAltDCR.iteritems(), key=lambda pair:pair[0]):
    tmp = getconfigData(pic, addr, AltDCRDef_node, offset, version)
    if tmp:
        config_lines += tmp

  seen = set()
  duplicates = set()

  # detect duplicate CSETTING names
  for line in config_lines:
    if line.field3 in seen and line.field1 == "CSETTING":
        duplicates.add(line.field3)
    else:
        seen.add(line.field3)
  # Copy entire list except duplicate CSETTING, preserving order of elements
  uniqueified_config_lines = [line for line in config_lines if not (line.field3 in duplicates)]

  # actually write configuration.data and html files.
  # For CSETTING entries
  # field4 is the description of the config word. alt_field4 contains the text
  # from PIC file instead of "alias for ..." (micromips)
  last_line = None
  last_csetting = None
  first = True
  for line in uniqueified_config_lines:
      if line.field1 == "CSETTING" and first != True:
        htmlfile.write("</table>\n\n")

      if line.field1 == "CSETTING":
        # detect if current line is a disambiguated entry
        # short name
        if last_line and last_line.field3.endswith(line.field3):
            line_to_write = ("%s:%s:%s:%s\n" % (line.field1,
                                            line.field2,
                                            line.field3,
                                            "Short name for %s" % (last_line.field3)))
            htmlfile.write("<table frame=box rules=cols summary='%s' style='margin:10px;' cellpadding=3><tr style=\"border: 2px solid;\"><th>%s</th><th>%s</th></tr>" % ("Short name for %s\n" % (last_line.field3), line.field3, "Short name for %s\n" % (last_line.field3)))
        else:
            if line.alt_field4 != None:
                line_to_write = ("%s:%s:%s:%s\n" % (line.field1,
                                                line.field2,
                                                line.field3,
                                                line.alt_field4))
                htmlfile.write("<table frame=box rules=cols summary='%s' style='margin:10px;' cellpadding=3><tr style=\"border: 2px solid;\"><th>%s</th><th>%s</th></tr>" % (line.alt_field4, line.field3, line.alt_field4))
            else:
                line_to_write = ("%s:%s:%s:%s\n" % (line.field1,
                                                line.field2,
                                                line.field3,
                                                "Short name for %s" % (last_line.field3)))
                htmlfile.write("<table frame=box rules=cols summary='%s' style='margin:10px;' cellpadding=3><tr style=\"border: 2px solid;\"><th>%s</th><th>%s</th></tr>" % ("Short name for %s\n" % (last_line.field3), line.field3, "Short name for %s\n" % (last_line.field3)))

        last_csetting = line
        first = False
      # cvalue/cword
      else:
            line_to_write = ("%s:%s:%s:%s\n" % (line.field1,
                                            line.field2,
                                            line.field3,
                                            line.field4))
            if line.field1 == "CVALUE":
                htmlfile.write("<tr><td>%s = %s</td><td><font size=\"2\">\n" % (last_csetting.field3, line.field3))
                if line.alt_field4:
                    htmlfile.write("%s\n" % line.alt_field4)
                elif line.field4:
                    htmlfile.write("%s\n" % line.field4)
                htmlfile.write("</font></td></tr>\n")

      configfile.write(line_to_write)
      last_line = line
  htmlfile.write("</table>\n\n")

def printKludge(cname,incfile):
      incfile.write ("\n");
      incfile.write ("        .extern %s\n" % (cname));
      incfile.write ("        .equiv  %sL, _%s\n" % (cname, cname));
      incfile.write ("        .equiv  %sH, _%s+1\n" % (cname, cname));

def makeunsigned (num) :
  if (num < 0) :
    num += ((sys.maxint + 1L) * 2L)

  return num;

def printInfoforHeader(pic,family,outfile,processHeader,cname,addr):
   p30f_ic = re.compile("IC(.)BUF")
   p30f_oc = re.compile("OC(.)RS")
   p30f_urt = re.compile("U(.)MODE")
   p30f_spi = re.compile("SPI(.)STAT")
   p30f_can = re.compile("C(.)RXF0SID")
   p30f_acc = re.compile("ACC(.)L")

   if (family == "30xxxx" ):
      ic_structure = [
          "\n",
          "/* Generic structure of entire SFR area for Input Capture modules */\n",
          "typedef struct tagIC {\n",
          "        unsigned int icxbuf;\n",
          "        unsigned int icxcon;\n",
          "} IC, *PIC;\n",
          "\n"
          ]
      oc_structure = [
          "\n",
          "/* Generic structure of entire SFR area for Output Compare modules */\n",
          "typedef struct tagOC {\n",
          "        unsigned int ocxrs;\n",
          "        unsigned int ocxr;\n",
          "        unsigned int ocxcon;\n",
          "} OC, *POC;\n",
          "\n"
          ]
      urt_structure = [
          "\n",
          "/* Generic structure of entire SFR area for each UART module */\n",
          "typedef struct tagUART {\n",
          "        unsigned int uxmode;\n",
          "        unsigned int uxsta;\n",
          "        unsigned int uxtxreg;\n",
          "        unsigned int uxrxreg;\n",
          "        unsigned int uxbrg;\n",
          "} UART, *PUART;\n",
          "\n",
          "#define UxMODEBITS U1MODEBITS\n",
          "#define UxSTABITS U1STABITS\n",
          "#define UxTXREGBITS U1TXREGBITS\n",
          "#define UxRXREGBITS U1RXREGBITS\n",
          "#define UxBRGBITS U1BRGBITS\n",
          "\n"
          ]
      spi_structure = [
          "\n",
          "/* Generic structure of entire SFR area for each SPI module */\n",
          "typedef struct tagSPI {\n",
          "        unsigned int spixstat;\n",
          "        unsigned int spixcon1;\n",
          "        unsigned int spixcon2;\n",
          "        unsigned int unused;\n",
          "        unsigned int spixbuf;\n",
          "} SPI, *PSPI;\n",
          "\n",
          "#define SPIxSTATBITS SPI1STATBITS\n",
          "#define SPIxCONBITS SPI1CONBITS\n",
          "\n"
          ]
      can_structure = [
          "\n",
          "/* Generic structure of entire SFR area for each CAN module */\n",
          "typedef struct tagCAN {\n",
          "        unsigned int cxrxf0sid;\n",
          "        unsigned int cxrxf0eidh;\n",
          "        unsigned int cxrxf0eidl;\n",
          "        unsigned int reserved0;\n",
          "        unsigned int cxrxf1sid;\n",
          "        unsigned int cxrxf1eidh;\n",
          "        unsigned int cxrxf1eidl;\n",
          "        unsigned int reserved1;\n",
          "        unsigned int cxrxf2sid;\n",
          "        unsigned int cxrxf2eidh;\n",
          "        unsigned int cxrxf2eidl;\n",
          "        unsigned int reserved2;\n",
          "        unsigned int cxrxf3sid;\n",
          "        unsigned int cxrxf3eidh;\n",
          "        unsigned int cxrxf3eidl;\n",
          "        unsigned int reserved3;\n",
          "        unsigned int cxrxf4sid;\n",
          "        unsigned int cxrxf4eidh;\n",
          "        unsigned int cxrxf4eidl;\n",
          "        unsigned int reserved4;\n",
          "        unsigned int cxrxf5sid;\n",
          "        unsigned int cxrxf5eidh;\n",
          "        unsigned int cxrxf5eidl;\n",
          "        unsigned int reserved5;\n",
          "        unsigned int cxrxm0sid;\n",
          "        unsigned int cxrxm0eidh;\n",
          "        unsigned int cxrxm0eidl;\n",
          "        unsigned int reserved6;\n",
          "        unsigned int cxrxm1sid;\n",
          "        unsigned int cxrxm1eidh;\n",
          "        unsigned int cxrxm1eidl;\n",
          "        unsigned int reserved7;\n",
          "        unsigned int cxtx2sid;\n",
          "        unsigned int cxtx2eid;\n",
          "        unsigned int cxtx2dlc;\n",
          "        unsigned int cxtx2b1;\n",
          "        unsigned int cxtx2b2;\n",
          "        unsigned int cxtx2b3;\n",
          "        unsigned int cxtx2b4;\n",
          "        unsigned int cxtx2con;\n",
          "        unsigned int cxtx1sid;\n",
          "        unsigned int cxtx1eid;\n",
          "        unsigned int cxtx1dlc;\n",
          "        unsigned int cxtx1b1;\n",
          "        unsigned int cxtx1b2;\n",
          "        unsigned int cxtx1b3;\n",
          "        unsigned int cxtx1b4;\n",
          "        unsigned int cxtx1con;\n",
          "        unsigned int cxtx0sid;\n",
          "        unsigned int cxtx0eid;\n",
          "        unsigned int cxtx0dlc;\n",
          "        unsigned int cxtx0b1;\n",
          "        unsigned int cxtx0b2;\n",
          "        unsigned int cxtx0b3;\n",
          "        unsigned int cxtx0b4;\n",
          "        unsigned int cxtx0con;\n",
          "        unsigned int cxrx1sid;\n",
          "        unsigned int cxrx1eid;\n",
          "        unsigned int cxrx1dlc;\n",
          "        unsigned int cxrx1b1;\n",
          "        unsigned int cxrx1b2;\n",
          "        unsigned int cxrx1b3;\n",
          "        unsigned int cxrx1b4;\n",
          "        unsigned int cxrx1con;\n",
          "        unsigned int cxrx0sid;\n",
          "        unsigned int cxrx0eid;\n",
          "        unsigned int cxrx0dlc;\n",
          "        unsigned int cxrx0b1;\n",
          "        unsigned int cxrx0b2;\n",
          "        unsigned int cxrx0b3;\n",
          "        unsigned int cxrx0b4;\n",
          "        unsigned int cxrx0con;\n",
          "        unsigned int cxctrl;\n",
          "        unsigned int cxcfg1;\n",
          "        unsigned int cxcfg2;\n",
          "        unsigned int cxintf;\n",
          "        unsigned int cxinte;\n",
          "        unsigned int cxec;\n",
          "        unsigned int reserved8[50];\n",
          "} CAN, *PCAN;\n"
          ]
      acc_structure = [
          "typedef struct tagACC {\n",
          "    unsigned int L;\n",
          "    unsigned int H;\n",
          "    unsigned char U;\n",
          "} ACC;\n"
          ]

      # begin Input Capture text
      if p30f_ic.search(cname) :
        re_match = p30f_ic.search(cname)

        if (re_match.group(1) == "1"):
          # preamble text for all devices
          for line in ic_structure:
            if processHeader=="True":
              outfile.write(line)
        if  cname.startswith("IC")and cname.endswith("BUF"):
          base_addresses.update({cname.replace("BUF",""):addr})
          if processHeader=="True":

           outfile.write ("\n/* SFR blocks for Input Capture modules */\n")
           outfile.write("#define IC%c IC%c\n" % (re_match.group(1), re_match.group(1)))
           outfile.write ("extern volatile IC IC%c __attribute__((__sfr__));\n" % re_match.group(1))
           outfile.write ("\n")

      # begin Output Compare text
      elif p30f_oc.search(cname)  :
        re_match = p30f_oc.search(cname)

        if (re_match.group(1) == "1"):
          # preamble text for all devices
          for line in oc_structure:
            if processHeader=="True":
              outfile.write(line)
        if  cname.startswith("OC")and cname.endswith("RS"):
          base_addresses.update({cname.replace("RS",""):addr})
          if processHeader=="True":

           outfile.write ("\n/* SFR blocks for Output Compare modules */\n")
           outfile.write("#define OC%c OC%c\n" % (re_match.group(1), re_match.group(1)))
           outfile.write ("extern volatile OC OC%c __attribute__((__sfr__));\n" % re_match.group(1))
           outfile.write ("\n")

      # begin UART text
      elif p30f_urt.search(cname)  :
        re_match = p30f_urt.search(cname)

        if (re_match.group(1) == "1"):
          # preamble text for all devices
          for line in urt_structure:
             if processHeader=="True":
              outfile.write(line)
        if  cname.startswith("U")and cname.endswith("MODE"):
          base_addresses.update({"U"+"ART"+re.sub("\D", "", cname):addr})
          if processHeader=="True":
           outfile.write ("\n/* SFR blocks for each UART module */\n")
           outfile.write("#define UART%c UART%c\n" % (re_match.group(1), re_match.group(1)))
           outfile.write ("extern volatile UART UART%c __attribute__((__sfr__));\n" % re_match.group(1))
           outfile.write ("\n")

      # begin SPI text
      elif p30f_spi.search(cname)  :
        re_match = p30f_spi.search(cname)

        if (re_match.group(1) == "1"):
          for line in spi_structure:
             if processHeader=="True":
              outfile.write(line)
        if  cname.startswith("SPI")and cname.endswith("STAT"):
          base_addresses.update({cname[:4]:addr})

        if processHeader=="True":
         outfile.write ("\n/* SFR blocks for each SPI module */\n")
         outfile.write("#define SPI%c SPI%c\n" % (re_match.group(1), re_match.group(1)))
         outfile.write ("extern volatile SPI SPI%c __attribute__((__sfr__));\n" % re_match.group(1))
         outfile.write ("\n")

      # begin CAN text
      elif p30f_can.search(cname):
        re_match = p30f_can.search(cname)

        if (re_match.group(1) == "1"):
        # preamble text for all devices
          for line in can_structure:
             if processHeader=="True":
              outfile.write(line)
        base_addresses.update({"CAN"+re.sub("\D", "", cname[:5]):addr})
        print cname
        print "!!!!!!!!!!!!!!!!!!!!!!!!1"
        if processHeader=="True":
         outfile.write ("\n/* SFR blocks for each CAN module */\n")
         outfile.write("#define CAN%c CAN%c\n" % (re_match.group(1), re_match.group(1)))
         outfile.write ("extern volatile CAN CAN%c __attribute__((__sfr__));\n" % re_match.group(1))
         outfile.write ("\n")


            # begin CAN text
      elif p30f_acc.search(cname) :
        re_match = p30f_acc.search(cname)

        if (re_match.group(1) == "A"):
        # preamble text for all devices
          for line in acc_structure:
             if processHeader=="True":
              outfile.write(line)
        if  cname== "ACCAL" :
          print family
          print pic.name
          print cname
          base_addresses.update({"ACCA":addr})
        if  cname== "ACCBL" :
          base_addresses.update({"ACCB":addr})
        if processHeader=="True":
         outfile.write ("\n/* SFR blocks for each ACC module */\n")
         outfile.write("#define ACC%c ACC%c\n" % (re_match.group(1), re_match.group(1)))
         outfile.write ("extern volatile ACC ACC%c __attribute__((__sfr__));\n" % re_match.group(1))
         outfile.write ("\n")

   else:

      # begin Input Capture text
      if cname == "IC1BUF" and processHeader=="True":
        # preamble text for all devices
        outfile.write ("\n")
        outfile.write ("/* Generic structure of entire SFR area for Input Capture modules */\n")
        outfile.write ("typedef struct tagIC {\n")
        outfile.write ("        unsigned int icxbuf;\n")
        outfile.write ("        unsigned int icxcon;\n")
        outfile.write ("} IC, *PIC;\n")
        outfile.write ("\n")

      if  cname.startswith("IC")and cname.endswith("BUF"):
        base_addresses.update({cname.replace("BUF",""):addr})
        if processHeader=="True":
         outfile.write ("/* SFR blocks for Input Capture modules */\n")
         outfile.write("#define IC%s IC%s\n" % (re.sub("\D", "", cname), re.sub("\D", "", cname) ))
         outfile.write ("extern volatile IC IC%s __attribute__((__sfr__));\n" %re.sub("\D", "", cname) )
         outfile.write ("\n")

      # begin Output Compare text
      if cname == "OC1RS" and  processHeader=="True":
        # preamble text for all devices
        outfile.write ("\n")
        outfile.write ("/* Generic structure of entire SFR area for Output Compare modules */\n")
        outfile.write ("typedef struct tagOC {\n")
        outfile.write ("        unsigned int ocxrs;\n")
        outfile.write ("        unsigned int ocxr;\n")
        outfile.write ("        unsigned int ocxcon;\n")
        outfile.write ("} OC, *POC;\n")
        outfile.write ("\n")

      if  cname.startswith("OC")and cname.endswith("RS"):
        base_addresses.update({cname.replace("RS",""):addr})
        if processHeader=="True":
         outfile.write ("/* SFR blocks for Output Compare modules */\n")
         outfile.write("#define OC%s OC%s\n" % (re.sub("\D", "", cname),re.sub("\D", "", cname)))
         outfile.write ("extern volatile OC OC%s __attribute__((__sfr__));\n" %re.sub("\D", "", cname) )
         outfile.write ("\n")


      # begin UART text
      if cname == "U1MODE" :
        # preamble text for all devices
       if processHeader=="True" :
        outfile.write ("\n")
        outfile.write ("/* Generic structure of entire SFR area for each UART module */\n")
        outfile.write ("typedef struct tagUART {\n")
        outfile.write ("        unsigned int uxmode;\n")
        outfile.write ("        unsigned int uxsta;\n")
        outfile.write ("        unsigned int uxtxreg;\n")
        outfile.write ("        unsigned int uxrxreg;\n")
        outfile.write ("        unsigned int uxbrg;\n")
        outfile.write ("} UART, *PUART;\n")
        outfile.write ("\n")
        outfile.write ("#define UxMODEBITS U1MODEBITS\n")
        outfile.write ("#define UxSTABITS U1STABITS\n")
        outfile.write ("#define UxTXREGBITS U1TXREGBITS\n")
        outfile.write ("#define UxRXREGBITS U1RXREGBITS\n")
        outfile.write ("#define UxBRGBITS U1BRGBITS\n")
        outfile.write ("\n")
        outfile.write ("/* SFR blocks for each UART module */\n")

        # all 24F, 24H, 30F, and 33F devices have 2 UART modules
        if (family in ("24xxxx", "30xxxx", "33xxxx")) :
          for i in range(1,3) :
            outfile.write("#define UART%d UART%d\n" % (i, i))
            outfile.write ("extern volatile UART UART%d __attribute__((__sfr__));\n" % i)
          outfile.write ("\n")
      #end UART text
      if  cname.startswith("U")and cname.endswith("MODE"):
        base_addresses.update({"U"+"ART"+re.sub("\D", "", cname):addr})

      # begin SPI text
      if cname == "SPI1STAT"  :
        # preamble text for all devices
       if processHeader=="True" :
        outfile.write ("\n")
        outfile.write ("/* Generic structure of entire SFR area for each SPI module */\n")
        outfile.write ("typedef struct tagSPI {\n")
        outfile.write ("        unsigned int spixstat;\n")
        outfile.write ("        unsigned int spixcon1;\n")
        outfile.write ("        unsigned int spixcon2;\n")
        outfile.write ("        unsigned int unused;\n")
        outfile.write ("        unsigned int spixbuf;\n")
        outfile.write ("} SPI, *PSPI;\n")
        outfile.write ("\n")
        outfile.write ("#define SPIxSTATBITS SPI1STATBITS\n")
        outfile.write ("#define SPIxCONBITS SPI1CONBITS\n")
        outfile.write ("\n")
        outfile.write ("/* SFR blocks for each SPI module */\n")

        # all 24F, 24H, 30F, and 33F devices have 2 SPI modules
        if (family in ("24xxxx", "30xxxx", "33xxxx")) :
          for i in range(1,3) :
            outfile.write("#define SPI%d SPI%d\n" % (i, i))
            outfile.write ("extern volatile SPI SPI%d __attribute__((__sfr__));\n" % i)
          outfile.write ("\n")
      #end SPI text
      if  cname.startswith("SPI")and cname.endswith("STAT"):
        base_addresses.update({cname[:4]:addr})

      # begin CAN text
      if cname == "FIRST_CAN_REGISTER" :
        # preamble text for all devices
       if processHeader=="True":
        outfile.write ("\n")
        outfile.write ("/* Generic structure of entire SFR area for each CAN module */\n")
        outfile.write ("typedef struct tagCAN {\n")
        # need Apps to define this
        outfile.write ("} CAN, *PCAN;\n")
        outfile.write ("\n")
        # no 30F compatibility defines required
        outfile.write ("\n")
        outfile.write ("/* SFR blocks for each CAN module */\n")
        print cname
        print "!!!!!!!!!!!!!!!!!!!"
       base_addresses.update({"CAN"+re.sub("\D", "", cname):addr})
      if  cname== "ACCAL" :
        base_addresses.update({"ACCA":addr})
      if  cname== "ACCBL" :
        base_addresses.update({"ACCB":addr})
