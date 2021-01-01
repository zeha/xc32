
if ls *.zip 1> /dev/null 2>&1 ; then
  rm *.zip
fi

cd DynLinker
make clean
make

VERSION=v${bamboo_XC32_FULL_VERSION}
if [[ ! $bamboo_XC32_FULL_BUILD =~ RC ]]; then
  VERSION=${VERSION}-${bamboo_XC32_FULL_BUILD} 
fi 
zip ../xc32-dl_${VERSION}_osx.zip xc32-dl

cd ..

