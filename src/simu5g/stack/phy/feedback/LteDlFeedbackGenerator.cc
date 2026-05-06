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

#include "simu5g/stack/phy/feedback/LteDlFeedbackGenerator.h"

namespace simu5g {

Define_Module(LteDlFeedbackGenerator);

using namespace omnetpp;
using namespace inet;

/*****************************
*    PRIVATE FUNCTIONS
*****************************/

/******************************
*    PROTECTED FUNCTIONS
******************************/

void LteDlFeedbackGenerator::initialize(int stage)
{
    EV << "DlFeedbackGenerator stage: " << stage << endl;
    if (stage == inet::INITSTAGE_LOCAL) {
        // Read NED parameters
        binder_.reference(this, "binderModule", true);
        fbPeriod_ = (simtime_t)(int(par("fbPeriod")) * TTI);// TTI -> seconds
        fbDelay_ = (simtime_t)(int(par("fbDelay")) * TTI);// TTI -> seconds
        if (fbPeriod_ <= fbDelay_) {
            throw cRuntimeError("Feedback Period MUST be greater than Feedback Delay");
        }
        fbType_ = getFeedbackType(par("feedbackType").stringValue());
        rbAllocationType_ = getRbAllocationType(par("rbAllocationType").stringValue());
        usePeriodic_ = par("usePeriodic");
        currentTxMode_ = aToTxMode(par("initialTxMode"));
        useUeDlFeedbackComputation_ = par("useUeDlFeedbackComputation");
        targetBler_ = par("targetBler");

        cModule *networkNode = getContainingNode(this);
        bool isNr = strcmp(getFullName(), "nrDlFbGen") == 0;
        nodeId_ = MacNodeId(networkNode->par(isNr ? "nrMacNodeId" : "macNodeId").intValue()); //TODO or

        // Initialize timers
        tPeriodicSensing_ = new TTimer(this);
        tPeriodicSensing_->setTimerId(PERIODIC_SENSING);

        tPeriodicTx_ = new TTimer(this);
        tPeriodicTx_->setTimerId(PERIODIC_TX);

        tAperiodicTx_ = new TTimer(this);
        tAperiodicTx_->setTimerId(APERIODIC_TX);
        feedbackComputationPisa_ = !useUeDlFeedbackComputation_;
        WATCH(fbType_);
        WATCH(rbAllocationType_);
        WATCH(fbPeriod_);
        WATCH(fbDelay_);
        WATCH(usePeriodic_);
        WATCH(currentTxMode_);
    }
    else if (stage == INITSTAGE_SIMU5G_BINDER_ACCESS) {
        masterId_ = binder_->getServingNode(nodeId_);
    }
    else if (stage == INITSTAGE_SIMU5G_PHYSICAL_LAYER) {
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " init" << endl;

        if (masterId_ != NODEID_NONE)                          // only if not detached
            initCellInfo();

        phy_.reference(this, "phyModule", true);

        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " phyUe taken" << endl;
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " phyUe used" << endl;
        if (useUeDlFeedbackComputation_)
            initializeFeedbackComputation();

        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " feedback computation initialize" << endl;
        WATCH(numBands_);
        WATCH(numPreferredBands_);
        if (masterId_ != NODEID_NONE && usePeriodic_ && !useUeDlFeedbackComputation_) {
            tPeriodicSensing_->start(0);
        }
    }
}

void LteDlFeedbackGenerator::handleMessage(cMessage *msg)
{
    TTimerMsg *tmsg = check_and_cast<TTimerMsg *>(msg);
    FbTimerType type = (FbTimerType)tmsg->getTimerId();

    if (type == PERIODIC_SENSING) {
        EV << NOW << " Periodic Sensing" << endl;
        tPeriodicSensing_->handle();
        tPeriodicSensing_->start(fbPeriod_);
        sensing(PERIODIC);
    }
    else if (type == PERIODIC_TX) {
        EV << NOW << " Periodic Tx" << endl;
        tPeriodicTx_->handle();
        sendFeedback(periodicFeedback, PERIODIC);
    }
    else if (type == APERIODIC_TX) {
        EV << NOW << " Aperiodic Tx" << endl;
        tAperiodicTx_->handle();
        sendFeedback(aperiodicFeedback, APERIODIC);
    }
    delete tmsg;
}

void LteDlFeedbackGenerator::initCellInfo()
{
    cellInfo_ = binder_->getCellInfoByNodeId(masterId_);
    EV << "DLFeedbackGenerator - nodeid: " << nodeId_ << " cellInfo taken" << endl;

    if (cellInfo_ != nullptr) {
        numBands_ = cellInfo_->getPrimaryCarrierNumBands();
        numPreferredBands_ = cellInfo_->getNumPreferredBands();
    }
    else
        throw cRuntimeError("LteDlFeedbackGenerator::initCellInfo - cellInfo is NULL pointer");

    EV << "DLFeedbackGenerator - nodeid: " << nodeId_
       << " used cellInfo: bands " << numBands_ << " preferred bands "
       << numPreferredBands_ << endl;
}

void LteDlFeedbackGenerator::initializeFeedbackComputation()
{
    if (lteFeedbackComputation_)
        delete lteFeedbackComputation_;
    lteFeedbackComputation_ = new LteFeedbackComputationRealistic(binder_, targetBler_, numBands_);
}

void LteDlFeedbackGenerator::sensing(FbPeriodicity per)
{
    if (per == PERIODIC && tAperiodicTx_->busy()
        && tAperiodicTx_->elapsed() < 0.001)
    {
        EV << NOW << " Aperiodic before Periodic in the same TTI: ignore Periodic" << endl;
        return;
    }

    if (per == APERIODIC && tAperiodicTx_->busy()) {
        EV << NOW << " Aperiodic overlapping: ignore second Aperiodic" << endl;
        return;
    }

    if (per == APERIODIC && tPeriodicTx_->busy()
        && tPeriodicTx_->elapsed() < 0.001)
    {
        EV << NOW << " Periodic before Aperiodic in the same TTI: remove Periodic" << endl;
        tPeriodicTx_->stop();
    }

    if (per == PERIODIC)
        tPeriodicTx_->start(fbDelay_);
    else if (per == APERIODIC)
        tAperiodicTx_->start(fbDelay_);
}

/***************************
*    PUBLIC FUNCTIONS
***************************/


LteDlFeedbackGenerator::~LteDlFeedbackGenerator()
{
    delete lteFeedbackComputation_;
    delete tPeriodicSensing_;
    delete tPeriodicTx_;
    delete tAperiodicTx_;
}

void LteDlFeedbackGenerator::aperiodicRequest()
{
    Enter_Method("aperiodicRequest()");
    EV << NOW << " Aperiodic request" << endl;
    if (useUeDlFeedbackComputation_ && aperiodicFeedback.empty()) {
        EV << NOW << " Aperiodic request ignored: no CSI-RS-based feedback available yet" << endl;
        return;
    }

    if (useUeDlFeedbackComputation_) {
        sendFeedback(aperiodicFeedback, APERIODIC);
        return;
    }
    sensing(APERIODIC);
}

void LteDlFeedbackGenerator::setTxMode(TxMode newTxMode)
{
    Enter_Method("setTxMode()");
    currentTxMode_ = newTxMode;
}

void LteDlFeedbackGenerator::sendFeedback(LteFeedbackDoubleVector fb,
        FbPeriodicity per)
{
    EV << "LteDlFeedbackGenerator::sendFeedback - sending " << periodicityToA(per) << " CSI feedback, nodeId: " << nodeId_ << endl;

    if (useUeDlFeedbackComputation_ && fb.empty()) {
        EV << NOW << " Feedback transmission skipped: no cached DL feedback available" << endl;
        return;
    }

    FeedbackRequest feedbackReq = getFeedbackRequest();

    LteFeedbackDoubleVector feedbackDl = useUeDlFeedbackComputation_ ? fb : LteFeedbackDoubleVector();
    LteFeedbackDoubleVector feedbackUl;

    //use PHY function to send feedback
    phy_->sendFeedback(feedbackDl, feedbackUl, feedbackReq);
}

FeedbackRequest LteDlFeedbackGenerator::getFeedbackRequest() const
{
    FeedbackRequest feedbackReq;
    feedbackReq.request = true;
    feedbackReq.type = fbType_;
    feedbackReq.txMode = currentTxMode_;
    feedbackReq.dasAware = false;
    feedbackReq.rbAllocationType = rbAllocationType_;
    feedbackReq.dlFeedbackFromUe = useUeDlFeedbackComputation_;
    return feedbackReq;
}

// TODO adjust default value
LteFeedbackComputation *LteDlFeedbackGenerator::getFeedbackComputationFromName(std::string name, ParameterMap& params)
{
    ParameterMap::iterator it;
    if (name == "REAL") {
        feedbackComputationPisa_ = true;
        return nullptr;
    }
    else
        return nullptr;
}

void LteDlFeedbackGenerator::handleHandover(MacCellId newEnbId)
{
    Enter_Method("LteDlFeedbackGenerator::handleHandover()");
    masterId_ = newEnbId;
    periodicFeedback.clear();
    aperiodicFeedback.clear();
    if (masterId_ != NODEID_NONE) {
        initCellInfo();
        if (useUeDlFeedbackComputation_)
            initializeFeedbackComputation();
        EV << NOW << " LteDlFeedbackGenerator::handleHandover - Master ID updated to " << masterId_ << endl;
        if (!useUeDlFeedbackComputation_ && usePeriodic_ && tPeriodicSensing_->idle())                                         // resume feedback
            tPeriodicSensing_->start(0);
    }
    else {
        cellInfo_ = nullptr;

        if (tPeriodicSensing_->busy())
            tPeriodicSensing_->stop();
        if (tPeriodicTx_->busy())
            tPeriodicTx_->stop();
    }
}

void LteDlFeedbackGenerator::handleCsiReferenceSignal(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    Enter_Method("handleCsiReferenceSignal()");

    if (!useUeDlFeedbackComputation_) {
        delete lteInfo;
        delete frame;
        return;
    }

    LteChannelModel *channelModel = phy_->getChannelModel(lteInfo->getCarrierFrequency());
    if (channelModel == nullptr)
        throw cRuntimeError("LteDlFeedbackGenerator::handleCsiReferenceSignal - channelModel is NULL pointer");

    take(frame);
    frame->setControlInfo(lteInfo);
    std::vector<double> snr = channelModel->getSINR(frame, lteInfo);

    std::map<Remote, int> antennaCws;
    antennaCws[MACRO] = 1;
    periodicFeedback = lteFeedbackComputation_->computeFeedback(
            fbType_, rbAllocationType_, currentTxMode_, antennaCws,
            numPreferredBands_, 0, snr, nodeId_);
    aperiodicFeedback = periodicFeedback;

    EV << NOW << " LteDlFeedbackGenerator::handleCsiReferenceSignal - Computed CSI feedback for DL direction" << endl;

    delete frame;

    sendFeedback(periodicFeedback, PERIODIC);
}

} //namespace
