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
// NR (TS 38.322) receiving side of the Unacknowledged Mode (UM) of the RLC
// layer: SI + byte-offset (SO) reassembly with a per-SDU SN window and a
// t-Reassembly timer. Shares the common RlcUmRxEntityBase shell with the LTE
// profile ~LteRlcUmRxEntity. Bound by default inside the ~NrRlcUmEntity
// compound. The NR SO-reassembly statistics are declared here so they are
// recorded only for NR entities.
//
simple NrRlcUmRxEntity like simu5g.stack.rlc.IRlcRxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/layer");
        string macModule = default("^.^.mac");  // Set by the enclosing RlcUmEntityBase compound (per leg)
        string rlcMuxModule = default("^.^.rlcMux");  // This bearer's leg mux; set by the RlcUmEntityBase compound
        int UM_Window_Size = default(2048);     // SDU reassembly window: 2048 (12-bit SN) or 32 (6-bit SN)
        double t_Reassembly @unit(s) = default(80ms);

        @signal[receivedPacketFromLowerLayer];
        @statistic[receivedPacketFromLowerLayer](source="receivedPacketFromLowerLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @signal[sentPacketToUpperLayer];
        @statistic[sentPacketToUpperLayer](source="sentPacketToUpperLayer"; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);


        //# Per-bearer statistics. A bearer serves one peer, so these are already
        //# per-UE; aggregate over a UE's bearers in the analysis tool.
        @signal[rlcDelayDl];
        @statistic[rlcDelayDl](title="Delay at the rlc layer DL"; unit="s"; source="rlcDelayDl"; record=mean);
        @signal[rlcThroughputDl];
        @statistic[rlcThroughputDl](title="Throughput at the rlc layer DL"; unit="Bps"; source="rlcThroughputDl"; record=simu5g_rateavg);
        @signal[rlcDelayUl];
        @statistic[rlcDelayUl](title="Delay at the rlc layer UL"; unit="s"; source="rlcDelayUl"; record=mean);
        @signal[rlcThroughputUl];
        @statistic[rlcThroughputUl](title="Throughput at the rlc layer UL"; unit="Bps"; source="rlcThroughputUl"; record=simu5g_rateavg);
        @signal[rlcPduDelayDl];
        @statistic[rlcPduDelayDl](title="PDU delay at the rlc layer DL"; unit="s"; source="rlcPduDelayDl"; record=mean);
        @signal[rlcPduThroughputDl];
        @statistic[rlcPduThroughputDl](title="PDU throughput at the rlc layer DL"; unit="Bps"; source="rlcPduThroughputDl"; record=simu5g_rateavg);
        @signal[rlcPduDelayUl];
        @statistic[rlcPduDelayUl](title="PDU delay at the rlc layer UL"; unit="s"; source="rlcPduDelayUl"; record=mean);
        @signal[rlcPduThroughputUl];
        @statistic[rlcPduThroughputUl](title="PDU throughput at the rlc layer UL"; unit="Bps"; source="rlcPduThroughputUl"; record=simu5g_rateavg);

    gates:
        input in;       // PDUs from MAC, via the RlcMux
        output out;     // Reassembled SDUs to the PDCP layer (direct per-DRB connection, not via a mux)
}
