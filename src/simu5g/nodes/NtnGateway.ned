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
import simu5g.stack.NtnFronthaulNic;
import simu5g.stack.NtnFeederLinkNic;

//
// Ground NTN gateway relay with one NIC facing the terrestrial gNB side and one
// facing the feeder link toward the satellite. The module relays traffic between
// the two sides and registers itself in the Binder as an NTN-specific node.
//
module NtnGateway
{
    parameters:
        @networkNode();
        @labels(node);
        @display("i=device/receiverdish;bgb=476,275");

        string nodeType = default("NTNGATEWAY"); // DO NOT CHANGE
        int macNodeId = default(16384 + simu5g_seq("NtnGateway_macNodeId"));
        *.interfaceTableModule = default(absPath(".interfaceTable"));

    gates:
        inout gnbLink @loose;
        input feederLinkRadioIn @loose @directIn;

    submodules:
        interfaceTable: InterfaceTable {
            @display("p=70,120;is=s");
        }
        mobility: <default("StationaryMobility")> like IMobility {
            @display("p=70,55;is=s");
        }
        gnbNic: NtnFronthaulNic {
            parameters:
                @display("p=188,207");
        }
        feederNic: NtnFeederLinkNic {
            parameters:
                @display("p=370,207");
        }
        relay: NtnRelay {
            parameters:
                @display("p=288,120");
        }

    connections:
        gnbNic.upperLayerOut --> relay.leftNicIn;
        gnbNic.upperLayerIn <-- relay.leftNicOut;
        gnbNic.phys <--> gnbLink;

        feederNic.upperLayerOut --> relay.rightNicIn;
        feederNic.upperLayerIn <-- relay.rightNicOut;
        feederNic.radioIn <-- feederLinkRadioIn;

}
