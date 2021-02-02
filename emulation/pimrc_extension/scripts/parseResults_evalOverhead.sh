#!/bin/bash

# use ./processDataFile.pl

NUMEROLOGY=$1
RBS=$2

ORIG=`pwd`

OUTPUT_FILE=${ORIG}/avgRtt.csv

# --- print header --- #
printf "X-Axis\tX-tics\tRTT\tRTT_conf\n" > ${OUTPUT_FILE}
        

cd direct

printf "0\tDirect\t" >> ${OUTPUT_FILE}
$ORIG/../../scripts/processDataFile.pl -n 300 -r >> ${OUTPUT_FILE}
printf "\n" >> ${OUTPUT_FILE}

cd $ORIG

cd baseline-emulation

printf "1\tEmulationBaseline\t" >> ${OUTPUT_FILE}
$ORIG/../../scripts/processDataFile.pl -n 300 -r >> ${OUTPUT_FILE}
printf "\n" >> ${OUTPUT_FILE}

cd $ORIG


cd baseline-emulation-5G-u_$NUMEROLOGY-rbs_$RBS
            
printf "2\tEmulation5G\t" >> ${OUTPUT_FILE}
$ORIG/../../scripts/processDataFile.pl -n 300 -r >> ${OUTPUT_FILE}
printf "\n" >> ${OUTPUT_FILE} 
        
cd $ORIG