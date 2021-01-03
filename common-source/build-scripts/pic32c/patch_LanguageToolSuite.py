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

def add_device(root, family_elem, device):
	# add lti:Device element
	new_device_elem = etree.SubElement(family_elem, etree.QName(root.nsmap['lti'], 'Device'))
	# PIC32 device names in .LanguageToolSuite file start with '32'
	device_to_add = device
	if device.startswith('PIC'):
		device_to_add = device[3:]
	new_device_elem.attrib[etree.QName(root.nsmap['lti'], 'device')] = device_to_add

def add_new_family(parent, family_name, devices):
	#add lti:DeviceFamily container element
	new_family_elem = etree.SubElement(parent, etree.QName(parent.nsmap['lti'], 'DeviceFamily'))
	new_family_elem.attrib[etree.QName(parent.nsmap['lti'], 'family')] = family_name

	# add lti:IncludeDir element
	include_dir = etree.SubElement(new_family_elem, etree.QName(parent.nsmap['lti'], 'IncludeDir'))
	include_dir.attrib[etree.QName(parent.nsmap['lti'], 'dir')] = 'pic32c/include'
	for device in devices:
		if device.startswith(family_name):
			add_device(parent, new_family_elem, device)


def get_family_elem(root, family_name):
	existing_families = root.findall('lti:DeviceFamily', root.nsmap)
	for f in existing_families:
		if f.get(etree.QName(root.nsmap['lti'], 'family')) == family_name:
			return f
	return None

def update_family(root, family_name, devices):
	family_elem = get_family_elem(root, family_name)
	if family_elem is None:
		return
	existing_devices = family_elem.findall('lti:Device', root.nsmap)
	existing_device_names = [d.get(etree.QName(root.nsmap['lti'], 'device')) for d in existing_devices]
	for device in devices:
		if device.startswith(family_name):
			device_to_search = device
			if device.startswith('PIC'):
				device_to_search = device[3:]
			if not device_to_search in existing_device_names:
				print "Add missing device " + device
				add_device(root, family_elem, device)
	
def patch_file(langugage_file, devices):
	parser = etree.XMLParser(remove_blank_text=True)
	tree = etree.parse(langugage_file, parser=parser)
	root = tree.getroot()
	LTI = "{%s}" % root.nsmap['lti']
	
	existing_families = root.findall('lti:DeviceFamily', root.nsmap)
	existing_family_names = [d.get(etree.QName(root.nsmap['lti'], 'family')) for d in existing_families]
	
	# add new PIC32C and CEC families if not already in the file
	# TODO some other family idefntification method to fix hardcoded new families list
	new_families = ['ATSAM', 'MEC17', 'CEC']
	for new_family_name in new_families:
		if not new_family_name in existing_family_names:
			add_new_family(root, new_family_name, devices)
		else:
			# add new devices in already existing families
			update_family(root, new_family_name, devices)

	tree.write(langugage_file, pretty_print=True, xml_declaration=True, encoding="utf-8", standalone=False)

if __name__ == "__main__":
	if len(sys.argv) != 3:
		print "Usage: python patch_LanguageToolSuite.py <pic32c-mcus.def> <.LanguageToolSuite>"
		exit(1)
	pic32c_devices = parse_devices(sys.argv[1])
	patch_file(sys.argv[2], pic32c_devices)
	exit(0)