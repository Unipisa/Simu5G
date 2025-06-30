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

#include "stack/phy/ChannelModel/LteChannelModel.h"

namespace simu5g {

//Define_Module(LteChannelModel);

void LteChannelModel::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        binder_.reference(this, "binderModule", true);

        componentCarrier_.reference(this, "componentCarrierModule", true);

        numBands_ = componentCarrier_->getNumBands();   // TODO fix this for UEs' channel model (probably it's not used)
        carrierFrequency_ = componentCarrier_->getCarrierFrequency();

        // register the carrier to the cellInfo module and the binder
        cellInfo_.reference(this, "cellInfoModule", false);
        if (cellInfo_ != nullptr) { // cInfo is NULL on UEs
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

bool LteChannelModel::isError(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    return true;
}

bool LteChannelModel::isError_D2D(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector)
{
    return true;
}

bool LteChannelModel::isErrorDas(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    return true;
}

} //namespace

