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

import simu5g.background.trafficGenerator.BackgroundTrafficGenerator;
import simu5g.stack.compManager.ILteCompManager;
import simu5g.stack.dcX2Forwarder.DcX2Forwarder;
import simu5g.stack.pdcp.DcMux;
import simu5g.stack.handoverX2Forwarder.HandoverX2Forwarder;
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
        mac.typename = default("LteMacEnb");
        phy.typename = default("LtePhyEnb");
        handoverPacketHolder.typename = default("HandoverPacketHolderEnb");
        packetFlowObserver.typename = default("PacketFlowObserverEnb");
        d2dCapable = default(false);          // DO NOT CHANGE
        bool compEnabled = default(false);
        *.handoverX2ForwarderModule = "^.handoverX2Forwarder";

    submodules:
        pdcpDcMux: DcMux {
            @display("p=609,331;is=s");
        }

        //#
        //# X2 manager
        //#
        x2Manager: LteX2Manager {
            @display("p=717,190;is=s");
        }

        //#
        //# Modules exploiting X2 interface
        //#
        compManager: <default("LteCompManagerProportional")> like ILteCompManager if compEnabled {
            @display("p=717,286;is=s");
        }
        handoverX2Forwarder: HandoverX2Forwarder {
            @display("p=717,50;is=s");
        }
        dcX2Forwarder: DcX2Forwarder if dualConnectivityEnabled {
            @display("p=609,190;is=s");
        }
        bgTrafficGenerator[numCarriers]: BackgroundTrafficGenerator {
            @display("p=80,60,col;g=left");
        }

    connections:
        //# connections between X2 Manager and its users
        compManager.x2ManagerIn <-- x2Manager.dataPortOut++ if compEnabled;
        compManager.x2ManagerOut --> x2Manager.dataPortIn++ if compEnabled;
        handoverX2Forwarder.x2ManagerIn <-- x2Manager.dataPortOut++;
        handoverX2Forwarder.x2ManagerOut --> x2Manager.dataPortIn++;
        handoverX2Forwarder.tunnelOut --> handoverPacketHolder.x2In;
        handoverPacketHolder.hoManagerOut --> handoverX2Forwarder.dataIn;
        dcX2Forwarder.x2ManagerIn <-- x2Manager.dataPortOut++ if dualConnectivityEnabled;
        dcX2Forwarder.x2ManagerOut --> x2Manager.dataPortIn++ if dualConnectivityEnabled;
        pdcpDcMux.dcManagerOut --> dcX2Forwarder.dataIn if dualConnectivityEnabled;
        dcX2Forwarder.tunnelOut --> pdcpDcMux.dcManagerIn if dualConnectivityEnabled;

        if ( sizeof(x2) > 0 ) {
            //# connections between X2 Manager and GTP (for handover)
            x2$i[0] --> x2Manager.x2GtpIn;
            x2$o[0] <-- x2Manager.x2GtpOut;
        }
        //# connections between X2 Manager and the outer world
        for i=1..sizeof(x2)-1 {
            x2$i[i] --> x2Manager.x2In++;
            x2$o[i] <-- x2Manager.x2Out++;
        }
}

