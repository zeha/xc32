#!/bin/bash
# this script produces on stdout an appropriate xc.h file for a given include family of PIC32C header files. 
# The path to the device header files should be passed as the first argument to the script.
#   Example: > generate-pic32c-xc.h.sh /Applications/microchip/xc32/v2.30-PS/pic32c/include
# Normally this script would be run from another part-support generation script.

procdir=$1

cat <<'EOF' 
 /**
 * \file
 *
 * \brief MPLAB XC32 header file for SAM/PIC32C devices
 *
EOF

echo " * Copyright (c) $(date '+%Y'), Microchip Technology Inc. and its subsidiaries (\"Microchip\")"

cat <<'EOF'
 *
 * \license_start
 *
 * \page License
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \license_stop
 *
EOF

echo " * File generated on $(date '+%Y-%m-%d')"

cat <<'EOF' 
 *
 */

#pragma once

#if defined __DUMMY_LIST_STARTER
EOF

pushd $procdir > /dev/null

if [ -d proc ] ; then
    directory="./proc"
else
    directory="."
fi

includes=()
while IFS=  read -r -d $'\0'; do
    includes+=("$REPLY")
done < <(find $directory -name "p*.h" -print0)
while IFS=  read -r -d $'\0'; do
    includes+=("$REPLY")
done < <(find $directory -name "sam*.h" -print0)

for include in "${includes[@]}"
do
    if [[ $include != *"/pio/"* ]] &&
       [[ $include != *"/component/"* ]] &&
       [[ $include != *"/pic32c.h" ]] &&
       [[ $include != *"/sam.h" ]] &&
       [[ $include != *"/instance/"* ]]; then
        include=${include#"./"}
        defstring="$(tr [a-z] [A-Z] <<< "${include%\.h}")"
        defstring=`echo ${defstring} | sed "s/^.*\///g"`
        if [[ $defstring == PIC* ]]; then
        echo "#elif defined(__"${defstring}"__) || defined(__"${defstring#PIC}"__)"
        elif [[ $defstring == SAM* ]]; then
        echo "#elif defined(__"${defstring}"__) || defined(__AT"${defstring}"__)"
        else
        echo "#elif defined(__"${defstring}"__)"
        fi
            echo "#  include \"${include}\""
    fi
done

popd > /dev/null

cat <<'EOF' 
#else
#  error Device-specific header file not found for selected device. Check for a new Device Family Pack (DFP).
#endif

#if defined (__LANGUAGE_C__) || defined (__LANGUAGE_C_PLUS_PLUS__)

#ifndef _nop
#define _nop() __builtin_nop()
#endif /* _nop */

#ifndef Nop
/* Consider using _nop rather than Nop */
#define Nop()  _nop()
#endif /* Nop */

#endif /* defined (__LANGUAGE_C__) || defined (__LANGUAGE_C_PLUS_PLUS__) */

EOF
