# Extracts image archives, copies DFP files and then archives the images back.
# Packs directory is the first argument to this script.
set -e

copydfps ()
{
  set +u
  local IMAGE_DIR="$1"

  dfp_org_dirs=$(find $PACKS_DIR -depth 1 -type d)
  for dir in ${dfp_org_dirs[@]}
  do
    dfp_dirs=$(find $dir -depth 1 -type d)
    for dfp_dir in ${dfp_dirs[@]}
    do
      dfp_version_dir=$(find $dfp_dir -depth 1 -type d)
      if [ -d ${dfp_version_dir} ]; then
        device_family=$(basename ${dfp_dir} | sed -e "s/x*_DFP//g")
        if [[ $device_family = *"CEC"* || $device_family = *"MEC"* ]]; then
          # skip CEC and MEC
          continue
        fi

        ldevice_family=$(echo ${device_family} | tr '[:upper:]' '[:lower:]')
        dfp_subdirs=$(find ${dfp_version_dir} -depth 1 -name "${ldevice_family}*" -type d)
        if [[ -z $dfp_subdirs ]]; then
          dfp_subdirs=$dfp_version_dir
          upper_dfp_subdir_basename=$device_family
        else
          upper_dfp_subdir_basename=""
        fi
        for dfp_subdir in ${dfp_subdirs[@]}
        do
          if [[ -z "$upper_dfp_subdir_basename" ]]; then
            upper_dfp_subdir_basename=$(basename ${dfp_subdir} | tr '[:lower:]' '[:upper:]')
          fi
          if [ -d ${dfp_subdir}/include_mcc ]; then
            echo Copying ${dfp_subdir}/include_mcc to ${IMAGE_DIR}/pic32c/include_mcc/proc/${upper_dfp_subdir_basename}
            cp -R ${dfp_subdir}/include_mcc ${IMAGE_DIR}/pic32c/include_mcc/proc/${upper_dfp_subdir_basename}
          fi
          if [ -d ${dfp_subdir}/include ]; then
            echo Copying ${dfp_subdir}/include to ${IMAGE_DIR}/pic32c/include/proc/${upper_dfp_subdir_basename}
            cp -R ${dfp_subdir}/include ${IMAGE_DIR}/pic32c/include/proc/${upper_dfp_subdir_basename}
          fi
          # TODO xc32 DFP content
          upper_dfp_subdir_basename=""
        done
      fi
    done
  done

  
  #handle CEC1702 and MEC17xx separately
  dfp_dir="${PACKS_DIR}"/Microchip/CEC_DFP/*
  if [ -d $dfp_dir ]; then
    #copy startup, linker script, atdf and svd
    echo Copying from ${dfp_dir}
    mkdir -p  ${IMAGE_DIR}/pic32c/lib/proc/CEC1702/
    cp ${dfp_dir}/xc32/startup_cec1702.c ${IMAGE_DIR}/pic32c/lib/proc/CEC1702/
    cp ${dfp_dir}/xc32/CEC1702.ld ${IMAGE_DIR}/pic32c/lib/proc/CEC1702/
    cp ${dfp_dir}/atdf/CEC1702.atdf ${IMAGE_DIR}/pic32c/lib/proc/CEC1702/
    cp ${dfp_dir}/svd/CEC1702.svd ${IMAGE_DIR}/pic32c/lib/proc/CEC1702/
    # TODO fix include files in DFP
    # TODO where to get CEC1302 ???
  fi

  dfp_dir="${PACKS_DIR}"/Microchip/MECxxxx_DFP/*/mec17
  if [ -d $dfp_dir ]; then
    echo Copying from ${dfp_dir}
    cp -R ${dfp_dir}/include ${IMAGE_DIR}/pic32c/include/proc/MEC170x
    # TODO xc32 content
  fi

  # TODO other special cases, PIC32M??
  set -u
}

this_show_usage () 
{
  echo "USAGE: $0 <packs directory> <xc32 image directory>"
  exit 1
}

PACKS_DIR="$1"
if [ -z ${PACKS_DIR} ]; then
  this_show_usage 
fi

if [ ! -d ${PACKS_DIR} ]; then
  echo "Could not find packs dir"
  this_show_usage
fi

IMAGE_DIR="$2"
if [ -z ${IMAGE_DIR} ]; then
  this_show_usage 
fi

if [ ! -d ${IMAGE_DIR} ]; then
  echo "Could not find image dir"
  this_show_usage
fi

copydfps $IMAGE_DIR