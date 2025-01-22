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

#include "stack/mac/scheduler/LteSchedulerEnbUl.h"
#include "stack/mac/LteMacEnb.h"
#include "stack/mac/LteMacEnbD2D.h"
#include "stack/mac/buffer/harq/LteHarqBufferRx.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/phy/LtePhyBase.h"

namespace simu5g {

using namespace omnetpp;

bool LteSchedulerEnbUl::checkEligibility(MacNodeId id, Codeword& cw, double carrierFrequency)
{
    HarqRxBuffers *harqRxBuff = mac_->getHarqRxBuffers(carrierFrequency);
    if (harqRxBuff == nullptr)                              // a new HARQ buffer will be created at reception
        return true;

    // check if harq buffer has already been created for this node
    if (harqRxBuff->find(id) != harqRxBuff->end()) {
        LteHarqBufferRx *ulHarq = harqRxBuff->at(id);

        // get current Harq Process for nodeId
        unsigned char currentAcid = harqStatus_[carrierFrequency].at(id);
        // get current Harq Process status
        std::vector<RxUnitStatus> status = ulHarq->getProcess(currentAcid)->getProcessStatus();
        // check if at least one codeword buffer is available for reception
        for ( ; cw < MAX_CODEWORDS; ++cw) {
            if (status.at(cw).second == RXHARQ_PDU_EMPTY) {
                return true;
            }
        }
    }
    return false;
}

void LteSchedulerEnbUl::updateHarqDescs()
{
    EV << NOW << "LteSchedulerEnbUl::updateHarqDescs  cell " << mac_->getMacCellId() << endl;

    for (const auto &[harqKey, harqBuffers] : *harqRxBuffers_) {
        for (const auto &[ueKey, ueBuffer] : harqBuffers) {
            auto currentStatus = harqStatus_[harqKey].find(ueKey);
            if (currentStatus != harqStatus_[harqKey].end()) {
                EV << NOW << "LteSchedulerEnbUl::updateHarqDescs UE " << ueKey << " OLD Current Process is  " << (unsigned int)currentStatus->second << endl;
                // updating current acid id
                currentStatus->second = (currentStatus->second + 1) % (ueBuffer->getProcesses());

                EV << NOW << "LteSchedulerEnbUl::updateHarqDescs UE " << ueKey << " NEW Current Process is " << (unsigned int)currentStatus->second << "(total harq processes " << ueBuffer->getProcesses() << ")" << endl;
            }
            else {
                EV << NOW << "LteSchedulerEnbUl::updateHarqDescs UE " << ueKey << " initialized the H-ARQ status " << endl;
                harqStatus_[harqKey][ueKey] = 0;
            }
        }
    }
}

bool LteSchedulerEnbUl::racschedule(double carrierFrequency, BandLimitVector *bandLim)
{
    EV << NOW << " LteSchedulerEnbUl::racschedule --------------------::[ START RAC-SCHEDULE ]::--------------------" << endl;
    EV << NOW << " LteSchedulerEnbUl::racschedule eNodeB: " << mac_->getMacCellId() << " Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

    // Get number of logical bands
    unsigned int numBands = mac_->getCellInfo()->getNumBands();
    unsigned int racAllocatedBlocks = 0;

    auto map_it = racStatus_.find(carrierFrequency);
    if (map_it != racStatus_.end()) {
        RacStatus& racStatus = map_it->second;
        for (const auto& [nodeId, _] : racStatus) {
            EV << NOW << " LteSchedulerEnbUl::racschedule handling RAC for node " << nodeId << endl;

            const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, UL, carrierFrequency);    // get the user info
            const std::set<Band>& allowedBands = txParams.readBands();
            BandLimitVector tempBandLim;
            std::string bands_msg = "BAND_LIMIT_SPECIFIED";
            if (bandLim == nullptr) {
                // Create a vector of band limit using all bands
                // FIXME: bandLim is never deleted

                // for each band of the band vector provided
                for (unsigned int i = 0; i < numBands; i++) {
                    BandLimit elem;
                    // copy the band
                    elem.band_ = Band(i);
                    EV << "Putting band " << i << endl;
                    for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
                        if (allowedBands.find(elem.band_) != allowedBands.end()) {
                            elem.limit_[j] = -1;
                        }
                        else {
                            elem.limit_[j] = -2;
                        }
                    }
                    tempBandLim.push_back(elem);
                }
                bandLim = &tempBandLim;
            }
            else {
                // for each band of the band vector provided
                for (unsigned int i = 0; i < numBands; i++) {
                    BandLimit& elem = bandLim->at(i);
                    for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
                        if (elem.limit_[j] == -2)
                            continue;

                        if (allowedBands.find(elem.band_) != allowedBands.end()) {
                            elem.limit_[j] = -1;
                        }
                        else {
                            elem.limit_[j] = -2;
                        }
                    }
                }
            }

            // FIXME default behavior
            // try to allocate one block to selected UE on at least one logical band of MACRO antenna, first codeword

            const unsigned int cw = 0;
            const unsigned int blocks = 1;

            bool allocation = false;

            unsigned int size = bandLim->size();
            for (Band b = 0; b < size; ++b) {
                // if the limit flag is set to skip, jump off
                int limit = bandLim->at(b).limit_.at(cw);
                if (limit == -2) {
                    EV << "LteSchedulerEnbUl::racschedule - skipping logical band according to limit value" << endl;
                    continue;
                }

                if (allocator_->availableBlocks(nodeId, MACRO, b) > 0) {
                    unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, blocks, UL, carrierFrequency);
                    if (bytes > 0) {

                        allocator_->addBlocks(MACRO, b, nodeId, 1, bytes);
                        racAllocatedBlocks++;

                        EV << NOW << "LteSchedulerEnbUl::racschedule UE: " << nodeId << "Handled RAC on band: " << b << endl;

                        allocation = true;
                        break;
                    }
                }
            }

            if (allocation) {
                // create scList id for current cid/codeword
                MacCid cid = idToMacCid(nodeId, SHORT_BSR);  // build the cid. Since this grant will be used for a BSR,
                                                             // we use the LCID corresponding to the SHORT_BSR
                std::pair<unsigned int, Codeword> scListId = {cid, cw};
                scheduleList_[carrierFrequency][scListId] = blocks;
            }
        }

        // clean up all requests
        racStatus.clear();
    }

    if (racAllocatedBlocks < numBands) {
        // serve RAC for background UEs
        racscheduleBackground(racAllocatedBlocks, carrierFrequency, bandLim);
    }

    // update available blocks
    unsigned int availableBlocks = numBands - racAllocatedBlocks;

    EV << NOW << " LteSchedulerEnbUl::racschedule racAllocatedBlocks: " << racAllocatedBlocks << " availableBlocks after rac schedule: " << availableBlocks << endl;
    EV << NOW << " LteSchedulerEnbUl::racschedule --------------------::[  END RAC-SCHEDULE  ]::--------------------" << endl;

    return availableBlocks == 0;
}

void LteSchedulerEnbUl::racscheduleBackground(unsigned int& racAllocatedBlocks, double carrierFrequency, BandLimitVector *bandLim)
{
    EV << NOW << " LteSchedulerEnbUl::racscheduleBackground - scheduling RAC for background UEs" << endl;

    std::list<MacNodeId> servedRac;

    IBackgroundTrafficManager *bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);

    // Get number of logical bands
    unsigned int numBands = mac_->getCellInfo()->getNumBands();

    for (auto it = bgTrafficManager->getWaitingForRacUesBegin(), et = bgTrafficManager->getWaitingForRacUesEnd(); it != et; ++it) {
        // get current nodeId
        MacNodeId bgUeId = BGUE_MIN_ID + *it;

        EV << NOW << " LteSchedulerEnbUl::racscheduleBackground handling RAC for node " << bgUeId << endl;

        BandLimitVector tempBandLim;
        std::string bands_msg = "BAND_LIMIT_SPECIFIED";
        if (bandLim == nullptr) {
            // Create a vector of band limit using all bands

            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++) {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
                    elem.limit_[j] = -1;
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }

        // FIXME default behavior
        // try to allocate one block to selected UE on at least one logical band of MACRO antenna, first codeword

        const unsigned int cw = 0;
        const unsigned int blocks = 1;

        unsigned int size = bandLim->size();
        for (Band b = 0; b < size; ++b) {
            // if the limit flag is set to skip, jump off
            int limit = bandLim->at(b).limit_.at(cw);
            if (limit == -2) {
                EV << "LteSchedulerEnbUl::racscheduleBackground - skipping logical band according to limit value" << endl;
                continue;
            }

            if (allocator_->availableBlocks(bgUeId, MACRO, b) > 0) {
                unsigned int bytes = blocks * (bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, UL));
                if (bytes > 0) {
                    allocator_->addBlocks(MACRO, b, bgUeId, 1, bytes);
                    racAllocatedBlocks++;

                    servedRac.push_back(bgUeId);

                    EV << NOW << "LteSchedulerEnbUl::racscheduleBackground UE: " << bgUeId << "Handled RAC on band: " << b << endl;

                    break;
                }
            }
        }
    }

    while (!servedRac.empty()) {
        // notify the traffic manager that the RAC for this UE has been served
        bgTrafficManager->racHandled(servedRac.front());
        servedRac.pop_front();
    }
}

bool LteSchedulerEnbUl::rtxschedule(double carrierFrequency, BandLimitVector *bandLim)
{
    try {
        EV << NOW << " LteSchedulerEnbUl::rtxschedule --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
        EV << NOW << " LteSchedulerEnbUl::rtxschedule eNodeB: " << mac_->getMacCellId() << " Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

        auto freqIt = harqRxBuffers_->find(carrierFrequency);
        if (freqIt != harqRxBuffers_->end()) {
            auto& rxBufferForCarrierFrequency = freqIt->second;
            for (auto it = rxBufferForCarrierFrequency.begin(); it != rxBufferForCarrierFrequency.end(); ) {
                // get current nodeId
                MacNodeId nodeId = it->first;

                if (nodeId == NODEID_NONE) {
                    // UE has left the simulation - erase queue and continue
                    it = rxBufferForCarrierFrequency.erase(it);
                    continue;
                }
                OmnetId id = binder_->getOmnetId(nodeId);
                if (id == 0) {
                    it = rxBufferForCarrierFrequency.erase(it);
                    continue;
                }

                // get current Harq Process for nodeId
                unsigned char currentAcid = harqStatus_[carrierFrequency].at(nodeId);

                // check whether the UE has a H-ARQ process waiting for retransmission. If not, skip UE.
                bool skip = true;
                unsigned char acid = (currentAcid + 2) % (it->second->getProcesses());
                LteHarqProcessRx *currentProcess = it->second->getProcess(acid);
                std::vector<RxUnitStatus> procStatus = currentProcess->getProcessStatus();
                for (const auto& status : procStatus) {
                    if (status.second == RXHARQ_PDU_CORRUPTED) {
                        skip = false;
                        break;
                    }
                }
                if (skip) {
                    ++it;
                    continue;
                }

                // Get user transmission parameters
                const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency);// get the user info

                unsigned int codewords = txParams.getLayers().size();// get the number of available codewords
                unsigned int allocatedBytes = 0;

                // TODO handle the codewords join case (sizeof(cw0+cw1) < currentTbs && currentLayers ==1)

                for (Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords > 0); ++cw) {
                    // FIXME PERFORMANCE: check for rtx status before calling rtxAcid
                    // perform a retransmission on available codewords for the selected acid
                    unsigned int rtxBytes = LteSchedulerEnbUl::schedulePerAcidRtx(nodeId, carrierFrequency, cw, acid, bandLim);
                    if (rtxBytes > 0) {
                        --codewords;
                        allocatedBytes += rtxBytes;

                        mac_->signalProcessForRtx(nodeId, carrierFrequency, UL, false);
                    }
                }
                EV << NOW << "LteSchedulerEnbUl::rtxschedule UE " << nodeId << " allocated bytes : " << allocatedBytes << endl;
                ++it;
            }
        }
        if (mac_->isD2DCapable()) {
            Direction dir = D2D;
            HarqBuffersMirrorD2D *harqBuffersMirrorD2D = check_and_cast<LteMacEnbD2D *>(mac_.get())->getHarqBuffersMirrorD2D(carrierFrequency);
            if (harqBuffersMirrorD2D != nullptr) {
                for (auto it_d2d = harqBuffersMirrorD2D->begin(); it_d2d != harqBuffersMirrorD2D->end(); ) {
                    MacNodeId senderId = (it_d2d->first).first; // Transmitter
                    MacNodeId destId = (it_d2d->first).second;  // Receiver

                    if (senderId == NODEID_NONE || binder_->getOmnetId(senderId) == 0) {
                        // UE has left the simulation - erase queue and continue
                        it_d2d = harqBuffersMirrorD2D->erase(it_d2d);
                        continue;
                    }
                    if (destId == NODEID_NONE || binder_->getOmnetId(destId) == 0) {
                        // UE has left the simulation - erase queue and continue
                        it_d2d = harqBuffersMirrorD2D->erase(it_d2d);
                        continue;
                    }

                    // get current Harq Process for nodeId
                    unsigned char currentAcid = harqStatus_[carrierFrequency].at(senderId);

                    // check whether the UE has a H-ARQ process waiting for retransmission. If not, skip UE.
                    bool skip = true;
                    unsigned char acid = (currentAcid + 2) % (it_d2d->second->getProcesses());
                    LteHarqProcessMirrorD2D *currentProcess = it_d2d->second->getProcess(acid);
                    std::vector<TxHarqPduStatus> procStatus = currentProcess->getProcessStatus();
                    for (const auto& status : procStatus) {
                        if (status == TXHARQ_PDU_BUFFERED) {
                            skip = false;
                            break;
                        }
                    }
                    if (skip) {
                        ++it_d2d;
                        continue;
                    }

                    EV << NOW << " LteSchedulerEnbUl::rtxschedule - D2D UE: " << senderId << " Acid: " << (unsigned int)currentAcid << endl;

                    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(senderId, dir, carrierFrequency);
                    unsigned int codewords = txParams.getLayers().size();
                    unsigned int allocatedBytes = 0;

                    for (Codeword cw = 0; (cw < MAX_CODEWORDS) && (codewords > 0); ++cw) {
                        unsigned int rtxBytes = LteSchedulerEnbUl::schedulePerAcidRtxD2D(destId, senderId, carrierFrequency, cw, acid, bandLim);
                        if (rtxBytes > 0) {
                            --codewords;
                            allocatedBytes += rtxBytes;

                            mac_->signalProcessForRtx(senderId, carrierFrequency, D2D, false);
                        }
                    }
                    EV << NOW << " LteSchedulerEnbUl::rtxschedule - D2D UE: " << senderId << " allocated bytes : " << allocatedBytes << endl;
                    ++it_d2d;
                }
            }
        }

        int availableBlocks = allocator_->computeTotalRbs();

        EV << NOW << " LteSchedulerEnbUl::rtxschedule residual OFDM Space: " << availableBlocks << endl;

        EV << NOW << " LteSchedulerEnbUl::rtxschedule --------------------::[  END RTX-SCHEDULE  ]::--------------------" << endl;

        return availableBlocks == 0;
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::rtxschedule(): %s", e.what());
    }
    return false;
}

bool LteSchedulerEnbUl::rtxscheduleBackground(double carrierFrequency, BandLimitVector *bandLim)
{
    try {
        EV << NOW << " LteSchedulerEnbUl::rtxscheduleBackground --------------------::[ START RTX-SCHEDULE-BACKGROUND ]::--------------------" << endl;
        EV << NOW << " LteSchedulerEnbUl::rtxscheduleBackground eNodeB: " << mac_->getMacCellId() << " Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

        // --- Schedule RTX for background UEs --- //
        std::map<MacNodeId, unsigned int> bgScheduledRtx;
        IBackgroundTrafficManager *bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);
        auto it = bgTrafficManager->getBackloggedUesBegin(direction_, true),
                                       et = bgTrafficManager->getBackloggedUesEnd(direction_, true);
        for ( ; it != et; ++it) {
            int bgUeIndex = *it;
            MacNodeId bgUeId = BGUE_MIN_ID + bgUeIndex;

            unsigned cw = 0;
            unsigned int rtxBytes = scheduleBgRtx(bgUeId, carrierFrequency, cw, bandLim);
            if (rtxBytes > 0)
                bgScheduledRtx[bgUeId] = rtxBytes;
            EV << NOW << "LteSchedulerEnbUl::rtxscheduleBackground BG UE " << bgUeId << " - allocated bytes : " << rtxBytes << endl;
        }

        // consume bytes
        for (auto & it : bgScheduledRtx)
            bgTrafficManager->consumeBackloggedUeBytes(it.first, it.second, direction_, true); // in bytes

        int availableBlocks = allocator_->computeTotalRbs();

        EV << NOW << " LteSchedulerEnbUl::rtxscheduleBackground residual OFDM Space: " << availableBlocks << endl;

        EV << NOW << " LteSchedulerEnbUl::rtxscheduleBackground --------------------::[  END RTX-SCHEDULE-BACKGROUND ]::--------------------" << endl;

        return availableBlocks == 0;
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::rtxscheduleBackground(): %s", e.what());
    }
    return false;
}

unsigned int LteSchedulerEnbUl::schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
        std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl)
{
    try {
        const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency);    // get the user info
        const std::set<Band>& allowedBands = txParams.readBands();
        BandLimitVector tempBandLim;
        std::string bands_msg = "BAND_LIMIT_SPECIFIED";
        if (bandLim == nullptr) {
            // Create a vector of band limits using all bands
            // FIXME: bandLim is never deleted

            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++) {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
                    if (allowedBands.find(elem.band_) != allowedBands.end()) {
                        elem.limit_[j] = -1;
                    }
                    else {
                        elem.limit_[j] = -2;
                    }
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }
        else {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++) {
                BandLimit& elem = bandLim->at(i);
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
                    if (elem.limit_[j] == -2)
                        continue;

                    if (allowedBands.find(elem.band_) != allowedBands.end()) {
                        elem.limit_[j] = -1;
                    }
                    else {
                        elem.limit_[j] = -2;
                    }
                }
            }
        }

        EV << NOW << "LteSchedulerEnbUl::rtxAcid - Node[" << mac_->getMacNodeId() << ", User[" << nodeId << ", Codeword[ " << cw << "], ACID[" << (unsigned int)acid << "] " << endl;

        LteHarqProcessRx *currentProcess = harqRxBuffers_->at(carrierFrequency).at(nodeId)->getProcess(acid);

        if (currentProcess->getUnitStatus(cw) != RXHARQ_PDU_CORRUPTED) {
            // exit if the current active HARQ process is not ready for retransmission
            EV << NOW << " LteSchedulerEnbUl::rtxAcid User is on ACID " << (unsigned int)acid << " HARQ process is IDLE. No RTX scheduled ." << endl;
            return 0;
        }

        Codeword allocatedCw = 0;
        // search for already allocated codeword
        // create "mirror" scList ID for other codeword than current
        std::pair<unsigned int, Codeword> scListMirrorId = std::pair<unsigned int, Codeword>(idToMacCid(nodeId, SHORT_BSR), MAX_CODEWORDS - cw - 1);
        if (scheduleList_.find(carrierFrequency) != scheduleList_.end()) {
            if (scheduleList_[carrierFrequency].find(scListMirrorId) != scheduleList_[carrierFrequency].end()) {
                allocatedCw = MAX_CODEWORDS - cw - 1;
            }
        }
        // get current process buffered PDU byte length
        unsigned int bytes = currentProcess->getByteLength(cw);
        // bytes to serve
        unsigned int toServe = bytes;
        // blocks to allocate for each band
        std::vector<unsigned int> assignedBlocks;
        // bytes which blocks from the preceding vector are supposed to satisfy
        std::vector<unsigned int> assignedBytes;

        // end loop signal [same as bytes>0, but more secure]
        bool finish = false;
        // for each band
        unsigned int size = bandLim->size();
        for (unsigned int i = 0; (i < size) && (!finish); ++i) {
            // save the band and the relative limit
            Band b = bandLim->at(i).band_;
            int limit = bandLim->at(i).limit_.at(cw);

            // TODO add support for multi CW
//                    ((allocatedCw == MAX_CODEWORDS) ? availableBytes(nodeId,antenna, b, cw) : mac_->getAmc()->blocks2bytes(nodeId, b, cw, allocator_->getBlocks(antenna,b,nodeId) , direction_));    // available space
            unsigned int bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, direction_, carrierFrequency);

            // use the provided limit as cap for available bytes, if it is not set to unlimited
            if (limit >= 0)
                bandAvailableBytes = limit < (int)bandAvailableBytes ? limit : bandAvailableBytes;

            EV << NOW << " LteSchedulerEnbUl::rtxAcid BAND " << b << endl;
            EV << NOW << " LteSchedulerEnbUl::rtxAcid total bytes:" << bytes << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " LteSchedulerEnbUl::rtxAcid Available: " << bandAvailableBytes << " bytes" << endl;

            unsigned int servedBytes = 0;
            // there's no room on current band for serving the entire request
            if (bandAvailableBytes < toServe) {
                // record the amount of served bytes
                servedBytes = bandAvailableBytes;
                // the request can be fully satisfied
            }
            else {
                // record the amount of served bytes
                servedBytes = toServe;
                // signal end loop - all data have been serviced
                finish = true;
            }
            unsigned int servedBlocks = (servedBytes == 0) ? 0 : 1;
            // update the bytes counter
            toServe -= servedBytes;
            // update the structures
            assignedBlocks.push_back(servedBlocks);
            assignedBytes.push_back(servedBytes);
        }

        if (toServe > 0) {
            // process couldn't be served - no sufficient space on available bands
            EV << NOW << " LteSchedulerEnbUl::rtxAcid Unavailable space for serving node " << nodeId << " ,HARQ Process " << (unsigned int)acid << " on codeword " << cw << endl;
            return 0;
        }
        else {
            // record the allocation
            unsigned int size = assignedBlocks.size();
            unsigned int cwAllocatedBlocks = 0;

            // create scList id for current node/codeword
            std::pair<unsigned int, Codeword> scListId = std::pair<unsigned int, Codeword>(idToMacCid(nodeId, SHORT_BSR), cw);

            for (unsigned int i = 0; i < size; ++i) {
                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                cwAllocatedBlocks += assignedBlocks.at(i);
                EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
                //! handle multi-codeword allocation
                if (allocatedCw != MAX_CODEWORDS) {
                    EV << NOW << " LteSchedulerEnbUl::rtxAcid - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna, b, nodeId, assignedBlocks.at(i), assignedBytes.at(i));
                }
                //! TODO check if ok bandLim->at.limit_.at(cw) = assignedBytes.at(i);
            }

            // signal a retransmission
            // schedule list contains number of granted blocks

            scheduleList_[carrierFrequency][scListId] = cwAllocatedBlocks;
            // mark codeword as used
            if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
                allocatedCws_.at(nodeId)++;
            }
            else {
                allocatedCws_[nodeId] = 1;
            }

            EV << NOW << " LteSchedulerEnbUl::rtxAcid HARQ Process " << (unsigned int)acid << " : " << bytes << " bytes served! " << endl;

            return bytes;
        }
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::rtxAcid(): %s", e.what());
    }
    return 0;
}

unsigned int LteSchedulerEnbUl::schedulePerAcidRtxD2D(MacNodeId destId, MacNodeId senderId, double carrierFrequency, Codeword cw, unsigned char acid,
        std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl)
{
    Direction dir = D2D;
    try {
        const UserTxParams& txParams = mac_->getAmc()->computeTxParams(senderId, dir, carrierFrequency);    // get the user info
        const std::set<Band>& allowedBands = txParams.readBands();
        BandLimitVector tempBandLim;
        std::string bands_msg = "BAND_LIMIT_SPECIFIED";
        if (bandLim == nullptr) {
            // Create a vector of band limit using all bands
            // FIXME: bandLim is never deleted

            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++) {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
                    if (allowedBands.find(elem.band_) != allowedBands.end()) {
                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j] = -1;
                    }
                    else {
                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j] = -2;
                    }
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }
        else {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++) {
                BandLimit& elem = bandLim->at(i);
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
                    if (elem.limit_[j] == -2)
                        continue;

                    if (allowedBands.find(elem.band_) != allowedBands.end()) {
                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j] = -1;
                    }
                    else {
                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j] = -2;
                    }
                }
            }
        }

        EV << NOW << "LteSchedulerEnbUl::schedulePerAcidRtxD2D - Node[" << mac_->getMacNodeId() << ", User[" << senderId << ", Codeword[ " << cw << "], ACID[" << (unsigned int)acid << "] " << endl;

        D2DPair pair(senderId, destId);

        // Get the current active HARQ process
        HarqBuffersMirrorD2D *harqBuffersMirrorD2D = check_and_cast<LteMacEnbD2D *>(mac_.get())->getHarqBuffersMirrorD2D(carrierFrequency);
        EV << "\t the acid that should be considered is " << (unsigned int)acid << endl;

        LteHarqProcessMirrorD2D *currentProcess = harqBuffersMirrorD2D->at(pair)->getProcess(acid);
        if (currentProcess->getUnitStatus(cw) != TXHARQ_PDU_BUFFERED) {
            // exit if the current active HARQ process is not ready for retransmission
            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D User is on ACID " << (unsigned int)acid << " HARQ process is IDLE. No RTX scheduled ." << endl;
            return 0;
        }

        Codeword allocatedCw = 0;
        //search for already allocated codeword
        //create "mirror" scList ID for other codeword than current
        std::pair<unsigned int, Codeword> scListMirrorId = {idToMacCid(senderId, D2D_SHORT_BSR), MAX_CODEWORDS - cw - 1};
        if (scheduleList_.find(carrierFrequency) != scheduleList_.end()) {
            if (scheduleList_[carrierFrequency].find(scListMirrorId) != scheduleList_[carrierFrequency].end()) {
                allocatedCw = MAX_CODEWORDS - cw - 1;
            }
        }
        // get current process buffered PDU byte length
        unsigned int bytes = currentProcess->getPduLength(cw);
        // bytes to serve
        unsigned int toServe = bytes;
        // blocks to allocate for each band
        std::vector<unsigned int> assignedBlocks;
        // bytes which blocks from the preceding vector are supposed to satisfy
        std::vector<unsigned int> assignedBytes;

        // end loop signal [same as bytes>0, but more secure]
        bool finish = false;
        // for each band
        unsigned int size = bandLim->size();
        for (unsigned int i = 0; (i < size) && (!finish); ++i) {
            // save the band and the relative limit
            Band b = bandLim->at(i).band_;
            int limit = bandLim->at(i).limit_.at(cw);

            // TODO add support for multi CW
            //unsigned int bandAvailableBytes = // if a codeword has been already scheduled for retransmission, limit available blocks to what's been allocated on that codeword
            //((allocatedCw == MAX_CODEWORDS) ? availableBytes(nodeId,antenna, b, cw) : mac_->getAmc()->blocks2bytes(nodeId, b, cw, allocator_->getBlocks(antenna,b,nodeId) , direction_));    // available space
            unsigned int bandAvailableBytes = availableBytes(senderId, antenna, b, cw, dir, carrierFrequency);

            // use the provided limit as cap for available bytes, if it is not set to unlimited
            if (limit >= 0)
                bandAvailableBytes = limit < (int)bandAvailableBytes ? limit : bandAvailableBytes;

            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D BAND " << b << endl;
            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D total bytes:" << bytes << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D Available: " << bandAvailableBytes << " bytes" << endl;

            unsigned int servedBytes = 0;
            // there's no room on current band for serving the entire request
            if (bandAvailableBytes < toServe) {
                // record the amount of served bytes
                servedBytes = bandAvailableBytes;
                // the request can be fully satisfied
            }
            else {
                // record the amount of served bytes
                servedBytes = toServe;
                // signal end loop - all data have been serviced
                finish = true;
                EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D ALL DATA HAVE BEEN SERVICED" << endl;
            }
            unsigned int servedBlocks = (servedBytes == 0) ? 0 : 1;
            // update the bytes counter
            toServe -= servedBytes;
            // update the structures
            assignedBlocks.push_back(servedBlocks);
            assignedBytes.push_back(servedBytes);
        }

        if (toServe > 0) {
            // process couldn't be served - no sufficient space on available bands
            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D Unavailable space for serving node " << senderId << " ,HARQ Process " << (unsigned int)acid << " on codeword " << cw << endl;
            return 0;
        }
        else {
            // record the allocation
            unsigned int size = assignedBlocks.size();
            unsigned int cwAllocatedBlocks = 0;

            // create scList id for current cid/codeword
            std::pair<unsigned int, Codeword> scListId = {idToMacCid(senderId, D2D_SHORT_BSR), cw};

            for (unsigned int i = 0; i < size; ++i) {
                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                cwAllocatedBlocks += assignedBlocks.at(i);
                EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
                //! handle multi-codeword allocation
                if (allocatedCw != MAX_CODEWORDS) {
                    EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna, b, senderId, assignedBlocks.at(i), assignedBytes.at(i));
                }
                //! TODO check if ok bandLim->at.limit_.at(cw) = assignedBytes.at(i);
            }

            // signal a retransmission
            // schedule list contains number of granted blocks
            scheduleList_[carrierFrequency][scListId] = cwAllocatedBlocks;
            // mark codeword as used
            if (allocatedCws_.find(senderId) != allocatedCws_.end()) {
                allocatedCws_.at(senderId)++;
            }
            else {
                allocatedCws_[senderId] = 1;
            }

            EV << NOW << " LteSchedulerEnbUl::schedulePerAcidRtxD2D HARQ Process " << (unsigned int)acid << " : " << bytes << " bytes served! " << endl;

            currentProcess->markSelected(cw);

            return bytes;
        }
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::schedulePerAcidRtxD2D(): %s", e.what());
    }
    return 0;
}

unsigned int LteSchedulerEnbUl::scheduleBgRtx(MacNodeId bgUeId, double carrierFrequency, Codeword cw, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl)
{
    try {
        IBackgroundTrafficManager *bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);
        unsigned int bytesPerBlock = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_);

        // get the RTX buffer size
        unsigned int queueLength = bgTrafficManager->getBackloggedUeBuffer(bgUeId, direction_, true); // in bytes
        if (queueLength == 0)
            return 0;

        RbMap allocatedRbMap;

        BandLimitVector tempBandLim;
        if (bandLim == nullptr) {
            // Create a vector of band limit using all bands
            // FIXME: bandLim is never deleted

            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++) {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
                    elem.limit_[j] = -2;
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }

        EV << NOW << "LteSchedulerEnbUl::scheduleBgRtx - Node[" << mac_->getMacNodeId() << ", User[" << bgUeId << "]" << endl;

        Codeword allocatedCw = 0;

        // bytes to serve
        unsigned int toServe = queueLength;
        // blocks to allocate for each band
        std::vector<unsigned int> assignedBlocks;
        // bytes which blocks from the preceding vector are supposed to satisfy
        std::vector<unsigned int> assignedBytes;

        // end loop signal [same as bytes>0, but more secure]
        bool finish = false;
        // for each band
        unsigned int size = bandLim->size();
        for (unsigned int i = 0; (i < size) && (!finish); ++i) {
            // save the band and the relative limit
            Band b = bandLim->at(i).band_;
            int limit = bandLim->at(i).limit_.at(cw);

            unsigned int bandAvailableBytes = availableBytesBackgroundUe(bgUeId, antenna, b, direction_, carrierFrequency, (limitBl) ? limit : -1); // available space (in bytes)

            // use the provided limit as cap for available bytes, if it is not set to unlimited
            if (limit >= 0)
                bandAvailableBytes = limit < (int)bandAvailableBytes ? limit : bandAvailableBytes;

            EV << NOW << " LteSchedulerEnbUl::scheduleBgRtx BAND " << b << endl;
            EV << NOW << " LteSchedulerEnbUl::scheduleBgRtx total bytes:" << queueLength << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " LteSchedulerEnbUl::scheduleBgRtx Available: " << bandAvailableBytes << " bytes" << endl;

            unsigned int servedBytes = 0;
            // there's no room on current band for serving the entire request
            if (bandAvailableBytes < toServe) {
                // record the amount of served bytes
                servedBytes = bandAvailableBytes;
                // the request can be fully satisfied
            }
            else {
                // record the amount of served bytes
                servedBytes = toServe;
                // signal end loop - all data have been serviced
                finish = true;
            }

            unsigned int servedBlocks = ceil((double)servedBytes / bytesPerBlock);

            // update the bytes counter
            toServe -= servedBytes;
            // update the structures
            assignedBlocks.push_back(servedBlocks);
            assignedBytes.push_back(servedBytes);
        }

        if (toServe > 0) {
            // process couldn't be served - no sufficient space on available bands
            EV << NOW << " LteSchedulerEnbUl::scheduleBgRtx Unavailable space for serving node " << bgUeId << endl;
            return 0;
        }
        else {
            std::map<Band, unsigned int> allocatedRbMapEntry;

            // record the allocation
            unsigned int size = assignedBlocks.size();
            unsigned int allocatedBytes = 0;
            for (unsigned int i = 0; i < size; ++i) {
                allocatedRbMapEntry[i] = 0;

                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                allocatedBytes += assignedBytes.at(i);
                allocatedRbMapEntry[i] += assignedBlocks.at(i);

                EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
                //! handle multi-codeword allocation
                if (allocatedCw != MAX_CODEWORDS) {
                    EV << NOW << " LteSchedulerEnbUl::scheduleBgRtx - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna, b, bgUeId, assignedBlocks.at(i), assignedBytes.at(i));
                }
            }

            // signal a retransmission

            // mark codeword as used
            if (allocatedCws_.find(bgUeId) != allocatedCws_.end())
                allocatedCws_.at(bgUeId)++;
            else
                allocatedCws_[bgUeId] = 1;

            EV << NOW << " LteSchedulerEnbUl::scheduleBgRtx: " << allocatedBytes << " bytes served! " << endl;

            // update rb map
            allocatedRbMap[antenna] = allocatedRbMapEntry;

            // if uplink interference is enabled, mark the occupation in the ul transmission map (for ul interference computation purposes)
            LteChannelModel *channelModel = mac_->getPhy()->getChannelModel(carrierFrequency);
            if (channelModel->isUplinkInterferenceEnabled())
                binder_->storeUlTransmissionMap(carrierFrequency, antenna, allocatedRbMap, bgUeId, mac_->getMacCellId(), bgTrafficManager->getTrafficGenerator(bgUeId), UL);

            return allocatedBytes;
        }
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::scheduleBgRtx(): %s", e.what());
    }
    return 0;
}

void LteSchedulerEnbUl::removePendingRac(MacNodeId nodeId)
{
    for (auto& item : racStatus_) {
        RacStatus::iterator elem_it = item.second.find(nodeId);
        if (elem_it != item.second.end())
            item.second.erase(nodeId);
    }
}

} //namespace

