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

//#include <math.h>
#include <cmath>
#include "simu5g/stack/phy/feedback/LteFeedbackComputationRealistic.h"
#include "simu5g/common/blerCurves/PhyPisaData.h"
#include "simu5g/common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;


LteFeedbackComputationRealistic::LteFeedbackComputationRealistic(Binder *binder, double targetBler, unsigned int numBands) : targetBler_(targetBler), numBands_(numBands), phyPisaData_(&(binder->phyPisaData))
{
    baseMin_.resize(phyPisaData_->nMcs(), 2);
}


void LteFeedbackComputationRealistic::generateBaseFeedback(int numBands, int numPreferredBands, LteFeedback& fb,
        FeedbackType fbType, int cw, RbAllocationType rbAllocationType, TxMode txmode, std::vector<double> snr)
{
    int layer = 1;
    std::vector<CqiVector> cqiTmp2;
    CqiVector cqiTmp;
    layer = cw < layer ? cw : layer;

    Cqi cqi;
    double mean = 0;
    if (fbType == WIDEBAND || rbAllocationType == TYPE2_DISTRIBUTED)
        mean = meanSnr(snr);
    if (fbType == WIDEBAND) {
        cqiTmp.resize(layer, 0);
        cqi = getCqi(txmode, mean);
        for (unsigned short & i : cqiTmp)
            i = cqi;
        fb.setWideBandCqi(cqiTmp);
    }
    else if (fbType == ALLBANDS) {
        if (rbAllocationType == TYPE2_LOCALIZED) {
            cqiTmp2.resize(layer);
            for (unsigned int i = 0; i < cqiTmp2.size(); i++) {
                cqiTmp2[i].resize(numBands, 0);
                for (int j = 0; j < numBands; j++) {
                    cqi = getCqi(txmode, snr[j]);
                    cqiTmp2[i][j] = cqi;
                }
                fb.setPerBandCqi(cqiTmp2[i], i);
            }
        }
        else if (rbAllocationType == TYPE2_DISTRIBUTED) {
            cqi = getCqi(txmode, mean);
            cqiTmp2.resize(layer);
            for (unsigned int i = 0; i < cqiTmp2.size(); i++) {
                cqiTmp2[i].resize(numBands, 0);
                for (int j = 0; j < numBands; j++) {
                    cqiTmp2[i][j] = cqi;
                }
                fb.setPerBandCqi(cqiTmp2[i], i);
            }
        }
    }
    else if (fbType == PREFERRED) {
        //TODO
    }
}

unsigned int LteFeedbackComputationRealistic::computeRank(MacNodeId id)
{
    return 1;
}

Cqi LteFeedbackComputationRealistic::getCqi(TxMode txmode, double snr)
{
    int newsnr = floor(snr + 0.5);
    if (newsnr < phyPisaData_->minSnr())
        return 0;
    if (newsnr > phyPisaData_->maxSnr())
        return 15;
    unsigned int txm = txModeToIndex[txmode];
    std::vector<double> min;
    int found = 0;
    double low = 2;

    min = baseMin_;
    for (int i = 0; i < phyPisaData_->nMcs(); i++) {
        double tmp = phyPisaData_->getBler(txm, i, newsnr);
        double diff = targetBler_ - tmp;
        min[i] = (diff > 0) ? diff : (diff * -1);
        if (low >= min[i]) {
            found = i;
            low = min[i];
        }
    }
    return found + 1;
}

LteFeedbackDoubleVector LteFeedbackComputationRealistic::computeFeedback(FeedbackType fbType,
        RbAllocationType rbAllocationType, TxMode currentTxMode,
        std::map<Remote, int> antennaCws, int numPreferredBands, int numRus,
        std::vector<double> snr, MacNodeId id)
{
    // Add enodeB to the number of antennas
    numRus++;
    LteFeedbackDoubleVector fbvv;
    fbvv.resize(numRus);
    for (int i = 0; i < numRus; i++)
        fbvv[i].resize(DL_NUM_TXMODE);

    // For each Remote
    for (int j = 0; j < numRus; j++) {
        LteFeedback fb;
        // For each txmode we generate a feedback excluding MU_MIMO because it is treated as SISO
        for (int z = 0; z < DL_NUM_TXMODE - 1; z++) {
            fb.reset();
            fb.setTxMode((TxMode)z);
            unsigned int rank = 1;
            if (z == TRANSMIT_DIVERSITY || z == SINGLE_ANTENNA_PORT0) {
                // Generate feedback for txmode z
                fb.setRankIndicator(rank);
                fb.setAntenna((Remote)j);
                generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws[(Remote)j], rbAllocationType, (TxMode)z, snr);
            }
            fbvv[j][z] = fb;
        }
    }
    return fbvv;
}

LteFeedbackVector LteFeedbackComputationRealistic::computeFeedback(const Remote remote, FeedbackType fbType,
        RbAllocationType rbAllocationType, TxMode currentTxMode,
        int antennaCws, int numPreferredBands, int numRus,
        std::vector<double> snr, MacNodeId id)
{
    LteFeedbackVector fbv;
    fbv.resize(DL_NUM_TXMODE);

    // For each txmode we generate a feedback
    for (int z = 0; z < DL_NUM_TXMODE; z++) {
        LteFeedback fb;
        fb.setTxMode((TxMode)z);
        unsigned int rank = 1;
        if (z == TRANSMIT_DIVERSITY || z == SINGLE_ANTENNA_PORT0) {
            // Generate feedback for txmode z
            fb.setRankIndicator(rank);
            fb.setAntenna(remote);
            generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws, rbAllocationType, (TxMode)z, snr);
        }
        fbv[z] = fb;
    }
    return fbv;
}

LteFeedback LteFeedbackComputationRealistic::computeFeedback(const Remote remote, TxMode txmode, FeedbackType fbType,
        RbAllocationType rbAllocationType,
        int antennaCws, int numPreferredBands, int numRus,
        std::vector<double> snr, MacNodeId id)
{
    // New Feedback
    LteFeedback fb;
    unsigned int rank = 1;
    fb.setRankIndicator(rank);

    // Set the remote in the feedback object
    fb.setAntenna(remote);
    fb.setTxMode(txmode);
    generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws, rbAllocationType, txmode, snr);
    return fb;
}

double LteFeedbackComputationRealistic::meanSnr(std::vector<double> snr)
{
    double mean = 0;
    for (const auto& value : snr)
        mean += value;
    mean /= snr.size();
    return mean;
}

} //namespace

