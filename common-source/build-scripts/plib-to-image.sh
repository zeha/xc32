#!/bin/bash

echo "START Extracting plibs to install images..."

# Export plibs to install image

DATE=`date +%Y%m%d`
#DATE="20111108"
TIME=`date +%H%M`
TVAL=x
BUILD=pic32-$DATE
DAYTONA_REPO_ROOT=https://chn-vm-piccode/svn/dev-sys/tools/daytona
WORKING_DIR=

NATIVEIMAGE=`uname`
NATIVEIMAGE+="-image"

if [ "$NATIVEIMAGE" == "Linux-image" ] ; then
  NATIVEIMAGE="Linux32-image";
fi

echo "Native image is $NATIVEIMAGE"

if [ "x$NATIVEIMAGE" == "xDarwin-image" ]
then
  LINUX32IMAGE="Linux32-image"
else
  LINUX32IMAGE=""
fi
TAG=trunk

# Process the arguments
while getopts b:FNt:g:D:Q opt
do
  case "$opt" in
     t)
      TVAL=$OPTARG
      TAG="$TVAL"
      ;;
     b)
      TVAL=$OPTARG
      TAG="$TVAL"
      BUILD=pic32-$TVAL-$DATE
      ;;
     g)
      TVAL="$OPTARG"
      TAG="$OPTARG"
      ;;
     D)
      WORKING_DIR="$OPTARG"
      ;;
     N)
     echo "No checkout"
      CHECKOUT="no"
      ;;
    \?) show_usage ;;
  esac
done

####
# assert_success()
#   If the first parameter is non-zero, print the second parameter
#   and exit the script
####
function assert_success ()
{
  local RESULT=$1
  local MESSAGE=$2
  if [ $RESULT != 0 ]
  then
    echo "$MESSAGE ($RESULT)"
    exit $RESULT
  fi
}
echo "Using TAG $TAG"
echo "Using WORKING_DIR $WORKING_DIR"

if [ "x$WORKING_DIR" == "x" ] ; then
  echo "ERROR: Set WORKING_DIR with the -D command-line option"
  exit -1
fi

cd $WORKING_DIR

cd $WORKING_DIR
rm plib-image.tar.bz2
svn export -q $DAYTONA_REPO_ROOT/xc32-plib-export/$TAG/plib-image.tar.bz2
assert_success $? "Export plib-image.tar.bz2 from SVN"

cd win32-image 
tar xjf ../plib-image.tar.bz2
assert_success $? "Explode plib-image.tar.bz2 to win32-image"

cd ..

cd $NATIVEIMAGE 
tar xjf ../plib-image.tar.bz2
assert_success $? "Explode plib-image.tar.bz2 to $NATIVEIMAGE"

cd ..

cd export-image 
tar xjf ../plib-image.tar.bz2
assert_success $? "Explode plib-image.tar.bz2 to export-image"
cd ..
if [ "x$LINUX32IMAGE" != "x" ]; then
  cd $LINUX32IMAGE
  tar xjf ../plib-image.tar.bz2
  assert_success $? "Explode plib-image.tar.bz2 to $LINUX32IMAGE"
  cd ..
fi

exit 0

