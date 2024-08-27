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

#include "stack/phy/feedback/LteSummaryBuffer.h"

namespace simu5g {

using namespace omnetpp;

void LteSummaryBuffer::createSummary(LteFeedback fb) {
    try {
        // RI
        if (fb.hasRankIndicator()) {
            Rank ri(fb.getRankIndicator());
            cumulativeSummary_.setRi(ri);
            if (ri > 1)
                totCodewords_ = 2;
        }

        // CQI
        if (fb.hasBandCqi()) { // Per-band
            std::vector<CqiVector> cqi = fb.getBandCqi();
            for (Codeword cw = 0; cw < cqi.size(); ++cw)
                for (Band i = 0; i < totBands_; ++i)
                    cumulativeSummary_.setCqi(cqi.at(cw).at(i), cw, i);
        }
        else {
            if (fb.hasWbCqi()) { // Wide-band
                CqiVector cqi(fb.getWbCqi());
                for (Codeword cw = 0; cw < cqi.size(); ++cw)
                    for (Band i = 0; i < totBands_; ++i)
                        cumulativeSummary_.setCqi(cqi.at(cw), cw, i); // repeats the same wb cqi on each band of the same cw
            }
            if (fb.hasPreferredCqi()) { // Preferred-band
                CqiVector cqi(fb.getPreferredCqi());
                BandSet bands(fb.getPreferredBands());
                for (Codeword cw = 0; cw < cqi.size(); ++cw)
                    for (const auto& band : bands)
                        cumulativeSummary_.setCqi(cqi.at(cw), cw, band); // puts the same cqi only on the preferred bands of the same cw
            }
        }

        // For the PMI it behaves similarly

        // PMI
        if (fb.hasBandPmi()) { // Per-band
            PmiVector pmi(fb.getBandPmi());
            for (Band i = 0; i < totBands_; ++i)
                cumulativeSummary_.setPmi(pmi.at(i), i);
        }
        else {
            if (fb.hasWbPmi()) {
                // Wide-band
                Pmi pmi(fb.getWbPmi());
                for (Band i = 0; i < totBands_; ++i)
                    cumulativeSummary_.setPmi(pmi, i);
            }
            if (fb.hasPreferredPmi()) {
                // Preferred-band
                Pmi pmi(fb.getPreferredPmi());
                BandSet bands(fb.getPreferredBands());
                for (const auto& band : bands)
                    cumulativeSummary_.setPmi(pmi, band);
            }
        }
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in LteSummaryBuffer::summarize(): %s",
                e.what());
    }
}

} //namespace

