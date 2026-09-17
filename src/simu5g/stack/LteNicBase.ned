//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
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
import simu5g.stack.mac.ILteMac;
import simu5g.stack.packetFlowObserver.IPacketFlowObserver;
import simu5g.stack.pdcp.IPdcpMux;
import simu5g.stack.phy.IPhy;
import simu5g.stack.rlc.IRlcMux;
import simu5g.stack.rrc.IRrc;
import simu5g.stack.phy.channelmodel.IChannelModel;


//
// The LteNicBase module serves as the foundational building block for LTE networking
// in Simu5G. It integrates protocols for LTE communication, including PDCP, RLC, MAC,
// and PHY layers. This base module allows higher-level modules, such as User Equipment (UE)
// and eNodeB (evolved Node B), to configure and utilize these layers according to their
// specific requirements.
//
// The PDCP and RLC entities are missing from the submodules on purpose: the RRC
// layer's ~BearerManagement creates them per bearer at run time.
//
module LteNicBase extends NetworkInterface like ICellularNic
{
    parameters:
        @class(NonlayoutingNetworkInterface);
        @display("i=block/ifcard;bgb=800,650;bgl=3");
        bool isWireless = true;
        string interfaceTableModule;
        string routingTableModule;

        bool hasRniSupport = default(false);

        string nodeType;
        double processingDelayIn @unit(s) = default(0s);   // Additional processing delay for incoming ip packets
        double processingDelayOut @unit(s) = default(0s);   // Additional processing delay for outgoing ip packets

        string channelModelType = default("StochasticChannelModel");

        bool dualConnectivityEnabled = default(false);
        int numCarriers = default(1);

        string address @mutable = default("auto");
        // The LTE leg's modules take their MacNodeId from the node
        phy.macNodeId = parent.macNodeId;
        mac.macNodeId = parent.macNodeId;
        *.interfaceTableModule = default(absPath(this.interfaceTableModule));
        *.routingTableModule = default(absPath(this.routingTableModule));
        **.dualConnectivityEnabled = default(this.dualConnectivityEnabled);

    gates:
        //# Gates connecting UE/eNB and LTE Stack
        //# Control Ports
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @loose;  // To receive messages sent using sendDirect()
        input nrRadioIn @loose;// For NR support
        inout x2[] @loose;     // Optional X2 manager

    submodules:
        // handover helper, performs packet holding / forwarding
        handoverPacketHolder: <> like IHandoverPacketHolder {
            @display("p=440,50;is=s");
        }
        // bridge between radio nic and network layer
        ip2nic: <default("Ip2Nic")> like INetworkLayer2CellularNic {
            nodeType = parent.nodeType;
            @display("p=310,120");
        }
        // RRC Layer
        rrc: <default("Rrc")> like IRrc {
            @display("p=80,60,col;g=left");
        }
        // PDCP submodules (flattened from former PdcpLayer compound)
        pdcpMux: <default("PdcpMux")> like IPdcpMux {
            @display("p=310,190;is=s");
        }
        // RLC submodules (flattened from former LteRlc compound)
        rlcMux: <default("RlcMux")> like IRlcMux {
            @display("p=310,430;is=s");
        }
        // MAC Layer
        mac: <> like ILteMac {
            @display("p=310,510");
        }
        // PHY submodule
        phy: <> like IPhy {
            @display("p=310,600");
        }

        channelModel[numCarriers]: <channelModelType> like IChannelModel {
            @display("p=80,60,col;g=left");
        }

        //# Modules used to take trace of PDCP pkt flow
        packetFlowObserver: <default("")> like IPacketFlowObserver if hasRniSupport {
            @display("p=80,60,col;g=left");
        }

    connections allowunconnected:
        handoverPacketHolder.stackOut --> ip2nic.upperLayerIn;
        ip2nic.stackOut --> pdcpMux.upperLayerIn;
        ip2nic.stackIn <-- pdcpMux.upperLayerOut;

        //# Internal LTE Stack Connections

        //# RLC <-> MAC
        rlcMux.macOut --> mac.upperLayerIn;
        rlcMux.macIn <-- mac.upperLayerOut;

        //# Connections from LTE Stack to radio interface
        mac.phyOut --> phy.upperGateIn;
        mac.phyIn <-- phy.upperGateOut;

        //# external: lower connection
        radioIn --> phy.radioIn;
        ip2nic.upperLayerOut --> {  delay = parent.processingDelayOut; } --> upperLayerOut;
        upperLayerIn --> {  delay = parent.processingDelayIn; } --> handoverPacketHolder.upperLayerIn;
}

