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

namespace simu5g {

Define_Module(BackgroundCellTrafficManager);

BackgroundCellTrafficManager::BackgroundCellTrafficManager()
{
}

BackgroundCellTrafficManager::~BackgroundCellTrafficManager()
{
    delete bgAmc_;
}

void BackgroundCellTrafficManager::initialize(int stage)
{
    BackgroundTrafficManagerBase::initialize(stage);
    if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        bgScheduler_.reference(this, "bgSchedulerModule", true);

        if (bgScheduler_->isNr())
            bgAmc_ = new BackgroundCellAmcNr(binder_);
        else
            bgAmc_ = new BackgroundCellAmc(binder_);
    }
    if (stage == inet::INITSTAGE_LAST-1)
    {
        bsTxPower_ = bgScheduler_->getTxPower();
        bsCoord_ = bgScheduler_->getPosition();
    }
}

bool BackgroundCellTrafficManager::isSetBgTrafficManagerInfoInit()
{
    return par("enablePeriodicCqiUpdate").boolValue() && par("computeAvgInterference").boolValue();
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

double BackgroundCellTrafficManager::getTtiPeriod()
{
    return bgScheduler_->getTtiPeriod();
}

double BackgroundCellTrafficManager::getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus)
{
    BackgroundCellChannelModel* bgChannelModel = bgScheduler_->getChannelModel();
    return bgChannelModel->getReceivedPower_bgUe(txPower, txPos, rxPos, dir, losStatus, bgScheduler_);
}

} //namespace

