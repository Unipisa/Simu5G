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

#ifndef _LTE_AMCPILOTD2D_H_
#define _LTE_AMCPILOTD2D_H_

#include "stack/mac/amc/AmcPilot.h"

namespace simu5g {

/**
 * @class AmcPilotD2D
 * @brief AMC pilot for D2D communication
 */
class AmcPilotD2D : public AmcPilot
{
    bool usePreconfiguredTxParams_ = false;
    UserTxParams *preconfiguredTxParams_ = nullptr;

  public:

    /**
     * Constructor
     * @param amc LteAmc owner module
     */
    AmcPilotD2D(Binder *binder, LteAmc *amc) :
        AmcPilot(binder, amc)
    {
        name_ = "D2D";
        mode_ = MIN_CQI;
    }

    /**
     * Assign logical bands for given nodeId and direction
     * @param id The mobile node ID.
     * @param dir The link direction.
     * @return The user transmission parameters computed.
     */
    const UserTxParams& computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency) override;

    void setPreconfiguredTxParams(Cqi cqi);

    // TODO reimplement these functions
    std::vector<Cqi> getMultiBandCqi(MacNodeId id, const Direction dir, double carrierFrequency) override { std::vector<Cqi> result; return result; }
    void setUsableBands(MacNodeId id, UsableBands usableBands) override {}
    UsableBands *getUsableBands(MacNodeId id) override { return nullptr; }
};

} //namespace

#endif

