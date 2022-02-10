#!/bin/bash


CONFIG=$1 
METRIC=$2

ORIGDIR=`pwd`
cd ..
mkdir stats/$CONFIG 2>/dev/null

# MAC CELL TPUT DL
case $METRIC in

  macCellThroughputDl)
    #opp_scavetool export -f 'name=~macCellThroughputDl:mean AND module=~*masterEnb*' -F 'CSV-S' -T s -o stats/${CONFIG}/sca_${METRIC}_enb.csv results/${CONFIG}/*.sca >/dev/null
    #opp_scavetool export -f 'name=~macCellThroughputDl:mean AND module=~*secondaryGnb*' -F 'CSV-S' -T s -o stats/${CONFIG}/sca_${METRIC}_gnb.csv results/${CONFIG}/*.sca >/dev/null
    opp_scavetool export -f 'name=~macCellThroughputDl:mean AND module=~*gnb*' -F 'CSV-S' -T s -o stats/${CONFIG}/sca_${METRIC}.csv results/${CONFIG}/*.sca >/dev/null
    ;;

  avgServedBlocksDl)
    opp_scavetool export -f 'name=~avgServedBlocksDl:mean AND module=~*gnb*' -F 'CSV-S' -T s -o stats/${CONFIG}/sca_${METRIC}.csv results/${CONFIG}/*.sca >/dev/null
    ;;

  *)
    echo "Unknown metric\n"

esac

#$ORIGDIR/scripts/getMean.pl $CONFIG ${METRIC}_enb
#$ORIGDIR/scripts/getMean.pl $CONFIG ${METRIC}_gnb
$ORIGDIR/scripts/getMean.pl $CONFIG ${METRIC}


cd $ORIGDIR


