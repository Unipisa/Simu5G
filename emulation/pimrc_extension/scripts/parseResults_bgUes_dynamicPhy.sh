#!/bin/bash

# this script uses ./processDataFile.pl

SNDINT=$1
MAXMESSAGES=$2
CA=1

for CA in 1;
do
    for RBS in 25 50 100;
    do

        RTT_OUTPUT_FILE="rtt-backgroundUes-ca=${CA}-rbs=${RBS}.csv"

        # --- print header --- #
        printf "Background_UEs\Numerology" > $RTT_OUTPUT_FILE
        for NUMEROLOGY in 0 1 2 3 4;
        do
            printf "\t${NUMEROLOGY}\t${NUMEROLOGY}-conf" >> $RTT_OUTPUT_FILE
        done
        printf "\n" >> $RTT_OUTPUT_FILE


        for BKUES in `seq 0 40 400` ;
        do
            # --- print row name --- "
            printf "$BKUES\t" >> $RTT_OUTPUT_FILE
            
            for NUMEROLOGY in 0 1 2 3 4;
            do
                ORIG=`pwd`
                SUBDIR="backgroundUes-ca=${CA}-u=$NUMEROLOGY-rbs=$RBS-sndInt=${SNDINT}-bkUEs=${BKUES}-#0"
                cd $SUBDIR

                ../../scripts/processDataFile.pl -n $MAXMESSAGES -r >> $ORIG/$RTT_OUTPUT_FILE
                printf "\t" >> $ORIG/$RTT_OUTPUT_FILE 
                cd $ORIG
                
            done

            printf "\n" >> $RTT_OUTPUT_FILE
        done

    done
done