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

package simu5g.nodes.NR;

import simu5g.nodes.eNodeB;

//
// This module implements a gNodeB for a 5G LTE network. It extends the ~eNodeB module
// by providing 5G connectivity to ~NRUe modules. It can be used in either StandAlone (SA) or
// Non-StandAlone (NSA) 5G deployments. In the former case, it is directly connected to a ~Upf
// module. In the latter case, this module can be configured as a secondary node of
// a ~eNodeB module, thus enabling E-UTRA Dual Connectivity (EN-DC).
//
module gNodeB extends eNodeB
{
    parameters:
        @networkNode();
        @display("i=device/antennatower;bgb=1260,600");
        collector.typename = default("GNodeBStatsCollector");
        gateway = default("upf"); // change this path if the gateway node is inside a compound module
        nodeType = "GNODEB";        // DO NOT CHANGE
        cellularNic.typename = default("NRNicEnb");
}
