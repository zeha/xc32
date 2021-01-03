#!/bin/bash

# This is the “root” script for generating XC32 contents in DFPs.
# Takes as input a valid DFP folder and selects between
# build-pic32m-dfp.sh or  build-pic32c-dfp.sh based on the device 
# family name  written in the <pack>.pdsc file.
#
# args
#   DFP folder
# output
#   XC32 content in the same DFP folder

SCRIPTROOT="$( cd "$( dirname "$0" )" && pwd )"
this_show_usage () 
{
  echo "USAGE: $0 <arm/mips> <pack directory>"
  exit 1
}

# initially does a 'git pull origin' and recalls itself
# with the '--doit' first arg
# DO NOT CALL directly the script with the '--doit' argument
LAST_ARGV="${@: -1}"
if [[ "$LAST_ARGV" != --doit ]]; then
  GIT_SSH_COMMAND="ssh -o StrictHostKeyChecking=no" git -C $SCRIPTROOT pull origin

  bash $0 $@ --doit
  exit 0
fi

# else if --doit arg then execute the work

# get the pack family arg
PACK_FAMILY_ARG=$1
PACK_FAMILY=`echo $PACK_FAMILY_ARG | tr '[:lower:]' '[:upper:]'`

# get packs dir
PACKDIR=$2
if [ -z $PACKDIR ]; then
  this_show_usage 
fi

# set MCHP_VERSION and MCHP_RESOURCE
# FIX ME: most probably these two should not be set here
if [ -z $MCHP_VERSION ]; then
  MCHP_VERSION="2.05"
fi
if [ -z $MCHP_RESOURCE ]; then
  MCHP_RESOURCE="A"
fi

# select PIC32C or PIC32M DFP build scripts based on family argument
if [[ "$PACK_FAMILY" == ARM ]] ; then
  echo "$0: $SCRIPTROOT/build-pic32c-dfp.sh $PACKDIR"
  $SCRIPTROOT/build-pic32c-dfp.sh $PACKDIR
elif [[ "$PACK_FAMILY" == MIPS ]]; then
  echo "$0: $SCRIPTROOT/build-pic32m-dfp.sh -v $MCHP_VERSION -r $MCHP_RESOURCE -p $PACKDIR"
  $SCRIPTROOT/build-pic32m-dfp.sh -v $MCHP_VERSION -r $MCHP_RESOURCE -p $PACKDIR
else
  this_show_usage
fi

chown -R 1000:1000 $PACKDIR
