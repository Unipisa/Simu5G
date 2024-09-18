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
// This module maps IP 4-Tuples to TFT identifiers, which are used by the ~GtpUser module
// contained in the same node. It interacts with the ~Binder to determine the packet's
// destination. This module resides in any module that includes a ~GtpUser module too, such
// as ~eNodeB, ~gNodeB, ~PgwStandard, ~Upf, and ~MECHost modules. For example, at a ~Upf
// module acting as entry point of the 5G network, it finds the destination endpoint of the 
// GTP tunnel the packet should traverse, i.e., the gNodeB serving the destination UE.
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
