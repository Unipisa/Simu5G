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

#include "simu5g/stack/phy/errormodel/MiesmErrorModel.h"

#include <algorithm>
#include <cmath>

#include "simu5g/common/binder/Binder.h"

namespace simu5g {

Define_Module(MiesmErrorModel);

void MiesmErrorModel::initialize(int stage)
{
    ErrorModel::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {
        beta_ = par("beta");
        useLambdaCalibration_ = par("useLambdaCalibration");
    }
}

double MiesmErrorModel::computePacketErrorRate(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& snrVector, LteChannelModel *channelModel, const ReceptionParams& params, bool useD2DMulticastThreshold, bool& forcedFailure) const
{
    if (params.cqi == 0 || params.cqi > 15)
        throw cRuntimeError("A packet has been transmitted with a cqi equal to 0 or greater than 15 cqi:%d txmode:%d dir:%d cw:%d rtx:%d", params.cqi, lteInfo->getTxMode(), params.direction, params.codeword, params.transmissionAttempt);

    double beta = beta_;
    if (useLambdaCalibration_) {
        double calibratedBeta = binder_->phyPisaData.getLambda(params.cqi - 1, std::min(params.txModeIndex, 2U));
        if (calibratedBeta > 0.0)
            beta = calibratedBeta;
    }

    if (beta <= 0.0)
        throw cRuntimeError("MiesmErrorModel::computePacketErrorRate - beta must be positive");

    double mutualInformationSum = 0.0;
    int totalAllocation = 0;
    for (const auto& [remoteUnit, rbList] : lteInfo->getGrantedBlocks()) {
        for (const auto& [band, allocation] : rbList) {
            if (allocation == 0)
                continue;

            if (params.cqi == 0 || params.cqi > 15)
                throw cRuntimeError("A packet has been transmitted with a cqi equal to 0 or greater than 15 cqi:%d txmode:%d dir:%d rb:%d cw:%d rtx:%d", params.cqi, lteInfo->getTxMode(), params.direction, band, params.codeword, params.transmissionAttempt);

            int snr = snrVector[band];
            int minSnr = useD2DMulticastThreshold ? 1 : binder_->phyPisaData.minSnr();
            if (snr < minSnr) {
                forcedFailure = true;
                return 1.0;
            }

            double linearSnr = pow(10.0, snr / 10.0);
            double mutualInformation = log2(1.0 + linearSnr / beta);
            mutualInformationSum += allocation * mutualInformation;
            totalAllocation += allocation;

            EV << " MiesmErrorModel::error direction " << dirToA(params.direction)
               << " node " << params.nodeId << " remote unit " << dasToA(remoteUnit)
               << " Band " << band << " SNR " << snr << " CQI " << params.cqi
               << " allocation " << allocation << endl;
        }
    }

    if (totalAllocation == 0)
        return 0.0;

    double effectiveMutualInformation = mutualInformationSum / totalAllocation;
    double effectiveLinearSnr = std::max(beta * (pow(2.0, effectiveMutualInformation) - 1.0), 1e-12);
    int effectiveSnr = (int)round(10.0 * log10(effectiveLinearSnr));
    double blockErrorRate = getBler(params.txModeIndex, params.cqi, effectiveSnr);

    EV << " MiesmErrorModel::error direction " << dirToA(params.direction)
       << " node " << params.nodeId << " effective SNR " << effectiveSnr
       << " CQI " << params.cqi << " BLER " << blockErrorRate << endl;

    return blockErrorRate;
}

} //namespace
