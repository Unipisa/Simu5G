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

#include "stack/phy/feedback/LteDlFeedbackGenerator.h"
#include "stack/phy/layer/LtePhyUe.h"

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
    if (stage == INITSTAGE_LOCAL)
    {
        // Read NED parameters
        fbPeriod_ = (simtime_t)(int(par("fbPeriod")) * TTI);// TTI -> seconds
        fbDelay_ = (simtime_t)(int(par("fbDelay")) * TTI);// TTI -> seconds
        if (fbPeriod_ <= fbDelay_)
        {
            error("Feedback Period MUST be greater than Feedback Delay");
        }
        fbType_ = getFeedbackType(par("feedbackType").stringValue());
        rbAllocationType_ = getRbAllocationType(
            par("rbAllocationType").stringValue());
        usePeriodic_ = par("usePeriodic");
        currentTxMode_ = aToTxMode(par("initialTxMode"));

        generatorType_ = getFeedbackGeneratorType(
            par("feedbackGeneratorType").stringValue());

        // TODO: find a more elegant way
        if (strcmp(getFullName(), "nrDlFbGen") == 0)
            masterId_ = getAncestorPar("nrMasterId");
        else
            masterId_ = getAncestorPar("masterId");
        nodeId_ = getAncestorPar("macNodeId");

        /** Initialize timers **/

        tPeriodicSensing_ = new TTimer(this);
        tPeriodicSensing_->setTimerId(PERIODIC_SENSING);

        tPeriodicTx_ = new TTimer(this);
        tPeriodicTx_->setTimerId(PERIODIC_TX);

        tAperiodicTx_ = new TTimer(this);
        tAperiodicTx_->setTimerId(APERIODIC_TX);
        feedbackComputationPisa_ = false;
        WATCH(fbType_);
        WATCH(rbAllocationType_);
        WATCH(fbPeriod_);
        WATCH(fbDelay_);
        WATCH(usePeriodic_);
        WATCH(currentTxMode_);
    }
    else if (stage == INITSTAGE_LINK_LAYER)
    {
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " init" << endl;

        if (masterId_ > 0)  // only if not detached
            initCellInfo();

        LtePhyUe* tmp;
        // TODO: find a more elegant way
        if (strcmp(getFullName(), "nrDlFbGen") == 0)
            tmp = dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("nrPhy"));
        else
            tmp = dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("phy"));

        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " phyUe taken" << endl;
        dasFilter_ = tmp->getDasFilter();
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " phyUe used" << endl;
//        initializeFeedbackComputation(par("feedbackComputation").xmlValue());


        // TODO: remove this parameter
        feedbackComputationPisa_ = true;

        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " feedback computation initialize" << endl;
        WATCH(numBands_);
        WATCH(numPreferredBands_);
        if (masterId_ > 0 && usePeriodic_)
        {
            tPeriodicSensing_->start(0);
        }
    }
}

void LteDlFeedbackGenerator::handleMessage(cMessage *msg)
{
    TTimerMsg *tmsg = check_and_cast<TTimerMsg*>(msg);
    FbTimerType type = (FbTimerType) tmsg->getTimerId();

    if (type == PERIODIC_SENSING)
    {
        EV << NOW << " Periodic Sensing" << endl;
        tPeriodicSensing_->handle();
        tPeriodicSensing_->start(fbPeriod_);
        sensing(PERIODIC);
    }
    else if (type == PERIODIC_TX)
    {
        EV << NOW << " Periodic Tx" << endl;
        tPeriodicTx_->handle();
        sendFeedback(periodicFeedback, PERIODIC);
    }
    else if (type == APERIODIC_TX)
    {
        EV << NOW << " Aperiodic Tx" << endl;
        tAperiodicTx_->handle();
        sendFeedback(aperiodicFeedback, APERIODIC);
    }
    delete tmsg;
}

void LteDlFeedbackGenerator::initCellInfo()
{
    cellInfo_ = getCellInfo(masterId_);
    EV << "DLFeedbackGenerator - nodeid: " << nodeId_ << " cellInfo taken" << endl;

    if (cellInfo_ != NULL)
    {
        antennaCws_ = cellInfo_->getAntennaCws();
        numBands_ = cellInfo_->getPrimaryCarrierNumBands();
        numPreferredBands_ = cellInfo_->getNumPreferredBands();
    }
    else
        throw cRuntimeError("LteDlFeedbackGenerator::initCellInfo - cellInfo is NULL pointer. Aborting");

    EV << "DLFeedbackGenerator - nodeid: " << nodeId_
       << " used cellInfo: bands " << numBands_ << " preferred bands "
       << numPreferredBands_ << endl;
}


void LteDlFeedbackGenerator::sensing(FbPeriodicity per)
{
    if (per == PERIODIC && tAperiodicTx_->busy()
        && tAperiodicTx_->elapsed() < 0.001)
    {
        /* In this TTI an APERIODIC sensing has been done
         * (an APERIODIC tx is scheduled).
         * Ignore this PERIODIC request.
         */
        EV << NOW << " Aperiodic before Periodic in the same TTI: ignore Periodic" << endl;
        return;
    }

    if (per == APERIODIC && tAperiodicTx_->busy())
    {
        /* An APERIODIC tx is already scheduled:
         * ignore this request.
         */
        EV << NOW << " Aperiodic overlapping: ignore second Aperiodic" << endl;
        return;
    }

    if (per == APERIODIC && tPeriodicTx_->busy()
        && tPeriodicTx_->elapsed() < 0.001)
    {
        /* In this TTI a PERIODIC sensing has been done.
         * Deschedule the PERIODIC tx and continue with APERIODIC.
         */
        EV << NOW << " Periodic before Aperiodic in the same TTI: remove Periodic" << endl;
        tPeriodicTx_->stop();
    }

    // Schedule feedback transmission
    if (per == PERIODIC)
        tPeriodicTx_->start(fbDelay_);
    else if (per == APERIODIC)
        tAperiodicTx_->start(fbDelay_);
}

        /***************************
         *    PUBLIC FUNCTIONS
         ***************************/

LteDlFeedbackGenerator::LteDlFeedbackGenerator()
{
    tPeriodicSensing_ = nullptr;
    tPeriodicTx_ = nullptr;
    tAperiodicTx_ = nullptr;
}

LteDlFeedbackGenerator::~LteDlFeedbackGenerator()
{
    delete tPeriodicSensing_;
    delete tPeriodicTx_;
    delete tAperiodicTx_;
}

void LteDlFeedbackGenerator::aperiodicRequest()
{
    Enter_Method("aperiodicRequest()");
    EV << NOW << " Aperiodic request" << endl;
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
    EV << "sendFeedback() in DL" << endl;
    EV << "Periodicity: " << periodicityToA(per) << " nodeId: " << nodeId_ << endl;

    FeedbackRequest feedbackReq;
    if (feedbackComputationPisa_)
    {
        feedbackReq.request = true;
        feedbackReq.genType = getFeedbackGeneratorType(
            getAncestorPar("feedbackGeneratorType").stringValue());
        feedbackReq.type = getFeedbackType(par("feedbackType").stringValue());
        feedbackReq.txMode = currentTxMode_;
        feedbackReq.rbAllocationType = rbAllocationType_;
    }
    else
    {
        feedbackReq.request = false;
    }

    //use PHY function to send feedback
    // TODO: find a more elegant way
    if (strcmp(getFullName(), "nrDlFbGen") == 0)
        (dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("nrPhy")))->sendFeedback(fb, fb, feedbackReq);
    else
        (dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("phy")))->sendFeedback(fb, fb, feedbackReq);
}

// TODO adjust default value
LteFeedbackComputation* LteDlFeedbackGenerator::getFeedbackComputationFromName(
    std::string name, ParameterMap& params)
{
    ParameterMap::iterator it;
    if (name == "REAL")
    {
        feedbackComputationPisa_ = true;
        return 0;
    }
    else
        return 0;
}


void LteDlFeedbackGenerator::handleHandover(MacCellId newEnbId)
{
    Enter_Method("LteDlFeedbackGenerator::handleHandover()");
    masterId_ = newEnbId;
    if (masterId_ != 0)
    {
        initCellInfo();
        EV << NOW << " LteDlFeedbackGenerator::handleHandover - Master ID updated to " << masterId_ << endl;
        if (tPeriodicSensing_->idle())  // resume feedback
            tPeriodicSensing_->start(0);
    }
    else
    {
        cellInfo_ = NULL;

        // stop measuring feedback
        tPeriodicSensing_->stop();
    }
}
