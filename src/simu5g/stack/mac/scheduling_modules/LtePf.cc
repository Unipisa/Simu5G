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

#include "stack/mac/scheduling_modules/LtePf.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"

namespace simu5g {

using namespace omnetpp;

void LtePf::prepareSchedule()
{
    EV << NOW << "LtePf::execSchedule ############### eNodeB " << eNbScheduler_->mac_->getMacNodeId() << " ###############" << endl;
    EV << NOW << "LtePf::execSchedule Direction: " << ((direction_ == DL) ? " DL " : " UL ") << endl;

    // Clear structures
    grantedBytes_.clear();

    // Create a working copy of the active set
    activeConnectionTempSet_ = *activeConnectionSet_;

    // Build the score list by cycling through the active connections.
    ScoreList score;

    for (const auto& cid : carrierActiveConnectionSet_) {
        MacNodeId nodeId = MacCidToNodeId(cid);
        OmnetId id = binder_->getOmnetId(nodeId);
        grantedBytes_[cid] = 0;

        if (nodeId == NODEID_NONE || id == 0) {
            // node has left the simulation - erase corresponding CIDs
            activeConnectionSet_->erase(cid);
            activeConnectionTempSet_.erase(cid);
            carrierActiveConnectionSet_.erase(cid);
            continue;
        }

        // if we are allocating the UL subframe, this connection may be either UL or D2D
        Direction dir;
        if (direction_ == UL)
            dir = (MacCidToLcid(cid) == D2D_SHORT_BSR) ? D2D : (MacCidToLcid(cid) == D2D_MULTI_SHORT_BSR) ? D2D_MULTI : direction_;
        else
            dir = DL;

        // check if node is still a valid node in the simulation - might have been dynamically removed
        if (binder_->getOmnetId(nodeId) == 0) {
            activeConnectionTempSet_.erase(cid);
            carrierActiveConnectionSet_.erase(cid);
            EV << "CID " << cid << " of node " << nodeId << " removed from active connection set - no OmnetId in Binder known.";
            continue;
        }

        // compute available blocks for the current user
        const UserTxParams& info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId, dir, carrierFrequency_);
        const std::set<Band>& bands = info.readBands();
        unsigned int codeword = info.getLayers().size();
        if (eNbScheduler_->allocatedCws(nodeId) == codeword)
            continue;
        auto it = bands.begin(), et = bands.end();

        bool cqiNull = false;
        for (unsigned int i = 0; i < codeword; i++) {
            if (info.readCqiVector()[i] == 0)
                cqiNull = true;
        }
        if (cqiNull)
            continue;

        // compute score based on total available bytes
        unsigned int availableBlocks = 0;
        unsigned int availableBytes = 0;
        // for each antenna
        for (auto antenna : info.readAntennaSet()) {
            // for each logical band
            //FIXME missing reset `it`??? it = bands.begin();
            for ( ; it != et; ++it) {
                unsigned int blocks = eNbScheduler_->readAvailableRbs(nodeId, antenna, *it);
                availableBlocks += blocks;
                availableBytes += eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId, *it, blocks, dir, carrierFrequency_);
            }
        }

        double s = .0;

        if (pfRate_.find(cid) == pfRate_.end()) pfRate_[cid] = 0;
        if (pfRate_[cid] < scoreEpsilon_) s = 1.0 / scoreEpsilon_;
        else if (availableBlocks > 0) s = ((availableBytes / availableBlocks) / pfRate_[cid]) + uniform(getEnvir()->getRNG(0), -scoreEpsilon_ / 2.0, scoreEpsilon_ / 2.0);
        else s = 0.0;

        // Create a new score descriptor for the connection, where the score is equal to the ratio between bytes per slot and long term rate
        ScoreDesc desc(cid, s);
        score.push(desc);

        EV << NOW << "LtePf::execSchedule CID " << cid << "- Score = " << s << endl;
    }

    // Schedule the connections in score order.
    while (!score.empty()) {
        // Pop the top connection from the list.
        ScoreDesc current = score.top();
        MacCid cid = current.x_; // The CID

        EV << NOW << "LtePf::execSchedule @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << endl;
        EV << NOW << "LtePf::execSchedule CID: " << cid;
        EV << NOW << "LtePf::execSchedule Score: " << current.score_ << endl;

        // Grant data to that connection.
        bool terminate = false;
        bool active = true;
        bool eligible = true;

        unsigned int granted = requestGrant(cid, 4294967295U, terminate, active, eligible);

        grantedBytes_[cid] += granted;

        EV << NOW << "LtePf::execSchedule Granted: " << granted << " bytes" << endl;

        // Exit immediately if the terminate flag is set.
        if (terminate) {
            EV << NOW << "LtePf::execSchedule TERMINATE " << endl;
            break;
        }

        // Pop the descriptor from the score list if the active or eligible flag are clear.
        if (!active || !eligible) {
            score.pop();

            if (!eligible)
                EV << NOW << "LtePf::execSchedule NOT ELIGIBLE " << endl;
        }

        // Set the connection as inactive if indicated by the grant ().
        if (!active) {
            EV << NOW << "LtePf::execSchedule NOT ACTIVE" << endl;
            activeConnectionTempSet_.erase(current.x_);
            carrierActiveConnectionSet_.erase(current.x_);
        }
    }
}

void LtePf::commitSchedule()
{
    unsigned int total = eNbScheduler_->resourceBlocks_;

    for (const auto& [cid, granted] : grantedBytes_) {
        EV << NOW << " LtePf::storeSchedule @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << endl;
        EV << NOW << " LtePf::storeSchedule CID: " << cid << endl;
        EV << NOW << " LtePf::storeSchedule Direction: " << ((direction_ == DL) ? "DL" : "UL") << endl;

        // Computing the short term rate
        double shortTermRate;

        if (total > 0)
            shortTermRate = double(granted) / double(total);
        else
            shortTermRate = 0.0;

        EV << NOW << " LtePf::storeSchedule Short Term Rate " << shortTermRate << endl;

        // Updating the long term rate
        double& longTermRate = pfRate_[cid];
        longTermRate = (1.0 - pfAlpha_) * longTermRate + pfAlpha_ * shortTermRate;

        EV << NOW << "LtePf::storeSchedule Long Term Rate = " << longTermRate;
    }

    *activeConnectionSet_ = activeConnectionTempSet_;
}

} //namespace

