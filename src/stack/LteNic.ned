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
import inet.networklayer.common.InterfaceEntry;
import simu5g.stack.phy.LtePhy;
import simu5g.stack.mac.LteMac;
import simu5g.stack.pdcp_rrc.LtePdcpRrc;
import simu5g.stack.phy.feedback.LteDlFeedbackGenerator;
import simu5g.stack.rlc.LteRlc;
import simu5g.stack.ip2nic.INetworkLayer2CellularNic;
import simu5g.stack.ip2nic.IP2Nic;
import simu5g.x2.LteX2Manager;
import simu5g.stack.compManager.LteCompManager;
import simu5g.stack.d2dModeSelection.D2DModeSelection;
import simu5g.stack.handoverManager.LteHandoverManager;
import simu5g.stack.dualConnectivityManager.DualConnectivityManager;
import simu5g.stack.phy.ChannelModel.LteChannelModelInterface;

// 
// Interface for the 3GPP Stack.
//
moduleinterface ICellularNic extends IWirelessInterface
{
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
module LteNicBase extends InterfaceEntry like ICellularNic
{
    parameters:

        @class(::inet::InterfaceEntry);
        @display("i=block/ifcard;bgb=391.552,443.52;bgl=3");
        string interfaceTableModule;
        string routingTableModule;
        string LtePdcpRrcType;      // One of: "LtePdcpRrcUe", "LtePdcpRrcEnb", "LtePdcpRrcUeD2D", "LtePdcpRrcEnbD2D" 
        string LteMacType;          // One of: "LteMacUe", "LteMacEnb", "LteMacUeD2D", "LteMacEnbD2D"
        string LtePhyType;
        string nodeType;
        double processingDelayIn @unit(s) = default(0s);   // additional processing delay for incoming ip packets
        double processingDelayOut @unit(s) = default(0s);   // additional processing delay for outgoing ip packets

        string LteChannelModelType = default("LteRealisticChannelModel");

        bool d2dCapable;            // inherit the value from the parent module

        bool dualConnectivityEnabled = default(false);
        int numCarriers = default(1);

        string address = default("auto");

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
            nodeType = nodeType;
            interfaceTableModule = interfaceTableModule;
            routingTableModule = routingTableModule;
            @display("p=295.68,67.2");
        }
        // PDCP-RRC Layer
        pdcpRrc: <LtePdcpRrcType> like LtePdcpRrc {
            @display("p=295.68,141.568");
        }
        // RLC Layer
        rlc: LteRlc {
            @display("p=295.68,226.68799");
            d2dCapable = d2dCapable;
        }
        // MAC Layer
        mac: <LteMacType> like LteMac {
            interfaceTableModule = interfaceTableModule;
            @display("p=296.576,314.496");
        }
        // LtePhy submodule
        phy: <LtePhyType> like LtePhy {
            @display("p=296.576,389.75998");
            @class(LtePhyType);
        }

        channelModel[numCarriers]: <LteChannelModelType> like LteChannelModelInterface {
            @display("p=44.8,389.75998");
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
        ip2nic.upperLayerOut --> {  delay = processingDelayOut; } --> upperLayerOut;
        upperLayerIn --> {  delay = processingDelayIn; } --> ip2nic.upperLayerIn;
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
        d2dCapable = default(true);             // DO NOT CHANGE
        bool d2dInitialMode = default(false);
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
        d2dCapable = default(false);          // DO NOT CHANGE
        bool compEnabled = default(false);
        string LteCompManagerType = default("LteCompManagerProportional");

    submodules:
        //#
        //# X2 manager
        //#       
        x2Manager: LteX2Manager {
            @display("p=79.743996,226.68799,row");
        }

        //#
        //# Modules exploiting X2 interface
        //#
        compManager: <LteCompManagerType> like LteCompManager if compEnabled {
            @display("p=172.928,314.496,row");
        }
        handoverManager: LteHandoverManager {
            @display("p=172.928,226.68799,row");
        }
        dualConnectivityManager: DualConnectivityManager if dualConnectivityEnabled {
            @display("p=172.928,141.568,row");
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
        d2dCapable = default(true);   // DO NOT CHANGE
        string d2dModeSelectionType = default("D2DModeSelectionBase");

    submodules:
        d2dModeSelection: <d2dModeSelectionType> like D2DModeSelection {
            @display("p=60,68,row");
        }
}
