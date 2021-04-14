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

#include "stack/mac/layer/NRMacUe.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/scheduler/LteSchedulerUeUl.h"
#include "inet/common/TimeTag_m.h"

Define_Module(NRMacUe);

NRMacUe::NRMacUe() : LteMacUeD2D()
{
}

NRMacUe::~NRMacUe()
{
}

void NRMacUe::handleSelfMessage()
{
    EV << "----- UE MAIN LOOP -----" << endl;

    // extract pdus from all harqrxbuffers and pass them to unmaker
    std::map<double, HarqRxBuffers>::iterator mit = harqRxBuffers_.begin();
    std::map<double, HarqRxBuffers>::iterator met = harqRxBuffers_.end();
    for (; mit != met; mit++)
    {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mit->first)) > 0)
            continue;

        HarqRxBuffers::iterator hit = mit->second.begin();
        HarqRxBuffers::iterator het = mit->second.end();

        std::list<Packet*> pduList;
        for (; hit != het; ++hit)
        {
            pduList = hit->second->extractCorrectPdus();
            while (! pduList.empty())
            {
                auto pdu = pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    EV << NOW << "NRMacUe::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int)currentHarq_ << endl;

    // no grant available - if user has backlogged data, it will trigger scheduling request
    // no harq counter is updated since no transmission is sent.

    bool noSchedulingGrants = true;
    auto git = schedulingGrant_.begin();
    auto get = schedulingGrant_.end();
    for (; git != get; ++git)
    {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(git->first)) > 0)
            continue;

        if (git->second != NULL)
            noSchedulingGrants = false;
    }

    if (noSchedulingGrants)
    {
        EV << NOW << " LteMacUe::handleSelfMessage " << nodeId_ << " NO configured grant" << endl;
        checkRAC();
        // TODO ensure all operations done  before return ( i.e. move H-ARQ rx purge before this point)
    }
    else
    {
        bool periodicGrant = false;
        bool checkRac = false;
        bool skip = false;
        for (git = schedulingGrant_.begin(); git != get; ++git)
        {
            if (git->second != NULL && git->second->getPeriodic())
            {
                periodicGrant = true;
                double carrierFreq = git->first;

                // Periodic checks
                if(--expirationCounter_[carrierFreq] < 0)
                {
                    // Periodic grant is expired
                    git->second = nullptr;
                    checkRac = true;
                }
                else if (--periodCounter_[carrierFreq]>0)
                {
                    skip = true;
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
    if (!noSchedulingGrants) // if a grant is configured
    {
        EV << NOW << " NRMacUe::handleSelfMessage " << nodeId_ << " entered scheduling" << endl;

        bool retx = false;

        HarqTxBuffers::iterator it2;
        LteHarqBufferTx * currHarq;
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

            for(it2 = mtit->second.begin(); it2 != mtit->second.end(); it2++)
            {
                currHarq = it2->second;
                unsigned int numProcesses = currHarq->getNumProcesses();

                for (unsigned int proc = 0; proc < numProcesses; proc++)
                {
                    LteHarqProcessTx* currProc = currHarq->getProcess(proc);

                    // check if the current process has unit ready for retx
                    bool ready = currProc->hasReadyUnits();
                    CwList cwListRetx = currProc->readyUnitsIds();

                    EV << "\t [process=" << proc << "] , [retx=" << ((ready)?"true":"false") << "] , [n=" << cwListRetx.size() << "]" << endl;

                    // check if one 'ready' unit has the same direction of the grant
                    bool checkDir = false;
                    CwList::iterator cit = cwListRetx.begin();
                    for (; cit != cwListRetx.end(); ++cit)
                    {
                        Codeword cw = *cit;
                        auto info = currProc->getPdu(cw)->getTag<UserControlInfo>();
                        if (info->getDirection() == schedulingGrant_[carrierFrequency]->getDirection())
                        {
                            checkDir = true;
                            break;
                        }
                    }

                    // if a retransmission is needed
                    if(ready && checkDir)
                    {
                        UnitList signal;
                        signal.first = proc;
                        signal.second = cwListRetx;
                        currHarq->markSelected(signal,schedulingGrant_[carrierFrequency]->getUserTxParams()->getLayers().size());
                        retx = true;
                    }
                }
            }
        }
        // if no retx is needed, proceed with normal scheduling
        if(!retx)
        {
            emptyScheduleList_ = true;
            std::map<double, LteSchedulerUeUl*>::iterator sit;
            for (sit = lcgScheduler_.begin(); sit != lcgScheduler_.end(); ++sit)
            {
                double carrierFrequency = sit->first;

                // skip if this is not the turn of this carrier
                if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency)) > 0)
                    continue;

                LteSchedulerUeUl* carrierLcgScheduler = sit->second;

                EV << "NRMacUe::handleSelfMessage - running LCG scheduler for carrier [" << carrierFrequency << "]" << endl;
                LteMacScheduleList* carrierScheduleList = carrierLcgScheduler->schedule();
                EV << "NRMacUe::handleSelfMessage - scheduled " << carrierScheduleList->size() << " connections on carrier " << carrierFrequency << endl;
                scheduleList_[carrierFrequency] = carrierScheduleList;
                if (!carrierScheduleList->empty())
                    emptyScheduleList_ = false;
            }

            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && emptyScheduleList_)
            {
                // no connection scheduled, but we can use this grant to send a BSR to the eNB
                macPduMake();
            }
            else
            {
                requestedSdus_ = macSduRequest(); // returns an integer
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
        std::map<double, HarqTxBuffers>::iterator mtit;
        for (mtit = harqTxBuffers_.begin(); mtit != harqTxBuffers_.end(); ++mtit)
        {
            EV << "\n carrier[ " << mtit->first << "] htxbuf.size " << mtit->second.size() << endl;

            HarqTxBuffers::iterator it;

            EV << "\n htxbuf.size " << harqTxBuffers_.size() << endl;

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

    decreaseNumerologyPeriodCounter();

    EV << "--- END UE MAIN LOOP ---" << endl;
}

void NRMacUe::macPduMake(MacCid cid)
{
    int64 size = 0;

    macPduList_.clear();

    bool bsrAlreadyMade = false;
    // UE is in D2D-mode but it received an UL grant (for BSR)
    auto git = schedulingGrant_.begin();
    for (; git != schedulingGrant_.end(); ++git)
    {
        double carrierFreq = git->first;

        // skip if this is not the turn of this carrier
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
            continue;

        if (git->second != nullptr && git->second->getDirection() == UL && emptyScheduleList_)
        {
            if (bsrTriggered_ || bsrD2DMulticastTriggered_)
            {
                // Compute BSR size taking into account only DM flows
                int sizeBsr = 0;
                LteMacBufferMap::const_iterator itbsr;
                for (itbsr = macBuffers_.begin(); itbsr != macBuffers_.end(); itbsr++)
                {
                    MacCid cid = itbsr->first;
                    Direction connDir = (Direction)connDesc_[cid].getDirection();

                    // if the bsr was triggered by D2D (D2D_MULTI), only account for D2D (D2D_MULTI) connections
                    if (bsrTriggered_ && connDir != D2D)
                        continue;
                    if (bsrD2DMulticastTriggered_ && connDir != D2D_MULTI)
                        continue;

                    sizeBsr += itbsr->second->getQueueOccupancy();

                    // take into account the RLC header size
                    if (sizeBsr > 0)
                    {
                        if (connDesc_[cid].getRlcType() == UM)
                            sizeBsr += RLC_HEADER_UM;
                        else if (connDesc_[cid].getRlcType() == AM)
                            sizeBsr += RLC_HEADER_AM;
                    }
                }

                if (sizeBsr > 0)
                {
                    // Call the appropriate function for make a BSR for a D2D communication
                    Packet* macPktBsr = makeBsr(sizeBsr);
                    auto info = macPktBsr->getTag<UserControlInfo>();

                    if (info != NULL)
                    {
                        info->setCarrierFrequency(carrierFreq);
                        info->setUserTxParams(git->second->getUserTxParams()->dup());
                        if (bsrD2DMulticastTriggered_)
                        {
                            info->setLcid(D2D_MULTI_SHORT_BSR);
                            bsrD2DMulticastTriggered_ = false;
                        }
                        else
                            info->setLcid(D2D_SHORT_BSR);
                    }

                    // Add the created BSR to the PDU List
                    if( macPktBsr != NULL )
                    {
                        LteChannelModel* channelModel = phy_->getChannelModel();
                        if (channelModel == NULL)
                            throw cRuntimeError("NRMacUe::macPduMake - channel model is a null pointer. Abort.");
                        else
                            macPduList_[channelModel->getCarrierFrequency()][ std::pair<MacNodeId, Codeword>( getMacCellId(), 0) ] = macPktBsr;
                        bsrAlreadyMade = true;
                        EV << "NRMacUe::macPduMake - BSR D2D created with size " << sizeBsr << "created" << endl;
                    }
                }
                else
                {
                    bsrD2DMulticastTriggered_ = false;
                    bsrTriggered_ = false;
                }
            }
            break;
        }
    }

    if(!bsrAlreadyMade)
    {
        // In a D2D communication if BSR was created above this part isn't executed
        // Build a MAC PDU for each scheduled user on each codeword
        std::map<double, LteMacScheduleList*>::iterator cit = scheduleList_.begin();
        for (; cit != scheduleList_.end(); ++cit)
        {
            double carrierFreq = cit->first;

            // skip if this is not the turn of this carrier
            if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(carrierFreq)) > 0)
                continue;

            LteMacScheduleList::const_iterator it;
            for (it = cit->second->begin(); it != cit->second->end(); it++)
            {
                Packet* macPkt;

                MacCid destCid = it->first.first;
                Codeword cw = it->first.second;

                // get the direction (UL/D2D/D2D_MULTI) and the corresponding destination ID
                FlowControlInfo* lteInfo = &(connDesc_.at(destCid));
                MacNodeId destId = lteInfo->getDestId();
                Direction dir = (Direction)lteInfo->getDirection();

                std::pair<MacNodeId, Codeword> pktId = std::pair<MacNodeId, Codeword>(destId, cw);
                unsigned int sduPerCid = it->second;

                if (sduPerCid == 0 && !bsrTriggered_ && !bsrD2DMulticastTriggered_)
                    continue;

                if (macPduList_.find(carrierFreq) == macPduList_.end())
                {
                    MacPduList newList;
                    macPduList_[carrierFreq] = newList;
                }
                MacPduList::iterator pit = macPduList_[carrierFreq].find(pktId);

                // No packets for this user on this codeword
                if (pit == macPduList_[carrierFreq].end())
                {
                    // Create a PDU
                    macPkt = new Packet("LteMacPdu");
                    auto header = makeShared<LteMacPdu>();
                    header->setHeaderLength(MAC_HEADER);
                    macPkt->insertAtFront(header);

                    macPkt->addTagIfAbsent<CreationTimeTag>()->setCreationTime(NOW);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(dir);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setLcid(MacCidToLcid(SHORT_BSR));
                    macPkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);
                    if (usePreconfiguredTxParams_)
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(preconfiguredTxParams_->dup());
                    else
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(schedulingGrant_[carrierFreq]->getUserTxParams()->dup());

                    macPduList_[carrierFreq][pktId] = macPkt;
                }
                else
                {
                    // Never goes here because of the macPduList_.clear() at the beginning
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

                    if (pkt != NULL)
                    {
                        // multicast support
                        // this trick gets the group ID from the MAC SDU and sets it in the MAC PDU
                        auto infoVec = getTagsWithInherit<LteControlInfo>(pkt);
                        if (infoVec.empty())
                            throw cRuntimeError("No tag of type LteControlInfo found");

                        int32 groupId =  infoVec.front().getMulticastGroupId();
                        if (groupId >= 0) // for unicast, group id is -1
                            macPkt->getTag<UserControlInfo>()->setMulticastGroupId(groupId);

                        drop(pkt);

                        auto header = macPkt->removeAtFront<LteMacPdu>();
                        header->pushSdu(pkt);
                        macPkt->insertAtFront(header);
                        sduPerCid--;
                    }
                    else
                        throw cRuntimeError("NRMacUe::macPduMake - extracted SDU is NULL. Abort.");
                }

                // consider virtual buffers to compute BSR size
                size += macBuffers_[destCid]->getQueueOccupancy();

                if (size > 0)
                {
                    // take into account the RLC header size
                    if (connDesc_[destCid].getRlcType() == UM)
                        size += RLC_HEADER_UM;
                    else if (connDesc_[destCid].getRlcType() == AM)
                        size += RLC_HEADER_AM;
                }
            }
        }
    }

    // Put MAC PDUs in H-ARQ buffers
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

        MacPduList::iterator pit;
        for (pit = lit->second.begin(); pit != lit->second.end(); pit++)
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
                // The tx buffer does not exist yet for this mac node id, create one
                LteHarqBufferTx* hb;
                // FIXME: hb is never deleted
                auto info = pit->second->getTag<UserControlInfo>();
                if (info->getDirection() == UL)
                    hb = new LteHarqBufferTx((unsigned int) ENB_TX_HARQ_PROCESSES, this, (LteMacBase*) getMacByMacNodeId(destId));
                else // D2D or D2D_MULTI
                    hb = new LteHarqBufferTxD2D((unsigned int) ENB_TX_HARQ_PROCESSES, this, (LteMacBase*) getMacByMacNodeId(destId));
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }

            // search for an empty unit within the first available process
            UnitList txList = txBuf->firstAvailable();
            EV << "NRMacUe::macPduMake - [Used Acid=" << (unsigned int)txList.first << "]" << endl;

            //Get a reference of the LteMacPdu from pit pointer (extract Pdu from the MAP)
            auto macPkt = pit->second;

            /* BSR related operations

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
            //        } */

            auto header = macPkt->removeAtFront<LteMacPdu>();
            // Attach BSR to PDU if RAC is won and wasn't already made
            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && !bsrAlreadyMade )
            {
                MacBsr* bsr = new MacBsr();
                bsr->setTimestamp(simTime().dbl());
                bsr->setSize(size);
                header->pushCe(bsr);
                bsrTriggered_ = false;
                bsrD2DMulticastTriggered_ = false;
                EV << "NRMacUe::macPduMake - BSR created with size " << size << endl;
            }

            macPkt->insertAtFront(header);

            EV << "NRMacUe: pduMaker created PDU: " << macPkt->str() << endl;

            // TODO: harq test
            // pdu transmission here (if any)
            // txAcid has HARQ_NONE for non-fillable codeword, acid otherwise
            if (txList.second.empty())
            {
                EV << "NRMacUe() : no available process for this MAC pdu in TxHarqBuffer" << endl;
                delete macPkt;
            }
            else
            {
                //Insert PDU in the Harq Tx Buffer
                //txList.first is the acid
                txBuf->insertPdu(txList.first,cw, macPkt);
            }
        }
    }
}
