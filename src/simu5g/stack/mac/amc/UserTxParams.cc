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

#include "simu5g/common/LteDefs.h"
#include "simu5g/stack/mac/amc/UserTxParams.h"

namespace simu5g {

using namespace omnetpp;

void UserTxParams::print(const char *s) const {
    try {
        EV << NOW << " " << s << " --------------------------\n";
        EV << NOW << " " << s << "           UserTxParams\n";
        EV << NOW << " " << s << " --------------------------\n";
        EV << NOW << " " << s << " TxMode: " << txModeToA(txMode) << "\n";
        EV << NOW << " " << s << " RI: " << ri << "\n";

        //*** CQIs *********************************************
        {
            EV << NOW << " " << s << " CQI = {";
            const char *sep = "";
            for (const auto& item : cqiVector) {
                EV << sep << item;
                sep = ", ";
            }
            EV << "}\n";
        }

        //******************************************************
        EV << NOW << " " << s << " PMI: " << pmi << "\n";

        //*** Bands ********************************************
        {
            EV << NOW << " " << s << " Bands = {";
            const char *sep = "";
            for (const auto& item : allowedBands_) {
                EV << sep << item;
                sep = ", ";
            }
            EV << "}\n";
        }
        //******************************************************

        EV << NOW << " " << s << " --------------------------\n";
    }
    catch (std::exception& e) {
        throw cRuntimeError("Exception in UserTxParams::print(): %s", e.what());
    }
}

} //namespace

