//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//


package simu5g.stack.rlc.um;

//
// LTE (TS 36.322) receiving side of the Unacknowledged Mode (UM) of the RLC
// layer: FI-walk reassembly across concatenated PDUs with a PDU-SN reordering
// window and a t-Reordering timer. Shares the common RlcUmRxEntityBase shell
// (MAC-mux plumbing, D2D hooks, UL burst-throughput accounting) with the NR
// profile ~NrRlcUmRxEntity. Bound by default inside the ~LteRlcUmEntity
// compound. Its throughput/delay statistics are emitted on the sibling rlcMux
// module (see ~RlcMux).
//
simple LteRlcUmRxEntity like simu5g.stack.rlc.IRlcRxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/layer");
        string macModule = default("^.^.mac");  // Set by the enclosing RlcUmEntityBase compound (per leg)
        string rlcMuxModule = default("^.^.rlcMux");  // This bearer's leg mux; set by the RlcUmEntityBase compound
        double timeout @unit(s) = default(1s);            // Timeout for RX Buffer
        int rxWindowSize = default(16);                   // PDU reorder window


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
