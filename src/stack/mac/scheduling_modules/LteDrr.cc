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

#include "stack/mac/scheduling_modules/LteDrr.h"
#include "stack/mac/scheduler/LteSchedulerEnb.h"

namespace simu5g {

using namespace omnetpp;

void LteDrr::prepareSchedule()
{
    activeTempList_ = activeList_;
    drrTempMap_ = drrMap_;

    bool terminateFlag = false, activeFlag = true, eligibleFlag = true;
    unsigned int eligible = activeTempList_.size();
    // Loop until the active list is not empty and there is spare room.
    while (!activeTempList_.empty() && eligible > 0) {
        // Get the current CID.
        MacCid cid = activeTempList_.current();

        MacNodeId nodeId = MacCidToNodeId(cid);

        // Check if node is still a valid node in the simulation - might have been dynamically removed.
        if (binder_->getOmnetId(nodeId) == 0) {
            activeTempList_.erase();          // Remove from the active list.
            activeConnectionTempSet_.erase(cid);
            carrierActiveConnectionSet_.erase(cid);
            EV << "CID " << cid << " of node " << nodeId << " removed from active connection set - no OmnetId in Binder known.";
            continue;
        }

        // Get the current DRR descriptor.
        DrrDesc& desc = drrTempMap_[cid];

        // Check for connection eligibility. If not, skip it.
        if (!desc.eligible_) {
            activeTempList_.move();
            eligible--;
            continue;
        }

        // Update the deficit counter.
        if (desc.addQuantum_) {
            desc.deficit_ += desc.quantum_;
            desc.addQuantum_ = false;
        }

        // Clear the flags passed to the grant() function.
        terminateFlag = false;
        activeFlag = true;
        eligibleFlag = true;
        // Try to schedule as many PDUs as possible (or fragments thereof).
        unsigned int scheduled = requestGrant(cid, desc.deficit_, terminateFlag, activeFlag, eligibleFlag);

        if (desc.deficit_ < scheduled)
            throw cRuntimeError("LteDrr::execSchedule CID:%d unexpected deficit value of %d [scheduled=%d]", cid, desc.deficit_, scheduled);

        // Update the deficit counter.
        desc.deficit_ -= scheduled;

        // Update the number of eligible connections.
        if (!eligibleFlag || !activeFlag) {
            eligible--;              // Decrement the number of eligible connections.
            desc.eligible_ = false;  // This queue is not eligible for service.
        }

        // Remove the queue if it has become inactive.
        if (!activeFlag) {
            activeTempList_.erase();          // Remove from the active list.
            activeConnectionTempSet_.erase(cid);
            carrierActiveConnectionSet_.erase(cid);
            desc.deficit_ = 0;       // Reset the deficit to zero.
            desc.active_ = false;   // Set this descriptor as inactive.
            desc.addQuantum_ = true;

            // If scheduling is going to stop and the current queue has not
            // been served entirely, then the RR pointer should NOT move to
            // the next element of the active list. Instead, the deficit
            // is decremented by a quantum, which will be added at
            // the beginning of the next round. Note that this step is only
            // performed if the deficit counter is greater than a quantum so
            // as not to give the queue more bandwidth than its fair share.
        }
        else if (terminateFlag && desc.deficit_ >= desc.quantum_) {
            desc.deficit_ -= desc.quantum_;

            // Otherwise, move the round-robin pointer to the next element.
        }
        else if (desc.deficit_ == 0) {
            desc.addQuantum_ = true;
            activeTempList_.move();
        }
        // else
        //     this connection still has to consume its deficit (e.g., because space has ended)
        //     so the pointer is not moved

        // Terminate scheduling, if the grant function specified so.
        if (terminateFlag)
            break;
    }
}

void LteDrr::commitSchedule()
{
    activeList_ = activeTempList_;
    *activeConnectionSet_ = activeConnectionTempSet_;
    drrMap_ = drrTempMap_;
}

void LteDrr::updateSchedulingInfo()
{
    // Get connections.
    LteMacBufferMap *conn;

    if (direction_ == DL) {
        conn = eNbScheduler_->mac_->getMacBuffers();
    }
    else if (direction_ == UL) {
        conn = eNbScheduler_->mac_->getBsrVirtualBuffers();
    }
    else {
        conn = nullptr;
        throw cRuntimeError("LteDrr::updateSchedulingInfo invalid direction");
    }

    // Select the minimum rate and MAC SDU size.
    double minSize = 0;
    double minRate = 0;
    for (auto& it : *conn) {
        MacCid cid = it.first;
        MacNodeId nodeId = MacCidToNodeId(cid);
        bool eligible = true;
        const UserTxParams& info = eNbScheduler_->mac_->getAmc()->computeTxParams(nodeId, direction_, carrierFrequency_);
        unsigned int codeword = info.getLayers().size();
        if (eNbScheduler_->allocatedCws(nodeId) == codeword)
            eligible = false;

        for (unsigned int i = 0; i < codeword; i++) {
            if (info.readCqiVector()[i] == 0)
                eligible = false;
        }
        if (minRate == 0 /* || pars.minReservedRate_ < minRate*/)
//                TODO add connections parameters and fix this value
            minRate = 500;
        if (minSize == 0 /*|| pars.maxBurst_ < minSize */)
            minSize = 160; // pars.maxBurst_;

        // Compute the quanta. If descriptors do not exist they are created.
        // The values of the other fields, e.g., active status, are not changed.

        drrMap_[cid].quantum_ = (unsigned int)(ceil((/*pars.minReservedRate_*/ 500 / minRate) * minSize));
        drrMap_[cid].eligible_ = eligible;
    }
}

void LteDrr::notifyActiveConnection(MacCid cid)
{
    EV << NOW << "LteDrr::notify CID: " << cid << endl;
    // This is a mirror structure of the active list, used by all the modules that want to know the list of active users.

    bool alreadyIn = false;
    activeList_.find(cid, alreadyIn);
    if (!alreadyIn) {
        activeList_.insert(cid);
        drrMap_[cid].active_ = true;
    }

    drrMap_[cid].eligible_ = true;

    EV << NOW << "LteSchedulerEnb::notifyDrr active: " << drrMap_[cid].active_ << endl;
}

} //namespace

