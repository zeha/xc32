#!/bin/bash
#
# Compare list of files in the install images against a list in a pre-written manifest
# file.
#
# Generate a manifest file with a command similar to
# find win32-image -name "*" -type f > [path-to-file]/win32-image-manifest.txt
#

BASEDIR=`pwd`

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
    echo "$MESSAGE ($RESULT)" | tee -a $LOGFILE
    exit $RESULT
  fi
} 

show_usage()
{
  # Shows the usage screen
  echo "USAGE:  $0 [-?]"
  echo "  -?            Show this usage message"
  exit 1
}

####
### Main script body
####

LOGFILE=`pwd`/manifest-check.log

# Process the arguments
while getopts Fwdl opt
do
  case "$opt" in
    w) SKIP_WINDOWS="yes" ;;
    d) SKIP_DARWIN="yes" ;;
    l) SKIP_LINUX="yes" ;;
    \?) show_usage ;;
  esac
done

# The manifest files should be in the same directory as this script.
SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "x$SKIP_DARWIN" == "x" ] ; then
	echo `date` "Checking image files against manifest for Darwin-image" | tee -a $LOGFILE

	if [ -e $SCRIPTDIR/Darwin-image-manifest.txt ]; then
	  # Each line in the manifest file should be a filename.
	  while read filename; do
		if [ ! -e "$filename" ] ; then
		  echo `date` " ERROR: Missing '$filename' from Darwin-image" | tee -a $LOGFILE
		  exit 1
		fi
	  done < "$SCRIPTDIR/Darwin-image-manifest.txt"
	else
	  echo `date` " ERROR: $SCRIPTDIR/Darwin-image-manifest.txt not found" | tee -a $LOGFILE
	  exit 1
	fi
fi

if [ "x$SKIP_LINUX" == "x" ] ; then
	echo `date` "Checking image files against manifest for Linux32-image" | tee -a $LOGFILE

	if [ -e $SCRIPTDIR/Linux32-image-manifest.txt ]; then
	  while read filename; do
		if [ ! -e "$filename" ] ; then
		  echo `date` " ERROR: Missing '$filename' from Linux32-image" | tee -a $LOGFILE
		  exit 1
		fi
	  done <  "$SCRIPTDIR/Linux32-image-manifest.txt"
	else
	  echo `date` " ERROR: $SCRIPTDIR/Linux32-image-manifest.txt not found" | tee -a $LOGFILE
	  exit 1
	fi
fi

if [ "x$SKIP_WINDOWS" == "x" ] ; then
	echo `date` "Checking image files against manifest for win32-image" | tee -a $LOGFILE

	if [ -e $SCRIPTDIR/win32-image-manifest.txt ]; then
	  while read filename; do
		if [ ! -e "$filename" ] ; then
		  echo `date` " ERROR: Missing '$filename' from win32-image" | tee -a $LOGFILE
		  exit 1
		fi
	  done < "$SCRIPTDIR/win32-image-manifest.txt"
	else
	  echo `date` " ERROR: $SCRIPTDIR/win32-image-manifest.txt not found" | tee -a $LOGFILE
	  exit 1
	fi
fi

exit 0

