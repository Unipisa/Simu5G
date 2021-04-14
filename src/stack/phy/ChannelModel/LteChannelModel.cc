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

//Define_Module(LteChannelModel);

void LteChannelModel::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        binder_ = getBinder();

        unsigned int componentCarrierIndex = par("componentCarrierIndex");
        componentCarrier_ = check_and_cast<ComponentCarrier*>(getModuleByPath("carrierAggregation")->getSubmodule("componentCarrier", componentCarrierIndex));

        numBands_ = componentCarrier_->getNumBands();   // TODO fix this for UEs' channel model (probably it's not used)
        carrierFrequency_ = componentCarrier_->getCarrierFrequency();

        // register the carrier to the cellInfo module and the binder
        cModule* cInfo = getParentModule()->getParentModule()->getSubmodule("cellInfo");
        if (cInfo != NULL)   // cInfo is NULL on UEs
        {
            cellInfo_ = check_and_cast<CellInfo*>(cInfo);
            cellInfo_->registerCarrier(carrierFrequency_, numBands_, componentCarrier_->getNumerologyIndex());
        }
    }
}

std::vector<double> LteChannelModel::getSINR(LteAirFrame *frame, UserControlInfo* lteInfo)
{
   std::vector<double> tmp;
   tmp.push_back(10000);
   return tmp;
}

std::vector<double> LteChannelModel::getRSRP_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord)
{
   std::vector<double> tmp;
   tmp.push_back(10000);
   return tmp;
}

std::vector<double> LteChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord,MacNodeId enbId)
{
   std::vector<double> tmp;
   tmp.push_back(10000);
   return tmp;
}

std::vector<double> LteChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo* lteInfo_1, MacNodeId destId, inet::Coord destCoord,MacNodeId enbId,const std::vector<double>& rsrpVector)
{
   std::vector<double> tmp;
   tmp.push_back(10000);
   return tmp;
}

std::vector<double> LteChannelModel::getSIR(LteAirFrame *frame, UserControlInfo* lteInfo)
{
   std::vector<double> tmp;
   tmp.push_back(10000);
   return tmp;
}

bool LteChannelModel::isError(LteAirFrame *frame, UserControlInfo* lteInfo)
{
   return true;
}

bool LteChannelModel::isError_D2D(LteAirFrame *frame, UserControlInfo* lteInfo,const std::vector<double>& rsrpVector)
{
   return true;
}

bool LteChannelModel::isErrorDas(LteAirFrame *frame, UserControlInfo* lteInfo)
{
   return true;
}
