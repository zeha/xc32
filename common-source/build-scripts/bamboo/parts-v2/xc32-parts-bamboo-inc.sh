#Script to set FOSSIL env variables called by xc32-parts-bamboo-stage1.sh and
#xc32-parts-bamboo-stage2.sh

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

#ERRHTMLREDIRECT is used by xc32-parts-bamboo-stage2.sh to output errors.html

read -d '' ERRHTMLREDIRECT <<EOF
<!DOCTYPE HTML>
<html lang="en-US">
    <head>
        <meta charset="UTF-8">
        <meta http-equiv="refresh" content="1;url=http://chn-sv-cyclone.microchip.com:8085/browse/XC32-PSN-PJU-${bamboo_BUILD_NUMBER}/test">
        <script type="text/javascript">
            window.location.href = "http://chn-sv-cyclone.microchip.com:8085/browse/XC32-PSN-PJU-${bamboo_BUILD_NUMBER}/test"
        </script>
        <title>Page Redirection</title>
    </head>
    <body>
        If you are not redirected automatically, follow the <a href='http://chn-sv-cyclone.microchip.com:8085/browse/XC32-PSN-PJU-${bamboo_BILD_NUMBER}/test'>FIXME: link to test result page</a>
    </body>
</html>
EOF

