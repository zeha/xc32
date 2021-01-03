
if [ -z $bamboo_XC32_SOURCE_BRATAG ]; then
  bamboo_XC32_SOURCE_BRATAG=trunk
fi

if [ -e builddir ]; then
  echo "Deleting existing builddir and log"
  rm -rf builddir
  rm xc32-build-all.log
fi 

./build-scripts/xc32-build-all.sh --jobs=4 --branch=$bamboo_XC32_SOURCE_BRATAG

cd $bamboo_build_working_directory
cd builddir

echo "Compressing images.. PWD is $(pwd)"
tar -czf install-Linux.tar.gz install-Linux
tar -czf install-Linux-nolm.tar.gz install-Linux-nolm
tar -czf install-mingw.tar.gz install-mingw
cd ..

