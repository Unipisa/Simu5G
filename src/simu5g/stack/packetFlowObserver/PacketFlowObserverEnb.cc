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

#include "PacketFlowObserverEnb.h"
#include "simu5g/stack/mac/LteMacBase.h"
#include "simu5g/stack/rlc/LteRlcDefs.h"
#include "simu5g/stack/rlc/packet/LteRlcPdu_m.h"
#include "simu5g/stack/rlc/um/NrRlcUmDataPdu.h"
#include "simu5g/stack/rlc/packet/PdcpTrackingTag_m.h"
#include "simu5g/stack/mac/packet/LteMacPdu.h"
#include "simu5g/common/LteCommon.h"
#include "simu5g/stack/pdcp/packet/LtePdcpPdu_m.h"
#include "simu5g/common/LteControlInfo.h"
#include <sstream>

namespace simu5g {

// NR-SO MAC SDUs carry a single SDU/segment keyed by the PDCP SN (snoMainPacket) and
// (for complete SDUs) no RLC SN, so the observer tracks them by snoMainPacket. Returns
// that key for an SO PDU, or the RLC SN for an LTE-FI PDU.
static unsigned int observerRlcKey(const inet::Ptr<const LteRlcDataPdu>& chunk)
{
    if (auto nr = inet::dynamicPtrCast<const NrRlcUmDataPdu>(chunk))
        return nr->getSnoMainPacket();
    return chunk->getPduSequenceNumber();
}

Define_Module(PacketFlowObserverEnb);

void PacketFlowObserverEnb::initialize(int stage)
{
    PacketFlowObserverBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        timesUe_.setName("delay");

        WATCH(connectionMap_);
        WATCH(ULPktDelay_);
        WATCH(ulGrants_);
        WATCH(packetLossRate_);
        WATCH(pdcpDelay_);
        WATCH(pdcpThroughput_);
        WATCH(pktDiscardCounterPerUe_);
        WATCH(sduDataVolume_);
    }
}

bool PacketFlowObserverEnb::hasDrbId(DrbKey drbKey)
{
    return connectionMap_.find(drbKey) != connectionMap_.end();
}

void PacketFlowObserverEnb::initDrbId(DrbKey drbKey, MacNodeId nodeId)
{
    if (connectionMap_.find(drbKey) != connectionMap_.end())
        throw cRuntimeError("%s::initDrbId - DRB %s already present", pfmType.c_str(), drbKey.str().c_str());

    // init new descriptor
    StatusDescriptor newDesc;
    newDesc.nodeId_ = nodeId;
    newDesc.burstId_ = 0;
    newDesc.burstState_ = false;

    connectionMap_[drbKey] = newDesc;
    EV_FATAL << NOW << " node id " << nodeId << " " << pfmType << "::initDrbId - initialized drbKey " << drbKey << endl;
}

void PacketFlowObserverEnb::clearDrbId(DrbKey drbKey)
{
    if (connectionMap_.find(drbKey) == connectionMap_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::clearDrbId - DRB ID " << drbKey << " not present." << endl;
        return;
    }
    else {
        connectionMap_[drbKey].pdcpStatus_.clear();
        connectionMap_[drbKey].rlcPdusPerSdu_.clear();
        connectionMap_[drbKey].rlcSdusPerPdu_.clear();
        connectionMap_[drbKey].macSdusPerPdu_.clear();
        connectionMap_[drbKey].burstStatus_.clear();
        connectionMap_[drbKey].burstId_ = 0;
        connectionMap_[drbKey].burstState_ = false;
    }

    EV_FATAL << NOW << " node id " << connectionMap_[drbKey].nodeId_ << " " << pfmType << "::clearDrbId - cleared data structures for drbKey " << drbKey << endl;
}

void PacketFlowObserverEnb::clearAllDrbIds()
{
    connectionMap_.clear();
    EV_FATAL << NOW << " " << pfmType << "::clearAllDrbIds - cleared data structures for all drbIds " << endl;
}

void PacketFlowObserverEnb::initPdcpStatus(StatusDescriptor *desc, unsigned int pdcp, unsigned int sduHeaderSize, simtime_t arrivalTime)
{
    // if pdcpStatus_ already present, error
    std::map<unsigned int, PdcpStatus>::iterator it = desc->pdcpStatus_.find(pdcp);
    if (it != desc->pdcpStatus_.end())
        throw cRuntimeError("%s::initPdcpStatus - PdcpStatus for PDCP sno [%d] already present for node %hu, this should not happen", pfmType.c_str(), pdcp, num(desc->nodeId_));

    PdcpStatus newpdcpStatus;

    newpdcpStatus.entryTime = arrivalTime;
    newpdcpStatus.discardedAtMac = false;
    newpdcpStatus.discardedAtRlc = false;
    newpdcpStatus.hasArrivedAll = false;
    newpdcpStatus.sentOverTheAir = false;
    newpdcpStatus.pdcpSduSize = sduHeaderSize; // ************************* pdcpSduSize is headerSize!!!
    newpdcpStatus.entryTime = arrivalTime;
    desc->pdcpStatus_[pdcp] = newpdcpStatus;
    EV_FATAL << pfmType << "::initPdcpStatus - PDCP PDU " << pdcp << "  with header size " << sduHeaderSize << " added" << endl;
}

void PacketFlowObserverEnb::insertPdcpSdu(inet::Packet *pdcpPkt)
{
    auto lteInfo = pdcpPkt->getTagForUpdate<FlowControlInfo>();
    DrbKey drbKey = ctrlInfoToTxDrbKey(lteInfo.get());

    /*
     * check here if the DRB ID relative to this pdcp pdu is
     * already present in the pfm
     */

    if (connectionMap_.find(drbKey) == connectionMap_.end())
        initDrbId(drbKey, lteInfo->getDestId());

    // Extract sequence number from PDCP header
    auto pdcpHeader = pdcpPkt->peekAtFront<LtePdcpHeader>();
    unsigned int pdcpSno = pdcpHeader->getSequenceNumber();
    int64_t pduSize = pdcpPkt->getByteLength();
    MacNodeId nodeId = lteInfo->getDestId();
    simtime_t entryTime = simTime();
    ////
    auto header = pdcpPkt->peekAtFront<LtePdcpHeader>();
    int sduSize = (B(pduSize) - header->getChunkLength()).get();

    int sduSizeBits = (b(pdcpPkt->getBitLength()) - header->getChunkLength()).get();

    if (sduDataVolume_.find(nodeId) == sduDataVolume_.end())
        sduDataVolume_[nodeId].dlBits = sduSizeBits;
    else
        sduDataVolume_[nodeId].dlBits += sduSizeBits;
////

    EV << pfmType << "::insertPdcpSdu - DL PDPC sdu bits: " << sduSizeBits << " sent to node: " << nodeId << endl;
    EV << pfmType << "::insertPdcpSdu - DL PDPC sdu bits: " << sduDataVolume_[nodeId].dlBits << " sent to node: " << nodeId << " in this period" << endl;

    auto cit = connectionMap_.find(drbKey);
    if (cit == connectionMap_.end()) {
        // this may occur after a handover (when data structures are cleared),
        // or when the observer receives signals from the other RLC stack in DC scenarios
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor *desc = &cit->second;

    initPdcpStatus(desc, pdcpSno, sduSize, entryTime);
    EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertPdcpSdu - PDCP status for PDCP PDU SN " << pdcpSno << " added. DRB ID " << drbKey << endl;

    // add user to delay time map if not already present since many DRB IDs can belong to one nodeId (UE)
    // consider to add at run time in case it is needed
    // put here for debug purposes
    pktDiscardCounterPerUe_[desc->nodeId_].total += 1;
    pktDiscardCounterTotal_.total += 1;
}

void PacketFlowObserverEnb::receivedPdcpSdu(inet::Packet *pdcpPkt)
{
    auto lteInfo = pdcpPkt->getTagForUpdate<FlowControlInfo>();
    MacNodeId nodeId = lteInfo->getSourceId();
    int64_t sduBits = pdcpPkt->getByteLength();

    if (sduDataVolume_.find(nodeId) == sduDataVolume_.end())
        sduDataVolume_[nodeId].ulBits = sduBits;
    else
        sduDataVolume_[nodeId].ulBits += sduBits;

    /*
     * update packetLossRate UL
     */
    // Extract sequence number from PDCP header
    auto pdcpHeader = pdcpPkt->peekAtFront<LtePdcpHeader>();
    unsigned int sno = pdcpHeader->getSequenceNumber();
    auto cit = packetLossRate_.find(nodeId);
    if (cit == packetLossRate_.end()) {
        packetLossRate_[nodeId].clear();
        cit = packetLossRate_.find(nodeId);
    }

    while (sno > cit->second.lastPdpcSno + 1) {
        cit->second.lastPdpcSno++;
        cit->second.totalPdcpSno++;
    }
    cit->second.lastPdpcSno = sno;
    cit->second.totalPdcpArrived += 1;
    cit->second.totalPdcpSno += 1;

    EV << pfmType << "::insertPdcpSdu - UL PDPC sdu bits: " << sduDataVolume_[nodeId].ulBits << " received from node: " << nodeId << endl;
}

void PacketFlowObserverEnb::insertRlcPdu(DrbKey drbKey, const LteRlcUmDataPdu *rlcPdu, RlcBurstStatus status) {
    EV << pfmType << "::insertRlcPdu - DRB ID: " << drbKey << endl;

    auto cit = connectionMap_.find(drbKey);
    if (cit == connectionMap_.end()) {
        // this may occur after a handover (when data structures are cleared),
        // or when the observer receives signals from the other RLC stack in DC scenarios
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor *desc = &cit->second;
    EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu - DRB ID " << drbKey << endl;

    unsigned int rlcSno = rlcPdu->getPduSequenceNumber();

    if (desc->rlcSdusPerPdu_.find(rlcSno) != desc->rlcSdusPerPdu_.end())
        throw cRuntimeError("%s::insertRlcPdu - RLC PDU SN %d already present for DRB %s", pfmType.c_str(), rlcSno, drbKey.str().c_str());


    // manage burst state, for debugging and avoid errors between rlc state and packetFlowObserver state
    if (status == START) {
        if (desc->burstState_ == true)
            throw cRuntimeError("%s::insertRlcPdu - node %hu and drbKey %s . RLC burst status START incompatible with local status %d", pfmType.c_str(), num(desc->nodeId_), drbKey.str().c_str(), desc->burstState_);
        BurstStatus newBurst;
        newBurst.isCompleted = false;
        newBurst.startBurstTransmission = simTime();
        newBurst.burstSize = 0;
        desc->burstStatus_[++(desc->burstId_)] = newBurst;
        desc->burstState_ = true;
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu START burst " << desc->burstId_ << " at: " << newBurst.startBurstTransmission << endl;
    }
    else if (status == STOP) {
        if (desc->burstState_ == false)
            throw cRuntimeError("%s::insertRlcPdu - node %hu and drbKey %s . RLC burst status STOP incompatible with local status %d", pfmType.c_str(), num(desc->nodeId_), drbKey.str().c_str(), desc->burstState_);
        desc->burstStatus_[desc->burstId_].isCompleted = true;
        desc->burstState_ = false;

        /*
         * If the burst ends in the same TTI, it must not be counted.
         * This workaround is due to the mac layer requesting rlc pdu
         * for each carrier in a TTI
         */
        simtime_t elapsedTime = simTime() - desc->burstStatus_[desc->burstId_].startBurstTransmission;
        if (!(elapsedTime >= TTI)) {
            // remove the burst structure
            desc->burstStatus_.erase(desc->burstId_);
            EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu burst " << desc->burstId_ << " deleted " << endl;
        }
        else {
            EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu STOP burst " << desc->burstId_ << " at: " << simTime() << endl;
        }
    }
    else if (status == INACTIVE) {
        if (desc->burstState_ == true)
            throw cRuntimeError("%s::insertRlcPdu - node %hu and drbKey %s . RLC burst status INACTIVE incompatible with local status %d", pfmType.c_str(), num(desc->nodeId_), drbKey.str().c_str(), desc->burstState_);
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu INACTIVE burst" << endl;
    }
    else if (status == ACTIVE) {
        if (desc->burstState_ == false)
            throw cRuntimeError("%s::insertRlcPdu - node %hu and drbKey %s . RLC burst status ACTIVE incompatible with local status %d", pfmType.c_str(), num(desc->nodeId_), drbKey.str().c_str(), desc->burstState_);
        auto bsit = desc->burstStatus_.find(desc->burstId_);
        if (bsit == desc->burstStatus_.end())
            throw cRuntimeError("%s::insertRlcPdu - node %hu and drbKey %s . Burst status not found during active burst", pfmType.c_str(), num(desc->nodeId_), drbKey.str().c_str());
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu ACTIVE burst " << desc->burstId_ << endl;
    }
    else {
        throw cRuntimeError("%s::insertRlcPdu RLCBurstStatus not recognized", pfmType.c_str());
    }

    FramingInfo fi = rlcPdu->getFramingInfo();
    for (size_t idx = 0; idx < rlcPdu->getNumSdu(); ++idx) {
        auto sduPacket = rlcPdu->getSdu(idx);
        auto pdcpTag = sduPacket->getTag<PdcpTrackingTag>();
        unsigned int pdcpSno = pdcpTag->getPdcpSequenceNumber();
        size_t pdcpPduLength = rlcPdu->getSduSize(idx); // TODO fix with size of the chunk!!

        EV << "PacketFlowObserverEnb::insertRlcPdu - pdcpSdu " << pdcpSno << " with length: " << pdcpPduLength << " bytes" << endl;
        //
        // store the RLC SDUs (PDCP PDUs) included in the RLC PDU
        desc->rlcSdusPerPdu_[rlcSno].insert(pdcpSno);

        // now store the inverse association, i.e., for each RLC SDU, record in which RLC PDU it is included
        desc->rlcPdusPerSdu_[pdcpSno].insert(rlcSno);

        // set the PDCP entry time
        auto pit = desc->pdcpStatus_.find(pdcpSno);
        if (pit == desc->pdcpStatus_.end())
            throw cRuntimeError("%s::insertRlcPdu - PdcpStatus for PDCP sno [%d] not present, this should not happen", pfmType.c_str(), pdcpSno);

        // last pdcp
        if (idx == rlcPdu->getNumSdu() - 1) {
            // 01 or 11, lsb 1 (3GPP TS 36.322)
            // means -> Last byte of the Data field does not correspond to the last byte of a RLC SDU.
            if (fi.lastIsFragment) {
                pit->second.hasArrivedAll = false;
            }
            else {
                pit->second.hasArrivedAll = true;
            }
        }
        // since it is not the last part of the rlc, this pdcp has been entirely inserted in RLCs
        else {
            pit->second.hasArrivedAll = true;
        }

        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu - drbKey[" << drbKey << "], insert PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
    }

    if (status == ACTIVE || status == START) {
        /*
         * NEW piece of code that counts the RLC sdu size as the burst dimension -
         *
         * According to TS 128 552 V15 (5G performance measurements) the throughput volume
         * is counted at the RLC SDU level
         */
        int rlcSduSize = (B(rlcPdu->getChunkLength()) - B(RLC_HEADER_UM)).get(); // RLC pdu size - RLC header

        auto bsit = desc->burstStatus_.find(desc->burstId_);
        if (bsit == desc->burstStatus_.end())
            throw cRuntimeError("%s::insertRlcPdu - node %hu and drbKey %s . Burst status not found during active burst", pfmType.c_str(), num(desc->nodeId_), drbKey.str().c_str());
        // add rlc to rlc set of the burst and the size
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu - drbKey[" << drbKey << "], insert RLC SDU of size " << rlcSduSize << endl;

        bsit->second.rlcPdu[rlcSno] = rlcSduSize;
    }
}

void PacketFlowObserverEnb::discardRlcPdu(DrbKey drbKey, unsigned int rlcSno, bool fromMac)
{
    auto cit = connectionMap_.find(drbKey);
    if (cit == connectionMap_.end()) {
        throw cRuntimeError("%s::discardRlcPdu - DRB %s not present. It must be initialized before", pfmType.c_str(), drbKey.str().c_str());
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor *desc = &cit->second;
    if (desc->rlcSdusPerPdu_.find(rlcSno) == desc->rlcSdusPerPdu_.end())
        return; // NR-SO discards carry rlcSno=0 (complete SDUs have no per-PDU SN); not tracked, skip

    // get the PDCP SDUs fragmented in this RLC PDU
    SequenceNumberSet pdcpSnoSet = desc->rlcSdusPerPdu_.find(rlcSno)->second;
    for (const auto& pdcpSno : pdcpSnoSet) {
        // find sdu -> rlc for this pdcp
        auto rit = desc->rlcPdusPerSdu_.find(pdcpSno);
        if (rit == desc->rlcPdusPerSdu_.end())
            throw cRuntimeError("%s::discardRlcPdu - PdcpStatus for PDCP sno [%d] with drbKey [%s] not present, this should not happen", pfmType.c_str(), pdcpSno, drbKey.str().c_str());

        // remove the RLC PDUs that contain a fragment of this pdcpSno
        rit->second.erase(rlcSno);

        // set this pdcp sdu as discarded, flag use in macPduArrive to not take into account this pdcp
        auto pit = desc->pdcpStatus_.find(pdcpSno);
        if (pit == desc->pdcpStatus_.end())
            throw cRuntimeError("%s::discardRlcPdu - PdcpStatus for PDCP sno [%d] already present, this should not happen", pfmType.c_str(), pdcpSno);

        if (fromMac)
            pit->second.discardedAtMac = true; // discarded rate stats also
        else
            pit->second.discardedAtRlc = true;

        // if the set is empty AND
        // the pdcp pdu has been completely encapsulated AND
        // a RLC referred to this PDCP has not been discarded at MAC (i.e. max num max NACKs) AND
        // a RLC referred to this PDCP has not been sent over the air
        // ---> all PDCP has been discarded at eNB before starting its transmission
        // count it in discarded stats
        // compliant with ETSI 136 314 at 4.1.5.1

        if (rit->second.empty() && pit->second.hasArrivedAll && !pit->second.discardedAtMac && !pit->second.sentOverTheAir) {
            EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::discardRlcPdu - drbKey[" << drbKey << "], discarded PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
            pktDiscardCounterPerUe_[desc->nodeId_].discarded += 1;
            pktDiscardCounterTotal_.discarded += 1;
        }
        // if the pdcp was entire and the set of rlc is empty, discard it
        if (rit->second.empty() && pit->second.hasArrivedAll) {
            desc->rlcPdusPerSdu_.erase(rit);
            //remove pdcp status
            desc->pdcpStatus_.erase(pit);
        }
    }
    removePdcpBurstRLC(desc, rlcSno, false);
    //remove discarded rlc pdu
    desc->rlcSdusPerPdu_.erase(rlcSno);
}

void PacketFlowObserverEnb::ensureMacPduMapping(MacNodeId peerId, const LteMacPdu *macPdu)
{
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if (len == 0)
        return; // BSR-only MAC PDU, nothing to track
    for (int i = 0; i < len; ++i) {
        // MAC's LCID maps 1:1 to the DRB ID, scoped by the HARQ peer (MAC-PDU SDUs
        // carry no tags: LteMacPdu::pushSdu clears them)
        DrbKey drbKey = DrbKey(peerId, DrbId(num(macPdu->getLcid(i))));
        auto cit = connectionMap_.find(drbKey);
        if (cit == connectionMap_.end())
            throw cRuntimeError("%s::ensureMacPduMapping - DRB %s not present", pfmType.c_str(), drbKey.str().c_str());

        StatusDescriptor *desc = &cit->second;
        if (desc->macSdusPerPdu_.find(macPduId) != desc->macSdusPerPdu_.end())
            continue; // already mapped (from a previous SDU with same DRB)

        for (int j = 0; j < len; ++j) {
            auto rlcPdu = macPdu->getSdu(j);
            auto rlcChunk = rlcPdu.peekAtFront<LteRlcDataPdu>();
            unsigned int rlcSno = observerRlcKey(rlcChunk);

            auto tit = desc->rlcSdusPerPdu_.find(rlcSno);
            if (tit == desc->rlcSdusPerPdu_.end()) {
                // NR-SO: build the per-SDU tracking on demand keyed by the PDCP SN
                // (= rlcSno for SO). LTE-FI PDUs must already be present.
                if (inet::dynamicPtrCast<const NrRlcUmDataPdu>(rlcChunk) != nullptr) {
                    auto pit = desc->pdcpStatus_.find(rlcSno);
                    if (pit == desc->pdcpStatus_.end())
                        continue; // PDCP SDU no longer tracked - skip
                    desc->rlcSdusPerPdu_[rlcSno].insert(rlcSno);
                    desc->rlcPdusPerSdu_[rlcSno].insert(rlcSno);
                    pit->second.hasArrivedAll = true;
                    tit = desc->rlcSdusPerPdu_.find(rlcSno);
                }
                else
                    throw cRuntimeError("%s::ensureMacPduMapping - RLC PDU ID %d not present in the status descriptor of drbKey %s", pfmType.c_str(), rlcSno, drbKey.str().c_str());
            }

            desc->macSdusPerPdu_[macPduId].insert(rlcSno);

            SequenceNumberSet pdcpSet = tit->second;
            for (const auto& pdcpSno : pdcpSet) {
                auto sdit = desc->pdcpStatus_.find(pdcpSno);
                if (sdit != desc->pdcpStatus_.end())
                    sdit->second.sentOverTheAir = true;
            }
        }
    }
}

void PacketFlowObserverEnb::macPduArrived(MacNodeId peerId, const LteMacPdu *macPdu)
{
    ensureMacPduMapping(peerId, macPdu);

    /*
     * retrieve the macPduId and the DRB ID (from MAC's LCID, which maps 1:1)
     */
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if (len == 0)
        return; // BSR-only MAC PDU, nothing to track
    for (int i = 0; i < len; ++i) {
        auto rlcPdu = macPdu->getSdu(i);
        DrbKey drbKey = DrbKey(peerId, DrbId(num(macPdu->getLcid(i))));  // MAC's LCID maps 1:1 to DRB ID
        auto cit = connectionMap_.find(drbKey);
        if (cit == connectionMap_.end())
            throw cRuntimeError("%s::macPduArrived - DRB %s not present. It must be initialized before", pfmType.c_str(), drbKey.str().c_str());

        // get the descriptor for this connection
        StatusDescriptor *desc = &cit->second;
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - MAC PDU " << macPduId << " of drbKey " << drbKey << " arrived." << endl;
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - Get MAC PDU ID [" << macPduId << "], which contains:" << endl;

        // === STEP 1 ==================================================== //
        // === recover the set of RLC PDU SN from the above MAC PDU ID === //

        if (macPduId == 0) {
            EV << NOW << " " << pfmType << "::insertMacPdu - The process does not contain entire SDUs" << endl;
            return;
        }

        auto mit = desc->macSdusPerPdu_.find(macPduId);
        if (mit == desc->macSdusPerPdu_.end())
            continue; // no tracked SDUs from this MAC PDU on this DRB (e.g. untracked SO segments)
        SequenceNumberSet rlcSnoSet = mit->second;

        // === STEP 2 ========================================================== //
        // === for each RLC PDU SN, recover the set of RLC SDU (PDCP PDU) SN === //

        for (const auto& rlcPduSno : rlcSnoSet) {
            EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - --> RLC PDU [" << rlcPduSno << "], which contains:" << endl;

            auto nit = desc->rlcSdusPerPdu_.find(rlcPduSno);
            if (nit == desc->rlcSdusPerPdu_.end())
                continue; // SO: entry already erased on completion - skip
            SequenceNumberSet pdcpSnoSet = nit->second;

            // === STEP 3 ============================================================================ //
            // === (PDCP PDU) SN, recover the set of RLC PDU where it is included,                 === //
            // === remove the above RLC PDU SN. If the set becomes empty, compute the delay if     === //
            // === all PDCP PDU fragments have been transmitted                                    === //

            for (const auto& pdcpPduSno : pdcpSnoSet) {
                EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - ----> PDCP PDU [" << pdcpPduSno << "]" << endl;

                auto oit = desc->rlcPdusPerSdu_.find(pdcpPduSno);
                if (oit == desc->rlcPdusPerSdu_.end())
                    continue; // SO: PDCP SDU already completed/erased - skip

                auto kt = oit->second.find(rlcPduSno);
                if (kt == oit->second.end())
                    continue; // SO: already removed - skip

                // the RLC PDU has been sent, so erase it from the set
                oit->second.erase(kt);

                auto pit = desc->pdcpStatus_.find(pdcpPduSno);
                if (pit == desc->pdcpStatus_.end())
                    continue; // SO: PDCP status already erased on completion - skip

                // check whether the set is now empty
                if (desc->rlcPdusPerSdu_[pdcpPduSno].empty()) {
                    if (pit->second.entryTime == 0)
                        continue; // SO: no entry time recorded for this SDU - skip

                    if (pit->second.hasArrivedAll && !pit->second.discardedAtRlc && !pit->second.discardedAtMac) {
                        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - ----> PDCP PDU [" << pdcpPduSno << "] has been completely sent, remove from PDCP buffer" << endl;

                        auto dit = pdcpDelay_.find(desc->nodeId_);
                        if (dit == pdcpDelay_.end()) {
                            pdcpDelay_[desc->nodeId_] = { 0, 0 }; // create new structure
                            dit = pdcpDelay_.find(desc->nodeId_);
                        }

                        double time = (simTime() - pit->second.entryTime).dbl();

                        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - PDCP PDU " << pdcpPduSno << " of drbKey " << drbKey << " acknowledged. Delay time: " << time << "s" << endl;

                        dit->second.time += (simTime() - pit->second.entryTime);

                        dit->second.pktCount += 1;

                        // remove pdcp status
                        desc->pdcpStatus_.erase(pit);
                        oit->second.clear();
                        desc->rlcPdusPerSdu_.erase(oit); // erase PDCP PDU SN
                    }
                }
            }
            desc->rlcSdusPerPdu_.erase(nit); // erase RLC PDU SN
            removePdcpBurstRLC(desc, rlcPduSno, true); // check if the pdcp is part of a burst
        }

        desc->macSdusPerPdu_.erase(mit); // erase MAC PDU ID
    }
}

void PacketFlowObserverEnb::discardMacPdu(MacNodeId peerId, const LteMacPdu *macPdu)
{
    ensureMacPduMapping(peerId, macPdu);

    /*
     * retrieve the macPduId and the DRB ID
     */
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if (len == 0)
        return; // BSR-only MAC PDU, nothing to track
    for (int i = 0; i < len; ++i) {
        auto rlcPdu = macPdu->getSdu(i);
        DrbKey drbKey = DrbKey(peerId, DrbId(num(macPdu->getLcid(i))));  // MAC's LCID maps 1:1 to DRB ID

        auto cit = connectionMap_.find(drbKey);
        if (cit == connectionMap_.end()) {
            throw cRuntimeError("%s::discardMacPdu - DRB %s not present. It must be initialized before", pfmType.c_str(), drbKey.str().c_str());
            return;
        }

        StatusDescriptor *desc = &cit->second;
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::discardMacPdu - MAC PDU " << macPduId << " of drbKey " << drbKey << " arrived." << endl;

        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::discardMacPdu - Get MAC PDU ID [" << macPduId << "], which contains:" << endl;

        if (macPduId == 0) {
            EV << NOW << " " << pfmType << "::discardMacPdu - The process does not contain entire SDUs" << endl;
            return;
        }

        auto mit = desc->macSdusPerPdu_.find(macPduId);
        if (mit == desc->macSdusPerPdu_.end())
            continue; // no tracked SDUs from this MAC PDU on this DRB (e.g. untracked SO segments)
        SequenceNumberSet rlcSnoSet = mit->second;

        for (const auto& sn : rlcSnoSet) {
            discardRlcPdu(drbKey, sn, true);
        }

        desc->macSdusPerPdu_.erase(mit); // erase MAC PDU ID
    }
}

void PacketFlowObserverEnb::removePdcpBurstRLC(StatusDescriptor *desc, unsigned int rlcSno, bool ack)
{
    // check end of a burst
    // for each burst_id, we have to search if the relative set has the RLC
    // it has been assumed that the bursts are quickly emptied so the operation
    // is not computationally heavy
    // the other solution is to create <rlcPdu, burst_id> map
    for (auto &[burstId, burstStatus] : desc->burstStatus_) {
        auto rlcpit = burstStatus.rlcPdu.find(rlcSno);
        if (rlcpit != burstStatus.rlcPdu.end()) {
            if (ack) {
                // if arrived, sum it to the thpVolDl
                burstStatus.burstSize += rlcpit->second;
            }
            burstStatus.rlcPdu.erase(rlcpit);
            if (burstStatus.rlcPdu.empty() && burstStatus.isCompleted) {
                // compute throughput
                auto tit = pdcpThroughput_.find(desc->nodeId_);
                if (tit == pdcpThroughput_.end()) {
                    pdcpThroughput_.insert({desc->nodeId_, { 0, 0 }});
                    tit = pdcpThroughput_.find(desc->nodeId_);
                }
                tit->second.pktSizeCount += burstStatus.burstSize; //*8 --> bits
                tit->second.time += (simTime() - burstStatus.startBurstTransmission);
                double tp = ((double)burstStatus.burstSize) / (simTime() - burstStatus.startBurstTransmission).dbl();

                EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::removePdcpBurst Burst " << burstId << " length " << simTime() - burstStatus.startBurstTransmission << "s, with size " << burstStatus.burstSize << "B -> tput: " << tp << " B/s" << endl;
                desc->burstStatus_.erase(burstId); // remove emptied burst
            }
            break;
        }
    }
}

void PacketFlowObserverEnb::resetDiscardCounterPerUe(MacNodeId id)
{
    std::map<MacNodeId, DiscardedPkts>::iterator it = pktDiscardCounterPerUe_.find(id);
    if (it == pktDiscardCounterPerUe_.end()) {
        // maybe it is possible? think about it
        // yes
        return;
    }
    it->second = { 0, 0 };
}

double PacketFlowObserverEnb::getDiscardedPktPerUe(MacNodeId id)
{
    std::map<MacNodeId, DiscardedPkts>::iterator it = pktDiscardCounterPerUe_.find(id);
    if (it == pktDiscardCounterPerUe_.end()) {
        // maybe it is possible? think about it
        // yes, if I do not discard anything
        return 0;
    }
    return ((double)it->second.discarded * 1000000) / it->second.total;
}

double PacketFlowObserverEnb::getDiscardedPkt()
{
    if (pktDiscardCounterTotal_.total == 0)
        return 0.0;
    return ((double)pktDiscardCounterTotal_.discarded * 1000000) / pktDiscardCounterTotal_.total;
}

void PacketFlowObserverEnb::grantSent(MacNodeId nodeId, unsigned int grantId)
{
    Grant grant = { grantId, simTime() };
    for (const auto& grant : ulGrants_[nodeId]) {
        if (grant.grantId == grantId)
            throw cRuntimeError("%s::grantSent - grant [%d] for nodeId [%hu] already present", pfmType.c_str(), grantId, num(nodeId));
    }
    EV_FATAL << NOW << " " << pfmType << "::grantSent - Added grant " << grantId << " for nodeId " << nodeId << endl;
    ulGrants_[nodeId].push_back(grant);
}

void PacketFlowObserverEnb::ulMacPduArrived(MacNodeId nodeId, unsigned int grantId)
{
    for (auto it = ulGrants_[nodeId].begin(); it != ulGrants_[nodeId].end(); ) {
        if (it->grantId == grantId) {

            simtime_t time = simTime() - it->sendTimestamp;
            EV_FATAL << NOW << " " << pfmType << "::ulMacPduArrived - TB received from nodeId " << nodeId << " related to grantId " << grantId << " after " << time.dbl() << " seconds" << endl;
            ULPktDelay_[nodeId].pktCount++;
            ULPktDelay_[nodeId].time += time;

            it = ulGrants_[nodeId].erase(it);
            return; // it is present only one grant with this grantId for this nodeId
        } else {
            ++it;
        }
    }
    // this may occur after a handover, when the grant was tracked by the old gNB
    EV_WARN << NOW << " " << pfmType << "::ulMacPduArrived - grant [" << grantId << "] for nodeId [" << nodeId << "] not present (possibly after handover)" << endl;
}

double PacketFlowObserverEnb::getDelayStatsPerUe(MacNodeId id)
{
    auto it = pdcpDelay_.find(id);
    if (it == pdcpDelay_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::getDelayStatsPerUe - Delay Stats for Node Id " << id << " not present." << endl;
        return 0;
    }

    if (it->second.pktCount == 0)
        return 0;
    EV_FATAL << NOW << " " << pfmType << "::getDelayStatsPerUe - Delay Stats for Node Id " << id << " total time: " << (it->second.time.dbl()) * 1000 << "ms, pktCount: " << it->second.pktCount << endl;
    double totalMs = (it->second.time.dbl()) * 1000; // ms
    double delayMean = totalMs / it->second.pktCount;
    return delayMean;
}

double PacketFlowObserverEnb::getUlDelayStatsPerUe(MacNodeId id)
{
    auto it = ULPktDelay_.find(id);
    if (it == ULPktDelay_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::getUlDelayStatsPerUe - Delay Stats for Node Id " << id << " not present." << endl;
        return 0;
    }

    if (it->second.pktCount == 0)
        return 0;
    EV_FATAL << NOW << " " << pfmType << "::getUlDelayStatsPerUe - Delay Stats for Node Id " << id << " total time: " << (it->second.time.dbl()) * 1000 << "ms, pktCount: " << it->second.pktCount << endl;
    double totalMs = (it->second.time.dbl()) * 1000; // ms
    double delayMean = totalMs / it->second.pktCount;
    return delayMean;
}

void PacketFlowObserverEnb::resetDelayCounterPerUe(MacNodeId id)
{
    auto it = pdcpDelay_.find(id);
    if (it == pdcpDelay_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::resetDelayCounterPerUe - Delay Stats for Node Id " << id << " not present." << endl;
        return;
    }

    it->second = { 0, 0 };
}

void PacketFlowObserverEnb::resetUlDelayCounterPerUe(MacNodeId id)
{
    auto it = ULPktDelay_.find(id);
    if (it == ULPktDelay_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::resetUlDelayCounterPerUe - Ul Delay Stats for Node Id " << id << " not present." << endl;
        return;
    }

    it->second = { 0, 0 };
}

double PacketFlowObserverEnb::getThroughputStatsPerUe(MacNodeId id)
{
    auto it = pdcpThroughput_.find(id);
    if (it == pdcpThroughput_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::getThroughputStatsPerUe - Throughput Stats for Node Id " << id << " not present." << endl;
        return 0.0;
    }

    double time = it->second.time.dbl(); // seconds
    if (time == 0) {
        return 0.0;
    }
    double throughput = ((double)(it->second.pktSizeCount)) / time;
    EV_FATAL << NOW << " " << pfmType << "::getThroughputStatsPerUe - Throughput Stats for Node Id " << id << " " << throughput << endl;
    return throughput;
}

void PacketFlowObserverEnb::resetThroughputCounterPerUe(MacNodeId id)
{
    auto it = pdcpThroughput_.find(id);
    if (it == pdcpThroughput_.end()) {
        EV_FATAL << NOW << " " << pfmType << "::resetThroughputCounterPerUe - Throughput Stats for Node Id " << id << " not present." << endl;
        return;
    }
    it->second = { 0, 0 };
}

void PacketFlowObserverEnb::deleteUe(MacNodeId nodeId)
{
    /* It has to be deleted:
     * all structures with MacNodeId id
     * all drbIds belonging to MacNodeId id
     */
    for (auto it = connectionMap_.begin(); it != connectionMap_.end(); ) {
        if (it->second.nodeId_ == nodeId) {
            it = connectionMap_.erase(it);
        }
        else {
            ++it;
        }
    }

    packetLossRate_.erase(nodeId);
    pdcpDelay_.erase(nodeId);
    pdcpThroughput_.erase(nodeId);
    pktDiscardCounterPerUe_.erase(nodeId);
    sduDataVolume_.erase(nodeId);
}

double PacketFlowObserverEnb::getPdpcLossRate()
{
    unsigned int lossPackets = 0; // Dloss
    unsigned int totalPackets = 0; // N (it also counts missing pdcp sno)

    for (const auto& ue : packetLossRate_) {
        lossPackets += ue.second.totalLossPdcp;
        totalPackets += ue.second.totalPdcpSno;
    }

    if (totalPackets == 0) return 0;

    return ((double)lossPackets * 1000000) / totalPackets;
}

double PacketFlowObserverEnb::getPdpcLossRatePerUe(MacNodeId id)
{
    auto it = packetLossRate_.find(id);
    if (it != packetLossRate_.end()) {
        return ((double)it->second.totalLossPdcp * 1000000) / it->second.totalPdcpSno;
    }
    else {
        return 0;
    }
}

void PacketFlowObserverEnb::resetPdpcLossRatePerUe(MacNodeId id)
{
    auto it = packetLossRate_.find(id);
    if (it != packetLossRate_.end()) {
        it->second.reset();
    }
}

void PacketFlowObserverEnb::resetPdpcLossRates()
{
    for (auto& ue : packetLossRate_) {
        ue.second.reset();
    }
}

uint64_t PacketFlowObserverEnb::getDataVolume(MacNodeId nodeId, Direction dir)
{
    auto node = sduDataVolume_.find(nodeId);
    if (node == sduDataVolume_.end())
        return 0;
    if (dir == DL)
        return node->second.dlBits;
    else if (dir == UL)
        return node->second.ulBits;
    else
        throw cRuntimeError("PacketFlowObserverEnb::getDataVolume - Wrong direction");
}

void PacketFlowObserverEnb::resetDataVolume(MacNodeId nodeId, Direction dir)
{
    auto node = sduDataVolume_.find(nodeId);
    if (node == sduDataVolume_.end())
        return;
    if (dir == DL)
        node->second.dlBits = 0;
    else if (dir == UL)
        node->second.ulBits = 0;
    else
        throw cRuntimeError("PacketFlowObserverEnb::getDataVolume - Wrong direction");
}

void PacketFlowObserverEnb::resetDataVolume(MacNodeId nodeId)
{
    auto node = sduDataVolume_.find(nodeId);
    if (node == sduDataVolume_.end())
        return;
    node->second.dlBits = 0;
    node->second.ulBits = 0;
}

} //namespace
