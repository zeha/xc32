#!/bin/bash


show_usage()
{
 # Shows the usage screen
 echo "USAGE: $0 [-t <tag> | -?]"
 echo " -w <destroot1>     Specify the relative destination root 1 for the prebuilt libraries"
 echo " -m <destroot2>     Specify the relative destination root 2 for the prebuilt libraries"
 echo " -l <destroot3>     Specify the relative destination root 3 for the prebuilt libraries"
 echo " -e <destroot4>     Specify the relative destination root 4 for the prebuilt libraries"
 echo " -?                 Show this usage message"
 exit 1
}

error_handler ()
{
  PATH=$OLDPATH
  cd $OLD_PWD
  exit 1
}

####
# assert_success()
#  If the first parameter is non-zero, print the second parameter
#  and exit the script
####
function assert_success ()
{
 local RESULT=$1
 local MESSAGE=$2
 if [ $RESULT != 0 ]
 then
  echo "$MESSAGE ($RESULT)"
  error_handler
 fi
}

function status_update ()
{
 local MESSAGE=$1
  echo "status: $MESSAGE"
}

### $1 Command
### $2 Error Message
run_eval() {
  echo ""
  echo "directory executed: `pwd`"
  echo "command executed: $1"
  bash -c "$1"
  assert_success $? "$2"
}


####################################################################################
OLDPATH=$PATH
OLD_PWD=$PWD
WINDOWSIMAGE=""
MACIMAGE=""
LINUXIMAGE=""
EXPORTIMAGE=""

# Process the arguments
while getopts :w:m:l:e: opt
do
 case "$opt" in
   w)
   WINDOWSIMAGE=$OPTARG
   ;;
   m)
   MACIMAGE=$OPTARG
   ;;
   l)
   LINUXIMAGE=$OPTARG
   ;;
   e)
   EXPORTIMAGE=$OPTARG
   ;;
   :)
   echo "Option -$OPTARG requires an argument." >&2
   exit 1
   ;;
  \?) show_usage ;;
 esac
done

if [ -e pic32-newlib ] ; then
  echo "Run the xc32-build-newlib.sh script from the newlib build directory (e.g. native-build-nolm/newlib)"
  exit 1
fi

if [ -e xc32-build-newlib.sh ] ; then
  echo "Run the xc32-build-newlib.sh script from the newlib build directory (e.g. native-build-nolm/newlib)"
  exit 1
fi

if [ "x$NATIVE_NOLM_IMAGE" == "x" ] ; then
  NATIVEIMAGE=install-
  NATIVEIMAGE+=`uname`
  if [ "$NATIVEIMAGE" == "install-linux" ] ; then
    NATIVEIMAGE="install-linux32";
  fi
  NATIVE_NOLM_IMAGE=$NATIVEIMAGE
  NATIVE_NOLM_IMAGE+="-nolm"
  INSTALLDIR_NATIVE_NOLM=$ROOT/$NATIVE_NOLM_IMAGE
  INSTALLDIR_NATIVE_NOLM_DOC=$INSTALLDIR_NATIVE_NOLM/share/doc/xc32-pic32m-gcc
fi

if [ "x$WINDOWSIMAGE" != "x" ] ; then
  if [ -e $WINDOWSIMAGE ] ; then
    echo "Windows image: $WINDOWSIMAGE"
  else
    echo "$WINDOWSIMAGE does not exist"
    exit 1
  fi
fi

if [ "x$MACIMAGE" != "x" ] ; then
  if [ -e $MACIMAGE ] ; then
    echo "Mac OS X image: $MACIMAGE"
  else
    echo "$MACIMAGE does not exist"
    exit 1
  fi
fi

if [ "x$LINUXIMAGE" != "x" ] ; then
  if [ -e $LINUXIMAGE ] ; then
    echo "Linux image: $LINUXIMAGE"
  else
    echo "$LINUXIMAGE does not exist"
    exit 1
  fi
fi

if [ "x$EXPORTIMAGE" != "x" ] ; then
  if [ -e $EXPORTIMAGE ] ; then
    echo "Export image: $EXPORTIMAGE"
  else
    echo "$EXPORTIMAGE does not exist"
    exit 1
  fi
fi

if [ "x$WORKING_DIR" == "x" ] ; then
  WORKING_DIR=$PWD/../..
fi

if [ "x$SOURCE_DIR" == "x" ] ; then
  SOURCE_DIR=${WORKING_DIR}/pic32m-source
fi

if [ ! -e $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-gcc ] ; then
  echo "ERROR: $WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-gcc does not exist"
  exit 1
fi

NEWLIB_CONFIGURE_FLAGS="--target=pic32mx --enable-target-optspace --disable-threads --enable-static --disable-libmudflap --disable-libssp --disable-libstdcxx-pch --disable-hosted-libstdcxx --with-arch=pic32mx --enable-sgxx-sde-multilib --with-gnu-as --with-gnu-ld --enable-languages=c,c++ --disable-shared --disable-nls --with-dwarf2 --disable-bootstrap --enable-obsolete --enable-sjlj-exceptions --disable-__cxa_atexit --disable-libfortran --with-bugurl=http://www.microchip.com/support --disable-libgomp --disable-libffi --program-prefix=pic32m- --with-newlib --enable-newlib-io-long-long --disable-newlib-multithread --disable-libgloss --disable-newlib-supplied-syscalls --disable-nls --disable-libunwind-exceptions --enable-libstdcxx-allocator=malloc --disable-newlib-atexit-alloc --disable-libstdcxx-verbose -enable-lto --enable-fixed-point --enable-obsolete --disable-sim --disable-checking"

if [ ! -e config.status ] ; then
    GCC_FOR_TARGET=$SOURCE_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-gcc CC_FOR_TARGET="$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-gcc -I$SOURCE_DIR/src48x/gcc/gcc/ginclude" CXX_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-g++ CPP_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-g++ AR_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-ar RANLIB_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-ranlib READELF_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-readelf STRIP_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin/pic32m-strip AS_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/pic32mx/bin/as LD_FOR_TARGET=$WORKING_DIR/$NATIVE_NOLM_IMAGE/pic32mx/bin/ld $SOURCE_DIR/pic32-newlib/configure $NEWLIB_CONFIGURE_FLAGS  --prefix=$PWD/newlib-temp-image --bindir="$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin" --libexecdir="$WORKING_DIR/$NATIVE_NOLM_IMAGE/bin/bin" CFLAGS_FOR_TARGET="-fno-short-double -mno-smart-io -fno-rtti -fno-exceptions -fomit-frame-pointer -DPREFER_SIZE_OVER_SPEED -D__OPTIMIZE_SIZE__ -O2 -fshort-wchar -fno-enforce-eh-specs -ffunction-sections -fdata-sections -DSMALL_MEMORY -D_NO_GETLOGIN -D_NO_GETPWENT -D_NO_GETUT -D_NO_GETPASS -D_NO_SIGSET" CCASFLAGS_FOR_TARGET="-fno-short-double -mno-smart-io -fno-rtti -fno-exceptions -fomit-frame-pointer -DPREFER_SIZE_OVER_SPEED -D__OPTIMIZE_SIZE__ -O2 -fshort-wchar -fno-enforce-eh-specs -ffunction-sections -fdata-sections -DSMALL_MEMORY -D_NO_GETLOGIN -D_NO_GETPWENT -D_NO_GETUT -D_NO_GETPASS -D_NO_SIGSET" XGCC_FLAGS_FOR_TARGET="-fno-short-double -mno-smart-io -fno-rtti -fno-exceptions -fomit-frame-pointer -DPREFER_SIZE_OVER_SPEED -D__OPTIMIZE_SIZE__ -O2 -fshort-wchar -fno-enforce-eh-specs -ffunction-sections -fdata-sections" --enable-cxx-flags="-fno-short-double -mno-smart-io -fno-rtti -fno-exceptions -fomit-frame-pointer -DPREFER_SIZE_OVER_SPEED -D__OPTIMIZE_SIZE__ -O2 -fshort-wchar -fno-enforce-eh-specs -ffunction-sections -fdata-sections"
else
    echo "Using existing configuration"
fi

make -j4 all CFLAGS_FOR_TARGET="-fno-short-double -mno-smart-io -fno-rtti -fno-exceptions -fomit-frame-pointer -DPREFER_SIZE_OVER_SPEED -D__OPTIMIZE_SIZE__ -O2 -fshort-wchar -fno-enforce-eh-specs -ffunction-sections -fdata-sections -DSMALL_MEMORY -D_NO_GETLOGIN -D_NO_GETPWENT -D_NO_GETUT -D_NO_GETPASS -D_NO_SIGSET" CCASFLAGS_FOR_TARGET="-fno-short-double -mno-smart-io -fno-rtti -fno-exceptions -fomit-frame-pointer -DPREFER_SIZE_OVER_SPEED -D__OPTIMIZE_SIZE__ -O2 -fshort-wchar -fno-enforce-eh-specs -ffunction-sections -fdata-sections -DSMALL_MEMORY -D_NO_GETLOGIN -D_NO_GETPWENT -D_NO_GETUT -D_NO_GETPASS -D_NO_SIGSET" XGCC_FLAGS_FOR_TARGET="-fno-short-double -mno-smart-io -fno-rtti -fno-exceptions -fomit-frame-pointer -DPREFER_SIZE_OVER_SPEED -D__OPTIMIZE_SIZE__ -O2 -fshort-wchar -fno-enforce-eh-specs -ffunction-sections -fdata-sections"

# Remove the old image
rm -rf $PWD/newlib-temp-image

# install the new image
make -j2 install

cd $PWD/newlib-temp-image
rm -rf share

# rename the build libraries to use our Newlib archive naming convention
find . -name "libc.a" -exec bash -c 'mv -f "$0" "${0/libc\.a/libc-newlib.a}"' {} \;
find . -name "libg.a" -exec bash -c 'mv -f "$0" "${0/libg\.a/libg-newlib.a}"' {} \;
find . -name "libm.a" -exec bash -c 'mv -f "$0" "${0/libm\.a/libm-newlib.a}"' {} \;

pushd pic32mx
# Move the include files to a newlib subdirectory
if [ -e include-newlib ] ; then
  rm -rf include-newlib
fi
mkdir -p include-newlib
run_eval "rsync -qavzCr --include=* \"include/\" \"./include-newlib/\""
rm -rf include
mkdir include
mv -f include-newlib include/newlib

popd 

# install the Newlib image into all of the compiler images
if [ "x$WINDOWSIMAGE" != "x" ] ; then
  run_eval "rsync -qavzCr --include=* \"pic32mx/\" \"../$WINDOWSIMAGE/pic32mx/\""
fi

if [ "x$MACIMAGE" != "x" ] ; then
  run_eval "rsync -qavzCr --include=* \"pic32mx/\" \"../$MACIMAGE/pic32mx/\""
fi

if [ "x$LINUXIMAGE" != "x" ] ; then
  run_eval "rsync -qavzCr --include=* \"pic32mx/\" \"../$LINUXIMAGE/pic32mx/\""
fi

if [ "x$EXPORTIMAGE" != "x" ] ; then
  run_eval "rsync -qavzCr --include=* \"pic32mx/\" \"../$EXPORTIMAGE/pic32mx/\""
fi

cd $OLD_PWD
PATH=$OLDPATH

echo "DONE: xc32-build-newlib.sh"
exit 0
