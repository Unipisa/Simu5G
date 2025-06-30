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

#ifndef _BACKGROUNDCELLAMCNR_H_
#define _BACKGROUNDCELLAMCNR_H_

#include <omnetpp.h>

#include "nodes/backgroundCell/BackgroundCellAmc.h"
#include "stack/mac/amc/NRMcs.h"

namespace simu5g {

class BackgroundCellAmcNr : public BackgroundCellAmc
{
  protected:

    NRMcsTable dlNrMcsTable_;
    NRMcsTable ulNrMcsTable_;
    NRMcsTable d2dNrMcsTable_;

    NRMCSelem getMcsElemPerCqi(Cqi cqi, const Direction dir);

    unsigned int getSymbolsPerSlot(double carrierFrequency, Direction dir);
    unsigned int getResourceElementsPerBlock(unsigned int symbolsPerSlot);
    unsigned int getResourceElements(unsigned int blocks, unsigned int symbolsPerSlot);
    unsigned int computeTbsFromNinfo(double nInfo, double coderate);

  public:
    BackgroundCellAmcNr(Binder *binder);

    unsigned int computeBitsPerRbBackground(Cqi cqi, const Direction dir, double carrierFrequency) override;
};

} //namespace

#endif

