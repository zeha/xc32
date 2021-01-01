#!/bin/bash

# This is a temporary script to generate SAM part-support
# It will be replaced by DFP harvesting as soon as that is ready

# Prerequisites
# * brew install ant
# * Download ivy.jar from http://ant.apache.org/ivy/download.html and copy to ${HOME}/.ant/lib
# * brew install python@2
# * pip install lxml
# * pip install jinja2
# * brew install dos2unix

set -e
SCRIPTROOT=$( cd "$( dirname "$0" )" && pwd )
RELEASE_BRANCH=release/XC32_2.20
#RELEASE_BRANCH=master
ROOT=`pwd`

makecleandir ()
{
	rm -rf $1
	mkdir -p $1
}

# Function to create temp directories 
maketempdir ()
{
	makecleandir ./SAM-gen-temp
	pushd ./SAM-gen-temp
	
	makecleandir ./atdf_source
	pushd ./atdf_source  > /dev/null
	ATDF_PATH=`pwd`
	popd  > /dev/null
	makecleandir ./pic_source
	pushd ./pic_source  > /dev/null
	PIC_PATH=`pwd`
	popd  > /dev/null
	
	makecleandir ./output
	pushd ./output > /dev/null
	OUTPUT_PATH=`pwd`
	popd > /dev/null
	
	popd
}

#Clone a git repo and checkout specific branch
#  $1 - GIT URL
#  $2 - Repository name 
#  $3 - Original branch name
#  $4 - new branch for regeneration
gitclone()
{
  local GITURL=$1
  local REPO=$2
  local ORIGIN_BRANCH=$3
  local TARGET_BRANCH=$4
  
  git clone ${GITURL}/${REPO}.git

  pushd ${REPO}

  if [[ "$ORIGIN_BRANCH" != "" ]]; then
    git checkout $ORIGIN_BRANCH
  fi

  if [[ "$TARGET_BRANCH" != "" ]]; then
    git checkout -b $TARGET_BRANCH
  fi
  popd
}
# Clone the repos that we need
getrepos ()
{
	pushd ./SAM-gen-temp
	gitclone ssh://git@bitbucket.microchip.com/dtd earlydfpsoften

	gitclone ssh://git@bitbucket.microchip.com/xc32 pic32c-headers_generator dev_xc32_part_support feature/regenerate

	PIC32C_HEADERS_GENERATOR_PATH=`pwd`/pic32c-headers_generator
	PYTHONPATH=${PIC32C_HEADERS_GENERATOR_PATH}/xml2h
	gitclone ssh://git@bitbucket.microchip.com/xc32 pic32c-libs ${RELEASE_BRANCH} feature/regenerate
	PIC32C_LIBS_PATH=`pwd`/pic32c-libs
	gitclone ssh://git@bitbucket.microchip.com/xc32 pic32m-docs ${RELEASE_BRANCH} feature/regenerate
	PIC32M_DOCS_PATH=`pwd`/pic32m-docs
	popd
}

getdfps ()
{
	pushd ./SAM-gen-temp/earlydfpsoften
	./setup.sh
	./clean.sh
	ant latest-32bit-dfps
	# Remove PIC32M (MIPS) DFPs. PIC32M part support is currently handled separately.
	cd ./packs/Microchip
	rm -rf PIC32MK-GP_DFP
	rm -rf PIC32MK-MC_DFP
	rm -rf PIC32MM-GPL-0XX_DFP
	rm -rf PIC32MM-GPM-0XX_DFP
	rm -rf PIC32MX_DFP
	rm -rf PIC32MZ-DA_DFP
	rm -rf PIC32MZ-EC_DFP
	rm -rf PIC32MZ-EF_DFP
	rm -rf PIC32MZ-W_DFP
	rm -rf PIC32WK_DFP
	rm -rf PIC32bit-Development-Only_DFP
	rm -rf PIC32bit-Internal-Only_DFP

	# The MECn2xx is a MIPS device. The MECn7xx is an ARM device.
	rm -rf MECxxxx_DFP
	
	# Not for XC32 v2.20
	rm -rf SAMA5D3_DEV_DFP
	rm -rf *DEV_DFP
	rm -rf SAMA5D3_DFP
	rm -rf SAM4L_DFP
	rm -rf SAMG_DFP
	rm -rf SAMD09_DFP
	rm -rf SAMD10_DFP
	rm -rf SAMD11_DFP
	rm -rf SAMDA1_DFP

	popd
}

copyatdffiles ()
{
	pushd ./SAM-gen-temp/earlydfpsoften/packs
	find . -name "*.atdf" -exec cp \{\} ../../atdf_source/ \;
	cd ../../atdf_source
	ATDF_SOURCE=`pwd`
	echo ATDF_SOURCE=$ATDF_SOURCE
	popd
	
	# Remove cancelled devices
	pushd ./SAM-gen-temp/atdf_source
	if [ -e PIC32CX0025SM10128.atdf ]; then
	  rm PIC32CX0025SM10128.atdf
	fi
	popd
	pushd ./SAM-gen-temp/pic_source
	if [ -e PIC32CX0025SM10128.PIC ]; then
	  rm PIC32CX0025SM10128.PIC
	fi
	popd
}

copypicfiles ()
{
	pushd ./SAM-gen-temp/earlydfpsoften/packs
	find . -name "*.PIC" -exec cp \{\} ../../pic_source/ \;
	cd ../../pic_source
	PIC_SOURCE=`pwd`
	echo PIC_SOURCE=$PIC_SOURCE
	popd
}

generatestartup ()
{
	pushd ./SAM-gen-temp/pic32c-headers_generator/xml2h
	
	PYTHONPATH=${PYTHONPATH} ATDF_PATH=${ATDF_PATH} OUTPUT_PATH=${OUTPUT_PATH} make -f Makefile-xc32 startups -j5
	cd ${OUTPUT_PATH}/xc32
	find . -type d -exec mkdir -p ${PIC32C_LIBS_PATH}/proc/\{\} \;
	find . -name "startup*.c" -exec cp \{\} ${PIC32C_LIBS_PATH}/proc/\{\} \;
	find . -name "startup*.S" -exec cp \{\} ${PIC32C_LIBS_PATH}/proc/\{\} \;
	echo NOTE: Copied new startup*.c to ${PIC32C_LIBS_PATH}/proc/
	popd
}

generatelinkerscripts ()
{
	pushd ./SAM-gen-temp/pic32c-headers_generator/xml2h
	
	PYTHONPATH=${PYTHONPATH} ATDF_PATH=${ATDF_PATH} OUTPUT_PATH=${OUTPUT_PATH} make -f Makefile-xc32 linkerscripts -j5
	cd ${OUTPUT_PATH}/xc32
	find . -type d -exec mkdir -p ${PIC32C_LIBS_PATH}/proc/\{\} \;
	find . -name "*.ld" -exec cp \{\} ${PIC32C_LIBS_PATH}/proc/\{\} \;  
	echo NOTE: Copied new *.ld to ${PIC32C_LIBS_PATH}/proc/
	popd
}

generatexch ()
{
	pushd ./SAM-gen-temp/pic32c-headers_generator/xml2h
	
	PYTHONPATH=${PYTHONPATH} ATDF_PATH=${ATDF_PATH} OUTPUT_PATH=${OUTPUT_PATH} make -f Makefile-xc32 xc_h
	cd ${OUTPUT_PATH}/xc32
	echo WARNING: include/xc.h and include_mcc/xc.h must be manually generated for now
#	cp xc.h ${PIC32C_LIBS_PATH}/include/
#	echo NOTE: Copied new xc.h to ${PIC32C_LIBS_PATH}/include/
#	cp mcc_xc.h ${PIC32C_LIBS_PATH}/include_mcc/xc.h
#	echo NOTE: Copied new xc.h to ${PIC32C_LIBS_PATH}/include_mcc/


	popd
}

generatemcudefs ()
{
	pushd ./SAM-gen-temp/pic32c-headers_generator/xml2h
	
	PYTHONPATH=${PYTHONPATH} ATDF_PATH=${ATDF_PATH} OUTPUT_PATH=${OUTPUT_PATH} make -f Makefile-xc32 pic32c_mcus
	popd

	cp ${OUTPUT_PATH}/xc32/pic32c-mcus.def ./SAM-gen-temp/pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen/include/pic32c-mcus.def
	echo NOTE: Copied new pic32c-mcus.def to pic32c-headers_generator/xml2h/mchp/xc/pic32c/specs_gen/include/
}

copyheaderfiles ()
{
	pushd ./SAM-gen-temp/earlydfpsoften/packs

        if [[ -z $(find . -type d -name "include_mcc") ]]; then
          include_mcc_src=include
          skip_include=yes
        else
          include_mcc_src=include_mcc
          skip_include=""
        fi
	
	find . -name ${include_mcc_src} > include_mcc.txt
	for dir in `cat include_mcc.txt`; do
		pushd ${dir} > /dev/null
		cd ..
		family=$(basename `pwd`)
		family_upper=$(echo $family | awk '{print toupper($0)}')
		if [[ ${family_upper}[0] =~ ^[^0-9] ]] ; then
			cd include_mcc
			find . -type d -exec mkdir -p ${PIC32C_LIBS_PATH}/include_mcc/proc/${family_upper}/\{\} \;
			find . -name "*.h" -exec cp \{\} ${PIC32C_LIBS_PATH}/include_mcc/proc/${family_upper}/\{\} \;
			popd > /dev/null
		else :
			version=$family_upper
			cd ..
			family_dir=`pwd`
			cd ..
			base_dir=`pwd`
			family_upper=${family_dir#${base_dir}/}
			family_upper=${family_upper%'_DFP'}
			family_upper=${family_upper%'-DEV'}
			family_upper=${family_upper%'_DEV'}
			cd ${family_dir}/${version}/include_mcc
			echo Copying ${family_dir}/${version}/include_mcc to ${PIC32C_LIBS_PATH}/include_mcc/proc/${family_upper}/
			mkdir -p $PIC32C_LIBS_PATH/include_mcc/proc/$family_upper
			find . -type d -exec mkdir -p \{\} $PIC32C_LIBS_PATH/include_mcc/proc/$family_upper/{} \;
			find . -name "*.h" -exec cp \{\} $PIC32C_LIBS_PATH/include_mcc/proc/$family_upper/{} \;
			popd  > /dev/null
		fi
	done

	find . -name "harmony" > harmony.txt
	for dir in `cat harmony.txt`; do
		pushd ${dir} > /dev/null
		cd ..
		family=$(basename `pwd`)
		family_upper=$(echo $family | awk '{print toupper($0)}')
		if [[ ${family_upper}[0] =~ ^[^0-9] ]] ; then
			cd harmony
			find . -type d -exec mkdir -p ${PIC32C_LIBS_PATH}/include_mcc/proc/${family_upper}/\{\} \;
			find . -name "*.h" -exec cp \{\} ${PIC32C_LIBS_PATH}/include_mcc/proc/${family_upper}/\{\} \;
			popd > /dev/null
		else :
			version=$family_upper
			cd ..
			family_dir=`pwd`
			cd ..
			base_dir=`pwd`
			family_upper=${family_dir#${base_dir}/}
			family_upper=${family_upper%'_DFP'}
			family_upper=${family_upper%'-DEV'}
			family_upper=${family_upper%'_DEV'}
			cd ${family_dir}/${version}/harmony
			echo Copying ${family_dir}/${version}/harmony to ${PIC32C_LIBS_PATH}/include_mcc/proc/${family_upper}/
			mkdir -p $PIC32C_LIBS_PATH/include_mcc/proc/$family_upper
			find . -type d -exec mkdir -p \{\} $PIC32C_LIBS_PATH/include_mcc/proc/$family_upper/{} \;
			find . -name "*.h" -exec cp \{\} $PIC32C_LIBS_PATH/include_mcc/proc/$family_upper/{} \;
			popd  > /dev/null
		fi
	done

        # hack here to account for newer DFPs putting the usual include_mcc content into just "include"
        if [[ -z "${skip_include}" ]]; then
  	  find . -name "include" > include.txt
  	  for dir in `cat include.txt`; do
  		pushd ${dir} > /dev/null
  		cd ..
  		family=$(basename `pwd`)
  		family_upper=$(echo $family | awk '{print toupper($0)}')
  		if [[ ${family_upper}[0] =~ ^[^0-9] ]] ; then
  			cd include
  			find . -type d -exec mkdir -p ${PIC32C_LIBS_PATH}/include/proc/${family_upper}/\{\} \;
  			find . -name "*.h" -exec cp \{\} ${PIC32C_LIBS_PATH}/include/proc/${family_upper}/\{\} \;
  			popd > /dev/null
  		else :
  			version=$family_upper
  			cd ..
  			family_dir=`pwd`
  			cd ..
  			base_dir=`pwd`
  			family_upper=${family_dir#${base_dir}/}
  			family_upper=${family_upper%'_DFP'}
  			family_upper=${family_upper%'-DEV'}
  			family_upper=${family_upper%'_DEV'}
  			cd ${family_dir}/${version}/include
  			echo Copying ${family_dir}/${version}/include to ${PIC32C_LIBS_PATH}/include/proc/${family_upper}/
  			mkdir -p $PIC32C_LIBS_PATH/include/proc/$family_upper
  			find . -type d -exec mkdir -p \{\} $PIC32C_LIBS_PATH/include/proc/$family_upper/{} \;
  			find . -name "*.h" -exec cp \{\} $PIC32C_LIBS_PATH/include/proc/$family_upper/{} \;
  			popd  > /dev/null
  		fi
  	  done
  	  cd ${PIC32C_LIBS_PATH}/include
  	  find . -name "*.h" -exec dos2unix \{\} \;
        fi # end if -z $skip_include

	cd ${PIC32C_LIBS_PATH}/include_mcc
	find . -name "*.h" -exec dos2unix \{\} \;
	popd > /dev/null
}

generateconfigdata ()
{
	pushd ./SAM-gen-temp/
	rm -rf configdata
	mkdir -p configdata
	cd configdata
	cp ${SCRIPTROOT}/py/*.xml ./
	ant setup
	cd extlibs
	unzip crownking.edc.python.jar
	cp ${SCRIPTROOT}/py/*.py ./
	N=2
	(
	for picfile in ${PIC_PATH}/*.PIC; do
		((i=i%N)); ((i++==0)) && wait
		./pic2cfg32.py --arch=c ${picfile} &
	done
	)
	wait
	cd 32Family
	find . -name "configuration.data" -exec cp \{\} ${PIC32C_LIBS_PATH}/proc/\{\} \;

	cd config_docs
	cp *.html ${PIC32M_DOCS_PATH}/config_docs/

	popd
}

commitchanges ()
{
	pushd ./SAM-gen-temp/
	
	pushd pic32c-libs
	
	pushd include_mcc
	if [ -e proc/pic32cx0025sm10128.h ] ; then
	  rm proc/pic32cx0025sm10128.h
	fi
	find . -type f -exec git add {} \;
	popd

	pushd include
	if [ -e proc/pic32cx0025sm10128.h ] ; then
	  rm proc/pic32cx0025sm10128.h
	fi
	find . -type f -exec git add {} \;
	popd

	pushd proc
	find . -type f -exec git add {} \;
	popd

	git commit -a -m "Regenerate"
	git push --set-upstream origin feature/regenerate
	popd

	pushd pic32m-docs
	git add PIC32ConfigSet.html
	pushd config_docs
	find . -type f -exec git add {} \;
	popd
	git commit -a -m "Regnerate"
	git push --set-upstream origin feature/regenerate
	popd

    pushd pic32c-headers_generator
	pushd xml2h/mchp/xc/pic32c/specs_gen/include/
	find . -name "*.def" -exec git add {} \;
	popd
	git commit -a -m "Regnerate"
	git push --set-upstream origin feature/regenerate
	popd

	popd
}

ATDF_PATH=`pwd`/SAM-gen-temp/atdf_source
PIC_PATH=`pwd`/SAM-gen-temp/pic_source
OUTPUT_PATH=`pwd`/SAM-gen-temp/output
PIC32C_LIBS_PATH=`pwd`/SAM-gen-temp/pic32c-libs
PIC32M_DOCS_PATH=`pwd`/SAM-gen-temp/pic32m-docs
PYTHONPATH=`pwd`/SAM-gen-temp/pic32c-headers_generator/xml2h

if echo "$ROOT" | grep -q "$SCRIPTROOT"; then
  echo Run this script from outside of the $SCRIPTROOT directory tree.
  exit 1
fi

maketempdir
getrepos

getdfps
copyatdffiles
copypicfiles

echo generatestartup
generatestartup
echo generatelinkerscripts
generatelinkerscripts
echo generatexc
generatexch
echo generatemcudefs
generatemcudefs
echo copyheaderfiles
copyheaderfiles

echo generateconfigdata
generateconfigdata

echo commitchanges
commitchanges

echo
echo 
echo  DONE. 
echo
echo  Remember that include/xc.h and include_mcc/xc.h is currently manually generated
echo
echo  New files have been copied to 
echo  \*  ${PIC32C_LIBS_PATH}
echo  \*  ${PIC32M_DOCS_PATH}
echo  \*  ${PIC32C_HEADERS_GENERATOR_PATH}
echo
echo
echo 

exit 0
