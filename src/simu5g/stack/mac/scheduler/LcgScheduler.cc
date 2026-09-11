//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/mac/scheduler/LcgScheduler.h"
#include "simu5g/stack/mac/buffer/LteMacBuffer.h"

namespace simu5g {

using namespace omnetpp;

LcgScheduler::LcgScheduler(LteMacUe *mac) : mac_(mac)
{
}

LcgScheduler& LcgScheduler::operator=(const LcgScheduler& other)
{
    if (&other == this)
        return *this;

    mac_ = other.mac_;
    ueScheduler_ = other.ueScheduler_;
    scheduleList_ = other.scheduleList_;
    scheduledBytesList_ = other.scheduledBytesList_;
    scheduledSoPduSizes_ = other.scheduledSoPduSizes_;
    statusMap_ = other.statusMap_;

    return *this;
}


ScheduleList& LcgScheduler::schedule(unsigned int availableBytes, Direction grantDir)
{
    // Clean up old schedule decisions.
    // For each cid, this map will store the amount of sent data (in SDUs).
    scheduleList_.clear();

    // Clean up old schedule decisions.
    // For each cid, this map will store the amount of sent data (in bytes, useful for macSduRequest).
    scheduledBytesList_.clear();

    // Clean up the NR-SO per-PDU plan (one entry per planned PDU for SO connections).
    scheduledSoPduSizes_.clear();

    // Clean up scheduling support status map
    statusMap_.clear();

    bool firstSdu = true;

    const LcgMap& lcgMap = mac_->getLcgMap();

    if (lcgMap.empty())
        return scheduleList_;

    // Per-connection service state for the round-robin within one LCG.
    struct ServedConn {
        MacCid cid;
        LteMacBuffer *vQueue;
        const LogicalChannelConfig *lcConfig;
        StatusElem *elem;
        bool rlcHeaderCharged = false; // FI: the connection's one RLC header, charged at its first served SDU
    };

    // Serves one unit from the connection: one SDU (FI) or one PDU carve (SO).
    // Returns false when the connection cannot make progress -- drained, or the
    // remaining grant does not cover its next unit's headers plus one payload
    // byte.
    auto serveOne = [&](ServedConn& c) -> bool {
        const LogicalChannelConfig& lcConfig = *c.lcConfig;

        if (c.vQueue->isEmpty())
            return false;

        if (lcConfig.soFraming) {
            // NR-SO: the RLC emits one SDU/segment per PDU (no concatenation),
            // so the grant is filled by multiplexing several PDUs into it. Record
            // each PDU's payload size; the MAC issues one SDU request per entry.
            // Continuation state is shared across this UE's per-carrier schedulers
            // (they all drain the same RLC TX buffer), so a carrier reserves the
            // header matching segmentation another carrier already did this TTI.
            std::map<MacCid, bool>& soFrontIsContinuation = mac_->getSoContinuationMap();
            int macHdr = (firstSdu ? MAC_HEADER : 0);
            // RLC header for this PDU: the exact segment-state header the RLC TX
            // will emit, so the carve and drain match byte-for-byte (sized for the
            // flow's SN field length). Only the front SDU can be a continuation.
            // UM: complete=1B (no SN) if the whole remaining SDU fits, else
            // first/continuation per nrUmHeaderBytes. AM always carries the SN
            // (nrAmHeaderBytes). TS 38.322 6.2.1.3/6.2.1.4.
            unsigned int snBits = lcConfig.snFieldLength;
            unsigned int rlcHdr;
            if (lcConfig.rlcMode == AM) {
                rlcHdr = nrAmHeaderBytes(soFrontIsContinuation[c.cid] ? NRUM_CONTINUATION : NRUM_FIRST, snBits);
            }
            else {
                int remaining = c.vQueue->front().first;
                NrUmSegState st = soFrontIsContinuation[c.cid] ? NRUM_CONTINUATION
                                : (remaining + 1 <= (int)availableBytes - macHdr) ? NRUM_COMPLETE
                                : NRUM_FIRST;
                rlcHdr = nrUmHeaderBytes(st, snBits);
            }
            int hdr = (int)rlcHdr + macHdr;
            if ((int)availableBytes <= hdr)
                return false; // no room for another PDU (header + at least 1 payload byte)
            int room = (int)availableBytes - hdr;
            PacketInfo info = c.vQueue->popFront();
            unsigned int payload;
            if ((int)info.first <= room) {
                payload = info.first; // whole (remaining) SDU fits in this PDU
                soFrontIsContinuation[c.cid] = false; // SDU fully sent
            }
            else {
                payload = (unsigned int)room; // segment; remainder stays queued
                info.first -= room;
                c.vQueue->pushFront(info);
                soFrontIsContinuation[c.cid] = true; // remainder is a continuation
            }
            // The MAC request size includes the RLC header (rlcPduMakeNr
            // subtracts it before segmenting), so add it back here so the
            // RLC carves exactly this payload.
            scheduledSoPduSizes_[c.cid].push_back(payload + rlcHdr);
            availableBytes -= hdr + payload;
            c.elem->sentData_ += hdr + payload;
            c.elem->sentSdus_++;
            firstSdu = false;
            if (c.vQueue->isEmpty())
                soFrontIsContinuation[c.cid] = false; // buffer drained; next SDU is fresh

            EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << ", " << c.cid
               << ": PDU of " << (hdr + payload) << " bytes, remaining grant: " << availableBytes << " bytes" << endl;
            return true;
        }
        else {
            // LTE FI: the connection's served SDUs concatenate into ONE RLC PDU
            // (and one MAC SDU) per TTI, so the RLC header is charged once, at
            // the connection's first served SDU.
            int macHdr = (firstSdu ? MAC_HEADER : 0);
            unsigned int rlcHdrSize = (lcConfig.rlcMode == AM) ? RLC_HEADER_AM : RLC_HEADER_UM;
            int rlcHdr = c.rlcHeaderCharged ? 0 : (int)rlcHdrSize;
            int overhead = macHdr + rlcHdr;
            if ((int)availableBytes <= overhead)
                return false; // no room for a payload byte

            int sduSize = c.vQueue->front().first;
            unsigned int served;
            if (sduSize + overhead <= (int)availableBytes) {
                // the whole SDU fits
                c.vQueue->popFront();
                served = sduSize;
                availableBytes -= sduSize + overhead;
            }
            else {
                // the SDU's head fills the rest of the grant; the tail stays queued
                int room = (int)availableBytes - overhead;
                PacketInfo info = c.vQueue->popFront();
                info.first -= room;
                c.vQueue->pushFront(info);
                served = room;
                availableBytes = 0;
            }
            // scheduled bytes carry the RLC header but not the MAC header
            c.elem->sentData_ += served + rlcHdr;
            c.elem->sentSdus_ = 1; // one concatenated MAC SDU per connection per TTI
            c.rlcHeaderCharged = true;
            firstSdu = false;

            EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << ", " << c.cid
               << ": " << served << " bytes of an SDU, remaining grant: " << availableBytes << " bytes" << endl;
            return true;
        }
    };

    // for all LCGs, in increasing-index priority order (strict priority across
    // groups; the cross-priority protection of the full LCP, the PBR token
    // bucket, is not modeled)
    for (unsigned short i = 0; i < NUM_LCGS && availableBytes > 0; ++i) {
        // -------------------------------------------------------------------------------------------------- //
        // A D2D-capable UE with both UL and D2D active connections may need to withhold
        // this UL grant so that it carries the BSR of an active D2D connection instead.
        // This is handled by the D2D subclass (LcgSchedulerD2D); no-op for non-D2D UEs.
        if (checkForPendingAdditionalBsr(grantDir, Lcg(i)))
            return scheduleList_;
        // -------------------------------------------------------------------------------------------------- //

        // collect the group's serviceable connections, in registration order
        std::vector<ServedConn> conns;
        auto it_pair = lcgMap.equal_range(Lcg(i));
        for (auto it = it_pair.first; it != it_pair.second; ++it) {
            MacCid cid = it->second.first;
            LteMacBuffer *vQueue = it->second.second;

            // connection must have the same direction as the grant
            if (mac_->getConnDesc(cid).getDirection() != grantDir)
                continue;
            if (vQueue->getQueueOccupancy() == 0)
                continue;

            StatusElem *elem = &statusMap_[cid];
            elem->occupancy_ = vQueue->getQueueLength();
            elem->sentData_ = 0;
            elem->sentSdus_ = 0;
            conns.push_back(ServedConn{ cid, vQueue, &mac_->getLogicalChannelConfig(cid), elem });
        }
        if (conns.empty())
            continue;

        EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << ", serving LCG " << i
           << " (" << conns.size() << " backlogged connections), remaining grant: " << availableBytes << " bytes" << endl;

        // Serve the group round-robin, one SDU (FI) or one PDU carve (SO) per
        // turn: connections of the same LCG have equal priority, and equal
        // priority means equal service (TS 36.321/38.321 sec 5.4.3.1), so a
        // connection's share must not depend on where bearer-establishment
        // order happened to place it in the registration map.
        bool progress = true;
        while (progress && availableBytes > 0) {
            progress = false;
            for (auto& c : conns) {
                if (availableBytes == 0)
                    break;
                if (serveOne(c))
                    progress = true;
            }
        }

        // record the group's schedule
        for (auto& c : conns) {
            if (c.elem->sentSdus_ > 0) {
                scheduleList_[c.cid] = c.elem->sentSdus_;
                scheduledBytesList_[c.cid] = c.elem->sentData_;
            }
        }
    } // END of LCG cycle

    return scheduleList_;
}

ScheduleList& LcgScheduler::getScheduledBytesList()
{
    return scheduledBytesList_;
}

const std::vector<unsigned int> *LcgScheduler::getScheduledSoPduSizes(MacCid cid) const
{
    auto it = scheduledSoPduSizes_.find(cid);
    return it != scheduledSoPduSizes_.end() ? &it->second : nullptr;
}

} //namespace simu5g
