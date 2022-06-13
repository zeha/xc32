#!/bin/bash

# This script does the XC32 DFP content generation for PIC32M-based DFPs.
# It takes the following steps:
#   0. Check input arguments and edc folder with input PIC files
#   1. Setup of crownking py framework
#     1.1 Fetches the crownking.edc.python jar file using Ant and Ivy scripts
#         from the ./py subfolder
#     1.2 Unzips crownking.edc.python.jar file to $PYDIR folder
#     1.3 Copies pic2lang32bit.py and py/pic2res.py to $PYDIR folder
#   2. Call pic32-parts-gen.sh to generate the content
#   3. Checkout of pic32-libs needed for startup routines
#   4. Create the build tree structure (makedirs)
#   5. Copy files to the right places in the build tree (copy_to_packdirs)
#
# Input args
#   -v <MCHP_VERSION>
#   -r <MCHP_RESOURCE>
#   -p <PACK_FOLDER> which contains input *.PIC files
#
# Output
#   XC32 content in <PACK_FOLDER>

SCRIPTROOT=$( cd "$( dirname "$0" )" && pwd )
source $SCRIPTROOT/pic32-parts-inc.sh
source $SCRIPTROOT/build-dfp-common.sh

this_show_usage () 
{
  echo "USAGE: $0 -p <packsdir>"
  echo " -p <packs directory>  Specify path to packs directory"
  exit 1
}

#Function to create/clean build dir
makedirs()
{
  #Create/clean log dir
  rm -rf $LOGDIR
  mkdir -p $PYLOGDIR

  log "Creating/cleaning build directories"
  #Create/clean build dir
  rm -rf $BUILDDIR
  mkdir -p $BUILDDIR
}

# Creates pack directory structure for XC32
#
# Example build tree structure:
# |-- Microchip PIC32MK-MC_DFP
#   ├── Microchip.PIC32MK-MC_DFP.pdsc
#   ├── edc
#   │   ├── PIC32MK0512MCF064.PIC
#   │   ├── PIC32MK0512MCF100.PIC
#   │   ├── PIC32MK0512MCH064.PIC
#       ...
#   ├── include
#   │   ├── proc
#   │   │   ├── p32mk0512mcf064.h
#   │   │   ├── p32mk0512mcf100.h
#   │   │   ├── p32mk0512mch064.h
#           ...
#   │   ├── xc-pic32m.h
#   │   └── xc.h
#   ├── package.content
#   └── xc32
#       ├── 32MK0512MCF064
#       │   ├── configuration.data
#       │   ├── p32MK0512MCF064.ld
#       │   ├── p32MK0512MCF064_pic.ld
#       │   ├── p32mk0512mcf064.S
#       │   ├── p32mk0512mcf064_div.S
#       │   └── p32mk0512mcf064_voff_init.S
#       ├── 32MK0512MCF100
#       │   ├── configuration.data
#       │   ├── p32MK0512MCF100.ld
#       │   ├── p32MK0512MCF100_pic.ld
#       │   ├── p32mk0512mcf100.S
#       │   ├── p32mk0512mcf100_div.S
#       │   └── p32mk0512mcf100_voff_init.S
#       ...
#       ├── device_files
#       │   ├── 32MK0512MCF064.info
#       │   ├── 32MK0512MCF100.info
#           ...
#       ├── startup
#       │   ├── cache-err-exception.S
#       │   ├── crt0.S
#       │   ├── crt0_pic.S
#       │   ├── crti.s
#       │   ├── crtn.s
#       │   ├── crtstuff.c
#       │   ├── debug-exception-return.S
#       │   ├── general-exception.S
#       │   ├── simple-tlb-refill-exception.S
#       │   └── software-debug-break.c
#       └── xc32_device.info
make_packdirs()
{
  PACKSDIR_XC32=${PACKSDIR}/xc32
  PACKSDIR_INCLUDE=${PACKSDIR}/include
  if [ -d "$PACKSDIR/xc32" ]; then
    rm -rf ${PACKSDIR}/xc32
  fi
  if [ -d "$PACKSDIR/include/proc" ]; then
    rm -rf ${PACKSDIR}/include/proc
  fi
  mkdir -p ${PACKSDIR}/include/proc
  mkdir -p ${PACKSDIR}/xc32/device_files
}

# copies generated files to their XC32 location within the DFP folder structure
copy_to_packdirs()
{
  PACKSDIR_XC32=${PACKSDIR}/xc32
  PACKSDIR_INCLUDE=${PACKSDIR}/include
  
  #list of PIC files to be used for
  PICFILELIST=(${PACKSDIR}/*.PIC)

  for PICFILE in "${PICFILELIST[@]}"
  do
    PICFILENAME=`basename $PICFILE`
    # UDEVICENAME is extracted from PICFILENAME which is supposed to have the
    # <part_name>.PIC format
    # LDEVICE name is the lower-case equivalent
    UDEVICENAME=`echo $PICFILENAME | tr '[:lower:]' '[:upper:]'`
    UDEVICENAME=`echo $UDEVICENAME | sed -e "s/\.*PIC//g"`
    LDEVICENAME=`echo $UDEVICENAME | tr '[:upper:]' '[:lower:]'`

    # find if this is an arm device
    if is_arm_picfile "$PICFILE"; then
      # skip if it's an ARM-based device
      continue
    fi
    # copy generated part-support files
    cp -R ${PYOUTDIR}/${UDEVICENAME} ${PACKSDIR}/xc32/${UDEVICENAME}
    # remove unnecessary Makefile
    rm ${PACKSDIR}/xc32/${UDEVICENAME}/Makefile

    # move header files to <packdir>/include/proc folder
    mv ${PACKSDIR}/xc32/${UDEVICENAME}/*.h ${PACKSDIR}/include/proc

    # copy device info files
    cp ${RESOUTDIR}/${UDEVICENAME}.info ${PACKSDIR}/xc32/device_files
  done

  # copy xc.h files
  cp ${PYOUTDIR}/xc.h ${PACKSDIR}/include
  cp ${PYOUTDIR}/xc-pic32m.h ${PACKSDIR}/include
  cp ${RESOUTDIR}/xc32_device.info ${PACKSDIR}/xc32

  # copy startup files
  cp -R ${PIC32M_LIBS_REPO}/libpic32/startup ${PACKSDIR}/xc32/startup
  rm ${PACKSDIR}/xc32/startup/Makefile.inc

  cd ${ROOTDIR}
}

#process command line opts
processopts $1 $2 $3 $4 $5 $6
if [ -z $PACKSDIR ]; then
  this_show_usage 
fi

makedirs

# get crownking.edc.python jar file
# cd ${SCRIPTROOT}/py
# ant setup
# checkreturn $? "Checking-out crownking.edc.python.jar failed" $LINENO
# if [ ! -f ./extlibs/crownking.edc.python.jar ]; then
#   echo "Checking-out crownking.edc.python.jar failed"
#   exit 1
# fi

# unzip crownking.edc.python jar file and copy 
# pic2lang32bit.py and pic2res.py scripts
# mkdir -p $PYDIR
# unzip -d $PYDIR ./extlibs/crownking.edc.python.jar
# cp *.py $PYDIR

cd ${ROOTDIR}
# call pic32-parts-gen.sh for generating DFP content ( -p argument )
${SCRIPTROOT}/pic32-parts-gen.sh -v ${MCHP_VERSION} -r ${MCHP_RESOURCE} -p ${PACKSDIR}
checkreturn $? "Error: pic32-parts-gen.sh -v ${MCHP_VERSION} -r ${MCHP_RESOURCE} -p ${PACKSDIR} failed!" $LINENO

# Checkout pic32m-libs from git for crt0.S
if [ -z $REPO_DIR ]; then
  REPO_DIR=/xc32/repo
fi
PIC32M_LIBS_REPO=${REPO_DIR}/pic32m-libs
PIC32M_LIBS_REPO_BRANCH=master
log "Checking-out ${PIC32M_LIBS_REPO} from git"
cd ${PIC32M_LIBS_REPO}
git checkout ${PIC32M_LIBS_REPO_BRANCH}
checkreturn $? "Checking-out ${PIC32M_LIBS_REPO} failed" $LINENO
GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git pull origin
checkreturn $? "Pulling ${PIC32M_LIBS_REPO} failed" $LINENO

make_packdirs
copy_to_packdirs
log " $0 ALL OK!"
