fossilopenup compilerpage_automate

set -e

NOTIFY_TEST=""
SET_LATEST=""

if [[ "x${bamboo_XC32_NOTIFY_TEST}" == "xtrue" || "x${bamboo_XC32_NOTIFY_TEST}" == "xTRUE" ]]; then
  NOTIFY_TEST="-t"
fi

if [[ "x${bamboo_XC32_SET_LATEST}" == "xtrue" || "x${bamboo_XC32_SET_LATEST}" == "xTRUE" ]]; then
  SET_LATEST="-l"
fi

VERSION=v${bamboo_XC32_FULL_VERSION}-${bamboo_XC32_FULL_BUILD}

python ./compilerpage_automate/compauto.py $NOTIFY_TEST $SET_LATEST xc32 internal ${VERSION} xc32-v* README.html xc32*.zip

#Run LTAutoLights

if [ -f ~/.cvspass ]; then 
  echo "~/.cvspass exists, will not login again"    
  if [ -z $CVSROOT ]; then
    echo "Setting CVSROOT based on first .cvspass entry"
    OS=$(uname -s)
    if [ "$OS" = "Darwin" ]; then
      export CVSROOT=$(head -1 ~/.cvspass | cut -d " " -f 2 | sed  -E 's/\:[0-9]+//')
    else 
      export CVSROOT=$(head -1 ~/.cvspass | cut -d " " -f 2 | sed  -r 's/\:[0-9]+//')
    fi 	
    echo "CVSROOT : $CVSROOT"
  else
    echo "Using system CVSROOT : $CVSROOT"
  fi
else 
  echo "~/.cvspass does not exist"
  if [ ! -z $CVSROOT ]; then 
    echo "CVSROOT is set in enviornment to $CVSROOT"
    NFIELDS=`echo $CVSROOT  | awk -F '[:,@]' '{print NF}'`
    if [[ 5 -eq $NFIELDS ]]; then 
      export CVSROOT=` echo "$CVSROOT"  | awk -F '[:,@]' '{print $1":"$2":"$3":"$3"@"$4":"$5}'`
      echo "Updated CVSROOT to $CVSROOT"
    fi
    echo "Trying to login to CVS"
  else 
    export  CVSROOT=:pserver:bekals:bekals@10.10.70.13:/cvs 
    echo "CVSROOT not set in enviornment"
    echo "Setting CVSROOT to $CVSROOT"
    echo "Logging in to CVS"
  fi
  cvs login 
  if [ $? != 0 ]; then 
    >&2 echo "CVS login failed"
    exit 1
  else
    echo "OK"
  fi
  GENCVSPASS=TRUE
fi

if [[ "x${bamboo_RUN_LTAUTOLIGHTS}" == "xtrue" || "x${bamboo_RUN_LTAUTOLIGHTS}" == "xTRUE" ]]; then
  echo `pwd`
  mv .LanguageToolSuite ./XC32-build-scripts/autolights/xc32.LanguageToolSuite
  cd ./XC32-build-scripts/autolights
  echo "java -classpath xercesImpl.jar:LTAutoLights.jar com.microchip.ltautolights.LTAutoLights -cfg xc32.properties -cver ${bamboo_XC32_FULL_VERSION}"
  java -classpath xercesImpl.jar:LTAutoLights.jar com.microchip.ltautolights.LTAutoLights -cfg xc32.properties -cver ${bamboo_XC32_FULL_VERSION}
  cd ../../
else
  >&2 echo "LTAutoLights not executed since RUN_LTAUTOLIGHTS=FALSE"
fi

if [ "$GENCVSPASS" == "TRUE" ]; then
  echo  "Deleting generated ~/.cvspass file"
  rm ~/.cvspass
fi
