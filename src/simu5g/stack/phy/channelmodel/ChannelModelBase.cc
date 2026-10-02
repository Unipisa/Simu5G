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

#include "simu5g/stack/phy/channelmodel/ChannelModelBase.h"

namespace simu5g {


void ChannelModelBase::initialize(int stage)
{
    if (stage == INITSTAGE_SIMU5G_POSTLOCAL) {
        binder_.reference(this, "binderModule", true);
        radioMedium_.reference(this, "radioMediumModule", true);

        componentCarrier_.reference(this, "componentCarrierModule", true);

        numBands_ = componentCarrier_->getNumBands();   // TODO fix this for UEs' channel model (probably it's not used)
        carrierFrequency_ = componentCarrier_->getCarrierFrequency();
        carrierFrequencyGHz_ = GHz(carrierFrequency_).get();
        carrierFrequencyHz_ = Hz(carrierFrequency_).get();
        log10CarrierFrequencyGHz_ = log10(carrierFrequencyGHz_);
    }
}

std::vector<double> ChannelModelBase::getSINR(AirFrame *frame, UserControlInfo *lteInfo)
{
    static const std::vector<double> tmp { 10000.0 };
    return tmp;
}

std::vector<double> ChannelModelBase::getRSRP(AirFrame *frame, UserControlInfo *lteInfo)
{
    static const std::vector<double> tmp { 10000.0 };
    return tmp;
}

} //namespace

