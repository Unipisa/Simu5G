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

#include <omnetpp.h>

#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/amc/NRMcs.h"

namespace simu5g {

/**
 * @class NRAMC
 * @brief NR AMC module for Omnet++ simulator
 *
 * TBS determination based on 3GPP TS 38.214 v15.6.0 (June 2019)
 */
class NRAmc : public LteAmc
{
    unsigned int getSymbolsPerSlot(double carrierFrequency, Direction dir);
    unsigned int getResourceElementsPerBlock(unsigned int symbolsPerSlot);
    unsigned int getResourceElements(unsigned int blocks, unsigned int symbolsPerSlot);
    unsigned int computeTbsFromNinfo(double nInfo, double coderate);

    unsigned int computeCodewordTbs(UserTxParams *info, Codeword cw, Direction dir, unsigned int numRe);

  public:

    NRMcsTable dlNrMcsTable_;    // TODO tables for UL and DL should be different
    NRMcsTable ulNrMcsTable_;
    NRMcsTable d2dNrMcsTable_;

    NRAmc(LteMacEnb *mac, Binder *binder, CellInfo *cellInfo, int numAntennas);

    NRMCSelem getMcsElemPerCqi(Cqi cqi, const Direction dir);

    unsigned int computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency) override;
    unsigned int computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency) override;
    unsigned int computeBitsPerRbBackground(Cqi cqi, const Direction dir, double carrierFrequency) override;

};

} //namespace

#endif

