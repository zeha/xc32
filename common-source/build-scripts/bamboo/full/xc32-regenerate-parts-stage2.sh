#!/bin/bash

set -e 

SCRIPTROOT=`dirname $0`

source ${SCRIPTROOT}/../../pic32-parts-inc.sh 
source ${SCRIPTROOT}/xc32-full-bamboo-inc.sh 

DATE=$(date)
cd fossilopen

fossiladdcommit pic32-libs "Bamboo check-in from 'RC generate part-support' build #${bamboo_buildNumber} on $DATE"
fossiladdcommit c30_resource "Bamboo check-in from 'RC generate part-support' build #${bamboo_buildNumber} on $DATE"
fossiladdcommit XC32-docs "Bamboo check-in from 'RC generate part-support' build #${bamboo_buildNumber} on $DATE"
