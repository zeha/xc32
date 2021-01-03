# This script downloads the DFPs associated with the XC32 devices from the
# Jira E-BUS.
#
# Calls py/xc32-parts-pipeline.py to get the list of parts that have 
# the "XC32" string listed in the "Target Yellow C Compiler" field.
#
# Uses stash/packs repository to identify the DFPs associated with the parts,
# by grep-ing the device name in the *.pkgdef files.
# 
# Uses gitlab/earlydfpsoften project to fetch the DFPs from artifactory. 
# Packs are fetched into the earlydfpsoften/packs folder.
SCRIPTROOT=$( cd "$( dirname "$0" )" && pwd )
set -e
set -u

#Clone a git repo and checkout specific branch
# $1 - git url
# $2 - repo name
# $3 - origin branch
# $4 - target branch
gitclone()
{
  set +u
  local GITURL=$1
  local REPO=$2
  local ORIGIN_BRANCH=$3
  local TARGET_BRANCH=$4

  if [[ ! -d "${REPO}" ]]; then
    echo "Cloning ${REPO}"
    git clone -q "${GITURL}/${REPO}.git"
  else
    echo "Updating ${REPO}"
    if [[ "$ORIGIN_BRANCH" != "" ]]; then
      git -C "${REPO}" checkout $ORIGIN_BRANCH
    fi
    git -C "${REPO}" pull --quiet origin
  fi

  if [[ "$ORIGIN_BRANCH" != "" ]]; then
    git -C "${REPO}" checkout $ORIGIN_BRANCH
  fi

  if [[ "$TARGET_BRANCH" != "" ]]; then
    git -C "${REPO}" checkout -b $TARGET_BRANCH
  fi
  set -u
}

# Clone the repos that we need
getrepos ()
{
  : ${GITLAB_URL:=git@gitlab.microchip.com:devtools/development}
  # GITLAB_URL="${GITLAB_URL%/}"
  echo "Using GITLAB_URL=${GITLAB_URL}/earlydfpsoften for fetching DFPs"
  gitclone ${GITLAB_URL} earlydfpsoften master

  : ${PACKS_URL=http://git@stash.corp.atmel.com/scm/devsup}
  echo "Using PACKS_URL=${PACKS_URL} for fetching DFP names"
  gitclone ${PACKS_URL} packs master
}

# $1 - filename to save output from the jira parsing script
EBUSdevices()
{
  local DEVICES_TXT_FILE="$1"
  # fetch device names from Jira E-BUS
  echo "Fetching device names from JIRA EBUS"
  python $SCRIPTROOT/../../py/xc32-parts-pipeline.py $DEVICES_TXT_FILE

  if [[ ! -f $SCRIPTROOT/$DEVICES_TXT_FILE ]]; then
    echo "Error: Couldn't find devices file $DEVICES_TXT_FILE"
  fi
}

# $1 - input file (containing device names parsed from JIRA, one per line)
writeivyfromdevicenames()
{
  local input_file="$1"
  # remove old 32bit.ivy.xml file
  rm -f 32bit.ivy.xml

# create new 32bit.ivy.xml file
cat > 32bit.ivy.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>
<ivy-module version="2.0"
            xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
            xsi:noNamespaceSchemaLocation="http://ant.apache.org/ivy/schemas/ivy.xsd">
    <info organisation="com.microchip.crownking" module="earlydfpsoften" revision="0.0"/>
    <dependencies>
EOF

  # get DFP names from device names based *.pkgdef files
  unique_dfps=""
  while IFS= read -r device
  do
    # *.pkgdef files are xmls that have a <target> element with name="device_name"
    # attributes. grep those in order to detect the DFP corresponding to a device.
    echo $device
    set +e
    results=$(grep -l -R -i --include \*.pkgdef --exclude-dir Internal "\<target name=\"$device" ./packs/packs/)
    dfpname=""
    if [[ ! -z "$results" ]]; then
      for filepath in $results; do
        filename=$(basename $filepath)
        # if device is found in a non-DEV|Internal|Development packs, give that
        # packs priority, else accept the development pack
        if [[ -z "$dfpname"
          || $dfpname = *"DEV"*
          || $dfpname = *"Internal-Only"*
          || $dfpname = *"Development-Only"* ]]; then
          dfpname="${filename%.*}"
        fi
      done
    else
      # log devices which don't have a DFP yet
      echo "Device $device was not found in any DFP"
    fi

    if [[ -z "$dfpname" ]]; then
      continue
    fi

    set -e
    # append prefix and suffix to the DFP name
    # prefix with "Atmel" for SAM devices, with "Microchip" for all the rest
    if [[ $dfpname = *"SAM"* ]]; then
      dfpfullname="Atmel.${dfpname}_DFP"
    else
      dfpfullname="Microchip.${dfpname}_DFP"
    fi

    # we don't want duplicates in the 32bit.ivy.xml file
    if [[ $unique_dfps != *"$dfpfullname"* ]]; then
      unique_dfps+=" $dfpfullname"
      echo "      <dependency org=\"atmel\" name=\"${dfpfullname}\" rev=\"latest.integration\">" >>32bit.ivy.xml
      echo "        <artifact name=\"${dfpfullname}\" type=\"zip\" ext=\"atpack\"/>" >>32bit.ivy.xml
      echo "      </dependency>" >>32bit.ivy.xml
    fi
  done <"$input_file"

# write last lines close elements
cat >> 32bit.ivy.xml << EOF
    </dependencies>
</ivy-module>
EOF
}

fetchdfps()
{
  pushd ./earlydfpsoften
  echo "Performing setup for earlydfpsoften"
  ./setup.sh
  ./clean.sh
  echo "Fetching latest DFPs"
  # fetch the DFPs by replacing the 32bit.ivy.xml file from earlydfpsoften
  # with the one we just created, then calling the ant "latest-32bit-dfps" target
  cp $SCRIPTROOT/32bit.ivy.xml .
  ant latest-32bit-dfps
  popd > /dev/null
}

DEVICES_FILE=xc32-devices.txt
getrepos
EBUSdevices $DEVICES_FILE
writeivyfromdevicenames $DEVICES_FILE
fetchdfps

echo "DONE: downloaded latest DFPs in earlydfpsoften/packs"
