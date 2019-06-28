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
#set -o pipefail

#umask 022

#exec < /dev/null

WORKING_DIR=`pwd`
LOGFILE=$WORKING_DIR/log-build-pic32c-specs.txt

script_path=`cd $(dirname $0) && pwd -P`
. $script_path/build-pic32c-common.sh

if [ -z "${HOST_MINGW:-}" ]; then
  HOST_MINGW=i686-w64-mingw32
  HOST_MINGW_TOOL=i686-w64-mingw32
fi

if [ -z "${MINGW_HOST_PREFIX:-}" ]; then
  MINGW_HOST_PREFIX=i686-w64-mingw32
fi

if [[ ! -e $ROOT/pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c ]] ; then
  assert_success 1 "ERROR: missing pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c"
fi

### $1 Command
### $2 Error Message
xc32_eval() {
  set +u
  echo ""
  echo "directory executed: `pwd`"
  echo "command executed: $1"
  echo "" >> $LOGFILE
  echo "directory executed: `pwd`" >> $LOGFILE
  echo "command executed: $1" >> $LOGFILE
  bash -c "$1"
  assert_success $? "$2"
  
}

##############################################
# Build spec files
##############################################
status_update "BEGINNING $0"

pushd $ROOT/pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen
make s-device-specs
assert_success $? "ERROR: failed make s-device-specs"

if [ ! -e device-specs ] ; then
  assert_success -1 "ERROR: failed to generate device-specs"
fi
popd

##############################################
# Copy spec files
##############################################

if [ -e install-Darwin ]; then
  status_update "Copying specs to install-Darwin"
  mkdir -p install-Darwin/bin/device_files/device-specs
  xc32_eval "rsync -qav  \"$ROOT/pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen/device-specs/\" \"install-Darwin/bin/device_files/device-specs\""
fi

if [ -e install-Darwin-nolm ]; then
  status_update "Copying specs to install-Darwin-nolm"
  mkdir -p install-Darwin-nolm/bin/device_files/device-specs
  xc32_eval "rsync -qav  \"$ROOT/pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen/device-specs/\" \"install-Darwin-nolm/bin/device_files/device-specs\""
fi

if [ -e install-Linux ]; then
  status_update "Copying specs to install-Linux"
  mkdir -p install-Linux/bin/device_files/device-specs
  xc32_eval "rsync -qav  \"$ROOT/pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen/device-specs/\" \"install-Linux/bin/device_files/device-specs\""
fi

if [ -e install-Linux-nolm ]; then
  status_update "Copying specs to install-Linux-nolm"
  mkdir -p install-Linux-nolm/bin/device_files/device-specs
  xc32_eval "rsync -qav  \"$ROOT/pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen/device-specs/\" \"install-Linux-nolm/bin/device_files/device-specs\""
fi

if [ -e install-mingw ]; then
  status_update "Copying specs to install-mingw"
  mkdir -p install-mingw/bin/device_files/device-specs
  xc32_eval "rsync -qav  \"$ROOT/pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen/device-specs/\" \"install-mingw/bin/device_files/device-specs\""
fi

status_update "Completed $0"
