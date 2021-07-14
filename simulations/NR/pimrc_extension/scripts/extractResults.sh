#!/bin/bash


CONFIG=$1
MAXRUN=9

INPUTDIR=results/$CONFIG

OUTPUTDIR=stats/$CONFIG
mkdir $OUTPUTDIR

# ===== EXPORT METRICS ===== #
echo "--> Exporting metrics..."
for U in 0; do
    for UES in 75; do
	for RANDCQI in true; do
            INPUTFILE=$INPUTDIR/u=${U}*UEs=${UES}-*randomCqi=${RANDCQI}*.sca
            
            OUTPUTFILE=$OUTPUTDIR/sca_rcvdSinrDl_u=${U}_ues=${UES}_randomCqi=${RANDCQI}.csv
            opp_scavetool export -f 'name=~rcvdSinrDl:mean AND module =~ "*.ue.cellularNic.nrChannelModel[0]"' -F 'CSV-S' -T s -o $OUTPUTFILE $INPUTFILE
            
            OUTPUTFILE=$OUTPUTDIR/sca_rcvdSinrUl_u=${U}_ues=${UES}_randomCqi=${RANDCQI}.csv
            opp_scavetool export -f 'name=~rcvdSinrUl:mean AND module =~ "*.ue.cellularNic.nrChannelModel[0]"' -F 'CSV-S' -T s -o $OUTPUTFILE $INPUTFILE

            OUTPUTFILE=$OUTPUTDIR/sca_measuredSinrDl_u=${U}_ues=${UES}_randomCqi=${RANDCQI}.csv
            opp_scavetool export -f 'name=~measuredSinrDl:mean AND module =~ "*.ue.cellularNic.nrChannelModel[0]"' -F 'CSV-S' -T s -o $OUTPUTFILE $INPUTFILE
            
            OUTPUTFILE=$OUTPUTDIR/sca_measuredSinrUl_u=${U}_ues=${UES}_randomCqi=${RANDCQI}.csv
            opp_scavetool export -f 'name=~measuredSinrUl:mean AND module =~ "*.ue.cellularNic.nrChannelModel[0]"' -F 'CSV-S' -T s -o $OUTPUTFILE $INPUTFILE
            
            OUTPUTFILE=$OUTPUTDIR/sca_averageCqiDl_u=${U}_ues=${UES}_randomCqi=${RANDCQI}.csv
            opp_scavetool export -f 'name=~averageCqiDl:mean AND module =~ "*.ue.cellularNic.nrPhy"' -F 'CSV-S' -T s -o $OUTPUTFILE $INPUTFILE
            
            OUTPUTFILE=$OUTPUTDIR/sca_averageCqiUl_u=${U}_ues=${UES}_randomCqi=${RANDCQI}.csv
            opp_scavetool export -f 'name=~averageCqiUl:mean AND module =~ "*.ue.cellularNic.nrPhy"' -F 'CSV-S' -T s -o $OUTPUTFILE $INPUTFILE
            
            OUTPUTFILE=$OUTPUTDIR/sca_rtt_u=${U}_ues=${UES}_randomCqi=${RANDCQI}.csv
            opp_scavetool export -f 'type=~statistics AND name=~rcvdPkLifetime:stats' -F 'CSV-S' -o $OUTPUTFILE $INPUTFILE

            OUTPUTFILE=$OUTPUTDIR/sca_servedBlocksDl_u=${U}_ues=${UES}_randomCqi=${RANDCQI}.csv
            opp_scavetool export -f 'name=~avgServedBlocksDl:mean AND module=~"*.gnb.*"' -F 'CSV-S' -o $OUTPUTFILE $INPUTFILE

            OUTPUTFILE=$OUTPUTDIR/sca_servedBlocksUl_u=${U}_ues=${UES}_randomCqi=${RANDCQI}.csv
            opp_scavetool export -f 'name=~avgServedBlocksUl:mean AND module=~"*.gnb.*"' -F 'CSV-S' -o $OUTPUTFILE $INPUTFILE
            
	done
    done
done                 




echo "--> End"




# vectors
# opp_scavetool export -f 'name=~smiRtt:vector' -F 'CSV-S' -T v -o stats/${CONFIG}/sca_smiRtt_ues=${UES}_u=${U}_run=${RUN}.csv results/${CONFIG}/*ues=${UES},*u=${U}-${RUN}.vec
