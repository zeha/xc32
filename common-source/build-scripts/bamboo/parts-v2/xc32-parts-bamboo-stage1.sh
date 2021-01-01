#!/bin/bash

# This script calls pic32-parts-gen.sh to generate part-support
# This script (stage) can be executed on any agent as it does not have any specific dependencies 

# This script requires the following enviornment variables to be defined by the plan
# XC32_PARTS_VERSION
# XC32_PARTS_RESOURCE
# XC32_PARTS_VCSBRATAG (optional)

SCRIPTROOT=`dirname $0`
#Include xc32-parts-bamboo.inc.sh to set fossil env variables
source ${SCRIPTROOT}/xc32-parts-bamboo-inc.sh

SCRIPTARGS="-v${bamboo_XC32_PARTS_VERSION} -r${bamboo_XC32_PARTS_RESOURCE}" 

if [ ! -z $bamboo_XC32_PARTS_VCSBRATAG ]; then
  SCRIPTARGS=${SCRIPTARGS}" -t$bamboo_XC32_PARTS_VCSBRATAG"
fi

if [ "$bamboo_XC32_PARTS_RESOURCE" = "A" ]; then
  echo "Skipping part-support generation since revision is A "
else
  echo "${SCRIPTROOT}/../../pic32-parts-gen.sh ${SCRIPTARGS}"
  ${SCRIPTROOT}/../../pic32-parts-gen.sh ${SCRIPTARGS}
  checkexitcode $? "pic32-parts-gen.sh returned non-zero value" $LINENO
fi

# image/ log/ and results folder should be exported as shared artifacts by this stage
