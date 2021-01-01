#!/bin/bash

HOST=$(uname -s)

if [ "$HOST" = "Darwin" ]; then
 FULLIMAGE=install-Darwin
else
 FULLIMAGE=install-Linux
fi

#Copy part-support to full installer image
tar -xzf ${FULLIMAGE}.tar.gz
if [ $? != 0 ]; then
   echo "ERROR: xc32-full-bamboo-smoke-parts.sh - Extracting ${FULLIMAGE}.tar.gz"
   exit 1
fi
cp -r ./image/pic32-libs/* ${FULLIMAGE}/pic32-libs/
if [ $? != 0 ]; then
   echo "ERROR: xc32-full-bamboo-smoke-parts.sh - Failed to copy part-support files to ${FULLIMAGE}/pic32-libs"
   exit 1
fi
cp -r ./image/pic32mx/* ${FULLIMAGE}/pic32mx/
if [ $? != 0 ]; then
   echo "ERROR: xc32-full-bamboo-smoke-parts.sh - Failed to copy part-support files to ${FULLIMAGE}/pic32mx"
   exit 1
fi

echo "xc32-full-bamboo-smoke-parts.sh -- Copying ./image/pic32mx/* to ../../${FULLIMAGE}/pic32mx/"
pushd ${FULLIMAGE}/pic32mx/lib/proc
find . -name "*.ld" -delete
find . -name "*.S" -delete
popd
pushd ./image/pic32mx
find . -type d -exec echo mkdir -p ${ROOTDIR}/${FULLIMAGE}/pic32mx/\{\} \;
find . -type d -exec mkdir -p ${ROOTDIR}/${FULLIMAGE}/pic32mx/\{\} \;
find . -type f -exec echo cp \{\} ${ROOTDIR}/${FULLIMAGE}/pic32mx/\{\} \;
find . -type f -exec cp \{\} ${ROOTDIR}/${FULLIMAGE}/pic32mx/\{\} \;
popd 

#Set xclm.conf
if [ "$HOST" = "Darwin" ]; then
  if [ ! -d $FULLIMAGE ]; then
    echo "No compiler artifact $FULLIMAGE found, check articaft downlod in stage 1"
    echo "PWD is:"
    pwd
    exit 1
  fi 
  export PATH=`pwd`/${FULLIMAGE}/bin:$PATH
  if [ ! -d ./${FULLIMAGE}/etc ]; then
    mkdir ./${FULLIMAGE}/etc
  fi
cat <<EOT > ./${FULLIMAGE}/etc/xclm.conf
<xclm>
        <xclm:LicenseDirectory xclm:path="/Library/Application Support/microchip/xclm/license/" />
        <xclm:Activation xclm:url="http://keygen.microchip.com" />
        <xclm:Demo xclm:url="http://www.microchip.com/xcdemo/handler.aspx" /> 
</xclm>
EOT
else
  if [ ! -d $FULLIMAGE ]; then
    echo "No compiler artifact $FULLIMAGE found, check artifact download in stage 1"
    echo "PWD is:"
    pwd
    exit 1
  fi
  export PATH=`pwd`/${FULLIMAGE}/bin:$PATH
  if [ ! -d ./${FULLIMAGE}/etc ]; then
    mkdir ./${FULLIMAGE}/etc
  fi
cat <<EOT > ./${FULLIMAGE}/etc/xclm.conf
<xclm>
        <xclm:LicenseDirectory xclm:path="/opt/microchip/xclm/license/" />
        <xclm:Activation xclm:url="http://keygen.microchip.com" />
        <xclm:Demo xclm:url="http://www.microchip.com/xcdemo/handler.aspx" /> 
</xclm>
EOT
fi

if [[ "x${bamboo_XC32_USE_FAKE_XCLM}" == "xtrue" || "x${bamboo_XC32_USE_FAKE_XCLM}" == "xTRUE" ]]; then
  #Copy fake xclm to full image
  rm ${FULLIMAGE}/bin/xclm
  cp ~/xc32/fxclm/bin/fxclm ${FULLIMAGE}/bin/xclm
fi 

cat <<EOT > ./smoke.c
#include <xc.h>
main()
{
  volatile int a;
  a = 24;
  //PORTB=24;
}

EOT

echo "Starting smoke test"
echo -n "PWD is"
pwd

for P in ./image/pic32-libs/proc/*
do
   if [[ ! $P =~ GENERIC ]]; then
    PROC=`echo $P | sed -e "s/^.*proc\///"`
    juLog -name="Smoke test ${PROC} support files" -ierror=warning ${FULLIMAGE}/bin/xc32-gcc -mprocessor=$PROC smoke.c
    rm a.out
   fi
done

rm smoke.c
