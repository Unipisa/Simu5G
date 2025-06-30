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

#include "PacketFlowManagerEnb.h"
#include "stack/mac/LteMacBase.h"
#include "stack/pdcp_rrc/LtePdcpRrc.h"
#include "stack/rlc/LteRlcDefs.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/mac/packet/LteMacPdu.h"
#include "common/LteCommon.h"

#include "common/LteControlInfo.h"
#include <sstream>

namespace simu5g {

Define_Module(PacketFlowManagerEnb);



void PacketFlowManagerEnb::initialize(int stage)
{
    if (stage == 1) {
        PacketFlowManagerBase::initialize(stage);
        if (headerCompressedSize_ == -1)
            headerCompressedSize_ = 0;

        timesUe_.setName("delay");
    }
}

bool PacketFlowManagerEnb::hasLcid(LogicalCid lcid)
{
    return connectionMap_.find(lcid) != connectionMap_.end();
}

void PacketFlowManagerEnb::initLcid(LogicalCid lcid, MacNodeId nodeId)
{
    if (connectionMap_.find(lcid) != connectionMap_.end())
        throw cRuntimeError("%s::initLcid - Logical CID %d already present. Aborting", pfmType.c_str(), lcid);

    // init new descriptor
    StatusDescriptor newDesc;
    newDesc.nodeId_ = nodeId;
    newDesc.burstId_ = 0;
    newDesc.burstState_ = false;

    BurstStatus newBurstStatus;
    newBurstStatus.burstSize = 0;
    newBurstStatus.startBurstTransmission = -1;

    connectionMap_[lcid] = newDesc;
    EV_FATAL << NOW << " node id " << nodeId << " " << pfmType << "::initLcid - initialized lcid " << lcid << endl;
}

void PacketFlowManagerEnb::clearLcid(LogicalCid lcid)
{
    if (connectionMap_.find(lcid) == connectionMap_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::clearLcid - Logical CID " << lcid << " not present." << endl;
        return;
    }
    else {
        connectionMap_[lcid].pdcpStatus_.clear();
        connectionMap_[lcid].rlcPdusPerSdu_.clear();
        connectionMap_[lcid].rlcSdusPerPdu_.clear();
        connectionMap_[lcid].macSdusPerPdu_.clear();
        connectionMap_[lcid].burstStatus_.clear();
        connectionMap_[lcid].burstId_ = 0;
        connectionMap_[lcid].burstState_ = false;

        //connectionMap_[lcid].macPduPerProcess_[i] = 0;
    }

    EV_FATAL << NOW << " node id " << connectionMap_[lcid].nodeId_ << " " << pfmType << "::clearLcid - cleared data structures for lcid " << lcid << endl;
}

void PacketFlowManagerEnb::clearAllLcid()
{
    connectionMap_.clear();
    EV_FATAL << NOW << " " << pfmType << "::clearAllLcid - cleared data structures for all lcids " << endl;
}

void PacketFlowManagerEnb::initPdcpStatus(StatusDescriptor *desc, unsigned int pdcp, unsigned int sduHeaderSize, simtime_t& arrivalTime)
{
    // if pdcpStatus_ already present, error
    std::map<unsigned int, PdcpStatus>::iterator it = desc->pdcpStatus_.find(pdcp);
    if (it != desc->pdcpStatus_.end())
        throw cRuntimeError("%s::initPdcpStatus - PdcpStatus for PDCP sno [%d] already present for node %hu, this should not happen. Abort", pfmType.c_str(), pdcp, num(desc->nodeId_));

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

void PacketFlowManagerEnb::insertPdcpSdu(inet::Packet *pdcpPkt)
{
    auto lteInfo = pdcpPkt->getTagForUpdate<FlowControlInfo>();
    LogicalCid lcid = lteInfo->getLcid();

    /*
     * check here if the LCID relative to this pdcp pdu is
     * already present in the pfm
     */

    if (connectionMap_.find(lcid) == connectionMap_.end())
        initLcid(lcid, lteInfo->getDestId());

    unsigned int pdcpSno = lteInfo->getSequenceNumber();
    int64_t pduSize = pdcpPkt->getByteLength();
    MacNodeId nodeId = lteInfo->getDestId();
    simtime_t entryTime = simTime();
    ////
    auto header = pdcpPkt->peekAtFront<LtePdcpPdu>();
    int sduSize = (B(pduSize) - header->getChunkLength()).get();

    int sduSizeBits = (b(pdcpPkt->getBitLength()) - header->getChunkLength()).get();

    if (sduDataVolume_.find(nodeId) == sduDataVolume_.end())
        sduDataVolume_[nodeId].dlBits = sduSizeBits;
    else
        sduDataVolume_[nodeId].dlBits += sduSizeBits;
////

    EV << pfmType << "::insertPdcpSdu - DL PDPC sdu bits: " << sduSizeBits << " sent to node: " << nodeId << endl;
    EV << pfmType << "::insertPdcpSdu - DL PDPC sdu bits: " << sduDataVolume_[nodeId].dlBits << " sent to node: " << nodeId << " in this period" << endl;

    auto cit = connectionMap_.find(lcid);
    if (cit == connectionMap_.end()) {
        // this may occur after a handover, when data structures are cleared
        // EV_FATAL << NOW << " node id "<< desc->nodeId_<< " " << pfmType << "::insertRlcPdu - Logical CID " << lcid << " not present." << endl;
        throw cRuntimeError("%s::insertPdcpSdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor *desc = &cit->second;

    initPdcpStatus(desc, pdcpSno, sduSize, entryTime);
    EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertPdcpSdu - PDCP status for PDCP PDU SN " << pdcpSno << " added. Logical cid " << lcid << endl;

    // add user to delay time map if not already present since many LCIDs can belong to one nodeId (UE)
    // consider to add at run time in case it is needed
    // put here for debug purposes
    pktDiscardCounterPerUe_[desc->nodeId_].total += 1;
    pktDiscardCounterTotal_.total += 1;
}

void PacketFlowManagerEnb::receivedPdcpSdu(inet::Packet *pdcpPkt)
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
    unsigned int sno = lteInfo->getSequenceNumber();
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

void PacketFlowManagerEnb::insertRlcPdu(LogicalCid lcid, const inet::Ptr<LteRlcUmDataPdu> rlcPdu, RlcBurstStatus status) {
    EV << pfmType << "::insertRlcPdu - Logical Cid: " << lcid << endl;

    auto cit = connectionMap_.find(lcid);
    if (cit == connectionMap_.end()) {
        // this may occur after a handover, when data structures are cleared
        // EV_FATAL << NOW << " node id "<< desc->nodeId_<< " " << pfmType << "::insertRlcPdu - Logical CID " << lcid << " not present." << endl;
        throw cRuntimeError("%s::insertRlcPdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor *desc = &cit->second;
    EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu - Logical CID " << lcid << endl;

    unsigned int rlcSno = rlcPdu->getPduSequenceNumber();

    if (desc->rlcSdusPerPdu_.find(rlcSno) != desc->rlcSdusPerPdu_.end())
        throw cRuntimeError("%s::insertRlcPdu - RLC PDU SN %d already present for logical CID %d. Aborting", pfmType.c_str(), rlcSno, lcid);


    // manage burst state, for debugging and avoid errors between rlc state and packetflowmanager state
    if (status == START) {
        if (desc->burstState_ == true)
            throw cRuntimeError("%s::insertRlcPdu - node %hu and lcid %d . RLC burst status START incompatible with local status %d. Aborting", pfmType.c_str(), num(desc->nodeId_), lcid, desc->burstState_);
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
            throw cRuntimeError("%s::insertRlcPdu - node %hu and lcid %d . RLC burst status STOP incompatible with local status %d. Aborting", pfmType.c_str(), num(desc->nodeId_), lcid, desc->burstState_);
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
            throw cRuntimeError("%s::insertRlcPdu - node %hu and lcid %d . RLC burst status INACTIVE incompatible with local status %d. Aborting", pfmType.c_str(), num(desc->nodeId_), lcid, desc->burstState_);
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu INACTIVE burst" << endl;
    }
    else if (status == ACTIVE) {
        if (desc->burstState_ == false)
            throw cRuntimeError("%s::insertRlcPdu - node %hu and lcid %d . RLC burst status ACTIVE incompatible with local status %d. Aborting", pfmType.c_str(), num(desc->nodeId_), lcid, desc->burstState_);
        auto bsit = desc->burstStatus_.find(desc->burstId_);
        if (bsit == desc->burstStatus_.end())
            throw cRuntimeError("%s::insertRlcPdu - node %hu and lcid %d . Burst status not found during active burst. Aborting", pfmType.c_str(), num(desc->nodeId_), lcid);
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu ACTIVE burst " << desc->burstId_ << endl;
    }
    else {
        throw cRuntimeError("%s::insertRlcPdu RLCBurstStatus not recognized", pfmType.c_str());
    }

    FramingInfo fi = rlcPdu->getFramingInfo();
    const RlcSduList *rlcSduList = rlcPdu->getRlcSduList();
    const RlcSduListSizes *rlcSduSizes = rlcPdu->getRlcSduSizes();
    auto lit = rlcSduList->begin();
    auto sit = rlcSduSizes->begin();
    for ( ; lit != rlcSduList->end(); ++lit, ++sit) {
        auto rlcSdu = (*lit)->peekAtFront<LteRlcSdu>();
        unsigned int pdcpSno = rlcSdu->getSnoMainPacket();
        unsigned int pdcpPduLength = *(sit); // TODO fix with size of the chunk!!

        EV << "PacketFlowManagerEnb::insertRlcPdu - pdcpSdu " << pdcpSno << " with length: " << pdcpPduLength << " bytes" << endl;
        //
        // store the RLC SDUs (PDCP PDUs) included in the RLC PDU
        desc->rlcSdusPerPdu_[rlcSno].insert(pdcpSno);

        // now store the inverse association, i.e., for each RLC SDU, record in which RLC PDU it is included
        desc->rlcPdusPerSdu_[pdcpSno].insert(rlcSno);

        // set the PDCP entry time
        auto pit = desc->pdcpStatus_.find(pdcpSno);
        if (pit == desc->pdcpStatus_.end())
            throw cRuntimeError("%s::insertRlcPdu - PdcpStatus for PDCP sno [%d] not present, this should not happen. Abort", pfmType.c_str(), pdcpSno);

        // last pdcp
        if (lit != rlcSduList->end() && lit == --rlcSduList->end()) {
            // 01 or 11, lsb 1 (3GPP TS 36.322)
            // means -> Last byte of the Data field does not correspond to the last byte of a RLC SDU.
            if ((fi & 1) == 1) {
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

        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu - lcid[" << lcid << "], insert PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
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
            throw cRuntimeError("%s::insertRlcPdu - node %hu and lcid %d . Burst status not found during active burst. Aborting", pfmType.c_str(), num(desc->nodeId_), lcid);
        // add rlc to rlc set of the burst and the size
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertRlcPdu - lcid[" << lcid << "], insert RLC SDU of size " << rlcSduSize << endl;

        bsit->second.rlcPdu[rlcSno] = rlcSduSize;
    }
}

void PacketFlowManagerEnb::discardRlcPdu(LogicalCid lcid, unsigned int rlcSno, bool fromMac)
{
    auto cit = connectionMap_.find(lcid);
    if (cit == connectionMap_.end()) {
        // this may occur after a handover, when data structures are cleared
        // EV_FATAL << NOW << " node id "<< desc->nodeId_<< " " << pfmType << "::discardRlcPdu - Logical CID " << lcid << " not present." << endl;
        throw cRuntimeError("%s::discardRlcPdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor *desc = &cit->second;
    if (desc->rlcSdusPerPdu_.find(rlcSno) == desc->rlcSdusPerPdu_.end())
        throw cRuntimeError("%s::discardRlcPdu - RLC PDU SN %d not present for logical CID %d. Aborting", pfmType.c_str(), rlcSno, lcid);

    // get the PDCP SDUs fragmented in this RLC PDU
    SequenceNumberSet pdcpSnoSet = desc->rlcSdusPerPdu_.find(rlcSno)->second;
    for (const auto& pdcpSno : pdcpSnoSet) {
        // find sdu -> rlc for this pdcp
        auto rit = desc->rlcPdusPerSdu_.find(pdcpSno);
        if (rit == desc->rlcPdusPerSdu_.end())
            throw cRuntimeError("%s::discardRlcPdu - PdcpStatus for PDCP sno [%d] with lcid [%d] not present, this should not happen. Abort", pfmType.c_str(), pdcpSno, lcid);

        // remove the RLC PDUs that contain a fragment of this pdcpSno
        rit->second.erase(rlcSno);

        // set this pdcp sdu as discarded, flag use in macPduArrive to not take into account this pdcp
        auto pit = desc->pdcpStatus_.find(pdcpSno);
        if (pit == desc->pdcpStatus_.end())
            throw cRuntimeError("%s::discardRlcPdu - PdcpStatus for PDCP sno [%d] already present, this should not happen. Abort", pfmType.c_str(), pdcpSno);

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
            EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::discardRlcPdu - lcid[" << lcid << "], discarded PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
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

void PacketFlowManagerEnb::insertMacPdu(inet::Ptr<const LteMacPdu> macPdu)
{
    EV << pfmType << "::insertMacPdu" << endl;

    /*
     * retrieve the macPduId and the Lcid
     */
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if (len == 0)
        throw cRuntimeError("%s::macPduArrived - macPdu has no Rlc pdu! This, here, should not happen", pfmType.c_str());
    for (int i = 0; i < len; ++i) {
        auto rlcPdu = macPdu->getSdu(i);
        auto lteInfo = rlcPdu.getTag<FlowControlInfo>();
        int lcid = lteInfo->getLcid();
        auto cit = connectionMap_.find(lcid);
        if (cit == connectionMap_.end()) {
            // this may occur after a handover, when data structures are cleared
            // EV_FATAL << NOW << " node id "<< desc->nodeId_<< " " << pfmType << "::insertMacPdu - Logical CID " << lcid << " not present." << endl;
            throw cRuntimeError("%s::insertMacPdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
            return;
        }

        // get the descriptor for this connection
        StatusDescriptor *desc = &cit->second;
        if (desc->macSdusPerPdu_.find(macPduId) != desc->macSdusPerPdu_.end())
            throw cRuntimeError("%s::insertMacPdu - MAC PDU ID %d already present for logical CID %d. Aborting", pfmType.c_str(), macPduId, lcid);

        for (int i = 0; i < len; ++i) {
            auto rlcPdu = macPdu->getSdu(i);

            unsigned int rlcSno = rlcPdu.peekAtFront<LteRlcUmDataPdu>()->getPduSequenceNumber();
            EV << "MAC pdu: " << macPduId << " has RLC pdu: " << rlcSno << endl;

            auto tit = desc->rlcSdusPerPdu_.find(rlcSno);
            if (tit == desc->rlcSdusPerPdu_.end())
                throw cRuntimeError("%s::insertMacPdu - RLC PDU ID %d not present in the status descriptor of lcid %d ", pfmType.c_str(), rlcSno, lcid);

            // store the MAC SDUs (RLC PDUs) included in the MAC PDU
            desc->macSdusPerPdu_[macPduId].insert(rlcSno);
            EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::insertMacPdu - lcid[" << lcid << "], insert RLC PDU " << rlcSno << " in MAC PDU " << macPduId << endl;

            // set the pdcp pdus related to this RLC as sent over the air since this method is called after the MAC ID
            // has been inserted in the HARQBuffer
            SequenceNumberSet pdcpSet = tit->second;
            for (const auto& pdcpSno : pdcpSet) {
                auto sdit = desc->pdcpStatus_.find(pdcpSno);
                if (sdit == desc->pdcpStatus_.end())
                    throw cRuntimeError("%s::insertMacPdu - PdcpStatus for PDCP sno [%d] not present, this should not happen. Abort", pfmType.c_str(), pdcpSno);
                sdit->second.sentOverTheAir = true;
            }
        }
    }
}

void PacketFlowManagerEnb::macPduArrived(inet::Ptr<const LteMacPdu> macPdu)
{
    /*
     * retrieve the macPduId and the Lcid
     */
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if (len == 0)
        throw cRuntimeError("%s::macPduArrived - macPdu has no Rlc pdu! This, here, should not happen", pfmType.c_str());
    for (int i = 0; i < len; ++i) {
        auto rlcPdu = macPdu->getSdu(i);
        auto lteInfo = rlcPdu.getTag<FlowControlInfo>();
        int lcid = lteInfo->getLcid();
        auto cit = connectionMap_.find(lcid);
        if (cit == connectionMap_.end()) {
            throw cRuntimeError("%s::macPduArrived - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
            return;
        }

        // get the descriptor for this connection
        StatusDescriptor *desc = &cit->second;
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - MAC PDU " << macPduId << " of lcid " << lcid << " arrived." << endl;
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - Get MAC PDU ID [" << macPduId << "], which contains:" << endl;

        // === STEP 1 ==================================================== //
        // === recover the set of RLC PDU SN from the above MAC PDU ID === //

        if (macPduId == 0) {
            EV << NOW << " " << pfmType << "::insertMacPdu - The process does not contain entire SDUs" << endl;
            return;
        }

        auto mit = desc->macSdusPerPdu_.find(macPduId);
        if (mit == desc->macSdusPerPdu_.end())
            throw cRuntimeError("%s::macPduArrived - MAC PDU ID %d not present for logical CID %d. Aborting", pfmType.c_str(), macPduId, lcid);
        SequenceNumberSet rlcSnoSet = mit->second;

        // === STEP 2 ========================================================== //
        // === for each RLC PDU SN, recover the set of RLC SDU (PDCP PDU) SN === //

        for (const auto& rlcPduSno : rlcSnoSet) {
            EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - --> RLC PDU [" << rlcPduSno << "], which contains:" << endl;

            auto nit = desc->rlcSdusPerPdu_.find(rlcPduSno);
            if (nit == desc->rlcSdusPerPdu_.end())
                throw cRuntimeError("%s::macPduArrived - RLC PDU SN %d not present for logical CID %d. Aborting", pfmType.c_str(), rlcPduSno, lcid);
            SequenceNumberSet pdcpSnoSet = nit->second;

            // === STEP 3 ============================================================================ //
            // === (PDCP PDU) SN, recover the set of RLC PDU where it is included,                 === //
            // === remove the above RLC PDU SN. If the set becomes empty, compute the delay if     === //
            // === all PDCP PDU fragments have been transmitted                                    === //

            for (const auto& pdcpPduSno : pdcpSnoSet) {
                EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - ----> PDCP PDU [" << pdcpPduSno << "]" << endl;

                auto oit = desc->rlcPdusPerSdu_.find(pdcpPduSno);
                if (oit == desc->rlcPdusPerSdu_.end())
                    throw cRuntimeError("%s::macPduArrived - PDCP PDU SN %d not present for logical CID %d. Aborting", pfmType.c_str(), pdcpPduSno, lcid);

                auto kt = oit->second.find(rlcPduSno);
                if (kt == oit->second.end())
                    throw cRuntimeError("%s::macPduArrived - RLC PDU SN %d not present in the set of PDCP PDU SN %d for logical CID %d. Aborting", pfmType.c_str(), pdcpPduSno, rlcPduSno, lcid);

                // the RLC PDU has been sent, so erase it from the set
                oit->second.erase(kt);

                auto pit = desc->pdcpStatus_.find(pdcpPduSno);
                if (pit == desc->pdcpStatus_.end())
                    throw cRuntimeError("%s::macPduArrived - PdcpStatus for PDCP sno [%d] not present for lcid [%d], this should not happen. Abort", pfmType.c_str(), pdcpPduSno, lcid);

                // check whether the set is now empty
                if (desc->rlcPdusPerSdu_[pdcpPduSno].empty()) {
                    if (pit->second.entryTime == 0)
                        throw cRuntimeError("%s::macPduArrived - PDCP PDU SN %d of Lcid %d has no entry time timestamp, this should not happen. Aborting", pfmType.c_str(), pdcpPduSno, lcid);

                    if (pit->second.hasArrivedAll && !pit->second.discardedAtRlc && !pit->second.discardedAtMac) {
                        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - ----> PDCP PDU [" << pdcpPduSno << "] has been completely sent, remove from PDCP buffer" << endl;

                        auto dit = pdcpDelay_.find(desc->nodeId_);
                        if (dit == pdcpDelay_.end()) {
                            pdcpDelay_[desc->nodeId_] = { 0, 0 }; // create new structure
                            dit = pdcpDelay_.find(desc->nodeId_);
                        }

                        double time = (simTime() - pit->second.entryTime).dbl();

                        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::macPduArrived - PDCP PDU " << pdcpPduSno << " of lcid " << lcid << " acknowledged. Delay time: " << time << "s" << endl;

                        dit->second.time += (simTime() - pit->second.entryTime);

                        dit->second.pktCount += 1;

                        // update next sno
                        nextPdcpSno_ = pdcpPduSno + 1;

                        // remove pdcp status
                        desc->pdcpStatus_.erase(pit);
                        oit->second.clear();
                        desc->rlcPdusPerSdu_.erase(oit); // erase PDCP PDU SN
                    }
                }
            }
            desc->rlcSdusPerPdu_.erase(nit); // erase RLC PDU SN
            // update next sno
            nextRlcSno_ = rlcPduSno + 1;
            removePdcpBurstRLC(desc, rlcPduSno, true); // check if the pdcp is part of a burst
        }

        desc->macSdusPerPdu_.erase(mit); // erase MAC PDU ID
    }
}

void PacketFlowManagerEnb::discardMacPdu(const inet::Ptr<const LteMacPdu> macPdu)
{
    /*
     * retrieve the macPduId and the Lcid
     */
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if (len == 0)
        throw cRuntimeError("%s::macPduArrived - macPdu has no Rlc pdu! This, here, should not happen", pfmType.c_str());
    for (int i = 0; i < len; ++i) {
        auto rlcPdu = macPdu->getSdu(i);
        auto lteInfo = rlcPdu.getTag<FlowControlInfo>();
        int lcid = lteInfo->getLcid();

        auto cit = connectionMap_.find(lcid);
        if (cit == connectionMap_.end()) {
            throw cRuntimeError("%s::discardMacPdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
            return;
        }

        StatusDescriptor *desc = &cit->second;
        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::discardMacPdu - MAC PDU " << macPduId << " of lcid " << lcid << " arrived." << endl;

        EV_FATAL << NOW << " node id " << desc->nodeId_ << " " << pfmType << "::discardMacPdu - Get MAC PDU ID [" << macPduId << "], which contains:" << endl;

        if (macPduId == 0) {
            EV << NOW << " " << pfmType << "::discardMacPdu - The process does not contain entire SDUs" << endl;
            return;
        }

        auto mit = desc->macSdusPerPdu_.find(macPduId);
        if (mit == desc->macSdusPerPdu_.end())
            throw cRuntimeError("%s::discardMacPdu - MAC PDU ID %d not present for logical CID %d. Aborting", pfmType.c_str(), macPduId, lcid);
        SequenceNumberSet rlcSnoSet = mit->second;

        for (const auto& sn : rlcSnoSet) {
            discardRlcPdu(lcid, sn, true);
        }

        desc->macSdusPerPdu_.erase(mit); // erase MAC PDU ID
    }
}

void PacketFlowManagerEnb::removePdcpBurstRLC(StatusDescriptor *desc, unsigned int rlcSno, bool ack)
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

void PacketFlowManagerEnb::resetDiscardCounterPerUe(MacNodeId id)
{
    std::map<MacNodeId, DiscardedPkts>::iterator it = pktDiscardCounterPerUe_.find(id);
    if (it == pktDiscardCounterPerUe_.end()) {
        // maybe it is possible? think about it
        // yes
        return;
    }
    it->second = { 0, 0 };
}

double PacketFlowManagerEnb::getDiscardedPktPerUe(MacNodeId id)
{
    std::map<MacNodeId, DiscardedPkts>::iterator it = pktDiscardCounterPerUe_.find(id);
    if (it == pktDiscardCounterPerUe_.end()) {
        // maybe it is possible? think about it
        // yes, if I do not discard anything
        //throw cRuntimeError("%s::getTotalDiscardedPckPerUe - nodeId [%hu] not present",pfmType.c_str(),  id);
        return 0;
    }
    return ((double)it->second.discarded * 1000000) / it->second.total;
}

double PacketFlowManagerEnb::getDiscardedPkt()
{
    if (pktDiscardCounterTotal_.total == 0)
        return 0.0;
    return ((double)pktDiscardCounterTotal_.discarded * 1000000) / pktDiscardCounterTotal_.total;
}

void PacketFlowManagerEnb::insertHarqProcess(LogicalCid lcid, unsigned int harqProcId, unsigned int macPduId)
{
}

void PacketFlowManagerEnb::grantSent(MacNodeId nodeId, unsigned int grantId)
{
    Grant grant = { grantId, simTime() };
    for (const auto& grant : ulGrants_[nodeId]) {
        if (grant.grantId == grantId)
            throw cRuntimeError("%s::grantSent - grant [%d] for nodeId [%hu] already present", pfmType.c_str(), grantId, num(nodeId));
    }
    EV_FATAL << NOW << " " << pfmType << "::grantSent - Added grant " << grantId << " for nodeId " << nodeId << endl;
    ulGrants_[nodeId].push_back(grant);
}

void PacketFlowManagerEnb::ulMacPduArrived(MacNodeId nodeId, unsigned int grantId)
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
    throw cRuntimeError("%s::ulMacPduArrived - grant [%d] for nodeId [%hu] not present", pfmType.c_str(), grantId, num(nodeId));
}

double PacketFlowManagerEnb::getDelayStatsPerUe(MacNodeId id)
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

double PacketFlowManagerEnb::getUlDelayStatsPerUe(MacNodeId id)
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

void PacketFlowManagerEnb::resetDelayCounterPerUe(MacNodeId id)
{
    auto it = pdcpDelay_.find(id);
    if (it == pdcpDelay_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::resetDelayCounterPerUe - Delay Stats for Node Id " << id << " not present." << endl;
        return;
    }

    it->second = { 0, 0 };
}

void PacketFlowManagerEnb::resetUlDelayCounterPerUe(MacNodeId id)
{
    auto it = ULPktDelay_.find(id);
    if (it == ULPktDelay_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::resetUlDelayCounterPerUe - Ul Delay Stats for Node Id " << id << " not present." << endl;
        return;
    }

    it->second = { 0, 0 };
}

double PacketFlowManagerEnb::getThroughputStatsPerUe(MacNodeId id)
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

void PacketFlowManagerEnb::resetThroughputCounterPerUe(MacNodeId id)
{
    auto it = pdcpThroughput_.find(id);
    if (it == pdcpThroughput_.end()) {
        EV_FATAL << NOW << " " << pfmType << "::resetThroughputCounterPerUe - Throughput Stats for Node Id " << id << " not present." << endl;
        return;
    }
    it->second = { 0, 0 };
}

void PacketFlowManagerEnb::deleteUe(MacNodeId nodeId)
{
    /* It has to be deleted:
     * all structures with MacNodeId id
     * all lcids belonging to MacNodeId id
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

double PacketFlowManagerEnb::getPdpcLossRate()
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

double PacketFlowManagerEnb::getPdpcLossRatePerUe(MacNodeId id)
{
    auto it = packetLossRate_.find(id);
    if (it != packetLossRate_.end()) {
        return ((double)it->second.totalLossPdcp * 1000000) / it->second.totalPdcpSno;
    }
    else {
        return 0;
    }
}

void PacketFlowManagerEnb::resetPdpcLossRatePerUe(MacNodeId id)
{
    auto it = packetLossRate_.find(id);
    if (it != packetLossRate_.end()) {
        it->second.reset();
    }
}

void PacketFlowManagerEnb::resetPdpcLossRates()
{
    for (auto& ue : packetLossRate_) {
        ue.second.reset();
    }
}

uint64_t PacketFlowManagerEnb::getDataVolume(MacNodeId nodeId, Direction dir)
{
    auto node = sduDataVolume_.find(nodeId);
    if (node == sduDataVolume_.end())
        return 0;
    if (dir == DL)
        return node->second.dlBits;
    else if (dir == UL)
        return node->second.ulBits;
    else
        throw cRuntimeError("PacketFlowManagerEnb::getDataVolume - Wrong direction");
}

void PacketFlowManagerEnb::resetDataVolume(MacNodeId nodeId, Direction dir)
{
    auto node = sduDataVolume_.find(nodeId);
    if (node == sduDataVolume_.end())
        return;
    if (dir == DL)
        node->second.dlBits = 0;
    else if (dir == UL)
        node->second.ulBits = 0;
    else
        throw cRuntimeError("PacketFlowManagerEnb::getDataVolume - Wrong direction");
}

void PacketFlowManagerEnb::resetDataVolume(MacNodeId nodeId)
{
    auto node = sduDataVolume_.find(nodeId);
    if (node == sduDataVolume_.end())
        return;
    node->second.dlBits = 0;
    node->second.ulBits = 0;
}

void PacketFlowManagerEnb::finish()
{
}

} //namespace

