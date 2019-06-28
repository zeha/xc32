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
set -o pipefail 

umask 022

exec < /dev/null

OLDPATH=$PATH
LOGFILE=`pwd`/xc32-build-all.log
BUILDSUBDIR=`pwd`/builddir

export MCHP_VERSION="2.05"
export MCHP_RESOURCE="A"

if [ $# -gt 3 ] ; then
    show_usage
    exit 1
fi

show_usage()
{
cat<<EOF
Usage: $0 [--skip_steps=...]

This script will build the xc32 toolchain.

OPTIONS:
  --skip_steps=STEPS    specify which build steps you want to skip.  Concatenate
                        them with comma for skipping more than one steps.  Available
                        steps are: checkout, pic32c, pic32m & source_tar.

  --branch=BRANCH       specify the fossil branch to checkout

  --tag=TAG             specify the fossil tag to checkout
  --jobs=N              specify the -j argument for make commands
  --debug               enable debug build
  --devel               enable development build
EOF
}

error () {
    set +u
    echo "$0: error: $@" >&2
    error_handler
    exit 1
    set -u
}

warning () {
    set +u
    echo "$0: warning: $@" >&2
    set -u
}

function status_update ()
{
 set +u
 local MESSAGE=$1
  if [ ! -z "${NOTIFY_SEND:-}" ] ; then
   $NOTIFY_SEND "$MESSAGE" "xc32++-build.sh"
  elif [ ! -z "${GROWL_SEND:-}" ] ; then
    $GROWL_SEND -sm "$MESSAGE"
  fi
  echo "status: $MESSAGE"
  #echo `date` "status: $MESSAGE" >> $LOGFILE
  set -u
}

####
# assert_success()
#  If the first parameter is non-zero, print the second parameter
#  and exit the script
####
function assert_success ()
{
 set +u
 local RESULT=$1
 local MESSAGE=$2
 if [ $RESULT != 0 ]
 then
  echo "$MESSAGE ($RESULT)"
  if [ ! -z "${NOTIFY_SEND:-}" ] ; then
   $NOTIFY_SEND "$MESSAGE" "XC32 Build Error"
  elif [ ! -z "${GROWL_SEND:-}" ] ; then
    $GROWL_SEND -sm "XC32 Build Error: $MESSAGE"
  fi
  echo "$MESSAGE ($RESULT)" >> $LOGFILE
  error_handler
 fi
}

check_pipe() {
  local -a status=("${PIPSTATUS[@]}")
  local limit=$1
  local ix

  if [ -z "$limit" ] ; then
      limit="${#status[@]}"
  fi
  for ((ix=0; ix != $limit ; ix++)); do
    if [ "${status[$ix]}" != "0" ] ; then
      error_handler
    fi
  done
}



error_handler ()
{
  unset GCC_FOR_TARGET
  unset XGCC_FOR_TARGET
  unset CC_FOR_TARGET
  unset CXX_FOR_TARGET
  unset GXX_FOR_TARGET
  unset CPP_FOR_TARGET
  unset CC_FOR_BUILD
  unset CXX_FOR_BUILD
  unset CC
  unset CPP
  unset CXX
  unset LD
  unset AR
  unset LDFLAGS
  unset ABI
  PATH=$OLDPATH
  export MINGW_HOST_PREFIX=$SAVE_MINGW_HOST_PREFIX
  export MINGW32_HOST_PREFIX=$SAVE_MINGW32_HOST_PREFIX
  export HOST_MINGW=$SAVE_HOST_MINGW
  export HOST_MINGW_TOOL=$SAVE_HOST_MINGW_TOOL
  exit 1
}

### $1 Command
### $2 Error Message
xc32_eval() {
  echo ""
  echo "directory executed: `pwd`"
  echo "command executed: $1"
  echo "" >> $LOGFILE
  echo "directory executed: `pwd`" >> $LOGFILE
  echo "command executed: $1" >> $LOGFILE
  bash -c "$1"
  assert_success $? "$2"
}

xc32_checkout_script() {
  set +u
  echo ""
  echo "directory executed: `pwd`"
  echo "command executed: $1 $2"
  echo "" >> $LOGFILE
  echo "directory executed: `pwd`" >> $LOGFILE
  echo "command executed: $1 $2" >> $LOGFILE
  bash -c "$1 $2"
  assert_success $? "ERROR: $1 $2"
  set -u
}

### $1 name of the directory
### $2 any extra arguments for the make
build_xc32_sh() {

  if [[ ! -e $1 ]] ; then
   mkdir -p $1
  fi

  if [[ ! -e $1/bin ]] ; then
   mkdir -p $1/bin
  fi
  
  XC32_SH_BUILD_VARIABLES=
#  if [ "x$DYLD_LIBRARY_PATH" != "x" ] ; then
#    XC32_SH_BUILD_VARIABLES="DYLD_LIBRARY_PATH=\$DYLD_LIBRARY_PATH"
#  fi

  pushd $1
  XC32_SH_SRC=${BUILDSUBDIR}/common-source/xc_shell
  cd $XC32_SH_SRC/bin

  #clean
  #necessary since there isn't a separate build directory
  COMMAND="make -f Makefile-xc32 -j$JOBS clean"
  xc32_eval "$COMMAND" "xc32-sh make clean failed"

  #make
  COMMAND="make -f Makefile-xc32 -j$JOBS xc32 $2 $XC32_SH_BUILD_VARIABLES"
  xc32_eval "$COMMAND" "xc32-sh make failed"

  #install
  #No need to add the /bin for call below
  echo export XC32_INSTALL=$1
  export XC32_INSTALL=$1
  COMMAND="make -f Makefile-xc32 -j$JOBS xc32-install $2  $XC32_SH_BUILD_VARIABLES"
  xc32_eval "$COMMAND" "xc32-sh make install failed"
  unset XC32_INSTALL
  popd
}

trap 'assert_success -1 "ERROR: Caught signal"' SIGHUP SIGINT SIGTERM

script_path=`cd $(dirname $0) && pwd -P`

CHECKOUT="yes"
SAVE_MINGW_HOST_PREFIX=$MINGW_HOST_PREFIX
SAVE_MINGW32_HOST_PREFIX=$MINGW32_HOST_PREFIX
SAVE_HOST_MINGW=$HOST_MINGW
SAVE_HOST_MINGW_TOOL=$HOST_MINGW_TOOL

TAG="x"
BRANCH="x"
JOBS=2
DEVEL=
DEBUG=
FULL_ONLY=no
skip_pic32m=no
skip_pic32c=no
skip_checkout=no
skip_source_tar=no
skip_steps=
jobs_set=no

if [[ "x$XC32_JOBS" != "x" ]]; then
  JOBS=$XC32_JOBS
  jobs_set=yes
fi

for ac_arg; do
    case $ac_arg in
      --skip_steps=*)
          skip_steps=`echo $ac_arg | sed -e "s/--skip_steps=//g" -e "s/,/ /g"`
          ;;
      --skip-steps=*)
          skip_steps=`echo $ac_arg | sed -e "s/--skip_steps=//g" -e "s/,/ /g"`
          ;;
      --branch=*)
          if [ "x$TAG" != "xx" ] ; then
            echo "ERROR: Do not specify both --tag and --branch\n"
            exit 1
          fi
          BRANCH=`echo $ac_arg | sed -e "s/--branch=//g"`
          ;;
      --tag=*)
          if [ "x$BRANCH" != "xx" ] ; then
            echo "ERROR: Do not specify both --tag and --branch\n"
            exit 1
          fi
          TAG=`echo $ac_arg | sed -e "s/--tag=//g"`
          ;;
      --jobs=*)
          JOBS=`echo $ac_arg | sed -e "s/--jobs=//g"`
          jobs_set=yes
          ;;
      --debug|-dbg)
          DEBUG=",debug" # flag for pic32c-toolchain script
          ;;
      --devel|-dev)
          DEVEL=",devel" # flag for pic32c-toolchain script
          ;;
      *)
        show_usage
        exit 1
        ;;
    esac
done


if [ "x$skip_steps" != "x" ]; then
    for ss in $skip_steps; do
        case $ss in
            checkout)
                      status_update "skip_checkout=yes"
                      skip_checkout=yes
                      ;;
            pic32c)
                      status_update "skip_pic32c=yes"
                      skip_pic32c=yes
                      ;;
            pic32m)
                      status_update "skip_pic32m=yes"
                      skip_pic32m=yes
                      ;;
            source_tar)
                      status_update "skip_source_tar=yes"
                      skip_source_tar=yes
                      ;;
            *)
                      echo "Unknown build steps: $ss" 1>&2
                      show_usage
                      exit 1
                      ;;
        esac
    done
fi


echo "Beginning xc32-build-all.sh" > $LOGFILE

if [[ -e $BUILDSUBDIR ]] ; then
 echo "WARNING: $BUILDSUBDIR already exists, using existing directory"
 sleep 5
else
  mkdir -p $BUILDSUBDIR
fi

pushd $BUILDSUBDIR

if [ "x$skip_checkout" == "xno" ] ; then
status_update "Checking out code"
  # Checkout all of the source files from the repositories
  if [ "x$BRANCH" != "xx" ] ; then
    CHECKOUT_COMMIT="${BRANCH}"
    if [[ "${BRANCH}" == trunk ]]; then
      CHECKOUT_COMMIT="master";
    fi
    xc32_checkout_script "$script_path/checkout-common-source.sh" "-b $BRANCH -c ${CHECKOUT_COMMIT}"
    xc32_checkout_script "$script_path/pic32m/checkout-pic32m-source.sh" "-b $BRANCH -c ${CHECKOUT_COMMIT}"
    xc32_checkout_script "$script_path/pic32c/checkout-pic32c-source.sh" "-b $BRANCH -c ${CHECKOUT_COMMIT}"
  elif [ "x$TAG" != "xx" ] ; then
    xc32_checkout_script "$script_path/checkout-common-source.sh" "-t $TAG -c ${TAG}"
    xc32_checkout_script "$script_path/pic32m/checkout-pic32m-source.sh" "-t $TAG -c ${TAG}"
    xc32_checkout_script "$script_path/pic32c/checkout-pic32c-source.sh" "-t $TAG -c ${TAG}"
  else
    xc32_checkout_script "$script_path/checkout-common-source.sh"
    xc32_checkout_script "$script_path/pic32m/checkout-pic32m-source.sh"
    xc32_checkout_script "$script_path/pic32c/checkout-pic32c-source.sh"
  fi

  # Make a copy of the build scripts in use in the common-source/build-scripts directory
  if [ -e common-source ] ; then
    pushd common-source
    rm -rf build-scripts
    mkdir -p build-scripts
    cp $script_path/xc32-build-all.sh ./build-scripts/
    pushd build-scripts
    rm -rf pic32c
    mkdir -p pic32c
    cp $script_path/pic32c/*.sh ./pic32c/
    rm -rf pic32m
    mkdir -p pic32m
    cp $script_path/pic32m/*.sh ./pic32m/
    popd   # build-scripts
    popd   # common-source
  fi
fi

JOBS_ARG=""
PIC32M_JOBS_ARG=""
if [ "x$jobs_set" == "xyes" ] ; then
  JOBS_ARG="--jobs=$JOBS"
  PIC32M_JOBS_ARG="-J$JOBS"
fi

if [ "x$skip_pic32m" == "xno" ] ; then
  xc32_eval "$script_path/pic32m/build-pic32m-prerequisites.sh $JOBS_ARG" "Build pic32m prerequisites"
fi
if [ "x$skip_pic32c" == "xno" ] ; then
  xc32_eval "$script_path/pic32c/build-pic32c-prerequisites.sh $JOBS_ARG" "Build pic32c prerequisites"
fi

if [[ $(uname -s) == "Darwin" ]]
then

#  if [[ $(hostname -s) == "CHN-SV-TYPHOON2" ]] ; then
#    OLDCC=$CC
#    export CC=/opt/local/bin/gcc-apple-4.2
#  fi

  build_xc32_sh "$BUILDSUBDIR/install-Darwin"
  build_xc32_sh "$BUILDSUBDIR/install-Darwin-nolm"
  
#  if [[ $(hostname -s) == "CHN-SV-TYPHOON2" ]] ; then
#    CC=$OLDCC
#  fi

else
  build_xc32_sh "$BUILDSUBDIR/install-Linux" "TARGET=linux EXTRAOPS='-m32 -march=i386'"
  build_xc32_sh "$BUILDSUBDIR/install-Linux-nolm" "TARGET=linux EXTRAOPS='-m32 -march=i386'"
  build_xc32_sh "$BUILDSUBDIR/install-mingw" "TARGET=mingw"
fi

# Must come AFTER building the prerequisites
if [ -e "$script_path/xc32-extract-xclm.sh" ] ; then
  xc32_eval "$script_path/xc32-extract-xclm.sh"
fi


if [ "x$skip_pic32m" == "xno" ] ; then
  xc32_eval "$script_path/pic32m/build-pic32m-toolchain.sh $PIC32M_JOBS_ARG" "Build pic32m toolchain"
fi


if [ "x$skip_pic32c" == "xno" ] ; then
echo "command executed: $script_path/pic32c/build-pic32c-toolchain.sh --skip_steps=gdb-with-python,mingw32-gdb-with-python --build_type=native${DEVEL}${DEBUG} $JOBS_ARG" >> $LOGFILE
  bash $script_path/pic32c/build-pic32c-toolchain.sh --skip_steps=gdb-with-python,mingw32-gdb-with-python --build_type=native${DEVEL}${DEBUG} $JOBS_ARG
  assert_success $? "Build pic32c toolchain: pic32c/build-pic32c-toolchain.sh"
  xc32_eval "$script_path/pic32c/build-pic32c-specs.sh" "Build and install pic32c spec files"
fi


# Add pic32c devices to deviceSupport.xml and .LanguageToolSuite
if [ -e $BUILDSUBDIR/pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen/include/pic32c-mcus.def ] ; then
  pushd $BUILDSUBDIR
  find . -name "deviceSupport.xml" -exec python2.7 $script_path/pic32c/patch_deviceSupport.py $BUILDSUBDIR/pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen/include/pic32c-mcus.def \{\} \;
  find . -name ".LanguageToolSuite" -exec python2.7 $script_path/pic32c/patch_LanguageToolSuite.py $BUILDSUBDIR/pic32c-source/pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen/include/pic32c-mcus.def \{\} \;
  popd
fi


if [ -e "$script_path/xc32-copy-collateral.sh" ] ; then
  xc32_eval "$script_path/xc32-copy-collateral.sh"
fi

if [ -e "$script_path/xc32-clean-install-images.sh" ] ; then
  xc32_eval "$script_path/xc32-clean-install-images.sh"
fi

if [ "x$skip_source_tar" == "xno" ] ; then
  if [ -e "$script_path/xc32-create-source-dist.sh" ] ; then
    xc32_eval "$script_path/xc32-create-source-dist.sh"
  fi
fi


status_update "DONE: Completed xc32-build-all.sh"

