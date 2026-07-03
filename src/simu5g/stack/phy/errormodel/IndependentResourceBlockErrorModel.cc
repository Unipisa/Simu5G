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

#include "simu5g/stack/phy/errormodel/IndependentResourceBlockErrorModel.h"

namespace simu5g {

Define_Module(IndependentResourceBlockErrorModel);

double IndependentResourceBlockErrorModel::computePacketErrorRate(LteAirFrame *frame, UserControlInfo *lteInfo, const std::vector<double>& snrVector, LteChannelModel *channelModel, const ReceptionParams& params, bool useD2DMulticastThreshold, double& sumSnr, int& usedRBs, bool& forcedFailure) const
{
    RbMap rbmap = lteInfo->getGrantedBlocks();

    double blockErrorRate = 0.0;
    double cumulativeSuccessProbability = 1.0;
    sumSnr = 0.0;
    usedRBs = 0;

    for (const auto& [remoteUnit, rbList] : rbmap) {
        for (const auto& [band, allocation] : rbList) {
            if (allocation == 0)
                continue;

            if (params.cqi == 0 || params.cqi > 15)
                throw cRuntimeError("A packet has been transmitted with a cqi equal to 0 or greater than 15 cqi:%d txmode:%d dir:%d rb:%d cw:%d rtx:%d", params.cqi, lteInfo->getTxMode(), params.direction, band, params.codeword, params.transmissionAttempt);

            sumSnr += snrVector[band];
            usedRBs++;

            int snr = snrVector[band];
            int minSnr = useD2DMulticastThreshold ? 1 : binder_->phyPisaData.minSnr();
            if (snr < minSnr) {
                forcedFailure = true;
                return 1.0;
            }

            blockErrorRate = getBler(params.txModeIndex, params.cqi, snr);

            EV << "\t bler computation: [itxMode=" << params.txModeIndex << "] - [cqi-1=" << params.cqi - 1
               << "] - [snr=" << snr << "]" << endl;

            double blockSuccessRate = 1.0 - blockErrorRate;
            double allocationSuccessProbability = pow(blockSuccessRate, (double)allocation);
            cumulativeSuccessProbability *= allocationSuccessProbability;

            EV << " IndependentResourceBlockErrorModel::error direction " << dirToA(params.direction)
               << " node " << params.nodeId << " remote unit " << dasToA(remoteUnit)
               << " Band " << band << " SNR " << snr << " CQI " << params.cqi
               << " BLER " << blockErrorRate << " success probability " << allocationSuccessProbability
               << " total success probability " << cumulativeSuccessProbability << endl;
        }
    }

    return 1.0 - cumulativeSuccessProbability;
}

} //namespace
