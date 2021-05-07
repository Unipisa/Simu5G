#!/bin/bash
# make sure you run '. setenv' in the Simu5G root directory before running this script


for RUN in `seq 0 29`; do

    # run simulation with foreground cells
    simu5g -u Cmdenv -c BgMulticellValidation-simulated -r $RUN --cmdenv-redirect-output=false omnetpp.ini

    # run the same simulation with background cells
    # the rtxRate is taken from the results produced by the previous simulation 
    simu5g -u Cmdenv -c BgMulticellValidation-generated -r $RUN --cmdenv-redirect-output=false omnetpp.ini

done
