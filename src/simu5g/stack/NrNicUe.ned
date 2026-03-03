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
import simu5g.stack.rlc.RlcEntityManager;
import simu5g.stack.rlc.RlcLowerMux;

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

        pdcpUpperMux.isNR = default(true);
        rrc.bearerManagement.pdcpRxEntityModuleType = default("simu5g.stack.pdcp.NrRxPdcpEntity");
        rrc.bearerManagement.pdcpTxEntityModuleType = default("simu5g.stack.pdcp.NrTxPdcpEntity");
        phy.typename = default("NrPhyUe");	        // TODO fix this
        											// LtePhy is of type NrPhyUe so that it is
        											// be aware of the existing of the second
        											// PHY layer (needed at handover)

        nrPhy.rlcUmModule = "^.nrRlcEntityManager";
        rrc.hasNrHandoverController = true;
        rrc.nrHandoverController.isNr = true;
        rrc.nrHandoverController.otherHandoverControllerModule = "^.handoverController";
        nrPhy.channelModelModule = "^.nrChannelModel[0]";
        nrPhy.feedbackGeneratorModule = "^.nrDlFbGen";
        nrPhy.handoverControllerModule = "^.rrc.nrHandoverController";
        rrc.nrHandoverController.macModule = "^.^.nrMac";
        rrc.nrHandoverController.feedbackGeneratorModule = "^.^.nrDlFbGen";
        rrc.handoverController.otherHandoverControllerModule = "^.nrHandoverController";

    submodules:
        // NR RLC submodules (flattened from former LteRlc compound)
        nrRlcLowerMux: RlcLowerMux {
            entityManagerModule = "^.nrRlcEntityManager";
            @display("p=570,430;is=s");
        }
        nrRlcEntityManager: RlcEntityManager {
            lowerMuxModule = "^.nrRlcLowerMux";
            macModule = "^.nrMac";
            isNR = true;
            hasD2DSupport = parent.d2dCapable;
            nodeType = parent.nodeType;
            @display("p=80,460");
        }
        // MAC Layer
        nrMac: <default("NrMacUe")> like ILteMac {
            @display("p=570,510");
        }
        // PHY Layer
        nrPhy: <default("NrPhyUe")> like ILtePhy {
            @display("p=570,600");
        }
        // NR Channel Model
        nrChannelModel[numNrCarriers]: <nrChannelModelType> like ILteChannelModel {
            @display("p=80,540");
        }

        // Feedback generator submodule
        nrDlFbGen: LteDlFeedbackGenerator {
            @display("p=80,620;is=s");
            phyModule = "^.nrPhy";
        }


        //#
        //# Modules used to take trace of PDCP pkt flow
        //#
        nrPacketFlowObserver: <default("NrPacketFlowObserverUe")> like IPacketFlowObserver if hasRniSupport {
            @display("p=80,700");
            macModule = "^.nrMac";
            rlcModule = "^";
            isNrObserver = true;
        }

    connections:

        //# NR RLC <-> MAC
        nrRlcLowerMux.macOut --> nrMac.upperLayerIn;
        nrRlcLowerMux.macIn <-- nrMac.upperLayerOut;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        nrMac.phyOut --> nrPhy.upperGateIn;
        nrMac.phyIn <-- nrPhy.upperGateOut;

        //# external: lower connection
        nrRadioIn --> nrPhy.radioIn;
}
