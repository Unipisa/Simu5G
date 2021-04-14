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
        if (fb.hasBandCqi()) // Per-band
        {
            std::vector<CqiVector> cqi = fb.getBandCqi();
            unsigned int n = cqi.size();
            for (Codeword cw = 0; cw < n; ++cw)
                for (Band i = 0; i < totBands_; ++i)
                    cumulativeSummary_.setCqi(cqi.at(cw).at(i), cw, i);
        } else {
            if (fb.hasWbCqi()) // Wide-band
            {
                CqiVector cqi(fb.getWbCqi());
                unsigned int n = cqi.size();
                for (Codeword cw = 0; cw < n; ++cw)
                    for (Band i = 0; i < totBands_; ++i)
                        cumulativeSummary_.setCqi(cqi.at(cw), cw, i); // ripete lo stesso wb cqi su ogni banda della stessa cw
            }
            if (fb.hasPreferredCqi()) // Preferred-band
            {
                CqiVector cqi(fb.getPreferredCqi());
                BandSet bands(fb.getPreferredBands());
                unsigned int n = cqi.size();
                BandSet::iterator et = bands.end();
                for (Codeword cw = 0; cw < n; ++cw)
                    for (BandSet::iterator it = bands.begin(); it != et; ++it)
                        cumulativeSummary_.setCqi(cqi.at(cw), cw, *it); // mette lo stesso cqi solo sulle bande preferite della stessa cw
            }
        }

        // Per il PMI si comporta in modo analogo

        // PMI
        if (fb.hasBandPmi()) // Per-band
        {
            PmiVector pmi(fb.getBandPmi());
            for (Band i = 0; i < totBands_; ++i)
                cumulativeSummary_.setPmi(pmi.at(i), i);
        } else {
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
                BandSet::iterator et = bands.end();
                for (BandSet::iterator it = bands.begin(); it != et; ++it)
                    cumulativeSummary_.setPmi(pmi, *it);
            }
        }
    } catch (std::exception& e) {
        throw omnetpp::cRuntimeError("Exception in LteSummaryBuffer::summarize(): %s",
                e.what());
    }
}


