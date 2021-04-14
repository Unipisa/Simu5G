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

    unsigned int computeCodewordTbs(UserTxParams* info, Codeword cw, Direction dir, unsigned int numRe);

  public:

    NRMcsTable dlNrMcsTable_;    // TODO tables for UL and DL should be different
    NRMcsTable ulNrMcsTable_;
    NRMcsTable d2dNrMcsTable_;

    NRAmc(LteMacEnb *mac, Binder *binder, CellInfo *cellInfo, int numAntennas);
    virtual ~NRAmc();

    NRMCSelem getMcsElemPerCqi(Cqi cqi, const Direction dir);

    virtual unsigned int computeReqRbs(MacNodeId id, Band b, Codeword cw, unsigned int bytes, const Direction dir, double carrierFrequency);
    virtual unsigned int computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
    virtual unsigned int computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency);
//    virtual unsigned int computeBytesOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
//    virtual unsigned int computeBytesOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency);

//    // multiband version of the above function. It returns the number of bytes that can fit in the given "blocks" of the given "band"
//    virtual unsigned int computeBytesOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
//    virtual unsigned int computeBitsOnNRbs_MB(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency);
};

#endif
