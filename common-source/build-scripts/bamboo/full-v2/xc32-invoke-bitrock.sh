#!/bin/bash

set -e

HOST=`uname -s`

COMPILER_VERSION=""
BUILD_NAME=""

LINUX_IMAGE_DIR="install-Linux"
DARWIN_IMAGE_DIR="install-Darwin"
WIND_IMAGE_DIR="install-mingw"

check_dir ()
{
  if [ ! -d "$1" ]; then
    echo "Error: Directory $1 does not exist"
    exit 1
  fi
}

check_file ()
{
  if [ ! -e "$1" ]; then
    echo "Error: $1 not generated"
    exit 1
  fi
}

buildinstallers()
{
  WORK_DIR=`pwd`   

  if [ "$HOST" = "Darwin" ]; then 
    BITRKPATH=/Applications/BitRock/bin/Builder.app/Contents/MacOS/installbuilder.sh 
  elif [ "$HOST" = "Linux" ]; then
    BITRKPATH=/opt/installbuilder/bin/builder
  fi 

  BITROCK_PROJECT=xc32-v2.xml
 
  if [[ $COMPILER_VERSION =~ SAFETY ]]; then
    BITROCK_PROJECT=xc32_integrity.xml
  fi
  
  FULL_VERSION=v$COMPILER_VERSION
  if [ ! -z $BUILD_NAME ]; then
    if [[ ! $BUILD_NAME =~ RC ]]; then 
      FULL_VERSION=${FULL_VERSION}-$BUILD_NAME
    fi 
  fi

  check_dir install
  
  for VAR in LINUX_IMAGE_DIR DARWIN_IMAGE_DIR WIND_IMAGE_DIR
  do
  if [[ "x${bamboo_SKIP_DARWIN}" == "xtrue" || "x${bamboo_SKIP_DARWIN}" == "xTRUE" ]]; then
    if [[ "$VAR" == "DARWIN_IMAGE_DIR" ]]; then
      continue
    fi
  fi
    check_dir ${!VAR}
    cp -r install ${!VAR}

    if [[ $VAR =~ LINUX ]]; then
      cd ${!VAR}/install
      echo "Building Linux installer"
      $BITRKPATH build $BITROCK_PROJECT linux-x64 $BITROCK_FLAGS --setvars project.version=$FULL_VERSION
      check_file xc32-${FULL_VERSION}-linux-x64-installer.run
      cp xc32-${FULL_VERSION}-linux-x64-installer.run $WORK_DIR
      if [[ $BUILD_NAME =~ RC ]]; then 
        mv $WORK_DIR/xc32-${FULL_VERSION}-linux-x64-installer.run  $WORK_DIR/xc32-${FULL_VERSION}-${BUILD_NAME}-linux-installer.run
      fi 
    fi        
    if [[ $VAR =~ WIND ]]; then
      cd ${!VAR}/install
      echo "Building Windows installer"
      $BITRKPATH build $BITROCK_PROJECT windows-x64 $BITROCK_FLAGS --setvars project.version=$FULL_VERSION
      check_file xc32-${FULL_VERSION}-windows-x64-installer.exe
      cp xc32-${FULL_VERSION}-windows-x64-installer.exe $WORK_DIR
      if [[ $BUILD_NAME =~ RC ]]; then 
        mv $WORK_DIR/xc32-${FULL_VERSION}-windows-x64-installer.exe  $WORK_DIR/xc32-${FULL_VERSION}-${BUILD_NAME}-windows-installer.exe
      fi 
    fi
    if [[ $VAR =~ DARWIN ]]; then
      cd ${!VAR}/install
      echo "Building OS X installer"
      $BITRKPATH build $BITROCK_PROJECT osx $BITROCK_FLAGS --setvars project.version=$FULL_VERSION  
      check_file xc32-${FULL_VERSION}-osx-installer.app
      if [ ! -e "xc32-${FULL_VERSION}-osx-installer.dmg" ]; then
        if [ "$HOST" == "Darwin" ]; then
          echo "Creating disk image file of OS X update installer"
          chmod +x mkdmg.sh
          ./mkdmg.sh xc32-${FULL_VERSION}-osx-installer.app
          check_file xc32-${FULL_VERSION}-osx-installer.dmg
        else
          echo "Creating .dmg using genisoimage on Linux"
          mkdir dummy 
          mv xc32-${FULL_VERSION}-osx-installer.app dummy
          echo "genisoimage -D -V MPLAB-XC32-Installer -no-pad -r -apple -o xc32-${FULL_VERSION}-osx-installer.dmg dummy"
          genisoimage -D -V MPLAB-XC32-Installer -no-pad -r -apple -o xc32-${FULL_VERSION}-osx-installer.dmg xc32-${FULL_VERSION}-osx-installer.app
          check_file xc32-${FULL_VERSION}-osx-installer.dmg
        fi
      else
          echo "Using .dmg created by BitRock"  
      fi
      cp xc32-${FULL_VERSION}-osx-installer.dmg $WORK_DIR
      if [[ $BUILD_NAME =~ RC ]]; then 
        mv $WORK_DIR/xc32-${FULL_VERSION}-osx-installer.dmg  $WORK_DIR/xc32-${FULL_VERSION}-${BUILD_NAME}-osx-installer.dmg
      fi 
    fi

    cd -
  done

  echo "ALL OK!"
}

show_usage ()
{
  echo "USAGE: $0 -v<compiler-version> -b<build-name> "
  echo "       Specify the following options"
  echo " -v<compiler-version>  Compiler version in xx.yy format"
  echo " -b<build-name>        Build name TCx/RCx (optional)   "
}

processopts ()
{
  FOUND=0
  while getopts v:b: opt
  do
    case "$opt" in
     v)
     TVAL=$OPTARG
     COMPILER_VERSION=$TVAL
     FOUND=1
     ;;
     b)
     TVAL=$OPTARG
     BUILD_NAME=$TVAL
     ;;
     \?) show_usage ;;
    esac
  done 
  if [ "$FOUND" = "0" ]; then
    show_usage
    exit 1
  fi
}

processopts $@
buildinstallers
