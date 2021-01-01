#!/bin/bash
ORIGINAL_DIR=$PWD

show_usage()
{
  # Shows the usage screen
  echo "USAGE:  $0 -s source -b branch"
  echo "  -s source     source version to branch"
  echo "  -b branch     branch name to apply to the release"
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
  GROWL_SEND="growlnotify "
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
  cd $ORIGINAL_DIR
  exit $RESULT
 fi
}

if [ $# = 0 ]
then
  show_usage
  cd $ORIGINAL_DIR
  exit 0
fi

TAGVERSION=
SOURCE=

all_fossil_repos=(
 ./common-source/Coverity
 ./common-source/xc_shell
 ./common-source/XC-installers
 ./pic32m-source/XC-libs
 ./pic32m-source/c30_resource
 ./pic32m-source/fdlibm
 ./pic32m-source/fpmath
 )

# DynLinker
# XC-installers


while getopts s:b: opt
do
  case "$opt" in
    s)
    SOURCEVER=$OPTARG
    ;;
    b)
    BRANCHNAME=$OPTARG
    ;;
    \?) show_usage ;;
  esac
done

if [ "x$BRANCHNAME" == "x" ]
then
  echo "Warning: missing destination branch name"
  echo "         update and sync only"
fi

if [ "x$BRANCHNAME" != "x" ] ; then
  echo "Branching as '$BRANCHNAME'"
fi

for i in "${all_fossil_repos[@]}"
do
  if [ ! -d $i ] ; then
    assert_success -1 "Required directory does not exist $i"
  fi
done

if [ ! -d ./pic32m-source/c30_resource ] ; then
  assert_success -1 "Required directory does not exist ./pic32m-source/c30_resource"
fi

for i in "${all_fossil_repos[@]}"
do
    pushd $i

    if [ "x$SOURCEVER" != "x" ] ; then
      fossil update "$SOURCEVER"
      assert_success $? "ERROR:  fossil update \"$SOURCEVER\" on $i"
    fi

    if [ "x$BRANCHNAME" != "x" ] ; then
      fossil branch new "$BRANCHNAME" current
      assert_success $? "ERROR: fossil branch new $BRANCHNAME on $i"
    fi
    popd
done

for i in "${all_fossil_repos[@]}"
do
    pushd $i

    fossil sync
    assert_success $? "ERROR: fossil sync on $i"
    popd
done

echo "Done."
# Clean exit
cd $ORIGINAL_DIR
exit 0
