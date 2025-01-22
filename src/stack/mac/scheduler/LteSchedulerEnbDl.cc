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

#include "stack/mac/scheduler/LteSchedulerEnbDl.h"
#include "stack/mac/LteMacEnb.h"
#include "stack/mac/scheduler/LteScheduler.h"
#include "stack/mac/allocator/LteAllocationModule.h"
#include "stack/mac/buffer/LteMacBuffer.h"

namespace simu5g {

using namespace omnetpp;

bool LteSchedulerEnbDl::checkEligibility(MacNodeId id, Codeword& cw, double carrierFrequency)
{
    HarqTxBuffers *harqTxBuff = mac_->getHarqTxBuffers(carrierFrequency);
    if (harqTxBuff == nullptr)                              // a new HARQ buffer will be created at transmission
        return true;

    // check if HARQ buffer has already been created for this node
    if (harqTxBuff->find(id) != harqTxBuff->end()) {
        LteHarqBufferTx *dlHarq = harqTxBuff->at(id);
        UnitList freeUnits = dlHarq->firstAvailable();

        if (freeUnits.first != HARQ_NONE) {
            if (freeUnits.second.empty())
                // there is a process currently selected for user <id>, but all of its codewords have already been used.
                return false;
            // retrieving the codeword index
            cw = freeUnits.second.front();
            // DEBUG check
            if (cw > MAX_CODEWORDS)
                throw cRuntimeError("LteSchedulerEnbDl::checkEligibility(): abnormal codeword id %d", (int)cw);
            return true;
        }
    }
    return true;
}

unsigned int LteSchedulerEnbDl::schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
        std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl)
{
    // Get user transmission parameters
    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency);    // get the user info
    const std::set<Band>& allowedBands = txParams.readBands();
    // TODO SK Get the number of codewords - FIX with correct mapping
    unsigned int codewords = txParams.getLayers().size();                // get the number of available codewords

    std::vector<BandLimit> tempBandLim;

    Codeword remappedCw = (codewords == 1) ? 0 : cw;

    if (bandLim == nullptr) {
        // Create a vector of band limits using all bands
        bandLim = &tempBandLim;
        bandLim->clear();

        unsigned int numBands = mac_->getCellInfo()->getNumBands();
        // for each band of the band vector provided
        for (unsigned int i = 0; i < numBands; i++) {
            BandLimit elem;
            // copy the band
            elem.band_ = Band(i);
            EV << "Putting band " << i << endl;
            for (unsigned int j = 0; j < codewords; j++) {
                if (allowedBands.find(elem.band_) != allowedBands.end()) {
                    elem.limit_[j] = -1;
                }
                else {
                    elem.limit_[j] = -2;
                }
            }
            bandLim->push_back(elem);
        }
    }
    else {
        unsigned int numBands = mac_->getCellInfo()->getNumBands();
        // for each band of the band vector provided
        for (unsigned int i = 0; i < numBands; i++) {
            BandLimit& elem = bandLim->at(i);
            for (unsigned int j = 0; j < codewords; j++) {
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

    EV << NOW << "LteSchedulerEnbDl::rtxAcid - Node [" << mac_->getMacNodeId() << "], User[" << nodeId << "],  Codeword [" << cw << "]  of [" << codewords << "] , ACID [" << (int)acid << "] " << endl;
    //! \test REALISTIC!!!  Multi User MIMO support
    if (mac_->muMimo() && (txParams.readTxMode() == MULTI_USER)) {
        // request amc for MU_MIMO pairing
        MacNodeId peer = mac_->getAmc()->computeMuMimoPairing(nodeId);
        if (peer != nodeId) {
            // this user has a valid pairing
            //1) register pairing  - if pairing is already registered false is returned
            if (allocator_->configureMuMimoPeering(nodeId, peer)) {
                EV << "LteSchedulerEnb::grant MU-MIMO pairing established: main user [" << nodeId << "], paired user [" << peer << "]" << endl;
            }
            else {
                EV << "LteSchedulerEnb::grant MU-MIMO pairing already exists between users [" << nodeId << "] and [" << peer << "]" << endl;
            }
        }
        else {
            EV << "LteSchedulerEnb::grant no MU-MIMO pairing available for user [" << nodeId << "]" << endl;
        }
    }
    //!\test experimental DAS support
    // registering DAS spaces to the allocator
    Plane plane = allocator_->getOFDMPlane(nodeId);
    allocator_->setRemoteAntenna(plane, antenna);

    // blocks to allocate for each band
    std::vector<unsigned int> assignedBlocks;
    // bytes which blocks from the preceding vector are supposed to satisfy
    std::vector<unsigned int> assignedBytes;

    HarqTxBuffers *harqTxBuff = mac_->getHarqTxBuffers(carrierFrequency);
    if (harqTxBuff == nullptr)
        throw cRuntimeError("LteSchedulerEnbDl::schedulePerAcidRtx - HARQ Buffer not found for carrier %f", carrierFrequency);
    LteHarqBufferTx *currHarq = harqTxBuff->at(nodeId);

    // bytes to serve
    unsigned int bytes = currHarq->pduLength(acid, cw);

    // check selected process status.
    std::vector<UnitStatus> pStatus = currHarq->getProcess(acid)->getProcessStatus();

    Codeword allocatedCw = 0;
    // search for already allocated codeword
    if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
        allocatedCw = allocatedCws_.at(nodeId);
    }
    // for each band
    unsigned int size = bandLim->size();

    for (unsigned int i = 0; i < size; ++i) {
        // save the band and the relative limit
        Band b = bandLim->at(i).band_;
        int limit = bandLim->at(i).limit_.at(remappedCw);

        EV << "LteSchedulerEnbDl::schedulePerAcidRtx --- BAND " << b << " LIMIT " << limit << "---" << endl;
        // if the limit flag is set to skip, jump off
        if (limit == -2) {
            EV << "LteSchedulerEnbDl::schedulePerAcidRtx - skipping logical band according to limit value" << endl;
            continue;
        }

        unsigned int available = 0;
        // if a codeword has been already scheduled for retransmission, limit available blocks to what's been allocated on that codeword
        if ((allocatedCw != 0)) {
            // get band allocated blocks
            int b1 = allocator_->getBlocks(antenna, b, nodeId);

            // limit eventually allocated blocks on other codeword to limit for current cw
            //b1 = (limitBl ? (b1>limit?limit:b1) : b1);
            available = (b1 == 0) ? 0 : mac_->getAmc()->computeBytesOnNRbs(nodeId, b, remappedCw, b1, direction_, carrierFrequency);
        }
        else
            available = availableBytes(nodeId, antenna, b, remappedCw, direction_, carrierFrequency, (limitBl) ? limit : -1);                                                                                                                                   // available space

        // use the provided limit as cap for available bytes, if it is not set to unlimited
        if (limit >= 0 && !limitBl)
            available = limit < (int)available ? limit : available;

        EV << NOW << "LteSchedulerEnbDl::rtxAcid ----- BAND " << b << "-----" << endl;
        EV << NOW << "LteSchedulerEnbDl::rtxAcid To serve: " << bytes << " bytes" << endl;
        EV << NOW << "LteSchedulerEnbDl::rtxAcid Available: " << available << " bytes" << endl;

        unsigned int allocation = 0;
        if (available < bytes) {
            allocation = available;
            bytes -= available;
        }
        else {
            allocation = bytes;
            bytes = 0;
        }

        if (allocatedCw == 0) {
            unsigned int blocks = 1;

            EV << NOW << "LteSchedulerEnbDl::rtxAcid Assigned blocks: " << blocks << endl;

            // assign only on the first codeword
            assignedBlocks.push_back(blocks);
            assignedBytes.push_back(allocation);
        }

        if (bytes == 0)
            break;
    }

    if (bytes > 0) {
        // process couldn't be served
        EV << NOW << "LteSchedulerEnbDl::rtxAcid Cannot serve HARQ Process" << acid << endl;
        return 0;
    }

    // record the allocation if performed
    size = assignedBlocks.size();
    // For each LB with assigned blocks
    for (unsigned int i = 0; i < size; ++i) {
        if (allocatedCw == 0) {
            // allocate the blocks
            allocator_->addBlocks(antenna, bandLim->at(i).band_, nodeId, assignedBlocks.at(i), assignedBytes.at(i));
        }
        // store the amount
        bandLim->at(i).limit_.at(remappedCw) = assignedBytes.at(i);
    }

    UnitList signal;
    signal.first = acid;
    signal.second.push_back(cw);

    EV << NOW << " LteSchedulerEnbDl::rtxAcid HARQ Process " << (int)acid << "  codeword  " << cw << " marking for retransmission " << endl;

    // if allocated codewords is not MAX_CODEWORDS, then there's another allocated codeword, update the codewords variable :

    if (allocatedCw != 0) {
        // TODO fixme this only works if MAX_CODEWORDS == 2
        --codewords;
        if (codewords <= 0)
            throw cRuntimeError("LteSchedulerEnbDl::rtxAcid(): erroneous codeword count %d", codewords);
    }

    // signal a retransmission
    currHarq->markSelected(signal, codewords);

    // mark codeword as used
    if (allocatedCws_.find(nodeId) != allocatedCws_.end()) {
        allocatedCws_.at(nodeId)++;
    }
    else {
        allocatedCws_[nodeId] = 1;
    }

    bytes = currHarq->pduLength(acid, cw);

    EV << NOW << " LteSchedulerEnbDl::rtxAcid HARQ Process " << (int)acid << "  codeword  " << cw << ", " << bytes << " bytes served!" << endl;

    return bytes;
}

unsigned int LteSchedulerEnbDl::scheduleBgRtx(MacNodeId bgUeId, double carrierFrequency, Codeword cw, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl)
{
    try {
        IBackgroundTrafficManager *bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);
        unsigned int bytesPerBlock = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_);

        // get the RTX buffer size
        unsigned int queueLength = bgTrafficManager->getBackloggedUeBuffer(bgUeId, direction_, true); // in bytes
        if (queueLength == 0)
            return 0;

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
                    elem.limit_[j] = -2;
                }
                tempBandLim.push_back(elem);
            }
            bandLim = &tempBandLim;
        }

        EV << NOW << "LteSchedulerEnbDl::scheduleBgRtx - Node[" << mac_->getMacNodeId() << ", User[" << bgUeId << "]" << endl;

        Codeword allocatedCw = 0;

        // bytes to serve
        unsigned int toServe = queueLength;
        // blocks to allocate for each band
        std::vector<unsigned int> assignedBlocks;
        // bytes which blocks from the preceding vector are supposed to satisfy
        std::vector<unsigned int> assignedBytes;

        // end loop signal [same as bytes > 0, but more secure]
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

            EV << NOW << " LteSchedulerEnbDl::scheduleBgRtx BAND " << b << endl;
            EV << NOW << " LteSchedulerEnbDl::scheduleBgRtx total bytes:" << queueLength << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " LteSchedulerEnbDl::scheduleBgRtx Available: " << bandAvailableBytes << " bytes" << endl;

            unsigned int servedBytes = 0;
            // there's no room on the current band for serving the entire request
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
            EV << NOW << " LteSchedulerEnbDl::scheduleBgRtx Unavailable space for serving node " << bgUeId << endl;
            return 0;
        }
        else {

            // record the allocation
            unsigned int size = assignedBlocks.size();
            unsigned int allocatedBytes = 0;
            for (unsigned int i = 0; i < size; ++i) {
                // For each LB for which blocks have been allocated
                Band b = bandLim->at(i).band_;

                allocatedBytes += assignedBytes.at(i);
                EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
                //! handle multi-codeword allocation
                if (allocatedCw != MAX_CODEWORDS) {
                    EV << NOW << " LteSchedulerEnbDl::scheduleBgRtx - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna, b, bgUeId, assignedBlocks.at(i), assignedBytes.at(i));
                }
            }

            // signal a retransmission

            // mark codeword as used
            if (allocatedCws_.find(bgUeId) != allocatedCws_.end())
                allocatedCws_.at(bgUeId)++;
            else
                allocatedCws_[bgUeId] = 1;

            EV << NOW << " LteSchedulerEnbDl::scheduleBgRtx: " << allocatedBytes << " bytes served! " << endl;

            return allocatedBytes;
        }
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSchedulerEnbUl::rtxAcid(): %s", e.what());
    }
    return 0;
}

bool LteSchedulerEnbDl::rtxschedule(double carrierFrequency, BandLimitVector *bandLim)
{
    EV << NOW << " LteSchedulerEnbDl::rtxschedule --------------------::[ START RTX-SCHEDULE ]::--------------------" << endl;
    EV << NOW << " LteSchedulerEnbDl::rtxschedule Cell:  " << mac_->getMacCellId() << " Direction: " << (direction_ == DL ? "DL" : "UL") << endl;

    // retrieving reference to HARQ entities
    HarqTxBuffers *harqQueues = mac_->getHarqTxBuffers(carrierFrequency);
    if (harqQueues != nullptr) {
        std::vector<BandLimit> usableBands;

        // examination of HARQ process in rtx status, adding them to scheduling list
        for (auto it = harqQueues->begin(); it != harqQueues->end(); ) {
            // For each UE
            MacNodeId nodeId = it->first;

            OmnetId id = binder_->getOmnetId(nodeId);
            if (id == 0) {
                // UE has left the simulation, erase HARQ queue
                it = harqQueues->erase(it);
                continue;
            }
            LteHarqBufferTx *currHarq = it->second;
            std::vector<LteHarqProcessTx *> *processes = currHarq->getHarqProcesses();

            // Get user transmission parameters
            const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency); // get the user info
            // TODO SK Get the number of codewords - FIX with correct mapping
            unsigned int codewords = txParams.getLayers().size(); // get the number of available codewords

            EV << NOW << " LteSchedulerEnbDl::rtxschedule  UE: " << nodeId << endl;
            // get the number of HARQ processes
            unsigned int process = 0;
            unsigned int maxProcesses = currHarq->getNumProcesses();
            for (process = 0; process < maxProcesses; ++process) {
                // for each HARQ process
                LteHarqProcessTx *currProc = (*processes)[process];

                if (allocatedCws_[nodeId] == codewords)
                    break;
                for (Codeword cw = 0; cw < codewords; ++cw) {

                    if (allocatedCws_[nodeId] == codewords)
                        break;
                    EV << NOW << " LteSchedulerEnbDl::rtxschedule process " << process << endl;

                    // skip processes which are not in rtx status
                    if (currProc->getUnitStatus(cw) != TXHARQ_PDU_BUFFERED) {
                        EV << NOW << " LteSchedulerEnbDl::rtxschedule detected Acid: " << process << " in status " << currProc->getUnitStatus(cw) << endl;
                        continue;
                    }

                    EV << NOW << " LteSchedulerEnbDl::rtxschedule detected RTX Acid: " << process << endl;

                    // Get the bandLimit for the current user
                    bool ret = getBandLimit(&usableBands, nodeId);
                    if (ret)
                        bandLim = &usableBands;                                                   // TODO fix this, must be combined with the band limit of the carrier
                    else
                        bandLim = nullptr;

                    // perform the retransmission
                    unsigned int bytes = schedulePerAcidRtx(nodeId, carrierFrequency, cw, process, bandLim);

                    // if a value different from zero is returned, there was a service
                    if (bytes > 0) {
                        EV << NOW << " LteSchedulerEnbDl::rtxschedule CODEWORD IS NOW BUSY!!!" << endl;

                        mac_->signalProcessForRtx(nodeId, carrierFrequency, DL, false);

                        // do not process this HARQ process anymore
                        // go to next codeword
                        break;
                    }
                }
            }
            ++it;
        }
    }

    unsigned int availableBlocks = allocator_->computeTotalRbs();
    EV << " LteSchedulerEnbDl::rtxschedule OFDM Space: " << availableBlocks << endl;
    EV << "    LteSchedulerEnbDl::rtxschedule --------------------::[  END RTX-SCHEDULE  ]::-------------------- " << endl;

    return availableBlocks == 0;
}

bool LteSchedulerEnbDl::rtxscheduleBackground(double carrierFrequency, BandLimitVector *bandLim)
{
    EV << NOW << " LteSchedulerEnbDl::rtxscheduleBackground --------------------::[ START RTX-SCHEDULE-BACKGROUND ]::--------------------" << endl;
    EV << NOW << " LteSchedulerEnbDl::rtxscheduleBackground Cell:  " << mac_->getMacCellId() << " Direction: " << (direction_ == DL ? "DL" : "UL") << endl;

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

        EV << NOW << "LteSchedulerEnbDl::rtxschedule BG UE " << bgUeId << " - allocated bytes : " << rtxBytes << endl;
    }

    // consume bytes
    for (auto & it : bgScheduledRtx)
        bgTrafficManager->consumeBackloggedUeBytes(it.first, it.second, direction_, true); // in bytes

    unsigned int availableBlocks = allocator_->computeTotalRbs();
    EV << " LteSchedulerEnbDl::rtxscheduleBackground OFDM Space: " << availableBlocks << endl;
    EV << "    LteSchedulerEnbDl::rtxscheduleBackground --------------------::[  END RTX-SCHEDULE-BACKGROUND  ]::-------------------- " << endl;

    return availableBlocks == 0;
}

bool LteSchedulerEnbDl::getBandLimit(std::vector<BandLimit> *bandLimit, MacNodeId ueId)
{
    bandLimit->clear();

    // get usable bands for this user
    UsableBands *usableBands = mac_->getAmc()->getPilotUsableBands(ueId);
    if (usableBands == nullptr) {
        // leave the bandLimit empty
        return false;
    }

    // check the number of codewords
    unsigned int numCodewords = 1;
    unsigned int numBands = mac_->getCellInfo()->getNumBands();
    // for each band of the band vector provided
    for (unsigned int i = 0; i < numBands; i++) {
        BandLimit elem;
        elem.band_ = Band(i);

        int limit = -2;

        // check whether band i is in the set of usable bands
        for (auto band : *usableBands) {
            if (band == i) {
                // band i must be marked as unlimited
                limit = -1;
                break;
            }
        }

        elem.limit_.clear();
        for (unsigned int j = 0; j < numCodewords; j++)
            elem.limit_.push_back(limit);

        bandLimit->push_back(elem);
    }

    return true;
}

} //namespace

