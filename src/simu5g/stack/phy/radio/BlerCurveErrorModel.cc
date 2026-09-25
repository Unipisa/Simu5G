//
//                  Simu5G
//
// Copyright (C) 2022-2026 Giovanni Nardini, Giovanni Stea et al. (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "simu5g/stack/phy/radio/BlerCurveErrorModel.h"

#include "simu5g/common/binder/Binder.h"

namespace simu5g {

Define_Module(BlerCurveErrorModel);

void BlerCurveErrorModel::initialize()
{
    binder_.reference(this, "binderModule", true);
    harqReduction_ = par("harqReduction");
}

std::optional<double> BlerCurveErrorModel::computePacketErrorRate(Cqi cqi, const std::vector<double>& sinrPerBand, const RbMap& grantedBlocks, unsigned char transmissionAttempt) const
{
    PhyPisaData& curves = binder_->phyPisaData;
    double cumulativeSuccessProbability = 1.0;
    // for each remote unit and each logical band used to transmit the packet
    for (const auto& [remoteUnit, rbList] : grantedBlocks) {
        for (const auto& [band, allocation] : rbList) {
            if (allocation == 0)
                continue;
            if (cqi == 0)
                return std::nullopt; // CQI 0 means channel below usable quality (e.g. after handover): loss

            int snr = sinrPerBand[band];
            double blockErrorRate;
            if (snr < curves.minSnr())
                return std::nullopt;
            else if (snr > curves.maxSnr())
                blockErrorRate = 0.0;
            else
                blockErrorRate = curves.getBler(cqi, snr);

            // the success probability according to the number of RBs used on the band
            double allocationSuccessProbability = pow(1.0 - blockErrorRate, (double)allocation);
            cumulativeSuccessProbability *= allocationSuccessProbability;

            EV << "BlerCurveErrorModel: remote unit " << dasToA(remoteUnit) << " band " << band << " SNR " << snr
               << " CQI " << cqi << " BLER " << blockErrorRate << " success probability " << allocationSuccessProbability
               << " total success probability " << cumulativeSuccessProbability << endl;
        }
    }
    double packetErrorRate = 1.0 - cumulativeSuccessProbability;
    // HARQ soft combining gain
    double effectiveErrorRate = packetErrorRate * pow(harqReduction_, transmissionAttempt - 1);
    EV << "BlerCurveErrorModel: packet error rate " << packetErrorRate << ", with the H-ARQ reduction " << effectiveErrorRate << endl;
    return effectiveErrorRate;
}

} // namespace simu5g
