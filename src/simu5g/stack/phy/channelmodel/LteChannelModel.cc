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

#include "simu5g/stack/phy/channelmodel/LteChannelModel.h"

namespace simu5g {


void LteChannelModel::initialize(int stage)
{
    if (stage == INITSTAGE_SIMU5G_POSTLOCAL) {
        binder_.reference(this, "binderModule", true);

        componentCarrier_.reference(this, "componentCarrierModule", true);

        numBands_ = componentCarrier_->getNumBands();   // TODO fix this for UEs' channel model (probably it's not used)
        carrierFrequency_ = componentCarrier_->getCarrierFrequency();
        carrierFrequencyGHz_ = GHz(carrierFrequency_).get();
        carrierFrequencyHz_ = Hz(carrierFrequency_).get();
        log10CarrierFrequencyGHz_ = log10(carrierFrequencyGHz_);
    }
    if (stage == INITSTAGE_SIMU5G_REGISTRATIONS) {
        // register the carrier to the cellInfo module and the binder
        cellInfo_.reference(this, "cellInfoModule", false);
        if (cellInfo_ != nullptr) { // cellInfo is NULL on UEs
            cellInfo_->registerCarrier(carrierFrequency_, numBands_, componentCarrier_->getNumerologyIndex());
        }
    }
}

std::vector<double> LteChannelModel::getSINR(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    static const std::vector<double> tmp { 10000.0 };
    return tmp;
}

std::vector<double> LteChannelModel::getRSRP(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    static const std::vector<double> tmp { 10000.0 };
    return tmp;
}

std::vector<double> LteChannelModel::getRSRP_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord)
{
    static const std::vector<double> tmp { 10000.0 };
    return tmp;
}

std::vector<double> LteChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord, MacNodeId enbId)
{
    static const std::vector<double> tmp { 10000.0 };
    return tmp;
}

std::vector<double> LteChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord, MacNodeId enbId, const std::vector<double>& rsrpVector)
{
    static const std::vector<double> tmp { 10000.0 };
    return tmp;
}

std::vector<double> LteChannelModel::getSIR(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    static const std::vector<double> tmp { 10000.0 };
    return tmp;
}

bool LteChannelModel::isReceptionSuccessful_D2D(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector)
{
    return true;
}

} //namespace

