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

package simu5g.stack.rlc.um;

//
// NR (TS 38.322) transmission side of the Unacknowledged Mode (UM) of the RLC
// layer: SI + byte-offset (SO) segmentation, one SDU segment per PDU, one sequence
// number per SDU. Shares the common RlcUmTxEntityBase shell (MAC plumbing, D2D
// mode-switch machinery) with the LTE profile ~LteRlcUmTxEntity. Bound by
// default inside the ~NrRlcUmEntity compound. The NR SO-segmentation statistics
// are declared here so they are recorded only for NR entities.
//
simple NrRlcUmTxEntity like simu5g.stack.rlc.IRlcTxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/layer");
        string macModule = default("^.^.mac");  // Set by the enclosing RlcUmEntityBase compound (per leg)
        int sn_FieldLength = default(12);   // UM SN field length in bits: 6 or 12

        @signal[wastedGrantedBytes];
        @statistic[wastedGrantedBytes](title="Wasted granted bytes"; unit="B"; source="wastedGrantedBytes"; record=sum);
        @signal[requestedPduSize];
        @statistic[requestedPduSize](title="Requested PDU size"; unit="B"; source="requestedPduSize"; record=vector);
        @signal[sentPduSize];
        @statistic[sentPduSize](title="Sent PDU size"; unit="B"; source="sentPduSize"; record=vector);
        @signal[receivedPacketFromUpperLayer];
        @statistic[receivedPacketFromUpperLayer](source="receivedPacketFromUpperLayer"; record=count,"sum(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToLowerLayer];
        @statistic[sentPacketToLowerLayer](source="sentPacketToLowerLayer"; record=count,"sum(packetBytes)"; interpolationmode=none);

    gates:
        input in;       // SDUs from the PDCP layer (direct per-DRB connection, not via a mux)
        input macIn;    // MAC SDU requests, via the RlcMux
        output out;     // PDUs and new-data notifications to MAC, via the RlcMux
}
