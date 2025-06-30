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

#include "common/LteCommon.h"

#include "stack/phy/feedback/LteSummaryFeedback.h"

namespace simu5g {

using namespace omnetpp;

double LteSummaryFeedback::confidence(simtime_t creationTime) const {
    simtime_t delta = simTime() - creationTime;

    if (delta < confidenceLowerBound_)
        return 1.0;
    if (delta > confidenceUpperBound_)
        return 0.0;
    return 1.0
           - (delta - confidenceLowerBound_)
           / (confidenceUpperBound_ - confidenceLowerBound_);
}

void LteSummaryFeedback::reset() {
    ri_ = NORANK;
    tRi_ = simTime();

    cqi_ = std::vector<CqiVector>(totCodewords_,
            CqiVector(logicalBandsTot_, NOSIGNALCQI));
    tCqi_ = std::vector<std::vector<simtime_t>>(totCodewords_,
            std::vector<simtime_t>(logicalBandsTot_, simTime()));

    pmi_ = PmiVector(logicalBandsTot_, NOPMI);
    tPmi_ = std::vector<simtime_t>(logicalBandsTot_, simTime());
    valid_ = false;
}

void LteSummaryFeedback::print(MacCellId cellId, MacNodeId nodeId,
        const Direction dir, TxMode txm, const char *s) const {
    bool debug = false;
    if (debug) {
        EV << NOW << " " << s << "     LteSummaryFeedback\n";
        EV << NOW << " " << s << " CellId: " << cellId << "\n";
        EV << NOW << " " << s << " NodeId: " << nodeId << "\n";
        EV << NOW << " " << s << " Direction: " << dirToA(dir) << "\n";
        EV << NOW << " " << s << " TxMode: " << txModeToA(txm) << "\n";
        EV << NOW << " " << s << " -------------------------\n";

        Rank ri = getRi();
        double c = getRiConfidence();
        EV << NOW << " " << s << " RI = " << ri << " [" << c << "]\n";

        unsigned char codewords = getTotCodewords();
        unsigned char bands = getTotLogicalBands();
        for (Codeword cw = 0; cw < codewords; ++cw) {
            EV << NOW << " " << s << " CQI[" << cw << "] = {";
            if (bands > 0) {
                EV << getCqi(cw, 0);
                for (Band b = 1; b < bands; ++b)
                    EV << ", " << getCqi(cw, b);
            }
            EV << "} [{";
            if (bands > 0) {
                c = getCqiConfidence(cw, 0);
                EV << c;
                for (Band b = 1; b < bands; ++b) {
                    c = getCqiConfidence(cw, b);
                    EV << ", " << c;
                }
            }
            EV << "}]\n";
        }

        EV << NOW << " " << s << " PMI = {";
        if (bands > 0) {
            EV << getPmi(0);
            for (Band b = 1; b < bands; ++b)
                EV << ", " << getPmi(b);
        }
        EV << "} [{";
        if (bands > 0) {
            c = getPmiConfidence(0);
            EV << c;
            for (Band b = 1; b < bands; ++b) {
                c = getPmiConfidence(b);
                EV << ", " << c;
            }
        }
        EV << "}]\n";
    }
}

} //namespace

