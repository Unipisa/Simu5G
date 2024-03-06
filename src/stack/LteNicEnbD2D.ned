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
