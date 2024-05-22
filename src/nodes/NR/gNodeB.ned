
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
// This module represents a gNodeB (Next Generation Node B) in a 5G New Radio (NR) network.
// It extends the eNodeB module, integrating NR-specific components and functionalities.
// The gNodeB acts as a base station in 5G networks, handling both user and control plane traffic.
// It supports various advanced features such as Carrier Aggregation, Dual Connectivity, and others
// which are essential for high data rates and low latency communications in 5G networks.
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
