
//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.corenetwork.trafficFlowFilter;

//
// The TrafficFlowFilter module maps IP 4-Tuples to TFT identifiers, identifying a bearer and associating it 
// with an ID recognizable by the first GTP-U entity. It interacts with the LTE Binder to determine the packet's
// destination. This module can reside in both eNodeBs and PGWs. At the PGW, it finds the destination endpoint 
// (eNodeB serving the destination UE) of the GTP tunnel to be established. At the eNodeB, the destination endpoint 
// is typically the PGW, but if the fastForwarding flag is enabled and the destination is within the same cell, 
// the packet is relayed to the Radio interface directly.
//
simple TrafficFlowFilter
{
    parameters:
        @display("i=block/filter");

        string binderModule = default("binder");
        string ownerType @enum(ENODEB,GNODEB,PGW,UPF,UPF_MEC); // must be one between ENODEB,GNODEB,PGW,UPF,UPF_MEC
        bool fastForwarding = default(true);
    gates:
        input internetFilterGateIn;
        output gtpUserGateOut;
}
