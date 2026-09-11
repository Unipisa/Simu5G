//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
// Editor: Mohamed Seliem (University College Cork)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/common/InitStages.h"
#include "simu5g/stack/mac/scheduler/LteSchedulerEnb.h"
#include "simu5g/stack/mac/allocator/LteAllocationModule.h"
#include "simu5g/stack/mac/allocator/LteAllocationModuleFrequencyReuse.h"
#include "simu5g/stack/mac/scheduler/LteScheduler.h"
#include "simu5g/stack/mac/scheduling_modules/LteDrr.h"
#include "simu5g/stack/mac/scheduling_modules/LteMaxCi.h"
#include "simu5g/stack/mac/scheduling_modules/LtePf.h"
#include "simu5g/stack/mac/scheduling_modules/LteMaxCiMultiband.h"
#include "simu5g/stack/mac/scheduling_modules/LteMaxCiOptMB.h"
#include "simu5g/stack/mac/scheduling_modules/LteMaxCiComp.h"
#include "simu5g/stack/mac/scheduling_modules/QoSAwareScheduler.h"
#include "simu5g/stack/mac/buffer/LteMacBuffer.h"
#include "simu5g/stack/mac/buffer/LteMacQueue.h"
#include "simu5g/stack/phy/PhyBase.h"

namespace simu5g {

using namespace omnetpp;

// Initialize statistics
simsignal_t LteSchedulerEnb::avgServedBlocksDlSignal_ = cComponent::registerSignal("avgServedBlocksDl");
simsignal_t LteSchedulerEnb::avgServedBlocksUlSignal_ = cComponent::registerSignal("avgServedBlocksUl");

LteSchedulerEnb::~LteSchedulerEnb()
{
    delete allocator_;
    for (auto* item : scheduler_)
        delete item;
}

void LteSchedulerEnb::initialize(int stage)
{
    if (stage != INITSTAGE_SIMU5G_AMC_SETUP)
        return;

    mac_ = check_and_cast<LteMacEnb *>(getParentModule());
    binder_ = check_and_cast<Binder *>(getModuleByPath(mac_->par("binderModule").stringValue()));
    direction_ = getDirection();
    resourceBlocks_ = mac_->getCellInfo()->getNumBands();

    harqTxBuffers_ = mac_->getHarqTxBuffers();
    harqRxBuffers_ = mac_->getHarqRxBuffers();

    // Create LteScheduler. One per carrier
    SchedDiscipline discipline = mac_->getSchedDiscipline(direction_);

    LteScheduler *newSched = nullptr;
    const CarrierInfoMap& carriers = mac_->getCellInfo()->getCarrierInfoMap();
    for (auto& item : carriers) {
        newSched = getScheduler(discipline);
        newSched->setEnbScheduler(this);
        newSched->setCarrierFrequency(item.second.carrierFrequency);
        newSched->setNumerologyIndex(item.second.numerologyIndex);     // set periodicity for this scheduler according to numerology
        newSched->initializeBandLimit();
        scheduler_.push_back(newSched);
    }

    // Create Allocator
    if (discipline == ALLOCATOR_BESTFIT)                                            // NOTE: create this type of allocator for every scheduler using Frequency Reuse
        allocator_ = new LteAllocationModuleFrequencyReuse(mac_, direction_);
    else
        allocator_ = new LteAllocationModule(mac_, direction_);

    initializeAllocator();

    WATCH(direction_);
    WATCH(resourceBlocks_);
    WATCH(utilization_);
    WATCH_SET(activeConnectionSet_);
    WATCH_MAP(allocatedCws_);
}

void LteSchedulerEnb::initializeSchedulerPeriodCounter(NumerologyIndex maxNumerologyIndex)
{
    for (const auto& schedulerItem : scheduler_)
        schedulerItem->initializeSchedulerPeriodCounter(maxNumerologyIndex);
}

std::map<GHz, LteMacScheduleList> *LteSchedulerEnb::schedule()
{
    EV << "LteSchedulerEnb::schedule performed by Node: " << mac_->getMacNodeId() << endl;

    // clearing structures for new scheduling
    for (auto & [key, value] : scheduleList_)
        value.clear();
    scheduledSoPduSizes_.clear();
    allocatedCws_.clear();

    // clean the allocator
    resetAllocator();

    // schedule one carrier at a time
    LteScheduler *scheduler = nullptr;
    for (auto & schedulerPtr : scheduler_) {
        scheduler = schedulerPtr;
        EV << "LteSchedulerEnb::schedule carrier [" << scheduler->getCarrierFrequency() << "]" << endl;

        unsigned int counter = scheduler->decreaseSchedulerPeriodCounter();
        if (counter > 0) {
            EV << " LteSchedulerEnb::schedule - not my turn (counter=" << counter << ")" << endl;
            continue;
        }

        // scheduling of RAC requests, retransmissions, and transmissions
        EV << "________________________start RAC+RTX _______________________________" << endl;
        if (!(scheduler->scheduleRacRequests()) && !(scheduler->scheduleRetransmissions())) {
            EV << "___________________________end RAC+RTX ________________________________" << endl;
            EV << "___________________________start SCHED ________________________________" << endl;
            scheduler->updateSchedulingInfo();
            scheduler->schedule();
            EV << "____________________________ end SCHED ________________________________" << endl;
        }
    }

    // record assigned resource blocks statistics
    resourceBlockStatistics();

    return &scheduleList_;
}

/*  COMPLETE:        scheduleGrant(cid,bytes,terminate,active,eligible,band_limit,antenna);
 *  ANTENNA UNAWARE: scheduleGrant(cid,bytes,terminate,active,eligible,band_limit);
 *  BAND UNAWARE:    scheduleGrant(cid,bytes,terminate,active,eligible);
 */
unsigned int LteSchedulerEnb::scheduleGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, GHz carrierFrequency, BandLimitVector *bandLim, Remote antenna, bool limitBl)
{
    // Get the node ID and logical connection ID
    MacNodeId nodeId = cid.getNodeId();
    LogicalCid flowId = cid.getLcid();

    // in the UL subframe a connection may be a D2D connection; promote the direction accordingly
    Direction dir = (direction_ == UL) ? directionFromBsrLcid(flowId, UL) : direction_;

    // Get user transmission parameters
    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, dir, carrierFrequency);
    const std::set<Band>& allowedBands = txParams.readBands();

    //get the number of codewords
    unsigned int numCodewords = txParams.getLayers().size();

    // TEST: check the number of codewords
    numCodewords = 1;

    EV << "LteSchedulerEnb::grant - deciding allowed Bands" << endl;
    std::string bands_msg = "BAND_LIMIT_SPECIFIED";
    std::vector<BandLimit> tempBandLim;
    if (bandLim == nullptr) {
        bands_msg = "NO_BAND_SPECIFIED";

        txParams.print("grant()");

        emptyBandLim_.clear();
        // Create a vector of band limit using all bands
        if (emptyBandLim_.empty()) {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // for each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++) {
                BandLimit elem;
                // copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                // mark as unlimited
                for (unsigned int j = 0; j < numCodewords; j++) {
                    EV << "- Codeword " << j << endl;
                    if (allowedBands.find(elem.band_) != allowedBands.end()) {
                        EV << "\t" << i << " " << "yes" << endl;
                        elem.limit_[j] = -1;
                    }
                    else {
                        EV << "\t" << i << " " << "no" << endl;
                        elem.limit_[j] = -2;
                    }
                }
                emptyBandLim_.push_back(elem);
            }
        }
        tempBandLim = emptyBandLim_;
        bandLim = &tempBandLim;
    }
    else {
        unsigned int numBands = mac_->getCellInfo()->getNumBands();
        // for each band of the band vector provided
        for (unsigned int i = 0; i < numBands; i++) {
            BandLimit& elem = bandLim->at(i);
            for (unsigned int j = 0; j < numCodewords; j++) {
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
    EV << "LteSchedulerEnb::grant(" << cid << "," << bytes << "," << terminate << "," << active << "," << eligible << "," << bands_msg << "," << dasToA(antenna) << ")" << endl;

    unsigned int totalAllocatedBytes = 0;  // total allocated data (in bytes)
    unsigned int totalAllocatedBlocks = 0; // total allocated data (in blocks)

    // === Perform normal operation for grant === //

    EV << "LteSchedulerEnb::grant --------------------::[ START GRANT ]::--------------------" << endl;
    EV << "LteSchedulerEnb::grant Cell: " << mac_->getMacCellId() << endl;
    EV << "LteSchedulerEnb::grant CID: " << cid << "(UE: " << nodeId << ", Flow: " << flowId << ") current Antenna [" << dasToA(antenna) << "]" << endl;

    // search for already allocated codeword
    unsigned int cwAlreadyAllocated = 0;
    if (allocatedCws_.find(nodeId) != allocatedCws_.end())
        cwAlreadyAllocated = allocatedCws_.at(nodeId);

    // Check OFDM space
    if (allocator_->computeTotalRbs() == 0)
    {
        terminate = true; // OFDM space ended, issuing terminate flag
        EV << "LteSchedulerEnb::grant Space ended, no scheduling." << endl;
        return 0;
    }

    // TODO This is just a BAD patch
    // check how a codeword may be reused (as in the if above) in case of non-empty OFDM space
    // otherwise check why a UE is stopped being scheduled while its buffer is not empty
    if (cwAlreadyAllocated > 0) {
        eligible = false;
        return 0;
    }

    // ===== DEBUG OUTPUT ===== //
    bool debug = false; // TODO: make this configurable
    if (debug) {
        if (limitBl)
            EV << "LteSchedulerEnb::grant blocks: " << bytes << endl;
        else
            EV << "LteSchedulerEnb::grant Bytes: " << bytes << endl;
        EV << "LteSchedulerEnb::grant Bands: {";
        unsigned int size = (*bandLim).size();
        if (size > 0) {
            EV << (*bandLim).at(0).band_;
            for (unsigned int i = 1; i < size; i++)
                EV << ", " << (*bandLim).at(i).band_;
        }
        EV << "}\n";
    }
    // ===== END DEBUG OUTPUT ===== //

    EV << "LteSchedulerEnb::grant TxMode: " << txModeToA(txParams.readTxMode()) << endl;
    EV << "LteSchedulerEnb::grant Available codewords: " << numCodewords << endl;

    // Retrieve the first free codeword checking the eligibility - check eligibility could modify current cw index.
    Codeword cw = 0; // current codeword, modified by reference by the checkEligibility function
    if (!checkEligibility(nodeId, cw, carrierFrequency) || cw >= numCodewords) {
        eligible = false;

        EV << "LteSchedulerEnb::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;
        EV << "LteSchedulerEnb::grant Total allocation: " << totalAllocatedBytes << " bytes" << endl;
        EV << "LteSchedulerEnb::grant NOT ELIGIBLE!!!" << endl;
        EV << "LteSchedulerEnb::grant --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes; // return the total number of served bytes
    }

    // Get virtual buffer reference
    LteMacBuffer *conn = (dir == DL) ? mac_->getMacBuffer(cid) : mac_->getBsrVirtualBuffer(cid);

    // get the buffer size -- the scheduled connection's own, which is what says
    // whether it is still an active candidate
    unsigned int queueLength = conn->getQueueOccupancy(); // in bytes
    if (queueLength == 0) {
        active = false;
        EV << "LteSchedulerEnb::scheduleGrant - scheduled connection is no longer active. Exiting grant " << endl;
        EV << "LteSchedulerEnb::grant --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes;
    }

    // An uplink grant is ONE transport block for the whole UE, which the UE's own
    // LCP then divides among its channels, so it is sized from everything the UE
    // has reported -- not from the single logical channel group whose
    // pseudo-connection won this scheduling round. The groups remain separate
    // candidates: they decide WHICH UE is served and how urgently, while the UE's
    // total decides how big its transport block is.
    //
    // The block is still drained from the scheduled group's mirror alone. The eNB
    // cannot know how the UE's LCP will actually divide the block, and crediting
    // another group with bytes the UE may not have sent from it would hide that
    // group's backlog until the next report -- the very blindness per-group
    // reporting exists to remove. A report replaces every mirror anyway.
    if (dir == UL && isUlBsrLcid(flowId)) {
        queueLength = 0;
        for (LteMacBuffer *mirror : mac_->getUlBacklogMirrors(nodeId))
            queueLength += mirror->getQueueOccupancy();
    }

    bool stop = false;
    unsigned int toServe = 0;
    for ( ; cw < numCodewords; ++cw) {
        EV << "LteSchedulerEnb::grant @@@@@ CODEWORD " << cw << " @@@@@" << endl;

        queueLength += MAC_HEADER + RLC_HEADER_UM;  // TODO RLC may be either UM or AM
        toServe = (queueLength <= bytes) ? queueLength : bytes; // do not serve more bytes than the maximum number of bytes requested
        EV << "LteSchedulerEnb::scheduleGrant bytes to be allocated: " << toServe << endl;

        unsigned int cwAllocatedBytes = 0;  // per codeword allocated bytes
        unsigned int cwAllocatedBlocks = 0; // used by uplink only, for signaling cw blocks usage to schedule list
        unsigned int vQueueItemCounter = 0; // per codeword MAC SDUs counter

        unsigned int allocatedCws = 0;
        unsigned int size = (*bandLim).size();
        for (unsigned int i = 0; i < size; ++i) { // for each band
            // save the band and the relative limit
            Band b = (*bandLim).at(i).band_;
            int limit = (*bandLim).at(i).limit_.at(cw);
            EV << "LteSchedulerEnb::grant --- BAND " << b << " LIMIT " << limit << "---" << endl;

            // if the limit flag is set to skip, jump off
            if (limit == -2) {
                EV << "LteSchedulerEnb::grant skipping logical band according to limit value" << endl;
                continue;
            }

            // search for already allocated codeword
            if (allocatedCws_.find(nodeId) != allocatedCws_.end())
                allocatedCws = allocatedCws_.at(nodeId);

            unsigned int bandAvailableBytes = 0;
            unsigned int bandAvailableBlocks = 0;
            // if there is a previous blocks allocation on the first codeword, blocks allocation is already available
            if (allocatedCws != 0) {
                // get band allocated blocks
                int b1 = allocator_->getBlocks(antenna, b, nodeId);
                // limit eventually allocated blocks on other codeword to limit for current cw
                bandAvailableBlocks = (limitBl ? (b1 > limit ? limit : b1) : b1);
                bandAvailableBytes = mac_->getAmc()->computeBytesOnNRbs(nodeId, b, cw, bandAvailableBlocks, dir, carrierFrequency);
            }
            else { // if limit is expressed in blocks, limit value must be passed to availableBytes function
                bandAvailableBlocks = allocator_->availableBlocks(nodeId, antenna, b);
                bandAvailableBytes = (bandAvailableBlocks == 0) ? 0 : availableBytes(nodeId, antenna, b, cw, dir, carrierFrequency, (limitBl) ? limit : -1); // available space (in bytes)
            }

            // if no allocation can be performed, notify to skip the band on next processing (if any)
            if (bandAvailableBytes == 0) {
                EV << "LteSchedulerEnb::grant Band " << b << " will be skipped since it has no space left." << endl;
                (*bandLim).at(i).limit_.at(cw) = -2;
                continue;
            }

            //if bandLimit is expressed in bytes
            if (!limitBl) {
                // use the provided limit as cap for available bytes, if it is not set to unlimited
                if (limit >= 0 && limit < (int)bandAvailableBytes) {
                    bandAvailableBytes = limit;
                    EV << "LteSchedulerEnb::grant Band space limited to " << bandAvailableBytes << " bytes according to limit cap" << endl;
                }
            }
            else {
                // if bandLimit is expressed in blocks
                if (limit >= 0 && limit < (int)bandAvailableBlocks) {
                    bandAvailableBlocks = limit;
                    EV << "LteSchedulerEnb::grant Band space limited to " << bandAvailableBlocks << " blocks according to limit cap" << endl;
                }
            }

            EV << "LteSchedulerEnb::grant Available Bytes: " << bandAvailableBytes << " available blocks " << bandAvailableBlocks << endl;

            unsigned int uBytes = (bandAvailableBytes > toServe) ? toServe : bandAvailableBytes;
            unsigned int uBlocks = 1;

            // allocate resources on this band
            if (allocatedCws == 0) {
                // mark here allocation
                allocator_->addBlocks(antenna, b, nodeId, uBlocks, uBytes);
                // add allocated blocks for this codeword
                cwAllocatedBlocks += uBlocks;
                totalAllocatedBlocks += uBlocks;
                cwAllocatedBytes += uBytes;
            }

            // update limit
            if (uBlocks > 0 && (*bandLim).at(i).limit_.at(cw) > 0) {
                (*bandLim).at(i).limit_.at(cw) -= uBlocks;
                if ((*bandLim).at(i).limit_.at(cw) < 0)
                    throw cRuntimeError("Limit decreasing error during booked resources allocation on band %d : new limit %d, due to blocks %d ",
                            b, (*bandLim).at(i).limit_.at(cw), uBlocks);
            }

            // update the counter of bytes to be served
            toServe = (uBytes > toServe) ? 0 : toServe - uBytes;
            queueLength = (uBytes > toServe) ? 0 : queueLength - uBytes;
            if (toServe == 0) {
                // all bytes booked, go to allocation
                stop = true;
                if (queueLength == 0)                                        // the connection has also terminated the buffer
                    active = false;
                break;
            }
            // continue allocating (if there are available bands)
        }// Closes loop on bands

        // === update virtual buffer === //

        // NR-SO DL connections emit one SDU/segment per RLC PDU (no concatenation), so
        // the grant is filled by multiplexing several PDUs into it: count each as a PDU
        // (vQueueItemCounter feeds the DL schedule-list SDU count) and record per-PDU
        // payload sizes for the MAC to request. LTE-FI keeps the single-concatenated-PDU
        // path below. (UL is handled by LcgScheduler.)
        bool soFraming = (dir == DL) && cwAllocatedBytes > 0 && mac_->getLogicalChannelConfig(cid).soFraming;
        if (soFraming) {
            std::vector<unsigned int>& pduSizes = scheduledSoPduSizes_[cid];
            int budget = (int)cwAllocatedBytes - MAC_HEADER;  // one MAC header for the TB
            while (!conn->isEmpty()) {
                unsigned int vPktSize = conn->front().first;
                // RLC header for this PDU: the exact segment-state header the RLC TX will
                // emit, so the carve and drain match byte-for-byte (sized for the flow's SN
                // field length). UM: complete=1B (no SN) if the whole SDU fits, else
                // first/continuation per nrUmHeaderBytes. AM always carries the SN. TS 38.322.
                unsigned int snBits = mac_->getLogicalChannelConfig(cid).snFieldLength;
                unsigned int rlcHdr;
                if (mac_->getLogicalChannelConfig(cid).rlcMode == AM) {
                    rlcHdr = nrAmHeaderBytes(soFrontIsContinuation_[cid] ? NRUM_CONTINUATION : NRUM_FIRST, snBits);
                }
                else {
                    NrUmSegState st = soFrontIsContinuation_[cid] ? NRUM_CONTINUATION
                                    : ((int)vPktSize + 1 <= budget) ? NRUM_COMPLETE
                                    : NRUM_FIRST;
                    rlcHdr = nrUmHeaderBytes(st, snBits);
                }
                if (budget <= (int)rlcHdr)
                    break;
                int room = budget - (int)rlcHdr;
                unsigned int payload;
                bool segmented = false;
                if ((int)vPktSize <= room) {
                    conn->popFront();
                    payload = vPktSize;
                    soFrontIsContinuation_[cid] = false;
                }
                else {
                    PacketInfo newPktInfo = conn->popFront();
                    newPktInfo.first = vPktSize - (unsigned int)room;
                    conn->pushFront(newPktInfo);
                    payload = (unsigned int)room;
                    segmented = true;
                    soFrontIsContinuation_[cid] = true;
                }
                // request size includes the RLC header (rlcPduMakeNr subtracts it back)
                pduSizes.push_back(payload + rlcHdr);
                budget -= (int)rlcHdr + (int)payload;
                vQueueItemCounter++;
                if (segmented)
                    break;  // grant exhausted by the segment
            }
            if (conn->isEmpty())
                soFrontIsContinuation_[cid] = false;  // buffer drained; next SDU is fresh
            if (pduSizes.empty())
                scheduledSoPduSizes_.erase(cid);
        }
        else {
            if (cwAllocatedBytes > 0)
                vQueueItemCounter++;                                    // increase counter of served SDU

            unsigned int consumedBytes = (cwAllocatedBytes == 0) ? 0 : cwAllocatedBytes - (MAC_HEADER + RLC_HEADER_UM);  // TODO RLC may be either UM or AM

            // number of bytes to be consumed from the virtual buffer
            while (!conn->isEmpty() && consumedBytes > 0) {

                unsigned int vPktSize = conn->front().first;
                if (vPktSize <= consumedBytes) {
                    // serve the entire vPkt, remove pkt info
                    conn->popFront();
                    consumedBytes -= vPktSize;
                    EV << "LteSchedulerEnb::grant - the first SDU/BSR is served entirely, remove it from the virtual buffer, remaining bytes to serve[" << consumedBytes << "]" << endl;
                }
                else {
                    // serve partial vPkt, update pkt info
                    PacketInfo newPktInfo = conn->popFront();
                    newPktInfo.first = newPktInfo.first - consumedBytes;
                    conn->pushFront(newPktInfo);
                    consumedBytes = 0;
                    EV << "LteSchedulerEnb::grant - the first SDU/BSR is partially served, update its size [" << newPktInfo.first << "]" << endl;
                }
            }
        }

        EV << "LteSchedulerEnb::grant Codeword allocation: " << cwAllocatedBytes << " bytes" << endl;
        if (cwAllocatedBytes > 0) {
            // mark codeword as used
            if (allocatedCws_.find(nodeId) != allocatedCws_.end())
                allocatedCws_.at(nodeId)++;
            else
                allocatedCws_[nodeId] = 1;

            totalAllocatedBytes += cwAllocatedBytes;

            // create entry in the schedule list for this carrier
            if (scheduleList_.find(carrierFrequency) == scheduleList_.end()) {
                LteMacScheduleList newScheduleList;
                scheduleList_[carrierFrequency] = newScheduleList;
            }
            // create entry in the schedule list for this pair <cid,cw>
            std::pair<MacCid, Codeword> scListId(cid, cw);
            if (scheduleList_[carrierFrequency].find(scListId) == scheduleList_[carrierFrequency].end())
                scheduleList_[carrierFrequency][scListId] = 0;

            // if direction is DL , then schedule list contains number of to-be-transmitted SDUs ,
            // otherwise it contains the number of granted blocks
            scheduleList_[carrierFrequency][scListId] += ((dir == DL) ? vQueueItemCounter : cwAllocatedBlocks);

            EV << "LteSchedulerEnb::grant CODEWORD IS NOW BUSY: GO TO NEXT CODEWORD." << endl;
            if (allocatedCws_.at(nodeId) == MAX_CODEWORDS) {
                eligible = false;
                stop = true;
            }
        }
        else {
            EV << "LteSchedulerEnb::grant CODEWORD IS FREE: NO ALLOCATION IS POSSIBLE IN NEXT CODEWORD." << endl;
            eligible = false;
            stop = true;
        }
        if (stop)
            break;
    } // Closes loop on Codewords

    EV << "LteSchedulerEnb::grant Total allocation: " << totalAllocatedBytes << " bytes, " << totalAllocatedBlocks << " blocks" << endl;
    EV << "LteSchedulerEnb::grant --------------------::[  END GRANT  ]::--------------------" << endl;

    return totalAllocatedBytes;
}

unsigned int LteSchedulerEnb::scheduleGrantBackground(MacCid bgCid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, GHz carrierFrequency, BandLimitVector *bandLim, Remote antenna, bool limitBl)
{
    MacNodeId bgUeId = bgCid.getNodeId();

    // Get the number of codewords
    unsigned int numCodewords = 1;

    EV << "LteSchedulerEnb::grant - deciding allowed Bands" << endl;
    std::string bands_msg = "BAND_LIMIT_SPECIFIED";
    std::vector<BandLimit> tempBandLim;
    if (bandLim == nullptr) {
        bands_msg = "NO_BAND_SPECIFIED";

        emptyBandLim_.clear();
        // Create a vector of band limit using all bands
        if (emptyBandLim_.empty()) {
            unsigned int numBands = mac_->getCellInfo()->getNumBands();
            // For each band of the band vector provided
            for (unsigned int i = 0; i < numBands; i++) {
                BandLimit elem;
                // Copy the band
                elem.band_ = Band(i);
                EV << "Putting band " << i << endl;
                // Mark all bands as unlimited
                for (unsigned int j = 0; j < numCodewords; j++)
                    elem.limit_[j] = -1;
                emptyBandLim_.push_back(elem);
            }
        }
        tempBandLim = emptyBandLim_;
        bandLim = &tempBandLim;
    }
    // Else use the one passed to the function

    EV << "LteSchedulerEnb::grant(" << bgCid << "," << bytes << "," << terminate << "," << active << "," << eligible << "," << bands_msg << ")" << endl;

    unsigned int totalAllocatedBytes = 0;  // total allocated data (in bytes)
    unsigned int totalAllocatedBlocks = 0; // total allocated data (in blocks)
    RbMap allocatedRbMap;

    // === Perform normal operation for grant === //

    EV << "LteSchedulerEnb::scheduleGrantBackground --------------------::[ START GRANT ]::--------------------" << endl;
    EV << "LteSchedulerEnb::scheduleGrantBackground Cell: " << mac_->getMacCellId() << endl;
    EV << "LteSchedulerEnb::scheduleGrantBackground CID: " << bgCid << "(UE: " << bgUeId << ")" << endl;

    // Search for already allocated codeword
    unsigned int cwAlreadyAllocated = 0;
    if (allocatedCws_.find(bgUeId) != allocatedCws_.end())
        cwAlreadyAllocated = allocatedCws_.at(bgUeId);

    if (cwAlreadyAllocated > 0) {
        terminate = true;
        return 0;
    }

    // Check OFDM space
    if (allocator_->computeTotalRbs() == 0) {
        terminate = true; // OFDM space ended, issuing terminate flag
        EV << "LteSchedulerEnb::scheduleGrantBackground Space ended, stop scheduling" << endl;
        return 0;
    }

    // ===== DEBUG OUTPUT ===== //
    bool debug = false; // TODO: make this configurable
    if (debug) {
        if (limitBl)
            EV << "LteSchedulerEnb::grant blocks: " << bytes << endl;
        else
            EV << "LteSchedulerEnb::grant Bytes: " << bytes << endl;
        EV << "LteSchedulerEnb::grant Bands: {";
        unsigned int size = (*bandLim).size();
        if (size > 0) {
            EV << (*bandLim).at(0).band_;
            for (unsigned int i = 1; i < size; i++)
                EV << ", " << (*bandLim).at(i).band_;
        }
        EV << "}\n";
    }
    // ===== END DEBUG OUTPUT ===== //

    IBackgroundTrafficManager *bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);

    // Get the buffer size
    unsigned int queueLength = bgTrafficManager->getBackloggedUeBuffer(bgUeId, direction_); // in bytes
    if (queueLength == 0) {
        active = false;
        EV << "LteSchedulerEnb::scheduleGrantBackground - scheduled connection is no longer active. Exiting grant " << endl;
        EV << "LteSchedulerEnb::scheduleGrantBackground --------------------::[  END GRANT  ]::--------------------" << endl;
        return totalAllocatedBytes;
    }

    bool stop = false;
    unsigned int toServe = 0;
    for (unsigned int cw = 0; cw < numCodewords; ++cw) {
        EV << "LteSchedulerEnb::scheduleGrantBackground @@@@@ CODEWORD " << cw << " @@@@@" << endl;

        queueLength += MAC_HEADER + RLC_HEADER_UM;  // TODO RLC may be either UM or AM
        toServe = queueLength;
        EV << "LteSchedulerEnb::scheduleGrantBackground bytes to be allocated: " << toServe << endl;

        unsigned int cwAllocatedBytes = 0;  // per codeword allocated bytes
        std::map<Band, unsigned int> allocatedRbMapEntry;

        unsigned int allocatedCws = 0;
        unsigned int size = (*bandLim).size();
        for (unsigned int i = 0; i < size; ++i) { // for each band

            // Save the band and the relative limit
            Band b = (*bandLim).at(i).band_;
            int limit = (*bandLim).at(i).limit_.at(cw);
            EV << "LteSchedulerEnb::scheduleGrantBackground --- BAND " << b << " LIMIT " << limit << "---" << endl;

            // If the limit flag is set to skip, jump off
            if (limit == -2) {
                EV << "LteSchedulerEnb::scheduleGrantBackground skipping logical band according to limit value" << endl;
                continue;
            }

            // Search for already allocated codeword
            if (allocatedCws_.find(bgUeId) != allocatedCws_.end())
                allocatedCws = allocatedCws_.at(bgUeId);

            unsigned int bandAvailableBytes = 0;
            unsigned int bandAvailableBlocks = 0;
            allocatedRbMapEntry[i] = 0;

            // If there is a previous blocks allocation on the first codeword, blocks allocation is already available
            if (allocatedCws != 0) {
                // Get band allocated blocks
                int b1 = allocator_->getBlocks(MACRO, b, bgUeId);
                // Limit eventually allocated blocks on other codeword to limit for current cw
                bandAvailableBlocks = (limitBl ? (b1 > limit ? limit : b1) : b1);
                bandAvailableBytes = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_);
            }
            else { // if limit is expressed in blocks, limit value must be passed to availableBytes function
                bandAvailableBytes = availableBytesBackgroundUe(bgUeId, antenna, b, direction_, carrierFrequency, (limitBl) ? limit : -1); // available space (in bytes)
                bandAvailableBlocks = allocator_->availableBlocks(bgUeId, antenna, b);
            }

            // If no allocation can be performed, notify to skip the band on next processing (if any)
            if (bandAvailableBytes == 0) {
                EV << "LteSchedulerEnb::scheduleGrantBackground Band " << b << " will be skipped since it has no space left." << endl;
                (*bandLim).at(i).limit_.at(cw) = -2;
                continue;
            }

            // If bandLimit is expressed in bytes
            if (!limitBl) {
                // Use the provided limit as cap for available bytes, if it is not set to unlimited
                if (limit >= 0 && limit < (int)bandAvailableBytes) {
                    bandAvailableBytes = limit;
                    EV << "LteSchedulerEnb::grant Band space limited to " << bandAvailableBytes << " bytes according to limit cap" << endl;
                }
            }
            else {
                // If bandLimit is expressed in blocks
                if (limit >= 0 && limit < (int)bandAvailableBlocks) {
                    bandAvailableBlocks = limit;
                    EV << "LteSchedulerEnb::grant Band space limited to " << bandAvailableBlocks << " blocks according to limit cap" << endl;
                }
            }

            EV << "LteSchedulerEnb::grant Available Bytes: " << bandAvailableBytes << " available blocks " << bandAvailableBlocks << endl;

            unsigned int uBytes = (bandAvailableBytes > queueLength) ? queueLength : bandAvailableBytes;
            unsigned int uBytesPerBlock = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_); // in bytes
            unsigned int uBlocks = ceil((double)uBytes / uBytesPerBlock);

            // Allocate resources on this band
            if (allocatedCws == 0) {
                // Mark here allocation
                allocator_->addBlocks(antenna, b, bgUeId, uBlocks, uBytes);
                // Add allocated blocks for this codeword
                totalAllocatedBlocks += uBlocks;
                cwAllocatedBytes += uBytes;

                allocatedRbMapEntry[i] += uBlocks;
            }

            // Update limit
            if (uBlocks > 0 && (*bandLim).at(i).limit_.at(cw) > 0) {
                (*bandLim).at(i).limit_.at(cw) -= uBlocks;
                if ((*bandLim).at(i).limit_.at(cw) < 0)
                    throw cRuntimeError("Limit decreasing error during booked resources allocation on band %d : new limit %d, due to blocks %d ",
                            b, (*bandLim).at(i).limit_.at(cw), uBlocks);
            }

            // Update the counter of bytes to be served
            toServe = (uBytes > toServe) ? 0 : toServe - uBytes;
            if (toServe == 0) {
                // All bytes booked, go to allocation
                stop = true;
                active = false;
                break;
            }
            // Continue allocating (if there are available bands)
        }// Closes loop on bands

        // Update rb map
        allocatedRbMap[antenna] = allocatedRbMapEntry;

        // === update buffer === //

        unsigned int consumedBytes = (cwAllocatedBytes == 0) ? 0 : cwAllocatedBytes - (MAC_HEADER + RLC_HEADER_UM);  // TODO RLC may be either UM or AM

        unsigned int toConsume;
        if (consumedBytes <= (queueLength - (MAC_HEADER + RLC_HEADER_UM)))
            toConsume = consumedBytes;
        else
            toConsume = queueLength - (MAC_HEADER + RLC_HEADER_UM);
        bgTrafficManager->consumeBackloggedUeBytes(bgUeId, toConsume, direction_); // in bytes

        if (direction_ == UL) {
            // If uplink interference is enabled, mark the occupation in the uplink transmission map (for uplink interference computation purposes)
            ChannelModelBase *channelModel = mac_->getPhy()->getChannelModel(carrierFrequency);
            if (channelModel->isUplinkInterferenceEnabled())
                binder_->storeUlTransmissionMap(carrierFrequency, antenna, allocatedRbMap, bgUeId, mac_->getMacCellId(), bgTrafficManager->getTrafficGenerator(bgUeId), UL);
        }

        EV << "LteSchedulerEnb::grant Codeword allocation: " << cwAllocatedBytes << " bytes" << endl;
        if (cwAllocatedBytes > 0) {
            // Mark codeword as used
            if (allocatedCws_.find(bgUeId) != allocatedCws_.end())
                allocatedCws_.at(bgUeId)++;
            else
                allocatedCws_[bgUeId] = 1;

            totalAllocatedBytes += cwAllocatedBytes;
            if (allocatedCws_.at(bgUeId) == MAX_CODEWORDS) {
                eligible = false;
                stop = true;
            }
        }
        else {
            EV << "LteSchedulerEnb::grant CODEWORD IS FREE: NO ALLOCATION IS POSSIBLE IN NEXT CODEWORD." << endl;
            eligible = false;
            stop = true;
        }
        if (stop)
            break;
    } // Closes loop on Codewords

    EV << "LteSchedulerEnb::grant Total allocation: " << totalAllocatedBytes << " bytes, " << totalAllocatedBlocks << " blocks" << endl;
    EV << "LteSchedulerEnb::grant --------------------::[  END GRANT  ]::--------------------" << endl;

    return totalAllocatedBytes;
}

void LteSchedulerEnb::backlog(MacCid cid)
{
    EV << "LteSchedulerEnb::backlog - backlogged data for Logical Cid " << cid << endl;
    if (cid == MacCid(MacNodeId(0), LogicalCid(1)))
        return;

    EV << NOW << " LteSchedulerEnb::backlog CID notified " << cid << endl;
    activeConnectionSet_.insert(cid);

    for (auto* schedulerItem : scheduler_)
        schedulerItem->notifyActiveConnection(cid);
}

unsigned int LteSchedulerEnb::readPerUeAllocatedBlocks(const MacNodeId nodeId,
        const Remote antenna, const Band b)
{
    return allocator_->getBlocks(antenna, b, nodeId);
}

unsigned int LteSchedulerEnb::readPerBandAllocatedBlocks(Plane plane, const Remote antenna, const Band band)
{
    return allocator_->getAllocatedBlocks(plane, antenna, band);
}

unsigned int LteSchedulerEnb::getInterferingBlocks(Plane plane, const Remote antenna, const Band band)
{
    return allocator_->getInterferingBlocks(plane, antenna, band);
}

unsigned int LteSchedulerEnb::readAvailableRbs(const MacNodeId id,
        const Remote antenna, const Band b)
{
    return allocator_->availableBlocks(id, antenna, b);
}

unsigned int LteSchedulerEnb::readTotalAvailableRbs()
{
    return allocator_->computeTotalRbs();
}

unsigned int LteSchedulerEnb::readRbOccupation(const MacNodeId id, GHz carrierFrequency, RbMap& rbMap)
{
    RbMap tmpRbMap;

    int ret = allocator_->rbOccupation(id, tmpRbMap);

    // Parse rbMap according to the carrier
    Band startingBand = mac_->getCellInfo()->getCarrierStartingBand(carrierFrequency);
    Band lastBand = mac_->getCellInfo()->getCarrierLastBand(carrierFrequency);
    for (const auto& [bandKey, bandValue] : tmpRbMap) {
        std::map<Band, unsigned int> remoteEntry;
        unsigned int i = 0;
        for (Band b = startingBand; b <= lastBand; b++) {
            remoteEntry[i] = tmpRbMap[bandKey][b];
            i++;
        }
        rbMap[bandKey] = remoteEntry;
    }

    return ret;
}

/*
 * OFDMA frame management
 */

void LteSchedulerEnb::initializeAllocator()
{
    // Initialize the allocator
    allocator_->init(resourceBlocks_, mac_->getCellInfo()->getNumBands());
}

void LteSchedulerEnb::resetAllocator()
{
    // Reset the allocator
    allocator_->reset(resourceBlocks_, mac_->getCellInfo()->getNumBands());
}

unsigned int LteSchedulerEnb::availableBytes(const MacNodeId id,
        Remote antenna, Band b, Codeword cw, Direction dir, GHz carrierFrequency, int limit)
{
    EV << "LteSchedulerEnb::availableBytes MacNodeId " << id << " Antenna " << dasToA(antenna) << " band " << b << " cw " << cw << endl;
    // Retrieving this user available resource blocks
    int blocks = allocator_->availableBlocks(id, antenna, b);
    // Consistency Check
    if (limit > blocks && limit != -1)
        throw cRuntimeError("LteSchedulerEnb::availableBytes signaled limit inconsistency with available space band b %d, limit %d, available blocks %d", b, limit, blocks);

    if (limit != -1)
        blocks = (blocks > limit) ? limit : blocks;

    unsigned int bytes = mac_->getAmc()->computeBytesOnNRbs(id, b, cw, blocks, dir, carrierFrequency);
    EV << "LteSchedulerEnb::availableBytes MacNodeId " << id << " blocks [" << blocks << "], bytes [" << bytes << "]" << endl;

    return bytes;
}

unsigned int LteSchedulerEnb::availableBytesBackgroundUe(const MacNodeId id, Remote antenna, Band b, Direction dir, GHz carrierFrequency, int limit)
{
    EV << "LteSchedulerEnb::availableBytes MacNodeId " << id << " Antenna " << dasToA(antenna) << " band " << b << endl;
    // Retrieving this user's available resource blocks
    int blocks = allocator_->availableBlocks(id, antenna, b);
    if (blocks == 0) {
        EV << "LteSchedulerEnb::availableBytes - No blocks available on band " << b << endl;
        return 0;
    }

    // Consistency Check
    if (limit > blocks && limit != -1)
        throw cRuntimeError("LteSchedulerEnb::availableBytes signaled limit inconsistency with available space band b %d, limit %d, available blocks %d", b, limit, blocks);

    if (limit != -1)
        blocks = (blocks > limit) ? limit : blocks;

    unsigned int bytesPerBlock = mac_->getBackgroundTrafficManager(carrierFrequency)->getBackloggedUeBytesPerBlock(id, dir);
    unsigned int bytes = bytesPerBlock * blocks;
    EV << "LteSchedulerEnb::availableBytes MacNodeId " << id << " blocks [" << blocks << "], bytes [" << bytes << "]" << endl;

    return bytes;
}

std::set<Band> LteSchedulerEnb::getOccupiedBands()
{
    return allocator_->getAllocatorOccupiedBands();
}

void LteSchedulerEnb::storeAllocationEnb(std::vector<std::vector<AllocatedRbsPerBandMapA>> allocatedRbsPerBand, std::set<Band> *untouchableBands)
{
    allocator_->storeAllocation(allocatedRbsPerBand, untouchableBands);
}

void LteSchedulerEnb::storeScListId(GHz carrierFrequency, std::pair<MacCid, Codeword> scList, unsigned int num_blocks)
{
    scheduleList_[carrierFrequency][scList] = num_blocks;
}

/*****************
* UTILITIES
*****************/

LteScheduler *LteSchedulerEnb::getScheduler(SchedDiscipline discipline)
{
    EV << "Creating LteScheduler " << schedDisciplineToA(discipline) << endl;

    switch (discipline) {
        case DRR:
            return new LteDrr(binder_);
        case PF:
            return new LtePf(binder_, mac_->par("pfAlpha").doubleValue());
        case MAXCI:
            return new LteMaxCi(binder_);
        case MAXCI_MB:
            return new LteMaxCiMultiband(binder_);
        case MAXCI_OPT_MB:
            return new LteMaxCiOptMB(binder_);
        case MAXCI_COMP:
            return new LteMaxCiComp(binder_);
        case ALLOCATOR_BESTFIT:
            // ALLOCATOR_BESTFIT is a D2D frequency-reuse discipline; only the D2D
            // uplink scheduler subclasses (LteSchedulerEnbUlD2D/NrSchedulerGnbUlD2D)
            // construct it. Reaching this in a non-D2D scheduler is a misconfiguration.
            throw cRuntimeError("ALLOCATOR_BESTFIT requires a D2D-capable uplink scheduler "
                    "(set ulSchedulerType to LteSchedulerEnbUlD2D/NrSchedulerGnbUlD2D)");
        case QOS_PF: {
            auto *scheduler = new QoSAwareScheduler(binder_, mac_->par("pfAlpha").doubleValue());
            scheduler->setDrbQosMap(mac_->getDrbQosMap());
            return scheduler;
        }

        default:
            throw cRuntimeError("LteScheduler not recognized");
            return nullptr;
    }
}

void LteSchedulerEnb::resourceBlockStatistics(bool sleep)
{
    if (sleep) {
        if (direction_ == DL)
            mac_->emit(avgServedBlocksDlSignal_, (long)0);
        return;
    }
    // Get a reference to the beginning and the end of the map which stores the blocks allocated
    // by each UE in each Band. In this case, the pair of iterators which refers
    // to the per-Band (first key) per-Ue (second key) map is requested
    auto planeIt =
        allocator_->getAllocatedBlocksBegin();

    double allocatedBlocks = 0;
    unsigned int antenna = 0;

    // For each antenna (MACRO/RUs)
    for (auto num : *planeIt) {
        allocatedBlocks += (double)(num);

        // collect the antenna utilization for the current Layer
        utilization_ += (double)(num);
    }

    utilization_ /= (((double)(antenna)) * ((double)resourceBlocks_));

    if (direction_ == DL)
        mac_->emit(avgServedBlocksDlSignal_, allocatedBlocks);
    else if (direction_ == UL)
        mac_->emit(avgServedBlocksUlSignal_, allocatedBlocks);
    else
        throw cRuntimeError("LteSchedulerEnb::resourceBlockStatistics(): Unrecognized direction %d", direction_);
}

ActiveSet *LteSchedulerEnb::readActiveConnections()
{
    return &activeConnectionSet_;
}

void LteSchedulerEnb::removeActiveConnections(MacNodeId nodeId)
{
    for (auto it = activeConnectionSet_.begin(); it != activeConnectionSet_.end(); ) {
        MacCid cid = *it;
        if (cid.getNodeId() == nodeId) {
            EV << NOW << "LteSchedulerEnb::removeActiveConnections CID removed " << cid << endl;
            it = activeConnectionSet_.erase(it);
        }
        else
            ++it;
    }
}

BandLimitVector *LteSchedulerEnb::applyAllowedBandLimits(MacNodeId nodeId, Direction dir, GHz carrierFrequency, BandLimitVector *bandLim, BandLimitVector& storage)
{
    const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, dir, carrierFrequency);    // get the user info
    const std::set<Band>& allowedBands = txParams.readBands();
    unsigned int numBands = mac_->getCellInfo()->getNumBands();
    if (bandLim == nullptr) {
        // create a vector of band limits using all bands
        for (unsigned int i = 0; i < numBands; i++) {
            BandLimit elem;
            // copy the band
            elem.band_ = Band(i);
            EV << "Putting band " << i << endl;
            for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
                elem.limit_[j] = (allowedBands.find(elem.band_) != allowedBands.end()) ? -1 : -2;
            storage.push_back(elem);
        }
        return &storage;
    }
    // mask the provided vector in place; already-excluded entries stay excluded
    for (unsigned int i = 0; i < numBands; i++) {
        BandLimit& elem = bandLim->at(i);
        for (unsigned int j = 0; j < MAX_CODEWORDS; j++) {
            if (elem.limit_[j] == -2)
                continue;
            elem.limit_[j] = (allowedBands.find(elem.band_) != allowedBands.end()) ? -1 : -2;
        }
    }
    return bandLim;
}

void LteSchedulerEnb::makeUniformBandLimits(BandLimitVector& storage, unsigned int numBands, int limit)
{
    for (unsigned int i = 0; i < numBands; i++) {
        BandLimit elem;
        // copy the band
        elem.band_ = Band(i);
        for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
            elem.limit_[j] = limit;
        storage.push_back(elem);
    }
}

unsigned int LteSchedulerEnb::allocateRtxBytes(MacNodeId nodeId, Direction dir, LogicalCid bsrLcid, unsigned int bytes, GHz carrierFrequency, Codeword cw, unsigned char acid, BandLimitVector *bandLim, Remote antenna, bool& served)
{
    served = false;

    Codeword allocatedCw = 0;
    // search for already allocated codeword
    // create "mirror" scList ID for other codeword than current
    std::pair<MacCid, Codeword> scListMirrorId = std::pair<MacCid, Codeword>(MacCid(nodeId, bsrLcid), MAX_CODEWORDS - cw - 1);
    if (scheduleList_.find(carrierFrequency) != scheduleList_.end()) {
        if (scheduleList_[carrierFrequency].find(scListMirrorId) != scheduleList_[carrierFrequency].end()) {
            allocatedCw = MAX_CODEWORDS - cw - 1;
        }
    }
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
        unsigned int bandAvailableBytes = availableBytes(nodeId, antenna, b, cw, dir, carrierFrequency);

        // use the provided limit as cap for available bytes, if it is not set to unlimited
        if (limit >= 0)
            bandAvailableBytes = limit < (int)bandAvailableBytes ? limit : bandAvailableBytes;

        EV << NOW << " LteSchedulerEnb::allocateRtxBytes BAND " << b << endl;
        EV << NOW << " LteSchedulerEnb::allocateRtxBytes total bytes:" << bytes << " still to serve: " << toServe << " bytes" << endl;
        EV << NOW << " LteSchedulerEnb::allocateRtxBytes Available: " << bandAvailableBytes << " bytes" << endl;

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
        EV << NOW << " LteSchedulerEnb::allocateRtxBytes Unavailable space for serving node " << nodeId << " ,HARQ Process " << (unsigned int)acid << " on codeword " << cw << endl;
        return 0;
    }
    else {
        // record the allocation
        unsigned int size = assignedBlocks.size();
        unsigned int cwAllocatedBlocks = 0;

        // create scList id for current node/codeword
        std::pair<MacCid, Codeword> scListId = std::pair<MacCid, Codeword>(MacCid(nodeId, bsrLcid), cw);

        for (unsigned int i = 0; i < size; ++i) {
            // For each LB for which blocks have been allocated
            Band b = bandLim->at(i).band_;

            cwAllocatedBlocks += assignedBlocks.at(i);
            EV << "\t Cw->" << allocatedCw << "/" << MAX_CODEWORDS << endl;
            //! handle multi-codeword allocation
            if (allocatedCw != MAX_CODEWORDS) {
                EV << NOW << " LteSchedulerEnb::allocateRtxBytes - adding " << assignedBlocks.at(i) << " to band " << i << endl;
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

        EV << NOW << " LteSchedulerEnb::allocateRtxBytes HARQ Process " << (unsigned int)acid << " : " << bytes << " bytes served! " << endl;

        served = true;
        return bytes;
    }
}

bool LteSchedulerEnb::rtxscheduleBackground(GHz carrierFrequency, BandLimitVector *bandLim)
{
    try {
        EV << NOW << " LteSchedulerEnb::rtxscheduleBackground --------------------::[ START RTX-SCHEDULE-BACKGROUND ]::--------------------" << endl;
        EV << NOW << " LteSchedulerEnb::rtxscheduleBackground Cell: " << mac_->getMacCellId() << " Direction: " << (direction_ == UL ? "UL" : "DL") << endl;

        // --- Schedule RTX for background UEs --- //
        std::map<MacNodeId, unsigned int> bgScheduledRtx;
        IBackgroundTrafficManager *bgTrafficManager = mac_->getBackgroundTrafficManager(carrierFrequency);
        auto it = bgTrafficManager->getBackloggedUesBegin(direction_, true),
                                       et = bgTrafficManager->getBackloggedUesEnd(direction_, true);
        for ( ; it != et; ++it) {
            int bgUeIndex = *it;
            MacNodeId bgUeId = MacNodeId(BGUE_MIN_ID + bgUeIndex);

            unsigned cw = 0;
            unsigned int rtxBytes = scheduleBgRtx(bgUeId, carrierFrequency, cw, bandLim);
            if (rtxBytes > 0)
                bgScheduledRtx[bgUeId] = rtxBytes;
            EV << NOW << "LteSchedulerEnb::rtxscheduleBackground BG UE " << bgUeId << " - allocated bytes : " << rtxBytes << endl;
        }

        // consume bytes
        for (auto& [bgUeId, scheduledBytes] : bgScheduledRtx)
            bgTrafficManager->consumeBackloggedUeBytes(bgUeId, scheduledBytes, direction_, true); // in bytes

        int availableBlocks = allocator_->computeTotalRbs();

        EV << NOW << " LteSchedulerEnb::rtxscheduleBackground residual OFDM Space: " << availableBlocks << endl;

        EV << NOW << " LteSchedulerEnb::rtxscheduleBackground --------------------::[  END RTX-SCHEDULE-BACKGROUND ]::--------------------" << endl;

        return availableBlocks == 0;
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSchedulerEnb::rtxscheduleBackground(): %s", e.what());
    }
    return false;
}

unsigned int LteSchedulerEnb::scheduleBgRtx(MacNodeId bgUeId, GHz carrierFrequency, Codeword cw, std::vector<BandLimit> *bandLim, Remote antenna, bool limitBl)
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
            makeUniformBandLimits(tempBandLim, mac_->getCellInfo()->getNumBands(), -2);
            bandLim = &tempBandLim;
        }

        EV << NOW << "LteSchedulerEnb::scheduleBgRtx - Node[" << mac_->getMacNodeId() << ", User[" << bgUeId << "]" << endl;

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

            EV << NOW << " LteSchedulerEnb::scheduleBgRtx BAND " << b << endl;
            EV << NOW << " LteSchedulerEnb::scheduleBgRtx total bytes:" << queueLength << " still to serve: " << toServe << " bytes" << endl;
            EV << NOW << " LteSchedulerEnb::scheduleBgRtx Available: " << bandAvailableBytes << " bytes" << endl;

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
            EV << NOW << " LteSchedulerEnb::scheduleBgRtx Unavailable space for serving node " << bgUeId << endl;
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
                    EV << NOW << " LteSchedulerEnb::scheduleBgRtx - adding " << assignedBlocks.at(i) << " to band " << i << endl;
                    allocator_->addBlocks(antenna, b, bgUeId, assignedBlocks.at(i), assignedBytes.at(i));
                }
            }

            // signal a retransmission

            // mark codeword as used
            if (allocatedCws_.find(bgUeId) != allocatedCws_.end())
                allocatedCws_.at(bgUeId)++;
            else
                allocatedCws_[bgUeId] = 1;

            EV << NOW << " LteSchedulerEnb::scheduleBgRtx: " << allocatedBytes << " bytes served! " << endl;

            // update rb map
            allocatedRbMap[antenna] = allocatedRbMapEntry;

            // uplink scheduler only: if uplink interference is enabled, mark the
            // occupation in the ul transmission map (for ul interference computation purposes)
            if (direction_ == UL) {
                ChannelModelBase *channelModel = mac_->getPhy()->getChannelModel(carrierFrequency);
                if (channelModel->isUplinkInterferenceEnabled())
                    binder_->storeUlTransmissionMap(carrierFrequency, antenna, allocatedRbMap, bgUeId, mac_->getMacCellId(), bgTrafficManager->getTrafficGenerator(bgUeId), UL);
            }

            return allocatedBytes;
        }
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSchedulerEnb::scheduleBgRtx(): %s", e.what());
    }
    return 0;
}

} //namespace
