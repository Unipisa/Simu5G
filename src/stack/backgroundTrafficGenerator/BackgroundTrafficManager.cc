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

#include "stack/backgroundTrafficGenerator/BackgroundTrafficManager.h"
#include "stack/backgroundTrafficGenerator/ActiveUeNotification_m.h"
#include "stack/mac/layer/LteMacEnb.h"
#include "stack/phy/layer/LtePhyEnb.h"
#include "stack/phy/ChannelModel/LteChannelModel.h"

Define_Module(BackgroundTrafficManager);


const double nrCqiTable[16] = {
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

double getCqiFromTable(double snr)
{
    for (unsigned int i=0; i<16; i++)
    {
        if (snr < nrCqiTable[i])
            return i-1;
    }
    return 15;
}

BackgroundTrafficManager::BackgroundTrafficManager()
{
    channelModel_ = nullptr;
}

void BackgroundTrafficManager::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
    {
        numBgUEs_ = getAncestorPar("numBgUes");
    }
    if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT)
    {
        // create vector of BackgroundUEs
        for (int i=0; i < numBgUEs_; i++)
            bgUe_.push_back(check_and_cast<TrafficGeneratorBase*>(getParentModule()->getSubmodule("bgUE", i)->getSubmodule("generator")));
    }
    if (stage == inet::INITSTAGE_PHYSICAL_LAYER)
    {
        // get the reference to the MAC layer
        mac_ = check_and_cast<LteMacEnb*>(getParentModule()->getParentModule()->getSubmodule("mac"));

        LteBinder* binder = getBinder();
        minSinr_ = binder->phyPisaData.minSnr();
        maxSinr_ = binder->phyPisaData.maxSnr();
    }
}

void BackgroundTrafficManager::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) // this is a activeUeNotification message
    {
        ActiveUeNotification* notification = check_and_cast<ActiveUeNotification*>(msg);

        // add the bg UE to the list of active UEs in x slots
        backloggedBgUes_[UL].push_back(notification->getIndex());

        delete notification;
    }
}

void BackgroundTrafficManager::notifyBacklog(int index, Direction dir)
{
    if (dir != DL && dir != UL)
        throw cRuntimeError("TrafficGeneratorBase::consumeBytes - unrecognized direction: %d" , dir);

    if (dir == UL)
        waitingForRac_.push_back(index);
    else
        backloggedBgUes_[DL].push_back(index);
}

Cqi BackgroundTrafficManager::computeCqi(Direction dir, inet::Coord bgUePos, double bgUeTxPower)
{
    if (channelModel_ == nullptr)
    {
        // get the reference to the channel model for the given carrier
        LtePhyEnb* phy = check_and_cast<LtePhyEnb*>(getParentModule()->getParentModule()->getSubmodule("phy"));
        enbTxPower_ = phy->getTxPwr();
        channelModel_ = phy->getChannelModel(carrierFrequency_);
        if (channelModel_ == nullptr)
            throw cRuntimeError("BackgroundTrafficManager::getCqi - cannot find channel model for carrier frequency %f", carrierFrequency_);
    }

    // this is a fictitious frame that needs to compute the SINR
    LteAirFrame *frame = new LteAirFrame("bgUeSinrComputationFrame");
    UserControlInfo *cInfo = new UserControlInfo();

    // build a control info
    cInfo->setSourceId(BGUE_MIN_ID);  // unique ID for bgUes
    cInfo->setDestId(mac_->getMacNodeId());  // ID of the e/gNodeB
    cInfo->setFrameType(FEEDBACKPKT);
    cInfo->setCoord(bgUePos);
    cInfo->setDirection(dir);
    if (dir == UL)
        cInfo->setTxPower(bgUeTxPower);
    else
        cInfo->setTxPower(enbTxPower_);

    std::vector<double> snr = channelModel_->getSINR_bgUe(frame, cInfo);

    // free memory
    delete frame;
    delete cInfo;

    // convert the SNR to CQI and compute the mean
    Cqi bandCqi, meanCqi = 0;
    std::vector<double>::iterator it = snr.begin();
    for (; it != snr.end(); ++it)
    {
        // select the CQI in the range [0,15] according to the "position" of
        // the SINR within the range [SINR_MIN, SINR_MAX]


        // TODO implement a lookup table that associates the SINR to a
        //      range of CQI values. Then extract a random number within
        //      that range

        bandCqi = getCqiFromTable(*it);

//        if (dir == UL)
//            std::cout << "\t snr = " << *it << " cqi = " << bandCqi << endl;
//
//        if (*it <= minSinr_)
//            bandCqi = 0;
//        else if (*it >= maxSinr_)
//            bandCqi = 15;
//        else
//        {
//            double range = maxSinr_ - minSinr_;
//            double normalizedSinr = *it - minSinr_;
//            bandCqi = 15 * ceil(normalizedSinr / range);
//        }

        meanCqi += bandCqi;
    }
    meanCqi /= snr.size();
    if(meanCqi < 2)
        meanCqi = 2;

    return meanCqi;
}

TrafficGeneratorBase* BackgroundTrafficManager::getTrafficGenerator(MacNodeId bgUeId)
{
    int index = bgUeId - BGUE_MIN_ID;
    return bgUe_.at(index);
}

std::list<int>::const_iterator BackgroundTrafficManager::getBackloggedUesBegin(Direction dir)
{
    return backloggedBgUes_[dir].begin();
}

std::list<int>::const_iterator BackgroundTrafficManager::getBackloggedUesEnd(Direction dir)
{
    return backloggedBgUes_[dir].end();
}

std::list<int>::const_iterator BackgroundTrafficManager::getWaitingForRacUesBegin()
{
    return waitingForRac_.begin();
}

std::list<int>::const_iterator BackgroundTrafficManager::getWaitingForRacUesEnd()
{
    return waitingForRac_.end();
}

unsigned int BackgroundTrafficManager::getBackloggedUeBuffer(MacNodeId bgUeId, Direction dir)
{
    int index = bgUeId - BGUE_MIN_ID;
    return bgUe_.at(index)->getBufferLength(dir);
}

unsigned int BackgroundTrafficManager::getBackloggedUeBytesPerBlock(MacNodeId bgUeId, Direction dir)
{
    int index = bgUeId - BGUE_MIN_ID;
    Cqi cqi = bgUe_.at(index)->getCqi(dir);

    // get bytes per block based on CQI
    return (mac_->getAmc()->computeBitsPerRbBackground(cqi, dir, carrierFrequency_) / 8);
}

unsigned int BackgroundTrafficManager::consumeBackloggedUeBytes(MacNodeId bgUeId, unsigned int bytes, Direction dir)
{
    int index = bgUeId - BGUE_MIN_ID;
    int newBuffLen = bgUe_.at(index)->consumeBytes(bytes, dir);

    if (newBuffLen == 0)  // bg UE is no longer active
        backloggedBgUes_[dir].remove(index);

    return newBuffLen;
}

void BackgroundTrafficManager::racHandled(MacNodeId bgUeId)
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

        double offset = mac_->getTtiPeriod() * 6;  // TODO make it configurable
                                                   //      there are 6 slots between the first BSR and actual data

        scheduleAt(NOW + offset, notification);
    }
}
