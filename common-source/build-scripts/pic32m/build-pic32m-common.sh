# Copyright (c) 2011-2015, ARM Limited
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
#     * Redistributions of source code must retain the above copyright notice,
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of ARM nor the names of its contributors may be used
#       to endorse or promote products derived from this software without
#       specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

OLDPATH=$PATH

error () {
    set +u
    echo "$0: error: $@" >&2
    exit 1
    
}

warning () {
    set +u
    echo "$0: warning: $@" >&2
    
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
  fossil settings autosync on --global
  exit 1
}

function status_update ()
{
 set +u
 local MESSAGE=$1
  if [ ! -z "${NOTIFY_SEND:-}" ] ; then
   $NOTIFY_SEND "$MESSAGE" "xc32++-build.sh"
  elif [ ! -z "${GROWL_SEND:-}" ] ; then
    $GROWL_SEND -p low -n build-pic32m -m "$MESSAGE"
  fi
  echo `date` "status: $MESSAGE" | tee -a $LOGFILE
  
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
    $GROWL_SEND -p high -n build-pic32m -sm "XC32 Build Error: $MESSAGE"
  fi
  echo "NOTIFY: $MESSAGE ($RESULT)" | tee -a $LOGFILE
  echo "$'\e]9;$MESSAGE ($RESULT)\007'"

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


copy_dir() {
    set +u
    mkdir -p "$2"

    (cd "$1" && tar cf - .) | (cd "$2" && tar xf -)
    
}

copy_dir_clean() {
    set +u
    mkdir -p "$2"
    (cd "$1" && tar cf - \
	--exclude=CVS --exclude=.svn --exclude=.git --exclude=.pc \
	--exclude="*~" --exclude=".#*" --exclude="*.bak" \
	--exclude="*.orig" --exclude="*.rej" --exclude="*.reg" \
	--exclude="*.fsl" --exclude="*.fossil" --exclude=".fslckout" \
	--exclude="OBJ" --exclude="*.o" --exclude="*.out" \
	--exclude="*.elf" --exclude="*.a" \
	.) | (cd "$2" && tar xf -)
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
	--exclude="*.orig" --exclude="*.rej" $4 $5 $6 $7 $8 $9 ${10} \
	-C $1 $2
    
}

# Clean all global shell variables except for those needed by build scripts
clean_env () {
    set +u
    local var_list
    local var
    var_list=`export|grep "^declare -x"|sed -e "s/declare -x //"|cut -d"=" -f1`
    
    for var in $var_list ; do
        case $var in
            BUILD_NUMBER|\
            DEJAGNU|\
            DISPLAY|\
            HOME|\
            LD_LIBRARY_PATH|\
            LOGNAME|\
            PATH|\
            PWD|\
            SHELL|\
            SHLVL|\
            TERM|\
            USER|\
            USERNAME|\
            com.apple.*|\
            XAUTHORITY|\
            MINGW_HOST_PREFIX|\
            MINGW32_HOST_PREFIX|\
            MINGW_HOST_PREFIX_PIC32M|\
            MINGW32_HOST_PREFIX_PIC32M|\
            HOST_MINGW_PIC32M|\
            HOST_MINGW_TOOL_PIC32M|\
            HOST_MINGW|\
            HOST_MINGW_TOOL|\
            FOSSIL_CLONES|\
            FOSSIL_USERNAME|\
            FOSSIL_PASSWORD|\
            FOSSIL_USER|\
            FOSSIL_LOGIN|\
            FOSSIL_SERVER|\
            INSTALLED_FOSSIL|\
            FOSSIL_BINARY|\
            NOTIFY_SEND|\
            GROWL_SEND|\
            MCHP_VERSION|\
            MCHP_RESOURCE|\
            DYLD_LIBRARY_PATH|\
            bamboo_XC32_XCLM_BRANCH|\
            bamboo_XC32_FULL_VERSION|\
            bamboo_buildKey|\
            bamboo_XC32_FULL_RESOURCE|\
            bamboo_XC32_XCLM_BRANCH)
            ;;
        *)
            unset $var
            ;;
        esac
    done

    export LANG=C
}

stack_level=0

# Start a new stack level to save variables
# Must call this before saving any variables
saveenv () {
    set +u
    # Force expr return 0 to avoid script fail
    stack_level=`expr $stack_level \+ 1 || true`
    eval stack_list_$stack_level=
    
}

# Save a variable to current stack level, and set new value to this var.
# If a variable has been saved, won't save it. Just set new value
# Must be called when stack_level > 0
# $1: variable name
# $2: new variable value
saveenvvar () {
    set +u
    if [ $stack_level -le 0 ]; then
        error Must call saveenv before calling saveenvvar
    fi
    local varname="$1"
    local newval="$2"
    eval local oldval=\"\${$varname}\"
    eval local saved=\"\${level_saved_${stack_level}_${varname}}\"
    if [ "x$saved" = "x" ]; then
        # The variable wasn't saved in the level before. Save it
        eval local temp=\"\${stack_list_$stack_level}\"
        eval stack_list_$stack_level=\"$varname $temp\"
        eval save_level_${stack_level}_$varname=\"$oldval\"
        eval level_saved_${stack_level}_$varname="yes"
        eval level_preset_${stack_level}_${varname}=\"\${$varname+set}\"
        #echo Save $varname: \"$oldval\"
    fi
    eval export $varname=\"$newval\"
    #echo $varname set to \"$newval\"
    
}

# Restore all variables that have been saved in current stack level
restoreenv () {
    set +u
    if [ $stack_level -le 0 ]; then
        error "Trying to restore from an empty stack"
    fi

    eval local list=\"\${stack_list_$stack_level}\"
    local varname
    for varname in $list; do
        eval local varname_preset=\"\${level_preset_${stack_level}_${varname}}\"
        if [ "x$varname_preset" = "xset" ] ; then
            eval $varname=\"\${save_level_${stack_level}_$varname}\"
        else
            unset $varname
        fi
        eval level_saved_${stack_level}_$varname=
        # eval echo $varname restore to \\\"\"\${$varname}\"\\\"
    done
    # Force expr return 0 to avoid script fail
    stack_level=`expr $stack_level \- 1 || true`
    
}

prependenvvar() {
    set +u
    eval local oldval=\"\$$1\"
    saveenvvar "$1" "$2$oldval"
    
}

prepend_path() {
    set +u
    eval local old_path="\"\$$1\""
    if [ x"$old_path" == "x" ]; then
        prependenvvar "$1" "$2"
    else
        prependenvvar "$1" "$2:"
    fi
    
}

# Strip binary files as in "strip binary" form, for both native(linux/mac) and mingw.
strip_binary() {
    set +e
    set +u
    if [[ $# -ne 2 ]]; then
        warning "strip_binary: Missing arguments"
        return 0
    fi
    local strip="$1"
    local bin="$2"

    if file ${bin} | grep -q -E -e '[[:space:]](ELF|PE|PE32)[[:space:]]'; then
        ${strip} ${bin} 2>/dev/null || true
    fi

    #set -e
}

# Copy target libraries from each multilib directories.
# Usage copy_multi_libs dst_prefix=... src_prefix=... target_gcc=...
copy_multi_libs() {
    local -a multilibs
    local multilib
    local multi_dir
    local src_prefix
    local dst_prefix
    local src_dir
    local dst_dir
    local target_gcc

    for arg in "$@" ; do
        eval "${arg// /\\ }"
    done

    multilibs=( $("${target_gcc}" -print-multi-lib 2>/dev/null) )
    for multilib in "${multilibs[@]}" ; do
        multi_dir="${multilib%%;*}"
        src_dir=${src_prefix}/${multi_dir}
        dst_dir=${dst_prefix}/${multi_dir}
        cp -f "${src_dir}/libstdc++.a" "${dst_dir}/libstdc++_nano.a"
        cp -f "${src_dir}/libsupc++.a" "${dst_dir}/libsupc++_nano.a"
        cp -f "${src_dir}/libc.a" "${dst_dir}/libc_nano.a"
        cp -f "${src_dir}/libg.a" "${dst_dir}/libg_nano.a"
        cp -f "${src_dir}/librdimon.a" "${dst_dir}/librdimon_nano.a"
        cp -f "${src_dir}/nano.specs" "${dst_dir}/"
        cp -f "${src_dir}/rdimon.specs" "${dst_dir}/"
        cp -f "${src_dir}/nosys.specs" "${dst_dir}/"
        cp -f "${src_dir}/"*crt0.o "${dst_dir}/"
    done
}

# Clean up unnecessary global shell variables
clean_env

ROOT=`pwd`
SRCDIR=$ROOT/pic32m-source/src48x
BUILD_NUMBER=1.0

# Build variables
NATIVE_NOLM_BUILD=build-pic32m-native-nolm
NATIVE_NOLM_IMAGE=install-native-nolm
NATIVE_LM_BUILD=build-pic32m-native

LINUX_BUILD=build-pic32m-linux32

BUILDDIR_NATIVE_NOLM=$ROOT/$NATIVE_NOLM_BUILD
BUILDDIR_NATIVE=$ROOT/$NATIVE_LM_BUILD
BUILDDIR_MINGW=$ROOT/build-pic32m-mingw
BUILDDIR_LINUX=$ROOT/$LINUX_BUILD

NATIVEIMAGE=install-
NATIVEIMAGE+=`uname`
#if [ "$NATIVEIMAGE" == "install-linux" ] ; then
#  NATIVEIMAGE="install-linux32";
#fi
NATIVE_NOLM_IMAGE=$NATIVEIMAGE
NATIVE_NOLM_IMAGE+="-nolm"

INSTALLDIR_NATIVE_NOLM=$ROOT/$NATIVE_NOLM_IMAGE
INSTALLDIR_NATIVE_NOLM_DOC=$INSTALLDIR_NATIVE_NOLM/share/doc/xc32-pic32m-gcc

INSTALLDIR_NATIVE=$ROOT/$NATIVEIMAGE
INSTALLDIR_NATIVE_DOC=$INSTALLDIR_NATIVE/share/doc/xc32-pic32m-gcc
INSTALLDIR_MINGW=$ROOT/install-mingw
INSTALLDIR_MINGW_DOC=$ROOT/install-mingw/share/doc/xc32-pic32m-gcc

PACKAGEDIR=$ROOT/pkg

BINUTILS=binutils
CLOOG=cloog
EXPAT=expat
GCC=gcc
GDB=gdb
GMP=gmp
NEWLIB_NANO=newlib
SAMPLES=samples
LIBELF=libelf
LIBICONV=libiconv
MPC=mpc
MPFR=mpfr
NEWLIB=newlib
ISL=isl
ZLIB=zlib
INSTALLATION=installation
SAMPLES=samples
BUILD_MANUAL=build-manual

CLOOG_PACK=$CLOOG.tar.gz
EXPAT_PACK=$EXPAT.tar.gz
GMP_PACK=$GMP.tar.bz2
LIBELF_PACK=$LIBELF.tar.gz
LIBICONV_PACK=$LIBICONV.tar.gz
MPC_PACK=$MPC.tar.gz
MPFR_PACK=$MPFR.tar.bz2
ISL_PACK=$ISL.tar.bz2
ZLIB_PACK=$ZLIB.tar.gz

RELEASEDATE=`date +%Y%m%d`
release_year=${RELEASEDATE:0:4}
release_month=${RELEASEDATE:4:2}
case $release_month in
    01|02|03)
        RELEASEVER=${release_year}q1
        ;;
    04|05|06)
        RELEASEVER=${release_year}q2
        ;;
    07|08|09)
        RELEASEVER=${release_year}q3
        ;;
    10|11|12)
        RELEASEVER=${release_year}q4
        ;;
esac

RELEASE_FILE=release.txt
README_FILE=readme.txt
LICENSE_FILE=license.txt
SAMPLES_DOS_FILES=$SAMPLES/readme.txt
BUILD_MANUAL_FILE=How-to-build-toolchain.pdf
GCC_VER=`cat $SRCDIR/$GCC/gcc/BASE-VER`
GCC_VER_NAME=`echo $GCC_VER | cut -d'.' -f1,2 | sed -e 's/\./_/g'`
if [[ $(uname -s) == "Darwin" ]]
then
    SEDOPTION='-E'
else
    SEDOPTION='-r'
fi
GCC_VER_SHORT=`echo $GCC_VER_NAME | sed $SEDOPTION 's/_/\./g'`

TARGET=pic32m
TARGET_DIR=pic32m
ENV_CFLAGS=
ENV_CPPFLAGS=
ENV_CXXFLAGS=
ENV_LDFLAGS=
BINUTILS_CONFIG_OPTS=
GCC_CONFIG_OPTS=
GDB_CONFIG_OPTS=
NEWLIB_CONFIG_OPTS=

PKGVERSION="Microchip Technology"
BUGURL="http://www.microchip.com/support"

# Set variables according to real environment to make this script can run
# on Ubuntu and Mac OS X.
uname_string=`uname | sed 'y/LINUXDARWIN/linuxdarwin/'`
host_arch=`uname -m | sed 'y/XI/xi/'`
if [ "x$uname_string" == "xlinux" ] ; then
    BUILD="$host_arch"-linux-gnu
    HOST_NATIVE="$host_arch"-linux-gnu
    READLINK=readlink
    JOBS=`grep ^processor /proc/cpuinfo|wc -l`
    if [ $JOBS -gt 4 ] ; then
      JOBS=$((JOBS/2))
    fi
    GCC_CONFIG_OPTS_LCPP="--with-host-libstdcxx=-static-libgcc -Wl,-Bstatic,-lstdc++,-Bdynamic -lm"
    TAR=tar
    MD5="md5sum -b"
    PACKAGE_NAME_SUFFIX=linux
elif [ "x$uname_string" == "xdarwin" ] ; then
    BUILD=i386-apple-darwin10
    HOST_NATIVE=i386-apple-darwin10
    READLINK=greadlink
    JOBS=$(sysctl -n hw.physicalcpu )
    if [ $JOBS -gt 3 ] ; then
      JOBS=$((JOBS/2))
    fi
    GCC_CONFIG_OPTS_LCPP="--with-host-libstdcxx=-static-libgcc -Wl,-lstdc++ -lm"
    TAR=gnutar
    MD5="md5 -r"
    PACKAGE_NAME_SUFFIX=mac
    DARWINTARGETARCH="-m32 -arch i386"
    SHASUM256="shasum -a 256 -p"
    if [ -e "/Developer-old" ]
    then
      DEVELOPERDIR="/Developer-old"
    elif [ -e "/Developer" ]
    then
      DEVELOPERDIR="/Developer"
    else
      unset DEVELOPERDIR
 fi
else
    error "Unsupported build system : $uname_string"
fi

OBJ_SUFFIX_MINGW=$TARGET-$RELEASEDATE-$HOST_MINGW
OBJ_SUFFIX_NATIVE=$TARGET-$RELEASEDATE-$HOST_NATIVE
PACKAGE_NAME=gcc-$TARGET-$GCC_VER_NAME-$RELEASEVER-$RELEASEDATE
PACKAGE_NAME_NATIVE=$PACKAGE_NAME-$PACKAGE_NAME_SUFFIX
PACKAGE_NAME_MINGW=$PACKAGE_NAME-win32
INSTALL_PACKAGE_NAME=pic32m-mchp
INSTALLBASE="PIC32M"
APPNAME="$PKGVERSION $GCC_VER_SHORT $release_year"

