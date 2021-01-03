source ~/xc32/bashrc

if [ -z $bamboo_XC32_SOURCE_BRATAG ]; then
  bamboo_XC32_SOURCE_BRATAG=trunk
fi

if [ -e xc32-${bamboo_XC32_SOURCE_BRATAG} ]; then
  rm -rf xc32-${bamboo_XC32_SOURCE_BRATAG}
  rm xc32-${bamboo_XC32_SOURCE_BRATAG}.log
fi 

./XC32-build-scripts/xc32++-build.sh -b $bamboo_XC32_SOURCE_BRATAG

deletefiles ()
{
  if [ -e $1 ]; then
    rm -rf $1
  fi
}

cd xc32-${bamboo_XC32_SOURCE_BRATAG}

deletefiles ./Darwin-image/device_files
deletefiles ./Darwin-image/docs
deletefiles ./Darwin-image/etc
deletefiles ./Darwin-image/examples
deletefiles ./Darwin-image/lib
deletefiles ./Darwin-image/pic32mx/bin/device_files
deletefiles ./Darwin-image/pic32mx/device_files
deletefiles ./Darwin-image/pic32mx/include
deletefiles ./Darwin-image/pic32mx/lib
deletefiles ./Darwin-image/pic32mx/xc32_device.info
deletefiles ./Darwin-image/pic32-libs
deletefiles ./Darwin-image/share

deletefiles ./Darwin-image/bin/device_files
deletefiles ./Darwin-image/bin/deviceSupport.xml
deletefiles ./Darwin-image/bin/xc32_device.info

echo "Compressing Darwin-image"
tar -czf Darwin-image.tar.gz ./Darwin-image

cd ..


