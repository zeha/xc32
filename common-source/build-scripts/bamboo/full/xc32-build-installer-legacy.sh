#!/bin/bash

set -e

HOST=`uname -s`

COMPILER_VERSION=""
BUILD_NAME=""

LINUX_IMAGE_DIR="Linux32-image"
DARWIN_IMAGE_DIR="Darwin-image"
WIND_IMAGE_DIR="win32-image"

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

  # Bamboo checksout XC-installers from fossil, so no need for this
  #export CVSROOT=:pserver:bekals@10.10.70.13:/cvs
  #echo "Checking-out BitRock files from cvs"
  #cvs co -d install installers/BitRock

  BITROCK_PROJECT=xc32_integrity.xml
 
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
    check_dir ${!VAR}
    cp -r install ${!VAR}

    if [[ $VAR =~ LINUX ]]; then
      #unzip -d xclm xclm_release/dist/linux.zip 
      #echo "Moving xclm files to Linux image"
      #mv xclm ${!VAR}
      cd ${!VAR}/install
      echo "Building Linux installer"
      $BITRKPATH build $BITROCK_PROJECT linux --parallel-compression-cores 2 --setvars project.version=$FULL_VERSION  
      check_file xc32-${FULL_VERSION}-linux-installer.run
      cp xc32-${FULL_VERSION}-linux-installer.run $WORK_DIR
      if [[ $BUILD_NAME =~ RC ]]; then 
        mv $WORK_DIR/xc32-${FULL_VERSION}-linux-installer.run  $WORK_DIR/xc32-${FULL_VERSION}-${BUILD_NAME}-linux-installer.run
      fi 
    fi        
    if [[ $VAR =~ WIND ]]; then
      #unzip -d xclm xclm_release/dist/windows.zip 
      #echo "Moving xclm files to Win image"
      ## workaround start ####
      #echo "Copying doc folder from linux.zip"
      #unzip -d xclm_linux xclm_release/dist/linux.zip 
      #rm -rf xclm/install/client/doc
      #mv xclm_linux/install/client/doc xclm/install/client
      #rm -rf xclm_linux
      ## workaround end ######
      #mv xclm ${!VAR}
      cd ${!VAR}/install
      echo "Building Windows installer"
      $BITRKPATH build $BITROCK_PROJECT windows --parallel-compression-cores 2 --setvars project.version=$FULL_VERSION      
      check_file xc32-${FULL_VERSION}-windows-installer.exe
      cp xc32-${FULL_VERSION}-windows-installer.exe $WORK_DIR
      if [[ $BUILD_NAME =~ RC ]]; then 
        mv $WORK_DIR/xc32-${FULL_VERSION}-windows-installer.exe  $WORK_DIR/xc32-${FULL_VERSION}-${BUILD_NAME}-windows-installer.exe
      fi 
    fi
    if [[ $VAR =~ DARWIN ]]; then
      #unzip -d xclm xclm_release/dist/mac.zip 
      #echo "Moving xclm files to Darwin image"
      #mv xclm ${!VAR}
      cd ${!VAR}/install
      echo "Building OS X installer"
      $BITRKPATH build $BITROCK_PROJECT osx --parallel-compression-cores 2 --setvars project.version=$FULL_VERSION  
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

processopts $1 $2 $3
buildinstallers
