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

#include "nodes/backgroundCell/BackgroundCellTrafficManager.h"
#include "nodes/backgroundCell/BackgroundScheduler.h"
#include "nodes/backgroundCell/BackgroundCellAmc.h"
#include "nodes/backgroundCell/BackgroundCellAmcNr.h"
#include "stack/backgroundTrafficGenerator/ActiveUeNotification_m.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/phy/layer/LtePhyEnb.h"
#include "stack/phy/ChannelModel/LteChannelModel.h"

Define_Module(BackgroundCellTrafficManager);

double BackgroundCellTrafficManager::nrCqiTable[16] = {
        -9999.0,
        -9999.0,
        -9999.0,
        -5.5,
        -3.5,
        -1.5,
        0.5,
        4.5,
        5.5,
        7.5,
        10.5,
        12.5,
        15.5,
        17.5,
        21.5,
        25.5
};

double BackgroundCellTrafficManager::getCqiFromTable(double snr)
{
    for (unsigned int i=0; i<16; i++)
    {
        if (snr < nrCqiTable[i])
            return i-1;
    }
    return 15;
}

BackgroundCellTrafficManager::BackgroundCellTrafficManager()
{
}

BackgroundCellTrafficManager::~BackgroundCellTrafficManager()
{
    delete bgAmc_;
}

void BackgroundCellTrafficManager::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
    {
        numBgUEs_ = getAncestorPar("numBgUes");
    }
    if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        bgScheduler_ = check_and_cast<BackgroundScheduler*>(getParentModule()->getParentModule()->getSubmodule("bgScheduler"));

        if (bgScheduler_->isNr())
            bgAmc_ = new BackgroundCellAmcNr();
        else
            bgAmc_ = new BackgroundCellAmc();

        phyPisaData_ = &(getBinder()->phyPisaData);
    }
    if (stage == inet::INITSTAGE_LAST-1)
    {
        bsTxPower_ = bgScheduler_->getTxPower();
        bsCoord_ = bgScheduler_->getPosition();

        // create vector of BackgroundUEs
        for (int i=0; i < numBgUEs_; i++)
            bgUe_.push_back(check_and_cast<TrafficGeneratorBase*>(getParentModule()->getSubmodule("bgUE", i)->getSubmodule("generator")));

        BgTrafficManagerInfo* info = new BgTrafficManagerInfo();
        info->init = false;
        info->bgTrafficManager = this;
        info->carrierFrequency = carrierFrequency_;
        info->allocatedRbsDl = 0.0;
        info->allocatedRbsUl = 0.0;
        info->allocatedRbsUeUl.resize(numBgUEs_, 0.0);

        if (!getAncestorPar("enablePeriodicCqiUpdate"))
        {
            if (getAncestorPar("computeAvgInterference"))
            {
                initializeAvgInterferenceComputation();
                info->init = true;
            }
        }

        getBinder()->addBgTrafficManagerInfo(info);
    }
}

unsigned int BackgroundCellTrafficManager::getNumBands()
{
    return bgScheduler_->getNumBands();
}


Cqi BackgroundCellTrafficManager::computeCqi(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower)
{
    BackgroundCellChannelModel* bgChannelModel = bgScheduler_->getChannelModel();
    TrafficGeneratorBase* bgUe = bgUe_.at(bgUeIndex);

    MacNodeId bgUeId = BGUE_MIN_ID + bgUeIndex;
    std::vector<double> snr = bgChannelModel->getSINR(bgUeId, bgUePos, bgUe, bgScheduler_, dir);

    // convert the SNR to CQI and compute the mean
    double meanSinr = 0;
    Cqi bandCqi, meanCqi = 0;
    std::vector<double>::iterator it = snr.begin();

    for (; it != snr.end(); ++it)
    {
        meanSinr += *it;

        // lookup table that associates the SINR to a range of CQI values
        bandCqi = computeCqiFromSinr(*it);

        meanCqi += bandCqi;
    }

    meanSinr /= snr.size();
    bgUe->collectMeasuredSinr(meanSinr, dir);

    meanCqi /= snr.size();
    if(meanCqi < 2)
        meanCqi = 2;

    return meanCqi;
}

unsigned int BackgroundCellTrafficManager::getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir)
{
    int index = bgUeId - BGUE_MIN_ID;
    Cqi cqi = bgUe_.at(index)->getCqi(dir);

    return bgAmc_->computeBitsPerRbBackground(cqi, dir, carrierFrequency_) / 8;
}

void BackgroundCellTrafficManager::racHandled(MacNodeId bgUeId)
{
    Enter_Method("BackgroundTrafficManager::racHandled");

    int index = bgUeId - BGUE_MIN_ID;

    waitingForRac_.remove(index);

    // some bytes have been added in the RB assigned for the first BSR, consume them from the buffer
    unsigned int servedWithFirstBsr = getBackloggedUeBytesPerBlock(bgUeId, UL);
    unsigned int newBuffLen = consumeBackloggedUeBytes(bgUeId, servedWithFirstBsr, UL);

    // if there are still data in the buffer
    if (newBuffLen > 0)
    {
        ActiveUeNotification* notification = new ActiveUeNotification("activeUeNotification");
        notification->setIndex(index);

        double offset = bgScheduler_->getTtiPeriod() * 6;  // TODO make it configurable
                                                             //      there are 6 slots between the first BSR and actual data
        scheduleAt(NOW + offset, notification);
    }
}

double BackgroundCellTrafficManager::getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus)
{
    BackgroundCellChannelModel* bgChannelModel = bgScheduler_->getChannelModel();
    return bgChannelModel->getReceivedPower_bgUe(txPower, txPos, rxPos, dir, losStatus, bgScheduler_);
}
