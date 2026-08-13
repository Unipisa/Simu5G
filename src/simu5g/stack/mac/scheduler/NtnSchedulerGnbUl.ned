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

package simu5g.stack.mac.scheduler;

//
// NR gNodeB uplink scheduler for a cell served through a transparent NTN path. It is
// ~NrSchedulerGnbUl with one change: a HARQ process that already has a retransmission
// granted gets no further grant until that one has had time to arrive.
//
// WHY THIS IS NEEDED ONLY ON A SATELLITE LINK. The inherited scheduler grants a
// retransmission for every receive process it finds corrupted, every slot. A process
// stays corrupted until its retransmission physically arrives, which terrestrially is
// about two slots -- so at most a grant or two are wasted and nobody notices. Over a
// geostationary link the same process stays corrupted for a whole round trip, upwards
// of five hundred slots, and the gNodeB would grant it again in every one of them. Each
// of those grants books resource blocks that no transmission will ever use.
//
// This is a consequence of booking resource blocks for the slot a transmission will be
// heard in rather than the slot that scheduled it, so it belongs with that change rather
// than being a pre-existing defect: before it, the gNodeB and the UE disagreed about
// which slot a grant referred to at all.
//
// The suppression window is one grant offset: the gNodeB learns whether a retransmission
// succeeded only once it has reached the slot that retransmission arrives in, which is
// one offset beyond the slot it is booking now.
//
// The filter is applied in schedulePerAcidRtx() rather than by reimplementing the
// retransmission loop. Returning zero is what the loop already reads as "not scheduled",
// so no codeword is consumed, no bytes are counted and no retransmission is signalled --
// exactly the path taken when the allocator has no room.
//
simple NtnSchedulerGnbUl extends NrSchedulerGnbUl
{
    parameters:
        @class(NtnSchedulerGnbUl);

        // Retransmission grants not issued because the process already had one in
        // flight. Over a geostationary link this counts what would otherwise have been
        // wasted allocations; terrestrially it would be zero.
        @signal[ntnRtxGrantsSuppressed];
        @statistic[ntnRtxGrantsSuppressed](title="Retransmission grants suppressed as already in flight"; unit=""; source="ntnRtxGrantsSuppressed"; record=sum);
}
