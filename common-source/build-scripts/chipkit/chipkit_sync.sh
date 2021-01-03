#!/bin/bash

WORKING_DIR=`pwd`/chipKIT-cxx
GITHUB_ACCOUNT=chipKIT32
GITHUB_REPO=https://github.com/$GITHUB_ACCOUNT/chipKIT-cxx.git
CHIPKIT_CXX_BRANCH=master
NEW_CHIPKIT_VERSION_BRANCH=$1
XC32_RELEASE_BRANCH=$2
SKIPCLONE=

#TODO  print usage below
#usage: chipkit_sync.sh <new chipkit-CXX release branch name> <XC32 release branch name>

if [ $# -eq 0 ] 
	then 
		echo "Usage: chipkit_sync.sh <new chipkit-CXX release branch name> <XC32 release branch name>"
		exit 1
	fi 

if [ "x$SKIPCLONE" == "x" ]; then
	git clone -b $CHIPKIT_CXX_BRANCH $GITHUB_REPO #$DIR
fi

cd $WORKING_DIR/src48x

# create a new update branch
git checkout -b $NEW_CHIPKIT_VERSION_BRANCH

# fossil export | git import
$FOSSIL_BINARY export --git $FOSSIL_CLONES/XC32-gcc-48.fsl | git fast-import

# merge release branch from fossil into chipkit update branch
git merge --squash -s recursive -Xsubtree=src48x --no-commit $XC32_RELEASE_BRANCH

# conflicts need to be resolved manually because of differences between chipkit
# compiler and xc32

# resolve merge conflicts to "ours" version if we know them
KEEP_OURS_LIST=(
	"binutils/bfd/configure"
	"binutils/bfd/version.h"
	"gcc/gcc/config.gcc"
	"gcc/gcc/config/host-linux.c"
	"gcc/gcc/configure"
	"gcc/gcc/version.c"
	"gcc/libgcc/config.host"
	"gcc/libgcc/config/mips/t-mips"
)

for filename in ${KEEP_OURS_LIST[@]}; do
	git checkout --ours $filename
	git add $filename
done

# TODO apply patch to change version number
VERSION_NUMBER=

exit 0