//
//                  Simu5G
//
// Authors: Esteban Egea Lopez (Universidad Politecnica de Cartagena), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

package simu5g.stack.rlc.am;

//
// NR (TS 38.322) transmission side of the Acknowledged Mode (AM) of the RLC
// layer: byte-offset (SO) segmentation with re-segmentation on retransmission,
// pollByte/pollPDU-driven status polling. Shares the common RlcAmTxEntityBase
// shell with the LTE profile ~LteRlcAmTxEntity. Bound by default inside the
// ~NrRlcAmEntity compound. The NR SO-segmentation statistics are declared here
// so they are recorded only for NR entities.
//
simple NrRlcAmTxEntity like simu5g.stack.rlc.IRlcAmTxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/layer");
        int AM_Window_Size = default(2048);       // 2048 (12-bit SN) or 131072 (18-bit SN)
        int pollByte @unit(B) = default(125kB);
        int pollPDU = default(64);
        int maxRtxThreshold = default(4);
        double t_PollRetransmit @unit(s) = default(80ms);
        string bearerManagementModule;   // Notified when maxRtxThreshold is reached; set by the enclosing entity

        @signal[receivedPacketFromUpperLayer];
        @statistic[receivedPacketFromUpperLayer](source="receivedPacketFromUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToLowerLayer];
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[txWindowOccupation];
        @statistic[txWindowOccupation](title="TX window occupation"; source="txWindowOccupation"; record=mean,vector);
        @signal[wastedGrantedBytes];
        @statistic[wastedGrantedBytes](title="Wasted granted bytes"; unit="B"; source="wastedGrantedBytes"; record=sum);
        @signal[requestedPduSize];
        @statistic[requestedPduSize](title="Requested PDU size"; unit="B"; source="requestedPduSize"; record=vector);
        @signal[enqueuedSduSize];
        @statistic[enqueuedSduSize](title="Enqueued SDU size"; unit="B"; source="enqueuedSduSize"; record=vector);
        @signal[enqueuedSduRate];
        @statistic[enqueuedSduRate](title="Enqueued SDU rate"; source="enqueuedSduRate"; record=vector);
        @signal[txWindowFull];
        @statistic[txWindowFull](title="TX window full events"; source="txWindowFull"; record=count);
        @signal[retransmissionPdu];
        @statistic[retransmissionPdu](title="Retransmitted PDUs"; source="retransmissionPdu"; record=count);

    gates:
        input in;       // SDUs from the PDCP layer (direct per-DRB connection, not via a mux)
        input macIn;    // MAC SDU requests, via the RlcMux
        output out;     // PDUs and new-data notifications to MAC, via the RlcMux
        input feedbackIn;   // STATUS PDUs received from the peer, from the co-located RX side (to ARQ)
        input statusIn; // Locally generated STATUS reports, from the co-located RX side (to transmit)
}
