#!/bin/bash

HOST=$(uname -s)

if [ "$HOST" = "Darwin" ]; then
 FULLIMAGE=Darwin-image
else
 FULLIMAGE=Linux32-image
fi

#Copy part-support to full installer image
tar -xzf ${FULLIMAGE}.tar.gz
cp -r image/pic32-libs/* ${FULLIMAGE}/pic32-libs/

#Set xclm.conf
if [ "$HOST" = "Darwin" ]; then
  if [ ! -d Darwin-image ]; then
    echo "No compiler artifact Darwin-image found, check articaft downlod in stage 1"
    echo "PWD is:"
    pwd
    exit 1
  fi 
  export PATH=`pwd`/Darwin-image/bin:$PATH
  if [ ! -d ./Darwin-image/etc ]; then
    mkdir ./Darwin-image/etc
  fi
cat <<EOT > ./Darwin-image/etc/xclm.conf
<xclm>
        <xclm:LicenseDirectory xclm:path="/Library/Application Support/microchip/xclm/license/" />
        <xclm:Activation xclm:url="http://keygen.microchip.com" />
        <xclm:Demo xclm:url="http://www.microchip.com/xcdemo/handler.aspx" /> 
</xclm>
EOT
else
  if [ ! -d Linux32-image ]; then
    echo "No compiler artifact Linux32-image found, check artifact download in stage 1"
    echo "PWD is:"
    pwd
    exit 1
  fi
  export PATH=`pwd`/Linux32-image/bin:$PATH
  if [ ! -d ./Linux32-image/etc ]; then
    mkdir ./Linux32-image/etc
  fi
cat <<EOT > ./Linux32-image/etc/xclm.conf
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
