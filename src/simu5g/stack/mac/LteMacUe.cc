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

#include "simu5g/stack/mac/LteMacUe.h"

#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>

#include "simu5g/corenetwork/statsCollector/UeStatsCollector.h"
#include "simu5g/stack/mac/buffer/LteMacBuffer.h"
#include "simu5g/stack/mac/buffer/LteMacQueue.h"
#include "simu5g/stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "simu5g/stack/mac/packet/LteMacSduRequest.h"
#include "simu5g/stack/mac/packet/LteRac_m.h"
#include "simu5g/stack/mac/packet/LteSchedulingGrant.h"
#include "simu5g/stack/mac/scheduler/LteSchedulerUeUl.h"
#include "simu5g/stack/mac/scheduler/LcgScheduler.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/stack/rlc/packet/LteRlcNewDataTag_m.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/packetFlowObserver/PacketFlowSignals.h"

namespace simu5g {

Define_Module(LteMacUe);

using namespace inet;
using namespace omnetpp;

LteMacUe::LteMacUe()
{
    nodeType_ = UE;

    // KLUDGE: this was uninitialized, this is just a guess
    harqProcesses_ = 8;
}

LteMacUe::~LteMacUe()
{
    for (auto &[key, scheduler] : lcgScheduler_)
        delete scheduler;

    for (auto &[key, grant] : schedulingGrant_) {
        if (grant != nullptr) {
            grant = nullptr;
        }
    }
}

void LteMacUe::initialize(int stage)
{
    LteMacBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        nodeId_ = MacNodeId(par("macNodeId").intValue());

        numPreambles_ = par("numPreambles");
        maxRacTryouts_ = par("maxRacAttempts");
        minRacBackoff_ = par("racBackoffMin");
        maxRacBackoff_ = par("racBackoffMax");
        raRespWinStart_ = par("raResponseWindow");
        bsrRtxTimerStart_ = par("retxBsrTimer");
    }
    else if (stage == INITSTAGE_SIMU5G_MAC_SCHEDULER_CREATION) {
        cellId_ = binder_->getServingNode(nodeId_);
    }
    else if (stage == inet::INITSTAGE_NETWORK_LAYER) {

        // display node ID above module icon
        getDisplayString().setTagArg("t", 0, opp_stringf("nodeId=%d", num(nodeId_)).c_str());

        // Insert UeInfo in the Binder
        UeInfo *info = new UeInfo();
        info->id = nodeId_;            // local mac ID
        info->cellId = cellId_;        // cell ID
        info->init = false;            // flag for phy initialization
        info->ue = networkNode_;  // reference to the UE module
        info->phy = phy_;

        binder_->addUeInfo(info);

        if (cellId_ != NODEID_NONE) {
            LteAmc *amc = check_and_cast<LteMacEnb *>(binder_->getMacByNodeId(cellId_))->getAmc();
            amc->attachUser(nodeId_, UL);
            amc->attachUser(nodeId_, DL);

            /*
             * @author Alessandro Noferi
             *
             * This piece of code connects the UeCollector to the relative base station Collector.
             * It checks the NIC, i.e. Lte or NR and chooses the correct UeCollector to connect.
             */

            bool isNrCell = binder_->isGNodeB(cellId_);

            if (isNrUe(nodeId_) && isNrCell) {
                EV << "I am a NR Ue with node id: " << nodeId_ << " connected to gnb with id: " << cellId_ << endl;
                if (!par("collectorModule").isEmptyString()) {
                    UeStatsCollector *ue = getModuleFromPar<UeStatsCollector>(par("collectorModule"), this);
                    binder_->addUeCollectorToEnodeB(nodeId_, ue, cellId_);
                }
            }
            else if (!isNrUe(nodeId_) && !isNrCell) {
                EV << "I am an LTE Ue with node id: " << nodeId_ << " connected to gnb with id: " << cellId_ << endl;
                if (!par("collectorModule").isEmptyString()) {
                    UeStatsCollector *ue = getModuleFromPar<UeStatsCollector>(par("collectorModule"), this);
                    binder_->addUeCollectorToEnodeB(nodeId_, ue, cellId_);
                }
            }
            else {
                EV << "I am a UE with node id: " << nodeId_ << " and the base station with id: " << cellId_ << " has a different type" << endl;
                // TODO: is this a valid check or is the collector module possible here:    ASSERT(par("collectorModule").isEmptyString());
            }
        }

        // find interface entry and use its address
        NetworkInterface *iface = findContainingNicModule(this);
        if (iface == nullptr)
            throw new cRuntimeError("no interface entry for lte interface - cannot bind node id [%hu]", num(nodeId_));

        auto ipv4if = iface->getProtocolData<Ipv4InterfaceData>();
        if (ipv4if == nullptr)
            throw new cRuntimeError("no Ipv4 interface data - cannot bind node id [%hu]", num(nodeId_));
        binder_->setMacNodeId(ipv4if->getIPAddress(), nodeId_);

        // for emulation mode
        const char *extHostAddress = networkNode_->par("extHostAddress").stringValue();
        if (strcmp(extHostAddress, "") != 0) {
            // register the address of the external host to enable forwarding
            binder_->setMacNodeId(Ipv4Address(extHostAddress), nodeId_);
        }
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        const auto& channelModels = phy_->getChannelModels();
        for (const auto& cm : channelModels) {
            lcgScheduler_[cm.first] = new LteSchedulerUeUl(this, cm.first);
        }
    }
    else if (stage == INITSTAGE_SIMU5G_TTI_SETUP) {

        // Start TTI tick
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);    // TTI TICK after other messages

        if (!isNrUe(nodeId_)) {
            // if this MAC layer refers to the LTE side of the UE, then the TTI is equal to 1ms
            ttiPeriod_ = TTI;
        }
        else {
            // otherwise, the period is equal to the minimum period according to the numerologies used by the carriers in this NR node
            ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(binder_->getUeMaxNumerologyIndex(nodeId_));

            // for each numerology available in this UE, set the corresponding timers
            const std::set<NumerologyIndex> *numerologyIndexSet = binder_->getUeNumerologyIndex(nodeId_);
            if (numerologyIndexSet != nullptr) {
                for (auto idx : *numerologyIndexSet) {
                    // set periodicity for this carrier according to its numerology
                    NumerologyPeriodCounter info;
                    info.max = 1 << (binder_->getUeMaxNumerologyIndex(nodeId_) - idx); // 2^(maxNumerologyIndex - numerologyIndex)
                    info.current = info.max - 1;
                    numerologyPeriodCounter_[idx] = info;
                }
            }
        }
        scheduleAt(NOW + ttiPeriod_, ttiTick_);
    }
}

void LteMacUe::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "flushHarqMsg") == 0) {
            flushHarqBuffers();
            delete msg;
            return;
        }
    }
    LteMacBase::handleMessage(msg);
}

int LteMacUe::macSduRequest()
{
    EV << "----- START LteMacUe::macSduRequest -----\n";
    int numRequestedSdus = 0;

    // get the number of granted bytes for each codeword
    std::vector<unsigned int> allocatedBytes;

    for (auto& grant : schedulingGrant_) {
        // skip if this is not the turn of this carrier
        if (!isCarrierActive(grant.first))
            continue;

        if (grant.second == nullptr)
            continue;

        for (size_t cw = 0; cw < grant.second->getGrantedCwBytesArraySize(); cw++)
            allocatedBytes.push_back(grant.second->getGrantedCwBytes(cw));
    }

    // Ask for a MAC sdu for each scheduled user on each codeword
    for (auto& [carrierFreq, scheduleList] : scheduleList_) {
        // skip if this is not the turn of this carrier
        if (!isCarrierActive(carrierFreq))
            continue;

        LteMacScheduleList::const_iterator it;
        for (const auto& it : *scheduleList) {
            MacCid destCid = it.first.first;
            Codeword cw = it.first.second;
            MacNodeId destId = destCid.getNodeId();

            auto key = std::make_pair(destCid, cw);
            LteMacScheduleList *scheduledBytesList = lcgScheduler_[carrierFreq]->getScheduledBytesList();
            auto bit = scheduledBytesList->find(key);

            // consume bytes on this codeword
            if (bit == scheduledBytesList->end())
                throw cRuntimeError("LteMacUe::macSduRequest - cannot find entry in scheduledBytesList");
            else {
                allocatedBytes[cw] -= bit->second;

                EV << NOW << " LteMacUe::macSduRequest - cid[" << destCid << "] - sdu size[" << bit->second << "B] - " << allocatedBytes[cw] << " bytes left on codeword " << cw << endl;

                // NR-SO connections fill the grant with several one-SDU/segment PDUs;
                // issue one request per planned PDU so the MAC PDU multiplexes them.
                // LTE-FI fills the grant with a single concatenated PDU (one request).
                const std::vector<unsigned int> *soSizes = lcgScheduler_[carrierFreq]->getScheduledSoPduSizes(destCid);
                std::vector<unsigned int> reqSizes;
                if (soSizes != nullptr && !soSizes->empty())
                    reqSizes = *soSizes;
                else
                    reqSizes.push_back(bit->second);

                for (unsigned int reqSize : reqSizes) {
                    // send the request message to the upper layer
                    // TODO: Replace by tag
                    auto pkt = new Packet("LteMacSduRequest");
                    auto macSduRequest = makeShared<LteMacSduRequest>();
                    macSduRequest->setChunkLength(b(1)); // TODO: should be 0
                    macSduRequest->setUeId(destId);
                    macSduRequest->setLcid(destCid.getLcid());
                    macSduRequest->setSduSize(reqSize);
                    pkt->insertAtFront(macSduRequest);
                    *(pkt->addTag<FlowControlInfo>()) = connDescOut_[destCid].flowInfo.toFlowControlInfo();
                    sendUpperPackets(pkt);

                    numRequestedSdus++;
                }
            }
        }
    }

    EV << "------ END LteMacUe::macSduRequest ------\n";
    return numRequestedSdus;
}

UnitList LteMacUe::reserveTxHarqUnits(LteHarqBufferTx *txBuf, Direction dir)
{
    // synchronous H-ARQ: use the empty units of the current process
    return txBuf->getEmptyUnits(currentHarq_);
}

bool LteMacUe::bufferizePacket(cPacket *cpkt)
{
    auto pkt = check_and_cast<Packet *>(cpkt);

    pkt->setTimestamp();           // add timestamp with current time to packet

    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    // obtain the cid from the packet information
    MacCid cid = MacCid(lteInfo->getDestId(), drbIdToLcid(lteInfo->getDrbId()));

    bool isNewDataInd = pkt->findTag<LteRlcNewDataTag>() != nullptr;

    // For RLC-AM, status reports need an outgoing connection in the reverse
    // direction; create it on demand. A non-notification packet with no
    // connection is a stale segment and is discarded.
    if (connDescOut_.find(cid) == connDescOut_.end()) {
        if (!isNewDataInd) {
            delete pkt;
            return false;
        }
        FlowDescriptor desc = FlowDescriptor::fromFlowControlInfo(*lteInfo);
        createOutgoingConnection(cid, desc);
    }

    OutgoingConnectionInfo& connInfo = connDescOut_.at(cid);
    LteMacQueue *queue = connInfo.queue;
    LteMacBuffer *vqueue = connInfo.buffer;

    // this packet is used to signal the arrival of new data in the RLC buffers
    if (pkt->findTag<LteRlcNewDataTag>()) {
        // remove the tag since it's just a notification
        pkt->removeTag<LteRlcNewDataTag>();
        // update the virtual buffer for this connection
        // build the virtual packet corresponding to this incoming packet
        auto pdcpTag = pkt->getTag<PdcpTrackingTag>();
        PacketInfo vpkt(pdcpTag->getOriginalPacketLength(), pkt->getTimestamp());
        vqueue->pushBack(vpkt);

        delete pkt;
        return true;    // notify the activation of the connection
    }

    if (pkt->getBitLength() <= 1) { // no data in this packet - should not be buffered
        delete pkt;
        return false;
    }

    // this is a MAC SDU, buffer it in the MAC buffer
    bool dropped = !queue->pushBack(pkt);

    if (dropped) {
        // unable to buffer the packet (packet is not enqueued and will be dropped): update statistics
        EV << "LteMacBuffers : queue" << cid << " is full - cannot buffer packet " << pkt->getId() << "\n";

        totalOverflowedBytes_ += pkt->getByteLength();

        double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
        simsignal_t signal = (lteInfo->getDirection() == DL) ? macBufferOverflowDlSignal_ : macBufferOverflowUlSignal_;
        emit(signal, sample);

        // discard the RLC
        if (hasListeners(rlcPduDiscardedSignal_)) {
            unsigned int rlcSno = pkt->peekAtFront<LteRlcDataPdu>()->getPduSequenceNumber();
            RlcDiscardSignalInfo discardInfo(ctrlInfoToTxDrbKey(lteInfo.get()), rlcSno);
            emit(rlcPduDiscardedSignal_, &discardInfo);
        }

        delete pkt;
        return false;
    }

    int64_t spaceLeft = queue->getQueueSize() - queue->getByteLength();
    EV << "LteMacBuffers : Using buffer for " << cid << ", Space left in the Queue: " << spaceLeft << "\n";

    return true;
}

int64_t LteMacUe::computeUlBsrSize() const
{
    // The backlog to report to the eNB: EVERY uplink connection's virtual-buffer
    // occupancy -- whether or not the connection was scheduled in this TTI -- plus,
    // for each connection with backlog, the RLC header the requested grant also has
    // to cover (reporting the bare occupancy would ask for systematically undersized
    // grants).
    int64_t size = 0;
    for (const auto& [cid, connInfo] : connDescOut_) {
        if (connInfo.flowInfo.getDirection() != UL)
            continue;
        unsigned int occupancy = connInfo.buffer->getQueueOccupancy();
        if (occupancy == 0)
            continue;
        size += occupancy;
        RlcMode rlcMode = getLogicalChannelConfig(cid).rlcMode;
        if (rlcMode == UM)
            size += RLC_HEADER_UM;
        else if (rlcMode == AM)
            size += RLC_HEADER_AM;
    }
    return size;
}

bool LteMacUe::buildStandaloneBsr()
{
    // handleSelfMessage() calls macPduMake() when a grant arrives with nothing
    // scheduled, precisely so the grant can carry a buffer status report. The SDU
    // loop has no schedule entry to hang a PDU on, so unless one is built here
    // that report is never produced and the grant is wasted.
    for (auto& [carrierFreq, grant] : schedulingGrant_) {
        // skip if this is not the turn of this carrier
        if (!isCarrierActive(carrierFreq))
            continue;

        if (grant == nullptr || grant->getDirection() != UL || !emptyScheduleList_)
            continue;

        // the first carrier whose grant matches decides
        if (!isBsrPending())
            return false;

        int64_t size = computeUlBsrSize();

        // Reported whatever the size: a zero report is the defined way to tell the
        // scheduler the buffers are empty (TS 36.321 / TS 38.321 5.4.5), and the
        // specs cancel a BSR on inclusion in a PDU, never because it would be zero.
        auto macPkt = new Packet("LteMacPdu");
        auto header = makeShared<LteMacPdu>();
        header->setHeaderLength(MAC_HEADER);

        MacBsr *bsr = new MacBsr();
        bsr->setTimestamp(simTime().dbl());
        bsr->setSize(size);
        header->pushCe(bsr);
        macPkt->insertAtFront(header);

        auto info = macPkt->addTagIfAbsent<UserControlInfo>();
        info->setSourceId(getMacNodeId());
        info->setDestId(getMacCellId());
        info->setDirection(UL);
        info->setPacketLcid(SHORT_BSR);
        info->setCarrierFrequency(carrierFreq);
        info->setUserTxParams(grant->getUserTxParams()->dup());
        macPkt->setTimestamp(NOW);

        cancelBsr();
        macPduList_[carrierFreq][{getMacCellId(), 0}] = macPkt;
        EV << "LteMacUe::buildStandaloneBsr - BSR-only PDU created, size " << size << endl;
        return true;
    }
    return false;
}

Packet *LteMacUe::createUlMacPdu(MacCid destCid, GHz carrierFreq, MacNodeId destId)
{
    auto macPkt = new Packet("LteMacPdu");
    auto header = makeShared<LteMacPdu>();
    header->setHeaderLength(MAC_HEADER);
    macPkt->insertAtFront(header);

    auto info = macPkt->addTagIfAbsent<UserControlInfo>();
    info->setSourceId(getMacNodeId());
    info->setDestId(destId);
    info->setDirection(UL);
    info->setUserTxParams(schedulingGrant_[carrierFreq]->getUserTxParams()->dup());
    /*
     * @author Alessandro Noferi
     * retrieve the grantId from the grant object in schedulingGrant_[carrierFreq]
     * and add it as tag for this macPkt.
     *
     * This is useful at eNB side to calculate the packet delay
     */
    info->setGrantId(schedulingGrant_[carrierFreq]->getGrantId());
    info->setCarrierFrequency(carrierFreq);
    // Declare which kind of BSR this PDU may carry. Left unset, the field keeps its
    // LCID_NONE default (65535), which is not a BsrType at all -- and the eNB keys
    // its BSR virtual buffers by it (see LteMacEnb::bsrCeCid). NrMacUe and the D2D
    // MAC have always stamped it; this is the LTE UE catching up.
    info->setPacketLcid(SHORT_BSR);

    macPkt->setTimestamp(NOW);
    return macPkt;
}

void LteMacUe::macPduMake(MacCid cid)
{
    int64_t bsrSize = 0;

    macPduList_.clear();

    bool bsrAlreadyMade = buildStandaloneBsr();
    const bool standaloneBsr = bsrAlreadyMade;

    // Build a MAC PDU for each scheduled user on each codeword
    if (!bsrAlreadyMade) {
        for (auto [carrierFreq, schList] : scheduleList_) {
            // skip if this is not the turn of this carrier
            if (!isCarrierActive(carrierFreq))
                continue;

            for (auto& item : *schList) {
                MacCid destCid = item.first.first;
                Codeword cw = item.first.second;
                unsigned int sduPerCid = item.second;

                if (shouldSkipScheduleEntry(sduPerCid))
                    continue;

                MacNodeId destId = pduDestId(destCid);
                std::pair<MacNodeId, Codeword> pktId = {destId, cw};

                if (macPduList_.find(carrierFreq) == macPduList_.end()) {
                    MacPduList newList;
                    macPduList_[carrierFreq] = newList;
                }
                auto pit = macPduList_[carrierFreq].find(pktId);

                // No packets for this user on this codeword
                Packet *macPkt;
                if (pit == macPduList_[carrierFreq].end()) {
                    macPkt = createUlMacPdu(destCid, carrierFreq, destId);
                    macPduList_[carrierFreq][pktId] = macPkt;
                }
                else {
                    macPkt = pit->second;
                }

                while (sduPerCid > 0) {
                    // Add SDU to PDU
                    // Find Mac Pkt

                    if (connDescOut_.find(destCid) == connDescOut_.end())
                        throw cRuntimeError("Unable to find mac buffer for cid %s", destCid.str().c_str());

                    OutgoingConnectionInfo& connInfo = connDescOut_.at(destCid);
                    if (connInfo.queue->isEmpty()) {
                        // an SO-framing (NR) bearer may legitimately run dry here: the
                        // scheduler counts segments, not whole SDUs
                        if (getLogicalChannelConfig(destCid).soFraming)
                            break;
                        throw cRuntimeError("Empty buffer for cid %s, while expected SDUs were %d", destCid.str().c_str(), sduPerCid);
                    }

                    auto pkt = check_and_cast<Packet *>(connInfo.queue->popFront());

                    // multicast support: carry the group id from the MAC SDU to the MAC PDU
                    if (auto flowInfo = pkt->findTag<FlowControlInfo>()) {
                        MacNodeId groupId = flowInfo->getD2dGroupId();
                        if (groupId != NODEID_NONE) // for unicast, group id is NONE
                            macPkt->getTagForUpdate<UserControlInfo>()->setPacketMulticastGroupId(groupId);
                    }

                    drop(pkt);

                    // Remove PdcpTrackingTag as it's no longer needed below MAC layer.
                    // Must happen before pushSdu(), which clears the SDU's tags and then
                    // deliberately re-attaches this one.
                    // TODO It won't succeed if tag is on a packet *inside* an lteRlcFragment,
                    // but removing those would be very complicated. Tag will be removed anyway
                    // on the receiver side.
                    pkt->removeTagIfPresent<PdcpTrackingTag>();

                    auto macPdu = macPkt->removeAtFront<LteMacPdu>();

                    macPdu->pushSdu(pkt, destCid.getLcid());
                    macPkt->insertAtFront(macPdu);
                    sduPerCid--;
                }
            }
        }
    }

    // Put MAC PDUs in H-ARQ buffers

    for (auto& lit : macPduList_) {
        GHz carrierFreq = lit.first;

        // skip if this is not the turn of this carrier
        if (!isCarrierActive(carrierFreq))
            continue;

        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end()) {
            HarqTxBuffers newHarqTxBuffers;
            harqTxBuffers_[carrierFreq] = newHarqTxBuffers;
        }
        HarqTxBuffers& harqTxBuffers = harqTxBuffers_[carrierFreq];

        for (auto & pit : lit.second) {

            MacNodeId destId = pit.first.first;
            Codeword cw = pit.first.second;

            // The direction is read back off the PDU rather than assumed UL: at an LTE
            // UE createUlMacPdu() stamped UL there, so this is the same value, and the
            // D2D leg needs the real one for both the buffer and the H-ARQ policy.
            Direction dir = (Direction)pit.second->getTag<UserControlInfo>()->getDirection();

            // Check if the HarqTx buffer already exists for the destId
            // Get a reference for the destId TXBuffer
            LteHarqBufferTx *txBuf;
            HarqTxBuffers::iterator hit = harqTxBuffers.find(destId);
            if (hit != harqTxBuffers.end()) {
                // The tx buffer already exists
                txBuf = hit->second;
            }
            else {
                // the tx buffer does not exist yet for this mac node id, create one
                // FIXME: hb is never deleted
                LteHarqBufferTx *hb = createTxHarqBuffer(destId, dir);
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }

            // search for an empty unit within the current HARQ process
            UnitList txList = reserveTxHarqUnits(txBuf, dir);
            EV << "LteMacUe::macPduMake - [Used Acid=" << (unsigned int)txList.first << "] , [curr=" << (unsigned int)currentHarq_ << "]" << endl;

            auto macPkt = pit.second;

            // BSR related operations

            // according to the TS 36.321 v8.7.0, when there are uplink resources assigned to the UE, a BSR
            // has to be sent even if there is no data in the user's queues. In few words, a BSR is always
            // triggered and has to be sent when there are enough resources


            auto macPdu = macPkt->removeAtFront<LteMacPdu>();
            // A triggered BSR is reported whatever the remaining size: a zero report is
            // the defined way (TS 36.321 / TS 38.321 5.4.5, buffer-size index 0) to tell
            // the scheduler the buffers drained into this very PDU. The specs cancel a
            // BSR only once it has been included in a PDU, or when the grant cannot fit
            // the CE -- never because the buffer is empty.
            if (isBsrPending() && !bsrAlreadyMade) {
                // report the WHOLE remaining backlog, scheduled connections or not:
                // this TTI's scheduling has already drained the virtual buffers, so
                // what they hold now is exactly what the eNB still has to grant for
                bsrSize = computeUlBsrSize();
                appendBsr(macPdu, bsrSize);
                bsrAlreadyMade = true;
            }

            // While backlog remains after a piggybacked report, keep the BSR
            // retransmission timer armed so checkRAC() waits for a grant instead of
            // issuing an unnecessary RAC request. A standalone BSR-only PDU leaves
            // the timer unarmed.
            if (bsrAlreadyMade && !standaloneBsr && bsrSize > 0)
                bsrRtxTimer_ = bsrRtxTimerStart_;
            else
                bsrRtxTimer_ = 0;

            // insert updated MacPdu
            macPkt->insertAtFront(macPdu);

            EV << "LteMacUe: pduMaker created PDU: " << macPkt->str() << endl;

            // TODO: harq test
            // pdu transmission here (if any)
            // txAcid has HARQ_NONE for non-fillable codeword, acid otherwise
            if (txList.second.empty()) {
                EV << "macPduMake() : no available process for this MAC PDU in TxHarqBuffer" << endl;
                delete macPkt;
            }
            else {
                txBuf->insertPdu(txList.first, cw, macPkt);
            }
        }
    }
}

LcgScheduler *LteMacUe::createLcgScheduler()
{
    return new LcgScheduler(this);
}

LteHarqBufferTx *LteMacUe::createTxHarqBuffer(MacNodeId destId, Direction dir)
{
    if (dir != UL)
        throw cRuntimeError("LteMacUe::createTxHarqBuffer: direction %s not supported", dirToA(dir).c_str());
    return new LteHarqBufferTx(binder_, (unsigned int)harqProcesses_, this,
            check_and_cast<LteMacBase *>(binder_->getMacByNodeId(cellId_)));
}

// TODO: implement differentiated BSR attach (TS 36.321 / TS 38.321 5.4.5).
// This always emits one fixed-size BSR and never looks at how much of the grant is
// left, so there is no long-vs-short/truncated selection, no "grant too small, BSR
// suppressed" accounting and no wasted-grant statistic.
// A sketch of the intended branching (long/short/truncated by remaining available
// bytes, plus the suppressed and wasted-byte statistics) survives upstream as the
// commented-out block in v1.5.1:src/simu5g/stack/mac/LteMacUe.cc. It is written
// against the old LteSchedulerUeUl::schedule and the pre-INET-packet-API data model,
// so none of it compiles today: a record of the decision structure, not reusable code.
void LteMacUe::appendBsr(inet::Ptr<LteMacPdu> macPdu, int size)
{
    MacBsr *bsr = new MacBsr();

    bsr->setTimestamp(simTime().dbl());
    bsr->setSize(size);
    macPdu->pushCe(bsr);

    bsrTriggered_ = false;

    EV << "LteMacUe::macPduMake - BSR with size " << size << " created" << endl;
}

void LteMacUe::macPduUnmake(cPacket *cpkt)
{
    auto pkt = check_and_cast<Packet *>(cpkt);
    auto macPdu = pkt->removeAtFront<LteMacPdu>();
    auto userInfo = pkt->getTag<UserControlInfo>();

    while (macPdu->hasSdu()) {
        // Extract and send SDU
        LogicalCid lcid;
        auto upPkt = macPdu->popSdu(lcid);
        take(upPkt);

        EV << "LteMacBase: pduUnmaker extracted SDU" << endl;

        MacNodeId senderId = userInfo->getSourceId();
        MacCid cid = MacCid(senderId, lcid);

        // For RLC-AM, status reports arrive in the reverse direction and may not
        // have an incoming connection. Create one from the stored outgoing connection.
        if (connDescIn_.find(cid) == connDescIn_.end()) {
            if (connDescOut_.find(cid) != connDescOut_.end()) {
                FlowDescriptor desc = connDescOut_.at(cid).flowInfo;
                desc.setSourceId(senderId);
                desc.setDestId(getMacNodeId());
                desc.setDirection(DL);
                createIncomingConnection(cid, desc);
            }
        }
        ASSERT(connDescIn_.find(cid) != connDescIn_.end());
        *upPkt->addTag<FlowControlInfo>() = connDescIn_[cid].toFlowControlInfo();

        sendUpperPackets(upPkt);
    }

    pkt->insertAtFront(macPdu);

    ASSERT(pkt->getOwner() == this);
    delete pkt;
}

void LteMacUe::handleUpperMessage(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    bool isLteRlcPduNewDataInd = (pkt->findTag<LteRlcNewDataTag>() != nullptr);

    // bufferize packet
    bufferizePacket(pkt);

    if (!isLteRlcPduNewDataInd) {
        requestedSdus_--;
        ASSERT(requestedSdus_ >= 0);
        // build a MAC PDU only after all MAC SDUs have been received from RLC
        if (requestedSdus_ == 0) {
            // make PDU and BSR (if necessary)
            macPduMake();
            // update current HARQ process id
            EV << NOW << " LteMacUe::handleMessage - incrementing counter for HARQ processes " << (unsigned int)currentHarq_ << " --> " << (currentHarq_ + 1) % harqProcesses_ << endl;
            currentHarq_ = (currentHarq_ + 1) % harqProcesses_;
        }
    }
}

void LteMacUe::handleSelfMessage()
{
    EV << "----- UE MAIN LOOP -----" << endl;

    // extract PDUs from all HARQ RX buffers and pass them to unmaker
    for (auto& [carrierFreq, harqRxMap] : harqRxBuffers_) {
        for (auto& [nodeId, rxBuffer] : harqRxMap) {
            std::list<Packet *> pduList = rxBuffer->extractCorrectPdus();
            while (!pduList.empty()) {
                auto pdu = pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    EV << NOW << " LteMacUe::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int)currentHarq_ << endl;

    // no grant available - if user has backlogged data, it will trigger scheduling request
    // no HARQ counter is updated since no transmission is sent.

    bool noSchedulingGrants = true;
    for (const auto& git : schedulingGrant_) {
        if (git.second != nullptr)
            noSchedulingGrants = false;
    }

    if (noSchedulingGrants) {
        EV << NOW << " LteMacUe::handleSelfMessage " << nodeId_ << " NO configured grant" << endl;

        // if necessary, a RAC request will be sent to obtain a grant
        checkRAC();
        // TODO ensure all operations done before return (i.e. move H-ARQ RX purge before this point)
    }
    else {
        bool periodicGrant = false;
        bool checkRac = false;
        bool skip = false;
        for (auto& git : schedulingGrant_) {
            if (git.second != nullptr && git.second->getPeriodic()) {
                periodicGrant = true;
                GHz carrierFreq = git.first;

                // Periodic checks
                if (--expirationCounter_[carrierFreq] < 0) {
                    // Periodic grant is expired
                    git.second = nullptr;
                    // if necessary, a RAC request will be sent to obtain a grant
                    checkRac = true;
                }
                else if (--periodCounter_[carrierFreq] > 0) {
                    skip = true;
                }
                else {
                    // resetting grant period
                    periodCounter_[carrierFreq] = git.second->getPeriod();
                    // this is periodic grant TTI - continue with frame sending
                    checkRac = false;
                    skip = false;
                    break;
                }
            }
        }
        if (periodicGrant) {
            if (checkRac)
                checkRAC();
            else {
                if (skip)
                    return;
            }
        }
    }

    scheduleList_.clear();
    requestedSdus_ = 0;
    if (!noSchedulingGrants) { // if a grant is configured
        if (!firstTx) {
            EV << "\t currentHarq_ counter initialized " << endl;
            firstTx = true;
            // the eNB will receive the first PDU in 2 TTI, thus initializing acid to 0
            currentHarq_ = harqProcesses_ - 2;
        }

        EV << NOW << " LteMacUe::handleSelfMessage " << nodeId_ << " entered scheduling" << endl;

        bool retx = false;

        LteHarqBufferTx *currHarq;
        for (auto& [carrierFrequency, harqTxMap] : harqTxBuffers_) {
            // skip if no grant is configured for this carrier
            if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end() || schedulingGrant_[carrierFrequency] == nullptr)
                continue;

            for (auto& [nodeId, harqBuffer] : harqTxMap) {
                EV << "\t Looking for retx in acid " << (unsigned int)currentHarq_ << endl;
                currHarq = harqBuffer;

                // check if the current process has unit ready for retx
                bool ready = currHarq->getProcess(currentHarq_)->hasReadyUnits();
                CwList cwListRetx = currHarq->getProcess(currentHarq_)->readyUnitsIds();

                EV << "\t [process=" << (unsigned int)currentHarq_ << "] , [retx=" << (ready ? "true" : "false")
                   << "] , [n=" << cwListRetx.size() << "]" << endl;

                // check if one 'ready' unit has the same direction as the grant
                bool checkDir = false;
                for (Codeword cw : cwListRetx) {
                    auto info = currHarq->getProcess(currentHarq_)->getPdu(cw)->getTag<UserControlInfo>();
                    if (info->getDirection() == schedulingGrant_[carrierFrequency]->getDirection()) {
                        checkDir = true;
                        break;
                    }
                }

                // if a retransmission is needed
                if (ready && checkDir) {
                    UnitList signal;
                    signal.first = currentHarq_;
                    signal.second = cwListRetx;
                    currHarq->markSelected(signal, schedulingGrant_[carrierFrequency]->getUserTxParams()->getLayers().size());
                    retx = true;
                }
            }
        }
        // if no retx is needed, proceed with normal scheduling
        if (!retx) {
            emptyScheduleList_ = true;
            std::map<GHz, LteSchedulerUeUl *>::iterator sit;
            for (auto [carrierFrequency, carrierLcgScheduler] : lcgScheduler_) {
                EV << "LteMacUe::handleSelfMessage - running LCG scheduler for carrier [" << carrierFrequency << "]" << endl;
                LteMacScheduleList *carrierScheduleList = carrierLcgScheduler->schedule();
                EV << "LteMacUe::handleSelfMessage - scheduled " << carrierScheduleList->size() << " connections on carrier " << carrierFrequency << endl;
                scheduleList_[carrierFrequency] = carrierScheduleList;
                if (!carrierScheduleList->empty())
                    emptyScheduleList_ = false;
            }

            if (isBsrPending() && emptyScheduleList_) {
                // no connection scheduled, but we can use this grant to send a BSR to the eNB
                macPduMake();
            }
            else {
                requestedSdus_ = macSduRequest(); // returns an integer
            }
        }

        // Message that triggers flushing of Tx H-ARQ buffers for all users
        // This way, flushing is performed after the (possible) reception of new MAC PDUs
        cMessage *flushHarqMsg = new cMessage("flushHarqMsg");
        flushHarqMsg->setSchedulingPriority(1);        // after other messages
        scheduleAt(NOW, flushHarqMsg);
    }

    //============================ DEBUG ==========================
    if (debugHarq_) {
        for (const auto& [carrierFreq, harqTxMap] : harqTxBuffers_) {
            EV << "\n carrier[ " << carrierFreq << "] htxbuf.size " << harqTxMap.size() << endl;

            EV << "\n htxbuf.size " << harqTxBuffers_.size() << endl;

            int cntOuter = 0;
            int cntInner = 0;
            for (auto [nodeId, currHarq] : harqTxMap) {
                BufferStatus harqStatus = currHarq->getBufferStatus();
                EV << "\t cicloOuter " << cntOuter << " - bufferStatus.size=" << harqStatus.size() << endl;
                for (const auto& jt : harqStatus) {
                    EV << "\t\t cicloInner " << cntInner << " - jt->size=" << jt.size()
                       << " - statusCw(0/1)=" << jt.at(0).second << "/" << jt.at(1).second << endl;
                }
            }
        }
    }
    //======================== END DEBUG ==========================

    // purge corrupted PDUs from the RX H-ARQ buffers (D2D purges DL-only; no-op otherwise)
    purgeRxHarqBuffers();

    if (requestedSdus_ == 0) {
        // update current HARQ process ID
        currentHarq_ = (currentHarq_ + 1) % harqProcesses_;
    }
    EV << "--- END UE MAIN LOOP ---" << endl;
}

void LteMacUe::macHandleGrant(cPacket *pktAux)
{
    EV << NOW << " LteMacUe::macHandleGrant - UE [" << nodeId_ << "] - Grant received" << endl;

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto grant = pkt->popAtFront<LteSchedulingGrant>();

    // delete old grant
    auto userInfo = pkt->getTag<UserControlInfo>();
    GHz carrierFrequency = userInfo->getCarrierFrequency();

    EV << NOW << " LteMacUe::macHandleGrant - Direction: " << dirToA(grant->getDirection()) << " Carrier: " << carrierFrequency << endl;

    if (schedulingGrant_.find(carrierFrequency) != schedulingGrant_.end() && schedulingGrant_[carrierFrequency] != nullptr) {
        schedulingGrant_[carrierFrequency] = nullptr;
    }

    // store received grant
    schedulingGrant_[carrierFrequency] = grant;

    if (grant->getPeriodic()) {
        periodCounter_[carrierFrequency] = grant->getPeriod();
        expirationCounter_[carrierFrequency] = grant->getExpiration();
    }

    EV << NOW << "Node " << nodeId_ << " received grant of blocks " << grant->getTotalGrantedBlocks()
       << ", bytes " << grant->getGrantedCwBytes(0) << endl;

    // clearing pending RAC requests
    racRequested_ = false;

    delete pkt;
}

void LteMacUe::macHandleRac(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto racPkt = pkt->peekAtFront<LteRac>();

    if (racPkt->getSuccess()) {
        EV << "LteMacUe::macHandleRac - UE " << nodeId_ << " won RAC" << endl;
        // if RAC is won, BSR has to be sent
        bsrTriggered_ = true;
        // reset RAC counter
        currentRacTry_ = 0;
        // reset RAC backoff timer
        racBackoffTimer_ = 0;
    }
    else {
        // RAC has failed
        if (++currentRacTry_ >= maxRacTryouts_) {
            EV << NOW << " UE " << nodeId_ << ", RAC reached max attempts : " << currentRacTry_ << endl;
            // no more RAC allowed
            //! TODO flush all buffers here
            // reset RAC counter
            currentRacTry_ = 0;
            // reset RAC backoff timer
            racBackoffTimer_ = 0;
        }
        else {
            // recompute backoff timer
            racBackoffTimer_ = uniform(minRacBackoff_, maxRacBackoff_);
            EV << NOW << " UE " << nodeId_ << " RAC attempt failed, backoff extracted : " << racBackoffTimer_ << endl;
        }
    }
    delete pkt;
}

void LteMacUe::checkRAC()
{
    EV << NOW << " LteMacUe::checkRAC , UE  " << nodeId_ << ", racTimer : " << racBackoffTimer_ << " maxRacTryOuts : " << maxRacTryouts_
       << ", raRespTimer:" << raRespTimer_ << endl;

    if (racBackoffTimer_ > 0) {
        racBackoffTimer_--;
        return;
    }

    if (raRespTimer_ > 0) {
        // decrease RAC response timer
        raRespTimer_--;
        EV << NOW << " LteMacUe::checkRAC - waiting for previous RAC requests to complete (timer=" << raRespTimer_ << ")" << endl;
        return;
    }

    if (bsrRtxTimer_ > 0) {
        // decrease BSR timer
        bsrRtxTimer_--;
        EV << NOW << " LteMacUe::checkRAC - waiting for a grant, BSR RTX timer has not expired yet (timer=" << bsrRtxTimer_ << ")" << endl;
        return;
    }

    //     Avoids double requests whithin same TTI window
    if (racRequested_) {
        EV << NOW << " LteMacUe::checkRAC - double RAC request" << endl;
        racRequested_ = false;
        return;
    }

    bool trigger = false;

    for (const auto& it : connDescOut_) {
        if (!(it.second.buffer->isEmpty())) {
            trigger = true;
            break;
        }
    }

    if (!trigger)
        EV << NOW << "UE " << nodeId_ << ", RAC aborted, no data in queues " << endl;

    if ((racRequested_ = trigger)) {
        auto pkt = new Packet("RacRequest");

        auto racReq = makeShared<LteRac>();
        racReq->setPreambleIndex(intuniform(0, numPreambles_ - 1));
        pkt->insertAtFront(racReq);

        GHz carrierFrequency = phy_->getPrimaryChannelModel()->getCarrierFrequency();
        pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFrequency);
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(RACPKT);

        sendLowerPackets(pkt);

        EV << NOW << " UE  " << nodeId_ << " cell " << cellId_ << " ,RAC request sent to PHY (preamble="
           << racReq->getPreambleIndex() << ")" << endl;

        // wait at least "raRespWinStart_" TTIs before another RAC request
        raRespTimer_ = raRespWinStart_;
    }
}

void LteMacUe::updateUserTxParam(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    auto lteInfo = pkt->getTagForUpdate<UserControlInfo>();

    if (lteInfo->getFrameType() != DATAPKT)
        return;

    GHz carrierFrequency = lteInfo->getCarrierFrequency();

    lteInfo->setUserTxParams(schedulingGrant_[carrierFrequency]->getUserTxParams()->dup());

    lteInfo->setTxMode(schedulingGrant_[carrierFrequency]->getUserTxParams()->readTxMode());

    int grantedBlocks = schedulingGrant_[carrierFrequency]->getTotalGrantedBlocks();

    lteInfo->setGrantedBlocks(schedulingGrant_[carrierFrequency]->getGrantedBlocks());
    lteInfo->setTotalGrantedBlocks(grantedBlocks);
}

void LteMacUe::flushHarqBuffers()
{
    // send the selected units to lower layers
    for (auto& [carrierFreq, harqTxBuffer] : harqTxBuffers_) {
        for (auto& [nodeId, harqBuffer] : harqTxBuffer)
            harqBuffer->sendSelectedDown();
    }

    // deleting non-periodic grant
    for (auto& [carrierFreq, grant] : schedulingGrant_) {
        if (grant != nullptr && !(grant->getPeriodic())) {
            grant = nullptr;
        }
    }
}

bool LteMacUe::getHighestBackloggedFlow(MacCid& cid, unsigned int& priority)
{
    // TODO : optimize if inefficient
    // TODO : implement priorities and LCGs
    // search in virtual buffer structures

    for (const auto& item : connDescOut_) {
        if (!item.second.buffer->isEmpty()) {
            cid = item.first;
            // TODO priority = something;
            return true;
        }
    }
    return false;
}

bool LteMacUe::getLowestBackloggedFlow(MacCid& cid, unsigned int& priority)
{
    // TODO : optimize if inefficient
    // TODO : implement priorities and LCGs
    for (auto it = connDescOut_.rbegin(); it != connDescOut_.rend(); ++it) {
        if (!it->second.buffer->isEmpty()) {
            cid = it->first;
            // TODO priority = something;
            return true;
        }
    }

    return false;
}

void LteMacUe::doHandover(MacNodeId targetEnb)
{
    cellId_ = targetEnb;
}

void LteMacUe::deleteQueues(MacNodeId nodeId)
{
    Enter_Method_Silent();


    // Delete outgoing connection descriptors
    for (auto cid : getActiveConnectionCids())
        deleteOutgoingConnection(cid);

    // delete incoming connection descriptors
    for (auto it = connDescIn_.begin(); it != connDescIn_.end(); )
        it = connDescIn_.erase(it);

    // delete logical channel configuration (a UE has a single peer, so this is a
    // wholesale wipe like the connDescIn_ loop above)
    lcConfig_.clear();

    // delete H-ARQ buffers
    for (auto& [key, buffer] : harqTxBuffers_) {
        for (auto hit = buffer.begin(); hit != buffer.end(); ) {
            delete hit->second; // Delete Queue
            hit = buffer.erase(hit); // Delete Element
        }
    }

    for (auto& [key, buffer] : harqRxBuffers_) {
        for (auto hit2 = buffer.begin(); hit2 != buffer.end(); ) {
            delete hit2->second; // Delete Queue
            hit2 = buffer.erase(hit2); // Delete Element
        }
    }

    // remove traffic descriptor and lcg entry
    lcgMap_.clear();
}

} //namespace
