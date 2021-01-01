#!/bin/bash

# This script is supposed to run on chn-sv-tornado build
# machine since it has the latest required Xcode version.
# This should be temporary until we update other OSX build
# machines and install the required certificate.

FILE=$1
KEYCHAIN=$2
PSWD=$3
CERT_SHA="423E4D09F3A7A393E811F3CBB66A2E7E478700ED"

if [ $# -ne 3 ]; then
  echo "Usage: remote_codesign.sh <file> <keychain> <keychain_passwd>"
  exit
fi

security unlock-keychain -p ${PSWD} ${KEYCHAIN}
echo "Signing ${FILE}"
cp "${FILE}" "${FILE}".signed
codesign --options runtime --timestamp -vvv -s ${CERT_SHA} "${FILE}".signed
