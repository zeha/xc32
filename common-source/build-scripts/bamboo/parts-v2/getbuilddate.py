# Script used by to get builddate from deviceSupport.xml

import sys
import xml.dom.minidom as minidom

filename = sys.argv[1]
doc = minidom.parse(filename)

deviceSupportElem = doc.getElementsByTagName('mp:deviceSupport')[0]
print deviceSupportElem.getAttribute('mp:builddate')
