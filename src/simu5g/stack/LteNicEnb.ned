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

import simu5g.stack.backgroundTrafficGenerator.BackgroundTrafficGenerator;
import simu5g.stack.compManager.ILteCompManager;
import simu5g.stack.dualConnectivityManager.DualConnectivityManager;
import simu5g.stack.handoverManager.LteHandoverManager;
import simu5g.x2.LteX2Manager;


//
// The LteNicEnb module is a network interface that provides LTE connectivity
// within an eNodeB. It implements the data and signaling planes of the LTE stack,
// which includes the PDCP, RLC, MAC, and PHY layers. The parameters and submodules
// allow customization of the eNodeB's behavior, including whether it supports
// D2D communication, CoMP (Coordinated MultiPoint), or dual connectivity.
//
module LteNicEnb extends LteNicBase
{
    parameters:
        pdcpRrc.typename = default("LtePdcpRrcEnb");
        mac.typename = default("LteMacEnb");
        phy.typename = default("LtePhyEnb");
        packetFlowManager.typename = default("PacketFlowManagerEnb");
        d2dCapable = default(false);          // DO NOT CHANGE
        bool compEnabled = default(false);
        *.handoverManagerModule = "^.handoverManager";

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
        compManager: <default("LteCompManagerProportional")> like ILteCompManager if compEnabled {
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

