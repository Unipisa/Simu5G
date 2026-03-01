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
import simu5g.stack.packetFlowObserver.IPacketFlowObserver;
import simu5g.stack.phy.ILtePhy;
import simu5g.stack.phy.channelmodel.ILteChannelModel;
import simu5g.stack.phy.feedback.LteDlFeedbackGenerator;
import simu5g.stack.rlc.LteRlc;
import simu5g.stack.rrc.HandoverController;


//
// Defines a User Equipment (UE) network interface card that
// supports both LTE and NR (New Radio) technologies. It extends the
// LteNicUeD2D module by adding additional submodules and parameters to
// support NR-specific functionalities.
//
module NrNicUe extends LteNicUeD2D
{
    parameters:
        string nrChannelModelType = default("NrChannelModel_3GPP38_901");

        packetFlowObserver.typename = default("PacketFlowObserverUe");

        int numNrCarriers = default(1);

        pdcp.typename = default("PdcpLayer");
        pdcp.isNR = default(true);
        pdcp.hasD2DSupport = default(true);
        pdcp.rxEntityModuleType = default("simu5g.stack.pdcp.NrRxPdcpEntity");
        pdcp.txEntityModuleType = default("simu5g.stack.pdcp.NrTxPdcpEntity");
        phy.typename = default("NrPhyUe");	        // TODO fix this
        											// LtePhy is of type NrPhyUe so that it is
        											// be aware of the existing of the second
        											// PHY layer (needed at handover)

        @display("bgb=694,443");
        nrPhy.rlcUmModule = "^.nrRlc.entityManager";
        nrHandoverController.otherHandoverControllerModule = "^.handoverController";
        nrPhy.channelModelModule = "^.nrChannelModel[0]";
        nrPhy.feedbackGeneratorModule = "^.nrDlFbGen";
        nrPhy.handoverControllerModule = "^.nrHandoverController";
        nrHandoverController.macModule = "^.nrMac";
        nrHandoverController.rlcUmModule = "^.nrRlc.entityManager";
        nrHandoverController.feedbackGeneratorModule = "^.nrDlFbGen";
        handoverController.otherHandoverControllerModule = "^.nrHandoverController";

    submodules:
        // RLC Layer
        nrRlc: LteRlc {
            @display("p=466,226");
            d2dCapable = parent.d2dCapable;
            *.macModule = "^.^.nrMac";
            entityManager.nodeType = parent.nodeType;
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

        // NR handover controller
        nrHandoverController: HandoverController {
            @display("p=144,100;is=s");
            isNr = true;
        }

        //#
        //# Modules used to take trace of PDCP pkt flow
        //#
        nrPacketFlowObserver: <default("NrPacketFlowObserverUe")> like IPacketFlowObserver if hasRniSupport {
            @display("p=200,300");
            macModule = "^.nrMac";
            rlcModule = "^.nrRlc";
            isNrObserver = true;
        }

    connections:

        //# PDCP <-> NR RLC
        pdcp.nrRlcOut --> nrRlc.upperLayerIn;
        pdcp.nrRlcIn <-- nrRlc.upperLayerOut;

        //# RLC <-> MAC
        nrRlc.macOut --> nrMac.upperLayerIn;
        nrRlc.macIn <-- nrMac.upperLayerOut;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        nrMac.MAC_to_PHY --> nrPhy.upperGateIn;
        nrMac.PHY_to_MAC <-- nrPhy.upperGateOut;

        //# external: lower connection
        nrRadioIn --> nrPhy.radioIn;
}
