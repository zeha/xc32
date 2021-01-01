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

ROOT=`pwd`
LOGTASKFILE=$ROOT/log-copy-collateral.txt
LOGFILE=$LOGTASKFILE

script_path=`cd $(dirname $0) && pwd -P`
. $script_path/pic32m/build-pic32m-common.sh

### 1 name of the directory
erase_and_make_dir() {
  if [ -e $1 ]; then
    xc32_eval "rm -rf $1"
  fi
  mkdir $1
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

# Create source package excluding source control information
#   parameter 1: base dir of the source tree
#   parameter 2: dirname of the source tree
#   parameter 3: target package name
#   parameter 4-10: additional excluding
# This function will create bz2 package for files under param1/param2, 
# excluding unnecessary parts, and create package named param2.
pack_dir_clean() {
    set +u
    tar cjfh $3 \
	--exclude=CVS --exclude=.svn --exclude=.git --exclude=.pc \
	--exclude="*~" --exclude=".#*" \
	--exclude=".fslckout" --exclude="*.fsl" --exclude="*.fossil" \
	--exclude="*.orig" --exclude="*.rej" $4 $5 $6 $7 $8 $9 ${10} \
	-C $1 $2
    
}

if [ -z "${HOST_MINGW:-}" ]; then
  HOST_MINGW=i686-w64-mingw32
  HOST_MINGW_TOOL=i686-w64-mingw32
fi

if [ -z "${MINGW_HOST_PREFIX:-}" ]; then
  MINGW_HOST_PREFIX=i686-w64-mingw32
fi

if [[ ! -e ./pic32m-source ]] ; then
  assert_success 1 "ERROR: ./pic32m-source"
fi

if [[ ! -e ./pic32c-source ]] ; then
  assert_success 1 "ERROR: ./pic32c-source"
fi

if [[ ! -e ./common-source ]] ; then
  assert_success 1 "ERROR: ./common-source"
fi

##############################################
# Create source package
##############################################

erase_and_make_dir "$ROOT/packaged-source"
pushd "$ROOT/packaged-source"

pack_dir_clean $ROOT pic32m-source/pic32-newlib pic32m-newlib.tar.bz2
pack_dir_clean $ROOT pic32m-source/src48x pic32m-gcc-binutils.tar.bz2

if [ ! -z $bamboo_XC32_FULL_VERSION ]; then
  ZIPNAME=xc32-v${bamboo_XC32_FULL_VERSION}-src.zip
else
  ZIPNAME=xc32-vX.YY-src.zip
fi

xc32_eval "zip -r $ZIPNAME ./*.tar.bz2"
rm pic32m-newlib.tar.bz2
rm pic32m-gcc-binutils.tar.bz2
pack_dir_clean $ROOT pic32c-source/XC32-arm-gcc pic32c-gcc-binutils.tar.bz2 "--exclude=newlib"
pack_dir_clean $ROOT pic32c-source/XC32-arm-gcc/newlib pic32c-newlib.tar.bz2
pack_dir_clean $ROOT common-source/build-scripts xc32-build-scripts.tar.bz2
xc32_eval "zip -r xc32-vX.YY-src.zip ./*.tar.bz2"
rm *.tar.bz2

status_update "Completed $0"
