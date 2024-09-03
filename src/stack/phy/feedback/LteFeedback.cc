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

#include <iostream>
#include "stack/phy/feedback/LteFeedback.h"

namespace simu5g {

using namespace omnetpp;


void LteFeedback::reset() {
    wideBandCqi_.clear();
    perBandCqi_.clear();
    preferredCqi_.clear();
    preferredBands_.clear();

    status_ = EMPTY;
    periodicFeedback_ = true;
    txMode_ = SINGLE_ANTENNA_PORT0;
    remoteAntennaId_ = MACRO;
}

void LteFeedback::print(MacCellId cellId, MacNodeId nodeId, Direction dir,
        const char *s) const {
    EV << NOW << " " << s << "         LteFeedback\n";
    EV << NOW << " " << s << " CellId: " << cellId << "\n";
    EV << NOW << " " << s << " NodeId: " << nodeId << "\n";
    EV << NOW << " " << s << " Antenna: " << dasToA(getAntennaId()) << "\n"; // XXX Generous
    EV << NOW << " " << s << " Direction: " << dirToA(dir) << "\n";
    EV << NOW << " " << s << " -------------------------\n";
    EV << NOW << " " << s << " TxMode: " << txModeToA(getTxMode()) << "\n";
    EV << NOW << " " << s << " Type: "
       << (isPeriodicFeedback() ? "PERIODIC" : "APERIODIC") << "\n";
    EV << NOW << " " << s << " -------------------------\n";

    if (isEmptyFeedback()) {
        EV << NOW << " " << s << " EMPTY!\n";
    }

    if (hasRankIndicator())
        EV << NOW << " " << s << " RI = " << getRankIndicator() << "\n";

    if (hasPreferredCqi()) {
        CqiVector cqi = getPreferredCqi();
        unsigned int codewords = cqi.size();
        for (Codeword cw = 0; cw < codewords; ++cw)
            EV << NOW << " " << s << " Preferred CQI[" << cw << "] = "
               << cqi.at(cw) << "\n";
    }

    if (hasWbCqi()) {
        CqiVector cqi = getWbCqi();
        unsigned int codewords = cqi.size();
        for (Codeword cw = 0; cw < codewords; ++cw)
            EV << NOW << " " << s << " Wideband CQI[" << cw << "] = "
               << cqi.at(cw) << "\n";
    }

    if (hasBandCqi()) {
        std::vector<CqiVector> cqi = getBandCqi();
        unsigned int codewords = cqi.size();
        for (Codeword cw = 0; cw < codewords; ++cw) {
            EV << NOW << " " << s << " Band CQI[" << cw << "] = {";
            unsigned int bands = cqi[cw].size();
            if (bands > 0) {
                EV << cqi[cw].at(0);
                for (Band b = 1; b < bands; ++b)
                    EV << ", " << cqi[cw].at(b);
            }
            EV << "}\n";
        }
    }

    if (hasPreferredPmi()) {
        Pmi pmi = getPreferredPmi();
        EV << NOW << " " << s << " Preferred PMI = " << pmi << "\n";
    }

    if (hasWbPmi()) {
        Pmi pmi = getWbPmi();
        EV << NOW << " " << s << " Wideband PMI = " << pmi << "\n";
    }

    if (hasBandCqi()) {
        PmiVector pmi = getBandPmi();
        EV << NOW << " " << s << " Band PMI = {";
        unsigned int bands = pmi.size();
        if (bands > 0) {
            EV << pmi.at(0);
            for (Band b = 1; b < bands; ++b)
                EV << ", " << pmi.at(b);
        }
        EV << "}\n";
    }

    if (hasPreferredCqi() || hasPreferredPmi()) {
        BandSet band = getPreferredBands();
        EV << NOW << " " << s << " Preferred Bands = {";
        const char *sep = "";
        for (const auto& b : band) {
            EV << sep << b;
            sep = ", ";
        }
        EV << "}\n";
    }
}

void LteMuMimoMatrix::print(const char *s) const {
    EV << NOW << " " << s << " ################" << endl;
    EV << NOW << " " << s << " LteMuMimoMatrix" << endl;
    EV << NOW << " " << s << " ################" << endl;
    for (unsigned int i = num(UE_MIN_ID); i < num(maxNodeId_); i++)
        EV << NOW << "" << i;
    EV << endl;
    for (unsigned int i = num(UE_MIN_ID); i < num(maxNodeId_); i++)
        EV << NOW << "" << muMatrix_[i];
}

} //namespace

