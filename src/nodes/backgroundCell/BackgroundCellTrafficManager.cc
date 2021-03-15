//
//                           Simu5G
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "nodes/backgroundCell/BackgroundCellTrafficManager.h"
#include "nodes/backgroundCell/BackgroundBaseStation.h"
#include "nodes/backgroundCell/BackgroundCellAmc.h"
#include "nodes/backgroundCell/BackgroundCellAmcNr.h"
#include "stack/backgroundTrafficGenerator/ActiveUeNotification_m.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/phy/layer/LtePhyEnb.h"
#include "stack/phy/ChannelModel/LteChannelModel.h"

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
    cSimpleModule::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
    {
        numBgUEs_ = getAncestorPar("numBgUes");
    }
    if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        bgBaseStation_ = check_and_cast<BackgroundBaseStation*>(getParentModule()->getParentModule()->getSubmodule("bgBaseStation"));

        if (bgBaseStation_->isNr())
            bgAmc_ = new BackgroundCellAmcNr();
        else
            bgAmc_ = new BackgroundCellAmc();

        enbTxPower_ = bgBaseStation_->getTxPower();

        // create vector of BackgroundUEs
        for (int i=0; i < numBgUEs_; i++)
            bgUe_.push_back(check_and_cast<TrafficGeneratorBase*>(getParentModule()->getSubmodule("bgUE", i)->getSubmodule("generator")));
    }
}


Cqi BackgroundCellTrafficManager::computeCqi(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower)
{
    // TODO compute CQI based on distance between UE and BS
//    inet::Coord bgBsCoord = bgBaseStation_->getPosition();

    Cqi cqi = intuniform(2,15);
    return cqi;
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

        double offset = bgBaseStation_->getTtiPeriod() * 6;  // TODO make it configurable
                                                             //      there are 6 slots between the first BSR and actual data
        scheduleAt(NOW + offset, notification);
    }
}
