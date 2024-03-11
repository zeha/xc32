#!/bin/bash

# This script is dependent on 
# https://bitbucket.microchip.com/projects/XCC/repos/xc-codesign/browse/codesigner.sh

UNAME="bamboo"
HNAME="chn-sv-tornado"

DIR=$1
PROJECT=$2

KEYCHAIN="xc32_codesign.keychain"
KEYCHAIN_PSWD="savethebeer"
XC_CODESIGN_DIR="./xc-codesign"
SIGN_SCRIPT="$XC_CODESIGN_DIR/codesigner.sh"

if [ -z "${DIR}" ] || [ ! -d "${DIR}" ]; then
  echo "Please specify install directory"
  exit
fi

if [ -z "${PROJECT}" ] ; then
  echo "Please specify target project. Eg: xc32"
  exit
fi

echo "Finding executables"

FILES=$(find "$DIR" -print)
TGT_DIR="build_$(date +%Y%m%d)"

REMOTE_DIR=${PROJECT}/${TGT_DIR}
REMOTE_ADDR=${UNAME}@${HNAME}

ssh "${REMOTE_ADDR}" mkdir -p "${REMOTE_DIR}/${XC_CODESIGN_DIR}"
scp "${XC_CODESIGN_DIR}"/* "${REMOTE_ADDR}":"${REMOTE_DIR}"/"$(basename ${XC_CODESIGN_DIR})"

for f in $FILES; do
  if ( file "$f" | grep -q Mach-O ); then
    file=$(basename "$f")
    if [[ "${file}" == *xclm* ]]; then
        echo "Skiping: ${file}"
        continue
    fi
    ssh "${REMOTE_ADDR}" mkdir -p "${REMOTE_DIR}/$(dirname $f)"
    scp "$f" "${REMOTE_ADDR}":"${REMOTE_DIR}/$f"
  fi
done

ssh "${REMOTE_ADDR}" "${REMOTE_DIR}"/"$(basename ${XC_CODESIGN_DIR})"/"$(basename ${SIGN_SCRIPT})" "${REMOTE_DIR}"

for f in $FILES; do
  if ( file "$f" | grep -q Mach-O ); then
    file=$(basename "$f")
    if [[ "${file}" == *xclm* ]]; then
        echo "Skiping: ${file}"
        continue
    fi
    scp "${REMOTE_ADDR}":"${REMOTE_DIR}"/"${f}" "${f}"
    ssh "${REMOTE_ADDR}" rm -r "${REMOTE_DIR}"/"${f}"
  fi
done
