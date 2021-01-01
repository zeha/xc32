#!/bin/bash

# Script for basic compile and link testing XC32 DFP content using xc32-gcc 
# -mdfp cmdline argument.
# Arguments are:
#   - xc32 install directory e.g. /Applications/microchip/xc32/v2.05
#   - packs directory
show_usage()
{
    echo "USAGE: ./$0 <XC32 install dir> <Packs dir>"
    exit 1
}

# Run generic test and report pass/fail result. Increment num_failures
# on failure. Command output is routed to file .test<n>.log
# Arguments are:
#   n - test number
#   clean/noclean - controls .elf file delete or not
#   s - short description
#   <remainder> command to run to produce output elf binary
generictest() {
  n=$1 
  shift
  do_clean=$1
  shift
  desc=$1
  shift
  cmd=$@
  $cmd -o test$n.elf >.test$n.log 2>&1
  rc=$?
  if [[ $rc != 0 || ! -f test$n.elf ]]; then
    echo "TEST$n $desc: FAIL" 1>&2
    num_failures=$(($num_failures+1))
  else
    echo "TEST$n $desc: PASS" 1>&2
    if [ "x$do_clean" == "xclean" ]; then
        # echo "DO CLEAN"
        rm test$n.elf
    fi
  fi
}

if [[ $# != 2 ]]; then
    show_usage
fi

XC32_INSTALL_DIR=$1/bin
PACKSDIR=$2
WORKDIR=`pwd`/testdir

if [ -d $WORKDIR ]; then
    rm -rf $WORKDIR
fi
mkdir -p $WORKDIR
cd $WORKDIR

# create dummy C test file
if [ ! -f dummy.c ]; then
	cat > dummy.c << EOF
#include <xc.h>
int main()
{
    while(1);
    return 0;
}
EOF
fi

# create dummy CPP test file
if [ ! -f dummy.cpp ]; then
	cat > dummy.cpp << EOF
#include <xc.h>
#include <iostream>

int main()
{
    while(1);
    return 0;
}
EOF
fi

echo "Testing -mdfp option in PIC32M toolchain..."

num_failures=0
num_device_failures=0
test_num=1
for DIR in $PACKSDIR/*
do
    VERSION_DIR=$DIR/__version__
    ATDF_FOLDERS=`find ${VERSION_DIR} -name atdf -type d`
    if [ -z "$ATDF_FOLDERS" ]; then
      echo "Couldn't find any atdf folder in ${PACKSDIR}!"
      show_usage
    fi

    NUM_ERRS=0
    for ATDF_PATH in ${ATDF_FOLDERS[@]}
    do
      PACKS_SUBDIR=${ATDF_PATH}/..
      echo "DFP basedir: $(basename $DIR)"
      echo $ATDF_PATH
      for DEVICE in $ATDF_PATH/*
      do
          PICFILENAME=`basename $DEVICE`
          UDEVICENAME=`echo $PICFILENAME | tr '[:lower:]' '[:upper:]'`
          UDEVICENAME=`echo $UDEVICENAME | sed -e "s/\.*ATDF//g"`
          LDEVICENAME=`echo $UDEVICENAME | tr '[:upper:]' '[:lower:]'`
          
          if [ -d $PACKS_SUBDIR/xc32/$UDEVICENAME ]; then
              echo "  Testing $UDEVICENAME:"

              # test C compile without -mdfp
              $XC32_INSTALL_DIR/xc32-gcc -mprocessor=${UDEVICENAME} dummy.c -o dummy.elf >.devicetest${UDEVICENAME}.nodfp.log 2>&1
              if [[ ! -f dummy.elf ]]; then
                  echo "DEVICE TEST C $device: FAIL" 1>&2
                  num_device_failures=$(($num_device_failures+1))
                  continue
              fi
              rm dummy.elf

              # Basic tests
              generictest $test_num "clean" "Basic -mdfp C test | Device $UDEVICENAME | DFP `basename $DIR`" \
                  $XC32_INSTALL_DIR/xc32-gcc -mprocessor=$UDEVICENAME -mdfp=$VERSION_DIR dummy.c
              test_num=$(($test_num+1))

              # test CPP compile without -mdfp 
              $XC32_INSTALL_DIR/xc32-g++ -mprocessor=${UDEVICENAME} dummy.cpp -o dummy.elf -Wl,--defsym=_min_heap_size=0x400 >.devicetest${UDEVICENAME}.cpp.nodfp.log 2>&1
              if [[ ! -f dummy.elf ]]; then
                  echo "DEVICE TEST CPP $device: FAIL" 1>&2
                  num_device_failures=$(($num_device_failures+1))
                  continue
              fi
              rm dummy.elf

              # CPP test
              generictest $test_num "clean" "Basic -mdfp CPP test | Device $UDEVICENAME | DFP `basename $DIR`" \
                  $XC32_INSTALL_DIR/xc32-g++ -mprocessor=$UDEVICENAME -mdfp=$VERSION_DIR dummy.cpp -Wl,--defsym=_min_heap_size=0x400
              test_num=$(($test_num+1))
          fi
      done
    done
done

echo "DONE"
echo ""
echo "Num failures: $num_failures" 1>&2
echo "Num device failures: $num_device_failures" 1>&2