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
// ~NrSchedulerGnbUl with one change: a HARQ process with a retransmission grant already
// in flight gets no further grant until that one has had time to arrive.
//
// The inherited scheduler re-grants every corrupted process every slot, which costs at
// most a grant or two terrestrially (~2 slots to hear back) but hundreds over a
// geostationary round trip -- each booking resource blocks nothing will use. The
// suppression window is one grant offset (~NtnNrMacGnb), since that is when the gNodeB
// learns whether the retransmission succeeded. Filter lives in schedulePerAcidRtx():
// returning zero is what the caller already reads as "not scheduled".
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
