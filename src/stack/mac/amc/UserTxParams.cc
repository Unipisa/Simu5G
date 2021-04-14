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

#include <omnetpp.h>

#include "stack/mac/amc/UserTxParams.h"

using namespace omnetpp;

void UserTxParams::print(const char* s) const {
    try {
        EV << NOW << " " << s << " --------------------------\n";
        EV << NOW << " " << s << "           UserTxParams\n";
        EV << NOW << " " << s << " --------------------------\n";
        EV << NOW << " " << s << " TxMode: " << txModeToA(txMode_) << "\n";
        EV << NOW << " " << s << " RI: " << ri_ << "\n";

        //*** CQIs *********************************************
        unsigned int codewords = cqiVector_.size();
        EV << NOW << " " << s << " CQI = {";
        if(codewords > 0)
        {
            EV << cqiVector_.at(0);
            for(Codeword cw = 1; cw < codewords; ++cw)
            EV << ", " << cqiVector_.at(cw);
        }
        EV << "}\n";
        //******************************************************

        EV << NOW << " " << s << " PMI: " << pmi_ << "\n";

        //*** Bands ********************************************
        std::set<Band>::const_iterator it = allowedBands_.begin();
        std::set<Band>::const_iterator et = allowedBands_.end();

        EV << NOW << " " << s << " Bands = {";
        if(it != et)
        {
            EV << *it;
            ++it;
            for(; it != et; ++it)
            EV << ", " << *it;
        }
        EV << "}\n";
        //******************************************************

        EV << NOW << " " << s << " --------------------------\n";
    }
    catch(std::exception& e)
    {
        throw omnetpp::cRuntimeError("Exception in UserTxParams::print(): %s", e.what());
    }
}
