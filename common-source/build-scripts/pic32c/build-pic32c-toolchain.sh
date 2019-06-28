#! /usr/bin/env bash
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

#set -e
#set -x
#set -u
#set -o pipefail

#umask 022

#exec < /dev/null

WORKING_DIR=`pwd`
LOGFILE=$WORKING_DIR/build-pic32c.log

script_path=`cd $(dirname $0) && pwd -P`
. $script_path/build-pic32c-common.sh

status_update "BEGINNING build-pic32c-toolchain.sh"

# This file contains the sequence of commands used to build the ARM EABI toolchain.
usage ()
{
cat<<EOF
Usage: $0 [--build_type=...] [--build_tools=...] [--skip_steps=...] [--jobs=...]

This script will build gcc arm embedded toolchain.

OPTIONS:
  --build_type=TYPE     specify build type to either ppa or native.
                        If followed by keyword debug, the produced binaries
                        will be debuggable.  The default case will be
                        non-debug native build. If keyword devel is given, the
                        _BUILD_MCHP_DEVEL_ flag will be set.

                        Example usages are as:
                        --build_type=native,devel
                        --build_type=ppa
                        --build_type=native,debug,devel
                        --build_type=ppa,debug

  --build_tools=TOOLS   specify where to find the native build tools that
                        will be used for building gcc arm embedded toolchain
                        and related dependent libraries.  If not specified,
                        the ones in your system will be used.

                        The prebuilt ones provided by arm embedded toolchain
                        team are supposed to run on 32bit build platform, thus
                        not suitable for 64bit platform.

  --skip_steps=STEPS    specify which build steps you want to skip.  Concatenate
                        them with comma for skipping more than one steps.  Available
                        steps are: gdb-with-python, mingw32, mingw32-gdb-with-python,
                        native, manual, and config.

  --jobs=N              specify the -j argument for make commands. 
EOF
}

if [ $# -gt 4 ] ; then
    usage
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

function set_native_tools ()
{

LINUX_HOST_PREFIX=i386-linux
LINUX32IMAGE=""

if [ "x$NATIVEIMAGE" == "xinstall-Darwin" ]
then
 if [ "x$LINUX" == "xyes" ]; then
   LINUX32IMAGE="install-linux"
 fi

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

elif [ "x$NATIVEIMAGE" == "x$LINUX32IMAGE" ]; then
 HOSTMACHINE=""
 BUILDMACHINE="--build=x86_64-pc-linux-gnu"
 LIBHOST='--with-host-libstdcxx="-static-libgcc -static-libstdc++ -Wl,-Bstatic,-lstdc++,-Bdynamic,-lm"'

 export ABI=32
 SHASUM256="shasum -a 256 -p"
 if [ "x$NUMBER_OF_JOBS" == "x" ] ; then
   let NUM_USE_CPU=$(awk '/^processor/ {++n} END {print n}' /proc/cpuinfo)
   # Don't use too many jobs
   if [ $NUM_USE_CPU -gt 4 ] ; then
      let NUM_USE_CPU=$NUM_USE_CPU/2
   fi
   NUMBER_OF_JOBS="-j$NUM_USE_CPU"
 fi
else

 LINUX32IMAGE=""
 HOSTMACHINE=""
 BUILDMACHINE=""
 LIBHOST='--with-host-libstdcxx="-static-libgcc -Wl,-Bstatic,-lstdc++,-Bdynamic,-lm"'
 NUMBER_OF_JOBS=$NUMBER_OF_JOBS
 export ABI=32
 SHASUM256="shasum -a 256 -p"
fi # NATIVEIMAGE

echo "HOSTMACHINE=$HOSTMACHINE" >> $LOGFILE
echo "LINUX32IMAGE=$LINUX32IMAGE" >> $LOGFILE
echo "BUILDMACHINE=$BUILDMACHINE" >> $LOGFILE
echo "LIBHOST=$LIBHOST" >> $LOGFILE
echo "NUMBER_OF_JOBS=$NUMBER_OF_JOBS" >> $LOGFILE
echo "WORKING_DIR=$WORKING_DIR" >> $LOGFILE
}

skip_mingw32=no
DEBUG_BUILD_OPTIONS=
is_ppa_release=no
is_native_build=yes
is_devel=no
skip_manual=yes
skip_steps=
skip_gdb_with_python=yes
skip_mingw32_gdb_with_python=yes
skip_config=no
build_type=
build_tools=

#MULTILIB_LIST="--with-multilib-list=rmprofile"
MULTILIB_LIST="--with-multilib-list=xc32cprofile"

for ac_arg; do
    case $ac_arg in
      --skip_steps=*)
	      skip_steps=`echo $ac_arg | sed -e "s/--skip_steps=//g" -e "s/,/ /g"`
	      ;;
      --build_type=*)
	      build_type=`echo $ac_arg | sed -e "s/--build_type=//g" -e "s/,/ /g"`
	      ;;
      --build_tools=*)
	      build_tools=`echo $ac_arg | sed -e "s/--build_tools=//g"`
	      build_tools_abs_path=`cd $build_tools && pwd -P`
	      if [ -d $build_tools_abs_path ]; then
	        if [ -d $build_tools_abs_path/gcc ]; then
	          export PATH=$build_tools_abs_path/gcc/bin:$PATH
	        fi
		if [ -d $build_tools_abs_path/mingw-w64-gcc ]; then
		  export PATH=$build_tools_abs_path/mingw-w64-gcc/bin:$PATH
		fi
		if [ -d $build_tools_abs_path/installjammer ]; then
		  export PATH=$build_tools_abs_path/installjammer:$PATH
		fi
		if [ -d $build_tools_abs_path/nsis ]; then
		  export PATH=$build_tools_abs_path/nsis:$PATH
		fi
		if [ -d $build_tools_abs_path/python ]; then
		  export PATH=$build_tools_abs_path/python/bin:$PATH
		  export LD_LIBRARY_PATH="$build_tools_abs_path/python/lib"
		  export PYTHONHOME="$build_tools_abs_path/python"
		fi
	      else
	        echo "The specified folder of build tools don't exist: $build_tools" 1>&2
		exit 1
	      fi
	      ;;
        --jobs=*)
            JOBS=`echo $ac_arg | sed -e "s/--jobs=//g"`
            ;;
      *)
        usage
        exit 1
        ;;
    esac
done

if [ "x$build_type" != "x" ]; then
  for bt in $build_type; do
    case $bt in
      ppa)
        is_ppa_release=yes
        is_native_build=no
        skip_gdb_with_python=yes
        ;;
      native)
        is_native_build=yes
        is_ppa_release=no
        ;;
      debug)
        DEBUG_BUILD_OPTIONS=" -O0 -g "
        ;;
      devel)
        is_devel=yes
        ;;
      *)
        echo "Unknown build type: $bt" 1>&2
        usage
        exit 1
        ;;
    esac
  done
else
  is_ppa_release=no
  is_native_build=yes
fi

if [ "x$skip_steps" != "x" ]; then
	for ss in $skip_steps; do
		case $ss in
		    manual)
                      status_update "skip_manual=yes"
                      skip_manual=yes
                      ;;
		    gdb-with-python)
                      status_update "skip_gdb_with_python=yes"
                      skip_gdb_with_python=yes
                      ;;
		    mingw32)
                      status_update "skip_mingw32=yes, skip_mingw32_gdb_with_python=yes"
                      skip_mingw32=yes
                      skip_mingw32_gdb_with_python=yes
                      ;;
		    mingw32-gdb-with-python)
                      status_update "skip_mingw32_gdb_with_python=yes"
                      skip_mingw32_gdb_with_python=yes
                      ;;
		    native)
                      status_update "skip_native=yes"
                      skip_native=yes
                      ;;
		    config)
                      status_update "skip_config=yes"
                      skip_config=yes
                      ;;
		    *)
                      echo "Unknown build steps: $ss" 1>&2
                      usage
                      exit 1
                      ;;
		esac
	done
fi

if [ "x$BUILD" == "xx86_64-apple-darwin10" ] || [ "x$is_ppa_release" == "xyes" ] || [ "x$uname_string" == "xdarwin" ] ; then
    skip_mingw32=yes
    skip_mingw32_gdb_with_python=yes    
fi

if [[ "$is_devel" == "yes" ]]; then
  DEVEL_CFLAGS=" -D_BUILD_MCHP_DEVEL_=1 -O0 -g2 "
fi

set_native_tools

#Building mingw gdb with python support requires python windows package and
#a special config file. If any of them is missing, we skip the build of
#mingw gdb with python support.
if [ "x$build_tools" == "x" ]; then
    if [ ! -d $SRCDIR/$PYTHON_WIN ] \
	 || [ ! -x $script_path/python-config.sh ]; then
	skip_mingw32_gdb_with_python=yes
    fi
else
    if [ ! -d $build_tools_abs_path/python-win ] \
	 || [ ! -x $build_tools_abs_path/python-config.sh ]; then
	skip_mingw32_gdb_with_python=yes
    fi
fi

if [ "x$is_ppa_release" != "xyes" ]; then
  ENV_CFLAGS=" -I$BUILDDIR_NATIVE/host-libs/zlib/include -O2 -Wno-error "
  ENV_CPPFLAGS=" -I$BUILDDIR_NATIVE/host-libs/zlib/include "
  ENV_LDFLAGS=" -L$BUILDDIR_NATIVE/host-libs/zlib/lib
                -L$BUILDDIR_NATIVE/host-libs/usr/lib "

  if [ "x$build_tools" != "x" ] && [ -d $build_tools_abs_path/python ]; then
    ENV_LDFLAGS+=" -L$build_tools_abs_path/python/lib "
  fi

  GCC_CONFIG_OPTS=" --build=$BUILD --host=$HOST_NATIVE
                    --with-gmp=$BUILDDIR_NATIVE/host-libs/usr
                    --with-mpfr=$BUILDDIR_NATIVE/host-libs/usr
                    --with-mpc=$BUILDDIR_NATIVE/host-libs/usr
                    --with-isl=$BUILDDIR_NATIVE/host-libs/usr
                    --with-libelf=$BUILDDIR_NATIVE/host-libs/usr "

  BINUTILS_CONFIG_OPTS=" --build=$BUILD --host=$HOST_NATIVE "

  NEWLIB_CONFIG_OPTS=" --build=$BUILD --host=$HOST_NATIVE "

  GDB_CONFIG_OPTS=" --build=$BUILD --host=$HOST_NATIVE
                    --with-libexpat-prefix=$BUILDDIR_NATIVE/host-libs/usr "
fi

mkdir -p $BUILDDIR_NATIVE
if [ "x$skip_mingw32" != "xyes" ] ; then
mkdir -p $BUILDDIR_MINGW
fi
rm -rf $PACKAGEDIR && mkdir -p $PACKAGEDIR

cd $SRCDIR

if [ "x$skip_native" != "xyes" ] ; then

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-0.txt
status_update "  Task [III-0] /$HOST_NATIVE/binutils/"
echo Task [III-0] /$HOST_NATIVE/binutils/
rm -rf $BUILDDIR_NATIVE/binutils && mkdir -p $BUILDDIR_NATIVE/binutils
pushd $BUILDDIR_NATIVE/binutils
saveenv
saveenvvar CFLAGS "$ENV_CFLAGS"
saveenvvar CPPFLAGS "$ENV_CPPFLAGS"
saveenvvar LDFLAGS "$ENV_LDFLAGS"
if [[ "$skip_config" != "yes" ]]; then
    echo $SRCDIR/$BINUTILS/configure  \
         ${BINUTILS_CONFIG_OPTS} \
         --target=$TARGET \
         --prefix=$INSTALLDIR_NATIVE_NOLM \
         --bindir=$INSTALLDIR_NATIVE_NOLM/bin/bin \
         --infodir=$INSTALLDIR_NATIVE_NOLM_DOC/info \
         --mandir=$INSTALLDIR_NATIVE_NOLM_DOC/man \
         --htmldir=$INSTALLDIR_NATIVE_NOLM_DOC/html \
         --pdfdir=$INSTALLDIR_NATIVE_NOLM_DOC/pdf \
         --disable-nls \
         --disable-werror \
         --disable-sim \
         --disable-gdb \
         --enable-interwork \
         --enable-plugins \
         --with-sysroot=$INSTALLDIR_NATIVE_NOLM/$TARGET_DIR \
         "--with-pkgversion=$PKGVERSION" >$LOGTASKFILE
    $SRCDIR/$BINUTILS/configure  \
        ${BINUTILS_CONFIG_OPTS} \
        --target=$TARGET \
        --prefix=$INSTALLDIR_NATIVE_NOLM \
        --bindir=$INSTALLDIR_NATIVE_NOLM/bin/bin \
        --infodir=$INSTALLDIR_NATIVE_NOLM_DOC/info \
        --mandir=$INSTALLDIR_NATIVE_NOLM_DOC/man \
        --htmldir=$INSTALLDIR_NATIVE_NOLM_DOC/html \
        --pdfdir=$INSTALLDIR_NATIVE_NOLM_DOC/pdf \
        --disable-nls \
        --disable-werror \
        --disable-sim \
        --disable-gdb \
        --enable-interwork \
        --enable-plugins \
        --with-sysroot=$INSTALLDIR_NATIVE_NOLM/$TARGET_DIR \
        "--with-pkgversion=$PKGVERSION" >$LOGTASKFILE 2>&1
    assert_success $? "Task [III-0] /$HOST_NATIVE/binutils : configure"
fi

if [ "x$DEBUG_BUILD_OPTIONS" != "x" ] ; then
    make CFLAGS="-I$BUILDDIR_NATIVE/host-libs/zlib/include $DEBUG_BUILD_OPTIONS" -j$JOBS >>$LOGTASKFILE 2>&1
else
    make -j$JOBS >>$LOGTASKFILE 2>&1
fi
assert_success $? "Task [III-0] /$HOST_NATIVE/binutils : make"

make install

# Install to $INSTALLDIR_NATIVE. This combination of flags seems to work
# for the binutils Makefile's weird path configuration, which is set
# up for DESTDIR="", prefix=$INSTALLDIR_NATIVE_NOLM, bindir=$INSTALLDIR_NATIVE_NOLM/bin/bin.
make DESTDIR="$INSTALLDIR_NATIVE/" bindir=bin/bin prefix="" install

assert_success $? "Task [III-0] /$HOST_NATIVE/binutils : install"

if [ "x$skip_manual" != "xyes" ]; then
	make install-html install-pdf
fi

copy_dir $INSTALLDIR_NATIVE_NOLM $BUILDDIR_NATIVE/target-libs
copy_dir $INSTALLDIR_NATIVE_NOLM $BUILDDIR_NATIVE_NOLM/target-libs
restoreenv
popd

pushd $INSTALLDIR_NATIVE_NOLM
rm -rf ./lib
popd

# Enabling this will clobber any LM-enabled stuff built for pic32m. Don't.
# Instead we install binutils in two locations.
# copy_dir $INSTALLDIR_NATIVE_NOLM $INSTALLDIR_NATIVE

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-1-nolm.txt
status_update "  Task [III-1-nolm] /$HOST_NATIVE/gcc-first/ NOLM"
echo Task [III-1-nolm] /$HOST_NATIVE/gcc-first/ NOLM
saveenv
saveenvvar CFLAGS "$ENV_CFLAGS -DSKIP_LICENSE_MANAGER -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CPPFLAGS "$ENV_CPPFLAGS -DSKIP_LICENSE_MANAGER -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CXXFLAGS "$ENV_CXXFLAGS -DSKIP_LICENSE_MANAGER -D_BUILD_XC32_ -D_BUILD_MCHP_"
rm -rf $BUILDDIR_NATIVE/gcc-first && mkdir -p $BUILDDIR_NATIVE/gcc-first
pushd $BUILDDIR_NATIVE/gcc-first
if [[ "$skip_config" != "yes" ]]; then
echo $SRCDIR/$GCC/configure --target=$TARGET \
    --prefix=$INSTALLDIR_NATIVE_NOLM \
    --bindir=$INSTALLDIR_NATIVE_NOLM/bin/bin \
    --libexecdir=$INSTALLDIR_NATIVE_NOLM/lib \
    --infodir=$INSTALLDIR_NATIVE_NOLM_DOC/info \
    --mandir=$INSTALLDIR_NATIVE_NOLM_DOC/man \
    --htmldir=$INSTALLDIR_NATIVE_NOLM_DOC/html \
    --pdfdir=$INSTALLDIR_NATIVE_NOLM_DOC/pdf \
    --enable-languages=c \
    --disable-decimal-float \
    --disable-libffi \
    --disable-libgomp \
    --disable-libmudflap \
    --disable-libquadmath \
    --disable-libssp \
    --disable-libstdcxx-pch \
    --disable-nls \
    --disable-shared \
    --disable-threads \
    --disable-tls \
    --disable-libgcc \
    --with-newlib \
    --without-headers \
    --with-gnu-as \
    --with-gnu-ld \
    --with-python-dir=share/gcc-$TARGET_DIR \
    --with-sysroot=$INSTALLDIR_NATIVE_NOLM/$TARGET_DIR \
    ${GCC_CONFIG_OPTS}                              \
    "${GCC_CONFIG_OPTS_LCPP}"                              \
    "--with-pkgversion=$PKGVERSION" \
    ${MULTILIB_LIST} >$LOGTASKFILE
$SRCDIR/$GCC/configure --target=$TARGET \
    --prefix=$INSTALLDIR_NATIVE_NOLM \
    --bindir=$INSTALLDIR_NATIVE_NOLM/bin/bin \
    --libexecdir=$INSTALLDIR_NATIVE_NOLM/lib \
    --infodir=$INSTALLDIR_NATIVE_NOLM_DOC/info \
    --mandir=$INSTALLDIR_NATIVE_NOLM_DOC/man \
    --htmldir=$INSTALLDIR_NATIVE_NOLM_DOC/html \
    --pdfdir=$INSTALLDIR_NATIVE_NOLM_DOC/pdf \
    --enable-languages=c \
    --disable-decimal-float \
    --disable-libffi \
    --disable-libgomp \
    --disable-libmudflap \
    --disable-libquadmath \
    --disable-libssp \
    --disable-libstdcxx-pch \
    --disable-nls \
    --disable-shared \
    --disable-threads \
    --disable-tls \
    --disable-libgcc \
    --with-newlib \
    --without-headers \
    --with-gnu-as \
    --with-gnu-ld \
    --with-python-dir=share/gcc-$TARGET_DIR \
    --with-sysroot=$INSTALLDIR_NATIVE_NOLM/$TARGET_DIR \
    ${GCC_CONFIG_OPTS}                              \
    "${GCC_CONFIG_OPTS_LCPP}"                              \
    "--with-pkgversion=$PKGVERSION" \
    ${MULTILIB_LIST} >>$LOGTASKFILE 2>&1
fi

make -j$JOBS all-gcc >>$LOGTASKFILE 2>&1

make install-gcc >>$LOGTASKFILE 2>&1

restoreenv
popd

pushd $INSTALLDIR_NATIVE_NOLM
rm -rf bin/$TARGET_DIR-gccbug
rm -rf ./lib/libiberty.a
rm -rf  include
popd

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-2.txt
echo Task [III-2] /$HOST_NATIVE/newlib/
saveenv
prepend_path PATH $INSTALLDIR_NATIVE_NOLM/bin
prepend_path PATH $INSTALLDIR_NATIVE_NOLM/bin/bin
saveenvvar CFLAGS_FOR_TARGET '-g -O2 -ffunction-sections -fdata-sections'
saveenvvar CXXFLAGS_FOR_TARGET '-g -O2 -ffunction-sections -fdata-sections'
saveenvvar CXXCPP '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar AR_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ar'
saveenvvar AS_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-as'
saveenvvar LD_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ld'
saveenvvar GCC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar XGCC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar CC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar CPP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar CXX_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-g++'
saveenvvar NM_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-nm'
saveenvvar RANLIB_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ranlib'
saveenvvar STRIP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-strip'
saveenvvar OBJDUMP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-objdump'

rm -rf $BUILDDIR_NATIVE/newlib && mkdir -p $BUILDDIR_NATIVE/newlib
pushd $BUILDDIR_NATIVE/newlib

if [[ ! -e $INSTALLDIR_NATIVE/bin/bin/$TARGET-cc ]] ; then
    ln -s $INSTALLDIR_NATIVE/bin/bin/$TARGET-gcc $INSTALLDIR_NATIVE/bin/bin/$TARGET-cc 
fi

if [[ "$skip_config" != "yes" ]]; then
$SRCDIR/$NEWLIB/configure  \
    $NEWLIB_CONFIG_OPTS \
    --target=$TARGET \
    --prefix=$INSTALLDIR_NATIVE \
    --infodir=$INSTALLDIR_NATIVE_DOC/info \
    --mandir=$INSTALLDIR_NATIVE_DOC/man \
    --htmldir=$INSTALLDIR_NATIVE_DOC/html \
    --pdfdir=$INSTALLDIR_NATIVE_DOC/pdf \
    --enable-newlib-io-long-long \
    --enable-newlib-register-fini \
    --disable-newlib-supplied-syscalls \
    --disable-nls >$LOGTASKFILE 2>&1
fi

make -j$JOBS >>$LOGTASKFILE 2>&1

make install >>$LOGTASKFILE 2>&1

if [ "x$skip_manual" != "xyes" ]; then
make pdf
mkdir -p $INSTALLDIR_NATIVE_DOC/pdf
cp $BUILDDIR_NATIVE/newlib/$TARGET_DIR/newlib/libc/libc.pdf $INSTALLDIR_NATIVE_DOC/pdf/libc.pdf
cp $BUILDDIR_NATIVE/newlib/$TARGET_DIR/newlib/libm/libm.pdf $INSTALLDIR_NATIVE_DOC/pdf/libm.pdf

make html
mkdir -p $INSTALLDIR_NATIVE_DOC/html
copy_dir $BUILDDIR_NATIVE/newlib/$TARGET_DIR/newlib/libc/libc.html $INSTALLDIR_NATIVE_DOC/html/libc
copy_dir $BUILDDIR_NATIVE/newlib/$TARGET_DIR/newlib/libm/libm.html $INSTALLDIR_NATIVE_DOC/html/libm
fi

# Copy newlib into native-nolm
xc32_eval "rsync -qavzCr --include=*.h --include=*.a --include=*.specs --include=*crt*.o --include=*/c++/** --include=*/ --exclude=* --prune-empty-dirs \"$INSTALLDIR_NATIVE/\" \"$INSTALLDIR_NATIVE_NOLM\""

popd
restoreenv

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-3.txt
status_update "  Task [III-3] /$HOST_NATIVE/newlib-nano/"
echo Task [III-3] /$HOST_NATIVE/newlib-nano/
saveenv
prepend_path PATH $INSTALLDIR_NATIVE_NOLM/bin
prepend_path PATH $INSTALLDIR_NATIVE_NOLM/bin/bin
saveenvvar CFLAGS_FOR_TARGET '-g -Os -ffunction-sections -fdata-sections'
saveenvvar CXXCPP '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar AR_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ar'
saveenvvar AS_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-as'
saveenvvar LD_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ld'
saveenvvar GCC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar XGCC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar CC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar CPP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar CPPCXX_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar CXX_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-g++'
saveenvvar NM_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-nm'
saveenvvar RANLIB_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ranlib'
saveenvvar STRIP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-strip'
saveenvvar OBJDUMP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-objdump'

rm -rf $BUILDDIR_NATIVE/newlib-nano && mkdir -p $BUILDDIR_NATIVE/newlib-nano
pushd $BUILDDIR_NATIVE/newlib-nano

if [[ "$skip_config" != "yes" ]]; then
$SRCDIR/$NEWLIB_NANO/configure  \
    $NEWLIB_CONFIG_OPTS \
    --target=$TARGET \
    --prefix=$BUILDDIR_NATIVE/target-libs \
    --disable-newlib-supplied-syscalls    \
    --enable-newlib-reent-small           \
    --disable-newlib-fvwrite-in-streamio  \
    --disable-newlib-fseek-optimization   \
    --disable-newlib-wide-orient          \
    --enable-newlib-nano-malloc           \
    --disable-newlib-unbuf-stream-opt     \
    --enable-lite-exit                    \
    --enable-newlib-global-atexit         \
    --enable-newlib-nano-formatted-io     \
    --disable-nls >$LOGTASKFILE 2>&1
fi

make -j$JOBS >>$LOGTASKFILE 2>&1
make install >>$LOGTASKFILE 2>&1

# Copy newlib-nano into native-nolm
xc32_eval "rsync -qavzCr --include=*.h --include=*.a --include=*.specs --include=*crt*.o --include=*/c++/** --include=*/ --exclude=* --prune-empty-dirs \"$BUILDDIR_NATIVE/target-libs/\" \"$BUILDDIR_NATIVE_NOLM/target-libs\""
xc32_eval "rsync -qavzCr --include=*.h --include=*.a --include=*.specs --include=*crt*.o --include=*/c++/** --include=*/ --exclude=* --prune-empty-dirs \"$INSTALLDIR_NATIVE/\" \"$INSTALLDIR_NATIVE_NOLM\""

popd
restoreenv


LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-3b-pic32c-libs.txt
echo Task [III-3b] /$HOST_NATIVE/pic32c-libs/ > $LOGTASKFILE
echo Task [III-3b] /$HOST_NATIVE/pic32c-libs/
status_update "  Task [III-3] /$HOST_NATIVE/pic32c-libs/"
saveenv
prepend_path PATH $INSTALLDIR_NATIVE_NOLM/bin
prepend_path PATH $INSTALLDIR_NATIVE_NOLM/bin/bin
saveenvvar CFLAGS_FOR_TARGET '-g -Os -ffunction-sections -fdata-sections'
saveenvvar CXXCPP '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar AR_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ar'
saveenvvar AS_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-as'
saveenvvar LD_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ld'
saveenvvar GCC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar XGCC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar CC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar CPP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar CPPCXX_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar CXX_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-g++'
saveenvvar NM_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-nm'
saveenvvar RANLIB_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ranlib'
saveenvvar STRIP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-strip'
saveenvvar OBJDUMP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-objdump'

rm -rf $BUILDDIR_NATIVE_NOLM/pic32c-libs
cp -r $ROOT/pic32c-source/pic32c-libs $BUILDDIR_NATIVE_NOLM
pushd $BUILDDIR_NATIVE_NOLM/pic32c-libs >>$LOGTASKFILE 2>&1

make DESTROOT="$INSTALLDIR_NATIVE_NOLM" -j$JOBS >>$LOGTASKFILE 2>&1
assert_success $? "ERROR: pic32c-libs make failed"

make DESTROOT="$INSTALLDIR_NATIVE_NOLM" install -j2 >>$LOGTASKFILE 2>&1
assert_success $? "ERROR: pic32c-libs install INSTALLDIR_NATIVE_NOLM failed"

make DESTROOT="$INSTALLDIR_NATIVE" install -j2 >>$LOGTASKFILE 2>&1
assert_success $? "ERROR: pic32c-libs install INSTALLDIR_NATIVE failed"

# mkdir -p $INSTALLDIR_MINGW
# make DESTROOT="$INSTALLDIR_MINGW" install -j2 >>$LOGTASKFILE 2>&1

popd
restoreenv


LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-4-nolm.txt
status_update "  Task [III-4-nolm] /$HOST_NATIVE/gcc-final/ NOLM"
echo Task [III-4-nolm] /$HOST_NATIVE/gcc-final/ NOLM
rm -f $INSTALLDIR_NATIVE/$TARGET_DIR/usr
rm -f $INSTALLDIR_NATIVE_NOLM/$TARGET_DIR/usr
ln -s . $INSTALLDIR_NATIVE_NOLM/$TARGET_DIR/usr

saveenv
saveenvvar CXXCPP 'cpp'
saveenvvar AR_FOR_TARGET $TARGET-ar
saveenvvar NM_FOR_TARGET $TARGET-nm
saveenvvar OBJDUMP_FOR_TARET $TARGET-objdump
saveenvvar STRIP_FOR_TARGET $TARGET-strip
saveenvvar CC_FOR_TARGET $TARGET-gcc
saveenvvar GCC_FOR_TARGET $TARGET-gcc
saveenvvar CXX_FOR_TARGET $TARGET-g++
saveenvvar CFLAGS "$ENV_CFLAGS $DEVEL_CFLAGS -DSKIP_LICENSE_MANAGER -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CPPFLAGS "$ENV_CPPFLAGS $DEVEL_CFLAGS -DSKIP_LICENSE_MANAGER -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CXXFLAGS "$ENV_CXXFLAGS $DEVEL_CFLAGS -DSKIP_LICENSE_MANAGER -D_BUILD_XC32_ -D_BUILD_MCHP_"

rm -rf $BUILDDIR_NATIVE_NOLM/gcc-final && mkdir -p $BUILDDIR_NATIVE_NOLM/gcc-final
pushd $BUILDDIR_NATIVE_NOLM/gcc-final

if [[ "$skip_config" != "yes" ]]; then
$SRCDIR/$GCC/configure --target=$TARGET \
    --prefix=$INSTALLDIR_NATIVE_NOLM \
    --bindir=$INSTALLDIR_NATIVE_NOLM/bin/bin \
    --libexecdir=$INSTALLDIR_NATIVE_NOLM/lib \
    --infodir=$INSTALLDIR_NATIVE_NOLM_DOC/info \
    --mandir=$INSTALLDIR_NATIVE_NOLM_DOC/man \
    --htmldir=$INSTALLDIR_NATIVE_NOLM_DOC/html \
    --pdfdir=$INSTALLDIR_NATIVE_NOLM_DOC/pdf \
    --enable-languages=c,c++ \
    --enable-plugins \
    --disable-decimal-float \
    --disable-libffi \
    --disable-libgomp \
    --disable-libmudflap \
    --disable-libquadmath \
    --disable-libssp \
    --disable-libstdcxx-pch \
    --disable-nls \
    --disable-shared \
    --disable-threads \
    --disable-tls \
    --with-gnu-as \
    --with-gnu-ld \
    --with-newlib \
    --with-headers=yes \
    --with-python-dir=share/gcc-$TARGET_DIR \
    --with-sysroot=$INSTALLDIR_NATIVE_NOLM/$TARGET_DIR \
    $GCC_CONFIG_OPTS                                \
    "${GCC_CONFIG_OPTS_LCPP}"                              \
    "--with-pkgversion=$PKGVERSION" \
    ${MULTILIB_LIST} >$LOGTASKFILE 2>&1
fi

# Passing USE_TM_CLONE_REGISTRY=0 via INHIBIT_LIBC_CFLAGS to disable
# transactional memory related code in crtbegin.o.
# This is a workaround. Better approach is have a t-* to set this flag via
# CRTSTUFF_T_CFLAGS
if [ "x$DEBUG_BUILD_OPTIONS" != "x" ]; then
  make -j$JOBS CXXFLAGS="$DEBUG_BUILD_OPTIONS" \
	       INHIBIT_LIBC_CFLAGS="-DUSE_TM_CLONE_REGISTRY=0" >>$LOGTASKFILE 2>&1
else
  make -j$JOBS INHIBIT_LIBC_CFLAGS="-DUSE_TM_CLONE_REGISTRY=0" >>$LOGTASKFILE 2>&1
fi

make install >>$LOGTASKFILE 2>&1

if [ "x$skip_manual" != "xyes" ]; then
	make install-html install-pdf
fi

pushd $INSTALLDIR_NATIVE_NOLM
rm -rf bin/$TARGET_DIR-gccbug
LIBIBERTY_LIBRARIES=`find $INSTALLDIR_NATIVE_NOLM/$TARGET_DIR/lib -name libiberty.a`
for libiberty_lib in $LIBIBERTY_LIBRARIES ; do
    rm -rf $libiberty_lib
done
rm -rf ./lib/libiberty.a
rm -rf  include
popd

rm -f $INSTALLDIR_NATIVE_NOLM/$TARGET_DIR/usr
popd
restoreenv




LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-5-nolm.txt
status_update "  Task [III-5-nolm] /$HOST_NATIVE/gcc-size-libstdcxx/ NOLM"
echo Task [III-5-nolm] /$HOST_NATIVE/gcc-size-libstdcxx/ NOLM
rm -f $BUILDDIR_NATIVE_NOLM/target-libs/$TARGET_DIR/usr
ln -s . $BUILDDIR_NATIVE_NOLM/target-libs/$TARGET_DIR/usr

saveenv
saveenvvar CXXCPP 'cpp'
saveenvvar AR_FOR_TARGET $TARGET-ar
saveenvvar NM_FOR_TARGET $TARGET-nm
saveenvvar OBJDUMP_FOR_TARET $TARGET-objdump
saveenvvar STRIP_FOR_TARGET $TARGET-strip
saveenvvar CC_FOR_TARGET $TARGET-gcc
saveenvvar GCC_FOR_TARGET $TARGET-gcc
saveenvvar CXX_FOR_TARGET $TARGET-g++
saveenvvar CFLAGS "$ENV_CFLAGS -DSKIP_LICENSE_MANAGER -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CPPFLAGS "$ENV_CPPFLAGS -DSKIP_LICENSE_MANAGER -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CXXFLAGS "$ENV_CXXFLAGS -DSKIP_LICENSE_MANAGER -D_BUILD_XC32_ -D_BUILD_MCHP_"

rm -rf $BUILDDIR_NATIVE/gcc-size-libstdcxx && mkdir -p $BUILDDIR_NATIVE/gcc-size-libstdcxx
pushd $BUILDDIR_NATIVE/gcc-size-libstdcxx

if [[ "$skip_config" != "yes" ]]; then
$SRCDIR/$GCC/configure --target=$TARGET \
    --prefix=$BUILDDIR_NATIVE_NOLM/target-libs \
    --enable-languages=c,c++ \
    --disable-decimal-float \
    --disable-libffi \
    --disable-libgomp \
    --disable-libmudflap \
    --disable-libquadmath \
    --disable-libssp \
    --disable-libstdcxx-pch \
    --disable-libstdcxx-verbose \
    --disable-nls \
    --disable-shared \
    --disable-threads \
    --disable-tls \
    --with-gnu-as \
    --with-gnu-ld \
    --with-newlib \
    --with-headers=yes \
    --with-python-dir=share/gcc-$TARGET_DIR \
    --with-sysroot=$BUILDDIR_NATIVE_NOLM/target-libs/$TARGET_DIR \
    $GCC_CONFIG_OPTS \
    "${GCC_CONFIG_OPTS_LCPP}"                              \
    "--with-pkgversion=$PKGVERSION" \
    ${MULTILIB_LIST} >$LOGTASKFILE 2>&1
fi

make -j$JOBS CXXFLAGS_FOR_TARGET="-g -Os -ffunction-sections -fdata-sections -fno-exceptions" >>$LOGTASKFILE 2>&1
make install -j2 >>$LOGTASKFILE 2>&1

copy_multi_libs src_prefix="$BUILDDIR_NATIVE_NOLM/target-libs/$TARGET_DIR/lib" \
                dst_prefix="$INSTALLDIR_NATIVE_NOLM/$TARGET_DIR/lib"           \
                target_gcc="$BUILDDIR_NATIVE_NOLM/target-libs/bin/$TARGET_DIR-gcc"

copy_multi_libs src_prefix="$BUILDDIR_NATIVE_NOLM/target-libs/$TARGET_DIR/lib" \
                dst_prefix="$INSTALLDIR_NATIVE/$TARGET_DIR/lib"           \
                target_gcc="$BUILDDIR_NATIVE_NOLM/target-libs/bin/$TARGET_DIR-gcc"
                
# Copy the nano configured newlib.h file into the location that nano.specs
# expects it to be.
mkdir -p $INSTALLDIR_NATIVE_NOLM/$TARGET_DIR/include/newlib-nano
cp -f $BUILDDIR_NATIVE_NOLM/target-libs/$TARGET_DIR/include/newlib.h \
		 $INSTALLDIR_NATIVE_NOLM/$TARGET_DIR/include/newlib-nano/newlib.h
mkdir -p $INSTALLDIR_NATIVE/$TARGET_DIR/include/newlib-nano
cp -f $BUILDDIR_NATIVE/target-libs/$TARGET_DIR/include/newlib.h \
		 $INSTALLDIR_NATIVE/$TARGET_DIR/include/newlib-nano/newlib.h

# Copy all the just built libs into the native installation.
xc32_eval "rsync -qaC --prune-empty-dirs --include=*.h --include=*.a --include=*.specs --include=*crt*.o --include=*/c++/** --include=*/ --exclude=* \"$INSTALLDIR_NATIVE_NOLM/\" \"$INSTALLDIR_NATIVE\""

popd
restoreenv


LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-4-lm.txt
status_update "  Task [III-4-lm] /$HOST_NATIVE/gcc-final/ LM"
echo Task [III-4-lm] /$HOST_NATIVE/gcc-final/ LM
rm -f $INSTALLDIR_NATIVE/$TARGET_DIR/usr
ln -s . $INSTALLDIR_NATIVE/$TARGET_DIR/usr

rm -rf $BUILDDIR_NATIVE/gcc-final && mkdir -p $BUILDDIR_NATIVE/gcc-final
pushd $BUILDDIR_NATIVE/gcc-final

#cd "$WORKING_DIR/$NATIVE_LM_BUILD/gcc"

if [ "x$SHASUM256" != "x" ]; then
  if [ ! -e $INSTALLDIR_NATIVE/xclm/install/client/bin/xclm ] ; then
    assert_success -1 "ERROR: Missing $INSTALLDIR_NATIVE/xclm/install/client/bin/xclm"
  fi
  XCLM_SHASUM=`$SHASUM256 $INSTALLDIR_NATIVE/xclm/install/client/bin/xclm | head -c 64`
  if [ ${#XCLM_SHASUM} != 64 ]; then
      assert_success -1 "ERROR: Failed to calculate SHASUM256 digest for $NATIVEIMAGE/bin/xclm"
  fi
  if [ "x$NATIVEIMAGE" == "x$LINUX32IMAGE" ]; then
    if [ ! -e $INSTALLDIR_NATIVE/xclm64/install/client/bin/xclm ] ; then
      assert_success -1 "ERROR: Missing $INSTALLDIR_NATIVE/xclm64/install/client/bin/xclm"
    fi
    XCLM64_SHASUM=`$SHASUM256 $INSTALLDIR_NATIVE/xclm64/install/client/bin/xclm | head -c 64`
    if [ ${#XCLM64_SHASUM} != 64 ]; then
        assert_success -1 "ERROR: Failed to calculate SHASUM256 digest for 64-bit xclm"
    fi
    XCLM_SHASUM_MACRO="-DMCHP_XCLM_SHA256_DIGEST=${XCLM_SHASUM} -DMCHP_XCLM64_SHA256_DIGEST=${XCLM64_SHASUM} -DMCHP_FXCLM_SHA256_DIGEST=8727ea3da9bdd624fee0130eb6133188719892bcbee7da32606911a8b08a1a8d "
  else
    XCLM_SHASUM_MACRO="-DMCHP_XCLM_SHA256_DIGEST=${XCLM_SHASUM} "
  fi 
fi

saveenv
saveenvvar CXXCPP 'cpp'
saveenvvar AR_FOR_TARGET $TARGET-ar
saveenvvar NM_FOR_TARGET $TARGET-nm
saveenvvar OBJDUMP_FOR_TARET $TARGET-objdump
saveenvvar STRIP_FOR_TARGET $TARGET-strip
saveenvvar CC_FOR_TARGET $TARGET-gcc
saveenvvar GCC_FOR_TARGET $TARGET-gcc
saveenvvar CXX_FOR_TARGET $TARGET-g++
saveenvvar CFLAGS "$ENV_CFLAGS $DEVEL_CFLAGS ${XCLM_SHASUM_MACRO} -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CPPFLAGS "$ENV_CPPFLAGS $DEVEL_CFLAGS ${XCLM_SHASUM_MACRO} -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CXXFLAGS "$ENV_CXXFLAGS $DEVEL_CFLAGS ${XCLM_SHASUM_MACRO} -D_BUILD_XC32_ -D_BUILD_MCHP_"

XCLM_SHASUM_MACRO=""
XCLM64_SHASUM=""

if [[ "$skip_config" != "yes" ]]; then
$SRCDIR/$GCC/configure --target=$TARGET \
    --prefix=$INSTALLDIR_NATIVE \
    --bindir=$INSTALLDIR_NATIVE/bin/bin \
    --libexecdir=$INSTALLDIR_NATIVE/lib \
    --infodir=$INSTALLDIR_NATIVE_DOC/info \
    --mandir=$INSTALLDIR_NATIVE_DOC/man \
    --htmldir=$INSTALLDIR_NATIVE_DOC/html \
    --pdfdir=$INSTALLDIR_NATIVE_DOC/pdf \
    --enable-languages=c,c++ \
    --enable-plugins \
    --disable-decimal-float \
    --disable-libffi \
    --disable-libgomp \
    --disable-libmudflap \
    --disable-libquadmath \
    --disable-libssp \
    --disable-libstdcxx-pch \
    --disable-nls \
    --disable-shared \
    --disable-threads \
    --disable-tls \
    --with-gnu-as \
    --with-gnu-ld \
    --with-newlib \
    --with-headers=yes \
    --with-python-dir=share/gcc-$TARGET_DIR \
    --with-sysroot=$INSTALLDIR_NATIVE/$TARGET_DIR \
    $GCC_CONFIG_OPTS                                \
    "${GCC_CONFIG_OPTS_LCPP}"                              \
    "--with-pkgversion=$PKGVERSION" \
    ${MULTILIB_LIST} >$LOGTASKFILE 2>&1
fi

# Passing USE_TM_CLONE_REGISTRY=0 via INHIBIT_LIBC_CFLAGS to disable
# transactional memory related code in crtbegin.o.
# This is a workaround. Better approach is have a t-* to set this flag via
# CRTSTUFF_T_CFLAGS
if [ "x$DEBUG_BUILD_OPTIONS" != "x" ]; then
  make all-gcc -j$JOBS CXXFLAGS="$DEBUG_BUILD_OPTIONS" \
	       INHIBIT_LIBC_CFLAGS="-DUSE_TM_CLONE_REGISTRY=0" >>$LOGTASKFILE 2>&1
else
  make all-gcc -j$JOBS INHIBIT_LIBC_CFLAGS="-DUSE_TM_CLONE_REGISTRY=0" >>$LOGTASKFILE 2>&1
fi

make install-gcc -j2 >>$LOGTASKFILE 2>&1

if [ "x$skip_manual" != "xyes" ]; then
	make install-html install-pdf >>$LOGTASKFILE 2>&1
fi

pushd $INSTALLDIR_NATIVE
rm -rf bin/$TARGET_DIR-gccbug
LIBIBERTY_LIBRARIES=`find $INSTALLDIR_NATIVE/$TARGET_DIR/lib -name libiberty.a`
for libiberty_lib in $LIBIBERTY_LIBRARIES ; do
    rm -rf $libiberty_lib
done
rm -rf ./lib/libiberty.a
rm -rf  include
popd

rm -f $INSTALLDIR_NATIVE/$TARGET_DIR/usr
popd
restoreenv






LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-6.txt
status_update "  Task [III-6] /$HOST_NATIVE/gdb/"
echo Task [III-6] /$HOST_NATIVE/gdb/
build_gdb()
{
	GDB_EXTRA_CONFIG_OPTS=$1

	rm -rf $BUILDDIR_NATIVE/gdb && mkdir -p $BUILDDIR_NATIVE/gdb
	pushd $BUILDDIR_NATIVE/gdb
	saveenv
	saveenvvar CFLAGS "$ENV_CFLAGS -I /opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 -I /usr/include/python2.7"
	saveenvvar CPPFLAGS "$ENV_CPPFLAGS -I /opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 -I /usr/include/python2.7"
	saveenvvar CXXFLAGS "$ENV_CXXFLAGS -I /opt/local/Library/Frameworks/Python.framework/Versions/2.7/include/python2.7 -I /usr/include/python2.7"
	saveenvvar LDFLAGS "$ENV_LDFLAGS"

        if [[ "$skip_config" != "yes" ]]; then
	$SRCDIR/$GDB/configure  \
	    --target=$TARGET \
	    --prefix=$INSTALLDIR_NATIVE \
	    --bindir=$INSTALLDIR_NATIVE/bin/bin \
	    --infodir=$INSTALLDIR_NATIVE_DOC/info \
	    --mandir=$INSTALLDIR_NATIVE_DOC/man \
	    --htmldir=$INSTALLDIR_NATIVE_DOC/html \
	    --pdfdir=$INSTALLDIR_NATIVE_DOC/pdf \
	    --disable-nls \
	    --disable-sim \
	    --disable-gas \
	    --disable-binutils \
	    --disable-ld \
	    --disable-gprof \
	    --with-libexpat \
	    --with-lzma=no \
	    --with-system-gdbinit=$INSTALLDIR_NATIVE/$HOST_NATIVE/$TARGET_DIR/lib/gdbinit \
	    $GDB_CONFIG_OPTS \
	    $GDB_EXTRA_CONFIG_OPTS \
	    '--with-gdb-datadir='\''${prefix}'\''/${TARGET_DIR}/share/gdb' \
	    "--with-pkgversion=$PKGVERSION" >$LOGTASKFILE 2>&1
        fi

	if [ "x$DEBUG_BUILD_OPTIONS" != "x" ] ; then
	    make CFLAGS="-I$BUILDDIR_NATIVE/host-libs/zlib/include $DEBUG_BUILD_OPTIONS" -j$JOBS >>$LOGTASKFILE 2>&1
	else
	    make -j$JOBS >>$LOGTASKFILE 2>&1
	fi

	make install >>$LOGTASKFILE 2>&1

	if [ "x$skip_manual" != "xyes" ]; then
		make install-html install-pdf
	fi

	restoreenv
	popd
}


#Always enable python support in GDB for PPA build.
if [ "x$is_ppa_release" == "xyes" ]; then
	build_gdb "--with-python=yes"
else
        #First we build GDB without python support.
	build_gdb "--with-python=no"

	#Then build gdb with python support.
	if [ "x$skip_gdb_with_python" == "xno" ]; then
		build_gdb "--with-python=yes --program-prefix=$TARGET-  --program-suffix=-py"
	fi
fi

if [ "x$is_ppa_release" != "xyes" -a "x$skip_manual" != "xyes" ]; then
LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-7.txt
status_update "  Task [III-7] /$HOST_NATIVE/build-manual"
echo TASK [III-7] /$HOST_NATIVE/build-manual
rm -rf $BUILDDIR_NATIVE/build-manual && mkdir -p $BUILDDIR_NATIVE/build-manual
pushd $BUILDDIR_NATIVE/build-manual
cp -r $SRCDIR/$BUILD_MANUAL/* .
echo "@set VERSION_PACKAGE ($PKGVERSION)" > version.texi
echo "@set CURRENT_YEAR  $release_year" >> version.texi
echo "@set CURRENT_MONTH $release_month" >> version.texi
echo "@set PKG_NAME $PACKAGE_NAME" >> version.texi
make clean
make >$LOGTASKFILE 2>&1
rm -rf $ROOT/How-to-build-toolchain.pdf
cp How-to-build-toolchain.pdf $ROOT
popd
fi

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-8.txt
status_update "  Task [III-8] /$HOST_NATIVE/pretidy/"
echo Task [III-8] /$HOST_NATIVE/pretidy/
rm -rf $INSTALLDIR_NATIVE/lib/libiberty.a
find $INSTALLDIR_NATIVE -name '*.la' -exec rm '{}' ';'

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-9.txt
status_update "  Task [III-9] /$HOST_NATIVE/strip_host_objects/"
echo Task [III-9] /$HOST_NATIVE/strip_host_objects/
if [ "x$DEBUG_BUILD_OPTIONS" = "x" ] ; then
    STRIP_BINARIES=`find $INSTALLDIR_NATIVE/bin/ -name $TARGET-\*`
    for bin in $STRIP_BINARIES ; do
        strip_binary strip $bin
    done

    STRIP_BINARIES=`find $INSTALLDIR_NATIVE/$TARGET_DIR/bin/ -maxdepth 1 -mindepth 1 -name \*`
    for bin in $STRIP_BINARIES ; do
        strip_binary strip $bin
    done

    if [ "x$uname_string" == "xdarwin" ]; then
        STRIP_BINARIES=`find $INSTALLDIR_NATIVE/lib/gcc/$TARGET_DIR/$GCC_VER/ -maxdepth 1 -name \* -perm +111 -and ! -type d`
    else
        STRIP_BINARIES=`find $INSTALLDIR_NATIVE/lib/gcc/$TARGET_DIR/$GCC_VER/ -maxdepth 1 -name \* -perm /111 -and ! -type d`
    fi
    for bin in $STRIP_BINARIES ; do
        strip_binary strip $bin
    done
fi

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-10.txt
status_update "  Task [III-10] /$HOST_NATIVE/strip_target_objects/"
echo Task [III-10] /$HOST_NATIVE/strip_target_objects/
saveenv
prepend_path PATH $INSTALLDIR_NATIVE/bin
prepend_path PATH $INSTALLDIR_NATIVE/bin/bin
TARGET_LIBRARIES=`find $INSTALLDIR_NATIVE/$TARGET_DIR/lib -name \*.a`
for target_lib in $TARGET_LIBRARIES ; do
    $TARGET-objcopy -R .comment -R .note -R .debug_info -R .debug_aranges -R .debug_pubnames -R .debug_pubtypes -R .debug_abbrev -R .debug_line -R .debug_str -R .debug_ranges -R .debug_loc $target_lib || true
done

TARGET_OBJECTS=`find $INSTALLDIR_NATIVE/$TARGET_DIR/lib -name \*.o`
for target_obj in $TARGET_OBJECTS ; do
    $TARGET-objcopy -R .comment -R .note -R .debug_info -R .debug_aranges -R .debug_pubnames -R .debug_pubtypes -R .debug_abbrev -R .debug_line -R .debug_str -R .debug_ranges -R .debug_loc $target_obj || true
done

TARGET_LIBRARIES=`find $INSTALLDIR_NATIVE/lib/gcc/$TARGET_DIR/$GCC_VER -name \*.a`
for target_lib in $TARGET_LIBRARIES ; do
    $TARGET-objcopy -R .comment -R .note -R .debug_info -R .debug_aranges -R .debug_pubnames -R .debug_pubtypes -R .debug_abbrev -R .debug_line -R .debug_str -R .debug_ranges -R .debug_loc $target_lib || true
done

TARGET_OBJECTS=`find $INSTALLDIR_NATIVE/lib/gcc/$TARGET_DIR/$GCC_VER -name \*.o`
for target_obj in $TARGET_OBJECTS ; do
    $TARGET-objcopy -R .comment -R .note -R .debug_info -R .debug_aranges -R .debug_pubnames -R .debug_pubtypes -R .debug_abbrev -R .debug_line -R .debug_str -R .debug_ranges -R .debug_loc $target_obj || true
done
restoreenv

# PPA release needn't following steps, so we exit here.
if [ "x$is_ppa_release" == "xyes" ] ; then
  exit 0
fi

fi # skip_native

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-III-11.txt
status_update "  Task [III-11] /$HOST_NATIVE/package_tbz2/"
echo Task [III-11] /$HOST_NATIVE/package_tbz2/
rm -f $PACKAGEDIR/$PACKAGE_NAME_NATIVE.tar.gz
pushd $BUILDDIR_NATIVE
rm -f $INSTALL_PACKAGE_NAME
#cp $ROOT/$RELEASE_FILE $INSTALLDIR_NATIVE_DOC/
#cp $ROOT/$README_FILE $INSTALLDIR_NATIVE_DOC/
#cp $ROOT/$LICENSE_FILE $INSTALLDIR_NATIVE_DOC/
copy_dir_clean $SRCDIR/$SAMPLES $INSTALLDIR_NATIVE/share/gcc-$TARGET_DIR/$SAMPLES
ln -s $INSTALLDIR_NATIVE $INSTALL_PACKAGE_NAME
${TAR} -czf $PACKAGEDIR/$PACKAGE_NAME_NATIVE.tar.gz   \
    --owner=0                               \
    --group=0                               \
    --exclude=host-$HOST_NATIVE             \
    --exclude=host-$HOST_MINGW              \
    $INSTALL_PACKAGE_NAME/$TARGET_DIR       \
    $INSTALL_PACKAGE_NAME/bin               \
    $INSTALL_PACKAGE_NAME/lib               \
    $INSTALL_PACKAGE_NAME/share             
rm -f $INSTALL_PACKAGE_NAME
popd

# skip building mingw32 toolchain if "--skip_mingw32" specified
# this huge if statement controls all $BUILDDIR_MINGW tasks till "task [IV-8]"
if [ "x$skip_mingw32" != "xyes" ] ; then
saveenv
saveenvvar CC_FOR_BUILD gcc
saveenvvar CC $HOST_MINGW_TOOL-gcc
saveenvvar CXX $HOST_MINGW_TOOL-g++
saveenvvar AR $HOST_MINGW_TOOL-ar
saveenvvar RANLIB $HOST_MINGW_TOOL-ranlib
saveenvvar STRIP $HOST_MINGW_TOOL-strip
saveenvvar NM $HOST_MINGW_TOOL-nm

saveenvvar CFLAGS "$ENV_CFLAGS -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CPPFLAGS "$ENV_CPPFLAGS -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CXXFLAGS "$ENV_CXXFLAGS -D_BUILD_XC32_ -D_BUILD_MCHP_"

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-IV-0.txt

#if [ "x$skip_native" != "xyes" ] ; then
status_update "  Task [IV-0] /$HOST_MINGW/host_unpack/"
echo Task [IV-0] /$HOST_MINGW/host_unpack/
rm -rf $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE && mkdir $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE
pushd $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE
${TAR} -xf $PACKAGEDIR/$PACKAGE_NAME_NATIVE.tar.gz --strip-components=1
popd
#fi # skip_native

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-IV-1.txt
status_update "  Task [IV-1] /$HOST_MINGW/binutils/"
echo Task [IV-1] /$HOST_MINGW/binutils/
prepend_path PATH $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE/bin
rm -rf $BUILDDIR_MINGW/binutils && mkdir -p $BUILDDIR_MINGW/binutils
pushd $BUILDDIR_MINGW/binutils
saveenv
saveenvvar CFLAGS "-I$BUILDDIR_MINGW/host-libs/zlib/include -O2 -Wno-error"
saveenvvar CPPFLAGS "-I$BUILDDIR_MINGW/host-libs/zlib/include"
saveenvvar LDFLAGS "-L$BUILDDIR_MINGW/host-libs/zlib/lib"
$SRCDIR/$BINUTILS/configure --build=$BUILD \
    --host=$HOST_MINGW \
    --target=$TARGET \
    --prefix=$INSTALLDIR_MINGW \
    --bindir=$INSTALLDIR_MINGW/bin/bin \
    --infodir=$INSTALLDIR_MINGW_DOC/info \
    --mandir=$INSTALLDIR_MINGW_DOC/man \
    --htmldir=$INSTALLDIR_MINGW_DOC/html \
    --pdfdir=$INSTALLDIR_MINGW_DOC/pdf \
    --disable-nls \
    --disable-sim \
    --disable-gdb \
    --enable-plugins \
    --with-sysroot=$INSTALLDIR_MINGW/$TARGET_DIR \
    "--with-pkgversion=$PKGVERSION" >$LOGTASKFILE 2>&1

if [ "x$DEBUG_BUILD_OPTIONS" != "x" ] ; then
    make CFLAGS="-I$BUILDDIR_MINGW/host-libs/zlib/include $DEBUG_BUILD_OPTIONS" -j$JOBS >>$LOGTASKFILE 2>&1
else
    make -j$JOBS >>$LOGTASKFILE 2>&1
fi

make install

if [ "x$skip_manual" != "xyes" ]; then
	make install-html install-pdf
fi

restoreenv
popd

pushd $INSTALLDIR_MINGW
rm -rf ./lib
popd

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-IV-2.txt
status_update "  Task [IV-2] /$HOST_MINGW/copy_libs/"
echo Task [IV-2] /$HOST_MINGW/copy_libs/
if [ "x$skip_manual" != "xyes" ]; then
copy_dir $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE/share/doc/gcc-$TARGET_DIR/html $INSTALLDIR_MINGW_DOC/html
copy_dir $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE/share/doc/gcc-$TARGET_DIR/pdf $INSTALLDIR_MINGW_DOC/pdf
fi
copy_dir $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE/$TARGET_DIR/lib $INSTALLDIR_MINGW/$TARGET_DIR/lib
copy_dir $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE/$TARGET_DIR/include $INSTALLDIR_MINGW/$TARGET_DIR/include
copy_dir $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE/$TARGET_DIR/include/c++ $INSTALLDIR_MINGW/$TARGET_DIR/include/c++
copy_dir $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE/lib/gcc/$TARGET_DIR $INSTALLDIR_MINGW/lib/gcc/$TARGET_DIR


LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-IV-3.txt
status_update "Task [IV-3] /$HOST_MINGW/gcc-final/"

echo Task [IV-3] /$HOST_MINGW/gcc-final/
echo Task [IV-3] /$HOST_MINGW/gcc-final/ > $LOGTASKFILE

if [ "x$SHASUM256" != "x" ]; then
  if [ ! -e $INSTALLDIR_MINGW/xclm/install/client/bin/xclm.exe ] ; then
    assert_success -1 "ERROR: Missing $INSTALLDIR_MINGW/xclm/install/client/bin/xclm.exe"
  fi
  XCLM_SHASUM=`$SHASUM256 $INSTALLDIR_MINGW/xclm/install/client/bin/xclm.exe | head -c 64`
  if [ ${#XCLM_SHASUM} != 64 ]; then
      assert_success -1 "ERROR: Failed to calculate SHASUM256 digest for $INSTALLDIR_MINGW/xclm/install/client/bin/xclm.exe"
  fi
  XCLM_SHASUM_MACRO="-DMCHP_XCLM_SHA256_DIGEST=${XCLM_SHASUM} "
fi

saveenv

prepend_path PATH $INSTALLDIR_NATIVE_NOLM/bin
prepend_path PATH $INSTALLDIR_NATIVE_NOLM/bin/bin
saveenvvar CFLAGS_FOR_TARGET '-g -O2 -ffunction-sections -fdata-sections'
saveenvvar CXXFLAGS_FOR_TARGET '-g -O2 -ffunction-sections -fdata-sections'
saveenvvar AR_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ar'
saveenvvar AS_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-as'
saveenvvar LD_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ld'
saveenvvar GCC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar XGCC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar CC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar CPP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar CXX_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-g++'
saveenvvar NM_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-nm'
saveenvvar RANLIB_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ranlib'
saveenvvar STRIP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-strip'
saveenvvar OBJDUMP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-objdump'

saveenvvar CFLAGS "$ENV_CFLAGS ${XCLM_SHASUM_MACRO} -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CPPFLAGS "$ENV_CPPFLAGS ${XCLM_SHASUM_MACRO} -D_BUILD_XC32_ -D_BUILD_MCHP_"
saveenvvar CXXFLAGS "$ENV_CXXFLAGS ${XCLM_SHASUM_MACRO} -D_BUILD_XC32_ -D_BUILD_MCHP_"

XCLM_SHASUM_MACRO=""
XCLM_SHASUM=""

pushd $INSTALLDIR_MINGW/$TARGET_DIR/
rm -f usr
ln -s . usr
popd
rm -rf $BUILDDIR_MINGW/gcc && mkdir -p $BUILDDIR_MINGW/gcc
pushd $BUILDDIR_MINGW/gcc
if [[ "$skip_config" != "yes" ]]; then
echo $SRCDIR/$GCC/configure --build=$BUILD --host=$HOST_MINGW --target=$TARGET \
    --prefix=$INSTALLDIR_MINGW \
    --bindir=$INSTALLDIR_MINGW/bin/bin \
    --libexecdir=$INSTALLDIR_MINGW/lib \
    --infodir=$INSTALLDIR_MINGW_DOC/info \
    --mandir=$INSTALLDIR_MINGW_DOC/man \
    --htmldir=$INSTALLDIR_MINGW_DOC/html \
    --pdfdir=$INSTALLDIR_MINGW_DOC/pdf \
    --enable-languages=c,c++ \
    --disable-decimal-float \
    --disable-libffi \
    --disable-libgomp \
    --disable-libmudflap \
    --disable-libquadmath \
    --disable-libssp \
    --disable-libstdcxx-pch \
    --disable-nls \
    --disable-shared \
    --disable-threads \
    --disable-tls \
    --with-gnu-as \
    --with-gnu-ld \
    --with-headers=yes \
    --with-newlib \
    --with-python-dir=share/gcc-$TARGET_DIR \
    --with-sysroot=$INSTALLDIR_MINGW/$TARGET_DIR \
    --with-libiconv-prefix=$BUILDDIR_MINGW/host-libs/usr \
    --with-gmp=$BUILDDIR_MINGW/host-libs/usr \
    --with-mpfr=$BUILDDIR_MINGW/host-libs/usr \
    --with-mpc=$BUILDDIR_MINGW/host-libs/usr \
    --with-isl=$BUILDDIR_MINGW/host-libs/usr \
    --with-cloog=$BUILDDIR_MINGW/host-libs/usr \
    --with-libelf=$BUILDDIR_MINGW/host-libs/usr \
    LDFLAGS=-static \
    "--with-host-libstdcxx=-static-libgcc -static-libstdc++ -Wl,-Bstatic,-lwinpthread,-lpthread,-lstdc++,-Bdynamic -lm" \
    "--with-pkgversion=$PKGVERSION" \
    ${MULTILIB_LIST} >>$LOGTASKFILE
$SRCDIR/$GCC/configure --build=$BUILD --host=$HOST_MINGW --target=$TARGET \
    --prefix=$INSTALLDIR_MINGW \
    --bindir=$INSTALLDIR_MINGW/bin/bin \
    --libexecdir=$INSTALLDIR_MINGW/lib \
    --infodir=$INSTALLDIR_MINGW_DOC/info \
    --mandir=$INSTALLDIR_MINGW_DOC/man \
    --htmldir=$INSTALLDIR_MINGW_DOC/html \
    --pdfdir=$INSTALLDIR_MINGW_DOC/pdf \
    --enable-languages=c,c++ \
    --disable-decimal-float \
    --disable-libffi \
    --disable-libgomp \
    --disable-libmudflap \
    --disable-libquadmath \
    --disable-libssp \
    --disable-libstdcxx-pch \
    --disable-nls \
    --disable-shared \
    --disable-threads \
    --disable-tls \
    --with-gnu-as \
    --with-gnu-ld \
    --with-headers=yes \
    --with-newlib \
    --with-python-dir=share/gcc-$TARGET_DIR \
    --with-sysroot=$INSTALLDIR_MINGW/$TARGET_DIR \
    --with-libiconv-prefix=$BUILDDIR_MINGW/host-libs/usr \
    --with-gmp=$BUILDDIR_MINGW/host-libs/usr \
    --with-mpfr=$BUILDDIR_MINGW/host-libs/usr \
    --with-mpc=$BUILDDIR_MINGW/host-libs/usr \
    --with-isl=$BUILDDIR_MINGW/host-libs/usr \
    --with-cloog=$BUILDDIR_MINGW/host-libs/usr \
    --with-libelf=$BUILDDIR_MINGW/host-libs/usr \
    LDFLAGS=-static \
    "--with-host-libstdcxx=-static-libgcc -Wl,-Bstatic,-lwinpthread,-lpthread,-lstdc++,-Bdynamic -lm" \
    "--with-pkgversion=$PKGVERSION" \
    ${MULTILIB_LIST} >>$LOGTASKFILE 2>&1
fi
if [ "x$DEBUG_BUILD_OPTIONS" != "x" ]; then
  make -j$JOBS CXXFLAGS="$DEBUG_BUILD_OPTIONS" all-gcc >>$LOGTASKFILE 2>&1
else
  make -j$JOBS all-gcc >>$LOGTASKFILE 2>&1
fi

make  install-gcc -j2 >>$LOGTASKFILE 2>&1

if [ -e /usr/i686-w64-mingw32/lib/libwinpthread-1.dll ] ; then
  cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll $INSTALLDIR_MINGW/bin/bin/
  cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll $INSTALLDIR_MINGW/lib/gcc/pic32c/6.2.1/
  cp /usr/i686-w64-mingw32/lib/libwinpthread-1.dll $INSTALLDIR_MINGW/pic32c/bin
fi

if [ "x$skip_manual" != "xyes" ]; then
	make install-html-gcc install-pdf-gcc >>$LOGTASKFILE 2>&1
fi
popd

pushd $INSTALLDIR_MINGW
rm -rf bin/$TARGET-gccbug
rm -rf  include
popd

copy_dir $BUILDDIR_MINGW/tools-$OBJ_SUFFIX_NATIVE/lib/gcc/$TARGET_DIR $INSTALLDIR_MINGW/lib/gcc/$TARGET_DIR
rm -rf $INSTALLDIR_MINGW/$TARGET_DIR/usr
rm -rf $INSTALLDIR_MINGW/lib/gcc/$TARGET_DIR/*/plugin
find $INSTALLDIR_MINGW -executable -and -not -type d -and -not -name \*.exe \
  -and -not -name liblto_plugin-0.dll -exec rm -f \{\} \;
restoreenv

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-IV-4.txt
status_update "  Task [IV-4] /$HOST_MINGW/gdb/"
echo Task [IV-4] /$HOST_MINGW/gdb/
build_mingw_gdb()
{
	MINGW_GDB_CONF_OPTS=$1
	rm -rf $BUILDDIR_MINGW/gdb && mkdir -p $BUILDDIR_MINGW/gdb
	pushd $BUILDDIR_MINGW/gdb
	saveenv
	saveenvvar CFLAGS "-I$BUILDDIR_MINGW/host-libs/zlib/include -O2 -Wno-error"
	saveenvvar CPPFLAGS "-I$BUILDDIR_MINGW/host-libs/zlib/include"
	saveenvvar LDFLAGS "-L$BUILDDIR_MINGW/host-libs/zlib/lib"
        if [[ "$skip_config" != "yes" ]]; then
	$SRCDIR/$GDB/configure --build=$BUILD \
	    --host=$HOST_MINGW \
	    --target=$TARGET \
	    --prefix=$INSTALLDIR_MINGW \
	    --bindir=$INSTALLDIR_MINGW/bin/bin \
	    --infodir=$INSTALLDIR_MINGW_DOC/info \
	    --mandir=$INSTALLDIR_MINGW_DOC/man \
	    --htmldir=$INSTALLDIR_MINGW_DOC/html \
	    --pdfdir=$INSTALLDIR_MINGW_DOC/pdf \
	    --disable-build-with-cxx \
	    --disable-nls \
	    --disable-sim \
	    --disable-gas \
	    --disable-binutils \
	    --disable-ld \
	    --disable-gprof \
	    --with-lzma=no \
	    $MINGW_GDB_CONF_OPTS \
	    --with-libexpat \
	    --with-libexpat-prefix=$BUILDDIR_MINGW/host-libs/usr \
	    --with-libiconv-prefix=$BUILDDIR_MINGW/host-libs/usr \
	    --with-system-gdbinit=$INSTALLDIR_MINGW/$HOST_MINGW/$TARGET_DIR/lib/gdbinit \
	    '--with-gdb-datadir='\''${prefix}'\''/${TARGET_DIR}/share/gdb' \
	    "--with-pkgversion=$PKGVERSION" >>$LOGTASKFILE 2>&1
        fi
	if [ "x$DEBUG_BUILD_OPTIONS" != "x" ] ; then
	    make CFLAGS="-I$BUILDDIR_MINGW/host-libs/zlib/include $DEBUG_BUILD_OPTIONS" -j$JOBS >>$LOGTASKFILE 2>&1
	else
	    make -j$JOBS >>$LOGTASKFILE 2>&1
	fi

	make install -j2
	if [ "x$skip_manual" != "xyes" ]; then
		make install-html install-pdf
	fi

	restoreenv
	popd
}

build_mingw_gdb "--with-python=no"

if [ "x$skip_mingw32_gdb_with_python" == "xno" ]; then
    if [ "x$build_tools" == "x" ]; then
	build_mingw_gdb "--with-python=$script_path/python-config.sh --program-suffix=-py --program-prefix=$TARGET-"
    else
	build_mingw_gdb "--with-python=$build_tools_abs_path/python-config.sh --program-suffix=-py --program-prefix=$TARGET-"
    fi
fi

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-IV-5.txt
status_update "  Task [IV-5] /$HOST_MINGW/pretidy/"
echo Task [IV-5] /$HOST_MINGW/pretidy/
pushd $INSTALLDIR_MINGW
rm -rf ./lib/libiberty.a
rm -rf $INSTALLDIR_MINGW_DOC/info
rm -rf $INSTALLDIR_MINGW_DOC/man

find $INSTALLDIR_MINGW -name '*.la' -exec rm '{}' ';'

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-IV-6.txt
status_update "  Task [IV-6] /$HOST_MINGW/strip_host_objects/"
echo Task [IV-6] /$HOST_MINGW/strip_host_objects/
STRIP_BINARIES=`find $INSTALLDIR_MINGW/bin/ -name $TARGET-\*.exe`
if [ "x$DEBUG_BUILD_OPTIONS" = "x" ] ; then
    for bin in $STRIP_BINARIES ; do
        strip_binary $HOST_MINGW_TOOL-strip $bin
    done

    STRIP_BINARIES=`find $INSTALLDIR_MINGW/$TARGET_DIR/bin/ -maxdepth 1 -mindepth 1 -name \*.exe`
    for bin in $STRIP_BINARIES ; do
        strip_binary $HOST_MINGW_TOOL-strip $bin
    done

    STRIP_BINARIES=`find $INSTALLDIR_MINGW/lib/gcc/$TARGET_DIR/$GCC_VER/ -name \*.exe`
    for bin in $STRIP_BINARIES ; do
        strip_binary $HOST_MINGW_TOOL-strip $bin
    done
fi

#LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-IV-7.txt
#status_update "  Task [IV-7] /$HOST_MINGW/installation/"
#echo Task [IV-7] /$HOST_MINGW/installation/
#rm -f $PACKAGEDIR/$PACKAGE_NAME_MINGW.exe
#pushd $BUILDDIR_MINGW
#rm -f $INSTALL_PACKAGE_NAME
#cp $ROOT/$RELEASE_FILE $INSTALLDIR_MINGW_DOC/
#cp $ROOT/$README_FILE $INSTALLDIR_MINGW_DOC/
#cp $ROOT/$LICENSE_FILE $INSTALLDIR_MINGW_DOC/
#copy_dir_clean $SRCDIR/$SAMPLES $INSTALLDIR_MINGW/share/gcc-$TARGET_DIR/$SAMPLES
#flip -m $INSTALLDIR_MINGW_DOC/$RELEASE_FILE
#flip -m $INSTALLDIR_MINGW_DOC/$README_FILE
#flip -m -b $INSTALLDIR_MINGW_DOC/$LICENSE_FILE
#flip -m $INSTALLDIR_MINGW/share/gcc-$TARGET_DIR/$SAMPLES_DOS_FILES
#rm -rf $INSTALLDIR_MINGW/include
#ln -s $INSTALLDIR_MINGW $INSTALL_PACKAGE_NAME

#cp $SRCDIR/$INSTALLATION/gccvar.bat $INSTALLDIR_MINGW/bin
#mkdir -p $SRCDIR/$INSTALLATION/output
#makensis -DBaseDir=$INSTALLDIR_MINGW  \
#         -DAppName="$APPNAME" \
#         -DAppNameStr="$PKGVERSION"   \
#         -DPackageName=$PACKAGE_NAME_MINGW   \
#         -DInstallDirBase="$INSTALLBASE"   \
#         -DInstallDirVer="$GCC_VER_SHORT $RELEASEVER" \
#         "-XOutFile $SRCDIR/$INSTALLATION/output/$PACKAGE_NAME_MINGW.exe"    \
#         $SRCDIR/$INSTALLATION/$TARGET_DIR-gnu-tools.nsi

#cp -rf $SRCDIR/$INSTALLATION/output/$PACKAGE_NAME_MINGW.exe $PACKAGEDIR/
#rm -f $INSTALL_PACKAGE_NAME
#popd


restoreenv


#LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-IV-8.txt
#status_update "  Task [IV-8] /Package toolchain in zip format/"
#echo Task [IV-8] /Package toolchain in zip format/
#pushd $INSTALLDIR_MINGW
#rm -f $PACKAGEDIR/$PACKAGE_NAME_MINGW.zip
#pushd $PACKAGEDIR && ln -s $INSTALLDIR_MINGW $INSTALL_PACKAGE_NAME
#zip -r9 $PACKAGEDIR/$PACKAGE_NAME_MINGW.zip $INSTALL_PACKAGE_NAME
#rm -rf $INSTALL_PACKAGE_NAME && popd
#popd

LOGTASKFILE=$WORKING_DIR/log-build-pic32c-Task-IV-7-pic32c-libs.txt
echo Task [IV-7] /$HOST_NATIVE/pic32c-libs/ > $LOGTASKFILE
echo Task [IV-7] /$HOST_NATIVE/pic32c-libs/
status_update "  Task [IV-7] /$HOST_NATIVE/pic32c-libs/"
saveenv
prepend_path PATH $INSTALLDIR_NATIVE_NOLM/bin
prepend_path PATH $INSTALLDIR_NATIVE_NOLM/bin/bin
saveenvvar CFLAGS_FOR_TARGET '-g -Os -ffunction-sections -fdata-sections'
saveenvvar CXXCPP '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar AR_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ar'
saveenvvar AS_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-as'
saveenvvar LD_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ld'
saveenvvar GCC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar XGCC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar CC_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc'
saveenvvar CPP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar CPPCXX_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-gcc -E'
saveenvvar CXX_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-g++'
saveenvvar NM_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-nm'
saveenvvar RANLIB_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-ranlib'
saveenvvar STRIP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-strip'
saveenvvar OBJDUMP_FOR_TARGET '$INSTALLDIR_NATIVE_NOLM/bin/bin/$TARGET-objdump'

pushd $BUILDDIR_NATIVE_NOLM/pic32c-libs >>$LOGTASKFILE 2>&1

make DESTROOT="$INSTALLDIR_MINGW" install >>$LOGTASKFILE 2>&1
assert_success $? "ERROR: pic32c-libs install INSTALLDIR_MINGW failed"

# Copy only headers, libs, specs, and startup/init code from previous
# builds into mingw installation.
xc32_eval "rsync --archive --quiet --cvs-exclude --prune-empty-dirs \
              --include=*.h --include=*.a --include=*.specs --include=*crt*.o \
              --exclude=examples/ --include=*/c++/** --include=*/ --exclude=* \
              \"$INSTALLDIR_NATIVE_NOLM/\" \"$INSTALLDIR_MINGW\""

popd
restoreenv

fi #end of if [ "x$skip_mingw32" != "xyes" ] ;

#LOGTASKFILE=$WORKING_DIR/log-build-pic32c-TaskIV.txt
# status_update "  Task [V-0] /package_sources/"
#echo Task [V-0] /package_sources/
#pushd $PACKAGEDIR
#rm -rf $PACKAGE_NAME && mkdir -p $PACKAGE_NAME/src
#cp -f $SRCDIR/$CLOOG_PACK $PACKAGE_NAME/src/
#cp -f $SRCDIR/$EXPAT_PACK $PACKAGE_NAME/src/
#cp -f $SRCDIR/$GMP_PACK $PACKAGE_NAME/src/
#cp -f $SRCDIR/$LIBELF_PACK $PACKAGE_NAME/src/
#cp -f $SRCDIR/$LIBICONV_PACK $PACKAGE_NAME/src/
#cp -f $SRCDIR/$MPC_PACK $PACKAGE_NAME/src/
#cp -f $SRCDIR/$MPFR_PACK $PACKAGE_NAME/src/
#cp -f $SRCDIR/$ISL_PACK $PACKAGE_NAME/src/
#cp -f $SRCDIR/$ZLIB_PACK $PACKAGE_NAME/src/
#pack_dir_clean $SRCDIR $BINUTILS $PACKAGE_NAME/src/$BINUTILS.tar.bz2
#pack_dir_clean $SRCDIR $GCC $PACKAGE_NAME/src/$GCC.tar.bz2
#pack_dir_clean $SRCDIR $GDB $PACKAGE_NAME/src/$GDB.tar.bz2 \
#  --exclude="gdb/testsuite/config/qemu.exp" --exclude="sim"
#pack_dir_clean $SRCDIR $NEWLIB $PACKAGE_NAME/src/$NEWLIB.tar.bz2
#pack_dir_clean $SRCDIR $SAMPLES $PACKAGE_NAME/src/$SAMPLES.tar.bz2
#pack_dir_clean $SRCDIR $BUILD_MANUAL $PACKAGE_NAME/src/$BUILD_MANUAL.tar.bz2
#if [ "x$skip_mingw32" != "xyes" ] ; then
#    pack_dir_clean $SRCDIR $INSTALLATION \
#      $PACKAGE_NAME/src/$INSTALLATION.tar.bz2 \
#      --exclude=build.log --exclude=output
#fi
#cp $ROOT/$RELEASE_FILE $PACKAGE_NAME/
#cp $ROOT/$README_FILE $PACKAGE_NAME/
#cp $ROOT/$LICENSE_FILE $PACKAGE_NAME/
#cp $ROOT/$BUILD_MANUAL_FILE $PACKAGE_NAME/
#cp $ROOT/build-common-gccv6.sh $PACKAGE_NAME/
#cp $ROOT/build-prerequisites-gccv6.sh $PACKAGE_NAME/
#cp $ROOT/build-toolchain-gccv6.sh $PACKAGE_NAME/
#tar cjf $PACKAGE_NAME-src.tar.bz2 $PACKAGE_NAME
#rm -rf $PACKAGE_NAME
#popd

#status_update "  Task [V-1] /md5_checksum/"
#echo Task [V-1] /md5_checksum/
#pushd $PACKAGEDIR
#rm -rf md5.txt
#$MD5 $PACKAGE_NAME_NATIVE.tar.gz     >>md5.txt
#if [ "x$skip_mingw32" != "xyes" ] ; then
#    $MD5 $PACKAGE_NAME_MINGW.exe         >>md5.txt
#    $MD5 $PACKAGE_NAME_MINGW.installjammer.exe >>md5.txt
#    $MD5 $PACKAGE_NAME_MINGW.zip         >>md5.txt
#fi
#$MD5 $PACKAGE_NAME-src.tar.bz2 >>md5.txt
#popd
status_update "COMPLETED build-pic32c-toolchain.sh"
echo "COMPLETED - build-toolshain.sh"

