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

#include <algorithm>

#include "simu5g/stack/phy/radio/CellularPhantomRadio.h"
#include "simu5g/stack/phy/radio/CellularTransmitter.h"

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
        bsNodeId_ = mac_->getMacNodeId();
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
    AirFrame *frame = new AirFrame("bgUeSinrComputationFrame");
    UserControlInfo *cInfo = new UserControlInfo();

    // Build a control info
    cInfo->setSourceId(getBgUeId(bgUeIndex));  // MacNodeId for the bgUe
    cInfo->setDestId(mac_->getMacNodeId());  // ID of the e/gNodeB
    cInfo->setFrameType(FEEDBACKPKT);
    cInfo->setCoord(bgUePos);
    cInfo->setDirection(dir);
    cInfo->setCarrierFrequency(carrierFrequency_);
    if (dir == UL)
        cInfo->setTxPower(bgUeTxPower);
    else
        cInfo->setTxPower(bsTxPower_);

    std::vector<double> snr = channelModel_->getSINR_bgUe(phy_.get(), frame, cInfo, bgUe_.at(bgUeIndex)->getSpeed());

    // Free memory
    delete frame;
    delete cInfo;

    return snr;
}

unsigned int BackgroundTrafficManager::getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir)
{
    int index = getBgUeIndex(bgUeId);
    Cqi cqi = bgUe_.at(index)->getCqi(dir);

    // Get bytes per block based on CQI
    return mac_->getAmc()->computeBitsPerRbBackground(cqi, dir, carrierFrequency_) / 8;
}

void BackgroundTrafficManager::registerTransmission(MacNodeId bgUeId, Direction dir, GHz carrierFrequency, const RbMap& allocatedRbMap)
{
    Enter_Method("registerTransmission");
    if (dir != DL && dir != UL)
        throw cRuntimeError("BackgroundTrafficManager::registerTransmission(): unrecognized direction %d", (int)dir);
    // an allocation that occupies no band stands for no transmission
    ASSERT(std::any_of(allocatedRbMap.begin(), allocatedRbMap.end(), [] (const auto& antennaBlocks) {
        return std::any_of(antennaBlocks.second.begin(), antennaBlocks.second.end(), [] (const auto& bandBlocks) { return bandBlocks.second != 0; });
    }));

    // one slot of the carrier, as the duration of a frame a radio sends on it
    double slotDuration = binder_->getSlotDurationFromNumerologyIndex(binder_->getNumerologyIndexFromCarrierFreq(carrierFrequency));
    if (dir == DL) {
        // the base station's radio sends it to the UE
        ASSERT(phy_->getMacNodeId() == bsNodeId_);
        auto transmission = phy_->getTransmitter()->createPhantomTransmission(phy_.get(), bgUeId, bsNodeId_, DL, carrierFrequency, allocatedRbMap, slotDuration, nullptr);
        radioMedium_->addPhantomTransmission(transmission);
    }
    else
        getBgUeRadio(bgUeId)->transmit(bsNodeId_, bsNodeId_, UL, carrierFrequency, allocatedRbMap, slotDuration, nullptr);
}

double BackgroundTrafficManager::getTtiPeriod()
{
    return mac_->getTtiPeriod();
}

} //namespace
