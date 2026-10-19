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
#include "simu5g/stack/phy/PhyEnb.h"
#include "simu5g/stack/phy/channelmodel/ChannelModelBase.h"
#include "simu5g/stack/phy/radio/CellularPhantomRadio.h"

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
    // a CQI is a what-if on the medium, as a real cell's background UE's: in DL
    // a transmission of the cell's phantom radio at the UE's, in UL one of the
    // UE's phantom radio at the cell's. The UE's position and power are its
    // radio's: bgUePos and bgUeTxPower, its mobility's position and its
    // generator's power, are the same values.
    MacNodeId bgUeId = getBgUeId(bgUeIndex);
    CellularPhantomRadio *bgUeRadio = getBgUeRadio(bgUeId);
    CellularPhantomRadio *cellRadio = bgScheduler_->getRadio();
    CellularPhantomRadio *transmitter = dir == DL ? cellRadio : bgUeRadio;
    CellularPhantomRadio *receiver = dir == DL ? bgUeRadio : cellRadio;
    CellularTransmission transmission;
    transmission.transmitter = transmitter;
    transmission.phantomCell = bgScheduler_.get();
    transmission.sourceId = transmitter->getMacNodeId();
    transmission.destId = receiver->getMacNodeId();
    transmission.cellId = bgScheduler_->getNodeId();
    transmission.direction = dir;
    transmission.frameType = FEEDBACKPKT;
    transmission.carrierFrequency = carrierFrequency_;
    transmission.txPower = transmitter->getTxPwr(dir);
    transmission.startPosition = transmitter->getCoord();
    transmission.txDirection = transmitter->getTxDirection();
    transmission.txAngle = transmitter->getTxAngle();
    transmission.startTime = simTime();
    std::vector<double> sinr = radioMedium_->computeSinr(receiver, transmission);

    // the cell's bands: the first ones of its carrier
    unsigned int numBands = bgScheduler_->getNumBands();
    if (numBands > sinr.size())
        throw cRuntimeError("BackgroundCellTrafficManager::getSINR(): the background cell has %u bands, its carrier %g GHz only %u",
                numBands, carrierFrequency_.get(), (unsigned int)sinr.size());
    sinr.resize(numBands);
    return sinr;
}

unsigned int BackgroundCellTrafficManager::getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir)
{
    int index = getBgUeIndex(bgUeId);
    Cqi cqi = bgUe_.at(index)->getCqi(dir);

    return bgAmc_->computeBitsPerRbBackground(cqi, dir, carrierFrequency_) / 8;
}

void BackgroundCellTrafficManager::registerTransmission(MacNodeId bgUeId, Direction dir, GHz carrierFrequency, const RbMap& allocatedRbMap)
{
    throw cRuntimeError("BackgroundCellTrafficManager::registerTransmission(): the transmissions of a background cell's UEs are sent by its scheduler");
}

double BackgroundCellTrafficManager::getTtiPeriod()
{
    return bgScheduler_->getTtiPeriod();
}

} //namespace
