#!/bin/bash
# make sure you run '. setenv' in the Simu5G root directory before running this script

TIMECMD="/usr/bin/time -f %e"

TIMEDIR=timeResults
mkdir $TIMEDIR

mkdir $TIMEDIR/bgMulticellValidation

FILENAME="u=0-ues=50-dist=50m"

for RUN in `seq 0 9`; do

    # run simulation with foreground cells
    $TIMECMD simu5g -u Cmdenv -c BgMulticellValidation-simulated -r $RUN --cmdenv-redirect-output=false omnetpp.ini 2>> $TIMEDIR/bgMulticellValidation/sim-$FILENAME-$RUN >/dev/null

    # run the same simulation with background cells
    # the rtxRate is taken from the results produced by the previous simulation 
    $TIMECMD simu5g -u Cmdenv -c BgMulticellValidation-generated -r $RUN --cmdenv-redirect-output=false omnetpp.ini 2>> $TIMEDIR/bgMulticellValidation/bg-$FILENAME-$RUN >/dev/null

done
