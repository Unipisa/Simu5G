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

namespace simu5g {

Define_Module(BackgroundTrafficManager);

BackgroundTrafficManager::BackgroundTrafficManager()
{
    channelModel_ = nullptr;
}

void BackgroundTrafficManager::initialize(int stage)
{
    BackgroundTrafficManagerBase::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL)
    {
        phy_.reference(this, "phyModule", true);
    }
    if (stage == inet::INITSTAGE_PHYSICAL_LAYER)
    {
        // get the reference to the MAC layer
        mac_.reference(this, "macModule", true); // TODO mac_ used in BackgroundTrafficManagerBase
    }
    if (stage == inet::INITSTAGE_LAST-1)
    {
        // get the reference to the channel model for the given carrier
        bsTxPower_ = phy_->getTxPwr();
        bsCoord_ = phy_->getCoord();
        channelModel_ = phy_->getChannelModel(carrierFrequency_);
        if (channelModel_ == nullptr)
            throw cRuntimeError("BackgroundTrafficManagerBase::initialize - cannot find channel model for carrier frequency %f", carrierFrequency_);
    }
}

unsigned int BackgroundTrafficManager::getNumBands()
{
    return channelModel_->getNumBands();
}

Cqi BackgroundTrafficManager::computeCqi(int bgUeIndex, Direction dir, inet::Coord bgUePos, double bgUeTxPower)
{
    // this is a fictitious frame that needs to compute the SINR
    LteAirFrame *frame = new LteAirFrame("bgUeSinrComputationFrame");
    UserControlInfo *cInfo = new UserControlInfo();

    // build a control info
    cInfo->setSourceId(BGUE_MIN_ID + bgUeIndex);  // MacNodeId for the bgUe
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

    // free memory
    delete frame;
    delete cInfo;

    // convert the SNR to CQI and compute the mean
    double meanSinr = 0;
    Cqi bandCqi, meanCqi = 0;
    std::vector<double>::iterator it = snr.begin();
    for (; it != snr.end(); ++it)
    {
        meanSinr += *it;

        bandCqi = computeCqiFromSinr(*it);
        meanCqi += bandCqi;
    }

    meanSinr /= snr.size();
    TrafficGeneratorBase* bgUe = bgUe_.at(bgUeIndex);
    bgUe->collectMeasuredSinr(meanSinr, dir);

    meanCqi /= snr.size();
    if(meanCqi < 2)
        meanCqi = 2;

    return meanCqi;
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

