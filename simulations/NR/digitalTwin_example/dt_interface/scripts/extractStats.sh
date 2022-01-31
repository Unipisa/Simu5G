#!/bin/bash


CONFIG=$1 
METRIC=$2

ORIGDIR=`pwd`
cd ..
mkdir stats/$CONFIG 2>/dev/null

# MAC CELL TPUT DL

opp_scavetool export -f 'name=~macCellThroughputDl:mean AND module=~*masterEnb*' -F 'CSV-S' -T s -o stats/${CONFIG}/sca_${METRIC}_enb.csv results/${CONFIG}/*.sca >/dev/null
opp_scavetool export -f 'name=~macCellThroughputDl:mean AND module=~*secondaryGnb*' -F 'CSV-S' -T s -o stats/${CONFIG}/sca_${METRIC}_gnb.csv results/${CONFIG}/*.sca >/dev/null

$ORIGDIR/scripts/getMean.pl $CONFIG ${METRIC}_enb
$ORIGDIR/scripts/getMean.pl $CONFIG ${METRIC}_gnb

cd $ORIGDIR







# ===== EXPORT METRICS ===== #
#echo "--> Exporting metrics..."
#for UES in 2 4 6 8 10 12 14 16 18 20; do
#    for U in 0 1 2; do
#		for CNDELAY in 0ms 0.2ms; do
#			for RUN in `seq 0 $MAXRUN`; do
#
#
#		        if [[ $1 != "veconly" ]] 
#		        then
#				    opp_scavetool export -f 'name=~smi*Msg:sum' -F 'CSV-S' -T s -o stats/${CONFIG}/sca_smiSentRcvdMsg_ues=${UES}_u=${U}_cnDelay=${CNDELAY}_run=${RUN}.csv results/${CONFIG}/*ues=${UES},*u=${U}*cnDelay=${CNDELAY}*-${RUN}.sca
		            #opp_scavetool export -f 'name=~avgServedBlocks*:mean' -F 'CSV-S' -T s -o stats/${CONFIG}/sca_avgServedBlocks_ues=${UES}_u=${U}_run=${RUN}.csv results/${CONFIG}/*ues=${UES}*u=${U}*-${RUN}.sca
#		        fi
#			done
#		done   
#	done                 
#done

#echo "--> End"
