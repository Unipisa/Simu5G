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

#include "PacketFlowManagerUe.h"
#include "stack/mac/LteMacBase.h"
#include "stack/pdcp_rrc/LtePdcpRrc.h"
#include "stack/rlc/LteRlcDefs.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/mac/packet/LteMacPdu.h"

#include "common/LteControlInfo.h"

#include <sstream>

namespace simu5g {

Define_Module(PacketFlowManagerUe);



void PacketFlowManagerUe::initialize(int stage)
{
    if (stage == 1) {
        PacketFlowManagerBase::initialize(stage);
    }
}

bool PacketFlowManagerUe::hasLcid(LogicalCid lcid)
{
    return connectionMap_.find(lcid) != connectionMap_.end();
}

void PacketFlowManagerUe::initLcid(LogicalCid lcid, MacNodeId nodeId)
{
    if (connectionMap_.find(lcid) != connectionMap_.end())
        throw cRuntimeError("%s::initLcid - Logical CID %d already present. Aborting", pfmType.c_str(), lcid);

    // init new descriptor
    StatusDescriptor newDesc;
    newDesc.nodeId_ = nodeId;
    newDesc.pdcpStatus_.clear();
    newDesc.rlcPdusPerSdu_.clear();
    newDesc.rlcSdusPerPdu_.clear();
    newDesc.macSdusPerPdu_.clear();
    newDesc.macPduPerProcess_.resize(harqProcesses_, 0);

    connectionMap_[lcid] = newDesc;
    EV_FATAL << NOW << "node id " << nodeId << " " << pfmType << "::initLcid - initialized lcid " << lcid << endl;
}

void PacketFlowManagerUe::clearLcid(LogicalCid lcid)
{
    ConnectionMap::iterator it = connectionMap_.find(lcid);
    if (it == connectionMap_.end()) {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType << "::clearLcid - Logical CID " << lcid << " not present." << endl;
        return;
    }

    StatusDescriptor *desc = &it->second;
    desc->pdcpStatus_.clear();
    desc->rlcPdusPerSdu_.clear();
    desc->rlcSdusPerPdu_.clear();
    desc->macSdusPerPdu_.clear();

    for (int i = 0; i < harqProcesses_; i++)
        desc->macPduPerProcess_[i] = 0;

    EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::clearLcid - cleared data structures for lcid " << lcid << endl;
}

void PacketFlowManagerUe::clearAllLcid()
{
    connectionMap_.clear();
    EV_FATAL << NOW << " " << pfmType << "::clearAllLcid - cleared data structures for all lcids " << endl;
}

void PacketFlowManagerUe::clearStats()
{
    clearAllLcid();
    resetDelayCounter();
    resetDiscardCounter();
}

void PacketFlowManagerUe::initPdcpStatus(StatusDescriptor *desc, unsigned int pdcp, unsigned int pdcpSize, simtime_t& arrivalTime)

{
    // if pdcpStatus_ already present, error
    std::map<unsigned int, PdcpStatus>::iterator it = desc->pdcpStatus_.find(pdcp);
    if (it != desc->pdcpStatus_.end())
        throw cRuntimeError("%s::initPdcpStatus - PdcpStatus for PDCP sno [%d] already present, this should not happen. Abort", pfmType.c_str(), pdcp);

    PdcpStatus newpdcpStatus;

    newpdcpStatus.entryTime = arrivalTime;
    newpdcpStatus.discardedAtMac = false;
    newpdcpStatus.discardedAtRlc = false;
    newpdcpStatus.hasArrivedAll = false;
    newpdcpStatus.sentOverTheAir = false;
    newpdcpStatus.pdcpSduSize = pdcpSize;

    desc->pdcpStatus_[pdcp] = newpdcpStatus;
}

void PacketFlowManagerUe::insertPdcpSdu(inet::Packet *pdcpPkt)
{
    EV << pfmType << "::insertPdcpSdu" << endl;
    auto lteInfo = pdcpPkt->getTagForUpdate<FlowControlInfo>();
    LogicalCid lcid = lteInfo->getLcid();

    if (connectionMap_.find(lcid) == connectionMap_.end())
        initLcid(lcid, lteInfo->getSourceId());

    unsigned int pdcpSno = lteInfo->getSequenceNumber();
    int64_t pdcpSize = pdcpPkt->getByteLength();
    simtime_t arrivalTime = simTime();

    ConnectionMap::iterator cit = connectionMap_.find(lcid);
    if (cit == connectionMap_.end()) {
        // this may occur after a handover, when data structures are cleared
        // EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType <<"::insertRlcPdu - Logical CID " << lcid << " not present." << endl;
        throw cRuntimeError("%s::insertPdcpSdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor *desc = &cit->second;

    initPdcpStatus(desc, pdcpSno, pdcpSize, arrivalTime);
    pktDiscardCounterTotal_.total += 1;

    EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::insertPdcpSdu - PDCP status for PDCP PDU SN " << pdcpSno << " added. Logical cid " << lcid << endl;
}

void PacketFlowManagerUe::insertRlcPdu(LogicalCid lcid, const inet::Ptr<LteRlcUmDataPdu> rlcPdu, RlcBurstStatus status)
{
    ConnectionMap::iterator cit = connectionMap_.find(lcid);
    if (cit == connectionMap_.end()) {
        // this may occur after a handover, when data structures are cleared
        throw cRuntimeError("%s::insertRlcPdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor *desc = &cit->second;

    unsigned int rlcSno = rlcPdu->getPduSequenceNumber();

    if (desc->rlcSdusPerPdu_.find(rlcSno) != desc->rlcSdusPerPdu_.end())
        throw cRuntimeError("%s::insertRlcPdu - RLC PDU SN %d already present for logical CID %d. Aborting", pfmType.c_str(), rlcSno, lcid);
    EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::insertRlcPdu - Logical CID " << lcid << endl;

    FramingInfo fi = rlcPdu->getFramingInfo();
    const RlcSduList *rlcSduList = rlcPdu->getRlcSduList();
    const RlcSduListSizes *rlcSduSizes = rlcPdu->getRlcSduSizes();
    auto lit = rlcSduList->begin();
    auto sit = rlcSduSizes->begin();
    for ( ; lit != rlcSduList->end(); ++lit, ++sit) {
        auto rlcSdu = (*lit)->peekAtFront<LteRlcSdu>();
        unsigned int pdcpSno = rlcSdu->getSnoMainPacket();
        unsigned int pdcpPduLength = *(sit);

        EV << pfmType << "::insertRlcPdu - pdcpSdu " << pdcpSno << " with length: " << pdcpPduLength << " bytes" << endl;

        // store the RLC SDUs (PDCP PDUs) included in the RLC PDU
        desc->rlcSdusPerPdu_[rlcSno].insert(pdcpSno);

        // now store the inverse association, i.e., for each RLC SDU, record in which RLC PDU is included
        desc->rlcPdusPerSdu_[pdcpSno].insert(rlcSno);

        // set the PDCP entry time
        std::map<unsigned int, PdcpStatus>::iterator pit = desc->pdcpStatus_.find(pdcpSno);
        if (pit == desc->pdcpStatus_.end())
            throw cRuntimeError("%s::insertRlcPdu - PdcpStatus for PDCP sno [%d] not present, this should not happen. Abort", pfmType.c_str(), pdcpSno);

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
        else {
            pit->second.hasArrivedAll = true;
        }

        EV_FATAL << NOW << " " << pfmType << "::insertRlcPdu - lcid[" << lcid << "], insert PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
    }
    EV << "size:" << desc->rlcSdusPerPdu_[rlcSno].size() << endl;
}

void PacketFlowManagerUe::discardRlcPdu(LogicalCid lcid, unsigned int rlcSno, bool fromMac)
{
    ConnectionMap::iterator cit = connectionMap_.find(lcid);
    if (cit == connectionMap_.end()) {
        // this may occur after a handover, when data structures are cleared
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
        auto rit = desc->rlcPdusPerSdu_.find(pdcpSno);
        if (rit == desc->rlcPdusPerSdu_.end())
            throw cRuntimeError("%s::discardRlcPdu - PdcpStatus for PDCP sno [%d] with lcid [%d] not present, this should not happen. Abort", pfmType.c_str(), pdcpSno, lcid);

        // remove the RLC PDUs that contains a fragment of this pdcpSno
        rit->second.erase(rlcSno);

        // set this pdcp sdu that a RLC has been discarded, i.e the arrived pdcp will be not entire.
        auto pit = desc->pdcpStatus_.find(pdcpSno);
        if (pit == desc->pdcpStatus_.end())
            throw cRuntimeError("%s::discardRlcPdu - PdcpStatus for PDCP sno [%d] already present, this should not happen. Abort", pfmType.c_str(), pdcpSno);

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
            EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::discardRlcPdu - lcid[" << lcid << "], discarded PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
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

void PacketFlowManagerUe::insertMacPdu(inet::Ptr<const LteMacPdu> macPdu)
{
    EV << pfmType << "::insertMacPdu" << endl;

    /*
     * retrieve the macPduId and the Lcid
     */
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if (len == 0)
        throw cRuntimeError("%s::macPduArrived - macPdu has no Rlc pdu! This should not happen", pfmType.c_str());
    for (int i = 0; i < len; ++i) {
        auto rlcPdu = macPdu->getSdu(i);
        auto lteInfo = rlcPdu.getTag<FlowControlInfo>();
        int lcid = lteInfo->getLcid();

        ConnectionMap::iterator cit = connectionMap_.find(lcid);
        if (cit == connectionMap_.end()) {
            // this may occur after a handover, when data structures are cleared
            throw cRuntimeError("%s::insertMacPdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
            return;
        }

        // get the descriptor for this connection
        StatusDescriptor *desc = &cit->second;
        if (desc->macSdusPerPdu_.find(macPduId) != desc->macSdusPerPdu_.end())
            throw cRuntimeError("%s::insertMacPdu - MAC PDU ID %d already present for logical CID %d. Aborting", pfmType.c_str(), macPduId, lcid);

        auto macSdu = macPdu->getSdu(i).peekAtFront<LteRlcUmDataPdu>();
        unsigned int rlcSno = macSdu->getPduSequenceNumber();

        auto aa = macPdu->getSdu(i); // all rlc pdus have the same lcid
        auto ee = aa.getTag<FlowControlInfo>();
        int ll = ee->getLcid();
        EV << "ALE LCID: " << ll << endl;

        EV << "MAC pdu: " << macPduId << " has RLC pdu: " << rlcSno << endl;

        // ??????
        // record the association RLC PDU - MAC PDU only if the RLC PDU contains at least one entire SDU
        // the condition holds if the RLC PDU SN is stored in the data structure rlcSdusPerPdu_

        std::map<unsigned int, SequenceNumberSet>::iterator tit = desc->rlcSdusPerPdu_.find(rlcSno);
        if (tit == desc->rlcSdusPerPdu_.end())
            throw cRuntimeError("%s::insertMacPdu - RLC PDU ID %d not present in the status descriptor of lcid %d ", pfmType.c_str(), rlcSno, lcid);

        // store the MAC SDUs (RLC PDUs) included in the MAC PDU
        desc->macSdusPerPdu_[macPduId].insert(rlcSno);
        EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::insertMacPdu - lcid[" << lcid << "], insert RLC PDU " << rlcSno << " in MAC PDU " << macPduId << endl;

        // set the pdcp pdus related to this RLC as sent over the air since this method is called after the MAC ID
        // has been inserted in the HARQBuffer
        SequenceNumberSet pdcpSet = tit->second;
        for (auto pit : pdcpSet) {
            auto sdit = desc->pdcpStatus_.find(pit);
            if (sdit == desc->pdcpStatus_.end())
                throw cRuntimeError("%s::insertMacPdu - PdcpStatus for PDCP sno [%d] not present, this should not happen. Abort", pfmType.c_str(), pit);
            sdit->second.sentOverTheAir = true;
        }
    }
}

void PacketFlowManagerUe::macPduArrived(inet::Ptr<const LteMacPdu> macPdu)
{
    EV << pfmType << "::macPduArrived" << endl;
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

        std::map<LogicalCid, StatusDescriptor>::iterator cit = connectionMap_.find(lcid);
        if (cit == connectionMap_.end()) {
            // this may occur after a handover, when data structures are cleared
            throw cRuntimeError("%s::macPduArrived - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
            return;
        }

        // get the descriptor for this connection
        StatusDescriptor *desc = &cit->second;

        EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::macPduArrived - Get MAC PDU ID [" << macPduId << "], which contains:" << endl;
        EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::macPduArrived - MAC PDU " << macPduId << " of lcid " << lcid << " arrived." << endl;

        // === STEP 1 ==================================================== //
        // === recover the set of RLC PDU SN from the above MAC PDU ID === //
        if (macPduId == 0) {
            EV << NOW << " " << pfmType << "::insertMacPdu - The process does not contain entire SDUs" << endl;
            return;
        }

        std::map<unsigned int, SequenceNumberSet>::iterator mit = desc->macSdusPerPdu_.find(macPduId);
        if (mit == desc->macSdusPerPdu_.end())
            throw cRuntimeError("%s::macPduArrived - MAC PDU ID %d not present for logical CID %d. Aborting", pfmType.c_str(), macPduId, lcid);
        SequenceNumberSet rlcSnoSet = mit->second;

        auto macSdu = rlcPdu.peekAtFront<LteRlcUmDataPdu>();
        unsigned int rlcSno = macSdu->getPduSequenceNumber();

        if (rlcSnoSet.find(rlcSno) == rlcSnoSet.end())
            throw cRuntimeError("%s::macPduArrived - RLC sno [%d] not present in rlcSnoSet structure for MAC PDU ID %d not present for logical CID %d. Aborting", pfmType.c_str(), rlcSno, macPduId, lcid);

        // === STEP 2 ========================================================== //
        // === for each RLC PDU SN, recover the set of RLC SDU (PDCP PDU) SN === //

        for (auto rlcPduSno : rlcSnoSet) {
            // for each RLC PDU
            EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::macPduArrived - --> RLC PDU [" << rlcPduSno << "], which contains:" << endl;

            std::map<unsigned int, SequenceNumberSet>::iterator nit = desc->rlcSdusPerPdu_.find(rlcPduSno);
            if (nit == desc->rlcSdusPerPdu_.end())
                throw cRuntimeError("%s::macPduArrived - RLC PDU SN %d not present for logical CID %d. Aborting", pfmType.c_str(), rlcPduSno, lcid);
            SequenceNumberSet pdcpSnoSet = nit->second;

            // === STEP 3 ============================================================================ //
            // === Since an RLC SDU may be fragmented in more than one RLC PDU, thus it must be     === //
            // === retransmitted only if all fragments have been transmitted.                      === //
            // === For each RLC SDU (PDCP PDU) SN, recover the set of RLC PDUs where it is included,=== //
            // === remove the above RLC PDU SN. If the set becomes empty, compute the delay if     === //
            // === all PDCP PDU fragments have been transmitted                                     === //

            for (auto pdcpPduSno : pdcpSnoSet) {
                // for each RLC SDU (PDCP PDU), get the set of RLC PDUs where it is included
                EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::macPduArrived - ----> PDCP PDU [" << pdcpPduSno << "]" << endl;

                std::map<unsigned int, SequenceNumberSet>::iterator oit = desc->rlcPdusPerSdu_.find(pdcpPduSno);
                if (oit == desc->rlcPdusPerSdu_.end())
                    throw cRuntimeError("%s::macPduArrived - PDCP PDU SN %d not present for logical CID %d. Aborting", pfmType.c_str(), pdcpPduSno, lcid);

                // oit->second is the set of RLC PDU in which the PDCP PDU is contained

                // the RLC PDU SN must be present in the set
                if (oit->second.find(rlcPduSno) == oit->second.end())
                    throw cRuntimeError("%s::macPduArrived - RLC PDU SN %d not present in the set of PDCP PDU SN %d for logical CID %d. Aborting", pfmType.c_str(), pdcpPduSno, rlcPduSno, lcid);

                // the RLC PDU has been sent, so erase it from the set
                oit->second.erase(rlcPduSno);

                std::map<unsigned int, PdcpStatus>::iterator pit = desc->pdcpStatus_.find(pdcpPduSno);
                if (pit == desc->pdcpStatus_.end())
                    throw cRuntimeError("%s::macPduArrived - PdcpStatus for PDCP sno [%d] not present for lcid [%d], this should not happen. Abort", pfmType.c_str(), pdcpPduSno, lcid);

                // check whether the set is now empty
                if (desc->rlcPdusPerSdu_[pdcpPduSno].empty()) {

                    // set the time for pdcpPduSno
                    if (pit->second.hasArrivedAll && !pit->second.discardedAtRlc && !pit->second.discardedAtMac) { // the whole current pdcp seqNum has been received
                        EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::macPduArrived - ----> PDCP PDU [" << pdcpPduSno << "] has been completely sent, remove from PDCP buffer" << endl;

                        double time = (simTime() - pit->second.entryTime).dbl();
                        pdcpDelay.time += time;
                        pdcpDelay.pktCount += 1;

                        EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::macPduArrived - PDCP PDU " << pdcpPduSno << " of lcid " << lcid << " acknowledged. Delay time: " << time << "s" << endl;

                        // update next sno
                        nextPdcpSno_ = pdcpPduSno + 1;

                        // remove pdcp status
                        oit->second.clear();
                        desc->rlcPdusPerSdu_.erase(oit); // erase PDCP PDU SN
                    }
                }
            }
            // update next sno
            nextRlcSno_ = rlcPduSno + 1;
        }

        mit->second.clear();
        desc->macSdusPerPdu_.erase(mit); // erase MAC PDU ID
    }
}

void PacketFlowManagerUe::discardMacPdu(const inet::Ptr<const LteMacPdu> macPdu)
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
            // this may occur after a handover, when data structures are cleared
            throw cRuntimeError("%s::discardMacPdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
            return;
        }

        // get the descriptor for this connection
        StatusDescriptor *desc = &cit->second;

        EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::discardMacPdu - Get MAC PDU ID [" << macPduId << "], which contains:" << endl;
        EV_FATAL << NOW << "node id " << desc->nodeId_ - 1025 << " " << pfmType << "::discardMacPdu - MAC PDU " << macPduId << " of lcid " << lcid << " arrived." << endl;

        // === STEP 1 ==================================================== //
        // === recover the set of RLC PDU SN from the above MAC PDU ID === //

        if (macPduId == 0) {
            EV << NOW << " " << pfmType << "::discardMacPdu - The process does not contain entire SDUs" << endl;
            return;
        }

        auto mit = desc->macSdusPerPdu_.find(macPduId);
        if (mit == desc->macSdusPerPdu_.end())
            throw cRuntimeError("%s::discardMacPdu - MAC PDU ID %d not present for logical CID %d. Aborting", pfmType.c_str(), macPduId, lcid);
        SequenceNumberSet rlcSnoSet = mit->second;

        auto macSdu = rlcPdu.peekAtFront<LteRlcUmDataPdu>();
        unsigned int rlcSno = macSdu->getPduSequenceNumber();

        if (rlcSnoSet.find(rlcSno) == rlcSnoSet.end())
            throw cRuntimeError("%s::macPduArrived - RLC sno [%d] not present in rlcSnoSet structure for MAC PDU ID %d not present for logical CID %d. Aborting", pfmType.c_str(), rlcSno, macPduId, lcid);

        // === STEP 2 ========================================================== //
        // === for each RLC PDU SN, recover the set of RLC SDU (PDCP PDU) SN === //

        for (const auto& rlcSno : rlcSnoSet) {
            discardRlcPdu(lcid, rlcSno, true);
        }

        mit->second.clear();
        desc->macSdusPerPdu_.erase(mit); // erase MAC PDU ID
    }
}

DiscardedPkts PacketFlowManagerUe::getDiscardedPkt()
{
    return pktDiscardCounterTotal_;
}

double PacketFlowManagerUe::getDelayStats()
{
    if (pdcpDelay.pktCount == 0)
        return 0;
    EV_FATAL << NOW << " " << pfmType << "::getDelayStats- Delay Stats total time: " << pdcpDelay.time.dbl() * 1000 << " pckcount: " << pdcpDelay.pktCount << endl;

    return (pdcpDelay.time.dbl() * 1000) / pdcpDelay.pktCount;
}

void PacketFlowManagerUe::resetDelayCounter()
{
    pdcpDelay = { 0, 0 };
}

void PacketFlowManagerUe::insertHarqProcess(LogicalCid lcid, unsigned int harqProcId, unsigned int macPduId)
{
}

void PacketFlowManagerUe::finish()
{
}

} //namespace

