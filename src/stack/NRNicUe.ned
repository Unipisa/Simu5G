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

import simu5g.stack.mac.ILteMac;
import simu5g.stack.packetFlowManager.IPacketFlowManager;
import simu5g.stack.phy.ChannelModel.ILteChannelModel;
import simu5g.stack.phy.ILtePhy;
import simu5g.stack.phy.feedback.LteDlFeedbackGenerator;
import simu5g.stack.rlc.LteRlc;


//
// This module defines a User Equipment (UE) network interface card that
// supports both LTE and NR (New Radio) technologies. It extends the
// LteNicUeD2D module by adding additional submodules and parameters to
// support NR-specific functionalities.
//
module NRNicUe extends LteNicUeD2D
{
    parameters:
        string NRChannelModelType = default("NRChannelModel_3GPP38_901");

        packetFlowManager.typename = default("PacketFlowManagerUe");

        int numNRCarriers = default(1);

        pdcpRrc.typename = default("NRPdcpRrcUe");
        phy.typename = default("NRPhyUe");	        // TODO fix this
        											// LtePhy is of type NRPhyUe so that it is
        											// be aware of the existing of the second
        											// PHY layer (needed at handover)

        @display("bgb=694,443");
        nrPhy.rlcUmModule = "^.nrRlc.um";
        nrPhy.otherPhyModule = "^.phy";
        nrPhy.channelModelModule = "^.nrChannelModel[0]";
        nrPhy.feedbackGeneratorModule = "^.nrDlFbGen";
        nrMac.packetFlowManagerModule = hasRNISupport ? "^.nrPacketFlowManager" : "";
        phy.otherPhyModule = "^.nrPhy";

    submodules:
        // RLC Layer
        nrRlc: LteRlc {
            @display("p=466,226");
            d2dCapable = parent.d2dCapable;
            *.macModule = "^.^.nrMac";
            *.packetFlowManagerModule = parent.hasRNISupport ? "^.^.nrPacketFlowManager" : "";
            um.nodeType = parent.nodeType;
        }
        // MAC Layer
        nrMac: <default("NRMacUe")> like ILteMac {
            @display("p=466,314");
        }
        // PHY Layer
        nrPhy: <default("NRPhyUe")> like ILtePhy {
            @display("p=466,389");
        }
        // NR Channel Model
        nrChannelModel[numNRCarriers]: <NRChannelModelType> like ILteChannelModel {
            @display("p=232,389");
        }

        // Feedback generator submodule
        nrDlFbGen: LteDlFeedbackGenerator {
            @display("p=144,50;is=s");
            phyModule = "^.nrPhy";
        }
 		//#
        //# Modules used to take trace of PDCP pkt flow
        //#
        nrPacketFlowManager: <default("NRPacketFlowManagerUe")> like IPacketFlowManager if hasRNISupport {
            @display("p=200,300");
        }

    connections:

        //# PDCP <-> RLC
        pdcpRrc.UM_Sap++ <--> nrRlc.UM_Sap;
        pdcpRrc.AM_Sap++ <--> nrRlc.AM_Sap;
        pdcpRrc.TM_Sap++ <--> nrRlc.TM_Sap;

        //# RLC <-> MAC
        nrRlc.RLC_to_MAC --> nrMac.RLC_to_MAC;
        nrRlc.MAC_to_RLC <-- nrMac.MAC_to_RLC;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        nrMac.MAC_to_PHY --> nrPhy.upperGateIn;
        nrMac.PHY_to_MAC <-- nrPhy.upperGateOut;

        //# external: lower connection
        nrRadioIn --> nrPhy.radioIn;
}
