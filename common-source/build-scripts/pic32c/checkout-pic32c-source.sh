#!/bin/bash

#
#  Copyright 2017 Microchip Technology Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

LOGFILE=`pwd`/checkout-pic32c-source.log

WORKING_DIR=`pwd`
echo WORKING DIR $WORKING_DIR
SOURCE_DIR=${WORKING_DIR}/pic32c-source
mkdir -p $SOURCE_DIR
pushd $SOURCE_DIR


error () {
    set +u
    echo "$0: error: $@" >&2
    cd $WORKING_DIR
    exit 1
    set -u
}

warning () {
    set +u
    echo "$0: warning: $@" >&2
    set -u
}

error_handler ()
{
  cd $WORKING_DIR
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
  GROWL_SEND="growlnotify"
 fi
 unset WHICH_GROWLSEND
fi

function status_update ()
{
 local MESSAGE=$1
  if [ "x$NOTIFY_SEND" != "x" ] ; then
   $NOTIFY_SEND "$MESSAGE" "checkout-pic32c-source.sh"
  elif [ "x$GROWL_SEND" != "x" ] ; then
    $GROWL_SEND -p low -n build-pic32c -m "$MESSAGE"
  fi
  echo "status: $MESSAGE"
  echo `date` "status: $MESSAGE" >> $LOGFILE
}

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
   $NOTIFY_SEND "$MESSAGE" "XC32 Build Error"
  elif [ "x$GROWL_SEND" != "x" ] ; then
    $GROWL_SEND -p high -n build-pic32c -sm "XC32 Build Error: $MESSAGE"
  fi
  echo "$MESSAGE ($RESULT)" >> $LOGFILE
  error_handler
 fi
}


# This file contains the sequence of commands used to checkout the XC32 PIC32C source
usage ()
{
 # Shows the usage screen
 echo "USAGE: $0 [-t <tag> | -b <branch> | -c <commit> | -?]"
 echo " -b <branch> Specify the branch for which you would like to build"
 echo " -t <tag>    Specify the tag for which you would like to build"
 echo " -c <commit> Specify the checkout commit for Git repositories"
 echo " -?          Show this usage message"
 exit 1
}

if [ -z "$bamboo_XC32_XCLM_BRANCH" ]; then
  XCLM_BRANCH=master
else
  XCLM_BRANCH=$bamboo_XC32_XCLM_BRANCH
fi 

CVSB="trunk"

if [ ! "x$FOSSIL_PASSWORD" = "x" ] ; then
  FOSSIL_PASSWORD_AT=":$FOSSIL_PASSWORD@"
fi

if [ "x$FOSSIL_SERVER" = "x" ] ; then
  FOSSIL_SERVER=fossil.microchip.com
fi

# order of repos must correspond to order of directories in all_fossil_directories
all_fossil_repos=(
)
# order of directories must correspond to order of repos in all_fossil_repos
all_fossil_directories=(
)

# Same, but for Git.
all_git_repos=(
 xc32/pic32c-gcc
 xc32/pic32c-libs
 xc32/pic32c-headers_generator
 xclm/xclm-release
)
all_git_directories=(
 XC32-arm-gcc
 pic32c-libs
 pic32c-headers_generator
 xclm_release
)

TAG="x"
BRANCH="x"
FULL_ONLY=no
CHECKOUT_COMMIT=master
PIC32C_HEADERS_GENERATOR_COMMIT=dev_xc32_part_support

# Process the arguments
while getopts t:b:c: opt
do
 case "$opt" in
   t)
   TVAL=$OPTARG
   TAG="$TVAL"
   CVSB=$TVAL
   ;;
   b)
   TVAL=$OPTARG
   BRANCH="$TVAL"
   BUILD=pic32-$TVAL-$DATE
   CVSB=$TVAL
   ;;
   c)
   CHECKOUT_COMMIT="${OPTARG}"
   ;;
  \?) usage ;;
 esac
done

#
# Verify existance of local clones. Sync if exist, create new clones if necessary
#

if [ "x$FOSSIL_CLONES" == "x" ] ; then
  error "ERROR: FOSSIL_CLONES environment variable not set"
fi

if [ -f $FOSSIL_CLONES ] ; then
  error "ERROR: $FOSSIL_CLONES clone dir does not exist"
else
  status_update "Using FOSSIL_CLONES=$FOSSIL_CLONES"
fi

# Set the base URL for Git clones. The default assumes the user has an
# SSH key that can authenticate to the server without any input.
: ${XC32_GIT_URL:=ssh://git@bitbucket.microchip.com:7999}
# Strip the trailing /, if any.
XC32_GIT_URL="${XC32_GIT_URL%/}"
status_update "Using XC32_GIT_URL=${XC32_GIT_URL}"

#
# Verify that we can reach the remote server before attempting to sync the repositories
#
ping -q -c 1 $FOSSIL_SERVER
if [ ! $? == 0 ]; then
  error "Fossil server $FOSSIL_SERVER could not be reached, not syncing"
else
  pushd $FOSSIL_CLONES > /dev/null
  status_update "Sync local repository clones"
  for i in "${all_fossil_repos[@]}"
  do
    if [[ -f $i.fsl ]]
    then
       echo "fossil sync -R $i.fsl"
       fossil sync -R $i.fsl
       assert_success $? "ERROR: fossil sync on $i"
    else
           VERSTR=`fossil version`
           if [[ $VERSTR =~ MCHP ]]; then
          echo "yes | fossil klone $i"
          yes | fossil klone $i
           else
          echo "fossil klone $i"
          fossil klone $i
           fi
      assert_success $? "ERROR: fossil klone on $i"
    fi
  done
  popd > /dev/null
fi

#
# Make sure the branches or tags exist
#
TAGOK=1;
status_update "Checking for tag/branch in repository"
for i in "${all_fossil_repos[@]}"
do
if [[ $i == "xclm_release" ]]; then
  continue
fi
if [[ $i == "Coverity" ]]; then
  continue
fi
if [[ $BRANCH != "x" ]]; then
    TVAL=`fossil branch list -R $FOSSIL_CLONES/$i.fsl | grep "  $BRANCH\$" | wc -l `
    if [ $TVAL == 1 ]; then
     echo "OK : $i.fsl : $BRANCH "
     FOSSIL_VERSION=$BRANCH
    else
     echo "ERROR: Missing $i.fsl : $BRANCH "
     TAGOK=0;
    fi
elif [[ $TAG != "x" ]]; then
    TVAL=`fossil tag list -R $FOSSIL_CLONES/$i.fsl | grep "$TAG\$" | wc -l `
    if [ $TVAL == 1 ]; then
     echo "OK : $i.fsl : $TAG "
     FOSSIL_VERSION=$TAG
    else
     echo "ERROR: Missing $i.fsl : $TAG "
     TAGOK=0;
    fi
else
    BRANCH="trunk"
    TAG="trunk"
    TVAL=`fossil branch list -R $FOSSIL_CLONES/$i.fsl | grep "  $BRANCH\$" | wc -l `
    if [ $TVAL == 1 ]; then
     echo "OK : $i.fsl : $BRANCH "
     FOSSIL_VERSION=$BRANCH
    else
     echo "ERROR: Missing $i.fsl : $BRANCH "
     TAGOK=0;
    fi
fi
done

if [ $TAGOK -eq 0 ] ; then
  if [[ $BRANCH != "x" ]]; then
    error "Missing branches '$BRANCH'"
  elif [[ $TAG != "x" ]]; then
    error "Missing tags '$TAG'"
  fi
fi

# We still use an SVN-style Branch/Tag for some modules
if [[ $TAG == "x" && $BRANCH != "x" ]]
then
 SVNTAG="branches/$BRANCH"
else
 if [ $TAG == "trunk" ]; then
   SVNTAG="trunk"
 else
   SVTAG="tags/$TAG"
 fi
fi

echo "Using XCLM branch $XCLM_BRANCH"

#
# Fossil open the repositories
#

status_update "Checking out the pic32c source code"
count_dirs=${#all_fossil_directories[*]}

for (( i=0; i<=$(( $count_dirs -1 )); i++ ))
do
  echo "Checking out source from ${all_fossil_repos[$i]} to ${all_fossil_directories[$i]}..." >> $LOGFILE
  if [ -e ${all_fossil_directories[$i]} ] ; then
    rm -rf ${all_fossil_directories[$i]}
  fi
  mkdir ${all_fossil_directories[$i]}
  assert_success $? "ERROR: mkdir ${all_fossil_directories[$i]}"
  pushd ${all_fossil_directories[$i]} > /dev/null
  if [[ "${all_fossil_repos[$i]}" == "xclm_release" ]] ; then
    fossil open $FOSSIL_CLONES/${all_fossil_repos[$i]}.fsl $XCLM_BRANCH > /dev/null
  elif [[ "${all_fossil_repos[$i]}" == "Coverity" ]] ; then
    fossil open $FOSSIL_CLONES/${all_fossil_repos[$i]}.fsl trunk > /dev/null
  else
    fossil open $FOSSIL_CLONES/${all_fossil_repos[$i]}.fsl $FOSSIL_VERSION > /dev/null
  fi
  assert_success $? "ERROR: fossil open $FOSSIL_CLONES/${all_fossil_repos[$i]}"
  popd > /dev/null
done

set -eu
for (( i=0; i<$(( ${#all_git_directories[*]} )); i++ ))
do
  gitrepo="${all_git_repos[$i]}"
  gitdir="${all_git_directories[$i]}"
  gitname="${gitrepo#*/}"

  commitid="${CHECKOUT_COMMIT}"
  if [[ "${gitname}" == xclm-release ]]; then
      commitid="${XCLM_BRANCH}"
  fi

  echo "Checking out source from ${gitname} to ${gitdir}" >> ${LOGFILE}
  if [[ ! -d "${gitdir}" ]]; then
      echo "Cloning Git repository ${gitname}" >> ${LOGFILE}
      (
        PS4=""; exec 2>> ${LOGFILE}; set -x
        git clone -q "${XC32_GIT_URL}/${gitrepo}.git" "${gitdir}"
      )
  fi
  if [[ "${gitname}" != pic32c-headers_generator ]]; then
      (
          PS4=""; exec 2>> ${LOGFILE}; set -x
          git -C "${gitdir}" pull --quiet origin
          git -C "${gitdir}" checkout "${commitid}"
      )
  fi
done
set +eu

cd $SOURCE_DIR
pushd xclm_release
cp ./dist/api/xclm_public.h $SOURCE_DIR/XC32-arm-gcc/gcc/gcc/config/mchp-cci/xclm_public.h
assert_success $? "ERROR: copy xclm_public.h"
popd > /dev/null

cd $WORKING_DIR
status_update "Source checkout for PIC32C complete."
exit 0

