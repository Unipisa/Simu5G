#include "simu5g/stack/phy/feedback/LteUlFeedbackGenerator.h"

#include "simu5g/stack/phy/LtePhyEnb.h"
#include "simu5g/stack/phy/feedback/LteFeedbackComputationRealistic.h"

namespace simu5g {

Define_Module(LteUlFeedbackGenerator);

using namespace omnetpp;

void LteUlFeedbackGenerator::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);
        targetBler_ = par("targetBler");
    }
    else if (stage == INITSTAGE_SIMU5G_PHYSICAL_LAYER) {
        phy_.reference(this, "phyModule", true);
        cellInfo_ = phy_->getCellInfo();
        if (cellInfo_ == nullptr)
            throw cRuntimeError("LteUlFeedbackGenerator::initialize - cellInfo is NULL pointer");

        numBands_ = cellInfo_->getPrimaryCarrierNumBands();
        numPreferredBands_ = cellInfo_->getNumPreferredBands();
        initializeFeedbackComputation();
    }
}

void LteUlFeedbackGenerator::initializeFeedbackComputation()
{
    delete lteFeedbackComputation_;
    lteFeedbackComputation_ = new LteFeedbackComputationRealistic(binder_, targetBler_, numBands_);
}

LteUlFeedbackGenerator::~LteUlFeedbackGenerator()
{
    delete lteFeedbackComputation_;
}

LteFeedbackDoubleVector LteUlFeedbackGenerator::computeUlFeedback(const FeedbackRequest& req, const std::vector<double>& snr, MacNodeId sourceNodeId)
{
    Enter_Method("computeUlFeedback()");
    EV_DEBUG << "LteUlFeedbackGenerator::computeUlFeedback - computing UL CSI" << endl;
    std::map<Remote, int> antennaCws;
    antennaCws[MACRO] = 1;

    return lteFeedbackComputation_->computeFeedback(req.type, req.rbAllocationType,
            req.txMode, antennaCws, numPreferredBands_, 0, snr, sourceNodeId);
}

LteFeedbackDoubleVector LteUlFeedbackGenerator::computeD2DFeedback(const FeedbackRequest& req, const std::vector<double>& snr, MacNodeId sourceNodeId)
{
    Enter_Method("computeD2DFeedback()");
    return computeUlFeedback(req, snr, sourceNodeId);
}

} // namespace simu5g
