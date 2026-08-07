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

    // compute SINR
    std::vector<double> snr = channelModel->getSINR(frame, lteinfo);

    return computeFeedbackFromSinr(lteinfo, snr);
}

LteFeedbackDoubleVector LteUlFeedbackGenerator::computeUlFeedback(UserControlInfo *lteinfo, const std::vector<double>& snr)
{
    Enter_Method("computeUlFeedback()");
    EV_INFO << "LteUlFeedbackGenerator::computeUlFeedback - computing UL CSI from an externally measured SINR" << endl;

    // The vector crosses a module boundary here, so a wrong size means the caller is wired
    // up incorrectly. Reject it rather than produce plausible-looking but meaningless CQI.
    if (static_cast<int>(snr.size()) != numBands_)
        throw cRuntimeError("LteUlFeedbackGenerator::computeUlFeedback - externally measured SINR vector has %d band(s), but feedback is computed over %d band(s)",
                static_cast<int>(snr.size()), numBands_);

    return computeFeedbackFromSinr(lteinfo, snr);
}

LteFeedbackDoubleVector LteUlFeedbackGenerator::computeFeedbackFromSinr(UserControlInfo *lteinfo, const std::vector<double>& snr)
{
    // The sender position is bookkeeping (MEC location services, D2D conflict graphs), not a
    // channel measurement, so it stays valid on a transparent NTN path where the frame reaches
    // us through the satellite.
    cellInfo_->setUePosition(lteinfo->getSourceId(), lteinfo->getCoord());

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
