#!/bin/bash
ORIGINAL_DIR=$PWD
FOSSERVER=fossil.microchip.com

TAGVERSION=
COTAG=
SOURCE=

export PATH=$HOME/xc32/fossil/script:$PATH

if [ -z $FOSSIL_CLONES ]; then
  export FOSSIL_CLONES=$HOME/xc32/fossil/clones
  export FOSSIL_LOGIN=bamboo:bamboo
fi

show_usage()
{
  # Shows the usage screen
  echo "USAGE:  $0 -t tag"
  echo "  -t tag        tag name to apply to the release"
  echo "  -u co_tag     Check-out 'co_tag' version before tagging instead of"
  echo "                complaining about missing repo"
  echo "  -?            Show this usage message"
  cd $ORIGINAL_DIR
  exit 1
}

# Does notify-send exist?
if [ "x$NOTIFY_SEND" == "x" ] ; then
 WHICH_NOTIFY_SEND=`which notify-send`
 if [ "x$WHICH_NOTIFY_SEND" == "x" ] ; then
  unset NOTIFY_SEND
 else
  NOTIFY_SEND=notify-send
 fi
 unset WHICH_NOTIFY_SEND
fi

# Does growlnotify exist?
if [ "x$GROWL_SEND" == "x" ] ; then
 WHICH_GROWLSEND=`which growlnotify`
 if [ "x$WHICH_GROWLSEND" == "x" ] ; then
  unset GROWL_SEND
 else
  GROWL_SEND="growlnotify -sm "
 fi
 unset WHICH_GROWLSEND
fi

####
# assert_success()
#  If the first parameter is non-zero, print the second parameter
#  and exit the script
####
function assert_success ()
{
 local RESULT=$1
 local MESSAGE=$2
 if [ $RESULT != 0 ]
 then
  echo "$MESSAGE ($RESULT)"
  if [ "x$NOTIFY_SEND" != "x" ] ; then
   $NOTIFY_SEND "$MESSAGE" "XC32 Tag Release"
  elif [ "x$GROWL_SEND" != "x" ] ; then
    $GROWL_SEND "xc32-tag-release.sh: $MESSAGE"
  fi
  cd $ORIGINAL_DIR
  exit $RESULT
 fi
}

#### 
# fossilcheck ()
#  Function to check if fossil evnviornment is setup properly
####
fossilcheck ()
{
  echo "Checking if FOSSIL_CLONES evn variable exists"
  if [ -z "$FOSSIL_CLONES" ]; then
    assert_success -1 "This script expects you to do fossil in MCHP way" 
  elif [ ! -d "$FOSSIL_CLONES" ]; then
    assert_success -1 "Directory $FOSSIL_CLONES does not exist" 
  else
    echo "Using fossil clones at $FOSSIL_CLONES"
  fi

  echo "Checking if fossil server can be reached"
  echo "ping -q -c 1 $FOSSERVER" 
  ping -q -c 1 $FOSSERVER 
  assert_success $? "Cannot reach fossil server at $FOSSSERVER" 
}

####
# fossilklonepull() 
#  Pull or klone (if required) $1
####
function fossilklonepull ()
{
  fossilcheck
  if [ -e ${FOSSIL_CLONES}/$1.fsl ]; then
    echo "fossil pull -R ${FOSSIL_CLONES}/$1.fsl" 
    fossil pull -R ${FOSSIL_CLONES}/$1.fsl 
    assert_success $? "pull on ${FOSSIL_CLONES}/$1.fsl failed" 
  else
    echo "Kloning $1.fsl to $FOSSIL_CLONES"
    VERSTR=`fossil version`
    if [[ $VERSTR =~ MCHP ]]; then
      echo "yes | fossil klone $1"
      yes | fossil klone $1 
    else
      echo "fossil klone $1" 
      fossil klone $1 
    fi
    assert_success $? "Kloning $1.fsl to $FOSSIL_CLONES failed"
  fi

}

####
# fossilopenup()
#  open/update a fossil clone (tree a fossil klone)
#  $1 - repo name $2 - branch/tag (optional, trunk if not specified)
####
function fossilopenup ()
{
  REPO=$1
  fossilklonepull $REPO

  if [ -z "$2" ]; then
    BRATAG=trunk
  else
    BRATAG=$2
  fi

  echo "Checking if branch/tag \"$BRATAG\" exists"
  TVAL=`fossil branch list -R $FOSSIL_CLONES/${REPO}.fsl | grep "  $BRATAG\$"`
  if [ $? -ne 0 ]; then
    TVAL=`fossil tag list -R $FOSSIL_CLONES/${REPO}.fsl | grep "$BRATAG\$"`
    assert_success $? "No branch or tag named \"$BRATAG\" in ${REPO}.fsl repo" 
  fi

  if [ ! -d "$REPO" ]; then
    echo "Opening $REPO at ."
    mkdir $REPO
    cd $REPO
    echo "fossil open ${FOSSIL_CLONES}/${REPO}.fsl $BRATAG"
    fossil open ${FOSSIL_CLONES}/${REPO}.fsl $BRATAG 
    assert_success $? "Error opening fossil repo $REPO" 
  else
    echo "Updating $REPO at ${FOSOPENDIR}/$REPO"
    cd $REPO
    echo "fossil update $BRATAG" >> ${FOSLOG}
    fossil update $BRATAG >> ${FOSLOG} 2>&1
    assert_success $? "Error updating fossil repo $REPO" 
  fi
  cd ..
}

if [ $# = 0 ]
then
  show_usage
  cd $ORIGINAL_DIR
  exit 0
fi

while getopts t:u: opt
do
  case "$opt" in
    t)
    TAGVERSION=$OPTARG
    ;;
    u)
    COTAG=$OPTARG
    ;;
    \?) show_usage ;;
  esac
done


if [ "x$COTAG" != "x" ] ; then
  all_fossil_repos=(
   pic32-libs
   XC32-docs
   XC32-examples
   XC-libs
   XCpp-libs
   XC32-gcc-48
   XC32-build-scripts
   fpmath
   c30_resource
   pic30-sh
   fdlibm
   xclm_release
   pic32-newlib
   DynLinker
   XC-installers
 )
else
  all_fossil_repos=(
 ./common-source/Coverity
 ./common-source/docs
 ./common-source/examples
 ./common-source/xc_shell
 ./common-source/xclm_release
 ./pic32m-source/src48x
 ./pic32m-source/XC-libs
 ./pic32m-source/XCpp-libs
 ./pic32m-source/c30_resource
 ./pic32m-source/fdlibm
 ./pic32m-source/fpmath
 ./pic32m-source/pic32-libs
 ./pic32m-source/pic32-newlib
 ./pic32c-source/XC32-arm-gcc
 ./pic32c-source/pic32c-libs
 ./other-source/XC32-build-scripts
 ./other-source/XC-installers
 ./other-source/DynLinker
 )
fi

if [ "x$TAGVERSION" == "x" ]
then
  echo Error: missing destination tag name
  show_usage
fi

if [ "x$TAGVERSION" != "x" ] ; then
  echo "Tagging as '$TAGVERSION'"
fi 

for i in "${all_fossil_repos[@]}" 
do
  if [ ! -d $i ]; then 
    if [ "x$COTAG" != "x" ] ; then
      fossilopenup $i $COTAG
    else
      assert_success -1 "Required directory does not exist $i"
    fi
  fi
done

for i in "${all_fossil_repos[@]}" 
do
    cd $i

    if [ "x$TAGVERSION" != "x" ] ; then
      fossil tag add "$TAGVERSION" current
      assert_success $? "ERROR: fossil tag add $TAGVERSION on $i"
    fi
    cd -
done

for i in "${all_fossil_repos[@]}" 
do
    cd $i
    
    fossil sync
    assert_success $? "ERROR: fossil sync on $i"
    cd -
done

echo "Done."
# Clean exit
cd $ORIGINAL_DIR
exit 0
