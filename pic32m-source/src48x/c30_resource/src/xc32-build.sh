#!/bin/bash

echo "Building generator tools"

CMD_VERSION=${1:-"0_00"};
CMD_RVERSION=${2:-0x7a};

(cd generator; rm -f lex.yy.o grammar.tab.o main.o resource.o reader.o	
 make PIC32=-DPIC32)


VERSION_ID=`echo ${MCHP_VERSION:-$CMD_VERSION} | perl -n -e 'if (/.*(\d+)[\._](\d\d?).*/) { print "$1.$2" }'`

echo "Building xc32 resource data files for version [$VERSION_ID]"


echo "(cd xc32; ../generator/rg +v $VERSION_ID (`echo $MCHP_RESOURCE | egrep -o \"[[:alnum:]]\"`) -DMCHP_VERSION=${VERSION_ID} -DMCHP_RESOURCE=${MCHP_RESOURCE:-$CMD_RVERSION} xc32_devices.res xc32_device.info .LanguageToolSuite.base deviceSupport.xml)"

(cd xc32; ../generator/rg +v "$VERSION_ID (`echo $MCHP_RESOURCE | egrep -o \"[[:alnum:]]\"`)" -DMCHP_VERSION=${VERSION_ID} -DMCHP_RESOURCE=${MCHP_RESOURCE:-$CMD_RVERSION} xc32_devices.res xc32_device.info .LanguageToolSuite.base deviceSupport.xml)

echo "(cd xc32; ../generator/rg +s -DMCHP_VERSION=${VERSION_ID} -DMCHP_RESOURCE=${MCHP_RESOURCE:-$CMD_RVERSION} xc32_devices.res xc32_device.info)"
(cd xc32; ../generator/rg +s -DMCHP_VERSION=${VERSION_ID} -DMCHP_RESOURCE=${MCHP_RESOURCE:-$CMD_RVERSION} xc32_devices.res xc32_device.info)

remove_duplicate_device_lines() {
    local seen=":"
    local device
    local line

    while IFS='' read -d $'\n' line; do
        if [[ "${line}" == *'<lti:Device lti:device='* ]]; then
            device="${line##*lti:device=\"}"
            device="${device%%\"*}"

            if [[ "${seen}" == *:${device}:* ]]; then
                continue
            fi

            seen+="${device}:"
        fi

        printf '%s\n' "${line}"
    done
}

echo "Removing duplicate device lines"
remove_duplicate_device_lines < xc32/.LanguageToolSuite.base > xc32/.LanguageToolSuite
