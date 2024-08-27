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

#include "stack/backgroundTrafficGenerator/BackgroundTrafficManagerBase.h"
#include "stack/backgroundTrafficGenerator/ActiveUeNotification_m.h"
#include "stack/phy/ChannelModel/LteChannelModel.h"

namespace simu5g {

const double BackgroundTrafficManagerBase::nrCqiTable[16] = {
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

double BackgroundTrafficManagerBase::getCqiFromTable(double snr)
{
    for (unsigned int i = 0; i < 16; i++) {
        if (snr < nrCqiTable[i])
            return i - 1;
    }
    return 15;
}


void BackgroundTrafficManagerBase::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        numBgUEs_ = par("numBgUes");
        binder_.reference(this, "binderModule", true);
    }
    if (stage == inet::INITSTAGE_PHYSICAL_ENVIRONMENT) {
        // create vector of BackgroundUEs
        for (int i = 0; i < numBgUEs_; i++)
            bgUe_.push_back(check_and_cast<TrafficGeneratorBase *>(getParentModule()->getSubmodule("bgUE", i)->getSubmodule("generator")));

        phyPisaData_ = &(binder_->phyPisaData);
    }
    if (stage == inet::INITSTAGE_LAST - 1) {
        BgTrafficManagerInfo *info = new BgTrafficManagerInfo();
        info->init = false;
        info->bgTrafficManager = this;
        info->carrierFrequency = carrierFrequency_;
        info->allocatedRbsDl = 0.0;
        info->allocatedRbsUl = 0.0;
        info->allocatedRbsUeUl.resize(numBgUEs_, 0.0);

        if (isSetBgTrafficManagerInfoInit()) {
            initializeAvgInterferenceComputation();
            info->init = true;
        }

        binder_->addBgTrafficManagerInfo(info);
    }
}

void BackgroundTrafficManagerBase::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) { // this is an activeUeNotification message
        ActiveUeNotification *notification = check_and_cast<ActiveUeNotification *>(msg);

        // add the bg UE to the list of active UEs in x slots
        backloggedBgUes_[UL].push_back(notification->getIndex());

        delete notification;
    }
}

void BackgroundTrafficManagerBase::notifyBacklog(int index, Direction dir, bool rtx)
{
    if (dir != DL && dir != UL)
        throw cRuntimeError("TrafficGeneratorBase::consumeBytes - unrecognized direction: %d", dir);

    if (!rtx) {
        if (dir == UL)
            waitingForRac_.push_back(index);
        else
            backloggedBgUes_[DL].push_back(index);
    }
    else {
        backloggedRtxBgUes_[dir].push_back(index);
    }
}

Cqi BackgroundTrafficManagerBase::computeCqi(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower)
{
    std::vector<double> snr = getSINR(bgUeIndex, dir, bgUePos, bgUeTxPower);

    // convert the SNR to CQI and compute the mean
    double meanSinr = 0;
    Cqi bandCqi, meanCqi = 0;

    for (const auto& value : snr) {
        meanSinr += value;

        // lookup table that associates the SINR to a range of CQI values
        bandCqi = computeCqiFromSinr(value);

        meanCqi += bandCqi;
    }

    meanSinr /= snr.size();
    TrafficGeneratorBase *bgUe = bgUe_.at(bgUeIndex);
    bgUe->collectMeasuredSinr(meanSinr, dir);

    meanCqi /= snr.size();
    if (meanCqi < 2)
        meanCqi = 2;

    return meanCqi;
}

Cqi BackgroundTrafficManagerBase::computeCqiFromSinr(double sinr)
{
    int newsnr = floor(sinr + 0.5);
    if (newsnr < phyPisaData_->minSnr())
        return 0;
    if (newsnr > phyPisaData_->maxSnr())
        return 15;

    unsigned int txm = 1;
    std::vector<double> min;
    int found = 0;
    double low = 2;

    // TODO do this once in the initialize
    min.resize(phyPisaData_->nMcs(), 2);

    double targetBler = 0.01; // TODO get this from parameters

    for (int i = 0; i < phyPisaData_->nMcs(); i++) {
        double tmp = phyPisaData_->getBler(txm, i, newsnr);
        double diff = targetBler - tmp;
        min[i] = (diff > 0) ? diff : (diff * -1);
        if (low >= min[i]) {
            found = i;
            low = min[i];
        }
    }
    return found + 1;
}

TrafficGeneratorBase *BackgroundTrafficManagerBase::getTrafficGenerator(MacNodeId bgUeId)
{
    int index = bgUeId - BGUE_MIN_ID;
    return bgUe_.at(index);
}

std::vector<TrafficGeneratorBase *>::const_iterator BackgroundTrafficManagerBase::getBgUesBegin()
{
    return bgUe_.begin();
}

std::vector<TrafficGeneratorBase *>::const_iterator BackgroundTrafficManagerBase::getBgUesEnd()
{
    return bgUe_.end();
}

std::list<int>::const_iterator BackgroundTrafficManagerBase::getBackloggedUesBegin(Direction dir, bool rtx)
{
    if (!rtx)
        return backloggedBgUes_[dir].begin();
    else
        return backloggedRtxBgUes_[dir].begin();
}

std::list<int>::const_iterator BackgroundTrafficManagerBase::getBackloggedUesEnd(Direction dir, bool rtx)
{
    if (!rtx)
        return backloggedBgUes_[dir].end();
    else
        return backloggedRtxBgUes_[dir].end();
}

std::list<int>::const_iterator BackgroundTrafficManagerBase::getWaitingForRacUesBegin()
{
    return waitingForRac_.begin();
}

std::list<int>::const_iterator BackgroundTrafficManagerBase::getWaitingForRacUesEnd()
{
    return waitingForRac_.end();
}

unsigned int BackgroundTrafficManagerBase::getBackloggedUeBuffer(MacNodeId bgUeId, Direction dir, bool rtx)
{
    int index = bgUeId - BGUE_MIN_ID;
    return bgUe_.at(index)->getBufferLength(dir, rtx);
}

unsigned int BackgroundTrafficManagerBase::consumeBackloggedUeBytes(MacNodeId bgUeId, unsigned int bytes, Direction dir, bool rtx)
{
    int index = bgUeId - BGUE_MIN_ID;
    int newBuffLen = bgUe_.at(index)->consumeBytes(bytes, dir, rtx);

    if (newBuffLen == 0) { // bg UE is no longer active
        if (!rtx)
            backloggedBgUes_[dir].remove(index);
        else
            backloggedRtxBgUes_[dir].remove(index);
    }
    return newBuffLen;
}

void BackgroundTrafficManagerBase::racHandled(MacNodeId bgUeId)
{
    Enter_Method("BackgroundTrafficManagerBase::racHandled");

    int index = bgUeId - BGUE_MIN_ID;

    waitingForRac_.remove(index);

    // some bytes have been added in the RB assigned for the first BSR, consume them from the buffer
    // TODO consider MAC and RLC header?
    unsigned int servedWithFirstBsr = getBackloggedUeBytesPerBlock(bgUeId, UL);
    unsigned int newBuffLen = consumeBackloggedUeBytes(bgUeId, servedWithFirstBsr, UL);

    // if there are still data in the buffer
    if (newBuffLen > 0) {
        ActiveUeNotification *notification = new ActiveUeNotification("activeUeNotification");
        notification->setIndex(index);

        double offset = getTtiPeriod() * 6;  // TODO make it configurable
                                             //      there are 6 slots between the first BSR and actual data
        scheduleAt(NOW + offset, notification);
    }
}

void BackgroundTrafficManagerBase::initializeAvgInterferenceComputation()
{
    avgCellLoad_ = 0;

    // get avg load for all the UEs and for the cell
    for (int i = 0; i < numBgUEs_; i++) {
        avgCellLoad_ += bgUe_.at(i)->getAvgLoad(DL);
        avgUeLoad_.push_back(bgUe_.at(i)->getAvgLoad(UL));
    }
}

} //namespace

