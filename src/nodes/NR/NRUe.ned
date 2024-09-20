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

import simu5g.corenetwork.statsCollector.NRueStatsCollector;
import simu5g.nodes.Ue;

//
// This module implements a User Equipment (UE) for a 5G LTE network. It extends the ~Ue module
// by implementing a ~NRNicUe as a Network Interface Card (NIC) module. This module can be
// used to model a device that can connect to either a 4G eNodeB or a 5G gNodeB, or both.
//
module NRUe extends Ue
{
    parameters:
        cellularNic.typename = default("NRNicUe");

        int nrMasterId @mutable = default(0);
        int nrMacNodeId @mutable = default(0); // TODO: this is not a real parameter
        int nrMacCellId @mutable = default(0); // TODO: this is not a real parameter
        cellularNic.nrMac.collectorModule = hasRNISupport ? "^.^.NRueCollector" : "";
        cellularNic.nrPhy.hasCollector = hasRNISupport;

    gates:
        input nrRadioIn @directIn;     // connection to master

    submodules:
        //# UeStatsCollector - for MEC
        NRueCollector: NRueStatsCollector if hasRNISupport {
            @display("p=127,506;is=s");
        }

    connections allowunconnected:

        cellularNic.nrRadioIn <-- nrRadioIn;
}
