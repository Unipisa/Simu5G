//
//                  Simu5G
//
// Authors: Esteban Egea Lopez (Universidad PolitÃ©cnica de Cartagena), Andras Varga (OpenSim Ltd)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.stack.rlc.am;

//
// LTE (TS 36.322) transmission side of the Acknowledged Mode (AM) of the RLC
// layer: one AMD PDU per MAC grant built by concatenation (FI framing), the
// built PDU as the ARQ unit, NACK-driven retransmission with re-segmentation
// into AMD PDU segments, pollPDU/pollByte-driven status polling with
// t-PollRetransmit, and radio link failure at maxRtxThreshold retransmissions.
// Shares the common RlcAmTxEntityBase shell with the NR profile
// ~NrRlcAmTxEntity, whose ARQ skeleton it deliberately mirrors. Bound by
// default inside the ~LteRlcAmEntity compound.
//
simple LteRlcAmTxEntity like simu5g.stack.rlc.IRlcAmTxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/layer");
        int AM_Window_Size = default(512);       // TS 36.322: 512 (10-bit SN space)
        int pollByte @unit(B) = default(125kB);
        int pollPDU = default(64);
        int maxRtxThreshold = default(4);
        double t_PollRetransmit @unit(s) = default(80ms);
        string bearerManagementModule;   // Notified when maxRtxThreshold is reached; set by the enclosing entity

        @signal[receivedPacketFromUpperLayer];
        @statistic[receivedPacketFromUpperLayer](source="receivedPacketFromUpperLayer"; record=count,"sum(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToLowerLayer];
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)"; interpolationmode=none);
        @signal[txWindowOccupation];
        @statistic[txWindowOccupation](title="TX window occupation"; source="txWindowOccupation"; record=mean);
        @signal[wastedGrantedBytes];
        @statistic[wastedGrantedBytes](title="Wasted granted bytes"; unit="B"; source="wastedGrantedBytes"; record=sum);
        @signal[requestedPduSize];
        @statistic[requestedPduSize](title="Requested PDU size"; unit="B"; source="requestedPduSize"; record=mean);
        @signal[enqueuedSduSize];
        @statistic[enqueuedSduSize](title="Enqueued SDU size"; unit="B"; source="enqueuedSduSize"; record=mean);
        @signal[enqueuedSduRate];
        @statistic[enqueuedSduRate](title="Enqueued SDU rate"; source="enqueuedSduRate"; record=mean);
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
