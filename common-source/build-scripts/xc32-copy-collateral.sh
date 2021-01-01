#! /usr/bin/env bash

#
#  Copyright 2017 Microchip Technology Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

#set -e
#set -x
#set -u
set -o pipefail

umask 022

exec < /dev/null

LOGTASKFILE=log-copy-collateral.txt
ROOT=`pwd`

script_path=`cd $(dirname $0) && pwd -P`
. $script_path/pic32m/build-pic32m-common.sh

### 1 name of the directory
erase_and_make_dir() {
  if [ -e $1 ]; then
    xc32_eval "rm -rf $1"
  fi
  mkdir -p $1
}

### $1 Command
### $2 Error Message
xc32_eval() {
  set +u
  echo ""
  echo "directory executed: `pwd`"
  echo "command executed: $1"
  echo "directory executed: `pwd`" >> $LOGTASKFILE
  echo "command executed: $1" >> $LOGTASKFILE
  bash -c "$1" >>$LOGTASKFILE 2>&1
  assert_success $? "$2"
}

if [ -z "${HOST_MINGW:-}" ]; then
  HOST_MINGW=i686-w64-mingw32
  HOST_MINGW_TOOL=i686-w64-mingw32
fi

if [ -z "${MINGW_HOST_PREFIX:-}" ]; then
  MINGW_HOST_PREFIX=i686-w64-mingw32
fi

if [[ ! -e $ROOT/common-source/xclm_release ]] ; then
  assert_success 1 "ERROR: missing common-source/xclm_release"
fi

##############################################
# Copy collateral into collateral-image
##############################################

erase_and_make_dir "$ROOT/collateral-image"
pushd "$ROOT/collateral-image"

erase_and_make_dir "$ROOT/collateral-image/examples"
copy_dir_clean "$ROOT/common-source/examples" "./examples"
pushd examples
#
# Remove all the scripts (sub)directories from the HPMD examples
#
for x in `find . -name scripts -print`
do
  rm -rf $x
done
popd

erase_and_make_dir "$ROOT/collateral-image/docs"
copy_dir_clean "$ROOT/common-source/docs" "./docs"
pushd ./docs
find . -name "config.mif" -exec rm -rf \{\} \;
popd

########
# Add Coverity static-analysis files to images
# This path is mandated by coverity - not up to us.
status_update "Add Coverity static-analysis files to images"
COVERITY_DIR="./etc/coverity/mchip/xc32"
erase_and_make_dir "$ROOT/collateral-image/$COVERITY_DIR"
copy_dir_clean "$ROOT/common-source/Coverity/mchip_xc32" "$COVERITY_DIR"

#
# pic32-libs containing pic32c-libs.zip & pic32m-libs.zip
#
erase_and_make_dir "$ROOT/collateral-image/pic32-libs"
pushd ./pic32-libs
copy_dir_clean "$ROOT/pic32m-source/pic32-libs" "./pic32m-libs"
copy_dir_clean "$ROOT/pic32c-source/pic32c-libs" "./pic32c-libs"
find . -name "OBJ" -exec rm -rf \{\} \;
assert_success $? "Remove OBJ object files from pic32-libs source directory"
find . -name "*.o" -delete
assert_success $? "Remove *.o object files from pic32-libs source directory"
find . -name "*.a" -delete
assert_success $? "Remove *.a archive files from pic32-libs source directory"
# Need to remove some files from the pic32m-libs image
pushd ./pic32m-libs
#
# save the startup code for installer C32-145
#
mkdir save
mkdir save/dsp
assert_success $? " mkdir save/dsp failed."
mkdir save/dsp/wrapper
assert_success $? " mkdir save/dsp/wrapper failed."
cp -R dsp/wrapper save/dsp

for x in `ls`
do
  if [[ -d $x ]]; then
  # remove all directories except
   if [[ $x != "peripheral" && $x != "include" && $x != "libpic32" && $x != "save" && $x != "debugsupport" && $x != "proc" && $x != "cppcfl" ]]; then
    echo `date` " Removing MTK directory $x..." | tee -a $LOGFILE
    rm -rf $x
   fi
  elif [[ $x != Makefile && $x != defines.mk && ${x%txt} == $x ]]; then
    # every regular file except above list
    rm -vf $x
  fi
done
#
# save the startup code for installer C32-145
#
pwd

mv save/dsp dsp
assert_success $? " mv save/dsp dsp failed."
rm -rf save
assert_success $? " rm -rf save failed."
popd
zip -r pic32m-libs.zip ./pic32m-libs
assert_success $? "zip pic32m-libs.zip"
zip -r pic32c-libs.zip ./pic32c-libs
assert_success $? "zip pic32c-libs.zip"
rm -rf ./pic32m-libs
rm -rf ./pic32c-libs

popd
cd "$ROOT"
##############################################
# Copy collateral into individual image dirs
##############################################

if [ -e install-Darwin ]; then
  status_update "Copy collateral into install-Darwin"
  xc32_eval "rsync -qavzCr --include=* \"$ROOT/collateral-image/\" \"$ROOT/install-Darwin\""
fi

if [ -e install-Darwin-nolm ]; then
  status_update "Copy collateral into install-Darwin-nolm"
  xc32_eval "rsync -qavzCr --include=* \"$ROOT/collateral-image/\" \"$ROOT/install-Darwin-nolm\""
fi

if [ -e install-Linux ]; then
  status_update "Copy collateral into install-Linux"
  xc32_eval "rsync -qavzCr --include=* \"$ROOT/collateral-image/\" \"$ROOT/install-Linux\""
fi

if [ -e install-Linux-nolm ]; then
  status_update "Copy collateral into install-Linux-nolm"
  xc32_eval "rsync -qavzCr --include=* \"$ROOT/collateral-image/\" \"$ROOT/install-Linux-nolm\""
fi

if [ -e install-mingw ]; then
  status_update "Copy collateral into install-mingw"
  xc32_eval "rsync -qavzCr --include=* \"$ROOT/collateral-image/\" \"$ROOT/install-mingw\""
fi

status_update "Completed $0"
