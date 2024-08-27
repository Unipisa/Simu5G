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

//#include <math.h>
#include <cmath>
#include "stack/phy/feedback/LteFeedbackComputationRealistic.h"
#include "common/blerCurves/PhyPisaData.h"
#include "common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

LteFeedbackComputationRealistic::LteFeedbackComputationRealistic(Binder *binder, double targetBler, std::map<MacNodeId, Lambda> *lambda,
        double lambdaMinTh, double lambdaMaxTh, double lambdaRatioTh, unsigned int numBands) : lambda_(lambda), targetBler_(targetBler), numBands_(numBands), lambdaMinTh_(lambdaMinTh), lambdaMaxTh_(lambdaMaxTh), lambdaRatioTh_(lambdaRatioTh), phyPisaData_(&(binder->phyPisaData))
{
    baseMin_.resize(phyPisaData_->nMcs(), 2);
}


void LteFeedbackComputationRealistic::generateBaseFeedback(int numBands, int numPreferredBands, LteFeedback& fb,
        FeedbackType fbType, int cw, RbAllocationType rbAllocationType, TxMode txmode, std::vector<double> snr)
{
    int layer = 1;
    std::vector<CqiVector> cqiTmp2;
    CqiVector cqiTmp;
    if (txmode == OL_SPATIAL_MULTIPLEXING) {
        // If rank is 1, SMUX is not a valid choice as Tx Mode
        if (fb.getRankIndicator() < 2)
            return;
        layer = fb.getRankIndicator();
    }
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
    if (lambda_->at(id).lambdaMin < lambdaMinTh_)
        return 1;
    else
        return 2;
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
        std::map<Remote, int> antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
        std::vector<double> snr, MacNodeId id)
{
    // Add enodeB to the number of antennas
    numRus++;
    // New Feedback
    LteFeedbackDoubleVector fbvv;
    fbvv.resize(numRus);
    // Resize all the vectors
    for (int i = 0; i < numRus; i++)
        fbvv[i].resize(DL_NUM_TXMODE);
    // For each Remote
    for (int j = 0; j < numRus; j++) {
        LteFeedback fb;
        // For each txmode we generate a feedback excluding MU_MIMO because it is treated as SISO
        for (int z = 0; z < DL_NUM_TXMODE - 1; z++) {
            // Reset the feedback object
            fb.reset();
            fb.setTxMode((TxMode)z);
            unsigned int rank = 1;
            if (z == OL_SPATIAL_MULTIPLEXING)
                rank = computeRank(id);
            if ((z == OL_SPATIAL_MULTIPLEXING && rank > 1) || z == TRANSMIT_DIVERSITY || z == SINGLE_ANTENNA_PORT0) {
                // Set the rank
                fb.setRankIndicator(rank);
                // Set the remote
                fb.setAntenna((Remote)j);
                // Set the PMI
                fb.setWideBandPmi(intuniform(getEnvir()->getRNG(0), 1, pow(rank, (double)2)));
                // Generate feedback for txmode z
                generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws[(Remote)j], rbAllocationType,
                        (TxMode)z, snr);
            }
            // Add the feedback to the feedback structure
            LteFeedback fb2 = fb;
            if (z == SINGLE_ANTENNA_PORT0) {
                fb2.setTxMode(MULTI_USER);
                fbvv[j][MULTI_USER] = fb2;
            }
            fbvv[j][z] = fb;
        }
    }
    return fbvv;
}

LteFeedbackVector LteFeedbackComputationRealistic::computeFeedback(const Remote remote, FeedbackType fbType,
        RbAllocationType rbAllocationType, TxMode currentTxMode,
        int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
        std::vector<double> snr, MacNodeId id)
{
    // New Feedback
    LteFeedbackVector fbv;
    // Resize
    fbv.resize(DL_NUM_TXMODE);
    LteFeedback fb;
    // For each txmode we generate a feedback
    for (int z = 0; z < DL_NUM_TXMODE; z++) {
        fb.reset();
        fb.setTxMode((TxMode)z);
        unsigned int rank = 1;
        if (z == OL_SPATIAL_MULTIPLEXING)
            rank = computeRank(id);
        if ((z == OL_SPATIAL_MULTIPLEXING && rank > 1) || z == TRANSMIT_DIVERSITY || z == SINGLE_ANTENNA_PORT0) {
            // Set the rank
            fb.setRankIndicator(rank);
            // Set the remote
            fb.setAntenna(remote);
            // Set the PMI
            fb.setWideBandPmi(intuniform(getEnvir()->getRNG(0), 1, pow(rank, (double)2)));
            // Generate feedback for txmode z
            generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws, rbAllocationType, (TxMode)z,
                    snr);
        }
        // Add the feedback to the feedback structure
        LteFeedback fb2 = fb;
        if (z == SINGLE_ANTENNA_PORT0) {
            fb2.setTxMode(MULTI_USER);
            fbv[MULTI_USER] = fb2;
        }
        fbv[z] = fb;
    }
    return fbv;
}

LteFeedback LteFeedbackComputationRealistic::computeFeedback(const Remote remote, TxMode txmode, FeedbackType fbType,
        RbAllocationType rbAllocationType,
        int antennaCws, int numPreferredBands, FeedbackGeneratorType feedbackGeneratortype, int numRus,
        std::vector<double> snr, MacNodeId id)
{
    // New Feedback
    LteFeedback fb;
    unsigned int rank = 1;
    // Set the rank for all the tx mode except for SISO and Tx diversity
    if (txmode == OL_SPATIAL_MULTIPLEXING || txmode == CL_SPATIAL_MULTIPLEXING || txmode == MULTI_USER) {
        // Compute Rank Index
        rank = computeRank(id);
        if (rank < 2 && (txmode == OL_SPATIAL_MULTIPLEXING || txmode == CL_SPATIAL_MULTIPLEXING))
            return fb;
        // Set Rank
        fb.setRankIndicator(rank);
    }
    else
        fb.setRankIndicator(rank);
    // Set the remote in the feedback object
    fb.setAntenna(remote);
    fb.setTxMode(txmode);
    generateBaseFeedback(numBands_, numPreferredBands, fb, fbType, antennaCws, rbAllocationType, txmode, snr);
    // Set PMI only for CL SMUX and MUMIMO
    if (txmode == CL_SPATIAL_MULTIPLEXING || txmode == MULTI_USER) {
        // Set PMI
        fb.setWideBandPmi(intuniform(getEnvir()->getRNG(0), 1, pow(rank, (double)2)));
    }
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

