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

#ifndef _NRAMC_H_
#define _NRAMC_H_

#include "simu5g/common/LteDefs.h"
#include "simu5g/stack/mac/amc/LteAmc.h"
#include "simu5g/stack/mac/amc/NrMcs.h"

namespace simu5g {

/**
 * NR Adaptive Modulation and Coding (AMC) implementation.
 *
 * TBS determination based on 3GPP TS 38.214 v15.6.0 (June 2019)
 */
class NrAmc : public LteAmc
{
    unsigned int getSymbolsPerSlot(GHz carrierFrequency, Direction dir);
    unsigned int getResourceElementsPerBlock(unsigned int symbolsPerSlot);
    unsigned int getResourceElements(unsigned int blocks, unsigned int symbolsPerSlot);
    unsigned int computeTbsFromNinfo(double nInfo, double coderate);

    unsigned int computeCodewordTbs(UserTxParams *info, Codeword cw, Direction dir, unsigned int numRe);

  public:

    NrMcsTable dlNrMcsTable_;    // TODO tables for UL and DL should be different
    NrMcsTable ulNrMcsTable_;
    NrMcsTable d2dNrMcsTable_;

    NrAmc(LteMacEnb *mac, Binder *binder, CellInfo *cellInfo, int numAntennas);

    NrMcsElem getMcsElemPerCqi(Cqi cqi, const Direction dir);

    unsigned int computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, GHz carrierFrequency) override;
    unsigned int computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, GHz carrierFrequency) override;
    unsigned int computeBitsPerRbBackground(Cqi cqi, const Direction dir, GHz carrierFrequency) override;

};

} //namespace

#endif

