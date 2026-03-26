//
//                  Simu5G
//
// Copyright (C) 2019-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/background/cell/BackgroundCellTrafficManager.h"
#include "simu5g/background/cell/BackgroundScheduler.h"
#include "simu5g/background/cell/BackgroundCellAmc.h"
#include "simu5g/background/cell/BackgroundCellAmcNr.h"
#include "simu5g/background/trafficGenerator/ActiveUeNotification_m.h"
#include "simu5g/stack/mac/LteMacEnb.h"
#include "simu5g/stack/phy/LtePhyEnb.h"
#include "simu5g/stack/phy/channelmodel/LteChannelModel.h"

namespace simu5g {

Define_Module(BackgroundCellTrafficManager);


BackgroundCellTrafficManager::~BackgroundCellTrafficManager()
{
    delete bgAmc_;
}

void BackgroundCellTrafficManager::initialize(int stage)
{
    BackgroundTrafficManagerBase::initialize(stage);
    if (stage == INITSTAGE_SIMU5G_AMC_SETUP) {
        bgScheduler_.reference(this, "bgSchedulerModule", true);

        if (bgScheduler_->isNr())
            bgAmc_ = new BackgroundCellAmcNr(binder_);
        else
            bgAmc_ = new BackgroundCellAmc(binder_);
    }
    if (stage == INITSTAGE_SIMU5G_BACKGROUNDTRAFFICMANAGER) {
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

std::vector<double> BackgroundCellTrafficManager::getSINR(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower)
{
    BackgroundCellChannelModel *bgChannelModel = bgScheduler_->getChannelModel();
    TrafficGeneratorBase *bgUe = bgUe_.at(bgUeIndex);

    MacNodeId bgUeId = MacNodeId(BGUE_MIN_ID + bgUeIndex);
    std::vector<double> snr = bgChannelModel->getSINR(bgUeId, bgUePos, bgUe, bgScheduler_, dir);
    return snr;
}

unsigned int BackgroundCellTrafficManager::getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir)
{
    int index = num(bgUeId) - BGUE_MIN_ID;
    Cqi cqi = bgUe_.at(index)->getCqi(dir);

    return bgAmc_->computeBitsPerRbBackground(cqi, dir, carrierFrequency_) / 8;
}

double BackgroundCellTrafficManager::getTtiPeriod()
{
    return bgScheduler_->getTtiPeriod();
}

double BackgroundCellTrafficManager::getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus)
{
    BackgroundCellChannelModel *bgChannelModel = bgScheduler_->getChannelModel();
    return bgChannelModel->getReceivedPower_bgUe(txPower, txPos, rxPos, dir, losStatus, bgScheduler_);
}

} //namespace
