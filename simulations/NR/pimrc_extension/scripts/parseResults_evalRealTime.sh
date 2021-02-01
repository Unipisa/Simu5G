#!/bin/bash

# This script produces charts for fixed radius and varying number of users
# use ./processDataFile.pl

NUMEROLOGY=$1
MAXMESSAGES=$2

RTT_OUTPUT_FILE="numerology=$NUMEROLOGY-rtt.csv"
ACKED_OUTPUT_FILE="numerology=$NUMEROLOGY-acked.csv"

# --- print header --- #
printf "Num Users" > $RTT_OUTPUT_FILE
for RBS in 10 25 50 100;
do
    printf "\t${RBS}\t${RBS}-conf" >> $RTT_OUTPUT_FILE
done
printf "\n" >> $RTT_OUTPUT_FILE


printf "Num Users" > $ACKED_OUTPUT_FILE
for RBS in 10 25 50 100;
do
    printf "\t${RBS}" >> $ACKED_OUTPUT_FILE
done
printf "\n" >> $ACKED_OUTPUT_FILE


for USERS in $(seq 0 20);
do
    # --- print row name --- "
    printf "$USERS\t" >> $RTT_OUTPUT_FILE
    
    for RBS in 10 25 50 100;
    do
        ORIG=`pwd`
        
        SUBDIR="numerology=$NUMEROLOGY-rbs=$RBS-ues=$USERS"
        cd $SUBDIR
            
        ./processDataFile.pl -n $MAXMESSAGES -r >> $RTT_OUTPUT_FILE
        printf "\t" >> $RTT_OUTPUT_FILE 
        
        ./processDataFile.pl -n $MAXMESSAGES -c >> $ACKED_OUTPUT_FILE
        printf "\t" >> $ACKED_OUTPUT_FILE 

        cd $ORIG
    done

    printf "\n" >> $OUTPUT_FILE
done
