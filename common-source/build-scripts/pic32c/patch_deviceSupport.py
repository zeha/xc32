import sys
import re
import os
from lxml import etree

def parse_devices(mcus_def_file):
	pic32c_devices = []
	with open(mcus_def_file, 'r') as fp:
		for line in fp:
			match_obj = re.search('PIC32C_MCU', line)
			if match_obj:
				str_cols = re.sub('[ (),\"]+', ' ', match_obj.string).strip().split(' ')
				if str_cols[1] == "NAME":
					continue
				else:
					device_name = str_cols[1]
					pic32c_devices.append(device_name)
	return pic32c_devices
	
def patch_device_support(device_support_file, devices):
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(device_support_file, parser=parser)
	root = tree.getroot()
	MP = "{%s}" % root.nsmap['mp']
	family = root.find('mp:family', root.nsmap)
	
	existing_devices = family.findall('mp:device', root.nsmap)
	existing_devices_names = [d.get(etree.QName(root.nsmap['mp'], 'name')) for d in existing_devices]
	for device_name in devices:
		if device_name.startswith('32'):
			device_name = 'PIC' + device_name
		if not device_name in existing_devices_names:
			newElem = etree.SubElement(family, etree.QName(root.nsmap['mp'], 'device'))
			newElem.attrib[etree.QName(root.nsmap['mp'], 'name')] = device_name
	tree.write(device_support_file, pretty_print=True, xml_declaration=True, encoding="utf-8", standalone=False)

if __name__ == "__main__":
	if len(sys.argv) != 3:
		print "Usage: python patch_deviceSupport.py <pic32c-mcus.def> <deviceSupport.xml>"
		exit(1)
	pic32c_devices = parse_devices(sys.argv[1])
	patch_device_support(sys.argv[2], pic32c_devices)
	exit(0)