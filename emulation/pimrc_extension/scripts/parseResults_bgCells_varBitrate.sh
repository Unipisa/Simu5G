#!/bin/bash

# this script uses ./processDataFile.pl

NUM_BG_CELLS=$1
MAXMESSAGES=$2
RBS=100
MSGLEN=1000

for BGUES in 100;
do

    RTT_OUTPUT_FILE="rtt-multicell-varFgBitrate-gnbs=${NUM_BG_CELLS}-ues=${BGUES}-rbs=${RBS}.csv"
    
    # --- print header --- #
    printf "SndInt\Numerology" > $RTT_OUTPUT_FILE
    for U in 0 2 4;
    do
        printf "\t${U}\t${U}-conf" >> $RTT_OUTPUT_FILE
    done
    printf "\n" >> $RTT_OUTPUT_FILE


    for SNDINT in 0.04 0.02 0.01 0.0067 0.005 0.004 0.0034;
    do
        # --- print row name --- "
        printf "$SNDINT\t" >> $RTT_OUTPUT_FILE
        
        for U in 0 2 4;
        do
            ORIG=`pwd`
            
            SUBDIR="varFgBitrate-gnbs=${NUM_BG_CELLS}-u=${U}-rbs=${RBS}-msgLen=${MSGLEN}-sndInt=${SNDINT}-bkUEs=${BGUES}-#0"
            cd $SUBDIR

            ../../scripts/processDataFile.pl -n $MAXMESSAGES -r >> $ORIG/$RTT_OUTPUT_FILE
            printf "\t" >> $ORIG/$RTT_OUTPUT_FILE 
            cd $ORIG
            
        done

        printf "\n" >> $RTT_OUTPUT_FILE
    done

done
  