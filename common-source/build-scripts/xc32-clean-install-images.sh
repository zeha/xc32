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
LOGTASKFILE="$WORKING_DIR/log-clean-install-images.txt"

script_path=`cd $(dirname $0) && pwd -P`
. $script_path/pic32m/build-pic32m-common.sh

if [ -z "${HOST_MINGW:-}" ]; then
  HOST_MINGW=i686-w64-mingw32
  HOST_MINGW_TOOL=i686-w64-mingw32
fi

if [ -z "${MINGW_HOST_PREFIX:-}" ]; then
  MINGW_HOST_PREFIX=i686-w64-mingw32
fi

if [[ ! -e $ROOT/common-source ]] ; then
  assert_success 1 "ERROR: missing common-source"
fi

##############################################
# Clean the install images
#
# in what follows, the mysterious Users/data*/build directories are
# strange artifacts from trying to copy/install man pages. 
# the exact path will unfortunately depend on the root directory
# of the build directory..
##############################################

if [ -e install-Darwin ]; then
  status_update "Clean install image install-Darwin"
  INSTALL_IMAGE=$ROOT/install-Darwin
  rm -rf $INSTALL_IMAGE/include
  rm -rf $INSTALL_IMAGE/man
  rm -rf $INSTALL_IMAGE/info
  rm -rf $INSTALL_IMAGE/share
  rm -rf $INSTALL_IMAGE/libsrc
  rm -rf $INSTALL_IMAGE/pic32mx/include/c++
  rm -rf $INSTALL_IMAGE/pic32mx/share
  rm -rf $INSTALL_IMAGE/Users
  rm -rf $INSTALL_IMAGE/build
  rm -rf $INSTALL_IMAGE/data*
  find $INSTALL_IMAGE -name "*.py" -delete
  find $INSTALL_IMAGE -name "*\**" -delete
fi

if [ -e install-Darwin-nolm ]; then
  status_update "Clean install image install-Darwin-nolm"
fi

if [ -e install-Linux ]; then
  status_update "Clean install image install-Linux"
  INSTALL_IMAGE=$ROOT/install-Linux
  rm -rf $INSTALL_IMAGE/include
  rm -rf $INSTALL_IMAGE/man
  rm -rf $INSTALL_IMAGE/info
  rm -rf $INSTALL_IMAGE/share
  rm -rf $INSTALL_IMAGE/libsrc
  rm -rf $INSTALL_IMAGE/pic32mx/include/c++
  rm -rf $INSTALL_IMAGE/pic32mx/share
  rm -rf $INSTALL_IMAGE/Users
  rm -rf $INSTALL_IMAGE/build
  rm -rf $INSTALL_IMAGE/data*
  find $INSTALL_IMAGE -name "*.py" -delete
  find $INSTALL_IMAGE -name "*\**" -delete
fi

if [ -e install-Linux-nolm ]; then
  status_update "Clean install image install-Linux-nolm"
fi

if [ -e install-mingw ]; then
  status_update "Clean install image install-mingw"
  INSTALLDIR_MINGW=$ROOT/install-mingw
  rm -rf $INSTALLDIR_MINGW/include
  rm -rf $INSTALLDIR_MINGW/man
  rm -rf $INSTALLDIR_MINGW/info
  rm -rf $INSTALLDIR_MINGW/share
  rm -rf $INSTALLDIR_MINGW/libsrc
  rm -rf $INSTALLDIR_MINGW/pic32mx/include/c++
  rm -rf $INSTALLDIR_MINGW/pic32mx/share
  rm -rf $INSTALLDIR_MINGW/Users
  rm -rf $INSTALLDIR_MINGW/build
  rm -rf $INSTALLDIR_MINGW/data*
  find $INSTALLDIR_MINGW -name "*.py" -delete
  find $INSTALLDIR_MINGW -name "*\**" -delete
fi

if [ -e export-image ]; then
  status_update "Clean install image install-Linux"
  INSTALL_IMAGE=$ROOT/export-image
  rm -rf $INSTALL_IMAGE/include
  rm -rf $INSTALL_IMAGE/man
  rm -rf $INSTALL_IMAGE/info
  rm -rf $INSTALL_IMAGE/share
  rm -rf $INSTALL_IMAGE/libsrc
  rm -rf $INSTALL_IMAGE/pic32mx/include/c++
  rm -rf $INSTALL_IMAGE/pic32mx/share
  rm -rf $INSTALL_IMAGE/Users
  rm -rf $INSTALL_IMAGE/build
  rm -rf $INSTALL_IMAGE/data*
  find $INSTALL_IMAGE -name "*.py" -delete
  find $INSTALL_IMAGE -name "*\**" -delete
fi

status_update "Completed $0"
