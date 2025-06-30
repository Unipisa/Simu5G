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

#include "nodes/backgroundCell/BackgroundCellAmc.h"
#include "stack/mac/amc/LteMcs.h"

namespace simu5g {

using namespace omnetpp;

BackgroundCellAmc::BackgroundCellAmc(Binder *binder) : binder_(binder)
{
    calculateMcsScale();

    // Scale MCS Tables
    dlMcsTable_.rescale(mcsScaleDl_);
    ulMcsTable_.rescale(mcsScaleUl_);
    d2dMcsTable_.rescale(mcsScaleD2D_);
}

void BackgroundCellAmc::calculateMcsScale()
{
    // TODO make parameters configurable

    // RBsubcarriers * (TTISymbols - SignallingSymbols) - pilotREs
    int ulRbSubcarriers = 12;
    int dlRbSubCarriers = 12;
    int ulRbSymbols = 7;
    ulRbSymbols *= 2; // slot --> RB
    int dlRbSymbols = 7;
    dlRbSymbols *= 2; // slot --> RB
    int ulSigSymbols = 1;
    int dlSigSymbols = 1;
    int ulPilotRe = 0;
    int dlPilotRe = 3;

    mcsScaleUl_ = mcsScaleD2D_ = ulRbSubcarriers * (ulRbSymbols - ulSigSymbols) - ulPilotRe;
    mcsScaleDl_ = dlRbSubCarriers * (dlRbSymbols - dlSigSymbols) - dlPilotRe;
}

unsigned int BackgroundCellAmc::computeBitsPerRbBackground(Cqi cqi, const Direction dir, double carrierFrequency)
{
    // DEBUG
    EV << NOW << " BackgroundCellAmc::computeBitsPerRbBackground CQI: " << cqi << " Direction: " << dirToA(dir) << endl;

    // if CQI == 0 the UE is out of range, thus return 0
    if (cqi == 0) {
        EV << NOW << " BackgroundCellAmc::computeBitsPerRbBackground - CQI equal to zero, return no bytes available" << endl;
        return 0;
    }

    unsigned int iTbs = getItbsPerCqi(cqi, dir);
    LteMod mod = cqiTable[cqi].mod_;
    unsigned int i = (mod == _QPSK ? 0 : (mod == _16QAM ? 9 : (mod == _64QAM ? 15 : 0)));

    EV << NOW << " BackgroundCellAmc::computeBitsPerRbBackground Modulation: " << modToA(mod) << " - iTbs: " << iTbs << " i: " << i << endl;

    unsigned char layers = 1;
    const unsigned int *tbsVect = itbs2tbs(mod, TRANSMIT_DIVERSITY, layers, iTbs - i);
    unsigned int blocks = 1;

    EV << NOW << " BackgroundCellAmc::computeBitsPerRbBackground Available space: " << tbsVect[blocks - 1] << "\n";
    return tbsVect[blocks - 1];
}

unsigned int BackgroundCellAmc::getItbsPerCqi(Cqi cqi, const Direction dir)
{
    // CQI threshold table selection
    McsTable *mcsTable;
    if (dir == DL)
        mcsTable = &dlMcsTable_;
    else if ((dir == UL) || (dir == D2D) || (dir == D2D_MULTI))
        mcsTable = &ulMcsTable_;
    else
        throw cRuntimeError("BackgroundCellAmc::getItbsPerCqi(): Unrecognized direction");

    CQIelem entry = cqiTable[cqi];
    LteMod mod = entry.mod_;
    double rate = entry.rate_;

    // Select the ranges for searching in the MCS table.
    unsigned int min = 0; // _QPSK
    unsigned int max = 9; // _QPSK
    if (mod == _16QAM) {
        min = 10;
        max = 16;
    }
    if (mod == _64QAM) {
        min = 17;
        max = 28;
    }

    // Initialize the working variables at the minimum value.
    MCSelem elem = mcsTable->at(min);
    unsigned int iTbs = elem.iTbs_;

    // Search in the MCS table from min to max until the rate exceeds
    // the threshold in an entry of the table.
    for (unsigned int i = min; i <= max; i++) {
        elem = mcsTable->at(i);
        if (elem.threshold_ <= rate)
            iTbs = elem.iTbs_;
        else
            break;
    }

    // Return the iTbs found.
    return iTbs;
}

} //namespace

