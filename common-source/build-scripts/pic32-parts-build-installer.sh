#!/bin/bash

#Script to create part support isntaller using BirRock

# Get common directory names from  pic32-parts-inc.sh 
source `dirname $0`/pic32-parts-inc.sh

makedirs() 
{
  # Create log dir if it does not exists 
  if [ ! -d "$LOGDIR" ]; then
    mkdir $LOGDIR
  fi

  log "Creating/cleaning installer output directory"

  rm -rf $INSTOUTDIR
  mkdir $INSTOUTDIR
 
}

checkbitrock () 
{
  log "Determining the OS on this machine"
  # Determine BitRock based on OS
  if [ "$HOST" = "Darwin" ]; then 
    BITRKPATH=/Applications/BitRock/bin/Builder.app/Contents/MacOS/installbuilder.sh 
  elif [ "$HOST" = "Linux" ]; then
    BITRKPATH=/opt/installbuilder/bin/builder
  else 
    false
    checkreturn $?  "Could not determine OS" ${LINENO}
  fi 

  log "Checking if BitRock exists"
  if [ ! -f "$BITRKPATH" ]; then
    checkreturn $? "${BITRKPATH} does not exist" ${LINENO}
  fi
}
 
buildinst()
{
  cd $IMAGEDIR
  
  # Did Bamboo already checkout the installer repository?
  if [ ! -e ../${FOREPBITRK} ]; then
    # We need to check it out from fossil
    log "Checking-out ${FOREPBITRK} from fossil"
    if [ "$HOST" = "Darwin" ]; then
      DIR=`echo $BITOUTDIR | sed -Ee "s/^(.+\/)*//"`
    else
      DIR=`echo $BITOUTDIR | sed -re "s/^(.+\/)*//"`
    fi

    fossil tree ${FOREPBITRK} > ${LOGDIR}/fossil.BitRock.log
    checkreturn $? "Checking-out ${FOREPBITRK} failed" $LINENO

    echo mv ${FOREPBITRK} $DIR >> ${LOGDIR}/fossil.BitRock.log
    mv ${FOREPBITRK} $DIR

  else
    log "Copying BitRock checked out by Bamboo"
    cp -r ../${FOREPBITRK} ${BITOUTDIR}
    checkreturn $? "Copying ${FOREPBITRK} to ${IMAGEDIR} failed"
  fi
  cd ${BITOUTDIR}
  
  #Delete any previous isntallers
  rm -rf xc32parts-v* &> /dev/null

  if [ "$MCHP_RESOURCE" = "A" ]; then
    MCHP_RESOURCE=""
  fi

  if [ ! -z "$MCHP_BUILD" ]; then
    MCHP_BUILD=-${MCHP_BUILD}
  fi

  if [ -z "$BUILDDATE" ]; then
    BUILDDATE=`date "+%Y-%m-%d"`
    checkreturn $? "Failed to get today's date" $LINENO
  else
    echo "Build date as set by enviornment variable is $BUILDDATE"
  fi

  log "Building update-installer for Linux"
  ${BITRKPATH} build xc32_parts-v2.xml linux --setvars project.version=v${MCHP_VERSION}${MCHP_RESOURCE} builddate=${BUILDDATE}
  checkreturn $? "Linux update-installer build failed" $LINENO
  LINUXINST=xc32-v${MCHP_VERSION}${MCHP_RESOURCE}-part-support-linux-installer.run 
  checkfile ${BITOUTDIR}/${LINUXINST} $LINENO
  if [ ! -z "$MCHP_BUILD" ]; then
    LINUXINST_B=xc32-v${MCHP_VERSION}${MCHP_RESOURCE}${MCHP_BUILD}-part-support-linux-installer.run 
  else
    LINUXINST_B=$LINUXINST
  fi
  cp ${BITOUTDIR}/${LINUXINST} ${INSTOUTDIR}/${LINUXINST_B}
  checkreturn $? "Failed to copy Linux installer to ${INSTOUTDIR}" $LINENO
 
# No need to tar linux isntaller 
#  cd  ${INSTOUTDIR}
#  tar -cvf ${LINUXINST_B}.tar ${LINUXINST_B} 
#  checkreturn $? "Failed to tar Linux installer" $LINENO
#  rm  ${LINUXINST_B}
#  cd -

  log "Building update-installer for OS X"
  ${BITRKPATH} build xc32_parts-v2.xml osx  --setvars project.version=v${MCHP_VERSION}${MCHP_RESOURCE} builddate=${BUILDDATE}
  checkreturn $? "OS X update installer build failed" $LINENO
  OSXINST=xc32-v${MCHP_VERSION}${MCHP_RESOURCE}-part-support-osx-installer.app 
  checkfile ${BITOUTDIR}/${OSXINST} $LINENO
  if [ "$HOST" == "Darwin" ]; then 
    if [ ! -e "xc32-v${MCHP_VERSION}${MCHP_RESOURCE}-part-support-osx-installer.dmg" ]; then
     log "Creating disk image file of OS X update installer"
     chmod +x ${BITOUTDIR}/mkdmg.sh
     ${BITOUTDIR}/mkdmg.sh ${BITOUTDIR}/${OSXINST}     
    else 
     echo "Using .dmg file created by BitRock"
    fi
    OSXINST=xc32-v${MCHP_VERSION}${MCHP_RESOURCE}-part-support-osx-installer.dmg
    checkfile ${BITOUTDIR}/${OSXINST} $LINENO
    if [ ! -z "$MCHP_BUILD" ]; then
      OSXINST_B=xc32-v${MCHP_VERSION}${MCHP_RESOURCE}${MCHP_BUILD}-part-support-osx-installer.dmg
    else
      OSXINST_B=$OSXINST
    fi
    cp        ${BITOUTDIR}/${OSXINST} ${INSTOUTDIR}/${OSXINST_B} 
  else
    log "Creating .dmg using genisoimage on Linux"
    if [ ! -z "$MCHP_BUILD" ]; then
      OSXINST_B=xc32-v${MCHP_VERSION}${MCHP_RESOURCE}${MCHP_BUILD}-part-support-osx-installer.dmg
    else
      OSXINST_B=xc32-v${MCHP_VERSION}${MCHP_RESOURCE}-part-support-osx-installer.dmg
    fi
    mkdir dummy && cp -r ${OSXINST} dummy
    echo "genisoimage -D -V XC32-parts-installer -no-pad -r -apple -o ${OSXINST_B} dummy"
    genisoimage -D -V XC32-parts-installer -no-pad -r -apple -o ${OSXINST_B} dummy
    checkfile ${BITOUTDIR}/${OSXINST_B} $LINENO
    cp -r ${BITOUTDIR}/${OSXINST_B} ${INSTOUTDIR}/
  fi
  checkreturn $? "Failed to copy OS X installer to ${INSTOUTDIR}" $LINENO

  log "Building update-installer for Windows"
  ${BITRKPATH} build xc32_parts-v2.xml windows --setvars project.version=v${MCHP_VERSION}${MCHP_RESOURCE} builddate=${BUILDDATE}
  checkreturn $? "Windos update-installer build failed" $LINENO
  WININST=xc32-v${MCHP_VERSION}${MCHP_RESOURCE}-part-support-windows-installer.exe 
  checkfile ${BITOUTDIR}/${WINSINT} $LINENO
  if [ ! -z "$MCHP_BUILD" ]; then
    WININST_B=xc32-v${MCHP_VERSION}${MCHP_RESOURCE}${MCHP_BUILD}-part-support-windows-installer.exe 
  else
    WININST_B=$WININST
  fi
  cp ${BITOUTDIR}/${WININST} ${INSTOUTDIR}/${WININST_B}
  checkreturn $? "Failed to copy Windows installer to ${INSTOUTDIR}" $LINENO

  #Copy readme to installer folder
  cp ${IMAGEDIR}/docs/XC32*.html ${INSTOUTDIR}
  checkreturn $? "Failed to copy README to installer dir" $LINENO

  log "Installers copied to ${INSTOUTDIR}"
}

#Script execution starts here

processopts $1 $2 $3 $4
makedirs
checkbitrock
buildinst
log " $0 ALL OK!"
