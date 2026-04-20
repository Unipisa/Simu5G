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

import inet.linklayer.contract.IEthernetInterface;
import inet.linklayer.ppp.PppInterface;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;

//
// Ground NTN gateway relay with one NIC facing the terrestrial gNB side and one
// facing the feeder link toward the satellite. The module relays traffic between
// the two sides and registers itself in the Binder as an NTN-specific node.
//
module NtnGateway
{
    parameters:
        @networkNode();
        @labels(node,ethernet-node);
        @display("i=abstract/server;bgb=420,220");

        string binderModule = default("binder");
        string nodeType = default("NTNGATEWAY"); // DO NOT CHANGE
        int macNodeId = default(NTN_GW_MIN_ID + simu5g_seq("NtnGateway_macNodeId"));
        double relayDelay @unit(s) = default(0s);
        *.interfaceTableModule = default(absPath(".interfaceTable"));

    gates:
        inout gnbLink @loose @labels(EtherFrame-conn);
        inout feederLink @loose @labels(PppFrame-conn);

    submodules:
        interfaceTable: InterfaceTable {
            @display("p=70,120;is=s");
        }
        mobility: <default("StationaryMobility")> like IMobility {
            @display("p=70,55;is=s");
        }
        gnbNic: <default("EthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=110,168");
        }
        feederNic: PppInterface {
            parameters:
                @display("p=310,168");
        }
        relay: NtnRelay {
            parameters:
                @display("p=210,88");
                binderModule = parent.binderModule;
                nodeType = parent.nodeType;
                relayDelay = parent.relayDelay;
        }

    connections:
        gnbNic.upperLayerOut --> relay.leftNicIn;
        gnbNic.upperLayerIn <-- relay.leftNicOut;
        feederNic.upperLayerOut --> relay.rightNicIn;
        feederNic.upperLayerIn <-- relay.rightNicOut;

        gnbNic.phys <--> gnbLink;
        feederNic.phys <--> feederLink;
}
