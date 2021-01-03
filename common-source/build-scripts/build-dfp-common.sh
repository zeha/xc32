#!/bin/bash

# $1 - PICFILE
is_arm_picfile()
{
  PICFILE=$1
  PICFILENAME=`basename ${PICFILE}`
  UDEVICENAME=`echo $PICFILENAME | tr '[:lower:]' '[:upper:]'`
  UDEVICENAME=`echo $UDEVICENAME | sed -e "s/\.*PIC//g"`
  IS_ARM_PIC=$(grep "arm32bit" ${PICFILE})
  if [[ ! -z "$IS_ARM_PIC" || "$UDEVICENAME" == ATSAM* || "$UDEVICENAME" == SAM* || "$UDEVICENAME" == CEC* || "$UDEVICENAME" == MEC17* || "$UDEVICENAME" == MEC15* || "$UDEVICENAME" == 32C* || "$UDEVICENAME" == PIC32C* ]]; then
    true
  else
    false
  fi
}
