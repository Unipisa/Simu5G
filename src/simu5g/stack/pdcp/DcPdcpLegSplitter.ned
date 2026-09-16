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

package simu5g.stack.pdcp;

//
// TX-side leg dispatcher of a dual-connectivity split bearer (two legs), steering
// each PDCP PDU to a leg from the split policy RRC pushes off the bearer's definition
// (TS 38.331 PDCP-Config, TS 38.323 5.2.1). A splitter serves exactly one direction:
// a UE's TX side is uplink, a base station's is downlink.
//
// Uplink (at the UE, which has both legs' RLC queues locally): below the
// ul-DataSplitThreshold the bearer stays on its primary path; at or above it, either
// leg may be used, and the spec leaves that choice to the implementation. The splitter
// fills it with ulLegSelection if the definition carries one, else by offloading to
// the least-occupied leg.
//
// Downlink (at the DC master): the secondary leg's RLC queue is at another node,
// across X2, and cannot be weighed here, so there is no threshold or load-balancing --
// dlLegSelection decides each PDU's leg, or the bearer stays on its primary leg. (Real
// downlink split flow control is X2-feedback-driven and is not modeled.)
//
// The leg-selection expression (RRC passes the direction's -- ulLegSelection or
// dlLegSelection) is over the per-bearer PDU counter "packetOrdinal" and returns the
// leg index directly, e.g. packetOrdinal % 2 for round robin. A single-leg (MCG or
// SCG) bearer has nothing to steer, and the module serves its second role alone.
//
// Legs whose peer is not attached are not offered to either policy: with one leg
// live the choice is made already. That leg's DC-specific id mapping on the way out
// is selected by its cell group (the legs parameter):
//  - an MCG leg (the anchor stack): ids already correct; emits pdcpSduSent.
//  - an SCG leg on a UE (the local secondary stack): rewrites the flow's
//    source/destination to that stack's node ids (serving node looked up per
//    packet, so handover is honored); emits pdcpSduSentNr.
//  - an SCG leg on a DC master (remote leg): rewrites the ids to (secondary
//    node, the UE's secondary-stack id) and adds the X2 addressing tag; the
//    PDU leaves via the DcMux.
//
// If no leg is live at all, the PDU falls back to leg 0 with a warning.
//
simple DcPdcpLegSplitter like IPdcpLegSplitter
{
    parameters:
        int numLegs;                        // Number of legs (set by the enclosing PdcpEntityBase)
        string legs;                        // Cell group of each leg, space-separated "MCG"/"SCG" (set by the enclosing PdcpEntityBase)
        string binderModule = default("binder");
        // The split policy (primary path, split threshold) and any legSelection override
        // are pushed by RRC from the bearer's definition; see banner comment.
        @display("i=block/dispatch");
        @signal[sentPacketToLowerLayer];
        @signal[pdcpSduSent];
        @signal[pdcpSduSentNr];
    gates:
        input in;               // PDUs from the TX entity
        output out[];           // To leg k (sized by the enclosing PdcpEntityBase's connections)
}
