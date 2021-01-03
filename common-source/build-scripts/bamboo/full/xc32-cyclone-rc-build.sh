
if [ -z $bamboo_XC32_SOURCE_BRATAG ]; then
  bamboo_XC32_SOURCE_BRATAG=trunk
fi

if [ -e xc32-${bamboo_XC32_SOURCE_BRATAG} ]; then
  rm -rf xc32-${bamboo_XC32_SOURCE_BRATAG}
  rm xc32-${bamboo_XC32_SOURCE_BRATAG}.log
fi 

./XC32-build-scripts/xc32++-build.sh -b $bamboo_XC32_SOURCE_BRATAG

#Copy readme for uploading 
cp ./xc32-${bamboo_XC32_SOURCE_BRATAG}/Linux32-image/docs/README.html .

deletefiles ()
{
  if [ -e $1 ]; then
    rm -rf $1
  fi
}

#Zip source archive for distribution
find ./xc32-${bamboo_XC32_SOURCE_BRATAG}/source-archive/xc32-tools-xc32-${bamboo_XC32_SOURCE_BRATAG}-src/c30_resource/ ! -name resource.c ! -name resource_types.h ! -name resource_info.h ! -name xc32_flag_definitions.h ! -name xc32_header_definition.h  -type f -delete
rm   ./xc32-${bamboo_XC32_SOURCE_BRATAG}/source-archive/xc32-tools-xc32-${bamboo_XC32_SOURCE_BRATAG}-src/c30_resource/src/c30/resource_info.h
find ./xc32-${bamboo_XC32_SOURCE_BRATAG}/source-archive/xc32-tools-xc32-${bamboo_XC32_SOURCE_BRATAG}-src/c30_resource/ -type d -empty -delete
cd ./xc32-${bamboo_XC32_SOURCE_BRATAG}/source-archive
mv xc32-tools-xc32-${bamboo_XC32_SOURCE_BRATAG}-src xc32-v${bamboo_XC32_FULL_VERSION}-src
zip -r xc32-v${bamboo_XC32_FULL_VERSION}-src.zip xc32-v${bamboo_XC32_FULL_VERSION}-src
cd $bamboo_build_working_directory
cp ./xc32-${bamboo_XC32_SOURCE_BRATAG}/source-archive/xc32-v${bamboo_XC32_FULL_VERSION}-src.zip .

cd xc32-${bamboo_XC32_SOURCE_BRATAG}

deletefiles ./Linux32-image/device_files
deletefiles ./Linux32-image/lib32

deletefiles ./win32-image/device_files
deletefiles ./win32-image/xc32_device.info

cp -r ./win32-image/etc ./Linux32-image

echo "Compressing win32-image"
cd win32-image/pic32-libs
zip -rq ../pic32-libs.zip *
cd ../..
rm -rf win32-image/pic32-libs
mkdir win32-image/pic32-libs
mv win32-image/pic32-libs.zip win32-image/pic32-libs
tar -czf win32-image.tar.gz ./win32-image

echo "Compressing Linux32-image"
cd Linux32-image/pic32-libs
zip -rq ../pic32-libs.zip *
cd ../..
rm -rf Linux32-image/pic32-libs
mkdir Linux32-image/pic32-libs
mv Linux32-image/pic32-libs.zip Linux32-image/pic32-libs
tar -czf Linux32-image.tar.gz ./Linux32-image

cd ..

  

