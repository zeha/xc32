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

script_path=`cd $(dirname $0) && pwd -P`
. $script_path/pic32m/build-pic32m-common.sh

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
# Extract xclm_release
##############################################

if [ -e install-Darwin ]; then
  status_update "Extracting xclm for install-Darwin"
  cd $ROOT/install-Darwin
  if [ -e ./bin/xclm ] ; then
    rm ./bin/xclm
  fi
  if [ -e ./xclm ] ; then
    rm -rf ./xclm
  fi
  if [ ! -e ./bin ] ; then
    mkdir -p bin
  fi
  mkdir ./xclm
  unzip -o $ROOT/common-source/xclm_release/dist/mac.zip  -d ./xclm/
  assert_success $? "ERROR: Failed to unzip xclm into install-Darwin/bin"
  #find . -name "xclm" -type f -perm -g+x -follow | xargs file | grep Mach-O | cut -d: -f1 | xargs strip -S
  cp ./xclm/install/client/bin/xclm ./bin/xclm
  cd $ROOT
fi

if [ -e install-Linux ]; then
  status_update "Extracting xclm for install-Linux"
  cd $ROOT/install-Linux
  if [ -e ./bin/xclm ] ; then
    rm ./bin/xclm
  fi
  if [ -e ./xclm ] ; then
    rm -rf ./xclm
  fi
  if [ ! -e ./bin ] ; then
    mkdir -p bin
  fi
  mkdir ./xclm
  unzip -o $ROOT/common-source/xclm_release/dist/linux.zip -d ./xclm/
  assert_success $? "ERROR: Failed to unzip xclm into install-Linux/bin"
  if [ -e ./bin/xclm64 ] ; then
    rm ./bin/xclm64
  fi
  if [ -e ./xclm64 ] ; then
    rm -rf ./xclm64
  fi
  mkdir ./xclm64
  unzip -o $ROOT/common-source/xclm_release/dist/linux64.zip -d ./xclm64/
  assert_success $? "ERROR: Failed to unzip xclm64 into install-Linux/bin"
  if [ "x$uname_string" == "xlinux" ] ; then
    find . -name "xclm" -type f -perm -g+x -follow | xargs file | grep ELF | cut -d: -f1 | xargs strip --strip-unneeded
  fi
  cp ./xclm/install/client/bin/xclm ./bin/xclm
  cd $ROOT
fi
if [ -e install-mingw ]; then
  status_update "Extracting xclm for install-mingw"
  cd $ROOT/install-mingw
  if [ -e ./bin/xclm.exe ] ; then
    rm ./bin/xclm.exe
  fi
  if [ -e ./xclm ] ; then
    rm -rf ./xclm
  fi
  if [ ! -e ./bin ] ; then
    mkdir -p bin
  fi
  mkdir ./xclm
  unzip -o $ROOT/common-source/xclm_release/dist/windows.zip -d ./xclm/
  assert_success $? "ERROR: Failed to unzip xclm into install-mingw/bin"
  find . -type f -name "xclm.exe" | xargs $HOST_MINGW_TOOL-strip --strip-unneeded
  cp ./xclm/install/client/bin/xclm.exe ./bin/xclm.exe
  cd $ROOT
fi

if [ "x$uname_string" == "xdarwin" ] ; then
  if [[ ! -e $ROOT/$NATIVEIMAGE/bin/xclm ]] ; then
    assert_success 1 "ERROR: Missing $ROOT/$NATIVEIMAGE/bin/xclm"
  fi
elif [ "x$uname_string" == "xlinux" ] ; then
  if [[ ! -e $ROOT/$NATIVEIMAGE/bin/xclm ]] ; then
    assert_success 1 "ERROR: Missing $ROOT/$NATIVEIMAGE/bin/xclm"
  fi
fi

status_update "Completed xc32-extract-xclm.sh"
