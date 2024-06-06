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
import simu5g.stack.ip2nic.INetworkLayer2CellularNic;
import simu5g.stack.mac.LteMac;
import simu5g.stack.packetFlowManager.PacketFlowManager;
import simu5g.stack.pdcp_rrc.LtePdcpRrc;
import simu5g.stack.phy.ChannelModel.LteChannelModelInterface;
import simu5g.stack.phy.LtePhy;
import simu5g.stack.rlc.LteRlc;


//
// Base module for the LTE Stack.
//
module LteNicBase extends NetworkInterface like ICellularNic
{
    parameters:

        @class(::inet::NetworkInterface);
        @display("i=block/ifcard;bgb=704,443;bgl=3");
        bool isWireless = true;
        string interfaceTableModule;
        string routingTableModule;
        string LtePdcpRrcType;      // One of: "LtePdcpRrcUe", "LtePdcpRrcEnb", "LtePdcpRrcUeD2D", "LtePdcpRrcEnbD2D"
        string LteMacType;          // One of: "LteMacUe", "LteMacEnb", "LteMacUeD2D", "LteMacEnbD2D"

        bool hasRNISupport = default(false);

        string LtePhyType;
        string nodeType;
        double processingDelayIn @unit(s) = default(0s);   // additional processing delay for incoming ip packets
        double processingDelayOut @unit(s) = default(0s);   // additional processing delay for outgoing ip packets

        string LteChannelModelType = default("LteRealisticChannelModel");

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
        inout EUTRAN_RRC_Sap;   // Control of RRC Layer
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @loose;  // to receive messages sent using sendDirect()
        input nrRadioIn @loose;// for NR support
        inout x2[] @loose;     // optional x2 manager

    submodules:
        // bridge between radio nic and network layer
        ip2nic: <default("IP2Nic")> like INetworkLayer2CellularNic {
            nodeType = parent.nodeType;
            @display("p=412,67");
        }
        // PDCP-RRC Layer
        pdcpRrc: <LtePdcpRrcType> like LtePdcpRrc {
            @display("p=362,141");
        }
        // RLC Layer
        rlc: LteRlc {
            @display("p=362,226");
            d2dCapable = parent.d2dCapable;
            *.macModule = "^.^.mac";
            *.packetFlowManagerModule = parent.hasRNISupport ? "^.^.packetFlowManager" : "";
        }
        // MAC Layer
        mac: <LteMacType> like LteMac {
            @display("p=363,314");
        }
        // LtePhy submodule
        phy: <LtePhyType> like LtePhy {
            @display("p=363,389");
        }

        channelModel[numCarriers]: <LteChannelModelType> like LteChannelModelInterface {
            @display("p=100,389");
        }

         		//#
        //# Modules used to take trace of PDCP pkt flow
        //#
        packetFlowManager: <default("")> like PacketFlowManager if hasRNISupport {
            @display("p=69,299");
        }

    connections allowunconnected:
        ip2nic.stackNic <--> pdcpRrc.DataPort;

        //#
        //# Internal LTE Stack Connections
        //#

        EUTRAN_RRC_Sap <--> pdcpRrc.EUTRAN_RRC_Sap;

        //# PDCP <-> RLC
        pdcpRrc.UM_Sap++ <--> rlc.UM_Sap;
        pdcpRrc.AM_Sap++ <--> rlc.AM_Sap;
        pdcpRrc.TM_Sap++ <--> rlc.TM_Sap;

        //# RLC <-> MAC
        rlc.RLC_to_MAC --> mac.RLC_to_MAC;
        rlc.MAC_to_RLC <-- mac.MAC_to_RLC;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        mac.MAC_to_PHY --> phy.upperGateIn;
        mac.PHY_to_MAC <-- phy.upperGateOut;

        //# external: lower connection
        radioIn --> phy.radioIn;
        ip2nic.upperLayerOut --> {  delay = parent.processingDelayOut; } --> upperLayerOut;
        upperLayerIn --> {  delay = parent.processingDelayIn; } --> ip2nic.upperLayerIn;
}

