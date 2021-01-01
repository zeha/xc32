
deletefiles ()
{
  if [ -e $1 ]; then
    rm -rf $1
  fi
}

fossilopenup verifyinst $bamboo_XC32_XC_INSTALLERS_BRANCH
fossilopenup XC-installers $bamboo_XC32_XC_INSTALLERS_BRANCH

deletefiles install
mv XC-installers install

#Prepare images
set -e

echo "Unpacking Linux image"
tar -xzf install-Linux.tar.gz
echo "Fixing Linux image"
cd install-Linux/bin/bin
if [ -L pic32c-cc ]; then
 rm pic32c-cc
 cp pic32c-gcc pic32c-cc
fi 
cd ../../../
find ./install-Linux -name 'libstdc++*-gdb.py' -delete

echo "Unpacking mingw image"
tar -xzf install-mingw.tar.gz
echo "Fixing mingw image"
cd install-mingw/bin/bin
if [ -L pic32c-cc ]; then
 rm pic32c-cc
 cp pic32c-gcc pic32c-cc
fi 
cd ../../../
find ./install-mingw -name 'libstdc++*-gdb.py' -delete

if [[ "x${bamboo_SKIP_DARWIN}" != "xtrue" && "x${bamboo_SKIP_DARWIN}" != "xTRUE" ]]; then
  echo "Unpacking Darwin image"
  tar -xzf install-Darwin.tar.gz
  echo "Fixing Darwin image"
  cd install-Darwin/bin/bin
  if [ -L pic32c-cc ]; then
   rm pic32c-cc
   cp pic32c-gcc pic32c-cc
  fi 
  cd ../../../
  find ./install-Darwin -name 'libstdc++*-gdb.py' -delete

  echo "Code Signing OSX binaries"
  ./build-scripts/bamboo/full-v2/xc_codesign.sh install-Darwin xc32
fi

#Replace readme, if required 
if [ -z $bamboo_UPDATE_README_FROM_BRANCH ]; then
 echo "Retaining readme in image as plan variable UPDATE_README_FROM_BRANCH is not set"
else
# echo "Uncomment else block in bamboo/full-v2/xc32-build-installer.sh"
# exit 1
 cd pic32m-docs
 git checkout $bamboo_UPDATE_README_FROM_BRANCH 
 cd ..
 >&2 echo "Updating readme and user guides from branch $bamboo_UPDATE_README_FROM_BRANCH"
 >&2 echo "Unset plan variable $bamboo_UPDATE_README_FROM_BRANCH to prevent this"
 
 echo "Copying readmes"
 cp ./pic32m-docs/README.html ./install-Linux/docs/README.html
 cp ./pic32m-docs/README.html ./install-Darwin/docs/README.html
 cp ./pic32m-docs/README.html ./install-mingw/docs/README.html
 cp ./pic32m-docs/README.html ./README.html

 echo "Removing old user guides"
 rm ./install-Linux/docs/*Guide.pdf
 rm ./install-Darwin/docs/*Guide.pdf
 rm ./install-mingw/docs/*Guide.pdf

 echo "Copying user guides"
 cp ./pic32m-docs/*Guide.pdf ./install-Linux/docs/
 cp ./pic32m-docs/*Guide.pdf ./install-Darwin/docs/
 cp ./pic32m-docs/*Guide.pdf ./install-mingw/docs/

fi


WORKDIR=$(pwd)

cd verifyinst
if [[ "x${bamboo_SKIP_DARWIN}" != "xtrue" && "x${bamboo_SKIP_DARWIN}" != "xTRUE" ]]; then
  ./deploy.sh -l${WORKDIR}/install-Linux -d${WORKDIR}/install-Darwin -w${WORKDIR}/install-mingw -iinclude -ilib32 -ishare -ixclm -ixclm64 
else
  ./deploy.sh -l${WORKDIR}/install-Linux -w${WORKDIR}/install-mingw -iinclude -ilib32 -ishare -ixclm -ixclm64 
fi
cd $WORKDIR

./build-scripts/bamboo/full-v2/xc32-invoke-bitrock.sh -v${bamboo_XC32_FULL_VERSION} -b${bamboo_XC32_FULL_BUILD}
