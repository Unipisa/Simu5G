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

import inet.linklayer.contract.IWirelessInterface;
import inet.networklayer.common.NetworkInterface;
import simu5g.stack.phy.LtePhy;
import simu5g.stack.mac.LteMac;
import simu5g.stack.pdcp_rrc.LtePdcpRrc;
import simu5g.stack.phy.feedback.LteDlFeedbackGenerator;
import simu5g.stack.rlc.LteRlc;
import simu5g.stack.ip2nic.INetworkLayer2CellularNic;
import simu5g.stack.ip2nic.IP2Nic;
import simu5g.stack.backgroundTrafficGenerator.BackgroundTrafficGenerator;
import simu5g.stack.compManager.LteCompManager;
import simu5g.stack.d2dModeSelection.D2DModeSelection;
import simu5g.stack.handoverManager.LteHandoverManager;
import simu5g.stack.dualConnectivityManager.DualConnectivityManager;
import simu5g.stack.phy.ChannelModel.LteChannelModelInterface;
import simu5g.x2.LteX2Manager;

import simu5g.stack.packetFlowManager.PacketFlowManager;
// 
// Interface for the 3GPP Stack.
//
moduleinterface ICellularNic extends IWirelessInterface
{
    @display("i=block/ifcard");
    string nodeType;
        //bool d2dCapable;            // inherit the value from the parent module
    gates:
        inout EUTRAN_RRC_Sap;   // Control of RRC Layer
        inout x2[] @loose;     // optional x2 manager
        input nrRadioIn @loose;        // for NR support
}

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
        
        string PacketFlowManagerType; // One of: "PacketFlowManagerUe", "PacketFlowManagerUe"
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
            interfaceTableModule = parent.interfaceTableModule;
            routingTableModule = parent.routingTableModule;
            @display("p=362,67");
        }
        // PDCP-RRC Layer
        pdcpRrc: <LtePdcpRrcType> like LtePdcpRrc {
            @display("p=362,141");
        }
        // RLC Layer
        rlc: LteRlc {
            @display("p=362,226");
            d2dCapable = parent.d2dCapable;
        }
        // MAC Layer
        mac: <LteMacType> like LteMac {
            interfaceTableModule = parent.interfaceTableModule;
            @display("p=363,314");
        }
        // LtePhy submodule
        phy: <LtePhyType> like LtePhy {
            @display("p=363,389");
            @class(LtePhyType);
        }

        channelModel[numCarriers]: <LteChannelModelType> like LteChannelModelInterface {
            @display("p=100,389");
        }

         		//#
        //# Modules used to take trace of PDCP pkt flow
        //#
        packetFlowManager: <PacketFlowManagerType> like PacketFlowManager if hasRNISupport {
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

//
// User Equipment of LTE stack
//
module LteNicUe extends LteNicBase
{
    parameters:
        LtePdcpRrcType = default("LtePdcpRrcUe");
        LteMacType = default("LteMacUe");
        LtePhyType = default("LtePhyUe");
        PacketFlowManagerType = default("PacketFlowManagerUe");
        d2dCapable = default(false);          // DO NOT CHANGE

    submodules:
        // Feedback generator submodule
        dlFbGen: LteDlFeedbackGenerator {
            @display("p=50,50;is=s");
        }
}

//
// D2D-capable User Equipment of LTE stack
//
module LteNicUeD2D extends LteNicUe
{
    parameters:
        LtePdcpRrcType = default("LtePdcpRrcUeD2D");
        LteMacType = default("LteMacUeD2D");
        LtePhyType = default("LtePhyUeD2D");
        PacketFlowManagerType = default("PacketFlowManagerUe");

        d2dCapable = default(true);             // DO NOT CHANGE
        bool d2dInitialMode = default(false);
        @display("bgb=704,443");
}

//
// eNodeB of LTE stack
//
module LteNicEnb extends LteNicBase
{
    parameters:
        LtePdcpRrcType = default("LtePdcpRrcEnb");
        LteMacType = default("LteMacEnb");
        LtePhyType = default("LtePhyEnb");
        PacketFlowManagerType = default("PacketFlowManagerEnb");
        d2dCapable = default(false);          // DO NOT CHANGE
        bool compEnabled = default(false);
        string LteCompManagerType = default("LteCompManagerProportional");

    submodules:
        //#
        //# X2 manager
        //#       
        x2Manager: LteX2Manager {
            @display("p=623,226,row");
        }

        //#
        //# Modules exploiting X2 interface
        //#
        compManager: <LteCompManagerType> like LteCompManager if compEnabled {
            @display("p=493,314,row");
        }
        handoverManager: LteHandoverManager {
            @display("p=493,226,row");
        }
        dualConnectivityManager: DualConnectivityManager if dualConnectivityEnabled {
            @display("p=493,141,row");
        }
        bgTrafficGenerator[numCarriers]: BackgroundTrafficGenerator {
            @display("p=100,307,row");
        }

    connections:
        //# connections between X2 Manager and its users
        compManager.x2ManagerIn <-- x2Manager.dataPort$o++ if compEnabled;
        compManager.x2ManagerOut --> x2Manager.dataPort$i++ if compEnabled;
        handoverManager.x2ManagerIn <-- x2Manager.dataPort$o++;
        handoverManager.x2ManagerOut --> x2Manager.dataPort$i++;
        dualConnectivityManager.x2ManagerIn <-- x2Manager.dataPort$o++ if dualConnectivityEnabled;
        dualConnectivityManager.x2ManagerOut --> x2Manager.dataPort$i++ if dualConnectivityEnabled;

        if ( sizeof(x2) > 0 ) {
            //# connections between X2 Manager and GTP (for handover)
            x2$i[0] --> x2Manager.x2Gtp$i;
            x2$o[0] <-- x2Manager.x2Gtp$o;
        }
        //# connections between X2 Manager and the outer world
        for i=1..sizeof(x2)-1 {
            x2$i[i] --> x2Manager.x2$i++;
            x2$o[i] <-- x2Manager.x2$o++;
        }
}

//
// eNodeB of LTE stack with support for D2D-capable UEs
//
module LteNicEnbD2D extends LteNicEnb
{
    parameters:
        LtePdcpRrcType = default("LtePdcpRrcEnbD2D");
        LteMacType = default("LteMacEnbD2D");
        LtePhyType = default("LtePhyEnbD2D");
        PacketFlowManagerType = default("PacketFlowManagerEnb");
        d2dCapable = default(true);   // DO NOT CHANGE
        string d2dModeSelectionType = default("D2DModeSelectionBase");

    submodules:
        d2dModeSelection: <d2dModeSelectionType> like D2DModeSelection {
            @display("p=623,314,row");
        }
}
