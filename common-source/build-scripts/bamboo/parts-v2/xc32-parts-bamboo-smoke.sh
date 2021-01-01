#!/bin/bash

HOST=`uname -s`

FULLIMAGE=install-Linux

#Copy part-support to full installer image
echo "xc32-parts-bamboo.smoke.sh: Copying part-support files to full installer image"
cp -r ./image/* ${FULLIMAGE}/
if [ $? != 0 ]; then
   echo "ERROR: Failed to copy part-support files to ${FULLIMAGE}/"
   exit 1
fi

rm -rf smoke.c
cat <<EOT >> ./smoke.c
#include <xc.h>
int main(void)
{
  volatile int a;
  a = 24;
  //PORTB=24;
}

EOT


for P in ./image/pic32mx/lib/proc/*
do
   if [[ ! $P =~ GENERIC ]]; then
    PROC=`echo $P | sed -e "s/^.*proc\///"`
    juLog -name="Smoke test ${PROC} support files" -ierror=warning -ierror=error -ierror=cannot ${FULLIMAGE}/bin/xc32-gcc -mprocessor=$PROC smoke.c
    rm a.out
   fi
done

if [ -e ./image/pic32c/lib/proc ] ; then
  for P in ./image/pic32c/lib/proc/*
  do
     PROC=`echo $P | sed -e "s/^.*proc\///"`
     PROC=`echo $PROC | sed -e "s/PIC32/32/"`
     juLog -name="Smoke test ${PROC} support files" -ierror=warning -ierror=error -ierror=cannot ${FULLIMAGE}/bin/xc32-gcc -mprocessor=$PROC smoke.c
     rm a.out
  done
fi


rm smoke.c

