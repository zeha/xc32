#!/bin/bash

FULLIMAGE=install-Linux-nolm

#Copy part-support to full installer image
cp -r image/* ${FULLIMAGE}/

cat <<EOT >> ./smoke.c
#include <xc.h>
main()
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

rm smoke.c

