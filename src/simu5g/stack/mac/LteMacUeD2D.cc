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

#include "stack/mac/LteMacUeD2D.h"

#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/packet/LteRac_m.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/scheduler/LteSchedulerUeUl.h"

namespace simu5g {

Define_Module(LteMacUeD2D);

using namespace inet;

simsignal_t LteMacUeD2D::rcvdD2DModeSwitchNotificationSignal_ = registerSignal("rcvdD2DModeSwitchNotification");

LteMacUeD2D::~LteMacUeD2D()
{
    delete preconfiguredTxParams_;
}

void LteMacUeD2D::initialize(int stage)
{
    LteMacUe::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        // check the RLC module type: if it is not "D2D", abort simulation
        cModule *rlcUm = inet::getModuleFromPar<cModule>(par("rlcUmModule"), this);
        std::string rlcUmType = rlcUm->getComponentType()->getName();
        if (rlcUmType != "LteRlcUmD2D")
            throw cRuntimeError("LteMacUeD2D::initialize - '%s' must be 'LteRlcUmD2D' instead of '%s'. Aborting", par("rlcUmModule").stringValue(), rlcUmType.c_str());

        cModule *pdcpRrc = inet::getModuleFromPar<cModule>(par("pdcpRrcModule"), this);
        std::string pdcpType = pdcpRrc->getComponentType()->getName();
        if (pdcpType != "LtePdcpRrcUeD2D" && pdcpType != "NRPdcpRrcUe")
            throw cRuntimeError("LteMacUeD2D::initialize - %s module found, must be LtePdcpRrcUeD2D or NRPdcpRrcUe. Aborting", pdcpType.c_str());
    }
    if (stage == inet::INITSTAGE_NETWORK_LAYER) {
        // get parameters
        usePreconfiguredTxParams_ = par("usePreconfiguredTxParams");

        if (cellId_ != NODEID_NONE) {
            preconfiguredTxParams_ = getPreconfiguredTxParams();

            // get the reference to the eNB
            enb_ = check_and_cast<LteMacEnbD2D *>(getMacByMacNodeId(binder_, cellId_));

            LteAmc *amc = check_and_cast<LteMacEnb *>(getSimulation()->getModule(binder_->getOmnetId(cellId_))->getSubmodule("cellularNic")->getSubmodule("mac"))->getAmc();
            amc->attachUser(nodeId_, D2D);

// TODO remove it. UeCollector connection made in LteMacUe Initialize
        }
        else
            enb_ = nullptr;
    }
}

//Function to create only a BSR for the eNB
Packet *LteMacUeD2D::makeBsr(int size) {

    auto macPkt = new Packet("LteMacPdu");
    auto header = makeShared<LteMacPdu>();
    header->setHeaderLength(MAC_HEADER);
    macPkt->setTimestamp(NOW);

    MacBsr *bsr = new MacBsr();
    bsr->setTimestamp(simTime().dbl());
    bsr->setSize(size);
    header->pushCe(bsr);
    macPkt->insertAtFront(header);
    macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
    macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
    macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);

    bsrTriggered_ = false;
    EV << "LteMacUeD2D::makeBsr() - BSR with size " << size << " bytes created" << endl;
    return macPkt;
}

void LteMacUeD2D::macPduMake(MacCid cid)
{
    int64_t size = 0;

    macPduList_.clear();

    bool bsrAlreadyMade = false;
    // UE is in D2D mode but it received an UL grant (for BSR)
    for (auto& gitem : schedulingGrant_) {
        if (gitem.second != nullptr && gitem.second->getDirection() == UL && emptyScheduleList_) {
            if (bsrTriggered_ || bsrD2DMulticastTriggered_) {
                // Compute BSR size taking into account only DM flows
                int sizeBsr = 0;
                for (const auto& itbsr : macBuffers_) {
                    MacCid cid = itbsr.first;
                    Direction connDir = (Direction)connDesc_[cid].getDirection();

                    // if the bsr was triggered by D2D (D2D_MULTI), only account for D2D (D2D_MULTI) connections
                    if (bsrTriggered_ && connDir != D2D)
                        continue;
                    if (bsrD2DMulticastTriggered_ && connDir != D2D_MULTI)
                        continue;

                    sizeBsr += itbsr.second->getQueueOccupancy();

                    // take into account the RLC header size
                    if (sizeBsr > 0) {
                        if (connDesc_[cid].getRlcType() == UM)
                            sizeBsr += RLC_HEADER_UM;
                        else if (connDesc_[cid].getRlcType() == AM)
                            sizeBsr += RLC_HEADER_AM;
                    }
                }

                if (sizeBsr > 0) {
                    // Call the appropriate function to make a BSR for a D2D communication
                    auto macPktBsr = makeBsr(sizeBsr);
                    auto info = macPktBsr->getTagForUpdate<UserControlInfo>();
                    double carrierFreq = gitem.first;
                    if (info != nullptr) {
                        info->setCarrierFrequency(carrierFreq);
                        info->setUserTxParams(gitem.second->getUserTxParams()->dup());
                        if (bsrD2DMulticastTriggered_) {
                            info->setLcid(D2D_MULTI_SHORT_BSR);
                            bsrD2DMulticastTriggered_ = false;
                        }
                        else
                            info->setLcid(D2D_SHORT_BSR);
                    }

                    // Add the created BSR to the PDU List
                    if (macPktBsr != nullptr) {
                        // select channel model for given carrier frequency
                        LteChannelModel *channelModel = phy_->getChannelModel(carrierFreq);
                        if (channelModel == nullptr)
                            throw cRuntimeError("NRMacUe::macPduMake - channel model is a null pointer. Abort.");
                        else
                            macPduList_[channelModel->getCarrierFrequency()][{getMacCellId(), 0}] = macPktBsr;
                        bsrAlreadyMade = true;
                        EV << "LteMacUeD2D::macPduMake - BSR D2D created with size " << sizeBsr << " bytes created" << endl;
                    }

                    bsrRtxTimer_ = bsrRtxTimerStart_;  // this prevents the UE from sending an unnecessary RAC request
                }
                else {
                    bsrD2DMulticastTriggered_ = false;
                    bsrTriggered_ = false;
                    bsrRtxTimer_ = 0;
                }
            }
            break;
        }
    }

    if (!bsrAlreadyMade) {
        // In a D2D communication if BSR was created above this part isn't executed
        // Build a MAC PDU for each scheduled user on each codeword
        for (auto[carrierFreq, schList] : scheduleList_) {
            Packet *macPkt = nullptr;

            LteMacScheduleList::const_iterator it;
            for (auto& item : *schList) {
                MacCid destCid = item.first.first;
                Codeword cw = item.first.second;

                // get the direction (UL/D2D/D2D_MULTI) and the corresponding destination ID
                FlowControlInfo *lteInfo = &(connDesc_.at(destCid));
                MacNodeId destId = lteInfo->getDestId();
                Direction dir = (Direction)lteInfo->getDirection();

                std::pair<MacNodeId, Codeword> pktId = {destId, cw};
                unsigned int sduPerCid = item.second;

                if (sduPerCid == 0 && !bsrTriggered_ && !bsrD2DMulticastTriggered_)
                    continue;

                if (macPduList_.find(carrierFreq) == macPduList_.end()) {
                    MacPduList newList;
                    macPduList_[carrierFreq] = newList;
                }
                MacPduList::iterator pit = macPduList_[carrierFreq].find(pktId);

                // No packets for this user on this codeword
                if (pit == macPduList_[carrierFreq].end()) {
                    // Always goes here because of the macPduList_.clear() at the beginning
                    // Build the Control Element of the MAC PDU

                    // Create a PDU
                    macPkt = new Packet("LteMacPdu");
                    auto header = makeShared<LteMacPdu>();
                    //macPkt = new LteMacPdu("LteMacPdu");
                    header->setHeaderLength(MAC_HEADER);
                    macPkt->insertAtFront(header);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(dir);
                    macPkt->addTagIfAbsent<UserControlInfo>()->setLcid(MacCidToLcid(SHORT_BSR));
                    macPkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);
                    if (usePreconfiguredTxParams_)
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(preconfiguredTxParams_->dup());
                    else
                        macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(schedulingGrant_[carrierFreq]->getUserTxParams()->dup());

                    macPkt->addTagIfAbsent<UserControlInfo>()->setGrantId(schedulingGrant_[carrierFreq]->getGrantId());

                    macPduList_[carrierFreq][pktId] = macPkt;
                }
                else {
                    // Never goes here because of the macPduList_.clear() at the beginning
                    macPkt = pit->second;
                }

                while (sduPerCid > 0) {
                    // Add SDU to PDU
                    // Find MAC Packet
                    if (mbuf_.find(destCid) == mbuf_.end())
                        throw cRuntimeError("Unable to find MAC buffer for cid %d", destCid);

                    if (mbuf_[destCid]->isEmpty())
                        throw cRuntimeError("Empty buffer for cid %d, while expected SDUs were %d", destCid, sduPerCid);

                    auto pkt = check_and_cast<Packet *>(mbuf_[destCid]->popFront());

                    // multicast support
                    // this trick gets the group ID from the MAC SDU and sets item in the MAC PDU
                    auto infoVec = getTagsWithInherit<LteControlInfo>(pkt);

                    if (infoVec.empty())
                        throw cRuntimeError("No tag of type LteControlInfo found");

                    int32_t groupId = infoVec.front().getMulticastGroupId();
                    if (groupId >= 0) // for unicast, group id is -1
                        macPkt->getTagForUpdate<UserControlInfo>()->setMulticastGroupId(groupId);

                    drop(pkt);

                    auto header = macPkt->removeAtFront<LteMacPdu>();
                    header->pushSdu(pkt);
                    macPkt->insertAtFront(header);
                    sduPerCid--;
                }

                // consider virtual buffers to compute BSR size
                size += macBuffers_[destCid]->getQueueOccupancy();

                if (size > 0) {
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
    for (auto& lit : macPduList_) {
        double carrierFreq = lit.first;

        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end()) {
            HarqTxBuffers newHarqTxBuffers;
            harqTxBuffers_[carrierFreq] = newHarqTxBuffers;
        }
        HarqTxBuffers& harqTxBuffers = harqTxBuffers_[carrierFreq];

        for (auto& pit : lit.second) {
            MacNodeId destId = pit.first.first;
            Codeword cw = pit.first.second;

            // Check if the HarqTx buffer already exists for the destId
            // Get a reference for the destId TXBuffer
            LteHarqBufferTx *txBuf;
            HarqTxBuffers::iterator hit = harqTxBuffers.find(destId);
            if (hit != harqTxBuffers.end()) {
                txBuf = hit->second;
            }
            else {
                // The tx buffer does not exist yet for this mac node id, create one
                LteHarqBufferTx *hb;
                // FIXME: hb is never deleted
                auto info = pit.second->getTag<UserControlInfo>();

                if (info->getDirection() == UL) {
                    hb = new LteHarqBufferTx(binder_, (unsigned int)ENB_TX_HARQ_PROCESSES, this, check_and_cast<LteMacBase *>(getMacByMacNodeId(binder_, destId)));
                }
                else { // D2D or D2D_MULTI
                    hb = new LteHarqBufferTxD2D(binder_, (unsigned int)ENB_TX_HARQ_PROCESSES, this, check_and_cast<LteMacBase *>(getMacByMacNodeId(binder_, destId)));
                }
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }

            // search for an empty unit within the current harq process
            UnitList txList = txBuf->getEmptyUnits(currentHarq_);
            EV << "LteMacUeD2D::macPduMake - [Used Acid=" << (unsigned int)txList.first << "] , [curr=" << (unsigned int)currentHarq_ << "]" << endl;

            // Get a reference of the LteMacPdu from pit pointer (extract PDU from the MAP)
            auto macPkt = pit.second;

            // BSR related operations

               // according to the TS 36.321 v8.7.0, when there are uplink resources assigned to the UE, a BSR
               // has to be sent even if there is no data in the user's queues. In a few words, a BSR is always
               // triggered and has to be sent when there are enough resources

               // TODO implement differentiated BSR attach
               //
               //            // if there's enough space for a LONG BSR, send it
               //            if( (availableBytes >= LONG_BSR_SIZE) ) {
               //                // Create a PDU if data were not scheduled
               //                if (pdu==0)
               //                    pdu = new LteMacPdu();
               //
               //                if(LteDebug::trace("LteSchedulerUeUl::schedule") || LteDebug::trace("LteSchedulerUeUl::schedule@bsrTracing"))
               //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - node %hu, sending a Long BSR...\n",NOW,nodeId);
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
               //                    fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - node %hu, sending a Short/Truncated BSR...\n",NOW,nodeId);
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
               //                fprintf(stderr, "%.9f LteSchedulerUeUl::schedule - node %hu, creating uplink PDU.\n", NOW, nodeId);
               //
               //        }

            auto header = macPkt->removeAtFront<LteMacPdu>();
            // Attach BSR to PDU if RAC is won and wasn't already made
            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && !bsrAlreadyMade && size > 0) {
                MacBsr *bsr = new MacBsr();
                bsr->setTimestamp(simTime().dbl());
                bsr->setSize(size);
                header->pushCe(bsr);
                bsrTriggered_ = false;
                bsrD2DMulticastTriggered_ = false;
                bsrAlreadyMade = true;
                EV << "LteMacUeD2D::macPduMake - BSR created with size " << size << endl;
            }

            if (bsrAlreadyMade && size > 0)                                              // this prevents the UE from sending an unnecessary RAC request
                bsrRtxTimer_ = bsrRtxTimerStart_;
            else
                bsrRtxTimer_ = 0;

            macPkt->insertAtFront(header);

            EV << "LteMacUeD2D: pduMaker created PDU: " << macPkt->str() << endl;

            // TODO: harq test
            // PDU transmission here (if any)
            // txAcid has HARQ_NONE for non-fillable codeword, acid otherwise
            if (txList.second.empty()) {
                EV << "LteMacUeD2D() : no available process for this MAC PDU in TxHarqBuffer" << endl;
                delete macPkt;
            }
            else {
                // Insert PDU in the HARQ Tx Buffer
                // txList.first is the acid
                txBuf->insertPdu(txList.first, cw, macPkt);
            }
        }
    }
}

void LteMacUeD2D::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        LteMacUe::handleMessage(msg);
        return;
    }

    auto pkt = check_and_cast<inet::Packet *>(msg);
    cGate *incoming = pkt->getArrivalGate();

    if (incoming == downInGate_) {
        auto userInfo = pkt->getTag<UserControlInfo>();

        if (userInfo->getFrameType() == D2DMODESWITCHPKT) {
            EV << "LteMacUeD2D::handleMessage - Received packet " << pkt->getName() <<
                " from port " << pkt->getArrivalGate()->getName() << endl;

            // message from PHY_to_MAC gate (from the lower layer)
            emit(receivedPacketFromLowerLayerSignal_, pkt);

            // call handler
            macHandleD2DModeSwitch(pkt);

            return;
        }
    }

    LteMacUe::handleMessage(msg);
}

void LteMacUeD2D::macHandleGrant(cPacket *pktAux)
{
    EV << NOW << " LteMacUeD2D::macHandleGrant - UE [" << nodeId_ << "] - Grant received " << endl;

    // extract grant
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto grant = pkt->popAtFront<LteSchedulingGrant>();

    auto userInfo = pkt->getTag<UserControlInfo>();
    double carrierFrequency = userInfo->getCarrierFrequency();
    EV << NOW << " LteMacUeD2D::macHandleGrant - Direction: " << dirToA(grant->getDirection()) << " Carrier: " << carrierFrequency << endl;

    // delete old grant
    if (schedulingGrant_.find(carrierFrequency) != schedulingGrant_.end() && schedulingGrant_[carrierFrequency] != nullptr) {
        schedulingGrant_[carrierFrequency] = nullptr;
    }

    // store received grant
    schedulingGrant_[carrierFrequency] = grant;
    if (grant->getPeriodic()) {
        periodCounter_[carrierFrequency] = grant->getPeriod();
        expirationCounter_[carrierFrequency] = grant->getExpiration();
    }

    EV << NOW << " Node " << nodeId_ << " received grant of blocks " << grant->getTotalGrantedBlocks()
       << ", bytes " << grant->getGrantedCwBytes(0) << " Direction: " << dirToA(grant->getDirection()) << endl;

    // clearing pending RAC requests
    racRequested_ = false;
    racD2DMulticastRequested_ = false;

    delete pkt;
}

void LteMacUeD2D::checkRAC()
{
    EV << NOW << " LteMacUeD2D::checkRAC , Ue  " << nodeId_ << ", racTimer : " << racBackoffTimer_ << " maxRacTryOuts : " << maxRacTryouts_
       << ", raRespTimer:" << raRespTimer_ << endl;

    if (racBackoffTimer_ > 0) {
        racBackoffTimer_--;
        return;
    }

    if (raRespTimer_ > 0) {
        // decrease RAC response timer
        raRespTimer_--;
        EV << NOW << " LteMacUeD2D::checkRAC - waiting for previous RAC requests to complete (timer=" << raRespTimer_ << ")" << endl;
        return;
    }

    if (bsrRtxTimer_ > 0) {
        // decrease BSR timer
        bsrRtxTimer_--;
        EV << NOW << " LteMacUe::checkRAC - waiting for a grant, BSR rtx timer has not expired yet (timer=" << bsrRtxTimer_ << ")" << endl;

        return;
    }

    // Avoids double requests within the same TTI window
    if (racRequested_) {
        EV << NOW << " LteMacUeD2D::checkRAC - double RAC request" << endl;
        racRequested_ = false;
        return;
    }
    if (racD2DMulticastRequested_) {
        EV << NOW << " LteMacUeD2D::checkRAC - double RAC request" << endl;
        racD2DMulticastRequested_ = false;
        return;
    }

    bool trigger = false;
    bool triggerD2DMulticast = false;

    LteMacBufferMap::const_iterator it;

    for (auto [cid, macBuffer] : macBuffers_) {
        if (!(macBuffer->isEmpty())) {
            if (connDesc_.at(cid).getDirection() == D2D_MULTI)
                triggerD2DMulticast = true;
            else
                trigger = true;
            break;
        }
    }

    if (!trigger && !triggerD2DMulticast) {
        EV << NOW << " LteMacUeD2D::checkRAC , Ue " << nodeId_ << ", RAC aborted, no data in queues " << endl;
    }

    if ((racRequested_ = trigger) || (racD2DMulticastRequested_ = triggerD2DMulticast)) {
        auto pkt = new Packet("RacRequest");
        double carrierFrequency = phy_->getPrimaryChannelModel()->getCarrierFrequency();
        pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFrequency);
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(RACPKT);

        auto racReq = makeShared<LteRac>();

        pkt->insertAtFront(racReq);
        sendLowerPackets(pkt);

        EV << NOW << " Ue  " << nodeId_ << " cell " << cellId_ << ", RAC request sent to PHY " << endl;

        // wait at least  "raRespWinStart_" TTIs before another RAC request
        raRespTimer_ = raRespWinStart_;
    }
}

void LteMacUeD2D::macHandleRac(cPacket *pktAux)
{
    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto racPkt = pkt->peekAtFront<LteRac>();

    if (racPkt->getSuccess()) {
        EV << "LteMacUeD2D::macHandleRac - Ue " << nodeId_ << " won RAC" << endl;
        // if RAC is won, BSR has to be sent
        if (racD2DMulticastRequested_)
            bsrD2DMulticastTriggered_ = true;
        else
            bsrTriggered_ = true;

        // reset RAC counter
        currentRacTry_ = 0;
        //reset RAC backoff timer
        racBackoffTimer_ = 0;
    }
    else {
        // RAC has failed
        if (++currentRacTry_ >= maxRacTryouts_) {
            EV << NOW << " Ue " << nodeId_ << ", RAC reached max attempts : " << currentRacTry_ << endl;
            // no more RAC allowed
            //! TODO flush all buffers here
            //reset RAC counter
            currentRacTry_ = 0;
            //reset RAC backoff timer
            racBackoffTimer_ = 0;
        }
        else {
            // recompute backoff timer
            racBackoffTimer_ = uniform(minRacBackoff_, maxRacBackoff_);
            EV << NOW << " Ue " << nodeId_ << " RAC attempt failed, backoff extracted : " << racBackoffTimer_ << endl;
        }
    }
    delete pkt;
}

void LteMacUeD2D::handleSelfMessage()
{
    EV << "----- UE MAIN LOOP -----" << endl;

    // extract PDUs from all HARQ RX buffers and pass them to unmaker
    for (auto& mit : harqRxBuffers_) {
        for (auto& hit : mit.second) {
            std::list<Packet *> pduList = hit.second->extractCorrectPdus();
            while (!pduList.empty()) {
                auto pdu = pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    EV << NOW << " LteMacUeD2D::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int)currentHarq_ << endl;

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
                double carrierFreq = git.first;

                // Periodic checks
                if (--expirationCounter_[carrierFreq] < 0) {
                    // Periodic grant is expired
                    git.second = nullptr;
                    // if necessary, a RAC request will be sent to obtain a grant
                    checkRac = true;
                    //return;
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
            currentHarq_ = UE_TX_HARQ_PROCESSES - 2;
        }

        EV << NOW << " LteMacUeD2D::handleSelfMessage " << nodeId_ << " entered scheduling" << endl;

        bool retx = false;

        LteHarqBufferTx *currHarq;
        for (auto& mtit : harqTxBuffers_) {
            double carrierFrequency = mtit.first;

            // skip if no grant is configured for this carrier
            if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end() || schedulingGrant_[carrierFrequency] == nullptr)
                continue;

            for (auto& it2 : mtit.second) {
                EV << "\t Looking for retx in acid " << (unsigned int)currentHarq_ << endl;
                currHarq = it2.second;

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
            std::map<double, LteSchedulerUeUl *>::iterator sit;
            for (auto [carrierFrequency, carrierLcgScheduler] : lcgScheduler_) {
                EV << "LteMacUeD2D::handleSelfMessage - running LCG scheduler for carrier [" << carrierFrequency << "]" << endl;
                LteMacScheduleList *carrierScheduleList = carrierLcgScheduler->schedule();
                EV << "LteMacUeD2D::handleSelfMessage - scheduled " << carrierScheduleList->size() << " connections on carrier " << carrierFrequency << endl;
                scheduleList_[carrierFrequency] = carrierScheduleList;
                if (!carrierScheduleList->empty())
                    emptyScheduleList_ = false;
            }

            if ((bsrTriggered_ || bsrD2DMulticastTriggered_) && emptyScheduleList_) {
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
        for (const auto& mtit : harqTxBuffers_) {
            EV << "\n carrier[ " << mtit.first << "] htxbuf.size " << mtit.second.size() << endl;

            EV << "\n htxbuf.size " << harqTxBuffers_.size() << endl;

            int cntOuter = 0;
            int cntInner = 0;
            for (auto [mtitKey, currHarq] : mtit.second) {
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

    unsigned int purged = 0;
    // purge from corrupted PDUs all Rx H-HARQ buffers
    for (auto& mit : harqRxBuffers_) {
        for (auto& hit : mit.second) {
            // purge corrupted PDUs only if this buffer is for a DL transmission. Otherwise, if you
            // purge PDUs for D2D communication, also "mirror" buffers will be purged
            if (hit.first == cellId_)
                purged += hit.second->purgeCorruptedPdus();
        }
    }
    EV << NOW << " LteMacUeD2D::handleSelfMessage Purged " << purged << " PDUs" << endl;

    if (requestedSdus_ == 0) {
        // update current HARQ process ID
        currentHarq_ = (currentHarq_ + 1) % harqProcesses_;
    }
    EV << "--- END UE MAIN LOOP ---" << endl;
}

UserTxParams *LteMacUeD2D::getPreconfiguredTxParams()
{
    UserTxParams *txParams = new UserTxParams();

    // default parameters for D2D
    txParams->isSet() = true;
    txParams->writeTxMode(TRANSMIT_DIVERSITY);
    Rank ri = 1;                                              // rank for TxD is one
    txParams->writeRank(ri);
    txParams->writePmi(intuniform(1, pow(ri, (double)2)));  // taken from LteFeedbackComputationRealistic::computeFeedback

    Cqi cqi = par("d2dCqi");
    if (cqi < 0 || cqi > 15) {
        delete txParams;
        throw cRuntimeError("LteMacUeD2D::getPreconfiguredTxParams - CQI %hu is not a valid value. Aborting", cqi);
    }
    txParams->writeCqi(std::vector<Cqi>(1, cqi));

    BandSet b;
    CellInfo *cellInfo = getCellInfo(binder_, nodeId_);
    if (cellInfo != nullptr) {
        for (Band i = 0; i < cellInfo->getNumBands(); ++i)
            b.insert(i);
    }
    else {
        delete txParams;
        throw cRuntimeError("LteMacUeD2D::getPreconfiguredTxParams - cellInfo is a NULL pointer. Aborting");
    }

    RemoteSet antennas;
    antennas.insert(MACRO);
    txParams->writeAntennas(antennas);

    return txParams;
}

void LteMacUeD2D::macHandleD2DModeSwitch(cPacket *pktAux)
{
    EV << NOW << " LteMacUeD2D::macHandleD2DModeSwitch - Start" << endl;

    // All data in the MAC buffers of the connection to be switched are deleted.

    auto pkt = check_and_cast<inet::Packet *>(pktAux);
    auto switchPkt = pkt->peekAtFront<D2DModeSwitchNotification>();

    bool txSide = switchPkt->getTxSide();
    MacNodeId peerId = switchPkt->getPeerId();
    LteD2DMode newMode = switchPkt->getNewMode();
    LteD2DMode oldMode = switchPkt->getOldMode();

    if (txSide) {
        emit(rcvdD2DModeSwitchNotificationSignal_, (long)1);

        Direction newDirection = (newMode == DM) ? D2D : UL;
        Direction oldDirection = (oldMode == DM) ? D2D : UL;

        // Find the correct connection involved in the mode switch
        for (auto& it : connDesc_) {
            MacCid cid = it.first;
            FlowControlInfo *lteInfo = &(it.second);

            if (lteInfo->getD2dRxPeerId() == peerId && (Direction)lteInfo->getDirection() == oldDirection) {
                EV << NOW << " LteMacUeD2D::macHandleD2DModeSwitch - found old connection with cid " << cid << ", erasing buffered data" << endl;
                if (oldDirection != newDirection) {
                    if (switchPkt->getClearRlcBuffer()) {
                        EV << NOW << " LteMacUeD2D::macHandleD2DModeSwitch - erasing buffered data" << endl;

                        // Empty virtual buffer for the selected cid
                        LteMacBufferMap::iterator macBuff_it = macBuffers_.find(cid);
                        if (macBuff_it != macBuffers_.end()) {
                            while (!(macBuff_it->second->isEmpty()))
                                macBuff_it->second->popFront();
                            delete macBuff_it->second;
                            macBuffers_.erase(macBuff_it);
                        }

                        // Empty real buffer for the selected cid (they should be already empty)
                        LteMacBuffers::iterator mBuf_it = mbuf_.find(cid);
                        if (mBuf_it != mbuf_.end()) {
                            while (mBuf_it->second->getQueueLength() > 0) {
                                cPacket *pdu = mBuf_it->second->popFront();
                                delete pdu;
                            }
                            delete mBuf_it->second;
                            mbuf_.erase(mBuf_it);
                        }
                    }

                    if (switchPkt->getInterruptHarq()) {
                        EV << NOW << " LteMacUeD2D::macHandleD2DModeSwitch - interrupting H-ARQ processes" << endl;

                        // Interrupt H-ARQ processes for SL
                        MacNodeId id = peerId;
                        for (auto& mtit : harqTxBuffers_) {
                            HarqTxBuffers::iterator hit = mtit.second.find(id);
                            if (hit != mtit.second.end()) {
                                for (int proc = 0; proc < (unsigned int)UE_TX_HARQ_PROCESSES; proc++) {
                                    hit->second->forceDropProcess(proc);
                                }
                            }

                            // Interrupt H-ARQ processes for UL
                            id = getMacCellId();
                            hit = mtit.second.find(id);
                            if (hit != mtit.second.end()) {
                                for (int proc = 0; proc < (unsigned int)UE_TX_HARQ_PROCESSES; proc++) {
                                    hit->second->forceDropProcess(proc);
                                }
                            }
                        }
                    }
                }

                // Abort BSR requests
                bsrTriggered_ = false;

                auto pktDup = pkt->dup();
                auto switchPkt_dup = pktDup->removeAtFront<D2DModeSwitchNotification>();
                switchPkt_dup->setOldConnection(true);
                pktDup->insertAtFront(switchPkt_dup);
                *(pktDup->addTagIfAbsent<FlowControlInfo>()) = *lteInfo;
                sendUpperPackets(pktDup);

                if (oldDirection != newDirection && switchPkt->getClearRlcBuffer()) {
                    EV << NOW << " LteMacUeD2D::macHandleD2DModeSwitch - clearing LCG map" << endl;

                    // Remove entry from lcgMap
                    for (auto lt = lcgMap_.begin(); lt != lcgMap_.end(); ) {
                        if (lt->second.first == cid) {
                            lt = lcgMap_.erase(lt);
                        }
                        else {
                            ++lt;
                        }
                    }
                }
                EV << NOW << " LteMacUeD2D::macHandleD2DModeSwitch - send switch signal to the RLC TX entity corresponding to the old mode, cid " << cid << endl;
            }
            else if (lteInfo->getD2dRxPeerId() == peerId && (Direction)lteInfo->getDirection() == newDirection) {
                EV << NOW << " LteMacUeD2D::macHandleD2DModeSwitch - send switch signal to the RLC TX entity corresponding to the new mode, cid " << cid << endl;
                if (oldDirection != newDirection) {

                    auto pktDup = pkt->dup();
                    auto switchPkt_dup = pktDup->removeAtFront<D2DModeSwitchNotification>();
                    switchPkt_dup->setOldConnection(false);
                    // switchPkt_dup->setSchedulingPriority(1);        // always after the old mode
                    pktDup->insertAtFront(switchPkt_dup);
                    *(pktDup->addTagIfAbsent<FlowControlInfo>()) = *lteInfo;
                    sendUpperPackets(pktDup);
                }
            }
        }
    }
    else { // rx side
        Direction newDirection = (newMode == DM) ? D2D : DL;
        Direction oldDirection = (oldMode == DM) ? D2D : DL;

        // Find the correct connection involved in the mode switch
        for (auto& item : connDescIn_) {
            MacCid cid = item.first;
            FlowControlInfo *lteInfo = &(item.second);
            if (lteInfo->getD2dTxPeerId() == peerId && (Direction)lteInfo->getDirection() == oldDirection) {
                EV << NOW << " LteMacUeD2D::macHandleD2DModeSwitch - found old connection with cid " << cid << ", send signal to the RLC RX entity" << endl;
                if (oldDirection != newDirection) {
                    if (switchPkt->getInterruptHarq()) {
                        // Interrupt H-ARQ processes for SL
                        MacNodeId id = peerId;
                        for (auto& mrit : harqRxBuffers_) {
                            HarqRxBuffers::iterator hit = mrit.second.find(id);
                            if (hit != mrit.second.end()) {
                                for (unsigned int proc = 0; proc < (unsigned int)UE_RX_HARQ_PROCESSES; proc++) {
                                    unsigned int numUnits = hit->second->getProcess(proc)->getNumHarqUnits();
                                    for (unsigned int i = 0; i < numUnits; i++) {
                                        hit->second->getProcess(proc)->purgeCorruptedPdu(i); // delete contained PDU
                                        hit->second->getProcess(proc)->resetCodeword(i);     // reset unit
                                    }
                                }
                            }
                        }

                        // Clear mirror H-ARQ buffers
                        enb_->deleteHarqBuffersMirrorD2D(peerId, nodeId_);

                        // Notify that this UE is switching during this TTI
                        resetHarq_[peerId] = NOW;
                    }

                    auto pktDup = pkt->dup();
                    auto switchPkt_dup = pktDup->removeAtFront<D2DModeSwitchNotification>();
                    switchPkt_dup->setOldConnection(true);
                    pktDup->insertAtFront(switchPkt_dup);
                    *(pktDup->addTagIfAbsent<FlowControlInfo>()) = *lteInfo;
                    sendUpperPackets(pktDup);
                }
            }
            else if (lteInfo->getD2dTxPeerId() == peerId && (Direction)lteInfo->getDirection() == newDirection) {
                EV << NOW << " LteMacUeD2D::macHandleD2DModeSwitch - found new connection with cid " << cid << ", send signal to the RLC RX entity" << endl;
                if (oldDirection != newDirection) {

                    auto pktDup = pkt->dup();
                    auto switchPkt_dup = pktDup->removeAtFront<D2DModeSwitchNotification>();
                    switchPkt_dup->setOldConnection(false);
                    pktDup->insertAtFront(switchPkt_dup);
                    *(pktDup->addTagIfAbsent<FlowControlInfo>()) = *lteInfo;
                    sendUpperPackets(pktDup);
                }
            }
        }
    }

    delete pkt;
}

void LteMacUeD2D::doHandover(MacNodeId targetEnb)
{
    if (targetEnb == NODEID_NONE)
        enb_ = nullptr;
    else {
        if (preconfiguredTxParams_ != nullptr)
            delete preconfiguredTxParams_;
        preconfiguredTxParams_ = getPreconfiguredTxParams();
        enb_ = check_and_cast<LteMacEnbD2D *>(getMacByMacNodeId(binder_, targetEnb));
    }
    LteMacUe::doHandover(targetEnb);
}

} //namespace

