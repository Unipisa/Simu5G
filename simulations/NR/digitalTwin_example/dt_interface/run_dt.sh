#!/bin/bash

# this file is generated automatically - DO NOT EDIT

cd ..
SIMU5G_SRC=$SIMU5G_ROOT/src
INET_SRC=`(cd $SIMU5G_ROOT/../inet4.3/src ; pwd)`
COMMAND_LINE_OPTIONS="$SIMU5G_ROOT/simulations:$SIMU5G_ROOT/emulation:$SIMU5G_SRC:$INET_SRC"

opp_runall -j10  opp_run -l $SIMU5G_SRC/simu5g -n $COMMAND_LINE_OPTIONS -c CA-FirstOnly -r 0,1,2,3,4,5,6,7,8,9 > sim_log
opp_runall -j10  opp_run -l $SIMU5G_SRC/simu5g -n $COMMAND_LINE_OPTIONS -c CA-BestChannel -r 0,1,2,3,4,5,6,7,8,9 >> sim_log
