
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
// NRUe Module
// This module represents a 5G NR (New Radio) User Equipment (UE) in the Simu5G network.
//
// The NRUe module extends the basic Ue module and includes additional parameters 
// and submodules specific to NR functionalities.
//
// Parameters:
// - nicType: Defines the type of NIC used by the UE, defaulting to "NRNicUe".
// - nrMasterId: A mutable parameter representing the master ID for NR.
// - nrMacNodeId: A mutable parameter for the NR MAC Node ID.
// - nrMacCellId: A mutable parameter for the NR MAC Cell ID.
//
// Gates:
// - nrRadioIn: A direct input gate for connections to the master node.
//
// Submodules:
// - NRueCollector: An optional submodule of type NRueStatsCollector that collects 
//   statistics for the NR UE, utilized if RNISupport is present.
//
// Connections:
// - Allows unconnected connections by default.
// - Connects the NR radio input directly to the cellular NIC's NR radio input.
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
