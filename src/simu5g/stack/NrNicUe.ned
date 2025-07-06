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
import simu5g.stack.phy.channelmodel.ILteChannelModel;
import simu5g.stack.phy.ILtePhy;
import simu5g.stack.phy.feedback.LteDlFeedbackGenerator;
import simu5g.stack.rlc.LteRlc;


//
// This module defines a User Equipment (UE) network interface card that
// supports both LTE and NR (New Radio) technologies. It extends the
// LteNicUeD2D module by adding additional submodules and parameters to
// support NR-specific functionalities.
//
module NrNicUe extends LteNicUeD2D
{
    parameters:
        string nrChannelModelType = default("NrChannelModel_3GPP38_901");

        packetFlowManager.typename = default("PacketFlowManagerUe");

        int numNrCarriers = default(1);

        pdcp.typename = default("NrPdcpUe");
        phy.typename = default("NrPhyUe");	        // TODO fix this
        											// LtePhy is of type NrPhyUe so that it is
        											// be aware of the existing of the second
        											// PHY layer (needed at handover)

        @display("bgb=694,443");
        nrPhy.rlcUmModule = "^.nrRlc.um";
        nrPhy.otherPhyModule = "^.phy";
        nrPhy.channelModelModule = "^.nrChannelModel[0]";
        nrPhy.feedbackGeneratorModule = "^.nrDlFbGen";
        nrMac.packetFlowManagerModule = hasRniSupport ? "^.nrPacketFlowManager" : "";
        phy.otherPhyModule = "^.nrPhy";

    submodules:
        // RLC Layer
        nrRlc: LteRlc {
            @display("p=466,226");
            d2dCapable = parent.d2dCapable;
            *.macModule = "^.^.nrMac";
            *.packetFlowManagerModule = parent.hasRniSupport ? "^.^.nrPacketFlowManager" : "";
            um.nodeType = parent.nodeType;
        }
        // MAC Layer
        nrMac: <default("NrMacUe")> like ILteMac {
            @display("p=466,314");
        }
        // PHY Layer
        nrPhy: <default("NrPhyUe")> like ILtePhy {
            @display("p=466,389");
        }
        // NR Channel Model
        nrChannelModel[numNrCarriers]: <nrChannelModelType> like ILteChannelModel {
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
        nrPacketFlowManager: <default("NrPacketFlowManagerUe")> like IPacketFlowManager if hasRniSupport {
            @display("p=200,300");
        }

    connections:

        //# PDCP <-> RLC
        pdcp.UM_Sap++ <--> nrRlc.UM_Sap;
        pdcp.AM_Sap++ <--> nrRlc.AM_Sap;
        pdcp.TM_Sap++ <--> nrRlc.TM_Sap;

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
