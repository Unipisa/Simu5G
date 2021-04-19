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
#include "nodes/backgroundCell/BackgroundBaseStation.h"
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
    BackgroundCellChannelModel* bgChannelModel = bgBaseStation_->getChannelModel();
    TrafficGeneratorBase* bgUe = bgUe_.at(bgUeIndex);

    MacNodeId bgUeId = BGUE_MIN_ID + bgUeIndex;
    std::vector<double> snr = bgChannelModel->getSINR(bgUeId, bgUePos, bgUe, bgBaseStation_, dir);

    // convert the SNR to CQI and compute the mean
    Cqi bandCqi, meanCqi = 0;
    std::vector<double>::iterator it = snr.begin();

    for (; it != snr.end(); ++it)
    {
        // lookup table that associates the SINR to a range of CQI values
        bandCqi = getCqiFromTable(*it);
        meanCqi += bandCqi;
    }
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

        double offset = bgBaseStation_->getTtiPeriod() * 6;  // TODO make it configurable
                                                             //      there are 6 slots between the first BSR and actual data
        scheduleAt(NOW + offset, notification);
    }
}
