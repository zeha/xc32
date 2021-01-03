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
    echo "DFP basedir: $(basename $DIR)"
    for DEVICE in $VERSION_DIR/edc/*
    do
        PICFILENAME=`basename $DEVICE`
        UDEVICENAME=`echo $PICFILENAME | tr '[:lower:]' '[:upper:]'`
        UDEVICENAME=`echo $UDEVICENAME | sed -e "s/\.*PIC//g"`
        LDEVICENAME=`echo $UDEVICENAME | tr '[:upper:]' '[:lower:]'`
        
        if [ -d $VERSION_DIR/xc32/$UDEVICENAME ]; then
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
            
            # save-temps test
            generictest $test_num "noclean" "Save temps -mdfp C test | Device $UDEVICENAME | DFP `basename $DIR`" \
                $XC32_INSTALL_DIR/xc32-gcc -mprocessor=$UDEVICENAME -mdfp=$VERSION_DIR dummy.c --save-temps
            if [[ ! -f test${test_num}.elf || ! -f crt0.o || ! -f p${UDEVICENAME}.o || ! -f p${UDEVICENAME}_div.o ]]; then
                echo "TEST$test_num $desc: FAIL" 1>&2
                num_failures=$(($num_failures+1))
            else
                echo "TEST$test_num $desc: PASS" 1>&2
            fi
            test_num=$(($test_num+1))            
            # cleanup
            rm *.elf *.s *.o *.i

            # PIE test
            generictest $test_num "noclean" "PIE -mdfp test | Device $UDEVICENAME | DFP `basename $DIR`" \
                $XC32_INSTALL_DIR/xc32-gcc -fPIC -pie -G0 -mprocessor=$UDEVICENAME -mdfp=$VERSION_DIR dummy.c --save-temps
            if [[ ! -f test${test_num}.elf || ! -f crt0_pic.o || ! -f p${UDEVICENAME}.o || ! -f p${UDEVICENAME}_div.o ]]; then
                echo "TEST$test_num $desc: FAIL" 1>&2
                num_failures=$(($num_failures+1))
            else
                # test ELF file is dyn linked
                file test${test_num}.elf | grep "dynamically linked" >>.test${test_num}.log 2>&1
                rc=$?
                if [[ $rc != 0 ]]; then
                    echo "TEST$test_num $desc: FAIL" 1>&2
                    num_failures=$(($num_failures+1))
                else
                    echo "TEST$test_num $desc: PASS" 1>&2
                fi
            fi
            test_num=$(($test_num+1))
            #cleanup
            rm *.elf *.s *.o *.i

            # user linker script
            cp $VERSION_DIR/xc32/$UDEVICENAME/p${UDEVICENAME}.ld my_linker_script.ld
            generictest $test_num "clean" "User linker script -mdfp C test | Device $UDEVICENAME | DFP `basename $DIR`" \
                $XC32_INSTALL_DIR/xc32-gcc -mprocessor=$UDEVICENAME -mdfp=$VERSION_DIR -T my_linker_script.ld dummy.c
            test_num=$(($test_num+1))
            rm my_linker_script.ld

            # nostartfiles test
            generictest $test_num "noclean" "nostartfiles -mdfp C test | Device $UDEVICENAME | DFP `basename $DIR`" \
                $XC32_INSTALL_DIR/xc32-gcc -mprocessor=$UDEVICENAME -mdfp=$VERSION_DIR -nostartfiles dummy.c --save-temps
            if [[ -f crt0.o ]]; then
                echo "TEST$test_num $desc: FAIL" 1>&2
                num_failures=$(($num_failures+1))
            else
                echo "TEST$test_num $desc: PASS" 1>&2
            fi
            test_num=$(($test_num+1))
            #cleanup
            rm *.elf *.s *.o *.i

            generictest $test_num "noclean" "-mno-default-isr-vectors -mdfp C test | Device $UDEVICENAME | DFP `basename $DIR`" \
                $XC32_INSTALL_DIR/xc32-gcc -mprocessor=$UDEVICENAME -mdfp=$VERSION_DIR -mno-default-isr-vectors dummy.c --save-temps
            if [[ -f p${UDEVICENAME}_div.o ]]; then
                echo "TEST$test_num $desc: FAIL" 1>&2
                num_failures=$(($num_failures+1))
            else
                echo "TEST$test_num $desc: PASS" 1>&2
            fi
            test_num=$(($test_num+1))
            #cleanup
            rm *.elf *.s *.o *.i

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

echo "DONE"
echo ""
echo "Num failures: $num_failures" 1>&2
echo "Num device failures: $num_device_failures" 1>&2