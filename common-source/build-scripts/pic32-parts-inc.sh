
if [[  ! ( -z $bamboo_XC32_PARTS_VERSION ) ||  ! ( -z $bamboo_XC32_FULL_VERSION ) ]];then
  BAMBOO_BUILD=1
fi

HOST=`uname -s`

MCHP_VERSION=""
MCHP_RESOURCE=""
MCHP_BUILD=""
VCSBRATAG=trunk

ROOTDIR=`pwd`
LOGDIR=${ROOTDIR}/log
PACKSDIR=""
NO_FLATTEN=""

CVSCROWN=crownking
#CVSBITRK=installers/BitRock

FOSSERVER=fossil.microchip.com
FOSOPENDIR=${ROOTDIR}/fossilopen
FOSLOG=${LOGDIR}/fossil.log
FOREPRESOU=c30_resource
FOREPP32LIB=pic32-libs
FOREPTGEN=partstablegen
FOREPCPAUTO=compilerpage_automate
FOREPBITRK=XC-installers

GITURL='ssh://git@bitbucket.microchip.com/xc32'
GITCLONEDIR=${ROOTDIR}/gitclones
GITP32MLIBS=pic32m-libs
GP32MLIBSDIR=${GITCLONEDIR}/${GITP32MLIBS}
GITP32MDOCS=pic32m-docs
GP32MDOCSDIR=${GITCLONEDIR}/${GITP32MDOCS}

BUILDDIR=${ROOTDIR}/build
PYLOGDIR=${LOGDIR}/py
IMAGEDIR=${ROOTDIR}/image
if [ "$HOST" = "Darwin" ]; then
  LOGFILE=${LOGDIR}/`echo $0 | sed -Ee  "s/^(.+\/)*//"`.log 
else
  LOGFILE=${LOGDIR}/`echo $0 | sed -re  "s/^(.+\/)*//"`.log 
fi

CROWNDIR=${BUILDDIR}/${CVSCROWN}
P32PICDIR=${CROWNDIR}/content/edc/32xxxx
if [ "x$SCRIPTROOT" == "x" ] ; then
  SCRIPTROOT=$( cd "$( dirname "$0" )" && pwd )
fi
if [ "x$PYDIR" == "x" ] ; then
  PYDIR=${SCRIPTROOT}/py/extlibs
fi
if [ "x$PYOUTDIR" == "x" ] ; then
  PYOUTDIR=${PYDIR}/32Family 
fi

echo "SCRIPTROOT=$SCRIPTROOT"
echo "PYDIR=$PYDIR"
echo "PYOUTDIR=$PYOUTDIR"

BADPICFILE=0

RESSRCDIR=${FOSOPENDIR}/${FOREPRESOU}/src
RESOUTDIR=${RESSRCDIR}/xc32

REPP32LIBDIR=${FOSOPENDIR}/${FOREPP32LIB}/libpic32

BITOUTDIR=${IMAGEDIR}/install

INSTOUTDIR=${ROOTDIR}/installer

UPSCRIPTDIR=${FOSOPENDIR}/${FOREPCPAUTO}

#Log $1 along with timestamp to ${LOGDIR}/${LOGFILE} and stdout
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
    exit -1
  fi
}

show_usage () 
{
  echo "USAGE: $0 -v <compiler-version> -r <resource-revision> [-b <build-name>] [-p <packs-directory>]"
  echo " -v<compiler-version>  Specify compiler version in "
  echo "                       x.yy format"
  echo " -r<resource-revision> A-Z for normal build "
  echo "                       1-9 for release candidates"
  echo "                       0 for nightly"
  echo " -b<build-name>        TCx/RCx (optional)"
  echo " -t<VCS BRANCH/TAG>    optional, trunk assumed if not specified"
  echo " -p<packs directory>   optional, DFP generation packs dir"
  exit 1
}

processopts ()
{
  while getopts v:r:b:t:p: opt
  do
   case "$opt" in
    v)
    TVAL=$OPTARG
    MCHP_VERSION="$TVAL"
    ;;
    r)
    TVAL=$OPTARG
    MCHP_RESOURCE="$TVAL"
    ;;
    b)
    TVAL=$OPTARG
    MCHP_BUILD="$TVAL"
    ;;
    t)
    TVAL=$OPTARG
    VCSBRATAG="$TVAL"
    ;;
    p)
    TVAL=$OPTARG
    PACKSDIR="$TVAL"
    ;;
    \?) show_usage ;;
   esac
  done

if [ "x$PACKSDIR" != "x" ]; then
  echo $PACKSDIR
  NO_FLATTEN="--no-flatten"
fi
  
if [ -z $MCHP_VERSION ]; then
  show_usage 
fi
if [ -z $MCHP_RESOURCE ]; then
  show_usage
fi

}

#Function to check if file exists and abort if not 
# $1 - filepath $2 - line no
checkfile ()
{
  if [ ! -e $1 ]; then 
    echo "Error $0:$2 File not generated: $1" | tee -a ${LOGFILE}

    BADPICFILE=1   

    # When we are doing a nightly build, don't stop if a device fails.
    if [ "$MCHP_RESOURCE" = "0" ]; then
      if [ ! -z $BAMBOO_BUILD ]; then
       juLog -name="File not generated $1" false 
       true
      fi
    else
      echo "****** -- $MCHP_RESOURCE -- I am here but I am not supposed to be here ******"
      exit 1
    fi
  fi
}

#Function to check if fossil evnviornment is setup properly
# No arguments 
fossilcheck ()
{
  log "Checking if FOSSIL_CLONES evn variable exists"
  if [ -z "$FOSSIL_CLONES" ]; then
    checkreturn -1 "This script expects you to do fossil in MCHP way" $LINENO
  elif [ ! -d "$FOSSIL_CLONES" ]; then
    checkreturn -1 "Directory $FOSSIL_CLONES does not exist" $LINENO
  else
    log "Using fossil clones at $FOSSIL_CLONES"
  fi	

  log "Checking if fossil server can be reached"
  echo "ping -q -c 1 $FOSSERVER >> ${FOSLOG}" >> ${FOSLOG}
  ping -q -c 1 $FOSSERVER >> ${FOSLOG} 2>&1
  checkreturn $? "Cannot reach fossil server at $FOSSSERVER" $LINENO
}

#Function to klone/pull fossil clones
# $1 - name of fossil repo to klone/pull
fossilklonepull ()
{
  fossilcheck 
  if [ -e ${FOSSIL_CLONES}/$1.fsl ]; then
    log "Pulling ${FOSSIL_CLONES}/$1.fsl"
    echo "fossil pull -R ${FOSSIL_CLONES}/$1.fsl" >> ${FOSLOG}
    fossil pull -R ${FOSSIL_CLONES}/$1.fsl >> ${FOSLOG} 2>&1
    checkreturn $? "pull on ${FOSSIL_CLONES}/$1.fsl failed" $LINENO
  else
    log "Kloning $1.fsl to $FOSSIL_CLONES"
    VERSTR=`fossil version`
    if [[ $VERSTR =~ MCHP ]]; then
      echo "yes | fossil klone $1" >> ${FOSLOG}
      yes | fossil klone $1 >> ${FOSLOG} 2>&1
    else
      echo "fossil klone $1" >> ${FOSLOG}
      fossil klone $1 >> ${FOSLOG} 2>&1
    fi
    checkreturn $? "Kloning $1.fsl to $FOSSIL_CLONES failed" $LINENO
  fi

}

#Function to open/update a fossil clone (tree a fossil klone)
#$1 - repo name $2 - branch/tag (optional, trunk if not specified)
fossilopenup ()
{
  REPO=$1
  fossilklonepull $REPO
 
  if [ -z "$2" ]; then
    BRATAG=trunk  
  else
    BRATAG=$2
  fi

  log "Checking if branch/tag \"$BRATAG\" exists"
  TVAL=`fossil branch list -R $FOSSIL_CLONES/${REPO}.fsl | grep "  $BRATAG\$"`
  if [ $? -ne 0 ]; then 
    TVAL=`fossil tag list -R $FOSSIL_CLONES/${REPO}.fsl | grep "  $BRATAG\$"`
    checkreturn $? "No branch or tag named \"$BRATAG\" in ${REPO}.fsl repo" $LINENO
  fi

  if [ ! -d "$FOSOPENDIR" ]; then
    mkdir $FOSOPENDIR
  fi
  cd $FOSOPENDIR

  if [ ! -d "$REPO" ]; then
    log "Opening $REPO at ${FOSOPENDIR}/$REPO"
    mkdir $REPO
    cd $REPO
    echo "fossil open ${FOSSIL_CLONES}/${REPO}.fsl $BRATAG" >> ${FOSLOG}
    fossil open ${FOSSIL_CLONES}/${REPO}.fsl $BRATAG >> ${FOSLOG} 2>&1
    checkreturn $? "Error opening fossil repo $REPO" $LINENO
  else
    log "Updating $REPO at ${FOSOPENDIR}/$REPO"
    cd $REPO
    echo "fossil update $BRATAG" >> ${FOSLOG}
    fossil update $BRATAG >> ${FOSLOG} 2>&1
    checkreturn $? "Error updating fossil repo $REPO" $LINENO
  fi

}

#Function to add/commit to already opened repo
# $1 - repo name (placed at pwd) $2 - commit message
fossiladdcommit()
{
  if [ ! -d "$1" ]; then
    echo "Repo $1 does not exist"
    exit 1
  fi

  cd $1

  fossil update

  fossil extra > ../$1.newfiles.log
  fossil diff > ../$1.diff.log 

  if [[ -s ../$1.newfiles.log ]]; then
    NEWFILES=$(fossil extra)
    fossil add $NEWFILES
  else
    echo "**** No new files to add to $1****"
  fi

  if [[ ( -s ../$1.newfiles.log ) || ( -s ../$1.diff.log ) ]]; then
    fossil commit -m "$2"
  else
    echo " **** Nothing to commit to $1****"
  fi
  
  rm ../$1.newfiles.log
  rm ../$1.diff.log

  cd ..
}

#Clone a git repo and checkout specific branch
# $1 - repo name  $2 - branch name
gitclone()
{
  REPO=$1
  BRANCH=$2

  if [[ ! -d $GITCLONEDIR ]]; then
    mkdir -p $GITCLONEDIR
  fi

  pushd $GITCLONEDIR

  git clone ${GITURL}/${REPO}.git

  cd ${GITCLONEDIR}/${REPO}

  if [[ "$BRANCH" != "" ]]; then
    git checkout $BRANCH
  fi

  popd 
}


#Add all modified and new files and push already checkedout repo
# $1 - repo $2 - commit message
gitaddpush()
{
  REPO=$1
  MSG=$2

  pushd ${GITCLONEDIR}/${REPO}

  git add .
  OUTPUT=$(git status)
  if [[ ! $OUTPUT =~ nothing ]]; then   
   git commit -m "$MSG"
   git push origin
  else 
    echo "*** Nothing to commit in $REPO repo ***"
  fi 
  popd
   
}

#Function to generate readme, called by both create-image and create-A-image
generatereadme ()
{
  fossilopenup $FOREPTGEN 
  
  READGENDIR=${FOSOPENDIR}/${FOREPTGEN}/parts_readme

  log "Creating list of supported devices"
  if [ -e ${IMAGEDIR}/pic32mx/lib/proc ] ; then
    ls ${IMAGEDIR}/pic32mx/lib/proc > ${READGENDIR}/devicelist.tmp
  fi
  if [ -e ${IMAGEDIR}/pic32c/lib/proc ] ; then
    ls ${IMAGEDIR}/pic32c/lib/proc >> ${READGENDIR}/devicelist.tmp
  fi
  TMP_FILE=`mktemp /tmp/devicetmp.XXXXXX`
  sed -e "/32MXGENERIC/d" "${READGENDIR}/devicelist.tmp" > $TMP_FILE
  sed -e "s/^PIC//" $TMP_FILE > "${READGENDIR}/devicelist.tmp"
  sort "${READGENDIR}/devicelist.tmp" > $TMP_FILE
  cp $TMP_FILE "${READGENDIR}/devicelist.tmp"
  
  #Add # indices for family 
  FAMILY=""
  INFILE="${READGENDIR}/devicelist.tmp"
  OUTFILE="${READGENDIR}/devicelist.txt"
  NEWDEVICESFILE="${READGENDIR}/newdevicelist.txt"
  OLDDEVICESFILE="${READGENDIR}/XC32_last_devicelist.txt"
  
  if [ -f $OUTFILE ]; then 
    rm $OUTFILE
  fi

  while IFS= read -r LINE
  do
    TEMP=`echo "$LINE" | sed  -e 's/\(^.\{4\}\).*/\1/'`
    if [ "$FAMILY" != "$TEMP" ] ; then 
      FAMILY=$TEMP
      if [[ "$FAMILY" =~ ^32 ]]; then 
        echo "#PIC${FAMILY}" >> $OUTFILE
      else
        echo "#${FAMILY:0:3}" >> $OUTFILE
      fi 
    fi 
    echo "$LINE" >> $OUTFILE
  done <"$INFILE"

  #diff devicelist with previous releses' device list to get newdevicelist
  diff ${OLDDEVICESFILE} ${INFILE}  | grep '>' | sed -e 's/>[[:space:]]*//' > ${NEWDEVICESFILE}

  log "Generating Part-support readme"
  cd ${IMAGEDIR}/docs 	
  if [ "$MCHP_RESOURCE" = "A" ]; then
    python ${READGENDIR}/genXCPartsReadme.py -m ${READGENDIR}/xc32_template.html -l $OUTFILE -t XC32 -v $MCHP_VERSION -n ${NEWDEVICESFILE}
  else
    python ${READGENDIR}/genXCPartsReadme.py -m ${READGENDIR}/xc32_template.html -l $OUTFILE -t XC32 -v $MCHP_VERSION -n ${NEWDEVICESFILE} -r $MCHP_RESOURCE
  fi
  checkreturn $? "Error generating update readme using genXCPartsReadme.py"   $LINENO
  if [ "$MCHP_RESOURCE" = "A" ]; then
    checkfile "${IMAGEDIR}/docs/XC32-v${MCHP_VERSION}-part-support-release-notes.html" $LINENO
  else
    checkfile "${IMAGEDIR}/docs/XC32-v${MCHP_VERSION}${MCHP_RESOURCE}-part-support-release-notes.html" $LINENO
  fi 
}

generatedevicesupportxml ()
{
  BUILDDATE=`date +"%Y-%m-%d"`
  echo "<?xml version='1.0' encoding='UTF-8' standalone='no'?>" > ${IMAGEDIR}/bin/deviceSupport.xml
  echo -n "<mp:deviceSupport xmlns:mp=\"http://crownking/mplab\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " >> ${IMAGEDIR}/bin/deviceSupport.xml
  echo -n "xsi:schemaLocation=\"http://crownking/mplab%20mplab/deviceSupport\" mp:version=\"${MCHP_VERSION} (${MCHP_RESOURCE})\" " >> ${IMAGEDIR}/bin/deviceSupport.xml
  echo "mp:builddate=\"${BUILDDATE}\">" >> ${IMAGEDIR}/bin/deviceSupport.xml
  echo "  <mp:family mp:family=\"32x\">" >> ${IMAGEDIR}/bin/deviceSupport.xml

  if [ -e ${IMAGEDIR}/pic32mx/lib/proc ] ; then
    for dev in `ls ${IMAGEDIR}/pic32mx/lib/proc`; do
      dev=$(echo "$dev" | sed 's/^32M/PIC32M/')
      echo "    <mp:device mp:name=\"$dev\"/>" >> ${IMAGEDIR}/bin/deviceSupport.xml
    done
  fi
  if [ -e ${IMAGEDIR}/pic32c/lib/proc ] ; then
    for dev in `ls ${IMAGEDIR}/pic32c/lib/proc`; do
      dev=$(echo "$dev" | sed 's/^32C/PIC32C/')
      echo "    <mp:device mp:name=\"$dev\"/>" >> ${IMAGEDIR}/bin/deviceSupport.xml
    done
  fi
  # Add the SAM names in addition to the ATSAM canonicalized names
  if [ -e ${IMAGEDIR}/pic32c/lib/proc ] ; then
    for dev in `ls ${IMAGEDIR}/pic32c/lib/proc`; do
      dev=$(echo "$dev" | sed 's/^ATSAM/SAM/')
      echo "    <mp:device mp:name=\"$dev\"/>" >> ${IMAGEDIR}/bin/deviceSupport.xml
    done
  fi

  echo "  </mp:family>" >> ${IMAGEDIR}/bin/deviceSupport.xml

  echo "</mp:deviceSupport>" >> ${IMAGEDIR}/bin/deviceSupport.xml
  echo "" >> ${IMAGEDIR}/bin/deviceSupport.xml
}
