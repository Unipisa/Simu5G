//
//                  Simu5G
//
// Authors: Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.rrc;

//
// UE-side component that manages handover decisions and procedures.
// Receives RSSI measurements from the PHY layer based on broadcast frames,
// and triggers handover when a better serving cell is detected.
//
simple HandoverController
{
    parameters:
        @display("i=block/control");
        @class(HandoverController);

        bool dynamicCellAssociation = default(false);

        // allows handovers in the UE
        bool enableHandover = default(false);

        // whether this instance is associated with the the NR or the LTE stack
        bool isNr;

        // handover duration
        double handoverLatency @unit(s) = default(50ms);
        double handoverDetachmentTime @unit(s) = default(handoverLatency > 0s ? handoverLatency/2 : 25ms);
        double handoverAttachmentTime @unit(s) = default(handoverLatency > 0s ? handoverLatency-handoverDetachmentTime : handoverDetachmentTime);

        // if true, get minRssi from PhyPisaData
        bool minRssiDefault = default(true);

        // configurable minimum threshold RSSI for attaching to an eNB (meaningful only if minRssiDefault==false)
        double minRssi @unit("dB") = default(-99.0dB);

        double hysteresisFactor = default(10);

        string binderModule = default("binder");
        string macModule = default("^.mac");
        string rlcUmModule = default("^.rlc.um"); // or "^.nrRlc.um"
        string pdcpModule = default("^.pdcp");
        string handoverPacketFilterModule = default("^.handoverPacketFilter");
        string feedbackGeneratorModule = default("^.dlFbGen"); // TODO or "^.nrDlFbGen"
        string otherHandoverControllerModule = default("");

        bool hasCollector = default(false); // true when node has a collector module (ueCollector/nrUeCollector) for this PHY module

        @signal[servingCell];
        @statistic[servingCell](title="ID of the serving cell for the UE"; unit=""; source="servingCell"; record=vector);

}
