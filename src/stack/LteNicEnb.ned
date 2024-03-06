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

