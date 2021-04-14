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
#include "stack/mac/layer/LteMacUe.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/scheduler/LteSchedulerUeUl.h"
#include "stack/mac/packet/LteRac_m.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/amc/UserTxParams.h"
#include "inet/networklayer/common/InterfaceEntry.h"
#include "inet/common/ModuleAccess.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "common/binder/Binder.h"
#include "stack/phy/layer/LtePhyBase.h"
#include "stack/mac/packet/LteMacSduRequest.h"
#include "stack/rlc/um/LteRlcUm.h"
#include "common/LteCommon.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/rlc/am/packet/LteRlcAmPdu_m.h"

Define_Module(LteMacUe);

using namespace inet;
using namespace omnetpp;

LteMacUe::LteMacUe() :
    LteMacBase()
{
    firstTx = false;

    currentHarq_ = 0;
//    periodCounter_ = 0;
//    expirationCounter_ = 0;
    racRequested_ = false;
    bsrTriggered_ = false;
    requestedSdus_ = 0;

    debugHarq_ = false;

    // TODO setup from NED
    racBackoffTimer_ = 0;
    maxRacTryouts_ = 0;
    currentRacTry_ = 0;
    minRacBackoff_ = 0;
    maxRacBackoff_ = 1;
    raRespTimer_ = 0;
    raRespWinStart_ = 3;
    nodeType_ = UE;

    // KLUDGE: this was unitialized, this is just a guess
    harqProcesses_ = 8;
}

LteMacUe::~LteMacUe()
{
    std::map<double, LteSchedulerUeUl*>::iterator sit;
    for (sit = lcgScheduler_.begin(); sit != lcgScheduler_.end(); ++sit)
        delete sit->second;

    auto git = schedulingGrant_.begin();
    for ( ; git != schedulingGrant_.end(); ++git)
    {
        if (git->second!=nullptr)
        {
//            delete git->second;
            git->second = nullptr;
        }
    }
}

void LteMacUe::initialize(int stage)
{
    LteMacBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL)
    {
        cqiDlMuMimo0_ = registerSignal("cqiDlMuMimo0");
        cqiDlMuMimo1_ = registerSignal("cqiDlMuMimo1");
        cqiDlMuMimo2_ = registerSignal("cqiDlMuMimo2");
        cqiDlMuMimo3_ = registerSignal("cqiDlMuMimo3");
        cqiDlMuMimo4_ = registerSignal("cqiDlMuMimo4");

        cqiDlTxDiv0_ = registerSignal("cqiDlTxDiv0");
        cqiDlTxDiv1_ = registerSignal("cqiDlTxDiv1");
        cqiDlTxDiv2_ = registerSignal("cqiDlTxDiv2");
        cqiDlTxDiv3_ = registerSignal("cqiDlTxDiv3");
        cqiDlTxDiv4_ = registerSignal("cqiDlTxDiv4");

        cqiDlSpmux0_ = registerSignal("cqiDlSpmux0");
        cqiDlSpmux1_ = registerSignal("cqiDlSpmux1");
        cqiDlSpmux2_ = registerSignal("cqiDlSpmux2");
        cqiDlSpmux3_ = registerSignal("cqiDlSpmux3");
        cqiDlSpmux4_ = registerSignal("cqiDlSpmux4");

        cqiDlSiso0_ = registerSignal("cqiDlSiso0");
        cqiDlSiso1_ = registerSignal("cqiDlSiso1");
        cqiDlSiso2_ = registerSignal("cqiDlSiso2");
        cqiDlSiso3_ = registerSignal("cqiDlSiso3");
        cqiDlSiso4_ = registerSignal("cqiDlSiso4");
    }
//    else if (stage == INITSTAGE_PHYSICAL_ENVIRONMENT_2)
//    {
//        // primary cell
//        LteChannelModel* chanModel = check_and_cast<LteChannelModel*>(getParentModule()->getSubmodule("channelModel", 0));
//        double carrierFreq = chanModel->getCarrierFrequency();
//        channelModel_[carrierFreq] = chanModel;
//
//        for (int index=1; index<chanModel->getVectorSize(); index++)
//        {
//            chanModel = check_and_cast<LteChannelModel*>(getParentModule()->getSubmodule("channelModel", 0));
//            carrierFreq = chanModel->getCarrierFrequency();
//            channelModel_[carrierFreq] = chanModel;
//        }
//    }
    else if (stage == INITSTAGE_LINK_LAYER)
    {
        if (strcmp(getFullName(),"nrMac") == 0)
            cellId_ = getAncestorPar("nrMasterId");
        else
            cellId_ = getAncestorPar("masterId");
    }
    else if (stage == INITSTAGE_NETWORK_LAYER)
    {
        if (strcmp(getFullName(),"nrMac") == 0)
            nodeId_ = getAncestorPar("nrMacNodeId");
        else
            nodeId_ = getAncestorPar("macNodeId");

        /* Insert UeInfo in the Binder */
        UeInfo* info = new UeInfo();
        info->id = nodeId_;            // local mac ID
        info->cellId = cellId_;        // cell ID
        info->init = false;            // flag for phy initialization
        info->ue = this->getParentModule()->getParentModule();  // reference to the UE module


        // get the reference to the PHY layer
        if (isNrUe(nodeId_))
            info->phy = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("nrPhy"));
        else
            info->phy = check_and_cast<LtePhyBase*>(getParentModule()->getSubmodule("phy"));

        phy_ = info->phy;

        binder_->addUeInfo(info);

        if (cellId_ > 0)
        {
            LteAmc *amc = check_and_cast<LteMacEnb *>(getMacByMacNodeId(cellId_))->getAmc();
            amc->attachUser(nodeId_, UL);
            amc->attachUser(nodeId_, DL);
        }

        // find interface entry and use its address
        IInterfaceTable *interfaceTable = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        // TODO: how do we find the LTE interface?
        InterfaceEntry * interfaceEntry = interfaceTable->findInterfaceByName("wlan");
        if(interfaceEntry == nullptr)
            throw new cRuntimeError("no interface entry for lte interface - cannot bind node %i", nodeId_);


        Ipv4InterfaceData* ipv4if = interfaceEntry->getProtocolData<Ipv4InterfaceData>();
        if(ipv4if == nullptr)
            throw new cRuntimeError("no Ipv4 interface data - cannot bind node %i", nodeId_);
        binder_->setMacNodeId(ipv4if->getIPAddress(), nodeId_);

        // Register the "ext" interface, if present
        if (hasPar("enableExtInterface") && getAncestorPar("enableExtInterface").boolValue())
        {
            // get address of the localhost to enable forwarding
            Ipv4Address extHostAddress = Ipv4Address(getAncestorPar("extHostAddress").stringValue());
            binder_->setMacNodeId(extHostAddress, nodeId_);
        }
    }
    else if (stage == inet::INITSTAGE_TRANSPORT_LAYER)
    {
        const std::map<double, LteChannelModel*>* channelModels = phy_->getChannelModels();
        std::map<double, LteChannelModel*>::const_iterator it = channelModels->begin();
        for (; it != channelModels->end(); ++it)
        {
            lcgScheduler_[it->first] = new LteSchedulerUeUl(this, it->first);
        }
    }
    else if (stage == inet::INITSTAGE_LAST)
    {
        /* Start TTI tick */
        // the period is equal to the minimum period according to the numerologies used by the carriers in this node
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);                                              // TTI TICK after other messages
        ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(binder_->getUeMaxNumerologyIndex(nodeId_));
        scheduleAt(NOW + ttiPeriod_, ttiTick_);

        const std::set<NumerologyIndex>* numerologyIndexSet = binder_->getUeNumerologyIndex(nodeId_);
        if (numerologyIndexSet != NULL)
        {
            std::set<NumerologyIndex>::const_iterator it = numerologyIndexSet->begin();
            for ( ; it != numerologyIndexSet->end(); ++it)
            {
                // set periodicity for this carrier according to its numerology
                NumerologyPeriodCounter info;
                info.max = 1 << (binder_->getUeMaxNumerologyIndex(nodeId_) - *it); // 2^(maxNumerologyIndex - numerologyIndex)
                info.current = info.max - 1;
                numerologyPeriodCounter_[*it] = info;
            }
        }
    }
}

void LteMacUe::handleMessage(cMessage* msg)
{
    if (msg->isSelfMessage())
    {
        if (strcmp(msg->getName(), "flushHarqMsg") == 0)
        {
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

    auto git = schedulingGrant_.begin();
    for (; git != schedulingGrant_.end(); ++git)
    {
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(git->first)) > 0)
            continue;

        if (git->second == NULL)
            continue;

        for (int cw=0; cw<git->second->getGrantedCwBytesArraySize(); cw++)
            allocatedBytes.push_back(git->second->getGrantedCwBytes(cw));
    }

    // Ask for a MAC sdu for each scheduled user on each codeword
    std::map<double, LteMacScheduleList*>::iterator cit = scheduleList_.begin();
    for (; cit != scheduleList_.end(); ++cit)
    {
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(cit->first)) > 0)
            continue;

        LteMacScheduleList::const_iterator it;
        for (it = cit->second->begin(); it != cit->second->end(); it++)
        {
            MacCid destCid = it->first.first;
            Codeword cw = it->first.second;
            MacNodeId destId = MacCidToNodeId(destCid);

            std::pair<MacCid,Codeword> key(destCid, cw);
            LteMacScheduleList* scheduledBytesList = lcgScheduler_[cit->first]->getScheduledBytesList();
            LteMacScheduleList::const_iterator bit = scheduledBytesList->find(key);

            // consume bytes on this codeword
            if (bit == scheduledBytesList->end())
                throw cRuntimeError("LteMacUe::macSduRequest - cannot find entry in scheduledBytesList");
            else
            {
                allocatedBytes[cw] -= bit->second;

                EV << NOW <<" LteMacUe::macSduRequest - cid[" << destCid << "] - sdu size[" << bit->second << "B] - " << allocatedBytes[cw] << " bytes left on codeword " << cw << endl;

                // send the request message to the upper layer
                // TODO: Replace by tag
                auto pkt = new Packet("LteMacSduRequest");
                auto macSduRequest = makeShared<LteMacSduRequest>();
                macSduRequest->setChunkLength(b(1)); // TODO: should be 0
                macSduRequest->setUeId(destId);
                macSduRequest->setLcid(MacCidToLcid(destCid));
                macSduRequest->setSduSize(bit->second);
                pkt->insertAtFront(macSduRequest);
                *(pkt->addTag<FlowControlInfo>()) = connDesc_[destCid];
                sendUpperPackets(pkt);

                numRequestedSdus++;
            }
        }
    }

    EV << "------ END LteMacUe::macSduRequest ------\n";
    return numRequestedSdus;
}

bool LteMacUe::bufferizePacket(cPacket* pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    if (pkt->getBitLength() <= 1) { // no data in this packet - should not be buffered
        delete pkt;
        return false;
    }

    pkt->setTimestamp();           // add time-stamp with current time to packet

    auto lteInfo = pkt->getTag<FlowControlInfo>();

    // obtain the cid from the packet informations
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // this packet is used to signal the arrival of new data in the RLC buffers
    if (checkIfHeaderType<LteRlcPduNewData>(pkt))
    {
        // update the virtual buffer for this connection

        // build the virtual packet corresponding to this incoming packet
        pkt->popAtFront<LteRlcPduNewData>();
        auto rlcSdu = pkt->peekAtFront<LteRlcSdu>();
        PacketInfo vpkt(rlcSdu->getLengthMainPacket(), pkt->getTimestamp());

        LteMacBufferMap::iterator it = macBuffers_.find(cid);
        if (it == macBuffers_.end())
        {
            LteMacBuffer* vqueue = new LteMacBuffer();
            vqueue->pushBack(vpkt);
            macBuffers_[cid] = vqueue;

            // make a copy of lte control info and store it to traffic descriptors map
            FlowControlInfo toStore(*lteInfo);
            connDesc_[cid] = toStore;
            // register connection to lcg map.
            LteTrafficClass tClass = (LteTrafficClass) lteInfo->getTraffic();

            lcgMap_.insert(LcgPair(tClass, CidBufferPair(cid, macBuffers_[cid])));

            EV << "LteMacBuffers : Using new buffer on node: " <<
            MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Bytes in the Queue: " <<
            vqueue->getQueueOccupancy() << "\n";
        }
        else
        {
            LteMacBuffer* vqueue = NULL;
            LteMacBufferMap::iterator it = macBuffers_.find(cid);
            if (it != macBuffers_.end())
                vqueue = it->second;

            if (vqueue != NULL)
            {
                vqueue->pushBack(vpkt);

                EV << "LteMacBuffers : Using old buffer on node: " <<
                MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
                vqueue->getQueueOccupancy() << "\n";
            }
            else
                throw cRuntimeError("LteMacUe::bufferizePacket - cannot find mac buffer for cid %d", cid);
        }

        delete pkt;
        return true;    // notify the activation of the connection
    }

    // this is a MAC SDU, bufferize it in the MAC buffer

    LteMacBuffers::iterator it = mbuf_.find(cid);
    if (it == mbuf_.end())
    {
        // Queue not found for this cid: create
        LteMacQueue* queue = new LteMacQueue(queueSize_);

        queue->pushBack(pkt);

        mbuf_[cid] = queue;

        EV << "LteMacBuffers : Using new buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }
    else
    {
        // Found
        LteMacQueue* queue = it->second;
        if (!queue->pushBack(pkt))
        {
            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
            if (lteInfo->getDirection()==DL)
            {
                emit(macBufferOverflowDl_,sample);
            }
            else
            {
                emit(macBufferOverflowUl_,sample);
            }

            EV << "LteMacBuffers : Dropped packet: queue" << cid << " is full\n";
            delete pkt;
            return false;
        }

        EV << "LteMacBuffers : Using old buffer on node: " <<
        MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << "(cid: " << cid << "), Space left in the Queue: " <<
        queue->getQueueSize() - queue->getByteLength() << "\n";
    }

    return true;
}

void LteMacUe::macPduMake(MacCid cid)
{
    int64 size = 0;

    macPduList_.clear();

    //  Build a MAC pdu for each scheduled user on each codeword
    std::map<double, LteMacScheduleList*>::iterator cit = scheduleList_.begin();
    for (; cit != scheduleList_.end(); ++cit)
    {
        double carrierFreq = cit->first;
        Packet *macPkt = nullptr;

        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        LteMacScheduleList::const_iterator it;
        for (it = cit->second->begin(); it != cit->second->end(); it++)
        {
            MacCid destCid = it->first.first;
            Codeword cw = it->first.second;

            // from an UE perspective, the destId is always the one of the eNb
            MacNodeId destId = getMacCellId();

            std::pair<MacNodeId, Codeword> pktId = std::pair<MacNodeId, Codeword>(destId, cw);
            unsigned int sduPerCid = it->second;

//            if (sduPerCid == 0 && !bsrTriggered_)
//                continue;

            if (macPduList_.find(carrierFreq) == macPduList_.end())
            {
                MacPduList newList;
                macPduList_[carrierFreq] = newList;
            }
            auto pit = macPduList_[carrierFreq].find(pktId);

            // No packets for this user on this codeword
            if (pit == macPduList_[carrierFreq].end())
            {
                macPkt = new Packet("LteMacPdu");
                auto header = makeShared<LteMacPdu>();
                header->setHeaderLength(MAC_HEADER);
                macPkt->insertAtFront(header);

                macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
                macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(schedulingGrant_[carrierFreq]->getUserTxParams()->dup());
                macPkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);

                //macPkt->setControlInfo(uinfo);
                macPkt->setTimestamp(NOW);
                macPduList_[carrierFreq][pktId] = macPkt;
            }
            else
            {
                macPkt = pit->second;
            }

            while (sduPerCid > 0)
            {
                // Add SDU to PDU
                // Find Mac Pkt

                if (mbuf_.find(destCid) == mbuf_.end())
                    throw cRuntimeError("Unable to find mac buffer for cid %d", destCid);

                if (mbuf_[destCid]->isEmpty())
                    throw cRuntimeError("Empty buffer for cid %d, while expected SDUs were %d", destCid, sduPerCid);

                auto pkt = check_and_cast<Packet *>(mbuf_[destCid]->popFront());
                drop(pkt);

                auto header = macPkt->removeAtFront<LteMacPdu>();
                header->pushSdu(pkt);
                macPkt->insertAtFront(header);
                sduPerCid--;
            }
            // consider virtual buffers to compute BSR size
            size += macBuffers_[destCid]->getQueueOccupancy();
        }
    }

    //  Put MAC pdus in H-ARQ buffers

    std::map<double, MacPduList>::iterator lit;
    for (lit = macPduList_.begin(); lit != macPduList_.end(); ++lit)
    {
        double carrierFreq = lit->first;
        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end())
        {
            HarqTxBuffers newHarqTxBuffers;
            harqTxBuffers_[carrierFreq] = newHarqTxBuffers;
        }
        HarqTxBuffers& harqTxBuffers = harqTxBuffers_[carrierFreq];



        for (MacPduList::iterator pit = lit->second.begin(); pit != lit->second.end(); pit++)
        {

            MacNodeId destId = pit->first.first;
            Codeword cw = pit->first.second;

            // Check if the HarqTx buffer already exists for the destId
            // Get a reference for the destId TXBuffer
            LteHarqBufferTx* txBuf;
            HarqTxBuffers::iterator hit = harqTxBuffers.find(destId);
            if ( hit != harqTxBuffers.end() )
            {
                // The tx buffer already exists
                txBuf = hit->second;
            }
            else
            {
                // the tx buffer does not exist yet for this mac node id, create one
                // FIXME: hb is never deleted
                LteHarqBufferTx* hb = new LteHarqBufferTx((unsigned int) ENB_TX_HARQ_PROCESSES, this,
                    (LteMacBase*) getMacByMacNodeId(cellId_));
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }

            //
     //        UnitList txList = (txBuf->firstAvailable());
     //        LteHarqProcessTx * currProc = txBuf->getProcess(currentHarq_);


            // search for an empty unit within current harq process
            UnitList txList = txBuf->getEmptyUnits(currentHarq_);
            EV << "LteMacUe::macPduMake - [Used Acid=" << (unsigned int)txList.first << "] , [curr=" << (unsigned int)currentHarq_ << "]" << endl;

            auto macPkt = pit->second;

            // BSR related operations

            // according to the TS 36.321 v8.7.0, when there are uplink resources assigned to the UE, a BSR
            // has to be send even if there is no data in the user's queues. In few words, a BSR is always
            // triggered and has to be send when there are enough resources

            // TODO implement differentiated BSR attach
            //
            //            // if there's enough space for a LONG BSR, send it
            //            if( (availableBytes >= LONG_BSR_SIZE) ) {
            //                // Create a PDU if data were not scheduled
            //                if (pdu==0)
            //                    pdu = new LteMacPdu();
            //
            //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
            //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - Node %d, sending a Long BSR...\n",NOW,nodeId);
            //
            //                // create a full BSR
            //                pdu->ctrlPush(fullBufferStatusReport());
            //
            //                // do not reset BSR flag
            //                mac_->bsrTriggered() = true;
            //
            //                availableBytes -= LONG_BSR_SIZE;
            //
            //            }
            //
            //            // if there's space only for a SHORT BSR and there are scheduled flows, send it
            //            else if( (mac_->bsrTriggered() == true) && (availableBytes >= SHORT_BSR_SIZE) && (highestBackloggedFlow != -1) ) {
            //
            //                // Create a PDU if data were not scheduled
            //                if (pdu==0)
            //                    pdu = new LteMacPdu();
            //
            //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
            //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - Node %d, sending a Short/Truncated BSR...\n",NOW,nodeId);
            //
            //                // create a short BSR
            //                pdu->ctrlPush(shortBufferStatusReport(highestBackloggedFlow));
            //
            //                // do not reset BSR flag
            //                mac_->bsrTriggered() = true;
            //
            //                availableBytes -= SHORT_BSR_SIZE;
            //
            //            }
            //            // if there's a BSR triggered but there's not enough space, collect the appropriate statistic
            //            else if(availableBytes < SHORT_BSR_SIZE && availableBytes < LONG_BSR_SIZE) {
            //                Stat::put(LTE_BSR_SUPPRESSED_NODE,nodeId,1.0);
            //                Stat::put(LTE_BSR_SUPPRESSED_CELL,mac_->cellId(),1.0);
            //            }
            //            Stat::put (LTE_GRANT_WASTED_BYTES_UL, nodeId, availableBytes);
            //        }
            //
            //        // 4) PDU creation
            //
            //        if (pdu!=0) {
            //
            //            pdu->cellId() = mac_->cellId();
            //            pdu->nodeId() = nodeId;
            //            pdu->direction() = mac::UL;
            //            pdu->error() = false;
            //
            //            if(LteDebug::trace("LteSchedulerUeUl::schedule"))
            //                fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - Node %d, creating uplink PDU.\n", NOW, nodeId);
            //
            //        }

            auto header = macPkt->removeAtFront<LteMacPdu>();
            if (bsrTriggered_)
            {
                MacBsr* bsr = new MacBsr();

                bsr->setTimestamp(simTime().dbl());
                bsr->setSize(size);
                header->pushCe(bsr);

                bsrTriggered_ = false;
                EV << "LteMacUe::macPduMake - BSR with size " << size << "created" << endl;
            }

            // insert updated MacPdu
            macPkt->insertAtFront(header);

            EV << "LteMacUe: pduMaker created PDU: " << macPkt->str() << endl;

            // TODO: harq test
            // pdu transmission here (if any)
            // txAcid has HARQ_NONE for non-fillable codeword, acid otherwise
            if (txList.second.empty())
            {
                EV << "macPduMake() : no available process for this MAC pdu in TxHarqBuffer" << endl;
                delete macPkt;
            }
            else
            {
                txBuf->insertPdu(txList.first,cw, macPkt);
            }
        }
    }
}

void LteMacUe::macPduUnmake(cPacket* pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto macPkt = pkt->removeAtFront<LteMacPdu>();
    while (macPkt->hasSdu())
    {
        // Extract and send SDU
        auto upPkt = macPkt->popSdu();
        take(upPkt);

        EV << "LteMacBase: pduUnmaker extracted SDU" << endl;

        // store descriptor for the incoming connection, if not already stored
        auto lteInfo = upPkt->getTag<FlowControlInfo>();
        MacNodeId senderId = lteInfo->getSourceId();
        LogicalCid lcid = lteInfo->getLcid();
        MacCid cid = idToMacCid(senderId, lcid);
        if (connDescIn_.find(cid) == connDescIn_.end())
        {
            FlowControlInfo toStore(*lteInfo);
            connDescIn_[cid] = toStore;
        }
        sendUpperPackets(upPkt);
    }

    pkt->insertAtFront(macPkt);

    ASSERT(pkt->getOwner() == this);
    delete pkt;
}

void LteMacUe::handleUpperMessage(cPacket* pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    bool isLteRlcPduNewDataInd = checkIfHeaderType<LteRlcPduNewData>(pkt);

    // bufferize packet
    bufferizePacket(pkt);

    if(!isLteRlcPduNewDataInd){
        requestedSdus_--;
        ASSERT(requestedSdus_ >= 0);
        // build a MAC PDU only after all MAC SDUs have been received from RLC
        if (requestedSdus_ == 0)
        {
            // make PDU and BSR (if necessary)
            macPduMake();
            // update current harq process id
            EV << NOW << " LteMacUe::handleMessage - incrementing counter for HARQ processes " << (unsigned int)currentHarq_ << " --> " << (currentHarq_+1)%harqProcesses_ << endl;
            currentHarq_ = (currentHarq_+1) % harqProcesses_;
        }
    }
}

void LteMacUe::handleSelfMessage()
{
    EV << "----- UE MAIN LOOP -----" << endl;

    // extract pdus from all harqrxbuffers and pass them to unmaker
    std::map<double, HarqRxBuffers>::iterator mit = harqRxBuffers_.begin();
    std::map<double, HarqRxBuffers>::iterator met = harqRxBuffers_.end();
    for (; mit != met; mit++)
    {
        HarqRxBuffers::iterator hit = mit->second.begin();
        HarqRxBuffers::iterator het = mit->second.end();

        std::list<Packet*> pduList;

        for (; hit != het; ++hit)
        {
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mit->first)) > 0)
                 continue;

            pduList=hit->second->extractCorrectPdus();
            while (! pduList.empty())
            {
                auto pdu=pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    EV << NOW << "LteMacUe::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int)currentHarq_ << endl;
    // updating current HARQ process for next TTI

    // no grant available - if user has backlogged data, it will trigger scheduling request
    // no harq counter is updated since no transmission is sent.

    bool noSchedulingGrants = true;
    auto git = schedulingGrant_.begin();
    auto get = schedulingGrant_.end();
    for (; git != get; ++git)
    {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(git->first)) > 0)
            continue;

        if (git->second != nullptr)
        {
            noSchedulingGrants = false;
            break;
        }
    }

    if (noSchedulingGrants)
    {
        EV << NOW << " LteMacUe::handleSelfMessage " << nodeId_ << " NO configured grant" << endl;

//        if (!bsrTriggered_)
//        {
        // if necessary, a RAC request will be sent to obtain a grant
        checkRAC();
//        }
        // TODO ensure all operations done  before return ( i.e. move H-ARQ rx purge before this point)
    }
    else
    {
        bool periodicGrant = false;
        bool checkRac = false;
        bool skip = false;
        for (git = schedulingGrant_.begin(); git != get; ++git)
        {
            if (git->second != nullptr && git->second->getPeriodic())
            {
                periodicGrant = true;
                double carrierFreq = git->first;

                // Periodic checks
                if(--expirationCounter_[carrierFreq] < 0)
                {
                    // Periodic grant is expired
//                    delete git->second;
                    git->second = nullptr;
                    // if necessary, a RAC request will be sent to obtain a grant
//                    checkRAC();
                    checkRac = true;
                    //return;
                }
                else if (--periodCounter_[carrierFreq]>0)
                {
                    skip = true;
//                    return;
                }
                else
                {
                    // resetting grant period
                    periodCounter_[carrierFreq]=git->second->getPeriod();
                    // this is periodic grant TTI - continue with frame sending
                    checkRac = false;
                    skip = false;
                    break;
                }
            }
        }
        if (periodicGrant)
        {
            if (checkRac)
                checkRAC();
            else
            {
                if (skip)
                    return;
            }
        }
    }

    scheduleList_.clear();
    requestedSdus_ = 0;
    if (!noSchedulingGrants) // if a grant is configured for at least one carrier
    {
        if(!firstTx)
        {
            EV << "\t currentHarq_ counter initialized " << endl;
            firstTx=true;
            // the eNb will receive the first pdu in 2 TTI, thus initializing acid to 0
            currentHarq_ = UE_TX_HARQ_PROCESSES - 2;
        }
//        EV << "\t " << schedulingGrant_ << endl;

//        //! \TEST  Grant Synchronization check
//        if (!(schedulingGrant_->getPeriodic()))
//        {
//            if ( false /* TODO currentHarq!=grant_->getAcid()*/)
//            {
//                EV << NOW << "FATAL! Ue " << nodeId_ << " Current Process is " << (int)currentHarq << " while Stored grant refers to acid " << /*(int)grant_->getAcid() << */  ". Aborting.   " << endl;
//                abort();
//            }
//        }

        // TODO check if current grant is "NEW TRANSMISSION" or "RETRANSMIT" (periodic grants shall always be "newtx"
//        if ( false/*!grant_->isNewTx() && harqQueue_->rtx(currentHarq) */)
//        {
        //        if ( LteDebug:r:trace("LteMacUe::newSubFrame") )
        //            fprintf (stderr,"%.9f UE: [%d] Triggering retransmission for acid %d\n",NOW,nodeId_,currentHarq);
        //        // triggering retransmission --- nothing to do here, really!
//        } else {
        // buffer drop should occour here.

        EV << NOW << " LteMacUe::handleSelfMessage " << nodeId_ << " entered scheduling" << endl;

        bool retx = false;

        HarqTxBuffers::iterator it2;
        std::map<double, HarqTxBuffers>::iterator mtit;
        for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
        {
            double carrierFrequency = mtit->first;

            // skip if this is not the turn of this carrier
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                continue;

            // skip if no grant is configured for this carrier
            if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end() || schedulingGrant_[carrierFrequency] == NULL)
                continue;

            LteHarqBufferTx * currHarq;
            for(it2 = mtit->second.begin(); it2 != mtit->second.end(); it2++)
            {
                EV << "\t Looking for retx in acid " << (unsigned int)currentHarq_ << endl;
                currHarq = it2->second;

                // check if the current process has unit ready for retx
                retx = currHarq->getProcess(currentHarq_)->hasReadyUnits();
                CwList cwListRetx = currHarq->getProcess(currentHarq_)->readyUnitsIds();

                EV << "\t [process=" << (unsigned int)currentHarq_ << "] , [retx=" << ((retx)?"true":"false")
                   << "] , [n=" << cwListRetx.size() << "]" << endl;

                // if a retransmission is needed
                if(retx)
                {
                    UnitList signal;
                    signal.first=currentHarq_;
                    signal.second = cwListRetx;
                    currHarq->markSelected(signal,schedulingGrant_[carrierFrequency]->getUserTxParams()->getLayers().size());
                }
            }
        }
        // if no retx is needed, proceed with normal scheduling
        if(!retx)
        {
            std::map<double, LteSchedulerUeUl*>::iterator sit;
            for (sit = lcgScheduler_.begin(); sit != lcgScheduler_.end(); ++sit)
            {
                double carrierFrequency = sit->first;

                // skip if this is not the turn of this carrier
                if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                    continue;

                // skip if no grant is configured for this carrier
                if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end() || schedulingGrant_[carrierFrequency] == NULL)
                    continue;

                LteSchedulerUeUl* carrierLcgScheduler = sit->second;
                LteMacScheduleList* carrierScheduleList = carrierLcgScheduler->schedule();
                scheduleList_[carrierFrequency] = carrierScheduleList;
            }

            requestedSdus_ = macSduRequest();
            if (requestedSdus_ == 0)
            {
                // no data to send, but if bsrTriggered is set, send a BSR
                macPduMake();
            }

        }

        // Message that triggers flushing of Tx H-ARQ buffers for all users
        // This way, flushing is performed after the (possible) reception of new MAC PDUs
        cMessage* flushHarqMsg = new cMessage("flushHarqMsg");
        flushHarqMsg->setSchedulingPriority(1);        // after other messages
        scheduleAt(NOW, flushHarqMsg);
    }

    //============================ DEBUG ==========================
    if (debugHarq_)
    {
        // TODO make this part optional to save computations
        std::map<double, HarqTxBuffers>::iterator mtit;
        for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
        {
            EV << "\n carrier[ " << mtit->first << "] htxbuf.size " << mtit->second.size() << endl;

            HarqTxBuffers::iterator it;
            int cntOuter = 0;
            int cntInner = 0;
            for(it = mtit->second.begin(); it != mtit->second.end(); it++)
            {
                LteHarqBufferTx* currHarq = it->second;
                BufferStatus harqStatus = currHarq->getBufferStatus();
                BufferStatus::iterator jt = harqStatus.begin(), jet= harqStatus.end();

                EV << "\t cicloOuter " << cntOuter << " - bufferStatus.size=" << harqStatus.size() << endl;
                for(; jt != jet; ++jt)
                {
                    EV << "\t\t cicloInner " << cntInner << " - jt->size=" << jt->size()
                       << " - statusCw(0/1)=" << jt->at(0).second << "/" << jt->at(1).second << endl;
                }
            }
        }
    }
    //======================== END DEBUG ==========================

    if (requestedSdus_ == 0)
    {
        // update current harq process id
        currentHarq_ = (currentHarq_+1) % harqProcesses_;
    }

    EV << "--- END UE MAIN LOOP ---" << endl;
}

void
LteMacUe::macHandleGrant(cPacket* pktAux)
{
    EV << NOW << " LteMacUe::macHandleGrant - UE [" << nodeId_ << "] - Grant received" << endl;

    auto pkt = check_and_cast<inet::Packet*> (pktAux);
    auto grant = pkt->popAtFront<LteSchedulingGrant>();

    // delete old grant
    auto userInfo = pkt->getTag<UserControlInfo>();
    double carrierFrequency = userInfo->getCarrierFrequency();

    EV << NOW << " LteMacUe::macHandleGrant - Direction: " << dirToA(grant->getDirection()) << " Carrier: " << carrierFrequency << endl;

    if (schedulingGrant_.find(carrierFrequency) != schedulingGrant_.end() && schedulingGrant_[carrierFrequency]!=nullptr)
    {
//        delete schedulingGrant_[carrierFrequency];
        schedulingGrant_[carrierFrequency] = nullptr;
    }

    // store received grant
    schedulingGrant_[carrierFrequency] = grant;

    if (grant->getPeriodic())
    {
        periodCounter_[carrierFrequency] = grant->getPeriod();
        expirationCounter_[carrierFrequency] = grant->getExpiration();
    }

    EV << NOW << "Node " << nodeId_ << " received grant of blocks " << grant->getTotalGrantedBlocks()
       << ", bytes " << grant->getGrantedCwBytes(0) << endl;

    // clearing pending RAC requests
    racRequested_=false;

    delete pkt;
}

void
LteMacUe::macHandleRac(cPacket* pktAux)
{
    auto pkt = check_and_cast<inet::Packet*> (pktAux);
    auto racPkt = pkt->peekAtFront<LteRac>();

    if (racPkt->getSuccess())
    {
        EV << "LteMacUe::macHandleRac - Ue " << nodeId_ << " won RAC" << endl;
        // is RAC is won, BSR has to be sent
        bsrTriggered_=true;
        // reset RAC counter
        currentRacTry_=0;
        //reset RAC backoff timer
        racBackoffTimer_=0;
    }
    else
    {
        // RAC has failed
        if (++currentRacTry_ >= maxRacTryouts_)
        {
            EV << NOW << " Ue " << nodeId_ << ", RAC reached max attempts : " << currentRacTry_ << endl;
            // no more RAC allowed
            //! TODO flush all buffers here
            //reset RAC counter
            currentRacTry_=0;
            //reset RAC backoff timer
            racBackoffTimer_=0;
        }
        else
        {
            // recompute backoff timer
            racBackoffTimer_= uniform(minRacBackoff_,maxRacBackoff_);
            EV << NOW << " Ue " << nodeId_ << " RAC attempt failed, backoff extracted : " << racBackoffTimer_ << endl;
        }
    }
    delete pkt;
}

void
LteMacUe::checkRAC()
{
    EV << NOW << " LteMacUe::checkRAC , Ue  " << nodeId_ << ", racTimer : " << racBackoffTimer_ << " maxRacTryOuts : " << maxRacTryouts_
       << ", raRespTimer:" << raRespTimer_ << endl;

    if (racBackoffTimer_>0)
    {
        racBackoffTimer_--;
        return;
    }

    if(raRespTimer_>0)
    {
        // decrease RAC response timer
        raRespTimer_--;
        EV << NOW << " LteMacUe::checkRAC - waiting for previous RAC requests to complete (timer=" << raRespTimer_ << ")" << endl;
        return;
    }

    //     Avoids double requests whithin same TTI window
    if (racRequested_)
    {
        EV << NOW << " LteMacUe::checkRAC - double RAC request" << endl;
        racRequested_=false;
        return;
    }

    bool trigger=false;

    LteMacBufferMap::const_iterator it;

    for (it = macBuffers_.begin(); it!=macBuffers_.end();++it)
    {
        if (!(it->second->isEmpty()))
        {
            trigger = true;
            break;
        }
    }

    if (!trigger)
    EV << NOW << "Ue " << nodeId_ << ",RAC aborted, no data in queues " << endl;

    if ((racRequested_=trigger))
    {
        auto pkt = new Packet("RacRequest");

        auto racReq = makeShared<LteRac>();
        pkt->insertAtFront(racReq);

        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(RACPKT);

        sendLowerPackets(pkt);

        EV << NOW << " Ue  " << nodeId_ << " cell " << cellId_ << " ,RAC request sent to PHY " << endl;

        // wait at least  "raRespWinStart_" TTIs before another RAC request
        raRespTimer_ = raRespWinStart_;
    }
}

void
LteMacUe::updateUserTxParam(cPacket* pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);

    auto lteInfo = pkt->getTag<UserControlInfo> ();

    if (lteInfo->getFrameType() != DATAPKT)
        return;

    double carrierFrequency = lteInfo->getCarrierFrequency();

    lteInfo->setUserTxParams(schedulingGrant_[carrierFrequency]->getUserTxParams()->dup());

    lteInfo->setTxMode(schedulingGrant_[carrierFrequency]->getUserTxParams()->readTxMode());

    int grantedBlocks = schedulingGrant_[carrierFrequency]->getTotalGrantedBlocks();

    lteInfo->setGrantedBlocks(schedulingGrant_[carrierFrequency]->getGrantedBlocks());
    lteInfo->setTotalGrantedBlocks(grantedBlocks);
}

void LteMacUe::flushHarqBuffers()
{
    // send the selected units to lower layers
    std::map<double, HarqTxBuffers>::iterator mtit;
    for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
    {
//        // skip if this is not the turn of this carrier
//        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mtit->first)) > 0)
//            continue;

        HarqTxBuffers::iterator it2;
        for(it2 = mtit->second.begin(); it2 != mtit->second.end(); it2++)
            it2->second->sendSelectedDown();
    }

    // deleting non-periodic grant
    auto git = schedulingGrant_.begin();
    for (; git != schedulingGrant_.end(); ++git)
    {
        if (git->second != nullptr && !(git->second->getPeriodic()))
        {
//            delete git->second;
            git->second=nullptr;
        }
    }
}

bool
LteMacUe::getHighestBackloggedFlow(MacCid& cid, unsigned int& priority)
{
    // TODO : optimize if inefficient
    // TODO : implement priorities and LCGs
    // search in virtual buffer structures

    LteMacBufferMap::const_iterator it = macBuffers_.begin(), et = macBuffers_.end();
    for (; it != et; ++it)
    {
        if (!it->second->isEmpty())
        {
            cid = it->first;
            // TODO priority = something;
            return true;
        }
    }
    return false;
}

bool
LteMacUe::getLowestBackloggedFlow(MacCid& cid, unsigned int& priority)
{
    // TODO : optimize if inefficient
    // TODO : implement priorities and LCGs
    LteMacBufferMap::const_reverse_iterator it = macBuffers_.rbegin(), et = macBuffers_.rend();
    for (; it != et; ++it)
    {
        if (!it->second->isEmpty())
        {
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
    LteMacBuffers::iterator mit;
    LteMacBufferMap::iterator vit;
    for (mit = mbuf_.begin(); mit != mbuf_.end(); )
    {
        while (!mit->second->isEmpty())
        {
            cPacket* pkt = mit->second->popFront();
            delete pkt;
        }
        delete mit->second;        // Delete Queue
        mbuf_.erase(mit++);        // Delete Elem
    }
    for (vit = macBuffers_.begin(); vit != macBuffers_.end(); )
    {
        while (!vit->second->isEmpty())
            vit->second->popFront();
        delete vit->second;                  // Delete Queue
        macBuffers_.erase(vit++);           // Delete Elem
    }

    // delete H-ARQ buffers
    std::map<double, HarqTxBuffers>::iterator mtit;
    for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
    {
        HarqTxBuffers::iterator hit;
        for (hit = mtit->second.begin(); hit != mtit->second.end(); )
        {
            delete hit->second; // Delete Queue
            mtit->second.erase(hit++); // Delete Elem
        }
    }

    std::map<double, HarqRxBuffers>::iterator mrit;
    for (mrit = harqRxBuffers_.begin(); mrit != harqRxBuffers_.end(); ++mrit)
    {
        HarqRxBuffers::iterator hit2;
        for (hit2 = mrit->second.begin(); hit2 != mrit->second.end();)
        {
             delete hit2->second; // Delete Queue
             mrit->second.erase(hit2++); // Delete Elem
        }
    }

    // remove traffic descriptor and lcg entry
    lcgMap_.clear();
    connDesc_.clear();
}
