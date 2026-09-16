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
// RRC Bearer Management — creates and tears down PDCP, RLC and MAC
// entities for data radio bearers.
//
simple BearerManagement like IBearerManagement
{
    parameters:
        @display("i=block/control");
        @class(BearerManagement);
        string binderModule = default("binder");   // To reach a peer's BearerManagement on RLF
        string bearerConfiguratorModule = default("bearerConfigurator");   // To return a torn-down bearer's DRB identity to its pool

        // RRC re-establishment after a radio link failure (TS 38.331 5.3.7). On RLF the peer's
        // link is released (Ip2Nic drops its traffic); t311 = cell-(re)selection timer, and when
        // it expires a suitable cell is assumed selected (single-cell), the RRCReestablishmentRequest
        // is sent and t301 (request -> complete) runs; on its expiry the peer is un-released and its
        // bearer re-establishes on demand. t311 = 0s disables re-establishment (release-to-IDLE).
        // PDU-level RRC signaling is abstracted by these timers, as for handover.
        double t311 @unit(s) = default(0s);     // RRC re-establishment cell-selection timer (0 => release-to-IDLE)
        double t301 @unit(s) = default(0.1s);   // RRC re-establishment (request -> complete) timer

        // PDCP entity parameters
        int headerCompressedSize @unit(B) = default(-1B);

        // PDCP entity types: the full PDCP entity is a compound (PdcpEntityBase, TX+RX sides plus
        // the leg split/join stage of a multi-leg bearer); the instantiable types are its
        // subclasses (LtePdcpEntity, NrPdcpEntity). At a DC secondary a bearer's PDCP lives at
        // the master, so the secondary installs an PdcpRelayEntity (tunnels PDUs to/from the
        // master) in its place.
        string pdcpEntityModuleType = default("simu5g.stack.pdcp.LtePdcpEntity");
        string pdcpRelayEntityModuleType = default("simu5g.stack.pdcp.PdcpRelayEntity");

        bool dualConnectivityEnabled = default(false);   // Set by the NIC; two-leg (split-capable) bearers

        // RLC entity types (compound modules): lteRlc* = LTE-FI bearers (TS 36.322),
        // nrRlc* = NR bearers (SI/SO wire format, TS 38.322)
        string rlcTmEntityModuleType = default("simu5g.stack.rlc.RlcTmEntity");
        string lteRlcUmEntityModuleType = default("simu5g.stack.rlc.LteRlcUmEntity");
        string lteRlcAmEntityModuleType = default("simu5g.stack.rlc.LteRlcAmEntity");
        string nrRlcUmEntityModuleType = default("simu5g.stack.rlc.NrRlcUmEntity");
        string nrRlcAmEntityModuleType = default("simu5g.stack.rlc.NrRlcAmEntity");
        // An NR-leg bearer uses the nrRlc* compound (NR SI/SO wire format); a config may
        // point these back to the lteRlc* compounds above to keep an NR leg on LTE RLC.
        // TM is transparent and identical for both RATs, so it has no LTE/NR split.

        // Module references for accessing other stack layers
        string registrationModule = default("^.registration");
        string drbTableModule = default("^.drbTable");
        string ip2nicModule = default("^.^.ip2nic");
        string handoverPacketHolderModule = default("^.^.handoverPacketHolder");   // Attachment-ledger push target (UE only)
        string pdcpMuxModule = default("^.^.pdcpMux");
        string pdcpDcMuxModule = default("^.^.pdcpDcMux");
        string d2dModeControllerModule = default("^.d2dModeController");
        string rlcMuxModule = default("^.^.rlcMux");
        string nrRlcMuxModule = default("^.^.nrRlcMux");
        string macModule = default("^.^.mac");
        string nrMacModule = default("^.^.nrMac");
        string sdapModule = default("^.^.sdap");   // Configuration push target; unresolvable (no SDAP on the NIC) = no pushes
        string qosFlowClassifierModule = default("^.^.qosFlowClassifier");   // Uplink QFI rule push target; unresolvable (not a UE stack with SDAP) = no pushes
}
