#!/bin/bash

# use ./processDataFile.pl

ORIG=`pwd`

OUTPUT_FILE=${ORIG}/rtt-baseline.csv

# --- print header --- #
printf "X-Axis\tX-tics\tRTT\tRTT_conf\n" > ${OUTPUT_FILE}
        

cd baseline-direct

printf "0\tDirect\t" >> ${OUTPUT_FILE}
$ORIG/../scripts/processDataFile.pl -n 3000 -r >> ${OUTPUT_FILE}
printf "\n" >> ${OUTPUT_FILE}

cd $ORIG

cd baseline-simple

printf "1\tEmulationBaseline\t" >> ${OUTPUT_FILE}
$ORIG/../scripts/processDataFile.pl -n 3000 -r >> ${OUTPUT_FILE}
printf "\n" >> ${OUTPUT_FILE}

cd $ORIG


# cd baseline-emulation-5G-u_$NUMEROLOGY-rbs_$RBS
#             
# printf "2\tEmulation5G\t" >> ${OUTPUT_FILE}
# $ORIG/../../scripts/processDataFile.pl -n 300 -r >> ${OUTPUT_FILE}
# printf "\n" >> ${OUTPUT_FILE} 
#         
# cd $ORIG