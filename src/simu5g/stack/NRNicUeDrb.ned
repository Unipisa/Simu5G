//
//                  Simu5G
//
// Authors: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.stack;

import simu5g.stack.mac.ILteMac;
import simu5g.stack.multiplexer.SdapDrbMultiplexer;
import simu5g.stack.multiplexer.MacDrbMultiplexer;
import simu5g.stack.packetFlowManager.IPacketFlowManager;
import simu5g.stack.pdcp.NrPdcpUe;
import simu5g.stack.pdcp.ILtePdcp;
import simu5g.stack.phy.channelmodel.ILteChannelModel;
import simu5g.stack.phy.ILtePhy;
import simu5g.stack.phy.feedback.LteDlFeedbackGenerator;
import simu5g.stack.rlc.LteRlc;
import simu5g.stack.sdap.NrTxSdapEntity;

//
// This module defines a User Equipment (UE) network interface card that
// supports NR (New Radio) technology with extended SDAP functionality.
// It builds upon the LTE-based LteNicUeD2D module by introducing full
// SDAP-based QoS Flow multiplexing, QFI-based flow differentiation, and
// support for multiple Data Radio Bearers (DRBs).
//
// When SDAP is enabled, the NIC dynamically routes packets to multiple
// PDCP and RLC instances based on their assigned DRBs, enabling per-flow
// bearer separation and future extension toward full 5G QoS-compliant simulation.
// The number of DRBs is fully configurable via the 'numDrbs' parameter.
// When SDAP is disabled, the NIC operates in legacy single-DRB mode
// for backward compatibility with existing Simu5G configurations.
//
module NRNicUeDrb extends LteNicUeD2D
{
    parameters:
        string nrChannelModelType = default("NrChannelModel_3GPP38_901");

        packetFlowManager.typename = default("PacketFlowManagerUe");

        int numNrCarriers = default(1);

        nrSdapDisabled = false;
        drbDisabled = false;

        int numDrbs; // this to allow multiple DRBs

        phy.typename = default("NrPhyUe");	        // TODO fix this
        											// LtePhy is of type NrPhyUe so that it is
        											// be aware of the existing of the second
        											// PHY layer (needed at handover)

        @display("bgb=694,443");
        //nrPhy.rlcUmModule = "^.nrRlc[0].um";
        nrPhy.otherPhyModule = "^.phy";
        nrPhy.channelModelModule = "^.nrChannelModel[0]";
        nrPhy.feedbackGeneratorModule = "^.nrDlFbGen";
        //nrMac.packetFlowManagerModule = hasRniSupport ? "^.nrPacketFlowManager" : "";
        phy.otherPhyModule = "^.nrPhy";

    submodules:
        nrTxSdapEntity: NrTxSdapEntity {
            numDrbs = parent.numDrbs;
            @display("p=596,66");
        }

        // CASE SDAP active: → multiple DRBs activated
        nrPdcp[numDrbs]: NrPdcpUe if !nrSdapDisabled {
            @display("p=596,175,r,10");
            drbIndex = index;  // assign DRB ID
        }

        // RLC Layer
        nrRlc[numDrbs]: LteRlc {
            @display("p=596,238,r,10");
            drbIndex = index;
            d2dCapable = parent.d2dCapable;
            *.macModule = "^.^.nrMac";
            *.packetFlowManagerModule = parent.hasRniSupport ? "^.^.nrPacketFlowManager" : "";
            um.nodeType = parent.nodeType;
        }
        // MAC Layer
        nrMac: <default("NrMacUe")> like ILteMac {
            @display("p=466,314");
            packetFlowManagerModule = parent.hasRniSupport ? "^.nrPacketFlowManager" : "";
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

        lMux: MacDrbMultiplexer {
            @display("p=596,314");
            gates:
                rlcIn[parent.numDrbs];
                rlcOut[parent.numDrbs];
        }

        hMux: SdapDrbMultiplexer {
            @display("p=596,111");
            gates:
                pdcpIn[parent.numDrbs];
                pdcpOut[parent.numDrbs];
        }

    connections:

        ip2nic.stackNic <--> nrTxSdapEntity.DataPort;

        // New multi-DRB connections when SDAP is enabled
        for i=0..numDrbs-1 {
            hMux.pdcpOut[i] --> nrPdcp[i].DataPort$i;
            hMux.pdcpIn[i] <-- nrPdcp[i].DataPort$o;

            //# PDCP <-> RLC
            nrPdcp[i].AM_Sap++ <--> nrRlc[i].AM_Sap;
            nrPdcp[i].UM_Sap++ <--> nrRlc[i].UM_Sap;
            nrPdcp[i].TM_Sap++ <--> nrRlc[i].TM_Sap;

            //# RLC <-> MUX
            nrRlc[i].RLC_to_MAC --> lMux.rlcIn[i];
            nrRlc[i].MAC_to_RLC <-- lMux.rlcOut[i];
        }

        hMux.sdapIn <-- nrTxSdapEntity.stackSdap$o;
        hMux.sdapOut --> nrTxSdapEntity.stackSdap$i;

		// while this is logically correct, simu5g rigid layers stacking causing issues.
		// todo: Either make changes to RLC layer or Mac layer to provide modularity.
		//# MUX <-> MAC
        lMux.macOut --> nrMac.RLC_to_MAC;
        lMux.macIn <-- nrMac.MAC_to_RLC;

		//nrRlc[3].RLC_to_MAC --> nrMac.RLC_to_MAC;
		//nrRlc[3].MAC_to_RLC <-- nrMac.MAC_to_RLC;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        nrMac.MAC_to_PHY --> nrPhy.upperGateIn;
        nrMac.PHY_to_MAC <-- nrPhy.upperGateOut;

        //# external: lower connection
        nrRadioIn --> nrPhy.radioIn;
}
