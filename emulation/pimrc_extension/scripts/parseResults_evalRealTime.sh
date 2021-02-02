#!/bin/bash

# this script uses ./processDataFile.pl

SNDINT=$1
MAXMESSAGES=$2

for RBS in 50; # 25 50 100;
do

    RTT_OUTPUT_FILE="rtt-sndInt=${SNDINT}ms-rbs=${RBS}.csv"
    ACKED_OUTPUT_FILE="acked-sndInt=${SNDINT}ms-rbs=${RBS}.csv"

    # --- print header --- #
    printf "Background_UEs\Numerology" > $RTT_OUTPUT_FILE
    for NUMEROLOGY in 0 1 2 3 4;
    do
        printf "\t${NUMEROLOGY}\t${NUMEROLOGY}-conf" >> $RTT_OUTPUT_FILE
    done
    printf "\n" >> $RTT_OUTPUT_FILE


    for BKUES in 0 1 2 3; # 25 50 100;
    do
        # --- print row name --- "
        printf "$BKUES\t" >> $RTT_OUTPUT_FILE
        
        for NUMEROLOGY in 0 1 2 3 4;
        do
            ORIG=`pwd`
            
            SUBDIR="u=$NUMEROLOGY-rbs=$RBS-sndInt=${SNDINT}-bkUEs=${BKUES}-#0"
            cd $SUBDIR

            ../../scripts/processDataFile.pl -n $MAXMESSAGES -r >> $ORIG/$RTT_OUTPUT_FILE
            printf "\t" >> $ORIG/$RTT_OUTPUT_FILE 
            
            #./processDataFile.pl -n $MAXMESSAGES -c >> $ACKED_OUTPUT_FILE
            #printf "\t" >> $ACKED_OUTPUT_FILE 

            cd $ORIG
        done

        printf "\n" >> $RTT_OUTPUT_FILE
    done

done