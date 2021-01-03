
set -e 

if [ -z $bamboo_XC32_SOURCE_BRATAG ]; then
  bamboo_XC32_SOURCE_BRATAG=trunk
fi

rm xc32-${bamboo_XC32_SOURCE_BRATAG}/win32-image.tar.gz
tar -xzf xc32-${bamboo_XC32_SOURCE_BRATAG}/Linux32-image.tar.gz
rm xc32-${bamboo_XC32_SOURCE_BRATAG}/Linux32-image.tar.gz

cat <<EOT > ./Linux32-image/etc/xclm.conf
<xclm>
        <xclm:LicenseDirectory xclm:path="/home/bamboo/xc32/licenses" />
        <xclm:Activation xclm:url="http://keygen.microchip.com" />
        <xclm:Demo xclm:url="http://www.microchip.com/xcdemo/handler.aspx" /> 
</xclm>
EOT

export PATH=$(pwd)/Linux32-image/bin:$PATH

source ~/xc32/bashrc-regression

if [ -e gcc_tests ]; then 
  rm -rf gcc_tests
fi

tar -xzf gcc_tests.tar.gz

sed -i "1iset COMPILER_ROOT $(pwd)/Linux32-image" gcc_tests/dejagnurc.bamboo
cp gcc_tests/dejagnurc.bamboo ~/.dejagnurc

cd gcc_tests

if [ -e log ]; then
  rm -rf log
fi

mkdir log

chmod +x ./lib/run
chmod +x *.sh

set +e
runtest --tool=gcc --target=pic32-*-* --target_board=pic32-sim-unix --verbose $1 SIM=./debug_m14ke.sh SDEGDBCMD=gdbmz.cmd
set -e

mv gcc.log log/
mv gcc.sum log/

cd log

../sum2junit.sh gcc.sum


