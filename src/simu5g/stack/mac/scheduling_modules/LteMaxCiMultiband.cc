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

#include <vector>

#include "stack/mac/scheduling_modules/LteMaxCiMultiband.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"

namespace simu5g {

using namespace std;
using namespace omnetpp;

bool debug = false;

void LteMaxCiMultiband::prepareSchedule()
{
    activeConnectionTempSet_ = *activeConnectionSet_;
    unsigned int byPs = 0;
    ScoreList score;

    // compute score based on total available bytes
    unsigned int availableBlocks = 0;
    unsigned int availableBytes = 0;
    unsigned int availableBytes_MB = 0;

    unsigned int totAvailableBlocks = 0;
    unsigned int totAvailableBytes_MB = 0;

    // UsableBands * usableBands;
    if (debug)
        cout << NOW << " LteMaxCiMultiband::prepareSchedule - Total Active Connections:" << activeConnectionTempSet_.size() << endl;
    for (MacCid cid : carrierActiveConnectionSet_) {
        // Current connection.
        MacNodeId nodeId = MacCidToNodeId(cid);
        OmnetId id = binder_->getOmnetId(nodeId);
        if (nodeId == NODEID_NONE || id == 0) {
            // node has left the simulation - erase corresponding CIDs
            activeConnectionSet_->erase(cid);
            activeConnectionTempSet_.erase(cid);
            carrierActiveConnectionSet_.erase(cid);
            continue;
        }
        // obtain a vector of CQI, one for each band
        std::vector<Cqi> vect = eNbScheduler_->mac_->getAmc()->readMultiBandCqi(nodeId, direction_, carrierFrequency_);
        if (debug)
            cout << NOW << " LteMaxCiMultiband::prepareSchedule - per band cqi for UE[" << nodeId << "]" << endl;

        totAvailableBlocks = 0;
        totAvailableBytes_MB = 0;

        // compute the number of bytes that can be fitted into each BAND
        for (int band = 0; band < vect.size(); ++band ) {
            unsigned int blocks = eNbScheduler_->readAvailableRbs(nodeId, MACRO, band);
            availableBlocks += blocks;
            availableBytes_MB = eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs_MB(nodeId, band, blocks, direction_, carrierFrequency_);
            availableBytes = eNbScheduler_->mac_->getAmc()->computeBytesOnNRbs(nodeId, band, blocks, direction_, carrierFrequency_);

            totAvailableBlocks += availableBlocks;
            totAvailableBytes_MB += availableBytes_MB;
            if (debug)
                cout << "\t" << band << ") CQI=" << vect[band] << " - Blocks=" << availableBlocks
                     << " - Bytes_MB=" << availableBytes_MB << " - Bytes=" << availableBytes << endl;
        }

        // current user bytes per slot
        byPs = (totAvailableBlocks > 0) ? (totAvailableBytes_MB / totAvailableBlocks) : 0;

        // Create a new score descriptor for the connection, where the score is equal to the ratio between bytes per slot and long term rate
        ScoreDesc desc(cid, byPs);
        // insert the cid score
        score.push(desc);
        if (debug)
            cout << NOW << " LteMaxCiMultiband::schedule computed for cid " << cid << " score of " << desc.score_ << endl;
    }

    // Schedule the connections in score order.
    while (!score.empty()) {
        // Pop the top connection from the list.
        ScoreDesc current = score.top();

        EV << NOW << " LteMaxCiMultiband::schedule scheduling connection " << current.x_ << " with score of " << current.score_ << endl;

        // Grant data to that connection.
        bool terminate = false;
        bool active = true;
        bool eligible = true;
        unsigned int granted = requestGrant(current.x_, 4294967295U, terminate, active, eligible);

        EV << NOW << "LteMaxCiMultiband::schedule granted " << granted << " bytes to connection " << current.x_ << endl;

        // Exit immediately if the terminate flag is set.
        if (terminate) break;

        // Pop the descriptor from the score list if the active or eligible flag are clear.
        if (!active || !eligible) {
            score.pop();
            EV << NOW << "LteMaxCiMultiband::schedule  connection " << current.x_ << " was found ineligible" << endl;
        }

        // Set the connection as inactive if indicated by the grant ().
        if (!active) {
            EV << NOW << "LteMaxCiMultiband::schedule scheduling connection " << current.x_ << " set to inactive " << endl;
            carrierActiveConnectionSet_.erase(current.x_);
            activeConnectionTempSet_.erase(current.x_);
        }
    }
}

void LteMaxCiMultiband::commitSchedule()
{
    *activeConnectionSet_ = activeConnectionTempSet_;
}

} //namespace

