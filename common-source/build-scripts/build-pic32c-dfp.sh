#!/bin/bash

# This script does the XC32 DFP content generation for PIC32M-based DFPs.
# It takes the following steps:
#   0. Check input argument, the DFP folder
#   1. Find atdf folders in DFP folder (including sub-folders)
#   2. Create XC32 pack folders
#   3. Checkout and pull updates for pic32c-headers_generator
#   4. Generate part support files for DFPs: startups, linkerscripts,
#      specs files and config data files
#   5. Check files were generated
#   6. Copy to output folder if _all_ files were generated
#
# Input args
#   <PACK_FOLDER> which _must_ contain <PACK_FOLDER>/atdf folder with
#                    input *.ATDF files
#
# Output
#   XC32 content in <PACK_FOLDER>/xc32

SCRIPTROOT=`dirname $0`
source $SCRIPTROOT/build-dfp-common.sh

if [ -z $PIC32C_PARTS_PATH ]; then
# TODO maybe a better idea use a DOCKER_BUILD variable and/or
# docker_PIC32C_PARTS_PATH, docker_PIC32C_PARTS_BRANCH
PIC32C_PARTS_PATH=/xc32/repo/pic32c-headers_generator
PIC32C_PARTS_BRANCH=dev_xc32_part_support
fi

show_usage () 
{
  echo "USAGE: $0 <packsdir>"
  echo "<packs directory>  Specify path to packs directory"
  exit 1
}

#Log $1 along with timestamp to and stdout
log ()
{
  echo "`date +"%F %T %Z :"`$1" | tee -a ${LOGFILE}
}

#Function used to check exit code and abort on error
# $1 - exit code  $2 - error message  $3 - line no.
checkreturn ()
{
  if [[ $1 != 0 ]]; then 
    echo "Error $0:$3 : $2" | tee -a ${LOGFILE}
    exit 1
  fi
}

#Function to check if file exists and abort if not 
# $1 - filepath $2 - line no
pic32c_check_file ()
{
  if [ ! -e $1 ]; then 
    echo "Error $0:$2 File not generated: $1" | tee -a ${LOGFILE}
    NUM_ERRS=$(($NUM_ERRS+1))
  fi
}

# Function to get a device name based on an atdf/pic file.
# n.b. the device name is defined by the file, not the name,
# but we rely on them corresponding exactly. 
device_name()
{
  # strip path and extension, normalize to uppercase
  local filename=$(basename ${1} | tr '[:lower:]' '[:upper:]')
  filename=${filename%.*}

  # Terrible, terrible hack: add AT prefix if missing. This is because the
  # part support hacks on the AT prefix, which in turn is because the 
  # XC shell enforces it.
  if [[ ${filename} == "SAM"* ]]; then
    echo "AT${filename}"
  else
    echo ${filename}
  fi
}

# Function to convert all inputs to lowercase.
to_lower()
{
  echo $@ | tr '[:upper:]' '[:lower:]'
}

# Function to check all files for given atdf input folder were generated
# in <PACK_FOLDER>/xc32
pic32c_check_part_files ()
{
  log "Checking generated XC32 PIC32C part support files in DFP"
  # check specs files, linkerscripts and startups for each device in the pack
  DIR=${TMP_PACK_DIR}/xc32
  for ATDF_FILE in "${ATDF_FILELIST[@]}"
  do
    # UDEVICENAME is extracted from ATDF_FILENAME which is supposed to have the
    # <part_name>.PIC format
    # LDEVICE name is the lower-case equivalent
    UDEVICENAME=$(device_name ${ATDF_FILE})
    LDEVICENAME=$(to_lower ${UDEVICENAME})
    # echo $UDEVICENAME
    pic32c_check_file "${DIR}/${UDEVICENAME}/specs-${UDEVICENAME}"
    pic32c_check_file "${DIR}/${UDEVICENAME}/${UDEVICENAME}.ld"
    pic32c_check_file "${DIR}/${UDEVICENAME}/startup_${LDEVICENAME}.*"
    pic32c_check_file "${DIR}/${UDEVICENAME}/configuration.data"
    pic32c_check_file "${DIR}/${UDEVICENAME}/pic32c"
  done

  # check <PACKSDIR>/xc32/include/xc.h file
  pic32c_check_file "${DIR}/include/xc.h"
}

# Creates pack directory structure for XC32
# Example:
# └── xc32
#     ├── PIC32CZ1038CA70100
#     │   ├── configuration.data
#     │   ├── PIC32CZ1038CA70100.ld
#     │   ├── specs-PIC32CZ1038CA70100
#     │   └── startup_pic32cz1038ca70100.c
#     ├── PIC32CZ1038CA70144
#     │   ├── configuration.data
#     │   ├── PIC32CZ1038CA70144.ld
#     │   ├── specs-PIC32CZ1038CA70144
#     │   └── startup_pic32cz1038ca70144.c
#  ...
#     ├── PIC32CZ2038CA70144
#     │   ├── configuration.data
#     │   ├── PIC32CZ2038CA70144.ld
#     │   ├── specs-PIC32CZ2038CA70144
#     │   └── startup_pic32cz2038ca70144.c
#     └── include
#         └── xc.h
make_temp_packdirs()
{
  log "Creating pack dir structure for XC32 in the DFP folder ${TMP_PACK_DIR}/xc32"
  if [ -d "$TMP_PACK_DIR/xc32" ]; then
    rm -rf ${TMP_PACK_DIR}/xc32
  fi

  # create dir
  mkdir -p ${TMP_PACK_DIR}/xc32/include
  mkdir -p ${TMP_PACK_DIR}/xc32/device-specs

  # create device dirs
  for ATDF_FILE in "${ATDF_FILELIST[@]}"
  do
    # UDEVICENAME is extracted from ATDF_FILENAME which is supposed to have the
    # <part_name>.PIC format
    UDEVICENAME=$(device_name ${ATDF_FILE})
    mkdir -p ${TMP_PACK_DIR}/xc32/${UDEVICENAME}
  done
}

pic32c_gen_part_files()
{
  cd ${PIC32C_PARTS_PATH}/xml2h
  export PYTHONPATH=`pwd`

  # generate startup files
  log "Generating startup files"
  ATDF_PATH=${ATDF_PATH} OUTPUT_PATH=${OUTPUT_PATH} make -f Makefile-xc32 startups &> ${OUTPUT_PATH}/startups.log

  # remove system_*.c devices and copy to DFP
  find ${OUTPUT_PATH}/xc32 -type f -name system_*.c -delete
  cp -R ${OUTPUT_PATH}/xc32 ${TMP_PACK_DIR}

  # generate linker scripts and copy to DFP
  log "Generating linker scripts"
  ATDF_PATH=${ATDF_PATH} OUTPUT_PATH=${OUTPUT_PATH} make -f Makefile-xc32 linkerscripts &> ${OUTPUT_PATH}/linkerscripts.log
  cp -R ${OUTPUT_PATH}/xc32 ${TMP_PACK_DIR}

  # generate xc.h file for DFP and copy to DFP
  log "Generating xc.h"
  rm -f ${OUTPUT_PATH}/xc32/xc.h
  rm -f ${TMP_PACK_DIR}/xc32/include/xc.h
  ATDF_PATH=${ATDF_PATH} OUTPUT_PATH=${OUTPUT_PATH} make -f Makefile-xc32 xc_h_dfp &> ${OUTPUT_PATH}/xc_h.log
  mv ${OUTPUT_PATH}/xc32/xc.h ${TMP_PACK_DIR}/xc32/include/

  # specs files generate and copy
  log "Generating specs files"
  ATDF_PATH=${ATDF_PATH} OUTPUT_PATH=${OUTPUT_PATH} make -f Makefile-xc32 pic32c_specs &> ${OUTPUT_PATH}/specs.log
  for ATDF_FILE in "${ATDF_FILELIST[@]}"
  do
    # UDEVICENAME is extracted from ATDF_FILENAME which is supposed to have the
    # <part_name>.ATDF format
    # LDEVICE name is the lower-case equivalent
    UDEVICENAME=$(device_name ${ATDF_FILE})

    # copy specs file for this device
    cp ${OUTPUT_PATH}/xc32/${UDEVICENAME}/specs-${UDEVICENAME} ${TMP_PACK_DIR}/xc32/${UDEVICENAME}
  done

  cd ${ROOTDIR}

  # generate configuration.data files and pic32c files for each device
  log "Generating configuration.data files"
  for PIC in "${PIC_FILELIST[@]}"
  do
    if is_arm_picfile "$PIC"; then
      # run pic2cfg32.py on the PIC file
      python ${CROWNKING_PYDIR}/pic2cfg32.py $PIC &> ${OUTPUT_PATH}/configdata.log
      # copy configuration.data file to the DFP folder
      pushd ./32Family
      # the crownking script may decide on its own device directory names. 
      # 'normalize' them here.
      for f in $(find . -type d -maxdepth 1); do
        if [[ -f "${f}/configuration.data" ]]; then
           UDEVICENAME=$(device_name ${f})
           mkdir -p ${TMP_PACK_DIR}/xc32/${UDEVICENAME}
           cp ${f}/configuration.data ${TMP_PACK_DIR}/xc32/${UDEVICENAME}
           touch ${f}/pic32c
           touch ${TMP_PACK_DIR}/xc32/${UDEVICENAME}/pic32c
        fi
      done
      popd
    fi
  done
}

if [ "$#" -ne 1 ]; then
  show_usage
fi

ROOTDIR=`pwd`
PACKSDIR=$1
# check PACSKDIR is ok
if [ ! -d "$PACKSDIR" ]; then
  echo "Error $PACKSDIR is not a valid directory!"
  show_usage
fi

# clear output path for part files generator
OUTPUT_PATH=$ROOTDIR/pic32c-parts-output
if [ -d "$OUTPUT_PATH" ]; then
  rm -rf ${OUTPUT_PATH}
fi
mkdir -p ${OUTPUT_PATH}/pack
TMP_PACK_DIR=${OUTPUT_PATH}/pack
LOGFILE=${OUTPUT_PATH}/build-pic32c-dfp.log
echo "LOG for build-pic32c-dfp.sh" | tee -a ${LOGFILE}

# get crownking.edc.python jar file
log "Checking-out crownking.edc.python.jar"
cd ${SCRIPTROOT}/py
rm -rf ./extlibs
ant setup
checkreturn $? "Checking-out crownking.edc.python.jar failed" $LINENO
if [ ! -f ./extlibs/crownking.edc.python.jar ]; then
  echo "Checking-out crownking.edc.python.jar failed"
  exit 1
fi

cd ${ROOTDIR}

# unzip crownking.edc.python jar file and copy 
# pic2lang32bit.py and pic2res.py scripts
CROWNKING_PYDIR=${ROOTDIR}/crownking/py
rm -rf ${CROWNKING_PYDIR}
mkdir -p $CROWNKING_PYDIR
unzip -d $CROWNKING_PYDIR ${SCRIPTROOT}/py/extlibs/crownking.edc.python.jar
cp ${SCRIPTROOT}/py/*.py $CROWNKING_PYDIR

# checkout and update pic32c part files generator repo
log "Pulling updates from pic32c-headers-generator bitbucket repository, branch ${PIC32C_PARTS_BRANCH}"
cd ${PIC32C_PARTS_PATH}
git checkout ${PIC32C_PARTS_BRANCH}
git pull origin

cd ${ROOTDIR}

# find atdf folders
ATDF_FOLDERS=`find ${PACKSDIR} -name *.atdf | xargs -I% dirname % | sort | uniq`
if [ -z "$ATDF_FOLDERS" ]; then
  echo "Couldn't find any atdf folder in ${PACKSDIR}!"
  show_usage
fi

# for every atdf folder found generate files

for ATDF_PATH in ${ATDF_FOLDERS[@]}
do
  EDC_PATH=${PACKSDIR}/edc

  if [ ! -d "$EDC_PATH" ]; then
    echo "Error $EDC_PATH is not a valid directory!"
    show_usage
  fi

  # create list of ATDF files in the DFP
  ATDF_FILELIST=(${ATDF_PATH}/*.atdf)

  # create list of PIC files in the DFP
  PIC_FILELIST=(${EDC_PATH}/*.PIC)

  # make temp pack directories for XC32
  make_temp_packdirs

  # generate files
  pic32c_gen_part_files

  NUM_ERRS=0
  # check all parts files were generated
  pic32c_check_part_files

  if [ "$NUM_ERRS" -eq 0 ]; then
    # copy output folder for XC32 in mounted packs dir
    cp -R $TMP_PACK_DIR/xc32 $PACKSDIR
    # remove tmp dir
    rm -rf $TMP_PACK_DIR
    mkdir -p $TMP_PACK_DIR
  else
    log "$0 : NOK files were not generated!"
    #exit 1
  fi
done

log "$0 : DONE!"
