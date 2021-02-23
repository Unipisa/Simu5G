#!/bin/bash
# make sure you run '. setenv' in the Simu5G root directory before running this script

INET_VERSION=4.3

SIMU5G_SRC=$SIMU5G_ROOT/src
INET_SRC=`(cd $SIMU5G_ROOT/../inet${INET_VERSION}/src ; pwd)`
COMMAND_LINE_OPTIONS="-n $SIMU5G_ROOT/simulations:$SIMU5G_ROOT/emulation:$SIMU5G_SRC:$INET_SRC"

valgrind --tool=callgrind opp_run -l $SIMU5G_SRC/lte $COMMAND_LINE_OPTIONS -u Cmdenv -c BgTrafficScalabilityEval-simulated $*

valgrind --tool=callgrind opp_run -l $SIMU5G_SRC/lte $COMMAND_LINE_OPTIONS -u Cmdenv -c BgTrafficScalabilityEval-generated $*

