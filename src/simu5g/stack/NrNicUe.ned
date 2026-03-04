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
import simu5g.stack.rlc.RlcMux;
import simu5g.stack.sdap.INrSdap;
import simu5g.stack.sdap.common.IReflectiveQosTable;

//
// Defines a User Equipment (UE) network interface card that
// supports both LTE and NR (New Radio) technologies. It extends the
// LteNicUeD2D module by adding additional submodules and parameters to
// support NR-specific functionalities.
//
module NrNicUe extends LteNicUeD2D
{
    parameters:
        bool hasSdap = default(false);
        bool useReflectiveQos = default(false);
        ip2nic.hasSdap = default(this.hasSdap);
        sdap.nodeRole = default("UE");
        sdap.useReflectiveQos = default(this.useReflectiveQos);
        sdap.reflectiveQosTableModule = default(this.useReflectiveQos ? "^.reflectiveQosTable" : "");
        string nrChannelModelType = default("NrChannelModel_3GPP38_901");

        packetFlowObserver.typename = default("PacketFlowObserverUe");

        int numNrCarriers = default(1);

        pdcpMux.isNR = default(true);
        rrc.bearerManagement.pdcpRxEntityModuleType = default("simu5g.stack.pdcp.NrRxPdcpEntity");
        rrc.bearerManagement.pdcpTxEntityModuleType = default("simu5g.stack.pdcp.NrTxPdcpEntity");
        phy.typename = default("NrPhyUe");	        // TODO fix this
        											// LtePhy is of type NrPhyUe so that it is
        											// be aware of the existing of the second
        											// PHY layer (needed at handover)

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
        sdap: <default("NrSdap")> like INrSdap if hasSdap {
            @display("p=230,155");
        }
        reflectiveQosTable: <default("ReflectiveQosTable")> like IReflectiveQosTable if useReflectiveQos && hasSdap {
            @display("p=80,60,col;g=left");
        }
        // NR RLC submodules (flattened from former LteRlc compound)
        nrRlcMux: RlcMux {
            @display("p=570,430;is=s");
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
            @display("p=80,60,col;g=left");
        }

        // Feedback generator submodule
        nrDlFbGen: LteDlFeedbackGenerator {
            @display("p=80,60,col;g=left");
            phyModule = "^.nrPhy";
        }

        nrPacketFlowObserver: <default("NrPacketFlowObserverUe")> like IPacketFlowObserver if hasRniSupport {
            @display("p=80,60,col;g=left");
            macModule = "^.nrMac";
            rlcModule = "^";
            isNrObserver = true;
        }

    connections:

        //# NR RLC <-> MAC
        nrRlcMux.macOut --> nrMac.upperLayerIn;
        nrRlcMux.macIn <-- nrMac.upperLayerOut;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        nrMac.phyOut --> nrPhy.upperGateIn;
        nrMac.phyIn <-- nrPhy.upperGateOut;

        //# external: lower connection
        nrRadioIn --> nrPhy.radioIn;

        //# SDAP layer (optional, spliced between ip2nic and pdcpMux)
        ip2nic.stackOut --> { @reconnect; } --> sdap.upperLayerIn if hasSdap;
        sdap.pdcpOut --> pdcpMux.upperLayerIn if hasSdap;
        pdcpMux.upperLayerOut --> { @reconnect; } --> sdap.pdcpIn if hasSdap;
        sdap.upperLayerOut --> ip2nic.stackIn if hasSdap;
}
