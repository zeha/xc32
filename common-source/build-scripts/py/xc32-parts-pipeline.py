#!/usr/bin/python

# Fetches devices which are supported or to-be-supported by XC32 from the 
# JIRA E-BUS (Development Tools Support Schedule).
# Script makes use of the JIRA Python API: 
#     https://jira.readthedocs.io/en/master/
# For now saves the devices and their fields into the xc32-devices.json file.
from jira import JIRA
import re
import os
import json
import sys

# List of Jira name vs customfield values
# Field Name                     Jira name          Parent Jira Type
# DOS# and Rev                   customfield_13722  Epic
# DOS Link                       customfield_13723  Epic
# Silicon Contact                customfield_13724  Epic
# Apps Contact                   customfield_13725  Epic
# Marketing Contact              customfield_13726  Epic
# Dev Tools Contact              customfield_13727  Epic
# DDT info Needed by             customfield_13728  Device
# Internal Eng Samples date      customfield_13730  Device
# Business Unit                  customfield_13720  Epic
# Assembler/Linker Yellow Light  customfield_13732  Device
# Target Yellow Light C Compiler customfield_13733  Device
# Target Yellow Light Studio DFP customfield_14537  Device
# Epic Link                      customfield_11220  Device
# Epic Name                      customfield_11221  Epic

dos_rev='customfield_13722'
dos_link='customfield_13723'
silicon_contact='customfield_13724'
apps_contact='customfield_13725'
marketing_contact='customfield_13726'
dev_contact='customfield_13727'
ddt_info='customfield_13728'
internal_eng='customfield_13730'
business_unit='customfield_13720'
assembler_yellow = 'customfield_13732'
c_yellow = 'customfield_13733'
dfp_yellow = 'customfield_14537'
epic_link = 'customfield_11220'
epic_name = 'customfield_11221'

# Do a jira search for the search_string and fields with a start number.
def _search_issues(jira, search_string, fields=None, startAt=0):
  issues = jira.search_issues(search_string,
                            json_result=True,
                            fields=fields,
                            startAt=startAt)
  return issues

# Search for search_string and return all issues irrespective of max.
def _search_issues_all(jira, search_string, fields=None):
  issues = _search_issues(jira, search_string,fields, 0)
  total = len(issues['issues'])
  while total < issues['total']:
    more_issues = _search_issues(jira, search_string, fields, total)
    issues['issues'] += more_issues['issues']
    total = len(issues['issues'])
  return issues

def usage():
  print "Usage: xc32-parts-pipeline.py <output filename> [-v <XC32 version e.g: 2.10>]"
  exit()

def main():
  # by default search for all devices with the "Target Yellow C Compiler"
  # field containing XC32
  search_all = True
  jsonout_enabled = False
  # TODO implement proper arg parsing
  if len(sys.argv) < 2:
    usage()
  output_filename = sys.argv[1]

  if len(sys.argv) > 2:
    if sys.argv[2] == "-v" and len(sys.argv) == 4:
      # let the user specify the XC32 version
      search_all = False
      version = sys.argv[3]
    else:
      usage()

  jiraUsername = "TestingQA_DevTools"
  jiraPassword = "devsys"
  jiraServer = 'https://jira.microchip.com:8443'
  jira = JIRA(jiraServer,basic_auth=(jiraUsername, jiraPassword))

  search_string = 'project=BUS and issuetype=Device'
  # E-BUS specific hard-coded conditions
  search_string += ' and summary!~"Integrated Tasks"'
  search_string += ' and issuekey>BUS-5876 and issuekey!=BUS-31558'

  if search_all:
    search_string += ' and "Target Yellow Light C Compiler"~XC32'
  else:
    search_string += ' and "Target Yellow Light C Compiler"~XC32.'
    search_string += version

  # print search_string

  # only the summary and c_yellow field are of interest and usable at the moment
  # TODO: clarify what else can be used for automating part-support based on
  # E-BUS
  device_fields = "summary"
  device_fields += "," + ddt_info
  device_fields += "," + internal_eng
  device_fields += "," + business_unit
  device_fields += "," + assembler_yellow
  device_fields += "," + c_yellow
  device_fields += "," + dfp_yellow
  device_fields += "," + epic_link
  device_issues = _search_issues_all(jira, search_string, device_fields)

  # save results to json file
  if jsonout_enabled:
    with open("xc32-devices.json", 'w') as fp:
      json.dump(device_issues, fp, sort_keys=True, indent=4, separators=(',', ': '))

  device_names = [ device['fields']['summary'] for device in device_issues['issues']]

  # save device names, xc32 version to text file
  with open(output_filename, 'w') as fp:
    for device in sorted(device_names):
      fp.write(device + '\n')

if __name__ == "__main__":
  main()