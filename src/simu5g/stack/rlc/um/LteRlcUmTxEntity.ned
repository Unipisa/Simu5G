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
// LTE (TS 36.322) transmission side of the Unacknowledged Mode (UM) of the RLC
// layer: FI framing + concatenation of multiple SDUs per PDU, one sequence number
// per PDU. Shares the common RlcUmTxEntityBase shell (MAC plumbing, D2D
// mode-switch machinery) with the NR profile ~NrRlcUmTxEntity. Bound by default
// inside the ~LteRlcUmEntity compound.
//
simple LteRlcUmTxEntity like simu5g.stack.rlc.IRlcTxEntity
{
    parameters:
        @dynamic(true);
        @display("i=block/layer");
        string macModule = default("^.^.mac");  // Set by the enclosing RlcUmEntityBase compound (per leg)
        int queueSize @unit(B) = default(2MiB);          // RLC TX entity SDU queue size (0: unlimited)
        @signal[rlcPduCreated];

    gates:
        input in;       // SDUs from the PDCP layer (direct per-DRB connection, not via a mux)
        input macIn;    // MAC SDU requests, via the RlcMux
        output out;     // PDUs and new-data notifications to MAC, via the RlcMux
}
