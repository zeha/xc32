#!/bin/bash

SCRIPTROOT=`dirname $0`
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

fossilopenup $FOREPP32LIB $bamboo_FOSSIL_BRANCH

echo "cp -r ${IMAGEDIR}/pic32-libs ${FOSOPENDIR}"
cp -r ${IMAGEDIR}/pic32-libs ${FOSOPENDIR}

FP32LIBDIR=${FOSOPENDIR}/$FOREPP32LIB
pushd $FP32LIBDIR
fossil status > ../pic32-libs.status.log
fossil diff > ../pic32-libs.diff.log
fossil extra > ../pic32-libs.newfiles.log
popd

pushd ${FOSOPENDIR}
fossilopenup c30_resource $bamboo_FOSSIL_BRANCH
popd

echo "cp ${PYOUTDIR}/xc32_DVSdevices.res ${FOSOPENDIR}/c30_resource/src/xc32"
cp ${PYOUTDIR}/xc32_DVSdevices.res ${FOSOPENDIR}/c30_resource/src/xc32

pushd ${FOSOPENDIR}/c30_resource
fossil status > ../c30_resource.status.log
fossil diff > ../c30_resource.diff.log
popd

pushd ${FOSOPENDIR}
fossilopenup XC32-docs $bamboo_FOSSIL_BRANCH
popd

echo "cp ${PYOUTDIR}/PIC32ConfigSet.html ${FOSOPENDIR}/XC32-docs"
cp ${PYOUTDIR}/PIC32ConfigSet.html ${FOSOPENDIR}/XC32-docs
echo "cp -r ${PYOUTDIR}/config_docs ${FOSOPENDIR}/XC32-docs/config_docs"
cp -r ${PYOUTDIR}/config_docs ${FOSOPENDIR}/XC32-docs/config_docs

pushd ${FOSOPENDIR}/XC32-docs
fossil status > ../XC32-docs.status.log
fossil diff > ../XC32-docs.diff.log
fossil extra > ../XC32-docs.newfiles.log
popd

