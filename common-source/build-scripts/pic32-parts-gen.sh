#!/bin/bash

SCRIPTROOT=$( cd "$( dirname "$0" )" && pwd )
source $SCRIPTROOT/pic32-parts-inc.sh
source $SCRIPTROOT/build-dfp-common.sh

if [ ! -z $BAMBOO_BUILD ]; then
  source `dirname $0`/bamboo/shell2junit/sh2ju.sh
  juLogClean
fi

#Function to check if pic2lang32bit.py generated all
# the expected files       $1 - .PIC filename
check_gen_parts ()
{
  DIR=$PYOUTDIR
  PICFILENAME=`echo $1 | sed -e "s/^.*32xxxx\///"`

  if [ "x$PACKSDIR" != "x" ]; then
    PICFILENAME=`basename $1`
  fi

  #Some .PIC files might not have their name in upper-case,
  #make sure they are converted to upper-care
  
  UDEVICENAME=`echo $PICFILENAME | tr '[:lower:]' '[:upper:]'`

  #Get rid of .PIC
  UDEVICENAME=`echo $UDEVICENAME | sed -e "s/\.*PIC//g"`

  #Lowercase device name
  LDEVICENAME=`echo $UDEVICENAME | tr '[:upper:]' '[:lower:]'`

  DIR="${DIR}/${UDEVICENAME}"

  if [ "$(ls -A $DIR)" ]; then
  #Check the files is directory is not empty
    #MEC devices will not have procdefs.ld and _voff_init.S files, skip them
    if [[ ! $UDEVICENAME =~ (MEC|32MM) ]]; then
      if [[ $UDEVICENAME =~ 32MZ ]]; then
        #PIC32MZ will not have procdefs.ld, but _voff_init.S instead
        checkfile "${DIR}/p${UDEVICENAME}_voff_init.S" $LINENO
      elif [[ $UDEVICENAME =~ 32MX ]]; then
        checkfile "${DIR}/procdefs.ld" $LINENO
      fi
    fi
  
    checkfile "${DIR}/Makefile" $LINENO
    checkfile "${DIR}/configuration.data" $LINENO
    checkfile "${DIR}/p${UDEVICENAME}.ld" $LINENO
    checkfile "${DIR}/p${LDEVICENAME}.h" $LINENO
    checkfile "${DIR}/p${UDEVICENAME}.S" $LINENO
    checkfile "${DIR}/p${UDEVICENAME}_div.S" $LINENO
    checkfile "${DIR}/p${UDEVICENAME}_pic.ld" $LINENO
  else
  #If empty, delete the directory 
    [ -d "$DIR" ] && rm -rf $DIR
    BADPICFILE=1
  fi
}

#Function to create/clean build dir
makedirs()
{
  #Create/clean log dir
  rm -rf $LOGDIR
  mkdir -p $PYLOGDIR  # Creates log as well as log/py dirs

  cd $ROOTDIR

  log "Creating/cleaning build directories"
  #Create/clean build dir
  rm -rf $BUILDDIR
  mkdir $BUILDDIR

  cd $ROOTDIR

}

#Function to generate part-support and resource files
generate ()
{
  cd $BUILDDIR

  if [ "x$PACKSDIR" == "x" ]; then
    # only if this is not a generate for DFP
    if [ -z $BAMBOO_BUILD ]; then
      #Get CROWNKING from CVS
      log "Checking-out $CVSCROWN from cvs"
      if [ "$VCSBRATAG" != "trunk" ]; then
        cvs -q co -r $VCSBRATAG $CVSCROWN > ${LOGDIR}/cvs.${CVSCROWN}.log
      else
        cvs -q co $CVSCROWN > ${LOGDIR}/cvs.${CVSCROWN}.log
      fi
      checkreturn $? "Checking-out $CVSCROWN failed" $LINENO
    else
      #Copy crownking that was checked-out by Bamboo
      log "Copying crownking from Bamboo checkout"
      cp -r ../crownking/ .
      checkreturn $? "Copying crownking failed" $LINENO
    fi
  fi
  
  # Use Crownking.py from Artifactory
  log "Get crownking.edc.python.jar from Artifactory using ant"
  pushd $SCRIPTROOT/py
  rm -rf extlibs
  ant setup
  checkreturn $? "'ant setup' failed" $LINENO
  cd extlibs
  unzip crownking.edc.python.jar
  checkreturn $? "'unzip crownking.edc.python.jar' failed" $LINENO
  cp ../*.py ./
  checkreturn $? "Copy XC32 Python scripts to extlibs failed" $LINENO
  PYDIR=`pwd`
  PYOUTDIR=$PYDIR/32Family
  popd
  
  if [ -e $CROWNDIR ] ; then
    cd $CROWNDIR
    export CROWNKING=`pwd`
  else
    log "$CROWNDIR does not exist"
  fi

  #Generate MEC1428.PIC and MEC1428.PIC from SG002.Maskset 
  # 2018-Feb-12:
  #   The SG002.Maskset file has been moved to the CVS attic
  #log "Flatten SG002.Maskset"
  #jython py/flattenpic.py content/edc/masksets/SG002.Maskset
  #cp content/edc/masksets/MEC1424.PIC ${P32PICDIR}
  #cp content/edc/masksets/MEC1426.PIC ${P32PICDIR}

  if [ "x$PACKSDIR" != "x" ]; then
    #list of PIC files to be used for
    log "Using PACKSDIR = $PACKSDIR"
    log "Using PACKS_SUBDIR = $PACK_SUBDIR"
    P32PICDIR=${PACKSDIR}/edc/
    PICFILELIST=(${PACKSDIR}/*.PIC)
  else
    #list of PIC files to be used for
    PICFILELIST=(${P32PICDIR}/{PIC32MK,PIC32MM,PIC32MX,PIC32MZ,MEC14,MGC,USB49,USB7,BT,USB249,WFI}*.PIC)
  fi
  
  cd $PYDIR

  #Collect indices of elements to be removed from $PICFILELIST[]
  INDEX=0
  PICLISTINDEX=0

  if [ "x$PACKSDIR" != "x" ]; then
    # Add ARM-based PIC files to a separate remove index list
    PICLISTINDEX=0
    INDEX=0
    for PICFILE in "${PICFILELIST[@]}"
    do
      if is_arm_picfile "$PICFILE"; then
        REMOVEINDEXLIST_ARM[$INDEX]=$PICLISTINDEX
        let INDEX++
      fi
      let PICLISTINDEX++
    done
  fi

  #Remove invalid .PIC files from PICFILELIST using the collected indices
  for REMOVEINDEX in "${REMOVEINDEXLIST[@]}"
  do
    FILENAME=`echo ${PICFILELIST[$REMOVEINDEX]} | sed -e "s/^.*32xxxx\///"`
    if [ "x$PACKSDIR" != "x" ]; then
      FILENAME=`basename $FILENAME`
    fi
  done

  #Remove ARM-based .PIC files from PICFILELIST
  if [ "x$PACKSDIR" != "x" ]; then
    for REMOVEINDEX in "${REMOVEINDEXLIST_ARM[@]}"
    do
      FILENAME=`echo ${PICFILELIST[$REMOVEINDEX]} | sed -e "s/^.*32xxxx\///"`
      if [ ! -z $FILENAME ]; then
        FILENAME=`basename $FILENAME`
      fi

      # if not already unset
      if [ ! -z ${PICFILELIST[$REMOVEINDEX]} ]; then
        log "Removing $FILENAME from PICFILELIST, ARM devices not supported here"
        unset PICFILELIST[$REMOVEINDEX]
      fi
    done
  fi

  #----Generate support files-----
  INDEX=0
  DEVICECOUNT=1
  for PICFILE in "${PICFILELIST[@]}"
  do
    BADPICFILE=0
    PICFILENAME=`echo $PICFILE | sed -e "s/^.*32xxxx\///"`
    if [ "x$PACKSDIR" != "x" ]; then
      PICFILENAME=`basename $PICFILE`
    fi
    log "Generating support files from $PICFILENAME"
    if [ -z $BAMBOO_BUILD ]; then
      python ./pic2lang32bit.py $NO_FLATTEN -p $PICFILE &> ${PYLOGDIR}/pic2lang.${PICFILENAME}.log
      checkreturn $? "${PYDIR}/pic2lang32bit.py failed to handle ${PICFILE}" $LINENO
    else
      juLog -name="Generate PS from ${PICFILENAME}" -ierror=error python ./pic2lang32bit.py -p $PICFILE &> ${PYLOGDIR}/pic2lang.${PICFILENAME}.log
      if [ $? != 0 ]; then
         echo "Failed to generate part-support from ${PICFILENAME}" >> ${LOGDIR}/errors.txt
      fi
    fi

    check_gen_parts $PICFILE

    #checkfile called from check_gen_parts updates BADPICFILE
    if [ "$BADPICFILE" != "0" ]; then
      BADPICFILEINDEXLIST[$INDEX]=`expr $DEVICECOUNT - 1`
      let INDEX++
    fi
    let DEVICECOUNT++
  done

  #Remove bad .PIC files from PICFILELIST using the collected indices
  for REMOVEINDEX in "${BADPICFILEINDEXLIST[@]}"
  do
    FILENAME=`echo ${PICFILELIST[$REMOVEINDEX]} | sed -e "s/^.*32xxxx\///"`
    log "Removing $FILENAME from PICFILELIST, no files generated by pic2lang32bit.py"
    if [ ! -z $BAMBOO_BUILD ]; then
      juLog -name="pic2Lang32bit.py failed to generate any files for $FILENAME"  false 
    fi
    unset PICFILELIST[$REMOVEINDEX]
    let DEVICECOUNT--
  done

  log "### Generated support files for $DEVICECOUNT devices ###"

  #----Generate resource DVS file-----
  cd $PYDIR
  log "Generating resource DVS file - xc32_DVSdevices.res"
  if [ -z $BAMBOO_BUILD ]; then
    log "python ./pic2res.py -v ${MCHP_VERSION} -r ${MCHP_RESOURCE} "${PICFILELIST[@]}" &> ${LOGDIR}/pic2res.log"
    python ./pic2res.py -v ${MCHP_VERSION} -r ${MCHP_RESOURCE} "${PICFILELIST[@]}" &> ${LOGDIR}/pic2res.log
    checkreturn $? "${PYDIR}/pic2res.py failed" $LINENO
  else
    juLog -name="Resource file generation" -ierror=error python ./pic2res.py -v ${MCHP_VERSION} -r ${MCHP_RESOURCE} "${PICFILELIST[@]}" &> ${LOGDIR}/pic2res.log
  fi

  checkfile "${PYOUTDIR}/xc32_DVSdevices.res" $LINENO
  checkfile "${PYOUTDIR}/xc.h" $LINENO
  checkfile "${PYOUTDIR}/xc-pic32m.h" $LINENO
  checkfile "${PYOUTDIR}/PIC32ConfigSet.html" $LINENO

  #If we are generating part-support for full build, then exit here
  if [ ! -z $bamboo_PART_GEN_FOR_FULL ]; then  
    exit 0
  fi

  #----Generate .info files------

  #Get c30_resource from fossil
  log "Checking-out $FOREPRESOU from fossil"
  fossilopenup ${FOREPRESOU} $VCSBRATAG
  checkreturn $? "Checking-out $FOREPRESOU failed" $LINENO

  rm ${RESOUTDIR}/xc32_DVSdevices.res
  #Get the newly generated xc32_DVSdevices.res
  cp ${PYOUTDIR}/xc32_DVSdevices.res $RESOUTDIR

  cd $RESSRCDIR

  #Set MCHP_VERSION and MCHP_RESOURCE
  MCHP_RESOURCE="\'${MCHP_RESOURCE}\'"
  export MCHP_RESOURCE
  export MCHP_VERSION
  log "Generating .info files"
  ./xc32-build.sh &> ${LOGDIR}/xc32-build.sh.log
  checkreturn $? "${RESSRCDIR}/xc32-build.sh failed" $LINENO

  checkfile "${RESOUTDIR}/xc32_device.info" $LINENO
  checkfile "${RESOUTDIR}/deviceSupport.xml" $LINENO

  MCHP_RESOURCE=`echo $MCHP_RESOURCE | cut -c3`

  #For each .PIC file, there must be a .info file
  # and each device must be present in deviceSupport.xml file
  for PICFILE in "${PICFILELIST[@]}"
  do
    PICFILENAME=`echo $PICFILE | sed -e "s/^.*32xxxx\///"`

    if [ "x$PACKSDIR" != "x" ]; then
      PICFILENAME=`basename $PICFILE`
      # UDEVICENAME=`head -n 3 ${PYLOGDIR}/pic2lang.${PICFILENAME}.log  | tail -n 1`
    fi

    #Some .PIC files might not have their name in upper-case,
    #make sure they are converted to upper-care
    UDEVICENAME=`echo $PICFILENAME | tr '[:lower:]' '[:upper:]'`

    #Get rid of .PIC
    UDEVICENAME=`echo $UDEVICENAME | sed -e "s/\.*PIC//g"`

    checkfile "${RESOUTDIR}/${UDEVICENAME}.info" $LINENO
    if [ ! -z $BAMBOO_BUILD ]; then
      juLog -name="Check if ${UDEVICENAME} exits in deviceSupport.xml" grep -q "$UDEVICENAME" "${RESOUTDIR}/deviceSupport.xml"
    fi
  done

}

#Script execution starts here
processopts $1 $2 $3 $4 $5 $6
# if [ "x$PACKSDIR" == "x" ]; then
  makedirs
# fi
generate
log " $0 ALL OK!"
