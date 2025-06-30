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

#include "stack/mac/scheduling_modules/LteAllocatorBestFit.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/mac/buffer/LteMacBuffer.h"
#include "stack/mac/conflict_graph/ConflictGraph.h"

namespace simu5g {

using namespace omnetpp;

LteAllocatorBestFit::LteAllocatorBestFit(Binder *binder) : LteScheduler(binder)
{
}

void LteAllocatorBestFit::checkHole(Candidate& candidate, Band holeIndex, unsigned int holeLen, unsigned int req)
{
    if (holeLen > req) {
        // this hole would satisfy the requested load
        if (!candidate.greater) {
            // The current candidate would not satisfy the requested load.
            // The hole is a better candidate. Update
            candidate.greater = true;
            candidate.index = holeIndex;
            candidate.len = holeLen;
        }
        else { // The current candidate would satisfy the requested load.
               // compare the length of the hole with that of the best candidate
            if (candidate.len > holeLen) {
                // the hole is a better candidate. Update
                candidate.greater = true;
                candidate.index = holeIndex;
                candidate.len = holeLen;
            }
        }
    }
    else {
        // this hole would NOT satisfy the requested load
        if (!candidate.greater) {
            // The current candidate would not satisfy the requested load.
            // compare the length of the hole with that of the best candidate
            if (candidate.len < holeLen) {
                // the hole is a better candidate. Update
                candidate.greater = false;
                candidate.index = holeIndex;
                candidate.len = holeLen;
            }
        }
        // else do not update. The current candidate would satisfy the requested load.
    }
}

bool LteAllocatorBestFit::checkConflict(const CGMatrix *cgMatrix, MacNodeId nodeIdA, MacNodeId nodeIdB)
{
    bool conflict = false;

    for (const auto &[key, value] : *cgMatrix) {
        if (key.srcId != nodeIdA)
            continue;

        for (const auto &[confKey, confValue] : value) {
            if (confKey.srcId != nodeIdB)
                continue;

            if (confValue) {
                conflict = true;
                break;
            }
        }

        if (conflict)
            break;
    }

    return conflict;
}

void LteAllocatorBestFit::prepareSchedule()
{
    EV << NOW << " LteAllocatorBestFit::schedule " << eNbScheduler_->mac_->getMacNodeId() << endl;

    if (conflictGraph_ == nullptr)
        conflictGraph_ = mac_->getConflictGraph();

    // Initialize SchedulerAllocation structures
    initAndReset();

    bool reuseD2D = mac_->isReuseD2DEnabled();
    bool reuseD2DMulti = mac_->isReuseD2DMultiEnabled();

    const CGMatrix *cgMatrix = nullptr;
    if (reuseD2D || reuseD2DMulti) {
        if (conflictGraph_ == nullptr)
            throw cRuntimeError("LteAllocatorBestFit::prepareSchedule - conflictGraph is a NULL pointer");
        cgMatrix = conflictGraph_->getConflictMatrix();
    }

    // Get the bands occupied by RAC and RTX
    std::set<Band> alreadyAllocatedBands = eNbScheduler_->getOccupiedBands();
    unsigned int firstUnallocatedBand;
    if (!alreadyAllocatedBands.empty()) {
        // Get the last band in the Set (a set is ordered so the last band is the latest occupied)
        firstUnallocatedBand = *alreadyAllocatedBands.rbegin();
    }
    else
        firstUnallocatedBand = 0;

    // Start the allocation of IM flows from the end of the frame
    int firstUnallocatedBandIM = eNbScheduler_->getResourceBlocks() - 1;

    // Get the active connection Set
    activeConnectionTempSet_ = *activeConnectionSet_;

    // Resume a MaxCi scoreList build mode
    // Build the score list by cycling through the active connections.
    ScoreList score;
    MacCid cid = 0;
    unsigned int blocks = 0;
    unsigned int byPs = 0;

    // Set for deleting inactive connections
    ActiveSet inactive_connections;

    for (unsigned int it1 : carrierActiveConnectionSet_) {
        // Current connection.
        cid = it1;

        MacNodeId nodeId = MacCidToNodeId(cid);

        // Get virtual buffer reference
        LteMacBuffer *conn = eNbScheduler_->bsrbuf_->at(cid);
        // Check whether the virtual buffer is empty
        if (conn->isEmpty()) {
            // The BSR buffer for this node is empty. Abort scheduling for the node: no data to transmit.
            EV << "LteAllocatorBestFit:: scheduled connection is no longer active. Exiting grant " << endl;
            // The BSR buffer for this node is empty so the connection is no longer active.
            inactive_connections.insert(cid);
            continue;
        }

        // Set the right direction for nodeId
        Direction dir;
        if (direction_ == UL)
            dir = (MacCidToLcid(cid) == D2D_SHORT_BSR) ? D2D : (MacCidToLcid(cid) == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : direction_;
        else
            dir = DL;

        // Compute available blocks for the current user
        const UserTxParams& info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId, dir, carrierFrequency_);
        const std::set<Band>& bands = info.readBands();
        unsigned int codeword = info.getLayers().size();
        bool cqiNull = false;
        for (unsigned int i = 0; i < codeword; i++) {
            if (info.readCqiVector()[i] == 0)
                cqiNull = true;
        }
        if (cqiNull) {
            EV << NOW << "Cqi null, direction: " << dirToA(dir) << endl;
            continue;
        }
        // No more free cw
        if (eNbScheduler_->allocatedCws(nodeId) == codeword) {
            EV << NOW << "Allocated codeword, direction: " << dirToA(dir) << endl;
            continue;
        }

        // Compute score based on total available bytes
        unsigned int availableBlocks = 0;
        unsigned int availableBytes = 0;

        // For each antenna
        for (const Remote& antenna : info.readAntennaSet()) {
            // For each logical band
            for (const Band& band : bands) {
                unsigned int blocks = eNbScheduler_->readAvailableRbs(nodeId, antenna, band);
                availableBlocks += blocks;
                availableBytes += eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId, band, blocks, dir, carrierFrequency_);
            }
        }

        blocks = availableBlocks;
        // current user bytes per slot
        byPs = (blocks > 0) ? (availableBytes / blocks) : 0;

        // Create a new score descriptor for the connection, where the score is equal to the ratio between bytes per slot and long term rate
        ScoreDesc desc(cid, byPs);
        // Insert the cid score in the right list
        score.push(desc);

        EV << NOW << " LteAllocatorBestFit::schedule computed for cid " << cid << " score of " << desc.score_ << endl;
    }

    //Delete inactive connections
    for (unsigned int inactive_connection : inactive_connections) {
        carrierActiveConnectionSet_.erase(inactive_connection);
        activeConnectionTempSet_.erase(inactive_connection);
    }

    // Schedule the connections in score order.
    while (!score.empty()) {
        // Pop the top connection from the list.
        ScoreDesc current = score.top();

        // Get the CID
        MacCid cid = current.x_;

        // get the node Id
        MacNodeId nodeId = MacCidToNodeId(cid);

        //Set the right direction for nodeId
        Direction dir;
        if (direction_ == UL)
            dir = (MacCidToLcid(cid) == D2D_SHORT_BSR) ? D2D : (MacCidToLcid(cid) == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : direction_;
        else
            dir = DL;

        EV << NOW << " LteAllocatorBestFit::schedule scheduling connection " << cid << " with score of " << current.score_ << endl;

        // Compute Tx params for the extracted node
        const UserTxParams& txParams = mac_->getAmc()->computeTxParams(nodeId, dir, carrierFrequency_);
        // Get virtual buffer reference
        LteMacBuffer *conn = eNbScheduler_->bsrbuf_->at(cid);

        // Get a reference of the first BSR
        PacketInfo vpkt = conn->front();
        // Get the size of the BSR
        unsigned int vQueueFrontSize = vpkt.first + MAC_HEADER;

        // Get the node CQI
        std::vector<Cqi> node_CQI = txParams.readCqiVector();

        // TODO add MIMO support
        Codeword cw = 0;
        unsigned int req_RBs = 0;

        // Calculate the number of Bytes available in a block
        unsigned int req_Bytes1RB = eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId, 0, cw, 1, dir, carrierFrequency_); // The band (here equals to 0) is useless

        // This calculation is for coherence with the allocation done in the ScheduleGrant function
        req_RBs = (vQueueFrontSize + req_Bytes1RB - 1) / req_Bytes1RB;

        // Get the total number of bands
        unsigned int numBands = mac_->getCellInfo()->getNumBands();

        unsigned int blocks = 0;
        // Set the band counter to zero
        int band = 0;
        // Create the set for booked bands
        std::vector<Band> bookedBands;

        // Scan the RBs and find the best candidate "hole" to allocate the UE
        // We need to allocate RBs in the hole with the minimum length, such that the request is satisfied
        // If satisfying the request is not possible, then allocate RBs in the hole with the maximum length
        Candidate candidate;
        candidate.index = 0;
        candidate.len = 0;
        candidate.greater = false;

        // info for the current hole considered
        bool newHole = true;
        Band holeIndex = 0;
        unsigned int holeLen = 0;

        // frequency reuse-enabled connections are allocated starting from the beginning of the subframe,
        // whereas non-reuse-enabled ones are allocated from the end of the subframe

        bool enableFrequencyReuse = (reuseD2D && dir == D2D) || (reuseD2DMulti && dir == D2D_MULTI);
        if (enableFrequencyReuse) { // if frequency reuse is possible for the connection's direction
            EV << NOW << " Connection " << cid << " can exploit frequency reuse, dir[" << dirToA(dir) << "]" << endl;

            // Check if the allocation is possible starting from the first unallocated band
            for ( band = firstUnallocatedBand; band < numBands; band++ ) {
                bool jump_band = false;
                // Jump to the next band if this has already been allocated to this node
                if (alreadyAllocatedBands.find(band) != alreadyAllocatedBands.end())
                    jump_band = true;
                /*
                 * Jump to the next band if:
                 * - the band is occupied by a non-reuse-enabled node.
                 */
                if (bandStatusMap_[band].first == EXCLUSIVE)
                    jump_band = true;

                /*
                 * Jump to the next band if the current band is occupied by a conflicting node
                 * (i.e. there's an edge in the conflict graph)
                 */
                // scan the nodes already allocated on this band
                for (const MacNodeId& allocatedNodeId : bandStatusMap_[band].second) {
                    if (checkConflict(cgMatrix, nodeId, allocatedNodeId)) {
                        jump_band = true;
                        break;
                    }
                }

                if (jump_band) {

                    if (!newHole) {
                        // found a hole <holeIndex,holeLen>
                        checkHole(candidate, holeIndex, holeLen, req_RBs);

                        // reset
                        newHole = true;
                        holeIndex = 0;
                        holeLen = 0;
                    }
                    jump_band = false;
                    continue;
                }

                // Going here means that this band can be allocated
                if (newHole) {
                    holeIndex = band;
                    newHole = false;
                }
                holeLen++;
            }
        }
        else {
            EV << NOW << " Connection " << cid << " cannot exploit frequency reuse, dir[" << dirToA(dir) << "]" << endl;

            bool jump_band = false;

            // Check if the allocation is possible starting from the first unallocated band (going back)
            for ( band = firstUnallocatedBandIM; band >= 0; band-- ) {
                // Jump to the next band if this has already been allocated
                if (alreadyAllocatedBands.find(band) != alreadyAllocatedBands.end())
                    jump_band = true;
                /*
                 * Jump to the next band if the band is allocated to another node
                 */
                if (bandStatusMap_[band].first != UNUSED)
                    jump_band = true;

                if (jump_band) {
                    if (!newHole) {
                        // found a hole <holeIndex,holeLen>
                        checkHole(candidate, holeIndex, holeLen, req_RBs);

                        // reset
                        newHole = true;
                        holeIndex = 0;
                        holeLen = 0;
                    }

                    jump_band = false;
                    continue;
                }

                // Going here means that this band can be allocated
                if (newHole) {
                    holeIndex = band;
                    newHole = false;
                }
                holeLen++;
            }
        }

        checkHole(candidate, holeIndex, holeLen, req_RBs);

        if (enableFrequencyReuse) {
            // allocate contiguous RBs in the best candidate
            for ( band = candidate.index; band < candidate.index + candidate.len; band++ ) {
                blocks++;

                EV << NOW << " LteAllocatorBestFit - UE " << nodeId << ": allocated RB " << band << " [" << blocks << "]" << endl;

                // Book the bands that must be allocated
                bookedBands.push_back(band);
                if (blocks == req_RBs)
                    break; // All the blocks and bytes have been allocated
            }
        }
        else {
            // allocate contiguous RBs in the best candidate
            unsigned int end = (candidate.len > candidate.index) ? 0 : candidate.index - candidate.len;
            for ( band = candidate.index; band > end; band-- ) {
                blocks++;

                EV << NOW << " LteAllocatorBestFit - UE " << nodeId << ": allocated RB " << band << " [" << blocks << "]" << endl;

                // Book the bands that must be allocated
                bookedBands.push_back(band);
                if (blocks == req_RBs)
                    break; // All the blocks and bytes have been allocated
            }
        }

        // If the booked request is sufficient to serve entirely or
        // partially a BSR (at least an RB) do the allocation
        if (blocks <= req_RBs && blocks != 0) {
            // Going here means that there's room for allocation (here's the true allocation)
            std::sort(bookedBands.begin(), bookedBands.end());
            // For all the bands previously booked
            for (auto band : bookedBands) {
                // TODO Find a correct way to specify plane and antenna
                allocatedRbsPerBand_[MAIN_PLANE][MACRO][band].ueAllocatedRbsMap_[nodeId] += 1;
                allocatedRbsPerBand_[MAIN_PLANE][MACRO][band].ueAllocatedBytesMap_[nodeId] += req_Bytes1RB;
                allocatedRbsPerBand_[MAIN_PLANE][MACRO][band].allocated_ += 1;
            }

            // Add the nodeId to the scheduleList (needed for Grants)
            std::pair<unsigned int, Codeword> scListId;
            scListId.first = cid;
            scListId.second = 0; // TODO add support for codewords different from 0
            eNbScheduler_->storeScListId(carrierFrequency_, scListId, blocks);

            // If it is a Cell UE we must reserve the bands
            if (!enableFrequencyReuse) {
                setAllocationType(bookedBands, EXCLUSIVE, nodeId);
            }
            else {
                setAllocationType(bookedBands, SHARED, nodeId);
            }

            // Extract the BSR if an entire BSR is served
            if (blocks == req_RBs) {
                // All the bytes have been served
                conn->popFront();
            }
            else {
                conn->front().first -= (blocks * req_Bytes1RB - MAC_HEADER - RLC_HEADER_UM); // Otherwise update the BSR size
            }
        }

        // Extract the node from the right set
        if (!score.empty())
            score.pop();
    }

    eNbScheduler_->storeAllocationEnb(allocatedRbsPerBand_, &alreadyAllocatedBands);
}

void LteAllocatorBestFit::commitSchedule()
{
    *activeConnectionSet_ = activeConnectionTempSet_;
}

void LteAllocatorBestFit::initAndReset()
{
    // Clear and reinitialize the allocatedBlocks structures and set available planes to 1 (just the main OFDMA space)
    allocatedRbsPerBand_.clear();
    allocatedRbsPerBand_.resize(MAIN_PLANE + 1);
    // Set the available antennas of the MAIN plane to 1 (just MACRO antenna)
    allocatedRbsPerBand_.at(MAIN_PLANE).resize(MACRO + 1);

    allocatedRbsUe_.clear();

    // Clear the OFDMA allocated blocks and set available planes to 1 (just the main OFDMA space)
    allocatedRbsMatrix_.clear();
    allocatedRbsMatrix_.resize(MAIN_PLANE + 1);
    // Set the available antennas of the MAIN plane to 1 (just MACRO antenna)
    allocatedRbsMatrix_.at(MAIN_PLANE).resize(MACRO + 1, 0);

    // Clear the perUEbandStatusMap
    perUEbandStatusMap_.clear();

    // Clear the bandStatusMap
    bandStatusMap_.clear();
    int numbands = eNbScheduler_->mac_->getAmc()->getSystemNumBands();
    // Set all bands to non-exclusive
    for (int i = 0; i < numbands; i++) {
        bandStatusMap_[i].first = UNUSED;
    }
}

void LteAllocatorBestFit::setAllocationType(std::vector<Band> bookedBands, AllocationUeType type, MacNodeId nodeId)
{
    for (const auto& band : bookedBands) {
        bandStatusMap_[band].first = type;
        bandStatusMap_[band].second.insert(nodeId);
    }
}

} //namespace

