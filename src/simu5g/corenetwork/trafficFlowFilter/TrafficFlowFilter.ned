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
// This module maps IP 4-Tuples to Traffic Flow Template (TFT) identifiers, which are used by the ~GtpUser module
// contained in the same node. It interacts with the ~Binder to determine the destination of the packet.
// This module resides in any module that also includes a ~GtpUser module, such as ~eNodeB, ~gNodeB, ~PgwStandard,
// ~Upf, and ~MECHost modules. For instance, in a ~Upf module acting as the entry point of the 5G network, it
// identifies the destination endpoint of the GTP tunnel that the packet should traverse, which is the gNodeB
// serving the destination User Equipment (UE).
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
