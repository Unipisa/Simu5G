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

import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import simu5g.stack.NtnFeederLinkNic;
import simu5g.stack.NtnServiceLinkNic;

//
// Transparent NTN satellite relay with one NIC facing the service link and one
// facing the feeder link. The satellite does not terminate UE procedures; it
// only relays traffic between its two sides and registers itself in the Binder.
//
module Satellite
{
    parameters:
        @networkNode();
        @labels(node,ethernet-node,wireless-node);
        @display("i=device/satellite;bgb=469,282");

        string nodeType = default("SATELLITE"); // DO NOT CHANGE
        int macNodeId = default(1280 + simu5g_seq("Satellite_macNodeId"));
        *.interfaceTableModule = default(absPath(".interfaceTable"));

    gates:
        input serviceLinkRadioIn @loose @directIn;
        input feederLinkRadioIn @loose @directIn;

    submodules:
        interfaceTable: InterfaceTable {
            @display("p=70,120;is=s");
        }
        mobility: <default("StationaryMobility")> like IMobility {
            @display("p=70,55;is=s");
        }
        serviceNic: NtnServiceLinkNic {
            parameters:
                @display("p=181,217");
        }
        feederNic: NtnFeederLinkNic {
            parameters:
                @display("p=384,217");
        }
        relay: NtnRelay {
            parameters:
                @display("p=282,120");
        }

    connections:
        serviceNic.upperLayerOut --> relay.leftNicIn;
        serviceNic.upperLayerIn <-- relay.leftNicOut;
        feederNic.upperLayerOut --> relay.rightNicIn;
        feederNic.upperLayerIn <-- relay.rightNicOut;

        serviceNic.radioIn <-- serviceLinkRadioIn;
        feederNic.radioIn <-- feederLinkRadioIn;
}
