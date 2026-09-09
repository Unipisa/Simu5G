//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/d2d/mac/amc/D2dAmcHelper.h"
#include "simu5g/stack/d2d/binder/D2dBinder.h"

namespace simu5g {

using namespace omnetpp;

void D2dAmcHelper::initD2D()
{
    // Get deployed UEs from the binder (the same set the base uses for DL/UL)
    for (MacNodeId ueId : amc_->getBinder()->getDeployedUes(amc_->getMacNodeId()))
        d2dConnectedUe_[ueId] = true;

    // Read D2D-specific parameters (reusing the UL MCS scale, as before)
    mcsScaleD2D_ = amc_->getCellInfo()->getMcsScaleUl();
    fbhbCapacityD2D_ = amc_->getParentModule()->par("fbhbCapacityD2D");
    lb_ = amc_->getParentModule()->par("summaryLowerBound");
    ub_ = amc_->getParentModule()->par("summaryUpperBound");

    // D2D
    EV << "D2D CONNECTED: " << d2dConnectedUe_.size() << endl;

    for (auto [nodeId, flag] : d2dConnectedUe_) { // For all UEs (D2D)
        d2dNodeIndex_[nodeId] = d2dRevNodeIndex_.size();
        d2dRevNodeIndex_.push_back(nodeId);
    }

    WATCH(mcsScaleD2D_);
    WATCH(fbhbCapacityD2D_);
    WATCH_MAP(d2dConnectedUe_);
    WATCH_MAP(d2dNodeIndex_);
    WATCH_VECTOR(d2dRevNodeIndex_);
}

void D2dAmcHelper::pushFeedbackD2D(MacNodeId id, LteFeedback fb, MacNodeId peerId, GHz carrierFrequency)
{
    EV << "Feedback from MacNodeId " << id << " (direction D2D), peerId = " << peerId << endl;

    std::map<MacNodeId, History_> *history = &d2dFeedbackHistory_[carrierFrequency];
    std::map<MacNodeId, unsigned int> *nodeIndex = &d2dNodeIndex_;

    // Put the feedback in the FBHB
    Remote antenna = fb.getAntennaId();
    TxMode txMode = fb.getTxMode();
    int index = (*nodeIndex).at(id);

    EV << "ID: " << id << endl;
    EV << "index: " << index << endl;

    if (history->find(peerId) == history->end()) {
        // initialize new history for this peer UE
        History_ newHist;

        ConnectedUesMap::const_iterator it, et;
        it = d2dConnectedUe_.begin();
        et = d2dConnectedUe_.end();
        for ( ; it != et; it++) { // For all UEs (D2D)
            newHist[antenna].push_back(std::vector<LteSummaryBuffer>(UL_NUM_TXMODE, LteSummaryBuffer(fbhbCapacityD2D_, MAXCW, amc_->getSystemNumBands(), lb_, ub_)));
        }
        (*history)[peerId] = newHist;
    }
    (*history)[peerId][antenna].at(index).at(txMode).put(fb);

    // delete the old UserTxParam for this <UE_dir_carrierFreq>, so that it will be recomputed next time it's needed
    if (d2dTxParams_.find(carrierFrequency) != d2dTxParams_.end() && d2dTxParams_.at(carrierFrequency).at(index).isValid())
        d2dTxParams_[carrierFrequency].at(index).restoreDefaultValues();

    // DEBUG
    EV << "PeerId: " << peerId << ", Antenna: " << dasToA(antenna) << ", TxMode: " << txMode << ", Index: " << index << endl;
    EV << "RECEIVED" << endl;
    fb.print(NODEID_NONE, id, D2D, "LteAmc::pushFeedbackD2D");
}

const LteSummaryFeedback& D2dAmcHelper::getFeedbackD2D(MacNodeId id, Remote antenna, TxMode txMode, MacNodeId peerId, GHz carrierFrequency)
{
    MacNodeId nh = amc_->getServingNodeOrSelf(id);

    if (id != nh)
        EV << NOW << " LteAmc::getFeedbackD2D detected " << nh << " as next hop for " << id << "\n";
    id = nh;

    // The per-carrier history is created lazily, as LteAmc::getHistory() does for UL/DL:
    // before the first D2D feedback is pushed on this carrier there is no entry at all,
    // and the first mode selection period can come before the first report.
    std::map<MacNodeId, History_>& carrierHistory = d2dFeedbackHistory_[carrierFrequency];

    if (peerId == NODEID_NONE) {
        // we return the first feedback stored in the structure
        for (const auto& [histNodeId, history] : carrierHistory) {
            if (histNodeId == NODEID_NONE) // skip fake UE 0
                continue;

            // Skip peers that have left the simulation. purgeDepartedNode() drops them at
            // unregistration, but pushFeedbackD2D() re-creates the key for any feedback that
            // still names them, so the map cannot be assumed clean here -- and
            // getD2DCapability() asserts that its arguments are registered UEs.
            if (!amc_->getBinder()->nodeExists(histNodeId))
                continue;

            if (d2dBinder_ == nullptr)
                d2dBinder_ = D2dBinder::getInstance(amc_->getBinder());
            if (d2dBinder_->getD2DCapability(id, histNodeId)) {
                peerId = histNodeId;
                break;
            }
        }

        // default feedback: when there is no feedback from peers yet (NOSIGNALCQI)
        if (peerId == NODEID_NONE)
            return noFeedbackD2D(carrierHistory, txMode);
    }

    auto peerIt = carrierHistory.find(peerId);
    if (peerIt == carrierHistory.end() || peerIt->second.count(antenna) == 0)
        throw cRuntimeError("D2dAmcHelper::getFeedbackD2D(): no D2D feedback history for peer %hu (antenna %s) on carrier %g GHz", num(peerId), dasToA(antenna).c_str(), carrierFrequency.get());
    auto indexIt = d2dNodeIndex_.find(id);
    if (indexIt == d2dNodeIndex_.end())
        throw cRuntimeError("D2dAmcHelper::getFeedbackD2D(): node %hu is not attached to this D2D AMC", num(id));
    return peerIt->second.at(antenna).at(indexIt->second).at(txMode).get();
}

const LteSummaryFeedback& D2dAmcHelper::noFeedbackD2D(std::map<MacNodeId, History_>& carrierHistory, TxMode txMode)
{
    // The fake UE 0 entry holds one never-filled summary buffer, whose get() reports
    // NOSIGNALCQI. attachUserD2D() also creates it (with the rest of the peer histories);
    // here it is created on demand for a carrier that has seen neither an attach nor a
    // feedback yet.
    std::vector<std::vector<LteSummaryBuffer>>& noneHistory = carrierHistory[NODEID_NONE][MACRO];
    if (noneHistory.empty())
        noneHistory.push_back(std::vector<LteSummaryBuffer>(UL_NUM_TXMODE, LteSummaryBuffer(fbhbCapacityD2D_, MAXCW, amc_->getSystemNumBands(), lb_, ub_)));
    return noneHistory.at(0).at(txMode).get();
}

bool D2dAmcHelper::existTxParamsD2D(MacNodeId id, GHz carrierFrequency)
{
    MacNodeId nh = amc_->getServingNodeOrSelf(id);
    if (id != nh)
        EV << NOW << " LteAmc::existTxParams detected " << nh << " as next hop for " << id << "\n";
    id = nh;

    if (d2dTxParams_.find(carrierFrequency) == d2dTxParams_.end())
        return false;

    return d2dTxParams_[carrierFrequency].at(d2dNodeIndex_.at(id)).isValid();
}

const UserTxParams& D2dAmcHelper::setTxParamsD2D(MacNodeId id, UserTxParams& info, GHz carrierFrequency)
{
    MacNodeId nh = amc_->getServingNodeOrSelf(id);
    if (id != nh)
        EV << NOW << " LteAmc::setTxParams detected " << nh << " as next hop for " << id << "\n";
    id = nh;

    info.setValid(true);

    /**
     * NOTE: if the antenna set has not been explicitly written in UserTxParams
     * by the AMC pilot, this antenna set contains only MACRO
     * (this is done by setting MACRO in UserTxParams constructor)
     */

    // DEBUG
    EV << NOW << " LteAmc::setTxParams DAS antenna set for user " << id << " is \t";
    for (auto it : info.readAntennaSet()) {
        EV << "[" << dasToA(it) << "]\t";
    }
    EV << endl;

    if (d2dTxParams_.find(carrierFrequency) == d2dTxParams_.end()) {
        // Initialize user transmission parameters structures
        std::vector<UserTxParams> tmp;
        tmp.resize(d2dConnectedUe_.size(), UserTxParams());
        d2dTxParams_[carrierFrequency] = tmp;
    }
    return d2dTxParams_[carrierFrequency].at(d2dNodeIndex_.at(id)) = info;
}

const UserTxParams& D2dAmcHelper::getTxParamsD2D(MacNodeId id, GHz carrierFrequency)
{
    MacNodeId nh = amc_->getServingNodeOrSelf(id);
    if (id != nh)
        EV << NOW << " LteAmc::getTxParams detected " << nh << " as next hop for " << id << "\n";
    id = nh;

    return d2dTxParams_[carrierFrequency].at(d2dNodeIndex_.at(id));
}

void D2dAmcHelper::printTxParamsD2D(GHz carrierFrequency)
{
    EV << "######################" << endl;
    EV << "# UserTxParams vector (" << dirToA(D2D) << ")" << endl;
    EV << "######################" << endl;

    std::vector<UserTxParams> *userInfo = &d2dTxParams_[carrierFrequency];
    std::vector<MacNodeId> *revIndex = &d2dRevNodeIndex_;

    for (int index = 0; index < userInfo->size(); index++) {
        EV << "Ue index: " << index << ", MacNodeId: " << (*revIndex)[index] << endl;
        userInfo->at(index).print("info");
    }
}

void D2dAmcHelper::rescaleD2D(double rePerRb)
{
    // D2D has no separate MCS table to rescale (the former write-only d2dMcsTable_
    // was removed as dead state); kept as the D2D branch of the rescaleMcsForDirection seam.
}

void D2dAmcHelper::purgeDepartedNode(MacNodeId nodeId)
{
    // Drop the node as a *peer* key of the D2D feedback history, on every carrier. This is
    // the only structure a never-attached node can appear in, and it is keyed by node id, so
    // erasing is safe. The index-keyed structures (d2dNodeIndex_ and the vectors it indexes)
    // are deliberately left alone: erasing an entry from the middle would shift every other
    // node's index. Those are the attached-user structures, which detachUserD2D() maintains.
    for (auto& [carrierFrequency, carrierHistory] : d2dFeedbackHistory_)
        carrierHistory.erase(nodeId);
}

void D2dAmcHelper::detachUserD2D(MacNodeId nodeId)
{
    EV << "##################################" << endl;
    EV << "# LteAmc::detachUser. Id: " << nodeId << ", direction: " << dirToA(D2D) << endl;
    EV << "##################################" << endl;
    try {
        unsigned int nodeIndex = d2dNodeIndex_.at(nodeId);

        // UE is no longer connected
        d2dConnectedUe_.at(nodeId) = false;

        // clear feedback data from history
        for (auto& [carrierFreq, carrierHist] : d2dFeedbackHistory_) {
            for (auto& [peerId, peerHist] : carrierHist) {
                if (peerId == NODEID_NONE)                                          // skip fake UE 0
                    continue;

                for (auto remote : *amc_->getAntennaSet()) {
                    peerHist.at(remote).at(nodeIndex).clear();
                }
            }
        }

        // clear user transmission parameters for this UE
        for (auto& [carrierFreq, txParams] : d2dTxParams_) {
            txParams.at(nodeIndex).restoreDefaultValues();
        }
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteAmc::detachUser(): %s", e.what());
    }
}

void D2dAmcHelper::attachUserD2D(MacNodeId nodeId)
{
    EV << "##################################" << endl;
    EV << "# LteAmc::attachUser. Id: " << nodeId << ", direction: " << dirToA(D2D) << endl;
    EV << "##################################" << endl;

    unsigned int nodeIndex;
    unsigned int fbhbCapacity = fbhbCapacityD2D_;
    unsigned int numTxModes = UL_NUM_TXMODE;

    // Prepare iterators and empty feedback data
    LteSummaryBuffer b = LteSummaryBuffer(fbhbCapacity, MAXCW, amc_->getSystemNumBands(), lb_, ub_);
    std::vector<LteSummaryBuffer> v = std::vector<LteSummaryBuffer>(numTxModes, b);

    // check if the UE is known (it has been here before)
    if (d2dConnectedUe_.find(nodeId) != d2dConnectedUe_.end()) {
        EV << "LteAmc::attachUser. Id " << nodeId << " is known (he has been here before)." << endl;

        // user is known, get his index
        nodeIndex = d2dNodeIndex_.at(nodeId);

        // clear user transmission parameters for this UE
        for (auto& item : d2dTxParams_) {
            item.second.at(nodeIndex).restoreDefaultValues();
        }

        // initialize empty feedback structures
        for (auto& hit : d2dFeedbackHistory_) {
            for (auto& ht : hit.second) {
                if (ht.first == NODEID_NONE)                                          // skip fake UE 0
                    continue;

                for (auto remote : *amc_->getAntennaSet()) {
                    (ht.second)[remote].at(nodeIndex) = v;
                }
            }
        }
    }
    else {
        EV << "LteAmc::attachUser. Id " << nodeId << " is not known (it is the first time we see him)." << endl;

        // new user: [] operator insert a new element in the map
        d2dNodeIndex_[nodeId] = d2dRevNodeIndex_.size();
        d2dRevNodeIndex_.push_back(nodeId);

        for (auto& item : d2dTxParams_) {
            item.second.push_back(UserTxParams());
        }

        // get newly created index
        nodeIndex = d2dNodeIndex_.at(nodeId);

        // initialize empty feedback structures
        // initialize an empty feedback for a fake user (id 0), in order to manage
        // the case of transmission before a feedback has been reported
        for (auto& [key, hist] : d2dFeedbackHistory_) {
            hist[NODEID_NONE] = History_();
            for (auto& [key2, peerHistory] : hist) {
                for (auto remote : *amc_->getAntennaSet()) {
                    peerHistory[remote].push_back(v); // XXX DEBUG THIS!!
                }
            }
        }
    }
    // Operation done in any case: use [] because new elements may be created
    d2dConnectedUe_[nodeId] = true;
}

void D2dAmcHelper::testUeD2D(MacNodeId nodeId)
{
    EV << "##################################" << endl;
    EV << "LteAmc::testUe (" << dirToA(D2D) << ")" << endl;

    int numTxModes = UL_NUM_TXMODE;

    unsigned int nodeIndex = d2dNodeIndex_.at(nodeId);
    bool isConnected = d2dConnectedUe_.at(nodeId);
    MacNodeId revIndex = d2dRevNodeIndex_.at(nodeIndex);

    EV << "Id: " << nodeId << endl;
    EV << "Index: " << nodeIndex << endl;
    EV << "Reverse index: " << revIndex << " (should be the same as ID)" << endl;
    EV << "Is connected: " << (isConnected ? "TRUE" : "FALSE") << endl;

    if (!isConnected)
        return;

    // If connected compute and print user transmission parameters and history
    for (const auto& [key, value] : d2dTxParams_) {
        UserTxParams info = value.at(nodeIndex);
        EV << "UserTxParams - carrier[" << key << "]" << endl;
        info.print("LteAmc::testUe");
    }

    for (const auto& hit : d2dFeedbackHistory_) {
        for (const auto& ht : hit.second) {
            const History_& peerHistory = ht.second;
            std::vector<LteSummaryBuffer> feedback;

            EV << "History" << endl;
            for (auto remote : *amc_->getAntennaSet()) {
                EV << "Remote: " << dasToA(remote) << endl;
                feedback = peerHistory.at(remote).at(nodeIndex);
                for (int i = 0; i < numTxModes; i++) {
                    // Print only non-empty feedback summary! (all cqi are != NOSIGNALCQI)
                    Cqi testCqi = (feedback.at(i).get()).getCqi(Codeword(0), Band(0));
                    if (testCqi == NOSIGNALCQI)
                        continue;

                    feedback.at(i).get().print(NODEID_NONE, nodeId, D2D, TxMode(i), "LteAmc::testUe");
                }
            }
        }
    }
    EV << "##################################" << endl;
}

} //namespace
