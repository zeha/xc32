#!/bin/bash

UNAME="buildmaster"
HNAME="chn-vm-devsign.mchp-main.com"

BASEDIR=$(dirname "$0")

DIR=$1
PROJECT=$2

SIGN_SCRIPT="${BASEDIR}/remote_xc_codesign_win.bat"

if [ ! -d $DIR ]; then
  echo "Please specify install directory"
  exit 1
fi

if [ -z "${PROJECT}" ] ; then
  echo "Please specify target project. Eg: xc32, xc16"
  exit 1
fi

echo "Finding executables"

FILES=$(find "$DIR" -print)
TGT_DIR="build_$(date +%Y%m%d)"

REMOTE_DIR=${PROJECT}/${TGT_DIR}
REMOTE_ADDR=${UNAME}@${HNAME}

ssh "${REMOTE_ADDR}" mkdir -p "${REMOTE_DIR}"
scp ${SIGN_SCRIPT} "${REMOTE_ADDR}":"${REMOTE_DIR}"/"$(basename ${SIGN_SCRIPT})"

for f in $FILES; do
  if ( file $f | grep -q PE32 ); then
    file=$(basename "$f")
    if [[ "${file}" == *xclm* || "${file}" == *verifyinst* ]]; then
        echo "Skiping: ${file}"
        continue
    fi
    echo "copying $f to windows"
    scp "$f" "${REMOTE_ADDR}":"${REMOTE_DIR}"
    echo "signing $f to windows"
    ssh "${REMOTE_ADDR}" "${REMOTE_DIR}"/"$(basename ${SIGN_SCRIPT})" "${REMOTE_DIR}"/"${file}"
    echo "copying back $f from windows"
    scp "${REMOTE_ADDR}":"${REMOTE_DIR}"/"${file}" "${f}"
    if [ "$?" != "0" ]; then
      echo "File $f failed to copy from windows"
      exit 1
    fi
  fi
done

# Remove the remote directory on the Windows machine used for signing
# The files are already copied back so it can be deleted at this point.
ssh "${REMOTE_ADDR}" rm -r "${REMOTE_DIR}"
