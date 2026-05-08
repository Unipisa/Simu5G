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
        fbType_ = getFeedbackType(par("feedbackType").stringValue());
        rbAllocationType_ = getRbAllocationType(par("rbAllocationType").stringValue());
        currentTxMode_ = aToTxMode(par("initialTxMode"));

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

LteFeedbackDoubleVector LteUlFeedbackGenerator::computeUlFeedback(UserControlInfo *lteinfo, LteAirFrame *frame)
{
    Enter_Method("computeUlFeedback()");
    EV_INFO << "LteUlFeedbackGenerator::computeUlFeedback - computing UL CSI" << endl;

    LteChannelModel *channelModel = phy_->getChannelModel(lteinfo->getCarrierFrequency());
    if (channelModel == nullptr)
        throw cRuntimeError("LteUlFeedbackGenerator::computeUlFeedback - channelModel is NULL pointer");

    Coord sendersPos = lteinfo->getCoord();
    cellInfo_->setUePosition(lteinfo->getSourceId(), sendersPos);

    // compute SINR
    std::vector<double> snr = channelModel->getSINR(frame, lteinfo);

    std::map<Remote, int> antennaCws;
    antennaCws[MACRO] = 1;

    return lteFeedbackComputation_->computeFeedback(fbType_, rbAllocationType_,
            currentTxMode_, antennaCws, numPreferredBands_, 0, snr, lteinfo->getSourceId());
}

LteFeedbackDoubleVector LteUlFeedbackGenerator::computeD2DFeedback(UserControlInfo *lteinfo, LteAirFrame *frame, MacNodeId peerId, const inet::Coord& peerCoord)
{
    Enter_Method("computeD2DFeedback()");
    EV_INFO << "LteUlFeedbackGenerator::computeD2DFeedback - computing D2D CSI" << endl;

    LteChannelModel *channelModel = phy_->getChannelModel(lteinfo->getCarrierFrequency());
    if (channelModel == nullptr)
        throw cRuntimeError("LteUlFeedbackGenerator::computeD2DFeedback - channelModel is NULL pointer");

    // compute SINR
    std::vector<double> snr = channelModel->getSINR_D2D(frame, lteinfo, peerId, peerCoord, phy_->getMacNodeId());

    std::map<Remote, int> antennaCws;
    antennaCws[MACRO] = 1;

    return lteFeedbackComputation_->computeFeedback(fbType_, rbAllocationType_,
            currentTxMode_, antennaCws, numPreferredBands_, 0, snr, lteinfo->getSourceId());
}

} // namespace simu5g
