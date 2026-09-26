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

#include "PacketFlowObserverUe.h"
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

Define_Module(PacketFlowObserverUe);


void PacketFlowObserverUe::initialize(int stage)
{
    PacketFlowObserverBase::initialize(stage);

    if (stage == inet::INITSTAGE_LOCAL) {
        WATCH(connectionMap_);
        WATCH(pdcpDelay);
    }
}

bool PacketFlowObserverUe::hasDrbId(DrbKey drbKey)
{
    return connectionMap_.find(drbKey) != connectionMap_.end();
}

void PacketFlowObserverUe::initDrbId(DrbKey drbKey, MacNodeId nodeId)
{
    if (connectionMap_.find(drbKey) != connectionMap_.end())
        throw cRuntimeError("%s::initDrbId - DRB %s already present", pfmType.c_str(), drbKey.str().c_str());

    // init new descriptor
    StatusDescriptor newDesc;
    newDesc.nodeId_ = nodeId;
    newDesc.pdcpStatus_.clear();
    newDesc.rlcPdusPerSdu_.clear();
    newDesc.rlcSdusPerPdu_.clear();
    newDesc.macSdusPerPdu_.clear();
    newDesc.macPduPerProcess_.resize(harqProcesses_, 0);

    connectionMap_[drbKey] = newDesc;
    EV_FATAL << NOW << "node id " << nodeId << " " << pfmType << "::initDrbId - initialized drbKey " << drbKey << endl;
}

void PacketFlowObserverUe::clearDrbId(DrbKey drbKey)
{
    ConnectionMap::iterator it = connectionMap_.find(drbKey);
    if (it == connectionMap_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::clearDrbId - DRB ID " << drbKey << " not present." << endl;
        return;
    }

    StatusDescriptor *desc = &it->second;
    desc->pdcpStatus_.clear();
    desc->rlcPdusPerSdu_.clear();
    desc->rlcSdusPerPdu_.clear();
    desc->macSdusPerPdu_.clear();

    for (int i = 0; i < harqProcesses_; i++)
        desc->macPduPerProcess_[i] = 0;

    EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::clearDrbId - cleared data structures for drbKey " << drbKey << endl;
}

void PacketFlowObserverUe::clearAllDrbIds()
{
    connectionMap_.clear();
    EV_FATAL << NOW << " " << pfmType << "::clearAllDrbIds - cleared data structures for all drbIds " << endl;
}

void PacketFlowObserverUe::clearStats()
{
    clearAllDrbIds();
    resetDelayCounter();
    resetDiscardCounter();
}

void PacketFlowObserverUe::initPdcpStatus(StatusDescriptor *desc, unsigned int pdcp, unsigned int pdcpSize, simtime_t& arrivalTime)

{
    // if pdcpStatus_ already present, error
    std::map<unsigned int, PdcpStatus>::iterator it = desc->pdcpStatus_.find(pdcp);
    if (it != desc->pdcpStatus_.end())
        throw cRuntimeError("%s::initPdcpStatus - PdcpStatus for PDCP sno [%d] already present, this should not happen", pfmType.c_str(), pdcp);

    PdcpStatus newpdcpStatus;

    newpdcpStatus.entryTime = arrivalTime;
    newpdcpStatus.discardedAtMac = false;
    newpdcpStatus.discardedAtRlc = false;
    newpdcpStatus.hasArrivedAll = false;
    newpdcpStatus.sentOverTheAir = false;
    newpdcpStatus.pdcpSduSize = pdcpSize;

    desc->pdcpStatus_[pdcp] = newpdcpStatus;
}

void PacketFlowObserverUe::insertPdcpSdu(inet::Packet *pdcpPkt)
{
    EV << pfmType << "::insertPdcpSdu" << endl;
    auto lteInfo = pdcpPkt->getTagForUpdate<FlowControlInfo>();
    DrbKey drbKey = ctrlInfoToTxDrbKey(lteInfo.get());

    if (connectionMap_.find(drbKey) == connectionMap_.end())
        initDrbId(drbKey, lteInfo->getSourceId());

    // Extract sequence number from PDCP header
    auto pdcpHeader = pdcpPkt->peekAtFront<LtePdcpHeader>();
    unsigned int pdcpSno = pdcpHeader->getSequenceNumber();
    int64_t pdcpSize = pdcpPkt->getByteLength();
    simtime_t arrivalTime = simTime();

    ConnectionMap::iterator cit = connectionMap_.find(drbKey);
    if (cit == connectionMap_.end()) {
        // this may occur after a handover (when data structures are cleared),
        // or when the observer receives signals from the other RLC stack in DC scenarios
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor *desc = &cit->second;

    initPdcpStatus(desc, pdcpSno, pdcpSize, arrivalTime);
    pktDiscardCounterTotal_.total += 1;

    EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::insertPdcpSdu - PDCP status for PDCP PDU SN " << pdcpSno << " added. DRB ID " << drbKey << endl;
}

void PacketFlowObserverUe::insertRlcPdu(DrbKey drbKey, const LteRlcUmDataPdu *rlcPdu, RlcBurstStatus status)
{
    ConnectionMap::iterator cit = connectionMap_.find(drbKey);
    if (cit == connectionMap_.end()) {
        // this may occur after a handover (when data structures are cleared),
        // or when the observer receives signals from the other RLC stack in DC scenarios
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor *desc = &cit->second;

    unsigned int rlcSno = rlcPdu->getPduSequenceNumber();

    if (desc->rlcSdusPerPdu_.find(rlcSno) != desc->rlcSdusPerPdu_.end())
        throw cRuntimeError("%s::insertRlcPdu - RLC PDU SN %d already present for DRB %s", pfmType.c_str(), rlcSno, drbKey.str().c_str());
    EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::insertRlcPdu - DRB ID " << drbKey << endl;

    FramingInfo fi = rlcPdu->getFramingInfo();
    for (size_t idx = 0; idx < rlcPdu->getNumSdu(); ++idx) {
        auto sduPacket = rlcPdu->getSdu(idx);
        auto pdcpTag = sduPacket->getTag<PdcpTrackingTag>();
        unsigned int pdcpSno = pdcpTag->getPdcpSequenceNumber();
        size_t pdcpPduLength = rlcPdu->getSduSize(idx); // TODO fix with size of the chunk!!

        EV << pfmType << "::insertRlcPdu - pdcpSdu " << pdcpSno << " with length: " << pdcpPduLength << " bytes" << endl;

        // store the RLC SDUs (PDCP PDUs) included in the RLC PDU
        desc->rlcSdusPerPdu_[rlcSno].insert(pdcpSno);

        // now store the inverse association, i.e., for each RLC SDU, record in which RLC PDU is included
        desc->rlcPdusPerSdu_[pdcpSno].insert(rlcSno);

        // set the PDCP entry time
        std::map<unsigned int, PdcpStatus>::iterator pit = desc->pdcpStatus_.find(pdcpSno);
        if (pit == desc->pdcpStatus_.end())
            throw cRuntimeError("%s::insertRlcPdu - PdcpStatus for PDCP sno [%d] not present, this should not happen", pfmType.c_str(), pdcpSno);

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
        else {
            pit->second.hasArrivedAll = true;
        }

        EV_FATAL << NOW << " " << pfmType << "::insertRlcPdu - drbKey[" << drbKey << "], insert PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
    }
    EV << "size:" << desc->rlcSdusPerPdu_[rlcSno].size() << endl;
}

void PacketFlowObserverUe::discardRlcPdu(DrbKey drbKey, unsigned int rlcSno, bool fromMac)
{
    ConnectionMap::iterator cit = connectionMap_.find(drbKey);
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
        auto rit = desc->rlcPdusPerSdu_.find(pdcpSno);
        if (rit == desc->rlcPdusPerSdu_.end())
            throw cRuntimeError("%s::discardRlcPdu - PdcpStatus for PDCP sno [%d] with drbKey [%s] not present, this should not happen", pfmType.c_str(), pdcpSno, drbKey.str().c_str());

        // remove the RLC PDUs that contains a fragment of this pdcpSno
        rit->second.erase(rlcSno);

        // set this pdcp sdu that a RLC has been discarded, i.e the arrived pdcp will be not entire.
        auto pit = desc->pdcpStatus_.find(pdcpSno);
        if (pit == desc->pdcpStatus_.end())
            throw cRuntimeError("%s::discardRlcPdu - PdcpStatus for PDCP sno [%d] already present, this should not happen", pfmType.c_str(), pdcpSno);

        if (fromMac)
            pit->second.discardedAtMac = true;
        else
            pit->second.discardedAtRlc = true;

        // if the set is empty AND
        // the pdcp pdu has been encapsulated all AND
        // a RLC referred to this PDCP has not been discarded at MAC (i.e 4 NACKs) AND
        // a RLC referred to this PDCP has not been sent over the air
        // ---> all PDCP has been discarded at eNB before start its transmission
        // compliant with ETSI 136 314 at 4.1.5.1

        if (rit->second.empty() && pit->second.hasArrivedAll && !pit->second.discardedAtMac && !pit->second.sentOverTheAir) {
            EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::discardRlcPdu - drbKey[" << drbKey << "], discarded PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
            pktDiscardCounterTotal_.discarded += 1;
        }
        // if the pdcp was entire and the set of rlc is empty, discard it
        if (rit->second.empty() && pit->second.hasArrivedAll) {
            desc->rlcPdusPerSdu_.erase(pdcpSno);
        }
    }
    // remove discarded rlc pdu
    desc->rlcSdusPerPdu_.erase(rlcSno);
}

void PacketFlowObserverUe::ensureMacPduMapping(MacNodeId peerId, const LteMacPdu *macPdu)
{
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if (len == 0)
        return; // BSR-only MAC PDU, nothing to track
    for (int i = 0; i < len; ++i) {
        // MAC's LCID maps 1:1 to the DRB ID, scoped by the HARQ peer (MAC-PDU SDUs
        // carry no tags: LteMacPdu::pushSdu clears them)
        DrbKey drbKey = DrbKey(peerId, DrbId(num(macPdu->getLcid(i))));

        ConnectionMap::iterator cit = connectionMap_.find(drbKey);
        if (cit == connectionMap_.end())
            throw cRuntimeError("%s::ensureMacPduMapping - DRB %s not present", pfmType.c_str(), drbKey.str().c_str());

        StatusDescriptor *desc = &cit->second;
        if (desc->macSdusPerPdu_.find(macPduId) != desc->macSdusPerPdu_.end())
            continue; // already mapped

        auto macSdu = macPdu->getSdu(i).peekAtFront<LteRlcDataPdu>();
        unsigned int rlcSno = observerRlcKey(macSdu);

        auto tit = desc->rlcSdusPerPdu_.find(rlcSno);
        if (tit == desc->rlcSdusPerPdu_.end()) {
            // NR-SO PDUs carry no rlcPduCreated signal; build the per-SDU tracking on
            // demand keyed by the PDCP SN (= rlcSno for SO). LTE-FI PDUs must already
            // be present (inserted via rlcPduCreated), so a miss is a real error.
            if (inet::dynamicPtrCast<const NrRlcUmDataPdu>(macSdu) != nullptr) {
                auto pit = desc->pdcpStatus_.find(rlcSno);
                if (pit == desc->pdcpStatus_.end())
                    continue; // PDCP SDU no longer tracked (delivered/discarded) - skip
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
        for (auto pdcpSno : pdcpSet) {
            auto sdit = desc->pdcpStatus_.find(pdcpSno);
            if (sdit != desc->pdcpStatus_.end())
                sdit->second.sentOverTheAir = true;
        }
    }
}

void PacketFlowObserverUe::macPduArrived(MacNodeId peerId, const LteMacPdu *macPdu)
{
    ensureMacPduMapping(peerId, macPdu);

    EV << pfmType << "::macPduArrived" << endl;
    /*
     * retrieve the macPduId and the Lcid
     */
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if (len == 0)
        return; // BSR-only MAC PDU, nothing to track
    for (int i = 0; i < len; ++i) {
        auto rlcPdu = macPdu->getSdu(i);
        DrbKey drbKey = DrbKey(peerId, DrbId(num(macPdu->getLcid(i))));  // MAC's LCID maps 1:1 to DRB ID

        ConnectionMap::iterator cit = connectionMap_.find(drbKey);
        if (cit == connectionMap_.end()) {
            throw cRuntimeError("%s::macPduArrived - DRB %s not present. It must be initialized before", pfmType.c_str(), drbKey.str().c_str());
        }

        // get the descriptor for this connection
        StatusDescriptor *desc = &cit->second;

        EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::macPduArrived - Get MAC PDU ID [" << macPduId << "], which contains:" << endl;
        EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::macPduArrived - MAC PDU " << macPduId << " of drbKey " << drbKey << " arrived." << endl;

        // === STEP 1 ==================================================== //
        // === recover the set of RLC PDU SN from the above MAC PDU ID === //
        if (macPduId == 0) {
            EV << NOW << " " << pfmType << "::insertMacPdu - The process does not contain entire SDUs" << endl;
            return;
        }

        std::map<unsigned int, SequenceNumberSet>::iterator mit = desc->macSdusPerPdu_.find(macPduId);
        if (mit == desc->macSdusPerPdu_.end())
            continue; // no tracked SDUs from this MAC PDU on this DRB (e.g. untracked SO segments)
        SequenceNumberSet rlcSnoSet = mit->second;

        auto macSdu = rlcPdu.peekAtFront<LteRlcDataPdu>();
        unsigned int rlcSno = observerRlcKey(macSdu);

        if (rlcSnoSet.find(rlcSno) == rlcSnoSet.end())
            continue; // SO: this PDCP SN was already completed/erased - skip

        // === STEP 2 ========================================================== //
        // === for each RLC PDU SN, recover the set of RLC SDU (PDCP PDU) SN === //

        for (auto rlcPduSno : rlcSnoSet) {
            // for each RLC PDU
            EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::macPduArrived - --> RLC PDU [" << rlcPduSno << "], which contains:" << endl;

            std::map<unsigned int, SequenceNumberSet>::iterator nit = desc->rlcSdusPerPdu_.find(rlcPduSno);
            if (nit == desc->rlcSdusPerPdu_.end())
                continue; // SO: entry already erased on completion - skip
            SequenceNumberSet pdcpSnoSet = nit->second;

            // === STEP 3 ============================================================================ //
            // === Since an RLC SDU may be fragmented in more than one RLC PDU, thus it must be     === //
            // === retransmitted only if all fragments have been transmitted.                      === //
            // === For each RLC SDU (PDCP PDU) SN, recover the set of RLC PDUs where it is included,=== //
            // === remove the above RLC PDU SN. If the set becomes empty, compute the delay if     === //
            // === all PDCP PDU fragments have been transmitted                                     === //

            for (auto pdcpPduSno : pdcpSnoSet) {
                // for each RLC SDU (PDCP PDU), get the set of RLC PDUs where it is included
                EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::macPduArrived - ----> PDCP PDU [" << pdcpPduSno << "]" << endl;

                std::map<unsigned int, SequenceNumberSet>::iterator oit = desc->rlcPdusPerSdu_.find(pdcpPduSno);
                if (oit == desc->rlcPdusPerSdu_.end())
                    continue; // SO: PDCP SDU already completed/erased - skip

                // oit->second is the set of RLC PDU in which the PDCP PDU is contained

                // the RLC PDU SN must be present in the set
                if (oit->second.find(rlcPduSno) == oit->second.end())
                    continue; // SO: already removed - skip

                // the RLC PDU has been sent, so erase it from the set
                oit->second.erase(rlcPduSno);

                std::map<unsigned int, PdcpStatus>::iterator pit = desc->pdcpStatus_.find(pdcpPduSno);
                if (pit == desc->pdcpStatus_.end())
                    continue; // SO: PDCP status already erased on completion - skip

                // check whether the set is now empty
                if (desc->rlcPdusPerSdu_[pdcpPduSno].empty()) {

                    // set the time for pdcpPduSno
                    if (pit->second.hasArrivedAll && !pit->second.discardedAtRlc && !pit->second.discardedAtMac) { // the whole current pdcp seqNum has been received
                        EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::macPduArrived - ----> PDCP PDU [" << pdcpPduSno << "] has been completely sent, remove from PDCP buffer" << endl;

                        double time = (simTime() - pit->second.entryTime).dbl();
                        pdcpDelay.time += time;
                        pdcpDelay.pktCount += 1;

                        EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::macPduArrived - PDCP PDU " << pdcpPduSno << " of drbKey " << drbKey << " acknowledged. Delay time: " << time << "s" << endl;

                        // remove pdcp status
                        oit->second.clear();
                        desc->rlcPdusPerSdu_.erase(oit); // erase PDCP PDU SN
                    }
                }
            }
        }

        mit->second.clear();
        desc->macSdusPerPdu_.erase(mit); // erase MAC PDU ID
    }
}

void PacketFlowObserverUe::discardMacPdu(MacNodeId peerId, const LteMacPdu *macPdu)
{
    ensureMacPduMapping(peerId, macPdu);

    /*
     * retrieve the macPduId and the Lcid
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

        // get the descriptor for this connection
        StatusDescriptor *desc = &cit->second;

        EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::discardMacPdu - Get MAC PDU ID [" << macPduId << "], which contains:" << endl;
        EV_FATAL << NOW << "node id " << num(desc->nodeId_) - 1025 << " " << pfmType << "::discardMacPdu - MAC PDU " << macPduId << " of drbKey " << drbKey << " arrived." << endl;

        // === STEP 1 ==================================================== //
        // === recover the set of RLC PDU SN from the above MAC PDU ID === //

        if (macPduId == 0) {
            EV << NOW << " " << pfmType << "::discardMacPdu - The process does not contain entire SDUs" << endl;
            return;
        }

        auto mit = desc->macSdusPerPdu_.find(macPduId);
        if (mit == desc->macSdusPerPdu_.end())
            continue; // no tracked SDUs from this MAC PDU on this DRB (e.g. untracked SO segments)
        SequenceNumberSet rlcSnoSet = mit->second;

        auto macSdu = rlcPdu.peekAtFront<LteRlcDataPdu>();
        unsigned int rlcSno = observerRlcKey(macSdu);

        if (rlcSnoSet.find(rlcSno) == rlcSnoSet.end())
            continue; // SO: this PDCP SN was already completed/erased - skip

        // === STEP 2 ========================================================== //
        // === for each RLC PDU SN, recover the set of RLC SDU (PDCP PDU) SN === //

        for (const auto& rlcSno : rlcSnoSet) {
            discardRlcPdu(drbKey, rlcSno, true);
        }

        mit->second.clear();
        desc->macSdusPerPdu_.erase(mit); // erase MAC PDU ID
    }
}

DiscardedPkts PacketFlowObserverUe::getDiscardedPkt()
{
    return pktDiscardCounterTotal_;
}

double PacketFlowObserverUe::getDelayStats()
{
    if (pdcpDelay.pktCount == 0)
        return 0;
    EV_FATAL << NOW << " " << pfmType << "::getDelayStats- Delay Stats total time: " << pdcpDelay.time.dbl() * 1000 << " pckcount: " << pdcpDelay.pktCount << endl;

    return (pdcpDelay.time.dbl() * 1000) / pdcpDelay.pktCount;
}

void PacketFlowObserverUe::resetDelayCounter()
{
    pdcpDelay = { 0, 0 };
}

} //namespace
