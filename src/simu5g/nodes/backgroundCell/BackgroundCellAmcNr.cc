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

#include "nodes/backgroundCell/BackgroundCellAmcNr.h"
#include "common/binder/Binder.h"

namespace simu5g {

using namespace omnetpp;

BackgroundCellAmcNr::BackgroundCellAmcNr(Binder *binder) : BackgroundCellAmc(binder)
{
}

unsigned int BackgroundCellAmcNr::computeBitsPerRbBackground(Cqi cqi, const Direction dir, double carrierFrequency)
{
    // DEBUG
    EV << NOW << " BackgroundCellAmcNr::computeBitsPerRbBackground CQI: " << cqi << " Direction: " << dirToA(dir) << " carrierFrequency: " << carrierFrequency << endl;

    // if CQI == 0 the UE is out of range, thus return 0
    if (cqi == 0) {
        EV << NOW << " BackgroundCellAmcNr::computeBitsPerRbBackground - CQI equal to zero, return no bytes available" << endl;
        return 0;
    }

    unsigned int blocks = 1;
    unsigned char layers = 1;

    // compute TBS

    NRMCSelem mcsElem = getMcsElemPerCqi(cqi, dir);
    unsigned int numRe = getResourceElements(blocks, getSymbolsPerSlot(carrierFrequency, dir));
    unsigned int modFactor;
    switch (mcsElem.mod_) {
        case _QPSK:   modFactor = 2;
            break;
        case _16QAM:  modFactor = 4;
            break;
        case _64QAM:  modFactor = 6;
            break;
        case _256QAM: modFactor = 8;
            break;
        default: throw cRuntimeError("NRAmc::computeCodewordTbs - unrecognized modulation.");
    }
    double coderate = mcsElem.coderate_ / 1024;
    double nInfo = numRe * coderate * modFactor * layers;

    unsigned int tbs = computeTbsFromNinfo(floor(nInfo), coderate);

    EV << NOW << " BackgroundCellAmcNr::computeBitsPerRbBackground Available space: " << tbs << "\n";
    return tbs;
}

NRMCSelem BackgroundCellAmcNr::getMcsElemPerCqi(Cqi cqi, const Direction dir)
{
    // CQI threshold table selection
    NRMcsTable *mcsTable;
    if (dir == DL)
        mcsTable = &dlNrMcsTable_;
    else if ((dir == UL) || (dir == D2D) || (dir == D2D_MULTI))
        mcsTable = &ulNrMcsTable_;
    else
        throw cRuntimeError("BackgroundCellAmcNr::getIMcsPerCqi(): Unrecognized direction");

    CQIelem entry = mcsTable->getCqiElem(cqi);
    LteMod mod = entry.mod_;
    double rate = entry.rate_;

    // Select the ranges for searching in the McsTable (extended reporting supported)
    unsigned int min = mcsTable->getMinIndex(mod);
    unsigned int max = mcsTable->getMaxIndex(mod);

    // Initialize the working variables at the minimum value.
    NRMCSelem ret = mcsTable->at(min);

    // Search in the McsTable from min to max until the rate exceeds
    // the coderate in an entry of the table.
    for (unsigned int i = min; i <= max; i++) {
        NRMCSelem elem = mcsTable->at(i);
        if (elem.coderate_ <= rate)
            ret = elem;
        else
            break;
    }

    // Return the MCSElem found.
    return ret;
}

unsigned int BackgroundCellAmcNr::getSymbolsPerSlot(double carrierFrequency, Direction dir)
{
    unsigned int totSymbols = 14;   // TODO get this parameter from CellInfo/Carrier

    SlotFormat sf = binder_->getSlotFormat(carrierFrequency);
    if (!sf.tdd)
        return totSymbols;

    // TODO handle FLEX symbols: so far, they are used as guard (hence, not used for scheduling)
    if (dir == DL)
        return sf.numDlSymbols;
    // else UL
    return sf.numUlSymbols;
}

unsigned int BackgroundCellAmcNr::getResourceElementsPerBlock(unsigned int symbolsPerSlot)
{
    unsigned int numSubcarriers = 12;   // TODO get this parameter from CellInfo/Carrier
    unsigned int reSignal = 1;
    unsigned int nOverhead = 0;

    return (numSubcarriers * symbolsPerSlot) - reSignal - nOverhead;
}

unsigned int BackgroundCellAmcNr::getResourceElements(unsigned int blocks, unsigned int symbolsPerSlot)
{
    unsigned int numRePerBlock = getResourceElementsPerBlock(symbolsPerSlot);

    if (numRePerBlock > 156)
        return 156 * blocks;

    return numRePerBlock * blocks;
}

unsigned int BackgroundCellAmcNr::computeTbsFromNinfo(double nInfo, double coderate)
{
    unsigned int tbs = 0;
    unsigned int _nInfo = 0;
    unsigned int n = 0;
    if (nInfo <= 3824) {
        n = std::max((int)3, (int)(floor(log2(nInfo) - 6)));
        _nInfo = std::max((unsigned int)24, (unsigned int)((1 << n) * floor(nInfo / (1 << n))));

        // get tbs from table
        unsigned int j = 0;
        for (j = 0; j < TBSTABLESIZE - 1; j++) {
            if (nInfoToTbs[j] >= _nInfo)
                break;
        }

        tbs = nInfoToTbs[j];
    }
    else {
        unsigned int C;
        n = floor(log2(nInfo - 24) - 5);
        _nInfo = (1 << n) * round((nInfo - 24) / (1 << n));
        if (coderate <= 0.25) {
            C = ceil((_nInfo + 24) / 3816);
            tbs = 8 * C * ceil((_nInfo + 24) / (8 * C)) - 24;
        }
        else {
            if (_nInfo >= 8424) {
                C = ceil((_nInfo + 24) / 8424);
                tbs = 8 * C * ceil((_nInfo + 24) / (8 * C)) - 24;
            }
            else {
                tbs = 8 * ceil((_nInfo + 24) / 8) - 24;
            }
        }
    }
    return tbs;
}

} //namespace

