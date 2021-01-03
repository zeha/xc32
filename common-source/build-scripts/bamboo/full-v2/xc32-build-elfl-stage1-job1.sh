
if ls *.zip 1> /dev/null 2>&1 ; then
  rm *.zip
fi

VERSION=v${bamboo_XC32_FULL_VERSION}
if [[ ! $bamboo_XC32_FULL_BUILD =~ RC ]]; then
  VERSION=${VERSION}-${bamboo_XC32_FULL_BUILD}
fi

if [ -e xc32_${VERSION}_ELF_Loader_Target ]; then
  rm -rf xc32_${VERSION}_ELF_Loader_Target
fi

mkdir xc32_${VERSION}_ELF_Loader_Target

cp -r ./DynLinker/ElfLoader.X ./xc32_${VERSION}_ELF_Loader_Target
zip -r xc32_${VERSION}_ELF_Loader_Target.zip ./xc32_${VERSION}_ELF_Loader_Target
cd DynLinker

make clean
make

zip ../xc32-dl_${VERSION}_linux.zip xc32-dl

make clean
make TARGET=mingw

zip ../xc32-dl_${VERSION}_windows.zip xc32-dl.exe

cd ..

