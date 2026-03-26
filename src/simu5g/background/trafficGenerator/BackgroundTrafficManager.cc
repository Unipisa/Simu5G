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

#include "simu5g/background/trafficGenerator/BackgroundTrafficManager.h"

namespace simu5g {

Define_Module(BackgroundTrafficManager);


void BackgroundTrafficManager::initialize(int stage)
{
    BackgroundTrafficManagerBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        phy_.reference(this, "phyModule", true);
        mac_.reference(this, "macModule", true);
    }
    if (stage == INITSTAGE_SIMU5G_BACKGROUNDTRAFFICMANAGER) {
        // Get the reference to the channel model for the given carrier
        bsTxPower_ = phy_->getTxPwr();
        bsCoord_ = phy_->getCoord();
        channelModel_ = phy_->getChannelModel(carrierFrequency_);
        if (channelModel_ == nullptr)
            throw cRuntimeError("BackgroundTrafficManagerBase::initialize - cannot find channel model for carrier frequency %f", carrierFrequency_.get());
    }
}

bool BackgroundTrafficManager::isSetBgTrafficManagerInfoInit()
{
    return !par("enablePeriodicCqiUpdate").boolValue() && par("computeAvgInterference").boolValue();
}

unsigned int BackgroundTrafficManager::getNumBands()
{
    return channelModel_->getNumBands();
}

std::vector<double> BackgroundTrafficManager::getSINR(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower)
{
    // This is a fictitious frame that we need to compute the SINR
    LteAirFrame *frame = new LteAirFrame("bgUeSinrComputationFrame");
    UserControlInfo *cInfo = new UserControlInfo();

    // Build a control info
    cInfo->setSourceId(MacNodeId(BGUE_MIN_ID + bgUeIndex));  // MacNodeId for the bgUe
    cInfo->setDestId(mac_->getMacNodeId());  // ID of the e/gNodeB
    cInfo->setFrameType(FEEDBACKPKT);
    cInfo->setCoord(bgUePos);
    cInfo->setDirection(dir);
    cInfo->setCarrierFrequency(carrierFrequency_);
    if (dir == UL)
        cInfo->setTxPower(bgUeTxPower);
    else
        cInfo->setTxPower(bsTxPower_);

    std::vector<double> snr = channelModel_->getSINR_bgUe(frame, cInfo);

    // Free memory
    delete frame;
    delete cInfo;

    return snr;
}

unsigned int BackgroundTrafficManager::getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir)
{
    int index = num(bgUeId) - BGUE_MIN_ID;
    Cqi cqi = bgUe_.at(index)->getCqi(dir);

    // Get bytes per block based on CQI
    return mac_->getAmc()->computeBitsPerRbBackground(cqi, dir, carrierFrequency_) / 8;
}

double BackgroundTrafficManager::getTtiPeriod()
{
    return mac_->getTtiPeriod();
}

double BackgroundTrafficManager::getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus)
{
    MacNodeId bsId = mac_->getMacNodeId();
    return channelModel_->getReceivedPower_bgUe(txPower, txPos, rxPos, dir, losStatus, bsId);
}

} //namespace
