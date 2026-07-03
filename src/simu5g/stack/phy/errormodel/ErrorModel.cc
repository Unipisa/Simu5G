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

#include "simu5g/stack/phy/errormodel/ErrorModel.h"

#include "simu5g/common/binder/Binder.h"
#include "simu5g/stack/mac/amc/UserTxParams.h"
#include "simu5g/stack/phy/LtePhyUe.h"
#include "simu5g/stack/phy/channelmodel/LteChannelModel.h"

namespace simu5g {

Define_Module(ErrorModel);

simsignal_t ErrorModel::rcvdSinrDlSignal_ = registerSignal("rcvdSinrDl");
simsignal_t ErrorModel::rcvdSinrUlSignal_ = registerSignal("rcvdSinrUl");
simsignal_t ErrorModel::rcvdSinrD2DSignal_ = registerSignal("rcvdSinrD2D");

void ErrorModel::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
        harqReduction_ = par("harqReduction");
    }
    else if (stage == INITSTAGE_SIMU5G_POSTLOCAL) {
        binder_.reference(this, "binderModule", true);
    }
}

ErrorModel::ReceptionParams ErrorModel::extractReceptionParams(UserControlInfo *lteInfo) const
{
    ReceptionParams params;
    params.codeword = lteInfo->getCw();
    params.numCodewords = lteInfo->getUserTxParams()->readCqiVector().size();

    if (params.numCodewords == 1)
        params.codeword = 0;

    params.cqi = lteInfo->getUserTxParams()->readCqiVector()[params.codeword];
    params.direction = (Direction)lteInfo->getDirection();
    params.nodeId = (params.direction == DL) ? lteInfo->getDestId() : lteInfo->getSourceId();
    params.transmissionAttempt = lteInfo->getTxNumber();
    params.txMode = (TxMode)lteInfo->getTxMode();
    params.txModeIndex = txModeToIndex[params.txMode];

    if (params.transmissionAttempt == 0)
        throw cRuntimeError("Transmissions counter should not be 0");

    return params;
}

double ErrorModel::getBler(unsigned int txModeIndex, Cqi cqi, int snr) const
{
    if (snr < binder_->phyPisaData.minSnr())
        return 1.0;
    if (snr > binder_->phyPisaData.maxSnr())
        return 0.0;
    return binder_->phyPisaData.getBler(txModeIndex, cqi - 1, snr);
}

bool ErrorModel::drawReceptionResult(double packetErrorRate, const ReceptionParams& params) const
{
    double effectiveErrorRateWithHarq = packetErrorRate * pow(harqReduction_, params.transmissionAttempt - 1);
    double randomSample = uniform(0.0, 1.0);

    EV << " ErrorModel::error direction " << dirToA(params.direction)
       << " node " << params.nodeId << " total ERROR probability  " << packetErrorRate
       << " per with H-ARQ error reduction " << effectiveErrorRateWithHarq
       << " - CQI[" << params.cqi << "]- random error extracted[" << randomSample << "]" << endl;

    bool receptionFailed = (randomSample <= effectiveErrorRateWithHarq);
    if (receptionFailed) {
        EV << "This is NOT your lucky day (" << randomSample << " < " << effectiveErrorRateWithHarq
           << ") -> do not receive." << endl;
        return false;
    }

    EV << "This is your lucky day (" << randomSample << " > " << effectiveErrorRateWithHarq
       << ") -> Receive AirFrame." << endl;
    return true;
}

void ErrorModel::emitSinrStatistics(LteChannelModel *channelModel, UserControlInfo *lteInfo, const ReceptionParams& params, double sumSnr, int usedRBs)
{
    if (!channelModel->collectSinrStatistics() || usedRBs == 0)
        return;

    if (params.direction == DL)
        channelModel->emit(rcvdSinrDlSignal_, sumSnr / usedRBs);
    else if (params.direction == D2D || params.direction == D2D_MULTI)
        channelModel->emit(rcvdSinrD2DSignal_, sumSnr / usedRBs);
    else {
        LteChannelModel *ueChannelModel = check_and_cast<LtePhyUe *>(binder_->getPhyByNodeId(params.nodeId))->getChannelModel(lteInfo->getCarrierFrequency());
        ueChannelModel->emit(rcvdSinrUlSignal_, sumSnr / usedRBs);
    }
}

double ErrorModel::computePacketErrorRate(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& snrVector, LteChannelModel *channelModel, const ReceptionParams& params, double& sumSnr, int& usedRBs) const
{
    throw cRuntimeError("ErrorModel::computePacketErrorRate - missing concrete error model implementation");
}

bool ErrorModel::isReceptionSuccessful(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& snrVector, LteChannelModel *channelModel)
{
    EV << "ErrorModel::isReceptionSuccessful" << endl;

    ReceptionParams params = extractReceptionParams(lteInfo);

    double sumSnr = 0.0;
    int usedRBs = 0;
    double packetErrorRate = computePacketErrorRate(frame, lteInfo, snrVector, channelModel, params, sumSnr, usedRBs);

    emitSinrStatistics(channelModel, lteInfo, params, sumSnr, usedRBs);
    return drawReceptionResult(packetErrorRate, params);
}

} //namespace
