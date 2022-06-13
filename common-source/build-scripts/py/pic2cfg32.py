#!/usr/bin/env python

'''
'pic2cfg32.py' generates configuration data for 32-bit devices.

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
    usage='Usage: pic2cfg32c.py picfile1 picfile2 picfile3...\n\n%s' % (
        __doc__.strip()))
_optionParser.add_option('-a', '--arch', dest='arch', help='select arch (m|c)', default='c')

# Add a summary of command-line options to the module docstring for
# Sphinx.
__doc__ += optionhelp.makeOptionSummary(_optionParser)

class _options(object):
    _mparams = { 'name': lambda s: s.replace("PIC32","32").upper(),
                 'version': 'Daytona',
                 'offset': 0xA0000000
               }
    _cparams = { 'name': lambda s: s.upper(),
                 'version': 'Microchip',
                 'offset': 0
               }
    def __init__(self,arch):
        if arch in ['c','C']:
            self._params = _options._cparams
        elif arch in ['m','M']:
            self._params = _options._mparams
        else:
            raise RuntimeError, 'invalid arch value ' + arch
    def __getattr__(self,name):
        if name in self._params:
            return self._params[name]
        raise AttributeError

namewithoutpic = ""
archopts = None

def parseOptions():
    '''Parse the command-line options (callback function).'''
    global archopts 
    options, args = _optionParser.parse_args()
    cmdlinetools.make_input_filename_list(args, _optionParser)
    archopts = _options(options.arch) 

def processSingleInputFile():
    '''Process a single input \*.PIC file (callback function).'''
    translatePICFile(cmdlinetools.long_filename)

def translatePICFile(path):
    '''Generate XC32 configuration data from a path to a \*.PIC file.'''
    doc = None
    try:
        pic = edc.PIC(path)
        doc = pic.doc
        translatePIC(pic)
    finally:
        domtools.unlink(doc)

def translatePIC(pic):
    '''Generate XC32 configuration data from an edc.PIC object.'''
    global namewithoutpic
    try:
        printConfigFiles(pic)
    finally:
        namewithoutpic = ""

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

def printConfigFiles(pic):
    global archopts
    picname=pic.name
    family=pic.arch
    namewithoutpic=picname.replace("PIC32", "32").upper()
    namewithoutpic=(archopts.name)(picname)
    path=os.getcwd()
    configpath=os.path.join(path,'32Family', 'config_docs')
    if not os.path.exists(configpath):
        os.makedirs(configpath)
    path=os.path.join(path,'32Family', namewithoutpic)
    if not os.path.exists(path):
        os.makedirs(path)

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
            configfile.write (archopts.version + " Configuration Word Definitions: 0001\n")
            langcommon.configDataFile(pic, configfile, htmlfile,  archopts.offset)

if __name__ == '__main__':
    cmdlinetools.main(parseOptions, processSingleInputFile)

