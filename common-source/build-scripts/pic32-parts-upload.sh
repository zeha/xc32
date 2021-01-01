#!/bin/bash

source `dirname $0`/pic32-parts-inc.sh
 
processopts $1 $2 $3 $4

log "Checking-out $FOREPCPAUTO from fossil"
fossilopenup $FOREPCPAUTO
checkreturn $? "Checking-out $FOREPCPAUTO failed"

if [ "$MCHP_BUILD" != "" ]; then
  MCHP_BUILD=_${MCHP_BUILD}
fi

if [ "$MCHP_RESOURCE" = "A" ]; then
  MCHP_RESOURCE="";
else
  MCHP_RESOURCE=_${MCHP_RESOURCE}
fi

if [ "$MCHP_RESOURCE" = "_0" ]; then
  #If nightly 
  BUILDTYPE=part-support
  PSVERSION=build_`date +"%Y%m%d"`
else
  BUILDTYPE=internal
  PSVERSION=v${MCHP_VERSION}${MCHP_RESOURCE}${MCHP_BUILD}_parts_patch
fi

log "Uploading installers at ${INSTOUTDIR} to compilers page"
log "python ${UPSCRIPTDIR}/compauto.py xc32 $BUILDTYPE $PSVERSION ${INSTOUTDIR}/*"

python ${UPSCRIPTDIR}/compauto.py xc32 $BUILDTYPE $PSVERSION ${INSTOUTDIR}/*
checkreturn $? "Failed to upload compilers to http://compilers page" $LINENO

