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

import simu5g.stack.mac.LteMac;
import simu5g.stack.packetFlowManager.PacketFlowManager;
import simu5g.stack.phy.ChannelModel.LteChannelModelInterface;
import simu5g.stack.phy.LtePhy;
import simu5g.stack.phy.feedback.LteDlFeedbackGenerator;
import simu5g.stack.rlc.LteRlc;


//
// UE with LTE+NR stack
//
module NRNicUe extends LteNicUeD2D
{
    parameters:
        string NRMacType = default("NRMacUe");
        string NRPhyType = default("NRPhyUe");
        string NRChannelModelType = default("NRChannelModel_3GPP38_901");

        PacketFlowManagerType = default("PacketFlowManagerUe");

        string NRPacketFlowManagerType = default("NRPacketFlowManagerUe");

        int numNRCarriers = default(1);

        LtePdcpRrcType = default("NRPdcpRrcUe");
        LtePhyType = default("NRPhyUe");	        // TODO fix this
        											// LtePhy is of type NRPhyUe so that it is
        											// be aware of the existing of the second
        											// PHY layer (needed at handover)

        @display("bgb=694,443");

    submodules:
        // RLC Layer
        nrRlc: LteRlc {
            @display("p=466,226");
            d2dCapable = parent.d2dCapable;
        }
        // MAC Layer
        nrMac: <NRMacType> like LteMac {
            interfaceTableModule = parent.interfaceTableModule;
            @display("p=466,314");
        }
        // PHY Layer
        nrPhy: <NRPhyType> like LtePhy {
            @display("p=466,389");
            @class(LtePhyType);
        }
        // NR Channel Model
        nrChannelModel[numNRCarriers]: <NRChannelModelType> like LteChannelModelInterface {
            @display("p=232,389");
        }

        // Feedback generator submodule
        nrDlFbGen: LteDlFeedbackGenerator {
            @display("p=144,50;is=s");
        }
 		//#
        //# Modules used to take trace of PDCP pkt flow
        //#
        nrPacketFlowManager: <NRPacketFlowManagerType> like PacketFlowManager if hasRNISupport {
            @display("p=197,299");
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
