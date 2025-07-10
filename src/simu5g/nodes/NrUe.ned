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


package simu5g.nodes;

import simu5g.corenetwork.statsCollector.NrUeStatsCollector;
import simu5g.nodes.LteUe;

//
// This module implements a User Equipment (UE) for a 5G LTE network. It extends the ~LteUe module
// by implementing a ~NrNicUe as a Network Interface Card (NIC) module. This module can be
// used to model a device that can connect to either a 4G eNodeB or a 5G gNodeB, or both.
//
module NrUe extends LteUe
{
    parameters:
        cellularNic.typename = default("NrNicUe");

        int nrMasterId @mutable = default(0);
        int nrMacNodeId @mutable = default(0); // TODO: this is not a real parameter
        int nrMacCellId @mutable = default(0); // TODO: this is not a real parameter
        cellularNic.nrMac.collectorModule = hasRniSupport ? "^.^.nrUeCollector" : "";
        cellularNic.nrPhy.hasCollector = hasRniSupport;

    gates:
        input nrRadioIn @directIn;     // connection to master

    submodules:
        //# UeStatsCollector - for MEC
        nrUeCollector: NrUeStatsCollector if hasRniSupport {
            @display("p=127,506;is=s");
        }

    connections allowunconnected:

        cellularNic.nrRadioIn <-- nrRadioIn;
}
