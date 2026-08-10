//
//                  Simu5G
//
// Copyright (C) 2012-2021 Giovanni Nardini, Giovanni Stea, Antonio Virdis et al. (University of Pisa)
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "LteDummyChannelModel.h"

namespace simu5g {

using namespace omnetpp;

Define_Module(LteDummyChannelModel);

// Reported on every band, in place of a computed value. Large enough that the AMC
// always picks the highest CQI, so the transport block size never varies with the
// channel and the only source of loss is the configured error rate.
static const double FAKE_SINR_DB = 10000;

void LteDummyChannelModel::initialize(int stage)
{
    LteChannelModel::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        perDl_ = &par("perDl");
        perUl_ = &par("perUl");
        perD2D_ = &par("perD2D");
        harqReduction_ = par("harqReduction");
    }
}

double LteDummyChannelModel::getErrorProbability(Direction dir, unsigned char txNumber) const
{
    if (txNumber == 0)
        throw cRuntimeError("LteDummyChannelModel::getErrorProbability(): transmission number must be at least 1");

    cPar *per;
    switch (dir) {
        case DL: per = perDl_; break;
        case UL: per = perUl_; break;
        case D2D:
        case D2D_MULTI: per = perD2D_; break;
        default:
            throw cRuntimeError("LteDummyChannelModel::getErrorProbability(): unexpected direction %d", (int)dir);
    }
    return per->doubleValue() * pow(harqReduction_, txNumber - 1);
}

std::vector<double> LteDummyChannelModel::getSINR(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    std::vector<double> tmp(numBands_, FAKE_SINR_DB);
    // fake SINR is needed by the handover function to decide if the terminal should trigger the handover
    return tmp;
}

std::vector<double> LteDummyChannelModel::computeReceptionSinr(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    return getSINR(frame, lteInfo);
}

std::vector<double> LteDummyChannelModel::getRSRP(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    std::vector<double> tmp(numBands_, FAKE_SINR_DB);
    // fake RSRP is needed by the handover function to decide if the terminal should trigger the handover
    return tmp;
}

std::vector<double> LteDummyChannelModel::computeReceptionRsrp(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    return getRSRP(frame, lteInfo);
}

std::vector<double> LteDummyChannelModel::getSINR_bgUe(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    std::vector<double> tmp(numBands_, FAKE_SINR_DB);
    // fake SINR is needed by the handover function to decide if the terminal should trigger the handover
    return tmp;
}

double LteDummyChannelModel::getReceivedPower_bgUe(double txPower, inet::Coord txPos, inet::Coord rxPos, Direction dir, bool losStatus, MacNodeId bsId)
{
    return 10000.0;
}

std::vector<double> LteDummyChannelModel::getRSRP_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord)
{
    std::vector<double> tmp(numBands_, FAKE_SINR_DB);
    return tmp;
}

std::vector<double> LteDummyChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord, MacNodeId enbId)
{
    std::vector<double> tmp(numBands_, FAKE_SINR_DB);
    // fake SINR is needed by the handover function to decide if the terminal should trigger the handover
    return tmp;
}

std::vector<double> LteDummyChannelModel::getSINR_D2D(LteAirFrame *frame, UserControlInfo *lteInfo_1, MacNodeId destId, inet::Coord destCoord, MacNodeId enbId, const std::vector<double>& rsrpVector)
{
    std::vector<double> tmp(numBands_, FAKE_SINR_DB);
    // fake SINR is needed by the handover function to decide if the terminal should trigger the handover
    return tmp;
}

std::vector<double> LteDummyChannelModel::getSIR(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    std::vector<double> tmp(numBands_, FAKE_SINR_DB);
    // fake SIR is needed by the handover function to decide if the terminal should trigger the handover
    return tmp;
}

bool LteDummyChannelModel::isReceptionSuccessful(LteAirFrame *frame, UserControlInfo *lteInfo)
{
    double per = getErrorProbability(lteInfo->getDirection(), lteInfo->getTxNumber());
    bool success = uniform(0.0, 1.0) > per;
    EV << "LteDummyChannelModel::isReceptionSuccessful - transmission " << (int)lteInfo->getTxNumber()
       << ", error probability " << per << " -> " << (success ? "received" : "lost") << endl;
    return success;
}

bool LteDummyChannelModel::isReceptionSuccessful_D2D(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& rsrpVector)
{
    double per = getErrorProbability(lteInfo->getDirection(), lteInfo->getTxNumber());
    bool success = uniform(0.0, 1.0) > per;
    EV << "LteDummyChannelModel::isReceptionSuccessful_D2D - transmission " << (int)lteInfo->getTxNumber()
       << ", error probability " << per << " -> " << (success ? "received" : "lost") << endl;
    return success;
}

} //namespace
