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
#include "stack/mac/layer/LteMacBase.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"
#include "stack/rlc/LteRlcDefs.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/mac/packet/LteMacPdu.h"

#include "common/LteControlInfo.h"

#include <sstream>


Define_Module(PacketFlowManagerUe);

PacketFlowManagerUe::PacketFlowManagerUe()
{
    connectionMap_.clear();
    pdcpDelay = {0, 0};
}

PacketFlowManagerUe::~PacketFlowManagerUe()
{
//times_.clear();
}

void PacketFlowManagerUe::initialize(int stage)
{
    if (stage == 1)
    {
        PacketFlowManagerBase::initialize(stage);
//        pdcp_ = check_and_cast<LtePdcpRrcUe *>(getParentModule()->getSubmodule("pdcpRrc"));
    }
}

bool PacketFlowManagerUe::checkLcid(LogicalCid lcid)
{
    if (connectionMap_.find(lcid) == connectionMap_.end())
        return false;
    return true;
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
    EV_FATAL << NOW << "node id "<< nodeId << " " << pfmType.c_str() <<"::initLcid - initialized lcid " << lcid << endl;
}

void PacketFlowManagerUe::clearLcid(LogicalCid lcid)
{
    ConnectionMap::iterator it = connectionMap_.find(lcid);
    if (it == connectionMap_.end())
    {
        // this may occur after a handover, when data structures are cleared
        EV_FATAL << NOW << " " << pfmType.c_str() <<"::clearLcid - Logical CID " << lcid << " not present." << endl;
        return;
    }

        StatusDescriptor* desc = &it->second;
        desc->pdcpStatus_.clear();
        desc->rlcPdusPerSdu_.clear();
        desc->rlcSdusPerPdu_.clear();
        desc->macSdusPerPdu_.clear();

        for (int i=0; i<harqProcesses_; i++)
            desc->macPduPerProcess_[i] = 0;

    EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::clearLcid - cleared data structures for lcid " << lcid << endl;
}

void PacketFlowManagerUe::clearAllLcid()
{
    connectionMap_.clear();
    EV_FATAL << NOW << " " << pfmType.c_str() <<"::clearAllLcid - cleared data structures for all lcids "<< endl;
}

void PacketFlowManagerUe::clearStats()
{
    clearAllLcid();
    resetDelayCounter();
    resetDiscardCounter();
}
void PacketFlowManagerUe::initPdcpStatus(StatusDescriptor* desc, unsigned int pdcp, unsigned int pdcpSize, simtime_t& arrivalTime)

{
    // if pdcpStatus_ already present, error
    std::map<unsigned int, PdcpStatus>::iterator it = desc->pdcpStatus_.find(pdcp);
    if(it != desc->pdcpStatus_.end())
        throw cRuntimeError("%s::initPdcpStatus - PdcpStatus for PDCP sno [%d] already present, this should not happen. Abort", pfmType.c_str(), pdcp);

    PdcpStatus newpdcpStatus;

    newpdcpStatus.entryTime = arrivalTime;
    newpdcpStatus.discardedAtMac = false;
    newpdcpStatus.discardedAtRlc = false;
    newpdcpStatus.hasArrivedAll  = false;
    newpdcpStatus.sentOverTheAir =  false;
    newpdcpStatus.pdcpSduSize = pdcpSize;

    desc->pdcpStatus_[pdcp] = newpdcpStatus;
}


void PacketFlowManagerUe::insertPdcpSdu(inet::Packet* pdcpPkt)
{
    EV << pfmType.c_str() <<"::insertPdcpSdu"<< endl;
//    // Control Information
//    auto pkt = check_and_cast<inet::Packet *> (pktAux);
    auto lteInfo = pdcpPkt->getTagForUpdate<FlowControlInfo>();
    LogicalCid lcid = lteInfo->getLcid();

    if (connectionMap_.find(lcid) == connectionMap_.end())
        initLcid(lcid, lteInfo->getSourceId());


    unsigned int pdcpSno = lteInfo->getSequenceNumber();
    int64_t  pdcpSize = pdcpPkt->getByteLength();
    int headerSize = lteInfo->getHeaderSize();
    MacNodeId nodeId = lteInfo->getSourceId();
    simtime_t arrivalTime = simTime();

    ConnectionMap::iterator cit = connectionMap_.find(lcid);
       if (cit == connectionMap_.end())
       {
           // this may occur after a handover, when data structures are cleared
           // EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::insertRlcPdu - Logical CID " << lcid << " not present." << endl;
           throw cRuntimeError("%s::insertPdcpSdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
           return;
       }

       // get the descriptor for this connection
       StatusDescriptor* desc = &cit->second;

       initPdcpStatus(desc, pdcpSno, pdcpSize, arrivalTime);
       pktDiscardCounterTotal_.total += 1;

       EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 <<" " << pfmType.c_str() <<"::insertPdcpSdu - PDCP status for PDCP PDU SN " << pdcpSno<<" added. Logicl cid " << lcid << endl;


}

//void PacketFlowManagerUe::insertRlcPdu(LogicalCid lcid, unsigned int rlcSno, SequenceNumberSet& pdcpSnoSet,  bool lastIsFrag)
//{
//
//    ConnectionMap::iterator cit = connectionMap_.find(lcid);
//    if (cit == connectionMap_.end())
//    {
//        // this may occur after a handover, when data structures are cleared
//        // EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::insertRlcPdu - Logical CID " << lcid << " not present." << endl;
//        throw cRuntimeError("%s::insertRlcPdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
//        return;
//    }
//
//    // get the descriptor for this connection
//    StatusDescriptor* desc = &cit->second;
//    if (desc->rlcSdusPerPdu_.find(rlcSno) != desc->rlcSdusPerPdu_.end())
//        throw cRuntimeError("%s::insertRlcPdu - RLC PDU SN %d already present for logical CID %d. Aborting", pfmType.c_str(), rlcSno, lcid);
//    EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::insertRlcPdu - Logical CID " << lcid << endl;
//    SequenceNumberSet::iterator sit = pdcpSnoSet.begin();
//    for (; sit != pdcpSnoSet.end(); ++sit)
//    {
//        unsigned int pdcpSno = *sit;
//        if(desc->nodeId_-1025 == 7)
////                ww.record(pdcpSno);
//        // store the RLC SDUs (PDCP PDUs) included in the RLC PDU
//        desc->rlcSdusPerPdu_[rlcSno].insert(pdcpSno);
//
//        // now store the inverse association, i.e., for each RLC SDU, record in which RLC PDU is included
//        desc->rlcPdusPerSdu_[pdcpSno].insert(rlcSno);
//        desc->rlcPdusPerSdu_[pdcpSno].insert(rlcSno);
//
//
//        // set the PDCP entry time
//        std::map<unsigned int, PdcpStatus>::iterator pit = desc->pdcpStatus_.find(pdcpSno);
//        if(pit == desc->pdcpStatus_.end())
//            throw cRuntimeError("%s::insertRlcPdu - PdcpStatus for PDCP sno [%d] not present, this should not happen. Abort", pfmType.c_str(), pdcpSno);
//
//        if(sit != pdcpSnoSet.end() && sit == --pdcpSnoSet.end()){
//            pit->second.hasArrivedAll = !lastIsFrag;
////            desc->allPdcdSduToRlc_[pdcpSno] = !lastIsFrag;
//        }else{
//            pit->second.hasArrivedAll = true;
//        }
//
//        EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::insertRlcPdu - lcid[" << lcid << "], insert PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
//    }
//}

void PacketFlowManagerUe::insertRlcPdu(LogicalCid lcid, const inet::Ptr<LteRlcUmDataPdu> rlcPdu, RlcBurstStatus status)
{
        ConnectionMap::iterator cit = connectionMap_.find(lcid);
        if (cit == connectionMap_.end())
        {
            // this may occur after a handover, when data structures are cleared
            // EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::insertRlcPdu - Logical CID " << lcid << " not present." << endl;
            throw cRuntimeError("%s::insertRlcPdu - Logical CID %d not present. It must be initialized before", pfmType.c_str(), lcid);
            return;
        }

        // get the descriptor for this connection
        StatusDescriptor* desc = &cit->second;

        unsigned int rlcSno = rlcPdu->getPduSequenceNumber();

        if (desc->rlcSdusPerPdu_.find(rlcSno) != desc->rlcSdusPerPdu_.end())
            throw cRuntimeError("%s::insertRlcPdu - RLC PDU SN %d already present for logical CID %d. Aborting", pfmType.c_str(), rlcSno, lcid);
        EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::insertRlcPdu - Logical CID " << lcid << endl;

//        FramingInfo fi = rlcPdu->getFramingInfo();
//
//        RlcSduList* rlcSduList = rlcPdu->getRlcSudList();
//        std::list<cPacket*>::const_iterator lit = rlcSduList->begin();
//        LteRlcSdu* rlcSdu;
//        FlowControlInfo* lteInfo;
        FramingInfo fi = rlcPdu->getFramingInfo();
        unsigned numSdu = rlcPdu->getNumSdu();
        const RlcSduList* rlcSduList = rlcPdu->getRlcSduList();
        const RlcSduListSizes* rlcSduSizes = rlcPdu->getRlcSduSizes();
        auto lit = rlcSduList->begin();
        auto sit = rlcSduSizes->begin();
        LteRlcSdu* rlcSdu;
        FlowControlInfo* lteInfo;

        for (; lit != rlcSduList->end(); ++lit, ++sit){
            auto rlcSdu = (*lit)->peekAtFront<LteRlcSdu>();
           //             lteInfo = check_and_cast<FlowControlInfo*>(rlcSdu->getControlInfo());


            unsigned int pdcpSno = rlcSdu->getSnoMainPacket();
            unsigned int pdcpPduLength = *(sit);

            EV << pfmType.c_str() <<"::insertRlcPdu - pdcpSdu " << pdcpSno << " with length: " << pdcpPduLength << "bytes" <<  endl;

            // store the RLC SDUs (PDCP PDUs) included in the RLC PDU
            desc->rlcSdusPerPdu_[rlcSno].insert(pdcpSno);

            // now store the inverse association, i.e., for each RLC SDU, record in which RLC PDU is included
            desc->rlcPdusPerSdu_[pdcpSno].insert(rlcSno);

            // set the PDCP entry time
            std::map<unsigned int, PdcpStatus>::iterator pit = desc->pdcpStatus_.find(pdcpSno);
            if(pit == desc->pdcpStatus_.end())
                throw cRuntimeError("%s::insertRlcPdu - PdcpStatus for PDCP sno [%d] not present, this should not happen. Abort", pfmType.c_str(), pdcpSno);

            if(lit != rlcSduList->end() && lit == --rlcSduList->end()){
                // 01 or 11, lsb 1 (3GPP TS 36.322)
                // means -> Last byte of the Data field does not correspond to the last byte of a RLC SDU.
                if((fi & 1) == 1)
                {
                    pit->second.hasArrivedAll = false;
                }
                else
                {
                    pit->second.hasArrivedAll = true;
                }

    //            desc->allPdcdSduToRlc_[pdcpSno] = !lastIsFrag;
            }
            else{
                pit->second.hasArrivedAll = true;
            }

            EV_FATAL << NOW  << " " << pfmType.c_str() <<"::insertRlcPdu - lcid[" << lcid << "], insert PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
        }
        EV << "size:"<< desc->rlcSdusPerPdu_[rlcSno].size()<< endl;
}


void PacketFlowManagerUe::discardRlcPdu(LogicalCid lcid, unsigned int rlcSno, bool fromMac)
{
    ConnectionMap::iterator cit = connectionMap_.find(lcid);
    if (cit == connectionMap_.end())
    {
        // this may occur after a handover, when data structures are cleared
        // EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::discardRlcPdu - Logical CID " << lcid << " not present." << endl;
        throw cRuntimeError("%s::discardRlcPdu - Logical CID %d not present. It must be initilized before", pfmType.c_str(), lcid);
        return;
    }

    // get the descriptor for this connection
    StatusDescriptor* desc = &cit->second;
    if (desc->rlcSdusPerPdu_.find(rlcSno) == desc->rlcSdusPerPdu_.end())
        throw cRuntimeError("%s::discardRlcPdu - RLC PDU SN %d not present for logical CID %d. Aborting", pfmType.c_str(), rlcSno, lcid);

    // get the PCDP SDUs fragmented in this RLC PDU
    SequenceNumberSet pdcpSnoSet = desc->rlcSdusPerPdu_.find(rlcSno)->second;
    SequenceNumberSet::iterator sit = pdcpSnoSet.begin();
    std::map<unsigned int, PdcpStatus>::iterator pit;
    std::map<unsigned int, SequenceNumberSet>::iterator rit;
    for (; sit != pdcpSnoSet.end(); ++sit)
    {
        unsigned int pdcpSno = *sit;

//        if(desc->nodeId_ - 1025 == 7)
//            if(myset.find(pdcpSno) == myset.end()){
//                myset.insert(pdcpSno);

//            }

        rit = desc->rlcPdusPerSdu_.find(pdcpSno);
        if(rit == desc->rlcPdusPerSdu_.end())
            throw cRuntimeError("%s::discardRlcPdu - PdcpStatus for PDCP sno [%d] with lcid [%d] not present, this should not happen. Abort", pfmType.c_str(), pdcpSno, lcid);

        // remove the RLC PDUs that contains a fragment of this pdcpSno
        rit->second.erase(rlcSno);


        // set this pdcp sdu that a RLC has been discarded, i.e the arrived pdcp will be not entire.
        pit = desc->pdcpStatus_.find(pdcpSno);
        if(pit == desc->pdcpStatus_.end())
            throw cRuntimeError("%s::discardRlcPdu - PdcpStatus for PDCP sno [%d] already present, this should not happen. Abort", pfmType.c_str(), pdcpSno);

        if(fromMac)
            pit->second.discardedAtMac = true;
        else
            pit->second.discardedAtRlc = true;

        // if the set is empty AND
        // the pdcp pdu has been encapsulated all AND
        // a RLC referred to this PDCP has not been discarded at MAC (i.e 4NACKs) AND
        // a RLC referred to this PDCP has not been sent over the air
        // ---> all PDCP has been discard at eNB before start its trasmission
        // compliant with ETSI 136 314 at 4.1.5.1

        if(rit->second.empty() && pit->second.hasArrivedAll && !pit->second.discardedAtMac && !pit->second.sentOverTheAir)
        {
            EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::discardRlcPdu - lcid[" << lcid << "], discarded PDCP PDU " << pdcpSno << " in RLC PDU " << rlcSno << endl;
            pktDiscardCounterTotal_.discarded += 1;
        }
        // if the pdcp was entire and the set of rlc is empty, discard it
        if(rit->second.empty() && pit->second.hasArrivedAll){
            desc->rlcPdusPerSdu_.erase(pdcpSno);
        }
    }
    //remove discarded rlc pdu
    desc->rlcSdusPerPdu_.erase(rlcSno);

}

void PacketFlowManagerUe::insertMacPdu(inet::Ptr<const LteMacPdu> macPdu)
{
    EV << pfmType << "::insertMacPdu" << endl;

    /*
     * retreive the macPduId and the Lcid
     */
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if(len == 0)
        throw cRuntimeError("%s::macPduArrived - macPdu has no Rlc pdu! This, here, should not happen",pfmType.c_str());
    for(int i = 0; i < len; ++i)
    {
        auto rlcPdu= macPdu->getSdu(i);
        auto lteInfo = rlcPdu.getTag<FlowControlInfo>();
        int lcid = lteInfo ->getLcid();

        ConnectionMap::iterator cit = connectionMap_.find(lcid);
        if (cit == connectionMap_.end())
        {
            // this may occur after a handover, when data structures are cleared
            // EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::insertMacPdu - Logical CID " << lcid << " not present." << endl;
            throw cRuntimeError("%s::insertMacPdu - Logical CID %d not present. It must be initilized before", pfmType.c_str(), lcid);
            return;
        }


        // get the descriptor for this connection
        StatusDescriptor* desc = &cit->second;
        if (desc->macSdusPerPdu_.find(macPduId) != desc->macSdusPerPdu_.end())
            throw cRuntimeError("%s::insertMacPdu - MAC PDU ID %d already present for logical CID %d. Aborting", pfmType.c_str(), macPduId, lcid);


        auto macSdu = macPdu->getSdu(i).peekAtFront<LteRlcUmDataPdu>();
        unsigned int rlcSno = macSdu->getPduSequenceNumber();

        //
        auto aa= macPdu->getSdu(i); // all rlc pdus have the same lcid
        auto ee = aa.getTag<FlowControlInfo>();
        int ll = ee ->getLcid();
        EV << "ALE LCID: " << ll << endl;

        //


        EV << "MAC pdu: " << macPduId  <<  " has RLC pdu: " << rlcSno << endl;


        // ??????
        // record the associaton RLC PDU - MAC PDU only if the RLC PDU contains at least one entire SDU
        // the condition holds if the RLC PDU SN is stored in the data structure rlcSdusPerPdu_


        std::map<unsigned int, SequenceNumberSet>::iterator tit = desc->rlcSdusPerPdu_.find(rlcSno);
        if(tit == desc->rlcSdusPerPdu_.end())
           throw cRuntimeError("%s::insertMacPdu - RLC PDU ID %d not present in the status descriptor of lcid %d ", pfmType.c_str(), rlcSno, lcid);

        // store the MAC SDUs (RLC PDUs) included in the MAC PDU
        desc->macSdusPerPdu_[macPduId].insert(rlcSno);
        EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::insertMacPdu - lcid[" << lcid << "], insert RLC PDU " << rlcSno << " in MAC PDU " << macPduId << endl;


        // set the pdcp pdus related to thi RLC as sent over the air since this method is called after the MAC ID
        // has been inserted in the HARQBuffer
        SequenceNumberSet pdcpSet = tit->second;
        SequenceNumberSet::iterator pit = pdcpSet.begin();
        std::map<unsigned int, PdcpStatus>::iterator sdit;
        for (; pit != pdcpSet.end(); ++pit)
        {
            sdit = desc->pdcpStatus_.find(*pit);
            if(sdit == desc->pdcpStatus_.end())
                throw cRuntimeError("%s::insertMacPdu - PdcpStatus for PDCP sno [%d] not present, this should not happen. Abort", pfmType.c_str(), *pit);
            sdit->second.sentOverTheAir = true;
        }
    }
}

void PacketFlowManagerUe::macPduArrived(inet::Ptr<const LteMacPdu> macPdu)
{
    EV << pfmType << "::macPduArrived" << endl;
    /*
     * retreive the macPduId and the Lcid
     */
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if(len == 0)
        throw cRuntimeError("%s::macPduArrived - macPdu has no Rlc pdu! This, here, should not happen",pfmType.c_str());
    for(int i = 0; i < len; ++i)
     {
         auto rlcPdu= macPdu->getSdu(i);
         auto lteInfo = rlcPdu.getTag<FlowControlInfo>();
         int lcid = lteInfo ->getLcid();

        std::map<LogicalCid, StatusDescriptor>::iterator cit = connectionMap_.find(lcid);
        if (cit == connectionMap_.end())
        {
            // this may occur after a handover, when data structures are cleared
            // EV_FATAL << NOW << " node id "<< desc->nodeId_<< " PacketFlowManagerEnb::notifyHarqProcess - Logical CID " << lcid << " not present." << endl;
            throw cRuntimeError("%s::macPduArrived - Logical CID %d not present. It must be initilized before", pfmType.c_str(),lcid);
            return;
        }

        // get the descriptor for this connection
        StatusDescriptor* desc = &cit->second;

        EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::macPduArrived - Get MAC PDU ID [" << macPduId << "], which contains:" << endl;
        EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::macPduArrived - MAC PDU "<< macPduId << " of lcid " << lcid << " arrived." << endl;

        // === STEP 1 ==================================================== //
        // === recover the set of RLC PDU SN from the above MAC PDU ID === //

        //    unsigned int macPduId = desc->macPduPerProcess_[macPdu];


        if (macPduId == 0)
        {
            EV << NOW << " " << pfmType.c_str() <<"::insertMacPdu - The process does not contain entire SDUs" << endl;
            return;
        }

//    desc->macPduPerProcess_[macPdu] = 0; // reset

    std::map<unsigned int, SequenceNumberSet>::iterator mit = desc->macSdusPerPdu_.find(macPduId);
    if (mit == desc->macSdusPerPdu_.end())
        throw cRuntimeError("%s::macPduArrived - MAC PDU ID %d not present for logical CID %d. Aborting", pfmType.c_str(), macPduId, lcid);
    SequenceNumberSet rlcSnoSet = mit->second;

    auto macSdu = rlcPdu.peekAtFront<LteRlcUmDataPdu>();
    unsigned int rlcSno = macSdu->getPduSequenceNumber();

    if(rlcSnoSet.find(rlcSno) == rlcSnoSet.end())
        throw cRuntimeError("%s::macPduArrived - RLC sno [%d] not present in rlcSnoSet structure for MAC PDU ID %d not present for logical CID %d. Aborting", pfmType.c_str(),rlcSno, macPduId, lcid);

    // === STEP 2 ========================================================== //
    // === for each RLC PDU SN, recover the set of RLC SDU (PDCP PDU) SN === //

    SequenceNumberSet::iterator it = rlcSnoSet.begin();

    SequenceNumberSet pdcpTti; // in case there are many rlc per mac pdu, this set takes trace of the tti counted per pdpc (!?)
    for (; it != rlcSnoSet.end(); ++it)
    {
        // for each RLC PDU
        unsigned int rlcPduSno = *it;

        EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::macPduArrived - --> RLC PDU [" << rlcPduSno << "], which contains:" << endl;

        std::map<unsigned int, SequenceNumberSet>::iterator nit = desc->rlcSdusPerPdu_.find(rlcPduSno);
        if (nit == desc->rlcSdusPerPdu_.end())
            throw cRuntimeError("%s::macPduArrived - RLC PDU SN %d not present for logical CID %d. Aborting", pfmType.c_str(), rlcPduSno, lcid);
        SequenceNumberSet pdcpSnoSet = nit->second;


        // === STEP 3 ============================================================================ //
        // === Since a RLC SDU may be fragmented in more than one RLC PDU, thus it must be     === //
        // === retransmitted only if all fragments have been transmitted.                      === //
        // === For each RLC SDU (PDCP PDU) SN, recover the set of RLC PDU where it is included,=== //
        // === remove the above RLC PDU SN. If the set becomes empty, compute the delay if     === //
        // === all PDCP PDU fragments have been transmitted                                     === //

        SequenceNumberSet::iterator jt = pdcpSnoSet.begin();
        for (; jt != pdcpSnoSet.end(); ++jt)
        {
            // for each RLC SDU (PDCP PDU), get the set of RLC PDUs where it is included
            unsigned int pdcpPduSno = *jt;

            EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::macPduArrived - ----> PDCP PDU [" << pdcpPduSno << "]" << endl;

            std::map<unsigned int, SequenceNumberSet>::iterator oit = desc->rlcPdusPerSdu_.find(pdcpPduSno);
            if (oit == desc->rlcPdusPerSdu_.end())
                throw cRuntimeError("%s::macPduArrived - PDCP PDU SN %d not present for logical CID %d. Aborting", pfmType.c_str(), pdcpPduSno, lcid);

            // oit->second is the set of RLC PDU in which the PDCP PDU is contained

            // the RLC PDU SN must be present in the set
            SequenceNumberSet::iterator kt = oit->second.find(rlcPduSno);
            if (kt == oit->second.end())
                 throw cRuntimeError("%s::macPduArrived - RLC PDU SN %d not present in the set of PDCP PDU SN %d for logical CID %d. Aborting", pfmType.c_str(), pdcpPduSno, rlcPduSno, lcid);

            // the RLC PDU has been sent, so erase it from the set
            oit->second.erase(kt);

            std::map<unsigned int, PdcpStatus>::iterator pit = desc->pdcpStatus_.find(pdcpPduSno);
            if(pit == desc->pdcpStatus_.end())
                throw cRuntimeError("%s::macPduArrived - PdcpStatus for PDCP sno [%d] not present for lcid [%d], this should not happen. Abort", pfmType.c_str(), pdcpPduSno, lcid);

            // check whether the set is now empty
            if (desc->rlcPdusPerSdu_[pdcpPduSno].empty())
            {

                // set the time for pdcpPduSno
    //                if(pit->second.entryTime == 0) // change to entrytime negativo
    //                    throw cRuntimeError("%s::macPduArrived - PDCP PDU SN %d of Lcid %d has not an entry time timestamp, this should not happen. Aborting", pfmType.c_str(), pdcpPduSno, lcid);

                if(pit->second.hasArrivedAll && !pit->second.discardedAtRlc && !pit->second.discardedAtMac)
                { // the whole current pdcp seqNum has been received
                    EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::macPduArrived - ----> PDCP PDU [" << pdcpPduSno << "] has been completely sent, remove from PDCP buffer" << endl;

                    double time = (simTime() - pit->second.entryTime).dbl();
                    pdcpDelay.time += time;
                    pdcpDelay.pktCount += 1;

                    EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::macPduArrived - PDCP PDU "<< pdcpPduSno << " of lcid " << lcid << " acknowledged. Delay time: " << time << "s"<< endl;

    //                        ii.record(pdcpPduSno);
                    // update next sno
                    nextPdcpSno_ = pdcpPduSno+1;

                    // remove pdcp status
                    oit->second.clear();
                    desc->rlcPdusPerSdu_.erase(oit); // erase PDCP PDU SN
                }
            }
       }
    //        nit->second.clear();
    //        desc->rlcSdusPerPdu_.erase(nit); // erase RLC PDU SN
        // update next sno
        nextRlcSno_ = rlcPduSno+1;
    }

    mit->second.clear();
    desc->macSdusPerPdu_.erase(mit); // erase MAC PDU ID
    }
}

void PacketFlowManagerUe::discardMacPdu(const inet::Ptr<const LteMacPdu> macPdu)
{
    /*
     * retreive the macPduId and the Lcid
     */
    int macPduId = macPdu->getId();
    int len = macPdu->getSduArraySize();
    if(len == 0)
        throw cRuntimeError("%s::macPduArrived - macPdu has no Rlc pdu! This, here, should not happen",pfmType.c_str());
    for(int i = 0; i < len; ++i)
    {
        auto rlcPdu= macPdu->getSdu(i);
        auto lteInfo = rlcPdu.getTag<FlowControlInfo>();
        int lcid = lteInfo ->getLcid();

        ConnectionMap::iterator cit = connectionMap_.find(lcid);
        if (cit == connectionMap_.end())
        {
            // this may occur after a handover, when data structures are cleared
            // EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::notifyHarqProcess - Logical CID " << lcid << " not present." << endl;
            throw cRuntimeError("%s::discardMacPdu - Logical CID %d not present. It must be initilized before", pfmType.c_str(), lcid);
            return;
        }

        // get the descriptor for this connection
        StatusDescriptor* desc = &cit->second;

        EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::discardMacPdu - Get MAC PDU ID [" << macPduId << "], which contains:" << endl;
        EV_FATAL << NOW << "node id "<< desc->nodeId_-1025 << " " << pfmType.c_str() <<"::discardMacPdu - MAC PDU "<< macPduId << " of lcid " << lcid << " arrived." << endl;

        // === STEP 1 ==================================================== //
        // === recover the set of RLC PDU SN from the above MAC PDU ID === //

    //    unsigned int macPduId = desc->macPduPerProcess_[macPdu];

        if (macPduId == 0)
        {
            EV << NOW << " " << pfmType.c_str() <<"::discardMacPdu - The process does not contain entire SDUs" << endl;
            return;
        }

    //    desc->macPduPerProcess_[macPdu] = 0; // reset

        std::map<unsigned int, SequenceNumberSet>::iterator mit = desc->macSdusPerPdu_.find(macPduId);
        if (mit == desc->macSdusPerPdu_.end())
            throw cRuntimeError("%s::discardMacPdu - MAC PDU ID %d not present for logical CID %d. Aborting", pfmType.c_str(), macPduId, lcid);
        SequenceNumberSet rlcSnoSet = mit->second;

        auto macSdu = rlcPdu.peekAtFront<LteRlcUmDataPdu>();
        unsigned int rlcSno = macSdu->getPduSequenceNumber();

       if(rlcSnoSet.find(rlcSno) == rlcSnoSet.end())
           throw cRuntimeError("%s::macPduArrived - RLC sno [%d] not present in rlcSnoSet structure for MAC PDU ID %d not present for logical CID %d. Aborting", pfmType.c_str(),rlcSno, macPduId, lcid);

        // === STEP 2 ========================================================== //
        // === for each RLC PDU SN, recover the set of RLC SDU (PDCP PDU) SN === //

        SequenceNumberSet::iterator it = rlcSnoSet.begin();
        for (; it != rlcSnoSet.end(); ++it)
        {
            discardRlcPdu(lcid, *it, true);

        }

        mit->second.clear();
        desc->macSdusPerPdu_.erase(mit); // erase MAC PDU ID
    }
}


DiscardedPkts PacketFlowManagerUe::getDiscardedPkt()
{
//    DiscardedPkts pair;
//    pair.discarded = pktDiscardCounterTotal_;
//    pair.total = pdcp_->getPktCount();
    return pktDiscardCounterTotal_;
}


double PacketFlowManagerUe::getDelayStats()
{
    if(pdcpDelay.pktCount == 0)
        return 0;
    EV_FATAL << NOW << " " << pfmType.c_str() <<"::getDelayStats- Delay Stats total time: "<< pdcpDelay.time.dbl()*1000 << " pckcount: " <<pdcpDelay.pktCount   << endl;

    return (pdcpDelay.time.dbl()*1000)/pdcpDelay.pktCount;
}
void PacketFlowManagerUe::resetDelayCounter()
{
    pdcpDelay = {0,0};
}



void PacketFlowManagerUe::insertHarqProcess(LogicalCid lcid, unsigned int harqProcId, unsigned int macPduId)
{
//    ConnectionMap::iterator cit = connectionMap_.find(lcid);
//    if (cit == connectionMap_.end())
//    {
//        // this may occur after a handover, when data structures are cleared
//        EV << NOW << " " << pfmType.c_str() <<"::insertHarqProcess - Logical CID " << lcid << " not present." << endl;
//        return;
//    }
//
//    // get the descriptor for this connection
//    StatusDescriptor* desc = &cit->second;
//
//    // record the associaton MAC PDU - HARQ process only if the MAC PDU contains a RLC PDU that, in turn, contains at least one entire SDU
//    // the condition holds if the MAC PDU ID is stored in the data structure macSdusPerPdu_
//    if (desc->macSdusPerPdu_.find(macPduId) != desc->macSdusPerPdu_.end())
//    {
//        // store the MAC PDU ID included into the given HARQ process
//        desc->macPduPerProcess_[harqProcId] = macPduId;
//        EV << NOW << " " << pfmType.c_str() <<"::insertMacPdu - lcid[" << lcid << "], insert MAC PDU " << macPduId << " in HARQ PROCESS " << harqProcId << endl;
//    }
}


void PacketFlowManagerUe::finish()
{
}
