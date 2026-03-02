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
// Models a User Equipment (UE) for a 5G/LTE network. It extends the ~LteUe module
// by implementing a ~NrNicUe as a Network Interface Card (NIC) module. This module can be
// used to model a device that can connect to either a 4G eNodeB or a 5G gNodeB, or both.
//
module NrUe extends LteUe
{
    parameters:
        cellularNic.typename = default("NrNicUe");

        int nrServingNodeId = default(0); // the initial serving gNodeB of the UE; 0 if none
        int nrMacNodeId = default(2049+simu5g_seq("NrUe_macNodeId"));

        cellularNic.nrMac.collectorModule = hasRniSupport ? "^.^.nrUeCollector" : "";
        cellularNic.rrc.nrHandoverController.hasCollector = hasRniSupport;
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
