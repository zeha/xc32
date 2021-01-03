#!/bin/bash


SCRIPTROOT=`dirname $0`
PYDIR=${SCRIPTROOT}/../../py/extlibs

source $SCRIPTROOT/../../pic32-parts-inc.sh
source ${SCRIPTROOT}/xc32-full-bamboo-inc.sh

SCRIPTARGS="-v${bamboo_XC32_FULL_VERSION} -r${bamboo_XC32_FULL_RESOURCE}"

if [ ! -z $bamboo_FOSSIL_BRANCH ]; then
  SCRIPTARGS=${SCRIPTARGS}" -t$bamboo_FOSSIL_BRANCH"
fi

echo "${SCRIPTROOT}/../../pic32-parts-gen.sh ${SCRIPTARGS}"
${SCRIPTROOT}/../../pic32-parts-gen.sh ${SCRIPTARGS}
checkexitcode $? "pic32-parts-gen.sh returned non-zero value" $LINENO


echo "${SCRIPTROOT}/../../pic32-parts-create-image.sh ${SCRIPTARGS}"
${SCRIPTROOT}/../../pic32-parts-create-image.sh ${SCRIPTARGS}
checkexitcode $? "pic32-parts-create-image.sh returned non-zero value" $LINENO

#Smoke test the generated part-support
source ${SCRIPTROOT}/../shell2junit/sh2ju.sh
source ${SCRIPTROOT}/xc32-full-bamboo-smoke-parts.sh

set -e

#Checkout pic32m-libs from Git and add new and modified files
if [ "x$GP32MLIBSDIR" == "x" ] ; then
  echo "ERROR: xc32-regenerate-parts.sh -- GP32MLIBSDIR not defined"
  exit 1
fi
rm -rf ${GP32MLIBSDIR}
gitclone $GITP32MLIBS ${bamboo_GIT_BRANCH}

echo "cp -r ${IMAGEDIR}/pic32-libs/* ${GP32MLIBSDIR}"
cp -r ${IMAGEDIR}/pic32-libs/* ${GP32MLIBSDIR}

pushd ${GP32MLIBSDIR}

git status > ${ROOTDIR}/pic32m-libs.status.log
git diff > ${ROOTDIR}/pic32m-libs.diff.log

popd

#Checkout pic32m-docs from Git and add new and modified files
if [ "x$GP32MDOCSDIR" == "x" ] ; then
  echo "ERROR: xc32-regenerate-parts.sh -- GP32MDOCSDIR not defined"
  exit 1
fi
rm -rf ${GP32MDOCSDIR}
gitclone $GITP32MDOCS ${bamboo_GIT_BRANCH}

echo "cp ${PYOUTDIR}/PIC32ConfigSet.html ${GP32MDOCSDIR}"
cp ${PYOUTDIR}/PIC32ConfigSet.html ${GP32MDOCSDIR}
echo "cp -r ${PYOUTDIR}/config_docs/* ${GP32MDOCSDIR}/config_docs/"
cp -r ${PYOUTDIR}/config_docs/* ${GP32MDOCSDIR}/config_docs/

pushd ${GP32MDOCSDIR}

git status > ${ROOTDIR}/pic32m-docs.status.log
git diff > ${ROOTDIR}/pic32m-docs.diff.log

popd

#Checkout c30_resource from Fossil and add new and modified files
if [ "x$FOSOPENDIR" == "x" ] ; then
  echo "ERROR: xc32-regenerate-parts.sh -- FOSOPENDIR not defined"
  exit 1
fi
rm -rf ${FOSOPENDIR}/c30_resource
fossilopenup c30_resource $bamboo_FOSSIL_BRANCH

echo "cp ${ROOTDIR}/${PYOUTDIR}/xc32_DVSdevices.res ${FOSOPENDIR}/c30_resource/src/xc32"
cp ${ROOTDIR}/${PYOUTDIR}/xc32_DVSdevices.res ${FOSOPENDIR}/c30_resource/src/xc32

pushd ${FOSOPENDIR}/c30_resource

fossil status > ${ROOTDIR}/c30_resource.status.log
fossil diff > ${ROOTDIR}/c30_resource.diff.log
popd

DATE=$(date)
pushd ${FOSOPENDIR}
fossiladdcommit c30_resource "Bamboo check-in from 'RC generate part-support' build #${bamboo_buildNumber} on $DATE"

gitaddpush pic32m-libs "Bamboo check-in from 'RC generate part-support' build #${bamboo_buildNumber} on $DATE"
gitaddpush pic32m-docs "Bamboo check-in from 'RC generate part-support' build #${bamboo_buildNumber} on $DATE"
popd
