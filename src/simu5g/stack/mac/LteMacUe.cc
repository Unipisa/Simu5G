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

#include "stack/mac/LteMacUe.h"

#include <inet/networklayer/ipv4/Ipv4InterfaceData.h>

#include "corenetwork/statsCollector/UeStatsCollector.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/packet/LteMacSduRequest.h"
#include "stack/mac/packet/LteRac_m.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/scheduler/LteSchedulerUeUl.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"

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
    if (stage == INITSTAGE_LOCAL) {
    }
    else if (stage == INITSTAGE_LINK_LAYER) {
        if (strcmp(getFullName(), "nrMac") == 0)
            cellId_ = MacNodeId(networkNode_->par("nrMasterId").intValue());
        else
            cellId_ = MacNodeId(networkNode_->par("masterId").intValue());
    }
    else if (stage == INITSTAGE_NETWORK_LAYER) {
        if (strcmp(getFullName(), "nrMac") == 0)
            nodeId_ = MacNodeId(networkNode_->par("nrMacNodeId").intValue());
        else
            nodeId_ = MacNodeId(networkNode_->par("macNodeId").intValue());

        // Insert UeInfo in the Binder
        UeInfo *info = new UeInfo();
        info->id = nodeId_;            // local mac ID
        info->cellId = cellId_;        // cell ID
        info->init = false;            // flag for phy initialization
        info->ue = networkNode_;  // reference to the UE module
        info->phy = phy_;

        binder_->addUeInfo(info);

        if (cellId_ != NODEID_NONE) {
            LteAmc *amc = check_and_cast<LteMacEnb *>(getMacByMacNodeId(binder_, cellId_))->getAmc();
            amc->attachUser(nodeId_, UL);
            amc->attachUser(nodeId_, DL);

            /*
             * @author Alessandro Noferi
             *
             * This piece of code connects the UeCollector to the relative base station Collector.
             * It checks the NIC, i.e. Lte or NR and chooses the correct UeCollector to connect.
             */

            cModule *module = binder_->getModuleByMacNodeId(cellId_);
            std::string nodeType;
            if (module->hasPar("nodeType"))
                nodeType = module->par("nodeType").stdstringValue();

            RanNodeType eNBType = binder_->getBaseStationTypeById(cellId_);

            if (isNrUe(nodeId_) && eNBType == GNODEB) {
                EV << "I am a NR Ue with node id: " << nodeId_ << " connected to gnb with id: " << cellId_ << endl;
                if (!par("collectorModule").isEmptyString()) {
                    UeStatsCollector *ue = getModuleFromPar<UeStatsCollector>(par("collectorModule"), this);
                    binder_->addUeCollectorToEnodeB(nodeId_, ue, cellId_);
                }
            }
            else if (!isNrUe(nodeId_) && eNBType == ENODEB) {
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
    else if (stage == inet::INITSTAGE_TRANSPORT_LAYER) {
        const auto *channelModels = phy_->getChannelModels();
        for (const auto& cm : *channelModels) {
            lcgScheduler_[cm.first] = new LteSchedulerUeUl(this, cm.first);
        }
    }
    else if (stage == inet::INITSTAGE_LAST) {

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
        if (grant.second == nullptr)
            continue;

        for (size_t cw = 0; cw < grant.second->getGrantedCwBytesArraySize(); cw++)
            allocatedBytes.push_back(grant.second->getGrantedCwBytes(cw));
    }

    // Ask for a MAC sdu for each scheduled user on each codeword
    for (auto& cit : scheduleList_) {
        LteMacScheduleList::const_iterator it;
        for (const auto& it : *cit.second) {
            MacCid destCid = it.first.first;
            Codeword cw = it.first.second;
            MacNodeId destId = MacCidToNodeId(destCid);

            auto key = std::make_pair(destCid, cw);
            LteMacScheduleList *scheduledBytesList = lcgScheduler_[cit.first]->getScheduledBytesList();
            auto bit = scheduledBytesList->find(key);

            // consume bytes on this codeword
            if (bit == scheduledBytesList->end())
                throw cRuntimeError("LteMacUe::macSduRequest - cannot find entry in scheduledBytesList");
            else {
                allocatedBytes[cw] -= bit->second;

                EV << NOW << " LteMacUe::macSduRequest - cid[" << destCid << "] - sdu size[" << bit->second << "B] - " << allocatedBytes[cw] << " bytes left on codeword " << cw << endl;

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

bool LteMacUe::bufferizePacket(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    if (pkt->getBitLength() <= 1) { // no data in this packet - should not be buffered
        delete pkt;
        return false;
    }

    pkt->setTimestamp();           // add timestamp with current time to packet

    auto lteInfo = pkt->getTagForUpdate<FlowControlInfo>();

    // obtain the cid from the packet information
    MacCid cid = ctrlInfoToMacCid(lteInfo);

    // this packet is used to signal the arrival of new data in the RLC buffers
    if (checkIfHeaderType<LteRlcPduNewData>(pkt)) {
        // update the virtual buffer for this connection

        // build the virtual packet corresponding to this incoming packet
        pkt->popAtFront<LteRlcPduNewData>();
        auto rlcSdu = pkt->peekAtFront<LteRlcSdu>();
        PacketInfo vpkt(rlcSdu->getLengthMainPacket(), pkt->getTimestamp());

        LteMacBufferMap::iterator it = macBuffers_.find(cid);
        if (it == macBuffers_.end()) {
            LteMacBuffer *vqueue = new LteMacBuffer();
            vqueue->pushBack(vpkt);
            macBuffers_[cid] = vqueue;

            // make a copy of lte control info and store it to traffic descriptors map
            FlowControlInfo toStore(*lteInfo);
            connDesc_[cid] = toStore;
            // register connection to lcg map.
            LteTrafficClass tClass = (LteTrafficClass)lteInfo->getTraffic();

            lcgMap_.insert(LcgPair(tClass, CidBufferPair(cid, macBuffers_[cid])));

            EV << "LteMacBuffers : Using new buffer on node: " <<
                MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Bytes in the Queue: " <<
                vqueue->getQueueOccupancy() << "\n";
        }
        else {
            LteMacBuffer *vqueue = nullptr;
            LteMacBufferMap::iterator it = macBuffers_.find(cid);
            if (it != macBuffers_.end())
                vqueue = it->second;

            if (vqueue != nullptr) {
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

    // this is a MAC SDU, buffer it in the MAC buffer

    LteMacBuffers::iterator it = mbuf_.find(cid);
    if (it == mbuf_.end()) {
        // Queue not found for this cid: create
        LteMacQueue *queue = new LteMacQueue(queueSize_);

        queue->pushBack(pkt);

        mbuf_[cid] = queue;

        EV << "LteMacBuffers : Using new buffer on node: " <<
            MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
            queue->getQueueSize() - queue->getByteLength() << "\n";
    }
    else {
        // Found
        LteMacQueue *queue = it->second;
        if (!queue->pushBack(pkt)) {
            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());
            if (lteInfo->getDirection() == DL) {
                emit(macBufferOverflowDlSignal_, sample);
            }
            else {
                emit(macBufferOverflowUlSignal_, sample);
            }

            EV << "LteMacBuffers : Dropped packet: queue" << cid << " is full\n";

            // @author Alessandro Noferi
            // discard the RLC
            if (packetFlowManager_ != nullptr) {
                unsigned int rlcSno = check_and_cast<LteRlcUmDataPdu *>(pkt)->getPduSequenceNumber();
                packetFlowManager_->discardRlcPdu(lteInfo->getLcid(), rlcSno);
            }

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
    int64_t size = 0;

    macPduList_.clear();

    // Build a MAC PDU for each scheduled user on each codeword
    for (auto [carrierFreq, schList] : scheduleList_) {
        Packet *macPkt = nullptr;

        for (auto& item : *schList) {
            MacCid destCid = item.first.first;
            Codeword cw = item.first.second;

            // from a UE perspective, the destId is always the one of the eNB
            MacNodeId destId = getMacCellId();

            std::pair<MacNodeId, Codeword> pktId = {destId, cw};
            unsigned int sduPerCid = item.second;

            if (macPduList_.find(carrierFreq) == macPduList_.end()) {
                MacPduList newList;
                macPduList_[carrierFreq] = newList;
            }
            auto pit = macPduList_[carrierFreq].find(pktId);

            // No packets for this user on this codeword
            if (pit == macPduList_[carrierFreq].end()) {
                macPkt = new Packet("LteMacPdu");
                auto header = makeShared<LteMacPdu>();
                header->setHeaderLength(MAC_HEADER);
                macPkt->insertAtFront(header);

                macPkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                macPkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                macPkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
                macPkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(schedulingGrant_[carrierFreq]->getUserTxParams()->dup());
                /*
                 * @author Alessandro Noferi
                 * retrieve the grantId from the grant object in schedulingGrant_[carrierFreq]
                 * and add it as tag for this macPkt.
                 *
                 * This is useful at eNB side to calculate the packet delay
                 */
                macPkt->addTagIfAbsent<UserControlInfo>()->setGrantId(schedulingGrant_[carrierFreq]->getGrantId());
                macPkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);

                //macPkt->setControlInfo(uinfo);
                macPkt->setTimestamp(NOW);
                macPduList_[carrierFreq][pktId] = macPkt;
            }
            else {
                macPkt = pit->second;
            }

            while (sduPerCid > 0) {
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

    // Put MAC PDUs in H-ARQ buffers

    for (auto& lit : macPduList_) {
        double carrierFreq = lit.first;

        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end()) {
            HarqTxBuffers newHarqTxBuffers;
            harqTxBuffers_[carrierFreq] = newHarqTxBuffers;
        }
        HarqTxBuffers& harqTxBuffers = harqTxBuffers_[carrierFreq];

        for (auto & pit : lit.second) {

            MacNodeId destId = pit.first.first;
            Codeword cw = pit.first.second;

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
                LteHarqBufferTx *hb = new LteHarqBufferTx(binder_, (unsigned int)ENB_TX_HARQ_PROCESSES, this,
                        check_and_cast<LteMacBase *>(getMacByMacNodeId(binder_, cellId_)));
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }

            // search for an empty unit within the current HARQ process
            UnitList txList = txBuf->getEmptyUnits(currentHarq_);
            EV << "LteMacUe::macPduMake - [Used Acid=" << (unsigned int)txList.first << "] , [curr=" << (unsigned int)currentHarq_ << "]" << endl;

            auto macPkt = pit.second;

            // BSR related operations

            // according to the TS 36.321 v8.7.0, when there are uplink resources assigned to the UE, a BSR
            // has to be sent even if there is no data in the user's queues. In few words, a BSR is always
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

            bool bsrAlreadyMade = false;
            auto header = macPkt->removeAtFront<LteMacPdu>();
            if (bsrTriggered_) {
                MacBsr *bsr = new MacBsr();

                bsr->setTimestamp(simTime().dbl());
                bsr->setSize(size);
                header->pushCe(bsr);

                bsrTriggered_ = false;
                bsrAlreadyMade = true;

                EV << "LteMacUe::macPduMake - BSR with size " << size << " created" << endl;
            }

            if (bsrAlreadyMade && size > 0)                                              // this prevents the UE from sending an unnecessary RAC request
                bsrRtxTimer_ = bsrRtxTimerStart_;
            else
                bsrRtxTimer_ = 0;

            // insert updated MacPdu
            macPkt->insertAtFront(header);

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

void LteMacUe::macPduUnmake(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto macPkt = pkt->removeAtFront<LteMacPdu>();
    while (macPkt->hasSdu()) {
        // Extract and send SDU
        auto upPkt = macPkt->popSdu();
        take(upPkt);

        EV << "LteMacBase: pduUnmaker extracted SDU" << endl;

        // store descriptor for the incoming connection, if not already stored
        auto lteInfo = upPkt->getTag<FlowControlInfo>();
        MacNodeId senderId = lteInfo->getSourceId();
        LogicalCid lcid = lteInfo->getLcid();
        MacCid cid = idToMacCid(senderId, lcid);
        if (connDescIn_.find(cid) == connDescIn_.end()) {
            FlowControlInfo toStore(*lteInfo);
            connDescIn_[cid] = toStore;
        }
        sendUpperPackets(upPkt);
    }

    pkt->insertAtFront(macPkt);

    ASSERT(pkt->getOwner() == this);
    delete pkt;
}

void LteMacUe::handleUpperMessage(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    bool isLteRlcPduNewDataInd = checkIfHeaderType<LteRlcPduNewData>(pkt);

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
    for (auto& mit : harqRxBuffers_) {
        std::list<Packet *> pduList;

        for (auto& hit : mit.second) {
            pduList = hit.second->extractCorrectPdus();
            while (!pduList.empty()) {
                auto pdu = pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    EV << NOW << "LteMacUe::handleSelfMessage " << nodeId_ << " - HARQ process " << (unsigned int)currentHarq_ << endl;
    // updating current HARQ process for next TTI

    // no grant available - if user has backlogged data, it will trigger scheduling request
    // no HARQ counter is updated since no transmission is sent.

    bool noSchedulingGrants = true;
    for (auto& git : schedulingGrant_) {
        if (git.second != nullptr) {
            noSchedulingGrants = false;
            break;
        }
    }

    if (noSchedulingGrants) {
        EV << NOW << " LteMacUe::handleSelfMessage " << nodeId_ << " NO configured grant" << endl;

        // if necessary, a RAC request will be sent to obtain a grant
        checkRAC();

        // TODO ensure all operations are done before return ( i.e. move H-ARQ RX purge before this point)
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
    if (!noSchedulingGrants) { // if a grant is configured for at least one carrier
        if (!firstTx) {
            EV << "\t currentHarq_ counter initialized " << endl;
            firstTx = true;
            // the eNB will receive the first PDU in 2 TTIs, thus initializing ACID to 0
            currentHarq_ = UE_TX_HARQ_PROCESSES - 2;
        }

        EV << NOW << " LteMacUe::handleSelfMessage " << nodeId_ << " entered scheduling" << endl;

        bool retx = false;

        for (auto& mtit : harqTxBuffers_) {
            double carrierFrequency = mtit.first;

            // skip if no grant is configured for this carrier
            if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end() || schedulingGrant_[carrierFrequency] == nullptr)
                continue;

            for (auto& it2 : mtit.second) {
                EV << "\t Looking for retransmission in ACID " << (unsigned int)currentHarq_ << endl;
                LteHarqBufferTx *currHarq = it2.second;

                // check if the current process has units ready for retransmission
                retx = currHarq->getProcess(currentHarq_)->hasReadyUnits();
                CwList cwListRetx = currHarq->getProcess(currentHarq_)->readyUnitsIds();

                EV << "\t [process=" << (unsigned int)currentHarq_ << "] , [reTX=" << ((retx) ? "true" : "false")
                   << "] , [n=" << cwListRetx.size() << "]" << endl;

                // if a retransmission is needed
                if (retx) {
                    UnitList signal;
                    signal.first = currentHarq_;
                    signal.second = cwListRetx;
                    currHarq->markSelected(signal, schedulingGrant_[carrierFrequency]->getUserTxParams()->getLayers().size());
                }
            }
        }
        // if no retransmission is needed, proceed with normal scheduling
        if (!retx) {
            for (auto [carrierFrequency, carrierLcgScheduler] : lcgScheduler_) {

                // skip if no grant is configured for this carrier
                if (schedulingGrant_.find(carrierFrequency) == schedulingGrant_.end() || schedulingGrant_[carrierFrequency] == nullptr)
                    continue;

                LteMacScheduleList *carrierScheduleList = carrierLcgScheduler->schedule();
                scheduleList_[carrierFrequency] = carrierScheduleList;
            }

            requestedSdus_ = macSduRequest();
            if (requestedSdus_ == 0) {
                // no data to send, but if bsrTriggered is set, send a BSR
                macPduMake();
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
        // TODO make this part optional to save computations
        for (auto& mtit : harqTxBuffers_) {
            EV << "\n carrier[ " << mtit.first << "] htxbuf.size " << mtit.second.size() << endl;

            //TODO const variables
            int cntOuter = 0;
            int cntInner = 0;
            for (auto [nodeId, currHarq] : mtit.second) {
                BufferStatus harqStatus = currHarq->getBufferStatus();

                EV << "\t cycleOuter " << cntOuter << " - bufferStatus.size=" << harqStatus.size() << endl;
                for (const auto& unitStatusVector : harqStatus) {
                    EV << "\t\t cycleInner " << cntInner << " - jt->size=" << unitStatusVector.size()
                       << " - statusCw(0/1)=" << unitStatusVector.at(0).second << "/" << unitStatusVector.at(1).second << endl;
                }
            }
        }
    }
    //======================== END DEBUG ==========================

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
    double carrierFrequency = userInfo->getCarrierFrequency();

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

    for (const auto& it : macBuffers_) {
        if (!(it.second->isEmpty())) {
            trigger = true;
            break;
        }
    }

    if (!trigger)
        EV << NOW << "UE " << nodeId_ << ", RAC aborted, no data in queues " << endl;

    if ((racRequested_ = trigger)) {
        auto pkt = new Packet("RacRequest");

        auto racReq = makeShared<LteRac>();
        pkt->insertAtFront(racReq);

        double carrierFrequency = phy_->getPrimaryChannelModel()->getCarrierFrequency();
        pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFrequency);
        pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDestId(getMacCellId());
        pkt->addTagIfAbsent<UserControlInfo>()->setDirection(UL);
        pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(RACPKT);

        sendLowerPackets(pkt);

        EV << NOW << " UE  " << nodeId_ << " cell " << cellId_ << " ,RAC request sent to PHY " << endl;

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
    for (auto& mtit : harqTxBuffers_) {
        for (auto& it2 : mtit.second)
            it2.second->sendSelectedDown();
    }

    // deleting non-periodic grant
    for (auto& git : schedulingGrant_) {
        if (git.second != nullptr && !(git.second->getPeriodic())) {
            git.second = nullptr;
        }
    }
}

bool LteMacUe::getHighestBackloggedFlow(MacCid& cid, unsigned int& priority)
{
    // TODO : optimize if inefficient
    // TODO : implement priorities and LCGs
    // search in virtual buffer structures

    for (const auto& item : macBuffers_) {
        if (!item.second->isEmpty()) {
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
    for (LteMacBufferMap::const_reverse_iterator it = macBuffers_.rbegin(); it != macBuffers_.rend(); ++it) {
        if (!it->second->isEmpty()) {
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

    for (auto mit = mbuf_.begin(); mit != mbuf_.end(); ) {
        while (!mit->second->isEmpty()) {
            cPacket *pkt = mit->second->popFront();
            delete pkt;
        }
        delete mit->second;        // Delete Queue
        mit = mbuf_.erase(mit);        // Delete Element
    }
    for (auto vit = macBuffers_.begin(); vit != macBuffers_.end(); ) {
        while (!vit->second->isEmpty())
            vit->second->popFront();
        delete vit->second;                  // Delete Queue
        vit = macBuffers_.erase(vit);           // Delete Element
    }

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
    connDesc_.clear();
}

} //namespace

