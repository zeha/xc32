
deletefiles ()
{
  if [ -e $1 ]; then
    rm -rf $1
  fi
}


fossilopenup verifyinst
fossilopenup XC-installers $bamboo_XC32_XC_INSTALLERS_BRANCH

deletefiles install
mv XC-installers install

#Prepare images
set -e

echo "Unpaking Linux image"
tar -xzf Linux32-image.tar.gz

echo "Unpaking Win image"
tar -xzf win32-image.tar.gz

echo "Unpaking OSX binaries only image"
tar -xzf Darwin-image.tar.gz

cp -r ./Linux32-image/docs       ./Darwin-image/
cp -r ./Linux32-image/etc        ./Darwin-image/
cp -r ./Linux32-image/examples   ./Darwin-image/
cp -r ./Linux32-image/lib        ./Darwin-image/

#cp -r ./Linux32-image/pic32mx/bin/device_files ./Darwin-image/pic32mx/bin
#cp -r ./Linux32-image/pic32mx/device_files     ./Darwin-image/pic32mx
cp -r ./Linux32-image/pic32mx/include          ./Darwin-image/pic32mx
cp -r ./Linux32-image/pic32mx/lib              ./Darwin-image/pic32mx
#cp    ./Linux32-image/pic32mx/xc32_device.info ./Darwin-image/pic32mx

rm -rf ./Darwin-image/pic32-libs/* 
mkdir -p ./Darwin-image/pic32-libs
cp    ./Linux32-image/pic32-libs/pic32-libs.zip   ./Darwin-image/pic32-libs/pic32-libs.zip
cp -r ./Linux32-image/bin/device_files      ./Darwin-image/bin
cp    ./Linux32-image/bin/deviceSupport.xml ./Darwin-image/bin
cp    ./Linux32-image/bin/xc32_device.info  ./Darwin-image/bin

#Replace readme, if required 
if [ -z $bamboo_UPDATE_README_FROM_BRANCH ]; then
 echo "Retaining readme in image as plan variable UPDATE_README_FROM_BRANCH is not set"
else
 fossilopenup XC32-docs $bamboo_UPDATE_README_FROM_BRANCH 
 >&2 echo "Updating readme and user guides from branch $bamboo_UPDATE_README_FROM_BRANCH"
 >&2 echo "Unset paln variable $bamboo_UPDATE_README_FROM_BRANCH to prevent this"
 
 echo "Copying readmes"
 cp ./XC32-docs/README.html ./Linux32-image/docs/README.html
 cp ./XC32-docs/README.html ./Darwin-image/docs/README.html
 cp ./XC32-docs/README.html ./win32-image/docs/README.html
 cp ./XC32-docs/README.html ./README.html

 echo "Removing old user guides"
 rm ./Linux32-image/docs/*Guide.pdf
 rm ./Darwin-image/docs/*Guide.pdf
 rm ./win32-image/docs/*Guide.pdf

 echo "Copying user guides"
 cp ./XC32-docs/*Guide.pdf ./Linux32-image/docs/
 cp ./XC32-docs/*Guide.pdf ./Darwin-image/docs/
 cp ./XC32-docs/*Guide.pdf ./win32-image/docs/

fi

WORKDIR=$(pwd)

# We copy 32-bit xclm by default to bin, this helps is unit-testing 
# Linux32-image before building installers. Not getting rid of it before 
# generating the manifest file will result in verify failuer on x86_64 host
if [ -f ${WORKDIR}/Linux32-image/bin/xclm ]; then
  rm ${WORKDIR}/Linux32-image/bin/xclm
fi

cd verifyinst
./deploy.sh -l${WORKDIR}/Linux32-image -d${WORKDIR}/Darwin-image -w${WORKDIR}/win32-image -ixclm -ixclm64
cd $WORKDIR

./XC32-build-scripts/bamboo/full/xc32-build-installer-legacy.sh -v${bamboo_XC32_FULL_VERSION} -b${bamboo_XC32_FULL_BUILD}
