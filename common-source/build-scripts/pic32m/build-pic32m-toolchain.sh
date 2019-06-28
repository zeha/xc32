#!/bin/bash

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
#
set -o pipefail

script_path=`cd $(dirname $0) && pwd -P`
. $script_path/build-pic32m-common.sh

# Move checkout to a separate script
CHECKOUT="no"

WINDOWS="yes"
LINUX=""
OSXONLY=""

# skip the mingw build when we are building on Mac OS X
if [ "x$uname_string" == "xdarwin" ] ; then
  WINDOWS=""
fi

SKIPLIBS=""
SKIPXCLIB=""
SKIPXCPPLIBS=""
SKIPNATIVE_NOLM=""
SKIPNATIVE=""

# Linux build on Mac
SKIPLINUXBUILD="yes"

SKIPCREATETAR="yes"
SKIPLEGACYPLIB="yes"

SKIPREBUILD=""
SKIPRESOURCEFILE=""
SKIPPPL="yes"
SKIPCLOOG="yes"

CVSB="trunk"

SUPPORT_HOSTED_LIBSTDCXX="--disable-hosted-libstdcxx"
SUPPORT_SJLJ_EXCEPTIONS="--enable-sjlj-exceptions"
SUPPORT_CHECKING="--disable-checking"
SUPPORT_GOFAST="--enable-gofast"

export CXX_FLAGS_FOR_TARGET="-frtti -fexceptions -fomit-frame-pointer -DPREFER_SIZE_OVER_SPEED -D__OPTIMIZE_SIZE__ -fshort-wchar -fno-unroll-loops -fenforce-eh-specs -ffunction-sections -fdata-sections -DSMALL_MEMORY -D_NO_GETLOGIN -D_NO_GETPWENT -D_NO_GETUT -D_NO_GETPASS -D_NO_SIGSET"

echo "START BUILD..."

SHASUM256=""
XCLM_SHASUM=""
XCLM64_SHASUM=""
XCLM_SHASUM_MACRO=""

CFLAGS="-O2"
CXXFLAGS="-O2"

WORKING_DIR=`pwd`
echo WORKING DIR $WORKING_DIR
SOURCE_DIR=${WORKING_DIR}/pic32m-source
SOURCE_GCC_DIR=$SOURCE_DIR/src48x

LOGFILE=$WORKING_DIR/build-pic32m.log
LOGTASKFILE=$LOGFILE

#PATH=/usr/local/i386-mingw32-4.3.0/bin:$PATH
#PATH=/usr/local/gcc-4.5.2-for-linux32/bin:$PATH

show_usage()
{
 # Shows the usage screen
 echo "USAGE: $0 [-t <tag> | -?]"
 echo " -b <branch> Specify the branch for which you would like to build"
 echo " -t <tag>    Specify the tag for which you would like to build"
 echo " -N          No fossil checkout build only"
 echo " -Q          Show fossil checkout (no quiet)"
 echo " -M          Build only OS X binaries only image"
 echo " -L          Skip MinGW32 build"
 echo " -J <N>      Specify the job count for make"
 echo " -?          Show this usage message"
 exit 1
}


function set_native_tools ()
{

LINUX_HOST_PREFIX=i386-linux
LINUX32IMAGE=""

if [ "x$NATIVEIMAGE" == "xinstall-Darwin" ]
then
 if [ "x$LINUX" == "xyes" ]; then
   LINUX32IMAGE="install-Linux"
 fi
 HOSTMACHINE="--host=i386-apple-darwin10"
 BUILDMACHINE="--build=i386-apple-darwin10"
 DARWINTARGETARCH="-m32 -arch i386"
 SHASUM256="shasum -a 256 -p"
 if [ -e "/Developer-old" ]
 then
  DEVELOPERDIR="/Developer-old"
 elif [ -e "/Developer" ]
 then
  DEVELOPERDIR="/Developer"
 else
  DEVELOPERDIR=
 fi

  export CXX_FOR_BUILD="$DEVELOPERDIR/usr/bin/g++-4.2 $DARWINTARGETARCH -I$WORKING_DIR/native-build/host-libs/include -isysroot $DEVELOPERDIR/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 -I$DEVELOPERDIR/SDKs/MacOSX10.5.sdk/usr/include/malloc "
  export CC_FOR_BUILD="$DEVELOPERDIR/usr/bin/gcc-4.2 $DARWINTARGETARCH -I$SOURCE_GCC_DIR/gcc/gcc/ginclude -I$WORKING_DIR/native-build/host-libs/include -isysroot $DEVELOPERDIR/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 -I$DEVELOPERDIR/SDKs/MacOSX10.5.sdk/usr/include/malloc "
  export CPP_FOR_BUILD="$DEVELOPERDIR/usr/bin/cpp-4.2 -I$SOURCE_GCC_DIR/gcc/gcc/ginclude -I$WORKING_DIR/native-build/host-libs/include -isysroot $DEVELOPERDIR/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 $DARWINTARGETARCH -I$DEVELOPERDIR/SDKs/MacOSX10.5.sdk/usr/include/malloc "
  export CC="$DEVELOPERDIR/usr/bin/gcc-4.2 -isysroot $DEVELOPERDIR/SDKs/MacOSX10.5.sdk -I$SOURCE_GCC_DIR/gcc/gcc/ginclude -mmacosx-version-min=10.5 $DARWINTARGETARCH -I$DEVELOPERDIR/SDKs/MacOSX10.5.sdk/usr/include/malloc -I$WORKING_DIR/native-build/host-libs/include -lstdc++ "
  export CPP="$DEVELOPERDIR/usr/bin/cpp-4.2 -isysroot $DEVELOPERDIR/SDKs/MacOSX10.5.sdk  -mmacosx-version-min=10.5 $DARWINTARGETARCH -I$WORKING_DIR/native-build/host-libs/include "
  export CPPCXX="$DEVELOPERDIR/usr/bin/cpp-4.2 -isysroot $DEVELOPERDIR/SDKs/MacOSX10.5.sdk  -mmacosx-version-min=10.5 $DARWINTARGETARCH -I$WORKING_DIR/native-build/host-libs/include "
  export CXX="$DEVELOPERDIR/usr/bin/g++-4.2 -isysroot $DEVELOPERDIR/SDKs/MacOSX10.5.sdk  -mmacosx-version-min=10.5 $DARWINTARGETARCH -I$WORKING_DIR/native-build/host-libs/include "
  export LD="$DEVELOPERDIR/usr/bin/g++-4.2 $DARWINTARGETARCH -isysroot $DEVELOPERDIR/SDKs/MacOSX10.5.sdk -mmacosx-version-min=10.5 -I$DEVELOPERDIR/SDKs/MacOSX10.5.sdk/usr/include/malloc -L$WORKING_DIR/native-build/host-libs/lib -I$WORKING_DIR/native-build/host-libs/include "
  export AR="$DEVELOPERDIR/usr/bin/ar"

  LIBHOST=""
  export ABI=32
  if [ "x$NUMBER_OF_JOBS" == "x" ] ; then
    let NUM_USE_CPU=$(sysctl -n hw.physicalcpu )
    # Don't use too many jobs on the Typhoon2 build server
    if [[ $(hostname -s) == "CHN-SV-TYPHOON2" ]] ; then
      if [ $NUM_USE_CPU -gt 3 ] ; then
        let NUM_USE_CPU=$NUM_USE_CPU/2
      fi
    else
      if [ $NUM_USE_CPU -gt 7 ] ; then
        let NUM_USE_CPU=$NUM_USE_CPU/2
      fi
    fi
    NUMBER_OF_JOBS="-j$NUM_USE_CPU"
  fi

elif [ "x$uname_string" == "xlinux" ] ; then
 export CC_FOR_BUILD="/usr/bin/gcc -m32 -march=i386"
 export CXX_FOR_BUILD="/usr/bin/g++ -m32 -march=i386"
 export CPP_FOR_BUILD="/usr/bin/cpp -m32 -march=i386"
 export CC="/usr/bin/gcc -m32 -march=i386"
 export CPP="/usr/bin/cpp -m32 -march=i386"
 export CPPCXX="/usr/bin/cpp -m32 -march=i386"
 export CXX="/usr/bin/g++ -m32 -march=i386"
 export LD="/usr/bin/g++ -m32 -march=i386"
 export AR="/usr/bin/ar" 
 HOSTMACHINE=""
 BUILDMACHINE="--build=x86_64-pc-linux-gnu"
 #BUILDMACHINE="" 
 LIBHOST='--with-host-libstdcxx="-static-libgcc -static-libstdc++ -Wl,-Bstatic,-lstdc++,-Bdynamic,-lm"'

 export ABI=32
 SHASUM256="shasum -a 256 -p"
 if [ "x$NUMBER_OF_JOBS" == "x" ] ; then
   let NUM_USE_CPU=$(awk '/^processor/ {++n} END {print n}' /proc/cpuinfo)
   # Don't use too many jobs
   if [ $NUM_USE_CPU -gt 3 ] ; then
      let NUM_USE_CPU=$NUM_USE_CPU/2
   fi
   NUMBER_OF_JOBS="-j$NUM_USE_CPU"
 fi
else
 export CC_FOR_BUILD="/usr/bin/gcc -m32 -march=i386"
 export CXX_FOR_BUILD="/usr/bin/g++ -m32 -march=i386"
 export CPP_FOR_BUILD="/usr/bin/cpp -m32 -march=i386"
 export CC="/usr/bin/gcc -m32 -march=i386"
 export CPP="/usr/bin/cpp -m32 -march=i386"
 export CPPCXX="/usr/bin/cpp -m32 -march=i386"
 export CXX="/usr/bin/g++ -m32 -march=i386"
 export LD="/usr/bin/g++ -m32 -march=i386"
 export AR="/usr/bin/ar"

 LINUX32IMAGE=""
 HOSTMACHINE=""
 BUILDMACHINE=""
 LIBHOST='--with-host-libstdcxx="-static-libgcc -Wl,-Bstatic,-lstdc++,-Bdynamic,-lm"'
 NUMBER_OF_JOBS=$NUMBER_OF_JOBS
 export ABI=32
 SHASUM="shasum -a 256 -p"
fi # NATIVEIMAGE

echo "HOSTMACHINE=$HOSTMACHINE" >> $LOGFILE
echo "LINUX32IMAGE=$LINUX32IMAGE" >> $LOGFILE
echo "BUILDMACHINE=$BUILDMACHINE" >> $LOGFILE
echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH" >> $LOGFILE
echo "LIBHOST=$LIBHOST" >> $LOGFILE
echo "NUMBER_OF_JOBS=$NUMBER_OF_JOBS" >> $LOGFILE
echo "WORKING_DIR=$WORKING_DIR" >> $LOGFILE
}

trap 'assert_success -1 "ERROR: Caught signal"' SIGHUP SIGINT SIGTERM

### Main script body

DATE=`date +%Y%m%d`
TIME=`date +%H%M`
TVAL=trunk
BUILD=pic32-$DATE

if [ ! "x$FOSSIL_PASSWORD" = "x" ] ; then
  FOSSIL_PASSWORD_AT=":$FOSSIL_PASSWORD@"
fi

if [ "x$FOSSIL_SERVER" = "x" ] ; then
  FOSSIL_SERVER=fossil.microchip.com
fi

#FOSSIL_REMOTE_REPO_ROOT=http://$FOSSIL_USERNAME$FOSSIL_PASSWORD_AT$FOSSIL_SERVER/repos.cgi

DAYTONA_REPO_ROOT=https://chn-vm-piccode.microchip.com/svn/dev-sys/tools/daytona

XC_LOCAL_REPO=$FOSSIL_CLONES/XC32-gcc-48.fsl
XC_LIBS_LOCAL_REPO=$FOSSIL_CLONES/XC-libs.fsl
XCPP_LIBS_LOCAL_REPO=$FOSSIL_CLONES/XCpp-libs.fsl

# order of repos must correspond to order of directories in all_source_directories
all_fossil_repos=(
 pic32-libs
 XC32-docs
 XC32-examples
 XC-libs
 XCpp-libs
 XC32-gcc-48
 c30_resource
 fpmath
 fdlibm
 xclm_release
 Coverity
 pic32-newlib
 )
# order of directories must correspond to order of repos in all_fossil_repos
all_source_directories=(
 pic32-libs
 docs
 examples
 XC-libs
 XCpp-libs
 src48x
 c30_resource
 fpmath
 fdlibm
 xclm_release
 Coverity
 pic32-newlib
)

TAG="x"
BRANCH="x"
FULL_ONLY=no

echo "Native image is $NATIVEIMAGE"

# Process the arguments
while getopts b:FMNJ:Lt:Q opt
do
 case "$opt" in
   t)
   TVAL=$OPTARG
   TAG="$TVAL"
   CVSB=$TVAL
   ;;
   b)
   TVAL=$OPTARG
   BRANCH="$TVAL"
   BUILD=pic32-$TVAL-$DATE
   CVSB=$TVAL
   ;;
   N)
   echo "No checkout"
   CHECKOUT="no"
   ;;
   L)
   echo "Linux build only"
   WINDOWS="no"
   ;;
   M)
   echo "OS X binary build only"
   OSXONLY="yes"
   WINDOWS="no"
   LINUX="no"
   SKIPXCPPLIBS="yes"
   SKIPLIBS="yes"
   SKIPXCLIB="yes"
   ;;
   J)
   TVAL=$OPTARG
   JOBS="$TVAL"
   ;;
  \?) show_usage ;;
 esac
done
 
if [ "x$NATIVEIMAGE" != "xinstall-Darwin" ]; then
  if [ "x$OSXONLY" == "xyes" ]; then
    echo "Error: Cannot specify -M option on non-OSX host for now"
    exit 1
  fi 
fi

NUMBER_OF_JOBS=-j$JOBS

# Avoid double-date build (YYYYMMDD-YYYYMMDD)

#if [[ ${TVAL%%$DATE} = $TVAL ]]; then
#  BUILD=xc32-$TVAL-$DATE
#else
#  BUILD=xc32-$TVAL-$TIME
#fi
BUILD=xc32-$TVAL

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

if [ "$CHECKOUT" = "yes" ]
then
	#
	# Verify existance of local clones. Sync if exist, create new clones if necessary
	#

	if [ "x$FOSSIL_CLONES" == "x" ] ; then
	  assert_success -1 "ERROR: FOSSIL_CLONES environment variable not set"
	fi

	if [ -f $FOSSIL_CLONES ] ; then
	  assert_success -1 "ERROR: $FOSSIL_CLONES clone dir does not exist"
	else
	  status_update "Using FOSSIL_CLONES=$FOSSIL_CLONES"
	fi

	#
	# Verify that we can reach the remote server before attempting to sync the repositories
	#
#	ping -q -c 1 $FOSSIL_SERVER
#	if [ ! $? == 0 ]; then
#	  status_update "WARNING: Fossil server $FOSSIL_SERVER could not be reached, not syncing"
#	  echo "Continue without syncing repository with remote server? (y/n)"
#	  while true; do
#	    read choice
#	    echo
#	    case $choice in
#	      y)
#		echo "Continuing by using local repository clone"
#		echo "WARNING: Disabling autosync"
#		fossil settings autosync off --global
#		break
#		;;
#	      n)
#		assert_success -1 "ERROR: Halting due to failed ping to remote server"
#		break
#		;;
#	      *)
#		echo "That is not a valid choice. Enter 'y' or 'n'."
#		;;
#	    esac
#	  done
#	else
	  pushd $FOSSIL_CLONES > /dev/null
	  status_update "Sync local repository clones"
	  for i in "${all_fossil_repos[@]}"
	  do
	    if [[ -f $i.fsl ]]
	    then
	       echo "fossil sync -R $i.fsl"
	       fossil sync -R $i.fsl
	       assert_success $? "ERROR: fossil sync on $i"
	    else
               VERSTR=`fossil version`
               if [[ $VERSTR =~ MCHP ]]; then
	          echo "yes | fossil klone $i"
	          yes | fossil klone $i
               else
	          echo "fossil klone $i"
	          fossil klone $i
               fi
	      assert_success $? "ERROR: fossil klone on $i"
	    fi
	  done
	  popd > /dev/null
#	fi
fi # CHECKOUT


if [ "$CHECKOUT" = "yes" ]
then
#
# Make sure the branches or tags exist
#

  status_update "Checking for tag/branch in repository"
  for i in "${all_fossil_repos[@]}"
  do
    if [[ $i == "xclm_release" ]]; then
      continue
    fi
    if [[ $i == "Coverity" ]]; then
      continue
    fi
	if [[ $BRANCH != "x" ]]; then
		TAGOK=1;
		TVAL=`fossil branch list -R $FOSSIL_CLONES/$i.fsl | grep "  $BRANCH\$" | wc -l `
		if [ $TVAL == 1 ]; then
		 echo "OK : $i.fsl : $BRANCH "
		 FOSSIL_VERSION=$BRANCH
		else
		 echo "ERROR : Missing $i.fsl : $BRANCH "
		 TAGOK=0;
		fi
	elif [[ $TAG != "x" ]]; then
		TAGOK=1;
		TVAL=`fossil tag list -R $FOSSIL_CLONES/$i.fsl | grep "$TAG\$" | wc -l `
		if [ $TVAL == 1 ]; then
		 echo "OK : $i.fsl : $TAG "
		 FOSSIL_VERSION=$TAG
		else
		 echo "ERROR : Missing $i.fsl : $TAG "
		 TAGOK=0;
		fi
	else
		BRANCH="trunk"
		TAG="trunk"
		TAGOK=1;
		TVAL=`fossil branch list -R $FOSSIL_CLONES/$i.fsl | grep "  $BRANCH\$" | wc -l `
		if [ $TVAL == 1 ]; then
		 echo "OK : $i.fsl : $BRANCH "
		 FOSSIL_VERSION=$BRANCH
		else
		 echo "ERROR : Missing $i.fsl : $BRANCH "
		 TAGOK=0;
		fi
	fi
  done

  if [ $TAGOK -eq 0 ] ; then
    if [[ $BRANCH != "x" ]]; then
      assert_success -1 "ERROR: Missing branches $BRANCH"
    elif [[ $TAG != "x" ]]; then
      assert_success -1 "ERROR: Missing tags $TAG"
    fi
    echo "See pic32-tag-release.sh"
    fossil settings autosync on --global
    exit 1
  fi

# We still use an SVN-style Branch/Tag for some modules
if [[ $TAG == "x" && $BRANCH != "x" ]]
then
 SVNTAG="branches/$BRANCH"
else
 if [ $TAG == "trunk" ]; then
   SVNTAG="trunk"
 else
   SVTAG="tags/$TAG"
 fi
fi

fi # checkout

export TMP
TMP=${WORKING_DIR}/tmp
mkdir -p $TMP

#
#  small program to ask for input, but to time out after seconds
#    usage:   [time] message
#
#    time defaults to 5 seconds
#
cat > ${TMP}/timed_test.c <<EOF
#include <stdio.h>
#include <unistd.h>

void main(int argc, char **argv) {
   char ch;
   int time;

   if (argc > 2 ) time=atoi(argv[1]);
   else time = 5;
   fprintf(stderr, "Timing for %d seconds\n",time);
   fprintf(stderr, "%s ", argv[argc-1]);
   alarm(time);
   while ((ch = getchar()) != '\n' ) putchar(ch);
}
EOF
gcc -o ${TMP}/read ${TMP}/timed_test.c > /dev/null 2>&1

if [ "x$SKIPXCPPLIBS" == "xyes" ] ; then
  echo
  echo "WARNING: Skipping C++ build"
  echo
fi

export MCHP_VERSION
export MCHP_RESOURCE
T="x"

if [ ! -z ${bamboo_buildKey+x} ]; then
  MCHP_VERSION=$bamboo_XC32_FULL_VERSION
  MCHP_RESOURCE=$bamboo_XC32_FULL_RESOURCE
fi

while [ "${MCHP_VERSION}" = "" ]; do
  T=`${TMP}/read 60 "Version number? "`
  if [ "$T" != "" ]; then
    # version numbers must have x.yy or x_yy
    if ( echo $T | grep -q "[0-9][0-9]*[ ._][0-9][0-9]*" );
    then
      MCHP_VERSION=$T;
      MCHP_VERSION=`echo "$MCHP_VERSION" | sed -e 's/ /_/g'`
    else
      echo "Please have a version id of the form xx.yy or xx_yy";
    fi
  fi
done

if [ "${MCHP_RESOURCE}" = "" ]; then
  echo "resource letter "
  T=`${TMP}/read 60 " A-Z for releases, 0-9 for release candidates? "`
  if [ "$T" != "" ]; then
    MCHP_RESOURCE=$T;
  fi
fi

MCHP_RESOURCE="\'${MCHP_RESOURCE}\'"
export MCHP_VERSION
export MCHP_RESOURCE

#
# Create the working directory
#
echo `date` " START PIC32 build." > $LOGFILE
echo `date` " Creating build in $WORKING_DIR..." >> $LOGFILE
#echo "Using XCLM branch $XCLM_BRANCH" >> $LOGFILE
#echo "Using XCLM branch $XCLM_BRANCH"

#if [ -e $WORKING_DIR ]
#then
# echo `date` " $WORKING_DIR already exists..." >> $LOGFILE
#else
# mkdir $WORKING_DIR
# assert_success $? "ERROR: creating directory $WORKING_DIR"
#fi

GCC_CONFIG_FLAGS=(
  --target=pic32mx
  --program-prefix=pic32m-
  --with-bugurl=http://www.microchip.com/support
  --with-dwarf2
  --with-newlib
  --with-gnu-as
  --with-gnu-ld
  --enable-cxx-flags=\"-mno-smart-io\"
  --enable-lto
  --enable-fixed-point
  --enable-gofast
  --enable-languages=c,c++
  --enable-sgxx-sde-multilibs
  --enable-sjlj-exceptions
  --enable-obsolete
  --disable-hosted-libstdcxx
  --disable-libstdcxx-pch
  --disable-libssp
  --disable-libmudflap
  --disable-libffi
  --disable-libfortran
  --disable-bootstrap
  --disable-shared
  --disable-__cxa_atexit
  --disable-nls
  --disable-libgomp
  --disable-threads
  --disable-sim
  --disable-decimal-float
  --disable-libquadmath
  --without-headers
  XGCC_FLAGS_FOR_TARGET=\"-frtti -fexceptions -fno-enforce-eh-specs -mno-smart-io\"
)

cd $WORKING_DIR

#
# Fossil open the repositories
#

if [ "$CHECKOUT" = "yes" ]
then
  status_update "Checking out the source code"
  count_dirs=${#all_source_directories[*]}

  for (( i=0; i<=$(( $count_dirs -1 )); i++ ))
  do
      echo "Checking out source from ${all_fossil_repos[$i]} to ${all_source_directories[$i]}..." >> $LOGFILE
      if [ -e ${all_source_directories[$i]} ] ; then
        rm -rf ${all_source_directories[$i]}
      fi
      mkdir ${all_source_directories[$i]}
      assert_success $? "ERROR: mkdir ${all_source_directories[$i]}"
      pushd ${all_source_directories[$i]} > /dev/null
      if [[ "${all_fossil_repos[$i]}" == "xclm_release" ]] ; then
        fossil open $FOSSIL_CLONES/${all_fossil_repos[$i]}.fsl $XCLM_BRANCH > /dev/null
      elif [[ "${all_fossil_repos[$i]}" == "Coverity" ]] ; then
        fossil open $FOSSIL_CLONES/${all_fossil_repos[$i]}.fsl trunk > /dev/null
      else
        fossil open $FOSSIL_CLONES/${all_fossil_repos[$i]}.fsl $FOSSIL_VERSION > /dev/null
      fi
      assert_success $? "ERROR: fossil open $FOSSIL_CLONES/${all_fossil_repos[$i]}"
      popd > /dev/null
  done

  cd $WORKING_DIR
  pushd xclm_release
  cp ./dist/api/xclm_public.h $SOURCE_GCC_DIR/gcc/gcc/config/mchp-cci/xclm_public.h
  assert_success $? "ERROR: copy xclm_public.h"
  popd > /dev/null

  cd $SOURCE_GCC_DIR
  cp -r $WORKING_DIR/c30_resource ./c30_resource
  assert_success $? "ERROR: moving out c30_resource to src48x directory"

  cd $WORKING_DIR

fi # END CHECKOUT

cd $WORKING_DIR

set_native_tools

if [ "$WINDOWS" = "yes" ] ; then
  cd $WORKING_DIR
  WIN_BUILD=build-pic32m-mingw
  WIN_IMAGE=install-mingw

fi
### $1 name of the build directory e.g. libbuild
build_install_headers() {
  CMD="make DESTROOT=$WORKING_DIR/$1 install-headers"
  xc32_eval "$CMD" "Error building install headers"
}

cd $WORKING_DIR

#if [ "x$SKIPREBUILD" == "x" ]; then

##########################################################################
# Install libc headers into cross compiler's install image directory
#######################################################################
cd $SOURCE_DIR/XC-libs/htc/XC32

# Install headers into cross compiler's install image directory
echo "Making libc headers for cross-compiler"
echo `date` "Making library headers for cross compiler's install image..." >> $LOGFILE

build_install_headers $NATIVE_NOLM_IMAGE
build_install_headers $NATIVEIMAGE
build_install_headers "export-image"

if [ "x$LINUX32IMAGE" != "x" ] ; then
  build_install_headers $LINUX32IMAGE
fi

if [ "$WINDOWS" = "yes" ] ; then
  build_install_headers $WIN_IMAGE
fi

cd $WORKING_DIR

##########################################################################
# Install fdlibm headers into cross compiler's install image directory
#######################################################################
cd $SOURCE_DIR/fdlibm/src
cd xc32

# Install headers into cross compiler's install image directory
echo "Making fdlibm headers for cross-compiler"
echo `date` "Making library headers for cross compiler's install image..." >> $LOGFILE

build_install_headers $NATIVE_NOLM_IMAGE
build_install_headers $NATIVEIMAGE
build_install_headers "export-image"

if [ "x$LINUX32IMAGE" != "x" ] ; then
  build_install_headers $LINUX32IMAGE
fi

if [ "$WINDOWS" = "yes" ] ; then
  build_install_headers $WIN_IMAGE
fi

cd $WORKING_DIR

#############################################
######### Build XC Cpp Library ##############
#############################################
if [ "x$SKIPXCPPLIBS" == "x" ] ; then
cd $SOURCE_DIR/XCpp-libs

# Install Cpp headers into cross compiler's install image directory
echo "Making Cpp headers for cross-compiler"
echo `date` "Making Cpp library headers for cross compiler's install image..." >> $LOGFILE

build_install_headers $NATIVE_NOLM_IMAGE
build_install_headers $NATIVEIMAGE
build_install_headers "export-image"

if [ "x$LINUX32IMAGE" != "x" ] ; then
  build_install_headers $LINUX32IMAGE
fi

if [ "x$WINDOWS" == "xyes" ] ; then
  build_install_headers  $WIN_IMAGE
fi

fi # SKIPXCPPLIBS

cd $WORKING_DIR

##########################################################################
# Install pic32-lib headers into cross compiler's install image directory
#######################################################################
cd $SOURCE_DIR/pic32-libs

echo "Making library headers for cross-compiler"
echo `date` "Making library headers for cross compiler's install image..." >> $LOGFILE

build_install_headers $NATIVE_NOLM_IMAGE
build_install_headers $NATIVEIMAGE
build_install_headers "export-image"

if [ "x$LINUX32IMAGE" != "x" ] ; then
  build_install_headers $LINUX32IMAGE
fi

if [ "$WINDOWS" = "yes" ] ; then
  build_install_headers  $WIN_IMAGE
fi

cd $WORKING_DIR

##############################################
######### Building Info files ################
#############################################
cd $SOURCE_DIR/c30_resource/src
./xc32-build.sh

status_update "Installing info files and .LanguageToolSuite file"

cd $SOURCE_DIR/c30_resource/src/xc32

if [ "SKIPRESOURCEFILE" != "yes" ]
then

        # TODO: Copying to multiple places for now. Eventually consolidate to /bin/device_files
		# Done the changes. Verify for a new TC build and then remove the commented out lines.

        mkdir -p $WORKING_DIR/$NATIVE_NOLM_IMAGE
        mkdir -p $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin
        mkdir -p $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/device_files
        cp *.info $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/device_files/.
        assert_success $? "ERROR: Copying device files"

        mkdir -p $WORKING_DIR/$NATIVE_NOLM_IMAGE/pic32mx
#        mkdir -p $WORKING_DIR/$NATIVE_NOLM_IMAGE/pic32mx/device_files/.
#        cp *.info $WORKING_DIR/$NATIVE_NOLM_IMAGE/pic32mx/device_files/.
#        assert_success $? "ERROR: Copying device files"

#        mkdir -p $WORKING_DIR/$NATIVE_NOLM_IMAGE/pic32mx/bin/device_files/.
#        cp *.info $WORKING_DIR/$NATIVE_NOLM_IMAGE/pic32mx/bin/device_files/.
#        assert_success $? "ERROR: Copying device files"

#        mkdir -p $WORKING_DIR/$NATIVE_NOLM_IMAGE/device_files/.
#        cp *.info $WORKING_DIR/$NATIVE_NOLM_IMAGE/device_files/.
#        assert_success $? "ERROR: Copying device files"

#        cp $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/device_files/xc32_device.info $WORKING_DIR/$NATIVE_NOLM_IMAGE/pic32mx/.
#        assert_success $? "ERROR: Copying xc32_device info file"

        cp $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/device_files/xc32_device.info $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/.
        assert_success $? "ERROR: Copying xc32_device info file"

        cp .LanguageToolSuite $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/.
        assert_success $? "ERROR: Copying .LanguageToolSuite file"

        # TODO: Copying to multiple places for now. Eventually consolidate to /bin/device_files
        mkdir -p $WORKING_DIR/$NATIVEIMAGE
        mkdir -p $WORKING_DIR/$NATIVEIMAGE/bin
        mkdir -p $WORKING_DIR/$NATIVEIMAGE/bin/device_files
        cp *.info $WORKING_DIR/$NATIVEIMAGE/bin/device_files/.
        assert_success $? "ERROR: Copying device info files"

        mkdir -p $WORKING_DIR/$NATIVEIMAGE/pic32mx
#        mkdir -p $WORKING_DIR/$NATIVEIMAGE/pic32mx/device_files/.
#        cp *.info $WORKING_DIR/$NATIVEIMAGE/pic32mx/device_files/.
#        assert_success $? "ERROR: Copying device info files"

#        mkdir -p $WORKING_DIR/$NATIVEIMAGE/pic32mx/bin/device_files/.
#        cp *.info $WORKING_DIR/$NATIVEIMAGE/pic32mx/bin/device_files/.
#        assert_success $? "ERROR: Copying device info files"

#        mkdir -p $WORKING_DIR/$NATIVEIMAGE/device_files/.
#        cp *.info $WORKING_DIR/$NATIVEIMAGE/device_files/.
#        assert_success $? "ERROR: Copying device info files"

#        cp $WORKING_DIR/$NATIVEIMAGE/bin/device_files/xc32_device.info $WORKING_DIR/$NATIVEIMAGE/pic32mx/.
#        assert_success $? "ERROR: Copying xc32_device info file"

        cp $WORKING_DIR/$NATIVEIMAGE/bin/device_files/xc32_device.info $WORKING_DIR/$NATIVEIMAGE/bin/.
        assert_success $? "ERROR: Copying xc32_device info file"

        cp deviceSupport.xml $WORKING_DIR/$NATIVEIMAGE/bin/.
        assert_success $? "ERROR: Copying deviceSupport.xml file"
        cp .LanguageToolSuite $WORKING_DIR/$NATIVEIMAGE/bin/.
        assert_success $? "ERROR: Copying .LanguageToolSuite file"

        cd $SOURCE_DIR/c30_resource/src/xc32

fi # Resource file install

if [ "$WINDOWS" == "yes" ] ; then

        if [ "SKIPRESOURCEFILE" != "yes" ]
        then
                mkdir -p $INSTALLDIR_MINGW/bin/device_files
                cp *.info $INSTALLDIR_MINGW/bin/device_files/.
                assert_success $? "ERROR: Copying $INSTALLDIR_MINGW/bin/device_files"

#                mkdir -p $INSTALLDIR_MINGW/pic32mx/device_files/.
#               cp *.info $INSTALLDIR_MINGW/pic32mx/device_files/.
#                assert_success $? "ERROR: Copying $INSTALLDIR_MINGW/bin/device_files"

#                mkdir -p $INSTALLDIR_MINGW/pic32mx/bin/device_files/.
#                cp *.info $INSTALLDIR_MINGW/pic32mx/bin/device_files/.
#                assert_success $? "ERROR: Copying $INSTALLDIR_MINGW/bin/device_files"

#                mkdir -p $INSTALLDIR_MINGW/device_files/.
#                cp *.info $INSTALLDIR_MINGW/device_files/.
#                assert_success $? "ERROR: Copying $INSTALLDIR_MINGW/bin/device_files"

#                cp $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/device_files/xc32_device.info $INSTALLDIR_MINGW/pic32mx/.
#                assert_success $? "ERROR: Copying xc32_device info file"

#                cp $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/device_files/xc32_device.info $INSTALLDIR_MINGW/.
#                assert_success $? "ERROR: Copying xc32_device info file"

                cp $INSTALLDIR_MINGW/bin/device_files/xc32_device.info $INSTALLDIR_MINGW/bin/.
                assert_success $? "ERROR: Copying xc32_device info file"

                cp deviceSupport.xml $INSTALLDIR_MINGW/bin/.
                assert_success $? "ERROR: Copying deviceSupport.xml file"
                cp .LanguageToolSuite $INSTALLDIR_MINGW/bin/.
                assert_success $? "ERROR: Copying .LanguageToolSuite file"

        fi # Resource file

fi

if [[ ! -e $WORKING_DIR/export-image/bin ]] ; then
 mkdir $WORKING_DIR/export-image/bin
fi

if [ "SKIPRESOURCEFILE" != "yes" ]
then
        cd $SOURCE_DIR/c30_resource/src/xc32
    
        mkdir $WORKING_DIR/export-image/bin/device_files
        cp *.info $WORKING_DIR/export-image/bin/device_files/.
        assert_success $? "ERROR: Copying device info files"
        cp $WORKING_DIR/export-image/bin/device_files/xc32_device.info $WORKING_DIR/export-image/bin/.
        assert_success $? "ERROR: Copying xc32_device info file"
        cp deviceSupport.xml $WORKING_DIR/export-image/bin/.
        assert_success $? "ERROR: Copying deviceSupport.xml"
        cp .LanguageToolSuite $WORKING_DIR/export-image/bin/.
        assert_success $? "ERROR: Copying .LanguageToolSuite file"
fi

if [ "x$LINUX32IMAGE" != "x" ] ; then

        if [ "SKIPRESOURCEFILE" != "yes" ]
        then
                cd $SOURCE_DIR/c30_resource/src/xc32

                mkdir -p $WORKING_DIR/$LINUX32IMAGE/bin/device_files
                cp *.info $WORKING_DIR/$LINUX32IMAGE/bin/device_files/.
                assert_success $? "ERROR: Copying device info files"

#                mkdir -p $WORKING_DIR/$LINUX32IMAGE/pic32mx/device_files/.
#                cp *.info $WORKING_DIR/$LINUX32IMAGE/pic32mx/device_files/.
#                assert_success $? "ERROR: Copying device info files"

#               mkdir -p $WORKING_DIR/$LINUX32IMAGE/pic32mx/bin/device_files/.
#                cp *.info $WORKING_DIR/$LINUX32IMAGE/pic32mx/bin/device_files/.
#                assert_success $? "ERROR: Copying device info files"

#                mkdir -p $WORKING_DIR/$LINUX32IMAGE/device_files/.
#                cp *.info $WORKING_DIR/$LINUX32IMAGE/device_files/.
#                assert_success $? "ERROR: Copying device info files"

#                cp $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/device_files/xc32_device.info $WORKING_DIR/$LINUX32IMAGE/pic32mx/.
#                cp $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/device_files/xc32_device.info $WORKING_DIR/$LINUX32IMAGE/.
#                assert_success $? "ERROR: Copying xc32_device info file"

#                assert_success $? "ERROR: Copying device info files"
                cp $WORKING_DIR/$LINUX32IMAGE/bin/device_files/xc32_device.info $WORKING_DIR/$LINUX32IMAGE/bin/.
                assert_success $? "ERROR: Copying xc32_device info file"

                cp deviceSupport.xml $WORKING_DIR/$LINUX32IMAGE/bin/.
                assert_success $? "ERROR: Copying deviceSupport.xml"
                cp .LanguageToolSuite $WORKING_DIR/$LINUX32IMAGE/bin/.
                assert_success $? "ERROR: Copying .LanguageToolSuite file"

        fi # Resource file
fi

cd $WORKING_DIR

##############################################
######### General functions ################
#############################################

### 1 name of the directory
erase_and_make_dir() {
  if [ -e $1 ]; then
    xc32_eval "rm -rf $1"
  fi
  mkdir $1
}

ensure_dir() {
  if [ ! -e $1 ] ; then
    status_update "WARNING: build-pic32m-toolchain creating directory $1"
    mkdir -p $1
  fi
}

### $1 name of the directory
### $2 any extra arguments for the make
build_xc32_sh() {
  set +u
  pushd $1
  
  status_update "Calling build_xc32_sh from pic32m. This should not happen anymore."
  exit -1 # We should no longer call this function
  
  XC32_SH_SRC=${WORKING_DIR}/common-source/xc32-sh
  cd $XC32_SH_SRC/bin

  #clean
  #necessary since there isn't a separate build directory
  COMMAND="make -j2 clean"
  xc32_eval "$COMMAND" "xc32-sh make clean failed"

  #make
  set +u
  COMMAND="make $NUMBER_OF_JOBS xc32 $2"
  xc32_eval "$COMMAND" "xc32-sh make failed"

  #install
  #No need to add the /bin for call below
  export XC32_INSTALL=$1
  set +u
  COMMAND="make -j2 xc32-install $2"
  xc32_eval "$COMMAND" "xc32-sh make install failed"
  unset XC32_INSTALL
  popd
}

### $1 name of the build directory e.g. native-build
### $2 name of alternate install directory e.g. install-Darwin
### $3 description of type of build e.g. cross
### $4 configure options
build_binutils() {
  set +u
  if [ "x$SKIPREBUILD" == "x" ]; then

   pushd $1
   erase_and_make_dir binutils
   cd binutils

  # Configure cross binutils
   echo `date` " Configuring $3 binutils build in $WORKING_DIR/$1..." >> $LOGFILE
   echo "Configure command: $SOURCE_GCC_DIR/binutils/configure $4" >> $LOGFILE
   xc32_eval "$SOURCE_GCC_DIR/binutils/configure $4" "ERROR: configuring $3 binutils build"

  # Make binutils and install it
   echo `date` " Making all in $1/build/binutils and installing..." >> $LOGFILE
   xc32_eval "make all $NUMBER_OF_JOBS" "ERROR: making/installing $3 binutils build"
   xc32_eval "make -j2 install" "ERROR: making/installing $3 binutils build"

  if [ "$2" != "x" ] ; then
     # Install to $INSTALLDIR_NATIVE. This combination of flags seems to work
     # for the binutils Makefile's weird path configuration, which is set
     # up for DESTDIR="", prefix=$INSTALLDIR_NATIVE_NOLM, bindir=$INSTALLDIR_NATIVE_NOLM/bin/bin.
     xc32_eval "make -j2 DESTDIR=$2/ bindir=bin/bin prefix='' install" "ERROR: installing $3 binutils build"
  fi

   # install to nolm as well.
   popd

  fi
  
}


### $1 name of the build directory e.g. native-build
### $2 description of type of build e.g. cross
### $3 configure command line taken as array
build_gcc() {
  set +u
  if [ "x$SKIPREBUILD" == "x" ]; then
   pushd $1
   erase_and_make_dir gcc
   cd gcc

  # Configure $2 compiler
   echo "" >> $LOGFILE
   echo `date` " Configuring cross compiler build in $1..." >> $LOGFILE

   declare -a COMMAND=("${!3}")
   TEMP=${COMMAND[@]}
   xc32_eval "$TEMP" "ERROR: configuring $2 build"

   # Make cross compiler (make all-gcc)
   echo `date` " Making all in $1/gcc and installing..." >> $LOGFILE
   COMMAND="make all-gcc $NUMBER_OF_JOBS"
   xc32_eval "$COMMAND" "ERROR: making/installing $2 build all-gcc"

   #Install cross compiler (make install-gcc)
   COMMAND="make install-gcc $NUMBER_OF_JOBS"
   xc32_eval "$COMMAND"

   COMMAND="make install-strip-gcc $NUMBER_OF_JOBS"
   xc32_eval "$COMMAND" "ERROR: making/installing $2 build install-strip"

   popd
   status_update "$2 build complete"

  fi
  
}

EXE=
ALL_EXES=(
          AR_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-ar${EXE}
          AS_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-as${EXE}
          LD_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-ld${EXE}
          GCC_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-gcc${EXE}
          XGCC_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-gcc${EXE}
          CC_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-gcc${EXE}
          CPP_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-gcc${EXE}
          CPPCXX_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-g++${EXE}
          CXX_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-g++${EXE}
          NM_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-nm${EXE}
          RANLIB_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-ranlib${EXE}
          STRIP_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-strip${EXE}
          OBJDUMP_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-objdump${EXE}
  )

### $1 name of the build directory e.g. native-build
build_gcc_targetlibs() {
  set +u
  if [ "x$SKIPREBUILD" == "x" ]; then
   pushd $1/gcc

   echo "" >> $LOGFILE
   echo `date` " build_gcc_targetlibs $1..." >> $LOGFILE
   set +u
  # Make everything else (make all-target)
   COMMAND="make all-target $NUMBER_OF_JOBS ${ALL_EXES[@]}"
   xc32_eval "$COMMAND" "ERROR: making/installing $2 build all-target"

   COMMAND="make install-strip-target -j2"
   xc32_eval "$COMMAND" "ERROR: making/installing $2 build install-target"
   popd
   status_update "$1 build_gcc_targetlibs complete"
   
  fi
  
}

copy_gcc_targetlibs() {
  set +u
  if [ "x$SKIPREBUILD" == "x" ]; then

  pushd $1/gcc
  COMMAND="make prefix=$2 install-strip-target -j2"
  xc32_eval "$COMMAND" "ERROR: copy_gcc_targetlibs $1"
  popd

  fi
  
}


############################################################################
# Build native compiler
############################################################################
cd $WORKING_DIR

if [ "x$SKIPNATIVE_NOLM" == "x" ] ; then
if [ "x$SKIPREBUILD" == "x" ] ; then
        # Build native cross compiler
        echo `date` " Creating cross build in $WORKING_DIR/$NATIVE_NOLM_BUILD..." >> $LOGFILE
        status_update "Beginning native xc32 build"

        cd $WORKING_DIR
        ensure_dir $NATIVE_NOLM_BUILD
        cd $NATIVE_NOLM_BUILD
        BUILD_DIR=`pwd`

        cd $WORKING_DIR
        LOGTASKFILE="$WORKING_DIR/log-build-pic32m-nativenolm-binutils.txt"
        echo "Open $LOGTASKFILE" >$LOGTASKFILE
        build_binutils "$WORKING_DIR/$NATIVE_NOLM_BUILD" "$WORKING_DIR/$NATIVEIMAGE" "cross" "--target=pic32mx --prefix="$WORKING_DIR/$NATIVE_NOLM_IMAGE" --bindir="$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin" --libexecdir="$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin" --disable-nls --disable-tui --disable-gdbtk --disable-shared --enable-static --disable-threads --disable-bootstrap --with-dwarf2 --enable-multilib --without-newlib --disable-sim --with-lib-path=: --enable-poison-system-directories --program-prefix=pic32m- --with-bugurl=http://www.microchip.com/support --disable-werror"

        cd $WORKING_DIR
        # Copy binutils executables into NATIVEIMAGE
        # Removed as this will clobber anything already installed under NATIVEIMAGE.
        # Instead, build_binutils installs to multiple locations.
        #xc32_eval "rsync -qavzCr --include=* \"$WORKING_DIR/$NATIVE_NOLM_IMAGE/\" \"$WORKING_DIR/$NATIVEIMAGE\""

        cd $BUILD_DIR

        erase_and_make_dir gcc
        cd gcc

        USE_CLOOG="--without-cloog"
        USE_PPL="--without-isl"

        LOGTASKFILE="$WORKING_DIR/log-build-pic32m-nativenolm-gcc.txt"
        echo "Open $LOGTASKFILE" >$LOGTASKFILE

        CONFIG_CMD=($SOURCE_GCC_DIR/gcc/configure "${ALL_EXES[@]}" $HOSTMACHINE $BUILDMACHINE CFLAGS=\"-DSKIP_LICENSE_MANAGER $CFLAGS\" CXXFLAGS=\"-DSKIP_LICENSE_MANAGER -g3 $CXXFLAGS\" --enable-static --disable-shared --prefix=$WORKING_DIR/$NATIVE_NOLM_IMAGE --includedir=$WORKING_DIR/$NATIVE_NOLM_IMAGE/pic32mx/include --bindir=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin --libexecdir=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin   --with-libelf=$WORKING_DIR/$NATIVE_LM_BUILD/host-libs --with-gmp=$WORKING_DIR/$NATIVE_LM_BUILD/host-libs ${USE_PPL} ${USE_CLOOG} "${GCC_CONFIG_FLAGS[@]}" "${LIBHOST[@]}" $SUPPORT_CHECKING)

        build_gcc "$WORKING_DIR/$NATIVE_NOLM_BUILD" "cross" CONFIG_CMD[@]
        build_gcc_targetlibs "$WORKING_DIR/$NATIVE_NOLM_BUILD"

        status_update "Cross build complete"
fi # SKIPREBUILD

        cd $WORKING_DIR

        # end strip native-image

fi # skipping native nolm

# end build native toolchain

# Set up path so that we can build the libraries and the win32 cross
# compiler using the cross compiler we just built
OLDPATH=$PATH
PATH=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin:$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin:$PATH

if [ "x$SKIPLIBS" == "x" ] ; then

        cd $WORKING_DIR

        ####
        # Build and install Newlib for chipKIT compatibility
        ####
        cd $NATIVE_NOLM_BUILD
        erase_and_make_dir "newlib"
        cd newlib

        NEWLIB_INSTALL_OPTS="-m ../../$NATIVEIMAGE -e  ../../$NATIVE_NOLM_IMAGE"
        if [ "$WINDOWS" = "yes" ] ; then
          NEWLIB_INSTALL_OPTS+=" -w ../../$WIN_IMAGE"
        fi
        if [ "x$LINUX32IMAGE" != "x" ] ; then
          NEWLIB_INSTALL_OPTS+=" -l ../../$LINUX32IMAGE"
        fi

        LOGTASKFILE="$WORKING_DIR/log-build-pic32m-targetlibs.txt"
        echo "Open $LOGTASKFILE" >$LOGTASKFILE

        xc32_eval "$ROOT/pic32m-source/pic32-newlib/xc32-build-newlib.sh $NEWLIB_INSTALL_OPTS" "Error building and installing Newlib"

        cd $ROOT/pic32m-source/pic32-libs

        # Build cross compiler libraries
        echo `date` " Making and installing cross-compiler pic32-libs libraries to $WORKING_DIR/$NATIVEIMAGE..." >> $LOGFILE
        xc32_eval "make DESTROOT="$WORKING_DIR/$NATIVE_NOLM_IMAGE" clean -j2" "ERROR: making libraries for cross build"
        xc32_eval "make DESTROOT="$WORKING_DIR/$NATIVE_NOLM_IMAGE" all $NUMBER_OF_JOBS" "ERROR: making libraries for cross build"

        # Install cross compiler libraries
        xc32_eval "make DESTROOT="$WORKING_DIR/$NATIVE_NOLM_IMAGE" install -j2" "ERROR: making pic32-libs for $NATIVE_NOLM_IMAGE cross build"
        xc32_eval "make DESTROOT="$WORKING_DIR/$NATIVEIMAGE" install -j2" "ERROR: making pic32-libs for $NATIVEIMAGE cross build"

        if [ "x$LINUX32IMAGE" != "x" ] ; then
         xc32_eval "make DESTROOT="$WORKING_DIR/$LINUX32IMAGE" install -j2" "ERROR: making pic32-libs for $LINUX32IMAGE cross build"
        fi

        xc32_eval "make DESTROOT="$WORKING_DIR/export-image" install -j2" "ERROR: making pic32-libs for export-image cross build"

        if [ "$WINDOWS" = "yes" ] ; then
          xc32_eval "make DESTROOT="$INSTALLDIR_MINGW" install -j2" "ERROR: making pic32-libs for $WIN_IMAGE cross build"
        fi

        status_update "cross-compiler pic32-libs library build complete"
        cd $ROOT/pic32m-source

        cd $ROOT/pic32m-source/fpmath/xc32

        # Build cross compiler libraries
        echo `date` " Making and installing cross-compiler fpmath/xc32 libraries to $WORKING_DIR/$NATIVEIMAGE..." >> $LOGFILE
        xc32_eval "make DESTROOT="$WORKING_DIR/$NATIVE_NOLM_IMAGE" all $NUMBER_OF_JOBS" "ERROR: making libraries for cross build"

        # Install cross compiler libraries
        xc32_eval "make DESTROOT="$WORKING_DIR/$NATIVE_NOLM_IMAGE" install -j2" "ERROR: making fpmath library for $NATIVE_NOLM_IMAGE cross build"
        xc32_eval "make DESTROOT="$WORKING_DIR/$NATIVEIMAGE" install -j2" "ERROR: making fpmath library for $NATIVEIMAGE cross build"

        if [ "x$LINUX32IMAGE" != "x" ] ; then
         xc32_eval "make DESTROOT="$WORKING_DIR/$LINUX32IMAGE" install -j2" "ERROR: making fpmath library for $LINUX32IMAGE cross build"
        fi

        xc32_eval "make DESTROOT="$WORKING_DIR/export-image" install -j2" "ERROR: making fpmath library for export-image cross build"

        if [ "$WINDOWS" = "yes" ] ; then
          xc32_eval "make DESTROOT="$INSTALLDIR_MINGW" install -j2" "ERROR: making fpmath library for $WIN_IMAGE cross build"
        fi

        status_update "cross-compiler fpmath library library build complete"
        cd $WORKING_DIR

        if [ "x$SKIPXCLIB" = "x" ] ; then
          cd $ROOT/pic32m-source/XC-libs/htc/XC32

          # Build cross compiler libraries once
          echo `date` " Making and installing cross-compiler XC-libs libraries to $WORKING_DIR/$NATIVE_NOLM_IMAGE..." >> $LOGFILE
          xc32_eval "find $ROOT/pic32m-source/XC-libs/htc/XC32 -name \"*.o\" -delete" "ERROR: Deleting object files from XC-libs/htc/XC32"
          xc32_eval "find $ROOT/pic32m-source/XC-libs/htc/XC32 -name \"*.eo\" -delete" "ERROR: Deleting object files from XC-libs/htc/XC32"
          xc32_eval "make DESTROOT=$WORKING_DIR/$NATIVE_NOLM_IMAGE all $NUMBER_OF_JOBS" "ERROR: making XC-libs/htc/XC32 libraries for cross build"

          # Then install
          xc32_eval "make DESTROOT=$WORKING_DIR/$NATIVE_NOLM_IMAGE install -j2" "ERROR: installing XC-libs/htc/XC32 libraries for $NATIVE_NOLM_IMAGE cross build"
          xc32_eval "make DESTROOT=$WORKING_DIR/$NATIVEIMAGE install -j2" "ERROR: installing XC-libs/htc/XC32 libraries for $NATIVEIMAGE cross build"

          if [ "x$LINUX32IMAGE" != "x" ] ; then
            xc32_eval "make DESTROOT="$WORKING_DIR/$LINUX32IMAGE" install -j2" "ERROR: making XC-libs libraries for $LINUX32IMAGE cross build"
          fi
          xc32_eval "make DESTROOT="$WORKING_DIR/export-image" install -j2" "ERROR: installing XC-libs libraries for export-image cross build"

          if [ "$WINDOWS" = "yes" ] ; then
            xc32_eval "make DESTROOT="$INSTALLDIR_MINGW" install -j2" "ERROR: installing XC-libs libraries for $WIN_IMAGE cross build"
          fi

          #Build fdlibm and install fdlibm
          cd $ROOT/pic32m-source/fdlibm/src
          cd xc32

          #Build fdlibm once
          echo `date` " Making and installing cross-compiler fdlibm libraries to $WORKING_DIR/$NATIVE_NOLM_IMAGE..." >> $LOGFILE
          xc32_eval "make DESTROOT=$WORKING_DIR/$NATIVE_NOLM_IMAGE all $NUMBER_OF_JOBS" "ERROR: making fdlibm  libraries for cross build"

          # Then install
          xc32_eval "make DESTROOT=$WORKING_DIR/$NATIVE_NOLM_IMAGE install -j2" "ERROR: making fdlibm libraries for $NATIVE_NOLM_IMAGE cross build"
          xc32_eval "make DESTROOT=$WORKING_DIR/$NATIVEIMAGE install -j2" "ERROR: making fdlibm libraries for $NATIVEIMAGE cross build"

          if [ "x$LINUX32IMAGE" != "x" ] ; then
            xc32_eval "make DESTROOT="$WORKING_DIR/$LINUX32IMAGE" install -j2" "ERROR: making fdlibm libraries for $LINUX32IMAGE cross build"
          fi
          xc32_eval "make DESTROOT="$WORKING_DIR/export-image" install -j2" "ERROR: making fdlibm libraries for export-image cross build"

          if [ "$WINDOWS" = "yes" ] ; then
            xc32_eval "make DESTROOT="$INSTALLDIR_MINGW" install -j2" "ERROR: making fdlibm libraries for $WIN_IMAGE cross build"
          fi
          
          status_update " pic32m cross-compiler fdlibm library build complete"

        fi

        if [ "x$SKIPXCPPLIBS" = "x" ] ; then
          # Dinkum Compleat
          cd $ROOT/pic32m-source/XCpp-libs

          # Build cross compiler libraries
          echo `date` " Making and installing cross-compiler XCpp-libs libraries to $WORKING_DIR/$NATIVE_NOLM_IMAGE..." >> $LOGFILE
          xc32_eval "find $ROOT/pic32m-source/XCpp-libs -name \"*.o\" -delete" "ERROR: Deleting object files from XCpp-libs"
          xc32_eval "make DESTROOT=$WORKING_DIR/$NATIVE_NOLM_IMAGE all $NUMBER_OF_JOBS" "ERROR: making libraries for cross build"

          # Then install
          xc32_eval "make DESTROOT=$WORKING_DIR/$NATIVE_NOLM_IMAGE install -j2" "ERROR: making XCpp-libs libraries for $NATIVE_NOLM_IMAGE cross build"

          xc32_eval "make DESTROOT=$WORKING_DIR/$NATIVEIMAGE install -j2" "ERROR: installing  XCpp-libs libraries for $NATIVEIMAGE cross build"

          if [ "x$LINUX32IMAGE" != "x" ] ; then
            xc32_eval "make DESTROOT=$WORKING_DIR/$LINUX32IMAGE install -j2" "ERROR: installing XC-libs libraries for $LINUX32IMAGE cross build"
          fi

          xc32_eval "make DESTROOT=$WORKING_DIR/export-image install -j2" "ERROR: installing XCpp-libs libraries for export-image cross build"

          if [ "$WINDOWS" = "yes" ] ; then
            xc32_eval "make DESTROOT=$INSTALLDIR_MINGW install -j2" "ERROR: installing XCpp-libs libraries for $WIN_IMAGE cross build"
          fi

          status_update "pic32m cross-compiler XCpp-libs library build complete"
        fi
fi # skip library build

if [ "x$SKIPNATIVE" == "x" ] ; then

####################################################################
# Begin building licensed native
####################################################################
# Build native cross compiler
echo `date` " Creating cross build in Licensed $WORKING_DIR/$NATIVEIMAGE..." >> $LOGFILE
status_update "pic32m Beginning native licensed xc32 build"

LOGTASKFILE="$WORKING_DIR/log-build-pic32m-native-lm-gcc.txt"
echo "Open $LOGTASKFILE" >$LOGTASKFILE

set_native_tools

# Build license compiler
cd "$WORKING_DIR"

if [ "x$SKIPREBUILD" == "x" ] ; then

ensure_dir "$NATIVE_LM_BUILD"
cd "$WORKING_DIR/$NATIVE_LM_BUILD"

erase_and_make_dir "gcc"
cd "$WORKING_DIR/$NATIVE_LM_BUILD/gcc"
if [ "x$SHASUM256" == "x" ] ; then
  assert_success -1 "ERROR: SHASUM256 not set in build-pic32m-toolchain.sh"
fi
if [ "x$uname_string" == "xdarwin" ] ; then
  if [ ! -e $WORKING_DIR/$NATIVEIMAGE/xclm/install/client/bin/xclm ] ; then
    assert_success -1 "ERROR: Missing $WORKING_DIR/$NATIVEIMAGE/xclm/install/client/bin/xclm"
  fi
  XCLM_SHASUM=`$SHASUM256 $WORKING_DIR/$NATIVEIMAGE/xclm/install/client/bin/xclm | head -c 64`
  if [ ${#XCLM_SHASUM} != 64 ]; then
      assert_success -1 "ERROR: Failed to calculate SHASUM256 digest for $NATIVEIMAGE/bin/xclm"
  fi
  XCLM_SHASUM_MACRO="-DMCHP_XCLM_SHA256_DIGEST=${XCLM_SHASUM} "
elif [ "x$uname_string" == "xlinux" ] ; then
  if [ ! -e $WORKING_DIR/$NATIVEIMAGE/xclm/install/client/bin/xclm ] ; then
    assert_success -1 "ERROR: Missing $WORKING_DIR/$NATIVEIMAGE/xclm/install/client/bin/xclm"
  fi
  XCLM_SHASUM=`$SHASUM256 $WORKING_DIR/$NATIVEIMAGE/xclm/install/client/bin/xclm | head -c 64`
  if [ ${#XCLM_SHASUM} != 64 ]; then
      assert_success -1 "ERROR: Failed to calculate SHASUM256 digest for $NATIVEIMAGE/bin/xclm"
  fi
  XCLM64_SHASUM=`$SHASUM256 $WORKING_DIR/$NATIVEIMAGE/xclm64/install/client/bin/xclm | head -c 64`
  if [ ${#XCLM64_SHASUM} != 64 ]; then
      assert_success -1 "ERROR: Failed to calculate SHASUM256 digest for 64-bit xclm"
  fi
  XCLM_SHASUM_MACRO="-DMCHP_XCLM_SHA256_DIGEST=${XCLM_SHASUM} -DMCHP_XCLM64_SHA256_DIGEST=${XCLM64_SHASUM} -DMCHP_FXCLM_SHA256_DIGEST=8727ea3da9bdd624fee0130eb6133188719892bcbee7da32606911a8b08a1a8d "
else
  echo "ERROR: No XCLM_SHASUM_MACRO defined"
  echo "ERROR: No XCLM_SHASUM_MACRO defined" >> $LOGFILE
fi

  CONFIG_CMD=("${ALL_EXES[@]}" $SOURCE_GCC_DIR/gcc/configure --enable-static CFLAGS=\"${XCLM_SHASUM_MACRO}$CFLAGS\" CXXFLAGS=\"${XCLM_SHASUM_MACRO}$CXXFLAGS\" --disable-shared --prefix=$WORKING_DIR/$NATIVEIMAGE --includedir=$WORKING_DIR/$NATIVEIMAGE/pic32mx/include --bindir=$WORKING_DIR/$NATIVEIMAGE/bin/bin --libexecdir=$WORKING_DIR/$NATIVEIMAGE/bin/bin --with-libelf=$WORKING_DIR/$NATIVE_LM_BUILD/host-libs "${LIBHOST[@]}" --with-gmp=$WORKING_DIR/$NATIVE_LM_BUILD/host-libs --with-ppl=$WORKING_DIR/$NATIVE_LM_BUILD/host-libs --with-cloog=$WORKING_DIR/$NATIVE_LM_BUILD/host-libs "${GCC_CONFIG_FLAGS[@]}" $SUPPORT_CHECKING)
  build_gcc "$WORKING_DIR/$NATIVE_LM_BUILD" "cross" CONFIG_CMD[@]
  copy_gcc_targetlibs "$WORKING_DIR/$NATIVE_NOLM_BUILD" "$WORKING_DIR/$NATIVEIMAGE"
  XCLM_SHASUM_MACRO=""

  # strip native-image
  cd $WORKING_DIR/$NATIVEIMAGE
  if [ "x$NATIVEIMAGE" == "xinstall-Darwin" ] ; then
    find ./bin -type f -perm -g+x -follow | xargs file | grep Mach-O | cut -d: -f1 | xargs $DEVELOPERDIR/usr/bin/strip -S
    find ./pic32mx/bin -type f -perm -g+x -follow | xargs file | grep Mach-O | cut -d: -f1 | xargs $DEVELOPERDIR/usr/bin/strip -S
  else
    find ./bin -type f -perm -g+x -follow | xargs file | grep ELF | cut -d: -f1 | xargs strip --strip-unneeded
    find ./pic32mx/bin -type f -perm -g+x -follow | xargs file | grep ELF | cut -d: -f1 | xargs strip --strip-unneeded
  fi
  
  cd $WORKING_DIR/$NATIVEIMAGE
  cp ./xclm/install/client/bin/xclm ./bin/xclm

fi # SKIPREBUILD

cd $WORKING_DIR


###### End Building licensed native #########

fi # SKIPNATIVE with LM

unset CC
unset CPP
unset CPPCXX
unset CXX
unset LD
unset AR
unset LDFLAGS

####################################################################
# Begin building Linux cross
####################################################################

LOGTASKFILE="$WORKING_DIR/log-build-pic32m-linux-binutils.txt"
echo "Open $LOGTASKFILE" >$LOGTASKFILE

# Build linux compiler
if [ "x$LINUX32IMAGE" == "x" ] ; then
  SKIPLINUXBUILD="yes"
fi #SKIP

if [ "x$SKIPREBUILD" == "x" ] ; then

if [ "x$SKIPLINUXBUILD" == "x" ] ; then
 unset CC
 unset CPP
 unset CPPCXX
 unset CXX
 unset AR
 unset LD
 unset LDFLAGS

 echo `date` " Creating linux cross build in $BUILDDIR_LINUX..." >> $LOGFILE
 cd $WORKING_DIR
 ensure_dir build-pic32m-Linux
 cd build-pic32m-Linux
 LINUX_BUILD_DIR=`pwd`

 build_binutils "$BUILDDIR_LINUX" "x" "linux32" "$BUILDMACHINE --host=$LINUX_HOST_PREFIX --target=pic32mx --prefix="$WORKING_DIR/$LINUX32IMAGE" --bindir="$WORKING_DIR/$LINUX32IMAGE/bin/bin" --libexecdir="$WORKING_DIR/$LINUX32IMAGE/bin/bin" --disable-nls --disable-tui --disable-gdbtk --disable-shared --enable-static --disable-threads --disable-bootstrap --with-dwarf2 --enable-multilib --without-newlib --disable-sim --with-lib-path=: --enable-poison-system-directories $LIBHOST --program-prefix=pic32m- --with-bugurl=http://www.microchip.com/support --disable-werror"

 cd $LINUX_BUILD_DIR
 erase_and_make_dir gcc
 cd gcc 
 
 USE_CLOOG="--without-cloog"
 USE_PPL="--without-isl"
 LOGTASKFILE="$WORKING_DIR/log-build-pic32m-linux-gcc.txt"
 echo "Open $LOGTASKFILE" >$LOGTASKFILE

 if [ "x$SHASUM256" != "x" ]; then
   XCLM_SHASUM=`$SHASUM256 $WORKING_DIR/$LINUX32IMAGE/bin/xclm | head -c 64`
   if [ ${#XCLM_SHASUM} != 64 ]; then
       assert_success -1 "ERROR: Failed to calculate SHASUM256 digest for $LINUX32IMAGE/bin/xclm"
   fi
   if [ "x$NATIVEIMAGE" == "x$LINUX32IMAGE" ]; then
     XCLM64_SHASUM=`$SHASUM256 $WORKING_DIR/$NATIVEIMAGE/xclm64/install/client/bin/xclm | head -c 64`
     if [ ${#XCLM64_SHASUM} != 64 ]; then
         assert_success -1 "ERROR: Failed to calculate SHASUM256 digest for 64-bit xclm"
     fi
     XCLM_SHASUM_MACRO="-DMCHP_XCLM_SHA256_DIGEST=${XCLM_SHASUM} -DMCHP_XCLM64_SHA256_DIGEST=${XCLM64_SHASUM} -DMCHP_FXCLM_SHA256_DIGEST=8727ea3da9bdd624fee0130eb6133188719892bcbee7da32606911a8b08a1a8d " 
   else 
     XCLM_SHASUM_MACRO="-DMCHP_XCLM_SHA256_DIGEST=${XCLM_SHASUM} "
   fi
 fi

        CONFIG_CMD=("${ALL_EXES[@]}" $SOURCE_GCC_DIR/gcc/configure ${BUILDMACHINE} --host=${LINUX_HOST_PREFIX} --prefix=$WORKING_DIR/$LINUX32IMAGE --includedir=$BUILDDIR_LINUX/pic32mx/include --bindir=$WORKING_DIR/$LINUX32IMAGE/bin/bin --libexecdir=$WORKING_DIR/$LINUX32IMAGE/bin/bin --with-libelf=$BUILDDIR_LINUX/linux-libs ${USE_CLOOG} ${USE_PPL} --with-gmp=$BUILDDIR_LINUX/linux-libs  CFLAGS=\"${XCLM_SHASUM_MACRO}$CFLAGS\" CXXFLAGS=\"${XCLM_SHASUM_MACRO}$CXXFLAGS\" "${GCC_CONFIG_FLAGS[@]}" $SUPPORT_CHECKING '${LIBHOST[@]}')
        build_gcc "$BUILDDIR_LINUX" "linux32 cross" CONFIG_CMD[@]
        copy_gcc_targetlibs "$WORKING_DIR/$NATIVE_NOLM_BUILD" "$WORKING_DIR/$LINUX32IMAGE"
        XCLM_SHASUM_MACRO=""

 status_update "pic32m Make linux32 Canadian cross build complete"

 unset CC
 unset CPP
 unset CPPCXX
 unset CXX
 unset LDFLAGS

 # strip $LINUX32IMAGE
 cd $WORKING_DIR/$LINUX32IMAGE
 find ./bin -type f -perm -g+x -follow | xargs file | grep ELF | cut -d: -f1 | xargs $LINUX_HOST_PREFIX-strip --strip-unneeded
 find ./pic32mx/bin -type f -perm -g+x -follow | xargs file | grep ELF | cut -d: -f1 | xargs $LINUX_HOST_PREFIX-strip --strip-unneeded

 cd $WORKING_DIR/$LINUX32IMAGE
 cp ./xclm/install/client/bin/xclm ./bin/xclm

 cd $WORKING_DIR

fi # SKIPLINUXBUILD

fi # SKIPREBUILD

# end build linux compiler

############################################################################
# Build mingw32/windows cross compiler
############################################################################

if [ "x$SKIPREBUILD" == "x" ] ; then

if [ "$WINDOWS" = "yes" ]; then
 unset CC_FOR_BUILD
 unset CXX_FOR_BUILD
# set CC="i686-w64-mingw32-gcc"
# set CPP="i686-w64-mingw32-cpp"
# set CPPCXX="i686-w64-mingw32-cpp"
# set CXX="i686-w64-mingw32-g++"
# set AR="i686-w64-mingw32-ar"
# set LD="i686-w64-mingw32-ld"
 unset LDFLAGS
 
  if [[ ! -z "${MINGW_HOST_PREFIX_PIC32M:-}" ]]; then
    MINGW_HOST_PREFIX_OLD=$MINGW_HOST_PREFIX
    MINGW32_HOST_PREFIX_OLD=$MINGW32_HOST_PREFIX
    HOST_MINGW_OLD=$HOST_MINGW
    HOST_MINGW_TOOL_OLD=$HOST_MINGW_TOOL
  
    MINGW_HOST_PREFIX=$MINGW_HOST_PREFIX_PIC32M
    MINGW32_HOST_PREFIX=$MINGW32_HOST_PREFIX_PIC32M
    HOST_MINGW=$HOST_MINGW_PIC32M
    HOST_MINGW_TOOL=$HOST_MINGW_TOOL_PIC32M
  fi

  if [ -z "${HOST_MINGW:-}" ]; then
    HOST_MINGW=i686-w64-mingw32
    HOST_MINGW_TOOL=i686-w64-mingw32
  fi

  if [ -z "${MINGW_HOST_PREFIX:-}" ]; then
    MINGW_HOST_PREFIX=i686-w64-mingw32
  fi

cd $WORKING_DIR
echo `date` " Creating cross build in $WORKING_DIR/$WIN_BUILD..." >> $LOGFILE
status_update "pic32m Creating cross build in $WORKING_DIR/$WIN_BUILD"

if [ ! -e $WIN_BUILD ] ; then
  status_update "WARNING: $WIN_BUILD does not exist"
  mkdir $WIN_BUILD
fi

assert_success $? "ERROR: creating directory $WORKING_DIR/$WIN_BUILD"

  cd $WORKING_DIR

  LOGTASKFILE="$WORKING_DIR/log-build-pic32m-mingw-binutils.txt"
  echo "Open $LOGTASKFILE" >$LOGTASKFILE

  # Configure mingw32-cross binutils
  echo `date` " Configuring win32 binutils build in $WORKING_DIR/$WIN_BUILD..." >> $LOGFILE
  build_binutils "$WIN_BUILD" "x" "windows" "--target=pic32mx --prefix=$INSTALLDIR_MINGW --bindir=$INSTALLDIR_MINGW/bin/bin --libexecdir=$INSTALLDIR_MINGW/bin/bin --host=$MINGW_HOST_PREFIX $BUILDMACHINE --disable-nls --disable-tui --disable-gdbtk --disable-shared --enable-static --disable-threads --disable-bootstrap --with-dwarf2 --enable-multilib --without-newlib --disable-sim --with-lib-path=: --enable-poison-system-directories --program-prefix=pic32m- --with-bugurl=http://www.microchip.com/support --disable-werror"

  cd $ROOT/$WIN_BUILD
  USE_CLOOG="--without-cloog"
  USE_PPL="--without-isl"
  LOGTASKFILE="$WORKING_DIR/log-build-pic32m-mingw-gcc.txt"
  echo "Open $LOGTASKFILE" >$LOGTASKFILE

 if [ "x$SHASUM256" != "x" ]; then
   XCLM_SHASUM=`$SHASUM256 $INSTALLDIR_MINGW/bin/xclm.exe | head -c 64`
   echo ${XCLM_SHASUM} >> $LOGFILE
   echo ${XCLM_SHASUM}
   if [ ${#XCLM_SHASUM} != 64 ]; then
       assert_success -1 "ERROR: Failed to calculate SHASUM256 digest for $INSTALLDIR_MINGW/bin/xclm.exe"
   fi
   XCLM_SHASUM_MACRO="-DMCHP_XCLM_SHA256_DIGEST=${XCLM_SHASUM} "
 fi
  cd $ROOT
  CFLAGS="-O2"
  CXXFLAGS="-O2"
  LIBHOST='--with-host-libstdcxx="-static-libgcc -Wl,-Bstatic,-lwinpthread,-lpthread,-lstdc++,-Bdynamic -lm"'

  CONFIG_CMD=("${ALL_EXES[@]}" $SOURCE_GCC_DIR/gcc/configure $BUILDMACHINE --host=$MINGW_HOST_PREFIX CFLAGS=\"${XCLM_SHASUM_MACRO}$CFLAGS\" CXXFLAGS=\"${XCLM_SHASUM_MACRO}$CXXFLAGS\" --prefix=$INSTALLDIR_MINGW --includedir=$INSTALLDIR_MINGW/pic32mx/include --bindir=$INSTALLDIR_MINGW/bin/bin --libexecdir=$INSTALLDIR_MINGW/bin/bin --disable-shared --with-libelf=$ROOT/$WIN_BUILD/host-libs --with-gmp=$ROOT/$WIN_BUILD/host-libs ${USE_PPL} ${USE_CLOOG} "${GCC_CONFIG_FLAGS[@]}" $SUPPORT_CHECKING --enable-poison-system-directories LDFLAGS=-static \'${LIBHOST[@]}\') 
  build_gcc "$WIN_BUILD" "cross" CONFIG_CMD[@]
  copy_gcc_targetlibs "$WORKING_DIR/$NATIVE_NOLM_BUILD" "$INSTALLDIR_MINGW"
  XCLM_SHASUM_MACRO=""

  rm $INSTALLDIR_MINGW/bin/xclm.exe

  # strip $INSTALLDIR_MINGW
  cd $INSTALLDIR_MINGW
  find ./bin -type f -name "*.exe" | xargs $MINGW_HOST_PREFIX-strip --strip-unneeded
  find ./pic32mx/bin -type f -name "*.exe" | xargs $MINGW_HOST_PREFIX-strip --strip-unneeded

  cd $WORKING_DIR
  
  if [ -e /usr/i686-w64-mingw32/lib/libwinpthread-1.dll ] ; then
    cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll $INSTALLDIR_MINGW/bin/
    cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll $INSTALLDIR_MINGW/bin/bin/
    cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll $INSTALLDIR_MINGW/lib/gcc/pic32mx/4.8.3/
    cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll $INSTALLDIR_MINGW/pic32mx/bin
  fi

  unset CC
  unset CPP
  unset CPPCXX
  unset CXX
  unset AR
  unset LD
  unset LDFLAGS

  status_update "pic32m Make minGW32 Canadian cross build complete"
fi # WINDOWS

fi # SKIPREBUILD

cd $WORKING_DIR

####
# Install the examples
####

##################   END HERE FOR NOW!!!!! ################################

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
unset CPPCXX
unset CXX
unset LD
unset AR
unset LDFLAGS
unset ABI
PATH=$OLDPATH

echo `date` "Completed build-pic32m-toolchain.sh" >> $LOGFILE
status_update "Completed build-pic32m-toolchain.sh"

fossil settings autosync on --global

exit 0

###########################################################################


LOGTASKFILE="$WORKING_DIR/log-build-pic32m-install-others.txt"
echo "Open $LOGTASKFILE" >$LOGTASKFILE

copy_dir_clean "examples" "$WORKING_DIR/$NATIVEIMAGE/examples"

#
# Remove all the scripts (sub)directories from the HPMD examples
#
for x in `find $WORKING_DIR/$NATIVEIMAGE/examples -name scripts -print`
do
  rm -rf $x
done

erase_and_make_dir "$INSTALLDIR_MINGW/examples"
copy_dir_clean "$WORKING_DIR/$NATIVEIMAGE/examples" "$INSTALLDIR_MINGW/examples"

erase_and_make_dir "$WORKING_DIR/export-image/examples"
copy_dir_clean "$WORKING_DIR/$NATIVEIMAGE/examples" "$WORKING_DIR/export-image/examples"

if [ "x$LINUX32IMAGE" != "x" ]; then
  erase_and_make_dir "$WORKING_DIR/$LINUX32IMAGE/examples"
  copy_dir_clean "$WORKING_DIR/$NATIVEIMAGE/examples" "$WORKING_DIR/$LINUX32IMAGE/examples"
fi

####
# Install the documentation
#
####
echo `date` " Installing documentation to $WORKING_DIR/$NATIVEIMAGE/docs..." >> $LOGFILE

erase_and_make_dir "$WORKING_DIR/$NATIVEIMAGE/docs"

# Copy from the fossil checkout to get the compiler docs
copy_dir_clean "$WORKING_DIR/docs" "$WORKING_DIR/$NATIVEIMAGE/docs"
assert_success $? "ERROR: Exporting fossil "

erase_and_make_dir "$INSTALLDIR_MINGW/docs"
copy_dir_clean "$WORKING_DIR/docs" "$INSTALLDIR_MINGW/docs"

erase_and_make_dir "$WORKING_DIR/export-image/docs"
copy_dir_clean "$WORKING_DIR/docs" "$WORKING_DIR/export-image/docs"

if [ "x$LINUX32IMAGE" != "x" ]; then
  erase_and_make_dir "$WORKING_DIR/$LINUX32IMAGE/docs"
  copy_dir_clean "$WORKING_DIR/docs" "$WORKING_DIR/$LINUX32IMAGE/docs"
fi

cd $WORKING_DIR
find . -name "config.mif" -exec rm -rf \{\} \;

###
# In the resultant install directory, there are a few extra directories
# that we don"t want for our Windows build.
###
echo `date` " Removing unecessary directories from build..." >> $LOGFILE
echo "Directory clean up for pic-tools"

if [ "$WINDOWS" = "yes" ] ; then
 rm -rf $INSTALLDIR_MINGW/include
 rm -rf $INSTALLDIR_MINGW/man
 rm -rf $INSTALLDIR_MINGW/info
 rm -rf $INSTALLDIR_MINGW/share
 rm -rf $INSTALLDIR_MINGW/libsrc
 rm -rf $INSTALLDIR_MINGW/pic32mx/include/c++
 rm -rf $INSTALLDIR_MINGW/pic32mx/share
 find $INSTALLDIR_MINGW -name "*.py" -delete
 find $INSTALLDIR_MINGW -name "*\**" -delete
fi

rm -rf $WORKING_DIR/$NATIVEIMAGE/include
rm -rf $WORKING_DIR/$NATIVEIMAGE/man
rm -rf $WORKING_DIR/$NATIVEIMAGE/info
rm -rf $WORKING_DIR/$NATIVEIMAGE/share
rm -rf $WORKING_DIR/$NATIVEIMAGE/libsrc
rm -rf $WORKING_DIR/$NATIVEIMAGE/pic32mx/include/c++
rm -rf $WORKING_DIR/$NATIVEIMAGE/pic32mx/share
find $WORKING_DIR/$NATIVEIMAGE -name "*.py" -delete
find $WORKING_DIR/$NATIVEIMAGE -name "*\**" -delete

rm -rf $WORKING_DIR/export-image/man
rm -rf $WORKING_DIR/export-image/info
rm -rf $WORKING_DIR/export-image/share
rm -rf $WORKING_DIR/export-image/libsrc
rm -rf $WORKING_DIR/export-image/pic32mx/include/c++
rm -rf $WORKING_DIR/export-image/pic32mx/share

if [ "x$LINUX32IMAGE" != "x" ] ; then
 rm -rf $WORKING_DIR/$LINUX32IMAGE/include
 rm -rf $WORKING_DIR/$LINUX32IMAGE/man
 rm -rf $WORKING_DIR/$LINUX32IMAGE/info
 rm -rf $WORKING_DIR/$LINUX32IMAGE/share
 rm -rf $WORKING_DIR/$LINUX32IMAGE/libsrc
 rm -rf $WORKING_DIR/$LINUX32IMAGE/pic32mx/include/c++
 rm -rf $WORKING_DIR/$LINUX32IMAGE/pic32mx/share
 find $WORKING_DIR/$LINUX32IMAGE -name "*.py" -delete
 find $WORKING_DIR/$LINUX32IMAGE -name "*\**" -delete
fi

echo "Making tar files"
# Tar installation directory.

echo `date` " Tar components to $WORKING_DIR/zips directory..." >> $LOGFILE
echo `date` " Tar installation directory..." >> $LOGFILE
cd $WORKING_DIR
if [[ ! -e zips ]] ; then
 mkdir zips
fi

# ZIP library source

echo `date` " ZIP library source..." >> $LOGFILE
cd $WORKING_DIR
rm -rf export
mkdir export
cd export

echo Exporting pic32-libs...
echo `date` " Copy pic32-libs to export/pic32-libs..." >> $LOGFILE

copy_dir_clean "$WORKING_DIR/pic32-libs/" "$WORKING_DIR/export/pic32-libs/"
assert_success $? "ERROR: Copy pic32-libs to export/pic32-libs failed"

echo `date` " Removing MTK source directories..." >> $LOGFILE
echo Removing MTK source directories...
cd $WORKING_DIR/export/pic32-libs
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
    echo `date` " Removing MTK directory $x..." >> $LOGFILE
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

cd $WORKING_DIR/export

cd pic32-libs
find . -name "OBJ" -exec rm -rf \{\} \;
assert_success $? "Remove OBJ object files from pic32-libs source directory"
find . -name "*.o" -delete
assert_success $? "Remove *.o object files from pic32-libs source directory"
find . -name "*.a" -delete
assert_success $? "Remove *.a archive files from pic32-libs source directory"
cd ..

copy_dir_clean "pic32-libs/" "$INSTALLDIR_MINGW/pic32-libs/"
assert_success $? "copy lib source to $INSTALLDIR_MINGW"
copy_dir_clean "pic32-libs/" "$WORKING_DIR/$NATIVEIMAGE/pic32-libs/"
assert_success $? "copy lib source to $NATIVEIMAGE"
copy_dir_clean "pic32-libs/" "$WORKING_DIR/export-image/pic32-libs/"
assert_success $? "copy lib source to export-image"
if [ "x$LINUX32IMAGE" != "x" ]; then
 copy_dir_clean "pic32-libs/" "$WORKING_DIR/$LINUX32IMAGE/pic32-libs/"
 assert_success $? "copy lib source to $LINUX32IMAGE"
fi

########

if [ "x$SKIPLEGACYPLIB" != "x" ]; then

	cd $WORKING_DIR/..

	echo "Extract the plibs to the install images" >> $LOGFILE
	status_update "pic32m Extract the plibs to the install images"
	SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
	echo "$SCRIPTDIR/plib-to-image.sh -g trunk -D $WORKING_DIR" >> $LOGFILE
	$SCRIPTDIR/plib-to-image.sh -g"trunk" -D"$WORKING_DIR"

	assert_success $? "plib-to-image"

fi

cd $WORKING_DIR
########

# Check the install images against the manifest files
MANIFEST_CHECK_ARGS=""
if [ "x$SKIPLINUXBUILD" == "xyes" ] ; then
  MANIFEST_CHECK_ARGS+=" -l"
fi
if [ "x$WINDOWS" == "xno" ] ; then
  MANIFEST_CHECK_ARGS+=" -w"
fi
if [ "x$NATIVE_IMAGE" != "xinstall-Darwin" ] ; then
  MANIFEST_CHECK_ARGS+=" -d"
fi

#echo "$SCRIPTDIR/pic32-manifest-check.sh $MANIFEST_CHECK_ARGS" >> $LOGFILE
#$SCRIPTDIR/pic32-manifest-check.sh $MANIFEST_CHECK_ARGS
#assert_success $? "pic32-manifest-check"

########

# Create MD5SUM on install image files so that we can better sanity check the installer
cd $WORKING_DIR
# Determine location of the md5sum utility
if [ "x$MD5SUM" == "x" ]; then
  echo "MD5SUM generation" >> $LOGFILE
  MD5SUM_LOC=`which md5sum`
  if [ "x$MD5SUM_LOC" != "x" ] ; then
    MD5SUM=md5sum
  else
    # Try the default macport coreutils location
    if [ -e /opt/local/libexec/gnubin/md5sum ] ; then
      MD5SUM=/opt/local/libexec/gnubin/md5sum
    else
      MD5SUM=""
    fi
  fi
  unset MINGW_GCC
fi

if [ "x$MD5SUM" != "x" ] ; then
 if [ "$WINDOWS" = "yes" ] ; then
  echo "Generating md5sum for $INSTALLDIR_MINGW" >> $LOGFILE
  echo "Generating md5sum for $INSTALLDIR_MINGW"
  cd $INSTALLDIR_MINGW
  find . -type f -exec $MD5SUM {} \; > ../zips/mingw.md5
 fi
  echo "Generating md5sum for $NATIVEIMAGE" >> $LOGFILE
  echo "Generating md5sum for $NATIVEIMAGE"
  cd $WORKING_DIR/$NATIVEIMAGE
  find . -type f -exec $MD5SUM {} \; > ../zips/$NATIVEIMAGE.md5
  if [ "x$LINUX32IMAGE" != "x" ]; then
    echo "Generating md5sum for $LINUX32IMAGE" >> $LOGFILE
    echo "Generating md5sum for $LINUX32IMAGE"
    cd $WORKING_DIR/$LINUX32IMAGE
    find . -type f -exec $MD5SUM {} \; > ../zips/$LINUX32IMAGE.md5
  fi
fi

###########

cd $WORKING_DIR
REV=${BUILD##pic32-}

echo "copy_dir_clean $SOURCE_GCC_DIR ./xc32-tools-$REV-src" >> $LOGFILE
echo "copy_dir_clean $SOURCE_GCC_DIR ./xc32-tools-$REV-src"

if [ -e source-archive ] ; then
  rm -rf source-archive
fi
mkdir source-archive
assert_success $? "mkdir source-archive"
cd source-archive
copy_dir_clean $SOURCE_GCC_DIR ./xc32-tools-$REV-src
assert_success $? "copy_dir_clean $SOURCE_GCC_DIR ./xc32-tools-$REV-src"

if [ "x$SKIPCREATETAR" == "x" ]; then

 echo "Generating tar for xc32-tools-$REV-src" >> $LOGFILE
 echo "Generating tar for xc32-tools-$REV-src"
 tar cjf $WORKING_DIR/zips/xc32-tools-$REV-src.tar.bz2 xc32-tools-$REV-src


###########

 cd $WORKING_DIR

 if [ "$WINDOWS" = "yes" ] ; then
  echo "Generating tar for Windows" >> $LOGFILE
  echo "Generating tar for Windows"
  tar cjf $WORKING_DIR/zips/xc32-tools-$REV-Windows.tar.bz2 install-mingw
  assert_success $? "tar mingw"
 fi
 echo "Generating tar for $NATIVEIMAGE" >> $LOGFILE
 echo "Generating tar for $NATIVEIMAGE"
 tar cjf $WORKING_DIR/zips/xc32-tools-$REV-$NATIVEIMAGE.tar.bz2 $NATIVEIMAGE
  assert_success $? "tar $NATIVEIMAGE"
 if [ "x$LINUX32IMAGE" != "x" ]; then
  echo "Generating tar for $LINUX32IMAGE" >> $LOGFILE
  echo "Generating tar for $LINUX32IMAGE"
  tar cjf $WORKING_DIR/zips/xc32-tools-$REV-$LINUX32IMAGE.tar.bz2 $LINUX32IMAGE
 assert_success $? "tar $LINUX32IMAGE"
 fi

fi #SKIPCREATETAR

cd $WORKING_DIR/..

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
unset CPPCXX
unset CXX
unset LD
unset AR
unset LDFLAGS
unset ABI
PATH=$OLDPATH

echo `date` "Completed build-pic32m-toolchain.sh" >> $LOGFILE
status_update "Completed build-pic32m-toolchain.sh"

exit 0
