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


package simu5g.stack;

import inet.networklayer.common.NetworkInterface;
import simu5g.stack.ip2nic.IHandoverPacketHolder;
import simu5g.stack.ip2nic.INetworkLayer2CellularNic;
import simu5g.stack.ip2nic.ITechnologyDecision;
import simu5g.stack.mac.ILteMac;
import simu5g.stack.packetFlowObserver.IPacketFlowObserver;
import simu5g.stack.pdcp.ILtePdcp;
import simu5g.stack.phy.ILtePhy;
import simu5g.stack.rlc.LteRlc;
import simu5g.stack.rrc.IRrc;
import simu5g.stack.phy.channelmodel.ILteChannelModel;


//
// The LteNicBase module serves as the foundational building block for LTE networking
// in Simu5G. It integrates protocols for LTE communication, including PDCP, RLC, MAC,
// and PHY layers. This base module allows higher-level modules, such as User Equipment (UE)
// and eNodeB (evolved Node B), to configure and utilize these layers according to their
// specific requirements.
//
module LteNicBase extends NetworkInterface like ICellularNic
{
    parameters:
        @class(::inet::NetworkInterface);
        @display("i=block/ifcard;bgb=704,443;bgl=3");
        bool isWireless = true;
        string interfaceTableModule;
        string routingTableModule;

        bool hasRniSupport = default(false);

        string nodeType;
        double processingDelayIn @unit(s) = default(0s);   // additional processing delay for incoming ip packets
        double processingDelayOut @unit(s) = default(0s);   // additional processing delay for outgoing ip packets

        string lteChannelModelType = default("LteRealisticChannelModel");

        bool d2dCapable;            // inherit the value from the parent module

        bool dualConnectivityEnabled = default(false);
        int numCarriers = default(1);

        string address @mutable = default("auto");
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.routingTableModule = default(absPath(this.routingTableModule));

    gates:
        //#
        //# Gates connecting UE/eNB and LTE Stack
        //#
        //# Control Ports
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @loose;  // to receive messages sent using sendDirect()
        input nrRadioIn @loose;// for NR support
        inout x2[] @loose;     // optional x2 manager

    submodules:
        // handover helper, performs packet holding / forwarding
        handoverPacketHolder: <> like IHandoverPacketHolder {
            @display("p=412,67");
        }
        // technology selection for outgoing packets
        technologyDecision: <default("TechnologyDecision")> like ITechnologyDecision {
            nodeType = parent.nodeType;
            @display("p=475,67");
        }
        // bridge between radio nic and network layer
        ip2nic: <default("Ip2Nic")> like INetworkLayer2CellularNic {
            nodeType = parent.nodeType;
            @display("p=412,97");
        }
        // RRC Layer
        rrc: <default("Rrc")> like IRrc {
            @display("p=200,141");
        }
        // PDCP Layer
        pdcp: <> like ILtePdcp {
            @display("p=362,141");
        }
        // RLC Layer
        rlc: LteRlc {
            @display("p=362,226");
            d2dCapable = parent.d2dCapable;
            *.macModule = "^.^.mac";
            entityManager.nodeType = parent.nodeType;
        }
        // MAC Layer
        mac: <> like ILteMac {
            @display("p=363,314");
        }
        // LtePhy submodule
        phy: <> like ILtePhy {
            @display("p=363,389");
        }

        channelModel[numCarriers]: <lteChannelModelType> like ILteChannelModel {
            @display("p=100,389");
        }

         		//#
        //# Modules used to take trace of PDCP pkt flow
        //#
        packetFlowObserver: <default("")> like IPacketFlowObserver if hasRniSupport {
            @display("p=70,300");
        }

    connections allowunconnected:
        handoverPacketHolder.stackOut --> technologyDecision.upperLayerIn;
        technologyDecision.lowerLayerOut --> ip2nic.upperLayerIn;
        ip2nic.stackOut --> pdcp.upperLayerIn;
        ip2nic.stackIn <-- pdcp.upperLayerOut;

        //#
        //# Internal LTE Stack Connections
        //#

        //# PDCP <-> RLC
        pdcp.rlcOut --> rlc.upperLayerIn;
        pdcp.rlcIn <-- rlc.upperLayerOut;

        //# RLC <-> MAC
        rlc.macOut --> mac.upperLayerIn;
        rlc.macIn <-- mac.upperLayerOut;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        mac.MAC_to_PHY --> phy.upperGateIn;
        mac.PHY_to_MAC <-- phy.upperGateOut;

        //# external: lower connection
        radioIn --> phy.radioIn;
        ip2nic.upperLayerOut --> {  delay = parent.processingDelayOut; } --> upperLayerOut;
        upperLayerIn --> {  delay = parent.processingDelayIn; } --> handoverPacketHolder.upperLayerIn;
}

