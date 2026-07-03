This example compares the available PHY error models on the same NR downlink
scenario.

Run one model:

    simu5g -u Cmdenv -f omnetpp.ini -c Eesm -r 0

Run all model/distance combinations:

    simu5g -u Cmdenv -f omnetpp.ini -c IndependentResourceBlock
    simu5g -u Cmdenv -f omnetpp.ini -c Eesm
    simu5g -u Cmdenv -f omnetpp.ini -c Miesm

Each configuration sweeps the UE x coordinate. The gNB is fixed, so the sweep
changes the received SINR. The error-model logs show:

- per-resource-block SINRs
- the selected error model
- the resulting packet error probability

IndependentResourceBlockErrorModel combines per-RB BLER values. EesmErrorModel
and MiesmErrorModel compute an effective SINR before querying the BLER curve.

To assess the differences, see for instance 'rcvdSinrDl', 'harqErrorRate' and 
'cbrReceivedThroughput' metrics.
