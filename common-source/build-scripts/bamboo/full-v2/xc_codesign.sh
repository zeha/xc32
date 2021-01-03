#!/bin/bash

UNAME="bamboo"
HNAME="chn-sv-tornado"

DIR=$1
PROJECT=$2

KEYCHAIN="xc32_codesign.keychain"
KEYCHAIN_PSWD="savethebeer"
SIGN_SCRIPT="./build-scripts/bamboo/full-v2/remote_xc_codesign.sh"

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

ssh "${REMOTE_ADDR}" mkdir -p "${REMOTE_DIR}"
scp ${SIGN_SCRIPT} "${REMOTE_ADDR}":"${REMOTE_DIR}"/"$(basename ${SIGN_SCRIPT})"

for f in $FILES; do
  if ( file "$f" | grep -q Mach-O ); then
    file=$(basename "$f")
    if [[ "${file}" == *xclm* ]]; then
        echo "Skiping: ${file}"
        continue
    fi
    scp "$f" "${REMOTE_ADDR}":"${REMOTE_DIR}"
    ssh "${REMOTE_ADDR}" "${REMOTE_DIR}"/"$(basename ${SIGN_SCRIPT})" "${REMOTE_DIR}"/"${file}" "${KEYCHAIN}" "${KEYCHAIN_PSWD}"
    scp "${REMOTE_ADDR}":"${REMOTE_DIR}"/"${file}".signed "${f}"
  fi
done
