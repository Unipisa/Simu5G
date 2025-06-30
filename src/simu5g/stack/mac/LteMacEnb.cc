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

#include <string>

#include <inet/common/ModuleAccess.h>
#include <inet/common/TimeTag_m.h>

#include "common/LteCommon.h"
#include "stack/mac/LteMacEnb.h"
#include "stack/mac/LteMacUe.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/buffer/LteMacQueue.h"
#include "stack/phy/packet/LteFeedbackPkt.h"
#include "stack/mac/scheduler/LteSchedulerEnbDl.h"
#include "stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "stack/mac/packet/LteSchedulingGrant.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/amc/NRAmc.h"
#include "stack/mac/amc/UserTxParams.h"
#include "stack/mac/packet/LteRac_m.h"
#include "stack/mac/packet/LteMacSduRequest.h"
#include "stack/phy/LtePhyBase.h"
#include "stack/rlc/packet/LteRlcDataPdu.h"
#include "stack/rlc/am/packet/LteRlcAmPdu_m.h"
#include "stack/packetFlowManager/PacketFlowManagerBase.h"
#include "stack/rlc/um/LteRlcUm.h"
#include "stack/pdcp_rrc/NRPdcpRrcEnb.h"

namespace simu5g {

Define_Module(LteMacEnb);

using namespace omnetpp;

/*********************
* PUBLIC FUNCTIONS
*********************/

LteMacEnb::LteMacEnb() :
    LteMacBase()
{
    nodeType_ = ENODEB;
}

LteMacEnb::~LteMacEnb()
{
    delete amc_;
    delete enbSchedulerDl_;
    delete enbSchedulerUl_;

    for (auto &[key, value] : bsrbuf_)
        delete value;
}

/***********************
* PROTECTED FUNCTIONS
***********************/

CellInfo *LteMacEnb::getCellInfo()
{
    // Get local cellInfo
    return cellInfo_.get();
}

int LteMacEnb::getNumAntennas()
{
    // Get number of antennas: +1 is for MACRO
    return cellInfo_->getNumRus() + 1;
}

SchedDiscipline LteMacEnb::getSchedDiscipline(Direction dir)
{
    if (dir == DL)
        return aToSchedDiscipline(
                par("schedulingDisciplineDl").stdstringValue());
    else if (dir == UL)
        return aToSchedDiscipline(
                par("schedulingDisciplineUl").stdstringValue());
    else {
        throw cRuntimeError("LteMacEnb::getSchedDiscipline(): unrecognized direction %d", (int)dir);
    }
}

void LteMacEnb::deleteQueues(MacNodeId nodeId)
{
    Enter_Method_Silent();

    LteMacBase::deleteQueues(nodeId);

    LteMacBufferMap::iterator bit;
    for (bit = bsrbuf_.begin(); bit != bsrbuf_.end(); ) {
        if (MacCidToNodeId(bit->first) == nodeId) {
            delete bit->second;
            bit = bsrbuf_.erase(bit);
        }
        else {
            ++bit;
        }
    }

    // remove active connections from the schedulers
    enbSchedulerDl_->removeActiveConnections(nodeId);
    enbSchedulerUl_->removeActiveConnections(nodeId);

    // remove pending RAC requests
    enbSchedulerUl_->removePendingRac(nodeId);
}

void LteMacEnb::initialize(int stage)
{
    LteMacBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        nodeId_ = MacNodeId(networkNode_->par("macNodeId").intValue());

        cellId_ = nodeId_;

        cellInfo_.reference(this, "cellInfoModule", true);

        // Get number of antennas
        numAntennas_ = getNumAntennas();

        eNodeBCount = par("eNodeBCount");
        WATCH(numAntennas_);
        WATCH_MAP(bsrbuf_);
    }
    else if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT) {
        // Create and initialize AMC module
        std::string amcType = par("amcType").stdstringValue();
        if (amcType == "NRAmc")
            amc_ = new NRAmc(this, binder_, cellInfo_, numAntennas_);
        else if (amcType == "LteAmc")
            amc_ = new LteAmc(this, binder_, cellInfo_, numAntennas_);
        else
            throw cRuntimeError("The amcType '%s' not recognized", amcType.c_str());

        std::string modeString = par("pilotMode").stdstringValue();

        // TODO use cEnum::get("simu5g::PilotComputationModes")->lookup(modeString);
        if (modeString == "AVG_CQI")
            amc_->setPilotMode(AVG_CQI);
        else if (modeString == "MAX_CQI")
            amc_->setPilotMode(MAX_CQI);
        else if (modeString == "MIN_CQI")
            amc_->setPilotMode(MIN_CQI);
        else if (modeString == "MEDIAN_CQI")
            amc_->setPilotMode(MEDIAN_CQI);
        else if (modeString == "ROBUST_CQI")
            amc_->setPilotMode(ROBUST_CQI);
        else
            throw cRuntimeError("LteMacEnb::initialize - Unknown Pilot Mode %s \n", modeString.c_str());

        cModule *hostModule = getContainingNode(this);

        // Insert EnbInfo in the Binder
        EnbInfo *info = new EnbInfo();
        info->id = nodeId_;            // local MAC ID
        info->nodeType = nodeType_;    // eNB or gNB
        info->type = MACRO_ENB;        // eNB Type
        info->init = false;            // flag for PHY initialization
        info->eNodeB = hostModule;  // reference to the eNodeB module
        binder_->addEnbInfo(info);

        // register the pairs <id,name> and <id, module> to the binder
        // Modified from getFullName() to getFullPath() to fix the usage in compound modules
        binder_->registerName(nodeId_, hostModule->getFullPath());
        binder_->registerModule(nodeId_, hostModule);
    }
    else if (stage == inet::INITSTAGE_LINK_LAYER) {
        // Create and initialize MAC Downlink scheduler
        if (enbSchedulerDl_ == nullptr) {
            enbSchedulerDl_ = new LteSchedulerEnbDl();
            (enbSchedulerDl_->resourceBlocks()) = cellInfo_->getNumBands();
            enbSchedulerDl_->initialize(DL, this, binder_);
        }

        // Create and initialize MAC Uplink scheduler
        if (enbSchedulerUl_ == nullptr) {
            enbSchedulerUl_ = new LteSchedulerEnbUl();
            (enbSchedulerUl_->resourceBlocks()) = cellInfo_->getNumBands();
            enbSchedulerUl_->initialize(UL, this, binder_);
        }

        const CarrierInfoMap *carriers = cellInfo_->getCarrierInfoMap();
        int i = 0;
        for (const auto& item : *carriers) {
            double carrierFrequency = item.second.carrierFrequency;
            bgTrafficManager_[carrierFrequency] = check_and_cast<IBackgroundTrafficManager *>(getParentModule()->getSubmodule("bgTrafficGenerator", i)->getSubmodule("manager"));
            bgTrafficManager_[carrierFrequency]->setCarrierFrequency(carrierFrequency);
            ++i;
        }
    }
    else if (stage == inet::INITSTAGE_LAST) {
        // Start TTI tick
        // the period is equal to the minimum period according to the numerologies used by the carriers in this node
        ttiTick_ = new cMessage("ttiTick_");
        ttiTick_->setSchedulingPriority(1);                                              // TTI TICK after other messages
        ttiPeriod_ = binder_->getSlotDurationFromNumerologyIndex(cellInfo_->getMaxNumerologyIndex());
        scheduleAt(NOW + ttiPeriod_, ttiTick_);

        const CarrierInfoMap *carriers = cellInfo_->getCarrierInfoMap();
        for (const auto& item : *carriers) {
            // set periodicity for this carrier according to its numerology
            NumerologyPeriodCounter info;
            info.max = 1 << (cellInfo_->getMaxNumerologyIndex() - item.second.numerologyIndex); // 2^(maxNumerologyIndex - numerologyIndex)
            info.current = info.max - 1;
            numerologyPeriodCounter_[item.second.numerologyIndex] = info;
        }

        // set the periodicity for each scheduler
        enbSchedulerDl_->initializeSchedulerPeriodCounter(cellInfo_->getMaxNumerologyIndex());
        enbSchedulerUl_->initializeSchedulerPeriodCounter(cellInfo_->getMaxNumerologyIndex());
    }
}

void LteMacEnb::handleMessage(cMessage *msg)
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

void LteMacEnb::macSduRequest()
{
    EV << "----- START LteMacEnb::macSduRequest -----\n";

    // Ask for a MAC SDU for each scheduled user on each carrier and each codeword
    std::map<double, LteMacScheduleList>::iterator cit;
    for (const auto& cit : *scheduleListDl_) { // loop on carriers

        for (const auto& item : cit.second) { // loop on CIDs
            MacCid destCid = item.first.first;
            // Codeword cw = item.first.second;
            MacNodeId destId = MacCidToNodeId(destCid);

            // for each band, count the number of bytes allocated for this UE (should be by CID)
            unsigned int allocatedBytes = 0;
            int numBands = cellInfo_->getNumBands();
            for (Band b = 0; b < numBands; b++) {
                // get the number of bytes allocated to this connection
                // (this represents the MAC PDU size)
                allocatedBytes += enbSchedulerDl_->allocator_->getBytes(MACRO, b, destId);
            }

            // send the request message to the upper layer
            auto pkt = new Packet("LteMacSduRequest");
            auto macSduRequest = makeShared<LteMacSduRequest>();
            macSduRequest->setUeId(destId);
            macSduRequest->setChunkLength(b(1)); // TODO: should be 0
            macSduRequest->setUeId(destId);
            macSduRequest->setLcid(MacCidToLcid(destCid));
            macSduRequest->setSduSize(allocatedBytes - MAC_HEADER);    // do not consider MAC header size
            pkt->insertAtFront(macSduRequest);
            if (queueSize_ != 0 && queueSize_ < macSduRequest->getSduSize()) {
                throw cRuntimeError("LteMacEnb::macSduRequest: configured queueSize too low - requested SDU will not fit in queue!"
                                    " (queue size: %d, SDU request requires: %d)", queueSize_, macSduRequest->getSduSize());
            }
            auto tag = pkt->addTag<FlowControlInfo>();
            *tag = connDesc_[destCid];
            sendUpperPackets(pkt);
        }
    }
    EV << "------ END LteMacEnb::macSduRequest ------\n";
}

void LteMacEnb::bufferizeBsr(MacBsr *bsr, MacCid cid)
{
    LteMacBufferMap::iterator it = bsrbuf_.find(cid);
    if (it == bsrbuf_.end()) {
        if (bsr->getSize() > 0) {
            // Queue not found for this CID: create
            LteMacBuffer *bsrqueue = new LteMacBuffer();

            PacketInfo vpkt(bsr->getSize(), bsr->getTimestamp());
            bsrqueue->pushBack(vpkt);
            bsrbuf_[cid] = bsrqueue;

            EV << "LteBsrBuffers : Added new BSR buffer for node: "
               << MacCidToNodeId(cid) << " for LCID: " << MacCidToLcid(cid)
               << " Current BSR size: " << bsr->getSize() << "\n";

            // signal backlog to Uplink scheduler
            enbSchedulerUl_->backlog(cid);
        }
        // do not store if BSR size = 0
    }
    else {
        // Found
        LteMacBuffer *bsrqueue = it->second;
        if (bsr->getSize() > 0) {
            // update buffer
            PacketInfo queuedBsr;
            if (!bsrqueue->isEmpty())
                queuedBsr = bsrqueue->popFront();

            queuedBsr.first = bsr->getSize();
            queuedBsr.second = bsr->getTimestamp();
            bsrqueue->pushBack(queuedBsr);

            EV << "LteBsrBuffers : Using old buffer for node: " << MacCidToNodeId(
                    cid) << " for LCID: " << MacCidToLcid(cid)
               << " Current BSR size: " << bsr->getSize() << "\n";

            // signal backlog to Uplink scheduler
            enbSchedulerUl_->backlog(cid);
        }
        else {
            // the UE has no backlog, remove BSR
            if (!bsrqueue->isEmpty())
                bsrqueue->popFront();

            EV << "LteBsrBuffers : Using old buffer for node: " << MacCidToNodeId(
                    cid) << " for LCID: " << MacCidToLcid(cid)
               << " - now empty" << "\n";
        }
    }
}

void LteMacEnb::sendGrants(std::map<double, LteMacScheduleList> *scheduleList)
{
    EV << NOW << "LteMacEnb::sendGrants " << endl;

    for (auto& citem : *scheduleList) {
        LteMacScheduleList& carrierScheduleList = citem.second;
        while (!carrierScheduleList.empty()) {
            LteMacScheduleList::iterator it, ot;
            it = carrierScheduleList.begin();

            Codeword cw = 0, otherCw = 0;
            MacCid cid = 0;
            MacNodeId nodeId = NODEID_NONE;
            unsigned int codewords = 0;
            unsigned int granted = 0;
            if (it != carrierScheduleList.end()) {
                cw = it->first.second;
                otherCw = MAX_CODEWORDS - cw;

                cid = it->first.first;
                nodeId = MacCidToNodeId(cid);

                granted = it->second;

                // Removing the visited element from scheduleList.
                carrierScheduleList.erase(it);
            }
            else
                throw cRuntimeError("LteMacEnb::sendGrants - schedule list is empty.");

            if (granted > 0) {
                // Increment the number of allocated Cw
                ++codewords;
            }
            else {
                // Active cw becomes the "other one"
                cw = otherCw;
            }

            std::pair<MacCid, Codeword> otherPair(num(nodeId), otherCw); // note: MacNodeId used as MacCid

            if ((ot = (carrierScheduleList.find(otherPair))) != (carrierScheduleList.end())) {
                // Increment the number of allocated Cw
                ++codewords;

                // Removing the visited element from scheduleList.
                carrierScheduleList.erase(ot);
            }

            if (granted == 0)
                continue; // Avoiding transmission of 0 grant (0 grant should not be created)

            EV << NOW << " LteMacEnb::sendGrants Node[" << getMacNodeId() << "] - "
               << granted << " blocks to grant for user " << nodeId << " on "
               << codewords << " codewords. CW[" << cw << "\\" << otherCw << "] carrier[" << citem.first << "]" << endl;

            // TODO: change to tag instead of chunk
            // TODO: Grant is set as aperiodic by default
            auto pkt = new Packet("LteGrant");

            auto grant = makeShared<LteSchedulingGrant>();
            grant->setDirection(UL);
            grant->setCodewords(codewords);

            // Set total granted blocks
            grant->setTotalGrantedBlocks(granted);

            pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
            pkt->addTagIfAbsent<UserControlInfo>()->setDestId(nodeId);
            pkt->addTagIfAbsent<UserControlInfo>()->setFrameType(GRANTPKT);
            pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(citem.first);

            // Get and set the user's UserTxParams
            const UserTxParams& ui = getAmc()->computeTxParams(nodeId, UL, citem.first);
            UserTxParams *txPara = new UserTxParams(ui);
            grant->setUserTxParams(txPara);

            // Acquiring remote antennas set from user info
            const std::set<Remote>& antennas = ui.readAntennaSet();

            // Get bands for this carrier
            const unsigned int firstBand = cellInfo_->getCarrierStartingBand(citem.first);
            const unsigned int lastBand = cellInfo_->getCarrierLastBand(citem.first);

            // HANDLE MULTICW
            for ( ; cw < codewords; ++cw) {
                unsigned int grantedBytes = 0;

                for (Band b = firstBand; b <= lastBand; ++b) {
                    unsigned int bandAllocatedBlocks = 0;

                    for (const auto& antenna : antennas) {
                        bandAllocatedBlocks += enbSchedulerUl_->readPerUeAllocatedBlocks(nodeId, antenna, b);
                    }

                    grantedBytes += amc_->computeBytesOnNRbs(nodeId, b, cw,
                            bandAllocatedBlocks, UL, citem.first);
                }

                grant->setGrantedCwBytes(cw, grantedBytes);
                EV << NOW << " LteMacEnb::sendGrants - granting " << grantedBytes << " on cw " << cw << endl;
            }

            RbMap map;

            enbSchedulerUl_->readRbOccupation(nodeId, citem.first, map);

            grant->setGrantedBlocks(map);
            pkt->insertAtFront(grant);

            /*
             * @author Alessandro Noferi
             * Notify the pfm about the successful arrival of a TB from a UE.
             * From ETSI TS 138314 V16.0.0 (2020-07)
             *   tSched: the point in time when the UL MAC SDU i is scheduled as
             *   per the scheduling grant provided
             */
            if (packetFlowManager_ != nullptr)
                packetFlowManager_->grantSent(nodeId, grant->getGrantId());

            // Send grant to PHY layer
            sendLowerPackets(pkt);
        }
    }
}

void LteMacEnb::macHandleRac(cPacket *pktAux)
{
    EV << NOW << "LteMacEnb::macHandleRac" << endl;

    auto pkt = check_and_cast<Packet *>(pktAux);

    auto racPkt = pkt->removeAtFront<LteRac>();
    auto uinfo = pkt->getTagForUpdate<UserControlInfo>();

    enbSchedulerUl_->signalRac(uinfo->getSourceId(), uinfo->getCarrierFrequency());

    // TODO: all RACs are marked as successful
    racPkt->setSuccess(true);
    pkt->insertAtFront(racPkt);

    uinfo->setDestId(uinfo->getSourceId());
    uinfo->setSourceId(nodeId_);
    uinfo->setDirection(DL);

    sendLowerPackets(pkt);
}

void LteMacEnb::macPduMake(MacCid cid)
{
    EV << "----- START LteMacEnb::macPduMake -----\n";
    // Finalizes the scheduling decisions according to the schedule list,
    // detaching SDUs from real buffers.

    macPduList_.clear();

    // Build a MAC PDU for each scheduled user on each codeword
    for (auto& cit : *scheduleListDl_) {
        double carrierFreq = cit.first;
        for (auto& it : cit.second) {
            Packet *macPacket = nullptr;
            MacCid destCid = it.first.first;

            if (destCid != cid)
                continue;

            // Check whether the RLC has sent some data. If not, skip
            // (e.g. because the size of the MAC PDU would contain only MAC header - MAC SDU requested size = 0B)
            if (mbuf_[destCid]->getQueueLength() == 0)
                break;

            Codeword cw = it.first.second;
            MacNodeId destId = MacCidToNodeId(destCid);
            std::pair<MacNodeId, Codeword> pktId = {destId, cw};
            unsigned int sduPerCid = it.second;
            unsigned int grantedBlocks = 0;
            TxMode txmode;

            if (macPduList_.find(carrierFreq) == macPduList_.end()) {
                MacPduList newList;
                macPduList_[carrierFreq] = newList;
            }

            // Add SDUs to PDU
            auto pit = macPduList_[carrierFreq].find(pktId);

            // No packets for this user on this codeword
            if (pit == macPduList_[carrierFreq].end()) {
                auto pkt = new Packet("LteMacPdu");
                pkt->addTagIfAbsent<UserControlInfo>()->setSourceId(getMacNodeId());
                pkt->addTagIfAbsent<UserControlInfo>()->setDestId(destId);
                pkt->addTagIfAbsent<UserControlInfo>()->setDirection(DL);
                pkt->addTagIfAbsent<UserControlInfo>()->setCarrierFrequency(carrierFreq);

                const UserTxParams& txInfo = amc_->computeTxParams(destId, DL, carrierFreq);

                UserTxParams *txPara = new UserTxParams(txInfo);

                pkt->addTagIfAbsent<UserControlInfo>()->setUserTxParams(txPara);
                txmode = txInfo.readTxMode();
                RbMap rbMap;

                pkt->addTagIfAbsent<UserControlInfo>()->setTxMode(txmode);
                pkt->addTagIfAbsent<UserControlInfo>()->setCw(cw);

                grantedBlocks = enbSchedulerDl_->readRbOccupation(destId, carrierFreq, rbMap);

                pkt->addTagIfAbsent<UserControlInfo>()->setGrantedBlocks(rbMap);
                pkt->addTagIfAbsent<UserControlInfo>()->setTotalGrantedBlocks(grantedBlocks);
                macPacket = pkt;

                auto macPkt = makeShared<LteMacPdu>();
                macPkt->setHeaderLength(MAC_HEADER);
                macPkt->addTagIfAbsent<CreationTimeTag>()->setCreationTime(NOW);
                macPacket->insertAtFront(macPkt);
                macPduList_[carrierFreq][pktId] = macPacket;
            }
            else {
                macPacket = pit->second;
            }

            while (sduPerCid > 0) {
                if ((mbuf_[destCid]->getQueueLength()) < (int)sduPerCid) {
                    throw cRuntimeError("Abnormal queue length detected while building MAC PDU for cid %d "
                                        "Queue real SDU length is %d while scheduled SDUs are %d",
                            destCid, mbuf_[destCid]->getQueueLength(), sduPerCid);
                }

                auto pkt = check_and_cast<Packet *>(mbuf_[destCid]->popFront());
                ASSERT(pkt != nullptr);

                drop(pkt);
                auto macPkt = macPacket->removeAtFront<LteMacPdu>();
                macPkt->pushSdu(pkt);
                macPacket->insertAtFront(macPkt);
                sduPerCid--;
            }
        }
    }

    std::map<double, MacPduList>::iterator lit;
    for (const auto& lit : macPduList_) {
        double carrierFreq = lit.first;
        if (harqTxBuffers_.find(carrierFreq) == harqTxBuffers_.end()) {
            HarqTxBuffers newHarqTxBuffers;
            harqTxBuffers_[carrierFreq] = newHarqTxBuffers;
        }
        HarqTxBuffers& harqTxBuffers = harqTxBuffers_[carrierFreq];

        for (const auto& pit : lit.second) {
            MacNodeId destId = pit.first.first;
            Codeword cw = pit.first.second;

            LteHarqBufferTx *txBuf;
            auto hit = harqTxBuffers.find(destId);
            if (hit != harqTxBuffers.end()) {
                txBuf = hit->second;
            }
            else {
                // FIXME: possible memory leak
                LteHarqBufferTx *hb = new LteHarqBufferTx(binder_, ENB_TX_HARQ_PROCESSES,
                        this, getMacUe(binder_, destId));
                harqTxBuffers[destId] = hb;
                txBuf = hb;
            }
            UnitList txList = (txBuf->firstAvailable());

            auto macPacket = pit.second;
            auto header = macPacket->peekAtFront<LteMacPdu>();
            EV << "LteMacBase: PDU Maker created PDU: " << macPacket->str() << endl;

            // PDU transmission here (if any)
            if (txList.second.empty()) {
                EV << "macPduMake() : no available process for this MAC PDU in TxHarqBuffer" << endl;
                delete macPacket;
            }
            else {
                if (txList.first == HARQ_NONE)
                    throw cRuntimeError("LteMacBase: PDU Maker sending to an incorrect void H-ARQ process. Aborting");
                txBuf->insertPdu(txList.first, cw, macPacket);
            }
        }
    }
    EV << "------ END LteMacEnb::macPduMake ------\n";
}

void LteMacEnb::macPduUnmake(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto macPkt = pkt->removeAtFront<LteMacPdu>();

    /*
     * @author Alessandro Noferi
     * Notify the pfm about the successful arrival of a TB from a UE.
     * From ETSI TS 138314 V16.0.0 (2020-07)
     * tSucc: the point in time when the MAC SDU i was received successfully by the network
     */
    auto userInfo = pkt->getTag<UserControlInfo>();

    if (packetFlowManager_ != nullptr) {
        packetFlowManager_->ulMacPduArrived(userInfo->getSourceId(), userInfo->getGrantId());
    }

    while (macPkt->hasSdu()) {
        // Extract and send SDU
        cPacket *upPkt = macPkt->popSdu();
        take(upPkt);

        // TODO: upPkt->info()
        EV << "LteMacBase: PDU Unmaker extracted SDU" << endl;
        sendUpperPackets(upPkt);
    }

    while (macPkt->hasCe()) {
        // Extract CE
        // TODO: see if BSR for CID or LCID
        MacBsr *bsr = check_and_cast<MacBsr *>(macPkt->popCe());
        auto lteInfo = pkt->getTag<UserControlInfo>();
        MacCid cid = idToMacCid(lteInfo->getSourceId(), 0);
        bufferizeBsr(bsr, cid);
        delete bsr;
    }
    pkt->insertAtFront(macPkt);

    ASSERT(pkt->getOwner() == this);
    delete pkt;
}

bool LteMacEnb::bufferizePacket(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);

    if (pkt->getBitLength() <= 1) { // no data in this packet
        delete pktAux;
        return false;
    }

    pkt->setTimestamp();        // Add timestamp with current time to packet

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

            // make a copy of lte control info and store it to the traffic descriptors map
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
                throw cRuntimeError("LteMacEnb::bufferizePacket - cannot find mac buffer for cid %d", cid);
        }

        delete pkt;
        return true; // this is only a new packet indication - only buffered in virtual queue
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
            // unable to buffer the packet (packet is not enqueued and will be dropped): update statistics
            EV << "LteMacBuffers : queue" << cid << " is full - cannot buffer packet " << pkt->getId() << "\n";

            totalOverflowedBytes_ += pkt->getByteLength();
            double sample = (double)totalOverflowedBytes_ / (NOW - getSimulation()->getWarmupPeriod());

            if (lteInfo->getDirection() == DL)
                emit(macBufferOverflowDlSignal_, sample);
            else
                emit(macBufferOverflowUlSignal_, sample);

            EV << "LteMacBuffers : Dropped packet: queue" << cid << " is full\n";
            // @author Alessandro Noferi
            // discard the RLC
            if (packetFlowManager_ != nullptr) {
                unsigned int rlcSno = check_and_cast<LteRlcUmDataPdu *>(pkt)->getPduSequenceNumber();
                packetFlowManager_->discardRlcPdu(lteInfo->getLcid(), rlcSno);
            }

            // TODO add delete pkt (memory leak?)
        }

        EV << "LteMacBuffers : Using old buffer on node: " <<
            MacCidToNodeId(cid) << " for Lcid: " << MacCidToLcid(cid) << ", Space left in the Queue: " <<
            queue->getQueueSize() - queue->getByteLength() << "\n";
    }

    return true;
}

void LteMacEnb::handleUpperMessage(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTag<FlowControlInfo>();
    MacCid cid = idToMacCid(lteInfo->getDestId(), lteInfo->getLcid());

    bool isLteRlcPduNewData = checkIfHeaderType<LteRlcPduNewData>(pkt);

    bool packetIsBuffered = bufferizePacket(pkt);  // will buffer (or destroy if the queue is full)

    if (!isLteRlcPduNewData && packetIsBuffered) {
        // new MAC SDU has been received (was requested by MAC, no need to notify the scheduler)
        // creates PDUs from the schedule list and puts them in HARQ buffers
        macPduMake(cid);
    }
    else if (isLteRlcPduNewData) {
        // new data - inform scheduler of the active connection
        enbSchedulerDl_->backlog(cid);
    }
}

void LteMacEnb::handleSelfMessage()
{
    /***************
    *  MAIN LOOP  *
    ***************/

    EV << "-----" << "ENB MAIN LOOP -----" << endl;

    // Reception

    // extract PDUs from all HARQ RX buffers and pass them to unmaker
    for (auto& mit : harqRxBuffers_) {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mit.first)) > 0)
            continue;

        for (auto& hit : mit.second) {
            auto pduList = hit.second->extractCorrectPdus();
            while (!pduList.empty()) {
                auto pdu = pduList.front();
                pduList.pop_front();
                macPduUnmake(pdu);
            }
        }
    }

    // UPLINK
    EV << "============================================== UPLINK ==============================================" << endl;
    // init and reset global allocation information
    if (binder_->getLastUpdateUlTransmissionInfo() < NOW)                                                            // once per TTI, even in case of multicell scenarios
        binder_->initAndResetUlTransmissionInfo();

    enbSchedulerUl_->updateHarqDescs();

    std::map<double, LteMacScheduleList> *scheduleListUl = enbSchedulerUl_->schedule();
    // send uplink grants to PHY layer
    sendGrants(scheduleListUl);
    EV << "============================================ END UPLINK ============================================" << endl;

    EV << "============================================ DOWNLINK ==============================================" << endl;
    // DOWNLINK

    // use this flag to enable/disable scheduling...don't look at me, this is very useful!!!
    bool activation = true;

    if (activation) {
        // clear previous schedule list
        if (scheduleListDl_ != nullptr) {
            for (auto& cit : *scheduleListDl_)
                cit.second.clear();
            scheduleListDl_->clear();
        }

        // perform Downlink scheduling
        scheduleListDl_ = enbSchedulerDl_->schedule();

        // requests SDUs to the RLC layer
        macSduRequest();
    }
    EV << "========================================== END DOWNLINK ============================================" << endl;

    // purge from corrupted PDUs all RX HARQ buffers for all users
    for (auto& mit : harqRxBuffers_) {
        if (getNumerologyPeriodCounter(binder_->getNumerologyIndexFromCarrierFreq(mit.first)) > 0)
            continue;

        for (auto& hit : mit.second)
            hit.second->purgeCorruptedPdus();
    }

    // Message that triggers flushing of TX HARQ buffers for all users
    // This way, flushing is performed after the (possible) reception of new MAC PDUs
    cMessage *flushHarqMsg = new cMessage("flushHarqMsg");
    flushHarqMsg->setSchedulingPriority(1);        // after other messages
    scheduleAt(NOW, flushHarqMsg);

    decreaseNumerologyPeriodCounter();

    EV << "--- END ENB MAIN LOOP ---" << endl;
}

void LteMacEnb::signalProcessForRtx(MacNodeId nodeId, double carrierFrequency, Direction dir, bool rtx)
{
    std::map<double, int> *needRtx = (dir == DL) ? &needRtxDl_ : (dir == UL) ? &needRtxUl_ :
        (dir == D2D) ? &needRtxD2D_ : throw cRuntimeError("LteMacEnb::signalProcessForRtx - direction %d not valid\n", dir);

    if (needRtx->find(carrierFrequency) == needRtx->end()) {
        if (!rtx)
            return;
        needRtx->insert({carrierFrequency, 0});
    }

    if (!rtx)
        (*needRtx)[carrierFrequency]--;
    else
        (*needRtx)[carrierFrequency]++;
}

int LteMacEnb::getProcessForRtx(double carrierFrequency, Direction dir)
{
    std::map<double, int> *needRtx = (dir == DL) ? &needRtxDl_ : (dir == UL) ? &needRtxUl_ :
        (dir == D2D) ? &needRtxD2D_ : throw cRuntimeError("LteMacEnb::getProcessForRtx - direction %d not valid\n", dir);

    if (needRtx->find(carrierFrequency) == needRtx->end())
        return 0;

    return needRtx->at(carrierFrequency);
}

void LteMacEnb::flushHarqBuffers()
{
    for (auto& mit : harqTxBuffers_) {
        for (auto& it : mit.second)
            it.second->sendSelectedDown();
    }
}

void LteMacEnb::macHandleFeedbackPkt(cPacket *pktAux)
{
    auto pkt = check_and_cast<Packet *>(pktAux);
    auto fb = pkt->peekAtFront<LteFeedbackPkt>();

    //LteFeedbackPkt* fb = check_and_cast<LteFeedbackPkt*>(pkt);
    LteFeedbackDoubleVector fbMapDl = fb->getLteFeedbackDoubleVectorDl();
    LteFeedbackDoubleVector fbMapUl = fb->getLteFeedbackDoubleVectorUl();
    //get Source Node Id<
    MacNodeId id = fb->getSourceNodeId();

    auto lteInfo = pkt->getTag<UserControlInfo>();

    for (auto& it : fbMapDl) {
        for (auto& jt : it) {
            if (!jt.isEmptyFeedback()) {
                amc_->pushFeedback(id, DL, jt, lteInfo->getCarrierFrequency());
            }
        }
    }
    for (auto& it : fbMapUl) {
        for (auto& jt : it) {
            if (!jt.isEmptyFeedback())
                amc_->pushFeedback(id, UL, jt, lteInfo->getCarrierFrequency());
        }
    }
    delete pkt;
}

void LteMacEnb::updateUserTxParam(cPacket *pktAux)
{

    auto pkt = check_and_cast<Packet *>(pktAux);
    auto lteInfo = pkt->getTagForUpdate<UserControlInfo>();

    if (lteInfo->getFrameType() != DATAPKT)
        return; // TODO check if this should be removed.

    auto dir = (Direction)lteInfo->getDirection();

    const UserTxParams& newParam = amc_->computeTxParams(lteInfo->getDestId(), dir, lteInfo->getCarrierFrequency());
    UserTxParams *tmp = new UserTxParams(newParam);

    lteInfo->setUserTxParams(tmp);
    RbMap rbMap;
    lteInfo->setTxMode(newParam.readTxMode());
    LteSchedulerEnb *scheduler = ((dir == DL) ? static_cast<LteSchedulerEnb *>(enbSchedulerDl_) : static_cast<LteSchedulerEnb *>(enbSchedulerUl_));

    int grantedBlocks = scheduler->readRbOccupation(lteInfo->getDestId(), lteInfo->getCarrierFrequency(), rbMap);

    lteInfo->setGrantedBlocks(rbMap);
    lteInfo->setTotalGrantedBlocks(grantedBlocks);
}

ActiveSet *LteMacEnb::getActiveSet(Direction dir)
{
    if (dir == DL)
        return enbSchedulerDl_->readActiveConnections();
    else
        return enbSchedulerUl_->readActiveConnections();
}

unsigned int LteMacEnb::getDlBandStatus(Band b)
{
    unsigned int i = enbSchedulerDl_->readPerBandAllocatedBlocks(MAIN_PLANE, MACRO, b);
    return i;
}

unsigned int LteMacEnb::getDlPrevBandStatus(Band b)
{
    unsigned int i = enbSchedulerDl_->getInterferingBlocks(MAIN_PLANE, MACRO, b);
    return i;
}

ConflictGraph *LteMacEnb::getConflictGraph()
{
    return nullptr;
}

double LteMacEnb::getUtilization(Direction dir)
{
    if (dir == DL) {
        return enbSchedulerDl_->getUtilization() * 100;
    }
    else if (dir == UL) {
        return enbSchedulerUl_->getUtilization() * 100;
    }
    else {
        throw cRuntimeError("LteMacEnb::getSchedDiscipline(): unrecognized direction %d", (int)dir);
    }
}

int LteMacEnb::getActiveUesNumber(Direction dir)
{
    std::set<MacNodeId> activeUeSet;

    /*
     * According to ETSI 136 314:
     * Active UEs in DL are users where there is
     * buffered data in MAC, RLC, and PDCP, plus data in HARQ
     * transmissions not yet terminated.
     */
    if (dir == DL) {
        // from macCid to NodeId
        for (auto& item : mbuf_) {
            if (item.second->getQueueLength() != 0)
                activeUeSet.insert(MacCidToNodeId(item.first)); // active users in MAC
        }

        std::map<double, HarqTxBuffers> *harqBuffers = getHarqTxBuffers();

        for (const auto& it1 : *harqBuffers) {
            const HarqTxBuffers& harqBuffer = it1.second;
            for (const auto& itHarq : harqBuffer) {
                if (itHarq.second->isHarqBufferActive()) {
                    activeUeSet.insert(itHarq.first); // active users in HARQ
                }
            }
        }

        // every time an RLC SDU enters the layer, a newPktData is sent to
        // mac to inform the presence of data in RLC.
        for (const auto& vit : macBuffers_) {
            if (!vit.second->isEmpty())
                activeUeSet.insert(MacCidToNodeId(vit.first)); // active users in RLC
        }
    }
    else if (dir == UL) {
        // extract PDUs from all harqRxBuffers and pass them to unmaker
        for (const auto& mit : harqRxBuffers_) {
            for (const auto& elem : mit.second) {
                if (elem.second->isHarqBufferActive()) {
                    activeUeSet.insert(elem.first); // active users in HARQ
                }
            }
        }

        // check the presence of UM
        LteRlcUm *rlcUm = inet::findModuleFromPar<LteRlcUm>(par("rlcUmModule"), this);
        if (rlcUm != nullptr) {
            std::set<MacNodeId> activeRlcUe;
            rlcUm->activeUeUL(&activeRlcUe);
            for (auto ue : activeRlcUe) {
                activeUeSet.insert(ue); // active users in RLC
            }
        }

        /*
         * If the PDCP layer is NRPdcpRrc and isDualConnectivityEnabled,
         * the PDCP layer can also have SDUs buffered.
         */

        NRPdcpRrcEnb *nrPdpc;
        cModule *pdcp = inet::getModuleFromPar<cModule>(par("pdcpRrcModule"), this);
        if (strcmp(pdcp->getClassName(), "NRPdcpRrcEnb") == 0) {
            nrPdpc = check_and_cast<NRPdcpRrcEnb *>(pdcp);
            std::set<MacNodeId> activePdcpUe;
            nrPdpc->activeUeUL(&activePdcpUe);
            for (auto ue: activePdcpUe) {
                activeUeSet.insert(ue); // active users in RLC
            }
        }
    }
    else {
        throw cRuntimeError("LteMacEnb::getSchedDiscipline(): unrecognized direction %d", (int)dir);
    }

    return activeUeSet.size();
}

} //namespace

