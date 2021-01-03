source ~/xc32/bashrcv2

if [ -z $bamboo_XC32_SOURCE_BRATAG ]; then
  bamboo_XC32_SOURCE_BRATAG=trunk
fi

if [ -e builddir ]; then
  echo "Deleting existing builddir and log"
  rm -rf builddir
  rm xc32-build-all.log
fi 

./build-scripts/xc32-build-all.sh --jobs=5 --branch=$bamboo_XC32_SOURCE_BRATAG

cd $bamboo_build_working_directory
cd builddir

echo "Compressing images.. PWD is $(pwd)"
tar -czf install-Darwin.tar.gz install-Darwin
cd ..
