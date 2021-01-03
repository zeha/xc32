#!/bin/bash

SCRIPTROOT=`dirname $0`
source ${SCRIPTROOT}/pic32-parts-inc.sh

processopts $1 $2 $3 $4

if [ "$MCHP_RESOURCE" != "A" ]; then
 ${SCRIPTROOT}/pic32-parts-gen.sh -v$MCHP_VERSION -r$MCHP_RESOURCE -t$VCSBRATAG
 checkreturn $? "pic32-parts-gen.sh returned non-zero value, bailing out"
 ${SCRIPTROOT}/pic32-parts-create-image.sh -v$MCHP_VERSION -r$MCHP_RESOURCE -t$VCSBRATAG
 checkreturn $? "pic32-parts-create-image.sh returned non-zero value, bailing out"
else
 ${SCRIPTROOT}/pic32-parts-create-A-image.sh -v$MCHP_VERSION -r$MCHP_RESOURCE -t$VCSBRATAG
 checkreturn $? "pic32-parts-create-A-image.sh returned non-zero value, bailing out"
fi

if [ ! -z "$MCHP_BUILD" ]; then 
  ${SCRIPTROOT}/pic32-parts-build-installer.sh -v$MCHP_VERSION -r$MCHP_RESOURCE -b$MCHP_BUILD -t$VCSBRATAG
else
  ${SCRIPTROOT}/pic32-parts-build-installer.sh -v$MCHP_VERSION -r$MCHP_RESOURCE -t$VCSBRATAG
fi
checkreturn $? "pic32-parts-build-installer.sh returned non-zero value, bailing out"

