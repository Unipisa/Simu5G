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

#include "stack/mac/scheduling_modules/LteMaxCi.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"
#include "stack/backgroundTrafficGenerator/IBackgroundTrafficManager.h"

namespace simu5g {

using namespace omnetpp;

void LteMaxCi::prepareSchedule()
{
    EV << NOW << " LteMaxCI::schedule " << eNbScheduler_->mac_->getMacNodeId() << endl;

    activeConnectionTempSet_ = *activeConnectionSet_;

    // Build the score list by cycling through the active connections.
    ScoreList score;
    unsigned int blocks = 0;
    unsigned int byPs = 0;

    for (MacCid cid : carrierActiveConnectionSet_) {
        // Current connection.
        MacNodeId nodeId = MacCidToNodeId(cid);
        OmnetId id = binder_->getOmnetId(nodeId);
        if (nodeId == NODEID_NONE || id == 0) {
            // Node has left the simulation - erase corresponding CIDs
            activeConnectionSet_->erase(cid);
            activeConnectionTempSet_.erase(cid);
            carrierActiveConnectionSet_.erase(cid);
            continue;
        }

        // If we are allocating the UL subframe, this connection may be either UL or D2D
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
        if (cqiNull)
            continue;
        // No more free cw
        if (eNbScheduler_->allocatedCws(nodeId) == codeword)
            continue;

        unsigned int availableBlocks = 0;
        unsigned int availableBytes = 0;
        // For each antenna
        for (const auto& antenna : info.readAntennaSet()) {
            // For each logical band
            for (const auto& band : bands) {
                unsigned int blocks = eNbScheduler_->readAvailableRbs(nodeId, antenna, band);
                availableBlocks += blocks;
                availableBytes += eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId, band, blocks, dir, carrierFrequency_);
            }
        }

        blocks = availableBlocks;
        // Current user bytes per slot
        byPs = (blocks > 0) ? (availableBytes / blocks) : 0;

        // Create a new score descriptor for the connection, where the score is equal to the ratio between bytes per slot and long term rate
        ScoreDesc desc(cid, byPs);
        // Insert the cid score
        score.push(desc);

        EV << NOW << " LteMaxCI::schedule computed for cid " << cid << " score of " << desc.score_ << endl;
    }

    if (direction_ == UL || direction_ == DL) { // D2D background traffic not supported (yet?)
        // Query the BgTrafficManager to get the list of backlogged bg UEs to be added to the scorelist. This work
        // is done by this module itself, so that backgroundTrafficManager is transparent to the scheduling policy in use

        IBackgroundTrafficManager *bgTrafficManager = eNbScheduler_->mac_->getBackgroundTrafficManager(carrierFrequency_);
        for (auto it = bgTrafficManager->getBackloggedUesBegin(direction_); it != bgTrafficManager->getBackloggedUesEnd(direction_); ++it) {
            int bgUeIndex = *it;
            MacNodeId bgUeId = BGUE_MIN_ID + bgUeIndex;

            // The cid for a background UE is a 32-bit integer composed as:
            // - the most significant 16 bits are set to the background UE id (BGUE_MIN_ID+index)
            // - the least significant 16 bits are set to 0 (lcid=0)
            MacCid bgCid = num(bgUeId) << 16;

            int bytesPerBlock = bgTrafficManager->getBackloggedUeBytesPerBlock(bgUeId, direction_);

            ScoreDesc bgDesc(bgCid, bytesPerBlock);
            score.push(bgDesc);
        }
    }

    // Schedule the connections in score order.
    while (!score.empty()) {
        // Pop the top connection from the list.
        ScoreDesc current = score.top();

        bool terminate = false;
        bool active = true;
        bool eligible = true;
        unsigned int granted;

        if (MacCidToNodeId(current.x_) >= BGUE_MIN_ID) {
            EV << NOW << " LteMaxCI::schedule scheduling background UE " << MacCidToNodeId(current.x_) << " with score of " << current.score_ << endl;

            // Grant data to that background connection.
            granted = requestGrantBackground(current.x_, 4294967295U, terminate, active, eligible);

            EV << NOW << "LteMaxCI::schedule granted " << granted << " bytes to background UE " << MacCidToNodeId(current.x_) << endl;
        }
        else {
            EV << NOW << " LteMaxCI::schedule scheduling connection " << current.x_ << " with score of " << current.score_ << endl;

            // Grant data to that connection.
            granted = requestGrant(current.x_, 4294967295U, terminate, active, eligible);

            EV << NOW << "LteMaxCI::schedule granted " << granted << " bytes to connection " << current.x_ << endl;
        }

        // Exit immediately if the terminate flag is set.
        if (terminate) break;

        // Pop the descriptor from the score list if the active or eligible flag are clear.
        if (!active || !eligible) {
            score.pop();
            EV << NOW << "LteMaxCI::schedule connection " << current.x_ << " was found ineligible" << endl;
        }

        // Set the connection as inactive if indicated by the grant.
        if (!active) {
            EV << NOW << "LteMaxCI::schedule scheduling connection " << current.x_ << " set to inactive " << endl;

            if (MacCidToNodeId(current.x_) <= BGUE_MIN_ID) {
                carrierActiveConnectionSet_.erase(current.x_);
                activeConnectionTempSet_.erase(current.x_);
            }
        }
    }
}

void LteMaxCi::commitSchedule()
{
    *activeConnectionSet_ = activeConnectionTempSet_;
}

} //namespace

