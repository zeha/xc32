#!/bin/bash
set -e

# Script to copy part support and info files from full installer
# to update-installer image folder
#

# Get common directory names from  pic32-parts-inc.sh
source `dirname $0`/pic32-parts-inc.sh

# Function to create xc32 isntallation image directories
makeimagedirs ()
{
  # Create log dir if it does not exists
  if [ ! -d "$LOGDIR" ]; then
    mkdir $LOGDIR
  fi

  log "Creating/cleaning image directories"

  # Create image dir
  rm -rf $IMAGEDIR
  mkdir $IMAGEDIR

  cd $IMAGEDIR
  mkdir -p bin/device_files
  mkdir -p bin/device_files/device-specs
  mkdir -p docs/config_docs
  mkdir -p pic32-libs
  mkdir -p pic32-libs/include/lega-c
  mkdir -p pic32-libs/include/proc
  mkdir -p pic32-libs/libpic32
  mkdir -p pic32-libs/proc
  mkdir -p pic32c/include_mcc
  mkdir -p pic32c/include_mcc/proc
  mkdir -p pic32c/include/proc
  mkdir -p pic32c/lib/proc
  mkdir -p pic32mx/bin/device_files
  mkdir -p pic32mx/device_files
  mkdir -p pic32mx/include/lega-c
  mkdir -p pic32mx/include/lega-c/sys
  mkdir -p pic32mx/include/proc
  mkdir -p pic32mx/include/sys
  mkdir -p pic32mx/lib/proc
}

copyparts2image ()
{

  if [ "x${bamboo_XC32_INSTALL_DIR}" = "x" ]; then
    log "{bamboo_}XC32_INSTALL_DIR evn variable not set, assuming default installation location"
    if [ "$HOST" = "Darwin" ]; then
      FULLINSTDIR=/Applications/microchip/xc32
    else
      FULLINSTDIR=/opt/microchip/xc32
    fi
  else
    FULLINSTDIR=$bamboo_XC32_INSTALL_DIR
  fi

  # We dont check that the file exists because the source is full compiler install
  # If copying fails, this script will halt because of set -e

  log "Creating part-installer image from full installation @ $FULLINSTDIR"

  if [ ! -d "$FULLINSTDIR" ]; then
    log "Error: Cannot find $FULLINSTDIR No such directory"
    log "Please install XC32 v${MCHP_VERSION} at $FULLINSTDIR or specify"
    log "installation location using {bamboo_}XC32_INSTALL_DIR env variable"
    exit 1
  fi

  # From v1.43 onwards we started providing pic32-libs.zip
  rm -rf ${FULLINSTDIR}/pic32-libs/pic32m-libs
  rm -rf ${FULLINSTDIR}/pic32-libs/pic32c-libs
  unzip ${FULLINSTDIR}/pic32-libs/pic32m-libs.zip -d ${FULLINSTDIR}/pic32-libs
  unzip ${FULLINSTDIR}/pic32-libs/pic32c-libs.zip -d ${FULLINSTDIR}/pic32-libs

  # Starting in XC32 v2.10, we need to copy libpic32.a since it contains the __pic32_data_init function.
  log "Copying pic32m libpic32.a to image directory"
  pushd ${FULLINSTDIR}/pic32mx
  find . -type d -exec mkdir -p ${IMAGEDIR}/pic32mx/\{\} \;
  find . -name "libpic32.a" -exec cp -v \{\} ${IMAGEDIR}/pic32mx/\{\} \;
  popd

  log "Copy pic32m device-specific header files to image directory"
  cp ${FULLINSTDIR}/pic32mx/include/proc/*.h ${IMAGEDIR}/pic32mx/include/proc/

  log "Copying FULLINSTDIR/pic32mx/lib/proc/<device> dirs image/pic32mx/lib/proc directory"
  cp -r ${FULLINSTDIR}/pic32mx/lib/proc/* ${IMAGEDIR}/pic32mx/lib/proc/

  log "Copying pic32c/include/proc and pic32c/lib/proc to image directory"
  cp -r ${FULLINSTDIR}/pic32c/include/proc/*  ${IMAGEDIR}/pic32c/include/proc/
  cp -r ${FULLINSTDIR}/pic32c/include_mcc/proc/*  ${IMAGEDIR}/pic32c/include_mcc/proc/
  cp -r ${FULLINSTDIR}/pic32c/lib/proc/*      ${IMAGEDIR}/pic32c/lib/proc/

  log "Copying xc.h and xc-pic32m.h to image directory"
  cp ${FULLINSTDIR}/pic32mx/include/xc.h ${IMAGEDIR}/pic32mx/include
  cp ${FULLINSTDIR}/pic32mx/include/xc-pic32m.h ${IMAGEDIR}/pic32mx/include
  cp ${FULLINSTDIR}/pic32c/include/xc.h ${IMAGEDIR}/pic32c/include
  cp ${FULLINSTDIR}/pic32c/include_mcc/xc.h ${IMAGEDIR}/pic32c/include_mcc
  cp ${FULLINSTDIR}/pic32c/include/sam.h ${IMAGEDIR}/pic32c/include
  cp ${FULLINSTDIR}/pic32c/include_mcc/sam.h ${IMAGEDIR}/pic32c/include_mcc


  log "Copying config .html files to image directory"
  cp ${FULLINSTDIR}/docs/PIC32ConfigSet.html ${IMAGEDIR}/docs
  cp ${FULLINSTDIR}/docs/config_docs/*.html ${IMAGEDIR}/docs/config_docs

  log "Copying .info files to image directory"
  cp ${FULLINSTDIR}/bin/xc32_device.info ${IMAGEDIR}/bin
  cp ${FULLINSTDIR}/bin/xc32_device.info ${IMAGEDIR}/pic32mx/bin
  cp -r ${FULLINSTDIR}/bin/device_files/* ${IMAGEDIR}/bin/device_files
  cp -r ${FULLINSTDIR}/bin/device_files/device-specs/* ${IMAGEDIR}/bin/device_files/device-specs
  cp -r ${IMAGEDIR}/bin/device_files/* ${IMAGEDIR}/pic32mx/bin/device_files
  cp -r ${IMAGEDIR}/bin/device_files/* ${IMAGEDIR}/pic32mx/device_files

  log "Copying .xml files to image directory"
  cp ${FULLINSTDIR}/bin/deviceSupport.xml ${IMAGEDIR}/bin
  cp ${FULLINSTDIR}/bin/.LanguageToolSuite ${IMAGEDIR}/bin

  cp ${FULLINSTDIR}/pic32mx/include/sys/l1cache.h ${IMAGEDIR}/pic32mx/include/sys
  cp ${FULLINSTDIR}/pic32mx/include/cp0defs.h ${IMAGEDIR}/pic32mx/include/

  cp ${FULLINSTDIR}/pic32mx/include/cp0defs.h ${IMAGEDIR}/pic32mx/include
  cp ${FULLINSTDIR}/pic32mx/include/lega-c/cp0defs.h ${IMAGEDIR}/pic32mx/include/lega-c

  #Compress pic32-libs
  pushd ${FULLINSTDIR}/pic32-libs
  rm -rf ../pic32-libs_v${bamboo_XC32_PARTS_VERSION}_parts_update.zip
  zip -r ${IMAGEDIR}/pic32-libs/pic32-libs_v${bamboo_XC32_PARTS_VERSION}_parts_update.zip pic32c-libs pic32m-libs
  popd
}

# Script execution starts here
processopts $1 $2 $3 $4
makeimagedirs
copyparts2image
generatereadme
log "$0 ALL OK!"
