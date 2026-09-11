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

    // for all LCGs, in increasing-index priority order
    for (unsigned short i = 0; i < NUM_LCGS; ++i) {
        // Prepare the iterators to cycle the entire scheduling set
        auto it_pair = lcgMap.equal_range(Lcg(i));
        auto it = it_pair.first, et = it_pair.second;

        EV << NOW << " LcgScheduler::schedule - Node  " << mac_->getMacNodeId() << ", serving LCG " << i << endl;

        // -------------------------------------------------------------------------------------------------- //
        // A D2D-capable UE with both UL and D2D active connections may need to withhold
        // this UL grant so that it carries the BSR of an active D2D connection instead.
        // This is handled by the D2D subclass (LcgSchedulerD2D); no-op for non-D2D UEs.
        if (checkForPendingAdditionalBsr(grantDir, Lcg(i)))
            return scheduleList_;
        // -------------------------------------------------------------------------------------------------- //

        //! FIXME Allocation of the same resource to flows with the same priority not implemented
        for (it = it_pair.first; it != et; ++it) {
            // processing all connections of the same LCG

            // get the connection virtual buffer
            LteMacBuffer *vQueue = it->second.second;

            // get the buffer size
            unsigned int queueLength = vQueue->getQueueOccupancy(); // in bytes

            // connection id of the processed connection
            MacCid cid = it->second.first;

            // get the Flow descriptor
            const FlowDescriptor& connDesc = mac_->getConnDesc(cid);
            // get the channel's RLC/logical-channel configuration, as pushed by RRC
            const LogicalChannelConfig& lcConfig = mac_->getLogicalChannelConfig(cid);
            // TODO get the QoS parameters

            // connection must have the same direction as the grant
            if (connDesc.getDirection() != grantDir)
                continue;

            unsigned int toServe = queueLength;
            // Check whether the virtual buffer is empty
            if (queueLength == 0) {
                EV << "LcgScheduler::schedule scheduled connection is no longer active " << endl;
                continue; // go to next connection
            }
            else {
                // we need to consider also the size of RLC and MAC headers
                if (lcConfig.rlcMode == UM)
                    toServe += RLC_HEADER_UM;
                else if (lcConfig.rlcMode == AM)
                    toServe += RLC_HEADER_AM;

                if (firstSdu)
                    toServe += MAC_HEADER;
            }

            // get a pointer to the appropriate status element: we need a tracing element
            // in order to store information about connections and data transmitted. These
            // information may be consulted at the end of the LCP algorithm
            StatusElem *elem;

            if (statusMap_.find(cid) == statusMap_.end()) {
                // the element does not exist, initialize it
                elem = &statusMap_[cid];
                elem->occupancy_ = vQueue->getQueueLength();
                elem->sentData_ = 0;
                elem->sentSdus_ = 0;
            }
            else {
                elem = &statusMap_[cid];
            }

            EV << NOW << " LcgScheduler::schedule Node " << mac_->getMacNodeId() << " , Parameters:" << endl;
            EV << "\t Logical Channel ID: " << cid.getLcid() << endl;
            EV << "\t CID: " << cid << endl;

            EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << ", remaining grant: " << availableBytes << " bytes " << endl;
            EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << " buffer Size: " << toServe << " bytes " << endl;

            int minBytes = firstSdu ? MAC_HEADER + RLC_HEADER_UM : RLC_HEADER_UM;

            if ((availableBytes > minBytes) && (toServe > 0)) {
                if (lcConfig.soFraming) {
                    // NR-SO: the RLC emits one SDU/segment per PDU (no concatenation),
                    // so fill the grant by multiplexing several PDUs into it. Record
                    // each PDU's payload size; the MAC issues one SDU request per entry.
                    // Continuation state is shared across this UE's per-carrier schedulers
                    // (they all drain the same RLC TX buffer), so a carrier reserves the
                    // header matching segmentation another carrier already did this TTI.
                    std::map<MacCid, bool>& soFrontIsContinuation = mac_->getSoContinuationMap();
                    std::vector<unsigned int>& pduSizes = scheduledSoPduSizes_[cid];
                    int budget = (int)availableBytes;
                    while (!vQueue->isEmpty()) {
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
                            rlcHdr = nrAmHeaderBytes(soFrontIsContinuation[cid] ? NRUM_CONTINUATION : NRUM_FIRST, snBits);
                        }
                        else {
                            int remaining = vQueue->front().first;
                            NrUmSegState st = soFrontIsContinuation[cid] ? NRUM_CONTINUATION
                                            : (remaining + 1 <= budget - macHdr) ? NRUM_COMPLETE
                                            : NRUM_FIRST;
                            rlcHdr = nrUmHeaderBytes(st, snBits);
                        }
                        int hdr = (int)rlcHdr + macHdr;
                        if (budget <= hdr)
                            break; // no room for another PDU (header + at least 1 payload byte)
                        int room = budget - hdr;
                        PacketInfo info = vQueue->popFront();
                        unsigned int payload;
                        bool segmented = false;
                        if ((int)info.first <= room) {
                            payload = info.first; // whole (remaining) SDU fits in this PDU
                            soFrontIsContinuation[cid] = false; // SDU fully sent
                        }
                        else {
                            payload = (unsigned int)room; // segment; remainder stays queued
                            info.first -= room;
                            vQueue->pushFront(info);
                            segmented = true;
                            soFrontIsContinuation[cid] = true; // remainder is a continuation
                        }
                        // The MAC request size includes the RLC header (rlcPduMakeNr
                        // subtracts it before segmenting), so add it back here so the
                        // RLC carves exactly this payload.
                        pduSizes.push_back(payload + rlcHdr);
                        int pduBytes = hdr + (int)payload;
                        budget -= pduBytes;
                        elem->sentData_ += pduBytes;
                        elem->sentSdus_++;
                        firstSdu = false;
                        if (segmented)
                            break; // grant exhausted by the segment
                    }
                    elem->occupancy_ = vQueue->getQueueOccupancy();
                    availableBytes = budget > 0 ? (unsigned int)budget : 0;
                    toServe = 0;
                    if (vQueue->isEmpty())
                        soFrontIsContinuation[cid] = false; // buffer drained; next SDU is fresh
                    if (pduSizes.empty())
                        scheduledSoPduSizes_.erase(cid);
                }
                else if (toServe <= availableBytes) {
                    // remove SDU from virtual buffer
                    vQueue->popFront();

                    // check if there is space for a SDU
                    int alloc = toServe;
                    if (firstSdu) {
                        alloc -= MAC_HEADER;
                        firstSdu = false;
                    }

                    // update the tracing element
                    elem->occupancy_ = vQueue->getQueueOccupancy();
                    elem->sentData_ += alloc;

                    if (lcConfig.rlcMode == UM)
                        alloc -= RLC_HEADER_UM;
                    else if (lcConfig.rlcMode == AM)
                        alloc -= RLC_HEADER_AM;

                    if (alloc > 0)
                        elem->sentSdus_++;

                    availableBytes -= toServe;

                    while (!vQueue->isEmpty()) {
                        // remove SDUs from virtual buffer
                        vQueue->popFront();
                    }

                    toServe = 0;

                    EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << ",  SDU of size " << elem->sentData_ << " selected for transmission" << endl;
                    EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << ", remaining grant: " << availableBytes << " bytes" << endl;
                    EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << " buffer Size: " << toServe << " bytes" << endl;
                }
                else {
                    int alloc = availableBytes;
                    if (firstSdu) {
                        alloc -= MAC_HEADER;
                        firstSdu = false;
                    }

                    // update the tracing element
                    elem->occupancy_ = vQueue->getQueueOccupancy();
                    elem->sentData_ += alloc;

                    if (lcConfig.rlcMode == UM)
                        alloc -= RLC_HEADER_UM;
                    else if (lcConfig.rlcMode == AM)
                        alloc -= RLC_HEADER_AM;

                    // check if there is space for a SDU
                    if (alloc > 0)
                        elem->sentSdus_++;

                    // update buffer
                    while (alloc > 0) {
                        // update pkt info
                        PacketInfo newPktInfo = vQueue->popFront();
                        if (newPktInfo.first > alloc) {
                            newPktInfo.first = newPktInfo.first - alloc;
                            vQueue->pushFront(newPktInfo);
                            alloc = 0;
                        }
                        else {
                            alloc -= newPktInfo.first;
                        }
                    }

                    toServe -= availableBytes;
                    availableBytes = 0;

                    EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << ",  SDU of size " << elem->sentData_ << " selected for transmission" << endl;
                    EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << ", remaining grant: " << availableBytes << " bytes" << endl;
                    EV << NOW << " LcgScheduler::schedule - Node " << mac_->getMacNodeId() << " buffer Size: " << toServe << " bytes" << endl;
                }
            }

            if (elem->sentSdus_ > 0) {
                // signal service for current connection
                unsigned int *servicedSdu = nullptr;

                if (scheduleList_.find(cid) == scheduleList_.end()) {
                    // the element does not exist, initialize it
                    servicedSdu = &scheduleList_[cid];
                    *servicedSdu = elem->sentSdus_;
                }
                else {
                    // connection already scheduled during this TTI
                    servicedSdu = &scheduleList_.at(cid);
                }

                // update scheduled bytes
                if (scheduledBytesList_.find(cid) == scheduledBytesList_.end()) {
                    scheduledBytesList_[cid] = elem->sentData_;
                }
                else {
                    scheduledBytesList_[cid] += elem->sentData_;
                }
            }
        } // END of connections cycle
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
