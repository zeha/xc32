#!/bin/bash

# This script calls pic32-parts-[create-image{-A},build-installer,upload].sh 
# This script (stage) has to be executed on typhoon due to following dependencies 
#   create-image requires un-licensed XC32 which is avialable on typhoon 
#   buiil-installer requires BitRock which is available only on typhoon

# This script requires the following enviornment variables to be defined by the plan
# XC32_PARTS_VERSION
# XC32_PARTS_RESOURCE
# XC32_PARTS_BUILD (optional)
# XC32_PARTS_VCSBRATAG (optional)

SCRIPTROOT=`dirname $0`
#Include xc32-parts-bamboo.inc.sh to set fossil env variables
source ${SCRIPTROOT}/xc32-parts-bamboo-inc.sh

SCRIPTARGS="-v${bamboo_XC32_PARTS_VERSION} -r${bamboo_XC32_PARTS_RESOURCE}" 

if [ ! -z $bamboo_XC32_PARTS_VCSBRATAG ]; then
  SCRIPTARGS=${SCRIPTARGS}" -t$bamboo_XC32_PARTS_VCSBRATAG"
fi

# This stage depends on  image/ log/ and results/ artifacts from the previous one

# It also depends on availability of a licensed XC32 compiler in $PATH to 
# build device-specific library
 
if [ ! -d install-Linux ]; then
  echo "No compiler artifact install-Linux found, check artifact download in stage 2"
  echo "PWD is:"
  pwd
  exit 1
fi
export PATH=`pwd`/install-Linux/bin:`pwd`/install-Linux/bin/bin:$PATH
if [ ! -d ./install-Linux/etc ]; then
  mkdir ./install-Linux/etc
fi
cat <<EOT > ./install-Linux/etc/xclm.conf
<xclm>
        <xclm:LicenseDirectory xclm:path="/opt/microchip/xclm/license/" />
        <xclm:Activation xclm:url="http://keygen.microchip.com" />
        <xclm:Demo xclm:url="http://www.microchip.com/xcdemo/handler.aspx" /> 
</xclm>
EOT


#This can be used to force the use of installed compiler for building device specific library
#export PATH=/home/bamboo/xc32/v1.41/bin:$PATH

if [ "$bamboo_XC32_PARTS_RESOURCE" = "A" ]; then
  echo "${SCRIPTROOT}/../../pic32-parts-create-A-image.sh ${SCRIPTARGS}"
  ${SCRIPTROOT}/../../pic32-parts-create-A-image.sh ${SCRIPTARGS}
  checkexitcode $? "pic32-parts-create-A-image.sh returned non-zero value, bailing out" $LINENO
else
  echo "${SCRIPTROOT}/../../pic32-parts-create-image.sh ${SCRIPTARGS}"
  ${SCRIPTROOT}/../../pic32-parts-create-image.sh ${SCRIPTARGS}
  checkexitcode $? "pic32-parts-create-image.sh returned non-zero value, bailing out" $LINENO
fi

#Smoke test the part-support
source ${SCRIPTROOT}/../shell2junit/sh2ju.sh
source ${SCRIPTROOT}/xc32-parts-bamboo-smoke.sh

if [ ! -z $bamboo_XC32_PARTS_BUILD ]; then
  SCRIPTARGS=${SCRIPTARGS}" -b$bamboo_XC32_PARTS_BUILD"
fi

#Copy build date from deviceSupport.xml if building A patch
if [ "$bamboo_XC32_PARTS_RESOURCE" = "A" ]; then
  BUILDDATE=$(python ${SCRIPTROOT}/getbuilddate.py ${SCRIPTROOT}/../../../image/bin/deviceSupport.xml)
  checkexitcode $? "Failed to get build date from deviceSuport.xml"
  export BUILDDATE
fi

#Provision to override build date from Bamboo env variable
if [ ! -z $bamboo_BUILDDATE ]; then
  export BUILDDATE=$bamboo_BUILDDATE
fi

echo "${SCRIPTROOT}/../../pic32-parts-build-installer.sh ${SCRIPTARGS}"
${SCRIPTROOT}/../../pic32-parts-build-installer.sh ${SCRIPTARGS}
checkexitcode $? "pic32-parts-build-installer.sh returned non-zero value, bailing out" $LINENO

#Prepare extra files (if nightly) and upload to http://compilers page

if [ "$bamboo_XC32_PARTS_RESOURCE" = "0" ]; then
  cat log/py/pic2lang* > installer/pic2lang32bit.output
  cp log/pic2res.log installer/pic2res.output 
  echo "$ERRHTMLREDIRECT"  > installer/errors.html
fi

SCRIPTARGS="-v${bamboo_XC32_PARTS_VERSION} -r${bamboo_XC32_PARTS_RESOURCE}" 

if [ ! -z $bamboo_XC32_PARTS_BUILD ]; then
  SCRIPTARGS=${SCRIPTARGS}" -b$bamboo_XC32_PARTS_BUILD"
fi

echo "${SCRIPTROOT}/../../pic32-parts-upload.sh ${SCRIPTARGS}"
${SCRIPTROOT}/../../pic32-parts-upload.sh ${SCRIPTARGS}
checkexitcode $? "pic32-parts-upload.sh returned non-zero value, bailing out" $LINENO

