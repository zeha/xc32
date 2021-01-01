#Script to set FOSSIL env variables called by xc32-regenerate-parts-legacy.sh 

export PATH=$HOME/xc32/fossil/script:$PATH

if [ -z $FOSSIL_CLONES ]; then
  export FOSSIL_CLONES=$HOME/xc32/fossil/clones
  export FOSSIL_LOGIN=bamboo:bamboo
fi

echo -n "PATH is "
echo "$PATH"

echo -n "PWD is "
pwd

HOST=`uname -s`

#Function used to check exit code and abort on error
# $1 - exit code  $2 - error message  $3 - line no.
checkexitcode () 
{
  if [[ $1 != 0 ]]; then  
    echo "Error $0:$3 : $2"
    exit 1
  fi
}

