#!/bin/bash

set -e

export PATH=$HOME/xc32/fossil/script:$PATH
export FOSSIL_CLONES=$HOME/xc32/fossil/clones
export FOSSIL_LOGIN=bamboo:bamboo


fossilklonepull ()
{
  if [ -e ${FOSSIL_CLONES}/$1.fsl ]; then
    echo "Pulling ${FOSSIL_CLONES}/$1.fsl"
    echo "fossil pull -R ${FOSSIL_CLONES}/$1.fsl"
    fossil pull -R ${FOSSIL_CLONES}/$1.fsl
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
  fi

}

fossilopenup ()
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
    TVAL=`fossil tag list -R $FOSSIL_CLONES/${REPO}.fsl | grep "  $BRATAG\$"`
    if [ $? -ne 0 ]; then
      echo "No branch or tag named \"$BRATAG\" in ${REPO}.fsl repo"
      exit 1
    fi
  fi

  if [ ! -d "$REPO" ]; then
    echo "Opening $REPO at ${FOSOPENDIR}/$REPO"
    mkdir $REPO
    cd $REPO
    echo "fossil open ${FOSSIL_CLONES}/${REPO}.fsl $BRATAG" 
    fossil open ${FOSSIL_CLONES}/${REPO}.fsl $BRATAG 
    if [ $? -ne 0 ]; then
      echo "Error opening fossil repo $REPO" 
    fi
  else
    echo "Updating $REPO at ${FOSOPENDIR}/$REPO"
    cd $REPO
    echo "fossil update $BRATAG" 
    fossil update $BRATAG 
    if [ $? -ne 0 ]; then
     echo "Error updating fossil repo $REPO"
    fi
  fi
  cd ..
}

