#!/bin/bash
# make sure you run '. setenv' in the Simu5G root directory before running this script

RBS=100
UES=75
DIST=125m

CONFIGFILE=trafficGeneratorConfig.ini 

for RUN in `seq 10 19`; do

    
    # run simulation with foreground cells
    simu5g -u Cmdenv -c BgMulticellValidation-simulated -r $RUN --cmdenv-redirect-output=false omnetpp.ini

#     REPETITION=$RUN
     REPETITION=$(( $RUN - 10 ))
#     REPETITION=$(( $RUN - 20 ))
    
    INIFILE="include trafficGeneratorConfigs/config-rbs=${RBS}-numBkUEs=${UES}-dist=${DIST}-repetition=${REPETITION}.ini"
    echo $INIFILE > $CONFIGFILE
    
    
    
    # run the same simulation with background cells
    # the rtxRate is taken from the results produced by the previous simulation 
#      simu5g -u Cmdenv -c BgMulticellValidation-generated -r $RUN --cmdenv-redirect-output=false omnetpp.ini
    
    # run the same simulation with background cells and random cqi
    # the rtxRate is taken from the results produced by the previous simulation 
    NEWRUN=$(( $RUN + 30 ))
    simu5g -u Cmdenv -c BgMulticellValidation-generated -r $NEWRUN --cmdenv-redirect-output=false omnetpp.ini

done
