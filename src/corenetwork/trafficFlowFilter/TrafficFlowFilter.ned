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

simple TrafficFlowFilter
{
    parameters:
        @display("i=block/filter");

        string ownerType @enum(ENODEB,GNODEB,PGW,UPF,UPF_MEC); // must be one between ENODEB,GNODEB,PGW,UPF,UPF_MEC
        bool fastForwarding = default(true);
    gates:
        input internetFilterGateIn;
        output gtpUserGateOut;
}
