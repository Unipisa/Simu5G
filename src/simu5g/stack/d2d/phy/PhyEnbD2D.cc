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

#include "simu5g/stack/d2d/phy/PhyEnbD2D.h"
#include "simu5g/stack/phy/packet/LteFeedbackPkt.h"
#include "simu5g/stack/d2d/binder/D2dBinder.h"
#include "simu5g/stack/d2d/phy/channelmodel/ID2dChannelModel.h"

namespace simu5g {

Define_Module(PhyEnbD2D);

using namespace omnetpp;
using namespace inet;

void PhyEnbD2D::initialize(int stage)
{
    PhyEnb::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        enableD2DCqiReporting_ = par("enableD2DCqiReporting");
        d2dBinder_.reference(this, "d2dBinderModule", true);
    }
}

void PhyEnbD2D::appendExtraFeedback(inet::Ptr<LteFeedbackPkt>& header, UserControlInfo *lteinfo, LteAirFrame *frame, ChannelModelBase *channelModel)
{
    if (!enableD2DCqiReporting_)
        return;

    // recompute the feedback-computation context from the request
    FeedbackRequest req = lteinfo->getFeedbackReq();
    TxMode txmode = req.txMode;
    FeedbackType type = req.type;
    RbAllocationType rbtype = req.rbAllocationType;
    std::map<Remote, int> antennaCws; // DAS functionality removed
    antennaCws[MACRO] = 1; // Default single antenna
    unsigned int numPreferredBand = cellInfo_->getNumPreferredBands();
    int nRus = 0;

    // Compute D2D feedback for all possible peering UEs
    for (const auto& ueInfo : binder_->getUeList()) {
        MacNodeId peerId = ueInfo->id;
        if (peerId != lteinfo->getSourceId() && d2dBinder_->getD2DCapability(lteinfo->getSourceId(), peerId) && binder_->getServingNodeOrSelf(peerId) == nodeId_) {
            // The source UE might communicate with this peer using D2D, so compute feedback (only in-cell D2D)

            // Retrieve the position of the peer
            Coord peerCoord = ueInfo->phy->getCoord();

            // Get SINR for this link
            std::vector<double> snr = check_and_cast<ID2dChannelModel *>(channelModel)->getSINR_D2D(frame, lteinfo, peerId, peerCoord, nodeId_);

            // Compute the feedback for this link
            LteFeedbackDoubleVector fb = lteFeedbackComputation_->computeFeedback(type, rbtype, txmode,
                    antennaCws, numPreferredBand, nRus, snr,
                    lteinfo->getSourceId());

            header->setLteFeedbackDoubleVectorD2D(peerId, fb);
        }
    }
}

} //namespace
