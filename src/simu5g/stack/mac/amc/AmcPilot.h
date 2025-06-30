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

#ifndef _LTE_AMCPILOT_H_
#define _LTE_AMCPILOT_H_

#include "common/LteCommon.h"
#include "stack/mac/amc/LteAmc.h"
#include "stack/mac/amc/UserTxParams.h"
#include "stack/phy/feedback/LteFeedback.h"

namespace simu5g {

/// Forward declaration of LteAmc class, used by AmcPilot.
class LteAmc;

/**
 * @class AmcPilot
 * @brief Abstract AMC Pilot class
 *
 * This is the base class for all AMC pilots.
 * If you want to add a new AMC pilot, you have to subclass
 * from this class and implement the computeTxParams method
 * according to your policy.
 */
class AmcPilot
{
  protected:
    Binder *binder_ = nullptr;

    //! LteAmc owner module
    LteAmc *amc_ = nullptr;

    //! Pilot Name
    std::string name_;

    //! contains for each user, the subset of Bands that will be considered in the AMC operations
    UsableBandsList usableBandsList_;

    // specifies how the final CQI will be computed from the per band CQIs (e.g. AVG, MAX, MIN)
    PilotComputationModes mode_;

  public:

    /**
     * Constructor
     * @param amc LteAmc owner module
     */
    AmcPilot(Binder *binder, LteAmc *amc) : binder_(binder), amc_(amc), name_("NONE")
    {
    }

    /**
     * Destructor
     */
    virtual ~AmcPilot() {
    }

    /**
     * Assign logical bands for given nodeId and direction.
     * @param id The mobile node ID.
     * @param dir The link direction.
     * @return The user transmission parameters computed.
     */
    virtual const UserTxParams& computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency) = 0;

    /**
     * Function to get the AMC Pilot name.
     * Useful for debug print.
     */
    std::string getName()
    {
        return name_;
    }

    virtual std::vector<Cqi> getMultiBandCqi(MacNodeId id, const Direction dir, double carrierFrequency) = 0;

    virtual void setUsableBands(MacNodeId id, UsableBands usableBands) = 0;
    virtual UsableBands *getUsableBands(MacNodeId id) = 0;

    void setMode(PilotComputationModes mode) { mode_ = mode; }
};

} //namespace

#endif

